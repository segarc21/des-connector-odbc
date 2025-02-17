// Copyright (c) 2001, 2024, Oracle and/or its affiliates.
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
  @file driver.h
  @brief Definitions needed by the driver
*/

#ifndef __DRIVER_H__
#define __DRIVER_H__

#include "../MYODBC_MYSQL.h"
#include "../MYODBC_CONF.h"
#include "../MYODBC_ODBC.h"
#include "telemetry.h"
#include "installer.h"

/* Disable _attribute__ on non-gcc compilers. */
#if !defined(__attribute__) && !defined(__GNUC__) && !defined(__clang__)
# define __attribute__(arg)
#endif

#ifdef APSTUDIO_READONLY_SYMBOLS
#define WIN32	/* Hack for rc files */
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


#include "error.h"
#include "parse.h"
#include <vector>
#include <list>
#include <mutex>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>

#define LOCK_STMT(S) CHECK_HANDLE(S); \
  std::unique_lock<std::recursive_mutex> slock(((STMT*)S)->lock)
#define LOCK_STMT_DEFER(S) CHECK_HANDLE(S); \
  std::unique_lock<std::recursive_mutex> slock(((STMT*)S)->lock, std::defer_lock)
#define DO_LOCK_STMT() slock.lock();

#define LOCK_DBC(D) std::unique_lock<std::recursive_mutex> dlock(((DBC*)D)->lock)
#define LOCK_DBC_DEFER(D) std::unique_lock<std::recursive_mutex> dlock(((DBC*)D)->lock, std::defer_lock)
#define DO_LOCK_DBC() dlock.lock();

#define LOCK_ENV(E) std::unique_lock<std::mutex> elock(E->lock)

// SQL_DRIVER_CONNECT_ATTR_BASE is not defined in all driver managers.
// Therefore use a custom constant until it becomes a standard.
#define DES_DRIVER_CONNECT_ATTR_BASE 0x00004000

#define CB_FIDO_GLOBAL DES_DRIVER_CONNECT_ATTR_BASE + 0x00001000
#define CB_FIDO_CONNECTION DES_DRIVER_CONNECT_ATTR_BASE + 0x00001001

#if defined(_WIN32) || defined(WIN32)
# define INTFUNC  __stdcall
# define EXPFUNC  __stdcall
# if !defined(HAVE_LOCALTIME_R)
#  define HAVE_LOCALTIME_R 1
# endif
#else
# define INTFUNC PASCAL
# define EXPFUNC __export CALLBACK
/* Simple macros to make dl look like the Windows library funcs. */
# define HMODULE void*
# define LoadLibrary(library) dlopen((library), RTLD_GLOBAL | RTLD_LAZY)
# define GetProcAddress(module, proc) dlsym((module), (proc))
# define FreeLibrary(module) dlclose((module))
#endif

#define ODBC_DRIVER	  "DES " DESODBC_STRSERIES " Driver"
#define DRIVER_NAME	  "DES ODBC " DESODBC_STRSERIES " Driver"
#define DRIVER_NONDSN_TAG "DRIVER={DES ODBC " DESODBC_STRSERIES " Driver}"

#if defined(__APPLE__)

#define DRIVER_QUERY_LOGFILE "/tmp/myodbc.sql"

#elif defined(_UNIX_)

#define DRIVER_QUERY_LOGFILE "/tmp/myodbc.sql"

#else

#define DRIVER_QUERY_LOGFILE "myodbc.sql"

#endif

/*
   Internal driver definitions
*/

/* Options for SQLFreeStmt */
#define FREE_STMT_RESET_BUFFERS 1000
#define FREE_STMT_RESET 1001
#define FREE_STMT_CLEAR_RESULT 1
#define FREE_STMT_DO_LOCK 2

#define DES_3_21_PROTOCOL 10	  /* OLD protocol */
#define CHECK_IF_ALIVE	    1800  /* Seconds between queries for ping */

#define DES_MAX_CURSOR_LEN 18   /* Max cursor name length */
#define DES_STMT_LEN 1024	  /* Max statement length */
#define DES_STRING_LEN 1024	  /* Max string length */
#define DES_MAX_SEARCH_STRING_LEN NAME_LEN+10 /* Max search string length */
/* Max Primary keys in a cursor * WHERE clause */
#define MY_MAX_PK_PARTS 32

#ifndef NEAR
#define NEAR
#endif

/* We don't make any assumption about what the default may be. */
#ifndef DEFAULT_TXN_ISOLATION
# define DEFAULT_TXN_ISOLATION 0
#endif

/* For compatibility with old mysql clients - defining error */
#ifndef ER_MUST_CHANGE_PASSWORD_LOGIN
# define ER_MUST_CHANGE_PASSWORD_LOGIN 1820
#endif

#ifndef CR_AUTH_PLUGIN_CANNOT_LOAD_ERROR
# define CR_AUTH_PLUGIN_CANNOT_LOAD_ERROR 2059
#endif

#ifndef SQL_PARAM_DATA_AVAILABLE
# define SQL_PARAM_DATA_AVAILABLE 101
#endif

/* Connection flags to validate after the connection*/
#define CHECK_AUTOCOMMIT_ON	1  /* AUTOCOMMIT_ON */
#define CHECK_AUTOCOMMIT_OFF	2  /* AUTOCOMMIT_OFF */

/* implementation or application descriptor? */
typedef enum { DESC_IMP, DESC_APP } desc_ref_type;

/* parameter or row descriptor? */
typedef enum { DESC_PARAM, DESC_ROW, DESC_UNKNOWN } desc_desc_type;

/* header or record field? (location in descriptor) */
typedef enum { DESC_HDR, DESC_REC } fld_loc;

typedef void (*fido_callback_func)(const char*);
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

#define PR_RIR  P_ROW(P_RI)
#define PR_WIR (P_ROW(P_WI) | PR_RIR)
#define PR_RAR  P_ROW(P_RA)
#define PR_WAR (P_ROW(P_WA) | PR_RAR)
#define PR_RIP  P_PAR(P_RI)
#define PR_WIP (P_PAR(P_WI) | PR_RIP)
#define PR_RAP  P_PAR(P_RI)
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
#define ARD_IS_BOUND(d) (d)&&((d)->data_ptr || (d)->octet_length_ptr)

/* get the dbc from a descriptor */
#define DESC_GET_DBC(X) (((X)->alloc_type == SQL_DESC_ALLOC_USER) ? \
                         (X)->dbc : (X)->stmt->dbc)

#define IS_BOOKMARK_VARIABLE(S) if (S->stmt_options.bookmarks != \
                                    SQL_UB_VARIABLE) \
  { \
    stmt->set_error("HY092", "Invalid attribute identifier", 0); \
    return SQL_ERROR; \
  }


/* data-at-exec type */
#define DAE_NORMAL 1 /* normal SQLExecute() */
#define DAE_SETPOS_INSERT 2 /* SQLSetPos() insert */
#define DAE_SETPOS_UPDATE 3 /* SQLSetPos() update */
/* data-at-exec handling done for current SQLSetPos() call */
#define DAE_SETPOS_DONE 10

#define DONT_USE_LOCALE_CHECK(STMT) if (!STMT->dbc->ds.opt_NO_LOCALE)

#ifndef HAVE_TYPE_VECTOR
#define DES_TYPE_VECTOR 242
#endif

#if defined _WIN32

  #define DECLARE_LOCALE_HANDLE int loc;

    #define __LOCALE_SET(LOC) \
    { \
      loc = _configthreadlocale(0); \
      _configthreadlocale(_ENABLE_PER_THREAD_LOCALE); \
      setlocale(LC_NUMERIC, LOC);  /* force use of '.' as decimal point */ \
    }

  #define __LOCALE_RESTORE() \
    { \
      setlocale(LC_NUMERIC, default_locale.c_str()); \
      _configthreadlocale(loc); \
    }

#elif defined LC_GLOBAL_LOCALE
  #define DECLARE_LOCALE_HANDLE locale_t nloc;

  #define __LOCALE_SET(LOC) \
    { \
      nloc = newlocale(LC_CTYPE_MASK, LOC, (locale_t)0); \
      uselocale(nloc); \
    }

  #define __LOCALE_RESTORE() \
    { \
      uselocale(LC_GLOBAL_LOCALE); \
      freelocale(nloc); \
    }
#else

  #define DECLARE_LOCALE_HANDLE

  #define __LOCALE_SET(LOC) \
      { \
        setlocale(LC_NUMERIC, LOC);  /* force use of '.' as decimal point */ \
      }

  #define __LOCALE_RESTORE() \
      { \
        setlocale(LC_NUMERIC, default_locale.c_str()); \
      }
#endif

#define C_LOCALE_SET(STMT) \
    DONT_USE_LOCALE_CHECK(STMT) \
    __LOCALE_SET("C")

#define DEFAULT_LOCALE_SET(STMT) \
    DONT_USE_LOCALE_CHECK(STMT) \
    __LOCALE_RESTORE()


typedef struct {
  int perms;
  SQLSMALLINT data_type; /* SQL_IS_SMALLINT, etc */
  fld_loc loc;
  size_t offset; /* offset of field in struct */
} desc_field;

/* descriptor */
struct STMT;

struct DESCREC{
  /* ODBC spec fields */
  SQLINTEGER  auto_unique_value; /* row only */
  SQLCHAR *   base_column_name; /* row only */
  SQLCHAR *   base_table_name; /* row only */
  SQLINTEGER  case_sensitive; /* row only */
  SQLCHAR *   catalog_name; /* row only */
  SQLSMALLINT concise_type;
  SQLPOINTER  data_ptr;
  SQLSMALLINT datetime_interval_code;
  SQLINTEGER  datetime_interval_precision;
  SQLLEN      display_size; /* row only */
  SQLSMALLINT fixed_prec_scale;
  SQLLEN  *   indicator_ptr;
  SQLCHAR *   label; /* row only */
  SQLULEN     length;
  SQLCHAR *   literal_prefix; /* row only */
  SQLCHAR *   literal_suffix; /* row only */
  SQLCHAR *   local_type_name;
  SQLCHAR *   name;
  SQLSMALLINT nullable;
  SQLINTEGER  num_prec_radix;
  SQLLEN      octet_length;
  SQLLEN     *octet_length_ptr;
  SQLSMALLINT parameter_type; /* param only */
  SQLSMALLINT precision;
  SQLSMALLINT rowver;
  SQLSMALLINT scale;
  SQLCHAR *   schema_name; /* row only */
  SQLSMALLINT searchable; /* row only */
  SQLCHAR *   table_name; /* row only */
  SQLSMALLINT type;
  SQLCHAR *   type_name;
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
    //des_bool alloced;
    /* Whether this parameter has been bound by the application
     * (if not, was created by dummy execution) */
    des_bool real_param_done;

    par_struct() : tempbuf(0), is_dae(0), real_param_done(false)
    {}

    par_struct(const par_struct& p) :
      tempbuf(p.tempbuf), is_dae(p.is_dae), real_param_done(p.real_param_done)
    { }

    void add_param_data(const char *chunk, unsigned long length);

    size_t val_length()
    {
      // Return the current position, not the buffer length
      return tempbuf.cur_pos;
    }

    char *val()
    {
      return tempbuf.buf;
    }

    void reset()
    {
      tempbuf.reset();
      is_dae = 0;
    }

  }par;

  /* row-specific */
  struct row_struct{
    DES_FIELD * field; /* Used *only* by IRD */
    ulong datalen; /* actual length, maintained for *each* row */
    /* TODO ugly, but easiest way to handle memory */
    SQLCHAR type_name[40];

    row_struct() : field(nullptr), datalen(0)
    {}

    void reset()
    {
      field = nullptr;
      datalen = 0;
      type_name[0] = 0;
    }
  }row;

  void desc_rec_init_apd();
  void desc_rec_init_ipd();
  void desc_rec_init_ard();
  void desc_rec_init_ird();
  void reset_to_defaults();

  DESCREC(desc_desc_type desc_type, desc_ref_type ref_type)
          : auto_unique_value(0), base_column_name(nullptr), base_table_name(nullptr),
            case_sensitive(0), catalog_name(nullptr), concise_type(0), data_ptr(nullptr),
            datetime_interval_code(0), datetime_interval_precision(0), display_size(0),
            fixed_prec_scale(0), indicator_ptr(nullptr), label(nullptr), length(0),
            literal_prefix(nullptr), literal_suffix(nullptr), local_type_name(nullptr),
            name(nullptr), nullable(0), num_prec_radix(0), octet_length(0),
            octet_length_ptr(nullptr), parameter_type(0), precision(0), rowver(0),
            scale(0), schema_name(nullptr), searchable(0), table_name(nullptr), type(0),
            type_name(nullptr), unnamed(0), is_unsigned(0), updatable(0),
            m_desc_type(desc_type), m_ref_type(ref_type)
  {
    reset_to_defaults();
  }

};


inline bool is_character_data_type(SQLSMALLINT type) {
  switch (type) {
    case SQL_LONGVARCHAR:
    case SQL_CHAR:
    case SQL_VARCHAR:
    case SQL_TYPE_DATE:
    case SQL_TYPE_TIME:
    case SQL_TYPE_TIMESTAMP:
      return true;
    default:
      return false;
  }
}

