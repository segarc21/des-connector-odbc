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
// ---------------------------------------------------------

/**
  @file driver.h
  @brief Definitions needed by the driver
*/

#ifndef __DRIVER_H__
#define __DRIVER_H__

#include "../MYODBC_CONF.h"
#include "../MYODBC_MYSQL.h"
#include "../MYODBC_ODBC.h"
#include "installer.h"

/* Disable _attribute__ on non-gcc compilers. */
#if !defined(__attribute__) && !defined(__GNUC__) && !defined(__clang__)
#define __attribute__(arg)
#endif

#ifdef APSTUDIO_READONLY_SYMBOLS
#define WIN32 /* Hack for rc files */
#endif

/* Needed for offsetof() CPP macro */
#include <stddef.h>

#ifdef RC_INVOKED
#define stdin
#endif

/* Misc definitions for AIX .. */
#ifndef crid_t
typedef int crid_t;
#endif
#ifndef class_id_t
typedef unsigned int class_id_t;
#endif

using std::nullptr_t;

//DESODBC: added some libraries
#include <iostream>
#include <list>
#include <mutex>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include "error.h"
#include "parse.h"

#define MAX_OUTPUT_WAIT_MS 2000

#define BUFFER_SIZE 4096

#define LOCK_STMT(S) \
  CHECK_HANDLE(S);   \
  std::unique_lock<std::recursive_mutex> slock(((STMT *)S)->lock)
#define LOCK_STMT_DEFER(S)                                        \
  CHECK_HANDLE(S);                                                \
  std::unique_lock<std::recursive_mutex> slock(((STMT *)S)->lock, \
                                               std::defer_lock)
#define DO_LOCK_STMT() slock.lock();

#define LOCK_DBC(D) \
  std::unique_lock<std::recursive_mutex> dlock(((DBC *)D)->lock)
#define LOCK_DBC_DEFER(D)                                        \
  std::unique_lock<std::recursive_mutex> dlock(((DBC *)D)->lock, \
                                               std::defer_lock)
#define DO_LOCK_DBC() dlock.lock();

#define LOCK_ENV(E) std::unique_lock<std::mutex> elock(E->lock)

// SQL_DRIVER_CONNECT_ATTR_BASE is not defined in all driver managers.
// Therefore use a custom constant until it becomes a standard.
#define DES_DRIVER_CONNECT_ATTR_BASE 0x00004000

#define CB_FIDO_GLOBAL DES_DRIVER_CONNECT_ATTR_BASE + 0x00001000
#define CB_FIDO_CONNECTION DES_DRIVER_CONNECT_ATTR_BASE + 0x00001001

#if defined(_WIN32) || defined(WIN32)
#define INTFUNC __stdcall
#define EXPFUNC __stdcall
#if !defined(HAVE_LOCALTIME_R)
#define HAVE_LOCALTIME_R 1
#endif
#else
#define INTFUNC PASCAL
#define EXPFUNC __export CALLBACK
/* Simple macros to make dl look like the Windows library funcs. */
#define HMODULE void *
#define LoadLibrary(library) dlopen((library), RTLD_GLOBAL | RTLD_LAZY)
#define GetProcAddress(module, proc) dlsym((module), (proc))
#define FreeLibrary(module) dlclose((module))
#endif

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
#define ODBC_DRIVER "DES " DESODBC_STRSERIES " Driver"
#define DRIVER_NAME "DES ODBC " DESODBC_STRSERIES " Driver"
#define DRIVER_NONDSN_TAG "DRIVER={DES ODBC " DESODBC_STRSERIES " Driver}"


/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
#if defined(__APPLE__)

#define DRIVER_QUERY_LOGFILE "/tmp/desodbc.sql"

#elif defined(_UNIX_)

#define DRIVER_QUERY_LOGFILE "/tmp/desodbc.sql"

#else

#define DRIVER_QUERY_LOGFILE "desodbc.sql"

#endif

/*
   Internal driver definitions
*/

/* Options for SQLFreeStmt */
#define FREE_STMT_RESET_BUFFERS 1000
#define FREE_STMT_RESET 1001
#define FREE_STMT_CLEAR_RESULT 1
#define FREE_STMT_DO_LOCK 2

#define DES_3_21_PROTOCOL 10 /* OLD protocol */
#define CHECK_IF_ALIVE 1800  /* Seconds between queries for ping */

#define DES_MAX_CURSOR_LEN 18                   /* Max cursor name length */
#define DES_STMT_LEN 1024                       /* Max statement length */
#define MY_STRING_LEN 1024                     /* Max string length */
#define DES_MAX_SEARCH_STRING_LEN NAME_LEN + 10 /* Max search string length */
/* Max Primary keys in a cursor * WHERE clause */
#define MY_MAX_PK_PARTS 32

#ifndef NEAR
#define NEAR
#endif

/* We don't make any assumption about what the default may be. */
#ifndef DEFAULT_TXN_ISOLATION
#define DEFAULT_TXN_ISOLATION 0
#endif

/* For compatibility with old mysql clients - defining error */
#ifndef ER_MUST_CHANGE_PASSWORD_LOGIN
#define ER_MUST_CHANGE_PASSWORD_LOGIN 1820
#endif

#ifndef CR_AUTH_PLUGIN_CANNOT_LOAD_ERROR
#define CR_AUTH_PLUGIN_CANNOT_LOAD_ERROR 2059
#endif

#ifndef SQL_PARAM_DATA_AVAILABLE
#define SQL_PARAM_DATA_AVAILABLE 101
#endif

/* Connection flags to validate after the connection*/
#define CHECK_AUTOCOMMIT_ON 1  /* AUTOCOMMIT_ON */
#define CHECK_AUTOCOMMIT_OFF 2 /* AUTOCOMMIT_OFF */

/* implementation or application descriptor? */
typedef enum { DESC_IMP, DESC_APP } desc_ref_type;

/* parameter or row descriptor? */
typedef enum { DESC_PARAM, DESC_ROW, DESC_UNKNOWN } desc_desc_type;

/* header or record field? (location in descriptor) */
typedef enum { DESC_HDR, DESC_REC } fld_loc;

typedef void (*fido_callback_func)(const char *);
extern fido_callback_func global_fido_callback;
extern std::mutex global_fido_mutex;

/* permissions - header, and base for record */
#define P_RI 1 /* imp */
#define P_WI 2
#define P_RA 4 /* app */
#define P_WA 8

/* macros to encode the constants above */
#define P_ROW(P) (P)
#define P_PAR(P) ((P) << 4)

#define PR_RIR P_ROW(P_RI)
#define PR_WIR (P_ROW(P_WI) | PR_RIR)
#define PR_RAR P_ROW(P_RA)
#define PR_WAR (P_ROW(P_WA) | PR_RAR)
#define PR_RIP P_PAR(P_RI)
#define PR_WIP (P_PAR(P_WI) | PR_RIP)
#define PR_RAP P_PAR(P_RI)
#define PR_WAP (P_PAR(P_WA) | PR_RAP)

/* macros to test type */
#define IS_APD(d) ((d)->desc_type == DESC_PARAM && (d)->ref_type == DESC_APP)
#define IS_IPD(d) ((d)->desc_type == DESC_PARAM && (d)->ref_type == DESC_IMP)
#define IS_ARD(d) ((d)->desc_type == DESC_ROW && (d)->ref_type == DESC_APP)
#define IS_IRD(d) ((d)->desc_type == DESC_ROW && (d)->ref_type == DESC_IMP)

/* additional field types needed, but not defined in ODBC */
#define SQL_IS_ULEN (-9)
#define SQL_IS_LEN (-10)

/* check if ARD record is a bound column */
#define ARD_IS_BOUND(d) (d) && ((d)->data_ptr || (d)->octet_length_ptr)

/* get the dbc from a descriptor */
#define DESC_GET_DBC(X) \
  (((X)->alloc_type == SQL_DESC_ALLOC_USER) ? (X)->dbc : (X)->stmt->dbc)

#define IS_BOOKMARK_VARIABLE(S)                               \
  if (S->stmt_options.bookmarks != SQL_UB_VARIABLE) {         \
    stmt->set_error("HY092", "Invalid attribute identifier"); \
    return SQL_ERROR;                                         \
  }

/* data-at-exec type */
#define DAE_NORMAL 1        /* normal SQLExecute() */
#define DAE_SETPOS_INSERT 2 /* SQLSetPos() insert */
#define DAE_SETPOS_UPDATE 3 /* SQLSetPos() update */
/* data-at-exec handling done for current SQLSetPos() call */
#define DAE_SETPOS_DONE 10

#define DONT_USE_LOCALE_CHECK(STMT) if (!STMT->dbc->ds.opt_NO_LOCALE)

#ifndef HAVE_TYPE_VECTOR
#endif

#if defined _WIN32

#define DECLARE_LOCALE_HANDLE int loc;

#define __LOCALE_SET(LOC)                                               \
  {                                                                     \
    loc = _configthreadlocale(0);                                       \
    _configthreadlocale(_ENABLE_PER_THREAD_LOCALE);                     \
    setlocale(LC_NUMERIC, LOC); /* force use of '.' as decimal point */ \
  }

#define __LOCALE_RESTORE()                         \
  {                                                \
    setlocale(LC_NUMERIC, default_locale.c_str()); \
    _configthreadlocale(loc);                      \
  }

#elif defined LC_GLOBAL_LOCALE
#define DECLARE_LOCALE_HANDLE locale_t nloc;

#define __LOCALE_SET(LOC)                              \
  {                                                    \
    nloc = newlocale(LC_CTYPE_MASK, LOC, (locale_t)0); \
    uselocale(nloc);                                   \
  }

#define __LOCALE_RESTORE()       \
  {                              \
    uselocale(LC_GLOBAL_LOCALE); \
    freelocale(nloc);            \
  }
#else

#define DECLARE_LOCALE_HANDLE

#define __LOCALE_SET(LOC) \
  { setlocale(LC_NUMERIC, LOC); /* force use of '.' as decimal point */ }

#define __LOCALE_RESTORE() \
  { setlocale(LC_NUMERIC, default_locale.c_str()); }
#endif

