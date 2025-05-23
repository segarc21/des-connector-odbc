// Copyright (c) 2012, 2024, Oracle and/or its affiliates.
// Modified in 2025 by Sergio Miguel Garcia Jimenez <segarc21@ucm.es>
// (see the next block comment below).
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2.0, as
// published by the Free Software Foundation.
//
// This program is designed to work with certain software (including
// but not limited to OpenSSL) that is licensed under separate terms, as
// designated in a particular file or component or in included license
// documentation. The authors of MySQL hereby grant you an additional
// permission to link the program and your derivative works with the
// separately licensed software that they have either included with
// the program or referenced in the documentation.
//
// Without limiting anything contained in the foregoing, this file,
// which is part of Connector/ODBC, is also subject to the
// Universal FOSS Exception, version 1.0, a copy of which can be found at
// https://oss.oracle.com/licenses/universal-foss-exception.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License, version 2.0, for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

// ---------------------------------------------------------
// Modified in 2025 by Sergio Miguel Garcia Jimenez <segarc21@ucm.es>,
// hereinafter the DESODBC developer, in the context of the GPLv2 derivate
// work DESODBC, an ODBC Driver of the open-source DBMS Datalog Educational
// System (DES) (see https://des.sourceforge.io/)
// ---------------------------------------------------------

/**
  @file  ssps.c
  @brief Functions to support use of Server Side Prepared Statements.
*/

#include <algorithm>
#include "driver.h"
#include "errmsg.h"


void free_result_bind(STMT *stmt) {
  if (stmt->result_bind != NULL) {
    auto field_cnt = stmt->field_count();

    /* buffer was allocated for each column */
    for (size_t i = 0; i < field_cnt; i++) {
      x_free(stmt->result_bind[i].buffer);

      if (stmt->lengths) {
        stmt->lengths[i] = 0;
      }
    }

    x_free(stmt->result_bind);
    stmt->result_bind = 0;
    stmt->array.reset();
  }
}

char *STMT::extend_buffer(char *to, size_t len) {
  return tempbuf.extend_buffer(to, len);
}

char *STMT::extend_buffer(size_t len) { return tempbuf.extend_buffer(len); }

char *STMT::add_to_buffer(const char *from, size_t len) {
  return tempbuf.add_to_buffer(from, len);
}

void STMT::free_lengths() { lengths.reset(); }

void STMT::alloc_lengths(size_t num) {
  lengths.reset(new unsigned long[num]());
}

void STMT::reset_setpos_apd() { setpos_apd.reset(); }

void STMT::free_unbind() { ard->reset(); }

/*
  Do only a "light" reset
*/
void STMT::reset() {
  buf_set_pos(0);

  // If data existed before invalidating the result array does not need freeing
  if (m_row_storage.invalidate()) result_array.reset();
}

void STMT::free_reset_out_params() {
  out_params_state = OPS_UNKNOWN;
  apd->free_paramdata();
  /* reset data-at-exec state */
  dae_type = 0;
  scroller.reset();
}

void STMT::free_reset_params() {
  /* remove all params and reset count to 0 (per spec) */
  /* http://msdn2.microsoft.com/en-us/library/ms709284.aspx */
  // NOTE: check if this breaks anything
  apd->records2.clear();
}

/* DESODBC:

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
void STMT::free_fake_result(bool clear_all_results) {
  if (!fake_result) {
    return;  // TODO: provisional solution.
  } else {
    reset_result_array();
    stmt_result_free(this);
  }
}

// Clear and free buffers bound in param_bind
void STMT::clear_param_bind() {
  for (auto bind : param_bind) {
    x_free(bind.buffer);
    bind.buffer = nullptr;
  }
  // No need to clear param_bind. It will be reused.
  // param_bind.clear();
}

// Reset result array in case when the row storage is not valid.
// The result data, which was not in the row storage must be cleared
// before filling it with the row storage data.
void STMT::reset_result_array() {
  if (this->result && this->result->internal_table)
    x_free(this->result->internal_table);
}

STMT::~STMT() {
  // Create a local mutex in the destructor.
  std::unique_lock<std::recursive_mutex> slock(lock);

  free_lengths();

  reset_setpos_apd();

  LOCK_DBC(dbc);
  dbc->stmt_list.remove(this);
  clear_param_bind();
}

void STMT::reset_getdata_position() {
  getdata.column = (uint)~0L;
  getdata.source = NULL;
  getdata.dst_bytes = (ulong)~0L;
  getdata.dst_offset = (ulong)~0L;
  getdata.src_offset = (ulong)~0L;
  getdata.latest_bytes = getdata.latest_used = 0;
}

long STMT::compute_cur_row(unsigned fFetchType, SQLLEN irow) {
  long cur_row = 0;
  long max_row = (long)num_rows(this);

  switch (fFetchType) {
    case SQL_FETCH_NEXT:
      cur_row = (current_row < 0 ? 0 : current_row + rows_found_in_set);
      break;
    case SQL_FETCH_PRIOR:
      cur_row = (current_row <= 0 ? -1 : (long)(current_row - ard->array_size));
      break;
    case SQL_FETCH_FIRST:
      cur_row = 0L;
      break;
    case SQL_FETCH_LAST:
      cur_row = max_row - (long)ard->array_size;
      break;
    case SQL_FETCH_ABSOLUTE:
      if (irow < 0) {
        /* Fetch from end of result set */
        if (max_row + irow < 0 && -irow <= (long)ard->array_size) {
          /*
            | FetchOffset | > LastResultRow AND
            | FetchOffset | <= RowsetSize
          */
          cur_row = 0; /* Return from beginning */
        } else
          cur_row = max_row + (long)irow; /* Ok if max_row <= -irow */
      } else
        cur_row = (long)irow - 1;
      break;

    case SQL_FETCH_RELATIVE:
      cur_row = current_row + (long)irow;
      if (current_row > 0 && cur_row < 0 &&
          (long)-irow <= (long)ard->array_size) {
        cur_row = 0;
      }
      break;

    case SQL_FETCH_BOOKMARK: {
      cur_row = (long)irow;
      if (cur_row < 0 && (-(long)irow) <= (long)ard->array_size) {
        cur_row = 0;
      }
    } break;

    default:
      set_error("HY106", "Fetch type out of range");
      throw error;
  }

  if (cur_row < 0) {
    current_row = -1; /* Before first row */
    rows_found_in_set = 0;
    data_seek(this, 0L);
    throw DESERROR(SQL_NO_DATA_FOUND);
  }
  if (cur_row > max_row) {
    cur_row = max_row;
  }

  if (!result_array && !if_forward_cache(this)) {
    /*
      If Dynamic, it loses the stmt->end_of_set, so
      seek to desired row, might have new data or
      might be deleted
    */
    if (stmt_options.cursor_type != SQL_CURSOR_DYNAMIC && cur_row &&
        cur_row == (long)(current_row + rows_found_in_set))
      row_seek(this, this->end_of_set);
    else
      data_seek(this, cur_row);
  }
  current_row = (long)cur_row;
  return current_row;
}