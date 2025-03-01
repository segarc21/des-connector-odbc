// Copyright (c) 2000, 2024, Oracle and/or its affiliates.
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
  @file  catalog.c
  @brief Catalog functions.
*/

#include "driver.h"
#include "catalog.h"

static char SC_type[10],SC_typename[20],SC_precision[10],SC_length[10],SC_scale[10],
SC_nullable[10], SC_coldef[10], SC_sqltype[10],SC_octlen[10],
SC_isnull[10];

static char *SQLCOLUMNS_values[]= {
    (char*)"", (char*)"", NullS, NullS, SC_type, SC_typename,
    SC_precision,
    SC_length, SC_scale, (char*)"10", SC_nullable, (char*)"DES column", //TODO: DES column?
    SC_coldef, SC_sqltype, NullS, SC_octlen, NullS, SC_isnull
};

static DES_FIELD SQLCOLUMNS_fields[] = {
    DESODBC_FIELD_NAME("TABLE_CAT", 0),
    DESODBC_FIELD_NAME("TABLE_SCHEM", 0),
    DESODBC_FIELD_NAME("TABLE_NAME", NOT_NULL_FLAG),
    DESODBC_FIELD_NAME("COLUMN_NAME", NOT_NULL_FLAG),
    DESODBC_FIELD_SHORT("DATA_TYPE", NOT_NULL_FLAG),
    DESODBC_FIELD_STRING("TYPE_NAME", 20, NOT_NULL_FLAG),
    DESODBC_FIELD_LONG("COLUMN_SIZE", 0),
    DESODBC_FIELD_LONG("BUFFER_LENGTH", 0),
    DESODBC_FIELD_SHORT("DECIMAL_DIGITS", 0),
    DESODBC_FIELD_SHORT("NUM_PREC_RADIX", 0),
    DESODBC_FIELD_SHORT("NULLABLE", NOT_NULL_FLAG),
    DESODBC_FIELD_NAME("REMARKS", 0),
    DESODBC_FIELD_NAME("COLUMN_DEF", 0),
    DESODBC_FIELD_SHORT("SQL_DATA_TYPE", NOT_NULL_FLAG),
    DESODBC_FIELD_SHORT("SQL_DATETIME_SUB", 0),
    DESODBC_FIELD_LONG("CHAR_OCTET_LENGTH", 0),
    DESODBC_FIELD_LONG("ORDINAL_POSITION", NOT_NULL_FLAG),
    DESODBC_FIELD_STRING("IS_NULLABLE", 3, 0),
};

const uint SQLCOLUMNS_FIELDS= (uint)array_elements(SQLCOLUMNS_values);

static char *SQLSPECIALCOLUMNS_values[] = {0, NULL, 0, NULL, 0, 0, 0, 0};

const uint SQLSPECIALCOLUMNS_FIELDS =
    (uint)array_elements(SQLSPECIALCOLUMNS_fields);


size_t ROW_STORAGE::set_size(size_t rnum, size_t cnum)
{
  size_t new_size = rnum * cnum;
  m_rnum = rnum;
  m_cnum = cnum;

  if (new_size)
  {
    m_data.resize(new_size, "");
    m_pdata.resize(new_size, nullptr);

    // Move the current row back if the array had shrunk
    if (m_cur_row >= rnum)
      m_cur_row = rnum - 1;
  }
  else
  {
    // Clear if the size is zero
    m_data.clear();
    m_pdata.clear();
    m_cur_row = 0;
  }

  return new_size;
}

bool ROW_STORAGE::next_row()
{
  ++m_cur_row;

  if (m_cur_row < m_rnum - 1)
  {
    return true;
  }
  // if not enough rows - add one more
  set_size(m_rnum + 1, m_cnum);
  return false;
}

const xstring & ROW_STORAGE::operator=(const xstring &val)
{
  size_t offs = m_cur_row * m_cnum + m_cur_col;
  m_data[offs] = val;
  m_pdata[offs] = m_data[offs].c_str();
  return m_data[offs];
}

xstring& ROW_STORAGE::operator[](size_t idx)
{
  if (idx >= m_cnum)
    throw ("Column number is out of bounds");

  m_cur_col = idx;
  return m_data[m_cur_row * m_cnum + m_cur_col];
}

const char** ROW_STORAGE::data()
{
  auto m_data_it = m_data.begin();
  auto m_pdata_it = m_pdata.begin();

  while(m_data_it != m_data.end())
  {
    *m_pdata_it = m_data_it->c_str();
    ++m_pdata_it;
    ++m_data_it;
  }
  return m_pdata.size() ? m_pdata.data() : nullptr;
}

