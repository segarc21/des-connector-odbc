// Copyright (c) 2000, 2024, Oracle and/or its affiliates.
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
  @file  catalog.cc
  @brief Catalog functions.
*/

#include "driver.h"
#include "catalog.h"

/*
****************************************************************************
SQLColumns
****************************************************************************
*/
static char SC_type[10], SC_typename[20], SC_precision[10], SC_length[10],
    SC_scale[10], SC_nullable[10], SC_coldef[10], SC_sqltype[10], SC_octlen[10],
    SC_isnull[10];

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
static char *SQLCOLUMNS_values[] = {
    (char *)"",  (char *)"",           NullS,     NullS,     SC_type,
    SC_typename, SC_precision,         SC_length, SC_scale,  (char *)"10",
    SC_nullable, (char *)"DES column",
    SC_coldef,   SC_sqltype,           NullS,     SC_octlen, NullS,
    SC_isnull};

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

const uint SQLCOLUMNS_FIELDS = (uint)array_elements(SQLCOLUMNS_values);

/* DESODBC:
    This function inserts the SQLColumns fields
    into the corresponding ResultTable.

    Original author: DESODBC Developer
*/
void ResultTable::insert_SQLColumns_cols() {
  insert_cols(SQLCOLUMNS_fields, array_elements(SQLCOLUMNS_fields));
}

DES_FIELD SQLSPECIALCOLUMNS_fields[] = {
    DESODBC_FIELD_SHORT("SCOPE", 0),
    DESODBC_FIELD_NAME("COLUMN_NAME", NOT_NULL_FLAG),
    DESODBC_FIELD_SHORT("DATA_TYPE", NOT_NULL_FLAG),
    DESODBC_FIELD_STRING("TYPE_NAME", 20, NOT_NULL_FLAG),
    DESODBC_FIELD_LONG("COLUMN_SIZE", 0),
    DESODBC_FIELD_LONG("BUFFER_LENGTH", 0),
    DESODBC_FIELD_LONG("DECIMAL_DIGITS", 0),
    DESODBC_FIELD_SHORT("PSEUDO_COLUMN", 0),
};

static char *SQLSPECIALCOLUMNS_values[] = {0, NULL, 0, NULL, 0, 0, 0, 0};

const uint SQLSPECIALCOLUMNS_FIELDS =
    (uint)array_elements(SQLSPECIALCOLUMNS_fields);

size_t ROW_STORAGE::set_size(size_t rnum, size_t cnum) {
  size_t new_size = rnum * cnum;
  m_rnum = rnum;
  m_cnum = cnum;

  if (new_size) {
    m_data.resize(new_size, "");
    m_pdata.resize(new_size, nullptr);

    // Move the current row back if the array had shrunk
    if (m_cur_row >= rnum) m_cur_row = rnum - 1;
  } else {
    // Clear if the size is zero
    m_data.clear();
    m_pdata.clear();
    m_cur_row = 0;
  }

  return new_size;
}

bool ROW_STORAGE::next_row() {
  ++m_cur_row;

  if (m_cur_row < m_rnum - 1) {
    return true;
  }
  // if not enough rows - add one more
  set_size(m_rnum + 1, m_cnum);
  return false;
}

const xstring &ROW_STORAGE::operator=(const xstring &val) {
  size_t offs = m_cur_row * m_cnum + m_cur_col;
  m_data[offs] = val;
  m_pdata[offs] = m_data[offs].c_str();
  return m_data[offs];
}

xstring &ROW_STORAGE::operator[](size_t idx) {
  if (idx >= m_cnum) throw("Column number is out of bounds");

  m_cur_col = idx;
  return m_data[m_cur_row * m_cnum + m_cur_col];
}

const char **ROW_STORAGE::data() {
  auto m_data_it = m_data.begin();
  auto m_pdata_it = m_pdata.begin();

  while (m_data_it != m_data.end()) {
    *m_pdata_it = m_data_it->c_str();
    ++m_pdata_it;
    ++m_data_it;
  }
  return m_pdata.size() ? m_pdata.data() : nullptr;
}

