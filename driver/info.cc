// Copyright (c) 2000, 2024, Oracle and/or its affiliates.
// Modified in 2025 by Sergio Miguel García Jiménez <segarc21@ucm.es>
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
// Modified in 2025 by Sergio Miguel García Jiménez <segarc21@ucm.es>,
// hereinafter the DESODBC developer, in the context of the GPLv2 derivate
// work DESODBC, an ODBC Driver of the open-source DBMS Datalog Educational
// System (DES) (see https://www.fdi.ucm.es/profesor/fernan/des/)
//
// The authorship of each section of this source file (comments,
// functions and other symbols) belongs to MyODBC unless we
// explicitly state otherwise.
// ---------------------------------------------------------

/**
@file  info.c
@brief Driver information functions.
*/

#include "driver.h"

#define MYINFO_SET_ULONG(val) \
do { \
  *((SQLUINTEGER *)num_info)= (val); \
  *value_len= sizeof(SQLUINTEGER); \
  return SQL_SUCCESS; \
} while(0)

#define MYINFO_SET_USHORT(val) \
do { \
  *((SQLUSMALLINT *)num_info)= (SQLUSMALLINT)(val); \
  *value_len= sizeof(SQLUSMALLINT); \
  return SQL_SUCCESS; \
} while(0)

#define MYINFO_SET_STR(val) \
do { \
  *char_info= (SQLCHAR *)(val); \
  return SQL_SUCCESS; \
} while(0)

static my_bool desodbc_ov2_inited = 0;


