// Copyright (c) 2007, 2024, Oracle and/or its affiliates.
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
  @file  unicode.c
  @brief Entry points for Unicode versions of ODBC functions
*/

#include "driver.h"
#include <sqlucode.h>
#include <stdarg.h>


#define NOT_IMPLEMENTED \
  return SQL_ERROR


/* Forward declarations. */
SQLRETURN SQL_API
SQLColAttributeWImpl(SQLHSTMT hstmt, SQLUSMALLINT column,
                     SQLUSMALLINT field, SQLPOINTER char_attr,
                     SQLSMALLINT char_attr_max, SQLSMALLINT *char_attr_len,
                     SQLLEN *num_attr);
SQLRETURN SQL_API
SQLGetConnectAttrWImpl(SQLHDBC hdbc, SQLINTEGER attribute, SQLPOINTER value,
                       SQLINTEGER value_max, SQLINTEGER *value_len);
SQLRETURN SQL_API
SQLGetDiagRecWImpl(SQLSMALLINT handle_type, SQLHANDLE handle,
                   SQLSMALLINT record, SQLWCHAR *sqlstate,
                   SQLINTEGER *native_error, SQLWCHAR *message,
                   SQLSMALLINT message_max, SQLSMALLINT *message_len);
SQLRETURN SQL_API
SQLPrepareWImpl(SQLHSTMT hstmt, SQLWCHAR *str, SQLINTEGER str_len,
                bool force_prepare);

SQLRETURN SQL_API
SQLSetConnectAttrWImpl(SQLHDBC hdbc, SQLINTEGER attribute,
                       SQLPOINTER value, SQLINTEGER value_len);


SQLRETURN SQL_API
SQLColAttributeW(SQLHSTMT hstmt, SQLUSMALLINT column,
                 SQLUSMALLINT field, SQLPOINTER char_attr,
                 SQLSMALLINT char_attr_max, SQLSMALLINT *char_attr_len,
#ifdef USE_SQLCOLATTRIBUTE_SQLLEN_PTR
                 SQLLEN *num_attr
#else
                 SQLPOINTER num_attr
#endif
               )
{
  LOCK_STMT(hstmt);

  return SQLColAttributeWImpl(hstmt, column, field, char_attr, char_attr_max,
                              char_attr_len, num_attr);
}


SQLRETURN SQL_API
SQLColAttributeWImpl(SQLHSTMT hstmt, SQLUSMALLINT column,
                     SQLUSMALLINT field, SQLPOINTER char_attr,
                     SQLSMALLINT char_attr_max, SQLSMALLINT *char_attr_len,
                     SQLLEN *num_attr)
{
  STMT *stmt= (STMT *)hstmt;
  SQLCHAR *value= NULL;
  SQLWCHAR *wvalue;
  SQLINTEGER len= SQL_NTS;
  uint errors;
  SQLRETURN rc= DESColAttribute(hstmt, column, field, &value, num_attr);

  if (value)
  {
    des_bool free_value= FALSE;
    wvalue= sqlchar_as_sqlwchar(stmt->dbc->cxn_charset_info, value,
                                &len, &errors);

    /* char_attr_max is in bytes, we want it in chars. */
    char_attr_max/= sizeof(SQLWCHAR);

    /* We set the error only when the result is intented to be returned */
    if ((char_attr || num_attr) && len > char_attr_max - 1)
      rc= stmt->set_error( DESERR_01004, NULL, 0);

    if (char_attr_len)
      *char_attr_len= (SQLSMALLINT)len * sizeof(SQLWCHAR);

    if (char_attr_max > 0)
    {
      len= desodbc_min(len, char_attr_max - 1);
      (void)memcpy((char *)char_attr, (const char *)wvalue,
                   len * sizeof(SQLWCHAR));
      ((SQLWCHAR *)char_attr)[len]= 0;
    }

    if (free_value)
      x_free(value);
    x_free(wvalue);
  }

  return rc;
}


SQLRETURN SQL_API
SQLColumnPrivilegesW(SQLHSTMT hstmt,
                     SQLWCHAR *catalog, SQLSMALLINT catalog_len,
                     SQLWCHAR *schema, SQLSMALLINT schema_len,
                     SQLWCHAR *table, SQLSMALLINT table_len,
                     SQLWCHAR *column, SQLSMALLINT column_len)
{
  SQLRETURN rc;
  SQLCHAR *catalog8, *schema8, *table8, *column8;
  SQLINTEGER len;
  uint errors= 0;
  DBC *dbc;

  LOCK_STMT(hstmt);

  dbc= ((STMT *)hstmt)->dbc;

  len= catalog_len;
  catalog8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, catalog, &len, &errors);
  catalog_len= (SQLSMALLINT)len;

  len= schema_len;
  schema8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, schema, &len, &errors);
  schema_len= (SQLSMALLINT)len;

  len= table_len;
  table8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, table, &len, &errors);
  table_len= (SQLSMALLINT)len;

  len= column_len;
  column8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, column, &len, &errors);
  column_len= (SQLSMALLINT)len;

  rc= DESColumnPrivileges(hstmt, catalog8, catalog_len, schema8, schema_len,
                            table8, table_len, column8, column_len);

  x_free(catalog8);
  x_free(schema8);
  x_free(table8);
  x_free(column8);

  return rc;
}


