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
@file  info.c
@brief Driver information functions.
*/

#include "driver.h"

#define DESINFO_SET_ULONG(val) \
do { \
  *((SQLUINTEGER *)num_info)= (val); \
  *value_len= sizeof(SQLUINTEGER); \
  return SQL_SUCCESS; \
} while(0)

#define DESINFO_SET_USHORT(val) \
do { \
  *((SQLUSMALLINT *)num_info)= (SQLUSMALLINT)(val); \
  *value_len= sizeof(SQLUSMALLINT); \
  return SQL_SUCCESS; \
} while(0)

#define DESINFO_SET_STR(val) \
do { \
  *char_info= (SQLCHAR *)(val); \
  return SQL_SUCCESS; \
} while(0)

static des_bool desodbc_ov2_inited = 0;


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
    DESINFO_SET_USHORT(0);

  case SQL_AGGREGATE_FUNCTIONS:
    DESINFO_SET_ULONG(SQL_AF_ALL | SQL_AF_AVG | SQL_AF_COUNT | SQL_AF_DISTINCT |
                     SQL_AF_MAX | SQL_AF_MIN | SQL_AF_SUM);

  case SQL_ALTER_DOMAIN:
    DESINFO_SET_ULONG(0);

  case SQL_ALTER_TABLE:
    /** @todo check if we should report more */
    DESINFO_SET_ULONG(SQL_AT_ADD_COLUMN | SQL_AT_DROP_COLUMN);

#ifndef USE_IODBC
  case SQL_ASYNC_DBC_FUNCTIONS:
    DESINFO_SET_ULONG(SQL_ASYNC_DBC_NOT_CAPABLE);
