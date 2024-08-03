/* Copyright (c) 2014, 2021, Oracle and/or its affiliates.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.

   Without limiting anything contained in the foregoing, this file,
   which is part of C Driver for MySQL (Connector/C), is also subject to the
   Universal FOSS Exception, version 1.0, a copy of which can be found at
   http://oss.oracle.com/licenses/universal-foss-exception.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

/**
  @file field_types.h

  @brief This file contains the field type.


  @note This file can be imported both from C and C++ code, so the
  definitions have to be constructed to support this.
*/

#ifndef FIELD_TYPES_INCLUDED
#define FIELD_TYPES_INCLUDED

/*
 * Constants exported from this package.
 */

/**
  Column types for MySQL
*/
enum enum_field_types
#if defined(__cplusplus) && __cplusplus > 201103L
    // N2764: Forward enum declarations, added in C++11
    : int
#endif /* __cplusplus */
{ DES_TYPE_DECIMAL,
  DES_TYPE_TINY,
  DES_TYPE_SHORT,
  DES_TYPE_LONG,
  DES_TYPE_FLOAT,
  DES_TYPE_DOUBLE,
  DES_TYPE_NULL,
  DES_TYPE_TIMESTAMP,
  DES_TYPE_LONGLONG,
  DES_TYPE_INT24,
  DES_TYPE_DATE,
  DES_TYPE_TIME,
  DES_TYPE_DATETIME,
  DES_TYPE_YEAR,
  DES_TYPE_NEWDATE, /**< Internal to MySQL. Not used in protocol */
  DES_TYPE_VARCHAR,
  DES_TYPE_BIT,
  DES_TYPE_TIMESTAMP2,
  DES_TYPE_DATETIME2,   /**< Internal to MySQL. Not used in protocol */
  DES_TYPE_TIME2,       /**< Internal to MySQL. Not used in protocol */
  DES_TYPE_TYPED_ARRAY, /**< Used for replication only */
  DES_TYPE_INVALID = 243,
  DES_TYPE_BOOL = 244, /**< Currently just a placeholder */
  DES_TYPE_JSON = 245,
  DES_TYPE_NEWDECIMAL = 246,
  DES_TYPE_ENUM = 247,
  DES_TYPE_SET = 248,
  DES_TYPE_TINY_BLOB = 249,
  DES_TYPE_MEDIUM_BLOB = 250,
  DES_TYPE_LONG_BLOB = 251,
  DES_TYPE_BLOB = 252,
  DES_TYPE_VAR_STRING = 253,
  DES_TYPE_STRING = 254,
  DES_TYPE_GEOMETRY = 255 };

#endif /* FIELD_TYPES_INCLUDED */