SQLRETURN SQL_API
SQLColumnsW(SQLHSTMT hstmt,
            SQLWCHAR *catalog, SQLSMALLINT catalog_len,
            SQLWCHAR *schema, SQLSMALLINT schema_len,
            SQLWCHAR *table, SQLSMALLINT table_len,
            SQLWCHAR *column, SQLSMALLINT column_len)
{
  SQLRETURN rc;
  SQLCHAR *catalog8, *schema8, *table8, *column8;
  SQLINTEGER len;
  uint errors= 0;
  DBC *dbc;

  LOCK_STMT(hstmt);

  dbc= ((STMT *)hstmt)->dbc;

  len= catalog_len;
  catalog8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, catalog, &len, &errors);
  catalog_len= (SQLSMALLINT)len;

  len= schema_len;
  schema8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, schema, &len, &errors);
  schema_len= (SQLSMALLINT)len;

  len= table_len;
  table8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, table, &len, &errors);
  table_len= (SQLSMALLINT)len;

  len= column_len;
  column8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, column, &len, &errors);
  column_len= (SQLSMALLINT)len;

  rc= DESColumns(hstmt, catalog8, catalog_len, schema8, schema_len,
                   table8, table_len, column8, column_len);

  x_free(catalog8);
  x_free(schema8);
  x_free(table8);
  x_free(column8);

  return rc;
}


SQLRETURN SQL_API
SQLConnectW(SQLHDBC hdbc, SQLWCHAR *dsn, SQLSMALLINT dsn_len,
            SQLWCHAR *user, SQLSMALLINT user_len,
            SQLWCHAR *auth, SQLSMALLINT auth_len)
{
  CHECK_HANDLE(hdbc);

  ((DBC *)hdbc)->unicode= TRUE; /* Hooray, a Unicode connection! */

  return DESConnect(hdbc, dsn, dsn_len, user, user_len, auth, auth_len);
}


SQLRETURN SQL_API
SQLDriverConnectW(SQLHDBC hdbc, SQLHWND hwnd,
                  SQLWCHAR *in, SQLSMALLINT in_len,
                  SQLWCHAR *out, SQLSMALLINT out_max, SQLSMALLINT *out_len,
                  SQLUSMALLINT completion)
{
  SQLSMALLINT dummy_out_len = 0;

  CHECK_HANDLE(hdbc);

  // Make sure out_len is never NULL at least internally
  if (!out_len)
    out_len = &dummy_out_len;

  ((DBC *)hdbc)->unicode= TRUE; /* Hooray, a Unicode connection! */

  return DESDriverConnect(hdbc, hwnd, in, in_len, out, out_max,
                            out_len, completion);
}


SQLRETURN SQL_API
SQLDescribeColW(SQLHSTMT hstmt, SQLUSMALLINT column,
                SQLWCHAR *name, SQLSMALLINT name_max, SQLSMALLINT *name_len,
                SQLSMALLINT *type, SQLULEN *size, SQLSMALLINT *scale,
                SQLSMALLINT *nullable)
{
  STMT *stmt= (STMT *)hstmt;
  SQLCHAR *value= NULL;
  SQLWCHAR *wvalue= NULL;
  SQLINTEGER len= SQL_NTS;
  SQLSMALLINT free_value= 0;
  uint errors;

  std::string name_str = stmt->table->index_to_name_col(column);
  //We convert the name (in std::string) to an unsigned char vector
  //in order to use the sqlchar_as_sqlwchar function provided by the MySQL ODBC source.
  std::vector<unsigned char> name_uchar(name_str.begin(), name_str.end());
  value = name_uchar.data();

  wvalue =
      sqlchar_as_sqlwchar(stmt->dbc->cxn_charset_info, value, &len, &errors);

  if (name_len) *name_len = (SQLSMALLINT)len;

  if (name && name_max > 0) {
    len = desodbc_min(len, name_max - 1);
    (void)memcpy((char *)name, (const char *)wvalue, len * sizeof(SQLWCHAR));
    ((SQLWCHAR *)name)[len] = 0;
  }

  if (type)
    *type = stmt->table->col_type(name_str);
  if (size)
    *size = stmt->table->col_size(name_str);
  if (scale)
    *scale = stmt->table->col_scale(name_str);
  if (nullable)
    *nullable = stmt->table->col_nullable(name_str);

  return SQL_SUCCESS;

  /*
  STMT *stmt= (STMT *)hstmt;
  SQLCHAR *value= NULL;
  SQLWCHAR *wvalue= NULL;
  SQLINTEGER len= SQL_NTS;
  SQLSMALLINT free_value= 0;
  uint errors;

  SQLRETURN rc;

  LOCK_STMT(hstmt);

  rc= DESDescribeCol(hstmt, column, &value, &free_value, type,
                                 size, scale, nullable);

  if (free_value == -1)
  {
    set_mem_error(stmt->dbc->des);
    return handle_connection_error(stmt);
  }

  if (value)
  {
    wvalue= sqlchar_as_sqlwchar(stmt->dbc->cxn_charset_info, value, &len,
                                &errors);
    if (len == -1)
    {
      if (free_value)
        x_free(value);
      set_mem_error(stmt->dbc->des);
      return handle_connection_error(stmt);
    }

    // We set the error only when the result is intented to be returned

  if (name && len > name_max - 1) rc = stmt->set_error(DESERR_01004, NULL, 0);

  if (name_len) *name_len = (SQLSMALLINT)len;

  if (name && name_max > 0) {
    len = desodbc_min(len, name_max - 1);
    (void)memcpy((char *)name, (const char *)wvalue, len * sizeof(SQLWCHAR));
    ((SQLWCHAR *)name)[len] = 0;
  }

  if (free_value) x_free(value);
  x_free(wvalue);
}

return rc;
*/

}


