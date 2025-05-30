# Copyright (c) 2007, 2024, Oracle and/or its affiliates.
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

##########################################################################

ADD_EXECUTABLE(dltest dltest.c)
set_target_properties(dltest PROPERTIES FOLDER Tests)
set_target_properties(dltest PROPERTIES PDB_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/test/)

add_version_info(dltest
  "MySQL Connector/ODBC driver module tester."
  "Checks whether the driver can be correctly loaded at run-time."
)

IF(NOT WIN32)

  INCLUDE_DIRECTORIES(${DL_INCLUDES})

  TARGET_LINK_LIBRARIES(dltest ${DL_LIBS})

  SET_TARGET_PROPERTIES(dltest PROPERTIES LINK_FLAGS "${DL_LFLAGS}")

ENDIF(NOT WIN32)

INSTALL(TARGETS dltest DESTINATION test COMPONENT tests)