/*
  @type    : internal
  @purpose : returns the next token
*/

const char *des_next_token(const char *prev_token,
                          const char **token,
                                char *data,
                          const char chr)
{
    const char *cur_token;

    if ( (cur_token= strchr(*token, chr)) )
    {
        if ( prev_token )
        {
            uint len= (uint)(cur_token-prev_token);
            strncpy(data,prev_token, len);
            data[len]= 0;
        }
        *token= (char *)cur_token+1;
        prev_token= cur_token;
        return cur_token+1;
    }
    return 0;
}


/**
  Create a fake result set in the current statement

  @param[in] stmt           Handle to statement
  @param[in] rowval         Row array
  @param[in] rowsize        sizeof(row array)
  @param[in] rowcnt         Number of rows
  @param[in] fields         Field array
  @param[in] fldcnt         Number of fields

  @return SQL_SUCCESS or SQL_ERROR (and diag is set)
*/

/*
  Check if no_catalog is set, but catalog is specified:
  CN is non-NULL pointer AND *CN is not an empty string AND
  CL the length is not zero

  Check if no_schema is set, but schema is specified:
  SN is non-NULL pointer AND *SN is not an empty string AND
  SL the length is not zero

  Check if catalog and schema were specified together and
  return an error if they were.
*/
#define CHECK_CATALOG_SCHEMA(ST, CN, CL, SN, SL) \
  if (ST->dbc->ds.opt_NO_CATALOG && CN && *CN && CL) \
    return ST->set_error("HY000", "Support for catalogs is disabled by " \
           "NO_CATALOG option, but non-empty catalog is specified."); \
  if (ST->dbc->ds.opt_NO_SCHEMA && SN && *SN && SL) \
    return ST->set_error("HY000", "Support for schemas is disabled by " \
           "NO_SCHEMA option, but non-empty schema is specified."); \
  if (CN && *CN && CL && SN && *SN && SL) \
    return ST->set_error("HY000", "Catalog and schema cannot be specified " \
           "together in the same function call.");


SQLRETURN SQL_API DESColumnPrivileges(
    SQLHSTMT hstmt, SQLCHAR *catalog_name, SQLSMALLINT catalog_len,
    SQLCHAR *schema_name, SQLSMALLINT schema_len, SQLCHAR *table_name,
    SQLSMALLINT table_len, SQLCHAR *column_name, SQLSMALLINT column_len) {
  STMT *stmt = (STMT *)hstmt;

  CLEAR_STMT_ERROR(hstmt);
  DES_SQLFreeStmt(hstmt, FREE_STMT_RESET);

  return SQL_ERROR; //TODO: mark as not implemented
}

/*
  @type    : ODBC 1.0 API
  @purpose : returns a list of tables and the privileges associated with
             each table. The driver returns the information as a result
             set on the specified statement.
*/
SQLRETURN SQL_API DESTablePrivileges(SQLHSTMT hstmt, SQLCHAR *catalog_name,
                                       SQLSMALLINT catalog_len,
                                       SQLCHAR *schema_name,
                                       SQLSMALLINT schema_len,
                                       SQLCHAR *table_name,
                                       SQLSMALLINT table_len) {
  STMT *stmt = (STMT *)hstmt;

  CLEAR_STMT_ERROR(hstmt);
  DES_SQLFreeStmt(hstmt, FREE_STMT_RESET);

  return SQL_ERROR; //TODO: implement as "unimplemented"
}

/*
****************************************************************************
SQLTables
****************************************************************************
*/


SQLRETURN SQL_API
DESTables(SQLHSTMT hstmt,
            SQLCHAR *catalog_name, SQLSMALLINT catalog_len,
            SQLCHAR *schema_name, SQLSMALLINT schema_len,
            SQLCHAR *table_name, SQLSMALLINT table_len,
            SQLCHAR *type_name, SQLSMALLINT type_len)
{

  SQLRETURN rc = SQL_SUCCESS;
  STMT *stmt= (STMT *)hstmt;

  CLEAR_STMT_ERROR(hstmt);
  DES_SQLFreeStmt(hstmt, FREE_STMT_RESET);

  std::string dbschema_str = "/dbschema";
  SQLCHAR *dbschema_sqlchar = reinterpret_cast<unsigned char *>(
      const_cast<char *>(dbschema_str.c_str()));
  SQLINTEGER dbschema_len = dbschema_str.size();

  stmt->type = SQLTABLES;
  
  rc = DES_SQLPrepare(hstmt, dbschema_sqlchar, dbschema_len, false, false);

  rc = DES_SQLExecute(stmt);


  return rc;
}