SQLRETURN SQL_API
SQLExecDirectW(SQLHSTMT hstmt, SQLWCHAR *str, SQLINTEGER str_len)
{
  int error;

  LOCK_STMT(hstmt);

  if ((error= SQLPrepareWImpl(hstmt, str, str_len, false)))
    return error;
  error= DES_SQLExecute((STMT *)hstmt);

  SQLFreeStmt(hstmt, SQL_CLOSE);

  return error;
  //return SQL_SUCCESS;
}


SQLRETURN SQL_API
SQLForeignKeysW(SQLHSTMT hstmt,
                SQLWCHAR *pk_catalog, SQLSMALLINT pk_catalog_len,
                SQLWCHAR *pk_schema, SQLSMALLINT pk_schema_len,
                SQLWCHAR *pk_table, SQLSMALLINT pk_table_len,
                SQLWCHAR *fk_catalog, SQLSMALLINT fk_catalog_len,
                SQLWCHAR *fk_schema, SQLSMALLINT fk_schema_len,
                SQLWCHAR *fk_table, SQLSMALLINT fk_table_len)
{
  SQLRETURN rc;
  DBC *dbc;

  LOCK_STMT(hstmt);
  


  dbc = ((STMT *)hstmt)->dbc;
  /*
   * In this preliminar version we will ignore the catalog and schema inputs.
   */

  STMT *stmt = (STMT *)hstmt;

  /*
    Important comments for the implementation of this function:
    (source: https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/sqlforeignkeys-function?view=sql-server-ver16)

    "
        If *PKTableName contains a table name, SQLForeignKeys returns a result set
        that contains the primary key of the specified table and all the foreign keys
        that refer to it. The list of foreign keys in other tables does not include
        foreign keys that point to unique constraints in the specified table.

        If *FKTableName contains a table name, SQLForeignKeys returns a result set that
        contains all the foreign keys in the specified table that point to primary keys
        in other tables, and the primary keys in the other tables to which they refer.
        The list of foreign keys in the specified table does not contain foreign keys
        that refer to unique constraints in other tables.

        If both *PKTableName and *FKTableName contain table names, SQLForeignKeys
        returns the foreign keys in the table specified in *FKTableName that refer to
        the primary key of the table specified in *PKTableName. This should be one key
        at most.
    "

  */

  //TODO: check whether "contain table names" is equivalent to the field being non-null.
  //TODO: check possible loss of data if converting to string (as warned by the compiler).
  if (pk_table != NULL && fk_table != NULL) {

    std::wstring pk_table_wstr(pk_table);
    std::string pk_table_str(pk_table_wstr.begin(), pk_table_wstr.end());

    std::wstring fk_table_wstr(fk_table);
    std::string fk_table_str(fk_table_wstr.begin(), fk_table_wstr.end());

    stmt->pk_table_name = pk_table_str;
    stmt->fk_table_name = fk_table_str;
    stmt->type = SQLFOREIGNKEYS_PKFK;

  } else if (pk_table != NULL) {

    std::wstring pk_table_wstr(pk_table);
    std::string pk_table_str(pk_table_wstr.begin(), pk_table_wstr.end());

    stmt->pk_table_name = pk_table_str;
  
    stmt->type = SQLFOREIGNKEYS_PK;
  }
    
  else if (fk_table != NULL) {

    std::wstring fk_table_wstr(fk_table);
    std::string fk_table_str(fk_table_wstr.begin(), fk_table_wstr.end());

    stmt->fk_table_name = fk_table_str;
  
    stmt->type = SQLFOREIGNKEYS_FK;

  }
  else
    return SQL_ERROR;

  // Now, we construct the query "/dbschema"
  const char dbschema_str[10] = "/dbschema";
  SQLINTEGER dbschema_len = 10;
  SQLCHAR *dbschema_sqlchar = (SQLCHAR *)dbschema_str;

  rc = DES_SQLPrepare(hstmt, dbschema_sqlchar, dbschema_len, false, false);

  rc = DES_SQLExecute(stmt);

  return rc;
}


SQLRETURN SQL_API
SQLGetConnectAttrW(SQLHDBC hdbc, SQLINTEGER attribute, SQLPOINTER value,
                   SQLINTEGER value_max, SQLINTEGER *value_len)
{
  CHECK_HANDLE(hdbc);

  return SQLGetConnectAttrWImpl(hdbc, attribute, value, value_max, value_len);
}


