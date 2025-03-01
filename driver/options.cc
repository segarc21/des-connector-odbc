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
  @file  options.c
  @brief Functions for handling handle attributes and options.
*/

#include "driver.h"
#include "errmsg.h"

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
                return set_handle_error(HandleType,Handle,DESERR_01S02,
                                        "Doesn't support asynchronous, changed to default",0);
            break;

        case SQL_ATTR_CURSOR_SENSITIVITY:
            if (ValuePtr != (SQLPOINTER) SQL_UNSPECIFIED)
            {
                return set_handle_error(HandleType,Handle,DESERR_01S02,
                                        "Option value changed to default cursor sensitivity(unspecified)",0);
            }
            break;

        case SQL_ATTR_CURSOR_TYPE:
            if (((STMT *)Handle)->dbc->ds.opt_FORWARD_CURSOR)
            {
                options->cursor_type= SQL_CURSOR_FORWARD_ONLY;
                if (ValuePtr != (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY)
                    return set_handle_error(HandleType,Handle,DESERR_01S02,
                                            "Forcing the use of forward-only cursor)",0);
            }
            else if (((STMT *)Handle)->dbc->ds.opt_DYNAMIC_CURSOR)
            {
                if (ValuePtr != (SQLPOINTER)SQL_CURSOR_KEYSET_DRIVEN)
                    options->cursor_type= (SQLUINTEGER)(SQLULEN)ValuePtr;

                else
                {
                    options->cursor_type= SQL_CURSOR_STATIC;
                    return set_handle_error(HandleType,Handle,DESERR_01S02,
                                            "Option value changed to default static cursor",0);
                }
            }
            else
            {
                if (ValuePtr == (SQLPOINTER)SQL_CURSOR_FORWARD_ONLY ||
                    ValuePtr == (SQLPOINTER)SQL_CURSOR_STATIC)
                    options->cursor_type= (SQLUINTEGER)(SQLULEN)ValuePtr;

                else
                {
                    options->cursor_type= SQL_CURSOR_STATIC;
                    return set_handle_error(HandleType,Handle,DESERR_01S02,
                                            "Option value changed to default static cursor",0);
                }
            }
            break;

        case SQL_ATTR_MAX_LENGTH:
            options->max_length= (SQLULEN) ValuePtr;
            break;

        case SQL_ATTR_MAX_ROWS:
            options->max_rows= (SQLULEN) ValuePtr;
            break;

        case SQL_ATTR_METADATA_ID:
            if (ValuePtr == (SQLPOINTER) SQL_TRUE)
                return set_handle_error(HandleType,Handle,DESERR_01S02,
                                        "Doesn't support SQL_ATTR_METADATA_ID to true, changed to default",0);
            break;

        case SQL_ATTR_RETRIEVE_DATA:
            options->retrieve_data = (ValuePtr != (SQLPOINTER)SQL_RD_OFF);
            break;

        case SQL_ATTR_SIMULATE_CURSOR:
            if (ValuePtr != (SQLPOINTER) SQL_SC_TRY_UNIQUE)
                return set_handle_error(HandleType,Handle,DESERR_01S02,
                                        "Option value changed to default cursor simulation",0);
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
            /* Do something only if the handle is STMT */
          return SQL_ERROR; //TODO: handle appropriately
            break;

        case SQL_ATTR_KEYSET_SIZE:
        case SQL_ATTR_CONCURRENCY:
        case SQL_ATTR_NOSCAN:
        default:
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
            *((SQLUINTEGER *) ValuePtr)= SQL_FALSE;
            break;

        case SQL_ATTR_QUERY_TIMEOUT:
            /* Do something only if the handle is STMT */
          return SQL_ERROR; //TODO: handle appropriately
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


/*
  @type    : myodbc3 internal
  @purpose : sets the connection attributes
*/

SQLRETURN SQL_API
DESSetConnectAttr(SQLHDBC hdbc, SQLINTEGER Attribute,
                    SQLPOINTER ValuePtr, SQLINTEGER StringLengthPtr)
{
  DBC *dbc= (DBC *) hdbc;


  switch (Attribute)
  {
    case SQL_ATTR_ACCESS_MODE:
      break;

    case SQL_ATTR_AUTOCOMMIT:
      return SQL_ERROR; //TODO: handle correctly

    case SQL_ATTR_LOGIN_TIMEOUT:
      return SQL_ERROR;  // TODO: handle correctly
      break;

    case SQL_ATTR_CONNECTION_TIMEOUT:
      {
        /*
          We don't do anything with this, but we pretend that we did
          to be consistent with Microsoft SQL Server.
        */
        return SQL_SUCCESS;
      }
      break;

      /*
        If this is done before connect (I would say a function
        sequence but .NET IDE does this) then we store the value but
        it is quite likely that it will get replaced by DATABASE in
        a DSN or connect string.
      */
    case SQL_ATTR_CURRENT_CATALOG:
        //TODO: handle correctly
        return SQL_ERROR;
        break;


    case SQL_ATTR_ODBC_CURSORS:
      if (dbc->ds.opt_FORWARD_CURSOR &&
        ValuePtr != (SQLPOINTER) SQL_CUR_USE_ODBC)
        return ((DBC*)hdbc)->set_error(DESERR_01S02,
          "Forcing the Driver Manager to use ODBC cursor library",0);
      break;

    case SQL_OPT_TRACE:
    case SQL_OPT_TRACEFILE:
    case SQL_QUIET_MODE:
    case SQL_TRANSLATE_DLL:
    case SQL_TRANSLATE_OPTION:
      {
        char buff[100];
        desodbc_snprintf(buff, sizeof(buff),
                        "Suppose to set this attribute '%d' through driver "
                        "manager, not by the driver",
                        (int)Attribute);
        return ((DBC*)hdbc)->set_error(DESERR_01S02, buff, 0);
      }

    case SQL_ATTR_PACKET_SIZE:
      break;

    case SQL_ATTR_TXN_ISOLATION:
      return SQL_ERROR; //TODO: handle correctly
      break;

#ifndef USE_IODBC
    case SQL_ATTR_RESET_CONNECTION:
      if (ValuePtr != (SQLPOINTER)SQL_RESET_CONNECTION_YES)
      {
        return dbc->set_error( "HY024", "Invalid attribute value", 0);
      }
      /* TODO 3.8 feature */
      reset_connection(dbc);
      dbc->need_to_wakeup= 1;

      return SQL_SUCCESS;
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
      return dbc->set_error( "HYC00",
                           "Optional feature not supported", 0);

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

  /* (Windows) DM checks SQL_ATTR_CONNECTION_DEAD before taking it from the pool and returning to the
     application. We can use wake-up procedure for diagnostics of whether connection is alive instead
     of mysql_ping(). But we are leaving this check for other attributes, too */

  switch (attrib)
  {
  case SQL_ATTR_ACCESS_MODE:
    *((SQLUINTEGER *)num_attr)= SQL_MODE_READ_WRITE;
    break;

  case SQL_ATTR_AUTO_IPD:
    *((SQLUINTEGER *)num_attr)= SQL_FALSE;
    break;

  case SQL_ATTR_AUTOCOMMIT:
    return SQL_ERROR; //TODO: handle correctly
    break;

  case SQL_ATTR_CONNECTION_DEAD:
    return SQL_ERROR; //TODO: handle correctly
    break;

  case SQL_ATTR_CONNECTION_TIMEOUT:
    /* We don't support this option, so it is always 0. */
    *((SQLUINTEGER *)num_attr)= 0;
    break;

  case SQL_ATTR_CURRENT_CATALOG:
    return SQL_ERROR; //TODO: check
    break;

  case SQL_ATTR_LOGIN_TIMEOUT:
    *((SQLUINTEGER *)num_attr)= dbc->login_timeout;
    break;

  case SQL_ATTR_ODBC_CURSORS:
    if (dbc->ds.opt_FORWARD_CURSOR)
      *((SQLUINTEGER *)num_attr)= SQL_CUR_USE_ODBC;
    else
      *((SQLUINTEGER *)num_attr)= SQL_CUR_USE_IF_NEEDED;
    break;

  case SQL_ATTR_PACKET_SIZE:
    return SQL_ERROR; //TODO: handle correctly
    break;

  case SQL_ATTR_TXN_ISOLATION:
    return SQL_ERROR; //TODO: handle correctly
    break;

  default:
    return set_handle_error(SQL_HANDLE_DBC, hdbc, DESERR_S1092, NULL, 0);
  }

  return result;
}


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
                return ((STMT*)hstmt)->set_error(DESERR_S1017,
                                 "Invalid use of an automatically allocated "
                                 "descriptor handle");

              if (desc->alloc_type == SQL_DESC_ALLOC_USER &&
                  stmt->dbc != desc->dbc)
                return ((STMT*)hstmt)->set_error(DESERR_S1024,
                                 "Invalid attribute value");

              if (desc->desc_type != DESC_UNKNOWN &&
                  desc->desc_type != desc_type)
              {
                return ((STMT*)hstmt)->set_error(DESERR_S1024,
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
                return ((STMT*)hstmt)->set_error(DESERR_S1C00,
                                 "Optional feature not implemented");
            break;

        case SQL_ATTR_IMP_PARAM_DESC:
        case SQL_ATTR_IMP_ROW_DESC:
            return ((STMT*)hstmt)->set_error(DESERR_S1024,
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
            return ((STMT*)hstmt)->set_error(DESERR_S1000,
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
      return set_env_error((ENV*)henv, DESERR_S1010, NULL, 0);

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
            return set_env_error((ENV*)henv,DESERR_S1024,NULL,0);
          }
          break;
        }
      case SQL_ATTR_OUTPUT_NTS:
          if (ValuePtr == (SQLPOINTER)SQL_TRUE)
              break;

      default:
          return set_env_error((ENV*)henv,DESERR_S1C00,NULL,0);
  }
  return SQL_SUCCESS;
}


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
        case SQL_ATTR_CONNECTION_POOLING:
            IF_NOT_NULL(ValuePtr, *(SQLINTEGER*)ValuePtr = SQL_CP_ONE_PER_DRIVER);
            break;

        case SQL_ATTR_ODBC_VERSION:
            IF_NOT_NULL(ValuePtr, *(SQLINTEGER*)ValuePtr= ((ENV *)henv)->odbc_ver);
            break;

        case SQL_ATTR_OUTPUT_NTS:
            IF_NOT_NULL(ValuePtr, *((SQLINTEGER*)ValuePtr)= SQL_TRUE);
            break;

        default:
            return set_env_error((ENV*)henv,DESERR_S1C00,NULL,0);
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

