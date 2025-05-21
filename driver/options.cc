// Copyright (c) 2000, 2024, Oracle and/or its affiliates.
// Modified in 2025 by Sergio Miguel García Jiménez <segarc21@ucm.es>
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
// Modified in 2025 by Sergio Miguel García Jiménez <segarc21@ucm.es>,
// hereinafter the DESODBC developer, in the context of the GPLv2 derivate
// work DESODBC, an ODBC Driver of the open-source DBMS Datalog Educational
// System (DES) (see https://www.fdi.ucm.es/profesor/fernan/des/)
//
// The authorship of each section of this source file (comments,
// functions and other symbols) belongs to MyODBC unless we
// explicitly state otherwise.
// ---------------------------------------------------------

/**
  @file  options.c
  @brief Functions for handling handle attributes and options.
*/

#include "driver.h"
#include "errmsg.h"

/* DESODBC:

    Modified according to DES' needs.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : myodbc3 internal
  @purpose : sets the common connection/stmt attributes
*/
static SQLRETURN set_constmt_attr(SQLSMALLINT  HandleType,
                                  SQLHANDLE    Handle,
                                  STMT_OPTIONS *options,
                                  SQLINTEGER   Attribute,
                                  SQLPOINTER   ValuePtr)
{
    switch (Attribute)
    {
        case SQL_ATTR_ASYNC_ENABLE:
            if (ValuePtr == (SQLPOINTER) SQL_ASYNC_ENABLE_ON)
            return set_handle_error(
                HandleType, Handle, "01S02",
                                        "Doesn't support asynchronous, changed to default");
            break;

        case SQL_ATTR_CURSOR_SENSITIVITY:
            if (ValuePtr != (SQLPOINTER) SQL_UNSPECIFIED)
            {
            return set_handle_error(HandleType, Handle, "01S02",
                                        "Option value changed to default cursor sensitivity (unspecified)");
            }
            break;

        case SQL_ATTR_CURSOR_TYPE:
          if (ValuePtr == (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY ||
              ValuePtr == (SQLPOINTER)SQL_CURSOR_STATIC)
            options->cursor_type = (SQLUINTEGER)(SQLULEN)ValuePtr;

          else {
            options->cursor_type = SQL_CURSOR_STATIC;
            return set_handle_error(
                HandleType, Handle, "01S02",
                "Option value changed to default static cursor");
          }
            break;

        case SQL_ATTR_MAX_LENGTH:
            options->max_length= (SQLULEN) ValuePtr;
            break;

        case SQL_ATTR_MAX_ROWS:
            options->max_rows= (SQLULEN) ValuePtr;
            break;

        case SQL_ATTR_METADATA_ID:
            options->metadata_id = (ValuePtr == (SQLPOINTER)SQL_TRUE);
            break;

        case SQL_ATTR_RETRIEVE_DATA:
            options->retrieve_data = (ValuePtr != (SQLPOINTER)SQL_RD_OFF);
            break;

        case SQL_ATTR_SIMULATE_CURSOR:
            if (ValuePtr != (SQLPOINTER) SQL_SC_TRY_UNIQUE)
            return set_handle_error(
                  HandleType, Handle, "01S02",
                                        "Option value changed to default cursor simulation");
            break;

        case 1226:/* MS SQL Server Extension */
        case 1227:
        case 1228:
            break;

        case SQL_ATTR_USE_BOOKMARKS:
          if (ValuePtr == (SQLPOINTER) SQL_UB_VARIABLE ||
              ValuePtr == (SQLPOINTER) SQL_UB_ON)
            options->bookmarks= (SQLUINTEGER) SQL_UB_VARIABLE;
          else
            options->bookmarks= (SQLUINTEGER) SQL_UB_OFF;
          break;

        case SQL_ATTR_FETCH_BOOKMARK_PTR:
          options->bookmark_ptr = ValuePtr;
          break;

        case SQL_ATTR_QUERY_TIMEOUT:
        case SQL_ATTR_KEYSET_SIZE:
        case SQL_ATTR_CONCURRENCY:
        case SQL_ATTR_NOSCAN:
        default:
          /* Do something only if the handle is STMT */
          return set_handle_error(HandleType, Handle, "01S02",
                                  "Unsuported option");
          break;
            /* ignored */
            break;
    }
    return SQL_SUCCESS;
}

/*
  @type    : myodbc3 internal
  @purpose : returns the common connection/stmt attributes
*/

static SQLRETURN
get_constmt_attr(SQLSMALLINT  HandleType,
                 SQLHANDLE    Handle,
                 STMT_OPTIONS *options,
                 SQLINTEGER   Attribute,
                 SQLPOINTER   ValuePtr,
                 SQLINTEGER   *StringLengthPtr __attribute__((unused)))
{
    switch (Attribute)
    {
        case SQL_ATTR_ASYNC_ENABLE:
            *((SQLUINTEGER *) ValuePtr)= SQL_ASYNC_ENABLE_OFF;
            break;

        case SQL_ATTR_CURSOR_SENSITIVITY:
            *((SQLUINTEGER *) ValuePtr)= SQL_UNSPECIFIED;
            break;

        case SQL_ATTR_CURSOR_TYPE:
            *((SQLUINTEGER *) ValuePtr)= options->cursor_type;
            break;

        case SQL_ATTR_MAX_LENGTH:
            *((SQLULEN *) ValuePtr)= options->max_length;
            break;

        case SQL_ATTR_MAX_ROWS:
            *((SQLULEN *) ValuePtr)= options->max_rows;
            break;

        case SQL_ATTR_METADATA_ID:
            *((SQLUINTEGER *) ValuePtr)= options->metadata_id;
            break;

        case SQL_ATTR_QUERY_TIMEOUT:
          /* Do something only if the handle is STMT */
          return set_handle_error(HandleType, Handle, "01S02",
                                  "Unsuported option");
          break;
            break;
        case SQL_ATTR_RETRIEVE_DATA:
            *((SQLULEN *) ValuePtr)= (options->retrieve_data ? SQL_RD_ON : SQL_RD_OFF);
            break;

        case SQL_ATTR_SIMULATE_CURSOR:
            *((SQLUINTEGER *) ValuePtr)= SQL_SC_TRY_UNIQUE;
            break;

        case SQL_ATTR_CONCURRENCY:
            *((SQLUINTEGER *) ValuePtr)= SQL_CONCUR_READ_ONLY;
            break;

        case SQL_KEYSET_SIZE:
            *((SQLUINTEGER *) ValuePtr)= 0L;
            break;

        case SQL_NOSCAN:
            *((SQLUINTEGER *) ValuePtr)= SQL_NOSCAN_ON;
            break;

        case SQL_ATTR_USE_BOOKMARKS:
            *((SQLUINTEGER *) ValuePtr) = options->bookmarks;
            break;

        case SQL_ATTR_FETCH_BOOKMARK_PTR:
          *((void **) ValuePtr) = options->bookmark_ptr;
          *StringLengthPtr= sizeof(SQLPOINTER);

        case 1226:/* MS SQL Server Extension */
        case 1227:
        case 1228:
        default:
            /* ignored */
            break;
    }
    return SQL_SUCCESS;
}


/* DESODBC:

    Renamed from the original MySQLSetConnectAttr
    and modified according to DES' needs.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : myodbc3 internal
  @purpose : sets the connection attributes
*/
SQLRETURN SQL_API
DESSetConnectAttr(SQLHDBC hdbc, SQLINTEGER Attribute,
                    SQLPOINTER ValuePtr, SQLINTEGER StringLengthPtr)
{
  DBC *dbc= (DBC *) hdbc;

  std::string catalog;
  SQLRETURN rc;
  std::string output;
  std::pair<SQLRETURN, std::string> pair;
  std::string query;

  switch (Attribute)
  {
    case SQL_ATTR_CURRENT_CATALOG:

      rc = dbc->getQueryMutex();
      if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return rc;

      query = "/use_db ";
      catalog = sqlcharptr_to_str((SQLCHAR *)ValuePtr, StringLengthPtr);
      query += catalog;

      pair = dbc->send_query_and_read(query);

      rc = pair.first;
      output = pair.second;

      if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        dbc->releaseQueryMutex();
        return rc;
      }

      dbc->releaseQueryMutex();

      //We do not want to return an error when receiving this message.
      if (!is_in_string(output, "Database already in use")) {
        rc = check_and_set_errors(SQL_HANDLE_DBC, dbc, output);
      }

      if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        return rc;
      }
      break;
    case SQL_ATTR_ACCESS_MODE:
      return dbc->set_error("HYC00",
                            "Unsupported option due to DES' characteristics");
      break;

    case SQL_ATTR_AUTOCOMMIT:
      return dbc->set_error("HYC00", "Unsupported option due to DES' characteristics");
      break;

    case SQL_ATTR_LOGIN_TIMEOUT:
      return dbc->set_error("HYC00",
                            "Unsupported option due to DES' characteristics");
      break;

    case SQL_ATTR_CONNECTION_TIMEOUT:
      return dbc->set_error("HY092", "Read-only attribute");
      break;

    case SQL_ATTR_ODBC_CURSORS:
      if (dbc->ds.opt_FORWARD_CURSOR &&
        ValuePtr != (SQLPOINTER) SQL_CUR_USE_ODBC)
        return ((DBC *)hdbc)
            ->set_error(
                "01S02",
          "Forcing the Driver Manager to use ODBC cursor library");
      break;

    case SQL_OPT_TRACE:
    case SQL_OPT_TRACEFILE:
    case SQL_QUIET_MODE:
    case SQL_TRANSLATE_DLL:
    case SQL_TRANSLATE_OPTION:
      {
        char buff[100];
        myodbc_snprintf(buff, sizeof(buff),
                        "Suppose to set this attribute '%d' through driver "
                        "manager, not by the driver",
                        (int)Attribute);
        return ((DBC *)hdbc)->set_error("01S02", buff);
      }

    case SQL_ATTR_PACKET_SIZE:
        return dbc->set_error("HYC00",
                              "Unsupported option due to DES' characteristics");
      break;

    case SQL_ATTR_TXN_ISOLATION:
      return dbc->set_error("HYC00",
                            "Unsupported option due to DES' characteristics");
      break;

#ifndef USE_IODBC
    case SQL_ATTR_RESET_CONNECTION:
      return dbc->set_error("HYC00", "Optional feature not implemented");
#endif

    case CB_FIDO_CONNECTION:
      dbc->fido_callback = (fido_callback_func)ValuePtr;
      break;
    case CB_FIDO_GLOBAL:
      {
        std::unique_lock<std::mutex> fido_lock(global_fido_mutex);
        global_fido_callback = (fido_callback_func)ValuePtr;
        break;
      }
    case SQL_ATTR_ENLIST_IN_DTC:
        return dbc->set_error("HYC00",
                              "Unsupported option due to DES' characteristics");

      /*
        3.x driver doesn't support any statement attributes
        at connection level, but to make sure all 2.x apps
        works fine...lets support it..nothing to lose..
      */
    default:
      return set_constmt_attr(2, dbc, &dbc->stmt_options, Attribute,
                                ValuePtr);
  }

  return SQL_SUCCESS;
}