SQLRETURN SQL_API
SQLGetConnectAttrWImpl(SQLHDBC hdbc, SQLINTEGER attribute, SQLPOINTER value,
                       SQLINTEGER value_max, SQLINTEGER *value_len)
{
  DBC *dbc= (DBC *)hdbc;
  SQLCHAR *char_value= NULL;

  SQLRETURN rc= 0;

  /*
    for numeric attributes value_max can be 0, so we must check for
    the valid output buffer to prevent crashes
  */
  if (value)
    rc= DESGetConnectAttr(hdbc, attribute, &char_value, value);

  if (char_value)
  {
    SQLWCHAR *wvalue;
    SQLINTEGER len= SQL_NTS;
    uint errors;
    desodbc::CHARSET_INFO *result_charset_info= dbc->cxn_charset_info;

    /*
      When SQLGetConnectAttr is called before connecting the connection
      is not established yet and its charset is unknown. We assume UTF8
      as the most suitable charset for the string result.
    */
    if(!dbc->cxn_charset_info)
    {
      result_charset_info= desodbc::get_charset_by_csname(transport_charset, DESF(DES_CS_PRIMARY),
                                                 DESF(0));
    }

    wvalue= sqlchar_as_sqlwchar(result_charset_info, char_value,
                                &len, &errors);

    /* value_max is in bytes, we want it in chars. */
    value_max/= sizeof(SQLWCHAR);

    /*
      This check is inside the statement, which does not
      execute if output buffer is NULL
      see: "if (char_value)"
    */
    if (len > value_max - 1)
      rc = dbc->set_error(DESERR_01004, NULL, 0);

    if (value_len)
      *value_len= len * sizeof(SQLWCHAR);

    if (value_max > 0)
    {
      len= desodbc_min(len, value_max - 1);
      (void)memcpy((char *)value, (const char *)wvalue,
                   len * sizeof(SQLWCHAR));
      ((SQLWCHAR *)value)[len]= 0;
    }

    x_free(wvalue);
  }

  return rc;
}


SQLRETURN SQL_API
SQLGetCursorNameW(SQLHSTMT hstmt, SQLWCHAR *cursor, SQLSMALLINT cursor_max,
                  SQLSMALLINT *cursor_len)
{
  SQLRETURN rc= SQL_SUCCESS;
  STMT *stmt= (STMT *)hstmt;
  SQLWCHAR *name;
  SQLINTEGER len= SQL_NTS;
  uint errors;

  LOCK_STMT(hstmt);

  CLEAR_STMT_ERROR(stmt);

  if (cursor_max < 0)
    return stmt->set_error( DESERR_S1090, NULL, 0);

  name= sqlchar_as_sqlwchar(stmt->dbc->cxn_charset_info,
                            DESGetCursorName(hstmt), &len, &errors);

  if (cursor_len)
    *cursor_len= (SQLSMALLINT)len;

  /* Warn if name truncated, but buffer is not null */
  if (cursor && len > cursor_max - 1)
    rc= stmt->set_error( DESERR_01004, NULL, 0);

  if (cursor_max > 0)
  {
    len= desodbc_min(len, cursor_max - 1);
    (void)memcpy((char *)cursor, (const char *)name, len * sizeof(SQLWCHAR));
    cursor[len]= 0;
  }

  x_free(name);

  return rc;
}


SQLRETURN SQL_API
SQLGetDiagFieldW(SQLSMALLINT handle_type, SQLHANDLE handle,
                 SQLSMALLINT record, SQLSMALLINT field,
                 SQLPOINTER info, SQLSMALLINT info_max,
                 SQLSMALLINT *info_len)
{
  DBC *dbc;
  SQLCHAR *value= NULL;
  SQLINTEGER len= SQL_NTS;

  SQLRETURN rc= SQL_SUCCESS;

  CHECK_HANDLE(handle);

  rc= DESGetDiagField(handle_type, handle, record, field,
                        &value, info);

  switch (handle_type) {
  case SQL_HANDLE_DBC:
    dbc= (DBC *)handle;
    break;
  case SQL_HANDLE_STMT:
    dbc= ((STMT *)handle)->dbc;
    break;
  case SQL_HANDLE_DESC:
    dbc= DESC_GET_DBC((DESC *)handle);
    break;
  case SQL_HANDLE_ENV:
  default:
    dbc= NULL;
  }

  if (value)
  {
    uint errors;
    SQLWCHAR *wvalue= sqlchar_as_sqlwchar((dbc && dbc->cxn_charset_info) ?
                                          dbc->cxn_charset_info :
                                          desodbc::default_charset_info,
                                          value, &len, &errors);

    /* info_max is in bytes, we want it in chars. */
    info_max/= sizeof(SQLWCHAR);

    /* We set the error only when the result is intented to be returned */
    if (info && len > info_max - 1)
      rc= dbc->set_error(DESERR_01004, NULL, 0);

    if (info_len)
      *info_len= (SQLSMALLINT)len * sizeof(SQLWCHAR);

    if (info_max > 0)
    {
      len= desodbc_min(len, info_max - 1);
      (void)memcpy((char *)info, (const char *)wvalue,
                   len * sizeof(SQLWCHAR));
      ((SQLWCHAR *)info)[len]= 0;
    }

    x_free(wvalue);
  }

  return rc;
}


SQLRETURN SQL_API
SQLGetDiagRecW(SQLSMALLINT handle_type, SQLHANDLE handle,
               SQLSMALLINT record, SQLWCHAR *sqlstate,
               SQLINTEGER *native_error, SQLWCHAR *message,
               SQLSMALLINT message_max, SQLSMALLINT *message_len)
{
  CHECK_HANDLE(handle);

  return SQLGetDiagRecWImpl(handle_type, handle, record, sqlstate, native_error,
                            message, message_max, message_len);
}


