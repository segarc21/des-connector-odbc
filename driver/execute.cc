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
//
// The authorship of each section of this source file (comments,
// functions and other symbols) belongs to MyODBC unless we
// explicitly state otherwise.
// ---------------------------------------------------------

/**
@file  execute.c
@brief Statement execution functions.
*/

#include <locale.h>
#include "driver.h"

const long long TIMEOUT = 1000;

char buffer[BUFFER_SIZE];
DWORD bytes_read = 0;

#ifdef _WIN32
DWORD WINAPI read_process(LPVOID lpParam) {
  HANDLE read_pipe = (HANDLE)lpParam;

  if (!ReadFile(read_pipe, buffer, sizeof(buffer) - sizeof(char), &bytes_read,
                NULL)) {
    DWORD err = GetLastError();
    if (err == ERROR_OPERATION_ABORTED) {
      return 0;  // forced abortion
    } else {
      return -1;  // real error
    }
  }

  buffer[bytes_read] = '\0';
  return 0;  // success
}
#endif

/* DESODBC:
  This function sends a query and buils the resulting
  DES_RESULT* structure.

  Original author: DESODBC Developer
*/
std::pair<SQLRETURN, DES_RESULT *> DBC::send_query_and_get_results(
    COMMAND_TYPE type, const std::string &query) {
  SQLRETURN ret = SQL_SUCCESS;
  DES_RESULT *res = nullptr;

  ret = this->getQueryMutex();
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return {ret, nullptr};

  auto pair = this->send_query_and_read(query);
  ret = pair.first;
  std::string tapi_output = pair.second;
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
    this->releaseQueryMutex();
    return {ret, nullptr};
  }

  ret = this->releaseQueryMutex();
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return {ret, nullptr};
  /*
   We do not have to worry to free memory, as it will be done automatically
   thanks to the DBC* attribute.
 */
  STMT *temp_stmt = new STMT(this);
  temp_stmt->type = type;
  temp_stmt->last_output = tapi_output;
  temp_stmt->result = get_result_metadata(temp_stmt);
  if (!temp_stmt->result) {
    return {SQL_ERROR, nullptr};
  } else {
    res = copy(temp_stmt->result);
  }

  return {SQL_SUCCESS, res};
}

/* DESODBC:
  This function sends a SELECT COUNT query and fetches
  the number in the resulting output.

  Original author: DESODBC Developer
*/
int STMT::send_select_count(std::string query) {
  int ret = -1;  // if error

  ret = this->dbc->getQueryMutex();
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return ret;

  auto pair = this->dbc->send_query_and_read(query);
  ret = pair.first;
  std::string tapi_output = pair.second;
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
    this->dbc->releaseQueryMutex();
    return ret;
  }

  ret = this->dbc->releaseQueryMutex();
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return ret;

  if (tapi_output.find("$error") != std::string::npos)
    return this->set_error("HY000", "Internal query error");

  std::vector<std::string> lines = getLines(tapi_output);

  ret = stoi(lines[4]);

  return ret;
}

/* DESODBC:
  This function sends a insert/update/delete query and acommodates
  the number of rows affected into its affected rows attribute.

  Original author: DESODBC Developer
*/
std::pair<SQLRETURN, std::string> STMT::send_update_and_fetch_info(
    std::string query) {
  SQLRETURN ret = SQL_SUCCESS;

  ret = this->dbc->getQueryMutex();
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return {ret, ""};

  auto pair = this->dbc->send_query_and_read(query);
  ret = pair.first;
  std::string tapi_output = pair.second;
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
    this->dbc->releaseQueryMutex();
    return {ret, tapi_output};
  }

  ret = this->dbc->releaseQueryMutex();
  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return {ret, ""};

  if (tapi_output.find("$error") != std::string::npos) {
    return {SQL_ERROR, tapi_output};
  } else {
    this->affected_rows = stoll(tapi_output);
    return {SQL_SUCCESS, tapi_output};
  }
  return {SQL_SUCCESS, tapi_output};
}

/* DESODBC:
  This function checks, given the output we have just read,
  whether we should stop or try to read more output.

  Original author: DESODBC Developer
*/
bool check_stop(const std::string &query, const std::string &tapi_output) {
  /*
    Note: doing something like "if bytes_read < BUFFER_SIZE - 1  --> true" when
    not calling /process, is not problematic. The read pipe ensure reading while
    there are no sensible time gaps between the printing characteres. For
    instance, executing /ls may have a enormous output; however, it will read it
    at once since there will not be any "pauses". However, /process is an
    exception to this rule. In between some parts of the output, it can wait the
    enough time necessary so that the pipe thinks that the output is already
    fetched. Fortunately, the /process output ensures a sentinel: that of "Info:
    Batch file processed.", "Unknown command or incorrect number of arguments.",
    "When processing file". Note: "$eot" is not a valid sentinel: it appears at
    the end when there was an error, but it appears early at the beginning when
    the command is correct, before all the processing.

    In /dbschema occurs the same when the ODBC application is DES itself,
    and connects to a DES process which connects to an external database.
  */

  bool is_process = is_in_string(query, "/process");
  bool is_dbschema = is_in_string(query, "/dbschema");

  if (!is_process && !is_dbschema && bytes_read < BUFFER_SIZE - 1) {
    return true;
  }

  if (is_process) {
    //The following messages will be fetched completely, as there are not sufficient delay
    //between reading each of these characters (the pipe reads them at once)
    if (is_in_string(tapi_output, "Info: Batch file processed.")) return true;
    if (is_in_string(tapi_output,
                     "Unknown command or incorrect number of arguments."))
      return true;
    if (is_in_string(tapi_output, "When processing file")) return true;
    /* We do not consider a default case. $eot is not a sentinel for some
     * commands: in some commands it doesn't appear, and in some others the
     * input doesn't end with $eot (check /ls). There are even some commands
     * that do not have a sentinel (i.e. /write $computation_time$).*/
  }

  if (is_dbschema) {
    if (is_in_string(tapi_output, "$eot")) return true;
  }

  return false;
}