/* DESODBC:

    Renamed from the original MySQLGetInfo and modified
    according to DES' needs.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/**
Return general information about the driver and data source
associated with a connection.

@param[in]  hdbc            Handle of database connection
@param[in]  fInfoType       Type of information to retrieve
@param[out] char_info       Pointer to buffer for returning string
@param[out] num_info        Pointer to buffer for returning numeric info
@param[out] value_len       Pointer to buffer for returning length (only
used for numeric data)
*/
SQLRETURN SQL_API
DESGetInfo(SQLHDBC hdbc, SQLUSMALLINT fInfoType,
             SQLCHAR **char_info, SQLPOINTER num_info, SQLSMALLINT *value_len)
{
  DBC *dbc = (DBC *)hdbc;
  SQLSMALLINT dummy;
  SQLINTEGER dummy_value;

  if (!value_len)
    value_len = &dummy;
  if (!num_info)
    num_info = &dummy_value;

  switch (fInfoType) {
  case SQL_ACTIVE_ENVIRONMENTS:
    MYINFO_SET_USHORT(0);

  case SQL_AGGREGATE_FUNCTIONS:
    MYINFO_SET_ULONG(SQL_AF_ALL | SQL_AF_AVG | SQL_AF_COUNT | SQL_AF_DISTINCT |
                     SQL_AF_MAX | SQL_AF_MIN | SQL_AF_SUM); //DES allow all of these

  case SQL_ALTER_DOMAIN:
    MYINFO_SET_ULONG(0); //DES does not allow ALTER DOMAIN


  //See 4.2.4.7 "Modifying Tables" for the following.
  //Also, see the rules of DDLstmt.
  case SQL_ALTER_TABLE:
    MYINFO_SET_ULONG(SQL_AT_ADD_TABLE_CONSTRAINT |
                      SQL_AT_DROP_TABLE_CONSTRAINT_CASCADE | SQL_AT_ADD_COLUMN |
                      SQL_AT_DROP_COLUMN | SQL_AT_DROP_COLUMN_CASCADE);

  //This ODBC Driver does not allow asyncronic execution.
#ifndef USE_IODBC //I imagine iODBC does not allow 3.8, and that is why MyODBC put this #ifndef.
  case SQL_ASYNC_DBC_FUNCTIONS:
    MYINFO_SET_ULONG(SQL_ASYNC_DBC_NOT_CAPABLE);
#endif

  case SQL_ASYNC_MODE:
    MYINFO_SET_ULONG(SQL_AM_NONE);

#ifdef SQL_ASYNC_NOTIFICATION
  case SQL_ASYNC_NOTIFICATION:
    MYINFO_SET_ULONG(SQL_ASYNC_NOTIFICATION_NOT_CAPABLE);
#endif

  case SQL_BATCH_ROW_COUNT:
    MYINFO_SET_ULONG(SQL_BRC_EXPLICIT);

  case SQL_BATCH_SUPPORT:
    MYINFO_SET_ULONG(SQL_BS_SELECT_EXPLICIT | SQL_BS_ROW_COUNT_EXPLICIT);

  case SQL_BOOKMARK_PERSISTENCE:
    MYINFO_SET_ULONG(SQL_BP_UPDATE | SQL_BP_DELETE);

  case SQL_CATALOG_LOCATION:
    MYINFO_SET_USHORT(SQL_CL_START); //When calling /dbschema, it is at the start.

  case SQL_CATALOG_NAME:
    MYINFO_SET_STR("Y");

  case SQL_CATALOG_NAME_SEPARATOR:
    MYINFO_SET_STR(":"); //the most common character is '.'. However,
                                                        //in DES, it seems to be ':': for example,
                                                        //catalog:table when calling /dbschema.

  case SQL_CATALOG_TERM:
    MYINFO_SET_STR("database");

  case SQL_CATALOG_USAGE:
    MYINFO_SET_ULONG(SQL_CU_DML_STATEMENTS |
       SQL_CU_TABLE_DEFINITION);

  case SQL_COLLATION_SEQ:
    MYINFO_SET_STR(dbc->cxn_charset_info->name);

  case SQL_COLUMN_ALIAS:
    MYINFO_SET_STR("Y"); //DES allows alias for columns.

  case SQL_CONCAT_NULL_BEHAVIOR: 
    MYINFO_SET_USHORT(SQL_CB_NULL);

  //We did not modify the conversions from MyODBC.
  case SQL_CONVERT_BIGINT:
  case SQL_CONVERT_BIT:
  case SQL_CONVERT_CHAR:
  case SQL_CONVERT_DATE:
  case SQL_CONVERT_DECIMAL:
  case SQL_CONVERT_DOUBLE:
  case SQL_CONVERT_FLOAT:
  case SQL_CONVERT_INTEGER:
  case SQL_CONVERT_LONGVARCHAR:
  case SQL_CONVERT_NUMERIC:
  case SQL_CONVERT_REAL:
  case SQL_CONVERT_SMALLINT:
  case SQL_CONVERT_TIME:
  case SQL_CONVERT_TIMESTAMP:
  case SQL_CONVERT_TINYINT:
  case SQL_CONVERT_VARCHAR:
  case SQL_CONVERT_WCHAR:
  case SQL_CONVERT_WVARCHAR:
  case SQL_CONVERT_WLONGVARCHAR:
    MYINFO_SET_ULONG(SQL_CVT_CHAR | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL |
                     SQL_CVT_INTEGER | SQL_CVT_SMALLINT | SQL_CVT_FLOAT |
                     SQL_CVT_REAL | SQL_CVT_DOUBLE | SQL_CVT_VARCHAR |
                     SQL_CVT_LONGVARCHAR | SQL_CVT_BIT | SQL_CVT_TINYINT |
                     SQL_CVT_BIGINT | SQL_CVT_DATE | SQL_CVT_TIME |
                     SQL_CVT_TIMESTAMP | SQL_CVT_WCHAR | SQL_CVT_WVARCHAR |
                     SQL_CVT_WLONGVARCHAR);

  case SQL_CONVERT_BINARY:
  case SQL_CONVERT_VARBINARY:
  case SQL_CONVERT_LONGVARBINARY:
  case SQL_CONVERT_INTERVAL_DAY_TIME:
  case SQL_CONVERT_INTERVAL_YEAR_MONTH:
    MYINFO_SET_ULONG(0);

  case SQL_CONVERT_FUNCTIONS:
    MYINFO_SET_ULONG(SQL_FN_CVT_CONVERT); //see "4.2.8 Conversion Functions"

  case SQL_CORRELATION_NAME:
    MYINFO_SET_USHORT(SQL_CN_ANY); //se the example given in "4.2.2 Main Features"

  case SQL_CREATE_ASSERTION:
  case SQL_CREATE_CHARACTER_SET:
  case SQL_CREATE_COLLATION:
  case SQL_CREATE_DOMAIN:
  case SQL_CREATE_SCHEMA:
    MYINFO_SET_ULONG(0);

  //TODO: should I put SQL_CT_COMMIT_DELETE?
  case SQL_CREATE_TABLE:
    MYINFO_SET_ULONG(SQL_CT_CREATE_TABLE | SQL_CT_TABLE_CONSTRAINT | SQL_CT_COMMIT_DELETE |
                     SQL_CT_LOCAL_TEMPORARY | SQL_CT_COLUMN_CONSTRAINT | SQL_CT_COLUMN_DEFAULT);

  case SQL_CREATE_TRANSLATION:
    MYINFO_SET_ULONG(0);

  case SQL_CREATE_VIEW:
    MYINFO_SET_ULONG(SQL_CV_CREATE_VIEW | SQL_CV_CHECK_OPTION); //TODO: I have omitted SQL_CV_CASCADED but I am not sure

  case SQL_CURSOR_COMMIT_BEHAVIOR:
  case SQL_CURSOR_ROLLBACK_BEHAVIOR:
    MYINFO_SET_USHORT(0);

#ifdef SQL_CURSOR_SENSITIVITY
  case SQL_CURSOR_SENSITIVITY:
    MYINFO_SET_ULONG(0);
#endif

#ifdef SQL_CURSOR_ROLLBACK_SQL_CURSOR_SENSITIVITY
  case SQL_CURSOR_ROLLBACK_SQL_CURSOR_SENSITIVITY:
    MYINFO_SET_ULONG(0);
#endif

  case SQL_DATA_SOURCE_NAME:
    MYINFO_SET_STR(dbc->ds.opt_DSN);

  case SQL_DATA_SOURCE_READ_ONLY:
    MYINFO_SET_STR("N");

  case SQL_DATABASE_NAME:
    if (is_connected(dbc)) {
      SQLRETURN rc = dbc->getQueryMutex();
      if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return rc;

      auto pair = dbc->send_query_and_read("/current_db");
      rc = pair.first;
      std::string current_db_output = pair.second;
      if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        dbc->releaseQueryMutex();
        return rc;
      }
      rc = dbc->releaseQueryMutex();
      if (rc != SQL_SUCCESS &&
          rc != SQL_SUCCESS_WITH_INFO) {
        return rc;
      }
      std::string db = getLines(current_db_output)[0];

      MYINFO_SET_STR(string_to_char_pointer(db)); //TODO: check memory leaks

    } else {
      return dbc->set_error(
          "HY000",
          "SQLGetInfo() needs an active connection to return current catalog");
    }
      
  case SQL_DATETIME_LITERALS:
    MYINFO_SET_ULONG(SQL_DL_SQL92_DATE | SQL_DL_SQL92_TIME |
                     SQL_DL_SQL92_TIMESTAMP);

  case SQL_DBMS_NAME:
    MYINFO_SET_STR("DES");

  case SQL_DBMS_VER:
    MYINFO_SET_STR("6.7");

    //DES does not support indexes
  /*
  case SQL_DDL_INDEX:
    MYINFO_SET_ULONG(SQL_DI_CREATE_INDEX | SQL_DI_DROP_INDEX);
  */
  case SQL_DEFAULT_TXN_ISOLATION:
    MYINFO_SET_ULONG(0); //DES does not support transactions

  case SQL_DESCRIBE_PARAMETER:
    MYINFO_SET_STR("Y"); //see ISLstmt syntax

  case SQL_DRIVER_NAME:
#ifdef MYODBC_UNICODEDRIVER
# ifdef WIN32
    MYINFO_SET_STR("desodbc" DESODBC_STRMAJOR_VERSION "w.dll");
# else
    MYINFO_SET_STR("libdesodbc" DESODBC_STRMAJOR_VERSION "w.so");
# endif
#else
# ifdef WIN32
    MYINFO_SET_STR("desodbc" DESODBC_STRMAJOR_VERSION "a.dll");
# else
    MYINFO_SET_STR("libdesodbc" DESODBC_STRMAJOR_VERSION "a.so");
# endif
#endif
  case SQL_DRIVER_ODBC_VER:
    MYINFO_SET_STR("03.80");

  case SQL_DRIVER_VER:
    MYINFO_SET_STR(DRIVER_VERSION);

  case SQL_DROP_ASSERTION:
  case SQL_DROP_CHARACTER_SET:
  case SQL_DROP_COLLATION:
  case SQL_DROP_DOMAIN:
  case SQL_DROP_SCHEMA:
  case SQL_DROP_TRANSLATION:
    MYINFO_SET_ULONG(0);

  //TODO: I have not put SQL_DT_RESTRICT. Check.
  case SQL_DROP_TABLE:
    MYINFO_SET_ULONG(SQL_DT_DROP_TABLE | SQL_DT_CASCADE);

  // TODO: I have not put SQL_DT_RESTRICT. Check.
  case SQL_DROP_VIEW:
    MYINFO_SET_ULONG(SQL_DV_DROP_VIEW | SQL_DV_CASCADE);

    //DESODBC does not support dynamic cursors.
    /*
  case SQL_DYNAMIC_CURSOR_ATTRIBUTES1:
    if (!dbc->ds.opt_FORWARD_CURSOR &&
        dbc->ds.opt_DYNAMIC_CURSOR)
      MYINFO_SET_ULONG(SQL_CA1_NEXT | SQL_CA1_ABSOLUTE | SQL_CA1_RELATIVE |
                       SQL_CA1_LOCK_NO_CHANGE | SQL_CA1_POS_POSITION |
                       SQL_CA1_POS_UPDATE | SQL_CA1_POS_DELETE |
                       SQL_CA1_POS_REFRESH | SQL_CA1_POSITIONED_UPDATE |
                       SQL_CA1_POSITIONED_DELETE | SQL_CA1_BULK_ADD);
    else
      MYINFO_SET_ULONG(0);

  case SQL_DYNAMIC_CURSOR_ATTRIBUTES2:
    if (!dbc->ds.opt_FORWARD_CURSOR &&
        dbc->ds.opt_DYNAMIC_CURSOR)
      MYINFO_SET_ULONG(SQL_CA2_SENSITIVITY_ADDITIONS |
                       SQL_CA2_SENSITIVITY_DELETIONS |
                       SQL_CA2_SENSITIVITY_UPDATES |
                       SQL_CA2_MAX_ROWS_SELECT | SQL_CA2_MAX_ROWS_INSERT |
                       SQL_CA2_MAX_ROWS_DELETE | SQL_CA2_MAX_ROWS_UPDATE |
                       SQL_CA2_CRC_EXACT | SQL_CA2_SIMULATE_TRY_UNIQUE);
    else
      MYINFO_SET_ULONG(0);
    */
  case SQL_EXPRESSIONS_IN_ORDERBY:
    MYINFO_SET_STR("Y"); //see tthe basic SQL query statement syntax in 4.2.6.1 Basic SQL Queries

  case SQL_FILE_USAGE:
    MYINFO_SET_USHORT(SQL_FILE_NOT_SUPPORTED);

  case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1:
    MYINFO_SET_ULONG(SQL_CA1_NEXT | SQL_CA1_ABSOLUTE | SQL_CA1_RELATIVE |
                     SQL_CA1_LOCK_NO_CHANGE | SQL_CA1_POS_POSITION |
                     SQL_CA1_POS_UPDATE | SQL_CA1_POS_DELETE |
                     SQL_CA1_POS_REFRESH | SQL_CA1_POSITIONED_UPDATE |
                     SQL_CA1_POSITIONED_DELETE | SQL_CA1_BULK_ADD);

  case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2:
    MYINFO_SET_ULONG(SQL_CA2_MAX_ROWS_SELECT | SQL_CA2_MAX_ROWS_INSERT |
                     SQL_CA2_MAX_ROWS_DELETE | SQL_CA2_MAX_ROWS_UPDATE | SQL_CA2_CRC_EXACT);

  case SQL_GETDATA_EXTENSIONS:
#ifndef USE_IODBC
    MYINFO_SET_ULONG(SQL_GD_ANY_COLUMN | SQL_GD_ANY_ORDER | SQL_GD_BLOCK |
                     SQL_GD_BOUND | SQL_GD_OUTPUT_PARAMS);
#else
    MYINFO_SET_ULONG(SQL_GD_ANY_COLUMN | SQL_GD_ANY_ORDER | SQL_GD_BLOCK |
                     SQL_GD_BOUND);
#endif

  case SQL_GROUP_BY:
    MYINFO_SET_USHORT(SQL_GB_NO_RELATION);

  case SQL_IDENTIFIER_CASE:
    MYINFO_SET_USHORT(SQL_IC_MIXED);

  case SQL_IDENTIFIER_QUOTE_CHAR:
    MYINFO_SET_STR("'");

    //DES not support indexes.
    /*
  case SQL_INDEX_KEYWORDS:
    MYINFO_SET_ULONG(SQL_IK_ALL);
    */

  case SQL_INFO_SCHEMA_VIEWS: //(context of /dbschema)
    MYINFO_SET_ULONG(
        SQL_ISV_ASSERTIONS | SQL_ISV_CHECK_CONSTRAINTS | SQL_ISV_COLUMNS |
        SQL_ISV_COLUMNS | SQL_ISV_CONSTRAINT_COLUMN_USAGE |
        SQL_ISV_CONSTRAINT_TABLE_USAGE | SQL_ISV_KEY_COLUMN_USAGE |
        SQL_ISV_REFERENTIAL_CONSTRAINTS | SQL_ISV_TABLES | SQL_ISV_VIEWS); //TODO: I have not put SQL_ISV_VIEW_COLUMN_USAGE nor SQL_ISV_VIEW_TABLE_USAGE, check.

  case SQL_INSERT_STATEMENT:
    MYINFO_SET_ULONG(SQL_IS_INSERT_LITERALS | SQL_IS_INSERT_SEARCHED |
                     SQL_IS_SELECT_INTO);

  case SQL_INTEGRITY:
    MYINFO_SET_STR("N"); //DES does not support Integrity Enhanced Facility (IEF)

    //Keyset driven cursors are not supported.
    /*
  case SQL_KEYSET_CURSOR_ATTRIBUTES1:
  case SQL_KEYSET_CURSOR_ATTRIBUTES2:
    MYINFO_SET_ULONG(0);
    */
  case SQL_KEYWORDS:
    /*
    Fetched from the DES manual running a script and checking the words one by one.
    */
    MYINFO_SET_STR(
        "ADD,ALL,ALTER,AND,ANY,AS,ASC,ASCENDING,ASSUME,AVG,BETWEEN,BY,"
        "CANDIDATE,CASCADE,CAST,CHAR,CHARACTER,CHECK,COLUMN,COMMIT,CONCAT,"
        "CONSTRAINT,COUNT,CREATE,DATA,DATABASE,DATABASES,DATE,DATETIME,DECIMAL,"
        "DEFAULT,DELETE,DESC,DESCENDING,DESCRIBE,DETERMINED,DIFFERENCE,"
        "DISTINCT,DIVISION,DOUBLE,DROP,DUAL,EXCEPT,EXISTS,EXTRACT,FALSE,FETCH,"
        "FIRST,FJOIN,FLOAT,FORALL,FOREIGN,FROM,FULL,GROUP,HAVING,IMPLIES,IN,"
        "INNER,INSERT,INSTR,INT,INTEGER,INTERSECT,INTO,IS,JOIN,KEY,LEFT,LENGTH,"
        "LIKE,LIMIT,LJOIN,LONGCHAR,LOWER,LPAD,LTRIM,MAX,MIN,MINUS,MONTH,NAME,"
        "NATURAL,NJOIN,NLJOIN,NOT,NRJOIN,NULL,NUMBER,NUMERIC,OFFSET,ON,ONLY,OR,"
        "ORDER,OUTER,PRECISION,PRIMARY,PRODUCT,PROJECT,REAL,RECURSIVE,"
        "REFERENCES,RENAME,REPLACE,REVERSE,RIGHT,RJOIN,ROLLBACK,ROWS,RPAD,"
        "RTRIM,SAVEPOINT,SELECT,SET,SHOW,SMALLINT,SORT,STRING,SUBSTR,SUM,TABLE,"
        "TABLES,TEXT,TIME,TIMESTAMP,TO,TOP,TRIM,TRUE,TYPE,UNION,UNIQUE,UPDATE,"
        "UPPER,USING,VALUES,VARCHAR,VARYING,VIEW,VIEWS,WHERE,WITH,WORK,XOR,"
        "YEAR,ZJOIN");

  case SQL_LIKE_ESCAPE_CLAUSE:
    MYINFO_SET_STR("N");

  case SQL_MAX_ASYNC_CONCURRENT_STATEMENTS:
    MYINFO_SET_ULONG(0);  // No specific limit

  case SQL_MAX_BINARY_LITERAL_LEN:
    MYINFO_SET_ULONG(0);  // No specific limit

  case SQL_MAX_CATALOG_NAME_LEN:
    MYINFO_SET_USHORT(0);  // No specific limit

  case SQL_MAX_CHAR_LITERAL_LEN:
    MYINFO_SET_ULONG(0);  // No specific limit

  case SQL_MAX_COLUMN_NAME_LEN:
    MYINFO_SET_USHORT(0);  // No specific limit

  case SQL_MAX_COLUMNS_IN_GROUP_BY:
    MYINFO_SET_USHORT(0);  // No specific limit

    //DES does not support indexes
    /*
  case SQL_MAX_COLUMNS_IN_INDEX:
    MYINFO_SET_USHORT(32);
    */

  case SQL_MAX_COLUMNS_IN_ORDER_BY:
    MYINFO_SET_USHORT(0);  // No specific limit

  case SQL_MAX_COLUMNS_IN_SELECT:
    MYINFO_SET_USHORT(0);  // No specific limit

  case SQL_MAX_COLUMNS_IN_TABLE:
    MYINFO_SET_USHORT(0); //No specific limit

  // SQL_ACTIVE_STATEMENTS in ODBC v1 has the same definition as
  // SQL_MAX_CONCURRENT_ACTIVITIES in ODBC v3.
  case SQL_MAX_CONCURRENT_ACTIVITIES:
    MYINFO_SET_USHORT(0);

  case SQL_MAX_CURSOR_NAME_LEN:
    MYINFO_SET_USHORT(DES_MAX_CURSOR_LEN);

  case SQL_MAX_DRIVER_CONNECTIONS:
#ifdef _WIN32
    MYINFO_SET_USHORT(MAX_CLIENTS);  // in Windows we can only have up to 256
                                      // clients connected and therefore, 256
                                      // possible concurrent activities.
#else
    MYINFO_SET_USHORT(0);  // No specific limit
#endif

  case SQL_MAX_IDENTIFIER_LEN:
    MYINFO_SET_USHORT(0); //in DES it is unlimited.

    //Not supported by DES:
    /*
  case SQL_MAX_INDEX_SIZE:
    MYINFO_SET_USHORT(3072);

  case SQL_MAX_PROCEDURE_NAME_LEN:
    MYINFO_SET_USHORT(N);
    */
  case SQL_MAX_ROW_SIZE:
    MYINFO_SET_ULONG(0); //No specific limit

  case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
    MYINFO_SET_STR("Y");

    //DESODBC does not work with schemas
    /*
  case SQL_MAX_SCHEMA_NAME_LEN:
    if (!dbc->ds.opt_NO_SCHEMA)
      MYINFO_SET_USHORT(64);
    else
      MYINFO_SET_USHORT(0);
    */
  case SQL_MAX_STATEMENT_LEN:
    MYINFO_SET_ULONG(0);

  case SQL_MAX_TABLE_NAME_LEN:
    MYINFO_SET_USHORT(0);

  case SQL_MAX_TABLES_IN_SELECT:
    MYINFO_SET_USHORT(0); //TODO: check

    //We will leave this as the username may take place in an external connection
  case SQL_MAX_USER_NAME_LEN:
    MYINFO_SET_USHORT(0);

  case SQL_MULT_RESULT_SETS:
    MYINFO_SET_STR("Y");

  case SQL_MULTIPLE_ACTIVE_TXN:
    MYINFO_SET_STR("N");

  case SQL_NEED_LONG_DATA_LEN:
    MYINFO_SET_STR("N");

  case SQL_NON_NULLABLE_COLUMNS:
    MYINFO_SET_USHORT(SQL_NNC_NON_NULL);

  case SQL_NULL_COLLATION:
    MYINFO_SET_USHORT(SQL_NC_HIGH);

    //all of them except SQL_FN_NUM_LOG10, SQL_FN_NUM_ATAN2, SQL_FN_NUM_DEGREES and SQL_FN_NUM_RADIANS.
    //technically SQL_FN_NUM_LOG10 is defined implicitly by using log/2 which allow us to
    //choose base 10. However, this is possible in MySQL and MyODBC did not include this value either.
  case SQL_NUMERIC_FUNCTIONS:
    MYINFO_SET_ULONG(SQL_FN_NUM_ABS | SQL_FN_NUM_ACOS | SQL_FN_NUM_ASIN |
                     SQL_FN_NUM_ATAN | SQL_FN_NUM_CEILING |
                     SQL_FN_NUM_COS | SQL_FN_NUM_COT | SQL_FN_NUM_EXP |
                     SQL_FN_NUM_FLOOR | SQL_FN_NUM_LOG | SQL_FN_NUM_MOD |
                     SQL_FN_NUM_SIGN | SQL_FN_NUM_SIN | SQL_FN_NUM_SQRT |
                     SQL_FN_NUM_TAN | SQL_FN_NUM_PI | SQL_FN_NUM_RAND | SQL_FN_NUM_POWER | SQL_FN_NUM_ROUND |
                     SQL_FN_NUM_TRUNCATE);

  /*DESODBC complies with the Core Interface Conformance, and could comply with Level 1 Interface conformance:
  in their requirements, it includes capabilities regarding procedures and transactions. DESODBC does not include these but not because of
  a lack of further development, but because of DES capabilities.*/
  case SQL_ODBC_API_CONFORMANCE:
    MYINFO_SET_USHORT(SQL_OIC_CORE);

  case SQL_ODBC_INTERFACE_CONFORMANCE:
    MYINFO_SET_ULONG(SQL_OIC_CORE);

  case SQL_ODBC_SQL_CONFORMANCE:
    MYINFO_SET_USHORT(
        SQL_SC_SQL92_ENTRY); //the same as MyODBC

  case SQL_OJ_CAPABILITIES:
    MYINFO_SET_ULONG(SQL_OJ_LEFT | SQL_OJ_RIGHT | SQL_OJ_NESTED |
                     SQL_OJ_NOT_ORDERED | SQL_OJ_INNER |
                     SQL_OJ_ALL_COMPARISON_OPS);

  case SQL_ORDER_BY_COLUMNS_IN_SELECT:
    MYINFO_SET_STR("N");

  //Same as MyODBC
  case SQL_PARAM_ARRAY_ROW_COUNTS:
    MYINFO_SET_ULONG(SQL_PARC_NO_BATCH);

  // Same as MyODBC
  case SQL_PARAM_ARRAY_SELECTS:
    MYINFO_SET_ULONG(SQL_PAS_NO_BATCH);

    //Procedures are not supported by DES
    /*
  case SQL_PROCEDURE_TERM:
    MYINFO_SET_STR("");
    */

  case SQL_PROCEDURES:
    MYINFO_SET_STR("N");

  case SQL_POS_OPERATIONS:
    MYINFO_SET_ULONG(SQL_POS_POSITION | SQL_POS_UPDATE | SQL_POS_DELETE |
                       SQL_POS_ADD | SQL_POS_REFRESH);

  case SQL_QUOTED_IDENTIFIER_CASE:
    MYINFO_SET_USHORT(SQL_IC_SENSITIVE);

  case SQL_ROW_UPDATES:
    MYINFO_SET_STR("N");

  case SQL_SCHEMA_TERM:
    MYINFO_SET_STR("dbschema");

  case SQL_SCHEMA_USAGE:
      MYINFO_SET_ULONG(0);

  case SQL_SCROLL_OPTIONS:
    MYINFO_SET_ULONG(SQL_SO_FORWARD_ONLY | SQL_SO_STATIC);

  case SQL_SEARCH_PATTERN_ESCAPE:
    MYINFO_SET_STR("\\");

    //Not appliable:
    /*
  case SQL_SERVER_NAME:
    MYINFO_SET_STR("");
    */

  case SQL_SPECIAL_CHARACTERS: //we will leave them the same as in MyODBC
    MYINFO_SET_STR(" !\"#%&'()*+,-.:;<=>?@[\\]^`{|}~");

  case SQL_SQL_CONFORMANCE:
    MYINFO_SET_ULONG(SQL_SC_SQL92_ENTRY); /* DES contains the majority of the SQL-92 intermediate level features.
    However, it does not contain characteristics such as privileges and transaction isolation levels.
    This is why we have put entry level SQL-92 compliant.*/

  case SQL_SQL92_DATETIME_FUNCTIONS:
    MYINFO_SET_ULONG(SQL_SDF_CURRENT_DATE | SQL_SDF_CURRENT_TIME |
                     SQL_SDF_CURRENT_TIMESTAMP);

  case SQL_SQL92_FOREIGN_KEY_DELETE_RULE:
  case SQL_SQL92_FOREIGN_KEY_UPDATE_RULE:
    MYINFO_SET_ULONG(SQL_SFKD_CASCADE | SQL_SFKD_NO_ACTION);

    //Not appliable
    /*
  case SQL_SQL92_GRANT:
    MYINFO_SET_ULONG(SQL_SG_DELETE_TABLE | SQL_SG_INSERT_COLUMN |
                     SQL_SG_INSERT_TABLE | SQL_SG_REFERENCES_TABLE |
                     SQL_SG_REFERENCES_COLUMN | SQL_SG_SELECT_TABLE |
                     SQL_SG_UPDATE_COLUMN | SQL_SG_UPDATE_TABLE |
                     SQL_SG_WITH_GRANT_OPTION);
                     */

    //DESODBC reused them from MyODBC:
  case SQL_SQL92_NUMERIC_VALUE_FUNCTIONS:
    MYINFO_SET_ULONG(SQL_SNVF_BIT_LENGTH | SQL_SNVF_CHAR_LENGTH |
                     SQL_SNVF_CHARACTER_LENGTH | SQL_SNVF_EXTRACT |
                     SQL_SNVF_OCTET_LENGTH | SQL_SNVF_POSITION);

  case SQL_SQL92_PREDICATES:
    MYINFO_SET_ULONG(SQL_SP_BETWEEN | SQL_SP_COMPARISON | SQL_SP_EXISTS |
                     SQL_SP_IN | SQL_SP_ISNOTNULL | SQL_SP_ISNULL |
                     SQL_SP_LIKE /*| SQL_SP_MATCH_FULL  |SQL_SP_MATCH_PARTIAL |
                                 SQL_SP_MATCH_UNIQUE_FULL | SQL_SP_MATCH_UNIQUE_PARTIAL |
                                 SQL_SP_OVERLAPS */ | SQL_SP_QUANTIFIED_COMPARISON /*|
                                 SQL_SP_UNIQUE */);

  case SQL_SQL92_RELATIONAL_JOIN_OPERATORS:
    MYINFO_SET_ULONG(SQL_SRJO_INNER_JOIN |
                     SQL_SRJO_LEFT_OUTER_JOIN | SQL_SRJO_NATURAL_JOIN |
                     SQL_SRJO_RIGHT_OUTER_JOIN | SQL_SRJO_FULL_OUTER_JOIN);

    //Not appliable:
    /*
  case SQL_SQL92_REVOKE:
    MYINFO_SET_ULONG(SQL_SR_DELETE_TABLE | SQL_SR_INSERT_COLUMN |
                     SQL_SR_INSERT_TABLE | SQL_SR_REFERENCES_TABLE |
                     SQL_SR_REFERENCES_COLUMN | SQL_SR_SELECT_TABLE |
                     SQL_SR_UPDATE_COLUMN | SQL_SR_UPDATE_TABLE);
    */

  case SQL_SQL92_ROW_VALUE_CONSTRUCTOR:
    MYINFO_SET_ULONG(SQL_SRVC_VALUE_EXPRESSION | SQL_SRVC_NULL |
                     SQL_SRVC_DEFAULT | SQL_SRVC_ROW_SUBQUERY);

    //See 4.7.5 "String Functions and Operators"
  case SQL_SQL92_STRING_FUNCTIONS:
    MYINFO_SET_ULONG(SQL_SSF_LOWER | SQL_SSF_UPPER |
                     SQL_SSF_SUBSTRING | SQL_SSF_TRIM_BOTH |
                     SQL_SSF_TRIM_LEADING | SQL_SSF_TRIM_TRAILING);
    //See 4.7.7 "Selection Functions"
  case SQL_SQL92_VALUE_EXPRESSIONS:
    MYINFO_SET_ULONG(SQL_SVE_CASE | SQL_SVE_CAST | SQL_SVE_COALESCE |
                     SQL_SVE_NULLIF);

  case SQL_STANDARD_CLI_CONFORMANCE:
    MYINFO_SET_ULONG(SQL_SCC_ISO92_CLI);

  case SQL_STATIC_CURSOR_ATTRIBUTES1:
    MYINFO_SET_ULONG(SQL_CA1_NEXT | SQL_CA1_ABSOLUTE | SQL_CA1_RELATIVE |
                     SQL_CA1_LOCK_NO_CHANGE | SQL_CA1_POS_POSITION |
                     SQL_CA1_POS_UPDATE | SQL_CA1_POS_DELETE |
                     SQL_CA1_POS_REFRESH | SQL_CA1_POSITIONED_UPDATE |
                     SQL_CA1_POSITIONED_DELETE | SQL_CA1_BULK_ADD);

  case SQL_STATIC_CURSOR_ATTRIBUTES2:
    MYINFO_SET_ULONG(SQL_CA2_MAX_ROWS_SELECT | SQL_CA2_MAX_ROWS_INSERT |
                     SQL_CA2_MAX_ROWS_DELETE | SQL_CA2_MAX_ROWS_UPDATE |
                     SQL_CA2_CRC_EXACT);

    /*
    I have put all the coincident ISO functions in 4.2.7 "String Operations". There are some more
    that may coincide with SQL_FN_STR_... attributes, but those are not ISO compliant and their
    names may be distinct (check SUBSTRING (SQL_FN_STR_SUBSTRING) vs. SUBSTR DES string function)
    */
  case SQL_STRING_FUNCTIONS:
    MYINFO_SET_ULONG(SQL_FN_STR_CONCAT | SQL_FN_STR_LENGTH);

  case SQL_SUBQUERIES:
    MYINFO_SET_ULONG(SQL_SQ_CORRELATED_SUBQUERIES | SQL_SQ_COMPARISON |
                     SQL_SQ_EXISTS | SQL_SQ_IN | SQL_SQ_QUANTIFIED);

  case SQL_SYSTEM_FUNCTIONS:
    MYINFO_SET_ULONG(0);

  case SQL_TABLE_TERM:
    MYINFO_SET_STR("table");

  case SQL_TIMEDATE_ADD_INTERVALS:
  case SQL_TIMEDATE_DIFF_INTERVALS:
    MYINFO_SET_ULONG(0);

  case SQL_TIMEDATE_FUNCTIONS:
    MYINFO_SET_ULONG(SQL_FN_TD_CURRENT_DATE | SQL_FN_TD_CURRENT_TIME |
                     SQL_FN_TD_CURRENT_TIMESTAMP | SQL_FN_TD_EXTRACT | SQL_FN_TD_HOUR | SQL_FN_TD_MINUTE | SQL_FN_TD_MONTH |
                     SQL_FN_TD_QUARTER | SQL_FN_TD_SECOND | SQL_FN_TD_YEAR);

    
  case SQL_TXN_CAPABLE:
    MYINFO_SET_USHORT(SQL_TC_NONE);

    //Not appliable
    /*
  case SQL_TXN_ISOLATION_OPTION:
    MYINFO_SET_ULONG(SQL_TXN_READ_COMMITTED);
    */
  case SQL_UNION:
    MYINFO_SET_ULONG(SQL_U_UNION | SQL_U_UNION_ALL);

    //Not appliable
    /*
  case SQL_USER_NAME:
    MYINFO_SET_STR(dbc->ds.opt_UID);
    */
  case SQL_XOPEN_CLI_YEAR:
    MYINFO_SET_STR("1992");


    //These are not supported in DES.
    /*
  case SQL_ACCESSIBLE_PROCEDURES:
  case SQL_ACCESSIBLE_TABLES:
    MYINFO_SET_STR("N");
    */

  case SQL_LOCK_TYPES:
    MYINFO_SET_ULONG(0);

  case SQL_OUTER_JOINS:
    MYINFO_SET_STR("Y");

  case SQL_POSITIONED_STATEMENTS:
    MYINFO_SET_ULONG(SQL_PS_POSITIONED_DELETE | SQL_PS_POSITIONED_UPDATE);

  case SQL_SCROLL_CONCURRENCY:
    MYINFO_SET_ULONG(SQL_SCCO_LOCK);

  case SQL_STATIC_SENSITIVITY:
    MYINFO_SET_ULONG(SQL_SS_ADDITIONS | SQL_SS_DELETIONS | SQL_SS_UPDATES);

  case SQL_FETCH_DIRECTION:
    MYINFO_SET_ULONG(SQL_FD_FETCH_NEXT | SQL_FD_FETCH_FIRST |
                      SQL_FD_FETCH_LAST | SQL_FD_FETCH_PRIOR |
                       SQL_FD_FETCH_ABSOLUTE | SQL_FD_FETCH_RELATIVE);

  default:
  {
    char buff[80];
    myodbc_snprintf(buff, sizeof(buff),
                    "Unsupported option: %d to SQLGetInfo",
                    fInfoType);
    return ((DBC *)hdbc)->set_error("HYC00", buff);
  }
  }

  return SQL_SUCCESS;
}