SQLRETURN SQL_API
SQLGetDiagRecWImpl(SQLSMALLINT handle_type, SQLHANDLE handle,
                   SQLSMALLINT record, SQLWCHAR *sqlstate,
                   SQLINTEGER *native_error, SQLWCHAR *message,
                   SQLSMALLINT message_max, SQLSMALLINT *message_len)
{
  SQLRETURN rc;
  DBC *dbc;
  SQLCHAR *msg_value= NULL, *sqlstate_value= NULL;
  SQLINTEGER len= SQL_NTS;
  uint errors;

  if (handle == NULL)
  {
    return SQL_INVALID_HANDLE;
  }

  switch (handle_type) {
  case SQL_HANDLE_DBC:
    dbc= (DBC *)handle;
    break;
  case SQL_HANDLE_STMT:
    dbc= ((STMT *)handle)->dbc;
    break;
  case SQL_HANDLE_DESC:
    dbc= DESC_GET_DBC((DESC *)handle);
    break;
  case SQL_HANDLE_ENV:
  default:
    dbc= NULL;
  }

  if (message_max < 0)
    return SQL_ERROR;

  rc= DESGetDiagRec(handle_type, handle, record, &sqlstate_value,
                      native_error, &msg_value);

  if (rc == SQL_NO_DATA_FOUND)
  {
    return SQL_NO_DATA_FOUND;
  }

  if (msg_value)
  {
    SQLWCHAR *wvalue= sqlchar_as_sqlwchar((dbc && dbc->cxn_charset_info) ?
                                          dbc->cxn_charset_info :
                                          desodbc::default_charset_info,
                                          msg_value, &len, &errors);

    /*
      We set the error only when the result is intented to be returned
      and message_max is greaater than 0
    */
    if (message && message_max && len > message_max - 1)
      rc = dbc->set_error(DESERR_01004, NULL, 0);

    if (message_len)
      *message_len= (SQLSMALLINT)len;

    if (message_max > 0)
    {
      len= desodbc_min(len, message_max - 1);
      (void)memcpy((char *)message, (const char *)wvalue,
                   len * sizeof(SQLWCHAR));
      ((SQLWCHAR *)message)[len]= 0;
    }

    x_free(wvalue);
  }

  len= SQL_NTS;
  if (sqlstate && sqlstate_value)
  {
    SQLWCHAR *wvalue= sqlchar_as_sqlwchar((dbc && dbc->cxn_charset_info) ?
                                          dbc->cxn_charset_info :
                                          desodbc::default_charset_info,
                                          sqlstate_value, &len, &errors);

    if (wvalue)
    {
      (void)memcpy((char *)sqlstate, (const char *)wvalue,
                   5 * sizeof(SQLWCHAR));
    }
    else
    {
      sqlstate[0]= '0';
      sqlstate[1]= '0';
      sqlstate[2]= '0';
      sqlstate[3]= '0';
      sqlstate[4]= '0';
    }
    ((SQLWCHAR *)sqlstate)[5]= 0;

    x_free(wvalue);
  }

  return rc;
}


SQLRETURN SQL_API
SQLGetInfoW(SQLHDBC hdbc, SQLUSMALLINT type, SQLPOINTER value,
            SQLSMALLINT value_max, SQLSMALLINT *value_len)
{
  DBC *dbc= (DBC *)hdbc;
  SQLCHAR *char_value= NULL;
  SQLINTEGER len= SQL_NTS;
  uint errors;

  SQLRETURN rc;

  CHECK_HANDLE(hdbc);

  rc= DESGetInfo(hdbc, type, &char_value, value, value_len);

  if (char_value)
  {
    SQLWCHAR *wvalue= sqlchar_as_sqlwchar((dbc->cxn_charset_info ?
                                           dbc->cxn_charset_info :
                                           desodbc::default_charset_info),
                                          char_value, &len, &errors);

    /* value_max is in bytes, we want it in chars. */
    value_max/= sizeof(SQLWCHAR);

    /*
      MSSQL implementation does not return the truncation warning if the
      value is not NULL and value_max is 0
     */
    if (value && value_max && len > value_max - 1)
      rc = dbc->set_error(DESERR_01004, NULL, 0);

    if (value_len)
      *value_len= (SQLSMALLINT)len * sizeof(SQLWCHAR);

    if (value && value_max > 0)
    {
      len= desodbc_min(len, value_max - 1);
      (void)memcpy((char *)value, (const char *)wvalue,
                   len * sizeof(SQLWCHAR));
      ((SQLWCHAR *)value)[len]= 0;
    }

    x_free(wvalue);
  }

  return rc;
}


SQLRETURN SQL_API
SQLGetStmtAttrW(SQLHSTMT hstmt, SQLINTEGER attribute, SQLPOINTER value,
                SQLINTEGER value_max, SQLINTEGER *value_len)
{
  LOCK_STMT(hstmt);

  return DESGetStmtAttr(hstmt, attribute, value, value_max, value_len);
}


/* This shouldn't be necessary, but iODBC expects it. */
SQLRETURN SQL_API
SQLGetTypeInfoW(SQLHSTMT hstmt, SQLSMALLINT type)
{
  LOCK_STMT(hstmt);

  return DESGetTypeInfo(hstmt, type);
}


SQLRETURN SQL_API
SQLNativeSqlW(SQLHDBC hdbc, SQLWCHAR *in, SQLINTEGER in_len,
              SQLWCHAR *out, SQLINTEGER out_max, SQLINTEGER *out_len)
{
  SQLRETURN rc= SQL_SUCCESS;

  LOCK_DBC(hdbc);

  if (in_len == SQL_NTS)
    in_len= (SQLINTEGER)sqlwcharlen(in);

  if (out_len)
    *out_len= in_len;

  if (out && in_len >= out_max)
    rc = ((DBC*)hdbc)->set_error(DESERR_01004, NULL, 0);

  if (out_max > 0)
  {
    if (in_len > out_max - 1)
    {
      in_len= out_max - 1;
    }

    (void)memcpy((char *)out, (const char *)in, in_len * sizeof(SQLWCHAR));
    out[in_len]= 0;
  }

  return rc;
}