#ifdef _WIN32
std::pair<SQLRETURN, std::string> DBC::read_DES_output_win(const std::string& query) {
  SQLRETURN err = SQL_SUCCESS;
  std::string tapi_output = "";
  bool finished_reading = false;

  bytes_read = 0;

  while (!finished_reading) {
    int ms = 0;
    while (ms < MAX_OUTPUT_WAIT_MS && bytes_read == 0) {
      HANDLE read_thread = CreateThread(NULL, 0, read_process,
                                        this->driver_to_des_out_rpipe, 0, NULL);
      if (!read_thread) {
        err = this->set_win_error("Failed to peek DES output", true);
        return {err, ""};
      }

      Sleep(10);
      ms += 10;

      //We will not handle errors for these because I/O is highly unpredictable.
      CancelSynchronousIo(read_thread); //if it is reading actual data, it will not interrupt the reading pipe.
      WaitForSingleObject(read_thread, MUTEX_TIMEOUT);

      DWORD code;
      if (GetExitCodeThread(read_thread, &code) != 0) {
        if (code == -1) {
          err = this->set_win_error("Failed to read DES output", true);
          return {err, ""};
        }
      }
      
    }
    if (bytes_read > 0) {
      std::string buffer_str = buffer;
      tapi_output += buffer_str;

      finished_reading = check_stop(query, tapi_output);
      bytes_read = 0;
    } else
      finished_reading = true;
  }
    return {err, tapi_output};
}
#else
std::pair<SQLRETURN, std::string> DBC::read_DES_output_unix(const std::string& query) {
  SQLRETURN err = SQL_SUCCESS;
  std::string tapi_output = "";
  bool finished_reading = false;
  bytes_read = 0;

  while (!finished_reading) {
    int ms = 0;
    while (ms < MAX_OUTPUT_WAIT_MS && bytes_read == 0) {
      usleep(10000);
      ms += 10;
      ioctl(this->driver_to_des_out_rpipe, FIONREAD, &bytes_read);
    }
    if (bytes_read > 0) {
      bytes_read = read(this->driver_to_des_out_rpipe, buffer, sizeof(buffer));
      if (bytes_read == -1) {
        return {this->set_unix_error("Error reading DES output pipe", true), ""};
      }
      buffer[bytes_read] = '\0';
      std::string buffer_str = buffer;
      tapi_output += buffer_str;

      finished_reading = check_stop(query, tapi_output);

      bytes_read = 0;

    } else
      finished_reading = true;
  }

  return {err, tapi_output};

}
#endif
/* DESODBC:
  This function sends a query and reads the output. It returns the output
  and whether there was a success or not.

  Original author: DESODBC Developer
*/
std::pair<SQLRETURN, std::string> DBC::send_query_and_read(
    const std::string &query) {
  int error = SQL_ERROR, native_error = 0;
  bool read_success = false;
  bool finished_reading = false;
  char *full_query_arr = nullptr;
  std::string tapi_output = "";
  std::string full_query = "";
  DWORD bytes_written;

  full_query = "/tapi " + query + '\n';  // query for the launched DES process

  // We convert the string to a char*.
  full_query_arr =
      new char[full_query.size() +
               sizeof(char)];  // we hold a final char for the delimiter '\0'
  std::copy(full_query.begin(), full_query.end(), full_query_arr);
  full_query_arr[full_query.size()] = '\0';

#ifdef _WIN32
  while (!this->driver_to_des_in_wpipe || !this->driver_to_des_out_rpipe)
    this->getDESProcessPipes();
  if (!WriteFile(this->driver_to_des_in_wpipe, full_query_arr,
                 strlen(full_query_arr), &bytes_written,
                 NULL)) {  // as we explained in the connection part,
                           // the final argument must be not null only when the
                           // pipe was created with overlapping
    error = this->set_win_error("Failed to send data to DES input", true);
    return {error, ""};
  }
#else
  if (write(this->driver_to_des_in_wpipe, full_query_arr,
            strlen(full_query_arr)) == -1) {
    perror("write");
  }
#endif

  delete[] full_query_arr;
  full_query_arr = nullptr;

  // If we send /q, we cannot read anything after that.
  if (query == "/q") return {SQL_SUCCESS, ""};

  /*
      Same considerations as those we took when reading the startup DES message.
      However, that output message had a fixed length and behavior. We introduce
      some new logic when treating a command output.
  */

  #ifdef _WIN32
  auto pair = this->read_DES_output_win(query);
  #else
  auto pair = this->read_DES_output_unix(query);
  #endif
  error = pair.first;
  tapi_output = pair.second;

  return {error, tapi_output};
}

/* DESODBC:
  This function gets the statement to build the result set
  based on the last output given (attribute of the STMT).

  Original author: DESODBC Developer
*/
SQLRETURN STMT::build_results() {
  if (!get_result_metadata(this)) {
    this->state = ST_EXECUTED;
    return SQL_SUCCESS;
  } else
    fix_result_types(this);

  return check_and_set_errors(SQL_HANDLE_STMT, this, this->last_output);
}

/* DESODBC:
  Function that executes a query into the DES executable (through its
  STDIN pipe), and loads its result into a internal table, structure held by the
  stmt.

  Original author: DESODBC Developer
*/
SQLRETURN DES_do_query(STMT *stmt, std::string query) {
  std::pair<SQLRETURN, std::string> pair = {SQL_ERROR, ""};
  SQLRETURN error = SQL_SUCCESS;
  std::string tapi_output = "";
  SQLRETURN release_mutex_err = SQL_SUCCESS;

  assert(stmt);
  LOCK_STMT_DEFER(stmt);

  if (query.empty()) {
    /* Probably error from insert_param */
    goto exit;
  }

  error = stmt->dbc->getQueryMutex();
  if (error != SQL_SUCCESS && error != SQL_SUCCESS_WITH_INFO) return error;

  switch (stmt->type) {
    case INSERT:
    case UPDATE:
    case DEL:
      pair = stmt->send_update_and_fetch_info(query);
      error = pair.first;
      tapi_output = pair.second;
      break;
    default:
      pair = stmt->dbc->send_query_and_read(query);
      error = pair.first;
      tapi_output = pair.second;
      break;
  }

  // We parse the TAPI output and create an internal table from the result view
  stmt->last_output = tapi_output;

  error = stmt->build_results();

  if (error != SQL_SUCCESS && error != SQL_SUCCESS_WITH_INFO) {
    stmt->dbc->releaseQueryMutex();
    return error;
  }

  release_mutex_err = stmt->dbc->releaseQueryMutex();
  if (release_mutex_err != SQL_SUCCESS &&
      release_mutex_err != SQL_SUCCESS_WITH_INFO)
    return error;

exit:
  /*
    If the original query was modified, we reset stmt->query so that the
    next execution re-starts with the original query.
  */
  if (GET_QUERY(&stmt->orig_query)) {
    stmt->query = stmt->orig_query;
    stmt->orig_query.reset(NULL, NULL, NULL);
  }
  return error;
}

/*
@type    : myodbc3 internal
@purpose : insert sql params at parameter positions
@param[in]      stmt        Statement
@param[in]      row         Parameters row
@param[in,out]  finalquery  if NULL, final query is not copied
@param[in,out]  length      Length of the query. Pointed value is used as
initial offset
@comment : it allocates and modifies finalquery (when finalquery!=NULL),
           so passing stmt->query->query can lead to memory leak.
*/

