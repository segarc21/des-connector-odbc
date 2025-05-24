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
@file  connect.c
@brief Connection functions.
*/

#include "driver.h"
#include "installer.h"
#include "stringutil.h"

#include <map>
#include <random>
#include <sstream>
#include <vector>

#ifndef _WIN32
#include <netinet/in.h>
#include <resolv.h>
#else
#include <windns.h>
#include <winsock2.h>
#endif

#ifndef _WIN32
// DESODBC: new headers to include in Unix-like systems
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdio>
#include <iostream>
#endif

#ifndef CLIENT_NO_SCHEMA
#define CLIENT_NO_SCHEMA 16
#endif

typedef BOOL (*PromptFunc)(SQLHWND, SQLWCHAR *, SQLUSMALLINT, SQLWCHAR *,
                           SQLSMALLINT, SQLSMALLINT *, SQLSMALLINT);

fido_callback_func global_fido_callback = nullptr;
std::mutex global_fido_mutex;

#ifdef _WIN32
/* DESODBC:
  Original author: DESODBC Developer
  */
LPCSTR DBC::build_name(const char *name_base) {
  std::string name_str = name_base;
  name_str += "_";
  name_str += this->connection_hash;

  char *name = new char[name_str.size() + 1];
  if (!name) throw std::bad_alloc();

  memcpy(name, name_str.c_str(), name_str.size());
  name[name_str.size()] = '\0';
  return name;
}

/* DESODBC:
    Original author: DESODBC Developer
    */
void DBC::get_concurrent_objects(const wchar_t *des_exec_path,
                          const wchar_t *des_working_dir) {
  std::wstring des_exec_path_wstr(des_exec_path);
  std::wstring des_working_dir_wstr(des_working_dir);

  this->connection_hash = std::to_string(wstr_hasher(des_working_dir_wstr));

  this->exec_hash_int = wstr_hasher(des_exec_path_wstr);

  this->SHARED_MEMORY_NAME = build_name(SHARED_MEMORY_NAME_BASE);
  this->SHARED_MEMORY_MUTEX_NAME = build_name(SHARED_MEMORY_MUTEX_NAME_BASE);
  this->QUERY_MUTEX_NAME = build_name(QUERY_MUTEX_NAME_BASE);
  this->REQUEST_HANDLE_EVENT_NAME = build_name(REQUEST_HANDLE_EVENT_NAME_BASE);
  this->REQUEST_HANDLE_MUTEX_NAME = build_name(REQUEST_HANDLE_MUTEX_NAME_BASE);
  this->HANDLE_SENT_EVENT_NAME = build_name(HANDLE_SENT_EVENT_NAME_BASE);
  this->FINISHING_EVENT_NAME = build_name(FINISHING_EVENT_NAME_BASE);
}

#else
/* DESODBC:
  Original author: DESODBC Developer
  */
const char * DBC::build_name(const char *name_base) {
  std::string name_str = name_base;
  name_str += "_";
  name_str += std::to_string(this->connection_hash_int);

  char *name = new char[name_str.size() + 1];
  if (!name) throw std::bad_alloc();

  memcpy(name, name_str.c_str(), name_str.size());
  name[name_str.size()] = '\0';
  return name;
}

/* DESODBC:
    Original author: DESODBC Developer
    */
void DBC::get_concurrent_objects(const char *des_exec_path,
                          const char *des_working_dir) {
  std::string des_exec_path_str(des_exec_path);
  std::string des_working_dir_str(des_working_dir);

  this->connection_hash = std::to_string(str_hasher(des_working_dir_str));

  this->connection_hash_int = 0;
  for (int i = 0; i < this->connection_hash.size(); ++i) {
    this->connection_hash_int =
        this->connection_hash_int * 10 + (this->connection_hash[i] - '0');
  }

  this->exec_hash = std::to_string(str_hasher(des_exec_path_str));

  this->exec_hash_int = 0;
  for (int i = 0; i < this->exec_hash.size(); ++i) {
    this->exec_hash_int = this->exec_hash_int * 10 + (this->exec_hash[i] - '0');
  }

  this->SHARED_MEMORY_NAME = build_name(SHARED_MEMORY_NAME_BASE);
  this->SHARED_MEMORY_MUTEX_NAME = build_name(SHARED_MEMORY_MUTEX_NAME_BASE);
  this->QUERY_MUTEX_NAME = build_name(QUERY_MUTEX_NAME_BASE);
  this->IN_WPIPE_NAME = build_name(IN_WPIPE_NAME_BASE);
  this->OUT_RPIPE_NAME = build_name(OUT_RPIPE_NAME_BASE);
}
#endif

/* DESODBC:
  This function creates the pipes for the
  DES process we are about to launch.

  Original author: DESODBC Developer
*/
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

#ifdef _WIN32

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
  if (!CreatePipe(&this->driver_to_des_out_rpipe,
                  &this->driver_to_des_out_wpipe, &des_pipe_sec_attr,
                  des_pipe_buf_size)) {
    CloseHandle(this->driver_to_des_out_rpipe);
    CloseHandle(this->driver_to_des_out_wpipe);
    return this->set_win_error("Failed to create DES output pipe", true);
  }

  /* Now we need to specify to the read handle for STDOUT
   * that it cannot be inherited. The problem with the children
   * having access to it is that the parent process (the driver)
   * doesn't have full control over it.*/

  if (!SetHandleInformation(this->driver_to_des_out_rpipe, HANDLE_FLAG_INHERIT,
                            0)) {
    CloseHandle(this->driver_to_des_out_rpipe);
    CloseHandle(this->driver_to_des_out_wpipe);
    return this->set_win_error(
        "Failed to set DES output pipe to be inheritable", true);
  }

  // Now, we do the same for the STDIN.
  if (!CreatePipe(&this->driver_to_des_in_rpipe, &this->driver_to_des_in_wpipe,
                  &des_pipe_sec_attr, des_pipe_buf_size)) {
    CloseHandle(this->driver_to_des_in_rpipe);
    CloseHandle(this->driver_to_des_out_wpipe);
    return this->set_win_error("Failed to create DES input pipe", true);
  }

  if (!SetHandleInformation(this->driver_to_des_in_wpipe, HANDLE_FLAG_INHERIT,
                            0)) {
    CloseHandle(this->driver_to_des_out_rpipe);
    CloseHandle(this->driver_to_des_out_wpipe);
    CloseHandle(this->driver_to_des_in_rpipe);
    CloseHandle(this->driver_to_des_in_wpipe);
    return this->set_win_error("Failed to set DES input pipe to be inheritable",
                               true);
  }