/*
Function sets up a result set containing details of the types
supported by mysql.
*/

DES_FIELD SQL_GET_TYPE_INFO_fields[] = {
    DESODBC_FIELD_STRING("TYPE_NAME", 32, NOT_NULL_FLAG),
    DESODBC_FIELD_SHORT("DATA_TYPE", NOT_NULL_FLAG),
    DESODBC_FIELD_LONG("COLUMN_SIZE", 0),
    DESODBC_FIELD_STRING("LITERAL_PREFIX", 2, 0),
    DESODBC_FIELD_STRING("LITERAL_SUFFIX", 1, 0),
    DESODBC_FIELD_STRING("CREATE_PARAMS", 15, 0),
    DESODBC_FIELD_SHORT("NULLABLE", NOT_NULL_FLAG),
    DESODBC_FIELD_SHORT("CASE_SENSITIVE", NOT_NULL_FLAG),
    DESODBC_FIELD_SHORT("SEARCHABLE", NOT_NULL_FLAG),
    DESODBC_FIELD_SHORT("UNSIGNED_ATTRIBUTE", 0),
    DESODBC_FIELD_SHORT("FIXED_PREC_SCALE", NOT_NULL_FLAG),
    DESODBC_FIELD_SHORT("AUTO_UNIQUE_VALUE", 0),
    DESODBC_FIELD_STRING("LOCAL_TYPE_NAME", 60, 0),
    DESODBC_FIELD_SHORT("MINIMUM_SCALE", 0),
    DESODBC_FIELD_SHORT("MAXIMUM_SCALE", 0),
    DESODBC_FIELD_SHORT("SQL_DATATYPE", NOT_NULL_FLAG),
    DESODBC_FIELD_SHORT("SQL_DATETIME_SUB", 0),
    DESODBC_FIELD_LONG("NUM_PREC_RADIX", 0),
    DESODBC_FIELD_SHORT("INTERVAL_PRECISION", 0),
};