#endif

  case SQL_ASYNC_MODE:
    DESINFO_SET_ULONG(SQL_AM_NONE);

  case SQL_BATCH_ROW_COUNT:
    DESINFO_SET_ULONG(SQL_BRC_EXPLICIT);

  case SQL_BATCH_SUPPORT:
    DESINFO_SET_ULONG(SQL_BS_SELECT_EXPLICIT | SQL_BS_ROW_COUNT_EXPLICIT |
                     SQL_BS_SELECT_PROC | SQL_BS_ROW_COUNT_PROC);

  case SQL_BOOKMARK_PERSISTENCE:
    DESINFO_SET_ULONG(SQL_BP_UPDATE | SQL_BP_DELETE);

  case SQL_CATALOG_LOCATION:
    DESINFO_SET_USHORT(SQL_CL_START);

  case SQL_CATALOG_NAME:
    DESINFO_SET_STR(dbc->ds.opt_NO_CATALOG ? "" : "Y");

  case SQL_CATALOG_NAME_SEPARATOR:
    DESINFO_SET_STR(dbc->ds.opt_NO_CATALOG ? "" : ".");

  case SQL_CATALOG_TERM:
    DESINFO_SET_STR(dbc->ds.opt_NO_CATALOG ? "" : "database");

  case SQL_CATALOG_USAGE:
    DESINFO_SET_ULONG(!dbc->ds.opt_NO_CATALOG ?
      (SQL_CU_DML_STATEMENTS | SQL_CU_PROCEDURE_INVOCATION |
       SQL_CU_TABLE_DEFINITION | SQL_CU_INDEX_DEFINITION |
       SQL_CU_PRIVILEGE_DEFINITION) :
                     0);

  case SQL_COLLATION_SEQ:
    DESINFO_SET_STR(dbc->cxn_charset_info->name);

  case SQL_COLUMN_ALIAS:
    DESINFO_SET_STR("Y");

  case SQL_CONCAT_NULL_BEHAVIOR:
    DESINFO_SET_USHORT(SQL_CB_NULL);

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
    DESINFO_SET_ULONG(SQL_CVT_CHAR | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL |
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
    DESINFO_SET_ULONG(0);

  case SQL_CONVERT_FUNCTIONS:
    /* MySQL's CONVERT() and CAST() functions aren't SQL compliant yet. */
    DESINFO_SET_ULONG(0);

  case SQL_CORRELATION_NAME:
    DESINFO_SET_USHORT(SQL_CN_DIFFERENT);

  case SQL_CREATE_ASSERTION:
  case SQL_CREATE_CHARACTER_SET:
  case SQL_CREATE_COLLATION:
  case SQL_CREATE_DOMAIN:
  case SQL_CREATE_SCHEMA:
    DESINFO_SET_ULONG(0);

  case SQL_CREATE_TABLE:
    DESINFO_SET_ULONG(SQL_CT_CREATE_TABLE | SQL_CT_COMMIT_DELETE |
                     SQL_CT_LOCAL_TEMPORARY | SQL_CT_COLUMN_DEFAULT |
                     SQL_CT_COLUMN_COLLATION);

  case SQL_CREATE_TRANSLATION:
    DESINFO_SET_ULONG(0);

  case SQL_CREATE_VIEW:
    DESINFO_SET_ULONG(SQL_CV_CREATE_VIEW | SQL_CV_CHECK_OPTION |
                      SQL_CV_CASCADED);

  case SQL_CURSOR_COMMIT_BEHAVIOR:
  case SQL_CURSOR_ROLLBACK_BEHAVIOR:
    DESINFO_SET_USHORT(SQL_CB_PRESERVE);

#ifdef SQL_CURSOR_SENSITIVITY
  case SQL_CURSOR_SENSITIVITY:
    DESINFO_SET_ULONG(SQL_UNSPECIFIED);
#endif

#ifdef SQL_CURSOR_ROLLBACK_SQL_CURSOR_SENSITIVITY
  case SQL_CURSOR_ROLLBACK_SQL_CURSOR_SENSITIVITY:
    DESINFO_SET_ULONG(SQL_UNSPECIFIED);
#endif

  case SQL_DATA_SOURCE_NAME:
    DESINFO_SET_STR(dbc->ds.opt_DSN);

  case SQL_DATA_SOURCE_READ_ONLY:
    DESINFO_SET_STR("N");

  case SQL_DATABASE_NAME:
      //TODO: implement appropriately
    if (is_connected(dbc))
      return dbc->set_error("HY000",
                           "SQLGetInfo() failed to return current catalog.",
                           0);
    DESINFO_SET_STR(!dbc->database.empty() ? dbc->database.c_str() : "null");

  case SQL_DATETIME_LITERALS:
    DESINFO_SET_ULONG(SQL_DL_SQL92_DATE | SQL_DL_SQL92_TIME |
                     SQL_DL_SQL92_TIMESTAMP);

  case SQL_DBMS_NAME:
    DESINFO_SET_STR("DES");

  case SQL_DBMS_VER:
    /** @todo technically this is not right: should be ##.##.#### */
    DESINFO_SET_STR("v6.7"); //TODO: check

  case SQL_DDL_INDEX:
    DESINFO_SET_ULONG(SQL_DI_CREATE_INDEX | SQL_DI_DROP_INDEX);

  case SQL_DEFAULT_TXN_ISOLATION:
    DESINFO_SET_ULONG(DEFAULT_TXN_ISOLATION);

  case SQL_DESCRIBE_PARAMETER:
    DESINFO_SET_STR("N");

  case SQL_DRIVER_NAME:
#ifdef MYODBC_UNICODEDRIVER
# ifdef WIN32
    DESINFO_SET_STR("myodbc" DESODBC_STRMAJOR_VERSION "w.dll");
# else
    DESINFO_SET_STR("libmyodbc" DESODBC_STRMAJOR_VERSION "w.so");
# endif
#else
# ifdef WIN32
    DESINFO_SET_STR("myodbc" DESODBC_STRMAJOR_VERSION "a.dll");
# else
    DESINFO_SET_STR("libmyodbc" DESODBC_STRMAJOR_VERSION "a.so");
# endif
#endif
  case SQL_DRIVER_ODBC_VER:
    DESINFO_SET_STR("03.80");               /* What standard we implement */

  case SQL_DRIVER_VER:
    DESINFO_SET_STR(DRIVER_VERSION);

  case SQL_DROP_ASSERTION:
  case SQL_DROP_CHARACTER_SET:
  case SQL_DROP_COLLATION:
  case SQL_DROP_DOMAIN:
  case SQL_DROP_SCHEMA:
  case SQL_DROP_TRANSLATION:
    DESINFO_SET_ULONG(0);

  case SQL_DROP_TABLE:
    DESINFO_SET_ULONG(SQL_DT_DROP_TABLE | SQL_DT_CASCADE | SQL_DT_RESTRICT);

  case SQL_DROP_VIEW:
    DESINFO_SET_ULONG(SQL_DV_DROP_VIEW | SQL_DV_CASCADE | SQL_DV_RESTRICT);

  case SQL_DYNAMIC_CURSOR_ATTRIBUTES1:
    if (!dbc->ds.opt_FORWARD_CURSOR &&
        dbc->ds.opt_DYNAMIC_CURSOR)
      DESINFO_SET_ULONG(SQL_CA1_NEXT | SQL_CA1_ABSOLUTE | SQL_CA1_RELATIVE |
                       SQL_CA1_LOCK_NO_CHANGE | SQL_CA1_POS_POSITION |
                       SQL_CA1_POS_UPDATE | SQL_CA1_POS_DELETE |
                       SQL_CA1_POS_REFRESH | SQL_CA1_POSITIONED_UPDATE |
                       SQL_CA1_POSITIONED_DELETE | SQL_CA1_BULK_ADD);
    else
      DESINFO_SET_ULONG(0);

  case SQL_DYNAMIC_CURSOR_ATTRIBUTES2:
    if (!dbc->ds.opt_FORWARD_CURSOR &&
        dbc->ds.opt_DYNAMIC_CURSOR)
      DESINFO_SET_ULONG(SQL_CA2_SENSITIVITY_ADDITIONS |
                       SQL_CA2_SENSITIVITY_DELETIONS |
                       SQL_CA2_SENSITIVITY_UPDATES |
                       SQL_CA2_MAX_ROWS_SELECT | SQL_CA2_MAX_ROWS_INSERT |
                       SQL_CA2_MAX_ROWS_DELETE | SQL_CA2_MAX_ROWS_UPDATE |
                       SQL_CA2_CRC_EXACT | SQL_CA2_SIMULATE_TRY_UNIQUE);
    else
      DESINFO_SET_ULONG(0);

  case SQL_EXPRESSIONS_IN_ORDERBY:
    DESINFO_SET_STR("Y");

  case SQL_FILE_USAGE:
    DESINFO_SET_USHORT(SQL_FILE_NOT_SUPPORTED);

  case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1:
    DESINFO_SET_ULONG(dbc->ds.opt_FORWARD_CURSOR ?
                     SQL_CA1_NEXT :
                     SQL_CA1_NEXT | SQL_CA1_ABSOLUTE | SQL_CA1_RELATIVE |
                     SQL_CA1_LOCK_NO_CHANGE | SQL_CA1_POS_POSITION |
                     SQL_CA1_POS_UPDATE | SQL_CA1_POS_DELETE |
                     SQL_CA1_POS_REFRESH | SQL_CA1_POSITIONED_UPDATE |
                     SQL_CA1_POSITIONED_DELETE | SQL_CA1_BULK_ADD);

  case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2:
    DESINFO_SET_ULONG(SQL_CA2_MAX_ROWS_SELECT | SQL_CA2_MAX_ROWS_INSERT |
                     SQL_CA2_MAX_ROWS_DELETE | SQL_CA2_MAX_ROWS_UPDATE |
                     (dbc->ds.opt_FORWARD_CURSOR ?
                      0 : SQL_CA2_CRC_EXACT));

  case SQL_GETDATA_EXTENSIONS:
#ifndef USE_IODBC
    DESINFO_SET_ULONG(SQL_GD_ANY_COLUMN | SQL_GD_ANY_ORDER | SQL_GD_BLOCK |
                     SQL_GD_BOUND | SQL_GD_OUTPUT_PARAMS);
#else
    DESINFO_SET_ULONG(SQL_GD_ANY_COLUMN | SQL_GD_ANY_ORDER | SQL_GD_BLOCK |
                     SQL_GD_BOUND);
#endif

  case SQL_GROUP_BY:
    DESINFO_SET_USHORT(SQL_GB_NO_RELATION);

  case SQL_IDENTIFIER_CASE:
    DESINFO_SET_USHORT(SQL_IC_MIXED);

  case SQL_IDENTIFIER_QUOTE_CHAR:
    DESINFO_SET_STR("`");

  case SQL_INDEX_KEYWORDS:
    DESINFO_SET_ULONG(SQL_IK_ALL);

  case SQL_INFO_SCHEMA_VIEWS:
    DESINFO_SET_ULONG(
        SQL_ISV_CHARACTER_SETS | SQL_ISV_COLLATIONS |
        SQL_ISV_COLUMN_PRIVILEGES | SQL_ISV_COLUMNS | SQL_ISV_KEY_COLUMN_USAGE |
        SQL_ISV_REFERENTIAL_CONSTRAINTS |
        /* SQL_ISV_SCHEMATA | */ SQL_ISV_TABLE_CONSTRAINTS |
        SQL_ISV_TABLE_PRIVILEGES | SQL_ISV_TABLES | SQL_ISV_VIEWS);

  case SQL_INSERT_STATEMENT:
    DESINFO_SET_ULONG(SQL_IS_INSERT_LITERALS | SQL_IS_INSERT_SEARCHED |
                     SQL_IS_SELECT_INTO);

  case SQL_INTEGRITY:
    DESINFO_SET_STR("N");

  case SQL_KEYSET_CURSOR_ATTRIBUTES1:
  case SQL_KEYSET_CURSOR_ATTRIBUTES2:
    DESINFO_SET_ULONG(0);

  case SQL_KEYWORDS:
    /*
    These lists were generated by taking the list of reserved words from
    the MySQL Reference Manual (which is, in turn, generated from the source)
    with the pre-reserved ODBC keywords removed.
    */
    DESINFO_SET_STR(
        "ACCESSIBLE,ANALYZE,ASENSITIVE,BEFORE,BIGINT,BINARY,BLOB,"
        "CALL,CHANGE,CONDITION,DATABASE,DATABASES,DAY_HOUR,"
        "DAY_MICROSECOND,DAY_MINUTE,DAY_SECOND,DELAYED,"
        "DETERMINISTIC,DISTINCTROW,DIV,DUAL,EACH,ELSEIF,ENCLOSED,"
        "ESCAPED,EXIT,EXPLAIN,FLOAT4,FLOAT8,FORCE,FULLTEXT,GENERAL,"
        "GET,HIGH_PRIORITY,HOUR_MICROSECOND,HOUR_MINUTE,"
        "HOUR_SECOND,IF,IGNORE,IGNORE_SERVER_IDS,INFILE,INOUT,INT1,"
        "INT2,INT3,INT4,INT8,IO_AFTER_GTIDS,IO_BEFORE_GTIDS,"
        "ITERATE,KEYS,KILL,LEAVE,LIMIT,LINEAR,LINES,LOAD,LOCALTIME,"
        "LOCALTIMESTAMP,LOCK,LONG,LONGBLOB,LONGTEXT,LOOP,"
        "LOW_PRIORITY,SOURCE_BIND,SOURCE_HEARTBEAT_PERIOD,"
        "SOURCE_SSL_VERIFY_SERVER_CERT,MAXVALUE,MEDIUMBLOB,"
        "MEDIUMINT,MEDIUMTEXT,MIDDLEINT,MINUTE_MICROSECOND,"
        "MINUTE_SECOND,MOD,MODIFIES,NO_WRITE_TO_BINLOG,NONBLOCKING,ONE_SHOT,"
        "OPTIMIZE,OPTIONALLY,OUT,OUTFILE,PARTITION,PURGE,RANGE,"
        "READ_ONLY,READS,READ_WRITE,REGEXP,RELEASE,RENAME,REPEAT,"
        "REPLACE,REQUIRE,RESIGNAL,RETURN,RLIKE,SCHEMAS,"
        "SECOND_MICROSECOND,SENSITIVE,SEPARATOR,SHOW,SIGNAL,SLOW,"
        "SPATIAL,SPECIFIC,SQL_AFTER_GTIDS,SQL_BEFORE_GTIDS,"
        "SQL_BIG_RESULT,SQL_CALC_FOUND_ROWS,SQLEXCEPTION,"
        "SQL_SMALL_RESULT,SSL,STARTING,STRAIGHT_JOIN,TERMINATED,"
        "TINYBLOB,TINYINT,TINYTEXT,TRIGGER,UNDO,UNLOCK,UNSIGNED,"
        "USE,UTC_DATE,UTC_TIME,UTC_TIMESTAMP,VARBINARY,"
        "VARCHARACTER,WHILE,X509,XOR,YEAR_MONTH,ZEROFILL");

  case SQL_LIKE_ESCAPE_CLAUSE:
    DESINFO_SET_STR("Y");

  case SQL_MAX_ASYNC_CONCURRENT_STATEMENTS:
    DESINFO_SET_ULONG(0);

  case SQL_MAX_BINARY_LITERAL_LEN:
    DESINFO_SET_ULONG(0);

  // SQL_MAX_QUALIFIER_NAME_LEN is also defined as SQL_MAX_CATALOG_NAME_LEN
  case SQL_MAX_CATALOG_NAME_LEN:
    if (!dbc->ds.opt_NO_CATALOG)
      DESINFO_SET_USHORT(64);
    else
      DESINFO_SET_USHORT(0);

  case SQL_MAX_CHAR_LITERAL_LEN:
    DESINFO_SET_ULONG(0);

  case SQL_MAX_COLUMN_NAME_LEN:
    DESINFO_SET_USHORT(NAME_LEN);

  case SQL_MAX_COLUMNS_IN_GROUP_BY:
    DESINFO_SET_USHORT(0); /* No specific limit */

  case SQL_MAX_COLUMNS_IN_INDEX:
    DESINFO_SET_USHORT(32);

  case SQL_MAX_COLUMNS_IN_ORDER_BY:
    DESINFO_SET_USHORT(0); /* No specific limit */

  case SQL_MAX_COLUMNS_IN_SELECT:
    DESINFO_SET_USHORT(0); /* No specific limit */

  case SQL_MAX_COLUMNS_IN_TABLE:
    DESINFO_SET_USHORT(0); /* No specific limit */

  // SQL_ACTIVE_STATEMENTS in ODBC v1 has the same definition as
  // SQL_MAX_CONCURRENT_ACTIVITIES in ODBC v3.
  case SQL_MAX_CONCURRENT_ACTIVITIES:
    // TODO: Fix Bug#34916959
    DESINFO_SET_USHORT(0); /* No specific limit */

  case SQL_MAX_CURSOR_NAME_LEN:
    DESINFO_SET_USHORT(DES_MAX_CURSOR_LEN);

  case SQL_MAX_DRIVER_CONNECTIONS:
    DESINFO_SET_USHORT(0); /* No specific limit */

  case SQL_MAX_IDENTIFIER_LEN:
    DESINFO_SET_USHORT(NAME_LEN);

  case SQL_MAX_INDEX_SIZE:
    DESINFO_SET_USHORT(3072);

  case SQL_MAX_PROCEDURE_NAME_LEN:
    DESINFO_SET_USHORT(NAME_LEN);

  case SQL_MAX_ROW_SIZE:
    DESINFO_SET_ULONG(0); /* No specific limit */

  case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
    DESINFO_SET_STR("Y");

  // SQL_MAX_OWNER_NAME_LEN is also defined as SQL_MAX_SCHEMA_NAME_LEN
  case SQL_MAX_SCHEMA_NAME_LEN:
    if (!dbc->ds.opt_NO_SCHEMA)
      DESINFO_SET_USHORT(64);
    else
      DESINFO_SET_USHORT(0);

  case SQL_MAX_STATEMENT_LEN:
    DESINFO_SET_ULONG(dbc->net_buffer_len);

  case SQL_MAX_TABLE_NAME_LEN:
    DESINFO_SET_USHORT(NAME_LEN);

  case SQL_MAX_TABLES_IN_SELECT:
    DESINFO_SET_USHORT(63);

  case SQL_MAX_USER_NAME_LEN:
    DESINFO_SET_USHORT(USERNAME_LENGTH);

  case SQL_MULT_RESULT_SETS:
    DESINFO_SET_STR("Y");

  case SQL_MULTIPLE_ACTIVE_TXN:
    DESINFO_SET_STR("Y");

  case SQL_NEED_LONG_DATA_LEN:
    DESINFO_SET_STR("N");

  case SQL_NON_NULLABLE_COLUMNS:
    DESINFO_SET_USHORT(SQL_NNC_NON_NULL);

  case SQL_NULL_COLLATION:
    DESINFO_SET_USHORT(SQL_NC_LOW);

  case SQL_NUMERIC_FUNCTIONS:
    DESINFO_SET_ULONG(SQL_FN_NUM_ABS | SQL_FN_NUM_ACOS | SQL_FN_NUM_ASIN |
                     SQL_FN_NUM_ATAN | SQL_FN_NUM_ATAN2 | SQL_FN_NUM_CEILING |
                     SQL_FN_NUM_COS | SQL_FN_NUM_COT | SQL_FN_NUM_EXP |
                     SQL_FN_NUM_FLOOR | SQL_FN_NUM_LOG | SQL_FN_NUM_MOD |
                     SQL_FN_NUM_SIGN | SQL_FN_NUM_SIN | SQL_FN_NUM_SQRT |
                     SQL_FN_NUM_TAN | SQL_FN_NUM_PI | SQL_FN_NUM_RAND |
                     SQL_FN_NUM_DEGREES | SQL_FN_NUM_LOG10 | SQL_FN_NUM_POWER |
                     SQL_FN_NUM_RADIANS | SQL_FN_NUM_ROUND |
                     SQL_FN_NUM_TRUNCATE);

  case SQL_ODBC_API_CONFORMANCE:
    DESINFO_SET_USHORT(SQL_OAC_LEVEL1);

  case SQL_ODBC_INTERFACE_CONFORMANCE:
    DESINFO_SET_ULONG(SQL_OIC_LEVEL1);

  case SQL_ODBC_SQL_CONFORMANCE:
    DESINFO_SET_USHORT(SQL_OSC_CORE);

  case SQL_OJ_CAPABILITIES:
    DESINFO_SET_ULONG(SQL_OJ_LEFT | SQL_OJ_RIGHT | SQL_OJ_NESTED |
                     SQL_OJ_NOT_ORDERED | SQL_OJ_INNER |
                     SQL_OJ_ALL_COMPARISON_OPS);

  case SQL_ORDER_BY_COLUMNS_IN_SELECT:
    DESINFO_SET_STR("N");

  case SQL_PARAM_ARRAY_ROW_COUNTS:
    DESINFO_SET_ULONG(SQL_PARC_NO_BATCH);

  case SQL_PARAM_ARRAY_SELECTS:
    DESINFO_SET_ULONG(SQL_PAS_NO_BATCH);

  case SQL_PROCEDURE_TERM:
    DESINFO_SET_STR("");

  case SQL_PROCEDURES:
    DESINFO_SET_STR("N");

  case SQL_POS_OPERATIONS:
    if (dbc->ds.opt_FORWARD_CURSOR)
      DESINFO_SET_ULONG(0);
    else
      DESINFO_SET_ULONG(SQL_POS_POSITION | SQL_POS_UPDATE | SQL_POS_DELETE |
                       SQL_POS_ADD | SQL_POS_REFRESH);

  case SQL_QUOTED_IDENTIFIER_CASE:
    DESINFO_SET_USHORT(SQL_IC_SENSITIVE);

  case SQL_ROW_UPDATES:
    DESINFO_SET_STR("N");

  case SQL_SCHEMA_TERM:
    DESINFO_SET_STR((dbc->ds.opt_NO_SCHEMA) ? "" : "database");

  case SQL_SCHEMA_USAGE:
    if (!dbc->ds.opt_NO_SCHEMA)
      DESINFO_SET_ULONG(SQL_SU_DML_STATEMENTS | SQL_SU_PROCEDURE_INVOCATION |
                       SQL_SU_TABLE_DEFINITION | SQL_SU_INDEX_DEFINITION |
                       SQL_SU_PRIVILEGE_DEFINITION);
    else
      DESINFO_SET_ULONG(0);

  case SQL_SCROLL_OPTIONS:
    DESINFO_SET_ULONG(SQL_SO_FORWARD_ONLY |
      (dbc->ds.opt_FORWARD_CURSOR ?
       0 : SQL_SO_STATIC |
       (dbc->ds.opt_DYNAMIC_CURSOR ? SQL_SO_DYNAMIC : 0)));

  case SQL_SEARCH_PATTERN_ESCAPE:
    DESINFO_SET_STR("\\");

  case SQL_SERVER_NAME:
    DESINFO_SET_STR("Servers not supported in DES."); //TODO: think a proper message

  case SQL_SPECIAL_CHARACTERS:
    /* We can handle anything but / and \xff. */
    DESINFO_SET_STR(" !\"#%&'()*+,-.:;<=>?@[\\]^`{|}~");

  case SQL_SQL_CONFORMANCE:
    DESINFO_SET_ULONG(SQL_SC_SQL92_INTERMEDIATE);

  case SQL_SQL92_DATETIME_FUNCTIONS:
    DESINFO_SET_ULONG(SQL_SDF_CURRENT_DATE | SQL_SDF_CURRENT_TIME |
                     SQL_SDF_CURRENT_TIMESTAMP);

  case SQL_SQL92_FOREIGN_KEY_DELETE_RULE:
  case SQL_SQL92_FOREIGN_KEY_UPDATE_RULE:
    DESINFO_SET_ULONG(0);

  case SQL_SQL92_GRANT:
    DESINFO_SET_ULONG(SQL_SG_DELETE_TABLE | SQL_SG_INSERT_COLUMN |
                     SQL_SG_INSERT_TABLE | SQL_SG_REFERENCES_TABLE |
                     SQL_SG_REFERENCES_COLUMN | SQL_SG_SELECT_TABLE |
                     SQL_SG_UPDATE_COLUMN | SQL_SG_UPDATE_TABLE |
                     SQL_SG_WITH_GRANT_OPTION);

  case SQL_SQL92_NUMERIC_VALUE_FUNCTIONS:
    DESINFO_SET_ULONG(SQL_SNVF_BIT_LENGTH | SQL_SNVF_CHAR_LENGTH |
                     SQL_SNVF_CHARACTER_LENGTH | SQL_SNVF_EXTRACT |
                     SQL_SNVF_OCTET_LENGTH | SQL_SNVF_POSITION);

  case SQL_SQL92_PREDICATES:
    DESINFO_SET_ULONG(SQL_SP_BETWEEN | SQL_SP_COMPARISON | SQL_SP_EXISTS |
                     SQL_SP_IN | SQL_SP_ISNOTNULL | SQL_SP_ISNULL |
                     SQL_SP_LIKE /*| SQL_SP_MATCH_FULL  |SQL_SP_MATCH_PARTIAL |
                                 SQL_SP_MATCH_UNIQUE_FULL | SQL_SP_MATCH_UNIQUE_PARTIAL |
                                 SQL_SP_OVERLAPS */ | SQL_SP_QUANTIFIED_COMPARISON /*|
                                 SQL_SP_UNIQUE */);

  case SQL_SQL92_RELATIONAL_JOIN_OPERATORS:
    DESINFO_SET_ULONG(SQL_SRJO_CROSS_JOIN | SQL_SRJO_INNER_JOIN |
                     SQL_SRJO_LEFT_OUTER_JOIN | SQL_SRJO_NATURAL_JOIN |
                     SQL_SRJO_RIGHT_OUTER_JOIN);

  case SQL_SQL92_REVOKE:
    DESINFO_SET_ULONG(SQL_SR_DELETE_TABLE | SQL_SR_INSERT_COLUMN |
                     SQL_SR_INSERT_TABLE | SQL_SR_REFERENCES_TABLE |
                     SQL_SR_REFERENCES_COLUMN | SQL_SR_SELECT_TABLE |
                     SQL_SR_UPDATE_COLUMN | SQL_SR_UPDATE_TABLE);

  case SQL_SQL92_ROW_VALUE_CONSTRUCTOR:
    DESINFO_SET_ULONG(SQL_SRVC_VALUE_EXPRESSION | SQL_SRVC_NULL |
                     SQL_SRVC_DEFAULT | SQL_SRVC_ROW_SUBQUERY);

  case SQL_SQL92_STRING_FUNCTIONS:
    DESINFO_SET_ULONG(SQL_SSF_CONVERT | SQL_SSF_LOWER | SQL_SSF_UPPER |
                     SQL_SSF_SUBSTRING | SQL_SSF_TRANSLATE | SQL_SSF_TRIM_BOTH |
                     SQL_SSF_TRIM_LEADING | SQL_SSF_TRIM_TRAILING);

  case SQL_SQL92_VALUE_EXPRESSIONS:
    DESINFO_SET_ULONG(SQL_SVE_CASE | SQL_SVE_CAST | SQL_SVE_COALESCE |
                     SQL_SVE_NULLIF);

  case SQL_STANDARD_CLI_CONFORMANCE:
    DESINFO_SET_ULONG(SQL_SCC_ISO92_CLI);

  case SQL_STATIC_CURSOR_ATTRIBUTES1:
    DESINFO_SET_ULONG(SQL_CA1_NEXT | SQL_CA1_ABSOLUTE | SQL_CA1_RELATIVE |
                     SQL_CA1_LOCK_NO_CHANGE | SQL_CA1_POS_POSITION |
                     SQL_CA1_POS_UPDATE | SQL_CA1_POS_DELETE |
                     SQL_CA1_POS_REFRESH | SQL_CA1_POSITIONED_UPDATE |
                     SQL_CA1_POSITIONED_DELETE | SQL_CA1_BULK_ADD);

  case SQL_STATIC_CURSOR_ATTRIBUTES2:
    DESINFO_SET_ULONG(SQL_CA2_MAX_ROWS_SELECT | SQL_CA2_MAX_ROWS_INSERT |
                     SQL_CA2_MAX_ROWS_DELETE | SQL_CA2_MAX_ROWS_UPDATE |
                     SQL_CA2_CRC_EXACT);

  case SQL_STRING_FUNCTIONS:
    DESINFO_SET_ULONG(SQL_FN_STR_ASCII | SQL_FN_STR_BIT_LENGTH |
                     SQL_FN_STR_CHAR | SQL_FN_STR_CHAR_LENGTH |
                     SQL_FN_STR_CONCAT | SQL_FN_STR_INSERT | SQL_FN_STR_LCASE |
                     SQL_FN_STR_LEFT | SQL_FN_STR_LENGTH | SQL_FN_STR_LOCATE |
                     SQL_FN_STR_LOCATE_2 | SQL_FN_STR_LTRIM |
                     SQL_FN_STR_OCTET_LENGTH | SQL_FN_STR_POSITION |
                     SQL_FN_STR_REPEAT | SQL_FN_STR_REPLACE | SQL_FN_STR_RIGHT |
                     SQL_FN_STR_RTRIM | SQL_FN_STR_SOUNDEX | SQL_FN_STR_SPACE |
                     SQL_FN_STR_SUBSTRING | SQL_FN_STR_UCASE);

  case SQL_SUBQUERIES:
    DESINFO_SET_ULONG(SQL_SQ_CORRELATED_SUBQUERIES | SQL_SQ_COMPARISON |
                     SQL_SQ_EXISTS | SQL_SQ_IN | SQL_SQ_QUANTIFIED);

  case SQL_SYSTEM_FUNCTIONS:
    DESINFO_SET_ULONG(SQL_FN_SYS_DBNAME | SQL_FN_SYS_IFNULL |
                     SQL_FN_SYS_USERNAME);

  case SQL_TABLE_TERM:
    DESINFO_SET_STR("table");

  case SQL_TIMEDATE_ADD_INTERVALS:
  case SQL_TIMEDATE_DIFF_INTERVALS:
    DESINFO_SET_ULONG(0);

  case SQL_TIMEDATE_FUNCTIONS:
    DESINFO_SET_ULONG(SQL_FN_TD_CURRENT_DATE | SQL_FN_TD_CURRENT_TIME |
                     SQL_FN_TD_CURRENT_TIMESTAMP | SQL_FN_TD_CURDATE |
                     SQL_FN_TD_CURTIME | SQL_FN_TD_DAYNAME |
                     SQL_FN_TD_DAYOFMONTH | SQL_FN_TD_DAYOFWEEK |
                     SQL_FN_TD_DAYOFYEAR | SQL_FN_TD_EXTRACT | SQL_FN_TD_HOUR |
                     /* SQL_FN_TD_JULIAN_DAY | */ SQL_FN_TD_MINUTE |
                     SQL_FN_TD_MONTH | SQL_FN_TD_MONTHNAME | SQL_FN_TD_NOW |
                     SQL_FN_TD_QUARTER | SQL_FN_TD_SECOND |
                     /*SQL_FN_TD_SECONDS_SINCE_MIDNIGHT | */
                     SQL_FN_TD_TIMESTAMPADD | SQL_FN_TD_TIMESTAMPDIFF |
                     SQL_FN_TD_WEEK | SQL_FN_TD_YEAR);

  case SQL_TXN_CAPABLE:
    DESINFO_SET_USHORT(SQL_TC_NONE);

  case SQL_TXN_ISOLATION_OPTION:
    DESINFO_SET_ULONG(SQL_TXN_READ_COMMITTED);

  case SQL_UNION:
    DESINFO_SET_ULONG(SQL_U_UNION | SQL_U_UNION_ALL);

  case SQL_USER_NAME:
    DESINFO_SET_STR(dbc->ds.opt_UID);

  case SQL_XOPEN_CLI_YEAR:
    DESINFO_SET_STR("1992");

    /* The following aren't listed in the MSDN documentation. */

  case SQL_ACCESSIBLE_PROCEDURES:
  case SQL_ACCESSIBLE_TABLES:
    DESINFO_SET_STR("N");

  case SQL_LOCK_TYPES:
    DESINFO_SET_ULONG(0);

  case SQL_OUTER_JOINS:
    DESINFO_SET_STR("Y");

  case SQL_POSITIONED_STATEMENTS:
    if (dbc->ds.opt_FORWARD_CURSOR)
      DESINFO_SET_ULONG(0);
    else
      DESINFO_SET_ULONG(SQL_PS_POSITIONED_DELETE | SQL_PS_POSITIONED_UPDATE);

  case SQL_SCROLL_CONCURRENCY:
    /** @todo this is wrong. */
    DESINFO_SET_ULONG(SQL_SS_ADDITIONS | SQL_SS_DELETIONS | SQL_SS_UPDATES);

  case SQL_STATIC_SENSITIVITY:
    DESINFO_SET_ULONG(SQL_SS_ADDITIONS | SQL_SS_DELETIONS | SQL_SS_UPDATES);

  case SQL_FETCH_DIRECTION:
    if (dbc->ds.opt_FORWARD_CURSOR)
      DESINFO_SET_ULONG(SQL_FD_FETCH_NEXT);
    else
      DESINFO_SET_ULONG(SQL_FD_FETCH_NEXT | SQL_FD_FETCH_FIRST |
                       SQL_FD_FETCH_LAST |
                       (dbc->ds.opt_NO_DEFAULT_CURSOR ? 0 : SQL_FD_FETCH_PRIOR) |
                       SQL_FD_FETCH_ABSOLUTE | SQL_FD_FETCH_RELATIVE);

  case SQL_ODBC_SAG_CLI_CONFORMANCE:
    DESINFO_SET_USHORT(SQL_OSCC_COMPLIANT);

  default:
  {
    char buff[80];
    desodbc_snprintf(buff, sizeof(buff),
                    "Unsupported option: %d to SQLGetInfo",
                    fInfoType);
    return ((DBC*)hdbc)->set_error(DESERR_S1C00, buff, 4000);
  }
  }

  return SQL_SUCCESS;
}