#else

  if (mkfifo(IN_WPIPE_NAME, 0666) == -1) {
    std::string msg = "Failed to create DES input pipe";
    msg += " (";
    msg += IN_WPIPE_NAME;
    msg += "). ";
    msg +=
        "Maybe is it already created? Check /tmp/ folder and remove named pipe "
        "if so";
    return this->set_unix_error(msg, true);
  }

  if (mkfifo(OUT_RPIPE_NAME, 0666) == -1) {
    std::string msg = "Failed to create DES output pipe";
    msg += " (";
    msg += OUT_RPIPE_NAME;
    msg += "). ";
    msg +=
        "Maybe is it already created? Check /tmp/ folder and remove named pipe "
        "if so";
    return this->set_unix_error(msg, true);
  }
#endif
  return SQL_SUCCESS;
}

#ifdef _WIN32
/* DESODBC:
  This function creates the DES global process in
  Windows.

  Original author: DESODBC Developer
*/
SQLRETURN DBC::create_DES_process(SQLWCHAR *des_exec_path,
                                SQLWCHAR *des_working_dir) {
  // Before creating the process, we need to create STARTUPINFO and
  // PROCESS_INFORMATION structures. Piece of code extracted from the
  // Microsoft's child process creating tutorial
  ZeroMemory(&this->startup_info_unicode, sizeof(this->startup_info_unicode));
  ZeroMemory(&this->process_info, sizeof(this->process_info));
  this->startup_info_unicode.cb = sizeof(this->startup_info_unicode);
  this->startup_info_unicode.hStdError = this->driver_to_des_out_wpipe;
  this->startup_info_unicode.hStdOutput = this->driver_to_des_out_wpipe;
  this->startup_info_unicode.hStdInput = this->driver_to_des_in_rpipe;
  this->startup_info_unicode.dwFlags |= STARTF_USESTDHANDLES;
  this->startup_info_unicode.dwFlags |= STARTF_USESHOWWINDOW;
  this->startup_info_unicode.wShowWindow = SW_HIDE;

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
  - dwCreationFlags: if 0, we need DETACHED_PROCESS and CREATE_NEW_PROCESS_GROUP
    so that the process does not die if the parent does.
  - lpEnvironment: if NULL, we use parent's environment block
  - lpCurrentDirectory: if NULL, we use parent's starting directory
  - lpStartupInfo: the pointer to a STARTUPINFO structure
  - lpProcessInformation: the pointer to a PROCESS_INFORMATION structure
  */

  if (!CreateProcessW(NULL, des_exec_path, NULL, NULL, TRUE,
                      DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP, NULL,
                      des_working_dir, &this->startup_info_unicode,
                      &this->process_info)) {
    CloseHandle(this->driver_to_des_out_rpipe);
    CloseHandle(this->driver_to_des_out_wpipe);
    CloseHandle(this->driver_to_des_in_rpipe);
    CloseHandle(this->driver_to_des_in_wpipe);
    return this->set_win_error(
        "Failed to create DES process given the info specified", true);
    return SQL_ERROR;
  }

  // We save the PID of the DES global process
  this->shmem->DES_pid = this->process_info.dwProcessId;

  /* Now, the DES process has just been created. However, we need to remove all
  the startup messages from DES that are allocated into the STDOUT read pipe. We
  read from this pipe using the ReadFile function (fileapi.h), and it writes it
  into a buffer. When the buffer contains "DES>", the startup messages will have
  ended. */

  CHAR buffer[BUFFER_SIZE];  // standard size
  DWORD bytes_read;

  bool finished_reading = false;
  std::string complete_reading_str = "";
  while (!finished_reading) {
    BOOL success =
        ReadFile(this->driver_to_des_out_rpipe, buffer,
                 sizeof(buffer) - sizeof(CHAR), &bytes_read,
                 NULL); /* we don't write the final CHAR so as to put the '\0'
                           char, like we will see.
                           the final argument must be not null only when
                           the pipe was created with overlapping */
    if (!success) {
      return this->set_win_error("Failed to read from DES output pipe", true);
    }

    buffer[bytes_read] =
        '/0';  // we need it to be zero-terminated, as advised by a warning when
               // tring to assign the char[] to a new std::string
    std::string buffer_str = buffer;

    complete_reading_str += buffer_str;

    finished_reading = (bytes_read < BUFFER_SIZE - 1);
  }

  this->shmem->des_process_created = true;
  return SQL_SUCCESS;
}

#else
/* DESODBC:
  This function creates the DES global process in
  Unix-like systems.

  Original author: DESODBC Developer
*/
SQLRETURN DBC::create_DES_process(const char *des_exec_path,
                                const char *des_working_dir) {
  SQLRETURN ret = SQL_SUCCESS;

  pid_t pid = fork();
  if (pid == -1) {
    return this->set_unix_error("Failed to fork to open DES", true);
  } else if (pid == 0) {
    this->driver_to_des_in_rpipe = open(IN_WPIPE_NAME, O_RDONLY);
    if (this->driver_to_des_in_rpipe == -1) {
      return this->set_unix_error("Failed to open DES input pipe", true);
    }

    this->driver_to_des_out_wpipe = open(OUT_RPIPE_NAME, O_WRONLY);
    if (this->driver_to_des_out_wpipe == -1) {
      ::close(this->driver_to_des_in_rpipe);
      return this->set_unix_error("Failed to open DES output pipe", true);
    }

    if (dup2(this->driver_to_des_in_rpipe, STDIN_FILENO) == -1) {
      ::close(this->driver_to_des_in_rpipe);
      ::close(this->driver_to_des_out_wpipe);
      return this->set_unix_error("Failed to dup2 on DES input pipe", true);
    }

    if (dup2(this->driver_to_des_out_wpipe, STDOUT_FILENO) == -1) {
      ::close(this->driver_to_des_in_rpipe);
      ::close(this->driver_to_des_out_wpipe);
      return this->set_unix_error("Failed to dup2 on DES output pipe", true);
    }

    this->shmem->DES_pid = getpid();

    if (chdir(des_working_dir) == -1) {
      ::close(this->driver_to_des_in_rpipe);
      ::close(this->driver_to_des_out_wpipe);
      return this->set_unix_error("Failed to change dir on DES_WORKING_DIR",
                                  true);
    }

    if (execlp(des_exec_path, des_exec_path, nullptr) == -1) {
      ::close(this->driver_to_des_in_wpipe);
      ::close(this->driver_to_des_out_rpipe);
      return this->set_unix_error("Failed to launch DES", true);
    }
    return SQL_ERROR;
  } else {
    this->shmem->des_process_created = true;

    ret = release_shared_memory_mutex();
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return ret;

    ret = get_DES_process_pipes();
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return ret;

    char buffer[4096];
    ssize_t bytes_read = 0;

    bool finished_reading = false;
    std::string complete_reading_str = "";
    while (!finished_reading) {
      int ms = 0;
      while (ms < MAX_OUTPUT_WAIT_MS && bytes_read == 0) {
        usleep(10000);
        ioctl(this->driver_to_des_out_rpipe, FIONREAD, &bytes_read);
        ms += 10;
      }
      if (bytes_read > 0) {
        bytes_read =
            read(this->driver_to_des_out_rpipe, buffer, sizeof(buffer));
        if (bytes_read == -1) {
          return this->set_unix_error("Failed to read DES output pipe", true);
        }
        buffer[bytes_read] =
            '/0';  // we need it to be zero-terminated, as advised by a warning
                   // when tring to assign the char[] to a new std::string
        std::string buffer_str = buffer;

        complete_reading_str += buffer_str;

        finished_reading = (bytes_read < BUFFER_SIZE - 1);
      } else
        finished_reading = true;
    }
    this->shmem->des_process_created = true;
  }

  return SQL_SUCCESS;
}
#endif