/* DESODBC:

    Renamed from the original MySQLGetConnectAttr.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/**
 Retrieve connection attribute values.

 @param[in]  hdbc
 @param[in]  attrib
 @param[out] char_attr
 @param[out] num_attr
*/
SQLRETURN SQL_API
DESGetConnectAttr(SQLHDBC hdbc, SQLINTEGER attrib, SQLCHAR **char_attr,
                    SQLPOINTER num_attr)
{
  DBC *dbc= (DBC *)hdbc;
  SQLRETURN result= SQL_SUCCESS;
  std::pair<SQLRETURN, std::string> pair;
  std::string current_db_output, current_db;

  switch (attrib)
  {
  case SQL_ATTR_ACCESS_MODE:
    *((SQLUINTEGER *)num_attr)= SQL_MODE_READ_WRITE;
    break;

  case SQL_ATTR_AUTO_IPD:
    *((SQLUINTEGER *)num_attr)= SQL_FALSE;
    break;

  case SQL_ATTR_AUTOCOMMIT:
    return dbc->set_error("HYC00",
                          "Unsupported option due to DES' characteristics");
    break;

  case SQL_ATTR_CONNECTION_DEAD:
    //We do a little test
    result = dbc->getQueryMutex();
    if (result != SQL_SUCCESS && result != SQL_SUCCESS_WITH_INFO) return result;

    pair = dbc->send_query_and_read("/current_db");
    result = pair.first;
    current_db_output = pair.second;
    dbc->releaseQueryMutex();
    if (result != SQL_SUCCESS && result != SQL_SUCCESS_WITH_INFO) {
      *((SQLUINTEGER *)num_attr) = SQL_CD_TRUE;
    } else
      *((SQLUINTEGER *)num_attr) = SQL_CD_FALSE;
    break;

  case SQL_ATTR_CONNECTION_TIMEOUT:
    /* We don't support this option, so it is always 0. */
    *((SQLUINTEGER *)num_attr)= 0;
    break;

  case SQL_ATTR_CURRENT_CATALOG:

    // We do a little test
    result = dbc->getQueryMutex();
    if (result != SQL_SUCCESS && result != SQL_SUCCESS_WITH_INFO) return result;

    pair = dbc->send_query_and_read("/current_db");
    result = pair.first;
    current_db_output = pair.second;
    if (result != SQL_SUCCESS && result != SQL_SUCCESS_WITH_INFO) {
      dbc->releaseQueryMutex();
      return result;
    }

    result = dbc->releaseQueryMutex();

    if (result != SQL_SUCCESS && result != SQL_SUCCESS_WITH_INFO) {
      return result;
    }

    current_db = getLines(current_db_output)[0];

    *char_attr = str_to_sqlcharptr(current_db);
    *((SQLUINTEGER *)num_attr) = 1;
    break;

  case SQL_ATTR_LOGIN_TIMEOUT:
    return dbc->set_error("HYC00",
                          "Unsupported option due to DES' characteristics");
    break;

  case SQL_ATTR_ODBC_CURSORS:
    if (dbc->ds.opt_FORWARD_CURSOR)
      *((SQLUINTEGER *)num_attr)= SQL_CUR_USE_ODBC;
    else
      *((SQLUINTEGER *)num_attr)= SQL_CUR_USE_IF_NEEDED;
    break;

  case SQL_ATTR_PACKET_SIZE:
    return dbc->set_error("HYC00",
                          "Unsupported option due to DES' characteristics");
    break;

  case SQL_ATTR_TXN_ISOLATION:
    return dbc->set_error("HYC00",
                          "Unsupported option due to DES' characteristics");
    break;

  default:
    return set_handle_error(SQL_HANDLE_DBC, hdbc, "HY092", "Invalid attribute");
  }

  return result;
}