SQLRETURN insert_params(STMT *stmt, SQLULEN row, std::string &finalquery) {
  assert(stmt);
  const char *query = GET_QUERY(&stmt->query);
  uint i, length, had_info = 0;
  SQLRETURN rc = SQL_SUCCESS;

  LOCK_DBC(stmt->dbc);

  adjust_param_bind_array(stmt);

  for (i = 0; i < stmt->param_count; ++i) {
    DESCREC *aprec = desc_get_rec(stmt->apd, i, FALSE);
    DESCREC *iprec = desc_get_rec(stmt->ipd, i, FALSE);
    const char *pos;
    DES_BIND *bind;

    if (stmt->dummy_state != ST_DUMMY_PREPARED &&
        (!aprec || !aprec->par.real_param_done)) {
      rc = stmt->set_error("07001",
                           "The number of parameter markers is not equal "
                           "to the number of parameters provided");
      goto error;
    }

    assert(iprec);

    pos = stmt->query.get_param_pos(i);
    length = (uint)(pos - query);

    if (stmt->add_to_buffer(query, length) == NULL) {
      goto memerror;
    }

    query = pos + 1; /* Skip '?' */

    rc = insert_param(stmt, NULL, stmt->apd, aprec, iprec, row);

    if (!SQL_SUCCEEDED(rc)) {
      goto error;
    } else {
      if (rc == SQL_SUCCESS_WITH_INFO) {
        had_info = 1;
      }
    }
  }

  /* if any ofr parameters return SQL_SUCCESS_WITH_iNFO - returning it
     SQLSTATE corresponds to last SQL_SUCCESS_WITH_iNFO
  */
  if (had_info) {
    rc = SQL_SUCCESS_WITH_INFO;
  }

  length = (uint)(GET_QUERY_END(&stmt->query) - query);

  if (stmt->add_to_buffer(query, length) == NULL) {
    goto memerror;
  }

  finalquery = std::string(stmt->buf(), stmt->buf_pos());

  return rc;

memerror: /* Too much data */
  rc = stmt->set_error("HY001", "Memory allocation error");

error:
  return rc;
}

static void put_null_param(STMT *stmt, DES_BIND *bind) {
  stmt->add_to_buffer("NULL", 4);
}

/* DESODBC:
  Original author: MyODBC
  Modified by: DESODBC Developer
*/
static void put_default_value(STMT *stmt, DES_BIND *bind) {
  stmt->add_to_buffer("null",
                      4);  // DES doesn't seem to support the "DEFAULT" keyword.
                           // In the manual (4.2.5.1) says it is supported, but
                           // I get an error. We then substitute by null.
}

static BOOL allocate_param_buffer(DES_BIND *bind, unsigned long length) {
  if (bind == NULL) return FALSE;

  /* have to be very careful with that. it is probably better to put into
       a separate data structure. and free right after use */
  if (bind->buffer == NULL) {
    bind->buffer = myodbc_malloc(length, MYF(0));
    bind->buffer_length = length;
  } else if (bind->buffer_length < length) {
    bind->buffer = myodbc_realloc(bind->buffer, length);
    bind->buffer_length = length;
  }

  if (bind->buffer == NULL) {
    return TRUE;
  }

  return FALSE;
}

/* Buffer has to be allocated - no checks */
static unsigned long add2param_value(DES_BIND *bind, unsigned long pos,
                                     const char *value, unsigned long length) {
  memcpy((char *)bind->buffer + pos, value, length);

  bind->length_value = pos + length;
  return pos + length;
}

bool bind_param(DES_BIND *bind, const char *value, unsigned long length,
                enum enum_field_types buffer_type) {
  if (bind->buffer == (void *)value) {
    return false;
  }

  if (allocate_param_buffer(bind, length)) {
    return true;
  }

  memcpy(bind->buffer, value, length);
  bind->buffer_type = buffer_type;
  bind->length_value = length;

  return false;
}

/* TRUE - on memory allocation error */
static BOOL put_param_value(STMT *stmt, DES_BIND *bind, const char *value,
                            unsigned long length) {
  if (bind) {
    return bind_param(bind, value, length, DES_TYPE_STRING);
  } else {
    stmt->add_to_buffer(value, length);
  }

  return FALSE;
}

SQLRETURN check_c2sql_conversion_supported(STMT *stmt, DESCREC *aprec,
                                           DESCREC *iprec) {
  if (aprec->type == SQL_DATETIME && iprec->type == SQL_INTERVAL ||
      aprec->type == SQL_INTERVAL && iprec->type == SQL_DATETIME) {
    return stmt->set_error("07006", "Conversion is not supported");
  }

  switch (aprec->concise_type) {
    /* Currently we do not support those types */
    case SQL_C_INTERVAL_YEAR:
    case SQL_C_INTERVAL_MONTH:
    case SQL_C_INTERVAL_DAY:
    case SQL_C_INTERVAL_HOUR:
    case SQL_C_INTERVAL_MINUTE:
    case SQL_C_INTERVAL_SECOND:
    case SQL_C_INTERVAL_YEAR_TO_MONTH:
    case SQL_C_INTERVAL_DAY_TO_HOUR:
    case SQL_C_INTERVAL_DAY_TO_MINUTE:
    case SQL_C_INTERVAL_DAY_TO_SECOND:
    case SQL_C_INTERVAL_MINUTE_TO_SECOND:
      return stmt->set_error("07006", "Conversion is not supported by driver");
  }

  return SQL_SUCCESS;
}