/* DESODBC:
  This function gets the input/output
  pipes from the already launched global DES process.

  Original author: DESODBC Developer
*/
SQLRETURN DBC::get_DES_process_pipes() {
#ifdef _WIN32
  bool finished = false;
  SQLRETURN ret;
  ret = get_shared_memory_mutex();
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return ret;
  ret = getRequestHandleMutex();
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
    release_shared_memory_mutex();
    return ret;
  }

  this->shmem->handle_sharing_info.handle_petitioner.pid =
      GetCurrentProcessId();
  this->shmem->handle_sharing_info.handle_petitioner.id = this->connection_id;

  while (!finished) {
    size_t random_id = -1;
    if (this->shmem->connected_clients_struct.size == 0) {
      releaseRequestHandleMutex();
      release_shared_memory_mutex();
      return this->set_error(
          "HY000",
          "Failed to get DES process pipes: no available clients to share "
          "pipes. Perhaps the sharers do have different privileges than you?");
    } else if (this->shmem->connected_clients_struct.size == 1)
      random_id = this->shmem->connected_clients_struct.connected_clients[0].id;
    else
      random_id =
          this->shmem->connected_clients_struct
              .connected_clients[rand() %
                                 (this->shmem->connected_clients_struct.size -
                                  1)]
              .id;  // we request the handles to a random peer
    this->shmem->handle_sharing_info.handle_petitionee.id = random_id;

    ret = release_shared_memory_mutex();
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
      releaseRequestHandleMutex();
      return ret;
    }

    ret = setRequestHandleEvent();
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
      releaseRequestHandleMutex();
      return ret;
    }
    DWORD handle_sent_event_signal =
        WaitForSingleObject(this->handle_sent_event, EVENT_TIMEOUT);

    ret = get_shared_memory_mutex();

    switch (handle_sent_event_signal) {
      case WAIT_ABANDONED:
      case WAIT_TIMEOUT:
        this->remove_client_from_shmem(
            this->shmem->connected_clients_struct,
            this->shmem->handle_sharing_info.handle_petitionee.id);
        continue;
      case WAIT_FAILED:
        release_shared_memory_mutex();
        releaseRequestHandleMutex();
        return this->set_win_error(
            "Failed to wait for event " + std::string(HANDLE_SENT_EVENT_NAME),
            true);
      default:
        break;
    }

    this->driver_to_des_out_rpipe = this->shmem->handle_sharing_info.out_handle;
    this->driver_to_des_in_wpipe = this->shmem->handle_sharing_info.in_handle;
    if (!this->driver_to_des_out_rpipe || !this->driver_to_des_in_wpipe) {
      int x;
    }

    // we reset the structure once we have saved the handles
    this->shmem->handle_sharing_info.in_handle = NULL;
    this->shmem->handle_sharing_info.out_handle = NULL;

    this->shmem->handle_sharing_info.handle_petitionee.id = 0;
    this->shmem->handle_sharing_info.handle_petitionee.pid = 0;
    this->shmem->handle_sharing_info.handle_petitioner.id = 0;
    this->shmem->handle_sharing_info.handle_petitioner.pid = 0;

    finished = true;
  }

  ret = releaseRequestHandleMutex();
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
    release_shared_memory_mutex();
    return ret;
  }
  ret = release_shared_memory_mutex();
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return ret;

#else

  timeval tv_start;
  gettimeofday(&tv_start, nullptr);

  bool success = false;

  while (!success) {
    this->driver_to_des_in_wpipe = open(IN_WPIPE_NAME, O_WRONLY | O_NONBLOCK);
    if (this->driver_to_des_in_wpipe != -1) {
      success = true;
    } else {
      timeval tv_end;
      gettimeofday(&tv_end, nullptr);
      long secs = tv_end.tv_sec - tv_start.tv_sec;
      if (secs >= MUTEX_TIMEOUT_SECONDS) {
        std::string msg = "Fetching pipe ";
        msg += std::string(IN_WPIPE_NAME);
        msg +=
            " timed-out. Perhaps a connection was closed unsafely. In that "
            "case, a manual removal of this named pipe may be required. If "
            "not, the "
            "problem must be in an old connection V shared memory segment that "
            "was not deleted. "
            "Try with ipcrm.";
        return this->set_unix_error(msg, false);
      } else
        usleep(100000);
    }
  }

  gettimeofday(&tv_start, nullptr);
  success = false;

  while (!success) {
    this->driver_to_des_out_rpipe = open(OUT_RPIPE_NAME, O_RDONLY | O_NONBLOCK);
    if (this->driver_to_des_out_rpipe != -1) {
      success = true;
    } else {
      timeval tv_end;
      gettimeofday(&tv_end, nullptr);
      long secs = tv_end.tv_sec - tv_start.tv_sec;
      if (secs >= MUTEX_TIMEOUT_SECONDS) {
        std::string msg = "Fetching pipe ";
        msg += std::string(OUT_RPIPE_NAME);
        msg +=
            " timed-out. Perhaps a connection was closed unsafely. In that "
            "case, a manual removal of this named pipe may be required. If "
            "not, the "
            "problem must be in an old connection V shared memory segment that "
            "was not deleted. "
            "Try with ipcrm.";
        return this->set_unix_error(msg, false);
      } else
        usleep(100000);
    }
  }