SQLRETURN SQL_API
SQLPrepareW(SQLHSTMT hstmt, SQLWCHAR *str, SQLINTEGER str_len)
{
  LOCK_STMT(hstmt);

  return SQLPrepareWImpl(hstmt, str, str_len, true);
}


SQLRETURN SQL_API
SQLPrepareWImpl(SQLHSTMT hstmt, SQLWCHAR *str, SQLINTEGER str_len,
                bool force_prepare)
{
  STMT *stmt= (STMT *)hstmt;
  uint errors;
  SQLCHAR *conv= sqlwchar_as_sqlchar(stmt->dbc->cxn_charset_info,
                                     str, &str_len, &errors);
  /* Character conversion problems are not tolerated. */
  if (errors)
  {
    x_free(conv);
    return stmt->set_error("22018", NULL, 0);
  }

  SQLRETURN rc = SQLPrepare(hstmt, conv, str_len, false, force_prepare);
  x_free(conv);
  return rc;
}


SQLRETURN SQL_API
SQLPrimaryKeysW(SQLHSTMT hstmt,
                SQLWCHAR *catalog, SQLSMALLINT catalog_len,
                SQLWCHAR *schema, SQLSMALLINT schema_len,
                SQLWCHAR *table, SQLSMALLINT table_len) {
  SQLRETURN rc;
  DBC *dbc;

  LOCK_STMT(hstmt);

  dbc = ((STMT *)hstmt)->dbc;
  /*
   * In this preliminar version we will ignore the catalog and schema inputs.
   */

  STMT *stmt = (STMT *)hstmt;
  stmt->type = SQLPRIMARYKEYS;

  // Now, we construct the query "/dbschema table"
  const char dbschema_str[11] = "/dbschema ";
  SQLINTEGER dbschema_len = 11;
  SQLCHAR *dbschema_sqlchar = (SQLCHAR *)dbschema_str;

  SQLCHAR *query = new SQLCHAR[dbschema_len + table_len + 1];
  SQLINTEGER query_length = dbschema_len + table_len + 1;
  memcpy(query, dbschema_str, dbschema_len);
  memcpy(query + dbschema_len, table, table_len);

  query[dbschema_len + table_len] = '/0';

  rc = DES_SQLPrepare(hstmt, query, query_length, false, false);

  rc = DES_SQLExecute(stmt);

  return rc;

}


SQLRETURN SQL_API
SQLProcedureColumnsW(SQLHSTMT hstmt,
                     SQLWCHAR *catalog, SQLSMALLINT catalog_len,
                     SQLWCHAR *schema, SQLSMALLINT schema_len,
                     SQLWCHAR *proc, SQLSMALLINT proc_len,
                     SQLWCHAR *column, SQLSMALLINT column_len)
{
  SQLRETURN rc;
  SQLCHAR *catalog8, *schema8, *proc8, *column8;
  SQLINTEGER len;
  uint errors= 0;
  DBC *dbc;

  LOCK_STMT(hstmt);

  dbc= ((STMT *)hstmt)->dbc;
  len= catalog_len;
  catalog8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, catalog, &len, &errors);
  catalog_len= (SQLSMALLINT)len;

  len= schema_len;
  schema8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, schema, &len, &errors);
  schema_len= (SQLSMALLINT)len;

  len= proc_len;
  proc8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, proc, &len, &errors);
  proc_len= (SQLSMALLINT)len;

  len= column_len;
  column8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, column, &len, &errors);
  column_len= (SQLSMALLINT)len;

  rc= DESProcedureColumns(hstmt, catalog8, catalog_len, schema8, schema_len,
                               proc8, proc_len, column8, column_len);

  x_free(catalog8);
  x_free(schema8);
  x_free(proc8);
  x_free(column8);

  return rc;
}


SQLRETURN SQL_API
SQLProceduresW(SQLHSTMT hstmt,
               SQLWCHAR *catalog, SQLSMALLINT catalog_len,
               SQLWCHAR *schema, SQLSMALLINT schema_len,
               SQLWCHAR *proc, SQLSMALLINT proc_len)
{
  SQLRETURN rc;
  SQLCHAR *catalog8, *schema8, *proc8;
  SQLINTEGER len;
  uint errors= 0;
  DBC *dbc;

  LOCK_STMT(hstmt);

  dbc= ((STMT *)hstmt)->dbc;
  len= catalog_len;
  catalog8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, catalog, &len, &errors);
  catalog_len= (SQLSMALLINT)len;

  len= schema_len;
  schema8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, schema, &len, &errors);
  schema_len= (SQLSMALLINT)len;

  len= proc_len;
  proc8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, proc, &len, &errors);
  proc_len= (SQLSMALLINT)len;

  rc= DESProcedures(hstmt, catalog8, catalog_len, schema8, schema_len,
                      proc8, proc_len);
  // Remove parameters
  ((STMT *)hstmt)->free_reset_params();

  x_free(catalog8);
  x_free(schema8);
  x_free(proc8);

  return rc;
}


SQLRETURN SQL_API
SQLSetConnectAttrW(SQLHDBC hdbc, SQLINTEGER attribute,
                   SQLPOINTER value, SQLINTEGER value_len)
{
  CHECK_HANDLE(hdbc);

  return SQLSetConnectAttrWImpl(hdbc, attribute, value, value_len);
}


