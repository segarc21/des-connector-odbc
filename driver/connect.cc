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
  @file  connect.c
  @brief Connection functions.
*/

#include "driver.h"
#include "installer.h"
#include "stringutil.h"
#include "telemetry.h"

#include <map>
#include <vector>
#include <sstream>
#include <random>

#ifndef _WIN32
#include <netinet/in.h>
#include <resolv.h>
#else
#include <winsock2.h>
#include <windns.h>
#endif

#ifndef CLIENT_NO_SCHEMA
# define CLIENT_NO_SCHEMA      16
#endif

typedef BOOL (*PromptFunc)(SQLHWND, SQLWCHAR *, SQLUSMALLINT,
                           SQLWCHAR *, SQLSMALLINT, SQLSMALLINT *,
                           SQLSMALLINT);

const char *my_os_charset_to_mysql_charset(const char *csname);

bool fido_callback_is_set = false;
fido_callback_func global_fido_callback = nullptr;
std::mutex global_fido_mutex;

bool oci_plugin_is_loaded = false;

/**
  Get the connection flags based on the driver options.

  @param[in]  options  Options flags

  @return Client flags suitable for @c mysql_real_connect().
*/
unsigned long get_client_flags(DataSource *ds)
{
  unsigned long flags= CLIENT_MULTI_RESULTS;

  if (ds->opt_SAFE || ds->opt_FOUND_ROWS)
    flags|= CLIENT_FOUND_ROWS;
  if (ds->opt_COMPRESSED_PROTO)
    flags|= CLIENT_COMPRESS;
  if (ds->opt_IGNORE_SPACE)
    flags|= CLIENT_IGNORE_SPACE;
  if (ds->opt_MULTI_STATEMENTS)
    flags|= CLIENT_MULTI_STATEMENTS;
  if (ds->opt_CLIENT_INTERACTIVE)
    flags|= CLIENT_INTERACTIVE;

  return flags;
}


void DBC::set_charset(std::string charset)
{
  // Use SET NAMES instead of mysql_set_character_set()
  // because running odbc_stmt() is thread safe.
  std::string query = "SET NAMES " + charset;
  if (execute_query(query.c_str(), query.length(), true))
  {
    //throw DESERROR("HY000", des); //TODO: remove this entire function.
  }
}

/**
 If it was specified, set the character set for the connection and
 other internal charset properties.

 @param[in]  dbc      Database connection
 @param[in]  charset  Character set name
*/
SQLRETURN DBC::set_charset_options(const char *charset)
try
{
  SQLRETURN rc = SQL_SUCCESS;
  if (unicode)
  {
    if (charset && charset[0])
    {
      // Set the warning message, SQL_SUCCESS_WITH_INFO result
      // and continue as normal.
      set_error("HY000",
        "CHARSET option is not supported by UNICODE version "
        "of MySQL Connector/ODBC", 0);
      rc = SQL_SUCCESS_WITH_INFO;
    }
    // For unicode driver always use UTF8MB4
    charset = transport_charset;
  }
  else if (!charset || !charset[0])
  {
    // For ANSI use default charset (latin1) if no chaset
    // option was specified.
    charset = ansi_default_charset;
  }

  set_charset(charset);
  DES_CHARSET_INFO my_charset;
  mysql_get_character_set_info(des, &my_charset);
  cxn_charset_info = desodbc::get_charset(my_charset.number, DESF(0));

  return rc;
}
catch(const DESERROR &e)
{
  error = e;
  return e.retcode;
}


SQLRETURN run_initstmt(DBC* dbc, DataSource* dsrc)
try
{
  if (dsrc->opt_INITSTMT)
  {
    /* Check for SET NAMES */
    if (is_set_names_statement(dsrc->opt_INITSTMT))
    {
      throw DESERROR("HY000", "SET NAMES not allowed by driver");
    }

    if (dbc->execute_query(dsrc->opt_INITSTMT, SQL_NTS, true) != SQL_SUCCESS)
    {
      return SQL_ERROR;
    }
  }
  return SQL_SUCCESS;
}
catch (const DESERROR& e)
{
  dbc->error = e;
  return e.retcode;
}

/*
  Retrieve DNS+SRV list.

  @param[in]  hostname  Full DNS+SRV address (ex: _mysql._tcp.example.com)
  @param[out]  total_weight   Retrieve sum of total weight.

  @return multimap containing list of SRV records
*/