#endif

  return SQL_SUCCESS;
}

#ifdef _WIN32
/* DESODBC:
  This function shares the DES global process pipes
  in the context of the Windows version.

  Original author: DESODBC Developer
*/
void DBC::sharePipes() {
  DWORD ret;

  SharedMemoryWin *shmem = this->shmem;
  HANDLE handles[] = {this->request_handle_event, this->finishing_event};

  while (true) {
    DWORD wait_event = WaitForMultipleObjects(
        (sizeof(handles) / sizeof(handles[0])), handles, FALSE, INFINITE);

    if (wait_event == WAIT_FAILED) {
      break;
    }

    if (wait_event == WAIT_OBJECT_0 + 0 &&
        shmem->handle_sharing_info.handle_petitioner.pid != 0 &&
        shmem->handle_sharing_info.handle_petitionee.id ==
            this->connection_id) {
      HANDLE petitioner_process_handle =
          OpenProcess(PROCESS_DUP_HANDLE, TRUE,
                      shmem->handle_sharing_info.handle_petitioner.pid);

      if (petitioner_process_handle == NULL) break;

      ret = DuplicateHandle(GetCurrentProcess(), this->driver_to_des_out_rpipe,
                            petitioner_process_handle,
                            &shmem->handle_sharing_info.out_handle, 0, TRUE,
                            DUPLICATE_SAME_ACCESS);
      if (ret == 0) break;

      ret = DuplicateHandle(GetCurrentProcess(), this->driver_to_des_in_wpipe,
                            petitioner_process_handle,
                            &shmem->handle_sharing_info.in_handle, 0, TRUE,
                            DUPLICATE_SAME_ACCESS);
      if (ret == 0) break;

      ResetEvent(this->request_handle_event);
      SetEvent(this->handle_sent_event);  // we notify the petitioner
    } else if (wait_event ==
               WAIT_OBJECT_0 + 1) {  // i.e. "finishing" has been signaled
      if (this->shmem->handle_sharing_info.handle_petitioner.id ==
          this->connection_id)
        break;
    }
  }
}

#endif

#ifdef _WIN32
/* DESODBC:
  This function gets a generic mutex
  in Windows.

  Original author: DESODBC Developer
*/
SQLRETURN DBC::get_mutex(HANDLE h, const std::string &name) {
  DWORD ret = WaitForSingleObject(h, MUTEX_TIMEOUT);
  if (ret == WAIT_TIMEOUT) {
    return this->set_win_error("Mutex " + std::string(name) + " time-outed",
                               false);
  } else if (ret == WAIT_FAILED) {
    return this->set_win_error(
        "Fetching mutex " + std::string(name) + " failed", true);
  } else if (ret == WAIT_ABANDONED) {
    return this->set_win_error(
        "Mutex " + std::string(name) + " in non-consistent state", false);
  }
  return SQL_SUCCESS;
}

/* DESODBC:
  This function releases a generic
  mutex in Windows.

  Original author: DESODBC Developer
*/
SQLRETURN DBC::release_mutex(HANDLE h, const std::string &name) {
  DWORD ret = ReleaseMutex(h);
  if (ret == 0) {
    return this->set_win_error("Failed to release mutex " + std::string(name),
                               true);
  }
  return SQL_SUCCESS;
}
#else
/* DESODBC:
  This function gets a generic mutex
  in Unix-like systems.

  Original author: DESODBC Developer
*/
SQLRETURN DBC::get_mutex(sem_t *s, const std::string &name) {
  timeval tv_start;
  gettimeofday(&tv_start, nullptr);

  bool success = false;

  while (!success) {
    if (sem_trywait(s) == 0) {
      success = true;
    } else {
      timeval tv_end;
      gettimeofday(&tv_end, nullptr);
      long secs = tv_end.tv_sec - tv_start.tv_sec;
      if (secs >= MUTEX_TIMEOUT_SECONDS) {
        std::string msg = "Fetching mutex ";
        msg += std::string(name);
        msg +=
            " timed-out. Perhaps a connection was closed unsafely. In that "
            "case, a manual removal of this mutex may be required (try ";

#ifdef __APPLE__
        msg +=
            "with unlink (2). In macOS, mutexes do not exist in the "
            "filesystem)";
#else
        msg += "removing the corresponding V shared memory segment with ipcrm)";
#endif
        return this->set_unix_error(msg, false);
      } else
        usleep(100000);
    }
  }
  return SQL_SUCCESS;
}


/* DESODBC:
  This function releases a generic
  mutex in Windows.

  Original author: DESODBC Developer
*/
SQLRETURN DBC::release_mutex(sem_t *s, const std::string &name) {
  if (sem_post(s) == -1) {
    return this->set_unix_error("Failed to release mutex " + std::string(name),
                                true);
  }
  return SQL_SUCCESS;
}
#endif

#ifdef _WIN32

/* DESODBC:
  This function gets the request handle mutex,
  which is only available in the Windows version.

  Original author: DESODBC Developer
*/
SQLRETURN DBC::getRequestHandleMutex() {
  return get_mutex(this->request_handle_mutex, REQUEST_HANDLE_MUTEX_NAME);
}

/* DESODBC:
  This function releases the request handle mutex,
  which is only available in the Windows version.

  Original author: DESODBC Developer
*/
SQLRETURN DBC::releaseRequestHandleMutex() {
  return release_mutex(this->request_handle_mutex, REQUEST_HANDLE_MUTEX_NAME);
}

/* DESODBC:
  This function sets a generic event in
  the context of Windows' event objects.

  Original author: DESODBC Developer
*/
SQLRETURN DBC::setEvent(HANDLE h, const std::string &name) {
  DWORD ret = SetEvent(h);
  if (ret == 0) {
    this->set_win_error("Failed to set event " + std::string(name), true);
  }
  return SQL_SUCCESS;
}