#define C_LOCALE_SET(STMT)    \
  DONT_USE_LOCALE_CHECK(STMT) \
  __LOCALE_SET("C")

#define DEFAULT_LOCALE_SET(STMT) \
  DONT_USE_LOCALE_CHECK(STMT)    \
  __LOCALE_RESTORE()

//The following are extracted from mysql header files.
#define NAME_CHAR_LEN 64           /**< Field/table name length */
#define SYSTEM_CHARSET_MBMAXLEN 3
#define NAME_LEN (NAME_CHAR_LEN * SYSTEM_CHARSET_MBMAXLEN)

/* DESODBC:
    New symbols for DESODBC.
    Original author: DESODBC Developer
*/
#define SHARED_MEMORY_NAME_BASE "DESODBC_SHMEM"
#define SHARED_MEMORY_MUTEX_NAME_BASE "DESODBC_SHMEM_MUTEX"
#define QUERY_MUTEX_NAME_BASE "DESODBC_QUERY_MUTEX"
#define DES_MAX_STRLEN 255  // we are mimicring PostgreSQL's convention
#ifdef _WIN32
#define REQUEST_HANDLE_EVENT_NAME_BASE "DESODBC_REQUEST_HANDLE_EVENT"
#define REQUEST_HANDLE_MUTEX_NAME_BASE "DESODBC_REQUEST_HANDLE_MUTEX"
#define HANDLE_SENT_EVENT_NAME_BASE "DESODBC_HANDLE_SENT_EVENT"
#define FINISHING_EVENT_NAME_BASE "DESODBC_FINISHING_EVENT"
#define MAX_CLIENTS 256
#define EVENT_TIMEOUT 5000
#define MUTEX_TIMEOUT 2000
#else
#define IN_WPIPE_NAME_BASE "/tmp/DESODBC_IN_WPIPE"
#define OUT_RPIPE_NAME_BASE "/tmp/DESODBC_OUT_RPIPE"
#define MUTEX_TIMEOUT_SECONDS 10
#endif


typedef struct {
  int perms;
  SQLSMALLINT data_type; /* SQL_IS_SMALLINT, etc */
  fld_loc loc;
  size_t offset; /* offset of field in struct */
} desc_field;

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/**
  Column types for DES
*/
enum enum_field_types
#if defined(__cplusplus) && __cplusplus > 201103L
    // N2764: Forward enum declarations, added in C++11
    : int
#endif /* __cplusplus */
{
  DES_TYPE_VARCHAR,
  DES_TYPE_STRING,
  DES_TYPE_CHAR_N,
  DES_TYPE_VARCHAR_N,
  DES_TYPE_CHAR,
  DES_TYPE_INTEGER,
  DES_TYPE_INT,
  DES_TYPE_FLOAT,
  DES_TYPE_REAL,
  DES_TYPE_DATE,
  DES_TYPE_TIME,
  DES_TYPE_DATETIME,
  DES_TYPE_TIMESTAMP,
  DES_TYPE_BLOB,  // needed for compatibility with insert_param
  DES_TYPE_TINY,  // needed for compatibility with insert_param
  DES_TYPE_SHORT, // needed for compatibility with MSAccess
  DES_TYPE_LONG,   // needed for compatibility with MSAccess
  DES_UNKNOWN_TYPE
};

/* DESODBC:
    Handy structure
    Original author: DESODBC Developer
*/
struct TypeAndLength {
  enum_field_types simple_type;
  SQLULEN len = -1;  // length as in number of characters; do not confuse with
                     // length as the DES_FIELD field (width of column)
};

/* DESODBC:
    Renamed and modified some of its attributes.
    Original author: MySQL
    Modified by: DESODBC Developer
*/
typedef struct DES_FIELD {
  char *name = nullptr;     /* Name of column */
  char *org_name = nullptr;  /* Original column name, if an alias */
  char *table = nullptr;    /* Table of column if column was a field */
  char *org_table = nullptr; /* Org table name, if table was an alias */
  char *db = nullptr;       /* Database for table */
  char *catalog = nullptr;  /* Catalog for table */
  char *def = nullptr;                /* Default value (set by mysql_list_fields) */
  unsigned long length;     /* Width of column (create length) */
  unsigned long max_length; /* Max width for selected set */
  unsigned int name_length;
  unsigned int org_name_length;
  unsigned int table_length;
  unsigned int org_table_length;
  unsigned int db_length;
  unsigned int catalog_length;
  unsigned int def_length;
  unsigned int flags;         /* Div flags */
  unsigned int decimals;      /* Number of decimals in field */
  unsigned int charsetnr;     /* Character set */
  enum enum_field_types type; /* Type of field. See mysql_com.h for types */
  void *extension;
} DES_FIELD;

struct DESCREC {
  /* ODBC spec fields */
  SQLINTEGER auto_unique_value; /* row only */
  SQLCHAR *base_column_name = nullptr; /* row only */
  SQLCHAR *base_table_name = nullptr;  /* row only */
  SQLINTEGER case_sensitive;    /* row only */
  SQLCHAR *catalog_name = nullptr;     /* row only */
  SQLSMALLINT concise_type;
  SQLPOINTER data_ptr;
  SQLSMALLINT datetime_interval_code;
  SQLINTEGER datetime_interval_precision;
  SQLLEN display_size; /* row only */
  SQLSMALLINT fixed_prec_scale;
  SQLLEN *indicator_ptr = nullptr;
  SQLCHAR *label = nullptr; /* row only */
  SQLULEN length;
  SQLCHAR *literal_prefix = nullptr; /* row only */
  SQLCHAR *literal_suffix = nullptr; /* row only */
  SQLCHAR *local_type_name = nullptr;
  SQLCHAR *name = nullptr;
  SQLSMALLINT nullable;
  SQLINTEGER num_prec_radix;
  SQLLEN octet_length;
  SQLLEN *octet_length_ptr = nullptr;
  SQLSMALLINT parameter_type; /* param only */
  SQLSMALLINT precision;
  SQLSMALLINT rowver;
  SQLSMALLINT scale;
  SQLCHAR *schema_name = nullptr; /* row only */
  SQLSMALLINT searchable; /* row only */
  SQLCHAR *table_name = nullptr;  /* row only */
  SQLSMALLINT type;
  SQLCHAR *type_name = nullptr;
  SQLSMALLINT unnamed;
  SQLSMALLINT is_unsigned;
  SQLSMALLINT updatable; /* row only */

  desc_desc_type m_desc_type;
  desc_ref_type m_ref_type;

  /* internal descriptor fields */

  /* parameter-specific */
  struct par_struct {
    /* value, value_length, and alloced are used for data
     * at exec parameters */

    tempBuf tempbuf;
    /*
      this parameter is data-at-exec. this is needed as cursor updates
      in ADO change the bind_offset_ptr between SQLSetPos() and the
      final call to SQLParamData() which makes it impossible for us to
      know any longer it was a data-at-exec param.
    */
    char is_dae;
    // my_bool alloced;
    /* Whether this parameter has been bound by the application
     * (if not, was created by dummy execution) */
    my_bool real_param_done;

    par_struct() : tempbuf(0), is_dae(0), real_param_done(false) {}

    par_struct(const par_struct &p)
        : tempbuf(p.tempbuf),
          is_dae(p.is_dae),
          real_param_done(p.real_param_done) {}

    void add_param_data(const char *chunk, unsigned long length);

    size_t val_length() {
      // Return the current position, not the buffer length
      return tempbuf.cur_pos;
    }

    char *val() { return tempbuf.buf; }

    void reset() {
      tempbuf.reset();
      is_dae = 0;
    }

  } par;

  /* row-specific */
  struct row_struct {
    DES_FIELD *field = nullptr; /* Used *only* by IRD */
    ulong datalen;    /* actual length, maintained for *each* row */
    /* TODO ugly, but easiest way to handle memory */
    SQLCHAR type_name[40];

    row_struct() : field(nullptr), datalen(0) {}

    void reset() {
      field = nullptr;
      datalen = 0;
      type_name[0] = 0;
    }
  } row;

  void desc_rec_init_apd();
  void desc_rec_init_ipd();
  void desc_rec_init_ard();
  void desc_rec_init_ird();
  void reset_to_defaults();

  DESCREC(desc_desc_type desc_type, desc_ref_type ref_type)
      : auto_unique_value(0),
        base_column_name(nullptr),
        base_table_name(nullptr),
        case_sensitive(0),
        catalog_name(nullptr),
        concise_type(0),
        data_ptr(nullptr),
        datetime_interval_code(0),
        datetime_interval_precision(0),
        display_size(0),
        fixed_prec_scale(0),
        indicator_ptr(nullptr),
        label(nullptr),
        length(0),
        literal_prefix(nullptr),
        literal_suffix(nullptr),
        local_type_name(nullptr),
        name(nullptr),
        nullable(0),
        num_prec_radix(0),
        octet_length(0),
        octet_length_ptr(nullptr),
        parameter_type(0),
        precision(0),
        rowver(0),
        scale(0),
        schema_name(nullptr),
        searchable(0),
        table_name(nullptr),
        type(0),
        type_name(nullptr),
        unnamed(0),
        is_unsigned(0),
        updatable(0),
        m_desc_type(desc_type),
        m_ref_type(ref_type) {
    reset_to_defaults();
  }
};

struct STMT;
struct DBC;

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
struct DESC {
  /* header fields */
  SQLSMALLINT alloc_type = 0;
  SQLULEN array_size = 0;
  SQLUSMALLINT *array_status_ptr = nullptr;
  /* NOTE: This field is defined as SQLINTEGER* in the descriptor
   * documentation, but corresponds to SQL_ATTR_ROW_BIND_OFFSET_PTR or
   * SQL_ATTR_PARAM_BIND_OFFSET_PTR when set via SQLSetStmtAttr(). The
   * 64-bit ODBC addendum says that when set via SQLSetStmtAttr(), this
   * is now a 64-bit value. These two are conflicting, so we opt for
   * the 64-bit value.
   */
  SQLULEN *bind_offset_ptr = nullptr;
  SQLINTEGER bind_type = 0;
  SQLLEN count = 0;  // Only used for SQLGetDescField()
  SQLLEN bookmark_count = 0;
  /* Everywhere(http://msdn.microsoft.com/en-us/library/ms713560(VS.85).aspx
     http://msdn.microsoft.com/en-us/library/ms712631(VS.85).aspx) I found
     it's referred as SQLULEN* */
  SQLULEN *rows_processed_ptr = nullptr;