struct Prio
{
  uint16_t prio;
  uint16_t weight;
  operator uint16_t() const
  {
    return prio;
  }

  bool operator < (const Prio &other) const
  {
    return prio < other.prio;
  }
};

struct Srv_host_detail
{
  std::string name;
  unsigned int port = MYSQL_PORT;
};


/*
  Parse a comma separated list of hosts, each optionally specifying a port after
  a colon. If port is not specified then the default port is used.
*/
std::vector<Srv_host_detail> parse_host_list(const char* hosts_str,
                                             unsigned int default_port)
{
  std::vector<Srv_host_detail> list;

  //if hosts_str = nullprt will still add empty host, meaning it will use socket
  std::string hosts(hosts_str ? hosts_str : "");

  size_t pos_i = 0;
  size_t pos_f = std::string::npos;

  do
  {
    pos_f = hosts.find_first_of(",:", pos_i);

    Srv_host_detail host_detail;
    host_detail.name = hosts.substr(pos_i, pos_f-pos_i);

    if( pos_f != std::string::npos && hosts[pos_f] == ':')
    {
      //port
      pos_i = pos_f+1;
      pos_f = hosts.find_first_of(',', pos_i);
      std::string tmp_port = hosts.substr(pos_i,pos_f-pos_i);
      long int val = std::atol(tmp_port.c_str());

      /*
        Note: strtol() returns 0 either if the number is 0
        or conversion was not possible. We distinguish two cases
        by cheking if end pointer was updated.
      */

      if ((val == 0 && tmp_port.length()==0) ||
          (val > 65535 || val < 0))
      {
        std::stringstream err;
        err << "Invalid port value in " << hosts;
        throw err.str();
      }

      host_detail.port = static_cast<uint16_t>(val);
    }
    else
    {
      host_detail.port = default_port;
    }

    pos_i = pos_f+1;

    list.push_back(host_detail);
  }while (pos_f < hosts.size());

  return list;
}


class dbc_guard
{
  DBC *m_dbc;
  bool m_success = false;
  public:

  dbc_guard(DBC *dbc) : m_dbc(dbc) {}
  void set_success(bool success) { m_success = success; }
  ~dbc_guard() { if (!m_success) m_dbc->close(); }
};

SQLRETURN DBC::createPipes() {
    /*

        We will create anonymous pipes so
        the driver can communicate with a new DES process.
        The idea is that the driver can read and write onto
        an exclusive instance of DES, as its executable provides
        a CLI.

        For creating the pipes, we will follow this Microsoft article that shows
        how to create a child process with redirected input and output:
        https://learn.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output

    */

  ENV *env = this->env;

  // We specify the security attributes of the pipe.
  SECURITY_ATTRIBUTES des_pipe_sec_attr;
  des_pipe_sec_attr.nLength =
      sizeof(SECURITY_ATTRIBUTES);  // the manual specifies to initialize this
                                    // as this size
  des_pipe_sec_attr.lpSecurityDescriptor =
      NULL;  // if NULL, the pipe has the default security configuration
  des_pipe_sec_attr.bInheritHandle =
      TRUE;  // we need this process to inherit the handle

  DWORD des_pipe_buf_size = 0;  // if 0, the system uses the default buffer
                                // size.

  // We create the pipe
  if (!CreatePipe(&env->driver_to_des_out_rpipe, &env->driver_to_des_out_wpipe,
                  &des_pipe_sec_attr,
                  des_pipe_buf_size)) {
    CloseHandle(env->driver_to_des_out_rpipe);
    CloseHandle(env->driver_to_des_out_wpipe);
    return SQL_ERROR;
  }

  /* Now we need to specify to the read handle for STDOUT
   * that it cannot be inherited. The problem with the children
   * having access to it is that the parent process (the driver)
   * doesn't have full control over it.*/

  if (!SetHandleInformation(env->driver_to_des_out_rpipe, HANDLE_FLAG_INHERIT,
                            0)) {
    CloseHandle(env->driver_to_des_out_rpipe);
    CloseHandle(env->driver_to_des_out_wpipe);
    return SQL_ERROR;
  }

  // Now, we do the same for the STDIN.
  if (!CreatePipe(&env->driver_to_des_in_rpipe, &env->driver_to_des_in_wpipe,
                  &des_pipe_sec_attr, des_pipe_buf_size)) {
    CloseHandle(env->driver_to_des_in_rpipe);
    CloseHandle(env->driver_to_des_out_wpipe);
    return SQL_ERROR;
  }

  if (!SetHandleInformation(env->driver_to_des_in_wpipe,
                            HANDLE_FLAG_INHERIT,
                            0)) {
    CloseHandle(env->driver_to_des_out_rpipe);
    CloseHandle(env->driver_to_des_out_wpipe);
    CloseHandle(env->driver_to_des_in_rpipe);
    CloseHandle(env->driver_to_des_in_wpipe);
    return SQL_ERROR;
  }


  return SQL_SUCCESS;
}