/* DESODBC:
  This function sets a finishing event in
  the context of Windows' event objects.

  Original author: DESODBC Developer
*/
SQLRETURN DBC::setFinishingEvent() {
  return setEvent(this->finishing_event, FINISHING_EVENT_NAME);
}

/* DESODBC:
  This function sets a request handle event in
  the context of Windows' event objects.

  Original author: DESODBC Developer
*/
SQLRETURN DBC::setRequestHandleEvent() {
  return setEvent(this->request_handle_event, REQUEST_HANDLE_EVENT_NAME);
}

/* DESODBC:
  This function removes a client from
  the shared memory file, which is a necessary
  step in the Windows version.

  Original author: DESODBC Developer
*/
void DBC::remove_client_from_shmem(ConnectedClients &pids, size_t id) {
  int index = 0;
  while (index < pids.size) {
    if (pids.connected_clients[index].id == id)
      break;
    else
      index++;
  }
  if (index == pids.size) return;

  pids.connected_clients[index].id = 0;
  pids.connected_clients[index].pid = 0;

  int i = index;

  while (i + 1 < pids.size) {
    pids.connected_clients[i] = pids.connected_clients[i + 1];
    i++;
  }

  pids.size -= 1;
}
#endif


/* DESODBC:
  This function gets the shared memory
  mutex.

  Original author: DESODBC Developer
*/
SQLRETURN DBC::get_shared_memory_mutex() {
#ifdef _WIN32
  return get_mutex(this->shared_memory_mutex, SHARED_MEMORY_MUTEX_NAME);
#else
#ifdef __APPLE__
  return get_mutex(this->shared_memory_mutex, SHARED_MEMORY_MUTEX_NAME);
#else
  return get_mutex(&this->shmem->shared_memory_mutex, "shared memory");
#endif
#endif
}

/* DESODBC:
  This function releases the shared
  memory mutex.

  Original author: DESODBC Developer
*/
SQLRETURN DBC::release_shared_memory_mutex() {
#ifdef _WIN32
  return release_mutex(this->shared_memory_mutex, SHARED_MEMORY_MUTEX_NAME);
#else
#ifdef __APPLE__
  return release_mutex(this->shared_memory_mutex, SHARED_MEMORY_MUTEX_NAME);
#else
  return release_mutex(&this->shmem->shared_memory_mutex, "shared memory");
#endif
#endif
}

/* DESODBC:
  This function gets the query mutex.

  Original author: DESODBC Developer
*/
SQLRETURN DBC::get_query_mutex() {
#ifdef _WIN32
  return get_mutex(this->query_mutex, QUERY_MUTEX_NAME);
#else
#ifdef __APPLE__
  return get_mutex(this->query_mutex, QUERY_MUTEX_NAME);
#else
  return get_mutex(&this->shmem->query_mutex, "query");
#endif
#endif
}

/* DESODBC:
  This function releases the query mutex.

  Original author: DESODBC Developer
*/
SQLRETURN DBC::release_query_mutex() {
#ifdef _WIN32
  return release_mutex(this->query_mutex, QUERY_MUTEX_NAME);
#else
#ifdef __APPLE__
  return release_mutex(this->query_mutex, QUERY_MUTEX_NAME);
#else
  return release_mutex(&this->shmem->query_mutex, "query");
#endif
#endif
}

/* DESODBC:
  This function modifies the working directory removing
  the last '\' or '/' delimiter character if so.
  (char version)

  Original author: DESODBC Developer
*/
const char *prepare_working_dir(const char *working_dir) {
  std::string working_dir_str(working_dir);
  size_t pos;
  if (working_dir_str.find("\\") != std::wstring::npos)
    pos = working_dir_str.find_last_of("\\");
  else
    pos = working_dir_str.find_last_of("/");

  if (pos != std::string::npos && pos == (working_dir_str.size() - 1)) {
    std::string dir_str = working_dir_str.substr(0, pos);
    size_t dir_size = dir_str.size();
    char *dir = string_to_char_pointer(dir_str);

    return dir;

  } else {
    return working_dir;
  }
}

/* DESODBC:
  This function modifies the working directory removing
  the last '\' or '/' delimiter character if so.
  (wchar version)

  Original author: DESODBC Developer
*/
const wchar_t *prepare_working_dir(const wchar_t *working_dir) {
  std::wstring working_dir_wstr(working_dir);
  size_t pos;
  if (working_dir_wstr.find(L"\\") != std::wstring::npos)
    pos = working_dir_wstr.find_last_of(L"\\");
  else
    pos = working_dir_wstr.find_last_of(L"/");

  if (pos != std::wstring::npos && pos == (working_dir_wstr.size() - 1)) {
    std::wstring dir_wstr = working_dir_wstr.substr(0, pos);
    size_t dir_size = dir_wstr.size();

    wchar_t *dir = string_to_wchar_pointer(dir_wstr);

    return dir;

  } else {
    return working_dir;
  }
}

