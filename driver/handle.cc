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
  @file  handle.c
  @brief Allocation and freeing of handles.
*/

/***************************************************************************
 * The following ODBC APIs are implemented in this file:		   *
 *									   *
 *   SQLAllocHandle	 (ISO 92)					   *
 *   SQLFreeHandle	 (ISO 92)					   *
 *   SQLFreeStmt	 (ISO 92)					   *
 *									   *
****************************************************************************/

#include "driver.h"
#include <mutex>
#include <mysql_sys/my_thr_init.cc>

thread_local long thread_count = 0;

std::mutex g_lock;

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
void ENV::add_dbc(DBC* dbc)
{
  LOCK_ENV(this);
  conn_list.emplace_back(dbc);
  active_dbcs_global_var.emplace_back(dbc);
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
void ENV::remove_dbc(DBC* dbc)
{
  LOCK_ENV(this);
  conn_list.remove(dbc);
  active_dbcs_global_var.remove(dbc);
}

bool ENV::has_connections()
{
  return conn_list.size() > 0;
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
DBC::DBC(ENV *p_env)  : env(p_env),
                        txn_isolation(DEFAULT_TXN_ISOLATION),
                        last_query_time((time_t) time((time_t*) 0))
{
  //Fast way to put into a std::string the address itself of a given pointer
  std::stringstream ss;
  ss << static_cast<void *>(this);
  std::string dbc_mem_address = ss.str();

  #ifdef _WIN32
  this->connection_id = this->str_hasher(std::to_string(GetCurrentProcessId()) + dbc_mem_address);
  #else
  this->connection_id = this->str_hasher(std::to_string(getpid()) + dbc_mem_address);
  #endif

  desodbc_ov_init(env->odbc_ver);
  env->add_dbc(this);
}

void DBC::add_desc(DESC* desc)
{
  desc_list.emplace_back(desc);
}

void DBC::remove_desc(DESC* desc)
{
  desc_list.remove(desc);
}


void DBC::free_explicit_descriptors()
{

  /* free any remaining explicitly allocated descriptors */
  for (auto it = desc_list.begin(); it != desc_list.end();)
  {
    DESC *desc = *it;
    it = desc_list.erase(it);
    delete desc;
  }
}

/* DESODBC:
  Original author: MyODBC
  Modified by: DESODBC Developer
*/
SQLRETURN DBC::close() {
  SQLRETURN ret;
  if (this->connected) {
#ifdef _WIN32
    ret = get_shared_memory_mutex();
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
      return ret;
    }

    this->shmem->handle_sharing_info.handle_petitioner.id = this->connection_id;

    ret = setFinishingEvent();
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
      release_shared_memory_mutex();
      return ret;
    }
    if (share_pipes_thread != nullptr && share_pipes_thread.get()->joinable())
      share_pipes_thread->join();
    share_pipes_thread.reset();

    this->shmem->handle_sharing_info.handle_petitioner.id =
        0;  // we reset this field
    ret = release_shared_memory_mutex();
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return ret;

    ret = get_shared_memory_mutex();
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
      return ret;
    }
    remove_client_from_shmem(this->shmem->connected_clients_struct,
                             this->connection_id);
    if (this->shmem->connected_clients_struct.size == 0) {
      DWORD pid = this->shmem->DES_pid;

      HANDLE des_process_handle = OpenProcess(PROCESS_TERMINATE, false, pid);
      if (des_process_handle == NULL) {
        ret = this->set_win_error(
            "Failed to access DES process with PID " + std::to_string(pid),
            true);
      }

      auto pair = this->send_query_and_read("/q");
      if (pair.first != SQL_SUCCESS && pair.first != SQL_SUCCESS_WITH_INFO) {
        ret = this->set_win_error(
            "Failed to terminate DES process with PID " + std::to_string(pid),
            true);
      }

      // Just in case the shared memory is cached when creating it again
      shmem->DES_pid = 0;
      shmem->handle_sharing_info = HandleSharingInfo();
      shmem->connected_clients_struct = ConnectedClients();
      shmem->des_process_created = false;
      shmem->exec_hash_int = 0;

      // All these functions closes the correspondent objects for only this
      // instance. Windows by itself destroys these objects when no instance has
      // access to them.
      if (!UnmapViewOfFile(shmem)) {
        ret = this->set_win_error("Failed to unmap shared memory file " +
                                      std::string(SHARED_MEMORY_NAME),
                                  true);
      }
    }

    ret = release_shared_memory_mutex();

    try_close(query_mutex);
    try_close(shared_memory_mutex);
    try_close(request_handle_mutex);
    try_close(request_handle_event);
    try_close(handle_sent_event);
    try_close(finishing_event);
#else

    ret = get_shared_memory_mutex();
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return ret;

    this->shmem->n_clients -= 1;
    if (this->shmem->n_clients == 0) {
      ret = release_shared_memory_mutex();
      if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return ret;

      auto pair = this->send_query_and_read("/q");
      if (pair.first != SQL_SUCCESS && pair.first != SQL_SUCCESS_WITH_INFO) {
        std::string msg = "Failed to terminate DES process with PID ";
        msg += std::to_string(this->shmem->DES_pid);
        ret = this->set_unix_error(msg, true);
      }

#ifdef __APPLE__
      if (sem_close(query_mutex) == -1) {
        ret = this->set_unix_error("Failed to close query mutex", true);
      }
      if (sem_close(shared_memory_mutex) == -1) {
        ret = this->set_unix_error("Failed to close shared memory mutex", true);
      }
      sem_unlink(QUERY_MUTEX_NAME);
      sem_unlink(SHARED_MEMORY_MUTEX_NAME);
#endif

      if (shmdt(this->shmem) == -1) {
        return this->set_unix_error(
            "Failed to detach shared memory from connector", true);
      }
      if (shmctl(this->shm_id, IPC_RMID, 0) == -1) {
        return this->set_unix_error("Failed to remove shared memory segment",
                                    true);
      }

      try_close(this->driver_to_des_out_rpipe);
      // try_close(this->driver_to_des_out_wpipe); Not needed
      // try_close(this->driver_to_des_in_rpipe); Not needed
      try_close(this->driver_to_des_in_wpipe);

      unlink(IN_WPIPE_NAME);
      unlink(OUT_RPIPE_NAME);

    } else {
      ret = release_shared_memory_mutex();

#ifdef __APPLE__
      if (sem_close(query_mutex) == -1) {
        ret = this->set_unix_error("Failed to close query mutex", true);
      }
      if (sem_close(shared_memory_mutex) == -1) {
        ret = this->set_unix_error("Failed to close shared memory mutex", true);
      }
#endif

      if (shmdt(this->shmem) == -1) {
        ret = this->set_unix_error(
            "Failed to detach shared memory from connector", true);
      }

      try_close(this->driver_to_des_out_rpipe);
      // try_close(this->driver_to_des_out_wpipe); Not needed
      // try_close(this->driver_to_des_in_rpipe); Not needed
      try_close(this->driver_to_des_in_wpipe);
    }

#endif
  }
  this->connected = false;

  return SQL_SUCCESS;
}