const wchar_t* get_executable_dir(const wchar_t *executable_path) {

  std::wstring executable_path_wstr(executable_path);
  size_t pos = executable_path_wstr.find_last_of(L"\\/");

  if (pos != std::wstring::npos) {
    std::wstring dir_wstr = executable_path_wstr.substr(0, pos);
    size_t dir_size = dir_wstr.size();

    wchar_t *dir = new wchar_t[dir_size + 1]; //TODO: consider making an 'util' function out of this
    dir_wstr.copy(dir, dir_size);
    dir[dir_size] = L'\0';

    return dir;

  } else { //Incorrect path. TODO: handle error appropiately.
    exit(1);
    return nullptr; //to prevent compiler errors
  }
}

SQLRETURN DBC::createDESProcess(SQLWCHAR* des_path) {

  ENV *env = this->env;

  // Before creating the process, we need to create STARTUPINFO and
  // PROCESS_INFORMATION structures. Piece of code extracted from the
  // Microsoft's child process creating tutorial
  ZeroMemory(&env->startup_info_unicode, sizeof(env->startup_info_unicode));
  ZeroMemory(&env->process_info, sizeof(env->process_info));
  env->startup_info_unicode.cb = sizeof(env->startup_info_unicode);
  env->startup_info_unicode.hStdError = env->driver_to_des_out_wpipe;
  env->startup_info_unicode.hStdOutput = env->driver_to_des_out_wpipe;
  env->startup_info_unicode.hStdInput = env->driver_to_des_in_rpipe;
  env->startup_info_unicode.dwFlags |= STARTF_USESTDHANDLES;
  env->startup_info_unicode.dwFlags |= STARTF_USESHOWWINDOW;
  env->startup_info_unicode.wShowWindow = SW_HIDE;

    /* Now, we will call the function CreateProcessW (processthreadsapi.h).
*
* From the win32 api manual:
*
* BOOL CreateProcessW(
    [in, optional]      LPCWSTR               lpApplicationName,
    [in, out, optional] LPWSTR                lpCommandLine,
    [in, optional]      LPSECURITY_ATTRIBUTES lpProcessAttributes,
    [in, optional]      LPSECURITY_ATTRIBUTES lpThreadAttributes,
    [in]                BOOL                  bInheritHandles,
    [in]                DWORD                 dwCreationFlags,
    [in, optional]      LPVOID                lpEnvironment,
    [in, optional]      LPCWSTR               lpCurrentDirectory,
    [in]                LPSTARTUPINFOW        lpStartupInfo,
    [out]               LPPROCESS_INFORMATION lpProcessInformation
  );

  - lpApplicationName: if NULL, it justs go for lpApplicationName (simpler).
  - lpCommandLine: route of the DES executable.
  - lpProcessAttributes: if NULL, the identifier of the new process cannot
      be inherited (preferrable, as we have explained before)
  - lpThreadAttributes: if NULL, it is not inheritable either.
  - bInheritHandles: if TRUE, handlers are inheritable.
  - dwCreationFlags: if 0, it has the default creation flags. We do not need
any special flags.
  - lpEnvironment: if NULL, we use parent's environment block
  - lpCurrentDirectory: if NULL, we use parent's starting directory
  - lpStartupInfo: the pointer to a STARTUPINFO structure
  - lpProcessInformation: the pointer to a PROCESS_INFORMATION structure

*/

  //Setting the current directory -the folder in which the des executable is in-.
  const wchar_t *des_dir = get_executable_dir(des_path);
  
  if (!CreateProcessW(NULL, des_path, NULL, NULL, TRUE, 0, NULL, des_dir,
                      &env->startup_info_unicode,
                      &env->process_info)) {
    CloseHandle(env->driver_to_des_out_rpipe);
    CloseHandle(env->driver_to_des_out_wpipe);
    CloseHandle(env->driver_to_des_in_rpipe);
    CloseHandle(env->driver_to_des_in_wpipe);
    return ERROR;
  }

  /* Now, the DES process has just been created. However, we need to remove all the startup messages
  from DES that are allocated into the STDOUT read pipe. We read from this pipe using the ReadFile
  function (fileapi.h), and it writes it into a buffer. When the buffer contains "DES>", the startup
  messages will have ended. */

  CHAR buffer[4096]; //standard size
  DWORD bytes_read;

  bool finished_reading = false;
  std::string complete_reading_str = "";
  while (!finished_reading) {
    BOOL success =
        ReadFile(env->driver_to_des_out_rpipe, buffer,
                            sizeof(buffer) - sizeof(CHAR), &bytes_read, NULL);/* we don't write the final CHAR so as to put the '\0'
                                                                                 char, like we will see.
                                                                                 the final argument must be not null only when
                                                                                 the pipe was created with overlapping */ 


    buffer[bytes_read] = '/0'; //we need it to be zero-terminated, as advised by a warning when tring to assign the char[] to a new std::string
    std::string buffer_str = buffer;

    complete_reading_str += buffer_str;

    if (!success || complete_reading_str.find("DES>") != std::string::npos) {
      finished_reading = true;
    }
  }

  env->shmem->des_process_created = true;

  return SQL_SUCCESS;
}