/*
            stmt
            ctype       Input(parameter) value C type
            iprec
  [in,out]  rec         Pointer input and output value
            length      Pointer for result length
            buff        Pointer to a buffer for result value
            buff_max    Size of the buffer

*/
static SQLRETURN convert_c_type2str(STMT *stmt, SQLSMALLINT ctype,
                                    DESCREC *iprec, char **res, long *length,
                                    char *buff, uint buff_max) {
  switch (ctype) {
    case SQL_C_BINARY:
    case SQL_C_CHAR:
      break;

    case SQL_C_WCHAR: {
      /* Convert SQL_C_WCHAR (utf-16 or utf-32) to utf-8. */
      int has_utf8_maxlen4 = 0;

      /* length is in bytes, we want chars */
      *length = *length / sizeof(SQLWCHAR);

      // The length is passed as pointer. We need to account for platforms
      // where sizeof(SQLINTEGER) != sizeof(long).
      SQLINTEGER int_len = (SQLINTEGER)*length;

      *res = (char *)sqlwchar_as_utf8_ext((SQLWCHAR *)*res, &int_len,
                                          (SQLCHAR *)buff, buff_max,
                                          &has_utf8_maxlen4);
      // Write the returned SQLINTEGER value into the output param.
      *length = int_len;

      break;
    }

    case SQL_C_BIT:
    case SQL_C_TINYINT:
    case SQL_C_STINYINT:
      *length = (long)(des_int2str((long)*((signed char *)*res), buff, -10, 0) -
                       buff);
      *res = buff;
      break;
    case SQL_C_UTINYINT:
      *length =
          (long)(des_int2str((long)*((unsigned char *)*res), buff, -10, 0) -
                 buff);
      *res = buff;
      break;
    case SQL_C_SHORT:
    case SQL_C_SSHORT:
      *length =
          (long)(des_int2str((long)*((short int *)*res), buff, -10, 0) - buff);
      *res = buff;
      break;
    case SQL_C_USHORT:
      *length = (long)(des_int2str((long)*((unsigned short int *)*res), buff,
                                   -10, 0) -
                       buff);
      *res = buff;
      break;
    case SQL_C_LONG:
    case SQL_C_SLONG:
      *length = (long)(des_int2str(*((SQLINTEGER *)*res), buff, -10, 0) - buff);
      *res = buff;
      break;
    case SQL_C_ULONG:
      *length = (long)(des_int2str(*((SQLUINTEGER *)*res), buff, 10, 0) - buff);
      *res = buff;
      break;
    case SQL_C_SBIGINT:
      *length = (long)(myodbc_ll2str(*((longlong *)*res), buff, -10) - buff);
      *res = buff;
      break;
    case SQL_C_UBIGINT:
      *length = (long)(myodbc_ll2str(*((ulonglong *)*res), buff, 10) - buff);
      *res = buff;
      break;
    case SQL_C_FLOAT:
      if (iprec->concise_type != SQL_NUMERIC &&
          iprec->concise_type != SQL_DECIMAL) {
        // Better precision
        myodbc_d2str(*((float *)*res), buff, buff_max);
      } else {
        // We should perpare this data for string comparison, less precision
        myodbc_d2str(*((float *)*res), buff, buff_max, false);
      }
      *length = (long)strlen(*res = buff);
      break;
    case SQL_C_DOUBLE:
      if (iprec->concise_type != SQL_NUMERIC &&
          iprec->concise_type != SQL_DECIMAL) {
        myodbc_d2str(*((double *)*res), buff, buff_max);
      } else {
        // We should perpare this data for string comparison, less precision
        myodbc_d2str(*((double *)*res), buff, buff_max, false);
      }
      *length = (long)strlen(*res = buff);
      break;
    case SQL_C_DATE:
    case SQL_C_TYPE_DATE: {
      DATE_STRUCT *date = (DATE_STRUCT *)*res;
      if (stmt->dbc->ds.opt_MIN_DATE_TO_ZERO && !date->year &&
          (date->month == date->day == 1)) {
        *length = myodbc_snprintf(buff, buff_max, "0000-00-00");
      } else {
        *length = myodbc_snprintf(buff, buff_max, "%04d-%02d-%02d", date->year,
                                  date->month, date->day);
      }
      *res = buff;
      break;
    }
    case SQL_C_TIME:
    case SQL_C_TYPE_TIME: {
      TIME_STRUCT *time = (TIME_STRUCT *)*res;

      if (time->hour > 23) {
        return stmt->set_error("22008", "Not a valid time value supplied");
      }

      *length = myodbc_snprintf(buff, buff_max, "%02d:%02d:%02d", time->hour,
                                time->minute, time->second);
      *res = buff;
      break;
    }
    case SQL_C_TIMESTAMP:
    case SQL_C_TYPE_TIMESTAMP: {
      TIMESTAMP_STRUCT *time = (TIMESTAMP_STRUCT *)*res;

      if (stmt->dbc->ds.opt_MIN_DATE_TO_ZERO && !time->year &&
          (time->month == time->day == 1)) {
        *length = myodbc_snprintf(buff, buff_max, "0000-00-00 %02d:%02d:%02d",
                                  time->hour, time->minute, time->second);
      } else {
        *length = myodbc_snprintf(
            buff, buff_max, "%04d-%02d-%02d %02d:%02d:%02d", time->year,
            time->month, time->day, time->hour, time->minute, time->second);
      }

      if (time->fraction) {
        char *tmp_buf = buff + *length;

        /* Start cleaning from the end */
        int tmp_pos = 9;

        myodbc_snprintf(tmp_buf, buff_max - *length, ".%09d", time->fraction);

        /*
          ODBC specification defines nanoseconds granularity for
          the fractional part of seconds. MySQL only supports
          microseconds for TIMESTAMP, TIME and DATETIME.

          We are trying to remove the trailing zeros because this
          does not really modify the data, but often helps to substitute
          9 digits with only 6.
        */
        while (tmp_pos && tmp_buf[tmp_pos] == '0') {
          tmp_buf[tmp_pos--] = 0;
        }

        *length += tmp_pos + 1;
      }

      *res = buff;

      break;
    }
    case SQL_C_NUMERIC: {
      int trunc;
      SQL_NUMERIC_STRUCT *sqlnum = (SQL_NUMERIC_STRUCT *)*res;
      sqlnum_to_str(sqlnum, (SQLCHAR *)(buff + buff_max - 1), (SQLCHAR **)res,
                    (SQLCHAR)iprec->precision, (SQLSCHAR)iprec->scale, &trunc);
      *length = (long)strlen(*res);
      /* TODO no way to return an error here? */
      if (trunc == SQLNUM_TRUNC_FRAC) { /* 01S07 SQL_SUCCESS_WITH_INFO */
        stmt->set_error("01S07", "Fractional truncation");
        return SQL_SUCCESS_WITH_INFO;
      } else if (trunc == SQLNUM_TRUNC_WHOLE) { /* 22003 SQL_ERROR */
        return SQL_ERROR;
      }
      break;
    }

    case SQL_C_INTERVAL_HOUR_TO_MINUTE:
    case SQL_C_INTERVAL_HOUR_TO_SECOND: {
      SQL_INTERVAL_STRUCT *interval = (SQL_INTERVAL_STRUCT *)*res;

      if (ctype == SQL_C_INTERVAL_HOUR_TO_MINUTE) {
        *length = myodbc_snprintf(buff, buff_max, "'%d:%02d:00'",
                                  interval->intval.day_second.hour,
                                  interval->intval.day_second.minute);
      } else {
        *length = myodbc_snprintf(buff, buff_max, "'%d:%02d:%02d'",
                                  interval->intval.day_second.hour,
                                  interval->intval.day_second.minute,
                                  interval->intval.day_second.second);
      }

      *res = buff;
      break;
    }
    /* If we are missing processing of some valid C type. Probably means a bug
     * elsewhere */
    default:
      return stmt->set_error("07006", "Conversion is not supported");
  }
  return SQL_SUCCESS;
}

#define TIME_FIELDS_NONZERO(ts) \
  (ts.hour || ts.minute || ts.second || ts.fraction)

static const std::string ts_chars{"0123456789-:"};

inline bool is_ts_char(char c) { return std::string::npos != ts_chars.find(c); }

