// Copyright (c) 2001, 2024, Oracle and/or its affiliates.
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
//
// The authorship of each section of this source file (comments,
// functions and other symbols) belongs to MyODBC unless we
// explicitly state otherwise.
// ---------------------------------------------------------

/***************************************************************************
 * MYUTIL.H								   *
 *									   *
 * @description: Prototype definations needed by the driver		   *
 *									   *
 * @author     : MySQL AB(monty@mysql.com, venu@mysql.com)		   *
 * @date       : 2001-Sep-22						   *
 * @product    : myodbc3						   *
 *									   *
 ****************************************************************************/


#ifndef __MYUTIL_H__
#define __MYUTIL_H__

#include "field_types.h"


//Values extracted from mysql_com.h
#define NOT_NULL_FLAG 1     /**< Field can't be NULL */
#define PRI_KEY_FLAG 2      /**< Field is part of a primary key */
#define UNIQUE_KEY_FLAG 4   /**< Field is part of a unique key */
#define MULTIPLE_KEY_FLAG 8 /**< Field is part of a key */
#define BLOB_FLAG 16        /**< Field is a blob */
#define UNSIGNED_FLAG 32    /**< Field is unsigned */
#define ZEROFILL_FLAG 64    /**< Field is zerofill */
#define BINARY_FLAG 128     /**< Field is binary   */
#define AUTO_INCREMENT_FLAG 512 /**< field is a autoincrement field */

/*
  Utility macros
*/

#define if_forward_cache(st) ((st)->stmt_options.cursor_type == SQL_CURSOR_FORWARD_ONLY && \
           (st)->dbc->ds.opt_NO_CACHE)
#define is_connected(dbc)    ((dbc)->connected)
#define reset_ptr(x) {if (x) x= 0;}
#define digit(A) ((int) (A - '0'))

/* truncation types in SQL_NUMERIC_STRUCT conversions */
#define SQLNUM_TRUNC_FRAC 1
#define SQLNUM_TRUNC_WHOLE 2

/* Conversion to SQL_TIMESTAMP_STRUCT errors(str_to_ts) */
#define SQLTS_NULL_DATE -1
#define SQLTS_BAD_DATE -2

/* Sizes of buffer for converion of 4 and 8 bytes integer values*/
#define MAX32_BUFF_SIZE 11
#define MAX64_BUFF_SIZE 21

#define des_int2str(val, dst, radix, upcase) \
    desodbc_int10_to_str((val), (dst), (radix))

#if LIBMYSQL_VERSION_ID >= 50100
typedef unsigned char * DYNAMIC_ELEMENT;
#else
typedef char * DYNAMIC_ELEMENT;
#endif

// Handle the removal of `def` and `def_length`
// from DES_FIELD struct in MySQL 8.3.0
// (see WL#16221 and WL#16383)

/*
  Utility function prototypes that share among files
*/

#ifdef _WIN32
/* DESODBC:
    Original author: DESODBC Developer
*/
void try_close(HANDLE h);
#else
/* DESODBC:
    Original author: DESODBC Developer
*/
void try_close(int fd);
#endif

#ifdef _WIN32
/* DESODBC:
    Original author: DESODBC Developer
*/
std::string GetLastWinErrMessage();
#endif

/* DESODBC:
    Original author: DESODBC Developer
*/
std::string convert2identifier(const std::string &arg);

/* DESODBC:
    Original author: DESODBC Developer
*/
std::string get_prepared_arg(STMT *stmt, SQLCHAR *name, SQLSMALLINT len);

/* DESODBC:
    Original author: DESODBC Developer
*/
std::string get_catalog(STMT *stmt, SQLCHAR *name, SQLSMALLINT len);

/* DESODBC:
    Original author: DESODBC Developer
*/
std::string odbc_pattern_to_regex_pattern(const std::string &odbc_pattern);


/* DESODBC:
    Original author: DESODBC Developer
*/
std::vector<std::string> search_odbc_pattern(
    const std::string &pattern, const std::vector<std::string> &v_str);