/* DESODBC:
    Structure that checks that:
    - A catalog has been specified,
    - Schemas have not been specified

    Original Autor: MyODBC
    Modified by: DESODBC Developer
*/
#define CHECK_SCHEMA(ST, SN, SL)                           \
  if (SN && SL > 0)                                                        \
    return ST->set_error("HYC00", "Schemas are not supported in DESODBC");

/*
****************************************************************************
SQLTables
****************************************************************************
*/

DES_FIELD SQLTABLES_fields[] = {
    DESODBC_FIELD_NAME("TABLE_CAT", 0),
    DESODBC_FIELD_NAME("TABLE_SCHEM", 0),
    DESODBC_FIELD_NAME("TABLE_NAME", 0),
    DESODBC_FIELD_NAME("TABLE_TYPE", 0),
    /*
      Table remark length is 80 characters
    */
    DESODBC_FIELD_STRING("REMARKS", 80, 0),
};

/* DESODBC:
    This function inserts the SQLTables fields
    into the corresponding ResultTable.

    Original author: DESODBC Developer
*/
void ResultTable::insert_metadata_cols() {
  insert_cols(SQLTABLES_fields, array_elements(SQLTABLES_fields));
}

/* DESODBC:
    This function corresponds to the
    implementation of SQLTables, which was
    MySQLTables in MyODBC. We have reused
    some of its skeleton.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DES_SQLTables(SQLHSTMT hstmt, SQLCHAR *catalog_name,
                                SQLSMALLINT catalog_len, SQLCHAR *schema_name,
                                SQLSMALLINT schema_len, SQLCHAR *table_name,
                                SQLSMALLINT table_len, SQLCHAR *type_name,
                                SQLSMALLINT type_len) {
  SQLRETURN rc = SQL_SUCCESS;
  std::pair<SQLRETURN, std::string> pair;
  STMT *stmt = (STMT *)hstmt;
  DBC *dbc = stmt->dbc;

  CLEAR_STMT_ERROR(hstmt);
  DES_SQLFreeStmt(hstmt, FREE_STMT_RESET);

  GET_NAME_LEN(stmt, catalog_name, catalog_len);
  GET_NAME_LEN(stmt, schema_name, schema_len);
  GET_NAME_LEN(stmt, table_name, table_len);
  GET_NAME_LEN(stmt, type_name, type_len);

  CHECK_SCHEMA(stmt, schema_name, schema_len);

  std::string catalog_name_str =
      get_prepared_arg(stmt, catalog_name, catalog_len);

  rc = dbc->getQueryMutex();
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return rc;

  if (catalog_name) {
    stmt->params_for_table.catalog_name = catalog_name_str;
  }
  if (table_name) {
    std::string table_name_str = get_prepared_arg(stmt, table_name, table_len);
    stmt->params_for_table.table_name = table_name_str;
  }
  if (type_name) {
    std::string table_type_str = get_prepared_arg(stmt, type_name, type_len);
    stmt->params_for_table.table_type = table_type_str;
  }
  stmt->type = SQLTABLES;

  rc = stmt->build_results();

  rc = dbc->releaseQueryMutex();
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return rc;

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
                         const char *ch_buflen, SQLSMALLINT sqltype,
                         size_t col_size, bool is_null) {
  switch (sqltype) {
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

  if (is_null) return 0;

  return (SQLULEN)std::strtoll(ch_buflen, nullptr, 10);
}

/* DESODBC:
    This function corresponds to the
    implementation of SQLColumns, which was
    MySQLColumns in MyODBC. We have reused
    some of its skeleton.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/** MyODBC comment:
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
SQLRETURN SQL_API DES_SQLColumns(SQLHSTMT hstmt, SQLCHAR *catalog_name,
                                 SQLSMALLINT catalog_len, SQLCHAR *schema_name,
                                 SQLSMALLINT schema_len, SQLCHAR *table_name,
                                 SQLSMALLINT table_len, SQLCHAR *column_name,
                                 SQLSMALLINT column_len)

{
  SQLRETURN rc = SQL_SUCCESS;
  std::pair<SQLRETURN, std::string> pair;
  STMT *stmt = (STMT *)hstmt;
  DBC *dbc = stmt->dbc;
  CLEAR_STMT_ERROR(hstmt);
  DES_SQLFreeStmt(hstmt, FREE_STMT_RESET);

  GET_NAME_LEN(stmt, catalog_name, catalog_len);
  GET_NAME_LEN(stmt, schema_name, schema_len);
  GET_NAME_LEN(stmt, table_name, table_len);
  GET_NAME_LEN(stmt, column_name, column_len);
  CHECK_SCHEMA(stmt, schema_name, schema_len);

  rc = dbc->getQueryMutex();
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return rc;

  pair = dbc->send_query_and_read("/current_db");
  rc = pair.first;
  std::string current_db_output = pair.second;
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    dbc->releaseQueryMutex();
    return rc;
  }
  std::string previous_db = getLines(current_db_output)[0];

  std::string catalog_name_str =
      get_prepared_arg(stmt, catalog_name, catalog_len);
  
  std::string use_db_query = "/use_db ";
  if (catalog_name_str.size() == 0) use_db_query += "$des";
  else
    use_db_query += catalog_name_str;

  pair = dbc->send_query_and_read(use_db_query);
  rc = pair.first;
  std::string use_db_output = pair.second;

  if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
    // We do not want to return an error when receiving this message.
    if (!is_in_string(use_db_output, "Database already in use")) {
      rc = check_and_set_errors(SQL_HANDLE_STMT, stmt, use_db_output);
    }
  }

  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    dbc->send_query_and_read(
        "/use_db " +
        previous_db);  // DESODBC: trying to revert the database change
    dbc->releaseQueryMutex();
    return rc;
  }

  std::string table_name_str = get_prepared_arg(stmt, table_name, table_len);
  stmt->params_for_table.table_name = table_name_str;

  if (column_name) {
    std::string column_str = get_prepared_arg(stmt, column_name, column_len);
    stmt->params_for_table.column_name = column_str;
  }

  stmt->type = SQLCOLUMNS;
  stmt->params_for_table.catalog_name = catalog_name_str;

  rc = stmt->build_results();
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    dbc->send_query_and_read("/use_db " + previous_db);
    dbc->releaseQueryMutex();
    return rc;
  }

  pair = dbc->send_query_and_read("/use_db " + previous_db);
  rc = pair.first;
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    dbc->releaseQueryMutex();
    return rc;
  }

  rc = dbc->releaseQueryMutex();
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return rc;

  return rc;
}

/*
****************************************************************************
SQLStatistics
****************************************************************************
*/