void shareHandles() {
  ENV *env = dbc_global_var->env;
  SharedMemory *shmem = env->shmem;

  while (true) {
    DWORD wait_event = WaitForSingleObject(env->request_handle_event, INFINITE);
    // TODO: handle errors for wait_event != WAIT_OBJECT_0
    if (wait_event == WAIT_OBJECT_0 &&
        shmem->handle_sharing_info.pid_handle_petitioner != 0 &&
        shmem->handle_sharing_info.pid_handle_petitionee ==
            GetCurrentProcessId()) {
      HANDLE petitioner_process_handle =
          OpenProcess(PROCESS_DUP_HANDLE, TRUE,
                      shmem->handle_sharing_info.pid_handle_petitioner);

      DuplicateHandle(GetCurrentProcess(), env->driver_to_des_out_rpipe,
                      petitioner_process_handle,
                      &shmem->handle_sharing_info.out_handle, 0, TRUE,
                      DUPLICATE_SAME_ACCESS);

      DuplicateHandle(GetCurrentProcess(), env->driver_to_des_in_wpipe,
                      petitioner_process_handle,
                      &shmem->handle_sharing_info.in_handle, 0, TRUE,
                      DUPLICATE_SAME_ACCESS);

      ResetEvent(env->request_handle_event);
      SetEvent(env->handle_sent_event);  // we notify the petitioner
    }
  }
}


void getDESProcessPipes() {
  bool finished = false;

  ENV *env = dbc_global_var->env;

  WaitForSingleObject(env->shared_memory_mutex, INFINITE);
  WaitForSingleObject(env->request_handle_mutex, INFINITE);

  env->shmem->handle_sharing_info.pid_handle_petitioner = GetCurrentProcessId();

  while (!finished) {
    DWORD random_pid =
        env->shmem->pids_connected_struct
            .pids_connected[rand() %
                            (env->shmem->pids_connected_struct.size -
                             1)];  // we request the handles to a random peer
                                   // TODO: impleemnt better criteria
    if (random_pid != GetCurrentProcessId()) {
      env->shmem->handle_sharing_info.pid_handle_petitionee = random_pid;

      SetEvent(env->request_handle_event);
      WaitForSingleObject(env->handle_sent_event,
                          INFINITE);  // TODO: handle errors

      env->driver_to_des_out_rpipe = env->shmem->handle_sharing_info.out_handle;
      env->driver_to_des_in_wpipe = env->shmem->handle_sharing_info.in_handle;

      // we reset the structure once we have saved the handles
      env->shmem->handle_sharing_info.in_handle = NULL;
      env->shmem->handle_sharing_info.out_handle = NULL;
      env->shmem->handle_sharing_info.pid_handle_petitionee = 0;
      env->shmem->handle_sharing_info.pid_handle_petitioner = 0;

      finished = true;
    }
  }

  ReleaseMutex(env->request_handle_mutex);
  ReleaseMutex(env->shared_memory_mutex);
}

