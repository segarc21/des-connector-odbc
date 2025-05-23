// Copyright (c) 2007, 2024, Oracle and/or its affiliates.
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
  @file  utility.c
  @brief Utility functions
*/

#include "driver.h"
#include "errmsg.h"
#include <ctype.h>
#include <iostream>
#include <map>

#define DATETIME_DIGITS 14

const SQLULEN sql_select_unlimited= (SQLULEN)-1;

/**
  Figure out the ODBC result types for each column in the result set.

  @param[in] stmt The statement with result types to be fixed.
*/
void fix_result_types(STMT *stmt)
{
  uint i;
  DES_RESULT *result= stmt->result;
  DESCREC *irrec;
  DES_FIELD *field;
  int capint32= stmt->dbc->ds.opt_COLUMN_SIZE_S32 ? 1 : 0;

  stmt->state= ST_EXECUTED;  /* Mark set found */

  /* Populate the IRD records */
  size_t f_count = stmt->field_count();
  for (i= 0; i < f_count; ++i)
  {
    irrec= desc_get_rec(stmt->ird, i, TRUE);
    /* TODO function for this */
    field= result->fields + i;

    irrec->row.field= field;
    irrec->type= get_sql_data_type(stmt, field, NULL);
    irrec->concise_type= get_sql_data_type(stmt, field,
                                           (char *)irrec->row.type_name);
    switch (irrec->concise_type)
    {
    case SQL_DATE:
    case SQL_TYPE_DATE:
    case SQL_TIME:
    case SQL_TYPE_TIME:
    case SQL_TIMESTAMP:
    case SQL_TYPE_TIMESTAMP:
      irrec->type= SQL_DATETIME;
      break;
    default:
      irrec->type= irrec->concise_type;
      break;
    }
    irrec->datetime_interval_code=
      get_dticode_from_concise_type(irrec->concise_type);
    irrec->type_name= (SQLCHAR *) irrec->row.type_name;
    irrec->length= get_column_size(stmt, field);
    irrec->octet_length= get_transfer_octet_length(stmt, field);
    irrec->display_size= get_display_size(stmt, field);
    /* According ODBC specs(http://msdn.microsoft.com/en-us/library/ms713558%28v=VS.85%29.aspx)
      "SQL_DESC_OCTET_LENGTH ... For variable-length character or binary types,
      this is the maximum length in bytes. This value does not include the null
      terminator" Thus there is no need to add 1 to octet_length for char types */
    irrec->precision= 0;
    /* Set precision for all non-char/blob types */
    switch (irrec->type)
    {
    case SQL_BINARY:
    case SQL_BIT:
    case SQL_CHAR:
    case SQL_WCHAR:
    case SQL_VARBINARY:
    case SQL_VARCHAR:
    case SQL_WVARCHAR:
    case SQL_LONGVARBINARY:
    case SQL_LONGVARCHAR:
    case SQL_WLONGVARCHAR:
      break;
    default:
      irrec->precision= (SQLSMALLINT) irrec->length;
      break;
    }
    irrec->scale= myodbc_max(0, get_decimal_digits(stmt, field));
    if ((field->flags & NOT_NULL_FLAG) &&
        !(field->type == DES_TYPE_TIMESTAMP) &&
        !(field->flags & AUTO_INCREMENT_FLAG))
      irrec->nullable= SQL_NO_NULLS;
    else
      irrec->nullable= SQL_NULLABLE;
    irrec->table_name= (SQLCHAR *)field->table;
    irrec->name= (SQLCHAR *)field->name;
    irrec->label= (SQLCHAR *)field->name;
    if (field->flags & AUTO_INCREMENT_FLAG)
      irrec->auto_unique_value= SQL_TRUE;
    else
      irrec->auto_unique_value= SQL_FALSE;
    /* We need support from server, when aliasing is there */
    irrec->base_column_name= (SQLCHAR *)field->org_name;
    irrec->base_table_name= (SQLCHAR *)field->org_table;
    if (field->flags & BINARY_FLAG) /* TODO this doesn't cut it anymore */
      irrec->case_sensitive= SQL_TRUE;
    else
      irrec->case_sensitive= SQL_FALSE;

    if (field->db && *field->db)
    {
        irrec->catalog_name= (SQLCHAR *)field->db;
    }
    else
    {
      irrec->catalog_name= (SQLCHAR *)(stmt->dbc->database.c_str());
    }

    irrec->fixed_prec_scale= SQL_FALSE;
    switch (field->type)
    {
    case DES_TYPE_STRING:
      if (field->charsetnr == BINARY_CHARSET_NUMBER)
      {
        irrec->literal_prefix= (SQLCHAR *) "0x";
        irrec->literal_suffix= (SQLCHAR *) "";
        // The charset number must be only changed for JSON
        break;
      }
      /* FALLTHROUGH */

    case DES_TYPE_DATE:
    case DES_TYPE_DATETIME:
    case DES_TYPE_TIMESTAMP:
    case DES_TYPE_TIME:
      irrec->literal_prefix= (SQLCHAR *) "'";
      irrec->literal_suffix= (SQLCHAR *) "'";
      break;

    default:
      irrec->literal_prefix= (SQLCHAR *) "";
      irrec->literal_suffix= (SQLCHAR *) "";
    }
    switch (field->type) {
      case DES_TYPE_SHORT:
      case DES_TYPE_LONG:
        irrec->num_prec_radix = 10;

    /* overwrite irrec->precision set above */
    case DES_TYPE_FLOAT:
      irrec->num_prec_radix= 2;
      irrec->precision= 23;
      break;

    default:
      irrec->num_prec_radix= 0;
      break;
    }
    irrec->schema_name= (SQLCHAR *) "";
    /*
      We limit BLOB/TEXT types to SQL_PRED_CHAR due an oversight in ADO
      causing problems with updatable cursors.
    */
    switch (irrec->concise_type)
    {
      case SQL_LONGVARBINARY:
      case SQL_LONGVARCHAR:
      case SQL_WLONGVARCHAR:
        irrec->searchable= SQL_PRED_CHAR;
        break;
      default:
        irrec->searchable= SQL_SEARCHABLE;
        break;
    }
    irrec->unnamed= SQL_NAMED;
    if (field->flags & UNSIGNED_FLAG)
      irrec->is_unsigned= SQL_TRUE;
    else
      irrec->is_unsigned= SQL_FALSE;
    if (field->table && *field->table)
      irrec->updatable= SQL_ATTR_READWRITE_UNKNOWN;
    else
      irrec->updatable= SQL_ATTR_READONLY;
  }
}

/*
  Copy a field to a byte string.

  @param[in]     stmt         Pointer to statement
  @param[out]    result       Buffer for result
  @param[in]     result_bytes Size of result buffer (in bytes)
  @param[out]    avail_bytes  Pointer to buffer for storing number of bytes
                              available as result
  @param[in]     field        Field being stored
  @param[in]     src          Source data for result
  @param[in]     src_bytes    Length of source data (in bytes)

  @return Standard ODBC result code
*/
SQLRETURN
copy_binary_result(STMT *stmt,
                   SQLCHAR *result, SQLLEN result_bytes, SQLLEN *avail_bytes,
                   DES_FIELD *field __attribute__((unused)),
                   char *src, unsigned long src_bytes)
{
  SQLRETURN rc= SQL_SUCCESS;
  ulong copy_bytes;

  if (!result_bytes)
    result= 0;       /* Don't copy anything! */

  assert(stmt);
  /* Apply max length to source data, if one was specified. */
  if (stmt->stmt_options.max_length &&
      src_bytes > stmt->stmt_options.max_length)
    src_bytes = (unsigned long)stmt->stmt_options.max_length;

  /* Initialize the source offset */
  if (!stmt->getdata.source)
    stmt->getdata.source= src;
  else
  {
    src_bytes -= (unsigned long)(stmt->getdata.source - src);
    src= stmt->getdata.source;

    /* If we've already retrieved everything, return SQL_NO_DATA_FOUND */
    if (src_bytes == 0)
      return SQL_NO_DATA_FOUND;
  }

  copy_bytes= desodbc_min((unsigned long)result_bytes, src_bytes);

  if (result && stmt->stmt_options.retrieve_data)
    memcpy(result, src, copy_bytes);

  if (avail_bytes && stmt->stmt_options.retrieve_data)
    *avail_bytes= src_bytes;

  stmt->getdata.source+= copy_bytes;

  if (src_bytes > (unsigned long)result_bytes)
  {
    stmt->set_error("01004", "String data, right truncated");
    rc= SQL_SUCCESS_WITH_INFO;
  }

  return rc;
}


/*
  Copy a field to an ANSI result string.

  @param[in]     stmt         Pointer to statement
  @param[out]    result       Buffer for result
  @param[in]     result_bytes Size of result buffer (in bytes)
  @param[out]    avail_bytes  Pointer to buffer for storing number of bytes
                              available as result
  @param[in]     field        Field being stored
  @param[in]     src          Source data for result
  @param[in]     src_bytes    Length of source data (in bytes)

  @return Standard ODBC result code
*/
SQLRETURN
copy_ansi_result(STMT *stmt,
                 SQLCHAR *result, SQLLEN result_bytes, SQLLEN *avail_bytes,
                 DES_FIELD *field, char *src, unsigned long src_bytes)
{
  SQLRETURN rc= SQL_SUCCESS;

  if (!result_bytes)
    result= 0;       /* Don't copy anything! */

  SQLLEN bytes;
  if (!avail_bytes)
    avail_bytes= &bytes;

  if (!result_bytes && !stmt->getdata.source)
  {
    *avail_bytes= src_bytes;
    stmt->set_error("01004", "String data, right truncated");
    return SQL_SUCCESS_WITH_INFO;
  }

  if (result_bytes)
    --result_bytes;

  rc= copy_binary_result(stmt, result, result_bytes, avail_bytes,
                          field, src, src_bytes);

  if (SQL_SUCCEEDED(rc) && result && stmt->stmt_options.retrieve_data)
    result[desodbc_min(*avail_bytes, result_bytes)]= '\0';

  return rc;
}