DES_FIELD SQLSTAT_fields[] = {
    DESODBC_FIELD_NAME("TABLE_CAT", 0),
    DESODBC_FIELD_NAME("TABLE_SCHEM", 0),
    DESODBC_FIELD_NAME("TABLE_NAME", NOT_NULL_FLAG),
    DESODBC_FIELD_SHORT("NON_UNIQUE", 0),
    DESODBC_FIELD_NAME("INDEX_QUALIFIER", 0),
    DESODBC_FIELD_NAME("INDEX_NAME", 0),
    DESODBC_FIELD_SHORT("TYPE", NOT_NULL_FLAG),
    DESODBC_FIELD_SHORT("ORDINAL_POSITION", 0),
    DESODBC_FIELD_NAME("COLUMN_NAME", 0),
    DESODBC_FIELD_STRING("ASC_OR_DESC", 1, 0),
    DESODBC_FIELD_LONG("CARDINALITY", 0),
    DESODBC_FIELD_LONG("PAGES", 0),
    DESODBC_FIELD_STRING("FILTER_CONDITION", 10, 0),
};

/* DESODBC:
    This function inserts the SQLStatistics fields
    into the corresponding ResultTable.

    Original author: DESODBC Developer
*/
void ResultTable::insert_SQLStatistics_cols() {
  insert_cols(SQLSTAT_fields, array_elements(SQLSTAT_fields));
}

