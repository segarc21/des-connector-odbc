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
  @file  results.c
  @brief Result set and related information functions.
*/

#include "driver.h"
#include <errmsg.h>
#include <ctype.h>
#include <locale.h>

#define SQL_MY_PRIMARY_KEY 1212



/* Verifies if C type is suitable for copying SQL_BINARY data
   http://msdn.microsoft.com/en-us/library/ms713559%28VS.85%29.aspx */
my_bool is_binary_ctype( SQLSMALLINT cType)
{
  return (cType == SQL_C_CHAR
       || cType == SQL_C_BINARY
       || cType == SQL_C_WCHAR);
}

/* Function that verifies if conversion from given sql type to c type supported.
   Based on http://msdn.microsoft.com/en-us/library/ms709280%28VS.85%29.aspx
   and underlying pages.
   Currently checks conversions for MySQL BIT(n) field(SQL_BIT or SQL_BINARY)
*/
my_bool odbc_supported_conversion(SQLSMALLINT sqlType, SQLSMALLINT cType)
{
  switch (sqlType)
  {
  case SQL_BIT:
    {
      switch (cType)
      {
      case SQL_C_DATE:
      case SQL_C_TYPE_DATE:
      case SQL_C_TIME:
      case SQL_C_TYPE_TIME:
      case SQL_C_TIMESTAMP:
      case SQL_C_TYPE_TIMESTAMP:
        return FALSE;
      }
    }
  case SQL_BINARY:
    {
      return is_binary_ctype(cType);
    }
  }

  return TRUE;
}


 /* Conversion supported by driver as exception to odbc specs
    (i.e. to odbc_supported_conversion() results).
    e.g. we map bit(n>1) to SQL_BINARY, but provide its conversion to numeric
    types */
 my_bool driver_supported_conversion(DES_FIELD * field, SQLSMALLINT cType)
 {
   switch(field->type)
   {
   case DES_TYPE_STRING:
     {
       switch (cType)
       {
       /* Date string is(often) identified as binary data by the driver.
          Have to either add exception here, or to change the way we detect if
          field is binary.
       */
       case SQL_C_TIMESTAMP:
       case SQL_C_DATE:
       case SQL_C_TIME:
       case SQL_C_TYPE_TIMESTAMP:
       case SQL_C_TYPE_DATE:
       case SQL_C_TYPE_TIME:

         return TRUE;
       }
     }
   }

   return FALSE;
 }


 SQLUSMALLINT sqlreturn2row_status(SQLRETURN res)
 {
   switch (res)
   {
     case SQL_SUCCESS:            return SQL_ROW_SUCCESS;
     case SQL_SUCCESS_WITH_INFO:  return SQL_ROW_SUCCESS_WITH_INFO;
   }

   return SQL_ROW_ERROR;
 }


/**
  Save bookmark value to specified buffer in specified ODBC C type.

  @param[in]  stmt        Handle of statement
  @param[in]  fCType      ODBC C type to return data as
  @param[out] rgbValue    Pointer to buffer for returning data
  @param[in]  cbValueMax  Length of buffer
  @param[out] pcbValue    Bytes used in the buffer, or SQL_NULL_DATA
  @param[out] value       Bookmark value
  @param[in]  length      Length of value
  @param[in]  arrec       ARD record for this column (can be NULL)
*/
SQLRETURN SQL_API
sql_get_bookmark_data(STMT *stmt, SQLSMALLINT fCType, uint column_number,
             SQLPOINTER rgbValue, SQLLEN cbValueMax, SQLLEN *pcbValue,
             char *value, ulong length, DESCREC *arrec)
{
  SQLLEN    tmp;
  SQLRETURN result= SQL_SUCCESS;

  if (cbValueMax < sizeof(long))
  {
    return stmt->set_error("HY090", "Invalid string or buffer length");
  }

  /* get the exact type if we don't already have it */
  if (fCType == SQL_C_DEFAULT)
  {
    fCType= SQL_C_VARBOOKMARK;

    if (!cbValueMax)
    {
      cbValueMax= bind_length(fCType, 0);
    }
  }
  else if (fCType == SQL_ARD_TYPE)
  {
    if (!arrec)
    {
      return stmt->set_error("07009", "Invalid descriptor index");
    }

    fCType= arrec->concise_type;
  }

  if (!pcbValue)
  {
    pcbValue= &tmp; /* Easier code */
  }

  switch (fCType)
  {
  case SQL_C_CHAR:
  case SQL_C_BINARY:
    {
      int ret;
      SQLCHAR *result_end;
      size_t copy_bytes;
      ret= copy_binary_result(stmt, (SQLCHAR *)rgbValue, cbValueMax,
                                pcbValue, NULL, value, length);
      if (SQL_SUCCEEDED(ret))
      {
        copy_bytes= desodbc_min((unsigned long)length, cbValueMax);
        result_end= (SQLCHAR *)rgbValue + copy_bytes;
        if (result_end)
          *result_end= 0;
      }
      return ret;
    }

  case SQL_C_WCHAR:
    {
      if(stmt->stmt_options.retrieve_data)
      {
        int ret;
        ret= utf8_as_sqlwchar((SQLWCHAR *)rgbValue,
                        (SQLINTEGER)(cbValueMax / sizeof(SQLWCHAR)),
                        (SQLCHAR *)value, length);
        if (!ret)
        {
          stmt->set_error("01004", "String data, right-truncated");
          return SQL_SUCCESS_WITH_INFO;
        }
      }

      if (pcbValue)
        *pcbValue= (SQLINTEGER)(cbValueMax / sizeof(SQLWCHAR));

    }

  case SQL_C_TINYINT:
  case SQL_C_STINYINT:
    if (rgbValue && stmt->stmt_options.retrieve_data)
      *((SQLSCHAR *)rgbValue)= (SQLSCHAR) get_int(stmt, column_number,
                                                  value, length);
    *pcbValue= 1;
    break;

  case SQL_C_UTINYINT:
    if (rgbValue && stmt->stmt_options.retrieve_data)
      *((SQLCHAR *)rgbValue)= (SQLCHAR)get_uint(stmt, column_number,
                                                value, length);
    *pcbValue= 1;
    break;

  case SQL_C_SHORT:
  case SQL_C_SSHORT:
    if (rgbValue && stmt->stmt_options.retrieve_data)
      *((SQLSMALLINT *)rgbValue)= (SQLSMALLINT) get_int(stmt, column_number,
                                                        value, length);
    *pcbValue= sizeof(SQLSMALLINT);
    break;

  case SQL_C_USHORT:
    if (rgbValue && stmt->stmt_options.retrieve_data)
      *((SQLUSMALLINT *)rgbValue)= (SQLUSMALLINT)get_uint(stmt, column_number,
                                                          value, length);
    *pcbValue= sizeof(SQLUSMALLINT);
    break;

  case SQL_C_LONG:
  case SQL_C_SLONG:
    if (rgbValue && stmt->stmt_options.retrieve_data)
    {
      if (length >= 10 && value[4] == '-' && value[7] == '-' &&
           (!value[10] || value[10] == ' '))
      {
        *((SQLINTEGER *)rgbValue)= ((SQLINTEGER) atol(value) * 10000L +
                                    (SQLINTEGER) atol(value + 5) * 100L +
                                    (SQLINTEGER) atol(value + 8));
      }
      else
        *((SQLINTEGER *)rgbValue)= (SQLINTEGER) get_int64(stmt,
                                              column_number, value, length);
    }
    *pcbValue= sizeof(SQLINTEGER);
    break;

  case SQL_C_ULONG:
    if (rgbValue && stmt->stmt_options.retrieve_data)
      *((SQLUINTEGER *)rgbValue) = (SQLUINTEGER) get_uint64(stmt, column_number,
                                                            value, length);
    *pcbValue= sizeof(SQLUINTEGER);
    break;

  case SQL_C_FLOAT:
    if (rgbValue && stmt->stmt_options.retrieve_data)
      *((float *)rgbValue)= (float) get_double(stmt, column_number,
                                               value, length);
    *pcbValue= sizeof(float);
    break;

  case SQL_C_DOUBLE:
    if (rgbValue && stmt->stmt_options.retrieve_data)
      *((double *)rgbValue)= (double) get_double(stmt, column_number,
                                                 value, length);
    *pcbValue= sizeof(double);
    break;

  case SQL_C_SBIGINT:
    /** @todo This is not right. SQLBIGINT is not always longlong. */
    if (rgbValue && stmt->stmt_options.retrieve_data)
      *((longlong *)rgbValue)= get_int64(stmt, column_number,
                                         value, length);
    *pcbValue= sizeof(longlong);
    break;

  case SQL_C_UBIGINT:
    /** @todo This is not right. SQLUBIGINT is not always ulonglong.  */
    if (rgbValue && stmt->stmt_options.retrieve_data)
        *((ulonglong *)rgbValue) = get_uint64(stmt, column_number,
                                                        value, length);
    *pcbValue= sizeof(ulonglong);
    break;

  default:
    return stmt->set_error("HY000",
                     "Restricted data type attribute violation");
    break;
  }

  if (stmt->getdata.source)  /* Second call to getdata */
    return SQL_NO_DATA_FOUND;

  return result;
}


char *fix_padding(STMT *stmt, SQLSMALLINT fCType, char *value, std::string &out_str,
              SQLLEN cbValueMax, ulong &data_len, DESCREC *irrec)
{
    if (stmt->dbc->ds.opt_PAD_SPACE &&
         (irrec->type == SQL_CHAR || irrec->type == SQL_WCHAR) &&
         (fCType == SQL_C_CHAR || fCType == SQL_C_WCHAR || fCType == SQL_C_BINARY)
       )
    {
      if (value)
        out_str = std::string(value, data_len);

      /* Calculate new data length with spaces */
      data_len = (ulong)(irrec->octet_length < cbValueMax ? irrec->octet_length : cbValueMax);

      out_str.resize(data_len, ' ');
      return (char*)out_str.c_str();
    }
    return value;
}