/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
DBC::~DBC()
{
  if (this->connected)
      DBC::close();
  if (env)
    env->remove_dbc(this);

  free_explicit_descriptors();
}

#ifdef _WIN32
/* DESODBC:
    This function sets a Windows error.

    Original author: DESODBC Developer
*/
SQLRETURN DBC::set_win_error(std::string err, bool show_win_err) {
  if (show_win_err) {
    err += ". Last Windows error message: ";
    err += '\"';
    err += GetLastWinErrMessage();
    err += '\"';
  }
  return this->set_error("HY000", string_to_char_pointer(err));

}
#else
/* DESODBC:
    This function sets a Unix error.

    Original author: DESODBC Developer
*/
SQLRETURN DBC::set_unix_error(std::string err, bool show_unix_err) {
  if (show_unix_err) {
    err += ". Last Unix-like error message: ";
    err += '\"';
    err += strerror(errno);
    err += '\"';
  }
  return this->set_error("HY000", string_to_char_pointer(err));

}
#endif


/* DESODBC:
    Original author: DESODBC Developer
*/
SQLRETURN ENV::set_error(const char *state, const char *msg) {
  error = DESERROR(state, msg);
  return error.retcode;
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN DBC::set_error(const char * state, const char * msg)
{
  error = DESERROR(state, msg);
  return error.retcode;
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLRETURN STMT::set_error(const char *state, const char *msg) {
  error = DESERROR(state, msg);
  return error.retcode;
}


/* DESODBC:
    Renamed from the original my_SQLAllocEnv()
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : myodbc3 internal
  @purpose : to allocate the environment handle and to maintain
       its list
*/
SQLRETURN SQL_API DES_SQLAllocEnv(SQLHENV *phenv)
{
  ENV *env;

  std::lock_guard<std::mutex> env_guard(g_lock);
  desodbc_init();

#ifndef USE_IODBC
    env = new ENV(SQL_OV_ODBC3_80);
#else
    env = new ENV(SQL_OV_ODBC3);
#endif
  *phenv = (SQLHENV)env;

  return SQL_SUCCESS;
}

/* DESODBC:
    Renamed from the original my_SQLFreeEnv()
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : myodbc3 internal
  @purpose : to free the environment handle
*/

SQLRETURN SQL_API DES_SQLFreeEnv(SQLHENV henv)
{
    ENV *env= (ENV *) henv;
    delete env;
#ifndef _UNIX_
#else
    desodbc_end();
#endif /* _UNIX_ */
    return(SQL_SUCCESS);
}

/* DESODBC:

    Renamed from the original my_SQLAllocConnect and
    modified according to DES' needs.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : myodbc3 internal
  @purpose : to allocate the connection handle and to
       maintain the connection list
*/
SQLRETURN SQL_API DES_SQLAllocConnect(SQLHENV henv, SQLHDBC *phdbc)
{
    DBC *dbc;
    ENV *penv= (ENV *) henv;

    if (!thread_count)
      desodbc::my_thread_init();

    ++thread_count;

    if (!penv->odbc_ver)
    {
      return penv->set_error("HY010",
                             "Can't allocate connection "
                             "until ODBC version specified.");
    }

    try
    {
      dbc = new DBC(penv);
      *phdbc = (SQLHDBC)dbc;
    }
    catch(...)
    {
      *phdbc = nullptr;
      return SQL_ERROR;
    }

    return(SQL_SUCCESS);
}

/* DESODBC:

    Renamed from the original my_SQLFreeConnect and
    modified according to DES' needs.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : myodbc3 internal
  @purpose : to allocate the connection handle and to
       maintain the connection list
*/
SQLRETURN SQL_API DES_SQLFreeConnect(SQLHDBC hdbc)
{
    DBC *dbc= (DBC *) hdbc;

    #ifdef _WIN32
    delete dbc->SHARED_MEMORY_NAME;
    delete dbc->SHARED_MEMORY_MUTEX_NAME;
    delete dbc->QUERY_MUTEX_NAME;
    delete dbc->REQUEST_HANDLE_EVENT_NAME;
    delete dbc->REQUEST_HANDLE_MUTEX_NAME;
    delete dbc->HANDLE_SENT_EVENT_NAME;
    delete dbc->FINISHING_EVENT_NAME;
    #else
    delete dbc->SHARED_MEMORY_NAME;
    delete dbc->SHARED_MEMORY_MUTEX_NAME;
    delete dbc->QUERY_MUTEX_NAME;
    delete dbc->IN_WPIPE_NAME;
    delete dbc->OUT_RPIPE_NAME;
    #endif
    delete dbc;

    if (thread_count)
    {
      /*
      if (--thread_count ==0)
        mysql_thread_end();
      */
    }

    return SQL_SUCCESS;
}


/* Allocates memory for parameter binds in vector.
 */
void STMT::allocate_param_bind(uint elements)
{
  if (param_bind.size() < elements)
  {
    query_attr_names.resize(elements);
    param_bind.reserve(elements);
    while(elements > param_bind.size())
      param_bind.emplace_back(DES_BIND{});
  }
}


int adjust_param_bind_array(STMT *stmt)
{
  stmt->allocate_param_bind(stmt->param_count);
  /* Need to init newly allocated area with 0s */
  //  memset(stmt->param_bind->buffer + sizeof(DES_BIND)*prev_max_elements, 0,
  //    sizeof(DES_BIND) * (stmt->param_bind->max_element - prev_max_elements));

  return 0;
}

/* DESODBC:
    Renamed from the original my_SQLAllocStmt
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : myodbc3 internal
  @purpose : allocates the statement handle
*/
SQLRETURN SQL_API DES_SQLAllocStmt(SQLHDBC hdbc, SQLHSTMT *phstmt)
{
  std::unique_ptr<STMT> stmt;
  DBC   *dbc= (DBC*) hdbc;

  try
  {
    stmt.reset(new STMT(dbc));
  }
  catch (...)
  {
    return dbc->set_error("HY001", "Memory allocation error");
  }

  *phstmt = (SQLHSTMT*)stmt.release();
  return SQL_SUCCESS;
}


/*
  @type    : ODBC 1.0 API
  @purpose : stops processing associated with a specific statement,
       closes any open cursors associated with the statement,
       discards pending results, or, optionally, frees all
       resources associated with the statement handle
*/

SQLRETURN SQL_API SQLFreeStmt(SQLHSTMT hstmt, SQLUSMALLINT fOption)
{
  CHECK_HANDLE(hstmt);
  return DES_SQLFreeStmt(hstmt, fOption);
}


/* DESODBC:
    Renamed from the original my_SQLFreeStmt()
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : myodbc3 internal
  @purpose : stops processing associated with a specific statement,
       closes any open cursors associated with the statement,
       discards pending results, or, optionally, frees all
       resources associated with the statement handle
*/
SQLRETURN SQL_API DES_SQLFreeStmt(SQLHSTMT hstmt, SQLUSMALLINT f_option)
{
  return DES_SQLFreeStmtExtended(hstmt, f_option,
    FREE_STMT_CLEAR_RESULT | FREE_STMT_DO_LOCK);
}

/* DESODBC:

    Renamed from the original my_SQLFreeStmtExtended
    and modified according to DESODBC's needs.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : myodbc3 internal
  @purpose : stops processing associated with a specific statement,
       closes any open cursors associated with the statement,
       discards pending results, or, optionally, frees all
       resources associated with the statement handle
*/
SQLRETURN SQL_API DES_SQLFreeStmtExtended(SQLHSTMT hstmt, SQLUSMALLINT f_option,
  SQLUSMALLINT f_extra)
{
    STMT *stmt= (STMT *) hstmt;
    uint i;
    LOCK_STMT_DEFER(stmt);
    if (f_extra & FREE_STMT_DO_LOCK)
    {
      DO_LOCK_STMT();
    }

    stmt->reset();

    if (f_option == SQL_UNBIND)
    {
      stmt->free_unbind();
      return SQL_SUCCESS;
    }

    stmt->free_reset_out_params();

    if (f_option == SQL_RESET_PARAMS)
    {
      stmt->free_reset_params();
      return SQL_SUCCESS;
    }

    stmt->free_fake_result((bool)(f_extra & FREE_STMT_CLEAR_RESULT));

    x_free(stmt->fields);   // TODO: Looks like STMT::fields is not used anywhere
    free_result(stmt->result);
    stmt->result= 0;
    stmt->fake_result= 0;
    stmt->fields= 0;
    stmt->free_lengths();
    stmt->current_values= 0;   /* For SQLGetData */
    stmt->fix_fields= 0;
    stmt->affected_rows= 0;
    stmt->current_row= stmt->rows_found_in_set= 0;
    stmt->cursor_row= -1;
    stmt->dae_type= 0;
    stmt->ird->reset();

    if (f_option == FREE_STMT_RESET_BUFFERS)
    {
      free_result_bind(stmt);
      stmt->array.reset();

      return SQL_SUCCESS;
    }

    stmt->state= ST_UNKNOWN;

    stmt->params_for_table.pk_table_name.clear();
    stmt->params_for_table.fk_table_name.clear();
    stmt->params_for_table.type_requested = SQL_TYPE_NULL;
    stmt->params_for_table.table_name.clear();
    stmt->params_for_table.column_name.clear();
    stmt->params_for_table.catalog_name.clear();
    stmt->params_for_table.table_type.clear();
    stmt->params_for_table.metadata_id = false;
    

    stmt->dummy_state= ST_DUMMY_UNKNOWN;
    stmt->cursor.pk_validated= FALSE;
    stmt->reset_setpos_apd();

    for (i= stmt->cursor.pk_count; i--;)
    {
      stmt->cursor.pkcol[i].bind_done= 0;
    }
    stmt->cursor.pk_count= 0;

    if (f_option == SQL_CLOSE)
        return SQL_SUCCESS;

    if (f_extra & FREE_STMT_CLEAR_RESULT)
    {
      stmt->array.reset();
    }

    /* At this point, only FREE_STMT_RESET and SQL_DROP left out */
    stmt->orig_query.reset(NULL, NULL, NULL);
    stmt->query.reset(NULL, NULL, NULL);

    stmt->param_count= 0;

    reset_ptr(stmt->apd->rows_processed_ptr);
    reset_ptr(stmt->ard->rows_processed_ptr);
    reset_ptr(stmt->ipd->array_status_ptr);
    reset_ptr(stmt->ird->array_status_ptr);
    reset_ptr(stmt->apd->array_status_ptr);
    reset_ptr(stmt->ard->array_status_ptr);
    reset_ptr(stmt->stmt_options.rowStatusPtr_ex);

    if (f_option == FREE_STMT_RESET)
    {
      return SQL_SUCCESS;
    }

    /* explicitly allocated descriptors are affected up until this point */
    stmt->apd->stmt_list_remove(stmt);
    stmt->ard->stmt_list_remove(stmt);

    // Unlock before destroying STMT to prevent unowned mutex.
    // See inside ~STMT()
    if (slock)
    {
      slock.unlock();
    }

    delete stmt;
    return SQL_SUCCESS;
}


/* DESODBC:
    Renamed from the original my_SQLAllocDesc()
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  Explicitly allocate a descriptor.
*/
SQLRETURN DES_SQLAllocDesc(SQLHDBC hdbc, SQLHANDLE *pdesc)
{
  DBC *dbc= (DBC *) hdbc;
  std::unique_ptr<DESC> desc(new DESC(NULL, SQL_DESC_ALLOC_USER,
                                      DESC_APP, DESC_UNKNOWN));

  LOCK_DBC(dbc);

  if (!desc)
    return dbc->set_error("HY001", "Memory allocation error");

  desc->dbc= dbc;

  /* add to this connection's list of explicit descriptors */
  dbc->add_desc(desc.get());

  *pdesc= desc.release();
  return SQL_SUCCESS;
}

/* DESODBC:
    Renamed from the original my_SQLFreeDesc()
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  Free an explicitly allocated descriptor. This resets all statements
  that it was associated with to their implicitly allocated descriptors.
*/
SQLRETURN DES_SQLFreeDesc(SQLHANDLE hdesc)
{
  DESC *desc= (DESC *) hdesc;
  DBC *dbc= desc->dbc;

  LOCK_DBC(dbc);

  if (!desc)
    return SQL_ERROR;
  if (desc->alloc_type != SQL_DESC_ALLOC_USER)
    return set_desc_error(desc, "HY017", "Invalid use of an automatically "
                          "allocated descriptor handle.");

  /* remove from DBC */
  dbc->remove_desc(desc);

  for (auto s : desc->stmt_list)
  {
    if (IS_APD(desc))
      s->apd= s->imp_apd;
    else if (IS_ARD(desc))
      s->ard= s->imp_ard;
  }

  delete desc;
  return SQL_SUCCESS;
}


/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : ODBC 3.0 API
  @purpose : allocates an environment, connection, statement, or
       descriptor handle
*/
SQLRETURN SQL_API SQLAllocHandle(SQLSMALLINT HandleType,
                                 SQLHANDLE   InputHandle,
                                 SQLHANDLE   *OutputHandlePtr)
{
    SQLRETURN error= SQL_ERROR;

    switch (HandleType)
    {
        case SQL_HANDLE_ENV:
            CHECK_ENV_OUTPUT(OutputHandlePtr);
            error= DES_SQLAllocEnv(OutputHandlePtr);
            break;

        case SQL_HANDLE_DBC:
            CHECK_HANDLE(InputHandle);
            CHECK_DBC_OUTPUT(InputHandle, OutputHandlePtr);
            error= DES_SQLAllocConnect(InputHandle,OutputHandlePtr);
            break;

        case SQL_HANDLE_STMT:
            CHECK_HANDLE(InputHandle);
            CHECK_STMT_OUTPUT(InputHandle, OutputHandlePtr);
            error= DES_SQLAllocStmt(InputHandle,OutputHandlePtr);
            break;

        case SQL_HANDLE_DESC:
            CHECK_HANDLE(InputHandle);
            CHECK_DESC_OUTPUT(InputHandle, OutputHandlePtr);
            error= DES_SQLAllocDesc(InputHandle, OutputHandlePtr);
            break;

        default:
          return ((DBC *)InputHandle)->set_error("HYC00", "Optional feature not implemented");
    }

    return error;
}


/*
  @type    : ODBC 3.8
  @purpose : Mapped to SQLCancel if HandleType is
*/
SQLRETURN SQL_API SQLCancelHandle(SQLSMALLINT  HandleType,
                          SQLHANDLE    Handle)
{
  CHECK_HANDLE(Handle);

  switch (HandleType)
  {
  case SQL_HANDLE_DBC:
    {
      DBC *dbc= (DBC*)Handle;
      return dbc->set_error("IM001", "Driver does not support this function");
    }
  /* Normally DM should map such call to SQLCancel */
  case SQL_HANDLE_STMT:
    return SQLCancel((SQLHSTMT) Handle);
  }

  return SQL_SUCCESS;
}


/*
  @type    : ODBC 3.0 API
  @purpose : frees resources associated with a specific environment,
       connection, statement, or descriptor handle
*/

SQLRETURN SQL_API SQLFreeHandle(SQLSMALLINT HandleType,
                                SQLHANDLE   Handle)
{
    SQLRETURN error= SQL_ERROR;

    CHECK_HANDLE(Handle);

    switch (HandleType)
    {
        case SQL_HANDLE_ENV:
            error= DES_SQLFreeEnv((ENV *)Handle);
            break;

        case SQL_HANDLE_DBC:
            error= DES_SQLFreeConnect((DBC *)Handle);
            break;

        case SQL_HANDLE_STMT:
            error= DES_SQLFreeStmt((STMT *)Handle, SQL_DROP);
            break;

        case SQL_HANDLE_DESC:
            error= DES_SQLFreeDesc((DESC *) Handle);
            break;


        default:
            break;
    }

    return error;
}