/* DESODBC:
    This function corresponds to the
    implementation of SQLStatistics, which was
    MySQLStatistics in MyODBC. We have reused
    some of its skeleton.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : ODBC 1.0 API
  @purpose : retrieves a list of statistics about a single table and the
       indexes associated with the table. The driver returns the
       information as a result set.
*/
SQLRETURN SQL_API DES_SQLStatistics(SQLHSTMT hstmt, SQLCHAR *catalog_name,
                                    SQLSMALLINT catalog_len,
                                    SQLCHAR *schema_name,
                                    SQLSMALLINT schema_len, SQLCHAR *table_name,
                                    SQLSMALLINT table_len, SQLUSMALLINT fUnique,
                                    SQLUSMALLINT fAccuracy) {
  SQLRETURN rc = SQL_SUCCESS;
  std::pair<SQLRETURN, std::string> pair;
  STMT *stmt = (STMT *)hstmt;

  CLEAR_STMT_ERROR(hstmt);
  DES_SQLFreeStmt(hstmt, FREE_STMT_RESET);

  GET_NAME_LEN(stmt, catalog_name, catalog_len);
  GET_NAME_LEN(stmt, schema_name, schema_len);
  GET_NAME_LEN(stmt, table_name, table_len);
  CHECK_SCHEMA(stmt, schema_name, schema_len);

  DBC *dbc = stmt->dbc;

  /* DESODBC:
    In DES, we cannot check statistics relative to indexes nor pages.
    We may only return the row relative to the cardinality of the table
    if requested.
  */

  rc = dbc->getQueryMutex();
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return rc;

  pair = dbc->send_query_and_read("/current_db");
  rc = pair.first;
  std::string current_db_output = pair.second;
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    dbc->releaseQueryMutex();
    return rc;
  }
  std::string previous_db = getLines(current_db_output)[0];

  std::string catalog_name_str = get_catalog(stmt, catalog_name, catalog_len);

  std::string use_db_query = "/use_db ";
  use_db_query += catalog_name_str;

  pair = dbc->send_query_and_read(use_db_query);
  rc = pair.first;
  std::string use_db_output = pair.second;

  if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
    // We do not want to return an error when receiving this message.
    if (!is_in_string(use_db_output, "Database already in use")) {
      rc = check_and_set_errors(SQL_HANDLE_STMT, stmt, use_db_output);
    }
  }

  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    dbc->send_query_and_read(
        "/use_db " +
        previous_db);  // DESODBC: trying to revert the database change
    dbc->releaseQueryMutex();
    return rc;
  }

  std::string table_name_str = get_prepared_arg(stmt, table_name, table_len);

  stmt->params_for_table.catalog_name = catalog_name_str;
  stmt->params_for_table.table_name = table_name_str;
  stmt->type = SQLSTATISTICS;

  rc = stmt->build_results();
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    dbc->send_query_and_read("/use_db " + previous_db);
    dbc->releaseQueryMutex();
    return rc;
  }

  pair = dbc->send_query_and_read("/use_db " + previous_db);
  rc = pair.first;
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    dbc->releaseQueryMutex();
    return rc;
  }

  rc = dbc->releaseQueryMutex();
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return rc;

  if (catalog_name_str != "$des" && catalog_name_str != "") {
    stmt->set_error("01000",
                    "Some attributes of the given external database that are "
                    "not shared with DES might "
                    "have been omitted");
    return SQL_SUCCESS_WITH_INFO;
  }
  return rc;
}

/*
    This function inserts the SQLSpecialColumns fields
    into the corresponding ResultTable.

    Original author: DESODBC Developer
*/
void ResultTable::insert_SQLSpecialColumns_cols() {
  insert_cols(SQLSPECIALCOLUMNS_fields,
              array_elements(SQLSPECIALCOLUMNS_fields));
}