/**
  Retrieve the data from a field as a specified ODBC C type.

  TODO arrec->indicator_ptr could be different than pcbValue
  ideally, two separate pointers would be passed here

  @param[in]  stmt        Handle of statement
  @param[in]  fCType      ODBC C type to return data as
  @param[in]  field       Field describing the type of the data
  @param[out] rgbValue    Pointer to buffer for returning data
  @param[in]  cbValueMax  Length of buffer
  @param[out] pcbValue    Bytes used in the buffer, or SQL_NULL_DATA
  @param[out] value       The field data to be converted and returned
  @param[in]  length      Length of value
  @param[in]  arrec       ARD record for this column (can be NULL)
*/
SQLRETURN SQL_API
sql_get_data(STMT *stmt, SQLSMALLINT fCType, uint column_number,
             SQLPOINTER rgbValue, SQLLEN cbValueMax, SQLLEN *pcbValue,
             char *value, ulong length, DESCREC *arrec)
{
  DES_FIELD *field= des_fetch_field_direct(stmt->result, column_number);
  SQLLEN    tmp;
  long long numeric_value = 0;
  unsigned long long u_numeric_value = 0;
  my_bool   convert= 1;
  SQLRETURN result= SQL_SUCCESS;
  char      as_string[50]; /* Buffer that might be required to convert other
                              types data to its string representation */

  /* get the exact type if we don't already have it */
  if (fCType == SQL_C_DEFAULT)
  {
    fCType= unireg_to_c_datatype(field);

    if (!cbValueMax)
    {
      cbValueMax= bind_length(fCType, 0);
    }
  }
  else if (fCType == SQL_ARD_TYPE)
  {
    if (!arrec)
    {
      return stmt->set_error("07009", "Invalid descriptor index");
    }

    fCType= arrec->concise_type;
  }

  /* set prec and scale for numeric */
  if (fCType == SQL_C_NUMERIC && rgbValue)
  {
    SQL_NUMERIC_STRUCT *sqlnum= (SQL_NUMERIC_STRUCT *) rgbValue;

    if (arrec) /* normally set via ard */
    {
      sqlnum->precision= (SQLSCHAR) arrec->precision;
      sqlnum->scale= (SQLCHAR) arrec->scale;
    }
    else /* just take the defaults */
    {
      sqlnum->precision= 38;
      sqlnum->scale= 0;
    }
  }

  if (is_null(stmt, column_number, value))
  {
    /* pcbValue must be available if its NULL */
    if (!pcbValue)
    {
      return stmt->set_error("22002",
                            "Indicator variable required but not supplied");
    }
    *pcbValue = SQL_NULL_DATA;
  }
  else
  {
    if (!odbc_supported_conversion(get_sql_data_type(stmt, field, 0), fCType)
     && !driver_supported_conversion(field,fCType))
    {
      /*The state 07009 was incorrect
      (http://msdn.microsoft.com/en-us/library/ms715441%28v=VS.85%29.aspx)
      */
      return stmt->set_error("07006", "Conversion is not possible");
    }

    if (!pcbValue)
    {
      pcbValue= &tmp; /* Easier code */
    }

    switch (fCType)
    {
    case SQL_C_CHAR:
      /*
        Handle BLOB -> CHAR conversion
        Conversion only for field which is having binary character set (63)
      */
      /*
      if ((field->flags & BINARY_FLAG) &&
        field->charsetnr == BINARY_CHARSET_NUMBER &&
        IS_LONGDATA(field->type) &&
        !field->decimals)
      {
        return copy_binhex_result(stmt, (SQLCHAR *)rgbValue,
          (SQLINTEGER)cbValueMax, pcbValue, value, length);
      }
      */
      /* fall through */

    case SQL_C_BINARY:
      {
        char buff[21];
        /* It seems that server version needs to be checked along with length,
           as fractions can mess everything up. but on other hand i do not know
           how to get MYSQL_TYPE_TIMESTAMP with short format date in modern
           servers. That also makes me think we do not need to consider
           possibility of having fractional part in complete_timestamp() */
        if (field->type == DES_TYPE_TIMESTAMP && length < 19)
        {
          /* Convert MySQL timestamp to full ANSI timestamp format. */
          /*TODO if stlen_ind_ptr was not passed - error has to be returned */
          if (complete_timestamp(value, length, buff) == NULL)
          {
            *pcbValue= SQL_NULL_DATA;
            break;
          }

          value= buff;
          length= 19;
        }

        if (fCType == SQL_C_BINARY)
        {
          return copy_binary_result(stmt, (SQLCHAR *)rgbValue, cbValueMax,
                                    pcbValue, field, value, length);
        }
        else
        {
          char *tmp= get_string(stmt, column_number, value, &length, as_string);
          return copy_ansi_result(stmt,(SQLCHAR*)rgbValue, cbValueMax, pcbValue,
                                  field, tmp,length);
        }
      }

    case SQL_C_WCHAR:
      {
        char *tmp= get_string(stmt, column_number, value, &length, as_string);

        /*
        if ((field->flags & BINARY_FLAG) &&
          field->charsetnr == BINARY_CHARSET_NUMBER &&
          IS_LONGDATA(field->type) &&
          !field->decimals)
        {
          return copy_binhex_result(stmt, (SQLWCHAR*)rgbValue,
                  (SQLINTEGER)(cbValueMax / sizeof(SQLWCHAR)),
                  pcbValue, tmp, length);
        }
        */

        return copy_wchar_result(stmt, (SQLWCHAR *)rgbValue,
                        (SQLINTEGER)(cbValueMax / sizeof(SQLWCHAR)), pcbValue,
                        field, tmp, length);
      }

    case SQL_C_BIT:
      if (rgbValue)
      {
        /* for MySQL bit(n>1) 1st byte may be '\0'. So testing already converted
           to a number value or atoi for other types. */
        if (!convert)
        {
          *((char *)rgbValue)= numeric_value > 0 ? '\1' : '\0';
        }
        else
        {
          *((char *)rgbValue)= get_int(stmt, column_number, value, length) > 0 ?
                               '\1' : '\0';
        }
      }

      *pcbValue= 1;
      break;

    case SQL_C_TINYINT:
    case SQL_C_STINYINT:
      if (rgbValue)
        *((SQLSCHAR *)rgbValue)= (SQLSCHAR)(convert
                                 ? get_int(stmt, column_number, value, length)
                                 : (numeric_value & (SQLSCHAR)(-1)));
      *pcbValue= 1;
      break;

    case SQL_C_UTINYINT:
      if (rgbValue)
        *((SQLCHAR *)rgbValue)= (SQLCHAR)(convert
                                ? get_uint(stmt, column_number, value, length)
                                : (numeric_value & (SQLCHAR)(-1)));
      *pcbValue= 1;
      break;

    case SQL_C_SHORT:
    case SQL_C_SSHORT:
      if (rgbValue)
        *((SQLSMALLINT *)rgbValue)= (SQLSMALLINT)(convert
                                ? get_int(stmt, column_number, value, length)
                                : (numeric_value & (SQLUSMALLINT)(-1)));
      *pcbValue= sizeof(SQLSMALLINT);
      break;

    case SQL_C_USHORT:
      if (rgbValue)
        *((SQLUSMALLINT *)rgbValue)= (SQLUSMALLINT)(convert
                                ? get_uint(stmt, column_number, value, length)
                                : (numeric_value & (SQLUSMALLINT)(-1)));
      *pcbValue= sizeof(SQLUSMALLINT);
      break;

    case SQL_C_LONG:
    case SQL_C_SLONG:
      if (rgbValue)
      {
        /* Check if it could be a date...... :) */
        if (convert)
        {
          if (length >= 10 && value[4] == '-' && value[7] == '-' &&
               (!value[10] || value[10] == ' '))
          {
            *((SQLINTEGER *)rgbValue)= ((SQLINTEGER) atol(value) * 10000L +
                                        (SQLINTEGER) atol(value + 5) * 100L +
                                        (SQLINTEGER) atol(value + 8));
          }
          else
            *((SQLINTEGER *)rgbValue)= (SQLINTEGER) get_int64(stmt,
                                                  column_number, value, length);
        }
        else
          *((SQLINTEGER *)rgbValue)= (SQLINTEGER)(numeric_value
                                                  & (SQLUINTEGER)(-1));
      }
      *pcbValue= sizeof(SQLINTEGER);
      break;

    case SQL_C_ULONG:
      if (rgbValue)
        *((SQLUINTEGER *)rgbValue)= (SQLUINTEGER)(convert ?
                                get_uint64(stmt, column_number, value, length) :
                                numeric_value & (SQLUINTEGER)(-1));
      *pcbValue= sizeof(SQLUINTEGER);
      break;

    case SQL_C_FLOAT:
      if (rgbValue)
        *((float *)rgbValue)= (float)(convert ? get_double(stmt, column_number,
                                    value, length) : numeric_value & (int)(-1));
      *pcbValue= sizeof(float);
      break;

    case SQL_C_DOUBLE:
      if (rgbValue)
        *((double *)rgbValue)= (double)(convert ? get_double(stmt, column_number,
                                    value, length) : numeric_value);
      *pcbValue= sizeof(double);
      break;

    case SQL_C_DATE:
    case SQL_C_TYPE_DATE:
      {
        SQL_DATE_STRUCT tmp_date;
        char *tmp= get_string(stmt, column_number, value, &length, as_string);

        if (!rgbValue)
        {
          rgbValue= (char *)&tmp_date;
        }

        if (!str_to_date((SQL_DATE_STRUCT *)rgbValue, tmp, length,
                          stmt->dbc->ds.opt_ZERO_DATE_TO_MIN))
        {
          *pcbValue= sizeof(SQL_DATE_STRUCT);
        }
        else
        {
          /*TODO - again, error if pcbValue was originally NULL */
          *pcbValue= SQL_NULL_DATA;  /* ODBC can't handle 0000-00-00 dates */
        }

        break;
      }
    case SQL_C_INTERVAL_HOUR_TO_SECOND:
    case SQL_C_INTERVAL_HOUR_TO_MINUTE:
      {
        if (field->type= DES_TYPE_TIME)
        {
          SQL_TIME_STRUCT ts;
          char *tmp= get_string(stmt,
                            column_number, value, &length, as_string);
          if (str_to_time_st(&ts, tmp))
          {
            *pcbValue= SQL_NULL_DATA;
          }
          else
          {
            SQL_INTERVAL_STRUCT *interval= (SQL_INTERVAL_STRUCT *)rgbValue;
            memset(interval, 0, sizeof(SQL_INTERVAL_STRUCT));

            interval->interval_type= (SQLINTERVAL)fCType;
            interval->intval.day_second.hour= ts.hour;
            interval->intval.day_second.minute= ts.minute;

            if (fCType == SQL_C_INTERVAL_HOUR_TO_SECOND)
            {
              interval->intval.day_second.second= ts.second;
            }
            else if(ts.second > 0)
            {
              /* Truncation */
              stmt->set_error("01S07", NULL);
              result= SQL_SUCCESS_WITH_INFO;
            }

            *pcbValue= sizeof(SQL_INTERVAL_STRUCT);
          }
        }
        else
        {
          return stmt->set_error("HY000",
                       "Restricted data type attribute violation");
        }

        break;
      }
    case SQL_C_TIME:
    case SQL_C_TYPE_TIME:
      if (field->type == DES_TYPE_TIMESTAMP ||
          field->type == DES_TYPE_DATETIME)
      {
        SQL_TIMESTAMP_STRUCT ts;

        switch (str_to_ts(&ts, get_string(stmt, column_number, value, &length,
                          as_string), SQL_NTS, stmt->dbc->ds.opt_ZERO_DATE_TO_MIN,
                          TRUE))
        {
        case SQLTS_BAD_DATE:
          return stmt->set_error("22018", "Data value is not a valid time(stamp) value");
        case SQLTS_NULL_DATE:
          *pcbValue= SQL_NULL_DATA;
          break;
        default:
          {
            SQL_TIME_STRUCT *time_info= (SQL_TIME_STRUCT *)rgbValue;

            if (time_info)
            {
              time_info->hour=   ts.hour;
              time_info->minute= ts.minute;
              time_info->second= ts.second;

              if (ts.fraction > 0)
              {
                stmt->set_error("01S07", NULL);
                result= SQL_SUCCESS_WITH_INFO;
              }
            }
            *pcbValue= sizeof(TIME_STRUCT);
          }
        }
      }
      else if (field->type == DES_TYPE_DATE)
      {
        SQL_TIME_STRUCT *time_info= (SQL_TIME_STRUCT *)rgbValue;

        if (time_info)
        {
          time_info->hour=   0;
          time_info->minute= 0;
          time_info->second= 0;
        }
        *pcbValue= sizeof(TIME_STRUCT);
      }
      else
      {
        SQL_TIME_STRUCT ts;
        char *tmp= get_string(stmt,
                            column_number, value, &length, as_string);
        if (str_to_time_st(&ts, tmp))
        {
          *pcbValue= SQL_NULL_DATA;
        }
        else
        {
          SQL_TIME_STRUCT *time_info= (SQL_TIME_STRUCT *)rgbValue;
          SQLUINTEGER fraction;

          if (ts.hour > 23)
          {
            return stmt->set_error("22007",
                           "Invalid time(hours) format. Use interval types instead");
          }
          if (time_info)
          {
            time_info->hour=   ts.hour;
            time_info->minute= ts.minute;
            time_info->second= ts.second;
          }

          *pcbValue= sizeof(TIME_STRUCT);

          get_fractional_part(tmp, SQL_NTS, TRUE, &fraction);

          if (fraction)
          {
            /* http://msdn.microsoft.com/en-us/library/ms713346%28v=VS.85%29.aspx
               We are loosing fractional part - thus we have to set correct sqlstate
               and return SQL_SUCCESS_WITH_INFO */
            stmt->set_error("01S07", NULL);
            result= SQL_SUCCESS_WITH_INFO;
          }
        }
      }
      break;

    case SQL_C_TIMESTAMP:
    case SQL_C_TYPE_TIMESTAMP:
      {
      char *tmp= get_string(stmt, column_number, value, &length, as_string);

      if (field->type == DES_TYPE_TIME)
      {
        SQL_TIME_STRUCT ts;

        if (str_to_time_st(&ts, tmp))
        {
          *pcbValue= SQL_NULL_DATA;
        }
        else
        {
          SQL_TIMESTAMP_STRUCT *timestamp_info=
            (SQL_TIMESTAMP_STRUCT *)rgbValue;
          time_t sec_time= time(NULL);
          struct tm cur_tm;

          if (ts.hour > 23)
          {
            sec_time+= (ts.hour/24)*24*60*60;
            ts.hour= ts.hour%24;
          }

          localtime_r(&sec_time, &cur_tm);

          /* I wornder if that hasn't to be server current date*/
          timestamp_info->year=   1900 + cur_tm.tm_year;
          timestamp_info->month=  1 + cur_tm.tm_mon; /* January is 0 in tm */
          timestamp_info->day=    cur_tm.tm_mday;
          timestamp_info->hour=   ts.hour;
          timestamp_info->minute= ts.minute;
          timestamp_info->second= ts.second;
          /* Fractional seconds must be 0 no matter what is actually in the field */
          timestamp_info->fraction= 0;
          *pcbValue= sizeof(SQL_TIMESTAMP_STRUCT);
        }
      }
      else
      {
        switch (str_to_ts((SQL_TIMESTAMP_STRUCT *)rgbValue, tmp, SQL_NTS,
                      stmt->dbc->ds.opt_ZERO_DATE_TO_MIN, TRUE))
        {
        case SQLTS_BAD_DATE:
          return stmt->set_error("22018", "Data value is not a valid date/time(stamp) value");

        case SQLTS_NULL_DATE:
          *pcbValue= SQL_NULL_DATA;
          break;

        default:
          *pcbValue= sizeof(SQL_TIMESTAMP_STRUCT);
        }
      }
      break;
      }

    case SQL_C_SBIGINT:
      /** @todo This is not right. SQLBIGINT is not always longlong. */
      if (rgbValue)
        *((longlong *)rgbValue)= (longlong)(convert ? get_int64(stmt,
                        column_number, value, length) : numeric_value);

      *pcbValue= sizeof(longlong);
      break;

    case SQL_C_UBIGINT:
      /** @todo This is not right. SQLUBIGINT is not always ulonglong.  */
      if (rgbValue)
          *((ulonglong *)rgbValue)= (ulonglong)(convert ? get_uint64(stmt,
                        column_number, value, length) : u_numeric_value);

      *pcbValue= sizeof(ulonglong);
      break;

    case SQL_C_NUMERIC:
      {
        int overflow= 0;
        SQL_NUMERIC_STRUCT *sqlnum= (SQL_NUMERIC_STRUCT *) rgbValue;

        if (rgbValue)
        {
          if (convert)
          {
            sqlnum_from_str(get_string(stmt, column_number, value, &length, as_string), sqlnum, &overflow);
            *pcbValue = sizeof(SQL_NUMERIC_STRUCT);
          }
          else /* bit field */
          {
            /* Lazy way - converting number we have to a string.
               If it couldn't happen we have to scale/unscale number - we would
               just reverse binary data */
            std::string _value;
            if (numeric_value)
              _value = std::to_string(numeric_value);
            else
              _value = std::to_string(u_numeric_value);

            sqlnum_from_str(_value.c_str(), sqlnum, &overflow);
            *pcbValue = sizeof(ulonglong);
          }

        }

        if (overflow == 1)
          return stmt->set_error("22003",
                                "Numeric value out of range");
        else if (overflow == 2)
        {
          stmt->set_error("01S07", "Fractional truncation");
          result = SQL_SUCCESS_WITH_INFO;
        }
      }
      break;

    default:
        return stmt->set_error("HY000",
                       "Restricted data type attribute violation");
      break;
    }
  }

  if (stmt->getdata.source)  /* Second call to getdata */
    return SQL_NO_DATA_FOUND;

  return result;
}


/*
  @type    : myodbc3 internal
  @purpose : execute the query if it is only prepared. This is needed
  because the ODBC standard allows calling some functions
  before SQLExecute().
*/

static SQLRETURN check_result(STMT *stmt)
{
  SQLRETURN error= 0;

  switch (stmt->state)
  {
    case ST_UNKNOWN:
      error= stmt->set_error("24000","Invalid cursor state");
      break;
    case ST_PREPARED:
      /*TODO: introduce state for statements prepared on the server side */
      if (stmt_returns_result(&stmt->query))
      {
        SQLULEN real_max_rows= stmt->stmt_options.max_rows;
        stmt->stmt_options.max_rows= 1;
        /* select limit will be restored back to max_rows before real execution */
        if ( (error= DES_SQLExecute(stmt)) == SQL_SUCCESS )
        {
          stmt->state= ST_PRE_EXECUTED;  /* mark for execute */
        }
        else
        {
          //set_sql_select_limit(stmt->dbc, real_max_rows, TRUE);
        }
        stmt->stmt_options.max_rows= real_max_rows;
      }
      else
        error = SQL_SUCCESS;
      break;
    case ST_PRE_EXECUTED:
    case ST_EXECUTED:
      error= SQL_SUCCESS;
  }
  return(error);
}