/* DESODBC:
  This function initializes the IPC objects.

  Original author: DESODBC Developer
*/
SQLRETURN DBC::initialize() {
#ifdef _WIN32
  HANDLE handle_map_file_shmem =
      CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
                         sizeof(SharedMemoryWin), SHARED_MEMORY_NAME);

  if (handle_map_file_shmem == nullptr) {
    return this->set_win_error("Failed to create/access shared memory file " +
                                   std::string(SHARED_MEMORY_NAME),
                               true);
  }

  shmem = (SharedMemoryWin *)MapViewOfFile(handle_map_file_shmem,
                                           FILE_MAP_ALL_ACCESS, 0, 0,
                                           sizeof(SharedMemoryWin));
  if (shmem == nullptr) {
    return this->set_win_error("Failed to map view of shared memory file " +
                                   std::string(SHARED_MEMORY_NAME),
                               true);
  }

  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle = TRUE;

  this->query_mutex = CreateMutexExA(&sa, QUERY_MUTEX_NAME, 0, SYNCHRONIZE);
  if (this->query_mutex == nullptr) {
    return this->set_win_error(
        "Failed to create mutex " + std::string(QUERY_MUTEX_NAME), true);
  }

  this->shared_memory_mutex =
      CreateMutexExA(&sa, SHARED_MEMORY_MUTEX_NAME, 0, SYNCHRONIZE);
  if (this->shared_memory_mutex == nullptr) {
    return this->set_win_error(
        "Failed to create mutex " + std::string(SHARED_MEMORY_MUTEX_NAME),
        true);
  }

  this->request_handle_mutex =
      CreateMutexExA(&sa, REQUEST_HANDLE_MUTEX_NAME, 0, SYNCHRONIZE);
  if (this->request_handle_mutex == nullptr) {
    return this->set_win_error(
        "Failed to create mutex " + std::string(REQUEST_HANDLE_MUTEX_NAME),
        true);
  }

  this->request_handle_event =
      CreateEventA(NULL, TRUE, FALSE, REQUEST_HANDLE_EVENT_NAME);
  if (this->request_handle_event == nullptr) {
    return this->set_win_error(
        "Failed to create event " + std::string(REQUEST_HANDLE_EVENT_NAME),
        true);
  }

  this->handle_sent_event =
      CreateEventA(NULL, TRUE, FALSE, HANDLE_SENT_EVENT_NAME);
  if (this->handle_sent_event == nullptr) {
    return this->set_win_error(
        "Failed to create event " + std::string(HANDLE_SENT_EVENT_NAME), true);
  }

  this->finishing_event = CreateEventA(NULL, TRUE, FALSE, FINISHING_EVENT_NAME);
  if (this->finishing_event == nullptr) {
    return this->set_win_error(
        "Failed to create event " + std::string(FINISHING_EVENT_NAME), true);
  }

  SQLRETURN ret = get_shared_memory_mutex();
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return ret;
#else
  this->shm_id = shmget(this->connection_hash_int, sizeof(SharedMemoryUnix),
                        0666 | IPC_CREAT);
  if (this->shm_id == -1) {
    return this->set_unix_error("Failed to create shared memory with key" +
                                    std::to_string(this->connection_hash_int),
                                true);
  }

  void *ptr = shmat(this->shm_id, nullptr, 0);
  if (ptr == (void *)-1) {
    return this->set_unix_error(
        "Failed to call shmat on shared memory with key" +
            std::to_string(this->connection_hash_int),
        true);
    perror("shmat");
    exit(1);
  }

  this->shmem = static_cast<SharedMemoryUnix *>(ptr);

#ifdef __APPLE__
  this->shared_memory_mutex =
      sem_open(SHARED_MEMORY_MUTEX_NAME, O_CREAT, S_IRWXU, 1);

  if (this->shared_memory_mutex == SEM_FAILED) {
    std::string msg = "Failed to create or open mutex ";
    msg += SHARED_MEMORY_MUTEX_NAME;
    return this->set_unix_error(msg, true);
  }

  this->query_mutex = sem_open(QUERY_MUTEX_NAME, O_CREAT, S_IRWXU, 1);

  if (this->query_mutex == SEM_FAILED) {
    std::string msg = "Failed to create or open mutex ";
    msg += QUERY_MUTEX_NAME;
    return this->set_unix_error(msg, true);
  }
#else
  if (!this->shmem->des_process_created) {
    if (sem_init(&this->shmem->shared_memory_mutex, 1, 1) == -1)
      return this->set_unix_error("Failed to create shared memory mutex", true);
    if (sem_init(&this->shmem->query_mutex, 1, 1) == -1)
      return this->set_unix_error("Failed to create query mutex", true);
  }
#endif

  SQLRETURN ret = get_shared_memory_mutex();
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return ret;

#endif

  return SQL_SUCCESS;
}

/* DESODBC:
  We have almost completely rewritten this
  function from MyODBC so as to adapt it to the DESODBC's needs.

  Original author: MyODBC
  Modified by: DESODBC Developer
*/
/**
Try to establish a connection to a MySQL server based on the data source
configuration.

@param[in]  ds   Data source information

@return Standard SQLRETURN code. If it is @c SQL_SUCCESS or @c
SQL_SUCCESS_WITH_INFO, a connection has been established.
*/
SQLRETURN DBC::connect(DataSource *dsrc) {
  SQLRETURN rc = SQL_SUCCESS;

  this->cxn_charset_info = desodbc::get_charset(48, MYF(0));  // DESODBC: latin1

#ifdef _WIN32

  /* DESODBC:
  We now save the path to the DES executable in Unicode format (wchars). It
  reads the DES executable field from the Data Source (loaded previously with
  data from the Windows Registries).*/

  if (!dsrc->opt_DES_EXEC || !dsrc->opt_DES_WORKING_DIR) return SQL_NEED_DATA;

  SQLWCHAR *des_exec_path = const_cast<SQLWCHAR *>(
      static_cast<const SQLWSTRING &>(dsrc->opt_DES_EXEC).c_str());
  SQLWCHAR *des_working_dir = const_cast<SQLWCHAR *>(
      static_cast<const SQLWSTRING &>(dsrc->opt_DES_WORKING_DIR).c_str());
  const wchar_t *prepared_working_dir = prepare_working_dir(des_working_dir);

#else
  const char *des_exec_path = static_cast<const char *>(dsrc->opt_DES_EXEC);
  const char *des_working_dir =
      static_cast<const char *>(dsrc->opt_DES_WORKING_DIR);
  const char *prepared_working_dir = prepare_working_dir(des_working_dir);
#endif

  this->get_concurrent_objects(des_exec_path, des_working_dir);

  rc = this->initialize();
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return rc;

#ifdef _WIN32
  SharedMemoryWin *shmem = this->shmem;
#else
  SharedMemoryUnix *shmem = this->shmem;
#endif
  if (shmem->exec_hash_int != 0 &&
      shmem->exec_hash_int != this->exec_hash_int) {
    release_shared_memory_mutex();
    return this->set_error(
        "HY000",
        "Trying to access a DES global process launched from an executable "
        "different from the one specified");
  }
#ifdef _WIN32
  if (shmem->connected_clients_struct.size == MAX_CLIENTS) {
    release_shared_memory_mutex();
    return this->set_error("HY000",
                           "Cannot connect. Maximum number of clients reached");
  }
#endif
    

  if (!this->shmem->des_process_created) {
    rc = this->createPipes();
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return rc;
#ifdef _WIN32
    rc = this->create_DES_process(des_exec_path,
                                &std::wstring(prepared_working_dir)[0]);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return rc;
#else
    rc = this->create_DES_process(des_exec_path, prepared_working_dir);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return rc;
    shmem->n_clients = 1;
#endif

    shmem->exec_hash_int = this->exec_hash_int;
  } else {
    rc = get_DES_process_pipes();
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return rc;
#ifndef _WIN32
    shmem->n_clients += 1;
#endif
  }

#ifdef _WIN32
  this->share_pipes_thread =
      std::unique_ptr<std::thread>(new std::thread(&DBC::sharePipes, this));
  if (!this->share_pipes_thread) throw std::bad_alloc();

  Client client;
  client.id = this->connection_id;
  client.pid = GetCurrentProcessId();
  shmem->connected_clients_struct
      .connected_clients[shmem->connected_clients_struct.size] = client;

  shmem->connected_clients_struct.size += 1;

  rc = release_shared_memory_mutex();
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return rc;

#else

  rc = release_shared_memory_mutex();
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return rc;

#endif
  this->connected = true;
  return SQL_SUCCESS;
}