/* DESODBC:
    This function corresponds to the
    implementation of SQLSpecialColumns, which was
    MySQLSpecialColumns in MyODBC. We have reused
    some of its skeleton.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : ODBC 1.0 API
  @purpose : retrieves the following information about columns within a
       specified table:
       - The optimal set of columns that uniquely identifies a row
         in the table.
       - Columns that are automatically updated when any value in the
         row is updated by a transaction
*/
SQLRETURN SQL_API DES_SQLSpecialColumns(
    SQLHSTMT hstmt, SQLUSMALLINT fColType, SQLCHAR *catalog,
    SQLSMALLINT catalog_len, SQLCHAR *schema, SQLSMALLINT schema_len,
    SQLCHAR *table_name, SQLSMALLINT table_len, SQLUSMALLINT fScope,
    SQLUSMALLINT fNullable) {
  /* DESODBC:
    As DES does not support indexing nor pseudocolumns, we will only return
    the primary keys when dealing with the SQLSPECIALCOLUMNS statement type.
    It will be necesary to call /dbschema table_name to do so.
  */
  SQLRETURN rc = SQL_SUCCESS;
  std::pair<SQLRETURN, std::string> pair;
  DBC *dbc;
  STMT *stmt = (STMT *)hstmt;

  CLEAR_STMT_ERROR(hstmt);
  DES_SQLFreeStmt(hstmt, FREE_STMT_RESET);

  GET_NAME_LEN(stmt, catalog, catalog_len);
  GET_NAME_LEN(stmt, schema, schema_len);
  GET_NAME_LEN(stmt, table_name, table_len);
  CHECK_SCHEMA(stmt, schema, schema_len);

  /*
  if (fColType == SQL_ROWVER) {
    return stmt->set_error("HYC00",
                           "The specified information cannot be retrieved by "
                           "DES in either local or external databases");
  }
  */

  std::string catalog_name_str = get_catalog(stmt, catalog, catalog_len);
  if (catalog_name_str != "$des")
    return stmt->set_error("HYC00",
                           "DESODBC cannot retrieve primary keys for external "
                           "databases, nor indexing or pseudocolumns");

  dbc = ((STMT *)hstmt)->dbc;

  rc = dbc->getQueryMutex();
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return rc;

  pair = dbc->send_query_and_read("/current_db");
  rc = pair.first;
  std::string current_db_output = pair.second;
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    dbc->releaseQueryMutex();
    return rc;
  }
  std::string previous_db = getLines(current_db_output)[0];

  pair = dbc->send_query_and_read("/use_db $des");
  rc = pair.first;
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    dbc->send_query_and_read(
        "/use_db " +
        previous_db);  // DESODBC: trying to revert the database change
    dbc->releaseQueryMutex();
    return rc;
  }

  std::string table_name_str = get_prepared_arg(stmt, table_name, table_len);
  stmt->params_for_table.table_name = table_name_str;
  stmt->type = SQLSPECIALCOLUMNS;

  rc = stmt->build_results();
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    dbc->send_query_and_read("/use_db " + previous_db);
    dbc->releaseQueryMutex();
    return rc;
  }

  pair = dbc->send_query_and_read("/use_db " + previous_db);
  rc = pair.first;
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    dbc->releaseQueryMutex();
    return rc;
  }

  rc = dbc->releaseQueryMutex();
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return rc;

  rc = SQL_SUCCESS_WITH_INFO;
  stmt->set_error("01000",
                  "Primary indexes have been returned. Information regarding "
                  "indexes might have been omitted due "
                  "to DES capabilities");

  return rc;
}

DES_FIELD SQLPRIM_KEYS_fields[] = {
    DESODBC_FIELD_NAME("TABLE_CAT", 0),
    DESODBC_FIELD_NAME("TABLE_SCHEM", 0),
    DESODBC_FIELD_NAME("TABLE_NAME", NOT_NULL_FLAG),
    DESODBC_FIELD_NAME("COLUMN_NAME", NOT_NULL_FLAG),
    DESODBC_FIELD_SHORT("KEY_SEQ", NOT_NULL_FLAG),
    DESODBC_FIELD_STRING("PK_NAME", 128, 0),
};

/* DESODBC:
    This function inserts the SQLPrimaryKeys fields
    into the corresponding ResultTable.

    Original author: DESODBC Developer
*/
void ResultTable::insert_SQLPrimaryKeys_cols() {
  insert_cols(SQLPRIM_KEYS_fields, array_elements(SQLPRIM_KEYS_fields));
};