  /* internal fields */
  desc_desc_type desc_type = DESC_PARAM;
  desc_ref_type ref_type = DESC_IMP;

  std::vector<DESCREC> bookmark2;
  std::vector<DESCREC> records2;

  DESERROR error;
  STMT *stmt = nullptr;
  DBC *dbc = nullptr;

  /* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
  SQLRETURN set_error(const char *state, const char *msg);

  void free_paramdata();
  void reset();
  SQLRETURN set_field(SQLSMALLINT recnum, SQLSMALLINT fldid, SQLPOINTER val,
                      SQLINTEGER buflen);

  DESC(STMT *p_stmt, SQLSMALLINT p_alloc_type, desc_ref_type p_ref_type,
       desc_desc_type p_desc_type);

  size_t rcount() {
    count = (SQLLEN)records2.size();
    return count;
  }

  ~DESC() {
    // if (desc_type == DESC_PARAM && ref_type == DESC_APP)
    //   free_paramdata();
  }

  /* SQL_DESC_ALLOC_USER-specific */
  std::list<STMT *> stmt_list;

  void stmt_list_remove(STMT *stmt) {
    if (alloc_type == SQL_DESC_ALLOC_USER) stmt_list.remove(stmt);
  }

  void stmt_list_add(STMT *stmt) {
    if (alloc_type == SQL_DESC_ALLOC_USER) stmt_list.emplace_back(stmt);
  }

  inline bool is_apd() {
    return desc_type == DESC_PARAM && ref_type == DESC_APP;
  }

  inline bool is_ipd() {
    return desc_type == DESC_PARAM && ref_type == DESC_IMP;
  }

  inline bool is_ard() { return desc_type == DESC_ROW && ref_type == DESC_APP; }

  inline bool is_ird() { return desc_type == DESC_ROW && ref_type == DESC_IMP; }
};

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/* Statement attributes */
struct STMT_OPTIONS {
  SQLUINTEGER cursor_type = 0;
  SQLUINTEGER simulateCursor = 0;
  SQLULEN max_length = 0, max_rows = 0;
  SQLULEN query_timeout = -1;
  SQLUSMALLINT *rowStatusPtr_ex = nullptr; /* set by SQLExtendedFetch */
  bool retrieve_data = true;
  SQLUINTEGER bookmarks = 0;
  void *bookmark_ptr = nullptr;
  bool bookmark_insert = false;
  bool metadata_id = false;  //DESODBC: added by DESODBC
};

#ifdef _WIN32

/* DESODBC:
    Original author: DESODBC Developer
*/
struct Client {
  size_t id;
  DWORD pid;
};

/* DESODBC:
    Original author: DESODBC Developer
*/
struct ConnectedClients {
  Client connected_clients[MAX_CLIENTS];
  int size = 0;
};

/* DESODBC:
    Original author: DESODBC Developer
*/
struct HandleSharingInfo {
  Client handle_petitioner;
  Client handle_petitionee;
  HANDLE in_handle = NULL;
  HANDLE out_handle = NULL;
};

/* DESODBC:
    Original author: DESODBC Developer
*/
struct SharedMemoryWin {
  DWORD DES_pid = -1;

  ConnectedClients connected_clients_struct;

  HandleSharingInfo handle_sharing_info;

  bool des_process_created = false;

  int exec_hash_int = 0;
};
#else

#ifndef _WIN32
/* DESODBC:
   Added new libraries.
*/
#include <fcntl.h>
#include <poll.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <iostream>
#endif

/* DESODBC:
    Original author: DESODBC Developer
*/
struct SharedMemoryUnix {
  int n_clients = 0;
  pid_t DES_pid = -1;
  bool des_process_created = false;
  int exec_hash_int = 0;

  sem_t shared_memory_mutex;
  sem_t query_mutex;
};

#endif
/* Environment handler */

struct ENV {
  bool initialized = false;

  int number_of_connections = 0;

  SQLINTEGER odbc_ver;
  std::list<DBC *> conn_list;
  DESERROR error;
  std::mutex lock;

  ENV(SQLINTEGER ver) : odbc_ver(ver) {}

  void add_dbc(DBC *dbc);
  void remove_dbc(DBC *dbc);
  bool has_connections();

  SQLRETURN set_error(const char *state, const char *errtext);

  ~ENV() {}
};

/* DESODBC:
    Original author: DESODBC Developer
*/
enum COMMAND_TYPE {
  UNKNOWN, SELECT, INSERT, DEL, UPDATE,
  PROCESS,
  SQLTABLES,
  SQLPRIMARYKEYS,
  SQLFOREIGNKEYS_PK,
  SQLFOREIGNKEYS_FK,
  SQLFOREIGNKEYS_PKFK,
  SQLGETTYPEINFO,
  SQLSTATISTICS,
  SQLSPECIALCOLUMNS,
  SQLCOLUMNS
};

struct DES_RESULT;

/* DESODBC:
    Added new attributes to support IPC.
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/* Connection handler */
struct DBC {
  ENV *env;

   /* DESODBC:
   New DESODBC attributes
    */
  size_t connection_id;

  std::hash<std::string> str_hasher;
  std::hash<std::wstring> wstr_hasher;

  std::string connection_hash = "";
  int connection_hash_int = 0;

  std::string exec_hash = "";
  int exec_hash_int = 0;

#ifdef _WIN32
  LPCSTR SHARED_MEMORY_NAME;
  LPCSTR SHARED_MEMORY_MUTEX_NAME;
  LPCSTR QUERY_MUTEX_NAME;
  LPCSTR REQUEST_HANDLE_EVENT_NAME;
  LPCSTR REQUEST_HANDLE_MUTEX_NAME;
  LPCSTR HANDLE_SENT_EVENT_NAME;
  LPCSTR FINISHING_EVENT_NAME;

  HANDLE query_mutex = nullptr;
  HANDLE shared_memory_mutex = nullptr;
  HANDLE request_handle_mutex = nullptr;

  HANDLE request_handle_event = nullptr;
  HANDLE handle_sent_event = nullptr;
  HANDLE finishing_event = nullptr;

  PROCESS_INFORMATION process_info;
  STARTUPINFOW startup_info_unicode;
  STARTUPINFO startup_info_ansi;

  SharedMemoryWin *shmem = nullptr;

  HANDLE driver_to_des_out_rpipe = nullptr;
  HANDLE driver_to_des_out_wpipe = nullptr;
  HANDLE driver_to_des_in_rpipe = nullptr;
  HANDLE driver_to_des_in_wpipe = nullptr;
  std::unique_ptr<std::thread> share_pipes_thread;

#else
  const char *SHARED_MEMORY_NAME;
  const char *SHARED_MEMORY_MUTEX_NAME;
  const char *QUERY_MUTEX_NAME;
  const char *IN_WPIPE_NAME;
  const char *OUT_RPIPE_NAME;

  int shm_id;
  SharedMemoryUnix *shmem;

#ifdef __APPLE__
  sem_t *shared_memory_mutex;
  sem_t *query_mutex;
#endif

  int driver_to_des_out_rpipe;
  int driver_to_des_out_wpipe;
  int driver_to_des_in_rpipe;
  int driver_to_des_in_wpipe;

#endif

  /* DESODBC:
   These are the remaining original MyODBC attributes
    */
  std::list<STMT *> stmt_list;
  std::list<DESC *> desc_list;  // Explicit descriptors
  STMT_OPTIONS stmt_options;
  DESERROR error;
  FILE *query_log = nullptr;
  char st_error_prefix[255] = {0};
  std::string database;
  SQLUINTEGER login_timeout = 0;
  time_t last_query_time = 0;
  int txn_isolation = 0;
  uint port = 0;
  uint cursor_count = 0;
  ulong net_buffer_len = 0;
  uint commit_flag = 0;
  bool has_query_attrs = false;
  std::recursive_mutex lock;

  std::string last_DES_error = "";

  bool connected = false;

  // Whether SQL*ConnectW was used
  bool unicode = false;
  // Connection charset ('ANSI' or utf-8)
  desodbc::CHARSET_INFO *cxn_charset_info = nullptr;
  // data source used to connect (parsed or stored)
  DataSource ds;
  // value of the sql_select_limit currently set for a session
  //   (SQLULEN)(-1) if wasn't set
  SQLULEN sql_select_limit = -1;
  // Connection have been put to the pool
  int need_to_wakeup = 0;
  fido_callback_func fido_callback = nullptr;

  DBC(ENV *p_env);

  /* DESODBC:
    The following symbols relate to DESODBC's IPC
    Original author: DESODBC Developer
  */

  #ifdef _WIN32
  LPCSTR build_name(const char *name_base);
  void get_concurrent_objects(const wchar_t *des_exec_path,
                            const wchar_t *des_working_dir);
  #else
  const char *build_name(const char *name_base);
  void get_concurrent_objects(const char *des_exec_path,
                            const char *des_working_dir);
  #endif

  SQLRETURN initialize();

  SQLRETURN createPipes();
  

  #ifdef _WIN32
  SQLRETURN create_DES_process(SQLWCHAR *des_exec_path,
                             SQLWCHAR *des_working_dir);
  #else
  SQLRETURN create_DES_process(const char *des_exec_path,
                             const char *des_working_dir);
  #endif

  SQLRETURN get_DES_process_pipes();

  #ifdef _WIN32
  void sharePipes();
  #endif

  #ifdef _WIN32
  SQLRETURN get_mutex(HANDLE h, const std::string &name);
  SQLRETURN release_mutex(HANDLE h, const std::string &name);
  #else
  SQLRETURN get_mutex(sem_t *s, const std::string &name);
  SQLRETURN release_mutex(sem_t *s, const std::string &name);
  #endif

  #ifdef _WIN32
  SQLRETURN getRequestHandleMutex();
  SQLRETURN releaseRequestHandleMutex();

  SQLRETURN setEvent(HANDLE h, const std::string &name);
  SQLRETURN setFinishingEvent();
  SQLRETURN setRequestHandleEvent();