/* DESODBC:
    Original author: DESODBC Developer
*/
bool is_character_sql_data_type(SQLSMALLINT sql_type);

/* DESODBC:
    Original author: DESODBC Developer
*/
bool is_decimal_des_data_type(enum_field_types type);

/* DESODBC:
    Original author: DESODBC Developer
*/
bool is_numeric_des_data_type(enum_field_types type);

/* DESODBC:
    Original author: DESODBC Developer
*/
bool is_character_des_data_type(enum_field_types type);

/* DESODBC:
    Original author: DESODBC Developer
*/
bool is_time_des_data_type(enum_field_types type);

/* DESODBC:
    Original author: DESODBC Developer
*/
std::vector<std::string> get_attrs(const std::string &str);

/* DESODBC:
    Original author: DESODBC Developer
*/
int des_type_2_sql_type(enum_field_types des_type);

/* DESODBC:
    Original author: DESODBC Developer
*/
std::string des_type_2_str(enum_field_types des_type);

/* DESODBC:
    Original author: DESODBC Developer
*/
SQLCHAR *str_to_sqlcharptr(const std::string &str);

/* DESODBC:
    Original author: DESODBC Developer
*/
char *sqlcharptr_to_charptr(SQLCHAR *sql_str, SQLUSMALLINT sql_str_len);

/* DESODBC:
    Original author: DESODBC Developer
*/
std::string sqlcharptr_to_str(SQLCHAR *sql_str, SQLUSMALLINT sql_str_len);

/* DESODBC:
    Original author: DESODBC Developer
*/
bool is_bulkable_statement(const std::string& query);

/* DESODBC:
    Original author: DESODBC Developer
*/
bool is_in_string(const std::string &str, const std::string &search);

/* DESODBC:
    Original author: DESODBC Developer
*/
TypeAndLength get_Type_from_str(const std::string &str);

/* DESODBC:
    Original author: DESODBC Developer
*/
SQLULEN get_type_size(enum_field_types type);

/* DESODBC:
    Original author: DESODBC Developer
*/
std::string Type_to_type_str(TypeAndLength type);

/* DESODBC:
    Original author: DESODBC Developer
*/
SQLULEN get_Type_size(TypeAndLength type);

/* DESODBC:
    Original author: DESODBC Developer
*/
TypeAndLength get_Type(enum_field_types type);

/* DESODBC:
    Original author: DESODBC Developer
*/
char *string_to_char_pointer(const std::string &str);

/* DESODBC:
    Original author: DESODBC Developer
*/
wchar_t *string_to_wchar_pointer(const std::wstring &w_str);

/* DESODBC:
    Original author: DESODBC Developer
*/
std::vector<std::string> getLines(const std::string &str);

/* DESODBC:
    Original author: DESODBC Developer
*/
std::vector<std::string> convertArrayNotationToStringVector(std::string str);

/* DESODBC:
    This function sets possible errors
    given the TAPI output.

    Original author: DESODBC Developer
*/
SQLRETURN set_error_from_tapi_output(SQLSMALLINT HandleType, SQLHANDLE Handle,
                                     const std::string &tapi_output);

/* DESODBC:
    This function checks the TAPI output and sets
    errors if so.

    Original author: DESODBC Developer
*/
SQLRETURN check_and_set_errors(SQLSMALLINT HandleType, SQLHANDLE Handle,
                               const std::string &tapi_output);


/* DESODBC:
    Original author: DESODBC Developer
*/
std::vector<std::string> filter_candidates(std::vector<std::string> &candidates,
                                           const std::string &key,
                                           bool metadata_id);

/* DESODBC:
    Original author: DESODBC Developer
*/
inline const std::vector<enum_field_types> supported_types = {
    DES_TYPE_VARCHAR,  DES_TYPE_STRING,   DES_TYPE_CHAR_N,  DES_TYPE_VARCHAR_N,
    DES_TYPE_INTEGER,  DES_TYPE_CHAR,     DES_TYPE_INTEGER, DES_TYPE_INT,
    DES_TYPE_FLOAT,    DES_TYPE_REAL,     DES_TYPE_DATE,    DES_TYPE_TIME,
    DES_TYPE_DATETIME, DES_TYPE_TIMESTAMP};