/**
  Try to establish a connection to a MySQL server based on the data source
  configuration.

  @param[in]  ds   Data source information

  @return Standard SQLRETURN code. If it is @c SQL_SUCCESS or @c
  SQL_SUCCESS_WITH_INFO, a connection has been established.
*/
SQLRETURN DBC::connect(DataSource *dsrc)
{
  SQLRETURN rc = SQL_SUCCESS;
  WaitForSingleObject(this->env->shared_memory_mutex, INFINITE);
  SharedMemory *shmem = this->env->shmem;

  shmem->pids_connected_struct
      .pids_connected[shmem->pids_connected_struct.size] =
      GetCurrentProcessId();

  shmem->pids_connected_struct.size += 1;

  ReleaseMutex(this->env->shared_memory_mutex);
  // rc = dbc->connect(&ds);

  // TODO: solution for only UTF-8, expand it to ANSI also.
  this->cxn_charset_info = desodbc::get_charset(255, DESF(0));

  /* We now save the path to the DES executable in Unicode format (wchars). It
 reads the DES executable field from the Data Source (loaded previously with
 data from the Windows Registries).
 TODO: expand it to ANSI also. */
  const SQLWSTRING &x = static_cast<const SQLWSTRING &>(dsrc->opt_DES_EXEC);
  SQLWCHAR *unicode_path = const_cast<SQLWCHAR *>(x.c_str());

  if (!this->env->shmem->des_process_created) {
    rc = this->createPipes();
    rc = this->createDESProcess(unicode_path);
  } else {
    getDESProcessPipes();
    rc = SQL_SUCCESS;
  }

  this->env->share_handle_thread =
      std::unique_ptr<std::thread>(new std::thread(shareHandles));

  if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
      this->connected = true;

  return rc;
}


/**
  Establish a connection to a data source.

  @param[in]  hdbc    Connection handle
  @param[in]  szDSN   Data source name (in connection charset)
  @param[in]  cbDSN   Length of data source name in bytes or @c SQL_NTS
  @param[in]  szUID   User identifier (in connection charset)
  @param[in]  cbUID   Length of user identifier in bytes or @c SQL_NTS
  @param[in]  szAuth  Authentication string (password) (in connection charset)
  @param[in]  cbAuth  Length of authentication string in bytes or @c SQL_NTS

  @return  Standard ODBC success codes

  @since ODBC 1.0
  @since ISO SQL 92
*/
SQLRETURN SQL_API DESConnect(SQLHDBC   hdbc,
                               SQLWCHAR *szDSN, SQLSMALLINT cbDSN,
                               SQLWCHAR *szUID, SQLSMALLINT cbUID,
                               SQLWCHAR *szAuth, SQLSMALLINT cbAuth)
{
  SQLRETURN rc;
  DBC *dbc= (DBC *)hdbc;
  DataSource ds;

#ifdef NO_DRIVERMANAGER
  return ((DBC*)dbc)->set_error("HY000",
                       "SQLConnect requires DSN and driver manager", 0);
#else

  /* Can't connect if we're already connected. */
  if (is_connected(dbc))
    return ((DBC*)hdbc)->set_error(DESERR_08002, NULL, 0);

  /* Reset error state */
  CLEAR_DBC_ERROR(dbc);

  ds.opt_DSN.set_remove_brackets(szDSN, cbDSN);
  ds.lookup();

  rc= dbc->connect(&ds);

  if (SQL_SUCCEEDED(rc) && (szUID || szAuth)) {
    dbc->set_error("01000", "The user/password provided was ignored (DES doesn't need it).", 0);
    rc = SQL_SUCCESS_WITH_INFO;
  }
  
  return rc;
#endif
}