/*
Returns the date-time component of ODBC string like
{dt yyyy-mm-dd hh:mm:ss}
*/
const char *get_date_time_substr(const char *data, long &len) {
  const char *d_start = data;
  long idx = 0;
  while (len && !is_ts_char(*d_start)) {
    --len;
    ++d_start;
  }

  if (!len) return nullptr;

  const char *d_end = d_start + len - 1;
  while (d_start < d_end && !is_ts_char(*d_end)) {
    --len;
    --d_end;
  }

  return d_start;
}

/*
Add the value of parameter to a string buffer.

@param[in]      mysql
@param[in,out]  toptr - either pointer to a string where to write
                parameter value, or a pointer to DES_BIND structure.
@param[in]      apd The APD of the current statement
@param[in]      aprec The APD record of the parameter
@param[in]      iprec The IPD record of the parameter
*/
SQLRETURN insert_param(STMT *stmt, DES_BIND *bind, DESC *apd, DESCREC *aprec,
                       DESCREC *iprec, SQLULEN row) {
  long length;
  char buff[128], *data = NULL;
  BOOL convert = FALSE, free_data = FALSE;
  DBC *dbc = stmt->dbc;
  SQLLEN *octet_length_ptr = NULL;
  SQLLEN *indicator_ptr = NULL;
  SQLRETURN result = SQL_SUCCESS;

  if (aprec->octet_length_ptr) {
    octet_length_ptr = (SQLLEN *)ptr_offset_adjust(
        aprec->octet_length_ptr, apd->bind_offset_ptr, apd->bind_type,
        sizeof(SQLLEN), row);
    length = (long)*octet_length_ptr;
  }

  indicator_ptr =
      (SQLLEN *)ptr_offset_adjust(aprec->indicator_ptr, apd->bind_offset_ptr,
                                  apd->bind_type, sizeof(SQLLEN), row);

  if (aprec->data_ptr) {
    SQLINTEGER default_size =
        bind_length(aprec->concise_type, (ulong)aprec->octet_length);
    data = (char *)ptr_offset_adjust(aprec->data_ptr, apd->bind_offset_ptr,
                                     apd->bind_type, default_size, row);
  }

  if (indicator_ptr && *indicator_ptr == SQL_NULL_DATA) {
    put_null_param(stmt, bind);

    return SQL_SUCCESS;
  }
  /*
    According to
    http://msdn.microsoft.com/en-us/library/ms710963%28VS.85%29.aspx

    "... If StrLen_or_IndPtr is a null pointer, the driver assumes that all
    input parameter values are non-NULL and that character and *binary* data
    is null-terminated."
  */
  else if (!octet_length_ptr || *octet_length_ptr == SQL_NTS) {
    if (data) {
      if (aprec->concise_type == SQL_C_WCHAR) {
        length = (long)sqlwcharlen((SQLWCHAR *)data) * sizeof(SQLWCHAR);
      } else {
        length = (long)strlen(data);
      }

      if (!octet_length_ptr && aprec->octet_length > 0 &&
          aprec->octet_length != SQL_SETPARAM_VALUE_MAX) {
        length = (long)desodbc_min(length, aprec->octet_length);
      }
    } else {
      length = 0; /* TODO? This is actually an error */
    }
  }
  /*
    We may see SQL_COLUMN_IGNORE from bulk INSERT operations, where we
    may have been told to ignore a column in one particular row. So we
    try to insert DEFAULT, or NULL for really old servers.




    In case there are less parameters than result columns we have to
    insert NULL or DEFAULT.
  */
  else if (*octet_length_ptr == SQL_COLUMN_IGNORE ||
           /* empty values mean it's an unbound column */
           (*octet_length_ptr == 0 && aprec->concise_type == SQL_C_DEFAULT &&
            aprec->par.val() == NULL)) {
    put_default_value(stmt, bind);
    return SQL_SUCCESS;
  } else if (IS_DATA_AT_EXEC(octet_length_ptr)) {
    length = (long)aprec->par.val_length();
    if (!(data = aprec->par.val())) {
      put_default_value(stmt, bind);
      return SQL_SUCCESS;
    }
  }

  PUSH_ERROR(check_c2sql_conversion_supported(stmt, aprec, iprec));

  switch (aprec->concise_type) {
    case SQL_C_BINARY:
    case SQL_C_CHAR:
      convert = 1;
      break;

    default:
      switch (convert_c_type2str(stmt, aprec->concise_type, iprec, &data,
                                 &length, buff, sizeof(buff))) {
        case SQL_ERROR:
          return SQL_ERROR;
        case SQL_SUCCESS_WITH_INFO:
          result = SQL_SUCCESS_WITH_INFO;
      }

      if (data == NULL) {
        goto memerror;
      }

      if (!(data >= buff && data < buff + sizeof(buff))) {
        free_data = TRUE;
      }
  }

  switch (iprec->concise_type) {
    case SQL_DATE:
    case SQL_TYPE_DATE:
    case SQL_TYPE_TIMESTAMP:
    case SQL_TIMESTAMP:
      if (data[0] == '{') /* Of type {d date } */
      {
        /* TODO: check if we need to check for truncation here as well? */
        if (bind != NULL) {
          /* First the datetime has to be filtered out of the brackets
             and other stuff. The validity of the format is up to the
             user.
          */
          const char *tt = get_date_time_substr(data, length);

          /* The length parameter is changed by get_date_time_substr()
             because it is passed as a reference */
          if (bind_param(bind, tt, length, DES_TYPE_STRING)) {
            goto memerror;
          }
        } else {
          stmt->add_to_buffer(data, length);
        }
        goto out;
      }

      if (iprec->concise_type == SQL_DATE ||
          iprec->concise_type == SQL_TYPE_DATE) {
        TIMESTAMP_STRUCT ts;

        str_to_ts(&ts, data, length, 1, TRUE);

        /* Overflow also possible if converted from other C types
            http://msdn.microsoft.com/en-us/library/ms709385%28v=vs.85%29.aspx
            (if time fields nonzero sqlstate 22008 )
            http://msdn.microsoft.com/en-us/library/aa937531%28v=sql.80%29.aspx
            (Class values other than 01, except for the class IM,
            indicate an error and are accompanied by a return code
            of SQL_ERROR)

            Not sure if fraction should be considered as an overflow.
            In fact specs say about "time fields only"
          */
        if (!stmt->dbc->ds.opt_NO_DATE_OVERFLOW && TIME_FIELDS_NONZERO(ts)) {
          return stmt->set_error("22008", "Date overflow");
        }
      }

      /* else treat as a string */
    case SQL_CHAR:
    case SQL_VARCHAR:
    case SQL_LONGVARCHAR:
    case SQL_WCHAR:
    case SQL_WVARCHAR:
    case SQL_WLONGVARCHAR: {
      if ((aprec->concise_type == SQL_C_WCHAR &&
           dbc->cxn_charset_info->number != UTF8_CHARSET_NUMBER) ||
          (aprec->concise_type != SQL_C_WCHAR &&
           aprec->concise_type != SQL_C_CHAR)) {
        if (bind != NULL) {
          if (bind_param(bind, data, length, DES_TYPE_BLOB)) {
            goto memerror;
          }

          goto out;
        }
      }
      /* We have only added the introducer, data is added below. */
      break;
    }
    case SQL_INTERVAL_HOUR_TO_MINUTE:
    case SQL_INTERVAL_HOUR_TO_SECOND:
      if (aprec->concise_type != SQL_C_INTERVAL_HOUR_TO_MINUTE &&
          aprec->concise_type != SQL_C_INTERVAL_HOUR_TO_SECOND) {
        /* Allow only interval to interval conversion */
        stmt->set_error("07006", "Conversion is not supported");
      } else {
        if (put_param_value(stmt, bind, buff, length)) {
          goto memerror;
        }
      }
      goto out;

    case SQL_TIME:
    case SQL_TYPE_TIME:
      if (aprec->concise_type == SQL_C_TIMESTAMP ||
          aprec->concise_type == SQL_C_TYPE_TIMESTAMP) {
        TIMESTAMP_STRUCT *time = (TIMESTAMP_STRUCT *)aprec->data_ptr;

        if (time->hour > 23) {
          return stmt->set_error("22008", "Not a valid time value supplied");
        }
        if (time->fraction) {
          /* fractional seconds truncated, need to set correct sqlstate 22008
          http://msdn.microsoft.com/en-us/library/ms709385%28v=vs.85%29.aspx */

          return stmt->set_error("22008", "Fractional truncation");
        }

        if (bind != NULL) {
          length = myodbc_snprintf(buff, sizeof(buff), "%02d:%02d:%02d",
                                   time->hour, time->minute, time->second);
        } else {
          length = myodbc_snprintf(buff, sizeof(buff), "'%02d:%02d:%02d'",
                                   time->hour, time->minute, time->second);
        }

        if (put_param_value(stmt, bind, buff, length)) {
          goto memerror;
        }
      } else {
        ulong time;
        int hours;
        SQLUINTEGER fraction;

        /* For now it is safer to assume a dot is always a separator */
        /* stmt->dbc->ds->dont_use_set_locale */
        get_fractional_part(data, length, TRUE, &fraction);

        if (fraction) {
          /* truncation need SQL_ERROR and sqlstate 22008*/
          return stmt->set_error("22008", "Fractional truncation");
        }

        time = str_to_time_as_long(data, length);
        hours = time / 10000;

        if (hours > 23) {
          return stmt->set_error("22008", "Not a valid time value supplied");
        }

        if (bind != NULL) {
          length = myodbc_snprintf(buff, sizeof(buff), "%02d:%02d:%02d", hours,
                                   (int)time / 100 % 100, (int)time % 100);
        } else {
          length =
              myodbc_snprintf(buff, sizeof(buff), "'%02d:%02d:%02d'", hours,
                              (int)time / 100 % 100, (int)time % 100);
        }

        if (put_param_value(stmt, bind, buff, length)) {
          goto memerror;
        }
      }

      goto out;

    case SQL_BIT: {
      if (bind != NULL) {
        char bit_val = atoi(data) != 0 ? 1 : 0;
        /* Generic ODBC supports only BIT(1) */
        bind_param(bind, &bit_val, 1, DES_TYPE_TINY);
      } else if (!convert) {
        stmt->add_to_buffer(data, 1);
      }
      goto out;
    }
    case SQL_FLOAT:
    case SQL_REAL:
    case SQL_DOUBLE:
      /* If we have string -> float ; Fix locale characters for number */
      if (convert) {
        char *to = buff, *from = data;
        char *end = from + length;
        const char *local_thousands_sep = thousands_sep.c_str();
        const char *local_decimal_point = decimal_point.c_str();
        size_t local_thousands_sep_length = thousands_sep.length();
        size_t local_decimal_point_length = decimal_point.length();

        if (!stmt->dbc->ds.opt_NO_LOCALE) {
          /* force use of . as decimal point */
          local_thousands_sep = ",";
          local_thousands_sep_length = 1;
          local_decimal_point = ".";
          local_decimal_point_length = 1;
        }

        while (*from && from < end) {
          if (from[0] == local_thousands_sep[0] &&
              desodbc::is_prefix(from, local_thousands_sep)) {
            from += local_thousands_sep_length;
          } else if (from[0] == local_decimal_point[0] &&
                     desodbc::is_prefix(from, local_decimal_point)) {
            from += local_decimal_point_length;
            *to++ = '.';
          } else {
            *to++ = *from++;
          }
        }
        if (to == buff) {
          *to++ = '0'; /* Fix for empty strings */
        }
        data = buff;
        length = (uint)(to - buff);

        convert = 0;
      }
      /* Fall through */
    default:
      if (!convert) {
        put_param_value(stmt, bind, data, length);
        goto out;
      }
  }

  if (bind != NULL && stmt->setpos_op == 0) {
    bind_param(bind, data, length, DES_TYPE_STRING);
  } else {
    bool put_quotes = is_character_sql_data_type(
        iprec->concise_type);  // TODO: temporal solution.
    if (put_quotes) stmt->add_to_buffer("'", 1);
    /* Make sure we have room for a fully-escaped string. */
    if (!(stmt->extend_buffer(length * 2))) {
      goto memerror;
    }

    stmt->add_to_buffer(data, length);
    if (put_quotes) stmt->add_to_buffer("'", 1);
  }

out:
  if (free_data) {
    x_free(data);
  }

  return result;

memerror:
  if (free_data) {
    x_free(data);
  }
  return stmt->set_error("HY001", "Memory allocation error");
}