const uint SQL_GET_TYPE_INFO_FIELDS = (uint)array_elements(SQL_GET_TYPE_INFO_fields);

char sql_searchable[6], sql_unsearchable[6], sql_nullable[6], sql_no_nulls[6],
sql_bit[6], sql_tinyint[6], sql_smallint[6], sql_integer[6], sql_bigint[6],
sql_float[6], sql_real[6], sql_double[6], sql_char[6], sql_varchar[6],
sql_longvarchar[6], sql_timestamp[6], sql_decimal[6], sql_numeric[6],
sql_varbinary[6], sql_time[6], sql_date[6], sql_binary[6],
sql_longvarbinary[6], sql_datetime[6], sql_wchar[6], sql_wvarchar[6],
sql_wlongvarchar[6];


/* DESODBC:
    Original author: DESODBC Developer
*/
void ResultTable::insert_SQLGetTypeInfo_cols() {
  insert_cols(SQL_GET_TYPE_INFO_fields,
              array_elements(SQL_GET_TYPE_INFO_fields));
}


/* DESODBC:

    Renamed from the original MySQLGetTypeInfo
    and modified according to DES' needs.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/**
Return information about data types supported by the server.

@param[in] hstmt     Handle of statement
@param[in] fSqlType  SQL data type or @c SQL_ALL_TYPES

@since ODBC 1.0
@since ISO SQL 92
*/
SQLRETURN SQL_API DESGetTypeInfo(SQLHSTMT hstmt, SQLSMALLINT fSqlType)
{
  STMT *stmt = (STMT *)hstmt;
  uint i;
  SQLRETURN error = SQL_SUCCESS;

  DES_SQLFreeStmt(hstmt, FREE_STMT_RESET);

  /* use ODBC2 types if called with ODBC3 types on an ODBC2 handle */
  if (stmt->dbc->env->odbc_ver == SQL_OV_ODBC2) {
    switch (fSqlType) {
      case SQL_TYPE_DATE:
        fSqlType = SQL_DATE;
        break;
      case SQL_TYPE_TIME:
        fSqlType = SQL_TIME;
        break;
      case SQL_TYPE_TIMESTAMP:
        fSqlType = SQL_TIMESTAMP;
        break;
    }
  }

  stmt->params_for_table.type_requested = fSqlType;
  stmt->type = SQLGETTYPEINFO;

  error = stmt->build_results();

  return error;
}