/**
  An alternative to SQLConnect that allows specifying more of the connection
  parameters, and whether or not to prompt the user for more information
  using the setup library.

  NOTE: The prompting done in this function using MYODBCUTIL_DATASOURCE has
        been observed to cause an access violation outside of our code
        (specifically in Access after prompting, before table link list).
        This has only been seen when setting a breakpoint on this function.

  @param[in]  hdbc  Handle of database connection
  @param[in]  hwnd  Window handle. May be @c NULL if no prompting will be done.
  @param[in]  szConnStrIn  The connection string
  @param[in]  cbConnStrIn  The length of the connection string in characters
  @param[out] szConnStrOut Pointer to a buffer for the completed connection
                           string. Must be at least 1024 characters.
  @param[in]  cbConnStrOutMax Length of @c szConnStrOut buffer in characters
  @param[out] pcbConnStrOut Pointer to buffer for returning length of
                            completed connection string, in characters
  @param[in]  fDriverCompletion Complete mode, one of @cSQL_DRIVER_PROMPT,
              @c SQL_DRIVER_COMPLETE, @c SQL_DRIVER_COMPLETE_REQUIRED, or
              @cSQL_DRIVER_NOPROMPT

  @return Standard ODBC return codes

  @since ODBC 1.0
  @since ISO SQL 92
*/
SQLRETURN SQL_API DESDriverConnect(SQLHDBC hdbc, SQLHWND hwnd,
                                     SQLWCHAR *szConnStrIn,
                                     SQLSMALLINT cbConnStrIn,
                                     SQLWCHAR *szConnStrOut,
                                     SQLSMALLINT cbConnStrOutMax,
                                     SQLSMALLINT *pcbConnStrOut,
                                     SQLUSMALLINT fDriverCompletion)
{
  SQLRETURN rc= SQL_SUCCESS;
  DBC *dbc= (DBC *)hdbc;
  DataSource ds;
  /* We may have to read driver info to find the setup library. */
  Driver driver;
  /* We never know how many new parameters might come out of the prompt */
  SQLWCHAR prompt_outstr[4096];
  BOOL bPrompt= FALSE;
  HMODULE hModule= NULL;
  SQLWSTRING conn_str_in, conn_str_out;
  SQLWSTRING prompt_instr;

  if (cbConnStrIn != SQL_NTS)
    conn_str_in = SQLWSTRING(szConnStrIn, cbConnStrIn);
  else
    conn_str_in = szConnStrIn;

  /* Parse the incoming string */
  if (ds.from_kvpair(conn_str_in.c_str(), (SQLWCHAR)';'))
  {
    rc= dbc->set_error( "HY000",
                      "Failed to parse the incoming connect string.", 0);
    goto error;
  }

#ifndef NO_DRIVERMANAGER
  /*
   If the connection string contains the DSN keyword, the driver retrieves
   the information for the specified data source (and merges it into the
   connection info with the provided connection info having precedence).

   This also allows us to get pszDRIVER (if not already given).
  */
  if (ds.opt_DSN)
  {
     ds.lookup();

    /*
      If DSN is used:
      1 - we want the connection string options to override DSN options
      2 - no need to check for parsing erros as it was done before
    */
     ds.from_kvpair(conn_str_in.c_str(), (SQLWCHAR)';');
  }
#endif

  /* If FLAG_NO_PROMPT is not set, force prompting off. */
  if (ds.opt_NO_PROMPT)
    fDriverCompletion= SQL_DRIVER_NOPROMPT;

  /*
    We only prompt if we need to.

    A not-so-obvious gray area is when SQL_DRIVER_COMPLETE or
    SQL_DRIVER_COMPLETE_REQUIRED was specified along without a server or
    password - particularly password. These can work with defaults/blank but
    many callers expect prompting when these are blank. So we compromise; we
    try to connect and if it works we say its ok otherwise we go to
    a prompt.
  */
  switch (fDriverCompletion)
  {
  case SQL_DRIVER_PROMPT:
    bPrompt= TRUE;
    break;

  case SQL_DRIVER_COMPLETE:
  case SQL_DRIVER_COMPLETE_REQUIRED:
    rc = dbc->connect(&ds);
    if (!SQL_SUCCEEDED(rc)) dbc->telemetry.set_error(dbc, dbc->error.message);
    if (SQL_SUCCEEDED(rc) &&
        ((conn_str_in.find(L"UID=") != std::wstring::npos) ||
         (conn_str_in.find(L"PWD=") != std::wstring::npos))) {
        dbc->set_error(
            "01000",
            "The user/password provided was ignored (DES doesn't need it).", 0);
        rc = SQL_SUCCESS_WITH_INFO;
    }

    if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
      goto connected;
    bPrompt= TRUE;
    break;

  case SQL_DRIVER_NOPROMPT:
    bPrompt= FALSE;
    break;

  default:
    rc= dbc->set_error("HY110", "Invalid driver completion.", 0);
    goto error;
  }

#ifdef __APPLE__
  /*
    We don't support prompting on Mac OS X.
  */
  if (bPrompt)
  {
    rc= dbc->set_error("HY000",
                       "Prompting is not supported on this platform. "
                       "Please provide all required connect information.",
                       0);
    goto error;
  }
#endif

  if (bPrompt)
  {
    PromptFunc pFunc;

    /*
     We can not present a prompt unless we can lookup the name of the setup
     library file name. This means we need a DRIVER. We try to look it up
     using a DSN (above) or, hopefully, get one in the connection string.

     This will, when we need to prompt, trump the ODBC rule where we can
     connect with a DSN which does not exist. A possible solution would be to
     hard-code some fall-back value for ds->pszDRIVER.
    */
    if (!ds.opt_DRIVER)
    {
      char szError[1024];
      sprintf(szError,
              "Could not determine the driver name; "
              "could not lookup setup library. DSN=(%s)\n",
              (const char*)ds.opt_DSN);
      rc= dbc->set_error("HY000", szError, 0);
      goto error;
    }

#ifndef NO_DRIVERMANAGER
    /* We can not present a prompt if we have a null window handle. */
    if (!hwnd)
    {
      rc= dbc->set_error("IM008", "Invalid window handle", 0);
      goto error;
    }

    /* if given a named DSN, we will have the path in the DRIVER field */
    if (ds.opt_DSN)
      driver.lib = ds.opt_DRIVER;
    /* otherwise, it's the driver name */
    else
      driver.name = ds.opt_DRIVER;

    if (driver.lookup())
    {
      char sz[1024];
      sprintf(sz, "Could not find driver '%s' in system information.",
              (const char*)ds.opt_DRIVER);

      rc= dbc->set_error("IM003", sz, 0);
      goto error;
    }

    if (!driver.setup_lib)
#endif
    {
      rc= dbc->set_error("HY000",
                        "Could not determine the file name of setup library.",
                        0);
      goto error;
    }

    /*
     We dynamically load the setup library so the driver itself does not
     depend on GUI libraries.
    */

    if (!(hModule= LoadLibrary(driver.setup_lib)))
    {
      char sz[1024];
      sprintf(sz, "Could not load the setup library '%s'.",
              (const char *)driver.setup_lib);
      rc= dbc->set_error("HY000", sz, 0);
      goto error;
    }

    pFunc= (PromptFunc)GetProcAddress(hModule, "Driver_Prompt");

    if (pFunc == NULL)
    {
#ifdef WIN32
      LPVOID pszMsg;

      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL, GetLastError(),
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR)&pszMsg, 0, NULL);
      rc= dbc->set_error("HY000", (char *)pszMsg, 0);
      LocalFree(pszMsg);
#else
      rc= dbc->set_error("HY000", dlerror(), 0);
#endif
      goto error;
    }

    /* create a string for prompting, and add driver manually */
    prompt_instr = ds.to_kvpair(';');
    prompt_instr.append(W_DRIVER_PARAM);
    SQLWSTRING drv = (const SQLWSTRING&)ds.opt_DRIVER;
    prompt_instr.append(drv);

    /*
      In case the client app did not provide the out string we use our
      inner buffer prompt_outstr
    */
    if (!pFunc(hwnd, (SQLWCHAR*)prompt_instr.c_str(), fDriverCompletion,
               prompt_outstr, sizeof(prompt_outstr), pcbConnStrOut,
               (SQLSMALLINT)dbc->unicode))
    {
      dbc->set_error("HY000", "User cancelled.", 0);
      rc= SQL_NO_DATA;
      goto error;
    }

    /* refresh our DataSource */
    ds.reset();
    if (ds.from_kvpair(prompt_outstr, ';'))
    {
      rc= dbc->set_error( "HY000",
                        "Failed to parse the prompt output string.", 0);
      goto error;
    }

    /*
      We don't need prompt_outstr after the new DataSource is created.
      Copy its contents into szConnStrOut if possible
    */
    if (szConnStrOut)
    {
      *pcbConnStrOut= (SQLSMALLINT)desodbc_min(cbConnStrOutMax, *pcbConnStrOut);
      memcpy(szConnStrOut, prompt_outstr, (size_t)*pcbConnStrOut*sizeof(SQLWCHAR));
      /* term needed if possibly truncated */
      szConnStrOut[*pcbConnStrOut - 1] = 0;
    }

  }

  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
  {
    goto error;
  }

  if (ds.opt_SAVEFILE)
  {
    /* We must disconnect if File DSN is created */
    dbc->close();
  }

