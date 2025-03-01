// Copyright (c) 2005, 2024, Oracle and/or its affiliates.
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

/* @EDIT_WARNING_MESSAGE@ */
#define SETUP_VERSION         "@CONNECTOR_MAJOR@.@CONNECTOR_MINOR_PADDED@.@CONNECTOR_PATCH_PADDED@"
#define DRIVER_VERSION        "0" SETUP_VERSION

#define DESODBC_VERSION        SETUP_VERSION
#define DESODBC_MAJOR_VERSION  @CONNECTOR_MAJOR@
#define DESODBC_STRMAJOR_VERSION  "@CONNECTOR_MAJOR@"
#define DESODBC_FILEVER        @CONNECTOR_MAJOR@,@CONNECTOR_MINOR@,@CONNECTOR_PATCH@,0
#define DESODBC_PRODUCTVER     DESODBC_FILEVER
#define DESODBC_STRFILEVER     "@CONNECTOR_MAJOR@, @CONNECTOR_MINOR@, @CONNECTOR_PATCH@, 0\0"
#define DESODBC_STRPRODUCTVER  DESODBC_STRFILEVER
#define DESODBC_CONN_ATTR_VER  "@CONNECTOR_MAJOR@.@CONNECTOR_MINOR@.@CONNECTOR_PATCH@"
#define DESODBC_LICENSE        "@CONNECTOR_LICENSE@"

#define DESODBC_STRSERIES      "@CONNECTOR_MAJOR@.@CONNECTOR_MINOR@"
#define DESODBC_STRQUALITY     "@CONNECTOR_QUALITY@"

#define DESODBC_VERSION_PREV  "@CONNECTOR_MAJOR_PREV@.@CONNECTOR_MINOR_PREV@.@CONNECTOR_PATCH_PREV@"

#ifdef DESODBC_UNICODEDRIVER
# define DESODBC_STRDRIVERID    DESODBC_STRSERIES"(w)"
# define DESODBC_STRDRIVERTYPE  "Unicode"
# define DESODBC_STRTYPE_SUFFIX "w"
#else
# define DESODBC_STRDRIVERID    DESODBC_STRSERIES"(a)"
# define DESODBC_STRDRIVERTYPE  "ANSI"
# define DESODBC_STRTYPE_SUFFIX "a"
#endif