/* DESODBC:
    Original author: DESODBC Developer
*/
inline const std::unordered_map<std::string, SQLSMALLINT> typestr_sqltype_map = {
    {"varchar()", SQL_VARCHAR},
    {"string", SQL_LONGVARCHAR},
    {"varchar", SQL_LONGVARCHAR},
    {"char", SQL_CHAR},  // we will use it with size_str=1
    {"char()", SQL_CHAR},
    {"integer_des", SQL_BIGINT},
    {"int", SQL_BIGINT},
    {"float", SQL_DOUBLE},
    {"real", SQL_DOUBLE},
    {"date", SQL_TYPE_DATE},
    {"time", SQL_TYPE_TIME},
    {"datetime", SQL_TYPE_TIMESTAMP},
    {"timestamp", SQL_TYPE_TIMESTAMP}};

/* DESODBC:
    Original author: DESODBC Developer
*/
inline const std::unordered_map<std::string, enum_field_types> typestr_simpletype_map =
    {{"varchar()", DES_TYPE_VARCHAR_N},
     {"string", DES_TYPE_STRING},
     {"varchar", DES_TYPE_VARCHAR},
     {"char", DES_TYPE_CHAR},  // we will use it with size_str=1
     {"char()", DES_TYPE_CHAR_N},
     //{"integer", DES_TYPE_INTEGER},
     {"int", DES_TYPE_INT},
     {"float", DES_TYPE_FLOAT},
     //{"real", DES_TYPE_REAL},
     {"datetime(date)", DES_TYPE_DATE},
     {"datetime(time)", DES_TYPE_TIME},
     {"datetime(datetime)", DES_TYPE_DATETIME}};

/* DESODBC:
    Original author: DESODBC Developer
*/
inline const std::vector<std::string> supported_table_types = {"TABLE", "VIEW"};