  void remove_client_from_shmem(ConnectedClients &pids, size_t id);
  #endif

  SQLRETURN get_shared_memory_mutex();
  SQLRETURN release_shared_memory_mutex();

  SQLRETURN get_query_mutex();
  SQLRETURN release_query_mutex();

  
  /*DESODBDC:
    Functions related to sending queries and fetching data.
    Original author: DESODBC
  */

  #ifdef _WIN32
  std::pair<SQLRETURN, std::string> read_DES_output_win(
      const std::string &query);
  #else
  std::pair<SQLRETURN, std::string> read_DES_output_unix(
      const std::string &query);
  #endif

  std::pair<SQLRETURN, std::string> send_query_and_read(
      const std::string &query);
  std::pair<SQLRETURN, DES_RESULT *> send_query_and_get_results(
      COMMAND_TYPE type, const std::string &query);
  
  // MyODBC functions:
  void free_explicit_descriptors();
  void free_connection_stmts();
  void add_desc(DESC *desc);
  void remove_desc(DESC *desc);

  /* DESODBC: setting errors in the context of
  * Windows or Unix Systems.
  * Original author: DESODBC Developer
  */
#ifdef _WIN32
  SQLRETURN set_win_error(std::string err, bool show_win_err);
#else
  SQLRETURN set_unix_error(std::string err, bool show_unix_err);
#endif

  /* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
    */
  SQLRETURN set_error(const char *state, const char *msg);
  SQLRETURN connect(DataSource *ds);

  SQLRETURN close();
  ~DBC();

  void set_charset(std::string charset);
  SQLRETURN set_charset_options(const char *charset);
};

/* DESODBC:
    Original author: DESODBC Developer
*/
inline std::list<DBC *> active_dbcs_global_var;

/* Statement states */

enum DES_STATE { ST_UNKNOWN = 0, ST_PREPARED, ST_PRE_EXECUTED, ST_EXECUTED };
enum DES_DUMMY_STATE {
  ST_DUMMY_UNKNOWN = 0,
  ST_DUMMY_PREPARED,
  ST_DUMMY_EXECUTED
};

struct DES_LIMIT_CLAUSE {
  unsigned long long offset;
  unsigned int row_count;
  const char *begin, *end;
  DES_LIMIT_CLAUSE(unsigned long long offs, unsigned int rc, char *b, char *e)
      : offset(offs), row_count(rc), begin(b), end(e) {}
};

struct DES_LIMIT_SCROLLER {
  tempBuf buf;
  char *query, *offset_pos;
  unsigned int row_count;
  unsigned long long start_offset;
  unsigned long long next_offset, total_rows, query_len;

  DES_LIMIT_SCROLLER()
      : buf(1024),
        query(buf.buf),
        offset_pos(query),
        row_count(0),
        start_offset(0),
        next_offset(0),
        total_rows(0),
        query_len(0) {}

  void extend_buf(size_t new_size) { buf.extend_buffer(new_size); }
  void reset() {
    next_offset = 0;
    offset_pos = query;
  }
};

/* DESODBC:
    Rename done.
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/* Statement primary key handler for cursors */
struct DES_PK_COLUMN {
  char name[NAME_LEN + 1];
  my_bool bind_done;

  DES_PK_COLUMN() {
    name[0] = 0;
    bind_done = FALSE;
  }
};

/* DESODBC:
    Rename done.
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/* Statement cursor handler */
struct DESCURSOR {
  std::string name;
  uint pk_count;
  my_bool pk_validated;
  DES_PK_COLUMN pkcol[MY_MAX_PK_PARTS];

  DESCURSOR() : pk_count(0), pk_validated(FALSE) {}
};

enum OUT_PARAM_STATE {
  OPS_UNKNOWN = 0,
  OPS_BEING_FETCHED,
  OPS_PREFETCHED,
  OPS_STREAMS_PENDING
};

#define CAT_SCHEMA_SET_FULL(STMT, C, S, V, CZ, SZ, CL, SL)   \
  {                                                          \
    bool cat_is_set = false;                                 \
    if (!STMT->dbc->ds.opt_NO_CATALOG && (CL || !SL)) {      \
      C = V;                                                 \
      S = nullptr;                                           \
      cat_is_set = true;                                     \
    }                                                        \
    if (!STMT->dbc->ds.opt_NO_SCHEMA && !cat_is_set && SZ) { \
      S = V;                                                 \
      C = nullptr;                                           \
    }                                                        \
  }

#define CAT_SCHEMA_SET(C, S, V) \
  CAT_SCHEMA_SET_FULL(stmt, C, S, V, catalog, schema, catalog_len, schema_len)

/* Main statement handler */

struct GETDATA {
  uint column;      /* Which column is being used with SQLGetData() */
  char *source;     /* Our current position in the source. */
  uchar latest[7];  /* Latest character to be converted. */
  int latest_bytes; /* Bytes of data in latest. */
  int latest_used;  /* Bytes of latest that have been used. */
  ulong src_offset; /* @todo remove */
  ulong dst_bytes;  /* Length of data once it is all converted (in chars). */
  ulong dst_offset; /* Current offset into dest. (ulong)~0L when not set. */

  GETDATA()
      : column(0),
        source(NULL),
        latest_bytes(0),
        latest_used(0),
        src_offset(0),
        dst_bytes(0),
        dst_offset(0) {}
};
struct ResultTable;

struct STMT;

/* DESODBC:
    Rename done.
    Original author: MySQL
    Modified by: DESODBC Developer
*/
typedef char **DES_ROW; /* return data as array of strings */

/* DESODBC:
    Rename done.
    Original author: MySQL
    Modified by: DESODBC Developer
*/
typedef struct DES_ROWS {
  struct DES_ROWS *next = nullptr; /* list of rows */
  DES_ROW data = nullptr;
  unsigned long length = 0;
} DES_ROWS;

/* DESODBC:
    Rename done.
    Original author: MySQL
    Modified by: DESODBC Developer
*/
typedef struct DES_DATA {
  DES_ROWS *data = nullptr;
  struct MEM_ROOT *alloc = nullptr;
  uint64_t rows = 0;
  unsigned int fields = 0;
} DES_DATA;

enum enum_resultset_metadata {
  /** No metadata will be sent. */
  RESULTSET_METADATA_NONE = 0,
  /** The server will send all metadata. */
  RESULTSET_METADATA_FULL = 1
};

/* DESODBC:
    Rename done and modified some of its
    attributes.
    Original author: MySQL
    Modified by: DESODBC Developer
*/
struct DES_RESULT {
  uint64_t row_count;
  DES_FIELD *fields = nullptr;
  struct DES_DATA *data = nullptr;
  DES_ROWS *data_cursor = nullptr;
  unsigned long *lengths = nullptr; /* column lengths of current row */
  const struct DES_METHODS *methods = nullptr;
  DES_ROW row = nullptr;         /* If unbuffered read */
  DES_ROW current_row = nullptr; /* buffer to current row */
  ResultTable *internal_table = nullptr;
  unsigned int field_count, current_field;
  bool eof; /* Used by mysql_fetch_row */
  /* mysql_stmt_close() had to cancel this result */
  bool unbuffered_fetch_cancelled;
  enum enum_resultset_metadata metadata;
  void *extension = nullptr;

  DES_RESULT();
  DES_RESULT(STMT *stmt);
};

// DESODBC:
// Mimicring MySQL extern functions
// (some of them, extracted from the MySQL Server's source code)

/* DESODBC:
    Original author: MySQL
    Modified by: DESODBC Developer
*/
inline static DES_ROW des_fetch_row(DES_RESULT *result) {
  DES_ROW tmp = nullptr;
  if (!result->data_cursor) {
    result->current_row = tmp;
    return tmp;
  }
  tmp = result->data_cursor->data;
  result->data_cursor = result->data_cursor->next;
  result->current_row = tmp;

  return tmp;
}

/* DESODBC:
    Original author: MySQL
    Modified by: DESODBC Developer
*/
inline static DES_FIELD *des_fetch_field_direct(DES_RESULT *res,
                                                unsigned int fieldnr) {
  if (fieldnr >= res->field_count || !res->fields) return (nullptr);
  return &(res)->fields[fieldnr];
}

/* DESODBC:
    Original author: MySQL
    Modified by: DESODBC Developer
*/
inline static unsigned int des_num_fields(DES_RESULT *res) {
  return res->field_count;
}

/* DESODBC:
    Original author: MySQL
    Modified by: DESODBC Developer
*/
inline static uint64_t des_num_rows(DES_RESULT *res) { return res->row_count; }

/* DESODBC:
    Original author: MySQL
    Modified by: DESODBC Developer
*/
inline static void des_data_seek(DES_RESULT *result, uint64_t offset) {
  DES_ROWS *tmp = nullptr;
  if (result->data)
    for (tmp = result->data->data; offset-- && tmp; tmp = tmp->next)
      ;
  result->current_row = nullptr;
  result->data_cursor = tmp;
}

/* DESODBC:
    Renamed from the original.
    Original author: MySQL
    Modified by: DESODBC Developer
*/
typedef DES_ROWS *DES_ROW_OFFSET; /* offset to current row */

/* DESODBC:
    Original author: MySQL
    Modified by: DESODBC Developer
*/
inline static DES_ROW_OFFSET des_row_seek(DES_RESULT *result,
                                          DES_ROW_OFFSET offset) {
  DES_ROW_OFFSET return_value = result->data_cursor;
  result->current_row = nullptr;
  result->data_cursor = offset;
  return return_value;
}

/* DESODBC:
    Original author: MySQL
    Modified by: DESODBC Developer
*/
inline static DES_ROW_OFFSET des_row_tell(DES_RESULT *res) {
  return res->data_cursor;
}



/*
  A string capable of being a NULL
*/
struct xstring : public std::string {
  bool m_is_null = false;

  using Base = std::string;

  xstring(std::nullptr_t) : m_is_null(true) {}

  xstring(char *s)
      : m_is_null(s == nullptr),
        Base(s == nullptr ? "" : std::forward<char *>(s)) {}

  template <class T>
  xstring(T &&s) : Base(std::forward<T>(s)) {}