/*
****************************************************************************
SQLColumns
****************************************************************************
*/

/**
  Compute the buffer length for SQLColumns.
  Mostly used for numeric values when I_S.COLUMNS.CHARACTER_OCTET_LENGTH is
  NULL.
*/
SQLLEN get_buffer_length(const char *type_name, const char *ch_size,
  const char *ch_buflen, SQLSMALLINT sqltype, size_t col_size,
  bool is_null)
{
  switch (sqltype)
  {
    case SQL_DECIMAL:
      return std::strtoll(ch_buflen, nullptr, 10);

    case SQL_TINYINT:
      return 1;

    case SQL_SMALLINT:
      return 2;

    case SQL_INTEGER:
      return 4;

    case SQL_REAL:
      return 4;

    case SQL_DOUBLE:
      return 8;

    case SQL_BIGINT:
      return 20;

    case SQL_DATE:
      return sizeof(SQL_DATE_STRUCT);

    case SQL_TIME:
      return sizeof(SQL_TIME_STRUCT);

    case SQL_TIMESTAMP:
      return sizeof(SQL_TIMESTAMP_STRUCT);

    case SQL_BIT:
      return col_size;
  }

  if (is_null)
    return 0;

  return (SQLULEN)std::strtoll(ch_buflen, nullptr, 10);
}

/**
  Get information about the columns in one or more tables.

  @param[in] hstmt            Handle of statement
  @param[in] catalog_name     Name of catalog (database)
  @param[in] catalog_len      Length of catalog
  @param[in] schema_name      Name of schema (unused)
  @param[in] schema_len       Length of schema name
  @param[in] table_name       Pattern of table names to match
  @param[in] table_len        Length of table pattern
  @param[in] column_name      Pattern of column names to match
  @param[in] column_len       Length of column pattern
*/
SQLRETURN SQL_API
DESColumns(SQLHSTMT hstmt, SQLCHAR *catalog_name, SQLSMALLINT catalog_len,
             SQLCHAR *schema_name,
             SQLSMALLINT schema_len,
             SQLCHAR *table_name, SQLSMALLINT table_len,
             SQLCHAR *column_name, SQLSMALLINT column_len)

{
  STMT *stmt= (STMT *)hstmt;

  CLEAR_STMT_ERROR(hstmt);
  DES_SQLFreeStmt(hstmt, FREE_STMT_RESET);

  return SQL_ERROR; //TODO: implement
}


/*
****************************************************************************
SQLStatistics
****************************************************************************
*/



/*
  @type    : ODBC 1.0 API
  @purpose : retrieves a list of statistics about a single table and the
       indexes associated with the table. The driver returns the
       information as a result set.
*/

SQLRETURN SQL_API
DESStatistics(SQLHSTMT hstmt,
                SQLCHAR *catalog_name, SQLSMALLINT catalog_len,
                SQLCHAR *schema_name,
                SQLSMALLINT schema_len,
                SQLCHAR *table_name, SQLSMALLINT table_len,
                SQLUSMALLINT fUnique,
                SQLUSMALLINT fAccuracy)
{
  STMT *stmt= (STMT *)hstmt;

  /*
    In DES, we cannot check statistics relative to indexes nor pages.
    We may only return the row relative to the cardinality of the table
    if requested.
  */

  stmt->type = SQLSTATISTICS;

  stmt->table_name = std::string(reinterpret_cast<char *>(table_name));

  std::string query_str = "select * from " + stmt->table_name + ";";
  SQLCHAR *query = reinterpret_cast<unsigned char*>(const_cast<char*>(query_str.c_str()));
  SQLINTEGER query_length = query_str.size();

  SQLRETURN rc = DES_SQLPrepare(hstmt, query, query_length, false, false);

  rc = DES_SQLExecute(stmt);

  return rc;
}


/*
  @type    : ODBC 1.0 API
  @purpose : retrieves the following information about columns within a
       specified table:
       - The optimal set of columns that uniquely identifies a row
         in the table.
       - Columns that are automatically updated when any value in the
         row is updated by a transaction
*/

