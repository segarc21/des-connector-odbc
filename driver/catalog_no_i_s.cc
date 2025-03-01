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
  @file   catalog_no_i_s.c
  @brief  Catalog functions not using I_S.
  @remark All functions suppose that parameters specifying other parameters lenthes can't SQL_NTS.
          caller should take care of that.
*/

#include "driver.h"
#include "catalog.h"


#define MY_MAX_TABPRIV_COUNT 21
#define MY_MAX_COLPRIV_COUNT 3

const char *SQLTABLES_priv_values[]=
{
    NULL,"",NULL,NULL,NULL,NULL,NULL
};

DES_FIELD SQLTABLES_priv_fields[]=
{
  DESODBC_FIELD_NAME("TABLE_CAT", 0),
  DESODBC_FIELD_NAME("TABLE_SCHEM", 0),
  DESODBC_FIELD_NAME("TABLE_NAME", NOT_NULL_FLAG),
  DESODBC_FIELD_NAME("GRANTOR", 0),
  DESODBC_FIELD_NAME("GRANTEE", NOT_NULL_FLAG),
  DESODBC_FIELD_NAME("PRIVILEGE", NOT_NULL_FLAG),
  DESODBC_FIELD_NAME("IS_GRANTABLE", 0),
};

const uint SQLTABLES_PRIV_FIELDS = (uint)array_elements(SQLTABLES_priv_values);

char *SQLCOLUMNS_priv_values[]=
{
  NULL,"",NULL,NULL,NULL,NULL,NULL,NULL
};

DES_FIELD SQLCOLUMNS_priv_fields[]=
{
  DESODBC_FIELD_NAME("TABLE_CAT", 0),
  DESODBC_FIELD_NAME("TABLE_SCHEM", 0),
  DESODBC_FIELD_NAME("TABLE_NAME", NOT_NULL_FLAG),
  DESODBC_FIELD_NAME("COLUMN_NAME", NOT_NULL_FLAG),
  DESODBC_FIELD_NAME("GRANTOR", 0),
  DESODBC_FIELD_NAME("GRANTEE", NOT_NULL_FLAG),
  DESODBC_FIELD_NAME("PRIVILEGE", NOT_NULL_FLAG),
  DESODBC_FIELD_NAME("IS_GRANTABLE", 0),
};


const uint SQLCOLUMNS_PRIV_FIELDS = (uint)array_elements(SQLCOLUMNS_priv_values);


/*
****************************************************************************
SQLForeignKeys
****************************************************************************
*/

const uint SQLFORE_KEYS_FIELDS = (uint)array_elements(SQLFORE_KEYS_fields);

/* Multiple array of Struct to store and sort SQLForeignKeys field */
typedef struct SQL_FOREIGN_KEY_FIELD
{
  char PKTABLE_CAT[NAME_LEN + 1];
  char PKTABLE_SCHEM[NAME_LEN + 1];
  char PKTABLE_NAME[NAME_LEN + 1];
  char PKCOLUMN_NAME[NAME_LEN + 1];
  char FKTABLE_CAT[NAME_LEN + 1];
  char FKTABLE_SCHEM[NAME_LEN + 1];
  char FKTABLE_NAME[NAME_LEN + 1];
  char FKCOLUMN_NAME[NAME_LEN + 1];
  int  KEY_SEQ;
  int  UPDATE_RULE;
  int  DELETE_RULE;
  char FK_NAME[NAME_LEN + 1];
  char PK_NAME[NAME_LEN + 1];
  int  DEFERRABILITY;
} MY_FOREIGN_KEY_FIELD;

char *SQLFORE_KEYS_values[]= {
    NULL,"",NULL,NULL,
    NULL,"",NULL,NULL,
    0,0,0,NULL,NULL,0
};


/*
 * Get a record from the array if exists otherwise allocate a new
 * record and return.
 *
 * @param records MY_FOREIGN_KEY_FIELD record
 * @param recnum  0-based record number
 *
 * @return The requested record or NULL if it doesn't exist
 *         (and isn't created).
 */