  xstring(SQLULEN v) : Base(std::to_string(v)) {}

  xstring(long long v) : Base(std::to_string(v)) {}

  xstring(SQLSMALLINT v) : Base(std::to_string(v)) {}

  xstring(long int v) : Base(std::to_string(v)) {}

  xstring(int v) : Base(std::to_string(v)) {}

  xstring(unsigned int v) : Base(std::to_string(v)) {}

  const char *c_str() const { return m_is_null ? nullptr : Base::c_str(); }
  size_t size() const { return m_is_null ? 0 : Base::size(); }

  bool is_null() { return m_is_null; }
};

/* DESODBC:
    Renamed from the original.
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
typedef struct DES_BIND {
  unsigned long *length = nullptr; /* output length pointer */
  bool *is_null = nullptr;         /* Pointer to null indicator */
  void *buffer = nullptr;          /* buffer to get/put data */
  /* set this if you want to track data truncations happened during fetch */
  bool *error = nullptr;
  unsigned char *row_ptr = nullptr; /* for the current data position */
  void (*fetch_result)(struct DES_BIND *, DES_FIELD *, unsigned char **row);
  void (*skip_result)(struct DES_BIND *, DES_FIELD *, unsigned char **row);
  /* output buffer length, must be set when fetching str/binary */
  unsigned long buffer_length;
  unsigned long offset;              /* offset position for char/binary fetch */
  unsigned long length_value;        /* Used if length is 0 */
  unsigned int param_number;         /* For null count and error messages */
  unsigned int pack_length;          /* Internal length for packed data */
  enum enum_field_types buffer_type; /* buffer type */
  bool error_value;                  /* used if error is 0 */
  bool is_unsigned;                  /* set if integer type is unsigned */
  bool long_data_used;               /* If used with mysql_send_long_data */
  bool is_null_value;                /* Used if is_null is 0 */
  void *extension;
} DES_BIND;

struct ROW_STORAGE {
  typedef std::vector<xstring> vstr;
  typedef std::vector<const char *> pstr;
  size_t m_rnum = 0, m_cnum = 0, m_cur_row = 0, m_cur_col = 0;
  bool m_eof = true;

  /*
    Data and pointers are in separate containers because for the pointers
    we will need to get the sequence of pointers returned by vector::data()
  */
  vstr m_data;
  pstr m_pdata;

  /*
    Setting zero for rows or columns makes the storage object invalid
  */
  size_t set_size(size_t rnum, size_t cnum);

  /*
    Invalidate the data array.
    Returns true if the data actually existed and was invalidated. False
    otherwise.
  */
  bool invalidate() {
    bool was_invalidated = is_valid();
    m_eof = true;
    set_size(0, 0);
    return was_invalidated;
  }

  /* Returns true if current row was last */
  bool eof() { return m_eof; }

  bool is_valid() { return m_rnum * m_cnum > 0; }

  bool next_row();

  /* Set the row counter to the first row */
  void first_row() {
    m_cur_row = 0;
    m_eof = m_rnum == 0;
  }

  xstring &operator[](size_t idx);

  void set_data(size_t idx, void *data, size_t size) {
    if (data)
      m_data[m_cur_row * m_cnum + idx].assign((const char *)data, size);
    else
      m_data[m_cur_row * m_cnum + idx] = nullptr;
    m_eof = false;
  }

  /* Copy row data from bind buffer into the storage one row at a time */
  void set_data(DES_BIND *bind) {
    for (size_t i = 0; i < m_cnum; ++i) {
      if (!(*bind[i].is_null))
        set_data(i, bind[i].buffer, *(bind[i].length));
      else
        set_data(i, nullptr, 0);
    }
  }

  /* Copy data from the storage into the bind buffer one row at a time */
  void fill_data(DES_BIND *bind) {
    if (m_cur_row >= m_rnum || m_eof) return;

    for (size_t i = 0; i < m_cnum; ++i) {
      auto &data = m_data[m_cur_row * m_cnum + i];
      *(bind[i].is_null) = data.is_null();
      *(bind[i].length) = (unsigned long)(data.is_null() ? -1 : data.length());
      if (!data.is_null()) {
        size_t copy_zero = bind[i].buffer_length > *(bind[i].length) ? 1 : 0;
        memcpy(bind[i].buffer, data.data(), *(bind[i].length) + copy_zero);
      }
    }
    // Set EOF if the last row was filled
    m_eof = (m_rnum <= (m_cur_row + 1));
    // Increment row counter only if not EOF
    m_cur_row += m_eof ? 0 : 1;
  }

  const xstring &operator=(const xstring &val);

  ROW_STORAGE() { set_size(0, 0); }

  ROW_STORAGE(size_t rnum, size_t cnum) : m_rnum(rnum), m_cnum(cnum) {
    set_size(rnum, cnum);
  }

  const char **data();
};

enum class EXCEPTION_TYPE { EMPTY_SET, CONN_ERR, GENERAL };

struct ODBCEXCEPTION {
  EXCEPTION_TYPE m_type = EXCEPTION_TYPE::GENERAL;
  std::string m_msg;

  ODBCEXCEPTION(EXCEPTION_TYPE t = EXCEPTION_TYPE::GENERAL) : m_type(t) {}

  ODBCEXCEPTION(std::string msg, EXCEPTION_TYPE t = EXCEPTION_TYPE::GENERAL)
      : m_type(t), m_msg(msg) {}
};

class charPtrBuf {
 private:
  std::vector<char *> m_buf;
  DES_ROW m_external_val = nullptr;

 public:
  void reset() {
    m_buf.clear();
    m_external_val = nullptr;
  }

  void set_size(size_t size) {
    m_buf.resize(size);
    m_external_val = nullptr;
  }

  void set(const void *data, size_t size) {
    set_size(size);
    char *buf = (char *)data;
    std::vector<char *> tmpVec(size, buf);
    m_buf = tmpVec;
  }

  operator DES_ROW() const {
    if (m_external_val || m_buf.size())
      return (DES_ROW)(m_external_val ? m_external_val : m_buf.data());
    return nullptr;
  }

  charPtrBuf &operator=(const DES_ROW external_val) {
    reset();
    m_external_val = external_val;
    return *this;
  }
};

#define NULL_STR nullptr

/* DESODBC:
    Internal representation of a column from a result view.
    Original author: DESODBC Developer
*/
struct Column {
  DES_FIELD *field;
  std::vector<char *> values;

  bool new_heap_used = false;

  TypeAndLength type;

  Column() {}

  Column(DES_FIELD *field) {
    this->field = field;
  }

  enum_field_types get_simple_type() { return field->type; }

  unsigned int getDecimals();
  unsigned long getLength(int row);
  unsigned int getColumnSize();
  unsigned int getMaxLength();
  DES_ROWS *generate_DES_ROWS(int current_row);
  DES_FIELD *get_DES_FIELD();
  Column(const std::string &table_name, const std::string &col_name,
         const TypeAndLength &col_type, const SQLSMALLINT &col_nullable);
  void update_row(const int row_index, char *value);
  void remove_row(const int row_index);
  SQLSMALLINT get_decimal_digits();

  void insert_value(char *value) { values.push_back(value); }
  std::string get_value(int index) const { return values[index - 1]; }

};

/* DESODBC:
    Original author: DESODBC Developer
*/
struct ForeignKeyInfo {
  std::string key;
  std::string foreign_table;
  std::string foreign_key;
};

/* DESODBC:
    Original author: DESODBC Developer
*/
struct DBSchemaRelationInfo {
  // Unordered maps name -> column index
  std::unordered_map<std::string, int> columns_index_map;
  std::unordered_map<std::string, TypeAndLength> columns_type_map;
  std::vector<std::string> primary_keys;
  std::vector<ForeignKeyInfo> foreign_keys;
  std::vector<std::string> not_nulls;

  std::string name;

  bool is_table = true;  // false -> is view
};

struct STMT;  // Forward declaration to let ResultTable have a STMT attribute
//(there is a cyclic dependence)
// Warning: the functions that use fields of STMT in ResultTable
// are forward-declared. Then, STMT is defined, and then,
// the functions are defined.

#define DES_FIELD_DEF NullS,
#define DES_FIELD_DEF_LENGTH 0,

/* A few character sets we care about. */
#define ASCII_CHARSET_NUMBER 11
#define BINARY_CHARSET_NUMBER 63
#define UTF8_CHARSET_NUMBER 33

/* DESODBC:
    These #define symbols have been renamed and modified
    from the original ones.
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/* Same us DESODBC_FIELD_STRING(name, NAME_LEN, flags) */
#define DESODBC_FIELD_NAME(name, flags)                                        \
  {                                                                            \
    (char *)(name), (char *)(name), NullS, NullS, NullS, NullS,                \
        DES_FIELD_DEF NAME_LEN, 0, 0, 0, 0, 0, 0, 0,                           \
        DES_FIELD_DEF_LENGTH(flags), 0, UTF8_CHARSET_NUMBER, DES_TYPE_VARCHAR, \
        NULL                                                                   \
  }

#define DESODBC_FIELD_STRING(name, len, flags)                                \
  {                                                                           \
    (char *)(name), (char *)(name), NullS, NullS, NullS, NullS,               \
        DES_FIELD_DEF(len *SYSTEM_CHARSET_MBMAXLEN), 0, 0, 0, 0, 0, 0, 0,     \
        DES_FIELD_DEF_LENGTH(flags), 0, UTF8_CHARSET_NUMBER, DES_TYPE_STRING, \
        NULL                                                                  \
  }

#define DESODBC_FIELD_SHORT(name, flags)                                      \
  {                                                                           \
    (char *)(name), (char *)(name), NullS, NullS, NullS, NullS,               \
        DES_FIELD_DEF 5, 5, 0, 0, 0, 0, 0, 0, DES_FIELD_DEF_LENGTH(flags), 0, \
        0, DES_TYPE_SHORT, NULL                                                 \
  }

#define DESODBC_FIELD_LONG(name, flags)                                      \
  {                                                                          \
    (char *)(name), (char *)(name), NullS, NullS, NullS, NullS,              \
        DES_FIELD_DEF 11, 11, 0, 0, 0, 0, 0, 0, DES_FIELD_DEF_LENGTH(flags), \
        0, 0, DES_TYPE_LONG, NULL                                             \
  }