/**
  Copy a result from the server into a buffer as a SQL_C_WCHAR.

  @param[in]     stmt        Pointer to statement
  @param[out]    result      Buffer for result
  @param[in]     result_len  Size of result buffer (in characters)
  @param[out]    avail_bytes Pointer to buffer for storing amount of data
                             available before this call
  @param[in]     field       Field being stored
  @param[in]     src         Source data for result
  @param[in]     src_bytes   Length of source data (in bytes)

  @return Standard ODBC result code
*/
SQLRETURN
copy_wchar_result(STMT *stmt,
                  SQLWCHAR *result, SQLINTEGER result_len, SQLLEN *avail_bytes,
                  DES_FIELD *field, char *src, long src_bytes)
{
  SQLRETURN rc= SQL_SUCCESS;
  char *src_end;
  SQLWCHAR *result_end;
  ulong used_chars= 0, error_count= 0;
  desodbc::CHARSET_INFO *from_cs = utf8_charset_info;

  if (!result_len)
    result= NULL; /* Don't copy anything! */

  result_end= result + result_len - 1;

  if (result == result_end)
  {
    *result= 0;
    result= 0;
  }

  /* Apply max length to source data, if one was specified. */
  if (stmt->stmt_options.max_length &&
      (ulong)src_bytes > stmt->stmt_options.max_length)
    src_bytes = (long)stmt->stmt_options.max_length;
  src_end= src + src_bytes;

  /* Initialize the source data */
  if (!stmt->getdata.source)
    stmt->getdata.source= src;
  else
    src= stmt->getdata.source;

  /* If we've already retrieved everything, return SQL_NO_DATA_FOUND */
  if (stmt->getdata.dst_bytes != (ulong)~0L &&
      stmt->getdata.dst_offset >= stmt->getdata.dst_bytes)
    return SQL_NO_DATA_FOUND;

  /* We may have a leftover char from the last call. */
  if (stmt->getdata.latest_bytes)
  {
    if (stmt->stmt_options.retrieve_data)
      memcpy(result, stmt->getdata.latest, sizeof(SQLWCHAR));
    ++result;

    if (result == result_end)
    {
      if (stmt->stmt_options.retrieve_data)
        *result= 0;
      result= NULL;
    }

    used_chars+= 1;
    stmt->getdata.latest_bytes= 0;
  }

  while (src < src_end)
  {
    /* Find the conversion functions. */
    auto mb_wc = from_cs->cset->mb_wc;
    auto wc_mb = utf16_charset_info->cset->wc_mb;
    desodbc::my_wc_t wc = 0;
    UTF16 ubuf[5] = {0, 0, 0, 0, 0};
    int to_cnvres;

    int cnvres= (*mb_wc)(from_cs, &wc, (uchar *)src, (uchar *)src_end);

    if (cnvres == MY_CS_ILSEQ)
    {
      ++error_count;
      cnvres= 1;
      wc= '?';
    }
    else if (cnvres < 0 && cnvres > MY_CS_TOOSMALL)
    {
      ++error_count;
      cnvres= abs(cnvres);
      wc= '?';
    }
    else if (cnvres < 0)
      return stmt->set_error("HY000",
                            "Unknown failure when converting character "
                            "from server character set.");

convert_to_out:
    // SQLWCHAR data should be UTF-16 on all platforms
    to_cnvres = (*wc_mb)(utf16_charset_info,
      wc, (uchar *)ubuf, (uchar *)ubuf + sizeof(ubuf));

    // Get the number of wide chars written
    size_t wchars_written = to_cnvres / 2;

    if (wchars_written > 0)
    {
      src+= cnvres;
      if (result)
      {
        if (stmt->stmt_options.retrieve_data)
          *result = ubuf[0];
        result++;
      }

      used_chars += (ulong)wchars_written;

      if (wchars_written > 1)
      {
        if (result && result != result_end)
        {
          if (stmt->stmt_options.retrieve_data)
            *result = ubuf[1];
          result++;
        }
        else if (result)
        {
          *((SQLWCHAR *)stmt->getdata.latest) = ubuf[1];
          stmt->getdata.latest_bytes = sizeof(SQLWCHAR);
          stmt->getdata.latest_used = 0;
          if (stmt->stmt_options.retrieve_data)
            *result = 0;
          result = NULL;

          if (stmt->getdata.dst_bytes != (ulong)~0L)
          {
            stmt->getdata.source+= cnvres;
            break;
          }
        }
        else
        {
          continue;
        }
      }

      if (result)
        stmt->getdata.source+= cnvres;

      if (result && result == result_end)
      {
        if (stmt->stmt_options.retrieve_data)
          *result= 0;
        result= NULL;
      }
    }
    else if (stmt->getdata.latest_bytes == MY_CS_ILUNI && wc != '?')
    {
      ++error_count;
      wc= '?';
      goto convert_to_out;
    }
    else
      return stmt->set_error("HY000",
                            "Unknown failure when converting character "
                            "to result character set.");
  }

  if (result && stmt->stmt_options.retrieve_data)
    *result= 0;

  if (result_len && stmt->getdata.dst_bytes == (ulong)~0L)
  {
    stmt->getdata.dst_bytes= used_chars * sizeof(SQLWCHAR);
    stmt->getdata.dst_offset= 0;
  }

  if (avail_bytes && stmt->stmt_options.retrieve_data)
  {
    if (result_len)
      *avail_bytes= stmt->getdata.dst_bytes - stmt->getdata.dst_offset;
    else
      *avail_bytes= used_chars * sizeof(SQLWCHAR);
  }

  stmt->getdata.dst_offset+= desodbc_min((ulong)(result_len ?
                                                result_len - 1 : 0),
                                        used_chars) * sizeof(SQLWCHAR);

  /* Did we truncate the data? */
  if (!result_len || stmt->getdata.dst_bytes > stmt->getdata.dst_offset)
  {
    stmt->set_error("01004", "String data, right truncated");
    rc= SQL_SUCCESS_WITH_INFO;
  }

  /* Did we encounter any character conversion problems? */
  if (error_count)
  {
    stmt->set_error("22018", "Invalid character value for cast specification");
    rc= SQL_SUCCESS_WITH_INFO;
  }

  return rc;
}


/*
  @type    : myodbc internal
  @purpose : is used when converting binary data to hexadecimal
*/
template <typename T>
SQLRETURN copy_binhex_result(STMT *stmt,
                             T *rgbValue, SQLINTEGER cbValueMax,
                             SQLLEN *pcbValue,
                             char *src, ulong src_length)
{
  /** @todo padding of BINARY */
  T *dst = (T*) rgbValue;
  ulong length;
  ulong max_length = (ulong)stmt->stmt_options.max_length;
  ulong *offset = &stmt->getdata.src_offset;
  T NEAR _dig_vec[] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
  };

  if (!cbValueMax)
  {
    /* Don't copy anything! */
    dst = 0;
  }

  if (max_length) /* If limit on char lengths */
  {
    set_if_smaller(cbValueMax, (long) max_length + 1);
    set_if_smaller(src_length, (max_length + 1) / 2);
  }

  if (*offset == (ulong) ~0L)
  {
    *offset = 0;   /* First call */
  }
  else if (*offset >= src_length)
  {
    return SQL_NO_DATA_FOUND;
  }

  src += *offset;
  src_length -= *offset;
  length = cbValueMax ? (ulong)(cbValueMax - 1) / 2 : 0;
  length = desodbc_min(src_length, length);
  (*offset) += length;     /* Fix for next call */
  if (pcbValue && stmt->stmt_options.retrieve_data)
    *pcbValue = src_length * 2 * sizeof(T);

  /* Bind allows null pointers */
  if (dst && stmt->stmt_options.retrieve_data)
  {
    ulong i;
    for (i= 0; i < length; ++i)
    {
      *dst ++= _dig_vec[(uchar) *src >> 4];
      *dst ++= _dig_vec[(uchar) *src++ & 15];
    }
    *dst = 0;
  }

  // All data is received when offset is not before
  // the end of the source
  if ( *offset * sizeof(T) >= src_length )
      return SQL_SUCCESS;

  stmt->set_error("01004", "String data, right truncated");
  return SQL_SUCCESS_WITH_INFO;
}

// Need to declare template specializations to avoid linker errors
template
SQLRETURN copy_binhex_result<SQLCHAR>(STMT* stmt,
  SQLCHAR* rgbValue, SQLINTEGER cbValueMax,
  SQLLEN* pcbValue, char* src, ulong src_length);

template
SQLRETURN copy_binhex_result<SQLWCHAR>(STMT* stmt,
  SQLWCHAR* rgbValue, SQLINTEGER cbValueMax,
  SQLLEN* pcbValue, char* src, ulong src_length);

/*
  @type    : myodbc internal
  @purpose : is used when converting a bit a SQL_C_CHAR
*/

template<typename T>
SQLRETURN do_copy_bit_result(STMT *stmt,
                             T *result, SQLLEN result_bytes,
                             SQLLEN *avail_bytes,
                             DES_FIELD *field __attribute__((unused)),
                             char *src, unsigned long src_bytes)
{
  // We need 2 chars, otherwise Don't copy anything! */
  if (result_bytes < 2)
    result = 0;

  /* Apply max length to source data, if one was specified. */
  if (stmt->stmt_options.max_length &&
      src_bytes > stmt->stmt_options.max_length)
    src_bytes = (unsigned long)stmt->stmt_options.max_length;

  /* Initialize the source offset */
  if (!stmt->getdata.source)
  {
    stmt->getdata.source= src;
  }
  else
  {
    src_bytes -= (unsigned long)(stmt->getdata.source - src);
    src= stmt->getdata.source;

    /* If we've already retrieved everything, return SQL_NO_DATA_FOUND */
    if (src_bytes == 0)
    {
      return SQL_NO_DATA_FOUND;
    }
  }

  if (result && stmt->stmt_options.retrieve_data)
  {
    result[0] = *src ? '1' : '0';
    result[1] = '\0';
  }


  if (avail_bytes && stmt->stmt_options.retrieve_data)
  {
    *avail_bytes= sizeof(T);
  }

  stmt->getdata.source++;

  return SQL_SUCCESS;
}