inline bool is_time_data_type(SQLSMALLINT type) {
  switch (type) {
    case SQL_TYPE_DATE:
    case SQL_TYPE_TIME:
    case SQL_TYPE_TIMESTAMP:
      return true;
    default:
      return false;
  }
}

struct STMT;
struct DBC;

struct DESC {
  /* header fields */
  SQLSMALLINT   alloc_type = 0;
  SQLULEN       array_size = 0;
  SQLUSMALLINT *array_status_ptr = nullptr;
  /* NOTE: This field is defined as SQLINTEGER* in the descriptor
   * documentation, but corresponds to SQL_ATTR_ROW_BIND_OFFSET_PTR or
   * SQL_ATTR_PARAM_BIND_OFFSET_PTR when set via SQLSetStmtAttr(). The
   * 64-bit ODBC addendum says that when set via SQLSetStmtAttr(), this
   * is now a 64-bit value. These two are conflicting, so we opt for
   * the 64-bit value.
   */
  SQLULEN      *bind_offset_ptr = nullptr;
  SQLINTEGER    bind_type = 0;
  SQLLEN        count = 0;  // Only used for SQLGetDescField()
  SQLLEN        bookmark_count = 0;
  /* Everywhere(http://msdn.microsoft.com/en-us/library/ms713560(VS.85).aspx
     http://msdn.microsoft.com/en-us/library/ms712631(VS.85).aspx) I found
     it's referred as SQLULEN* */
  SQLULEN      *rows_processed_ptr = nullptr;

  /* internal fields */
  desc_desc_type  desc_type = DESC_PARAM;
  desc_ref_type   ref_type = DESC_IMP;

  std::vector<DESCREC> bookmark2;
  std::vector<DESCREC> records2;

  DESERROR error;
  STMT *stmt;
  DBC *dbc;

  void free_paramdata();
  void reset();
  SQLRETURN set_field(SQLSMALLINT recnum, SQLSMALLINT fldid,
                      SQLPOINTER val, SQLINTEGER buflen);

  DESC(STMT *p_stmt, SQLSMALLINT p_alloc_type,
    desc_ref_type p_ref_type, desc_desc_type p_desc_type);

  size_t rcount()
  {
    count = (SQLLEN)records2.size();
    return count;
  }

  ~DESC()
  {
    //if (desc_type == DESC_PARAM && ref_type == DESC_APP)
    //  free_paramdata();
  }

  /* SQL_DESC_ALLOC_USER-specific */
    std::list<STMT*> stmt_list;


  void stmt_list_remove(STMT *stmt)
  {
    if (alloc_type == SQL_DESC_ALLOC_USER)
      stmt_list.remove(stmt);
  }

  void stmt_list_add(STMT *stmt)
  {
    if (alloc_type == SQL_DESC_ALLOC_USER)
      stmt_list.emplace_back(stmt);
  }

  inline bool is_apd()
  {
    return desc_type == DESC_PARAM && ref_type == DESC_APP;
  }

  inline bool is_ipd()
  {
    return desc_type == DESC_PARAM && ref_type == DESC_IMP;
  }

  inline bool is_ard()
  {
    return desc_type == DESC_ROW && ref_type == DESC_APP;
  }

  inline bool is_ird()
  {
    return desc_type == DESC_ROW && ref_type == DESC_IMP;
  }

  SQLRETURN set_error(char *state, const char *message, uint errcode);

};

/* Statement attributes */

struct STMT_OPTIONS
{
  SQLUINTEGER      cursor_type = 0;
  SQLUINTEGER      simulateCursor = 0;
  SQLULEN          max_length = 0, max_rows = 0;
  SQLULEN          query_timeout = -1;
  SQLUSMALLINT    *rowStatusPtr_ex = nullptr; /* set by SQLExtendedFetch */
  bool            retrieve_data = true;
  SQLUINTEGER     bookmarks = 0;
  void            *bookmark_ptr = nullptr;
  bool            bookmark_insert = false;
};

#define SHARED_MEMORY_NAME "Global\\DESODBC_SHMEM"
#define SHARED_MEMORY_MUTEX_NAME "Global\\DESODBC_SHMEM_MUTEX"
#define QUERY_MUTEX_NAME "Global\\DESODBC_QUERY_MUTEX"
#define REQUEST_HANDLE_EVENT_NAME "Global\\DESODBC_REQUEST_HANDLE_EVENT"
#define REQUEST_HANDLE_MUTEX_NAME "Global\\DESODBC_REQUEST_MUTEX_EVENT"
#define HANDLE_SENT_EVENT_NAME "Global\\DESODBC_HANDLE_SENT_EVENT"

#define MAX_CLIENTS 256

class Cursor {
  int row; //starts by 1. 0 means: every row

};

struct PidsConnected {
  DWORD pids_connected[MAX_CLIENTS];
  int size = 0;
};

struct HandleSharingInfo {
  DWORD pid_handle_petitioner = 0;
  DWORD pid_handle_petitionee = 0;
  HANDLE in_handle = NULL;
  HANDLE out_handle = NULL;
};

struct SharedMemory {
  PidsConnected pids_connected_struct;

  HandleSharingInfo handle_sharing_info;

  bool des_process_created = false;
};

/* Environment handler */

struct ENV
{
  HANDLE query_mutex;
  HANDLE shared_memory_mutex;
  HANDLE request_handle_mutex;

  HANDLE request_handle_event;
  HANDLE handle_sent_event;

  std::unique_ptr<std::thread> share_handle_thread;

  PROCESS_INFORMATION process_info;
  STARTUPINFOW startup_info_unicode;
  HANDLE driver_to_des_out_rpipe = NULL;
  HANDLE driver_to_des_out_wpipe = NULL;
  HANDLE driver_to_des_in_rpipe = NULL;
  HANDLE driver_to_des_in_wpipe = NULL;

  bool initialized = false;

  int number_of_connections = 0;

  SharedMemory *shmem = NULL;

  SQLINTEGER   odbc_ver;
  std::list<DBC*> conn_list;
  DESERROR      error;
  std::mutex lock;

  ENV(SQLINTEGER ver) : odbc_ver(ver)
  {
    HANDLE handle_map_file_shmem = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        sizeof(SharedMemory),
        SHARED_MEMORY_NAME
    );

    if (handle_map_file_shmem == nullptr) {
      exit(1);  // TODO: handle this with SQL errors. Generally the problem is the lack of admin privileges.
    }

    shmem = (SharedMemory *)MapViewOfFile(
        handle_map_file_shmem,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(SharedMemory)
    );

    if (shmem == nullptr) {
      exit(1);  // TODO: handle this with SQL errors
    }

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;


    this->query_mutex = CreateMutexExA(&sa, QUERY_MUTEX_NAME, 0, SYNCHRONIZE);
    this->shared_memory_mutex = CreateMutexExA(&sa, SHARED_MEMORY_MUTEX_NAME, 0, SYNCHRONIZE);
    this->request_handle_mutex = CreateMutexExA(&sa, REQUEST_HANDLE_MUTEX_NAME, 0, SYNCHRONIZE);

    if (this->query_mutex == nullptr) {
      exit(1);  // TODO: handle this with SQL errors
    }

    this->request_handle_event =
        CreateEventA(NULL,
                    TRUE,
                    FALSE,
                    REQUEST_HANDLE_EVENT_NAME
        );

    this->handle_sent_event =
        CreateEventA(NULL,
                     TRUE,
                     FALSE,
                     HANDLE_SENT_EVENT_NAME
        );

  }

  void add_dbc(DBC* dbc);
  void remove_dbc(DBC* dbc);
  bool has_connections();

  ~ENV() {
      share_handle_thread->join();
  }
};

/* Connection handler */
struct DBC
{
  ENV           *env;
  DES         *des;
  std::list<STMT*> stmt_list;
  std::list<DESC*> desc_list; // Explicit descriptors
  STMT_OPTIONS  stmt_options;
  DESERROR       error;
  FILE          *query_log = nullptr;
  char          st_error_prefix[255] = { 0 };
  std::string   database;
  SQLUINTEGER   login_timeout = 0;
  time_t        last_query_time = 0;
  int           txn_isolation = 0;
  uint          port = 0;
  uint          cursor_count = 0;
  ulong         net_buffer_len = 0;
  uint          commit_flag = 0;
  bool          has_query_attrs = false;
  std::recursive_mutex lock;

  // Whether SQL*ConnectW was used
  bool          unicode = false;
  // Connection charset ('ANSI' or utf-8)
  desodbc::CHARSET_INFO *cxn_charset_info = nullptr;
  // data source used to connect (parsed or stored)
  DataSource    ds;
  // value of the sql_select_limit currently set for a session
  //   (SQLULEN)(-1) if wasn't set
  SQLULEN       sql_select_limit = -1;
  // Connection have been put to the pool
  int           need_to_wakeup = 0;
  fido_callback_func fido_callback = nullptr;

  telemetry::Telemetry<DBC> telemetry;

  DBC(ENV *p_env);
  SQLRETURN createPipes();
  SQLRETURN openPipes();
  SQLRETURN createDESProcess(SQLWCHAR* des_path);
  void free_explicit_descriptors();
  void free_connection_stmts();
  void add_desc(DESC* desc);
  void remove_desc(DESC *desc);
  SQLRETURN set_error(char *state, const char *message, uint errcode);
  SQLRETURN set_error(char *state);
  SQLRETURN connect(DataSource *ds);
  void execute_prep_stmt(DES_STMT *pstmt, std::string &query,
    std::vector<DES_BIND> &param_bind, DES_BIND *result_bind);

  inline bool transactions_supported()
  { return des->server_capabilities & CLIENT_TRANSACTIONS; }

  inline bool autocommit_is_on()
  { return des->server_status & SERVER_STATUS_AUTOCOMMIT; }

  void close();
  ~DBC();

  void set_charset(std::string charset);
  SQLRETURN set_charset_options(const char* charset);
  SQLRETURN set_error(desodbc_errid errid, const char* errtext,
    SQLINTEGER errcode);
  SQLRETURN execute_query(const char *query,
    SQLULEN query_length, des_bool req_lock);
};

extern DBC *dbc_global_var;


/* Statement states */

enum DES_STATE { ST_UNKNOWN = 0, ST_PREPARED, ST_PRE_EXECUTED, ST_EXECUTED };
enum DES_DUMMY_STATE { ST_DUMMY_UNKNOWN = 0, ST_DUMMY_PREPARED, ST_DUMMY_EXECUTED };


struct DES_LIMIT_CLAUSE
{
  unsigned long long  offset;
  unsigned int        row_count;
  const char                *begin, *end;
  DES_LIMIT_CLAUSE(unsigned long long offs, unsigned int rc, char* b, char *e) :
    offset(offs), row_count(rc), begin(b), end(e)
  {}

};

struct DES_LIMIT_SCROLLER
{
   tempBuf buf;
   char *query, *offset_pos;
   unsigned int       row_count;
   unsigned long long start_offset;
   unsigned long long next_offset, total_rows, query_len;

   DES_LIMIT_SCROLLER() : buf(1024), query(buf.buf), offset_pos(query),
                         row_count(0), start_offset(0), next_offset(0),
                         total_rows(0), query_len(0)
   {}

   void extend_buf(size_t new_size) { buf.extend_buffer(new_size); }
   void reset() { next_offset = 0; offset_pos = query; }
};

/* Statement primary key handler for cursors */
struct DES_PK_COLUMN
{
  char	      name[NAME_LEN+1];
  des_bool     bind_done;

  DES_PK_COLUMN()
  {
    name[0] = 0;
    bind_done = FALSE;
  }
};


/* Statement cursor handler */
struct DESCURSOR
{
  std::string name;
  uint	       pk_count;
  des_bool      pk_validated;
  DES_PK_COLUMN pkcol[MY_MAX_PK_PARTS];

  DESCURSOR() : pk_count(0), pk_validated(FALSE)
  {}

};

enum OUT_PARAM_STATE
{
  OPS_UNKNOWN= 0,
  OPS_BEING_FETCHED,
  OPS_PREFETCHED,
  OPS_STREAMS_PENDING
};

#define CAT_SCHEMA_SET_FULL(STMT, C, S, V, CZ, SZ, CL, SL) { \
  bool cat_is_set = false; \
  if (!STMT->dbc->ds.opt_NO_CATALOG && (CL || !SL)) \
  { \
    C = V;\
    S = nullptr; \
    cat_is_set = true; \
  } \
  if (!STMT->dbc->ds.opt_NO_SCHEMA && !cat_is_set && SZ) \
  { \
    S = V; \
    C = nullptr; \
  } \
}

#define CAT_SCHEMA_SET(C, S, V) \
  CAT_SCHEMA_SET_FULL(stmt, C, S, V, catalog, schema, catalog_len, schema_len)


/* Main statement handler */

struct GETDATA{
  uint column;      /* Which column is being used with SQLGetData() */
  char *source;     /* Our current position in the source. */
  uchar latest[7];  /* Latest character to be converted. */
  int latest_bytes; /* Bytes of data in latest. */
  int latest_used;  /* Bytes of latest that have been used. */
  ulong src_offset; /* @todo remove */
  ulong dst_bytes;  /* Length of data once it is all converted (in chars). */
  ulong dst_offset; /* Current offset into dest. (ulong)~0L when not set. */