#define DESODBC_FIELD_LONGLONG(name, flags)                                  \
  {                                                                          \
    (char *)(name), (char *)(name), NullS, NullS, NullS, NullS,              \
        DES_FIELD_DEF 20, 20, 0, 0, 0, 0, 0, 0, DES_FIELD_DEF_LENGTH(flags), \
        0, 0, DES_TYPE_INT, NULL                                             \
  }

/* DESODBC:
    Original author: DESODBC Developer
*/
struct STMT_params_for_table {
  // Some of these values may be empty. Useful for SQLForeignKeys calls
  std::string pk_table_name = "";
  std::string fk_table_name = "";

  // Useful for SQLGetTypeInfo calls
  SQLSMALLINT type_requested = SQL_TYPE_NULL;

  std::string table_name =
      "";  // This value may be empty. Useful for SQLStatistics calls

  std::string column_name =
      "";  // This value may be empty. Useful for SQLColumns calls

  std::string catalog_name = "";

  std::string table_type = "";

  bool metadata_id = false;
};

/* DESODBC:
    Original author: DESODBC Developer
*/
struct ResultTable {  // Internal representation of a result view.
  std::string table_name = "";

  DBC *dbc = nullptr;

  COMMAND_TYPE command_type;

  STMT_params_for_table params;

  std::string str = "";  // last TAPI output (from stmt)
  // Vector of column names, ordered by insertion time.
  std::vector<std::string> names_ordered;

  // Some calls are given to the driver using only the
  // column name. We need therefore to search the column given its name.
  std::unordered_map<std::string, Column> columns;

  ResultTable() {}
  ResultTable(STMT *stmt);
  ResultTable(COMMAND_TYPE type, const std::string &output);

  size_t col_count();
  size_t row_count();

  void insert_col(const std::string &tableName, const std::string &columnName,
                  const TypeAndLength &columnType,
                  const SQLSMALLINT &columnNullable);
  void insert_col(DES_FIELD *field);
  void insert_cols(DES_FIELD array[], int array_size);

  void insert_value(const std::string &columnName, char *value);
  void insert_value(const std::string &columnName, const std::string &value);

  unsigned long *fetch_lengths(int current_row);
  DES_ROW generate_DES_ROW(const int index);
  DES_ROWS *generate_DES_ROWS(const int current_row);
  DES_FIELD *get_DES_FIELD(int col_index);

  std::vector<ForeignKeyInfo> get_foreign_keys_from_TAPI(
      const std::vector<std::string> &lines, int &index);
  DBSchemaRelationInfo get_relation_info(const std::vector<std::string> &lines,
                                       int &index);
  std::unordered_map<std::string, DBSchemaRelationInfo> get_all_relations_info(
      const std::string &str);

  void build_table();
  void build_table_select();

  void insert_SQLGetTypeInfo_cols();
  void build_table_SQLGetTypeInfo();

  void insert_SQLStatistics_cols();
  void build_table_SQLStatistics();

  void insert_SQLSpecialColumns_cols();
  void build_table_SQLSpecialColumns();

  void insert_SQLColumns_cols();
  void build_table_SQLColumns();

  void insert_SQLPrimaryKeys_cols();
  void build_table_SQLPrimaryKeys();

  void insert_SQLForeignKeys_cols();
  void build_table_SQLForeignKeys_PK();
  void build_table_SQLForeignKeys_FK();
  void build_table_SQLForeignKeys_PKFK();

  void insert_metadata_cols();
  void build_table_SQLTables();

};

typedef uint64_t my_ulonglong;

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
struct STMT {
  DBC *dbc = nullptr;
  my_bool fake_result;
  charPtrBuf array;
  charPtrBuf result_array;
  DES_ROW current_values = nullptr;
  DES_ROW (*fix_fields)(STMT *stmt, DES_ROW row);
  DES_FIELD *fields = nullptr;
  DES_ROW_OFFSET end_of_set;
  tempBuf tempbuf;
  ROW_STORAGE m_row_storage;

  DES_RESULT *result = new DES_RESULT(this);  //DESODBC: New attribute

  std::vector<SQLCHAR *> bookmarks;

  std::string last_output = ""; //DESODBC: New attribute

  STMT_params_for_table params_for_table; //DESODBC: New attribute

  // DESODBC: New attribute
  COMMAND_TYPE type = UNKNOWN;  // unknown by default

  // DESODBC: New attribute
  bool new_row_des = true;

  // DESODBC: New attribute
  int current_row_des = 1;

  DESCURSOR cursor;
  DESERROR error;
  STMT_OPTIONS stmt_options;

  std::string catalog_name;

  DES_PARSED_QUERY query, orig_query;
  std::vector<DES_BIND> param_bind;
  std::vector<const char *> query_attr_names;

  std::unique_ptr<my_bool[]> rb_is_null;
  std::unique_ptr<my_bool[]> rb_err;
  std::unique_ptr<unsigned long[]> rb_len;
  std::unique_ptr<unsigned long[]> lengths;

  my_ulonglong affected_rows;
  long current_row;
  long cursor_row;
  char dae_type; /* data-at-exec type */

  GETDATA getdata;

  uint param_count, current_param, rows_found_in_set;

  enum DES_STATE state;
  enum DES_DUMMY_STATE dummy_state;

  /* APD for data-at-exec on SQLSetPos() */
  std::unique_ptr<DESC> setpos_apd;
  DESC *setpos_apd2;
  SQLSETPOSIROW setpos_row;
  SQLUSMALLINT setpos_lock;
  SQLUSMALLINT setpos_op;

  DES_BIND *result_bind;

  DES_LIMIT_SCROLLER scroller;

  enum OUT_PARAM_STATE out_params_state;

  DESC m_ard, *ard;
  DESC m_ird, *ird;
  DESC m_apd, *apd;
  DESC m_ipd, *ipd;

  /* implicit descriptors */
  DESC *imp_ard;
  DESC *imp_apd;

  std::recursive_mutex lock;

  /* DESODBC:
    Original author: DESODBC
  */
  std::pair<SQLRETURN, std::string> send_update_and_fetch_info(std::string query);

  /* DESODBC:
    Original author: DESODBC
  */
  int send_select_count(std::string query);

  /* DESODBC:
    Original author: DESODBC
  */
  SQLRETURN build_results();


  char *extend_buffer(char *to, size_t len);
  char *extend_buffer(size_t len);

  char *add_to_buffer(const char *from, size_t len);
  char *buf() { return tempbuf.buf; }
  char *endbuf() { return tempbuf.buf + tempbuf.cur_pos; }
  size_t buf_pos() { return tempbuf.cur_pos; }
  size_t buf_len() { return tempbuf.buf_len; }

  /* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
  size_t field_count();

  /* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
  DES_ROW fetch_row(bool read_unbuffered = false);
  void buf_set_pos(size_t pos) { tempbuf.cur_pos = pos; }
  void buf_add_pos(size_t pos) { tempbuf.cur_pos += pos; }
  void buf_remove_trail_zeroes() { tempbuf.remove_trail_zeroes(); }
  void alloc_lengths(size_t num);
  void reset_getdata_position();
  void allocate_param_bind(uint elements);
  long compute_cur_row(unsigned fFetchType, SQLLEN irow);

  void reset();
  void reset_result_array();
  void reset_setpos_apd();

  void free_lengths();
  void free_unbind();
  void free_reset_out_params();
  void free_reset_params();

  /* DESODBC:

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
  void free_fake_result(bool clear_all_results);

  /* DESODBC:
    Original author: DESODBC
  */
  SQLRETURN set_error(const char *state, const char *errtext);

  void clear_attr_names() { query_attr_names.clear(); }

  STMT(DBC *d)
      : dbc(d),
        result(NULL),
        fake_result(false),
        array(),
        result_array(),
        current_values(NULL),
        fields(NULL),
        end_of_set(NULL),
        tempbuf(),
        stmt_options(dbc->stmt_options),
        lengths(nullptr),
        affected_rows(0),
        current_row(0),
        cursor_row(0),
        dae_type(0),
        param_count(0),
        current_param(0),
        rows_found_in_set(0),
        state(ST_UNKNOWN),
        dummy_state(ST_DUMMY_UNKNOWN),
        setpos_row(0),
        setpos_lock(0),
        setpos_op(0),
        result_bind(NULL),
        out_params_state(OPS_UNKNOWN),

        m_ard(this, SQL_DESC_ALLOC_AUTO, DESC_APP, DESC_ROW),
        ard(&m_ard),
        m_ird(this, SQL_DESC_ALLOC_AUTO, DESC_IMP, DESC_ROW),
        ird(&m_ird),
        m_apd(this, SQL_DESC_ALLOC_AUTO, DESC_APP, DESC_PARAM),
        apd(&m_apd),
        m_ipd(this, SQL_DESC_ALLOC_AUTO, DESC_IMP, DESC_PARAM),
        ipd(&m_ipd),
        imp_ard(ard),
        imp_apd(apd) {
    allocate_param_bind(10);

    LOCK_DBC(dbc);
    dbc->stmt_list.emplace_back(this);
  }

  ~STMT();

  void clear_param_bind();

 private:
  /*
    Create a phony, non-functional STMT handle used as a placeholder.


    Warning: The handle should not be used other than for storing attributes
    added using `add_query_attr()`.
  */

  STMT(DBC *d, size_t param_cnt)
      : dbc{d},
        query_attr_names{param_cnt},
        m_ard(this, SQL_DESC_ALLOC_AUTO, DESC_APP, DESC_ROW),
        m_ird(this, SQL_DESC_ALLOC_AUTO, DESC_IMP, DESC_ROW),
        m_apd(this, SQL_DESC_ALLOC_AUTO, DESC_APP, DESC_PARAM),
        m_ipd(this, SQL_DESC_ALLOC_AUTO, DESC_IMP, DESC_PARAM) {}

  friend DBC;
};

/* DESODBC:
    Original author: MySQL
    Modified by: DESODBC Developer
*/
inline static uint64_t des_affected_rows(STMT *stmt) {
  return stmt->affected_rows;
}

