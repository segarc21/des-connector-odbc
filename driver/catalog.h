// Copyright (c) 2010, 2024, Oracle and/or its affiliates.
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
  @file  catalog.h
  @brief some definitions required for catalog functions
*/


/**
   enums for resultsets returned by catalog functions - we don't want
   magic numbers
*/

#include "field_types.h"
#include "mysql.h"

/* SQLColumns */
enum desodbcColumns {des_cTABLE_CAT= 0,      des_cTABLE_SCHEM,      des_cTABLE_NAME,
              /*3*/ des_cCOLUMN_NAME,       des_cDATA_TYPE,        des_cTYPE_NAME,
              /*6*/ des_cCOLUMN_SIZE,       des_cBUFFER_LENGTH,    des_cDECIMAL_DIGITS,
              /*9*/ des_cNUM_PREC_RADIX,    des_cNULLABLE,         des_cREMARKS,
              /*12*/des_cCOLUMN_DEF,        des_cSQL_DATA_TYPE,    des_cSQL_DATETIME_SUB,
              /*15*/des_cCHAR_OCTET_LENGTH, des_cORDINAL_POSITION, des_cIS_NULLABLE };

/* SQLProcedureColumns */
enum desodbcProcColumns {des_pcPROCEDURE_CAT= 0, des_pcPROCEDURE_SCHEM,  des_pcPROCEDURE_NAME,
                  /*3*/ des_pcCOLUMN_NAME,      des_pcCOLUMN_TYPE,      des_pcDATA_TYPE,
                  /*6*/ des_pcTYPE_NAME,        des_pcCOLUMN_SIZE,      des_pcBUFFER_LENGTH,
                  /*9*/ des_pcDECIMAL_DIGITS,   des_pcNUM_PREC_RADIX,   des_pcNULLABLE,
                  /*12*/des_pcREMARKS,          des_pcCOLUMN_DEF,       des_pcSQL_DATA_TYPE,
                  /*15*/des_pcSQL_DATETIME_SUB, des_pcCHAR_OCTET_LENGTH,des_pcORDINAL_POSITION,
                  /*18*/des_pcIS_NULLABLE };

typedef std::vector<DES_BIND> vec_bind;

/*
 * Helper class to be used with Catalog functions in order to obtain
 * data from Information_Schema.
*/
struct ODBC_CATALOG {
  STMT *stmt;
  tempBuf temp;
  std::string query;
  std::string from;
  std::string join;
  std::string where;
  std::string order_by;
  size_t col_count;
  std::vector<std::string> columns;
  DES_ROW current_row = nullptr;
  unsigned long *current_lengths = nullptr;
  DES_RESULT *des_res = nullptr;

  SQLCHAR *m_catalog;
  unsigned long m_catalog_len;
  SQLCHAR *m_schema;
  unsigned long m_schema_len;
  SQLCHAR *m_table;
  unsigned long m_table_len;
  SQLCHAR *m_column;
  unsigned long m_column_len;

  ODBC_CATALOG(STMT *s, size_t ccnt, std::string from_i_s,
    SQLCHAR *catalog, unsigned long catalog_len,
    SQLCHAR *schema, unsigned long schema_len,
    SQLCHAR *table, unsigned long table_len,
    SQLCHAR *column, unsigned long column_len);

  ODBC_CATALOG(STMT *s, size_t ccnt, std::string from_i_s,
    SQLCHAR *catalog, unsigned long catalog_len,
    SQLCHAR *schema, unsigned long schema_len,
    SQLCHAR *table, unsigned long table_len);

  ~ODBC_CATALOG();

  void add_param(const char *qstr, SQLCHAR *data, unsigned long &len);
  void add_column(std::string);

  // The string need to specify the join type such as LEFT JOIN ...
  void set_join(std::string s) { join = s; }
  void set_where(std::string s) { where = s; }
  void set_order_by(std::string s) { order_by = s; }
  void execute();
  size_t num_rows();
  DES_ROW fetch_row();
  bool is_null_value(int column);
  unsigned long *get_lengths();
  void data_seek(unsigned int rownum);
};

/* Some common(for i_s/no_i_s) helper functions */
const char *des_next_token(const char *prev_token,
                          const char **token,
                                char *data,
                          const char chr);

SQLRETURN
create_empty_fake_resultset(STMT *stmt, DES_ROW rowval, size_t rowsize,
                            DES_FIELD *fields, uint fldcnt);

SQLRETURN
create_fake_resultset(STMT *stmt, DES_ROW rowval, size_t rowsize,
                      des_ulonglong rowcnt, DES_FIELD *fields, uint fldcnt,
                      bool copy_rowval);


/* no_i_s functions */

DES_RESULT *db_status(STMT *stmt, std::string &db);

std::string get_database_name(STMT *stmt,
                              SQLCHAR *catalog, SQLINTEGER catalog_len,
                              SQLCHAR *schema, SQLINTEGER schema_len,
                              bool try_reget = true);


DES_RESULT *table_status(STMT        *stmt,
                        SQLCHAR     *db,
                        SQLSMALLINT  db_length,
                        SQLCHAR     *table,
                        SQLSMALLINT  table_length,
                        des_bool      wildcard,
                        des_bool      show_tables,
                        des_bool      show_views);

SQLRETURN
primary_keys_no_i_s(SQLHSTMT hstmt,
                    SQLCHAR *catalog, SQLSMALLINT catalog_len,
                    SQLCHAR *schema,
                    SQLSMALLINT schema_len,
                    SQLCHAR *table, SQLSMALLINT table_len);


SQLRETURN
procedure_columns_no_i_s(SQLHSTMT hstmt,
                         SQLCHAR *szCatalogName, SQLSMALLINT cbCatalogName,
                         SQLCHAR *szSchemaName,
                         SQLSMALLINT cbSchemaName,
                         SQLCHAR *szProcName, SQLSMALLINT cbProcName,
                         SQLCHAR *szColumnName, SQLSMALLINT cbColumnName);


SQLRETURN
special_columns_no_i_s(SQLHSTMT hstmt, SQLUSMALLINT fColType,
                       SQLCHAR *szTableQualifier, SQLSMALLINT cbTableQualifier,
                       SQLCHAR *szTableOwner,
                       SQLSMALLINT cbTableOwner,
                       SQLCHAR *szTableName, SQLSMALLINT cbTableName,
                       SQLUSMALLINT fScope,
                       SQLUSMALLINT fNullable);

/*
  @purpose : retrieves a list of statistics about a single table and the
       indexes associated with the table. The driver returns the
       information as a result set.
*/

SQLRETURN
statistics_no_i_s(SQLHSTMT hstmt,
                  SQLCHAR *catalog, SQLSMALLINT catalog_len,
                  SQLCHAR *schema,
                  SQLSMALLINT schema_len,
                  SQLCHAR *table, SQLSMALLINT table_len,
                  SQLUSMALLINT fUnique,
                  SQLUSMALLINT fAccuracy);

SQLRETURN
tables_no_i_s(SQLHSTMT hstmt,
              SQLCHAR *catalog, SQLSMALLINT catalog_len,
              SQLCHAR *schema, SQLSMALLINT schema_len,
              SQLCHAR *table, SQLSMALLINT table_len,
              SQLCHAR *type, SQLSMALLINT type_len);