/**
Create strings from some integers for easy initialization of string arrays.

@todo get rid of this. it is evil.
*/
void init_getfunctions(void)
{
  des_int2str(SQL_SEARCHABLE, sql_searchable, -10, 0);
  des_int2str(SQL_UNSEARCHABLE, sql_unsearchable, -10, 0);
  des_int2str(SQL_NULLABLE, sql_nullable, -10, 0);
  des_int2str(SQL_NO_NULLS, sql_no_nulls, -10, 0);
  des_int2str(SQL_BIT, sql_bit, -10, 0);
  des_int2str(SQL_TINYINT, sql_tinyint, -10, 0);
  des_int2str(SQL_SMALLINT, sql_smallint, -10, 0);
  des_int2str(SQL_INTEGER, sql_integer, -10, 0);
  des_int2str(SQL_BIGINT, sql_bigint, -10, 0);
  des_int2str(SQL_FLOAT, sql_float, -10, 0);
  des_int2str(SQL_REAL, sql_real, -10, 0);
  des_int2str(SQL_DOUBLE, sql_double, -10, 0);
  des_int2str(SQL_CHAR, sql_char, -10, 0);
  des_int2str(SQL_VARCHAR, sql_varchar, -10, 0);
  des_int2str(SQL_LONGVARCHAR, sql_longvarchar, -10, 0);
  des_int2str(SQL_TYPE_TIMESTAMP, sql_timestamp, -10, 0);
  des_int2str(SQL_DECIMAL, sql_decimal, -10, 0);
  des_int2str(SQL_NUMERIC, sql_numeric, -10, 0);
  des_int2str(SQL_VARBINARY, sql_varbinary, -10, 0);
  des_int2str(SQL_TYPE_TIME, sql_time, -10, 0);
  des_int2str(SQL_TYPE_DATE, sql_date, -10, 0);
  des_int2str(SQL_LONGVARBINARY, sql_longvarbinary, -10, 0);
  des_int2str(SQL_BINARY, sql_binary, -10, 0);
  des_int2str(SQL_DATETIME, sql_datetime, -10, 0);
  des_int2str(SQL_WCHAR, sql_wchar, -10, 0);
  des_int2str(SQL_WVARCHAR, sql_wvarchar, -10, 0);
  des_int2str(SQL_WLONGVARCHAR, sql_wlongvarchar, -10, 0);
# if (ODBCVER < 0x0300)
  desodbc_sqlstate2_init();
  desodbc_ov2_inited = 1;
# endif
}