/* DESODBC:

    Renamed from the original MySQLSetStmtAttr.

    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : myodbc3 internal
  @purpose : sets the statement attributes
*/
SQLRETURN SQL_API
DESSetStmtAttr(SQLHSTMT hstmt, SQLINTEGER Attribute, SQLPOINTER ValuePtr,
                 SQLINTEGER StringLengthPtr __attribute__((unused)))
{
    STMT *stmt= (STMT *)hstmt;
    SQLRETURN result= SQL_SUCCESS;
    STMT_OPTIONS *options= &stmt->stmt_options;

    CLEAR_STMT_ERROR(stmt);

    switch (Attribute)
    {
        case SQL_ATTR_CURSOR_SCROLLABLE:
            if (ValuePtr == (SQLPOINTER)SQL_NONSCROLLABLE &&
                options->cursor_type != SQL_CURSOR_FORWARD_ONLY)
                options->cursor_type= SQL_CURSOR_FORWARD_ONLY;

            else if (ValuePtr == (SQLPOINTER)SQL_SCROLLABLE &&
                     options->cursor_type == SQL_CURSOR_FORWARD_ONLY)
                options->cursor_type= SQL_CURSOR_STATIC;
            break;

        case SQL_ATTR_APP_PARAM_DESC:
        case SQL_ATTR_APP_ROW_DESC:
            {
              DESC *desc= (DESC *) ValuePtr;
              DESC **dest= NULL;
              desc_desc_type desc_type;

              if (Attribute == SQL_ATTR_APP_PARAM_DESC)
              {
                dest= &stmt->apd;
                desc_type= DESC_PARAM;
              }
              else if (Attribute == SQL_ATTR_APP_ROW_DESC)
              {
                dest= &stmt->ard;
                desc_type= DESC_ROW;
              }

              (*dest)->stmt_list_remove(stmt);

              /* reset to implicit if null */
              if (desc == SQL_NULL_HANDLE)
              {
                if (Attribute == SQL_ATTR_APP_PARAM_DESC)
                  stmt->apd= stmt->imp_apd;
                else if (Attribute == SQL_ATTR_APP_ROW_DESC)
                  stmt->ard= stmt->imp_ard;
                break;
              }

              if (desc->alloc_type == SQL_DESC_ALLOC_AUTO &&
                  desc->stmt != stmt)
                return ((STMT *)hstmt)
                    ->set_error("HY017",
                                 "Invalid use of an automatically allocated "
                                 "descriptor handle");

              if (desc->alloc_type == SQL_DESC_ALLOC_USER &&
                  stmt->dbc != desc->dbc)
                return ((STMT *)hstmt)
                    ->set_error("HY024",
                                 "Invalid attribute value");

              if (desc->desc_type != DESC_UNKNOWN &&
                  desc->desc_type != desc_type)
              {
                return ((STMT *)hstmt)
                    ->set_error("HY024",
                                 "Descriptor type mismatch");
              }

              assert(desc);
              assert(dest);

              desc->stmt_list_add(stmt);

              desc->desc_type= desc_type;
              *dest= desc;
            }
            break;

        case SQL_ATTR_AUTO_IPD:
        case SQL_ATTR_ENABLE_AUTO_IPD:
            if (ValuePtr != (SQLPOINTER)SQL_FALSE)
            return ((STMT *)hstmt)
                ->set_error("HYC00",
                                 "Optional feature not implemented");
            break;

        case SQL_ATTR_IMP_PARAM_DESC:
        case SQL_ATTR_IMP_ROW_DESC:
          return ((STMT *)hstmt)
              ->set_error("HY024",
                             "Invalid attribute/option identifier");

        case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
            return stmt_SQLSetDescField(stmt, stmt->apd, 0,
                                        SQL_DESC_BIND_OFFSET_PTR,
                                        ValuePtr, SQL_IS_POINTER);

        case SQL_ATTR_PARAM_BIND_TYPE:
            return stmt_SQLSetDescField(stmt, stmt->apd, 0,
                                        SQL_DESC_BIND_TYPE,
                                        ValuePtr, SQL_IS_INTEGER);

        case SQL_ATTR_PARAM_OPERATION_PTR: /* need to support this ....*/
            return stmt_SQLSetDescField(stmt, stmt->apd, 0,
                                        SQL_DESC_ARRAY_STATUS_PTR,
                                        ValuePtr, SQL_IS_POINTER);

        case SQL_ATTR_PARAM_STATUS_PTR: /* need to support this ....*/
            return stmt_SQLSetDescField(stmt, stmt->ipd, 0,
                                        SQL_DESC_ARRAY_STATUS_PTR,
                                        ValuePtr, SQL_IS_POINTER);

        case SQL_ATTR_PARAMS_PROCESSED_PTR: /* need to support this ....*/
            return stmt_SQLSetDescField(stmt, stmt->ipd, 0,
                                        SQL_DESC_ROWS_PROCESSED_PTR,
                                        ValuePtr, SQL_IS_POINTER);

        case SQL_ATTR_PARAMSET_SIZE:
            return stmt_SQLSetDescField(stmt, stmt->apd, 0,
                                        SQL_DESC_ARRAY_SIZE,
                                        ValuePtr, SQL_IS_ULEN);


        case SQL_ATTR_ROW_ARRAY_SIZE:
        case SQL_ROWSET_SIZE:
            return stmt_SQLSetDescField(stmt, stmt->ard, 0,
                                        SQL_DESC_ARRAY_SIZE,
                                        ValuePtr, SQL_IS_ULEN);

        case SQL_ATTR_ROW_BIND_OFFSET_PTR:
            return stmt_SQLSetDescField(stmt, stmt->ard, 0,
                                        SQL_DESC_BIND_OFFSET_PTR,
                                        ValuePtr, SQL_IS_POINTER);

        case SQL_ATTR_ROW_BIND_TYPE:
            return stmt_SQLSetDescField(stmt, stmt->ard, 0,
                                        SQL_DESC_BIND_TYPE,
                                        ValuePtr, SQL_IS_INTEGER);

        case SQL_ATTR_ROW_NUMBER:
          return ((STMT *)hstmt)
              ->set_error("HY092",
                             "Trying to set read-only attribute");

        case SQL_ATTR_ROW_OPERATION_PTR:
            return stmt_SQLSetDescField(stmt, stmt->ard, 0,
                                        SQL_DESC_ARRAY_STATUS_PTR,
                                        ValuePtr, SQL_IS_POINTER);

        case SQL_ATTR_ROW_STATUS_PTR:
            return stmt_SQLSetDescField(stmt, stmt->ird, 0,
                                        SQL_DESC_ARRAY_STATUS_PTR,
                                        ValuePtr, SQL_IS_POINTER);

        case SQL_ATTR_ROWS_FETCHED_PTR:
            return stmt_SQLSetDescField(stmt, stmt->ird, 0,
                                        SQL_DESC_ROWS_PROCESSED_PTR,
                                        ValuePtr, SQL_IS_POINTER);

        case SQL_ATTR_SIMULATE_CURSOR:
            options->simulateCursor= (SQLUINTEGER)(SQLULEN)ValuePtr;
            break;

            /*
              3.x driver doesn't support any statement attributes
              at connection level, but to make sure all 2.x apps
              works fine...lets support it..nothing to lose..
            */
        default:
            result= set_constmt_attr(3,hstmt,options,
                                     Attribute,ValuePtr);
    }
    return result;
}