SQLRETURN SQL_API
SQLSetConnectAttrWImpl(SQLHDBC hdbc, SQLINTEGER attribute,
                       SQLPOINTER value, SQLINTEGER value_len)
{
  SQLRETURN rc;
  DBC *dbc= (DBC *)hdbc;
  des_bool free_value= FALSE;
  /* Let's make it for windows only so far */
#ifdef _WIN32
  SQLINTEGER len= value_len == SQL_NTS ? SQL_NTS : value_len / sizeof(SQLWCHAR);
#else
  SQLINTEGER len= value_len == SQL_NTS ? SQL_NTS : value_len;
#endif

  if (attribute == SQL_ATTR_CURRENT_CATALOG)
  {
    uint errors= 0;

    /* Verifying for incorrect val_len */
    if (value_len < 0 && value_len != SQL_NTS)
    {
      return dbc->set_error("HY090",
                  " StringLength argument was less "
                  "than 0 but was not SQL_NTS " , 0);
    }

    if (is_connected(dbc))
      value= sqlwchar_as_sqlchar(dbc->cxn_charset_info,
                                 (SQLWCHAR*)value, &len, &errors);
    else
      value= sqlwchar_as_sqlchar(desodbc::default_charset_info,
                                 (SQLWCHAR*)value, &len, &errors);
    free_value= TRUE;
  }

  rc= DESSetConnectAttr(hdbc, attribute, value, len);

  if (free_value)
    x_free(value);

  return rc;
}


SQLRETURN SQL_API
SQLSetCursorNameW(SQLHSTMT hstmt, SQLWCHAR *name, SQLSMALLINT name_len)
{
  SQLRETURN rc;
  SQLINTEGER len= name_len;
  uint errors= 0;
  SQLCHAR *name_char;
  DBC *dbc;

  LOCK_STMT(hstmt);

  dbc= ((STMT *)hstmt)->dbc;
  name_char= sqlwchar_as_sqlchar(dbc->cxn_charset_info,
                                          name, &len, &errors);

  rc= DESSetCursorName(hstmt, name_char, (SQLSMALLINT)len);

  x_free(name_char);

  /* Character conversion problems are not tolerated. */
  if (errors)
  {
    return ((STMT *)hstmt)->set_error("HY000",
                          "Cursor name included characters that could not "
                          "be converted to connection character set", 0);
  }

  return rc;
}


SQLRETURN SQL_API
SQLSetStmtAttrW(SQLHSTMT hstmt, SQLINTEGER attribute,
                SQLPOINTER value, SQLINTEGER value_len)
{
  LOCK_STMT(hstmt);

  /* Nothing special to do, since we don't have any string stmt attribs */
  return DESSetStmtAttr(hstmt, attribute, value, value_len);
}


SQLRETURN SQL_API
SQLSpecialColumnsW(SQLHSTMT hstmt, SQLUSMALLINT type,
                   SQLWCHAR *catalog, SQLSMALLINT catalog_len,
                   SQLWCHAR *schema, SQLSMALLINT schema_len,
                   SQLWCHAR *table, SQLSMALLINT table_len,
                   SQLUSMALLINT scope, SQLUSMALLINT nullable)
{
  SQLRETURN rc;
  SQLCHAR *catalog8, *schema8, *table8;
  SQLINTEGER len;
  uint errors= 0;
  DBC *dbc;

  LOCK_STMT(hstmt);

  dbc= ((STMT *)hstmt)->dbc;
  len= catalog_len;
  catalog8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, catalog, &len, &errors);
  catalog_len= (SQLSMALLINT)len;

  len= schema_len;
  schema8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, schema, &len, &errors);
  schema_len= (SQLSMALLINT)len;

  len= table_len;
  table8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, table, &len, &errors);
  table_len= (SQLSMALLINT)len;

  rc= DESSpecialColumns(hstmt, type, catalog8, catalog_len,
                          schema8, schema_len, table8, table_len,
                          scope, nullable);

  x_free(catalog8);
  x_free(schema8);
  x_free(table8);

  return rc;
}


SQLRETURN SQL_API
SQLStatisticsW(SQLHSTMT hstmt,
               SQLWCHAR *catalog, SQLSMALLINT catalog_len,
               SQLWCHAR *schema, SQLSMALLINT schema_len,
               SQLWCHAR *table, SQLSMALLINT table_len,
               SQLUSMALLINT unique, SQLUSMALLINT accuracy)
{
  SQLRETURN rc;
  SQLCHAR *catalog8, *schema8, *table8;
  SQLINTEGER len;
  uint errors= 0;
  DBC *dbc;

  LOCK_STMT(hstmt);

  dbc= ((STMT *)hstmt)->dbc;
  len= catalog_len;
  catalog8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, catalog, &len, &errors);
  catalog_len= (SQLSMALLINT)len;

  len= schema_len;
  schema8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, schema, &len, &errors);
  schema_len= (SQLSMALLINT)len;

  len= table_len;
  table8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, table, &len, &errors);
  table_len= (SQLSMALLINT)len;

  rc= DESStatistics(hstmt, catalog8, catalog_len, schema8, schema_len,
                      table8, table_len, unique, accuracy);

  x_free(catalog8);
  x_free(schema8);
  x_free(table8);

  return rc;
}


