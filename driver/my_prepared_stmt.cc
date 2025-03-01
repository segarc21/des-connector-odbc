// Copyright (c) 2012, 2024, Oracle and/or its affiliates.
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

/**
  @file  ssps.c
  @brief Functions to support use of Server Side Prepared Statements.
*/

#include <algorithm>
#include "driver.h"
#include "errmsg.h"

/* {{{ my_l_to_a() -I- */
static char *my_l_to_a(char *buf, size_t buf_size, long long a) {
  desodbc_snprintf(buf, buf_size, "%lld", (long long)a);
  return buf;
}
/* }}} */

/* {{{ my_ul_to_a() -I- */
static char *my_ul_to_a(char *buf, size_t buf_size, unsigned long long a) {
  desodbc_snprintf(buf, buf_size, "%llu", (unsigned long long)a);
  return buf;
}
/* }}} */

char *numeric2binary(char *dst, long long src, unsigned int byte_count) {
  char byte;

  while (byte_count) {
    byte = src & 0xff;
    *(dst + (--byte_count)) = byte;
    src = src >> 8;
  }

  return dst;
}

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

void ssps_close(STMT *stmt) {
  stmt->buf_set_pos(0);
}

bool is_varlen_type(enum enum_field_types type) {
  return (type == DES_TYPE_BLOB || type == DES_TYPE_TINY_BLOB ||
          type == DES_TYPE_MEDIUM_BLOB || type == DES_TYPE_LONG_BLOB ||
          type == DES_TYPE_VAR_STRING || type == DES_TYPE_JSON ||
          type == DES_TYPE_VECTOR);
}

/* The structure and following allocation function are borrowed from c/c++ and
 * adopted */
typedef struct tagBST {
  char *buffer = NULL;
  size_t size = 0;
  enum enum_field_types type;

  tagBST(char *b, size_t s, enum enum_field_types t)
      : buffer(b), size(s), type(t) {}

  /*
    To bind blob parameters the fake 1 byte allocation has to be made
    otherwise libmysqlclient throws the assertion.
    This will be reallocated later.
  */
  bool is_varlen_alloc() { return is_varlen_type(type); }
} st_buffer_size_type;

/* {{{ allocate_buffer_for_field() -I- */
static st_buffer_size_type allocate_buffer_for_field(
    const DES_FIELD *const field, BOOL outparams) {
  st_buffer_size_type result(NULL, 0, field->type);

  switch (field->type) {
    case DES_TYPE_NULL:
      break;

    case DES_TYPE_TINY:
      result.size = 1;
      break;

    case DES_TYPE_SHORT:
      result.size = 2;
      break;

    case DES_TYPE_INT24:
    case DES_TYPE_LONG:
      result.size = 4;
      break;

    /*
      For consistency with results obtained through DES_ROW
      we must fetch FLOAT and DOUBLE as character data
    */
    case DES_TYPE_DOUBLE:
    case DES_TYPE_FLOAT:
      result.size = 24;
      result.type = DES_TYPE_STRING;
      break;

    case DES_TYPE_LONGLONG:
      result.size = 8;
      break;

    case DES_TYPE_YEAR:
      result.size = 2;
      break;

    case DES_TYPE_TIMESTAMP:
    case DES_TYPE_DATE:
    case DES_TYPE_TIME:
    case DES_TYPE_DATETIME:
      result.size = sizeof(MYSQL_TIME);
      break;

    case DES_TYPE_TINY_BLOB:
    case DES_TYPE_MEDIUM_BLOB:
    case DES_TYPE_LONG_BLOB:
    case DES_TYPE_BLOB:
    case DES_TYPE_STRING:
    case DES_TYPE_VAR_STRING:
    case DES_TYPE_JSON:
    case DES_TYPE_VECTOR:
      /* We will get length with fetch and then fetch column */
      if (field->length > 0 && field->length < 1025)
        result.size = field->length + 1;
      else {
        // This is to keep mysqlclient library happy.
        // The buffer will be reallocated later.
        result.size = 1024;
      }
      break;

    case DES_TYPE_DECIMAL:
    case DES_TYPE_NEWDECIMAL:
      result.size = 64;
      break;

#if A1
    case DES_TYPE_TIMESTAMP:
    case DES_TYPE_YEAR:
      result.size = 10;
      break;
#endif
#if A0
    // There two are not sent over the wire
    case DES_TYPE_ENUM:
    case DES_TYPE_SET:
#endif
    case DES_TYPE_BIT:
      result.type = (enum_field_types)DES_TYPE_BIT;
      if (outparams) {
        /* For out params we surprisingly get it as string representation of a
           number representing those bits. Allocating buffer to accommodate
           largest string possible - 8 byte number + NULL terminator.
           We will need to terminate the string to convert it to a number */
        result.size = 30;
      } else {
        result.size = (field->length + 7) / 8;
      }

      break;
    case DES_TYPE_GEOMETRY:
    default:
      /* Error? */
      1;
  }

  if (result.size > 0) {
    result.buffer = (char *)desodbc_malloc(result.size, DESF(0));
  }

  return result;
}
/* }}} */