/* DESODBC:
  This function corresponds to the
  implementation of SQLConnect, which was
  MySQLConnect in MyODBC. We have reused
  almost everything.

  Original author: MyODBC
  Modified by: DESODBC Developer
*/
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
SQLRETURN SQL_API DES_SQLConnect(SQLHDBC hdbc, SQLWCHAR *szDSN,
                                 SQLSMALLINT cbDSN, SQLWCHAR *szUID,
                                 SQLSMALLINT cbUID, SQLWCHAR *szAuth,
                                 SQLSMALLINT cbAuth) {
  SQLRETURN rc;
  DBC *dbc = (DBC *)hdbc;
  DataSource ds;

#ifdef NO_DRIVERMANAGER
  return ((DBC *)dbc)
      ->set_error("HY000", "SQLConnect requires DSN and driver manager", 0);
#else

  /* Can't connect if we're already connected. */
  if (is_connected(dbc))
    return ((DBC *)hdbc)->set_error("08002", "Connection name in use");

  /* Reset error state */
  CLEAR_DBC_ERROR(dbc);

  ds.opt_DSN.set_remove_brackets(szDSN, cbDSN);
  ds.lookup();

  try {
    rc = dbc->connect(&ds);
  } catch (const std::bad_alloc &e) {
    return dbc->set_error("HY001", "Memory allocation error");
  }

  if (SQL_SUCCEEDED(rc) && (szUID || szAuth)) {
    dbc->set_error(
        "01000",
        "The user/password provided was ignored (DES doesn't need it).");
    rc = SQL_SUCCESS_WITH_INFO;
  }

  return rc;
#endif
}

/* DESODBC:
  This function tries to close all the
  active connections when destroying this
  driver instance. The goal is to
  have a safe close of the DESODBC's IPC
  objects.

  Original author: DESODBC Developer
*/
void safe_close_connections() {
  for (auto hdbc : active_dbcs_global_var) {
    hdbc->close();
  }
}

/* DESODBC:
  This function automatically calls
  safe_close_connections when destroying this
  driver instance.

  Original author: DESODBC Developer
*/
__attribute__((constructor)) void init() { atexit(safe_close_connections); }