/*
  @type    : myodbc3 internal
  @purpose : does the any open param binding
*/

SQLRETURN do_dummy_parambind(SQLHSTMT hstmt)
{
    SQLRETURN rc;
    STMT *stmt= (STMT *)hstmt;
    uint     nparam;

    for ( nparam= 0; nparam < stmt->param_count; ++nparam )
    {
        DESCREC *aprec= desc_get_rec(stmt->apd, nparam, TRUE);
        if (!aprec->par.real_param_done)
        {
            /* do the dummy bind temporarily to get the result set
               and once everything is done, remove it */
            if (!SQL_SUCCEEDED(rc= DES_SQLBindParameter(hstmt, nparam+1,
                                                       SQL_PARAM_INPUT,
                                                       SQL_C_CHAR,
                                                       SQL_VARCHAR, 0, 0,
                                                       (SQLPOINTER)"NULL", SQL_NTS, NULL)))
                return rc;
            /* reset back to false (this is the *dummy* param bind) */
            aprec->par.real_param_done= FALSE;
        }
    }
    stmt->dummy_state= ST_DUMMY_PREPARED;
    return(SQL_SUCCESS);
}

/*
  @type    : ODBC 1.0 API
  @purpose : returns the number of columns in a result set
*/

SQLRETURN SQL_API SQLNumResultCols(SQLHSTMT  hstmt, SQLSMALLINT *pccol)
{
  SQLRETURN error;
  STMT *stmt= (STMT *) hstmt;

  CHECK_HANDLE(hstmt);
  CHECK_DATA_OUTPUT(hstmt, pccol);

  if (stmt->param_count > 0 && stmt->dummy_state == ST_DUMMY_UNKNOWN &&
      (stmt->state != ST_PRE_EXECUTED || stmt->state != ST_EXECUTED)) {
    if (do_dummy_parambind(hstmt) != SQL_SUCCESS) {
      return SQL_ERROR;
    }
  }
  if ((error = check_result(stmt)) != SQL_SUCCESS) {
    return error;
  }

  *pccol= (SQLSMALLINT) stmt->ird->rcount();

  return SQL_SUCCESS;
}

/* DESODBC:
    Renamed from the original MySQLDescribeCol()
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/**
  Get some basic properties of a column.

  @param[in]  hstmt      Statement handle
  @param[in]  column     Column number (starting with 1)
  @param[out] name       Pointer to column name
  @param[out] need_free  Whether the column name needs to be freed.
  @param[out] type       Column SQL type
  @param[out] size       Column size
  @param[out] scale      Scale
  @param[out] nullable   Whether the column is nullable
*/
SQLRETURN SQL_API
DESDescribeCol(SQLHSTMT hstmt, SQLUSMALLINT column,
                 SQLCHAR **name, SQLSMALLINT *need_free, SQLSMALLINT *type,
                 SQLULEN *size, SQLSMALLINT *scale, SQLSMALLINT *nullable) {
  SQLRETURN error;
  STMT *stmt = (STMT *)hstmt;
  DESCREC *irrec;

  *need_free = 0;

  /* SQLDescribeCol can be called before SQLExecute. Thus we need make sure that
     all parameters have been bound */
  if (stmt->param_count > 0 && stmt->dummy_state == ST_DUMMY_UNKNOWN &&
      (stmt->state != ST_PRE_EXECUTED || stmt->state != ST_EXECUTED)) {
    if (do_dummy_parambind(hstmt) != SQL_SUCCESS) return SQL_ERROR;
  }

  if ((error = check_result(stmt)) != SQL_SUCCESS) return error;
  if (!stmt->result) return stmt->set_error("07005", "No result set");

  if (column == 0 || column > stmt->ird->rcount()) {
    return stmt->set_error("07009", "Invalid descriptor index");
  }

  irrec = desc_get_rec(stmt->ird, column - 1, FALSE);
  if (!irrec) {
    return SQL_ERROR;  // The error info is already set inside desc_get_rec()
  }

  if (type) *type = irrec->concise_type;
  if (size) *size = irrec->length;
  if (scale) *scale = irrec->scale;
  if (nullable) *nullable = irrec->nullable;

  *name = (SQLCHAR *)irrec->name;

  return SQL_SUCCESS;
}


/* DESODBC:
    Renamed from the original MySQLColAttribute()
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  Retrieve an attribute of a column in a result set.

  @param[in]  hstmt          Handle to statement
  @param[in]  column         The column to retrieve data for, indexed from 1
  @param[in]  attrib         The attribute to be retrieved
  @param[out] char_attr      Pointer to a string pointer for returning strings
                             (caller must make their own copy)
  @param[out] num_attr       Pointer to an integer to return the value if the
                             @a attrib corresponds to a numeric type

  @since ODBC 1.0
*/
SQLRETURN SQL_API
DESColAttribute(SQLHSTMT hstmt, SQLUSMALLINT column,
                  SQLUSMALLINT attrib, SQLCHAR **char_attr, SQLLEN *num_attr)
{
  STMT *stmt= (STMT *)hstmt;
  SQLLEN nparam= 0;
  SQLRETURN error= SQL_SUCCESS;
  DESCREC *irrec;

  /* DESColAttribute can be called before SQLExecute. Thus we need make sure
    that all parameters have been bound */
  if (stmt->param_count > 0 && stmt->dummy_state == ST_DUMMY_UNKNOWN &&
      (stmt->state != ST_PRE_EXECUTED || stmt->state != ST_EXECUTED)) {
    if (do_dummy_parambind(hstmt) != SQL_SUCCESS) return SQL_ERROR;
  }

  if (check_result(stmt) != SQL_SUCCESS) return SQL_ERROR;

  if (!stmt->result)
  {
    return stmt->set_error("07005", "No result set");
  }

  /* we report bookmark type if requested, nothing else */
  if (attrib == SQL_DESC_TYPE && column == 0)
  {
    *(SQLINTEGER *)num_attr= SQL_INTEGER;
    return SQL_SUCCESS;
  }

  if (column == 0 || column > stmt->ird->rcount())
    return ((STMT *)hstmt)->set_error("07009", "Invalid descriptor index");

  if (!num_attr)
    num_attr= &nparam;

  if ((error= check_result(stmt)) != SQL_SUCCESS)
    return error;

  if (attrib == SQL_DESC_COUNT || attrib == SQL_COLUMN_COUNT)
  {
    *num_attr= stmt->ird->rcount();
    return SQL_SUCCESS;
  }

  irrec= desc_get_rec(stmt->ird, column - 1, FALSE);
  if (!irrec)
  {
    return SQL_ERROR; // The error info is already set inside desc_get_rec()
  }

  /*
     Map to descriptor fields. This approach is only valid
     for ODBC 3.0 API applications.

     @todo Add additional logic to properly handle these fields
           for ODBC 2.0 API applications.
  */
  switch (attrib)
  {
  case SQL_COLUMN_SCALE:
    attrib= SQL_DESC_SCALE;
    break;
  case SQL_COLUMN_PRECISION:
    attrib= SQL_DESC_PRECISION;
    break;
  case SQL_COLUMN_NULLABLE:
    attrib= SQL_DESC_NULLABLE;
    break;
  case SQL_COLUMN_LENGTH:
    attrib= SQL_DESC_OCTET_LENGTH;
    break;
  case SQL_COLUMN_NAME:
    attrib= SQL_DESC_NAME;
    break;
  }

  switch (attrib)
  {
  case SQL_DESC_AUTO_UNIQUE_VALUE:
  case SQL_DESC_CASE_SENSITIVE:
  case SQL_DESC_FIXED_PREC_SCALE:
  case SQL_DESC_NULLABLE:
  case SQL_DESC_NUM_PREC_RADIX:
  case SQL_DESC_PRECISION:
  case SQL_DESC_SCALE:
  case SQL_DESC_SEARCHABLE:
    // Mark the result column as not searchable for BIT(N > 1) columns.
    // This is needed to prevent the use of such columns in comparisons
    // like " WHERE bit_col = _binary'....'" which is not supported
    // by MySQL Server.
    // The type name for such columns is "bit", but the type ID is
    // changed to SQL_BINARY (-2).
    if (SQL_BINARY == irrec->concise_type &&
        strncmp((const char *)irrec->type_name, "bit", 3) == 0) {
      *num_attr = SQL_PRED_NONE;
      break;
    }

  case SQL_DESC_TYPE:
  case SQL_DESC_CONCISE_TYPE:
  case SQL_DESC_UNNAMED:
  case SQL_DESC_UNSIGNED:
  case SQL_DESC_UPDATABLE:
    error= stmt_SQLGetDescField(stmt, stmt->ird, column, attrib,
#ifdef USE_SQLCOLATTRIBUTE_SQLLEN_PTR
                                (SQLPOINTER)num_attr,SQL_IS_LEN,
#else
                                num_attr, SQL_IS_INTEGER,
#endif
               NULL);
    break;

  case SQL_DESC_DISPLAY_SIZE:
  case SQL_DESC_LENGTH:
  case SQL_DESC_OCTET_LENGTH:
    error= stmt_SQLGetDescField(stmt, stmt->ird, column, attrib,
                                num_attr, SQL_IS_LEN, NULL);
    break;

  /* We need support from server, when aliasing is there */
  case SQL_DESC_BASE_COLUMN_NAME:
    *char_attr= irrec->base_column_name ? irrec->base_column_name :
                                          (SQLCHAR *) "";
    break;

  case SQL_DESC_LABEL:
  case SQL_DESC_NAME:
    *char_attr= irrec->name;
    break;

  case SQL_DESC_BASE_TABLE_NAME:
    *char_attr= irrec->base_table_name ? irrec->base_table_name :
                                         (SQLCHAR *) "";
    break;

  case SQL_DESC_TABLE_NAME:
    *char_attr= irrec->table_name ? irrec->table_name : (SQLCHAR *) "";
    break;

  case SQL_DESC_CATALOG_NAME:
    *char_attr= irrec->catalog_name;
    break;

  case SQL_DESC_LITERAL_PREFIX:
    *char_attr= irrec->literal_prefix;
    break;

  case SQL_DESC_LITERAL_SUFFIX:
    *char_attr= irrec->literal_suffix;
    break;

  case SQL_DESC_SCHEMA_NAME:
    *char_attr= irrec->schema_name;
    break;

  case SQL_DESC_TYPE_NAME:
    *char_attr= irrec->type_name;
    break;

  case SQL_DESC_LOCAL_TYPE_NAME:
    *char_attr = (SQLCHAR*)"";

  /*
    Hack : Fix for the error from ADO 'rs.resync' "Key value for this row
    was changed or deleted at the data store.  The local row is now deleted.
    This should also fix some Multi-step generated error cases from ADO
  */
  case SQL_MY_PRIMARY_KEY: /* MSSQL extension !! */
    *(SQLINTEGER *)num_attr= ((irrec->row.field->flags & PRI_KEY_FLAG) ?
                              SQL_TRUE : SQL_FALSE);
    break;

  default:
    return stmt->set_error("HY091",
                          "Invalid descriptor field identifier");
  }

  return error;
}


/*
  @type    : ODBC 1.0 API
  @purpose : binds application data buffers to columns in the result set
*/

SQLRETURN SQL_API SQLBindCol(SQLHSTMT      StatementHandle,
                             SQLUSMALLINT  ColumnNumber,
                             SQLSMALLINT   TargetType,
                             SQLPOINTER    TargetValuePtr,
                             SQLLEN        BufferLength,
                             SQLLEN *      StrLen_or_IndPtr)
{
  SQLRETURN rc;
  STMT *stmt = (STMT *)StatementHandle;
  DESCREC *arrec;
  /* TODO if this function fails, the SQL_DESC_COUNT should be unchanged in ard
   */

  LOCK_STMT(stmt);
  CLEAR_STMT_ERROR(stmt);

  if (!TargetValuePtr && !StrLen_or_IndPtr) /* Handling unbinding */
  {
    /*
       If unbinding the last bound column, we reduce the
       ARD records until the highest remaining bound column.
    */
    if (ColumnNumber == stmt->ard->rcount()) {
      stmt->ard->records2.pop_back();  // Remove the last
      while (stmt->ard->rcount()) {
        arrec = desc_get_rec(stmt->ard, (int)stmt->ard->rcount() - 1, FALSE);
        if (ARD_IS_BOUND(arrec))
          break;
        else
          stmt->ard->records2.pop_back();  // TODO: do it more gracefully
      }
    } else {
      arrec = desc_get_rec(stmt->ard, ColumnNumber - 1, FALSE);
      if (arrec) {
        arrec->data_ptr = NULL;
        arrec->octet_length_ptr = NULL;
      }
    }
    return SQL_SUCCESS;
  }

  if ((ColumnNumber == 0 && stmt->stmt_options.bookmarks == SQL_UB_OFF) ||
      (stmt->state == ST_EXECUTED && ColumnNumber > stmt->ird->rcount())) {
    return stmt->set_error("07009", "Invalid descriptor index");
  }

  arrec = desc_get_rec(stmt->ard, ColumnNumber - 1, TRUE);

  if ((rc = stmt_SQLSetDescField(
           stmt, stmt->ard, ColumnNumber, SQL_DESC_CONCISE_TYPE,
           (SQLPOINTER)(size_t)TargetType, SQL_IS_SMALLINT)) != SQL_SUCCESS)
    return rc;
  if ((rc = stmt_SQLSetDescField(
           stmt, stmt->ard, ColumnNumber, SQL_DESC_OCTET_LENGTH,
           (SQLPOINTER)(size_t)bind_length(TargetType, (ulong)BufferLength),
           SQL_IS_LEN)) != SQL_SUCCESS)
    return rc;
  if ((rc = stmt_SQLSetDescField(stmt, stmt->ard, ColumnNumber,
                                 SQL_DESC_DATA_PTR, TargetValuePtr,
                                 SQL_IS_POINTER)) != SQL_SUCCESS)
    return rc;
  if ((rc = stmt_SQLSetDescField(stmt, stmt->ard, ColumnNumber,
                                 SQL_DESC_INDICATOR_PTR, StrLen_or_IndPtr,
                                 SQL_IS_POINTER)) != SQL_SUCCESS)
    return rc;
  if ((rc = stmt_SQLSetDescField(stmt, stmt->ard, ColumnNumber,
                                 SQL_DESC_OCTET_LENGTH_PTR, StrLen_or_IndPtr,
                                 SQL_IS_POINTER)) != SQL_SUCCESS)
    return rc;

  return SQL_SUCCESS;

}

/*
  @type    : ODBC 1.0 API
  @purpose : retrieves data for a single column in the result set. It can
  be called multiple times to retrieve variable-length data
  in parts
*/