static DES_ROW fetch_varlength_columns(STMT *stmt, DES_ROW values) {
  const size_t num_fields = stmt->field_count();
  unsigned int i;
  uint desc_index = ~0L, stream_column = ~0L;

  // Only do something if values have not been fetched yet
  if (values) return values;

  if (stmt->out_params_state == OPS_STREAMS_PENDING) {
    desc_find_outstream_rec(stmt, &desc_index, &stream_column);
  }

  bool reallocated_buffers = false;
  for (i = 0; i < num_fields; ++i) {
    if (i == stream_column) {
      /* Skipping this column */
      desc_find_outstream_rec(stmt, &desc_index, &stream_column);
    } else {
      if (!(*stmt->result_bind[i].is_null) &&
          is_varlen_type(stmt->result_bind[i].buffer_type) &&
          stmt->result_bind[i].buffer_length < *stmt->result_bind[i].length) {
        /* TODO Realloc error proc */
        stmt->array[i] = (char *)myodbc_realloc(stmt->array[i],
                                                *stmt->result_bind[i].length);

        stmt->lengths[i] = *stmt->result_bind[i].length;
        stmt->result_bind[i].buffer_length = *stmt->result_bind[i].length;
        reallocated_buffers = true;
      }

      // Result bind buffer should already be freed by now.
      stmt->result_bind[i].buffer = stmt->array[i];

      if (stmt->lengths[i] > 0) {
        // For fixed-length types we should not set zero length
        stmt->result_bind[i].buffer_length = stmt->lengths[i];
      }

    }
  }

  fill_ird_data_lengths(stmt->ird, stmt->result_bind[0].length,
                        stmt->result->field_count);

  return stmt->array;
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

bool STMT::is_dynamic_cursor() {
  return stmt_options.cursor_type == SQL_CURSOR_DYNAMIC;
}

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

SQLRETURN STMT::set_error(desodbc_errid errid, const char *errtext) {
  error = DESERROR(errid, errtext);
  return error.retcode;
}

SQLRETURN STMT::set_error(desodbc_errid errid) {
  error = DESERROR(errid);
  return error.retcode;
}

SQLRETURN STMT::set_error(const char *state, const char *msg) {
  error = DESERROR(state, msg);
  return error.retcode;
}

SQLRETURN STMT::set_error(const char *state) {
  error = DESERROR(state, ""); //TODO: research what to do
  return error.retcode;
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
      set_error(DESERR_S1106, "Fetch type out of range");
      throw error;
  }

  if (cur_row < 0) {
    current_row = -1; /* Before first row */
    rows_found_in_set = 0;
    data_seek(this, 0L);
    throw DESERROR(SQL_NO_DATA_FOUND);
  }
  if (cur_row > max_row) {
    if (scroller_exists(this)) {
      while (cur_row > (max_row = (long)scroller_move(this)))
        ;

      switch (scroller_prefetch(this)) {
        case SQL_NO_DATA:
          throw DESERROR(SQL_NO_DATA_FOUND);
        case SQL_ERROR:
          set_error(DESERR_S1000);
          throw error;
      }
    } else {
      cur_row = max_row;
    }
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

bool bind_param(DES_BIND *bind, const char *value, unsigned long length,
                enum enum_field_types buffer_type);

void STMT::add_query_attr(const char *name, std::string val) {
  query_attr_names.emplace_back(name);
  size_t num = query_attr_names.size();
  // Consolidate the size of attribute names and param binds vectors.
  allocate_param_bind(num);

  DES_BIND *bind = &param_bind[num - 1];
  bind_param(bind, val.c_str(), val.length(), DES_TYPE_STRING);
}

bool STMT::query_attr_exists(const char *name) {
  if (m_ipd.rcount() == 0 || name == nullptr) return false;

  size_t len = strlen(name);
  for (auto &c : m_ipd.records2) {
    const char *v = c.par.val();
    if (v == nullptr || c.par.val_length() < len) continue;

    if (strncmp(name, v, len) == 0) return true;
  }
  return false;
}

SQLRETURN STMT::bind_query_attrs(bool use_ssps) {
  uint rcount = (uint)apd->rcount();
  if (rcount < param_count) {
    return set_error(DESERR_07001,
                     "The number of parameter markers is larger "
                     "than he number of parameters provided");
  } else if (!dbc->has_query_attrs) {
    return set_error(DESERR_01000,
                     "The server does not support query attributes");
  }

  uint num = param_count;

  // If anything is added to query_attr_names it means the parameter was added
  // as well. All other attributes go after it.
  uint param_idx = query_attr_names.size();

  allocate_param_bind(rcount + 1);

  while (num < rcount) {
    DESCREC *aprec = desc_get_rec(apd, num, false);
    DESCREC *iprec = desc_get_rec(ipd, num, false);

    /*
      IPREC and APREC should both be not NULL if query attributes
      or parameters are set.
    */
    if (!aprec || !iprec) return SQL_SUCCESS;  // Nothing to do

    DES_BIND *bind = &param_bind[param_idx];

    query_attr_names.emplace_back(iprec->par.val());

    // This will just fill the bind structure and do the param data conversion
    if (insert_param(this, bind, apd, aprec, iprec, 0) == SQL_ERROR) {
      return set_error("HY000",
                       "The number of attributes is larger than the "
                       "number of attribute values provided");
    }
    ++num;
    ++param_idx;
  }

  DES_BIND *bind = param_bind.data();
  const char **names = (const char **)query_attr_names.data();

  //TODO: implement
  /*
  if (mysql_bind_param(dbc->des, (unsigned int)query_attr_names.size(), bind,
                       names)) {
    set_error("HY000");
    // Clear only attr names. Params will be reused.
    clear_attr_names();
    return SQL_SUCCESS_WITH_INFO;
  }
  */
  return SQL_SUCCESS;
}

/* --------------- Type conversion functions -------------- */

#define ALLOC_IFNULL(buff, size) \
  ((buff) == NULL ? (char *)desodbc_malloc(size, DESF(0)) : buff)

template <typename T>
T binary2numeric(char *src, uint64 srcLen) {
  T dst = 0;

  while (srcLen) {
    /* if source binary data is longer than 8 bytes(size of long long)
       we consider only minor 8 bytes */
    if (srcLen > sizeof(T)) continue;
    dst += ((T)(0xff & *src)) << (--srcLen) * 8;
    ++src;
  }

  return dst;
}

long long binary2ll(char *src, uint64 srcLen) {
  return binary2numeric<long long>(src, srcLen);
}

unsigned long long binary2ull(char *src, uint64 srcLen) {
  return binary2numeric<unsigned long long>(src, srcLen);
}


/* }}} */

DES_BIND *get_param_bind(STMT *stmt, unsigned int param_number, int reset) {
  DES_BIND *bind = &stmt->param_bind[param_number];

  if (reset) {
    bind->is_null_value = 0;
    bind->is_unsigned = 0;

    /* as far as looked - this trick is safe */
    bind->is_null = &bind->is_null_value;
    bind->length = &bind->length_value;
  }

  return bind;
}