/* DESODBC:
  This function corresponds to the
  implementation of SQLDriverConnect, which was
  MySQLDriverConnect in MyODBC. We have reused
  almost everything.

  Original author: MyODBC
  Modified by: DESODBC Developer
*/
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
SQLRETURN SQL_API DES_SQLDriverConnect(
    SQLHDBC hdbc, SQLHWND hwnd, SQLWCHAR *szConnStrIn, SQLSMALLINT cbConnStrIn,
    SQLWCHAR *szConnStrOut, SQLSMALLINT cbConnStrOutMax,
    SQLSMALLINT *pcbConnStrOut, SQLUSMALLINT fDriverCompletion) {
  SQLRETURN rc = SQL_SUCCESS;
  DBC *dbc = (DBC *)hdbc;
  DataSource ds;
  /* We may have to read driver info to find the setup library. */
  Driver driver;
  /* We never know how many new parameters might come out of the prompt */
  SQLWCHAR prompt_outstr[4096];
  BOOL bPrompt = FALSE;
  HMODULE hModule = NULL;
  SQLWSTRING conn_str_in, conn_str_out;
  SQLWSTRING prompt_instr;

  if (cbConnStrIn != SQL_NTS)
    conn_str_in = SQLWSTRING(szConnStrIn, cbConnStrIn);
  else
    conn_str_in = szConnStrIn;

  /* Parse the incoming string */
  if (ds.from_kvpair(conn_str_in.c_str(), (SQLWCHAR)';')) {
    rc =
        dbc->set_error("HY000", "Failed to parse the incoming connect string.");
    goto error;
  }

#ifndef NO_DRIVERMANAGER
  /*
   If the connection string contains the DSN keyword, the driver retrieves
   the information for the specified data source (and merges it into the
   connection info with the provided connection info having precedence).

   This also allows us to get pszDRIVER (if not already given).
  */
  if (ds.opt_DSN) {
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
  if (ds.opt_NO_PROMPT) fDriverCompletion = SQL_DRIVER_NOPROMPT;

  /*
    We only prompt if we need to.

    A not-so-obvious gray area is when SQL_DRIVER_COMPLETE or
    SQL_DRIVER_COMPLETE_REQUIRED was specified along without a server or
    password - particularly password. These can work with defaults/blank but
    many callers expect prompting when these are blank. So we compromise; we
    try to connect and if it works we say its ok otherwise we go to
    a prompt.
  */
  switch (fDriverCompletion) {
    case SQL_DRIVER_PROMPT:
      bPrompt = TRUE;
      break;

    case SQL_DRIVER_COMPLETE:
    case SQL_DRIVER_COMPLETE_REQUIRED:
      try {
        rc = dbc->connect(&ds);
      } catch (const std::bad_alloc &e) {
        return dbc->set_error("HY001", "Memory allocation error");
      }

      if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
        goto connected;
      else if (rc == SQL_NEED_DATA)
        bPrompt = true;
      else
        goto error;
      break;

    case SQL_DRIVER_NOPROMPT:
      bPrompt = FALSE;
      break;

    default:
      rc = dbc->set_error("HY110", "Invalid driver completion.");
      goto error;
  }

#ifdef __APPLE__
  /*
    We don't support prompting on Mac OS X.
  */
  if (bPrompt) {
    rc = dbc->set_error("HY000",
                        "Prompting is not supported on this platform. "
                        "Please provide all required connect information.");
    goto error;
  }
#endif

  if (bPrompt) {
    PromptFunc pFunc;

    /*
     We can not present a prompt unless we can lookup the name of the setup
     library file name. This means we need a DRIVER. We try to look it up
     using a DSN (above) or, hopefully, get one in the connection string.




     This will, when we need to prompt, trump the ODBC rule where we can
     connect with a DSN which does not exist. A possible solution would be to
     hard-code some fall-back value for ds->pszDRIVER.
    */
    if (!ds.opt_DRIVER) {
      char szError[1024];
      sprintf(szError,
              "Could not determine the driver name; "
              "could not lookup setup library. DSN=(%s)\n",
              (const char *)ds.opt_DSN);
      rc = dbc->set_error("HY000", szError);
      goto error;
    }

#ifndef NO_DRIVERMANAGER
    /* We can not present a prompt if we have a null window handle. */
    if (!hwnd) {
      rc = dbc->set_error("IM008", "Invalid window handle");
      goto error;
    }

    /* if given a named DSN, we will have the path in the DRIVER field */
    if (ds.opt_DSN) driver.lib = ds.opt_DRIVER;
    /* otherwise, it's the driver name */
    else
      driver.name = ds.opt_DRIVER;

    if (driver.lookup()) {
      char sz[1024];
      sprintf(sz, "Could not find driver '%s' in system information.",
              (const char *)ds.opt_DRIVER);

      rc = dbc->set_error("IM003", sz);
      goto error;
    }

    if (!driver.setup_lib)
#endif
    {
      rc = dbc->set_error(
          "HY000", "Could not determine the file name of setup library.");
      goto error;
    }

    /*
     We dynamically load the setup library so the driver itself does not
     depend on GUI libraries.
    */

    if (!(hModule = LoadLibrary(driver.setup_lib))) {
      char sz[1024];
      sprintf(sz, "Could not load the setup library '%s'.",
              (const char *)driver.setup_lib);
      rc = dbc->set_error("HY000", sz);
      goto error;
    }

    pFunc = (PromptFunc)GetProcAddress(hModule, "Driver_Prompt");

    if (pFunc == NULL) {
#ifdef WIN32
      LPVOID pszMsg;

      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL, GetLastError(),
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&pszMsg,
                    0, NULL);
      rc = dbc->set_error("HY000", (char *)pszMsg);
      LocalFree(pszMsg);
#else
      rc = dbc->set_error("HY000", dlerror());
#endif
      goto error;
    }

    /* create a string for prompting, and add driver manually */
    prompt_instr = ds.to_kvpair(';');
    prompt_instr.append(W_DRIVER_PARAM);
    SQLWSTRING drv = (const SQLWSTRING &)ds.opt_DRIVER;
    prompt_instr.append(drv);

    /*
      In case the client app did not provide the out string we use our
      inner buffer prompt_outstr
    */
    if (!pFunc(hwnd, (SQLWCHAR *)prompt_instr.c_str(), fDriverCompletion,
               prompt_outstr, sizeof(prompt_outstr), pcbConnStrOut,
               (SQLSMALLINT)dbc->unicode)) {
      dbc->set_error("HY000", "User cancelled.");
      rc = SQL_NO_DATA;
      goto error;
    }

    /* refresh our DataSource */
    ds.reset();
    if (ds.from_kvpair(prompt_outstr, ';')) {
      rc = dbc->set_error("HY000", "Failed to parse the prompt output string.");
      goto error;
    }

    /*
      We don't need prompt_outstr after the new DataSource is created.
      Copy its contents into szConnStrOut if possible
    */
    if (szConnStrOut) {
      *pcbConnStrOut =
          (SQLSMALLINT)desodbc_min(cbConnStrOutMax, *pcbConnStrOut);
      memcpy(szConnStrOut, prompt_outstr,
             (size_t)*pcbConnStrOut * sizeof(SQLWCHAR));
      /* term needed if possibly truncated */
      szConnStrOut[*pcbConnStrOut - 1] = 0;
    }
  }

  rc = dbc->connect(&ds);

  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    goto error;
  }

  if (ds.opt_SAVEFILE) {
    /* We must disconnect if File DSN is created */
    dbc->close();
  }

connected:

  /* copy input to output if connected without prompting */
  if (!bPrompt) {
    size_t copylen;
    conn_str_out = conn_str_in;
    if (ds.opt_SAVEFILE) {
      SQLWSTRING pwd_temp = (const SQLWSTRING &)ds.opt_PWD;

      /* make sure the password does not go into the output buffer */
      ds.opt_PWD = nullptr;

      conn_str_out = ds.to_kvpair(';');

      /* restore old values */
      ds.opt_PWD = pwd_temp;
    }

    size_t inlen = conn_str_out.length();
    copylen =
        desodbc_min((size_t)cbConnStrOutMax, inlen + 1) * sizeof(SQLWCHAR);

    if (szConnStrOut && copylen) {
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
    if (pcbConnStrOut) *pcbConnStrOut = (SQLSMALLINT)inlen;
  }

  /* return SQL_SUCCESS_WITH_INFO if truncated output string */
  if (pcbConnStrOut && cbConnStrOutMax &&
      cbConnStrOutMax - sizeof(SQLWCHAR) <= *pcbConnStrOut * sizeof(SQLWCHAR)) {
    dbc->set_error("01004", "String data, right truncated");
    rc = SQL_SUCCESS_WITH_INFO;
  }

error:

  if (hModule) FreeLibrary(hModule);

  return rc;
}

void DBC::free_connection_stmts() {
  for (auto it = stmt_list.begin(); it != stmt_list.end();) {
    STMT *stmt = *it;
    it = stmt_list.erase(it);
    DES_SQLFreeStmt((SQLHSTMT)stmt, SQL_DROP);
  }
  stmt_list.clear();
}

/* DESODBC:
  This function has been almost completely reused
  from MyODBC.

  Original author: MyODBC
  Modified by: DESODBC Developer
*/
/**
Disconnect a connection.

@param[in]  hdbc   Connection handle
@return  Standard ODBC return codes

@since ODBC 1.0
@since ISO SQL 92
*/
SQLRETURN SQL_API SQLDisconnect(SQLHDBC hdbc) {
  DBC *dbc = (DBC *)hdbc;

  CHECK_HANDLE(hdbc);

  dbc->free_connection_stmts();

  SQLRETURN ret = dbc->close();

  /* free allocated packet buffer */
  dbc->database.clear();
  return ret;
}