/* DESODBC:
    This function corresponds to the
    implementation of SQLPrimaryKeys, which was
    MySQLPrimaryKeys in MyODBC. We have reused
    some of its skeleton.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : ODBC 1.0 API
  @purpose : returns the column names that make up the primary key for a table.
       The driver returns the information as a result set. This function
       does not support returning primary keys from multiple tables in
       a single call
*/
SQLRETURN SQL_API DES_SQLPrimaryKeys(SQLHSTMT hstmt, SQLCHAR *catalog_name,
                                     SQLSMALLINT catalog_len,
                                     SQLCHAR *schema_name,
                                     SQLSMALLINT schema_len,
                                     SQLCHAR *table_name,
                                     SQLSMALLINT table_len) {
  SQLRETURN rc = SQL_SUCCESS;
  std::pair<SQLRETURN, std::string> pair;
  DBC *dbc;
  STMT *stmt = (STMT *)hstmt;

  CLEAR_STMT_ERROR(hstmt);
  DES_SQLFreeStmt(hstmt, FREE_STMT_RESET);

  dbc = ((STMT *)hstmt)->dbc;

  GET_NAME_LEN(stmt, catalog_name, catalog_len);
  GET_NAME_LEN(stmt, schema_name, schema_len);
  GET_NAME_LEN(stmt, table_name, table_len);
  CHECK_SCHEMA(stmt, schema_name, schema_len);

  std::string catalog_name_str = get_catalog(stmt, catalog_name, catalog_len);

  if (catalog_name_str != "$des")
    return stmt->set_error("HYC00",
                           "DESODBC cannot retrieve primary or foreign keys "
                           "for external databases");

  rc = dbc->getQueryMutex();
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return rc;

  std::string table_name_str = get_prepared_arg(stmt, table_name, table_len);
  std::string main_query = "/dbschema ";
  main_query += catalog_name_str;
  main_query += ":";
  main_query += table_name_str;
  pair = dbc->send_query_and_read(main_query);
  rc = pair.first;
  std::string main_output = pair.second;

  if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
    rc = check_and_set_errors(SQL_HANDLE_STMT, stmt, main_output);
  }

  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    dbc->releaseQueryMutex();
    return rc;
  }

  stmt->params_for_table.catalog_name = catalog_name_str;
  stmt->params_for_table.table_name = table_name_str;
  stmt->type = SQLPRIMARYKEYS;
  stmt->last_output = main_output;

  rc = stmt->build_results();
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    dbc->releaseQueryMutex();
    return rc;
  }

  rc = dbc->releaseQueryMutex();
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return rc;

  return rc;
}

/*
****************************************************************************
SQLForeignKeys
****************************************************************************
*/

DES_FIELD SQLFORE_KEYS_fields[] = {
    DESODBC_FIELD_NAME("PKTABLE_CAT", 0),
    DESODBC_FIELD_NAME("PKTABLE_SCHEM", 0),
    DESODBC_FIELD_NAME("PKTABLE_NAME", NOT_NULL_FLAG),
    DESODBC_FIELD_NAME("PKCOLUMN_NAME", NOT_NULL_FLAG),
    DESODBC_FIELD_NAME("FKTABLE_CAT", 0),
    DESODBC_FIELD_NAME("FKTABLE_SCHEM", 0),
    DESODBC_FIELD_NAME("FKTABLE_NAME", NOT_NULL_FLAG),
    DESODBC_FIELD_NAME("FKCOLUMN_NAME", NOT_NULL_FLAG),
    DESODBC_FIELD_SHORT("KEY_SEQ", NOT_NULL_FLAG),
    DESODBC_FIELD_SHORT("UPDATE_RULE", 0),
    DESODBC_FIELD_SHORT("DELETE_RULE", 0),
    DESODBC_FIELD_NAME("FK_NAME", 0),
    DESODBC_FIELD_NAME("PK_NAME", 0),
    DESODBC_FIELD_SHORT("DEFERRABILITY", 0),
};

/* DESODBC:
    This function inserts the SQLForeignKeys fields
    into the corresponding ResultTable.

    Original author: DESODBC Developer
*/
void ResultTable::insert_SQLForeignKeys_cols() {
  insert_cols(SQLFORE_KEYS_fields, array_elements(SQLFORE_KEYS_fields));
};