/* DESODBC:
    Renamed from the original MySQLGetStmtAttr
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : myodbc3 internal
  @purpose : returns the statement attribute values
*/
SQLRETURN SQL_API
DESGetStmtAttr(SQLHSTMT hstmt, SQLINTEGER Attribute, SQLPOINTER ValuePtr,
                 SQLINTEGER BufferLength __attribute__((unused)),
                 SQLINTEGER *StringLengthPtr)
{
    SQLRETURN result= SQL_SUCCESS;
    STMT *stmt= (STMT *) hstmt;
    STMT_OPTIONS *options= &stmt->stmt_options;
    SQLINTEGER vparam= 0;
    SQLINTEGER len;

    if (!ValuePtr)
        ValuePtr= &vparam;

    if (!StringLengthPtr)
        StringLengthPtr= &len;

    switch (Attribute)
    {
        case SQL_ATTR_CURSOR_SCROLLABLE:
            if (options->cursor_type == SQL_CURSOR_FORWARD_ONLY)
                *(SQLUINTEGER*)ValuePtr= SQL_NONSCROLLABLE;
            else
                *(SQLUINTEGER*)ValuePtr= SQL_SCROLLABLE;
            break;

        case SQL_ATTR_AUTO_IPD:
            *(SQLUINTEGER *)ValuePtr= SQL_FALSE;
            break;

        case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
            *(SQLPOINTER *)ValuePtr= stmt->apd->bind_offset_ptr;
            break;

        case SQL_ATTR_PARAM_BIND_TYPE:
            *(SQLINTEGER *)ValuePtr= stmt->apd->bind_type;
            break;

        case SQL_ATTR_PARAM_OPERATION_PTR: /* need to support this ....*/
            *(SQLPOINTER *)ValuePtr= stmt->apd->array_status_ptr;
            break;

        case SQL_ATTR_PARAM_STATUS_PTR: /* need to support this ....*/
            *(SQLPOINTER *)ValuePtr= stmt->ipd->array_status_ptr;
            break;

        case SQL_ATTR_PARAMS_PROCESSED_PTR: /* need to support this ....*/
            *(SQLPOINTER *)ValuePtr= stmt->ipd->rows_processed_ptr;
            break;

        case SQL_ATTR_PARAMSET_SIZE:
            *(SQLUINTEGER *)ValuePtr = (SQLUINTEGER)stmt->apd->array_size;
            break;

        case SQL_ATTR_ROW_ARRAY_SIZE:
        case SQL_ROWSET_SIZE:
            *(SQLUINTEGER *)ValuePtr = (SQLUINTEGER)stmt->ard->array_size;
            break;

        case SQL_ATTR_ROW_BIND_OFFSET_PTR:
            *((SQLPOINTER *) ValuePtr)= stmt->ard->bind_offset_ptr;
            break;

        case SQL_ATTR_ROW_BIND_TYPE:
            *((SQLINTEGER *) ValuePtr)= stmt->ard->bind_type;
            break;

        case SQL_ATTR_ROW_NUMBER:
            *(SQLUINTEGER *)ValuePtr= stmt->current_row+1;
            break;

        case SQL_ATTR_ROW_OPERATION_PTR: /* need to support this ....*/
            *(SQLPOINTER *)ValuePtr= stmt->ard->array_status_ptr;
            break;

        case SQL_ATTR_ROW_STATUS_PTR:
            *(SQLPOINTER *)ValuePtr= stmt->ird->array_status_ptr;
            break;

        case SQL_ATTR_ROWS_FETCHED_PTR:
            *(SQLPOINTER *)ValuePtr= stmt->ird->rows_processed_ptr;
            break;

        case SQL_ATTR_SIMULATE_CURSOR:
            *(SQLUINTEGER *)ValuePtr= options->simulateCursor;
            break;

        case SQL_ATTR_APP_ROW_DESC:
            *(SQLPOINTER *)ValuePtr= stmt->ard;
            *StringLengthPtr= sizeof(SQLPOINTER);
            break;

        case SQL_ATTR_IMP_ROW_DESC:
            *(SQLPOINTER *)ValuePtr= stmt->ird;
            *StringLengthPtr= sizeof(SQLPOINTER);
            break;

        case SQL_ATTR_APP_PARAM_DESC:
            *(SQLPOINTER *)ValuePtr= stmt->apd;
            *StringLengthPtr= sizeof(SQLPOINTER);
            break;

        case SQL_ATTR_IMP_PARAM_DESC:
            *(SQLPOINTER *)ValuePtr= stmt->ipd;
            *StringLengthPtr= sizeof(SQLPOINTER);
            break;

            /*
              3.x driver doesn't support any statement attributes
              at connection level, but to make sure all 2.x apps
              works fine...lets support it..nothing to lose..
            */
        default:
            result= get_constmt_attr(3,hstmt,options,
                                     Attribute,ValuePtr,
                                     StringLengthPtr);
    }

    return result;
}