/* DESODBC:
    Original author: MySQL
    Modified by: DESODBC Developer
*/
inline static unsigned long *des_fetch_lengths(STMT *stmt) {
  delete stmt->result->lengths;
  stmt->result->lengths =
      stmt->result->internal_table->fetch_lengths(stmt->current_row);
  return stmt->result->lengths;
}

//DESODBC: forward declaration due to call from des_store_result
static inline void free_result(DES_RESULT *result);

/* DESODBC:
    Original author: MySQL
    Modified by: DESODBC Developer
*/
inline static DES_RESULT *des_store_result(STMT *stmt) {
  DES_RESULT *res = new DES_RESULT(stmt);
  if (!res) throw std::bad_alloc();

  res->field_count = res->internal_table->col_count();
  if (res->field_count == 0) {
    free_result(res);
    return nullptr;
  }

  res->row_count = res->internal_table->row_count();

  res->current_field = 0;
  res->eof = true;
  res->unbuffered_fetch_cancelled = false;
  res->metadata = RESULTSET_METADATA_FULL;

  DES_FIELD *fields = (DES_FIELD *)malloc(res->field_count * sizeof(DES_FIELD));

  for (int i = 0; i < res->field_count; ++i) {
    DES_FIELD *field = fields + i;
    memcpy(field, res->internal_table->get_DES_FIELD(i), sizeof(DES_FIELD));
  }

  res->fields = fields;

  DES_DATA *data = new DES_DATA;
  if (!data) throw std::bad_alloc();

  data->rows = res->row_count;
  data->fields = res->field_count;

  data->data = res->internal_table->generate_DES_ROWS(0);

  res->data = data;
  res->data_cursor = res->data->data;

  res->lengths = res->internal_table->fetch_lengths(0);

  res->current_row = nullptr;
  res->row = nullptr;

  return res;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
inline static char *copy(char *old) {
  if (!old) return nullptr;
  char *cpy = new char[strlen(old) + 1];
  memcpy(cpy, old, strlen(old));
  cpy[strlen(old)] = '\0';
  return cpy;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
inline static void memcpy(DES_FIELD *cpy, DES_FIELD *old) {
  cpy->name = copy(old->name);
  cpy->org_name = copy(old->org_name);
  cpy->table = copy(old->table);
  cpy->org_table = copy(old->org_table);
  cpy->db = copy(old->db);
  cpy->catalog = copy(old->catalog);
  cpy->def = copy(old->def);
  cpy->length = old->length;
  cpy->max_length = old->max_length;
  cpy->name_length = old->name_length;
  cpy->org_name_length = old->org_name_length;
  cpy->table_length = old->table_length;
  cpy->org_table_length = old->org_table_length;
  cpy->db_length = old->db_length;
  cpy->catalog_length = old->catalog_length;
  cpy->def_length = old->def_length;
  cpy->flags = old->flags;
  cpy->decimals = old->decimals;
  cpy->charsetnr = old->charsetnr;
  cpy->type = old->type;
  cpy->extension = malloc(sizeof(old->extension));
}

/* DESODBC:
    Original author: DESODBC Developer
*/
inline static DES_FIELD *copy(DES_FIELD *old) {
  if (!old) return nullptr;
  DES_FIELD *cpy = new DES_FIELD;
  memcpy(cpy, old);
  return cpy;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
inline static char **copy(char **old, int size) {
  if (!old) return nullptr;
  char **cpy = (char **)malloc(sizeof(char *) * size);
  for (int i = 0; i < size; ++i) {
    char *old_elem = *(old + i);
    char **cpy_pos = cpy + i;
    if (!old_elem)
      *(cpy_pos) = nullptr;
    else {
      *(cpy_pos) = (char *)malloc(sizeof(char) * strlen(old_elem));
      memcpy(cpy_pos, old_elem, sizeof(char) * strlen(old_elem));
    }
  }
  return cpy;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
inline static DES_ROWS *copy(DES_ROWS *old, int n_fields) {
  if (!old) return nullptr;
  DES_ROWS *cpy = new DES_ROWS;
  cpy->length = old->length;
  cpy->data = copy(old->data, n_fields);
  if (!old->next)
    cpy->next = nullptr;
  else {
    cpy->next = copy(old->next, n_fields);
  }
  return cpy;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
inline static DES_DATA *copy(DES_DATA *old) {
  if (!old) return nullptr;
  DES_DATA *cpy = new DES_DATA;
  cpy->rows = old->rows;
  cpy->fields = old->fields;
  cpy->data = copy(old->data, old->fields);

  return cpy;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
inline static ResultTable *copy(ResultTable *old) {
  if (!old) return nullptr;
  ResultTable *cpy = new ResultTable;
  cpy->params.table_name = old->params.table_name;
  cpy->params.type_requested = old->params.type_requested;
  cpy->params.pk_table_name = old->params.fk_table_name;
  cpy->params.column_name = old->params.column_name;
  cpy->params.catalog_name = old->params.catalog_name;
  cpy->params.table_type = old->params.table_type;
  cpy->params.metadata_id = old->params.metadata_id;
  cpy->table_name = old->table_name;
  cpy->command_type = old->command_type;
  cpy->str = old->str;
  cpy->names_ordered = old->names_ordered;
  cpy->columns = old->columns;

  // Making the undone deep copies
  for (auto pair : cpy->columns) {
    Column col = pair.second;
    col.field = copy(col.field);
    for (int i = 0; i < col.values.size(); ++i) {
      char *old_ptr = col.values[i];
      col.values[i] = new char;
      memcpy(col.values[i], old_ptr, strlen(old_ptr));
    }
  }

  return cpy;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
inline static DES_RESULT *copy(DES_RESULT *old) {
  if (!old) return nullptr;
  DES_RESULT *cpy = new DES_RESULT;
  cpy->row_count = old->row_count;
  cpy->field_count = old->field_count;

  cpy->fields = (DES_FIELD *)malloc(cpy->field_count * sizeof(DES_FIELD));
  for (int i = 0; i < cpy->field_count; ++i) {
    DES_FIELD *old_field = old->fields + i;
    DES_FIELD *new_field = cpy->fields + i;
    memcpy(new_field, old_field);
  }
  cpy->data = copy(old->data);
  cpy->data_cursor =
      cpy->data->data;  // When copying the result table, we are resetting the
                        // cursor. TODO: check if appropriate

  cpy->lengths =
      (unsigned long *)malloc(sizeof(unsigned long) * cpy->field_count);

  for (int i = 0; i < cpy->field_count; ++i) {
    unsigned long *old_length = old->lengths + i;
    unsigned long *new_length = cpy->lengths + i;
    *new_length = *old_length;
  }

  cpy->row = copy(old->row, cpy->field_count);
  cpy->current_row = copy(old->current_row, cpy->field_count);

  cpy->internal_table = copy(old->internal_table);

  return cpy;
}

static inline void free_result(ResultTable *table) {
    /*
        ResultTable itself does not have attributes in heap (except
        DBC, but we must not delete it), but Column does.
    */
  for (int i = 0; i < table->names_ordered.size(); ++i) {
      Column col = table->columns[table->names_ordered[i]];
    if (col.field && col.new_heap_used) {
      delete col.field->name;
      col.field->name = nullptr;

      delete col.field->org_name;
      col.field->org_name = nullptr;

      delete col.field->table;
      col.field->table = nullptr;

      delete col.field->org_table;
      col.field->org_table = nullptr;
    }
    delete col.field;
    for (int j = 0; j < col.values.size(); ++j) {
    delete col.values[j];
    col.values[j] = nullptr;
    }
  }

  delete table;
  
}

static inline void free_result(char** ptr, int size) {
  if (!ptr) return;

  for (int i = 0; i < size; ++i) {
    char *elem = *(ptr + i);
    if (elem) delete elem;
    elem = nullptr;
  }
  delete ptr;
}

inline static void free_result(DES_ROWS *rows, int n_fields) {
  if (!rows) return;
  free_result(rows->data, rows->length);
  if (rows->next){
    free_result(rows->next, n_fields);
  }
  delete rows;
}
static inline void free_result(DES_DATA *data) {
  if (!data) return;
  free_result(data->data, data->fields);

  delete data;
}

static inline void free_result(DES_RESULT *result) {
  if (!result) return;

  delete[] result->fields;
  result->fields = nullptr;

  free_result(result->data);
  result->data = nullptr;

  delete result->lengths;
  result->lengths = nullptr;

  free_result(result->row, result->field_count);
  result->row = nullptr;

  free_result(result->current_row, result->field_count);
  result->current_row = nullptr;

  free_result(result->internal_table);
  result->internal_table = nullptr;

  delete result;

}

/* DESODBC:
    Original author: MySQL
    Modified by: DESODBC Developer
*/
inline static void des_free_result(DES_RESULT *result) { free_result(result); }

struct ODBC_RESULTSET {
  DES_RESULT *res = nullptr;
  ODBC_RESULTSET(DES_RESULT *r = nullptr) : res(r) {}

  void reset(DES_RESULT *r = nullptr) {
    if (res) des_free_result(res);
    res = r;
  }

  DES_RESULT *release() {
    DES_RESULT *tmp = res;
    res = nullptr;
    return tmp;
  }

  DES_RESULT *operator=(DES_RESULT *r) {
    reset(r);
    return res;
  }

  operator DES_RESULT *() const { return res; }

  operator bool() const { return res != nullptr; }

  ~ODBC_RESULTSET() { reset(); }
};

/* DESODBC:
    Rename done.
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
namespace desodbc {
struct HENV {
  SQLHENV henv = nullptr;

  HENV(unsigned long v) {
    size_t ver = v;
    SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &henv);

    if (SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)ver, 0) !=
        SQL_SUCCESS) {
      throw DESERROR(SQL_HANDLE_ENV, henv, SQL_ERROR);
    }
  }

  HENV() : HENV(SQL_OV_ODBC3) {}

  operator SQLHENV() const { return henv; }

  ~HENV() { SQLFreeHandle(SQL_HANDLE_ENV, henv); }
};

struct HDBC {
  SQLHDBC hdbc = nullptr;
  SQLHENV henv;
  xstring connout = nullptr;
  SQLCHAR ch_out[512] = {0};

  HDBC(SQLHENV env, DataSource *params) : henv(env) {
    SQLWSTRING string_connect_in;

    assert((bool)params->opt_DRIVER);
    params->opt_DSN.set_default(nullptr);
    string_connect_in = params->to_kvpair(';');
    if (SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc) != SQL_SUCCESS) {
      throw DESERROR(SQL_HANDLE_ENV, henv, SQL_ERROR);
    }

    if (SQLDriverConnectW(hdbc, NULL, (SQLWCHAR *)string_connect_in.c_str(),
                          SQL_NTS, NULL, 0, NULL,
                          SQL_DRIVER_NOPROMPT) != SQL_SUCCESS) {
      throw DESERROR(SQL_HANDLE_DBC, hdbc, SQL_ERROR);
    }
  }

  operator SQLHDBC() const { return hdbc; }

  ~HDBC() {
    SQLDisconnect(hdbc);
    SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
  }
};

struct HSTMT {
  SQLHDBC hdbc;
  SQLHSTMT hstmt = nullptr;
  HSTMT(SQLHDBC dbc) : hdbc(dbc) {
    if (SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt) != SQL_SUCCESS) {
      throw DESERROR(SQL_HANDLE_STMT, hstmt, SQL_ERROR);
    }
  }

  operator SQLHSTMT() const { return hstmt; }

  ~HSTMT() { SQLFreeHandle(SQL_HANDLE_STMT, hstmt); }
};
};  // namespace desodbc

extern std::string thousands_sep, decimal_point, default_locale;
#ifndef _UNIX_
extern HINSTANCE NEAR s_hModule; /* DLL handle. */
#endif

#ifdef WIN32
extern std::string current_dll_location;
extern std::string default_plugin_location;
#endif

/*
  Resource defines for "SQLDriverConnect" dialog box
*/
#define ID_LISTBOX 100
#define CONFIGDSN 1001
#define CONFIGDEFAULT 1002
#define EDRIVERCONNECT 1003

#include "myutil.h"
#include "stringutil.h"

/* DESODBC:
    Renamed from the original MySQLColAttribute()
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DESColAttribute(SQLHSTMT hstmt, SQLUSMALLINT column,
                                  SQLUSMALLINT attrib, SQLCHAR **char_attr,
                                  SQLLEN *num_attr);

/* DESODBC:
    This function corresponds to the
    implementation of SQLColumns, which was
    MySQLColumns in MyODBC. We have reused
    some of its skeleton.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DES_SQLColumns(SQLHSTMT hstmt, SQLCHAR *catalog,
                                 SQLSMALLINT catalog_len, SQLCHAR *schema,
                                 SQLSMALLINT schema_len, SQLCHAR *sztable,
                                 SQLSMALLINT table_len, SQLCHAR *column,
                                 SQLSMALLINT column_len);

/* DESODBC:
    This function corresponds to the
    implementation of SQLConnect, which was
    MySQLConnect in MyODBC. We have reused
    almost everything.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DES_SQLConnect(SQLHDBC hdbc, SQLWCHAR *szDSN,
                                 SQLSMALLINT cbDSN, SQLWCHAR *szUID,
                                 SQLSMALLINT cbUID, SQLWCHAR *szAuth,
                                 SQLSMALLINT cbAuth);

/* DESODBC:
    Renamed from the original MySQLDescribeCol()
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DESDescribeCol(SQLHSTMT hstmt, SQLUSMALLINT column,
                                 SQLCHAR **name, SQLSMALLINT *need_free,
                                 SQLSMALLINT *type, SQLULEN *def,
                                 SQLSMALLINT *scale, SQLSMALLINT *nullable);

/* DESODBC:
    This function corresponds to the
    implementation of SQLDriverConnect, which was
    MySQLDriverConnect in MyODBC. We have reused
    almost everything.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DES_SQLDriverConnect(SQLHDBC hdbc, SQLHWND hwnd, SQLWCHAR *in,
                                       SQLSMALLINT in_len, SQLWCHAR *out,
                                       SQLSMALLINT out_max,
                                       SQLSMALLINT *out_len,
                                       SQLUSMALLINT completion);

/* DESODBC:
    This function corresponds to the
    implementation of SQLForeignKeys, which was
    MySQLForeignKeys in MyODBC. We have reused
    some of its skeleton.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DES_SQLForeignKeys(
    SQLHSTMT hstmt, SQLCHAR *pkcatalog, SQLSMALLINT pkcatalog_len,
    SQLCHAR *pkschema, SQLSMALLINT pkschema_len, SQLCHAR *pktable,
    SQLSMALLINT pktable_len, SQLCHAR *fkcatalog, SQLSMALLINT fkcatalog_len,
    SQLCHAR *fkschema, SQLSMALLINT fkschema_len, SQLCHAR *fktable,
    SQLSMALLINT fktable_len);

/* DESOBDC:
   We are using the MySQL cursor name because it does
   not conflict with anything regarding DES or DESODBC.
*/
SQLCHAR *MySQLGetCursorName(HSTMT hstmt);

/* DESODBC:

    Renamed from the original MySQLGetInfo and modified
    according to DES' needs.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DESGetInfo(SQLHDBC hdbc, SQLUSMALLINT fInfoType,
                             SQLCHAR **char_info, SQLPOINTER num_info,
                             SQLSMALLINT *value_len);

/* DESODBC:

    Renamed from the original MySQLGetConnectAttr.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DESGetConnectAttr(SQLHDBC hdbc, SQLINTEGER attrib,
                                    SQLCHAR **char_attr, SQLPOINTER num_attr);


SQLRETURN MySQLGetDescField(SQLHDESC hdesc, SQLSMALLINT recnum, SQLSMALLINT fldid,
                          SQLPOINTER valptr, SQLINTEGER buflen,
                          SQLINTEGER *strlen);

/* DESODBC:
    This function corresponds to the
    implementation of SQLGetDiagField, which was
    MySQLGetDiagField in MyODBC. We have reused
    most of it.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DES_SQLGetDiagField(SQLSMALLINT handle_type, SQLHANDLE handle,
                                      SQLSMALLINT record,
                                      SQLSMALLINT identifier,
                                      SQLCHAR **char_value,
                                      SQLPOINTER num_value);

SQLRETURN SQL_API MySQLGetDiagRec(SQLSMALLINT handle_type, SQLHANDLE handle,
                                  SQLSMALLINT record, SQLCHAR **sqlstate,
                                  SQLINTEGER *native, SQLCHAR **message);

/* DESODBC:
    Renamed from the original MySQLGetStmtAttr
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DESGetStmtAttr(SQLHSTMT hstmt, SQLINTEGER Attribute,
                                 SQLPOINTER ValuePtr,
                                 SQLINTEGER BufferLength
                                 __attribute__((unused)),
                                 SQLINTEGER *StringLengthPtr);

/* DESODBC:

    Renamed from the original MySQLGetTypeInfo
    and modified according to DES' needs.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DESGetTypeInfo(SQLHSTMT hstmt, SQLSMALLINT fSqlType);

/* DESODBC:
    Renamed from the original MySQLPrepare
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DESPrepare(SQLHSTMT hstmt, SQLCHAR *query, SQLINTEGER len,
                             bool reset_select_limit, bool force_prepare);

/* DESODBC:
    This function corresponds to the
    implementation of SQLPrimaryKeys, which was
    MySQLPrimaryKeys in MyODBC. We have reused
    some of its skeleton.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DES_SQLPrimaryKeys(SQLHSTMT hstmt, SQLCHAR *catalog,
                                     SQLSMALLINT catalog_len, SQLCHAR *schema,
                                     SQLSMALLINT schema_len, SQLCHAR *table,
                                     SQLSMALLINT table_len);

/* DESODBC:

    Renamed from the original MySQLSetConnectAttr
    and modified according to DES' needs.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DESSetConnectAttr(SQLHDBC hdbc, SQLINTEGER Attribute,
                                    SQLPOINTER ValuePtr,
                                    SQLINTEGER StringLengthPtr);

/* DESOBDC:
   We are using the MySQL cursor name because it does
   not conflict with anything regarding DES or DESODBC.
*/
SQLRETURN SQL_API MySQLSetCursorName(SQLHSTMT hstmt, SQLCHAR *name,
                                     SQLSMALLINT len);

/* DESODBC:

    Renamed from the original MySQLSetStmtAttr.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DESSetStmtAttr(SQLHSTMT hstmt, SQLINTEGER attribute,
                                 SQLPOINTER value, SQLINTEGER len);

/* DESODBC:
    This function corresponds to the
    implementation of SQLSpecialColumns, which was
    MySQLSpecialColumns in MyODBC. We have reused
    some of its skeleton.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DES_SQLSpecialColumns(SQLHSTMT hstmt, SQLUSMALLINT type,
                                        SQLCHAR *catalog,
                                        SQLSMALLINT catalog_len,
                                        SQLCHAR *schema, SQLSMALLINT schema_len,
                                        SQLCHAR *table, SQLSMALLINT table_len,
                                        SQLUSMALLINT scope,
                                        SQLUSMALLINT nullable);

/* DESODBC:
    This function corresponds to the
    implementation of SQLStatistics, which was
    MySQLStatistics in MyODBC. We have reused
    some of its skeleton.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DES_SQLStatistics(SQLHSTMT hstmt, SQLCHAR *catalog,
                                    SQLSMALLINT catalog_len, SQLCHAR *schema,
                                    SQLSMALLINT schema_len, SQLCHAR *table,
                                    SQLSMALLINT table_len, SQLUSMALLINT unique,
                                    SQLUSMALLINT accuracy);

/* DESODBC:
    This function corresponds to the
    implementation of SQLTables, which was
    MySQLTables in MyODBC. We have reused
    some of its skeleton.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN SQL_API DES_SQLTables(SQLHSTMT hstmt, SQLCHAR *catalog,
                                SQLSMALLINT catalog_len, SQLCHAR *schema,
                                SQLSMALLINT schema_len, SQLCHAR *table,
                                SQLSMALLINT table_len, SQLCHAR *type,
                                SQLSMALLINT type_len);

#endif /* __DRIVER_H__ */