  GETDATA() : column(0), source(NULL), latest_bytes(0),
              latest_used(0), src_offset(0), dst_bytes(0), dst_offset(0)
  {}
};
class ResultTable;

struct STMT;

class DES_RESULT {
 public:
  uint64_t row_count;
  DES_FIELD *fields;
  struct DES_DATA *data;
  DES_ROWS *data_cursor;
  unsigned long *lengths; /* column lengths of current row */
  DES *handle;            /* for unbuffered reads */
  const struct DES_METHODS *methods;
  DES_ROW row;         /* If unbuffered read */
  DES_ROW current_row; /* buffer to current row */
  ResultTable *internal_table;
  unsigned int field_count, current_field;
  bool eof; /* Used by mysql_fetch_row */
  /* mysql_stmt_close() had to cancel this result */
  bool unbuffered_fetch_cancelled;
  enum enum_resultset_metadata metadata;
  void *extension;

  DES_RESULT();
  DES_RESULT(STMT *stmt);
  DES_RESULT(STMT *stmt, const std::string &tapi_output);
};

// Mimicring MySQL extern functions
// (some of them, extracted from the MySQL Server's source code)

inline static DES_ROW des_fetch_row(DES_RESULT *result) {
  DES_ROW tmp = nullptr;
  if (!result->data_cursor) return tmp;
  tmp = result->data_cursor->data;
  result->data_cursor = result->data_cursor->next;
  result->current_row = tmp;

  return tmp;
}

inline static void des_free_result(DES_RESULT *result) {
    delete result; //TODO: delete every field to prevent memory leaks. Just a quick coarse solution (for now, I can't predict which subfields will change)
}


inline static DES_FIELD *des_fetch_field_direct(DES_RESULT *res,
                                                unsigned int fieldnr) {
  if (fieldnr >= res->field_count || !res->fields) return (nullptr);
  return &(res)->fields[fieldnr];
}

inline static unsigned int des_num_fields(DES_RESULT *res) { return res->field_count; }

inline static uint64_t des_num_rows(DES_RESULT *res) {
    return res->row_count;
}

inline static void des_data_seek(DES_RESULT* result, uint64_t offset) {
  DES_ROWS *tmp = nullptr;
  if (result->data)
    for (tmp = result->data->data; offset-- && tmp; tmp = tmp->next)
      ;
  result->current_row = nullptr;
  result->data_cursor = tmp;

}

inline static DES_RESULT* des_stmt_result_metadata(DES_STMT* stmt) {
  return nullptr;
}

inline static DES_RESULT* des_use_result(DES* des) { return nullptr;

}

inline static uint64_t des_stmt_num_rows(DES_STMT* stmt) { return 0;

}

inline static const char* des_stmt_error(DES_STMT* stmt) { return nullptr;

}

inline static int des_stmt_errno(DES_STMT* stmt) { return 0;

}

inline static int des_stmt_fetch(DES_STMT* stmt) { return 0;

}

inline static DES_ROW_OFFSET des_stmt_row_seek(DES_STMT* stmt,
    DES_ROW_OFFSET offset) {
  return NULL;
}

inline static DES_ROW_OFFSET des_row_seek(DES_RESULT* result,
    DES_ROW_OFFSET offset) {
  DES_ROW_OFFSET return_value = result->data_cursor;
  result->current_row = nullptr;
  result->data_cursor = offset;
  return return_value;
}

inline static void des_stmt_data_seek(DES_STMT* stmt, uint64_t offset) {


}

inline static DES_ROW_OFFSET des_stmt_row_tell(DES_STMT* stmt) { return NULL;

}

inline static DES_ROW_OFFSET des_row_tell(DES_RESULT* res) {
  return res->data_cursor;
}

inline static int des_stmt_next_result(DES_STMT* stmt) { return 0;

}

inline static int des_next_result(DES* des) { return -1; //TODO: research

}

struct ODBC_RESULTSET
{
  DES_RESULT *res = nullptr;
  ODBC_RESULTSET(DES_RESULT *r = nullptr) : res(r)
  {}

  void reset(DES_RESULT *r = nullptr)
  { if (res) des_free_result(res); res = r; }

  DES_RESULT *release()
  {
    DES_RESULT *tmp = res;
    res = nullptr;
    return tmp;
  }

  DES_RESULT * operator=(DES_RESULT *r)
  { reset(r); return res; }

  operator DES_RESULT*() const
  { return res; }

  operator bool() const
  { return res != nullptr; }

  ~ODBC_RESULTSET() { reset(); }
};

/*
  A string capable of being a NULL
*/
struct xstring : public std::string
{
  bool m_is_null = false;

  using Base = std::string;

  xstring(std::nullptr_t) : m_is_null(true)
  {}

  xstring(char* s) : m_is_null(s == nullptr),
                  Base(s == nullptr ? "" : std::forward<char*>(s))
  {}

  template <class T>
  xstring(T &&s) : Base(std::forward<T>(s))
  {}

  xstring(SQLULEN v) : Base(std::to_string(v))
  {}

  xstring(long long v) : Base(std::to_string(v))
  {}

  xstring(SQLSMALLINT v) : Base(std::to_string(v))
  {}

  xstring(long int v) : Base(std::to_string(v))
  {}

  xstring(int v) : Base(std::to_string(v))
  {}

  xstring(unsigned int v) : Base(std::to_string(v))
  {}


  const char *c_str() const { return m_is_null ? nullptr : Base::c_str(); }
  size_t size() const { return m_is_null ? 0 : Base::size(); }

  bool is_null() { return m_is_null; }

};

struct ROW_STORAGE
{

  typedef std::vector<xstring> vstr;
  typedef std::vector<const char*> pstr;
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
    Returns true if the data actually existed and was invalidated. False otherwise.
  */
  bool invalidate()
  {
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
  void first_row() { m_cur_row = 0; m_eof = m_rnum == 0; }

  xstring& operator[](size_t idx);

  void set_data(size_t idx, void *data, size_t size)
  {
    if (data)
      m_data[m_cur_row * m_cnum + idx].assign((const char*)data, size);
    else
      m_data[m_cur_row * m_cnum + idx] = nullptr;
    m_eof = false;
  }

  /* Copy row data from bind buffer into the storage one row at a time */
  void set_data(DES_BIND *bind)
  {
    for(size_t i = 0; i < m_cnum; ++i)
    {
      if (!(*bind[i].is_null))
        set_data(i, bind[i].buffer, *(bind[i].length));
      else
        set_data(i, nullptr, 0);
    }
  }

  /* Copy data from the storage into the bind buffer one row at a time */
  void fill_data(DES_BIND *bind)
  {
    if (m_cur_row >= m_rnum || m_eof)
      return;

    for(size_t i = 0; i < m_cnum; ++i)
    {
      auto &data = m_data[m_cur_row * m_cnum + i];
      *(bind[i].is_null) = data.is_null();
      *(bind[i].length) = (unsigned long)(data.is_null() ? -1 : data.length());
      if (!data.is_null())
      {
        size_t copy_zero = bind[i].buffer_length > *(bind[i].length) ? 1 : 0;
        memcpy(bind[i].buffer, data.data(), *(bind[i].length) + copy_zero);
      }
    }
    // Set EOF if the last row was filled
    m_eof = (m_rnum <= (m_cur_row + 1));
    // Increment row counter only if not EOF
    m_cur_row += m_eof ? 0 : 1;
  }

  const xstring & operator=(const xstring &val);

  ROW_STORAGE()
  { set_size(0, 0); }

  ROW_STORAGE(size_t rnum, size_t cnum) : m_rnum(rnum), m_cnum(cnum)
  { set_size(rnum, cnum); }

  const char **data();

};

enum class EXCEPTION_TYPE {EMPTY_SET, CONN_ERR, GENERAL};

struct ODBCEXCEPTION
{
  EXCEPTION_TYPE m_type = EXCEPTION_TYPE::GENERAL;
  std::string m_msg;

  ODBCEXCEPTION(EXCEPTION_TYPE t = EXCEPTION_TYPE::GENERAL) : m_type(t)
  {}

  ODBCEXCEPTION(std::string msg, EXCEPTION_TYPE t = EXCEPTION_TYPE::GENERAL) :
    m_type(t), m_msg(msg)
  {}

};

struct ODBC_STMT
{
  DES_STMT *m_stmt = nullptr;

  ODBC_STMT(DES *des) {
    if (des)
    m_stmt = mysql_stmt_init(des);
  }
  operator DES_STMT*() { return m_stmt; }
  ~ODBC_STMT() {
    if (m_stmt)
    mysql_stmt_close(m_stmt);
  }
};


class charPtrBuf {
  private:

  std::vector<char*> m_buf;
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

