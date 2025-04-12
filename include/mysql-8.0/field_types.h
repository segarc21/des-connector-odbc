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
  Column types for DES
*/
enum enum_field_types
#if defined(__cplusplus) && __cplusplus > 201103L
    // N2764: Forward enum declarations, added in C++11
    : int
#endif /* __cplusplus */
{ DES_TYPE_VARCHAR,
  DES_TYPE_STRING,
  DES_TYPE_CHAR_N,
  DES_TYPE_VARCHAR_N,
  DES_TYPE_CHAR,
  DES_TYPE_INTEGER,
  DES_TYPE_INT,
  DES_TYPE_FLOAT,
  DES_TYPE_REAL,
  DES_TYPE_DATE,
  DES_TYPE_TIME,
  DES_TYPE_DATETIME,
  DES_TYPE_TIMESTAMP,
  DES_TYPE_BLOB,   // needed somewhere, but I need to know why. TODO
  DES_TYPE_TINY,  // needed somewhere, but I need to know why. TODO
  DES_UNKNOWN_TYPE
};

#include "sql.h"
#include "sqltypes.h"
inline const SQLULEN DES_DEFAULT_DATA_CHARACTER_SIZE =
    255;  // This seems to be a standard, along with
          // 8000. TODO: research

//Handy structure
struct TypeAndLength {
  enum_field_types simple_type;
  SQLULEN len = -1;  // length as in number of characters; do not confuse with
                     // length as the DES_FIELD field (width of column)
};

#endif /* FIELD_TYPES_INCLUDED */
