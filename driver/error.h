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

/***************************************************************************
 * ERROR.H								   *
 *									   *
 * @description: Definitions for error handling				   *
 *									   *
 * @author     : MySQL AB(monty@mysql.com, venu@mysql.com)		   *
 * @date       : 2001-Aug-15						   *
 * @product    : myodbc3						   *
 *									   *
****************************************************************************/

#ifndef __ERROR_H__
#define __ERROR_H__

/* Including driver version definitions */
#include "../MYODBC_CONF.h"
/*
  myodbc internal error constants
*/
#define ER_INVALID_CURSOR_NAME	 514
#define ER_ALL_COLUMNS_IGNORED	 537

/*
  myodbc error prefix
*/
#define DESODBC_ERROR_PREFIX	 "[DES][ODBC " DESODBC_STRDRIVERID " Driver]"
#define DESODBC_ERROR_CODE_START  500

#define CLEAR_ENV_ERROR(env)   (((ENV *)env)->error.clear())
#define CLEAR_DBC_ERROR(dbc)   (((DBC *)dbc)->error.clear())
#define CLEAR_STMT_ERROR(stmt) (((STMT *)stmt)->error.clear())
#define CLEAR_DESC_ERROR(desc) (((DESC *)desc)->error.clear())

#define NEXT_ERROR(error) \
  (error.current ? 2 : (error.current= 1))

#define NEXT_ENV_ERROR(env)   NEXT_ERROR(((ENV *)env)->error)
#define NEXT_DBC_ERROR(dbc)   NEXT_ERROR(((DBC *)dbc)->error)
#define NEXT_STMT_ERROR(stmt) NEXT_ERROR(((STMT *)stmt)->error)
#define NEXT_DESC_ERROR(desc) NEXT_ERROR(((DESC *)desc)->error)

/*
  list of MyODBC3 error codes
*/
typedef enum desodbc_errid
{
    DESERR_01000 = 0,
    DESERR_01004,
    DESERR_01S02,
    DESERR_01S03,
    DESERR_01S04,
    DESERR_01S06,
    DESERR_07001,
    DESERR_07005,
    DESERR_07006,
    DESERR_07009,
    DESERR_08002,
    DESERR_08003,
    DESERR_24000,
    DESERR_25000,
    DESERR_25S01,
    DESERR_34000,
    DESERR_HYT00,
    DESERR_S1000,
    DESERR_S1001,
    DESERR_S1002,
    DESERR_S1003,
    DESERR_S1004,
    DESERR_S1007,
    DESERR_S1009,
    DESERR_S1010,
    DESERR_S1011,
    DESERR_S1012,
    DESERR_S1013,
    DESERR_S1015,
    DESERR_S1016,
    DESERR_S1017,
    DESERR_S1024,
    DESERR_S1090,
    DESERR_S1091,
    DESERR_S1092,
    DESERR_S1093,
    DESERR_S1095,
    DESERR_S1106,
    DESERR_S1107,
    DESERR_S1109,
    DESERR_S1C00,

    DESERR_21S01,
    DESERR_23000,
    DESERR_42000,
    DESERR_42S01,
    DESERR_42S02,
    DESERR_42S12,
    DESERR_42S21,
    DESERR_42S22,
    DESERR_08S01,
    /* Please add new errors to the end of enum, and not in alphabet order */
    DESERR_08004,
} desodbc_errid;

/*
  error handler structure
*/
struct DESERROR
{
  SQLRETURN   retcode = 0;
  char        current = 0;
  std::string message;
  SQLINTEGER  native_error = 0;
  std::string sqlstate;

  DESERROR()
  {}

  DESERROR(SQLRETURN rc) : DESERROR()
  {
    retcode = rc;
  }

  DESERROR(desodbc_errid errid, const char *errtext, SQLINTEGER errcode,
    const char *prefix);

  DESERROR(const char *state, const char *msg, SQLINTEGER errcode,
    const char *prefix);

  DESERROR(SQLSMALLINT htype, SQLHANDLE handle, SQLRETURN rc)
  {
    SQLCHAR     state[6], msg[SQL_MAX_MESSAGE_LENGTH];
    SQLSMALLINT length;
    SQLRETURN   drc;

    /** @todo Handle multiple diagnostic records. */
    drc = SQLGetDiagRecA(htype, handle, 1, state, &native_error,
      msg, SQL_MAX_MESSAGE_LENGTH - 1, &length);

    if (drc == SQL_SUCCESS || drc == SQL_SUCCESS_WITH_INFO)
    {
      sqlstate = (const char*)state;
      message = (const char*)msg;
    }
    else
    {
      sqlstate = (const char*)"00000";
      message = (const char*)"Did not get expected diagnostics";
    }

    retcode = rc;
  }

  operator std::string() const
  {
    return message;
  }

  operator bool() const
  {
    return native_error != 0 || retcode != 0;
  }

  void clear()
  {
    retcode = 0;
    message.clear();
    current = 0;
    native_error = 0;
    sqlstate.clear();
  }

  DESERROR(const char* state, MYSQL* mysql) :
    DESERROR(state, mysql_error(mysql),
      mysql_errno(mysql), DESODBC_ERROR_PREFIX)
  {}

  DESERROR(const char* state, std::string errmsg) :
    DESERROR(state, errmsg.c_str(), 0, DESODBC_ERROR_PREFIX)
  {}
};

/*
  error handler-predefined structure
  odbc2 state, odbc3 state, message and return code
*/
typedef struct desodbc3_err_str {
  char	   sqlstate[6];  /* ODBC3 STATE, if SQL_OV_ODBC2, then ODBC2 STATE */
  char	   message[SQL_MAX_MESSAGE_LENGTH+1];/* ERROR MSG */
  SQLRETURN   retcode;	    /* RETURN CODE */
} DESODBC3_ERR_STR;

#endif /* __ERROR_H__ */