/**
Fix some initializations based on the ODBC version.
*/
void desodbc_ov_init(SQLINTEGER odbc_version)
{
  if (odbc_version == SQL_OV_ODBC2)
  {
    des_int2str(SQL_TIMESTAMP, sql_timestamp, -10, 0);
    des_int2str(SQL_DATE, sql_date, -10, 0);
    des_int2str(SQL_TIME, sql_time, -10, 0);
    desodbc_ov2_inited = 1;
  }
  else
  {
    if (!desodbc_ov2_inited)
      return;
    desodbc_ov2_inited = 0;

    des_int2str(SQL_TYPE_TIMESTAMP, sql_timestamp, -10, 0);
    des_int2str(SQL_TYPE_DATE, sql_date, -10, 0);
    des_int2str(SQL_TYPE_TIME, sql_time, -10, 0);
  }
}


/* DESODBC:

    Modified according to DES' needs.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/**
List of functions supported in the driver.
*/
SQLUSMALLINT desodbc3_functions[] =
{
    //Commented: functions not available due to DES restrictions.
  SQL_API_SQLALLOCHANDLE,
  SQL_API_SQLBINDCOL,
  SQL_API_SQLBINDPARAM,
  //SQL_API_SQLCANCEL,
#ifndef USE_IODBC
  //SQL_API_SQLCANCELHANDLE,
#else
  SQL_API_SQLGETSTMTOPTION,
  SQL_API_SQLSETSTMTOPTION,
  SQL_API_SQLPARAMOPTIONS,
#endif
  SQL_API_SQLCLOSECURSOR,
  SQL_API_SQLCOLATTRIBUTE,
  SQL_API_SQLCOLUMNS,
  SQL_API_SQLCONNECT,
  SQL_API_SQLCOPYDESC,
  SQL_API_SQLDATASOURCES,
  SQL_API_SQLDESCRIBECOL,
  SQL_API_SQLDISCONNECT,
  //SQL_API_SQLENDTRAN,
  SQL_API_SQLEXECDIRECT,
  SQL_API_SQLEXECUTE,
  SQL_API_SQLFETCH,
  SQL_API_SQLFETCHSCROLL,
  SQL_API_SQLFREEHANDLE,
  SQL_API_SQLFREESTMT,
  SQL_API_SQLGETCONNECTATTR,
  SQL_API_SQLGETCURSORNAME,
  SQL_API_SQLGETDATA,
  SQL_API_SQLGETDESCFIELD,
  SQL_API_SQLGETDESCREC,
  SQL_API_SQLGETDIAGFIELD,
  SQL_API_SQLGETDIAGREC,
  SQL_API_SQLGETENVATTR,
  SQL_API_SQLGETFUNCTIONS,
  SQL_API_SQLGETINFO,
  SQL_API_SQLGETSTMTATTR,
  SQL_API_SQLGETTYPEINFO,
  SQL_API_SQLNUMRESULTCOLS,
  //SQL_API_SQLPARAMDATA,
  SQL_API_SQLPREPARE,
  //SQL_API_SQLPUTDATA,
  SQL_API_SQLROWCOUNT,
  SQL_API_SQLSETCONNECTATTR,
  SQL_API_SQLSETCURSORNAME,
  SQL_API_SQLSETDESCFIELD,
  SQL_API_SQLSETDESCREC,
  SQL_API_SQLSETENVATTR,
  SQL_API_SQLSETSTMTATTR,
  SQL_API_SQLSPECIALCOLUMNS,
  SQL_API_SQLSTATISTICS,
  SQL_API_SQLTABLES,
  SQL_API_SQLBULKOPERATIONS,
  SQL_API_SQLBINDPARAMETER,
  SQL_API_SQLBROWSECONNECT,
  SQL_API_SQLCOLATTRIBUTE,
  //SQL_API_SQLCOLUMNPRIVILEGES ,
  SQL_API_SQLDESCRIBEPARAM,
  SQL_API_SQLDRIVERCONNECT,
  SQL_API_SQLDRIVERS,
  SQL_API_SQLEXTENDEDFETCH,
  SQL_API_SQLFOREIGNKEYS,
  SQL_API_SQLMORERESULTS,
  SQL_API_SQLNATIVESQL,
  SQL_API_SQLNUMPARAMS,
  SQL_API_SQLPRIMARYKEYS,
  //SQL_API_SQLPROCEDURECOLUMNS,
  //SQL_API_SQLPROCEDURES,
  SQL_API_SQLSETPOS
  //SQL_API_SQLTABLEPRIVILEGES
};