  void set(const void* data, size_t size) {
    set_size(size);
    char *buf = (char*)data;
    std::vector<char*> tmpVec(size, buf);
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

#define NULL_STR "null"

inline const SQLULEN DES_DEFAULT_DATA_CHARACTER_SIZE = 255;  // This seems to be a standard, along with
                                                        // 8000. TODO: research

inline const std::vector<SQLSMALLINT> supported_types = {
    SQL_VARCHAR,   SQL_LONGVARCHAR,   SQL_CHAR, SQL_INTEGER,
    SQL_SMALLINT,  SQL_FLOAT,         SQL_REAL, SQL_TYPE_DATE,
    SQL_TYPE_TIME, SQL_TYPE_TIMESTAMP};

inline const std::unordered_map<std::string, SQLSMALLINT> typestr_simpletype_map = {
    {"varchar()", SQL_VARCHAR},
    {"string", SQL_LONGVARCHAR},
    {"varchar", SQL_LONGVARCHAR},
    {"char", SQL_CHAR},  // we will use it with size_str=1
    {"char()", SQL_CHAR},
    {"integer", SQL_INTEGER},
    {"int", SQL_SMALLINT},
    {"float", SQL_FLOAT},
    {"real", SQL_REAL},
    {"date", SQL_TYPE_DATE},
    {"time", SQL_TYPE_TIME},
    {"datetime", SQL_TYPE_TIMESTAMP},
    {"timestamp", SQL_TYPE_TIMESTAMP}};

struct Type {
  SQLSMALLINT simple_type;
  SQLULEN size = -1; //useful for character data types
};

inline std::string Type_to_type_str(Type type) {
  for (auto pair : typestr_simpletype_map) {
    std::string type_str = pair.first;
    SQLSMALLINT simple_type = pair.second;

    type_str.erase(std::remove(type_str.begin(), type_str.end(), '('),
                   type_str.end());
    type_str.erase(std::remove(type_str.begin(), type_str.end(), ')'),
                   type_str.end());

    if (type.simple_type == simple_type) {
      if (is_character_data_type(simple_type) &&
          !is_time_data_type(simple_type)) {
        if (type.size !=
            DES_DEFAULT_DATA_CHARACTER_SIZE) {  // TODO: study policies for
                                            // considering varchar() or varchar
                                            // depending on size.
          return type_str + "(" + std::to_string(type.size) +
                 ")";  // i'm not sure if I should return varchar() or
                       // varchar(46), for example, given that
                       // typestr_simpletype_map holds varchar(). TODO: check
        } else {
          return type_str;
        }
      } else
        return type_str;
    }
  }

  return "";
}

inline SQLULEN get_type_size(SQLSMALLINT type) {
  switch (type) {
    case SQL_SMALLINT:
      return 5;
    case SQL_INTEGER:
      return 10;
    case SQL_REAL:
      return 7;
    case SQL_FLOAT:
      return 15;
    case SQL_DOUBLE:
      return 15;
    case SQL_TYPE_DATE:
      return 10;
    case SQL_TYPE_TIME:
      return 8;
    case SQL_TYPE_TIMESTAMP:
      return 19;
    // TODO: check the following. I am not sure wgucglimits
    // I should pick.
    case SQL_LONGVARCHAR:  // theorically, it is infinite. Which value do I return?
    case SQL_CHAR:
    case SQL_VARCHAR:
      return DES_DEFAULT_DATA_CHARACTER_SIZE;  
    default:
      return 0;
  }
}

inline char* string_to_char_pointer(const std::string &str) {
  char* ptr = new char[str.size() + 1];
  std::strcpy(ptr, str.c_str());
  return ptr;
}

inline Type get_Type(SQLSMALLINT type) { return {type, get_type_size(type)}; }

//Internal representation of a column from a result view.
class Column {
 public:
  std::string name;
  std::string table;
  std::string db = "$des"; //TODO: we could have load a .ddb
  std::string catalog = "def"; //TODO: research
  Type type;
  SQLSMALLINT nullable;
  std::vector<std::string> values;

  Column() {}

  unsigned long getLength(int row) {
      return values[row].size(); //TODO: size() throws a size_t, research what to do to ensure compatibility
  }

  unsigned int getMaxLength() {
      unsigned int max = 0;

      for (auto value : values) {
        if (value.size() > max) max = value.size();
      }

      return max;
  }

  DES_ROWS* generate_DES_ROWS(int current_row) {
    DES_ROWS *rows = new DES_ROWS;
    DES_ROWS *ptr = rows;
    for (int i = 0; current_row + i < values.size(); ++i) {

      ptr->data = new char*;
      *(ptr->data) = string_to_char_pointer(values[current_row + i]);

      if (current_row + (i + 1) < values.size()) {
        ptr->next = new DES_ROWS;
        ptr = ptr->next;
      } else
        ptr->next = nullptr;

      //length doesn't seem to be modified, as I have verified debugging MySQL ODBC. TODO: research
    }
    return rows;
  }

  DES_FIELD* generate_DES_FIELD() {
    DES_FIELD *field = new DES_FIELD;

    field->name = string_to_char_pointer(this->name);
    field->org_name = string_to_char_pointer(this->name);
    field->name_length = this->name.size();
    field->org_name_length = this->name.size();

    field->table = string_to_char_pointer(this->table);
    field->org_table = string_to_char_pointer(this->table);
    field->table_length = this->table.size();
    field->org_table_length = this->table.size();

    field->db = string_to_char_pointer(this->db);
    field->db_length = this->db.size();

    field->catalog = string_to_char_pointer(this->catalog);
    field->catalog_length = this->catalog.size();
    
    field->def = nullptr; //TODO: research
    field->def_length = 0;

    field->flags = 0; //TODO: research

    field->decimals = 0;

    field->charsetnr = 255; //TODO: research

    field->type = DES_TYPE_VAR_STRING; //TODO: use a map

    field->extension = nullptr;

    field->length = DES_DEFAULT_DATA_CHARACTER_SIZE; //TODO: discriminate by type
    field->max_length = this->getMaxLength();

    return field;
    
  }

  Column(const std::string& table_name, const std::string &col_name, const Type &col_type, const SQLSMALLINT &col_nullable)
      : table(table_name), name(col_name),
        type(col_type),
        nullable(col_nullable) {}

  Type get_type() { return type; }
  SQLULEN get_size() {
    // Values fetched from:
    // https://learn.microsoft.com/en-us/sql/odbc/reference/appendixes/column-size?view=sql-server-ver16

      //TODO: do I need to define all of these size types,
      //or only the types that DES support (which I am currently
      //doing)? what about the size of these, which are dependent
      //on the underlying Prolog system of DES?
    return type.size;

  }
  void refresh_row(const int row_index) {
      /*
    string_to_char_pointer(values[row_index], (char *)target_value_binding);
    *str_len_or_ind_binding = values[row_index].size();
    */
  }
  void update_row(const int row_index, std::string value) {
    values[row_index] = value;
  }
  void remove_row(const int row_index) { values.erase(values.begin() + row_index);}
  SQLSMALLINT get_decimal_digits() {
      //It seems that there is no type in DES that has this attribute.
      //Consequently, we should always return zero.
      //Check https://learn.microsoft.com/en-us/sql/odbc/reference/appendixes/decimal-digits?view=sql-server-ver16
      return 0;
  }
  SQLSMALLINT get_nullable() { return nullable; }

  void insert_value(const std::string &value) { values.push_back(value); }
  std::string get_value(int index) const { return values[index-1]; }

  SQLRETURN copy_result_in_memory(size_t RowNumber, SQLSMALLINT TargetType,
                                  SQLPOINTER TargetValuePtr,
                                  SQLLEN BufferLength,
                                  SQLLEN *StrLen_or_IndPtr) {

      // TODO: Check if the given pointers are null or not
    std::string std_str = get_value(RowNumber);

    if (std_str == NULL_STR) {  // temporal debugging solution. null values must
                              // be taken into account before this
      //(right now, if a varchar is called literally "null", it is recognized as
      // null) TODO: fix
      *StrLen_or_IndPtr = SQL_NULL_DATA;
      return SQL_SUCCESS;
    }
    
    std::wstring unicode_str(std_str.begin(), std_str.end());
    int int_value;
    int x;
    size_t size;
    size_t unit_size;
    switch (TargetType) {
      case SQL_C_CHAR:
        unit_size = sizeof(char);
        return copy(TargetValuePtr, StrLen_or_IndPtr, BufferLength,
             BufferLength * unit_size, std_str,
             (std_str.size() + 1) * unit_size);
        break;
      case SQL_C_BINARY:
        x = 0;  // for debugging purposes
        break;
      case SQL_WVARCHAR:
      case SQL_C_WCHAR:
        unit_size = sizeof(SQLWCHAR);
        return copy(TargetValuePtr, StrLen_or_IndPtr, BufferLength,
             BufferLength * unit_size, unicode_str,
             (unicode_str.size() + 1) * unit_size);
        break;
      case SQL_C_BIT:
        x = 0;  // for debugging purposes
        break;
      case SQL_C_TINYINT:
        x = 0;  // for debugging purposes
        break;
      case SQL_C_STINYINT:
        x = 0;  // for debugging purposes
        break;
      case SQL_C_UTINYINT:
        x = 0;  // for debugging purposes
        break;
      case SQL_C_SHORT:
        x = 0;  // for debugging purposes
        break;
      case SQL_C_SSHORT:
        x = 0;  // for debugging purposes
        break;
      case SQL_C_USHORT:
        x = 0;  // for debugging purposes
        break;
      case SQL_C_LONG:
        x = 0;  // for debugging purposes
        break;
      case SQL_C_SLONG:
        int_value = std::stoi(std_str);
        *static_cast<int *>(TargetValuePtr) = int_value;
        *StrLen_or_IndPtr = sizeof(int);
        break;
      case SQL_C_ULONG:
        x = 0;  // for debugging purposes
        break;
      case SQL_C_FLOAT:
        x = 0;  // for debugging purposes
        break;
      case SQL_C_DOUBLE:
        x = 0;  // for debugging purposes
        break;
      case SQL_C_DATE:
        x = 0;  // for debugging purposes
        break;
      case SQL_C_TYPE_DATE:
        x = 0;  // for debugging purposes
        break;
      case SQL_C_INTERVAL_HOUR_TO_SECOND:
        x = 0;  // for debugging purposes
        break;
      case SQL_C_INTERVAL_HOUR_TO_MINUTE:
        x = 0;  // for debugging purposes
        break;
      case SQL_C_TIME:
        x = 0;  // for debugging purposes
        break;
      case SQL_C_TYPE_TIME:
        x = 0;  // for debugging purposes
        break;
      case SQL_C_TIMESTAMP:
        x = 0;  // for debugging purposes
        break;
      case SQL_C_TYPE_TIMESTAMP:
        x = 0;  // for debugging purposes
        break;
      case SQL_C_SBIGINT:
        x = 0;  // for debugging purposes
        break;
      case SQL_C_UBIGINT:
        x = 0;  // for debugging purposes
        break;
      case SQL_C_NUMERIC:
        x = 0;  // for debugging purposes
        break;
    }

    return SQL_SUCCESS;
  
  }

  void update_binding(SQLUSMALLINT row) {
    /*
    if (target_value_binding && str_len_or_ind_binding) {
        copy_result_in_memory(row, type.simple_type, target_value_binding, buffer_length_binding, str_len_or_ind_binding);

    }
    */
  }

private:
  template <typename GenericString>
 SQLRETURN copy(SQLPOINTER TargetValuePtr,
           SQLLEN *StrLen_or_IndPtr, SQLLEN BufferLength,
           SQLLEN BufferSize, GenericString str, size_t str_size) {
   SQLRETURN rc = SQL_SUCCESS;
   size_t size = min(BufferSize, str_size);
    if (str.size() >= BufferLength) {
      rc = SQL_SUCCESS_WITH_INFO; //there is a partial success: an informational message has to be generated.
                                  //TODO: implement the consequences
   }
    std::memcpy(TargetValuePtr, str.c_str(),
                size);
   *StrLen_or_IndPtr = str.size();

   return rc;
  }

};



//TODO: guarantee encapsulation.

enum COMMAND_TYPE {
    UNKNOWN,
    SELECT,
    PROCESS,
    SQLTABLES,
    SQLPRIMARYKEYS,
    SQLFOREIGNKEYS_PK,
    SQLFOREIGNKEYS_FK,
    SQLFOREIGNKEYS_PKFK,
    SQLGETTYPEINFO,
    SQLSTATISTICS,
    SQLSPECIALCOLUMNS
};

struct ForeignKeyInfo {
  std::string key;
  std::string foreign_table;
  std::string foreign_key;
};

struct DBSchemaTableInfo {
    //Unordered maps name -> column index
  std::unordered_map<std::string, int> columns_index_map;
  std::unordered_map<std::string, Type> columns_type_map;
  std::vector<std::string> primary_keys;
  std::vector<ForeignKeyInfo> foreign_keys;
  std::vector<std::string> not_nulls;

  std::string name;
};


struct STMT; //Forward declaration to let ResultTable have a STMT attribute
//(there is a cyclic dependence)
//Warning: the functions that use fields of STMT in ResultTable
//are forward-declared. Then, STMT is defined, and then,
//the functions are defined.

class ResultTable { //Internal representation of a result view.
 public:

  STMT *stmt = NULL; //the info saved by the stmt structure may contain useful info for constructing the table.

  //Vector of column names, ordered by insertion time.
  std::vector<std::string> names_ordered;
    
  //Some calls are given to the driver using only the
  //column name. We need therefore to search the column given its name.
  std::unordered_map<std::string, Column> columns;

  ResultTable() {} //empty table for some metadata functions

  unsigned long* fetch_lengths(int current_row) {

    unsigned long* lengths = (unsigned long *)malloc(names_ordered.size()*sizeof(unsigned long));

    for (int i = 0; i < names_ordered.size(); ++i) {
      unsigned long* length = lengths + i;
      *length = columns[names_ordered[i]].getLength(current_row);
    }

    return lengths;
  }
  
  DES_ROW generate_DES_ROW(const int index) {
    
    int n_cols = names_ordered.size();
    DES_ROW row = new char*[n_cols];

    for (int i = 0; i < n_cols; ++i) {
      row[i] = string_to_char_pointer(columns[names_ordered[i]].values[index]);
    }

    return row;
  }

  DES_ROWS* generate_DES_ROWS(const int current_row) {

    DES_ROWS *rows = new DES_ROWS;
    DES_ROWS *ptr = rows;
    int n_rows = columns[names_ordered[0]].values.size(); //sloppy way to do this. TODO: guarantee encapsulation
    for (int i = 0; current_row + i < n_rows; ++i) {
      ptr->data = generate_DES_ROW(current_row+i);

      if (current_row + (i + 1) < n_rows) {
        ptr->next = new DES_ROWS;
        ptr = ptr->next;
      } else
        ptr->next = nullptr;

      // length doesn't seem to be modified, as I have verified debugging MySQL
      // ODBC. TODO: research
    }
    return rows;
  }

  DES_FIELD* generate_DES_FIELD(int col_index) {
    return columns[names_ordered[col_index]].generate_DES_FIELD();
  }

  std::vector<ForeignKeyInfo> getForeignKeysFromTAPI(
      const std::vector<std::string> &lines, int &index) {
    std::vector<ForeignKeyInfo> result;

    while (lines[index] != "$") {
      std::string str = lines[index];

      // We erase these brackets for easer the parsing
      str.erase(std::remove(str.begin(), str.end(), '['), str.end());
      str.erase(std::remove(str.begin(), str.end(), ']'), str.end());
      ForeignKeyInfo fki;
      // We skip the "non foreign" table name
      int i = 0;
      while (str[i] != '.') i++;
      i++;  // we skip '.'
      std::string key = "";
      while (str[i] != ' ') {
        key += str[i];
        i++;
      }
      i += 4;  // we skip " -> "
      std::string foreign_table = "";
      while (str[i] != '.') {
        foreign_table += str[i];
        i++;
      }
      i++;  // we skip '.'
      std::string foreign_key = "";
      while (i < str.size()) {
        foreign_key += str[i];
        i++;
      }

      fki.key = key;
      fki.foreign_table = foreign_table;
      fki.foreign_key = foreign_key;

      result.push_back(fki);

      index++;
    }

    return result;
  }

  DBSchemaTableInfo getTableInfo(const std::vector<std::string> &lines,
                                 int& index) {
      //We asume that the first given line by index is "$table".
      //Leaves the index to the position when all we are
      //interested in has been already fetched.

      DBSchemaTableInfo table_info;

      index++;

      std::string table_name = lines[index];
      table_info.name = table_name;

      index++;
      int col_index = 1;
      while (index < lines.size() && lines[index] != "$") {
        std::string column_name = lines[index];
        Type type = get_Type_from_str(lines[index + 1]);
        
        table_info.columns_index_map.insert({column_name, col_index});
        table_info.columns_type_map.insert({column_name, type});

        col_index++;
        index += 2;  // in each iteration we are fetching name and type of each
                 // column
      }
      index++;
      if (lines[index] != "$")
        table_info.not_nulls = convertArrayNotationToStringVector(lines[index]);
      index += 2;
      if (lines[index] != "$")
        table_info.primary_keys =
            convertArrayNotationToStringVector(lines[index]);
      index += 3;
      if (lines[index] != "$")
        table_info.foreign_keys = getForeignKeysFromTAPI(lines, index);

      return table_info;

  }

  std::unordered_map<std::string, DBSchemaTableInfo> getAllTablesInfo(const std::string &str) {
      //Table name -> DBSchemaTableInfo structure
    std::unordered_map<std::string, DBSchemaTableInfo> main_map;

    std::vector<std::string> lines = getLines(str);
    //Table name -> its TAPI output
    std::unordered_map<std::string, std::string> table_str_map;

    int i = 0;
    DBSchemaTableInfo table_info;
    while (true) {

      while (i < lines.size() && lines[i] != "$table") {
        i++;
      }
      if (i == lines.size()) break;

      table_info = getTableInfo(lines, i);      

      main_map.insert({table_info.name, table_info});

    }

    return main_map;

  }

  void insert_metadata_cols() {
    /*
      Default table (representation of metadata table).
      We learnt from this studying calls to the MySQL ODBC.
      We replicated the returned columns of the default table
      and each of its characteristics.
  */
    insert_col("", "TABLE_CAT", get_Type(SQL_VARCHAR), SQL_NULLABLE);
    insert_col("", "TABLE_SCHEM", get_Type(SQL_VARCHAR), SQL_NULLABLE);
    insert_col("", "TABLE_NAME", get_Type(SQL_VARCHAR), SQL_NULLABLE);
    insert_col("", "TABLE_TYPE", get_Type(SQL_VARCHAR), SQL_NULLABLE);
    insert_col("", "REMARKS", get_Type(SQL_VARCHAR), SQL_NULLABLE);
  }

  void insert_SQLPrimaryKeys_cols() {
    insert_col("", "TABLE_CAT", get_Type(SQL_VARCHAR), SQL_NULLABLE);
    insert_col("", "TABLE_SCHEM", get_Type(SQL_VARCHAR), SQL_NULLABLE);
    insert_col("", "TABLE_NAME", get_Type(SQL_VARCHAR), SQL_NO_NULLS);
    insert_col("", "COLUMN_NAME", get_Type(SQL_VARCHAR), SQL_NO_NULLS);
    insert_col("", "KEY_SEQ", get_Type(SQL_SMALLINT), SQL_NO_NULLS);
    insert_col("", "PK_NAME", get_Type(SQL_VARCHAR), SQL_NULLABLE);
  };

  void insert_SQLForeignKeys_cols() {
    insert_col("", "PKTABLE_CAT", get_Type(SQL_VARCHAR), SQL_NULLABLE);
    insert_col("", "PKTABLE_SCHEM", get_Type(SQL_VARCHAR), SQL_NULLABLE);
    insert_col("", "PKTABLE_NAME", get_Type(SQL_VARCHAR), SQL_NO_NULLS);
    insert_col("", "PKCOLUMN_NAME", get_Type(SQL_VARCHAR), SQL_NO_NULLS);
    insert_col("", "FKTABLE_CAT", get_Type(SQL_VARCHAR), SQL_NULLABLE);
    insert_col("", "FKTABLE_SCHEM", get_Type(SQL_VARCHAR), SQL_NULLABLE);
    insert_col("", "FKTABLE_NAME", get_Type(SQL_VARCHAR), SQL_NO_NULLS);
    insert_col("", "FKCOLUMN_NAME", get_Type(SQL_VARCHAR), SQL_NO_NULLS);
    insert_col("", "KEY_SEQ", get_Type(SQL_SMALLINT), SQL_NO_NULLS);
    insert_col("", "UPDATE_RULE", get_Type(SQL_SMALLINT), SQL_NULLABLE);
    insert_col("", "DELETE_RULE", get_Type(SQL_SMALLINT), SQL_NULLABLE);
    insert_col("", "FK_NAME", get_Type(SQL_VARCHAR), SQL_NULLABLE);
    insert_col("", "PK_NAME", get_Type(SQL_VARCHAR), SQL_NULLABLE);
    insert_col("", "DEFERRABILITY", get_Type(SQL_SMALLINT), SQL_NULLABLE);
  };

  void insert_SQLGetTypeInfo_cols() {
    insert_col("", "TYPE_NAME", get_Type(SQL_VARCHAR), SQL_NO_NULLS);
    insert_col("", "DATA_TYPE", get_Type(SQL_SMALLINT), SQL_NO_NULLS);
    insert_col("", "COLUMN_SIZE", get_Type(SQL_INTEGER), SQL_NULLABLE);
    insert_col("", "LITERAL_PREFIX", get_Type(SQL_VARCHAR), SQL_NULLABLE);
    insert_col("", "LITERAL_SUFFIX", get_Type(SQL_VARCHAR), SQL_NULLABLE);
    insert_col("", "CREATE_PARAMS", get_Type(SQL_VARCHAR), SQL_NULLABLE);
    insert_col("", "NULLABLE", get_Type(SQL_SMALLINT), SQL_NO_NULLS);
    insert_col("", "CASE_SENSITIVE", get_Type(SQL_SMALLINT), SQL_NO_NULLS);
    insert_col("", "SEARCHABLE", get_Type(SQL_SMALLINT), SQL_NO_NULLS);
    insert_col("", "UNSIGNED_ATTRIBUTE", get_Type(SQL_SMALLINT), SQL_NULLABLE);
    insert_col("", "FIXED_PREC_SCALE", get_Type(SQL_SMALLINT), SQL_NO_NULLS);
    insert_col("", "AUTO_UNIQUE_VALUE", get_Type(SQL_SMALLINT), SQL_NULLABLE);
    insert_col("", "LOCAL_TYPE_NAME", get_Type(SQL_VARCHAR), SQL_NULLABLE);
    insert_col("", "MINIMUM_SCALE", get_Type(SQL_SMALLINT), SQL_NULLABLE);
    insert_col("", "MAXIMUM_SCALE", get_Type(SQL_SMALLINT), SQL_NULLABLE);
    insert_col("", "SQL_DATA_TYPE", get_Type(SQL_SMALLINT), SQL_NO_NULLS);
    insert_col("", "SQL_DATETIME_SUB", get_Type(SQL_SMALLINT), SQL_NULLABLE);
    insert_col("", "NUM_PREC_RADIX", get_Type(SQL_INTEGER), SQL_NULLABLE);
    insert_col("", "INTERVAL_PRECISION", get_Type(SQL_SMALLINT), SQL_NULLABLE);
  }

  void insert_SQLStatistics_cols() {
    insert_col("", "TABLE_CAT", get_Type(SQL_VARCHAR), SQL_NULLABLE);
    insert_col("", "TABLE_SCHEM", get_Type(SQL_VARCHAR), SQL_NULLABLE);
    insert_col("", "TABLE_NAME", get_Type(SQL_VARCHAR), SQL_NO_NULLS);
    insert_col("", "NON_UNIQUE", get_Type(SQL_SMALLINT), SQL_NULLABLE);
    insert_col("", "INDEX_QUALIFIER", get_Type(SQL_VARCHAR), SQL_NULLABLE);
    insert_col("", "INDEX_NAME", get_Type(SQL_VARCHAR), SQL_NULLABLE);
    insert_col("", "TYPE", get_Type(SQL_SMALLINT), SQL_NO_NULLS);
    insert_col("", "ORDINAL_POSITION", get_Type(SQL_SMALLINT), SQL_NULLABLE);
    insert_col("", "COLUMN_NAME", get_Type(SQL_VARCHAR), SQL_NULLABLE);
    insert_col("", "ASC_OR_DESC", {SQL_CHAR, 1}, SQL_NULLABLE);  // CHAR(1)
    insert_col("", "CARDINALITY", get_Type(SQL_INTEGER), SQL_NULLABLE);
    insert_col("", "PAGES", get_Type(SQL_INTEGER), SQL_NULLABLE);
    insert_col("", "FILTER_CONDITION", get_Type(SQL_VARCHAR), SQL_NULLABLE);
  }

  void insert_SQLSpecialColumns_cols(){
    insert_col("", "SCOPE", get_Type(SQL_SMALLINT), SQL_NULLABLE);
    insert_col("", "COLUMN_NAME", get_Type(SQL_VARCHAR), SQL_NO_NULLS);
    insert_col("", "DATA_TYPE", get_Type(SQL_SMALLINT), SQL_NO_NULLS);
    insert_col("", "TYPE_NAME", get_Type(SQL_VARCHAR), SQL_NO_NULLS);
    insert_col("", "COLUMN_SIZE", get_Type(SQL_INTEGER), SQL_NULLABLE);
    insert_col("", "BUFFER_LENGTH", get_Type(SQL_INTEGER), SQL_NULLABLE);
    insert_col("", "DECIMAL_DIGITS", get_Type(SQL_SMALLINT), SQL_NULLABLE);
    insert_col("", "PSEUDO_COLUMN", get_Type(SQL_SMALLINT), SQL_NULLABLE);

  }

  ResultTable(STMT *stmt);

  ResultTable(STMT *stmt, const std::string &str);

  std::vector<std::string> getLines(const std::string &str) {
    std::vector<std::string> lines;
    std::stringstream ss(str);
    std::string line;

    while (std::getline(ss, line, '\n')) {
      lines.push_back(line.substr(
          0, line.size() - 1));  // to remove the char '/r' of each line
    }
    return lines;
  }

  std::vector<std::string> convertArrayNotationToStringVector(std::string str) {

    std::vector<std::string> str_vector;

    str.erase(std::remove(str.begin(), str.end(), '['), str.end());
    str.erase(std::remove(str.begin(), str.end(), ']'), str.end());

    std::stringstream ss(str);
    std::string element_str;

    while (std::getline(ss, element_str, ',')) {
      str_vector.push_back(element_str.substr(0, element_str.size()));
    }

    return str_vector;

  }

  void create_results_SQLGetTypeInfo();

  void parse_SQLForeignKeys_PK(const std::string &str);

  void parse_SQLForeignKeys_FK(const std::string &str);

  void parse_SQLForeignKeys_PKFK(const std::string &str);

  void create_results_SQLStatistics(const std::string &str);

  void create_results_SQLSpecialColumns(const std::string &str);

  void parse_SQLPrimaryKeys(const std::string &str) {
      //TODO: handle errors
    insert_SQLPrimaryKeys_cols();

    // First, we separate the TAPI str into lines.
    std::vector<std::string> lines = getLines(str);

    int i = 0;
    DBSchemaTableInfo table_info = getTableInfo(lines, i);

    if (table_info.primary_keys.size() == 0) return;

    for (int i = 0; i < table_info.primary_keys.size(); ++i) {
      std::string TABLE_CAT = "";
      std::string TABLE_SCHEM = "";
      std::string TABLE_NAME = "";
      std::string COLUMN_NAME = table_info.primary_keys[i];
      int KEY_SEQ = table_info.columns_index_map[table_info.primary_keys[i]];
      std::string PK_NAME = "";

      insert_value("TABLE_CAT", TABLE_CAT);
      insert_value("TABLE_SCHEM", TABLE_SCHEM);
      insert_value("TABLE_NAME", TABLE_NAME);
      insert_value("COLUMN_NAME", COLUMN_NAME);
      insert_value("KEY_SEQ", std::to_string(KEY_SEQ)); //check if this is correct
      insert_value("PK_NAME", PK_NAME);
    }

  }

  void parse_SQLTables(const std::string &str) {

    insert_metadata_cols();

    std::vector<std::vector<std::string>> results;

    // First, we separate the TAPI str into lines.
    std::vector<std::string> lines = getLines(str);

    int i = 0;

    while (i < lines.size()) {
      if (lines[i] == "$eot") break;
      std::string TABLE_CAT = "";  // TODO: check
      std::string TABLE_SCHEM = "";  // TODO: check
      std::string TABLE_NAME = "";
      std::string TABLE_TYPE = "";
      std::string REMARKS = "";  // TODO: consider adding additional info.

      if (lines[i] == "$table") {
        TABLE_TYPE = "TABLE";
      } else if (lines[i] == "$view") {
        TABLE_TYPE = "VIEW";
      }

      i++;

      TABLE_NAME = lines[i];

      insert_value("TABLE_CAT", TABLE_CAT);
      insert_value("TABLE_SCHEM", TABLE_SCHEM);
      insert_value("TABLE_NAME", TABLE_NAME);
      insert_value("TABLE_TYPE", TABLE_TYPE);
      insert_value("REMARKS", REMARKS);

      while (lines[i].size() <= 1 || lines[i][0] != '$') i++;

    }
  }

  Type get_Type_from_str(const std::string& str){
    std::string type_str = str;
    SQLULEN size = -1;

    size_t first_parenthesis_pos = type_str.find('(', 0);
    size_t pos = first_parenthesis_pos;
    if (pos != std::string::npos) {
      std::string size_str = "";

      pos++;
      while (pos < type_str.size() && type_str[pos] != ')') {
        size_str += type_str[pos];
        pos++;
      }

      size = stoi(size_str);

      type_str = type_str.substr(0, first_parenthesis_pos + 1) + ')';
    } else
      size = get_type_size(typestr_simpletype_map.at(type_str));

    SQLSMALLINT simple_type = typestr_simpletype_map.at(type_str);

    return {simple_type, size};
  
  };

  //Constructor of table from the parsing a TAPI-codified DES result.
  //Returns the number of rows of the result (valuable for SQLStatistics)
  int parse_select(const std::string& str, bool fetch_values) {

    int n_rows = 0;

    // First, we separate the TAPI str into lines.
    std::vector<std::string> lines = getLines(str);

    std::vector<std::string> column_names;

    //We ignore the first line, that contains the keywords success/answer.
    int i = 1;

    // If the first message is not answer, we can be sure that the output
    // originates from a non-select query. We then create the default
    // metadata table on the else clause.
    if (lines.size() >= 1 && lines[0] == "answer") {
      bool checked_cols = false;

      while (!checked_cols && lines[i] != "$eot") {
        //For each column, TAPI gives in a line its name and then its type.
        size_t pos_dot = lines[i].find('.', 0);
        std::string table = lines[i].substr(0, pos_dot);
        std::string name =
            lines[i].substr(pos_dot + 1, lines[i].size() - (pos_dot + 1));
        Type type = get_Type_from_str(lines[i + 1]);

        // I have put SQL_NULLABLE_UNKNOWN because I do not know
        // if a result table from select may have an attribute "nullable"
        if (fetch_values) {
          insert_col(table, name, type, SQL_NULLABLE_UNKNOWN);
        }

        column_names.push_back(name);

        i += 2;

        if (lines[i] == "$")
            checked_cols = true;
      }
      if (lines[i] == "$eot") return n_rows;

      i++;  //We skip the final $
      std::string aux = lines[i];
      while (aux != "$eot") {
        for (int j = 0; j < column_names.size(); ++j) {
          std::string name_col = column_names[j];
          std::string value = lines[i];

          //When we reach a varchar value, we remove the " ' " characters provided by the TAPI.
          if (value.size() > 0 && value[0] == '\'') {
            value = value.substr(1, value.size() - 2);
          }
          if (fetch_values)
            insert_value(name_col, value);


          i++;
        }
        aux = lines[i];
        i++; //to skip $ or $eot
      }
      n_rows++;
    }
    else { //if it is not possible, we create the default metadata table.
      insert_metadata_cols();
    }

    return n_rows;
  }

  Type col_type(const std::string &name) { return columns[name].get_type(); }
  SQLULEN col_size(const std::string &name) { return columns[name].get_size(); }
  SQLSMALLINT col_decimal_digits(const std::string &name) { return columns[name].get_decimal_digits(); }
  SQLSMALLINT col_nullable(const std::string &name) { return columns[name].get_nullable(); }

  size_t col_count() { return names_ordered.size(); }

  size_t row_count() {
    if (names_ordered.size() != 0)
      return columns[names_ordered[0]].values.size();
    else
      return 0;
  }

  std::string index_to_name_col(size_t index) {
    return names_ordered[index - 1];
  }

  void update_bound_cols(SQLUSMALLINT row) {
    for (auto pair : columns) {
        Column col = pair.second;
        col.update_binding(row);
    
    }
  
  }

  void refresh_row(const int row_index) {
    for (auto pair : columns) {
        Column col = pair.second;
      col.refresh_row(row_index);
    }
  
  }

  void add_row();
  void update_row(const int row_index);
  void remove_row(const int row_index);

    void insert_col(const std::string& tableName, const std::string &columnName, const Type &columnType,
                  const SQLSMALLINT &columnNullable) {
    names_ordered.push_back(columnName);
    columns[columnName] =
        Column(tableName, columnName, columnType, columnNullable);
  }

  void insert_value(const std::string &columnName, const std::string &value) {
    columns[columnName].insert_value(value);
  }

  SQLRETURN copy_result_in_memory(size_t RowNumber, size_t ColumnNumber,
                                  SQLSMALLINT TargetType,
                                  SQLPOINTER TargetValuePtr,
                                  SQLLEN BufferLength,
                                  SQLLEN *StrLen_or_IndPtr) {
    
    ColumnNumber--;
    return columns[names_ordered[ColumnNumber]].copy_result_in_memory(RowNumber, TargetType, TargetValuePtr, BufferLength, StrLen_or_IndPtr);
    
  }
};

struct STMT
{
  DBC               *dbc;
  des_bool           fake_result;
  charPtrBuf        array;
  charPtrBuf        result_array;
  DES_ROW         current_values;
  DES_ROW         (*fix_fields)(STMT *stmt, DES_ROW row);
  DES_FIELD	      *fields;
  DES_ROW_OFFSET  end_of_set;
  tempBuf           tempbuf;
  ROW_STORAGE       m_row_storage;

  //Adding the DES structures over the MySQL STMT. Temporal changes
  DES_RESULT *result = new DES_RESULT(this);  // TODO review

  std::vector<SQLCHAR*> bookmarks;

  //Adding the query (SQLPrepare). Temporal changes
  SQLCHAR* des_query = NULL;

  std::string last_output = "";

  // Some of these values may be empty. Useful for SQLForeignKeys calls
  std::string pk_table_name = "";
  std::string fk_table_name = "";

  // Useful for SQLGetTypeInfo calls
  SQLSMALLINT type_requested = SQL_ALL_TYPES;

  //Type of this command
  COMMAND_TYPE type = UNKNOWN;  // unknown by default

  bool new_row_des = true;
  int current_row_des = 1;

  DESCURSOR          cursor;
  DESERROR           error;
  STMT_OPTIONS      stmt_options;
  std::string       table_name; // This value may be empty. Useful for SQLStatistics calls
  std::string       catalog_name;

  DES_PARSED_QUERY	query, orig_query;
  std::vector<DES_BIND> param_bind;
  std::vector<const char*> query_attr_names;

  std::unique_ptr<des_bool[]> rb_is_null;
  std::unique_ptr<des_bool[]> rb_err;
  std::unique_ptr<unsigned long[]> rb_len;
  std::unique_ptr<unsigned long[]> lengths;

  des_ulonglong      affected_rows;
  long              current_row;
  long              cursor_row;
  char              dae_type; /* data-at-exec type */

  GETDATA           getdata;

  uint		param_count, current_param, rows_found_in_set;

  enum DES_STATE state;
  enum DES_DUMMY_STATE dummy_state;

  /* APD for data-at-exec on SQLSetPos() */
  std::unique_ptr<DESC> setpos_apd;
  DESC *setpos_apd2;
  SQLSETPOSIROW setpos_row;
  SQLUSMALLINT setpos_lock;
  SQLUSMALLINT setpos_op;

  DES_STMT *ssps;
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
  telemetry::Telemetry<STMT> telemetry;

  telemetry::Telemetry<DBC>& conn_telemetry()
  {
    assert(dbc);
    return dbc->telemetry;
  }

  int ssps_bind_result();

  char* extend_buffer(char *to, size_t len);
  char* extend_buffer(size_t len);

  char* add_to_buffer(const char *from, size_t len);
  char* buf() { return tempbuf.buf; }
  char* endbuf() { return tempbuf.buf + tempbuf.cur_pos; }
  size_t buf_pos() { return tempbuf.cur_pos; }
  size_t buf_len() { return tempbuf.buf_len; }
  size_t field_count();
  DES_ROW fetch_row(bool read_unbuffered = false);
  void buf_set_pos(size_t pos) { tempbuf.cur_pos = pos; }
  void buf_add_pos(size_t pos) { tempbuf.cur_pos += pos; }
  void buf_remove_trail_zeroes() { tempbuf.remove_trail_zeroes(); }
  void alloc_lengths(size_t num);
  void free_lengths();
  void reset_getdata_position();
  void reset_setpos_apd();
  void allocate_param_bind(uint elements);
  long compute_cur_row(unsigned fFetchType, SQLLEN irow);

  SQLRETURN bind_query_attrs(bool use_ssps);
  void reset();

  void free_unbind();
  void free_reset_out_params();
  void free_reset_params();
  void free_fake_result(bool clear_all_results);

  bool is_dynamic_cursor();

  SQLRETURN set_error(desodbc_errid errid, const char *errtext, SQLINTEGER errcode);
  SQLRETURN set_error(const char *state, const char *errtext, SQLINTEGER errcode);

  /*
    Error message and errno is taken from dbc->mysql
  */
  SQLRETURN set_error(desodbc_errid errid);

  void add_query_attr(const char *name, std::string val);
  bool query_attr_exists(const char *name);
  void clear_attr_names() { query_attr_names.clear(); }

  /*
    Error message and errno is taken from dbc->mysql
  */
  SQLRETURN set_error(const char *state);

  STMT(DBC *d) : dbc(d), result(NULL), fake_result(false), array(), result_array(),
    current_values(NULL), fields(NULL), end_of_set(NULL),
    tempbuf(),
    stmt_options(dbc->stmt_options), lengths(nullptr), affected_rows(0),
    current_row(0), cursor_row(0), dae_type(0),
    param_count(0), current_param(0),
    rows_found_in_set(0),
    state(ST_UNKNOWN), dummy_state(ST_DUMMY_UNKNOWN),
    setpos_row(0), setpos_lock(0), setpos_op(0),
    ssps(NULL), result_bind(NULL), out_params_state(OPS_UNKNOWN),

    m_ard(this, SQL_DESC_ALLOC_AUTO, DESC_APP, DESC_ROW),
    ard(&m_ard),
    m_ird(this, SQL_DESC_ALLOC_AUTO, DESC_IMP, DESC_ROW),
    ird(&m_ird),
    m_apd(this, SQL_DESC_ALLOC_AUTO, DESC_APP, DESC_PARAM),
    apd(&m_apd),
    m_ipd(this, SQL_DESC_ALLOC_AUTO, DESC_IMP, DESC_PARAM),
    ipd(&m_ipd),
    imp_ard(ard), imp_apd(apd)
  {
    allocate_param_bind(10);

    LOCK_DBC(dbc);
    dbc->stmt_list.emplace_back(this);
  }

  ~STMT();

  void clear_param_bind();
  void reset_result_array();

  void reset_row_indexes() {
    new_row_des = true;
    current_row_des = 1; //what if there were no rows? TODO
  }

  private:

  /*
    Create a phony, non-functional STMT handle used as a placeholder.

    Warning: The hanlde should not be used other than for storing attributes added using `add_query_attr()`.
  */

  STMT(DBC *d, size_t param_cnt)
    : dbc{d}
    , query_attr_names{param_cnt}
    , ssps(nullptr)
    , m_ard(this, SQL_DESC_ALLOC_AUTO, DESC_APP, DESC_ROW)
    , m_ird(this, SQL_DESC_ALLOC_AUTO, DESC_IMP, DESC_ROW)
    , m_apd(this, SQL_DESC_ALLOC_AUTO, DESC_APP, DESC_PARAM)
    , m_ipd(this, SQL_DESC_ALLOC_AUTO, DESC_IMP, DESC_PARAM)
  {}

  friend DBC;
};

inline static unsigned long *des_fetch_lengths(STMT *stmt) {
  return stmt->result->internal_table->fetch_lengths(stmt->current_row);
}

inline static DES_RESULT *des_store_result(DES *des) { return nullptr; }

inline static DES_RESULT *des_store_result(STMT *stmt) {
  DES_RESULT *res = new DES_RESULT(stmt, stmt->last_output);
  res->field_count = res->internal_table->col_count();
  if (res->field_count == 0) {
    delete res;  // TODO: consider using the appropiate free function
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
    DES_FIELD* generated_DES_FIELD = res->internal_table->generate_DES_FIELD(i);
    memcpy(field, generated_DES_FIELD,
           sizeof(DES_FIELD));
    delete generated_DES_FIELD;
   
  }

  res->fields = fields;

  DES_DATA *data = new DES_DATA;
  data->rows = res->row_count;
  data->fields = res->field_count;

  data->data = res->internal_table->generate_DES_ROWS(0);
  
  res->data = data;
  res->data_cursor = res->data->data;

  return res;
}
//Defining the following ResultTable functions in a header file with the
//inline keyword prevents us from having linking errors

SQLRETURN do_quiet_internal_query(std::string query);

inline void ResultTable::add_row() {

  std::vector<std::string> new_values;
  std::vector<bool> is_character_data;
  for (int i = 0; i < names_ordered.size(); ++i) {
    Column col = columns[names_ordered[i]];
    is_character_data.push_back(
        is_character_data_type(col.get_type().simple_type));
    //new_values.push_back(std::string((char *)col.target_value_binding, *col.str_len_or_ind_binding));
  }

  std::string query = "insert into " + this->stmt->table_name + " values(";
  for (int i = 0; i < new_values.size(); ++i) {
    if (is_character_data[i])
      query += '\'' + new_values[i] + '\'';
    else
      query += new_values[i];
    if (i != new_values.size() - 1) {
      query += ",";
    }
  }

  query += ')';

  if (do_quiet_internal_query(query) == SQL_SUCCESS) {
    for (int i = 0; i < names_ordered.size(); ++i) {
      insert_value(names_ordered[i], new_values[i]);
    }
  } else
    exit(1);  // TODO: handle error


}

inline void ResultTable::update_row(int row_index) {
  /*

  std::vector<std::string> names;
  std::vector<std::string> values;
  std::vector<std::string> new_values;
  std::vector<bool> is_character_data;
  for (int i = 0; i < names_ordered.size(); ++i) {
    names.push_back(names_ordered[i]);
    Column col = columns[names_ordered[i]];
    values.push_back(col.get_value(row_index));
    is_character_data.push_back(
        is_character_data_type(col.get_type().simple_type));

    SQLPOINTER *address = new SQLPOINTER;
    SQLINTEGER len = 30;
    stmt_SQLGetDescField(stmt, stmt->ard, i+1, SQL_DESC_DATA_PTR, address,
                               SQL_IS_POINTER, &len); //TODO: handle errors
    new_values.push_back(std::string(static_cast<char *>(*address)));
  }

  std::string query = "UPDATE " + this->stmt->table_name + " SET ";
  for (int i = 0; i < names.size(); ++i) {
    query += names[i] + "=";
    if (is_character_data[i])
      query += '\'' + new_values[i] + '\'';
    else
      query += values[i];
    if (i != names.size() - 1) {
      query += ',';
    }
  }
  query += " WHERE "; //PROBLEM: multiple rows can be deleted if there are duplicates. TODO
  for (int i = 0; i < names.size(); ++i) {
    query += names[i] + "=";
    if (is_character_data[i])
      query += '\'' + values[i] + '\'';
    else
      query += values[i];
    if (i != names.size() - 1) {
      query += " AND ";
    }
  }

  if (do_quiet_internal_query(query) == SQL_SUCCESS) {
    for (int i = 0; i < names_ordered.size(); ++i) {
      names.push_back(names_ordered[i]);
      Column col = columns[names_ordered[i]];
      col.update_row(row_index, new_values[i]);
    }
  } else
    exit(1);  // TODO: handle error
    */
}

inline void ResultTable::remove_row(int row_index) {
  std::vector<std::string> names;
  std::vector<std::string> values;
  std::vector<bool> is_character_data;
  for (int i = 0; i < names_ordered.size(); ++i) {
    names.push_back(names_ordered[i]);
    Column col = columns[names_ordered[i]];
    values.push_back(col.get_value(row_index));
    is_character_data.push_back(
        is_character_data_type(col.get_type().simple_type));
  }

  std::string query = "DELETE FROM " + this->stmt->table_name + " WHERE ";
  for (int i = 0; i < names.size(); ++i) {
    query += names[i] + "=";
    if (is_character_data[i])
      query += '\'' + values[i] + '\'';
    else
      query += values[i];
    if (i != names.size() - 1) {
      query += " AND ";
    }
  }

  if (do_quiet_internal_query(query) == SQL_SUCCESS) {
    for (int i = 0; i < names_ordered.size(); ++i) {
      names.push_back(names_ordered[i]);
      Column col = columns[names_ordered[i]];
      col.remove_row(row_index);
    }
  }
  else
      exit(1); //TODO: handle error
}

inline ResultTable::ResultTable(STMT *stmt, const std::string &str) {
  this->stmt = stmt;
  switch (this->stmt->type) {
    case SELECT:
      parse_select(str, true);
      break;
    case SQLTABLES:
      parse_SQLTables(str);
      break;
    case PROCESS:
      insert_metadata_cols();
      break;
    case SQLPRIMARYKEYS:
      parse_SQLPrimaryKeys(str);
      break;
    case SQLFOREIGNKEYS_FK:
      parse_SQLForeignKeys_FK(str);
      break;
    case SQLFOREIGNKEYS_PK:
      parse_SQLForeignKeys_PK(str);
      break;
    case SQLFOREIGNKEYS_PKFK:
      parse_SQLForeignKeys_PKFK(str);
      break;
    case SQLGETTYPEINFO:
      create_results_SQLGetTypeInfo();
      break;
    case SQLSTATISTICS:
      create_results_SQLStatistics(str);
      break;
    case SQLSPECIALCOLUMNS:
      create_results_SQLSpecialColumns(str);
      break;
  }
}

inline ResultTable::ResultTable(STMT *stmt) {
  this->stmt = stmt;
  insert_metadata_cols();
}


inline void ResultTable::create_results_SQLGetTypeInfo() {
  insert_SQLGetTypeInfo_cols();
  SQLSMALLINT type_requested = this->stmt->type_requested;

  for (int i = 0; i < supported_types.size(); i++) {
    if (type_requested == supported_types[i] || type_requested == SQL_ALL_TYPES) {

      for (auto pair : typestr_simpletype_map) {
        std::string des_type_name = pair.first;
        SQLSMALLINT sql_data_type = pair.second;
        if (sql_data_type == type_requested ||
            type_requested == SQL_ALL_TYPES) {

          bool type_is_character_data = is_character_data_type(sql_data_type);
          bool type_is_time_data = is_time_data_type(sql_data_type);

          insert_value("TYPE_NAME", des_type_name);
          insert_value("DATA_TYPE", std::to_string(sql_data_type));
          if (des_type_name == "char") //special case (char is not considered in SQL standards)
            insert_value("COLUMN_SIZE", std::to_string(1));
          else
            insert_value("COLUMN_SIZE",
                         std::to_string(get_type_size(sql_data_type)));

          if (type_is_character_data) {
            insert_value("LITERAL_PREFIX", "\'");
            insert_value("LITERAL_SUFFIX", "\'");
          } else {
            insert_value("LITERAL_PREFIX", NULL_STR);
            insert_value("LITERAL_SUFFIX", NULL_STR);
          }
          insert_value("CREATE_PARAMS",
              NULL_STR);  // TODO: I do not really understand this field
          insert_value(
              "NULLABLE",
              std::to_string(SQL_NULLABLE_UNKNOWN));  // TODO: I currently do not know which
                                                      // DES types are nullable
          if (type_is_character_data && !type_is_time_data) {
            insert_value("CASE_SENSITIVE", std::to_string(SQL_TRUE));
          } else {
            insert_value("CASE_SENSITIVE", std::to_string(SQL_FALSE));
          }

          //TODO: check if the following is correct according
          //to DES policies
          if (type_is_character_data && !type_is_time_data) {
            insert_value("SEARCHABLE", std::to_string(SQL_SEARCHABLE));
          } else {
            insert_value("SEARCHABLE", std::to_string(SQL_PRED_BASIC));
          }

          if (type_is_character_data) {
            insert_value("UNSIGNED_ATTRIBUTE", NULL_STR);
          } else
            insert_value("UNSIGNED_ATTRIBUTE", std::to_string(SQL_FALSE));

          if (type_requested == SQL_LONGVARCHAR)
            insert_value("FIXED_PREC_SCALE", std::to_string(SQL_FALSE));
          else
            insert_value("FIXED_PREC_SCALE", std::to_string(SQL_TRUE));

          //It seems that DES does not have auto-incrementing types.
          //TODO: confirm it
          if (type_is_character_data)
            insert_value("AUTO_UNIQUE_VALUE",
                         NULL_STR);  // a time data type can be considered as
                                   // numeric? TODO: research
          else
            insert_value("AUTO_UNIQUE_VALUE", std::to_string(FALSE));

          insert_value("LOCAL_TYPE_NAME",
                       NULL_STR);  // TODO: research this concept
          insert_value("MINIMUM_SCALE",
                       NULL_STR);  // TODO: The concept of scale does not seem
                                   // to
                                 // be appliable to DES
          insert_value("MAXIMUM_SCALE",
                       NULL_STR);  // or, is it the minimum and maximum value?
                                 // (i.e. in integers)
          insert_value("SQL_DATA_TYPE", std::to_string(sql_data_type));

          //It seems odd that this interpretation of the SQL_DATETIME_SUB column is correct.
          //TODO: research
          if (type_is_time_data) {
            switch (sql_data_type) {
                case SQL_TYPE_DATE:
                    insert_value("SQL_DATETIME_SUB", std::to_string(SQL_DATE));
                    break;
                case SQL_TYPE_TIME:
                  insert_value("SQL_DATETIME_SUB", std::to_string(SQL_TIME));
                  break;
                case SQL_TYPE_TIMESTAMP:
                  insert_value("SQL_DATETIME_SUB", std::to_string(SQL_TIMESTAMP));
                  break;
                default:
                  insert_value("SQL_DATETIME_SUB", NULL_STR);
                  break;
            }
          } else
            insert_value("SQL_DATETIME_SUB", NULL_STR);

          if (type_is_character_data)
            insert_value("NUM_PREC_RADIX", NULL_STR);
          else //is numeric
            insert_value("NUM_PREC_RADIX", "10");

          insert_value("INTERVAL_PRECISION", NULL_STR); 
        
        }
      }
      if (type_requested == SQL_ALL_TYPES) {  // then, we have already consulted
                                              // every type. We must finish now.
        break;
      }
    }
  }

}

inline void ResultTable::create_results_SQLSpecialColumns(
    const std::string& str) {

    insert_SQLSpecialColumns_cols();
    std::vector<std::string> lines = getLines(str);
    int index = 0;
    DBSchemaTableInfo table_info = getTableInfo(lines, index);

    for (int i = 0; i < table_info.primary_keys.size(); ++i) {
      std::string primary_key = table_info.primary_keys[i];
      Type type = table_info.columns_type_map.at(primary_key);
      insert_value("SCOPE", std::to_string(SQL_SCOPE_SESSION));
      insert_value("COLUMN_NAME", primary_key);
      insert_value("DATA_TYPE", std::to_string(type.simple_type));
      insert_value("TYPE_NAME", Type_to_type_str(type));
      insert_value("COLUMN_SIZE", std::to_string(type.size));
      insert_value("BUFFER_LENGTH", NULL_STR); //TODO: implement
      insert_value("DECIMAL_DIGITS", NULL_STR);
      insert_value("PSEUDO_COLUMN", std::to_string(SQL_PC_NOT_PSEUDO));
    }

}

inline void ResultTable::create_results_SQLStatistics(const std::string &str)
{
  insert_SQLStatistics_cols();

  int n_rows = parse_select(str, false);

  insert_value("TABLE_CAT", NULL_STR);
  insert_value("TABLE_SCHEM", NULL_STR);
  insert_value("TABLE_NAME", this->stmt->table_name);
  insert_value("NON_UNIQUE", NULL_STR);
  insert_value("INDEX_QUALIFIER", NULL_STR);
  insert_value("INDEX_NAME", NULL_STR);
  insert_value("TYPE", std::to_string(SQL_TABLE_STAT));
  insert_value("ORDINAL_POSITION", NULL_STR);
  insert_value("COLUMN_NAME", NULL_STR);
  insert_value("ASC_OR_DESC", NULL_STR);
  insert_value("CARDINALITY", std::to_string(n_rows));
  insert_value("PAGES", NULL_STR);
  insert_value("FILTER_CONDITION", NULL_STR);
}

inline void ResultTable::parse_SQLForeignKeys_PK(const std::string &str) {
  insert_SQLForeignKeys_cols();

  std::string pk_table_name = this->stmt->pk_table_name;

  std::unordered_map<std::string, DBSchemaTableInfo> tables_info =
      getAllTablesInfo(str);

  for (auto pair_name_table_info : tables_info) {
    std::string fk_table_name = pair_name_table_info.first;
    DBSchemaTableInfo fk_table_info = pair_name_table_info.second;

    std::vector<std::string> keysReferringToPkTable;
    for (int i = 0; i < fk_table_info.foreign_keys.size(); ++i) {
      if (fk_table_info.foreign_keys[i].foreign_table == pk_table_name) {
        insert_value("PKTABLE_CAT", "");
        insert_value("PKTABLE_SCHEM", "");
        insert_value("PKTABLE_NAME", pk_table_name);
        insert_value("PKCOLUMN_NAME",
                     fk_table_info.foreign_keys[i].foreign_key);
        insert_value("FKTABLE_CAT", "");
        insert_value("FKTABLE_SCHEM", "");
        insert_value("FKTABLE_NAME", fk_table_name);
        insert_value("FKCOLUMN_NAME", fk_table_info.foreign_keys[i].key);
        insert_value(
            "KEY_SEQ",
            std::to_string(fk_table_info.columns_index_map
                               [fk_table_info.foreign_keys[i]
                                    .key]));  // I assume that KEY_SEQ refers
                                              // to the foreign key in every
                                              // case. TODO: check
        // We will leave these unspecified until further development.
        insert_value("UPDATE_RULE", "");
        insert_value("DELETE_RULE", "");
        insert_value("FK_NAME", "");
        insert_value("PK_NAME", "");
        insert_value("DEFERRABILITY", "");
      }
    }
  }
}

inline void ResultTable::parse_SQLForeignKeys_FK(const std::string &str) {
  insert_SQLForeignKeys_cols();

  std::string table_name = this->stmt->fk_table_name;

  std::unordered_map<std::string, DBSchemaTableInfo> tables_info =
      getAllTablesInfo(str);

  DBSchemaTableInfo table_info = tables_info[table_name];
  std::vector<ForeignKeyInfo> foreign_keys = table_info.foreign_keys;

  for (int i = 0; i < foreign_keys.size(); ++i) {
    insert_value("PKTABLE_CAT", "");
    insert_value("PKTABLE_SCHEM", "");
    insert_value("PKTABLE_NAME", foreign_keys[i].foreign_table);
    insert_value("PKCOLUMN_NAME", foreign_keys[i].foreign_key);
    insert_value("FKTABLE_CAT", "");
    insert_value("FKTABLE_SCHEM", "");
    insert_value("FKTABLE_NAME", this->stmt->fk_table_name);
    insert_value("FKCOLUMN_NAME", foreign_keys[i].key);
    insert_value("KEY_SEQ",
                 std::to_string(
                     table_info.columns_index_map
                         [foreign_keys[i].key]));  // I assume that KEY_SEQ
                                                   // refers to the foreign key
                                                   // in every case. TODO: check
    // We will leave these unspecified until further development.
    insert_value("UPDATE_RULE", "");
    insert_value("DELETE_RULE", "");
    insert_value("FK_NAME", "");
    insert_value("PK_NAME", "");
    insert_value("DEFERRABILITY", "");
  }
}

inline void ResultTable::parse_SQLForeignKeys_PKFK(const std::string &str) {
  insert_SQLForeignKeys_cols();

  std::string pk_table_name = this->stmt->pk_table_name;
  std::string fk_table_name = this->stmt->fk_table_name;

  std::unordered_map<std::string, DBSchemaTableInfo> tables_info =
      getAllTablesInfo(str);

  for (auto pair_name_table_info : tables_info) {
    std::string local_fk_table_name = pair_name_table_info.first;
    DBSchemaTableInfo local_fk_table_info = pair_name_table_info.second;

    std::vector<std::string> keysReferringToPkTable;
    if (local_fk_table_name ==
        fk_table_name) { /* this if condition is the only
                            difference with the
                            parse_sqlforeign_pk
                            function. TODO: consider refactoring
                            */
      for (int i = 0; i < local_fk_table_info.foreign_keys.size(); ++i) {
        if (local_fk_table_info.foreign_keys[i].foreign_table ==
            pk_table_name) {
          insert_value("PKTABLE_CAT", "");
          insert_value("PKTABLE_SCHEM", "");
          insert_value("PKTABLE_NAME", pk_table_name);
          insert_value("PKCOLUMN_NAME",
                       local_fk_table_info.foreign_keys[i].foreign_key);
          insert_value("FKTABLE_CAT", "");
          insert_value("FKTABLE_SCHEM", "");
          insert_value("FKTABLE_NAME", local_fk_table_name);
          insert_value("FKCOLUMN_NAME",
                       local_fk_table_info.foreign_keys[i].key);
          insert_value(
              "KEY_SEQ",
              std::to_string(local_fk_table_info.columns_index_map
                                 [local_fk_table_info.foreign_keys[i]
                                      .key]));  // I assume that KEY_SEQ
                                                // refers to the foreign key
                                                // in every case. TODO: check
          // We will leave these unspecified until further development.
          insert_value("UPDATE_RULE", "");
          insert_value("DELETE_RULE", "");
          insert_value("FK_NAME", "");
          insert_value("PK_NAME", "");
          insert_value("DEFERRABILITY", "");
        }
      }
    }
  }
}

inline DES_RESULT::DES_RESULT() { this->internal_table = new ResultTable(); }

inline DES_RESULT::DES_RESULT(STMT *stmt) { this->internal_table = new ResultTable(stmt); }

inline DES_RESULT::DES_RESULT(STMT *stmt, const std::string &tapi_output) {
  this->internal_table = new ResultTable(stmt, tapi_output);
}


namespace desodbc {
  struct HENV
  {
    SQLHENV henv = nullptr;

    HENV(unsigned long v)
    {
      size_t ver = v;
      SQLAllocHandle(SQL_HANDLE_ENV, nullptr, &henv);

      if (SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)ver, 0) != SQL_SUCCESS)
      {
        throw DESERROR(SQL_HANDLE_ENV, henv, SQL_ERROR);
      }
    }

    HENV() : HENV(SQL_OV_ODBC3)
    {}

    operator SQLHENV() const
    {
      return henv;
    }

    ~HENV()
    {
      SQLFreeHandle(SQL_HANDLE_ENV, henv);
    }
  };

