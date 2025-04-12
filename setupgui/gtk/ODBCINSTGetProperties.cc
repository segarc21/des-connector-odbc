// Copyright (c) 2014, 2024, Oracle and/or its affiliates.
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

#ifdef UNICODE
/*
 Unicode must be disabled, otherwise we will have lots of compiler errors
 about conflicting declarations in odbcinstext.h and odbcinst.h
*/
#undef UNICODE
#endif

#include <odbcinstext.h>
#include <cstdlib>
#include <cstring>

static const char *MYODBC_OPTIONS[][3] = {
  {"DES_EXEC",          "T", "The path of the DES executable"},
  {"DES_WORKING_DIR",   "T", "The working directory specified for DES"},
  {NULL, NULL, NULL}
};

static const char *paramsOnOff[] = {"0", "1", NULL};

int ODBCINSTGetProperties(HODBCINSTPROPERTY propertyList)
{
  int i= 0;
  do
  {
    /* Allocate next element */
    propertyList->pNext= (HODBCINSTPROPERTY)malloc(sizeof(ODBCINSTPROPERTY));
    propertyList= propertyList->pNext;

    /* Reset everything to zero */
    memset(propertyList, 0, sizeof(ODBCINSTPROPERTY));

    /* copy the option name */
    strncpy( propertyList->szName, MYODBC_OPTIONS[i][0],
             strlen(MYODBC_OPTIONS[i][0]));

    /* We make the value always empty by default */
    propertyList->szValue[0]= '\0';

    switch(MYODBC_OPTIONS[i][1][0])
    {
      /* COMBOBOX */
      case 'C':
        propertyList->nPromptType= ODBCINST_PROMPTTYPE_COMBOBOX;

        /* Prepare data for the combobox */
        propertyList->aPromptData= (char**)malloc(sizeof(paramsOnOff));
        memcpy(propertyList->aPromptData, paramsOnOff, sizeof(paramsOnOff));
      break;

      /* FILE NAME */
      case 'F':
        propertyList->nPromptType= ODBCINST_PROMPTTYPE_FILENAME;
      break;

      /* TEXTBOX */
      case 'T':
      default:
        propertyList->nPromptType= ODBCINST_PROMPTTYPE_TEXTEDIT;
    }

    /* Finally, set the help text */
    propertyList->pszHelp= strdup(MYODBC_OPTIONS[i][2]);

  }while (MYODBC_OPTIONS[++i][0]);

  return 1;
}