SQLRETURN SQL_API
SQLTablePrivilegesW(SQLHSTMT hstmt,
                    SQLWCHAR *catalog, SQLSMALLINT catalog_len,
                    SQLWCHAR *schema, SQLSMALLINT schema_len,
                    SQLWCHAR *table, SQLSMALLINT table_len)
{
  SQLRETURN rc;
  SQLCHAR *catalog8, *schema8, *table8;
  SQLINTEGER len;
  uint errors= 0;
  DBC *dbc;

  LOCK_STMT(hstmt);

  dbc= ((STMT *)hstmt)->dbc;
  len= catalog_len;
  catalog8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, catalog, &len, &errors);
  catalog_len= (SQLSMALLINT)len;

  len= schema_len;
  schema8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, schema, &len, &errors);
  schema_len= (SQLSMALLINT)len;

  len= table_len;
  table8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, table, &len, &errors);
  table_len= (SQLSMALLINT)len;

  rc= DESTablePrivileges(hstmt, catalog8, catalog_len, schema8, schema_len,
                           table8, table_len);

  x_free(catalog8);
  x_free(schema8);
  x_free(table8);

  return rc;
}


SQLRETURN SQL_API
SQLTablesW(SQLHSTMT hstmt,
           SQLWCHAR *catalog, SQLSMALLINT catalog_len,
           SQLWCHAR *schema, SQLSMALLINT schema_len,
           SQLWCHAR *table, SQLSMALLINT table_len,
           SQLWCHAR *type, SQLSMALLINT type_len)
{
  SQLRETURN rc;
  SQLCHAR *catalog8, *schema8, *table8, *type8;
  SQLINTEGER len;
  uint errors= 0;
  DBC *dbc;

  LOCK_STMT(hstmt);

  dbc= ((STMT *)hstmt)->dbc;

  /* we must preserve NULL/blank strings for SQLTables() semantics */

  len= catalog_len;
  catalog8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, catalog, &len, &errors);
  if (catalog && !len)
    catalog8= (SQLCHAR*)"";
  catalog_len= (SQLSMALLINT)len;

  len= schema_len;
  schema8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, schema, &len, &errors);
  if (schema && !len)
    schema8= (SQLCHAR*)"";
  schema_len= (SQLSMALLINT)len;

  len= table_len;
  table8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, table, &len, &errors);
  if (table && !len)
    table8= (SQLCHAR*)"";
  table_len= (SQLSMALLINT)len;

  len= type_len;
  type8= sqlwchar_as_sqlchar(dbc->cxn_charset_info, type, &len, &errors);
  type_len= (SQLSMALLINT)len;

  rc= DESTables(hstmt, catalog8, catalog_len, schema8, schema_len,
                  table8, table_len, type8, type_len);

  rc = SQL_SUCCESS;

  if (catalog_len)
    x_free(catalog8);
  if (schema_len)
    x_free(schema8);
  if (table_len)
    x_free(table8);
  x_free(type8);

  return rc;

}


SQLRETURN SQL_API
SQLGetDescFieldW(SQLHDESC hdesc, SQLSMALLINT record, SQLSMALLINT field,
                 SQLPOINTER value, SQLINTEGER value_max, SQLINTEGER *value_len)
{
  CHECK_HANDLE(hdesc);

  return DESGetDescField(hdesc, record, field, value, value_max, value_len);
}


SQLRETURN SQL_API
SQLGetDescRecW(SQLHDESC hdesc, SQLSMALLINT record, SQLWCHAR *name,
               SQLSMALLINT name_max, SQLSMALLINT *name_len, SQLSMALLINT *type,
               SQLSMALLINT *subtype, SQLLEN *length, SQLSMALLINT *precision,
               SQLSMALLINT *scale, SQLSMALLINT *nullable)
{
  NOT_IMPLEMENTED;
}


SQLRETURN SQL_API
SQLSetDescFieldW(SQLHDESC hdesc, SQLSMALLINT record, SQLSMALLINT field,
                 SQLPOINTER value, SQLINTEGER value_len)
{
  CHECK_HANDLE(hdesc);
  DESC *desc = (DESC*)hdesc;
  SQLRETURN rc = SQL_SUCCESS;
  SQLPOINTER val = value;
  SQLCHAR *val8 = nullptr;
  uint errors= 0;

  switch(field)
  {
    case SQL_DESC_NAME:
      // Assume a string value, which needs conversion
      val8= sqlwchar_as_sqlchar(desc->dbc->cxn_charset_info,
                                (SQLWCHAR*)value, &value_len, &errors);

      // Set SQL_NTS instead of the real length or type won't be recognized
      value_len = SQL_NTS;
      break;
  }

  rc = desc->set_field(record, field,
                       val8 ? val8 : val,
                       value_len);
  x_free(val8);
  return rc;
}


SQLRETURN SQL_API
SQLSetDescRecW(SQLHDESC hdesc, SQLSMALLINT record, SQLSMALLINT type,
               SQLSMALLINT subtype, SQLLEN length, SQLSMALLINT precision,
               SQLSMALLINT scale, SQLPOINTER data_ptr,
               SQLLEN *octet_length_ptr, SQLLEN *indicator_ptr)
{
  NOT_IMPLEMENTED;
}


SQLRETURN SQL_API
SQLBrowseConnectW(SQLHDBC hdbc, SQLWCHAR *in, SQLSMALLINT in_len,
                  SQLWCHAR *out, SQLSMALLINT out_max, SQLSMALLINT *out_len)
{
  CHECK_HANDLE(hdbc);

  return ((DBC*)hdbc)->set_error(DESERR_S1000,
    "Driver does not support this API", 0);
}


#ifdef NOT_IMPLEMENTED_YET
//SQLDataSourcesW


//SQLDriversW


#endif