  struct HDBC
  {
    SQLHDBC hdbc = nullptr;
    SQLHENV henv;
    xstring connout = nullptr;
    SQLCHAR ch_out[512] = { 0 };

    HDBC(SQLHENV env, DataSource *params) : henv(env)
    {
      SQLWSTRING  string_connect_in;

      assert((bool)params->opt_DRIVER);
      params->opt_DSN.set_default(nullptr);
      string_connect_in = params->to_kvpair(';');
      if (SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc) != SQL_SUCCESS)
      {
        throw DESERROR(SQL_HANDLE_ENV, henv, SQL_ERROR);
      }

      if (SQLDriverConnectW(hdbc, NULL, (SQLWCHAR*)string_connect_in.c_str(),
        SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT) != SQL_SUCCESS)
      {
        throw DESERROR(SQL_HANDLE_DBC, hdbc, SQL_ERROR);
      }

    }

    operator SQLHDBC() const
    {
      return hdbc;
    }

    ~HDBC()
    {
      SQLDisconnect(hdbc);
      SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
    }
  };


  struct HSTMT
  {
    SQLHDBC hdbc;
    SQLHSTMT hstmt = nullptr;
    HSTMT(SQLHDBC dbc) : hdbc(dbc)
    {
      if (SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt) != SQL_SUCCESS)
      {
        throw DESERROR(SQL_HANDLE_STMT, hstmt, SQL_ERROR);
      }
    }

