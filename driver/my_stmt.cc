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
  @file  my_stmt.c
  @brief Some "methods" for STMT "class" - functions that dispatch a call to either
         prepared statement version or to regular statement version(i.e. using mysql_stmt_*
         of mysql_* functions of API, depending of that has been used in that STMT.
         also contains "mysql_*" versions of "methods".
*/

#include "driver.h"

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/* Errors processing? */
BOOL returned_result(STMT *stmt)
{ return des_num_fields(stmt->result) > 0; }


/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
my_bool free_current_result(STMT *stmt)
{
  my_bool res= 0;
  if (stmt->result)
  {
    stmt_result_free(stmt);
    stmt->result= NULL;
  }
  return res;
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/* Name may be misleading, the idea is stmt - for directly executed statements,
   i.e using mysql_* part of api, ssps - prepared on server, using mysql_stmt
 */
static
DES_RESULT * stmt_get_result(STMT *stmt)
{
  return des_store_result(stmt);
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/* For text protocol this get result itself as well. Besides for text protocol
   we need to use/store each resultset of multiple resultsets */
DES_RESULT * get_result_metadata(STMT *stmt)
{
  /* just a precaution, mysql_free_result checks for NULL anywat */
  des_free_result(stmt->result);

  stmt->result = stmt_get_result(stmt);

  return stmt->result;
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
size_t STMT::field_count()
{
  return result && result->field_count > 0 ? result->field_count : 0;
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
my_ulonglong affected_rows(STMT *stmt)
{
  return stmt->affected_rows;
}

my_ulonglong update_affected_rows(STMT *stmt)
{
  my_ulonglong last_affected;

  last_affected= affected_rows(stmt);

  stmt->affected_rows+= last_affected;

  return last_affected;
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
my_ulonglong num_rows(STMT *stmt)
{
  return des_num_rows(stmt->result);
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
DES_ROW STMT::fetch_row(bool read_unbuffered)
{
    return des_fetch_row(result);
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
unsigned long* fetch_lengths(STMT *stmt)
{
  return des_fetch_lengths(stmt);
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
DES_ROW_OFFSET row_seek(STMT *stmt, DES_ROW_OFFSET offset)
{
  return des_row_seek(stmt->result, offset);
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
void data_seek(STMT *stmt, my_ulonglong offset)
{
  des_data_seek(stmt->result, offset);
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
DES_ROW_OFFSET row_tell(STMT *stmt)
{
    return des_row_tell(stmt->result);
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
int next_result(STMT *stmt)
{ return -1; //TODO: research
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/* --- Data conversion methods --- */
int get_int(STMT *stmt, ulong column_number, char *value, ulong length)
{
  return (int)strtol(value, NULL, 10);
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
unsigned int get_uint(STMT* stmt, ulong column_number, char* value, ulong length)
{
  return (unsigned int)strtoul(value, NULL, 10);
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
long long get_int64(STMT *stmt, ulong column_number, char *value, ulong length)
{
  return strtoll(value, NULL, 10);
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
unsigned long long get_uint64(STMT* stmt, ulong column_number, char* value, ulong length)
{
  return strtoull(value, NULL, 10);
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
char * get_string(STMT *stmt, ulong column_number, char *value, ulong *length,
                  char * buffer)
{
  return value;
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
double get_double(STMT *stmt, ulong column_number, char *value,
                  ulong length)
{
  return myodbc_strtod(value, length);
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
BOOL is_null(STMT *stmt, ulong column_number, char *value)
{
  return value == NULL;
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/* Prepares statement depending on connection option either on a client or
   on a server. Returns SQLRETURN result code since preparing on client or
   server can produce errors, memory allocation to name one.  */
SQLRETURN prepare(STMT *stmt, char * query, SQLINTEGER query_length,
          bool reset_sql_limit, bool force_prepare)
{
  assert(stmt);
  /* TODO: I guess we always have to have query length here */
  if (query_length <= 0)
  {
    query_length = query ? (SQLINTEGER)strlen(query) : 0;
  }

  stmt->query.reset(query, query + query_length,
                    stmt->dbc->cxn_charset_info);
  /* Tokenising string, detecting and storing parameters placeholders, removing {}
     So far the only possible error is memory allocation. Thus setting it here.
     If that changes we will need to make "parse" to set error and return rc */
  if (parse(&stmt->query))
  {
    return stmt->set_error("HY000", "Internal error parsing the query");
  }

  stmt->param_count = (uint)PARAM_COUNT(stmt->query);

  {
    /* Creating desc records for each parameter */
    uint i;
    for (i= 0; i < stmt->param_count; ++i)
    {
      DESCREC *aprec= desc_get_rec(stmt->apd, i, TRUE);
      DESCREC *iprec= desc_get_rec(stmt->ipd, i, TRUE);
    }
  }

  /* Reset current_param so that SQLParamData starts fresh. */
  stmt->current_param= 0;
  stmt->state= ST_PREPARED;

  return SQL_SUCCESS;
}