/*
@type    : myodbc3 internal
@purpose : positioned cursor update/delete
*/

SQLRETURN do_my_pos_cursor_std(STMT *pStmt, STMT *pStmtCursor) {
  const char *pszQuery = GET_QUERY(&pStmt->query);
  std::string query;
  SQLRETURN nReturn;

  if (pStmt->error.native_error == ER_INVALID_CURSOR_NAME) {
    return pStmt->set_error("HY000", "ER_INVALID_CURSOR_NAME");
  }

  while (isspace(*pszQuery)) ++pszQuery;

  query = pszQuery;

  if (!myodbc_casecmp(pszQuery, "delete", 6)) {
    nReturn = des_pos_delete_std(pStmtCursor, pStmt, 1, query);
  } else if (!myodbc_casecmp(pszQuery, "update", 6)) {
    nReturn = des_pos_update_std(pStmtCursor, pStmt, 1, query);
  } else {
    nReturn =
        pStmt->set_error("HY000", "Specified SQL syntax is not supported");
  }

  if (SQL_SUCCEEDED(nReturn)) pStmt->state = ST_EXECUTED;

  return (nReturn);
}

/*
@type    : ODBC 1.0 API
@purpose : executes a prepared statement, using the current values
of the parameter marker variables if any parameter markers
exist in the statement
*/