SQLRETURN SQL_API
DESSpecialColumns(SQLHSTMT hstmt, SQLUSMALLINT fColType,
                    SQLCHAR *catalog, SQLSMALLINT catalog_len,
                    SQLCHAR *schema, SQLSMALLINT schema_len,
                    SQLCHAR *table_name, SQLSMALLINT table_len,
                    SQLUSMALLINT fScope,
                    SQLUSMALLINT fNullable)
{
  STMT        *stmt=(STMT *) hstmt;
  /*
   * In this preliminar version we will ignore the catalog and schema inputs.
   */

  stmt->type = SQLSPECIALCOLUMNS;

  /*
    As DES does not support indexing nor pseudocolumns, we will only return
    the primary keys when dealing with the SQLSPECIALCOLUMNS statement type.
    It will be necesary to call /dbschema table_name to do so.
  */ 
  std::string dbschema_str = "/dbschema";
  SQLCHAR *dbschema_sqlchar = reinterpret_cast<unsigned char *>(
      const_cast<char *>(dbschema_str.c_str()));
  SQLINTEGER dbschema_len = dbschema_str.size();
  // Now, we construct the query "/dbschema table_name"
  std::string table_name_str = std::string(reinterpret_cast<char *>(table_name));
  std::string query_str = "/dbschema " + table_name_str;
  SQLCHAR *query =
      reinterpret_cast<unsigned char *>(const_cast<char *>(query_str.c_str()));
  SQLINTEGER query_length = query_str.size();

  SQLRETURN rc = DES_SQLPrepare(hstmt, query, query_length, false, false);

  rc = DES_SQLExecute(stmt);

  return rc;
}

/*
  @type    : ODBC 1.0 API
  @purpose : returns the column names that make up the primary key for a table.
       The driver returns the information as a result set. This function
       does not support returning primary keys from multiple tables in
       a single call
*/

SQLRETURN SQL_API
DESPrimaryKeys(SQLHSTMT hstmt,
                 SQLCHAR *catalog_name, SQLSMALLINT catalog_len,
                 SQLCHAR *schema_name,
                 SQLSMALLINT schema_len,
                 SQLCHAR *table_name, SQLSMALLINT table_len)
{
  SQLRETURN rc;
  DBC *dbc;
  STMT *stmt= (STMT *) hstmt;

  CLEAR_STMT_ERROR(hstmt);
  DES_SQLFreeStmt(hstmt, FREE_STMT_RESET);

  dbc = ((STMT *)hstmt)->dbc;
  /*
   * In this preliminar version we will ignore the catalog and schema inputs.
   */

  stmt->type = SQLPRIMARYKEYS;

  // Now, we construct the query "/dbschema table_name"
  std::string table_name_str(reinterpret_cast<char const *>(table_name));
  std::string query_str = "/dbschema " + table_name_str;
  SQLCHAR *query =
      reinterpret_cast<unsigned char *>(const_cast<char *>(query_str.c_str()));
  SQLINTEGER query_length = query_str.size();

  rc = DES_SQLPrepare(hstmt, query, query_length, false, false);

  rc = DES_SQLExecute(stmt);

  return rc;
}