SQLRETURN SQL_API SQLGetData(SQLHSTMT      StatementHandle,
                             SQLUSMALLINT  ColumnNumber,
                             SQLSMALLINT   TargetType,
                             SQLPOINTER    TargetValuePtr,
                             SQLLEN        BufferLength,
                             SQLLEN *      StrLen_or_IndPtr)
{
  STMT *stmt = (STMT *)StatementHandle;
  SQLRETURN result = SQL_SUCCESS;
  ulong length = 0;
  DESCREC *irrec, *arrec;
  /*
    Signed column number required for bookmark column 0,
    which will become -1 when decremented later.
  */
  SQLSMALLINT sColNum = ColumnNumber;

  LOCK_STMT(stmt);

  if (!stmt->result || (!stmt->current_values &&
                        stmt->out_params_state != OPS_STREAMS_PENDING)) {
    stmt->set_error("24000", "SQLGetData without a preceding SELECT");
    return SQL_ERROR;
  }

  if ((sColNum < 1 &&
       stmt->stmt_options.bookmarks == (SQLUINTEGER)SQL_UB_OFF) ||
      ColumnNumber > stmt->ird->rcount()) {
    return stmt->set_error("07009", "Invalid descriptor index");
  }

  if (sColNum == 0 && TargetType != SQL_C_BOOKMARK &&
      TargetType != SQL_C_VARBOOKMARK) {
    return stmt->set_error("HY003", "Program type out of range");
  }

  --sColNum; /* Easier code if start from 0 which will make bookmark column -1
              */

  if (stmt->out_params_state == OPS_STREAMS_PENDING) {
    /* http://msdn.microsoft.com/en-us/library/windows/desktop/ms715441%28v=vs.85%29.aspx
        "07009 Invalid descriptor index ...The Col_or_Param_Num value was not
       equal to the ordinal of the parameter that is available." Returning error
       if requested parameter number is different from the last call to
        SQLParamData */
    if (sColNum != stmt->current_param) {
      return stmt->set_error("07009",
                             "The parameter number value was not equal to \
                                            the ordinal of the parameter that is available.");
    } else {
      /* In getdatat column we keep out parameter column number in the result */
      sColNum = stmt->getdata.column;
    }

    if (TargetType != SQL_C_BINARY) {
      return stmt->set_error(
          "HYC00",
          "Stream output parameters supported for SQL_C_BINARY"
          " only");
    }
  }

  if (sColNum != stmt->getdata.column) {
    /* New column. Reset old offset */
    stmt->reset_getdata_position();
    stmt->getdata.column = sColNum;
  }
  irrec = desc_get_rec(stmt->ird, sColNum, FALSE);

  assert(irrec);

  if ((sColNum == -1 && stmt->stmt_options.bookmarks == SQL_UB_VARIABLE)) {
    std::string _value;
    /* save position set using SQLSetPos in buffer */
    _value = std::to_string((stmt->cursor_row > 0) ? stmt->cursor_row : 0);
    arrec = desc_get_rec(stmt->ard, sColNum, FALSE);
    result = sql_get_bookmark_data(
        stmt, TargetType, sColNum, TargetValuePtr, BufferLength,
        StrLen_or_IndPtr, (char *)_value.data(), (ulong)_value.length(), arrec);
  } else {
    /* catalog functions with "fake" results won't have lengths */
    length = irrec->row.datalen;
    if (!length && stmt->current_values[sColNum])
      length = (ulong)strlen(stmt->current_values[sColNum]);

    arrec = desc_get_rec(stmt->ard, sColNum, FALSE);

    /* String will be used as a temporary storage which frees itself
     * automatically */
    std::string temp_str;
    char *value = fix_padding(stmt, TargetType, stmt->current_values[sColNum],
                              temp_str, BufferLength, length, irrec);

    result = sql_get_data(stmt, TargetType, sColNum, TargetValuePtr,
                          BufferLength, StrLen_or_IndPtr, value, length, arrec);
  }

  return result;
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : ODBC 1.0 API
  @purpose : determines whether more results are available on a statement
  containing SELECT, UPDATE, INSERT, or DELETE statements and,
  if so, initializes processing for those results
*/
SQLRETURN SQL_API SQLMoreResults( SQLHSTMT hstmt )
{
  STMT *stmt = (STMT *)hstmt;
  int nRetVal = 0;
  SQLRETURN nReturn = SQL_SUCCESS;

  LOCK_STMT(stmt);
  LOCK_DBC(stmt->dbc);
  CLEAR_STMT_ERROR(stmt);

  /*
    http://msdn.microsoft.com/en-us/library/ms714673%28v=vs.85%29.aspx

    For some drivers, output parameters and return values are not available
    until all result sets and row counts have been processed. For such
    drivers, output parameters and return values become available when
    SQLMoreResults returns SQL_NO_DATA.
  */
  if ( stmt->state != ST_EXECUTED )
  {
    nReturn = SQL_NO_DATA;
    goto exitSQLMoreResults;
  }

  /* try to get next resultset */
  nRetVal = next_result(stmt);

  /* no more resultsets */
  if (nRetVal < 0)
  {
    nReturn = SQL_NO_DATA;
    goto exitSQLMoreResults;
  }

  /* cleanup existing resultset */
  nReturn = DES_SQLFreeStmtExtended((SQLHSTMT)stmt, SQL_CLOSE, 0);
  if (!SQL_SUCCEEDED( nReturn ))
  {
    goto exitSQLMoreResults;
  }

  /* start using the new resultset */
  stmt->result = get_result_metadata(stmt);

  if (!stmt->result)
  {
    /* no fields means; INSERT, UPDATE or DELETE so no resultset is fine */
    if (!stmt->field_count())
    {
      stmt->state = ST_EXECUTED;
      stmt->affected_rows = affected_rows(stmt);
      goto exitSQLMoreResults;
    }
    /* we have fields but no resultset (not even an empty one) - this is bad */
    nReturn = stmt->set_error("HY000", "Fields exist but not the result set");
    goto exitSQLMoreResults;
  }

  free_result_bind(stmt);
  fix_result_types(stmt);


exitSQLMoreResults:

  return nReturn;
}


/*
  @type    : ODBC 1.0 API
  @purpose : returns the number of rows affected by an UPDATE, INSERT,
  or DELETE statement;an SQL_ADD, SQL_UPDATE_BY_BOOKMARK,
  or SQL_DELETE_BY_BOOKMARK operation in SQLBulkOperations;
  or an SQL_UPDATE or SQL_DELETE operation in SQLSetPos
*/

SQLRETURN SQL_API SQLRowCount( SQLHSTMT hstmt,
                               SQLLEN * pcrow )
{
    STMT *stmt= (STMT *) hstmt;

    CHECK_HANDLE(hstmt);
    CHECK_DATA_OUTPUT(hstmt, pcrow);

    if ( stmt->result )
    {
      if (stmt->fake_result)
        *pcrow = stmt->result->row_count;
      else
        // for SetPos operations result is defined and they use direct execution
        *pcrow= (SQLLEN) affected_rows(stmt);
    }
    else
    {
        *pcrow= (SQLLEN) stmt->affected_rows;
    }
    return SQL_SUCCESS;
}


/**
  Populate the data lengths in the IRD for the current row

  @param[in]  ird         IRD to populate
  @param[in]  lengths     Data lengths from mysql_fetch_lengths()
  @param[in]  fields      Number of fields
*/
void fill_ird_data_lengths(DESC *ird, ulong *lengths, uint fields)
{
  uint i;
  DESCREC *irrec;

  if (ird->rcount() == 0 && fields > 0)
  {
    // Expand IRD if it has no records
    desc_get_rec(ird, fields - 1, true);
  }

  assert(fields == ird->rcount());

  /* This will be NULL for catalog functions with "fake" results */
  if (!lengths)
  {
    return;
  }

  for (i= 0; i < fields; ++i)
  {
    irrec= desc_get_rec(ird, i, FALSE);
    assert(irrec);

    irrec->row.datalen= lengths[i];
  }
}


/**
  Populate a single row of bookmark fetch buffers

  @param[in]  stmt        Handle of statement
  @param[in]  value       Row number with Offset
  @param[in]  rownum      Row number of current fetch block
*/
static SQLRETURN
fill_fetch_bookmark_buffers(STMT *stmt, ulong value, uint rownum)
{
  SQLRETURN res= SQL_SUCCESS, tmp_res;
  DESCREC *arrec;

  IS_BOOKMARK_VARIABLE(stmt);
  arrec= desc_get_rec(stmt->ard, -1, FALSE);

  if (ARD_IS_BOUND(arrec))
  {
    SQLLEN *pcbValue= NULL;
    SQLPOINTER TargetValuePtr= NULL;
    ulong copy_bytes= 0;

    stmt->reset_getdata_position();

    if (arrec->data_ptr)
    {
      TargetValuePtr= ptr_offset_adjust(arrec->data_ptr,
                                        stmt->ard->bind_offset_ptr,
                                        stmt->ard->bind_type,
                                        (SQLINTEGER)arrec->octet_length, rownum);
    }

    if (arrec->octet_length_ptr)
    {
      pcbValue= (SQLLEN*)ptr_offset_adjust(arrec->octet_length_ptr,
                                    stmt->ard->bind_offset_ptr,
                                    stmt->ard->bind_type,
                                    sizeof(SQLLEN), rownum);
    }

    std::string _value = std::to_string((value > 0) ? value : 0);
    tmp_res= sql_get_bookmark_data(stmt, arrec->concise_type, (uint)0,
                          TargetValuePtr, arrec->octet_length, pcbValue,
                          (char*)_value.data(), (ulong)_value.length(), arrec);
    if (tmp_res != SQL_SUCCESS)
    {
      if (tmp_res == SQL_SUCCESS_WITH_INFO)
      {
        if (res == SQL_SUCCESS)
          res= tmp_res;
      }
      else
      {
        res= SQL_ERROR;
      }
    }
  }

  return res;
}


/**
  Populate a single row of fetch buffers

  @param[in]  stmt        Handle of statement
  @param[in]  values      Row buffers from libmysql
  @param[in]  rownum      Row number of current fetch block
*/
static SQLRETURN
fill_fetch_buffers(STMT *stmt, DES_ROW values, uint rownum)
{
  SQLRETURN res= SQL_SUCCESS, tmp_res;
  int i;
  ulong length= 0;
  DESCREC *irrec, *arrec;

  for (i= 0; i < desodbc_min(stmt->ird->rcount(), stmt->ard->rcount()); ++i, ++values)
  {
    irrec= desc_get_rec(stmt->ird, i, FALSE);
    arrec= desc_get_rec(stmt->ard, i, FALSE);
    assert(irrec && arrec);

    if (ARD_IS_BOUND(arrec))
    {
      SQLLEN *pcbValue= NULL;
      SQLPOINTER TargetValuePtr= NULL;

      stmt->reset_getdata_position();

      if (arrec->data_ptr)
      {
        TargetValuePtr= ptr_offset_adjust(arrec->data_ptr,
                                          stmt->ard->bind_offset_ptr,
                                          stmt->ard->bind_type,
                                          (SQLINTEGER)arrec->octet_length, rownum);
      }

      /* catalog functions with "fake" results won't have lengths */
      length= irrec->row.datalen;

      if (!length && *values)
      {
        length = (ulong)strlen(*values);
      }

      /* We need to pass that pointer to the sql_get_data so it could detect
         22002 error - for NULL values that pointer has to be supplied by user.
       */
      if (arrec->octet_length_ptr)
      {
        pcbValue= (SQLLEN*)ptr_offset_adjust(arrec->octet_length_ptr,
                                      stmt->ard->bind_offset_ptr,
                                      stmt->ard->bind_type,
                                      sizeof(SQLLEN), rownum);
      }

      std::string temp_str;
      char *temp_val = fix_padding(stmt, arrec->concise_type, *values,
                                   temp_str, arrec->octet_length,
                                   length, irrec);

      tmp_res= sql_get_data(stmt, arrec->concise_type, (uint)i,
                            TargetValuePtr, arrec->octet_length, pcbValue,
                            temp_val, length, arrec);

      if (tmp_res != SQL_SUCCESS)
      {
        if (tmp_res == SQL_SUCCESS_WITH_INFO)
        {
          if (res == SQL_SUCCESS)
            res= tmp_res;
        }
        else
        {
          res= SQL_ERROR;
        }
      }
    }
  }

  return res;
}


/*
  @type    : myodbc3 internal
  @purpose : fetches the specified row from the result set and
             returns data for all bound columns. Row can be specified
             at an absolute or relative position
*/
SQLRETURN SQL_API myodbc_single_fetch( SQLHSTMT             hstmt,
                                       SQLUSMALLINT         fFetchType,
                                       SQLLEN               irow,
                                       SQLULEN             *pcrow,
                                       SQLUSMALLINT        *rgfRowStatus,
                                       my_bool              upd_status )
{
  SQLULEN           rows_to_fetch;
  long              cur_row, max_row;
  SQLRETURN         row_res, res;
  STMT              *stmt= (STMT *) hstmt;
  DES_ROW         values= 0;
  DES_ROW_OFFSET  save_position= 0;
  SQLULEN           dummy_pcrow;
  BOOL              disconnected= FALSE;
  long              brow= 0;

  try
  {

    if ( !stmt->result )
      return stmt->set_error("24000", "Fetch without a SELECT");
    cur_row = stmt->current_row;

    if ( !pcrow )
      pcrow= &dummy_pcrow;

    /* for scrollable cursor("scroller") max_row is max row for currently
       fetched part of resultset */
    max_row= (long) num_rows(stmt);
    stmt->reset_getdata_position();
    stmt->current_values= 0;          /* For SQLGetData */

    switch ( fFetchType )
    {
      case SQL_FETCH_NEXT:
        cur_row= (stmt->current_row < 0 ? 0 :
                  stmt->current_row + stmt->rows_found_in_set);
        break;
      case SQL_FETCH_PRIOR:
        cur_row= (stmt->current_row <= 0 ? -1 :
                  (long)(stmt->current_row - stmt->ard->array_size));
        break;
      case SQL_FETCH_FIRST:
        cur_row= 0L;
        break;
      case SQL_FETCH_LAST:
        cur_row = max_row - (long)stmt->ard->array_size;
        break;
      case SQL_FETCH_ABSOLUTE:
        if (irow < 0)
        {
          /* Fetch from end of result set */
          if ( max_row+irow < 0 && -irow <= (long) stmt->ard->array_size )
          {
            /*
              | FetchOffset | > LastResultRow AND
              | FetchOffset | <= RowsetSize
            */
            cur_row= 0;     /* Return from beginning */
          }
          else
            cur_row = max_row + (long)irow;     /* Ok if max_row <= -irow */
        }
        else
          cur_row= (long) irow - 1;
        break;

      case SQL_FETCH_RELATIVE:
        cur_row = stmt->current_row + (long)irow;
        if (stmt->current_row > 0 && cur_row < 0 &&
           (long) - irow <= (long)stmt->ard->array_size)
        {
          cur_row= 0;
        }
        break;

      case SQL_FETCH_BOOKMARK:
        {
          if (stmt->stmt_options.bookmark_ptr)
          {
            DESCREC *arrec;
            IS_BOOKMARK_VARIABLE(stmt);
            arrec= desc_get_rec(stmt->ard, -1, FALSE);

            if (arrec->concise_type == SQL_C_BOOKMARK)
            {
              brow = (long)(*((SQLLEN *) stmt->stmt_options.bookmark_ptr));
            }
            else
            {
              brow= atol((const char*) stmt->stmt_options.bookmark_ptr);
            }
          }

          cur_row = brow + (long)irow;
          if (cur_row < 0 && (long)-irow <= (long)stmt->ard->array_size)
          {
            cur_row= 0;
          }
        }
        break;

      default:
          return stmt->set_error("HY106", "Fetch type out of range");
    }

    if ( cur_row < 0 )
    {
      stmt->current_row= -1;  /* Before first row */
      stmt->rows_found_in_set= 0;
      data_seek(stmt, 0L);
      stmt->set_error("01S07", "One or more row has error.");
      return SQL_SUCCESS_WITH_INFO; //SQL_NO_DATA_FOUND
    }
    if ( cur_row > max_row )
    {
      cur_row = max_row;
    }

    if ( !stmt->result_array && !if_forward_cache(stmt) )
    {
        /*
          If Dynamic, it loses the stmt->end_of_set, so
          seek to desired row, might have new data or
          might be deleted
        */
        if ( stmt->stmt_options.cursor_type != SQL_CURSOR_DYNAMIC &&
             cur_row && cur_row == (long)(stmt->current_row +
                                          stmt->rows_found_in_set) )
            row_seek(stmt, stmt->end_of_set);
        else
            data_seek(stmt, cur_row);
    }
    stmt->current_row= cur_row;

    rows_to_fetch = desodbc_min(max_row - cur_row, (long)stmt->ard->array_size);

    /* out params has been silently fetched */
    if (rows_to_fetch == 0)
    {
      if (stmt->out_params_state != OPS_UNKNOWN)
      {
        rows_to_fetch= 1;
      }
      else
      {
        *pcrow= 0;
        stmt->rows_found_in_set= 0;

        if ( upd_status && stmt->ird->rows_processed_ptr )
        {
          *stmt->ird->rows_processed_ptr= 0;
        }

        stmt->set_error("01S07", "One or more row has error.");
        return SQL_SUCCESS_WITH_INFO; //SQL_NO_DATA_FOUND
      }
    }

    res= SQL_SUCCESS;
    {
      save_position= row_tell(stmt);
      /* - Actual fetching happens here - */
      if (!(values = stmt->fetch_row()) )
      {
        goto exitSQLSingleFetch;
      }

      if ( stmt->fix_fields )
      {
          values= (*stmt->fix_fields)(stmt,values);
      }

      stmt->current_values= values;
    }

    if (!stmt->fix_fields)
    {
        fill_ird_data_lengths(stmt->ird, fetch_lengths(stmt),
                              stmt->result->field_count);
    }

    row_res= fill_fetch_buffers(stmt, values, cur_row);

    /* For SQL_SUCCESS we need all rows to be SQL_SUCCESS */
    if (res != row_res)
    {
      /* Any successful row makes overall result SQL_SUCCESS_WITH_INFO */
      if (SQL_SUCCEEDED(row_res))
      {
        res= SQL_SUCCESS_WITH_INFO;
      }
      /* Else error */
      else if (cur_row == 0)
      {
        /* SQL_ERROR only if all rows fail */
        res= SQL_ERROR;
      }
      else
      {
        res= SQL_SUCCESS_WITH_INFO;
      }
    }

    /* "Fetching" includes buffers filling. I think errors in that
       have to affect row status */

    if (rgfRowStatus)
    {
      rgfRowStatus[cur_row]= sqlreturn2row_status(row_res);
    }
    /*
      No need to update rowStatusPtr_ex, it's the same as rgfRowStatus.
    */
    if (upd_status && stmt->ird->array_status_ptr)
    {
      stmt->ird->array_status_ptr[cur_row]= sqlreturn2row_status(row_res);
    }

exitSQLSingleFetch:
    stmt->rows_found_in_set= 1;
    *pcrow= cur_row;

    disconnected = FALSE;

    if ( upd_status && stmt->ird->rows_processed_ptr )
    {
      *stmt->ird->rows_processed_ptr= cur_row;
    }

    /* It is possible that both rgfRowStatus and array_status_ptr are set
    (and upp_status is TRUE) */
    if ( rgfRowStatus )
    {
      rgfRowStatus[cur_row]= disconnected ? SQL_ROW_ERROR : SQL_ROW_NOROW;
    }
    /*
      No need to update rowStatusPtr_ex, it's the same as rgfRowStatus.
    */
    if ( upd_status && stmt->ird->array_status_ptr )
    {
      stmt->ird->array_status_ptr[cur_row]= disconnected? SQL_ROW_ERROR
                                                  : SQL_ROW_NOROW;
    }

    if (SQL_SUCCEEDED(res) && !if_forward_cache(stmt))
    {
      /* reset result position */
      stmt->end_of_set= row_seek(stmt, save_position);
    }

    if (SQL_SUCCEEDED(res)
      && stmt->rows_found_in_set < stmt->ard->array_size)
    {
      if (disconnected)
      {
        return SQL_ERROR;
      }
      else if (stmt->rows_found_in_set == 0)
      {
        stmt->set_error("01S07", "One or more row has error.");
        return SQL_SUCCESS_WITH_INFO; //SQL_NO_DATA_FOUND
      }
    }
  }
  catch(DESERROR &e)
  {
    res = e.retcode;
  }
  return res;
}

/* DESODBC:
    Renamed from the original my_SQLExtendedFetch
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : myodbc3 internal
  @purpose : fetches the specified rowset of data from the result set and
  returns data for all bound columns. Rowsets can be specified
  at an absolute or relative position

  This function is way to long and needs to be structured.
*/
SQLRETURN SQL_API DES_SQLExtendedFetch( SQLHSTMT             hstmt,
                                       SQLUSMALLINT         fFetchType,
                                       SQLLEN               irow,
                                       SQLULEN             *pcrow,
                                       SQLUSMALLINT        *rgfRowStatus,
                                       my_bool              upd_status )
{
    SQLULEN           rows_to_fetch;
    long              cur_row, max_row;
    SQLULEN           i;
    SQLRETURN         row_res, res, row_book= SQL_SUCCESS;
    STMT              *stmt= (STMT *) hstmt;
    DES_ROW         values= 0;
    DES_ROW_OFFSET  save_position= 0;
    SQLULEN           dummy_pcrow;
    BOOL              disconnected= FALSE;
    long              brow= 0;

    if ( !stmt->result )
    {
        res = stmt->set_error("24000", "Fetch without a SELECT");
        throw stmt->error;
    }

    if (stmt->out_params_state != OPS_UNKNOWN)
    {
        switch(stmt->out_params_state)
        {
        case OPS_BEING_FETCHED:
            return SQL_NO_DATA_FOUND;
        default:
            /* TODO: Need to remember real fetch' result */
            /* just in case... */
            stmt->out_params_state= OPS_BEING_FETCHED;
        }
    }

    cur_row = stmt->current_row;

    if ( stmt->stmt_options.cursor_type == SQL_CURSOR_FORWARD_ONLY )
    {
        if ( fFetchType != SQL_FETCH_NEXT && !stmt->dbc->ds.opt_SAFE )
        {
        res = stmt->set_error("HY106",
                "Wrong fetchtype with FORWARD ONLY cursor");
        throw stmt->error;
        }
    }

    if ( !pcrow )
        pcrow= &dummy_pcrow;

    /* for scrollable cursor("scroller") max_row is max row for currently
        fetched part of resultset */
    max_row= (long) num_rows(stmt);
    stmt->reset_getdata_position();
    stmt->current_values= 0;          /* For SQLGetData */
    cur_row = stmt->compute_cur_row(fFetchType, irow);

    rows_to_fetch = desodbc_min(max_row - cur_row, (long)stmt->ard->array_size);

    /* out params has been silently fetched */
    if (rows_to_fetch == 0)
    {
        if (stmt->out_params_state != OPS_UNKNOWN)
        {
        rows_to_fetch= 1;
        }
        else
        {
        *pcrow= 0;
        stmt->rows_found_in_set= 0;
        if ( upd_status && stmt->ird->rows_processed_ptr )
        {
            *stmt->ird->rows_processed_ptr= 0;
        }
        return SQL_NO_DATA_FOUND;
        }
    }

    res= SQL_SUCCESS;
    for (i= 0 ; i < rows_to_fetch ; ++i)
    {
        values = nullptr;

        if ( stmt->result_array )
        {
        values= stmt->result_array + cur_row*stmt->result->field_count;
        if ( i == 0 )
        {
            stmt->current_values= values;
        }
        }
        else
        {
        /* This code will ensure that values is always set */
        if ( i == 0 )
        {
            save_position= row_tell(stmt);
        }
        /* - Actual fetching happens here - */
        if ( stmt->out_params_state == OPS_UNKNOWN
            && !(values = stmt->fetch_row()) )
        {
          break;
        }

        if (stmt->out_params_state != OPS_UNKNOWN)
        {
            values= stmt->array;
        }

        if (stmt->fix_fields)
        {
            values= (*stmt->fix_fields)(stmt,values);
        }

        stmt->current_values= values;
        }

        if (!stmt->fix_fields)
        {
        /* lengths contains lengths for all rows. Alternate use could be
            filling ird buffers in the (fix_fields) function. In this case
            lengths could contain just one array with rules for lengths
            calculating(it can work out in many cases like in catalog functions
            there some fields from results of auxiliary query are simply mixed
            somehow and constant fields added ).
            Another approach could be using of "array" and "order" arrays
            and special fix_fields callback, that will fix array and set
            lengths in ird*/
        if (stmt->lengths)
        {
            fill_ird_data_lengths(stmt->ird, stmt->lengths.get() + cur_row*stmt->result->field_count,
                                stmt->result->field_count);
        }
        else
        {
            fill_ird_data_lengths(stmt->ird, fetch_lengths(stmt),
                                stmt->result->field_count);
        }
        }

        if (fFetchType == SQL_FETCH_BOOKMARK &&
            stmt->stmt_options.bookmarks == SQL_UB_VARIABLE)
        {
        row_book= fill_fetch_bookmark_buffers(stmt, (ulong)(irow + i + 1), (uint)i);
        }
        row_res= fill_fetch_buffers(stmt, values, (uint)i);

        /* For SQL_SUCCESS we need all rows to be SQL_SUCCESS */
        if (res != row_res || res != row_book)
        {
        /* Any successful row makes overall result SQL_SUCCESS_WITH_INFO */
        if (SQL_SUCCEEDED(row_res) && SQL_SUCCEEDED(row_res))
        {
            res= SQL_SUCCESS_WITH_INFO;
        }
        /* Else error */
        else if (i == 0)
        {
            /* SQL_ERROR only if all rows fail */
            res= SQL_ERROR;
        }
        else
        {
            res= SQL_SUCCESS_WITH_INFO;
        }
        }

        /* "Fetching" includes buffers filling. I think errors in that
            have to affect row status */

        if (rgfRowStatus)
        {
        rgfRowStatus[i]= sqlreturn2row_status(row_res);
        }
        /*
        No need to update rowStatusPtr_ex, it's the same as rgfRowStatus.
        */
        if (upd_status && stmt->ird->array_status_ptr)
        {
        stmt->ird->array_status_ptr[i]= sqlreturn2row_status(row_res);
        }

        ++cur_row;
    }   /* fetching cycle end*/

    stmt->rows_found_in_set = (uint)i;
    *pcrow= i;

    disconnected = FALSE;

    if ( upd_status && stmt->ird->rows_processed_ptr )
    {
        *stmt->ird->rows_processed_ptr= i;
    }

    /* It is possible that both rgfRowStatus and array_status_ptr are set
    (and upp_status is TRUE) */
    for ( ; i < stmt->ard->array_size ; ++i )
    {
        if ( rgfRowStatus )
        {
        rgfRowStatus[i]= disconnected ? SQL_ROW_ERROR : SQL_ROW_NOROW;
        }
        /*
        No need to update rowStatusPtr_ex, it's the same as rgfRowStatus.
        */
        if ( upd_status && stmt->ird->array_status_ptr )
        {
        stmt->ird->array_status_ptr[i]= disconnected? SQL_ROW_ERROR
                                                    : SQL_ROW_NOROW;
        }
    }

    if (SQL_SUCCEEDED(res) && !stmt->result_array && !if_forward_cache(stmt))
    {
        /* reset result position */
        stmt->end_of_set= row_seek(stmt, save_position);
    }

    if (SQL_SUCCEEDED(res)
        && stmt->rows_found_in_set < stmt->ard->array_size)
    {
        if (disconnected)
        {
        res = SQL_ERROR;
        throw stmt->error;
        }
        else if (stmt->rows_found_in_set == 0)
        {
        return SQL_NO_DATA_FOUND;
        }
    }

  return res;
}


/*
  @type    : ODBC 1.0 API
  @purpose : fetches the specified rowset of data from the result set and
  returns data for all bound columns. Rowsets can be specified
  at an absolute or relative position
*/

SQLRETURN SQL_API SQLExtendedFetch( SQLHSTMT        hstmt,
                                    SQLUSMALLINT    fFetchType,
                                    SQLLEN          irow,
                                    SQLULEN        *pcrow,
                                    SQLUSMALLINT   *rgfRowStatus )
{
    SQLRETURN rc;
    SQLULEN rows= 0;
    STMT_OPTIONS *options;

    LOCK_STMT(hstmt);

    options= &((STMT *)hstmt)->stmt_options;
    options->rowStatusPtr_ex= rgfRowStatus;

    rc= DES_SQLExtendedFetch(hstmt, fFetchType, irow, &rows, rgfRowStatus, 1);
    if (pcrow)
      *pcrow= (SQLULEN) rows;

    return rc;
}


/*
  @type    : ODBC 3.0 API
  @purpose : fetches the specified rowset of data from the result set and
  returns data for all bound columns. Rowsets can be specified
  at an absolute or relative position
*/

SQLRETURN SQL_API SQLFetchScroll( SQLHSTMT      StatementHandle,
                                  SQLSMALLINT   FetchOrientation,
                                  SQLLEN        FetchOffset )
{
    STMT *stmt = (STMT *)StatementHandle;
    STMT_OPTIONS *options;

    LOCK_STMT(stmt);

    options= &stmt->stmt_options;
    options->rowStatusPtr_ex= NULL;

    if (FetchOrientation == SQL_FETCH_BOOKMARK
        && stmt->stmt_options.bookmark_ptr)
    {
      DESCREC *arrec;
      IS_BOOKMARK_VARIABLE(stmt);
      arrec= desc_get_rec(stmt->ard, -1, FALSE);
      if (!arrec)
      {
        return SQL_ERROR; // The error info is already set inside desc_get_rec()
      }

      FetchOffset += get_bookmark_value(arrec->concise_type,
                       stmt->stmt_options.bookmark_ptr);
    }

    SQLRETURN rc = DES_SQLExtendedFetch(StatementHandle, FetchOrientation, FetchOffset,
                               stmt->ird->rows_processed_ptr, stmt->ird->array_status_ptr,
                               0);
    return rc;
}

/*
  @type    : ODBC 1.0 API
  @purpose : fetches the next rowset of data from the result set and
  returns data for all bound columns
*/

SQLRETURN SQL_API SQLFetch(SQLHSTMT StatementHandle) {
  STMT *stmt = (STMT *)StatementHandle;
  STMT_OPTIONS *options;

  LOCK_STMT(stmt);

  options = &stmt->stmt_options;
  options->rowStatusPtr_ex = NULL;

  return DES_SQLExtendedFetch(StatementHandle, SQL_FETCH_NEXT, 0,
                             stmt->ird->rows_processed_ptr,
                             stmt->ird->array_status_ptr, 0);
}

/* DESODBC:
    Original author: DESODBC Developer
*/
size_t ResultTable::col_count() { return names_ordered.size(); }

/* DESODBC:
    Original author: DESODBC Developer
*/
size_t ResultTable::row_count() {
  if (names_ordered.size() != 0)
    return columns[names_ordered[0]].values.size();
  else
    return 0;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
std::string ResultTable::index_to_name_col(size_t index) {
  return names_ordered[index - 1];
}

/* DESODBC:
    Original author: DESODBC Developer
*/
void ResultTable::refresh_row(const int row_index) {
  for (auto pair : columns) {
    Column col = pair.second;
    col.refresh_row(row_index);
  }
}

/* DESODBC:
    Original author: DESODBC Developer
*/
void ResultTable::insert_col(const std::string &tableName,
                        const std::string &columnName,
                             const TypeAndLength &columnType,
                             const SQLSMALLINT &columnNullable) {
  names_ordered.push_back(columnName);
  columns[columnName] =
      Column(tableName, columnName, columnType, columnNullable);
}

/* DESODBC:
    Original author: DESODBC Developer
*/
void ResultTable::insert_col(DES_FIELD *field) {
  std::string name = field->name;
  names_ordered.push_back(name);
  columns[name] = Column(field);
}

/* DESODBC:
    Original author: DESODBC Developer
*/
void ResultTable::insert_value(const std::string &columnName, char *value) {
  columns[columnName].insert_value(value);
}

/* DESODBC:
    Original author: DESODBC Developer
*/
void ResultTable::insert_value(const std::string &columnName,
                               const std::string &value) {
  columns[columnName].insert_value(string_to_char_pointer(value));
}

/* DESODBC:
    Original author: DESODBC Developer
*/
unsigned long Column::getLength(int row) {
  if (!values.empty() && values[row])
    return strlen(values[row]);
  else
    return 0;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
unsigned int Column::getDecimals() {
  if (this->field->type == DES_TYPE_FLOAT ||
      this->field->type == DES_TYPE_REAL) {
    unsigned int max_decimals = 0;
    for (auto value : values) {
      std::string val_str(value);
      size_t pos = val_str.find('.');
      if (pos != std::string::npos) {
        unsigned int decimals =
            val_str.substr(pos + 1, val_str.size() - (pos + 1)).size();
        if (decimals > max_decimals)
            max_decimals = decimals;
      }
    }

    return max_decimals;

  } else
    return 0;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
unsigned int Column::getColumnSize() {
  if (type.len == 0)
    return get_type_size(type.simple_type);
  else
    return type.len;

}

/* DESODBC:
    Original author: DESODBC Developer
*/
unsigned int Column::getMaxLength() {
  unsigned int max = 0;

  for (auto value : values) {
    if (value && strlen(value) > max) max = strlen(value);
  }

  SQLULEN type_size = this->get_simple_type();
  if (max > this->type.len)
    return max;
  else
    return this->type.len;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
DES_ROWS * Column::generate_DES_ROWS(int current_row) {
  DES_ROWS *rows = new DES_ROWS;
  if (!rows) throw std::bad_alloc();

  DES_ROWS *ptr = rows;
  for (int i = 0; current_row + i < values.size(); ++i) {
    ptr->data = new char *;
    if (!ptr->data) throw std::bad_alloc();

    *(ptr->data) = values[current_row + i];

    if (current_row + (i + 1) < values.size()) {
      ptr->next = new DES_ROWS;
      if (!ptr->next) throw std::bad_alloc();

      ptr = ptr->next;
    } else
      ptr->next = nullptr;

    // length doesn't seem to be modified, as I have verified debugging MySQL
    // ODBC. TODO: research
  }
  return rows;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
DES_FIELD * Column::get_DES_FIELD() {
  // We need to have these values updated right now
  this->field->max_length =
      getMaxLength();
  this->field->decimals = getDecimals();
  return this->field;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
Column::Column(const std::string &table_name, const std::string &col_name,
               const TypeAndLength &col_type, const SQLSMALLINT &col_nullable) {
  this->field = new DES_FIELD;
  if (!this->field) throw std::bad_alloc();

  this->type = col_type;

  /*
  When using this constructor, we are allocating in heap
  some values via string_to_char_pointer. We have to remember
  to delete them.
  */

  this->new_heap_used = true;

  this->field->name = string_to_char_pointer(col_name);
  this->field->org_name = string_to_char_pointer(col_name);
  this->field->name_length = col_name.size();
  this->field->org_name_length = col_name.size();

  this->field->table = string_to_char_pointer(table_name);
  this->field->org_table = string_to_char_pointer(table_name);
  this->field->table_length = table_name.size();
  this->field->org_table_length = table_name.size();

  this->field->db = "$des";  /*It does not matter whether it comes from
                                                     another database. This value will not be shown
                                                     to the user*/
  this->field->db_length = 4;

  this->field->catalog = "def";
  this->field->catalog_length = 3;

  this->field->def = nullptr;
  this->field->def_length = 0;

  this->field->flags = col_nullable == 1 ? NOT_NULL_FLAG : 0;

  this->field->decimals = 0;

  this->field->charsetnr = 48; //latin1

  this->field->extension = nullptr;

  this->field->type = col_type.simple_type;

  this->field->length = get_Type_size(col_type);

  this->field->max_length =
      0;  // the column right now is empty.
          // it will update when a function requires to know this value.
}

/* DESODBC:
    Original author: DESODBC Developer
*/
void Column::refresh_row(const int row_index) {
  /*
string_to_char_pointer(values[row_index], (char *)target_value_binding);
*str_len_or_ind_binding = values[row_index].size();
*/
}

/* DESODBC:
    Original author: DESODBC Developer
*/
void Column::update_row(const int row_index, char *value) {
  if (values[row_index]) delete values[row_index];
  values[row_index] = value;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
void Column::remove_row(const int row_index) {
  values.erase(values.begin() + row_index);
}

/* DESODBC:
    Original author: DESODBC Developer
*/
SQLSMALLINT Column::get_decimal_digits() {
  // It seems that there is no type in DES that has this attribute.
  // Consequently, we should always return zero.
  // Check
  // https://learn.microsoft.com/en-us/sql/odbc/reference/appendixes/decimal-digits?view=sql-server-ver16
  return 0;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
unsigned long *ResultTable::fetch_lengths(int current_row) {
  unsigned long *lengths =
      (unsigned long *)malloc(names_ordered.size() * sizeof(unsigned long));

  if (!lengths) {
    throw std::bad_alloc();
  }

  for (int i = 0; i < names_ordered.size(); ++i) {
    unsigned long *length = lengths + i;
    *length = columns[names_ordered[i]].getLength(current_row);
  }

  return lengths;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
DES_ROW ResultTable::generate_DES_ROW(const int index) {
  int n_cols = names_ordered.size();
  DES_ROW row = new char *[n_cols];

  if (!row)
    throw std::bad_alloc();

  for (int i = 0; i < n_cols; ++i) {
    row[i] = columns[names_ordered[i]].values[index];
  }

  return row;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
DES_ROWS *ResultTable::generate_DES_ROWS(const int current_row) {
  DES_ROWS *rows = new DES_ROWS;
  if (!rows) throw std::bad_alloc();

  rows->data = nullptr;
  rows->next = nullptr;

  DES_ROWS *ptr = rows;
  int n_rows =
      columns[names_ordered[0]].values.size(); //there will always be a column (the metadata ones)

  for (int i = 0; current_row + i < n_rows; ++i) {
    ptr->data = generate_DES_ROW(current_row + i);

    if (current_row + (i + 1) < n_rows) {
      ptr->next = new DES_ROWS;
      if (!ptr->next) throw std::bad_alloc();

      ptr = ptr->next;
    } else
      ptr->next = nullptr;

    // length doesn't seem to be modified, as I have verified debugging MySQL
    // ODBC.
  }
  return rows;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
DES_FIELD* ResultTable::get_DES_FIELD(int col_index) {
  return columns[names_ordered[col_index]].get_DES_FIELD();
}

/* DESODBC:
    Original author: DESODBC Developer
*/
std::vector<ForeignKeyInfo> ResultTable::getForeignKeysFromTAPI(
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

/* DESODBC:
    Original author: DESODBC Developer
*/
DBSchemaRelationInfo ResultTable::getRelationInfo(
    const std::vector<std::string> &lines,
                               int &index) {
  // We asume that the first given line by index is "$table" or "$view".
  // Leaves the index to the position when all we are
  // interested in has been already fetched.

  DBSchemaRelationInfo relation_info;

  if (lines[index] == "$table") {

    relation_info.is_table = true;

    index++;

    std::string relation_name = lines[index];
    relation_info.name = relation_name;

    index++;
    int col_index = 1;
    while (index < lines.size() && lines[index] != "$") {
      std::string column_name = lines[index];
      TypeAndLength type = get_Type_from_str(lines[index + 1]);

      relation_info.columns_index_map.insert({column_name, col_index});
      relation_info.columns_type_map.insert({column_name, type});

      col_index++;
      index += 2;  // in each iteration we are fetching name and type of each
                   // column
    }
    if (index < lines.size()) {
      /*
          NN
          $
          PK
          $
          CK
          ...
          CK
          $
          FK
          ...
          FK
          $
          FD
          ...
          FD
          $
          IC
          ...
          IC
      */
      index++;
      if (lines[index] != "$table" &&
          lines[index] != "$view" &&
          lines[index] !=
              "$eot") {  //== $table or $view will occur in external connections.

        // now, it ideally points to NN content; if not, to the bottom delimiter
        // of NN.
        if (lines[index] != "$") {
          relation_info.not_nulls =
              convertArrayNotationToStringVector(lines[index]);
          index += 2;
        } else  // i.e., it points to the bottom delimiter of NN = upper
                // delimiter of PK
          index += 1;

        // now, it ideally points to PK content; if not, to the bottom delimiter
        // of PK.
        if (lines[index] != "$") {
          relation_info.primary_keys =
              convertArrayNotationToStringVector(lines[index]);
          index += 2;
        } else {
          index += 1;
        }

        // now, it ideally points to CKs content; if not, to the bottom
        // delimiter of NN.

        while (lines[index] != "$") index++;  // we now ignore CKs

        index++;

        // now, it ideally points to FKs content; if not, to the bottom
        // delimiter of FK.
        if (lines[index] != "$")
          relation_info.foreign_keys = getForeignKeysFromTAPI(
              lines, index);  // this function updates the index by itself.
        else
          index++;

        // now, it ideally points to FDs content; if not, to the bottom
        // delimiter of FD.
        while (lines[index] != "$") index++;  // we now ignore FDs
        index++;
        // now, it ideally points to ICs content; if not, to the bottom
        // delimiter of IC.
        while (index < lines.size() && lines[index] != "$" &&
               lines[index] != "$table" && lines[index] != "$view" &&
               lines[index] != "$eot")
          index++;  // we now ignore ICs
      }
    }
  } else if (lines[index] == "$view") {

    relation_info.is_table = false;

    index++;

    if (lines[index] != "view" && lines[index] != "$eot") {

        // Now, we are in the first line of:
      /*
          relation_kind
          relation_name
          column_name
          type
          ...
          column_name
          type
          $
          SQL
          ...
          SQL
          $
          Datalog
          ...
          Datalog
          $eot
      */

      index++;  // we ignore relation_kind

      std::string relation_name = lines[index];
      relation_info.name = relation_name;

      index++;
      int col_index = 1;
      while (index < lines.size() && lines[index] != "$") {
        std::string column_name = lines[index];
        TypeAndLength type = get_Type_from_str(lines[index + 1]);

        relation_info.columns_index_map.insert({column_name, col_index});
        relation_info.columns_type_map.insert({column_name, type});

        col_index++;
        index += 2;  // in each iteration we are fetching name and type of each
                     // column
      }
      if (index < lines.size()) {
        index++;
        /*
        SQL
        ...
        SQL
        $
        Datalog
        ...
        Datalog
        $eot
        */
        // Now we are pointing to first SQL field or bottom delimiter of SQL
        // fields.
        while (lines[index] != "$") index++;  // we now ignore SQLs

        index++;
        // Now we are pointing to first Datalog field or bottom delimiter of
        // Datalog fields.

        while (index < lines.size() && lines[index] != "$" &&
               lines[index] != "$table" && lines[index] != "$view" &&
               lines[index] != "$eot")
          index++;  // we now ignore Datalog fields
      }

    }
  }

  return relation_info;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
std::unordered_map<std::string, DBSchemaRelationInfo> ResultTable::getAllRelationsInfo(
    const std::string &str) {
  // Table name -> DBSchemaRelationInfo structure
  std::unordered_map<std::string, DBSchemaRelationInfo> main_map;

  std::vector<std::string> lines = getLines(str);
  // Table name -> its TAPI output
  std::unordered_map<std::string, std::string> table_str_map;

  int i = 0;
  DBSchemaRelationInfo relation_info;
  while (true) {
    while (i < lines.size() && lines[i] != "$table" && lines[i] != "$view") {
      i++;
    }
    if (i == lines.size()) break;

    relation_info = getRelationInfo(lines, i);

    main_map.insert({relation_info.name, relation_info});
  }

  return main_map;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
void ResultTable::insert_cols(DES_FIELD array[], int array_size) {
  for (int i = 0; i < array_size; ++i) {
    DES_FIELD *field = new DES_FIELD;
    if (!field) throw std::bad_alloc();

    memcpy(field, &array[i], sizeof(DES_FIELD));
    insert_col(field);
  }
}

/* DESODBC:
    Original author: DESODBC Developer
*/
void ResultTable::build_table() {
  switch (this->command_type) {
    case SELECT:
      build_table_select();
      break;
    case SQLTABLES:
      build_table_SQLTables();
      break;
    case PROCESS:
      insert_metadata_cols();
      break;
    case SQLPRIMARYKEYS:
      build_table_SQLPrimaryKeys();
      break;
    case SQLFOREIGNKEYS_FK:
      build_table_SQLForeignKeys_FK();
      break;
    case SQLFOREIGNKEYS_PK:
      build_table_SQLForeignKeys_PK();
      break;
    case SQLFOREIGNKEYS_PKFK:
      build_table_SQLForeignKeys_PKFK();
      break;
    case SQLGETTYPEINFO:
      build_table_SQLGetTypeInfo();
      break;
    case SQLSTATISTICS:
      build_table_SQLStatistics();
      break;
    case SQLSPECIALCOLUMNS:
      build_table_SQLSpecialColumns();
      break;
    case SQLCOLUMNS:
      build_table_SQLColumns();
      break;
    default:
      insert_metadata_cols();
      break;
  }

}

/* DESODBC:
    Original author: DESODBC Developer
*/
ResultTable::ResultTable(COMMAND_TYPE type, const std::string& output) {
  this->command_type = type;
  this->str = output;

  this->build_table();
}

/* DESODBC:
    Original author: DESODBC Developer
*/
ResultTable::ResultTable(STMT *stmt) {
  this->dbc = stmt->dbc;
  this->params.column_name = stmt->params_for_table.column_name;
  this->params.table_name = stmt->params_for_table.table_name;
  this->params.table_type = stmt->params_for_table.table_type;
  this->command_type = stmt->type;
  this->params.type_requested = stmt->params_for_table.type_requested;
  this->params.pk_table_name = stmt->params_for_table.pk_table_name;
  this->params.fk_table_name = stmt->params_for_table.fk_table_name;
  this->params.catalog_name = stmt->params_for_table.catalog_name;
  if (stmt->dbc->env->odbc_ver == SQL_OV_ODBC2)
      this->params.metadata_id = true;
  else
    this->params.metadata_id = stmt->stmt_options.metadata_id;
  this->str = stmt->last_output;

  this->build_table();
}

/* DESODBC:
    Original author: DESODBC Developer
*/
void ResultTable::build_table_SQLForeignKeys_PK() {
  insert_SQLForeignKeys_cols();

  std::string pk_table_name = this->params.pk_table_name;

  std::unordered_map<std::string, DBSchemaRelationInfo> tables_info =
      getAllRelationsInfo(str);

  for (auto pair_name_table_info : tables_info) {
    std::string fk_table_name = pair_name_table_info.first;
    DBSchemaRelationInfo fk_table_info = pair_name_table_info.second;

    if (fk_table_info.is_table) {
      std::vector<std::string> keysReferringToPkTable;
      for (int i = 0; i < fk_table_info.foreign_keys.size(); ++i) {
        if (fk_table_info.foreign_keys[i].foreign_table == pk_table_name) {
          insert_value("PKTABLE_CAT", this->params.catalog_name);
          insert_value("PKTABLE_SCHEM", NULL_STR);
          insert_value("PKTABLE_NAME", pk_table_name);
          insert_value("PKCOLUMN_NAME",
                       fk_table_info.foreign_keys[i].foreign_key);
          insert_value("FKTABLE_CAT", this->params.catalog_name);
          insert_value("FKTABLE_SCHEM", NULL_STR);
          insert_value("FKTABLE_NAME", fk_table_name);
          insert_value("FKCOLUMN_NAME", fk_table_info.foreign_keys[i].key);
          insert_value(
              "KEY_SEQ",
              std::to_string(fk_table_info.columns_index_map
                                 [fk_table_info.foreign_keys[i]
                                      .key]));  // I assume that KEY_SEQ refers
                                                // to the foreign key in every
                                                // case. TODO: check
          insert_value("UPDATE_RULE", std::to_string(SQL_CASCADE)); //see 4.2.4.5   Renaming Tables.
          insert_value("DELETE_RULE", std::to_string(SQL_CASCADE)); //see 4.2.4.3   Dropping Tables
          insert_value("FK_NAME", fk_table_info.foreign_keys[i].key);
          insert_value("PK_NAME", fk_table_info.foreign_keys[i].foreign_key);
          insert_value("DEFERRABILITY", std::to_string(SQL_NOT_DEFERRABLE));
        }
      }
    }

  }
}

/* DESODBC:
    Original author: DESODBC Developer
*/
void ResultTable::build_table_SQLForeignKeys_FK() {
  insert_SQLForeignKeys_cols();

  std::string table_name = this->params.fk_table_name;

  std::unordered_map<std::string, DBSchemaRelationInfo> tables_info =
      getAllRelationsInfo(str);

  DBSchemaRelationInfo table_info = tables_info[table_name];

  if (table_info.is_table) {
    std::vector<ForeignKeyInfo> foreign_keys = table_info.foreign_keys;

    for (int i = 0; i < foreign_keys.size(); ++i) {
      insert_value("PKTABLE_CAT", this->params.catalog_name);
      insert_value("PKTABLE_SCHEM", NULL_STR);
      insert_value("PKTABLE_NAME", foreign_keys[i].foreign_table);
      insert_value("PKCOLUMN_NAME", foreign_keys[i].foreign_key);
      insert_value("FKTABLE_CAT", this->params.catalog_name);
      insert_value("FKTABLE_SCHEM", NULL_STR);
      insert_value("FKTABLE_NAME", this->params.fk_table_name);
      insert_value("FKCOLUMN_NAME", foreign_keys[i].key);
      insert_value(
          "KEY_SEQ",
          std::to_string(
              table_info.columns_index_map
                  [foreign_keys[i].key]));  // I assume that KEY_SEQ
                                            // refers to the foreign key
                                            // in every case. TODO: check
      insert_value(
          "UPDATE_RULE",
          std::to_string(SQL_CASCADE));  // see 4.2.4.5   Renaming Tables.
      insert_value(
          "DELETE_RULE",
          std::to_string(SQL_CASCADE));  // see 4.2.4.3   Dropping Tables
      insert_value("FK_NAME", foreign_keys[i].key);
      insert_value("PK_NAME", foreign_keys[i].foreign_key);
      insert_value("DEFERRABILITY", std::to_string(SQL_NOT_DEFERRABLE)); //not deferrable as it does not apply to DES
    }
  }
}

/* DESODBC:
    Original author: DESODBC Developer
*/
void ResultTable::build_table_SQLForeignKeys_PKFK() {
  insert_SQLForeignKeys_cols();

  std::string pk_table_name = this->params.pk_table_name;
  std::string fk_table_name = this->params.fk_table_name;

  std::unordered_map<std::string, DBSchemaRelationInfo> tables_info =
      getAllRelationsInfo(str);

  for (auto pair_name_table_info : tables_info) {
    std::string local_fk_table_name = pair_name_table_info.first;
    DBSchemaRelationInfo local_fk_table_info = pair_name_table_info.second;

    if (local_fk_table_info.is_table) {
      std::vector<std::string> keysReferringToPkTable;
      if (local_fk_table_name ==
          fk_table_name) { /* this if condition is the only
                              difference with the
                              parse_sqlforeign_pk
                              function.
                              */
        for (int i = 0; i < local_fk_table_info.foreign_keys.size(); ++i) {
          if (local_fk_table_info.foreign_keys[i].foreign_table ==
              pk_table_name) {
            insert_value("PKTABLE_CAT", this->params.catalog_name);
            insert_value("PKTABLE_SCHEM", NULL_STR);
            insert_value("PKTABLE_NAME", pk_table_name);
            insert_value("PKCOLUMN_NAME",
                         local_fk_table_info.foreign_keys[i].foreign_key);
            insert_value("FKTABLE_CAT", this->params.catalog_name);
            insert_value("FKTABLE_SCHEM", NULL_STR);
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
            insert_value(
                "UPDATE_RULE",
                std::to_string(SQL_CASCADE));  // see 4.2.4.5   Renaming Tables.
            insert_value(
                "DELETE_RULE",
                std::to_string(SQL_CASCADE));  // see 4.2.4.3   Dropping Tables
            insert_value("FK_NAME", local_fk_table_info.foreign_keys[i].key);
            insert_value("PK_NAME",
                         local_fk_table_info.foreign_keys[i].foreign_key);
            insert_value("DEFERRABILITY", std::to_string(SQL_NOT_DEFERRABLE));
          }
        }
      }
    }

  }
}

/* DESODBC:
    Original author: DESODBC Developer
*/
void ResultTable::build_table_SQLColumns() {
  insert_SQLColumns_cols();

  std::string catalog_name_param = this->params.catalog_name;

  std::string table_name_search = this->params.table_name;

  std::string column_name_search = this->params.column_name;

  std::vector<std::string> dbs;
  auto pair = dbc->send_query_and_read("/show_dbs");
  SQLRETURN rc = pair.first;
  std::string dbs_str = pair.second;
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    return;
  }

  std::vector<std::string> candidate_dbs = getLines(dbs_str);
  candidate_dbs.erase(
      std::remove(candidate_dbs.begin(), candidate_dbs.end(), "$eot"),
      candidate_dbs.end());

  dbs = filter_candidates(candidate_dbs, catalog_name_param,
                          this->params.metadata_id);

  for (int i = 0; i < dbs.size(); ++i) {
    std::string query_dbschema = "/dbschema ";
    query_dbschema += dbs[i];
    auto pair = this->dbc->send_query_and_read(query_dbschema);
    SQLRETURN rc = pair.first;
    std::string dbschema_str = pair.second;
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
      return;
    }

    std::unordered_map<std::string, DBSchemaRelationInfo> map =
        getAllRelationsInfo(dbschema_str);

    std::vector<std::string> dbschema_tables;
    for (auto pair : map) {
      if (pair.second.is_table)  // we only deal with tables in SQLColumns.
        dbschema_tables.push_back(pair.first);
    }

    std::vector<std::string> dbschema_table_names = filter_candidates(
        dbschema_tables, table_name_search, this->params.metadata_id);

    for (auto dbschema_table_name : dbschema_table_names) {

      pair = this->dbc->send_query_and_read("/current_db");
      rc = pair.first;
      if (!SQL_SUCCEEDED(rc)) return;

      std::string current_db_output = pair.second;
      std::string previous_db = getLines(current_db_output)[0];

      std::string query_usedb = "/use_db ";
      query_usedb += dbs[i];

      pair = this->dbc->send_query_and_read(query_usedb);
      rc = pair.first;
      if (!SQL_SUCCEEDED(rc)) return;

      std::string select_query = "select * from " + dbschema_table_name;
      pair = dbc->send_query_and_read(select_query);
      rc = pair.first;
      std::string select_query_output = pair.second;
      if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        return;
      }

      std::string query_usepreviousdb = "/use_db ";
      query_usepreviousdb += previous_db;

      pair = this->dbc->send_query_and_read(query_usepreviousdb);
      rc = pair.first;
      if (!SQL_SUCCEEDED(rc)) return;

      ResultTable table(SELECT, select_query_output);

      std::vector<std::string> col_names = table.names_ordered;
      col_names = filter_candidates(col_names, column_name_search,
                                    this->params.metadata_id);

      for (int j = 0; j < col_names.size(); ++j) {
        Column col = table.columns[col_names[j]];
        DES_FIELD *field = col.get_DES_FIELD();
        insert_value("TABLE_CAT", dbs[i]);
        insert_value("TABLE_SCHEM", NULL_STR);
        insert_value("TABLE_NAME", dbschema_table_name);
        insert_value("COLUMN_NAME", col_names[j]);

        enum_field_types des_type = field->type;
        int sql_type = des_type_2_sql_type(des_type);
        insert_value("DATA_TYPE", std::to_string(sql_type));
        insert_value("TYPE_NAME", des_type_2_str(des_type));

        insert_value("COLUMN_SIZE", std::to_string(col.getColumnSize()));

        TypeAndLength tal = {col.get_simple_type(), col.getMaxLength()};
        insert_value("BUFFER_LENGTH",
                     std::to_string(get_transfer_octet_length(tal)));

        insert_value("DECIMAL_DIGITS", std::to_string(field->decimals));

        if (is_numeric_des_data_type(des_type)) {
          insert_value("NUM_PREC_RADIX", std::string("10"));
        } else
          insert_value("NUM_PREC_RADIX", NULL_STR);

        insert_value("NULLABLE",
                     std::to_string(
                         SQL_NULLABLE_UNKNOWN));
        insert_value("REMARKS", std::string(""));
        insert_value(
            "COLUMN_DEF", std::string("NULL"));
        if (sql_type == SQL_TYPE_DATE)
          insert_value("SQL_DATA_TYPE", std::to_string(SQL_DATETIME));
        else
          insert_value("SQL_DATA_TYPE", std::to_string(sql_type));

        if (sql_type == SQL_TYPE_DATE)
          insert_value("SQL_DATETIME_SUB", std::to_string(SQL_CODE_DATE));
        else if (sql_type == SQL_TYPE_TIME)
          insert_value("SQL_DATETIME_SUB", std::to_string(SQL_CODE_TIME));
        else if (sql_type == SQL_TYPE_TIMESTAMP)
          insert_value("SQL_DATETIME_SUB", std::to_string(SQL_CODE_TIMESTAMP));
        else
          insert_value("SQL_DATETIME_SUB", std::to_string(0));

        if (is_character_des_type(field))) {
      insert_value("CHAR_OCTET_LENGTH", std::to_string(tal.len));
    }
        else
          insert_value("CHAR_OCTET_LENGTH", NULL_STR);

        insert_value("ORDINAL_POSITION", std::to_string(j + 1));
        insert_value("IS_NULLABLE", std::string("YES"));
      }
    }

  }

}

/* DESODBC:
    Original author: DESODBC Developer
*/
void ResultTable::build_table_SQLPrimaryKeys() {
  insert_SQLPrimaryKeys_cols();

  // First, we separate the TAPI str into lines.
  std::vector<std::string> lines = getLines(str);

  int i = 0;
  DBSchemaRelationInfo table_info = getRelationInfo(lines, i);

  if (table_info.primary_keys.size() == 0) return;

  for (int i = 0; i < table_info.primary_keys.size(); ++i) {
    std::string TABLE_CAT = this->params.catalog_name;
    std::string TABLE_SCHEM = "";
    std::string TABLE_NAME = this->params.table_name;
    std::string COLUMN_NAME = table_info.primary_keys[i];
    int KEY_SEQ = table_info.columns_index_map[table_info.primary_keys[i]];
    std::string PK_NAME = "";

    insert_value("TABLE_CAT", TABLE_CAT);
    insert_value("TABLE_SCHEM", NULL_STR);
    insert_value("TABLE_NAME", TABLE_NAME);
    insert_value("COLUMN_NAME", COLUMN_NAME);
    insert_value("KEY_SEQ", std::to_string(KEY_SEQ));
    insert_value("PK_NAME", COLUMN_NAME);
  }
}

/* DESODBC:
    Original author: DESODBC Developer
*/
void ResultTable::build_table_SQLTables() {
  insert_metadata_cols();

  std::string table_name_param = this->params.table_name;
  std::string catalog_name_param = this->params.catalog_name;
  std::string table_type_param = this->params.table_type;

  std::vector<std::string> specified_table_types;

  if (table_type_param.size() > 0) {
    std::transform(table_type_param.begin(), table_type_param.end(),
                   table_type_param.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    table_type_param.erase(
        std::remove(table_type_param.begin(), table_type_param.end(), '\''),
        table_type_param.end());

    std::stringstream ss(table_type_param);
    std::string element_str;

    while (std::getline(ss, element_str, ',')) {
      specified_table_types.push_back(
          element_str.substr(0, element_str.size()));
    }
  }

  std::vector<std::string> dbs;
  auto pair = dbc->send_query_and_read("/show_dbs");
  SQLRETURN rc = pair.first;
  std::string dbs_str = pair.second;
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    return;
  }

  std::vector<std::string> candidate_dbs = getLines(dbs_str);
  candidate_dbs.erase(
      std::remove(candidate_dbs.begin(), candidate_dbs.end(), "$eot"),
      candidate_dbs.end());

  dbs = filter_candidates(candidate_dbs, catalog_name_param,
                          this->params.metadata_id);


  if (catalog_name_param == SQL_ALL_CATALOGS && table_name_param.size() == 0) {
    for (int i = 0; i < dbs.size(); ++i) {
      insert_value("TABLE_CAT", dbs[i]);
      insert_value("TABLE_SCHEM", NULL_STR);
      insert_value("TABLE_NAME", NULL_STR);
      insert_value("TABLE_TYPE", NULL_STR);
      insert_value("REMARKS", NULL_STR);
    }

  } else if (table_type_param == SQL_ALL_TABLE_TYPES &&
             table_name_param.size() == 0 && catalog_name_param.size() == 0) {
    for (int i = 0; i < supported_table_types.size(); ++i) {
      insert_value("TABLE_CAT", NULL_STR);
      insert_value("TABLE_SCHEM", NULL_STR);
      insert_value("TABLE_NAME", NULL_STR);
      insert_value("TABLE_TYPE", supported_table_types[i]);
      insert_value("REMARKS", NULL_STR);
    }

  } else { //standard case
    for (int i = 0; i < dbs.size(); ++i) {
      std::string dbschema_query = "/dbschema ";
      dbschema_query += dbs[i];
      auto pair = dbc->send_query_and_read(dbschema_query);
      SQLRETURN rc = pair.first;
      std::string dbschema_query_output = pair.second;
      if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
        return;
      }

      std::unordered_map<std::string, DBSchemaRelationInfo> map =
          getAllRelationsInfo(dbschema_query_output);

      std::vector<std::string> dbschema_tables;
      for (auto pair : map) {
        dbschema_tables.push_back(pair.first);
      }

      std::vector<std::string> dbschema_table_names = filter_candidates(
          dbschema_tables, table_name_param, this->params.metadata_id);

      std::vector<std::string> lines = getLines(dbschema_query_output);

      int j = 0;

      while (j < lines.size()) {
        if (lines[j] == "$eot") break;
        std::string TABLE_CAT = dbs[i];
        std::string TABLE_SCHEM = "";
        std::string TABLE_NAME = "";
        std::string TABLE_TYPE = "";
        std::string REMARKS = "";

        if (lines[j] == "$table") {
          TABLE_TYPE = "TABLE";
          j++;
          TABLE_NAME = lines[j];
        } else if (lines[j] == "$view") {
          TABLE_TYPE = "VIEW";
          j += 2;
          TABLE_NAME = lines[j];
        } else
          break;

        bool table_in_requested_tables = true;
        if (table_name_param.size() > 0) {
          table_in_requested_tables =
              (std::find(dbschema_table_names.begin(),
                         dbschema_table_names.end(),
                         TABLE_NAME) != dbschema_table_names.end());
        }

        bool type_compatible = true;
        if (specified_table_types.size() > 0) {  // e.d. the user manually put types
        if (table_type_param != SQL_ALL_TABLE_TYPES &&
            table_type_param.size() > 0) {
            std::string table_type_lowered = TABLE_TYPE;
            std::transform(table_type_lowered.begin(),
                            table_type_lowered.end(),
                            table_type_lowered.begin(),
                            [](unsigned char c) { return std::tolower(c); });

            if (std::find(specified_table_types.begin(),
                        specified_table_types.end(),
                        table_type_lowered) == specified_table_types.end())
            type_compatible = false;
        }
        }

        if (type_compatible && table_in_requested_tables) {
          insert_value("TABLE_CAT", TABLE_CAT);
          insert_value("TABLE_SCHEM", NULL_STR);
          insert_value("TABLE_NAME", TABLE_NAME);
          insert_value("TABLE_TYPE", TABLE_TYPE);
          insert_value("REMARKS", REMARKS);
        }

        while (lines[j].size() <= 1 || lines[j][0] != '$') j++;
      }
    }
  }
  
}

/* DESODBC:
    Original author: DESODBC Developer
*/
void ResultTable::build_table_select() {

  // First, we separate the TAPI str into lines.
  std::vector<std::string> lines = getLines(str);

  std::vector<std::string> column_names;

  // We ignore the first line, that contains the keywords success/answer.
  int i = 1;

  //No columns: we are dealing with an empty table.
  if (lines[i] == "$")
      return;

  // If the first message is not answer, we can be sure that the output
  // originates from a non-select query. We then create the default
  // metadata table on the else clause.
  if (lines.size() >= 1 && lines[0] == "answer") {
    bool checked_cols = false;

    while (!checked_cols && lines[i] != "$eot") {
      // For each column, TAPI gives in a line its name and then its type.
      size_t pos_dot = lines[i].find('.', 0);
      std::string table = lines[i].substr(0, pos_dot);
      this->table_name = table;
      std::string name = lines[i].substr(pos_dot + 1, lines[i].size() - (pos_dot+1));
      TypeAndLength type = get_Type_from_str(lines[i + 1]);

      // I have put SQL_NULLABLE_UNKNOWN because I do not know
      // if a result table from select may have an attribute "nullable"
      insert_col(table, name, type, SQL_NULLABLE_UNKNOWN);

      column_names.push_back(name);

      i += 2;

      if (lines[i] == "$") checked_cols = true;
    }

    i++;  // We skip the final $
    if (i == lines.size()) return;
    std::string aux = lines[i];
    while (aux != "$eot") {
      for (int j = 0; j < column_names.size(); ++j) {
        std::string name_col = column_names[j];
        std::string value = lines[i];

        if (value == "null")
          insert_value(name_col, nullptr);
        else {
          // When we reach a varchar value, we remove the " ' " characters
          // provided by the TAPI.
          if (value.size() > 0 && value[0] == '\'') {
            value = value.substr(1, value.size() - 2);
          }

          insert_value(name_col, value);
        }

        i++;
      }
      aux = lines[i];
      i++;  // to skip $ or $eot
    }
  } else {  // if it is not possible, we create the default metadata table.
    insert_metadata_cols();
  }
}

/* DESODBC:
    Original author: DESODBC Developer
*/
void ResultTable::build_table_SQLStatistics() {
  insert_SQLStatistics_cols();

  std::string catalog_name = this->params.catalog_name;
  std::string table_name = this->params.table_name;

  std::string select_query = "select * from " + table_name;
  auto pair = dbc->send_query_and_read(select_query);
  SQLRETURN rc = pair.first;
  std::string select_query_output = pair.second;
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    return;
  }
  ResultTable select_table(SELECT, select_query_output);

  insert_value("TABLE_CAT", catalog_name);
  insert_value("TABLE_SCHEM", NULL_STR);
  insert_value("TABLE_NAME", table_name);
  insert_value("NON_UNIQUE", NULL_STR);
  insert_value("INDEX_QUALIFIER", NULL_STR);
  insert_value("INDEX_NAME", NULL_STR);
  insert_value("TYPE", std::to_string(SQL_TABLE_STAT));
  insert_value("ORDINAL_POSITION", NULL_STR);
  insert_value("COLUMN_NAME", NULL_STR);
  insert_value("ASC_OR_DESC", NULL_STR);
  insert_value("CARDINALITY", std::to_string(select_table.row_count()));
  insert_value("PAGES", NULL_STR);
  insert_value("FILTER_CONDITION", NULL_STR);
  
}

/* DESODBC:
    Original author: DESODBC Developer
*/
void ResultTable::build_table_SQLSpecialColumns() {
  insert_SQLSpecialColumns_cols();

  std::string table_name = this->params.table_name;

  std::string main_query = "/dbschema ";
  main_query += table_name;
  auto pair = dbc->send_query_and_read(main_query);
  SQLRETURN rc = pair.first;
  std::string main_output = pair.second;
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    return;
  }

  std::vector<std::string> lines = getLines(main_output);
  int index = 0;
  DBSchemaRelationInfo table_info = getRelationInfo(lines, index);

  //We need the table so as to know the buffer length for character data types.
  std::string select_query = "select * from ";
  select_query += table_name;
  pair = dbc->send_query_and_read(select_query);
  rc = pair.first;
  std::string select_query_output = pair.second;
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {
    return;
  }

  ResultTable table(SELECT, select_query_output);

  for (int i = 0; i < table_info.primary_keys.size(); ++i) {
    std::string primary_key = table_info.primary_keys[i];
    TypeAndLength type = table_info.columns_type_map.at(primary_key);
    insert_value("SCOPE", std::to_string(SQL_SCOPE_SESSION));
    insert_value("COLUMN_NAME", primary_key);
    insert_value("DATA_TYPE", std::to_string(type.simple_type));
    insert_value("TYPE_NAME", Type_to_type_str(type));
    insert_value("COLUMN_SIZE", std::to_string(get_Type_size(type)));

    if (is_character_des_data_type(type.simple_type) && type.len == UINT64_MAX) {
      insert_value("BUFFER_LENGTH",
                   std::to_string(table.columns[primary_key].getMaxLength()));
    } else
      insert_value("BUFFER_LENGTH",
                   std::to_string(get_transfer_octet_length(type)));
      
    insert_value("DECIMAL_DIGITS", NULL_STR);
    insert_value("PSEUDO_COLUMN", std::to_string(SQL_PC_NOT_PSEUDO));
  }
}

/* DESODBC:
    Original author: DESODBC Developer
*/
void ResultTable::build_table_SQLGetTypeInfo() {
  insert_SQLGetTypeInfo_cols();
  SQLSMALLINT type_requested = this->params.type_requested;

  for (int i = 0; i < supported_types.size(); i++) {
    enum_field_types des_type = supported_types[i];
    std::string des_type_name = des_type_2_str(des_type);
    SQLSMALLINT sql_data_type = des_type_2_sql_type(des_type);

    if (type_requested == sql_data_type ||
        type_requested == SQL_ALL_TYPES) {

        bool type_is_character_data = is_character_des_data_type(des_type);
        bool type_is_time_data = is_time_des_data_type(des_type);

        insert_value("TYPE_NAME", des_type_name);
        insert_value("DATA_TYPE", std::to_string(sql_data_type));
        if (des_type_name ==
            "char")  // special case (char is not considered in SQL standards)
        insert_value("COLUMN_SIZE", std::to_string(1));
        else
        insert_value("COLUMN_SIZE",
                        std::to_string(get_type_size(des_type)));

        if (type_is_character_data) {
        insert_value("LITERAL_PREFIX", std::string("\'"));
        insert_value("LITERAL_SUFFIX", std::string("\'"));
        } else {
        insert_value("LITERAL_PREFIX", NULL_STR);
        insert_value("LITERAL_SUFFIX", NULL_STR);
        }
        if (is_in_string(des_type_name, "(N)"))
          insert_value("CREATE_PARAMS", std::string("length"));
        else
          insert_value("CREATE_PARAMS", NULL_STR);

        insert_value("NULLABLE", std::to_string(SQL_NULLABLE));  //every data type can be null given some column.

        if (type_is_character_data && !type_is_time_data) {
        insert_value("CASE_SENSITIVE", std::to_string(SQL_TRUE));
        } else {
        insert_value("CASE_SENSITIVE", std::to_string(SQL_FALSE));
        }

        // TODO: check if the following is correct according
        // to DES policies
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


        insert_value("AUTO_UNIQUE_VALUE", std::to_string(SQL_FALSE));

        insert_value("LOCAL_TYPE_NAME", des_type_name);

        if (is_decimal_des_data_type(des_type)) {
          insert_value("MINIMUM_SCALE",
                       std::to_string(0));
          insert_value("MAXIMUM_SCALE",
                       std::to_string(53));  // 53 is the number of maximum decimals in a double precision datatype (DES' real and float)
        } else {
          insert_value("MINIMUM_SCALE",
                       NULL_STR);
          insert_value("MAXIMUM_SCALE",
                       NULL_STR);
        }
        
        insert_value("SQL_DATATYPE", std::to_string(sql_data_type));

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
        else  // is numeric
        insert_value("NUM_PREC_RADIX", std::string("10"));

        insert_value("INTERVAL_PRECISION", NULL_STR);
    }
  }
}