/* DESODBC:
    Renamed from the original my_SQLPrepare()
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN         DES_SQLPrepare (SQLHSTMT hstmt, SQLCHAR *szSqlStr,
                                 SQLINTEGER cbSqlStr,
                                 bool reset_select_limit,
                                 bool force_prepare);

/* DESODBC:

    Renamed from the original my_SQLExecute and modified
    according to DES' needs.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN         DES_SQLExecute         (STMT * stmt);

/* DESODBC:
    Renamed from the original my_SQLFreeStmt()
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DES_SQLFreeStmt        (SQLHSTMT hstmt,SQLUSMALLINT fOption);

/* DESODBC:

    Renamed from the original my_SQLFreeStmtExtended
    and modified according to DESODBC's needs.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DES_SQLFreeStmtExtended(SQLHSTMT hstmt, SQLUSMALLINT fOption,
                                         SQLUSMALLINT fExtra);

/* DESODBC:
    Renamed from the original my_SQLAllocStmt
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DES_SQLAllocStmt       (SQLHDBC hdbc,SQLHSTMT *phstmt);


SQLRETURN         insert_params         (STMT *stmt, SQLULEN row, std::string &finalquery);
void      fix_result_types  (STMT *stmt);

/* DESODBC:
    Renamed from the original my_pos_delete_std
    and modified according to DES' needs.
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN des_pos_delete_std (STMT *stmt,STMT *stmtParam,
                        SQLUSMALLINT irow, std::string &str);

/* DESODBC:
    Renamed from the original my_pos_update_std.
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN des_pos_update_std (STMT *stmt,STMT *stmtParam,
                        SQLUSMALLINT irow, std::string &str);

char *    check_if_positioned_cursor_exists (STMT *stmt, STMT **stmtNew);
SQLRETURN insert_param  (STMT *stmt, DES_BIND *bind, DESC *apd,
                        DESCREC *aprec, DESCREC *iprec, SQLULEN row);



SQLRETURN
copy_ansi_result(STMT *stmt,
                 SQLCHAR *result, SQLLEN result_bytes, SQLLEN *used_bytes,
                 DES_FIELD *field, char *src, unsigned long src_bytes);
SQLRETURN
copy_binary_result(STMT *stmt,
                   SQLCHAR *result, SQLLEN result_bytes, SQLLEN *used_bytes,
                   DES_FIELD *field, char *src, unsigned long src_bytes);
template <typename T>
SQLRETURN copy_binhex_result(STMT *stmt,
           T *rgbValue, SQLINTEGER cbValueMax,
           SQLLEN *pcbValue, char *src,
           ulong src_length);
SQLRETURN wcopy_bit_result(STMT *stmt,
                          SQLWCHAR *result, SQLLEN result_bytes, SQLLEN *used_bytes,
                          DES_FIELD *field, char *src, unsigned long src_bytes);
SQLRETURN copy_wchar_result(STMT *stmt,
                            SQLWCHAR *rgbValue, SQLINTEGER cbValueMax,
                            SQLLEN *pcbValue, DES_FIELD *field, char *src,
                            long src_length);

SQLRETURN set_desc_error  (DESC *desc, char *state,
                          const char *message);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLSMALLINT get_sql_data_type           (STMT *stmt, DES_FIELD *field, char *buff);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLULEN     get_column_size             (STMT *stmt, DES_FIELD *field);

SQLULEN     fill_column_size_buff       (char *buff, STMT *stmt, DES_FIELD *field);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLSMALLINT get_decimal_digits          (STMT *stmt, DES_FIELD *field);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLLEN get_transfer_octet_length(TypeAndLength tal);

/* DESODBC:
    Implemented for DES using the mapping DES data type
    -> SQL data type -> length, as defined in that url.
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLLEN get_transfer_octet_length(STMT *stmt, DES_FIELD *field);

SQLLEN      fill_transfer_oct_len_buff  (char *buff, STMT *stmt, DES_FIELD *field);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLLEN      get_display_size            (STMT *stmt, DES_FIELD *field);

SQLSMALLINT get_dticode_from_concise_type       (SQLSMALLINT concise_type);
SQLSMALLINT get_concise_type_from_datetime_code (SQLSMALLINT dticode);
SQLSMALLINT get_concise_type_from_interval_code (SQLSMALLINT dticode);
SQLSMALLINT get_type_from_concise_type          (SQLSMALLINT concise_type);
SQLLEN      get_bookmark_value                  (SQLSMALLINT fCType, SQLPOINTER rgbValue);

#define is_char_sql_type(type) \
  ((type) == SQL_CHAR || (type) == SQL_VARCHAR || (type) == SQL_LONGVARCHAR)
#define is_wchar_sql_type(type) \
  ((type) == SQL_WCHAR || (type) == SQL_WVARCHAR || (type) == SQL_WLONGVARCHAR)
#define is_binary_sql_type(type) \
  ((type) == SQL_BINARY || (type) == SQL_VARBINARY || \
   (type) == SQL_LONGVARBINARY)

/* DESODBC:
    Original author: DESODBC Developer
*/
#define is_numeric_des_type(field) \
  ((field)->type == DES_TYPE_INTEGER || (field)->type == DES_TYPE_INT || \
   (field)->type == DES_TYPE_FLOAT || \
   ((field)->type == DES_TYPE_REAL)

/* DESODBC:
    Original author: DESODBC Developer
*/
#define is_character_des_type(field)                                       \
  ((field)->type == DES_TYPE_VARCHAR || (field)->type == DES_TYPE_STRING || \
   (field)->type == DES_TYPE_CHAR_N || \
   ((field)->type == DES_TYPE_VARCHAR_N)


/* DESODBC:
    Renamed from the original my_SQLBindParameter
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DES_SQLBindParameter(SQLHSTMT hstmt,SQLUSMALLINT ipar,
              SQLSMALLINT fParamType,
              SQLSMALLINT fCType, SQLSMALLINT fSqlType,
              SQLULEN cbColDef,
              SQLSMALLINT ibScale,
              SQLPOINTER  rgbValue,
              SQLLEN cbValueMax,
              SQLLEN *pcbValue);

/* DESODBC:
    Renamed from the original my_SQLExtendedFetch
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DES_SQLExtendedFetch(SQLHSTMT hstmt, SQLUSMALLINT fFetchType,
              SQLLEN irow, SQLULEN *pcrow,
              SQLUSMALLINT *rgfRowStatus, my_bool upd_status);
SQLRETURN SQL_API myodbc_single_fetch( SQLHSTMT hstmt, SQLUSMALLINT fFetchType,
              SQLLEN irow, SQLULEN *pcrow,
              SQLUSMALLINT *rgfRowStatus, my_bool upd_status);
SQLRETURN SQL_API sql_get_data(STMT *stmt, SQLSMALLINT fCType,
              uint column_number, SQLPOINTER rgbValue,
              SQLLEN cbValueMax, SQLLEN *pcbValue,
              char *value, ulong length, DESCREC *arrec);
SQLRETURN SQL_API sql_get_bookmark_data(STMT *stmt, SQLSMALLINT fCType,
              uint column_number, SQLPOINTER rgbValue,
              SQLLEN cbValueMax, SQLLEN *pcbValue,
              char *value, ulong length, DESCREC *arrec);

void fix_padded_length(STMT *stmt, SQLSMALLINT fCType,
              SQLLEN cbValueMax, SQLLEN *pcbValue, ulong data_len,
              DESCREC *irrec);

/* DESODBC:
    This function corresponds to the
    implementation of SQLSetPos, which was
    my_SQLSetPos in MyODBC. We have reused much of the
    original function.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DES_SQLSetPos(SQLHSTMT hstmt, SQLSETPOSIROW irow,
                               SQLUSMALLINT fOption, SQLUSMALLINT fLock);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
int       unireg_to_c_datatype  (DES_FIELD *field);


int       default_c_type        (int sql_data_type);
ulong     bind_length           (int sql_data_type,ulong length);
my_bool   str_to_date           (SQL_DATE_STRUCT *rgbValue, const char *str,
                                uint length, int zeroToMin);
int       str_to_ts             (SQL_TIMESTAMP_STRUCT *ts, const char *str, int len,
                                int zeroToMin, BOOL dont_use_set_locale);
my_bool str_to_time_st          (SQL_TIME_STRUCT *ts, const char *str);
ulong str_to_time_as_long       (const char *str,uint length);
void  init_getfunctions         (void);

/* DESODBC:
    Renamed from the original myodbc_end()
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
void  desodbc_init               (void);


void  desodbc_ov_init            (SQLINTEGER odbc_version);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN set_handle_error          (SQLSMALLINT HandleType, SQLHANDLE handle, const char* state, const char *errtext);

/* DESODBC:
    Renamed from the original my_SQLAllocEnv()
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DES_SQLAllocEnv      (SQLHENV * phenv);

/* DESODBC:

    Renamed from the original my_SQLAllocConnect and
    modified according to DES' needs.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DES_SQLAllocConnect  (SQLHENV henv, SQLHDBC *phdbc);

/* DESODBC:

    Renamed from the original my_SQLFreeConnect and
    modified according to DES' needs.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DES_SQLFreeConnect   (SQLHDBC hdbc);

/* DESODBC:
    Renamed from the original my_SQLFreeEnv()
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DES_SQLFreeEnv       (SQLHENV henv);

/* DESODBC:
    Renamed and modified from the original desodbc_end()
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
void desodbc_end();

void    set_current_cursor_data   (STMT *stmt,SQLUINTEGER irow);
int     myodbc_strcasecmp         (const char *s, const char *t);
int     myodbc_casecmp            (const char *s, const char *t, uint len);


DESCREC*  desc_get_rec            (DESC *desc, int recnum, my_bool expand);

DESC*     desc_alloc              (STMT *stmt, SQLSMALLINT alloc_type,
                                  desc_ref_type ref_type, desc_desc_type desc_type);
void      desc_free               (DESC *desc);
int       desc_find_dae_rec       (DESC *desc);
SQLRETURN
stmt_SQLSetDescField      (STMT *stmt, DESC *desc, SQLSMALLINT recnum,
                          SQLSMALLINT fldid, SQLPOINTER val, SQLINTEGER buflen);
SQLRETURN
stmt_SQLGetDescField      (STMT *stmt, DESC *desc, SQLSMALLINT recnum,
                          SQLSMALLINT fldid, SQLPOINTER valptr,
                          SQLINTEGER buflen, SQLINTEGER *strlen);
SQLRETURN stmt_SQLCopyDesc(STMT *stmt, DESC *src, DESC *dest);

void sqlnum_from_str      (const char *numstr, SQL_NUMERIC_STRUCT *sqlnum,
                          int *overflow_ptr);
void sqlnum_to_str        (SQL_NUMERIC_STRUCT *sqlnum, SQLCHAR *numstr,
                          SQLCHAR **numbegin, SQLCHAR reqprec, SQLSCHAR reqscale,
                          int *truncptr);
void *ptr_offset_adjust   (void *ptr, SQLULEN *bind_offset,
                          SQLINTEGER bind_type, SQLINTEGER default_size,
                          SQLULEN row);


const char *get_fractional_part   (const char * str, int len,
                                  BOOL dont_use_set_locale,
                                  SQLUINTEGER * fraction);
/* Convert MySQL timestamp to full ANSI timestamp format. */
char *          complete_timestamp  (const char * value, ulong length, char buff[21]);
BOOL            myodbc_isspace      (desodbc::CHARSET_INFO* cs, const char * begin, const char *end);
BOOL            myodbc_isnum        (desodbc::CHARSET_INFO* cs, const char * begin, const char *end);

#define NO_OUT_PARAMETERS         0
#define GOT_OUT_PARAMETERS        1
#define GOT_OUT_STREAM_PARAMETERS 2


/* handle.c*/
int           adjust_param_bind_array (STMT *stmt);

void fill_ird_data_lengths (DESC *ird, ulong *lengths, uint fields);

/* my_stmt.c */

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
BOOL              returned_result     (STMT *stmt);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
my_bool           free_current_result (STMT *stmt);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
DES_RESULT *       get_result_metadata (STMT *stmt);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
my_ulonglong      affected_rows       (STMT *stmt);


my_ulonglong      update_affected_rows(STMT *stmt);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
my_ulonglong      num_rows            (STMT *stmt);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
unsigned long*    fetch_lengths       (STMT *stmt);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
DES_ROW_OFFSET  row_seek            (STMT *stmt, DES_ROW_OFFSET offset);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
void              data_seek           (STMT *stmt, my_ulonglong offset);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
DES_ROW_OFFSET  row_tell            (STMT *stmt);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
int               next_result         (STMT *stmt);

#define IGNORE_THROW(A) try{ A; }catch(...){}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
int           get_int     (STMT *stmt, ulong column_number, char *value,
                          ulong length);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
unsigned int  get_uint    (STMT *stmt, ulong column_number, char *value,
                          ulong length);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
long long     get_int64   (STMT *stmt, ulong column_number, char *value,
                          ulong length);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
unsigned long long get_uint64(STMT * stmt, ulong column_number, char* value,
                          ulong length);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
char *        get_string  (STMT *stmt, ulong column_number, char *value,
                          ulong *length, char * buffer);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
double        get_double  (STMT *stmt, ulong column_number, char *value,
                          ulong length);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
BOOL          is_null     (STMT *stmt, ulong column_number, char *value);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN     prepare     (STMT *stmt, char * query, SQLINTEGER query_length,
                           bool reset_sql_limit, bool force_prepare);

void free_result_bind(STMT *stmt);

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
inline
void stmt_result_free(STMT * stmt)
{
  if (!stmt->result)
    return;

  if (stmt->fake_result)
  {
    x_free(stmt->result);
  }

  stmt->result = NULL;
}

#ifdef __WIN__
#define cmp_database(A,B) myodbc_strcasecmp((const char *)(A),(const char *)(B))
#else
#define cmp_database(A,B) strcmp((A),(B))
#endif

/*
  Check if an octet_length_ptr is a data-at-exec field.
  WARNING: This macro evaluates the argument multiple times.
*/
#define IS_DATA_AT_EXEC(X) ((X) && \
                            (*(X) == SQL_DATA_AT_EXEC || \
                             *(X) <= SQL_LEN_DATA_AT_EXEC_OFFSET))

/* Macro evaluates SQLRETURN expression and in case of error pushes it up in callstack */
#define PUSH_ERROR(sqlreturn_expr) do\
                                   {\
                                     SQLRETURN lrc= (sqlreturn_expr);\
                                     if (!SQL_SUCCEEDED(lrc))\
                                       return lrc;\
                                   } while(0)

/* Same as previous, but remembers to given var2rec SQL_SUCCESS_WITH_INFO */
#define PUSH_ERROR_EXT(var2rec,sqlreturn_expr) do\
                                               {\
                                                 SQLRETURN lrc= (sqlreturn_expr);\
                                                 if (!SQL_SUCCEEDED(lrc))\
                                                   return lrc;\
                                                 else if (lrc!=SQL_SUCCESS)\
                                                        var2rec= lrc;\
                                               } while(0)

/* Same as PUSH_ERROR but does not break execution in case of specified 'error' */
#define PUSH_ERROR_UNLESS(sqlreturn_expr, error) do\
                                                 {\
                                                   SQLRETURN lrc= (sqlreturn_expr);\
                                                   if (!SQL_SUCCEEDED(lrc)&&lrc!=error)\
                                                     return lrc;\
                                                 } while(0)

/* Same as PUSH_ERROR_UNLESS but remembers to given var2rec SQL_SUCCESS_WITH_INFO */
#define PUSH_ERROR_UNLESS_EXT(var2rec, sqlreturn_expr, error) do\
                                                 {\
                                                   SQLRETURN lrc= (sqlreturn_expr);\
                                                   if (!SQL_SUCCEEDED(lrc)&&lrc!=error)\
                                                     return lrc;\
                                                   else if (lrc!=SQL_SUCCESS)\
                                                          var2rec= lrc;\
                                                 } while(0)

#define GET_NAME_LEN(S, N, L) L = (L == SQL_NTS ? (N ? (SQLSMALLINT)strlen((char *)N) : 0) : L); \
  if (L > NAME_LEN) \
    return S->set_error("HY090", \
           "One or more parameters exceed the maximum allowed name length");

#define CHECK_HANDLE(h) if (h == NULL) return SQL_INVALID_HANDLE

#define CHECK_DATA_POINTER(S, D, C) if (D == NULL && C != 0 && C != SQL_DEFAULT_PARAM && C != SQL_NULL_DATA) \
                                   return S->set_error("HY009", "Invalid use of NULL pointer");

#define CHECK_STRLEN_OR_IND(S, D, C) if (D != NULL && C < 0 && C != SQL_NTS && C != SQL_NULL_DATA) \
                                   return S->set_error("HY090", "Invalid string or buffer length");

#define CHECK_ENV_OUTPUT(e) if(e == NULL) return SQL_ERROR

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
#define CHECK_DBC_OUTPUT(e, d) \
  if (d == NULL) return ((ENV *)e)->set_error("HY009", "Invalid use of null pointer")

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
#define CHECK_STMT_OUTPUT(d, s) \
  if (s == NULL)                \
  return ((DBC *)d)->set_error("HY009", "Invalid use of null pointer")

#define CHECK_DESC_OUTPUT(d, s) CHECK_STMT_OUTPUT(d, s)

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
#define CHECK_DATA_OUTPUT(s, d) \
  if (d == NULL) return ((STMT *)s)->set_error("HY000", "Invalid output buffer")

#define IF_NOT_NULL(v, x) if (v != NULL) x

#endif /* __MYUTIL_H__ */