SQLRETURN wcopy_bit_result(STMT *stmt,
                          SQLWCHAR *result, SQLLEN result_bytes, SQLLEN *avail_bytes,
                          DES_FIELD *field __attribute__((unused)),
                          char *src, unsigned long src_bytes)
{
  return do_copy_bit_result<SQLWCHAR>(stmt, result, result_bytes, avail_bytes, field,
                            src, src_bytes);
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
std::map<std::string, int> sql_data_types_map = {
    {"varchar", SQL_LONGVARCHAR},
    {"string", SQL_LONGVARCHAR},
    {"char(N)", SQL_CHAR},
    {"varchar(N)", SQL_CHAR},
    {"char", SQL_CHAR},
    {"integer_des", SQL_BIGINT},
    {"int", SQL_BIGINT},
    {"float", SQL_DOUBLE},
    {"real", SQL_DOUBLE},
    {"date", SQL_TYPE_DATE},
    {"time", SQL_TYPE_TIME},
    {"datetime", SQL_DATETIME},
    {"timestamp", SQL_TYPE_TIMESTAMP}
};

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/**
  Get the SQL data type and (optionally) type name for a DES_FIELD.

  @param[in]  stmt
  @param[in]  field
  @param[out] buff

  @return  The SQL data type.
*/
SQLSMALLINT get_sql_data_type(STMT *stmt, DES_FIELD *field, char *buff)
{
  my_bool field_is_binary= (field->charsetnr == BINARY_CHARSET_NUMBER ? 1 : 0) &&
                           ((field->org_table_length > 0 ? 1 : 0) ||
                            !stmt->dbc->ds.opt_NO_BINARY_RESULT);

  switch (field->type) {
    case DES_TYPE_VARCHAR:
      if (buff) (void)desodbc_stpmov(buff, "varchar");
      return SQL_LONGVARCHAR;
    case DES_TYPE_STRING:
      if (buff) (void)desodbc_stpmov(buff, "string");
      return SQL_LONGVARCHAR;
    case DES_TYPE_CHAR_N:
      if (buff) (void)desodbc_stpmov(buff, "char");
      return SQL_CHAR;
    case DES_TYPE_VARCHAR_N:
      if (buff) (void)desodbc_stpmov(buff, "varchar");
      return SQL_VARCHAR;
    case DES_TYPE_CHAR:
      if (buff) (void)desodbc_stpmov(buff, "char");
      return SQL_CHAR;
    case DES_TYPE_INTEGER:
      if (buff) (void)desodbc_stpmov(buff, "integer_des");
      return SQL_BIGINT;
    case DES_TYPE_INT:
      if (buff) (void)desodbc_stpmov(buff, "int");
      return SQL_BIGINT;
    case DES_TYPE_SHORT:
      if (buff) (void)desodbc_stpmov(buff, "smallint");
      return SQL_SMALLINT;
    case DES_TYPE_LONG:
      if (buff) (void)desodbc_stpmov(buff, "integer");
      return SQL_INTEGER;
    case DES_TYPE_FLOAT:
      if (buff) (void)desodbc_stpmov(buff, "float");
      return SQL_DOUBLE;
    case DES_TYPE_REAL:
      if (buff) (void)desodbc_stpmov(buff, "real");
      return SQL_DOUBLE;
    case DES_TYPE_DATE:
      if (buff) (void)desodbc_stpmov(buff, "date");
      if (stmt->dbc->env->odbc_ver == SQL_OV_ODBC3) return SQL_TYPE_DATE;
      return SQL_DATE;
    case DES_TYPE_TIME:
      if (buff) (void)desodbc_stpmov(buff, "time");
      if (stmt->dbc->env->odbc_ver == SQL_OV_ODBC3) return SQL_TYPE_TIME;
      return SQL_TIME;
    case DES_TYPE_DATETIME:
      if (buff) (void)desodbc_stpmov(buff, "datetime");
      return SQL_DATETIME;
    case DES_TYPE_TIMESTAMP:
      if (buff) (void)desodbc_stpmov(buff, "timestamp");
      if (stmt->dbc->env->odbc_ver == SQL_OV_ODBC3) return SQL_TYPE_TIMESTAMP;
      return SQL_TYPE_TIMESTAMP;
  }

  if (buff)
    *buff= '\0';
  return SQL_UNKNOWN_TYPE;
}


/**
  Fill the transfer octet length buffer accordingly to size of SQLLEN
  @param[in,out]  buff
  @param[in]      stmt
  @param[in]      field

  @return  void
*/
SQLLEN fill_transfer_oct_len_buff(char *buff, STMT *stmt, DES_FIELD *field)
{
  /* The only possible negative value get_transfer_octet_length can return is SQL_NO_TOTAL
     But it can return value which is greater that biggest signed integer(%ld).
     Thus for other values we use %lu. %lld should fit
     all (currently) possible in mysql values.
  */
  SQLLEN len= get_transfer_octet_length(stmt, field);

  sprintf(buff, len == SQL_NO_TOTAL ? "%d" : (sizeof(SQLLEN) == 4 ? "%lu" : "%lld"), len );

  return len;
}


/**
  Fill the column size buffer accordingly to size of SQLULEN
  @param[in,out]  buff
  @param[in]      stmt
  @param[in]      field

  @return  void
*/
SQLULEN fill_column_size_buff(char *buff, STMT *stmt, DES_FIELD *field)
{
  SQLULEN size= get_column_size(stmt, field);
  sprintf(buff, (size== SQL_NO_TOTAL ? "%d" :
      (sizeof(SQLULEN) == 4 ? "%lu" : "%llu")), size);
  return size;
}


/**
  Capping length value if connection option is set
*/
static SQLLEN cap_length(STMT *stmt, unsigned long real_length)
{
  if (stmt->dbc->ds.opt_COLUMN_SIZE_S32 != 0 && real_length > INT_MAX32)
    return INT_MAX32;

  return real_length;
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/**
  Get the column size (in characters) of a field, as defined at:
    http://msdn2.microsoft.com/en-us/library/ms711786.aspx

  @param[in]  stmt
  @param[in]  field

  @return  The column size of the field
*/
SQLULEN get_column_size(STMT *stmt, DES_FIELD *field)
{
  SQLULEN length= field->length;
  /* Work around a bug in some versions of the server. */
  if (field->max_length > field->length)
    length= field->max_length;

  //length= cap_length(stmt, (unsigned long)length);

  switch (field->type) {

  case DES_TYPE_VARCHAR:
  case DES_TYPE_STRING:  
  case DES_TYPE_CHAR_N:
  case DES_TYPE_VARCHAR_N:
    return DES_MAX_STRLEN;
  case DES_TYPE_CHAR:
    return 1;
  case DES_TYPE_INTEGER:
  case DES_TYPE_INT:
    return 19;
  case DES_TYPE_SHORT:
    return 5;
  case DES_TYPE_LONG:
    return 10;
  case DES_TYPE_FLOAT:
  case DES_TYPE_REAL:
    return 53;
  case DES_TYPE_DATE:
    return 10;
  case DES_TYPE_TIME:
    return 8;
  case DES_TYPE_DATETIME:
  case DES_TYPE_TIMESTAMP:
    return 19;
  }

  return SQL_NO_TOTAL;
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/**
  Get the decimal digits of a field, as defined at:
    http://msdn2.microsoft.com/en-us/library/ms709314.aspx

  @param[in]  stmt
  @param[in]  field

  @return  The decimal digits, or @c SQL_NO_TOTAL where it makes no sense

  The function has to return SQLSMALLINT, since it corresponds to SQL_DESC_SCALE
  or SQL_DESC_PRECISION for some data types.
*/
SQLSMALLINT get_decimal_digits(STMT *stmt __attribute__((unused)),
                          DES_FIELD *field)
{
  switch (field->type) {
    case DES_TYPE_FLOAT:
    case DES_TYPE_REAL:
      return field->decimals;


  /* All exact numeric types. */
    case DES_TYPE_INTEGER:
    case DES_TYPE_INT:
    case DES_TYPE_SHORT:
    case DES_TYPE_LONG:
    case DES_TYPE_DATE:
    case DES_TYPE_TIME:
    case DES_TYPE_DATETIME:
    case DES_TYPE_TIMESTAMP:
      return 0;

  default:
    /*
       This value is only used in some catalog functions. It's co-erced
       to zero for all descriptor use.
    */
    return SQL_NO_TOTAL;
  }
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/**
  Get the transfer octet length of a field, as defined at:
    http://msdn2.microsoft.com/en-us/library/ms713979.aspx

  @param[in]  stmt
  @param[in]  field

  @return  The transfer octet length
*/

SQLLEN get_transfer_octet_length(TypeAndLength tal) {
  switch (tal.simple_type) {
    case DES_TYPE_VARCHAR:
    case DES_TYPE_STRING:
    case DES_TYPE_CHAR_N:
    case DES_TYPE_VARCHAR_N: {
      return tal.len;
    }
    case DES_TYPE_CHAR:
      return 1;
    case DES_TYPE_INTEGER:
    case DES_TYPE_INT:
      return 8;
    case DES_TYPE_SHORT:
      return 2;
    case DES_TYPE_LONG:
      return 4;
    case DES_TYPE_FLOAT:
    case DES_TYPE_REAL:
      return 8;
    case DES_TYPE_DATE:
      return sizeof(SQL_DATE_STRUCT);
    case DES_TYPE_TIME:
      return sizeof(SQL_TIME_STRUCT);
    case DES_TYPE_DATETIME:
    case DES_TYPE_TIMESTAMP:
      return sizeof(SQL_TIMESTAMP_STRUCT);
  }

  return SQL_NO_TOTAL;
}

/* DESODBC:
    Implemented for DES using the mapping DES data type
    -> SQL data type -> length, as defined in that url.
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLLEN get_transfer_octet_length(STMT *stmt, DES_FIELD *field)
{
  SQLLEN length;
  if (field->length > INT_MAX64)
    length= INT_MAX64;
  else
    length= field->length;

  TypeAndLength tal;
  tal.simple_type = field->type;
  tal.len = length;

  return get_transfer_octet_length(tal);

}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/**
  Get the display size of a field, as defined at:
    http://msdn2.microsoft.com/en-us/library/ms713974.aspx

  @param[in]  stmt
  @param[in]  field

  @return  The display size
*/
SQLLEN get_display_size(STMT *stmt __attribute__((unused)),DES_FIELD *field)
{
  int capint32 = stmt->dbc->ds.opt_COLUMN_SIZE_S32 ? 1 : 0;
  unsigned int mbmaxlen = get_charset_maxlen(field->charsetnr);

  switch (field->type) {
    case DES_TYPE_VARCHAR:
    case DES_TYPE_STRING:
    case DES_TYPE_CHAR_N:
    case DES_TYPE_VARCHAR_N: {
      unsigned long length;
      if (field->charsetnr == BINARY_CHARSET_NUMBER)
        length = field->length * 2;
      else
        length = field->length / mbmaxlen;

      if (capint32 && length > INT_MAX32) length = INT_MAX32;
      return length;
    }
    case DES_TYPE_CHAR:
      return 1;
    case DES_TYPE_INTEGER:
    case DES_TYPE_INT:
      return 20;
    case DES_TYPE_SHORT:
      return 5;
    case DES_TYPE_LONG:
      return 10;
    case DES_TYPE_FLOAT:
    case DES_TYPE_REAL:
      return 24;
    case DES_TYPE_DATE:
      return 10;
    case DES_TYPE_TIME:
      return 8;
    case DES_TYPE_DATETIME:
    case DES_TYPE_TIMESTAMP:
      return 19;
  }

  return SQL_NO_TOTAL;
}


/*
   Map the concise type (value or param) to the correct datetime or
   interval code.
   See SQLSetDescField()/SQL_DESC_DATETIME_INTERVAL_CODE docs for details.
*/
SQLSMALLINT
get_dticode_from_concise_type(SQLSMALLINT concise_type)
{
  /* figure out SQL_DESC_DATETIME_INTERVAL_CODE from SQL_DESC_CONCISE_TYPE */
  switch (concise_type)
  {
  case SQL_C_TYPE_DATE:
    return SQL_CODE_DATE;
  case SQL_C_TYPE_TIME:
    return SQL_CODE_TIME;
  case SQL_C_TIMESTAMP:
  case SQL_C_TYPE_TIMESTAMP:
    return SQL_CODE_TIMESTAMP;
  case SQL_C_INTERVAL_DAY:
    return SQL_CODE_DAY;
  case SQL_C_INTERVAL_DAY_TO_HOUR:
    return SQL_CODE_DAY_TO_HOUR;
  case SQL_C_INTERVAL_DAY_TO_MINUTE:
    return SQL_CODE_DAY_TO_MINUTE;
  case SQL_C_INTERVAL_DAY_TO_SECOND:
    return SQL_CODE_DAY_TO_SECOND;
  case SQL_C_INTERVAL_HOUR:
    return SQL_CODE_HOUR;
  case SQL_C_INTERVAL_HOUR_TO_MINUTE:
    return SQL_CODE_HOUR_TO_MINUTE;
  case SQL_C_INTERVAL_HOUR_TO_SECOND:
    return SQL_CODE_HOUR_TO_SECOND;
  case SQL_C_INTERVAL_MINUTE:
    return SQL_CODE_MINUTE;
  case SQL_C_INTERVAL_MINUTE_TO_SECOND:
    return SQL_CODE_MINUTE_TO_SECOND;
  case SQL_C_INTERVAL_MONTH:
    return SQL_CODE_MONTH;
  case SQL_C_INTERVAL_SECOND:
    return SQL_CODE_SECOND;
  case SQL_C_INTERVAL_YEAR:
    return SQL_CODE_YEAR;
  case SQL_C_INTERVAL_YEAR_TO_MONTH:
    return SQL_CODE_YEAR_TO_MONTH;
  default:
    return 0;
  }
}


/*
   Map the SQL_DESC_DATETIME_INTERVAL_CODE to the SQL_DESC_CONCISE_TYPE
   for datetime types.

   Constant returned is valid for both param and value types.
*/
SQLSMALLINT get_concise_type_from_datetime_code(SQLSMALLINT dticode)
{
  switch (dticode)
  {
  case SQL_CODE_DATE:
    return SQL_C_TYPE_DATE;
  case SQL_CODE_TIME:
    return SQL_C_TYPE_DATE;
  case SQL_CODE_TIMESTAMP:
    return SQL_C_TYPE_TIMESTAMP;
  default:
    return 0;
  }
}


/*
   Map the SQL_DESC_DATETIME_INTERVAL_CODE to the SQL_DESC_CONCISE_TYPE
   for interval types.

   Constant returned is valid for both param and value types.
*/
SQLSMALLINT get_concise_type_from_interval_code(SQLSMALLINT dticode)
{
  switch (dticode)
  {
  case SQL_CODE_DAY:
    return SQL_C_INTERVAL_DAY;
  case SQL_CODE_DAY_TO_HOUR:
    return SQL_C_INTERVAL_DAY_TO_HOUR;
  case SQL_CODE_DAY_TO_MINUTE:
    return SQL_C_INTERVAL_DAY_TO_MINUTE;
  case SQL_CODE_DAY_TO_SECOND:
    return SQL_C_INTERVAL_DAY_TO_SECOND;
  case SQL_CODE_HOUR:
    return SQL_C_INTERVAL_HOUR;
  case SQL_CODE_HOUR_TO_MINUTE:
    return SQL_C_INTERVAL_HOUR_TO_MINUTE;
  case SQL_CODE_HOUR_TO_SECOND:
    return SQL_C_INTERVAL_HOUR_TO_SECOND;
  case SQL_CODE_MINUTE:
    return SQL_C_INTERVAL_MINUTE;
  case SQL_CODE_MINUTE_TO_SECOND:
    return SQL_C_INTERVAL_MINUTE_TO_SECOND;
  case SQL_CODE_MONTH:
    return SQL_C_INTERVAL_MONTH;
  case SQL_CODE_SECOND:
    return SQL_C_INTERVAL_SECOND;
  case SQL_CODE_YEAR:
    return SQL_C_INTERVAL_YEAR;
  case SQL_CODE_YEAR_TO_MONTH:
    return SQL_C_INTERVAL_YEAR_TO_MONTH;
  default:
    return 0;
  }
}


/*
   Map the concise type to a (possibly) more general type.
*/
SQLSMALLINT get_type_from_concise_type(SQLSMALLINT concise_type)
{
  /* set SQL_DESC_TYPE from SQL_DESC_CONCISE_TYPE */
  switch (concise_type)
  {
  /* datetime data types */
  case SQL_C_TYPE_DATE:
  case SQL_C_TYPE_TIME:
  case SQL_C_TYPE_TIMESTAMP:
    return SQL_DATETIME;
  /* interval data types */
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
  case SQL_C_INTERVAL_HOUR_TO_MINUTE:
  case SQL_C_INTERVAL_HOUR_TO_SECOND:
  case SQL_C_INTERVAL_MINUTE_TO_SECOND:
    return SQL_INTERVAL;
  /* else, set same */
  default:
    return concise_type;
  }
}


/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
/*
  @type    : myodbc internal
  @purpose : returns internal type to C type
*/
//See https://learn.microsoft.com/en-us/sql/odbc/reference/appendixes/c-data-types?view=sql-server-ver16
int unireg_to_c_datatype(DES_FIELD *field)
{
    switch ( field->type )
    {
      case DES_TYPE_VARCHAR:
      case DES_TYPE_STRING:
      case DES_TYPE_CHAR_N:
      case DES_TYPE_VARCHAR_N:
        return SQL_C_CHAR;
      case DES_TYPE_CHAR:
        return SQL_C_UTINYINT;
      case DES_TYPE_INTEGER:
      case DES_TYPE_INT:
        return SQL_C_SBIGINT;
      case DES_TYPE_FLOAT:
      case DES_TYPE_REAL:
        return SQL_C_DOUBLE;
      case DES_TYPE_DATE:
        return SQL_C_DATE;
      case DES_TYPE_TIME:
        return SQL_C_TIME;
      case DES_TYPE_DATETIME:
      case DES_TYPE_TIMESTAMP:
        return SQL_C_TIMESTAMP;
      case DES_TYPE_SHORT:
        return SQL_C_SSHORT;
      case DES_TYPE_LONG:
        return SQL_C_LONG;
      default:
        return SQL_C_CHAR;
    }
}


/*
  @type    : myodbc internal
  @purpose : returns default C type for a given SQL type
*/

int default_c_type(int sql_data_type)
{
    switch ( sql_data_type )
    {
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:
        case SQL_DECIMAL:
        case SQL_NUMERIC:
        default:
            return SQL_C_CHAR;
        case SQL_BIGINT:
            return SQL_C_SBIGINT;
        case SQL_BIT:
            return SQL_C_BIT;
        case SQL_TINYINT:
            return SQL_C_TINYINT;
        case SQL_SMALLINT:
            return SQL_C_SHORT;
        case SQL_INTEGER:
            return SQL_C_LONG;
        case SQL_REAL:
        case SQL_FLOAT:
            return SQL_C_FLOAT;
        case SQL_DOUBLE:
            return SQL_C_DOUBLE;
        case SQL_BINARY:
        case SQL_VARBINARY:
        case SQL_LONGVARBINARY:
            return SQL_C_BINARY;
        case SQL_DATE:
        case SQL_TYPE_DATE:
            return SQL_C_DATE;
        case SQL_TIME:
        case SQL_TYPE_TIME:
            return SQL_C_TIME;
        case SQL_TIMESTAMP:
        case SQL_TYPE_TIMESTAMP:
            return SQL_C_TIMESTAMP;
    }
}


/*
  @type    : myodbc internal
  @purpose : returns bind length
*/

ulong bind_length(int sql_data_type,ulong length)
{
    switch ( sql_data_type )
    {
        case SQL_C_BIT:
        case SQL_C_TINYINT:
        case SQL_C_STINYINT:
        case SQL_C_UTINYINT:
            return 1;
        case SQL_C_SHORT:
        case SQL_C_SSHORT:
        case SQL_C_USHORT:
            return 2;
        case SQL_C_LONG:
        case SQL_C_SLONG:
        case SQL_C_ULONG:
            return sizeof(SQLINTEGER);
        case SQL_C_FLOAT:
            return sizeof(float);
        case SQL_C_DOUBLE:
            return sizeof(double);
        case SQL_C_DATE:
        case SQL_C_TYPE_DATE:
            return sizeof(DATE_STRUCT);
        case SQL_C_TIME:
        case SQL_C_TYPE_TIME:
            return sizeof(TIME_STRUCT);
        case SQL_C_TIMESTAMP:
        case SQL_C_TYPE_TIMESTAMP:
            return sizeof(TIMESTAMP_STRUCT);
        case SQL_C_SBIGINT:
        case SQL_C_UBIGINT:
            return sizeof(longlong);
        case SQL_C_NUMERIC:
          return sizeof(SQL_NUMERIC_STRUCT);
        default:                  /* For CHAR, VARCHAR, BLOB, DEFAULT...*/
            return length;
    }
}


/**
  Get bookmark value from SQL_ATTR_FETCH_BOOKMARK_PTR buffer

  @param[in]  fCType      ODBC C type to return data as
  @param[out] rgbValue    Pointer to buffer for returning data
*/
SQLLEN get_bookmark_value(SQLSMALLINT fCType, SQLPOINTER rgbValue)
{
  switch (fCType)
  {
  case SQL_C_CHAR:
  case SQL_C_BINARY:
    return atol((const char*) rgbValue);

  case SQL_C_WCHAR:
    return sqlwchartoul((SQLWCHAR *)rgbValue);

  case SQL_C_TINYINT:
  case SQL_C_STINYINT:
  case SQL_C_UTINYINT:
  case SQL_C_SHORT:
  case SQL_C_SSHORT:
  case SQL_C_USHORT:
  case SQL_C_LONG:
  case SQL_C_SLONG:
  case SQL_C_ULONG:
  case SQL_C_FLOAT:
  case SQL_C_DOUBLE:
  case SQL_C_SBIGINT:
  case SQL_C_UBIGINT:
    return *((SQLLEN *) rgbValue);
  }
   return 0;
}


/*
  @type    : myodbc internal
  @purpose : convert a possible string to a timestamp value
*/

int str_to_ts(SQL_TIMESTAMP_STRUCT *ts, const char *str, int len, int zeroToMin,
              BOOL dont_use_set_locale)
{
    uint year, length;
    char buff[DATETIME_DIGITS + 1], *to;
    const char *end;
    SQL_TIMESTAMP_STRUCT tmp_timestamp;
    SQLUINTEGER fraction;

    if ( !ts )
    {
      ts= (SQL_TIMESTAMP_STRUCT *) &tmp_timestamp;
    }

    /* SQL_NTS is (naturally) negative and is caught as well */
    if (len < 0)
    {
      len = (int)strlen(str);
    }

    /* We don't wan to change value in the out parameter directly
       before we know that string is a good datetime */
    end= get_fractional_part(str, len, dont_use_set_locale, &fraction);

    if (end == NULL || end > str + len)
    {
      end= str + len;
    }

    for ( to= buff; str < end; ++str )
    {
      if ( isdigit(*str) )
      {
        if (to < buff+sizeof(buff)-1)
        {
          *to++= *str;
        }
        else
        {
          /* We have too many numbers in the string and we not gonna tolerate it */
          return SQLTS_BAD_DATE;
        }
      }
    }

    length= (uint) (to-buff);

    if ( length == 6 || length == 12 )  /* YYMMDD or YYMMDDHHMMSS */
    {
      memmove(buff+2, buff, length);

      if ( buff[0] <= '6' )
      {
          buff[0]='2';
          buff[1]='0';
      }
      else
      {
          buff[0]='1';
          buff[1]='9';
      }

      length+= 2;
      to+= 2;
    }

    if (length < DATETIME_DIGITS)
    {
      desodbc::strfill(buff + length, DATETIME_DIGITS - length, '0');
    }
    else
    {
      *to= 0;
    }

    year= (digit(buff[0])*1000+digit(buff[1])*100+digit(buff[2])*10+digit(buff[3]));

    if (!strncmp(&buff[4], "00", 2) || !strncmp(&buff[6], "00", 2))
    {
      if (!zeroToMin) /* Don't convert invalid */
        return SQLTS_NULL_DATE;

      /* convert invalid to min allowed */
      if (!strncmp(&buff[4], "00", 2))
        buff[5]= '1';
      if (!strncmp(&buff[6], "00", 2))
        buff[7]= '1';
    }

    ts->year=     year;
    ts->month=    digit(buff[4])*10+digit(buff[5]);
    ts->day=      digit(buff[6])*10+digit(buff[7]);
    ts->hour=     digit(buff[8])*10+digit(buff[9]);
    ts->minute=   digit(buff[10])*10+digit(buff[11]);
    ts->second=   digit(buff[12])*10+digit(buff[13]);
    ts->fraction= fraction;

    return 0;
}

/*
  @type    : myodbc internal
  @purpose : convert a possible string to a time value
*/

my_bool str_to_time_st(SQL_TIME_STRUCT *ts, const char *str)
{
    char buff[24],*to, *tokens[3] = {0, 0, 0};
    int num= 0, int_hour=0, int_min= 0, int_sec= 0;
    SQL_TIME_STRUCT tmp_time;

    if ( !ts )
        ts= (SQL_TIME_STRUCT *) &tmp_time;

    /* remember the position of the first numeric string */
    tokens[0]= buff;

    for ( to= buff ; *str && to < buff+sizeof(buff)-1 ; ++str )
    {
        if (isdigit(*str))
            *to++= *str;
        else if (num < 2)
        {
          /*
            terminate the string and remember the beginning of the
            new one only if the time component number is not out of
            range
          */
          *to++= 0;
          tokens[++num]= to;
        }
        else
          /* We can leave the loop now */
          break;
    }
    /* Put the final termination character */
    *to= 0;

    int_hour= tokens[0] ? atoi(tokens[0]) : 0;
    int_min=  tokens[1] ? atoi(tokens[1]) : 0;
    int_sec=  tokens[2] ? atoi(tokens[2]) : 0;

    /* Convert seconds into minutes if necessary */
    if (int_sec > 59)
    {
      int_min+= int_sec / 60;
      int_sec= int_sec % 60;
    }

    /* Convert minutes into hours if necessary */
    if (int_min > 59)
    {
      int_hour+= int_min / 60;
      int_min= int_min % 60;
    }

    ts->hour   = (SQLUSMALLINT)(int_hour < 65536 ? int_hour : 65535);
    ts->minute = (SQLUSMALLINT)int_min;
    ts->second = (SQLUSMALLINT)int_sec;

    return 0;
}

/*
  @type    : myodbc internal
  @purpose : convert a possible string to a data value. if
             zeroToMin is specified, YEAR-00-00 dates will be
             converted to the min valid ODBC date
*/

my_bool str_to_date(SQL_DATE_STRUCT *rgbValue, const char *str,
                    uint length, int zeroToMin)
{
    uint field_length,year_length,digits,i,date[3];
    const char *pos;
    const char *end= str+length;
    for ( ; !isdigit(*str) && str != end ; ++str ) ;
    /*
      Calculate first number of digits.
      If length= 4, 8 or >= 14 then year is of format YYYY
      (YYYY-MM-DD,  YYYYMMDD)
    */
    for ( pos= str; pos != end && isdigit(*pos) ; ++pos ) ;
    digits= (uint) (pos-str);
    year_length= (digits == 4 || digits == 8 || digits >= 14) ? 4 : 2;
    field_length= year_length-1;

    for ( i= 0 ; i < 3 && str != end; ++i )
    {
        uint tmp_value= (uint) (uchar) (*str++ - '0');
        while ( str != end && isdigit(str[0]) && field_length-- )
        {
            tmp_value= tmp_value*10 + (uint) (uchar) (*str - '0');
            ++str;
        }
        date[i]= tmp_value;
        while ( str != end && !isdigit(*str) )
            ++str;
        field_length= 1;   /* Rest fields can only be 2 */
    }
    if (i <= 1 || (i > 1 && !date[1]) || (i > 2 && !date[2]))
    {
      if (!zeroToMin) /* Convert? */
        return 1;

      rgbValue->year=  date[0];
      rgbValue->month= (i > 1 && date[1]) ? date[1] : 1;
      rgbValue->day=   (i > 2 && date[2]) ? date[2] : 1;
    }
    else
    {
      while ( i < 3 )
        date[i++]= 1;

      rgbValue->year=  date[0];
      rgbValue->month= date[1];
      rgbValue->day=   date[2];
    }
    return 0;
}


/*
  @type    : myodbc internal
  @purpose : convert a time string to a (ulong) value.
  At least following formats are recogniced
  HHMMSS HHMM HH HH.MM.SS  {t HH:MM:SS }
  @return  : HHMMSS
*/

ulong str_to_time_as_long(const char *str, uint length)
{
    uint i,date[3];
    const char *end= str+length;

    if ( length == 0 )
        return 0;

    for ( ; !isdigit(*str) && str != end ; ++str ) --length;

    for ( i= 0 ; i < 3 && str != end; ++i )
    {
        uint tmp_value= (uint) (uchar) (*str++ - '0');
        --length;

        while ( str != end && isdigit(str[0]) )
        {
            tmp_value= tmp_value*10 + (uint) (uchar) (*str - '0');
            ++str;
            --length;
        }
        date[i]= tmp_value;
        while ( str != end && !isdigit(*str) )
        {
            ++str;
            --length;
        }
    }
    if ( length && str != end )
        return str_to_time_as_long(str, length);/* timestamp format */

    if ( date[0] > 10000L || i < 3 )    /* Properly handle HHMMSS format */
        return(ulong) date[0];

    return(ulong) date[0] * 10000L + (ulong) (date[1]*100L+date[2]);
}

/*
  @type    : myodbc internal
  @purpose : compare strings without regarding to case
*/

int myodbc_strcasecmp(const char *s, const char *t)
{
  if (!s && !t)
  {
    return 0;
  }

  if (!s || !t)
  {
    return 1;
  }

  while (toupper((uchar) *s) == toupper((uchar) *t++))
    if (!*s++)
      return 0;
  return((int) toupper((uchar) s[0]) - (int) toupper((uchar) t[-1]));
}


/*
  @type    : myodbc internal
  @purpose : compare strings without regarding to case
*/

int myodbc_casecmp(const char *s, const char *t, uint len)
{
  if (!s && !t)
  {
    return 0;
  }

  if ((!s && t) || (s && !t))
    return 1;

  if (!s || !t)
  {
    return (int)len + 1;
  }

  while (len-- != 0 && toupper(*s++) == toupper(*t++))
    ;
  return (int)len + 1;
}

/**
  Scale an int[] representing SQL_C_NUMERIC

  @param[in] ary   Array in little endian form
  @param[in] s     Scale
*/
void sqlnum_scale(unsigned *ary, int s)
{
  /* multiply out all pieces */
  while (s--)
  {
    ary[0] *= 10;
    ary[1] *= 10;
    ary[2] *= 10;
    ary[3] *= 10;
    ary[4] *= 10;
    ary[5] *= 10;
    ary[6] *= 10;
    ary[7] *= 10;
  }
}


/**
  Unscale an int[] representing SQL_C_NUMERIC. This
  leaves the last element (0) with the value of the
  last digit.

  @param[in] ary   Array in little endian form
*/
static void sqlnum_unscale_le(unsigned *ary)
{
  int i;
  for (i= 7; i > 0; --i)
  {
    ary[i - 1] += (ary[i] % 10) << 16;
    ary[i] /= 10;
  }
}


/**
  Unscale an int[] representing SQL_C_NUMERIC. This
  leaves the last element (7) with the value of the
  last digit.

  @param[in] ary   Array in big endian form
*/
static void sqlnum_unscale_be(unsigned *ary, int start)
{
  int i;
  for (i= start; i < 7; ++i)
  {
    ary[i + 1] += (ary[i] % 10) << 16;
    ary[i] /= 10;
  }
}


/**
  Perform the carry to get all elements below 2^16.
  Should be called right after sqlnum_scale().

  @param[in] ary   Array in little endian form
*/
static void sqlnum_carry(unsigned *ary)
{
  int i;
  /* carry over rest of structure */
  for (i= 0; i < 7; ++i)
  {
    ary[i+1] += ary[i] >> 16;
    ary[i] &= 0xffff;
  }
}


/**
  Retrieve a SQL_NUMERIC_STRUCT from a string. The requested scale
  and precesion are first read from sqlnum, and then updated values
  are written back at the end.

  @param[in] numstr       String representation of number to convert
  @param[in] sqlnum       Destination struct
  @param[in] overflow_ptr Whether or not whole-number overflow occurred.
                          This indicates failure, and the result of sqlnum
                          is undefined.
*/

void sqlnum_from_str(const char *numstr, SQL_NUMERIC_STRUCT *sqlnum,
                     int *overflow_ptr)
{
  /*
     We use 16 bits of each integer to convert the
     current segment of the number leaving extra bits
     to multiply/carry
  */
  unsigned build_up[8], tmp_prec_calc[8];
  /* current segment as integer */
  unsigned int curnum;
  /* current segment digits copied for strtoul() */
  char curdigs[5];
  /* number of digits in current segment */
  int usedig;
  int i;
  int len;
  char *decpt= strchr((char*)numstr, '.');
  int overflow= 0;
  SQLSCHAR reqscale= sqlnum->scale;
  SQLCHAR reqprec= sqlnum->precision;

  memset(&sqlnum->val, 0, sizeof(sqlnum->val));
  memset(build_up, 0, sizeof(build_up));

  /* handle sign */
  if (!(sqlnum->sign= !(*numstr == '-')))
    ++numstr;

  len= (int) strlen(numstr);
  sqlnum->precision= len;
  sqlnum->scale= 0;

  /* process digits in groups of <=4 */
  for (i= 0; i < len; i += usedig)
  {
    if (i + 4 < len)
      usedig= 4;
    else
      usedig= len - i;
    /*
       if we have the decimal point, ignore it by setting it to the
       last char (will be ignored by strtoul)
    */
    if (decpt && decpt >= numstr + i && decpt < numstr + i + usedig)
    {
      usedig = (int) (decpt - (numstr + i) + 1);
      sqlnum->scale= len - (i + usedig);
      --sqlnum->precision;
      decpt= NULL;
    }

    if (overflow)
      goto end;

    /* grab just this piece, and convert to int */
    memcpy(curdigs, numstr + i, usedig);
    curdigs[usedig]= 0;
    curnum= strtoul(curdigs, NULL, 10);
    if (curdigs[usedig - 1] == '.')
      sqlnum_scale(build_up, usedig - 1);
    else
      sqlnum_scale(build_up, usedig);
    /* add the current number */
    build_up[0] += curnum;
    sqlnum_carry(build_up);
    if (build_up[7] & ~0xffff)
      overflow= 1;
  }

  /* scale up to SQL_DESC_SCALE */
  if (reqscale > 0 && reqscale > sqlnum->scale)
  {
    while (reqscale > sqlnum->scale)
    {
      sqlnum_scale(build_up, 1);
      sqlnum_carry(build_up);
      ++sqlnum->scale;
    }
  }
  /* scale back, truncating decimals */
  else if (reqscale < sqlnum->scale)
  {
    while (reqscale < sqlnum->scale && sqlnum->scale > 0)
    {
      sqlnum_unscale_le(build_up);

      // Value 2 of overflow indicates truncation, not critical
      if (build_up[0] % 10)
        overflow = 2;

      build_up[0] /= 10;
      --sqlnum->precision;
      --sqlnum->scale;
    }
  }

  /* scale back whole numbers while there's no significant digits */
  if (reqscale < 0)
  {
    memcpy(tmp_prec_calc, build_up, sizeof(build_up));
    while (reqscale < sqlnum->scale)
    {
      sqlnum_unscale_le(tmp_prec_calc);
      if (tmp_prec_calc[0] % 10)
      {
        overflow= 1;
        goto end;
      }
      sqlnum_unscale_le(build_up);
      tmp_prec_calc[0] /= 10;
      build_up[0] /= 10;
      --sqlnum->precision;
      --sqlnum->scale;
    }
  }

  /* calculate minimum precision */
  memcpy(tmp_prec_calc, build_up, sizeof(build_up));

  {
    SQLCHAR temp_precision = sqlnum->precision;

    do
    {
      sqlnum_unscale_le(tmp_prec_calc);
      i= tmp_prec_calc[0] % 10;
      tmp_prec_calc[0] /= 10;
      if (i == 0)
        --temp_precision;
    } while (i == 0 && temp_precision > 0);

    /* detect precision overflow */
    if (temp_precision > reqprec)
      overflow= 1;
  }

  /* compress results into SQL_NUMERIC_STRUCT.val */
  for (i= 0; i < 8; ++i)
  {
    int elem= 2 * i;
    sqlnum->val[elem]= build_up[i] & 0xff;
    sqlnum->val[elem+1]= (build_up[i] >> 8) & 0xff;
  }

end:
  if (overflow_ptr)
    *overflow_ptr= overflow;
}


/**
  Convert a SQL_NUMERIC_STRUCT to a string. Only val and sign are
  read from the struct. precision and scale will be updated on the
  struct with the final values used in the conversion.

  @param[in] sqlnum       Source struct
  @param[in] numstr       Buffer to convert into string. Note that you
                          MUST use numbegin to read the result string.
                          This should point to the LAST byte available.
                          (We fill in digits backwards.)
  @param[in,out] numbegin String pointer that will be set to the start of
                          the result string.
  @param[in] reqprec      Requested precision
  @param[in] reqscale     Requested scale
  @param[in] truncptr     Pointer to set the truncation type encountered.
                          If SQLNUM_TRUNC_WHOLE, this indicates a failure
                          and the contents of numstr are undefined and
                          numbegin will not be written to.
*/
void sqlnum_to_str(SQL_NUMERIC_STRUCT *sqlnum, SQLCHAR *numstr,
                   SQLCHAR **numbegin, SQLCHAR reqprec, SQLSCHAR reqscale,
                   int *truncptr)
{
  unsigned expanded[8];
  int i, j;
  int max_space= 0;
  int calcprec= 0;
  int trunc= 0; /* truncation indicator */

  *numstr--= 0;

  /*
     it's expected to have enough space
     (~at least min(39, max(prec, scale+2)) + 3)
  */

  /*
     expand the packed sqlnum->val so we have space to divide through
     expansion happens into an array in big-endian form
  */
  for (i= 0; i < 8; ++i)
    expanded[7 - i]= (sqlnum->val[(2 * i) + 1] << 8) | sqlnum->val[2 * i];

  /* max digits = 39 = log_10(2^128)+1 */
  for (j= 0; j < 39; ++j)
  {
    /* skip empty prefix */
    while (!expanded[max_space])
      ++max_space;
    /* if only the last piece has a value, it's the end */
    if (max_space >= 7)
    {
      i= 7;
      if (!expanded[7])
      {
        /* special case for zero, we'll end immediately */
        if (!*(numstr + 1))
        {
          *numstr--= '0';
          calcprec= 1;
        }
        break;
      }
    }
    else
    {
      /* extract the next digit */
      sqlnum_unscale_be(expanded, max_space);
    }
    *numstr--= '0' + (expanded[7] % 10);
    expanded[7] /= 10;
    ++calcprec;
    if (j == reqscale - 1)
      *numstr--= '.';
  }

  sqlnum->scale= reqscale;

  /* add <- dec pt */
  if (calcprec < reqscale)
  {
    while (calcprec < reqscale)
    {
      *numstr--= '0';
      --reqscale;
    }
    *numstr--= '.';
    *numstr--= '0';
  }

  /* handle fractional truncation */
  if (calcprec > reqprec && reqscale > 0)
  {
    SQLCHAR *end= numstr + strlen((char *)numstr) - 1;
    while (calcprec > reqprec && reqscale)
    {
      *end--= 0;
      --calcprec;
      --reqscale;
    }
    if (calcprec > reqprec && reqscale == 0)
    {
      trunc= SQLNUM_TRUNC_WHOLE;
      goto end;
    }
    if (*end == '.')
    {
      *end--= '\0';
    }
    else
    {
      /* move the dec pt-- ??? */
      /*
      char c2, c= numstr[calcprec - reqscale];
      numstr[calcprec - reqscale]= '.';
      while (reqscale)
      {
        c2= numstr[calcprec + 1 - reqscale];
        numstr[calcprec + 1 - reqscale]= c;
        c= c2;
        --reqscale;
      }
      */
    }
    trunc= SQLNUM_TRUNC_FRAC;
  }

  /* add zeros for negative scale */
  if (reqscale < 0)
  {
    int i;
    reqscale *= -1;
    for (i= 1; i <= calcprec; ++i)
      *(numstr + i - reqscale)= *(numstr + i);
    numstr -= reqscale;
    memset(numstr + calcprec + 1, '0', reqscale);
  }

  sqlnum->precision= calcprec;

  /* finish up, handle auxilary fix-ups */
  if (!sqlnum->sign)
  {
    *numstr--= '-';
  }
  ++numstr;
  *numbegin= numstr;

end:
  if (truncptr)
    *truncptr= trunc;
}


/**
  Adjust a pointer based on bind offset and bind type.

  @param[in] ptr The base pointer
  @param[in] bind_offset_ptr The bind offset ptr (can be NULL).
             (SQL_ATTR_PARAM_BIND_OFFSET_PTR, SQL_ATTR_ROW_BIND_OFFSET_PTR,
              SQL_DESC_BIND_OFFSET_PTR)
  @param[in] bind_type The bind type. Should be SQL_BIND_BY_COLUMN (0) or
             the length of a row for row-wise binding. (SQL_ATTR_PARAM_BIND_TYPE,
             SQL_ATTR_ROW_BIND_TYPE, SQL_DESC_BIND_TYPE)
  @param[in] default_size The column size if bind type = SQL_BIND_BY_COLUMN.
  @param[in] row The row number.

  @return The base pointer with the offset added. If the base pointer is
          NULL, NULL is returned.
 */
void *ptr_offset_adjust(void *ptr, SQLULEN *bind_offset_ptr,
                        SQLINTEGER bind_type, SQLINTEGER default_size,
                        SQLULEN row)
{
  size_t offset= 0;
  if (bind_offset_ptr)
    offset= (size_t) *bind_offset_ptr;

  if (bind_type == SQL_BIND_BY_COLUMN)
    offset+= default_size * row;
  else
    offset+= bind_type * row;

  return ptr ? ((SQLCHAR *) ptr) + offset : NULL;
}

/* DESODBC:
    Original author: MyODBC
    Modified by: DESODBC Developer
*/
SQLTypeMap SQL_TYPE_MAP_values[]=
{
  /* SQL_CHAR= 1 */
    {(SQLCHAR *)"varchar", 7, SQL_LONGVARCHAR, DES_TYPE_VARCHAR, 0, 0},
    {(SQLCHAR *)"string", 6, SQL_LONGVARCHAR, DES_TYPE_STRING, 0, 0},
    {(SQLCHAR *)"char", 4, SQL_CHAR, DES_TYPE_CHAR_N, 0, 0},
    {(SQLCHAR *)"varchar", 7, SQL_CHAR, DES_TYPE_VARCHAR_N, 0, 0},
    {(SQLCHAR *)"char", 4, SQL_CHAR, DES_TYPE_CHAR, 1, 0},
    {(SQLCHAR *)"integer_des", 7, SQL_BIGINT, DES_TYPE_INTEGER, 19, 1},
    {(SQLCHAR *)"smallint", 7, SQL_SMALLINT, DES_TYPE_SHORT, 5, 1},
    {(SQLCHAR *)"integer", 7, SQL_INTEGER, DES_TYPE_LONG, 8, 1},
    {(SQLCHAR *)"int", 3, SQL_BIGINT, DES_TYPE_INT, 19, 1},
    {(SQLCHAR *)"float", 5, SQL_DOUBLE, DES_TYPE_FLOAT, 53, 1},
    {(SQLCHAR *)"real", 4, SQL_DOUBLE, DES_TYPE_REAL, 53, 1},
    {(SQLCHAR *)"date", 4, SQL_TYPE_DATE, DES_TYPE_DATE, 10, 1},
    {(SQLCHAR *)"time", 4, SQL_TYPE_TIME, DES_TYPE_TIME, 8, 1},
    {(SQLCHAR *)"datetime", 8, SQL_DATETIME, DES_TYPE_DATETIME, 19, 1},
    {(SQLCHAR *)"timestamp", 9, SQL_TYPE_TIMESTAMP, DES_TYPE_TIMESTAMP, 19, 1}
};



/**
   Gets fractional time of a second from datetime or time string.

   @param[in]  value                (date)time string
   @param[in]  len                  length of value buffer
   @param[in]  dont_use_set_locale  use dot as decimal part separator
   @param[out] fraction             buffer where to put fractional part
                                    in nanoseconds

   Returns pointer to decimal point in the string
*/
const char *
get_fractional_part(const char * str, int len, BOOL dont_use_set_locale,
                    SQLUINTEGER * fraction)
{
  const char *decptr= NULL, *end;
  size_t decpoint_len= 1;

  if (len < 0)
  {
    len = (int)strlen(str);
  }

  end= str + len;

  if (dont_use_set_locale)
  {
    decptr= strchr(str, '.');
  }
  else
  {
    decpoint_len = decimal_point.length();
    const char *locale_decimal_point = decimal_point.c_str();
    while (*str && str < end)
    {
      if (str[0] == locale_decimal_point[0] &&
          desodbc::is_prefix(str, locale_decimal_point))
      {
        decptr= str;
        break;
      }

      ++str;
    }
  }

  /* If decimal point is the last character - we don't have fractional part */
  if (decptr && decptr < end - decpoint_len)
  {
    char buff[10], *ptr;

    desodbc::strfill(buff, sizeof(buff)-1, '0');
    str= decptr + decpoint_len;

    for (ptr= buff; str < end && ptr < buff + sizeof(buff); ++ptr)
    {
      /* there actually should not be anything that is not a digit... */
      if (isdigit(*str))
      {
        *ptr= *str++;
      }
    }

    buff[9]= 0;
    *fraction= atoi(buff);
  }
  else
  {
    *fraction= 0;
    decptr= NULL;
  }

  return decptr;
}

/* Convert MySQL timestamp to full ANSI timestamp format. */
char * complete_timestamp(const char * value, ulong length, char buff[21])
{
  char *pos;
  uint i;

  if (length == 6 || length == 10 || length == 12)
  {
    /* For two-digit year, < 60 is considered after Y2K */
    if (value[0] <= '6')
    {
      buff[0]= '2';
      buff[1]= '0';
    }
    else
    {
      buff[0]= '1';
      buff[1]= '9';
    }
  }
  else
  {
    buff[0]= value[0];
    buff[1]= value[1];
    value+= 2;
    length-= 2;
  }

  buff[2]= *value++;
  buff[3]= *value++;
  buff[4]= '-';

  if (value[0] == '0' && value[1] == '0')
  {
    /* Month was 0, which ODBC can't handle. */
    return NULL;
  }

  pos= buff+5;
  length&= 30;  /* Ensure that length is ok */

  for (i= 1, length-= 2; (int)length > 0; length-= 2, ++i)
  {
    *pos++= *value++;
    *pos++= *value++;
    *pos++= i < 2 ? '-' : (i == 2) ? ' ' : ':';
  }
  for ( ; pos != buff + 20; ++i)
  {
    *pos++= '0';
    *pos++= '0';
    *pos++= i < 2 ? '-' : (i == 2) ? ' ' : ':';
  }

  return buff;
}


tempBuf::tempBuf(size_t size) : buf(nullptr), buf_len(0), cur_pos(0)
{
  if (size)
    extend_buffer(size);
}

tempBuf::tempBuf(const tempBuf& b)
{
  *this = b;
}

tempBuf::tempBuf(const char* src, size_t len) {
  add_to_buffer(src, len);
}

char *tempBuf::extend_buffer(size_t len)
{
  if (cur_pos > buf_len)
    throw "Position is outside of buffer";

  if (len > buf_len - cur_pos)
  {
    buf = (char*)realloc(buf, buf_len + len);

    // TODO: smarter processing for Out-of-Memory
    if (buf == NULL)
      throw "Not enough memory for buffering";
    buf_len += len;
  }

  return buf + cur_pos; // Return position in the new buffer
}


char *tempBuf::extend_buffer(char *to, size_t len)
{
  cur_pos = to - buf;

  return extend_buffer(len);
}

char *tempBuf::add_to_buffer(const char *from, size_t len)
{
  if (cur_pos > buf_len)
    throw "Position is outside of buffer";

  size_t extend_by = buf_len - cur_pos >= len ? 0 :
    buf_len - cur_pos + len;

  extend_buffer(extend_by);
  memcpy(buf + cur_pos, from, len);
  cur_pos += len;
  return buf + cur_pos;
}

char *tempBuf::add_to_buffer(char *to, const char *from, size_t len)
{
  cur_pos = to - buf;
  if (cur_pos > buf_len)
    throw "Position is outside of buffer";

  return add_to_buffer(from, len);
}


void tempBuf::remove_trail_zeroes()
{
  while (cur_pos && buf[cur_pos-1] == '\0') --cur_pos;
}

void tempBuf::reset()
{
  cur_pos = 0;
}

tempBuf::~tempBuf()
{
  if (buf_len && buf)
  {
    free(buf);
  }
}

tempBuf::operator bool()
{
  return buf != nullptr;
}

void tempBuf::operator=(const tempBuf& b)
{
  buf_len = 0;
  cur_pos = 0;
  buf = nullptr;
  if (b.buf_len)
  {
    extend_buffer(b.buf_len);
    memcpy(buf, b.buf, b.cur_pos);
  }
  // setting to the same position as the original
  cur_pos = b.cur_pos;
}

/*
  Get the offset and row numbers from a string with LIMIT

  @param[in] cs        charset
  @param[in] query     query
  @param[in] query_end end of query
  @param[out] offs_out output buffer for offset
  @param[out] rows_out output buffer for rows

  @return the position where LIMIT OFFS, ROWS is ending
*/
const char* get_limit_numbers(desodbc::CHARSET_INFO* cs, const char *query, const char * query_end,
                       unsigned long long *offs_out, unsigned int *rows_out)
{
  char digit_buf[30];
  int index_pos = 0;

  // Skip spaces after LIMIT
  while ((query_end > query) && myodbc_isspace(cs, query, query_end))
    ++query;

  // Collect all numbers for the offset
  while ((query_end > query) && myodbc_isnum(cs, query, query_end))
  {
    digit_buf[index_pos] = *query;
    ++index_pos;
    ++query;
  }

  if (!index_pos)
  {
    // Something went wrong, the limit numbers are not found
    return query;
  }

  digit_buf[index_pos] = '\0';
  *offs_out = (unsigned long long)atoll(digit_buf);

  // Find the row number for "LIMIT offset, row_number"
  while ((query_end > query) && !myodbc_isnum(cs, query, query_end))
    ++query;

  if (query == query_end)
  {
    // It was "LIMIT row_number" without offset
    // What we thought was offset is in fact the number of rows
    *rows_out = (unsigned long)*offs_out;
    *offs_out = 0;
    return query;
  }

  index_pos = 0; // reset index to use with another number

  // Collect all numbers for the row number
  while ((query_end > query) && myodbc_isnum(cs, query, query_end))
  {
    digit_buf[index_pos] = *query;
    ++index_pos;
    ++query;
  }

  digit_buf[index_pos] = '\0';
  *rows_out = (unsigned long)atol(digit_buf);
  return query;
}

BOOL myodbc_isspace(desodbc::CHARSET_INFO* cs, const char * begin, const char *end)
{
  int ctype;
  cs->cset->ctype(cs, &ctype, (const uchar*) begin, (const uchar*) end);

  return ctype & _MY_SPC;
}

BOOL myodbc_isnum(desodbc::CHARSET_INFO* cs, const char * begin, const char *end)
{
  int ctype;
  cs->cset->ctype(cs, &ctype, (const uchar*)begin, (const uchar*)end);

  return ctype & _MY_NMR;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
std::vector<std::string> getLines(const std::string &str) {
  std::vector<std::string> lines;
  std::stringstream ss(str);
  std::string line;

  while (std::getline(ss, line, '\n')) {
#ifdef _WIN32
    lines.push_back(line.substr(
        0, line.size() - 1));  // to remove the char '/r' of each line
#else
    // For the UNIX version, we need to remove the characters CR and
    // non-printing space
    line.erase(std::remove(line.begin(), line.end(), 13), line.end());  // CR
    line.erase(std::remove(line.begin(), line.end(), 32),
               line.end());  // non-printing space
    lines.push_back(line);   // there seems that '/r' char isn't placed in the
                             // DES unix version
#endif
  }
  return lines;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
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

/* DESODBC:
    Original author: DESODBC Developer
*/
char *string_to_char_pointer(const std::string &str) {
  char *ptr = new char[str.size() + 1];
  if (!ptr) throw std::bad_alloc();
  std::strcpy(ptr, str.c_str());
  return ptr;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
wchar_t *string_to_wchar_pointer(const std::wstring &w_str) {
  wchar_t *ptr = new wchar_t[w_str.size() + 1];
  if (!ptr) throw std::bad_alloc();

  w_str.copy(ptr, w_str.size());
  ptr[w_str.size()] = L'\0';

  return ptr;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
TypeAndLength get_Type_from_str(const std::string &str) {
  std::string type_str = str;
  SQLULEN size = -1;

  std::transform(type_str.begin(), type_str.end(), type_str.begin(),
               [](unsigned char c) { return std::tolower(c); });

  //Special cases in external databases.
  if (is_in_string(type_str, "integer(") ||
      is_in_string(type_str, "varbinary("))
    return {DES_TYPE_INTEGER, get_type_size(DES_TYPE_INTEGER)};

  size_t first_parenthesis_pos = type_str.find('(', 0);
  size_t pos = first_parenthesis_pos;
  if (pos != std::string::npos && !is_in_string(type_str, "datetime")) {
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

  enum_field_types simple_type = typestr_simpletype_map.at(type_str);

  return {simple_type, size};
};

/* DESODBC:
    Original author: DESODBC Developer
*/
SQLULEN get_type_size(enum_field_types type) {
  switch (type) {
    case DES_TYPE_INTEGER:
    case DES_TYPE_INT:
      return 19;
    case DES_TYPE_FLOAT:
    case DES_TYPE_REAL:
      return 59;
    case DES_TYPE_DATE:
      return 10;
    case DES_TYPE_TIME:
      return 8;
    case DES_TYPE_DATETIME:
    case DES_TYPE_TIMESTAMP:
      return 19;
    // "Variable-length string of up to the maximum length of the underlying Prolog atom."
    // which is infinite. Therefore, I will pick the standard 255 (inspired by PostgreSQL)
    case DES_TYPE_VARCHAR:
    case DES_TYPE_STRING:
      return DES_MAX_STRLEN;
    default:
      return 0;
  }
}

/* DESODBC:
    Original author: DESODBC Developer
*/
std::string Type_to_type_str(TypeAndLength type) {
  for (auto pair : typestr_simpletype_map) {
    std::string type_str = pair.first;
    enum_field_types simple_type = pair.second;

    type_str.erase(std::remove(type_str.begin(), type_str.end(), '('),
                   type_str.end());
    type_str.erase(std::remove(type_str.begin(), type_str.end(), ')'),
                   type_str.end());

    if (type.simple_type == simple_type) {
      if (is_character_des_data_type(simple_type) &&
          !is_time_des_data_type(simple_type)) {
        if (type.len != -1) {
          return type_str + "(" + std::to_string(type.len) +
                 ")";
        } else {
          return type_str;
        }
      } else
        return type_str;
    }
  }

  return "";
}

/* DESODBC:
    Original author: DESODBC Developer
*/
SQLULEN get_Type_size(TypeAndLength type) {
  SQLULEN small_type_size = get_type_size(type.simple_type);
  if (type.len != -1) {
    return small_type_size * SYSTEM_CHARSET_MBMAXLEN;
  } else
    return small_type_size;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
TypeAndLength get_Type(enum_field_types type) {
  return {type, get_type_size(type)};
}

/* DESODBC:
    Original author: DESODBC Developer
*/
bool is_in_string(const std::string &str, const std::string &search) {
  return str.find(search) != std::string::npos;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
char *sqlcharptr_to_charptr(SQLCHAR* sql_str, SQLUSMALLINT sql_str_len) {
  char *charptr = new char[sql_str_len+1];
  memcpy(charptr, sql_str, sql_str_len);
  charptr[sql_str_len] = '\0';
  return charptr;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
SQLCHAR *str_to_sqlcharptr(const std::string &str) {
  SQLCHAR *ptr = new SQLCHAR[str.size() + 1];
  memcpy(ptr, str.c_str(), str.size());
  ptr[str.size()] = '\0';
  return ptr;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
std::string sqlcharptr_to_str(SQLCHAR *sql_str, SQLUSMALLINT sql_str_len) {
  char *ptr = sqlcharptr_to_charptr(sql_str, sql_str_len);
  std::string str(ptr);
  delete ptr;
  return str;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
std::string des_type_2_str(enum_field_types des_type) {
    for (SQLTypeMap value : SQL_TYPE_MAP_values) {
        if (value.des_type == des_type) {
            char *ptr = sqlcharptr_to_charptr(value.type_name, value.name_length);
            std::string name_str = std::string(ptr);
            delete ptr;
            return name_str;
        }
    }
    return "";
}

/* DESODBC:
    Original author: DESODBC Developer
*/
int des_type_2_sql_type(enum_field_types des_type) {
  switch (des_type) {
    case DES_TYPE_VARCHAR:
      return SQL_LONGVARCHAR;
    case DES_TYPE_STRING:
      return SQL_LONGVARCHAR;
    case DES_TYPE_CHAR_N:
      return SQL_CHAR;
    case DES_TYPE_VARCHAR_N:
      return SQL_CHAR;
    case DES_TYPE_CHAR:
      return SQL_CHAR;
    case DES_TYPE_INTEGER:
    case DES_TYPE_INT:
      return SQL_BIGINT;
    case DES_TYPE_FLOAT:
      return SQL_DOUBLE;
    case DES_TYPE_REAL:
      return SQL_DOUBLE;
    case DES_TYPE_DATE:
      return SQL_TYPE_DATE;
    case DES_TYPE_TIME:
      return SQL_TYPE_TIME;
    case DES_TYPE_DATETIME:
      return SQL_DATETIME;
    case DES_TYPE_TIMESTAMP:
      return SQL_TYPE_TIMESTAMP;
    case DES_TYPE_SHORT:
      return SQL_SMALLINT;
    case DES_TYPE_LONG:
      return SQL_INTEGER;
    default:
      return SQL_UNKNOWN_TYPE;
  }
}

/* DESODBC:
    Original author: DESODBC Developer
*/
bool is_decimal_des_data_type(enum_field_types type) {
  switch (type) {
    case DES_TYPE_FLOAT:
    case DES_TYPE_REAL:
      return true;
    default:
      return false;
  }
}

/* DESODBC:
    Original author: DESODBC Developer
*/
bool is_numeric_des_data_type(enum_field_types type) {
  switch (type) {
    case DES_TYPE_INT:
    case DES_TYPE_INTEGER:
    case DES_TYPE_FLOAT:
    case DES_TYPE_REAL:
    case DES_TYPE_SHORT:
    case DES_TYPE_LONG:
      return true;
    default:
      return false;
  }
}

/* DESODBC:
    Original author: DESODBC Developer
*/
bool is_character_des_data_type(enum_field_types type) {
  switch (type) {
    case DES_TYPE_CHAR:
    case DES_TYPE_CHAR_N:
    case DES_TYPE_VARCHAR_N:
    case DES_TYPE_VARCHAR:
    case DES_TYPE_STRING:
    case DES_TYPE_DATE:
    case DES_TYPE_TIME:
    case DES_TYPE_DATETIME:
    case DES_TYPE_TIMESTAMP:
      return true;
    default:
      return false;
  }
}

/* DESODBC:
    Original author: DESODBC Developer
*/
bool is_time_des_data_type(enum_field_types type) {
  switch (type) {
    case DES_TYPE_DATE:
    case DES_TYPE_TIME:
    case DES_TYPE_DATETIME:
    case DES_TYPE_TIMESTAMP:
      return true;
    default:
      return false;
  }
}

/* DESODBC:
    Original author: DESODBC Developer
*/
bool is_character_sql_data_type(SQLSMALLINT sql_type) {
  switch (sql_type) {
    case SQL_NUMERIC:
    case SQL_DECIMAL:
    case SQL_INTEGER:
    case SQL_SMALLINT:
    case SQL_TINYINT:
    case SQL_BIGINT:
    case SQL_FLOAT:
    case SQL_REAL:
    case SQL_DOUBLE:
      return false;
    default:
      return true;
  }
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
                 nullptr, errorCode,
                 0,
                 (LPSTR)&errorMsg, 0, nullptr);

  if (errorMsg)
    return std::string(errorMsg);
  else
    return "";
}
#endif

#ifdef _WIN32
/* DESODBC:
    Original author: DESODBC Developer
*/
void try_close(HANDLE h) {
  if (h)
      CloseHandle(h);
}
#else
/* DESODBC:
    Original author: DESODBC Developer
*/
void try_close(int fd) {
  ::close(fd);
}
#endif

/* DESODBC:
    Original author: DESODBC Developer
*/
bool is_bulkable_statement(const std::string& query) {
  bool bulkable = false;
  std::string query_cp = query;
  std::transform(query_cp.begin(), query_cp.end(), query_cp.begin(), [](unsigned char c){ return std::tolower(c); });
  query_cp.erase(std::remove(query_cp.begin(), query_cp.end(), 13), query_cp.end());  // CR
  query_cp.erase(std::remove(query_cp.begin(), query_cp.end(), 32), query_cp.end());  // non-printing space
  query_cp.erase(std::remove(query_cp.begin(), query_cp.end(), ' '), query_cp.end());  // printing space

  std::string keyword = query_cp.substr(0, 6); //position 0, 6 characters (all the words we check have six characters)
  if (is_in_string(query_cp, "/sql")) {
    std::string keyword = query_cp.substr(4, 6);
  }

  //bulkable |= (keyword == "select");
  bulkable |= (keyword == "insert");
  bulkable |= (keyword == "update");
  bulkable |= (keyword == "delete");
  bulkable |= (keyword == "create");
  bulkable |= (keyword == "/dbsch");
  bulkable |= (keyword == "/show_");
  bulkable |= (keyword == "/curre");
  //bulkable |= (keyword == "/use_d"); It is dangerous to not put delay in /use_db. Sometimes it has some computation time.

  return bulkable;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
std::string odbc_pattern_to_regex_pattern(const std::string &odbc_pattern) {
  std::string regex_pattern = "";

  //We need to know the special characters so we can escape them.
  const std::string special_characters =
      "^$.*+?()[]{}|";  //The default grammar for std::regex is https://en.cppreference.com/w/cpp/regex/ecmascript
                          //The atom PatternCharacter shows that the special characters are these. '\' has been omitted
                          //because the preconditions establish that '_' and '%' is escaped when treating it as a literal.
  int i = 0;
  while (i < odbc_pattern.size()) {
    bool is_special_character =
        special_characters.find(odbc_pattern[i]) != std::string::npos;

    if (is_special_character) regex_pattern += "\\";
    
    if (odbc_pattern[i] == '%')
      regex_pattern += ".*";
    else if (odbc_pattern[i] == '_')
      regex_pattern += ".";
    else if (odbc_pattern[i] == '\\' && i + 1 < odbc_pattern.size()) {
      regex_pattern += odbc_pattern[i];
      regex_pattern += odbc_pattern[i+1];
      i += 1; //the other i +=1 will be done at the end of this iteration
    } else
      regex_pattern += odbc_pattern[i];

    i += 1;
  }
  return regex_pattern;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
std::vector<std::string> search_odbc_pattern(const std::string &pattern,
                                        const std::vector<std::string> &v_str) {

  std::string regex_pattern = odbc_pattern_to_regex_pattern(pattern);
  std::regex rx(regex_pattern);

  std::vector<std::string> coincidences;
  for (int i = 0; i < v_str.size(); ++i) {
    if (std::regex_match(v_str[i], rx)) {
      coincidences.push_back(v_str[i]);
    }
  }

  return coincidences;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
std::vector<std::string> get_attrs(const std::string &str) {
  std::vector<std::string> attrs;
  size_t start = 0, end = 0;
  int i = 0;
  while (i < str.size()) {
    if (str[i] == '=') {
      std::string attr = "";
      i++;
      while (i < str.size() && str[i] != ' ') {
        attr += str[i];
        i++;
      }
      attrs.push_back(attr);
    } else
      i++;
  }
  return attrs;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
std::string convert2identifier(const std::string &arg) {

  const size_t first = arg.find_first_not_of(" \t\r");
  const size_t last = arg.find_last_not_of(" \t\r");

  std::string cleaned_str = arg;

  //All the following characters are valid delimiters in DES.
  if (cleaned_str.find_first_of("\'\"`") ==
      std::string::npos) {  // i.e., it is not quoted
    cleaned_str = cleaned_str.substr(0, last + 1);
    std::transform(cleaned_str.begin(), cleaned_str.end(), cleaned_str.begin(),
                   [](unsigned char c) { return std::toupper(c); });
  } else {
    cleaned_str = arg.substr(first + 1, last - first - 1);
  }

  return cleaned_str;

}

/* DESODBC:
    Original author: DESODBC Developer
*/
std::string get_prepared_arg(STMT *stmt, SQLCHAR *name, SQLSMALLINT len) {
  std::string str = sqlcharptr_to_str(name, len);
  if (stmt->stmt_options.metadata_id)
    return convert2identifier(str);
  else
    return str;
}

/* DESODBC:
    Original author: DESODBC Developer
*/
std::string get_catalog(STMT *stmt, SQLCHAR *name, SQLSMALLINT len) {
  if (!name || !(*name) || len == 0)
    return "$des";
  else
    return get_prepared_arg(stmt, name, len);
}

/* DESODBC:
    This function sets possible errors
    given the TAPI output.

    Original author: DESODBC Developer
*/
SQLRETURN set_error_from_tapi_output(SQLSMALLINT HandleType, SQLHANDLE Handle,
                                     const std::string &tapi_output) {

  /* Note: there are multiple ways to parse the info messages depending on the
  command.
  * There is not a standard syntax for every command: in 5.18.1.2 (DES v6.7
  manual), it says that the answers can be defined specifically for some
  commands. For instance, it can be checked an error of /process should be
  parsed differently than an "$error" notified by /ls. We then throw the full
  TAPI msg. It is not a bad idea: after all, the application using this
  connector may find it useful to work on the TAPI notation, since the
  application itself can be a computer program that uses this TAPI to its
  advantage.
  */

  std::vector<std::string> lines = getLines(tapi_output);

  /*
    We already know there is a $error. But only when we find
    a $error followed by a 0, we can know there is a real error.
    If not, all the $error keywords are followed by 1 or 2 (i.e.,
    info message). Check DES manual 5.18.1.2
  */

  bool real_error = false;
  int i = 0;
  while (i < lines.size()) {
    if (i + 1 < lines.size()) {
      if (lines[i] == "$error" && lines[i + 1] == "0") {
        real_error = true;
        break;
      }
      
    }
    i += 1;
  }

  std::string msg_str = "Full TAPI output: ";
  msg_str += tapi_output;
  const char *msg = string_to_char_pointer(msg_str);

  if (real_error) {
    if (HandleType == SQL_HANDLE_STMT)
      ((STMT *)Handle)->set_error("HY000", msg);
    else if (HandleType == SQL_HANDLE_DBC)
      ((DBC *)Handle)->set_error("HY000", msg);
  } else {
    if (HandleType == SQL_HANDLE_STMT)
      ((STMT *)Handle)->set_error("01000", msg);
    else if (HandleType == SQL_HANDLE_DBC)
      ((DBC *)Handle)->set_error("01000", msg);
  }

  delete msg;

  if (real_error)
      return SQL_ERROR;
  else
    return SQL_SUCCESS_WITH_INFO;
}

/* DESODBC:
    This function checks the TAPI output and sets
    errors if so.

    Original author: DESODBC Developer
*/
SQLRETURN check_and_set_errors(SQLSMALLINT HandleType, SQLHANDLE Handle,
                               const std::string &tapi_output) {
  if (is_in_string(tapi_output, "$error")) {
    return set_error_from_tapi_output(HandleType, Handle, tapi_output);
  } else if (is_in_string(tapi_output, "$success")) {
    return SQL_SUCCESS;
  } else {
    if (tapi_output.size() == 0) return SQL_SUCCESS;

    std::string msg_str = "Full TAPI output: ";
    msg_str += tapi_output;
    const char *msg = string_to_char_pointer(msg_str);
    if (HandleType == SQL_HANDLE_STMT)
      ((STMT *)Handle)->set_error("01000", msg);
    else if (HandleType == SQL_HANDLE_DBC)
      ((DBC *)Handle)->set_error("01000", msg);
    delete msg;
    return SQL_SUCCESS_WITH_INFO;
  }
}

/* DESODBC:
    Original author: DESODBC Developer
*/
std::vector<std::string> filter_candidates(std::vector<std::string>& candidates, const std::string &key, bool metadata_id) {
  std::vector<std::string> res;
  if (key.size() == 0)
    res = candidates;
  else {
    if (!metadata_id) {
      res = search_odbc_pattern(key, candidates);
    } else {
      for (int i = 0; i < candidates.size(); ++i) {
        if (candidates[i] == key) {
          res = {key};
          break;
        }
      }
    }
  }
  return res;
}