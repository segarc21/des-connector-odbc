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
  @file  error.c
  @brief Error handling functions.
*/

#include "driver.h"

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
DESERROR::DESERROR(const char* state, const char* msg)
{
  sqlstate = state ? state : "";
  message = std::string(msg);
  retcode = SQL_ERROR;
}

#ifdef _WIN32
/* DESODBC:
    Original author: DESODBC Developer
*/
std::string GetLastWinErrMessage() {
  DWORD errorCode = GetLastError();
  LPSTR errorMsg = nullptr;

  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                 nullptr, errorCode, 0, (LPSTR)&errorMsg, 0, nullptr);

  if (errorMsg)
    return std::string(errorMsg);
  else
    return "";
}

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
SQLRETURN DBC::set_error(const char *state, const char *msg) {
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

/*
  @type    : myodbc3 internal
  @purpose : sets the descriptor level errors
*/
SQLRETURN set_desc_error(DESC *        desc,
                         char *        state,
                         const char *  message)
{
  desc->error = DESERROR(state, message);
  return SQL_ERROR;
}


/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : myodbc3 internal
  @purpose : sets the error information to appropriate handle.
  it also sets the SQLSTATE based on the ODBC VERSION
*/
SQLRETURN set_handle_error(SQLSMALLINT HandleType, SQLHANDLE handle,
                           const char* state, const char *errtext) {
  switch (HandleType) {
    case SQL_HANDLE_ENV:
      return ((ENV *)handle)->set_error(state, errtext);
    case SQL_HANDLE_DBC:
      return ((DBC *)handle)->set_error(state, errtext);
    case SQL_HANDLE_STMT: {
      return ((STMT *)handle)->set_error(state, errtext);
    }
    case SQL_HANDLE_DESC: {
      return ((DESC *)handle)->set_error(state, errtext);
    }
    default:
      return SQL_INVALID_HANDLE;
  }
}

/**
*/
SQLRETURN SQL_API
MySQLGetDiagRec(SQLSMALLINT handle_type, SQLHANDLE handle, SQLSMALLINT record,
                SQLCHAR **sqlstate, SQLINTEGER *native, SQLCHAR **message)
{
  DESERROR *error;
  SQLINTEGER tmp_native;

  if (!native)
    native= &tmp_native;

  if (!handle || record <= 0)
    return SQL_ERROR;

  /*
    Currently we are not supporting error list, so
    if RecNumber > 1, return no data found
  */
  if (record > 1)
    return SQL_NO_DATA_FOUND;

  if (handle_type == SQL_HANDLE_STMT)
    error= &((STMT *)handle)->error;
  else if (handle_type == SQL_HANDLE_DBC)
    error= &((DBC *)handle)->error;
  else if (handle_type == SQL_HANDLE_ENV)
    error= &((ENV *)handle)->error;
  else if (handle_type == SQL_HANDLE_DESC)
    error= &((DESC *)handle)->error;
  else
    return SQL_INVALID_HANDLE;

  if (error->message.empty())
  {
    *message= (SQLCHAR *)"";
    *sqlstate= (SQLCHAR *)"00000";
    *native= 0;
    return SQL_NO_DATA_FOUND;
  }

  *message= (SQLCHAR *)error->message.c_str();
  *sqlstate= (SQLCHAR *)error->sqlstate.c_str();
  *native= error->native_error;

  return SQL_SUCCESS;
}


bool is_odbc3_subclass(std::string sqlstate)
{
  const char *states[]= { "01S00", "01S01", "01S02", "01S06", "01S07", "07S01",
    "08S01", "21S01", "21S02", "25S01", "25S02", "25S03", "42S01", "42S02",
    "42S11", "42S12", "42S21", "42S22", "HY095", "HY097", "HY098", "HY099",
    "HY100", "HY101", "HY105", "HY107", "HY109", "HY110", "HY111", "HYT00",
    "HYT01", "IM001", "IM002", "IM003", "IM004", "IM005", "IM006", "IM007",
    "IM008", "IM010", "IM011", "IM012"};
  size_t i;

  if (sqlstate.empty())
    return false;

  for (i= 0; i < sizeof(states) / sizeof(states[0]); ++i)
    if (sqlstate.compare(states[i]) == 0)
      return true;

  return false;
}


/* DESODBC:
    This function corresponds to the
    implementation of SQLGetDiagField, which was
    MySQLGetDiagField in MyODBC. We have reused
    most of it.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : ODBC 3.0 API
  @purpose : returns the current value of a field of a record of the
  diagnostic data structure (associated with a specified handle)
  that contains error, warning, and status information
*/
SQLRETURN SQL_API
DES_SQLGetDiagField(SQLSMALLINT handle_type, SQLHANDLE handle, SQLSMALLINT record,
                  SQLSMALLINT identifier, SQLCHAR **char_value,
                  SQLPOINTER num_value)
{
  SQLLEN num;

  /* Handle may not be these types, but this saves lots of casts below. */
  STMT *stmt= (STMT *)handle;
  DBC *dbc= (DBC *)handle;
  DESC *desc= (DESC *)handle;
  DESERROR *error;

  if (!num_value)
    num_value= &num;

  if (!handle)
    return SQL_ERROR;

  if (handle_type == SQL_HANDLE_DESC)
    error= &desc->error;
  else if (handle_type == SQL_HANDLE_STMT)
    error= &stmt->error;
  else if (handle_type == SQL_HANDLE_DBC)
    error= &dbc->error;
  else if (handle_type == SQL_HANDLE_ENV)
    error= &((ENV *)handle)->error;
  else
    return SQL_ERROR;

  if (record > 1)
    return SQL_NO_DATA_FOUND;

  switch (identifier)
  {
  /*  Header fields */
  case SQL_DIAG_CURSOR_ROW_COUNT:
    if (handle_type != SQL_HANDLE_STMT)
      return SQL_ERROR;
    if (!stmt->result)
      *(SQLLEN *)num_value = 0;
    else
      *(SQLLEN *)num_value = (SQLLEN) des_num_rows(stmt->result);
    return SQL_SUCCESS;

  case SQL_DIAG_DYNAMIC_FUNCTION:
    if (handle_type != SQL_HANDLE_STMT)
      return SQL_ERROR;
    *char_value= (SQLCHAR *)"";
    return SQL_SUCCESS;

  case SQL_DIAG_DYNAMIC_FUNCTION_CODE:
    if (handle_type != SQL_HANDLE_STMT)
      return SQL_ERROR;
    *(SQLINTEGER *)num_value= 0;
    return SQL_SUCCESS;

  case SQL_DIAG_NUMBER:
    *(SQLINTEGER *)num_value= 1;
    return SQL_SUCCESS;

  case SQL_DIAG_RETURNCODE:
    *(SQLRETURN *)num_value= error->retcode;
    return SQL_SUCCESS;

  case SQL_DIAG_ROW_COUNT:
    if (handle_type != SQL_HANDLE_STMT)
      return SQL_ERROR;
    *(SQLLEN *)num_value= (SQLLEN)stmt->affected_rows;
    return SQL_SUCCESS;

  /* Record fields */
  case SQL_DIAG_CLASS_ORIGIN:
    {
      if (record <= 0)
        return SQL_ERROR;
      auto &sqlstate = error->sqlstate;

      if (!sqlstate.empty() && sqlstate[0] == 'I' && sqlstate[1] == 'M')
        *char_value= (SQLCHAR *)"ODBC 3.0";
      else
        *char_value= (SQLCHAR *)"ISO 9075";
    }
    return SQL_SUCCESS;

  case SQL_DIAG_COLUMN_NUMBER:
    if (record <= 0)
      return SQL_ERROR;
    *(SQLINTEGER *)num_value= SQL_COLUMN_NUMBER_UNKNOWN;
    return SQL_SUCCESS;

  case SQL_DIAG_CONNECTION_NAME:
  {
    DataSource *ds;
    if (record <= 0)
      return SQL_ERROR;

    if (handle_type == SQL_HANDLE_DESC)
      ds= &desc->stmt->dbc->ds;
    else if (handle_type == SQL_HANDLE_STMT)
      ds= &stmt->dbc->ds;
    else if (handle_type == SQL_HANDLE_DBC)
      ds= &dbc->ds;
    else
      *char_value= (SQLCHAR*)"";

    if (ds)
      *char_value = ds->opt_DSN;
    return SQL_SUCCESS;
  }

  case SQL_DIAG_MESSAGE_TEXT:
    if (record <= 0)
      return SQL_ERROR;
    *char_value = (SQLCHAR *)error->message.c_str();
    return SQL_SUCCESS;

  case SQL_DIAG_NATIVE:
    *(SQLINTEGER *)num_value= error->native_error;
    return SQL_SUCCESS;

  case SQL_DIAG_ROW_NUMBER:
    if (record <= 0)
      return SQL_ERROR;
    *(SQLLEN *)num_value= SQL_ROW_NUMBER_UNKNOWN;
    return SQL_SUCCESS;

  case SQL_DIAG_SERVER_NAME:
  {
    DataSource *ds;
    if (record <= 0)
      return SQL_ERROR;
    if (handle_type == SQL_HANDLE_DESC)
      ds = &desc->stmt->dbc->ds;
    else if (handle_type == SQL_HANDLE_STMT)
      ds = &stmt->dbc->ds;
    else if (handle_type == SQL_HANDLE_DBC)
      ds = &dbc->ds;
    else
      *char_value= (SQLCHAR *)"";

    if (ds)
        *char_value = ds->opt_DES_EXEC;

    return SQL_SUCCESS;
  }

  case SQL_DIAG_SQLSTATE:
    if (record <= 0)
      return SQL_ERROR;
    *char_value= (SQLCHAR *)error->sqlstate.c_str();
    return SQL_SUCCESS;

  case SQL_DIAG_SUBCLASS_ORIGIN:
    if (record <= 0)
      return SQL_ERROR;
    {
      if (record <= 0)
        return SQL_ERROR;

      if (is_odbc3_subclass((char*)error->sqlstate.c_str()))
        *char_value= (SQLCHAR *)"ODBC 3.0";
      else
        *char_value= (SQLCHAR *)"ISO 9075";
    }
    return SQL_SUCCESS;

  default:
    return SQL_ERROR;
  }
}