MY_FOREIGN_KEY_FIELD *fk_get_rec(std::vector<MY_FOREIGN_KEY_FIELD> &records, unsigned int recnum)
{
  while(records.size() <= recnum)
    records.push_back(MY_FOREIGN_KEY_FIELD());

  return &records[recnum];
}


/*
****************************************************************************
SQLPrimaryKeys
****************************************************************************
*/

const uint SQLPRIM_KEYS_FIELDS = (uint)array_elements(SQLPRIM_KEYS_fields);

const long SQLPRIM_LENGTHS[]= {0, 0, 1, 5, 4, -7};

char *SQLPRIM_KEYS_values[]= {
    NULL,"",NULL,NULL,0,NULL
};

/*
****************************************************************************
SQLProcedure Columns
****************************************************************************
*/

char *SQLPROCEDURECOLUMNS_values[]= {
       "", "", NullS, NullS, "", "", "",
       "", "", "", "10", "",
       "MySQL column", "", "", NullS, "",
       NullS, ""
};

/* TODO make LONGLONG fields just LONG if SQLLEN is 4 bytes */
DES_FIELD SQLPROCEDURECOLUMNS_fields[]=
{
  DESODBC_FIELD_NAME("PROCEDURE_CAT",     0),
  DESODBC_FIELD_NAME("PROCEDURE_SCHEM",   0),
  DESODBC_FIELD_NAME("PROCEDURE_NAME",    NOT_NULL_FLAG),
  DESODBC_FIELD_NAME("COLUMN_NAME",       NOT_NULL_FLAG),
  DESODBC_FIELD_SHORT ("COLUMN_TYPE",       NOT_NULL_FLAG),
  DESODBC_FIELD_SHORT ("DATA_TYPE",         NOT_NULL_FLAG),
  DESODBC_FIELD_STRING("TYPE_NAME",         20,       NOT_NULL_FLAG),
  DESODBC_FIELD_LONGLONG("COLUMN_SIZE",       0),
  DESODBC_FIELD_LONGLONG("BUFFER_LENGTH",     0),
  DESODBC_FIELD_SHORT ("DECIMAL_DIGITS",    0),
  DESODBC_FIELD_SHORT ("NUM_PREC_RADIX",    0),
  DESODBC_FIELD_SHORT ("NULLABLE",          NOT_NULL_FLAG),
  DESODBC_FIELD_NAME("REMARKS",           0),
  DESODBC_FIELD_NAME("COLUMN_DEF",        0),
  DESODBC_FIELD_SHORT ("SQL_DATA_TYPE",     NOT_NULL_FLAG),
  DESODBC_FIELD_SHORT ("SQL_DATETIME_SUB",  0),
  DESODBC_FIELD_LONGLONG("CHAR_OCTET_LENGTH", 0),
  DESODBC_FIELD_LONG  ("ORDINAL_POSITION",  NOT_NULL_FLAG),
  DESODBC_FIELD_STRING("IS_NULLABLE",       3,        0),
};

const uint SQLPROCEDURECOLUMNS_FIELDS =
    (uint)array_elements(SQLPROCEDURECOLUMNS_fields);

/*
****************************************************************************
SQLStatistics
****************************************************************************
*/

char SS_type[10];
char *SQLSTAT_values[]={NullS,NullS,"","",NullS,"",SS_type,"","","","",NullS,NullS};

const uint SQLSTAT_FIELDS = (uint)array_elements(SQLSTAT_fields);

/*
****************************************************************************
SQLTables
****************************************************************************
*/

uint SQLTABLES_qualifier_order[]= {0};
const char *SQLTABLES_values[] = {"","",NULL,"TABLE","MySQL table"};
const char *SQLTABLES_qualifier_values[] = {"",NULL,NULL,NULL,NULL};
const char *SQLTABLES_owner_values[] = {NULL,"",NULL,NULL,NULL};
const char *SQLTABLES_type_values[3][5] = {
    {NULL,NULL,NULL,"TABLE",NULL},
    {NULL,NULL,NULL,"SYSTEM TABLE",NULL},
    {NULL,NULL,NULL,"VIEW",NULL},
};


const uint SQLTABLES_FIELDS = (uint)array_elements(SQLTABLES_values);