SQLRETURN SQL_API SQLExecute(SQLHSTMT hstmt) {
  LOCK_STMT(hstmt);

  return DES_SQLExecute((STMT *)hstmt);
}

BOOL map_error_to_param_status(SQLUSMALLINT *param_status_ptr, SQLRETURN rc) {
  if (param_status_ptr) {
    switch (rc) {
      case SQL_SUCCESS:
        *param_status_ptr = SQL_PARAM_SUCCESS;
        break;
      case SQL_SUCCESS_WITH_INFO:
        *param_status_ptr = SQL_PARAM_SUCCESS_WITH_INFO;
        break;

      default:
        /* SQL_PARAM_ERROR is set at the end of processing for last erroneous
           paramset so we have diagnostics for it */
        *param_status_ptr = SQL_PARAM_DIAG_UNAVAILABLE;
        return TRUE;
    }
  }

  return FALSE;
}

/* DESODBC:
  Renamed from the original my_SQLExecute and modified
  according to DES' needs.

  Original author: MyODBC
  Modified by: DESODBC Developer
*/
SQLRETURN DES_SQLExecute(STMT *pStmt) {
  std::string query;
  char *cursor_pos;
  int dae_rec, one_of_params_not_succeded = 0;
  bool is_select_stmt, is_process_stmt, is_delete_stmt, is_update_stmt,
      is_insert_stmt;
  int connection_failure = 0;
  STMT *pStmtCursor = pStmt;
  SQLRETURN rc = 0;
  SQLULEN row, length = 0;

  SQLUSMALLINT *param_operation_ptr = NULL, *param_status_ptr = NULL,
               *lastError = NULL;

  /* need to have a flag indicating if all parameters failed */
  int all_parameters_failed = pStmt->apd->array_size > 1 ? 1 : 0;

  if (!pStmt) return SQL_ERROR;

  CLEAR_STMT_ERROR(pStmt);

  pStmt->clear_attr_names();

  if (!GET_QUERY(&pStmt->query)) {
    rc = pStmt->set_error("HY010", "No previous SQLPrepare done");
    throw pStmt->error;
  }

  if (is_set_names_statement(GET_QUERY(&pStmt->query))) {
    rc = pStmt->set_error("42000", "SET NAMES not allowed by driver");
    throw pStmt->error;
  }

  if ((cursor_pos = check_if_positioned_cursor_exists(pStmt, &pStmtCursor))) {
    /* Save a copy of the query, because we're about to modify it. */
    pStmt->orig_query = pStmt->query;

    /* Cursor statement use mysql_use_result - thus any operation
    will couse commands out of sync */
    if (if_forward_cache(pStmtCursor)) {
      rc = pStmt->set_error("HY010", "Function sequence error");
      throw pStmt->error;
    }

    /* Chop off the 'WHERE CURRENT OF ...' - doing it a hard way...*/
    *cursor_pos = '\0';

    rc = do_my_pos_cursor_std(pStmt, pStmtCursor);
    if (!SQL_SUCCEEDED(rc))
      throw pStmt->error;
    else
      return rc;
  }

  DES_SQLFreeStmt((SQLHSTMT)pStmt, FREE_STMT_RESET_BUFFERS);

  query = GET_QUERY(&pStmt->query);

  // TODO: do this more elegantly.
  is_select_stmt = pStmt->query.is_select_statement();
  is_process_stmt = pStmt->query.is_process_statement();
  is_insert_stmt = pStmt->query.is_insert_statement();
  is_update_stmt = pStmt->query.is_update_statement();
  is_delete_stmt = pStmt->query.is_delete_statement();

  if (is_select_stmt)
    pStmt->type = SELECT;
  else if (is_process_stmt)
    pStmt->type = PROCESS;
  else if (is_insert_stmt)
    pStmt->type = INSERT;
  else if (is_delete_stmt)
    pStmt->type = DEL;
  else
    pStmt->type = UNKNOWN;

  if (pStmt->ipd->rows_processed_ptr) {
    *pStmt->ipd->rows_processed_ptr = (SQLULEN)0;
  }

  LOCK_DBC(pStmt->dbc);

  for (row = 0; row < pStmt->apd->array_size; ++row) {
    if (pStmt->param_count) {
      /* "The SQL_DESC_ROWS_PROCESSED_PTR field of the APD points to a buffer
      that contains the number of sets of parameters that have been processed,
      including error sets."
      "If SQL_NEED_DATA is returned, the value pointed to by the
      SQL_DESC_ROWS_PROCESSED_PTR field of the APD is set to the set of
      parameters that is being processed". And actually driver may continue to
      process paramsets after error. We need to decide do we want that.
      (http://msdn.microsoft.com/en-us/library/ms710963%28VS.85%29.aspx
      see "Using Arrays of Parameters")
      */
      if (pStmt->ipd->rows_processed_ptr) *pStmt->ipd->rows_processed_ptr += 1;

      param_operation_ptr = (SQLUSMALLINT *)ptr_offset_adjust(
          pStmt->apd->array_status_ptr, NULL, 0 /*SQL_BIND_BY_COLUMN*/,
          sizeof(SQLUSMALLINT), row);
      param_status_ptr = (SQLUSMALLINT *)ptr_offset_adjust(
          pStmt->ipd->array_status_ptr, NULL, 0 /*SQL_BIND_BY_COLUMN*/,
          sizeof(SQLUSMALLINT), row);

      if (param_operation_ptr && *param_operation_ptr == SQL_PARAM_IGNORE) {
        /* http://msdn.microsoft.com/en-us/library/ms712631%28VS.85%29.aspx
        - comments for SQL_ATTR_PARAM_STATUS_PTR */
        if (param_status_ptr) *param_status_ptr = SQL_PARAM_UNUSED;

        continue;
      }

      /*
       * If any parameters are required at execution time, cannot perform the
       * statement. It will be done through SQLPutData() and SQLParamData().
       */
      if ((dae_rec = desc_find_dae_rec(pStmt->apd)) > -1) {
        if (pStmt->apd->array_size > 1) {
          rc = pStmt->set_error("HYC00",
                                "Parameter arrays "
                                "with data at execution are not supported");
          lastError = param_status_ptr;

          one_of_params_not_succeded = 1;

          /* For other errors we continue processing of paramsets
              So this creates some inconsistency. But I guess that's better
              that user see diagnostics for this type of error */
          break;
        }

        pStmt->current_param = dae_rec;
        pStmt->dae_type = DAE_NORMAL;

        return SQL_NEED_DATA;
      }

      /* Making copy of the built query if that is not last paramset for
          select query. */
      if (is_select_stmt && row < pStmt->apd->array_size - 1) {
        // Just ignore the dummy query
        std::string dummy;
        rc = insert_params(pStmt, row, dummy);
      } else {
        rc = insert_params(pStmt, row, query);
      }

      /* Setting status for this paramset*/
      if (map_error_to_param_status(param_status_ptr, rc)) {
        lastError = param_status_ptr;
      }

      if (rc != SQL_SUCCESS) {
        one_of_params_not_succeded = 1;
      }

      if (!SQL_SUCCEEDED(rc)) {
        continue /*return rc*/;
      }

      /* For "SELECT" statement constructing single statement using
          "UNION ALL" */
      if (pStmt->apd->array_size > 1 && is_select_stmt) {
        if (row < pStmt->apd->array_size - 1) {
          const char *stmtsBinder = " UNION ALL ";
          const size_t binderLength = strlen(stmtsBinder);

          pStmt->add_to_buffer(stmtsBinder, binderLength);
          length += binderLength;
        }
      }
    }

    if (!is_select_stmt || row == pStmt->apd->array_size - 1) {
      if (!connection_failure) {
        try {
          rc = DES_do_query(pStmt, query);
        } catch (const std::bad_alloc &e) {
          return pStmt->set_error("HY001", "Memory allocation error");
        }

      } else {
        /*
        If the original query was modified, we reset stmt->query so that the
        next execution re-starts with the original query.
        */
        if (GET_QUERY(&pStmt->orig_query)) {
          pStmt->query = pStmt->orig_query;
          pStmt->orig_query.reset(NULL, NULL, NULL);
        }

        /* with broken connection we always return error for all next queries
         */
        rc = SQL_ERROR;
      }

      if (map_error_to_param_status(param_status_ptr, rc)) {
        lastError = param_status_ptr;
      }

      /* if we have anything but not SQL_SUCCESS for any paramset, we return
          SQL_SUCCESS_WITH_INFO as the whole operation result */
      if (rc != SQL_SUCCESS) {
        one_of_params_not_succeded = 1;
      } else {
        all_parameters_failed = 0;
      }

      length = 0;
    }
  }

  /* Changing status for last detected error to SQL_PARAM_ERROR as we have
      diagnostics for it */
  if (lastError != NULL) {
    *lastError = SQL_PARAM_ERROR;
  }

  /* Setting not processed paramsets status to SQL_PARAM_UNUSED
      this is needed if we stop paramsets processing on error.
  */
  if (param_status_ptr != NULL) {
    while (++row < pStmt->apd->array_size) {
      param_status_ptr = (SQLUSMALLINT *)ptr_offset_adjust(
          pStmt->ipd->array_status_ptr, NULL, 0 /*SQL_BIND_BY_COLUMN*/,
          sizeof(SQLUSMALLINT), row);

      *param_status_ptr = SQL_PARAM_UNUSED;
    }
  }
  /* code */

  if (pStmt->dummy_state == ST_DUMMY_PREPARED)
    pStmt->dummy_state = ST_DUMMY_EXECUTED;

  if (pStmt->apd->array_size > 1) {
    if (all_parameters_failed) {
      rc = SQL_ERROR;
      throw pStmt->error;
    } else if (one_of_params_not_succeded != 0) {
      return SQL_SUCCESS_WITH_INFO;
    }
  }

  return rc;
}