/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : ODBC 3.0 API
  @purpose : sets the environment attributes
*/
SQLRETURN SQL_API
SQLSetEnvAttr(SQLHENV    henv,
              SQLINTEGER Attribute,
              SQLPOINTER ValuePtr,
              SQLINTEGER StringLength __attribute__((unused)))
{
  CHECK_HANDLE(henv);

  if (((ENV *)henv)->has_connections())
    return ((ENV *)henv)
        ->set_error("HY010", "There exists open connections");

  switch (Attribute)
  {
      case SQL_ATTR_ODBC_VERSION:
        {
          switch((SQLINTEGER)(SQLLEN)ValuePtr)
          {
          case SQL_OV_ODBC2:
          case SQL_OV_ODBC3:
#ifndef USE_IODBC
          case SQL_OV_ODBC3_80:
#endif
            ((ENV *)henv)->odbc_ver= (SQLINTEGER)(SQLLEN)ValuePtr;
            break;
          default:
            return ((ENV *)henv)
                ->set_error("HY024", "Invalid attribute");
          }
          break;
        }
      case SQL_ATTR_OUTPUT_NTS:
          if (ValuePtr == (SQLPOINTER)SQL_TRUE)
              break;

      default:
        return ((ENV *)henv)->set_error("HYC00", "Option not appliable for DESODBC");
  }
  return SQL_SUCCESS;
}