/*
Function sets up a result set containing details of the types
supported by mysql.
*/

const uint SQL_GET_TYPE_INFO_FIELDS = (uint)array_elements(SQL_GET_TYPE_INFO_fields);

char sql_searchable[6], sql_unsearchable[6], sql_nullable[6], sql_no_nulls[6],
sql_bit[6], sql_tinyint[6], sql_smallint[6], sql_integer[6], sql_bigint[6],
sql_float[6], sql_real[6], sql_double[6], sql_char[6], sql_varchar[6],
sql_longvarchar[6], sql_timestamp[6], sql_decimal[6], sql_numeric[6],
sql_varbinary[6], sql_time[6], sql_date[6], sql_binary[6],
sql_longvarbinary[6], sql_datetime[6], sql_wchar[6], sql_wvarchar[6],
sql_wlongvarchar[6];

char *SQL_GET_TYPE_INFO_values[][19] =
{
  /* SQL_BIT= -7 */
  { "bit",sql_bit,"1","'","'",NULL,sql_nullable,"0",sql_searchable,NULL,"0",NULL,"bit",NULL,NULL,sql_bit,NULL,NULL,NULL },

  /* SQL_TINY= -6 */
  { "tinyint",sql_tinyint,"3","'","'",NULL,sql_nullable,"0",sql_searchable,"0","0","0","tinyint",NULL,NULL,sql_tinyint,NULL,"10",NULL },
  { "tinyint unsigned",sql_tinyint,"3","'","'",NULL,sql_nullable,"0",sql_searchable,"1","0","0","tinyint unsigned",NULL,NULL,sql_tinyint,NULL,"10",NULL },
  { "tinyint auto_increment",sql_tinyint,"3","'","'",NULL,sql_no_nulls,"0",sql_searchable,"0","0","1","tinyint auto_increment",NULL,NULL,sql_tinyint, NULL,"10",NULL },
  { "tinyint unsigned auto_increment",sql_tinyint,"3","'","'",NULL,sql_no_nulls, "0",sql_searchable,"1","0","1","tinyint unsigned auto_increment",NULL,NULL, sql_tinyint,NULL,"10",NULL },

  /* SQL_BIGINT= -5 */
  { "bigint",sql_bigint,"19","'","'",NULL,sql_nullable,"0",sql_searchable,"0","0","0","bigint",NULL,NULL,sql_bigint,NULL,"10",NULL },
  { "bigint unsigned",sql_bigint,"20","'","'",NULL,sql_nullable,"0",sql_searchable,"1","0","0","bigint unsigned",NULL,NULL,sql_bigint,NULL,"10",NULL },
  { "bigint auto_increment",sql_bigint,"19","'","'",NULL,sql_no_nulls,"0",sql_searchable,"0","0","1","bigint auto_increment",NULL,NULL,sql_bigint,NULL,"10",NULL },
  { "bigint unsigned auto_increment",sql_bigint,"20","'","'",NULL,sql_no_nulls, "0",sql_searchable,"1","0","1","bigint unsigned auto_increment",NULL,NULL,sql_bigint, NULL,"10",NULL },

  /* SQL_LONGVARBINARY= -4 */
  { "long varbinary",sql_longvarbinary,"16777215","'","'",NULL,sql_nullable,"0",sql_searchable,NULL,"0",NULL,"long varbinary",NULL,NULL,sql_longvarbinary,NULL,NULL,NULL },
  { "blob",sql_longvarbinary,"65535","'","'",NULL,sql_nullable,"0",sql_searchable,NULL,"0",NULL,"blob",NULL,NULL,sql_longvarbinary,NULL,NULL,NULL },
  { "longblob",sql_longvarbinary,"2147483647","'","'",NULL,sql_nullable,"0",sql_searchable,NULL,"0",NULL,"longblob",NULL,NULL,sql_longvarbinary,NULL,NULL,NULL },
  { "tinyblob",sql_longvarbinary,"255","'","'",NULL,sql_nullable,"0",sql_searchable,NULL,"0",NULL,"tinyblob",NULL,NULL,sql_longvarbinary,NULL,NULL,NULL },
  { "mediumblob",sql_longvarbinary,"16777215","'","'",NULL,sql_nullable,"0",sql_searchable,NULL,"0",NULL,"mediumblob",NULL,NULL,sql_longvarbinary,NULL,NULL,NULL },

  /* SQL_VARBINARY= -3 */
  { "varbinary",sql_varbinary,"255","'","'","length",sql_nullable,"0",sql_searchable,NULL,"0",NULL,"varbinary",NULL,NULL,sql_varbinary,NULL,NULL,NULL },
  { "vector()",sql_varbinary,"16382",NULL,NULL,"length",sql_nullable,"0",sql_searchable,NULL,"0",NULL,"vector",NULL,NULL,sql_varbinary,NULL,NULL,NULL },

  /* SQL_BINARY= -2 */
  { "binary",sql_binary,"255","'","'","length",sql_nullable,"0",sql_searchable,NULL,"0",NULL,"binary",NULL,NULL,sql_binary,NULL,NULL,NULL },

  /* SQL_LONGVARCHAR= -1 */
  { "long varchar",sql_longvarchar,"16777215","'","'",NULL,sql_nullable,"0",sql_searchable,NULL,"0",NULL,"mediumtext",NULL,NULL,sql_longvarchar,NULL,NULL,NULL },
  { "text",sql_longvarchar,"65535","'","'",NULL,sql_nullable,"0",sql_searchable,NULL,"0",NULL,"text",NULL,NULL,sql_longvarchar,NULL,NULL,NULL },
  { "mediumtext",sql_longvarchar,"16777215","'","'",NULL,sql_nullable,"0",sql_searchable,NULL,"0",NULL,"mediumtext",NULL,NULL,sql_longvarchar,NULL,NULL,NULL },
  { "longtext",sql_longvarchar,"2147483647","'","'",NULL,sql_nullable,"0",sql_searchable,NULL,"0",NULL,"longtext",NULL,NULL,sql_longvarchar,NULL,NULL,NULL },
  { "tinytext",sql_longvarchar,"255","'","'",NULL,sql_nullable,"0",sql_searchable,NULL,"0",NULL,"tinytext",NULL,NULL,sql_longvarchar,NULL,NULL,NULL },

  // For JSON maximum column length (in characters) is taken to be maximum
  // octet length of LONGTEXT (4G) divided by 4 bytes/character for
  // utf8mb4 encoding.
  { "json",sql_longvarchar,"1073741823","'","'",NULL,sql_nullable,"1","1",NULL,"0",NULL,"json",NULL,NULL,sql_longvarchar,NULL,NULL,NULL },

  { "long varchar",sql_wlongvarchar,"16777215","'","'",NULL,sql_nullable,"0",sql_searchable,NULL,"0",NULL,"mediumtext",NULL,NULL,sql_longvarchar,NULL,NULL,NULL },
  { "text",sql_wlongvarchar,"65535","'","'",NULL,sql_nullable,"0",sql_searchable,NULL,"0",NULL,"text",NULL,NULL,sql_longvarchar,NULL,NULL,NULL },
  { "mediumtext",sql_wlongvarchar,"16777215","'","'",NULL,sql_nullable,"0",sql_searchable,NULL,"0",NULL,"mediumtext",NULL,NULL,sql_longvarchar,NULL,NULL,NULL },
  { "longtext",sql_wlongvarchar,"2147483647","'","'",NULL,sql_nullable,"0",sql_searchable,NULL,"0",NULL,"longtext",NULL,NULL,sql_longvarchar,NULL,NULL,NULL },
  { "tinytext",sql_wlongvarchar,"255","'","'",NULL,sql_nullable,"0",sql_searchable,NULL,"0",NULL,"tinytext",NULL,NULL,sql_longvarchar,NULL,NULL,NULL },

  // For JSON maximum column length (in characters) is taken to be maximum
  // octet length of LONGTEXT (4G) divided by 4 bytes/character for
  // utf8mb4 encoding.
  { "json",sql_wlongvarchar,"1073741823","'","'",NULL,sql_nullable,"1","1",NULL,"0",NULL,"json",NULL,NULL,sql_wlongvarchar,NULL,NULL,NULL},

  /* SQL_CHAR= 1 */
  { "char",sql_char, "255","'","'","length",sql_nullable,"0",sql_searchable,NULL,"0",NULL,"char",NULL,NULL,sql_char,"0",NULL,NULL },
  { "char",sql_wchar,"255","'","'","length",sql_nullable,"0",sql_searchable,NULL,"0",NULL,"char",NULL,NULL,sql_wchar,"0",NULL,NULL },

  /* SQL_NUMERIC= 2 */
  { "numeric",sql_numeric,"19","'","'","precision,scale",sql_nullable,"0",sql_searchable,"0","0","0","numeric","0","19",sql_numeric,NULL,"10",NULL },

  /* SQL_DECIMAL= 3 */
  { "decimal",sql_decimal,"19","'","'","precision,scale",sql_nullable,"0",sql_searchable,"0","0","0","decimal","0","19",sql_decimal,NULL,"10",NULL },

  /* SQL_INTEGER= 4 */
  { "integer",sql_integer,"10","'","'",NULL,sql_nullable,"0",sql_searchable,"0","0","0","integer",NULL,NULL,sql_integer,NULL,"10",NULL },
  { "integer unsigned",sql_integer,"10","'","'",NULL,sql_nullable,"0",sql_searchable,"1","0","0","integer unsigned",NULL,NULL,sql_integer,NULL,"10",NULL },
  { "int",sql_integer,"10","'","'",NULL,sql_nullable,"0",sql_searchable,"0","0","0","integer",NULL,NULL,sql_integer,NULL,"10",NULL },
  { "int unsigned",sql_integer,"10","'","'",NULL,sql_nullable,"0",sql_searchable,"1","0","0","integer unsigned",NULL,NULL,sql_integer,NULL,"10",NULL },
  { "mediumint",sql_integer,"7","'","'",NULL,sql_nullable,"0",sql_searchable,"0","0","0","Medium integer",NULL,NULL,sql_integer,NULL,"10",NULL },
  { "mediumint unsigned",sql_integer,"8","'","'",NULL,sql_nullable,"0",sql_searchable,"1","0","0","Medium integer unsigned",NULL,NULL,sql_integer,NULL,"10",NULL },
  { "integer auto_increment",sql_integer,"10","'","'",NULL,sql_no_nulls,"0",sql_searchable,"0","0","1","integer auto_increment",NULL,NULL,sql_integer, NULL,"10",NULL },
  { "integer unsigned auto_increment",sql_integer,"10","'","'",NULL,sql_no_nulls, "0",sql_searchable,"1","0","1","integer unsigned auto_increment",NULL,NULL, sql_integer,NULL,"10",NULL },
  { "int auto_increment",sql_integer,"10","'","'",NULL,sql_no_nulls,"0",sql_searchable,"0","0","1","integer auto_increment",NULL,NULL,sql_integer, NULL,"10",NULL },
  { "int unsigned auto_increment",sql_integer,"10","'","'",NULL,sql_no_nulls,"0",sql_searchable,"1","0","1","integer unsigned auto_increment",NULL,NULL,sql_integer,NULL,"10",NULL },
  { "mediumint auto_increment",sql_integer,"7","'","'",NULL,sql_no_nulls,"0",sql_searchable,"0","0","1","Medium integer auto_increment",NULL,NULL,sql_integer,NULL,"10",NULL },
  { "mediumint unsigned auto_increment",sql_integer,"8","'","'",NULL,sql_no_nulls, "0",sql_searchable,"1","0","1","Medium integer unsigned auto_increment",NULL,NULL, sql_integer,NULL,"10",NULL },

  /* SQL_SMALLINT= 5 */
  { "smallint",sql_smallint,"5","'","'",NULL,sql_nullable,"0",sql_searchable,"0","0","0","smallint",NULL,NULL,sql_smallint,NULL,"10",NULL },
  { "smallint unsigned",sql_smallint,"5","'","'",NULL,sql_nullable,"0",sql_searchable,"1","0","0","smallint unsigned",NULL,NULL,sql_smallint,NULL,"10",NULL },
  { "smallint auto_increment",sql_smallint,"5","'","'",NULL,sql_no_nulls,"0",sql_searchable,"0","0","1","smallint auto_increment",NULL,NULL,sql_smallint,NULL,"10",NULL },
  { "smallint unsigned auto_increment",sql_smallint,"5","'","'",NULL,sql_no_nulls, "0",sql_searchable,"1","0","1","smallint unsigned auto_increment",NULL,NULL, sql_smallint,NULL,"10",NULL },

  /* SQL_FLOAT= 6 */
  { "double",sql_float,"15","'","'",NULL,sql_nullable,"0",sql_searchable,"0","0","0","double","0","4",sql_float, NULL,"2",NULL },
  { "double auto_increment",sql_float,"15","'","'",NULL,sql_no_nulls,"0",sql_searchable,"0","0","1","double auto_increment","0","4",sql_float,NULL,"2",NULL },

  /* SQL_REAL= 7 */
  { "float",sql_real,"7","'","'",NULL,sql_nullable, "0",sql_unsearchable,"0","0","0","float","0","2",sql_real, NULL,"2",NULL },
  { "float auto_increment",sql_real,"7","'","'",NULL,sql_no_nulls,"0",sql_unsearchable,"0","0","1","float auto_increment","0","2",sql_real,NULL,"2",NULL },

  /* SQL_DOUBLE= 8 */
  { "double",sql_double,"15","'","'",NULL,sql_nullable,"0",sql_searchable,"0","0","0","double","0","4",sql_double,NULL,"2",NULL },
  { "double auto_increment",sql_double,"15","'","'",NULL,sql_no_nulls,"0",sql_searchable,"0","0","1","double auto_increment","0","4",sql_double,NULL,"2",NULL },

  /* SQL_TYPE_DATE= 91 */
  { "date",sql_date,"10","'","'",NULL,sql_nullable,"0",sql_searchable,NULL,"0",NULL,"date",NULL,NULL,sql_datetime,sql_date,NULL,NULL },

  /* SQL_TYPE_TIME= 92 */
  { "time",sql_time,"8","'","'",NULL,sql_nullable,"0",sql_searchable,NULL,"0",NULL,"time",NULL,NULL,sql_datetime,sql_time,NULL,NULL },

  /* YEAR - SQL_SMALLINT */
  { "year",sql_smallint,"4","'","'",NULL,sql_nullable,"0",sql_searchable,"0","0","0","year",NULL,NULL,sql_smallint,NULL,"10",NULL },

  /* SQL_TYPE_TIMESTAMP= 93 */
  { "datetime",sql_timestamp,"21","'","'",NULL,sql_nullable,"0",sql_searchable,NULL,"0",NULL,"datetime","0","0",sql_datetime,sql_timestamp,NULL,NULL },
  { "timestamp",sql_timestamp,"14","'","'",NULL,sql_no_nulls,"0",sql_searchable,NULL,"0",NULL,"timestamp","0","0",sql_datetime,sql_timestamp,NULL,NULL },

  /* SQL_VARCHAR= 12 */
  { "varchar",sql_varchar,"255","'","'","length",sql_nullable,"0",sql_searchable,NULL,"0",NULL,"varchar","0","0",sql_varchar,NULL,"10",NULL },
  { "varchar",sql_wvarchar,"255","'","'","length",sql_nullable,"0",sql_searchable,NULL,"0",NULL,"varchar","0","0",sql_wvarchar,NULL,"10",NULL },

  /* ENUM and SET are not included -- it confuses some applications. */
};

const uint DES_DATA_TYPES = (uint)array_elements(SQL_GET_TYPE_INFO_values);

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

  stmt->type_requested = fSqlType;
  stmt->type = SQLGETTYPEINFO;

  error = stmt->do_local_query();
  stmt->fake_result = 1;

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
    desodbc_sqlstate2_init();
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
    desodbc_sqlstate3_init();
  }
}


/**
List of functions supported in the driver.
*/
SQLUSMALLINT desodbc3_functions[] =
{
    //Commented: functions not available due to DES restrictions.
  SQL_API_SQLALLOCHANDLE,
  SQL_API_SQLBINDCOL,
  SQL_API_SQLBINDPARAM,
  SQL_API_SQLCANCEL,
#ifndef USE_IODBC
  SQL_API_SQLCANCELHANDLE,
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
  SQL_API_SQLENDTRAN,
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
  SQL_API_SQLPARAMDATA,
  SQL_API_SQLPREPARE,
  SQL_API_SQLPUTDATA,
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