    operator SQLHSTMT() const
    {
      return hstmt;
    }

    ~HSTMT()
    {
      SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    }
  };
};


extern std::string thousands_sep, decimal_point, default_locale;
#ifndef _UNIX_
extern HINSTANCE NEAR s_hModule;  /* DLL handle. */
#endif

#ifdef WIN32
extern std::string current_dll_location;
extern std::string default_plugin_location;
#endif

/*
  Resource defines for "SQLDriverConnect" dialog box
*/
#define ID_LISTBOX  100
#define CONFIGDSN 1001
#define CONFIGDEFAULT 1002
#define EDRIVERCONNECT	1003

/* New data type definitions for compatibility with MySQL 5 */
#ifndef DES_TYPE_NEWDECIMAL
# define DES_TYPE_NEWDECIMAL 246
#endif
#ifndef DES_TYPE_BIT
# define DES_TYPE_BIT 16
#endif

DES_LIMIT_CLAUSE find_position4limit(desodbc::CHARSET_INFO* cs, const char *query, const char * query_end);

#include "myutil.h"
#include "stringutil.h"

//TODO: it is obvious that we commited renaming errors.
SQLRETURN SQL_API DESColAttribute(SQLHSTMT hstmt, SQLUSMALLINT column,
                                    SQLUSMALLINT attrib, SQLCHAR **char_attr,
                                    SQLLEN *num_attr);