/**
Get information on which functions are supported by the driver.

@param[in]  hdbc      Handle of database connection
@param[in]  fFunction Function to check, @c SQL_API_ODBC3_ALL_FUNCTIONS,
or @c SQL_API_ALL_FUNCTIONS
@param[out] pfExists  Pointer to either one @c SQLUSMALLINT or an array
of SQLUSMALLINT for returning results

@since ODBC 1.0
@since ISO SQL 92
*/
SQLRETURN SQL_API SQLGetFunctions(SQLHDBC hdbc __attribute__((unused)),
                                  SQLUSMALLINT fFunction,
                                  SQLUSMALLINT *pfExists)
{
  SQLUSMALLINT index, desodbc_func_size;

  desodbc_func_size = sizeof(desodbc3_functions) / sizeof(desodbc3_functions[0]);

  if (fFunction == SQL_API_ODBC3_ALL_FUNCTIONS)
  {
    /* Clear and set bits in the 4000 bit vector */
    memset(pfExists, 0,
           sizeof(SQLUSMALLINT) * SQL_API_ODBC3_ALL_FUNCTIONS_SIZE);
    for (index = 0; index < desodbc_func_size; ++index)
    {
      SQLUSMALLINT id = desodbc3_functions[index];
      pfExists[id >> 4] |= (1 << (id & 0x000F));
    }
    return SQL_SUCCESS;
  }

  if (fFunction == SQL_API_ALL_FUNCTIONS)
  {
    /* Clear and set elements in the SQLUSMALLINT 100 element array */
    memset(pfExists, 0, sizeof(SQLUSMALLINT) * 100);
    for (index = 0; index < desodbc_func_size; ++index)
    {
      if (desodbc3_functions[index] < 100)
        pfExists[desodbc3_functions[index]] = SQL_TRUE;
    }
    return SQL_SUCCESS;
  }

  *pfExists = SQL_FALSE;
  for (index = 0; index < desodbc_func_size; ++index)
  {
    if (desodbc3_functions[index] == fFunction)
    {
      *pfExists = SQL_TRUE;
      break;
    }
  }

  return SQL_SUCCESS;
}