/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : ODBC 3.0 API
  @purpose : returns the environment attributes
*/
SQLRETURN SQL_API
SQLGetEnvAttr(SQLHENV    henv,
              SQLINTEGER Attribute,
              SQLPOINTER ValuePtr,
              SQLINTEGER BufferLength __attribute__((unused)),
              SQLINTEGER *StringLengthPtr __attribute__((unused)))
{
    CHECK_HANDLE(henv);
    /* NULL is acceptable for ValuePtr, so we are not checking for it here */

    switch ( Attribute )
    {
        case SQL_ATTR_ODBC_VERSION:
            IF_NOT_NULL(ValuePtr, *(SQLINTEGER*)ValuePtr= ((ENV *)henv)->odbc_ver);
            break;

        case SQL_ATTR_OUTPUT_NTS:
            IF_NOT_NULL(ValuePtr, *((SQLINTEGER*)ValuePtr)= SQL_TRUE);
            break;

        default:
          return ((ENV *)henv)
              ->set_error("HYC00", "Option not appliable for DESODBC");
    }
    return SQL_SUCCESS;
}

#ifdef USE_IODBC

// iODBC has problems mapping SQLGetStmtOption()/SQLSetStmtOption() to
// SQLGetStmtAttr()/SQLSetStmtAttr()

SQLRETURN SQL_API
SQLGetStmtOption(SQLHSTMT hstmt,SQLUSMALLINT option, SQLPOINTER param)
{
  LOCK_STMT(hstmt);

  return DESGetStmtAttr(hstmt, option, param, SQL_NTS, (SQLINTEGER *)NULL);
}


SQLRETURN SQL_API
SQLSetStmtOption(SQLHSTMT hstmt, SQLUSMALLINT option, SQLULEN param)
{
  LOCK_STMT(hstmt);

  return DESSetStmtAttr(hstmt, option, (SQLPOINTER)param, SQL_NTS);
}

#endif