/* DESODBC:
    This function corresponds to the
    implementation of SQLForeignKeys, which was
    MySQLForeignKeys in MyODBC. We have reused
    some of its skeleton.

    Original author: MyODBC
    Modified by: DESODBC Developer
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
SQLRETURN SQL_API DES_SQLForeignKeys(
    SQLHSTMT hstmt, SQLCHAR *pk_catalog_name, SQLSMALLINT pk_catalog_len,
    SQLCHAR *pk_schema_name, SQLSMALLINT pk_schema_len, SQLCHAR *pk_table_name,
    SQLSMALLINT pk_table_len, SQLCHAR *fk_catalog_name,
    SQLSMALLINT fk_catalog_len, SQLCHAR *fk_schema_name,
    SQLSMALLINT fk_schema_len, SQLCHAR *fk_table_name,
    SQLSMALLINT fk_table_len) {
  SQLRETURN rc = SQL_SUCCESS;
  std::pair<SQLRETURN, std::string> pair;
  DBC *dbc;

  LOCK_STMT(hstmt);

  STMT *stmt = (STMT *)hstmt;

  CLEAR_STMT_ERROR(hstmt);
  DES_SQLFreeStmt(hstmt, FREE_STMT_RESET);

  GET_NAME_LEN(stmt, pk_catalog_name, pk_catalog_len);
  GET_NAME_LEN(stmt, fk_catalog_name, fk_catalog_len);
  GET_NAME_LEN(stmt, pk_schema_name, pk_schema_len);
  GET_NAME_LEN(stmt, fk_schema_name, fk_schema_len);
  GET_NAME_LEN(stmt, pk_table_name, pk_table_len);
  GET_NAME_LEN(stmt, fk_table_name, fk_table_len);

  CHECK_SCHEMA(stmt, pk_schema_name,
                       pk_schema_len);
  CHECK_SCHEMA(stmt, fk_schema_name,
                       fk_schema_len);

  dbc = ((STMT *)hstmt)->dbc;

  std::string pk_catalog_str = get_catalog(stmt, pk_catalog_name, pk_catalog_len);
  std::string fk_catalog_str =
      get_catalog(stmt, fk_catalog_name, fk_catalog_len);

  if (pk_catalog_str != "$des" || fk_catalog_str != "$des")
    return stmt->set_error("HYC00",
                           "DESODBC cannot retrieve primary or foreign keys "
                           "for external databases");

  /* DESODBC:
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

  stmt->params_for_table.catalog_name =
      pk_catalog_str;  // DESODBC: could also be fk_catalog_str. Both values are
                       // "$des".

  std::string pk_table_str = "";
  std::string fk_table_str = "";
  if (pk_table_name != NULL)
    pk_table_str = get_prepared_arg(stmt, pk_table_name, pk_table_len);
  if (fk_table_name != NULL)
    fk_table_str = get_prepared_arg(stmt, fk_table_name, fk_table_len);

  
  if (pk_table_str.size() > 0 && fk_table_str.size() > 0) {
    stmt->params_for_table.pk_table_name = pk_table_str;
    stmt->params_for_table.fk_table_name = fk_table_str;
    stmt->type = SQLFOREIGNKEYS_PKFK;

  } else if (pk_table_str.size() > 0) {
    stmt->params_for_table.pk_table_name = pk_table_str;

    stmt->type = SQLFOREIGNKEYS_PK;
  }

  else if (fk_table_str.size() > 0) {
    stmt->params_for_table.fk_table_name = fk_table_str;

    stmt->type = SQLFOREIGNKEYS_FK;

  } else
    return stmt->set_error("HY000", "Not any tables have been specified");

  std::string main_query = "/dbschema ";
  main_query += pk_catalog_str;  // DESODBC: could also be fk_catalog_str. Both
                                 // values are "$des".
  pair = dbc->send_query_and_read(main_query);
  rc = pair.first;
  std::string main_output = pair.second;

  rc = dbc->getQueryMutex();
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return rc;

  stmt->last_output = main_output;

  rc = stmt->build_results();
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    dbc->releaseQueryMutex();
    return rc;
  }

  rc = dbc->releaseQueryMutex();
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return rc;

  return rc;
}