static SQLRETURN select_dae_param_desc(STMT *stmt, DESC **apd,
                                       unsigned int *param_count) {
  *param_count = stmt->param_count;
  /* get the correct APD for the dae type we're handling */
  switch (stmt->dae_type) {
    case DAE_NORMAL:
      *apd = stmt->apd;
      break;
    case DAE_SETPOS_INSERT:
    case DAE_SETPOS_UPDATE:
      *apd = stmt->setpos_apd.get();
      *param_count = (unsigned int)stmt->ard->rcount();
      break;
    default:
      return stmt->set_error("HY010", "Invalid data at exec state");
  }

  return SQL_SUCCESS;
}

/* Looks for next DAE parameter and returns true if finds it */
static SQLRETURN find_next_dae_param(STMT *stmt, SQLPOINTER *token) {
  unsigned int i, param_count;
  DESC *apd;

  PUSH_ERROR(select_dae_param_desc(stmt, &apd, &param_count));

  for (i = stmt->current_param; i < param_count; ++i) {
    DESCREC *aprec = desc_get_rec(apd, i, FALSE);
    SQLLEN *octet_length_ptr;

    assert(aprec);
    octet_length_ptr = (SQLLEN *)ptr_offset_adjust(
        aprec->octet_length_ptr, apd->bind_offset_ptr, apd->bind_type,
        sizeof(SQLLEN), 0);

    /* get the "placeholder" pointer the application bound */
    if (IS_DATA_AT_EXEC(octet_length_ptr)) {
      SQLINTEGER default_size =
          bind_length(aprec->concise_type, (ulong)aprec->octet_length);
      stmt->current_param = i + 1;
      if (token) {
        *token = ptr_offset_adjust(aprec->data_ptr, apd->bind_offset_ptr,
                                   apd->bind_type, default_size, 0);
      }
      aprec->par.reset();
      aprec->par.is_dae = 1;

      return SQL_NEED_DATA;
    }
  }

  return SQL_SUCCESS;
}

/* DESODBC:
  Original author: MyODBC
  Modified by: DESODBC Developer
*/
/*
@type    : ODBC 1.0 API
@purpose : is used in conjunction with SQLPutData to supply parameter
data at statement execution time
*/
SQLRETURN SQL_API SQLParamData(SQLHSTMT hstmt, SQLPOINTER *prbgValue) {
  return ((STMT *)hstmt)
      ->set_error("IM001", "Data-at-execution is not supported in DES");
}

/* DESODBC:
  Original author: MyODBC
  Modified by: DESODBC Developer
*/
/*
@type    : ODBC 1.0 API
@purpose : allows an application to send data for a parameter or column to
the driver at statement execution time. This function can be used
to send character or binary data values in parts to a column with
a character, binary, or data source specific data type.
*/

SQLRETURN SQL_API SQLPutData(SQLHSTMT hstmt, SQLPOINTER rgbValue,
                             SQLLEN cbValue) {
  return ((STMT *)hstmt)
      ->set_error("IM001", "Data-at-execution is not supported in DES");
}

/* DESODBC:

  Original author: MyODBC
  Modified by: DESODBC Developer
*/
/**
Cancel the query by opening another connection and using KILL when called
from another thread while the query lock is being held. Otherwise, treat as
SQLFreeStmt(hstmt, SQL_CLOSE).

@param[in]  hstmt  Statement handle

@return Standard ODBC result code
*/
SQLRETURN SQL_API SQLCancel(SQLHSTMT hstmt) {
  return ((STMT *)hstmt)
      ->set_error("IM001", "DESODBC does not support this function");
}