/*
****************************************************************************
SQLForeignKeys
****************************************************************************
*/
/**
  Retrieve either a list of foreign keys in a specified table, or the list
  of foreign keys in other tables that refer to the primary key in the
  specified table. (We currently only support the former, not the latter.)

  @param[in] hstmt           Handle of statement
  @param[in] pk_catalog Catalog (database) of table with primary key that
                             we want to see foreign keys for
  @param[in] pk_catalog_len  Length of @a pk_catalog
  @param[in] pk_schema  Schema of table with primary key that we want to
                             see foreign keys for (unused)
  @param[in] pk_schema_len   Length of @a pk_schema
  @param[in] pk_table   Table with primary key that we want to see foreign
                             keys for
  @param[in] pk_table_len    Length of @a pk_table
  @param[in] fk_catalog Catalog (database) of table with foreign keys we
                             are interested in
  @param[in] fk_catalog_len  Length of @a fk_catalog
  @param[in] fk_schema  Schema of table with foreign keys we are
                             interested in
  @param[in] fk_schema_len   Length of fk_schema
  @param[in] fk_table   Table with foreign keys we are interested in
  @param[in] fk_table_len    Length of @a fk_table

  @return SQL_SUCCESS

  @since ODBC 1.0
*/
SQLRETURN SQL_API
DESForeignKeys(SQLHSTMT hstmt,
                 SQLCHAR *pk_catalog_name,
                 SQLSMALLINT pk_catalog_len,
                 SQLCHAR *pk_schema_name,
                 SQLSMALLINT pk_schema_len,
                 SQLCHAR *pk_table_name, SQLSMALLINT pk_table_len,
                 SQLCHAR *fk_catalog_name, SQLSMALLINT fk_catalog_len,
                 SQLCHAR *fk_schema_name,
                 SQLSMALLINT fk_schema_len,
                 SQLCHAR *fk_table_name, SQLSMALLINT fk_table_len) {
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
    (source:
    https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/sqlforeignkeys-function?view=sql-server-ver16)

    "
        If *PKTableName contains a table name, SQLForeignKeys returns a result
    set that contains the primary key of the specified table and all the foreign
    keys that refer to it. The list of foreign keys in other tables does not
    include foreign keys that point to unique constraints in the specified
    table.

        If *FKTableName contains a table name, SQLForeignKeys returns a result
    set that contains all the foreign keys in the specified table that point to
    primary keys in other tables, and the primary keys in the other tables to
    which they refer. The list of foreign keys in the specified table does not
    contain foreign keys that refer to unique constraints in other tables.

        If both *PKTableName and *FKTableName contain table names,
    SQLForeignKeys returns the foreign keys in the table specified in
    *FKTableName that refer to the primary key of the table specified in
    *PKTableName. This should be one key at most.
    "

  */

  // TODO: check whether "contain table names" is equivalent to the field being
  // non-null.
  // TODO: check possible loss of data if converting to string (as warned by the
  // compiler).
  if (pk_table_name != NULL && fk_table_name != NULL) {
    std::string pk_table_str(reinterpret_cast<char const *>(pk_table_name));

    std::string fk_table_str(reinterpret_cast<char const *>(fk_table_name));

    stmt->pk_table_name = pk_table_str;
    stmt->fk_table_name = fk_table_str;
    stmt->type = SQLFOREIGNKEYS_PKFK;

  } else if (pk_table_name != NULL) {
    std::string pk_table_str(reinterpret_cast<char const *>(pk_table_name));

    stmt->pk_table_name = pk_table_str;

    stmt->type = SQLFOREIGNKEYS_PK;
  }

  else if (fk_table_name != NULL) {
    std::string fk_table_str(reinterpret_cast<char const *>(fk_table_name));

    stmt->fk_table_name = fk_table_str;

    stmt->type = SQLFOREIGNKEYS_FK;

  } else
    return SQL_ERROR;

  // Now, we construct the query "/dbschema"
  std::string dbschema_str = "/dbschema";
  SQLCHAR *dbschema_sqlchar = reinterpret_cast<unsigned char *>(
      const_cast<char *>(dbschema_str.c_str()));
  SQLINTEGER dbschema_len = dbschema_str.size();

  rc = DES_SQLPrepare(hstmt, dbschema_sqlchar, dbschema_len, false, false);

  rc = DES_SQLExecute(stmt);

  return rc;
}

/*
****************************************************************************
SQLProcedures and SQLProcedureColumns
****************************************************************************
*/

/**
  Get the list of procedures stored in a catalog (database). This is done by
  generating the appropriate query against INFORMATION_SCHEMA. If no
  database is specified, the current database is used.

  @param[in] hstmt            Handle of statement
  @param[in] catalog_name     Name of catalog (database)
  @param[in] catalog_len      Length of catalog
  @param[in] schema_name      Pattern of schema (unused)
  @param[in] schema_len       Length of schema name
  @param[in] proc_name        Pattern of procedure names to fetch
  @param[in] proc_len         Length of procedure name
*/
SQLRETURN SQL_API
DESProcedures(SQLHSTMT hstmt,
                SQLCHAR *catalog_name, SQLSMALLINT catalog_len,
                SQLCHAR *schema_name,
                SQLSMALLINT schema_len,
                SQLCHAR *proc_name, SQLSMALLINT proc_len)
{
  SQLRETURN rc;
  STMT *stmt= (STMT *)hstmt;

  CLEAR_STMT_ERROR(hstmt);
  DES_SQLFreeStmt(hstmt, FREE_STMT_RESET);

  return SQL_ERROR; //TODO: handle appropriately
}


/*
****************************************************************************
SQLProcedure Columns
****************************************************************************
*/

/*
  @type    : ODBC 1.0 API
  @purpose : returns the list of input and output parameters, as well as
  the columns that make up the result set for the specified
  procedures. The driver returns the information as a result
  set on the specified statement
*/

SQLRETURN SQL_API
DESProcedureColumns(SQLHSTMT hstmt,
                    SQLCHAR *catalog_name, SQLSMALLINT catalog_len,
                    SQLCHAR *schema_name,
                    SQLSMALLINT schema_len,
                    SQLCHAR *proc_name, SQLSMALLINT proc_len,
                    SQLCHAR *column_name, SQLSMALLINT column_len)
{
  STMT *stmt= (STMT *)hstmt;

  CLEAR_STMT_ERROR(hstmt);
  DES_SQLFreeStmt(hstmt, FREE_STMT_RESET);

  return SQL_ERROR; //TODO: handle appropriately
}