SQLRETURN SQL_API DESColumnPrivileges(SQLHSTMT hstmt,
                                        SQLCHAR *catalog,
                                        SQLSMALLINT catalog_len,
                                        SQLCHAR *schema, SQLSMALLINT schema_len,
                                        SQLCHAR *table, SQLSMALLINT table_len,
                                        SQLCHAR *column,
                                        SQLSMALLINT column_len);
SQLRETURN SQL_API DESColumns(SQLHSTMT hstmt,
                               SQLCHAR *catalog, SQLSMALLINT catalog_len,
                               SQLCHAR *schema, SQLSMALLINT schema_len,
                               SQLCHAR *sztable, SQLSMALLINT table_len,
                               SQLCHAR *column, SQLSMALLINT column_len);
SQLRETURN SQL_API DESConnect(SQLHDBC hdbc, SQLWCHAR *szDSN, SQLSMALLINT cbDSN,
                               SQLWCHAR *szUID, SQLSMALLINT cbUID,
                               SQLWCHAR *szAuth, SQLSMALLINT cbAuth);
SQLRETURN SQL_API DESDescribeCol(SQLHSTMT hstmt, SQLUSMALLINT column,
                                   SQLCHAR **name, SQLSMALLINT *need_free,
                                   SQLSMALLINT *type, SQLULEN *def,
                                   SQLSMALLINT *scale, SQLSMALLINT *nullable);