connected:

  /* copy input to output if connected without prompting */
  if (!bPrompt)
  {
    size_t copylen;
    conn_str_out = conn_str_in;
    if (ds.opt_SAVEFILE)
    {
      SQLWSTRING pwd_temp = (const SQLWSTRING &)ds.opt_PWD;

      /* make sure the password does not go into the output buffer */
      ds.opt_PWD = nullptr;

      conn_str_out = ds.to_kvpair(';');

      /* restore old values */
      ds.opt_PWD = pwd_temp;
    }

    size_t inlen = conn_str_out.length();
    copylen = desodbc_min((size_t)cbConnStrOutMax, inlen + 1) * sizeof(SQLWCHAR);

    if (szConnStrOut && copylen)
    {
      memcpy(szConnStrOut, conn_str_out.c_str(), copylen);
      /* term needed if possibly truncated */
      szConnStrOut[(copylen / sizeof(SQLWCHAR)) - 1] = 0;
    }

    /*
      Even if the buffer for out connection string is NULL
      it will still return the total number of characters
      (excluding the null-termination character for character data)
      available to return in the buffer pointed to by
      OutConnectionString.
      https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/sqldriverconnect-function?view=sql-server-ver16
    */
    if (pcbConnStrOut)
      *pcbConnStrOut = (SQLSMALLINT)inlen;
  }

  /* return SQL_SUCCESS_WITH_INFO if truncated output string */
  if (pcbConnStrOut && cbConnStrOutMax &&
      cbConnStrOutMax - sizeof(SQLWCHAR) <= *pcbConnStrOut * sizeof(SQLWCHAR))
  {
    dbc->set_error("01004", "String data, right truncated.", 0);
    rc= SQL_SUCCESS_WITH_INFO;
  }

