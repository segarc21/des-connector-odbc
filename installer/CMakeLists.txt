# Copyright (c) 2007, 2024, Oracle and/or its affiliates.
# Modified in 2025 by Sergio Miguel Garcia Jimenez <segarc21@ucm.es>
# (see the next block comment below).
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0, as
# published by the Free Software Foundation.
#
# This program is designed to work with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms, as
# designated in a particular file or component or in included license
# documentation. The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have either included with
# the program or referenced in the documentation.
#
# Without limiting anything contained in the foregoing, this file,
# which is part of Connector/ODBC, is also subject to the
# Universal FOSS Exception, version 1.0, a copy of which can be found at
# https://oss.oracle.com/licenses/universal-foss-exception.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

# ---------------------------------------------------------
# Modified in 2025 by Sergio Miguel Garcia Jimenez <segarc21@ucm.es>,
# hereinafter the DESODBC developer, in the context of the GPLv2 derivate
# work DESODBC, an ODBC Driver of the open-source DBMS Datalog Educational
# System (DES) (see https://des.sourceforge.io/)
# ---------------------------------------------------------

##########################################################################



INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/util)

IF(DL_INCLUDES)
  INCLUDE_DIRECTORIES(${DL_INCLUDES})
ENDIF(DL_INCLUDES)

ADD_EXECUTABLE(desodbc-installer desodbc-installer.cc)

add_version_info(desodbc-installer
  "DES Connector/ODBC installer tool."
  "Provides installer tool functionality."
)

IF(WIN32)
  IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
    LINK_DIRECTORIES(${MYSQL_DIR}/lib/debug)
    LINK_DIRECTORIES(${MYSQL_DIR}/lib)
  ELSEIF(CMAKE_BUILD_TYPE STREQUAL "")
    LINK_DIRECTORIES(${MYSQL_DIR}/lib/opt)
    LINK_DIRECTORIES(${MYSQL_DIR}/lib)
  ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")

  TARGET_LINK_LIBRARIES(desodbc-installer desodbc-util ${ODBCLIB} ${ODBCINSTLIB}
        ${MYSQL_CLIENT_LIBS} legacy_stdio_definitions.lib)

  IF (WITH_NODEFAULTLIB)
    SET_TARGET_PROPERTIES(desodbc-installer PROPERTIES
      LINK_FLAGS_DEBUG "/NODEFAULTLIB:${WITH_NODEFAULTLIB}"
      LINK_FLAGS_RELWITHDEBINFO "/NODEFAULTLIB:${WITH_NODEFAULTLIB}"
      LINK_FLAGS_RELEASE "/NODEFAULTLIB:${WITH_NODEFAULTLIB}")
  ENDIF ()

  SET_TARGET_PROPERTIES(desodbc-installer PROPERTIES
        LINK_FLAGS "${MYSQLODBCCONN_LINK_FLAGS_ENV} ${MYSQL_LINK_FLAGS}")
ELSE(WIN32)
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ODBC_LINK_FLAGS}")
  TARGET_LINK_LIBRARIES(desodbc-installer desodbc-util
      ${ODBC_LINK_FLAGS} ${MYSQL_CLIENT_LIBS} ${CMAKE_THREAD_LIBS_INIT} m)
  SET_TARGET_PROPERTIES(desodbc-installer PROPERTIES
        LINK_FLAGS "${MYSQLODBCCONN_LINK_FLAGS_ENV} ${MYSQL_LINK_FLAGS}")
ENDIF(WIN32)

if(APPLE)
  set_property(TARGET desodbc-installer PROPERTY BUILD_WITH_INSTALL_RPATH ON)
  set_property(TARGET desodbc-installer APPEND PROPERTY INSTALL_RPATH "@loader_path/../lib")
  set_property(TARGET desodbc-installer APPEND PROPERTY INSTALL_RPATH "@loader_path/../lib/private")
  set_property(TARGET desodbc-installer APPEND PROPERTY INSTALL_RPATH "/usr/local/iODBC/lib")
  set_property(TARGET desodbc-installer APPEND PROPERTY INSTALL_RPATH "/usr/local/var/homebrew/linked/libiodbc/lib")
elseif(NOT WIN32)
  set_property(TARGET desodbc-installer PROPERTY BUILD_WITH_INSTALL_RPATH ON)
  set_property(TARGET desodbc-installer APPEND PROPERTY INSTALL_RPATH "$ORIGIN/../lib/")
  set_property(TARGET desodbc-installer APPEND PROPERTY INSTALL_RPATH "$ORIGIN/../lib/private")
endif()

mangle_osx_rpaths(desodbc-installer)

IF(MYSQL_CXX_LINKAGE)
  SET_TARGET_PROPERTIES(desodbc-installer PROPERTIES
        LINKER_LANGUAGE CXX
        COMPILE_FLAGS "${MYSQLODBCCONN_COMPILE_FLAGS_ENV} ${MYSQL_CXXFLAGS}")
ENDIF(MYSQL_CXX_LINKAGE)

IF (WIN32)
  INSTALL(TARGETS desodbc-installer
    CONFIGURATIONS Debug
    DESTINATION bin/Debug)

  INSTALL(TARGETS desodbc-installer
    CONFIGURATIONS Release RelWithDebInfo
    DESTINATION bin)
ELSE()
  INSTALL(TARGETS desodbc-installer DESTINATION bin)
ENDIF()