SQLRETURN SQL_API DESDriverConnect(SQLHDBC hdbc, SQLHWND hwnd,
                                     SQLWCHAR *in, SQLSMALLINT in_len,
                                     SQLWCHAR *out, SQLSMALLINT out_max,
                                     SQLSMALLINT *out_len,
                                     SQLUSMALLINT completion);
SQLRETURN SQL_API DESForeignKeys(SQLHSTMT hstmt,
                                   SQLCHAR *pkcatalog,
                                   SQLSMALLINT pkcatalog_len,
                                   SQLCHAR *pkschema, SQLSMALLINT pkschema_len,
                                   SQLCHAR *pktable, SQLSMALLINT pktable_len,
                                   SQLCHAR *fkcatalog,
                                   SQLSMALLINT fkcatalog_len,
                                   SQLCHAR *fkschema, SQLSMALLINT fkschema_len,
                                   SQLCHAR *fktable, SQLSMALLINT fktable_len);
SQLCHAR *DESGetCursorName(HSTMT hstmt);
SQLRETURN SQL_API DESGetInfo(SQLHDBC hdbc, SQLUSMALLINT fInfoType,
                               SQLCHAR **char_info, SQLPOINTER num_info,
                               SQLSMALLINT *value_len);
SQLRETURN SQL_API DESGetConnectAttr(SQLHDBC hdbc, SQLINTEGER attrib,
                                      SQLCHAR **char_attr, SQLPOINTER num_attr);
SQLRETURN DESGetDescField(SQLHDESC hdesc, SQLSMALLINT recnum,
                            SQLSMALLINT fldid, SQLPOINTER valptr,
                            SQLINTEGER buflen, SQLINTEGER *strlen);
SQLRETURN SQL_API DESGetDiagField(SQLSMALLINT handle_type, SQLHANDLE handle,
                                    SQLSMALLINT record, SQLSMALLINT identifier,
                                    SQLCHAR **char_value, SQLPOINTER num_value);
SQLRETURN SQL_API DESGetDiagRec(SQLSMALLINT handle_type, SQLHANDLE handle,
                                  SQLSMALLINT record, SQLCHAR **sqlstate,
                                  SQLINTEGER *native, SQLCHAR **message);
SQLRETURN SQL_API DESGetStmtAttr(SQLHSTMT hstmt, SQLINTEGER Attribute,
                                   SQLPOINTER ValuePtr,
                                   SQLINTEGER BufferLength
                                     __attribute__((unused)),
                                  SQLINTEGER *StringLengthPtr);
SQLRETURN SQL_API DESGetTypeInfo(SQLHSTMT hstmt, SQLSMALLINT fSqlType);
SQLRETURN SQL_API DESPrepare(SQLHSTMT hstmt, SQLCHAR *query, SQLINTEGER len,
                               bool reset_select_limit,
                               bool force_prepare);
SQLRETURN SQL_API DESPrimaryKeys(SQLHSTMT hstmt,
                                   SQLCHAR *catalog, SQLSMALLINT catalog_len,
                                   SQLCHAR *schema, SQLSMALLINT schema_len,
                                   SQLCHAR *table, SQLSMALLINT table_len);
SQLRETURN SQL_API DESProcedureColumns(SQLHSTMT hstmt,
                                        SQLCHAR *catalog,
                                        SQLSMALLINT catalog_len,
                                        SQLCHAR *schema,
                                        SQLSMALLINT schema_len,
                                        SQLCHAR *proc,
                                        SQLSMALLINT proc_len,
                                        SQLCHAR *column,
                                        SQLSMALLINT column_len);
SQLRETURN SQL_API DESProcedures(SQLHSTMT hstmt,
                                  SQLCHAR *catalog, SQLSMALLINT catalog_len,
                                  SQLCHAR *schema, SQLSMALLINT schema_len,
                                  SQLCHAR *proc, SQLSMALLINT proc_len);
SQLRETURN SQL_API DESSetConnectAttr(SQLHDBC hdbc, SQLINTEGER Attribute,
                                      SQLPOINTER ValuePtr,
                                      SQLINTEGER StringLengthPtr);
SQLRETURN SQL_API DESSetCursorName(SQLHSTMT hstmt, SQLCHAR *name,
                                     SQLSMALLINT len);
SQLRETURN SQL_API DESSetStmtAttr(SQLHSTMT hstmt, SQLINTEGER attribute,
                                   SQLPOINTER value, SQLINTEGER len);
SQLRETURN SQL_API DESSpecialColumns(SQLHSTMT hstmt, SQLUSMALLINT type,
                                      SQLCHAR *catalog, SQLSMALLINT catalog_len,
                                      SQLCHAR *schema, SQLSMALLINT schema_len,
                                      SQLCHAR *table, SQLSMALLINT table_len,
                                      SQLUSMALLINT scope,
                                      SQLUSMALLINT nullable);
SQLRETURN SQL_API DESStatistics(SQLHSTMT hstmt,
                                  SQLCHAR *catalog, SQLSMALLINT catalog_len,
                                  SQLCHAR *schema, SQLSMALLINT schema_len,
                                  SQLCHAR *table, SQLSMALLINT table_len,
                                  SQLUSMALLINT unique, SQLUSMALLINT accuracy);
SQLRETURN SQL_API DESTablePrivileges(SQLHSTMT hstmt,
                                       SQLCHAR *catalog,
                                       SQLSMALLINT catalog_len,
                                       SQLCHAR *schema, SQLSMALLINT schema_len,
                                       SQLCHAR *table, SQLSMALLINT table_len);
SQLRETURN SQL_API DESTables(SQLHSTMT hstmt,
                              SQLCHAR *catalog, SQLSMALLINT catalog_len,
                              SQLCHAR *schema, SQLSMALLINT schema_len,
                              SQLCHAR *table, SQLSMALLINT table_len,
                              SQLCHAR *type, SQLSMALLINT type_len);

#endif /* __DRIVER_H__ */