error:

  if (!SQL_SUCCEEDED(rc))
    dbc->telemetry.set_error(dbc, dbc->error.message);
  if (hModule)
    FreeLibrary(hModule);

  return rc;
}


void DBC::free_connection_stmts()
{
  for (auto it = stmt_list.begin(); it != stmt_list.end(); )
  {
      STMT *stmt = *it;
      it = stmt_list.erase(it);
      DES_SQLFreeStmt((SQLHSTMT)stmt, SQL_DROP);
  }
  stmt_list.clear();
}


/**
  Disconnect a connection.

  @param[in]  hdbc   Connection handle

  @return  Standard ODBC return codes

  @since ODBC 1.0
  @since ISO SQL 92
*/
SQLRETURN SQL_API SQLDisconnect(SQLHDBC hdbc)
{
  DBC *dbc= (DBC *) hdbc;

  CHECK_HANDLE(hdbc);

  dbc->free_connection_stmts();

  dbc->close();

  if (dbc->ds.opt_LOG_QUERY)
    end_query_log(dbc->query_log);

  /* free allocated packet buffer */

  dbc->database.clear();
  return SQL_SUCCESS;
}


SQLRETURN DBC::execute_query(const char* query,
  SQLULEN query_length, des_bool req_lock)
{
  SQLRETURN result = SQL_SUCCESS;
  LOCK_DBC_DEFER(this);

  if (req_lock)
  {
    DO_LOCK_DBC();
  }

  if (query_length == SQL_NTS)
  {
    query_length = strlen(query);
  }

  if (check_if_server_is_alive(this) ||
    mysql_real_query(des, query, (unsigned long)query_length))
  {
    result = set_error(DESERR_S1000, mysql_error(des),
      mysql_errno(des));
  }

  return result;

}
