// Copyright (c) 2003, 2024, Oracle and/or its affiliates.
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

#include "odbctap.h"

#define TEST_BUFFER_SIZE 256

DECLARE_TEST(simple_select_standard)
{
  ok_sql(hstmt, "DROP TABLE IF EXISTS tabletest");

  ok_sql(hstmt,
         "CREATE TABLE tabletest (id INT PRIMARY KEY, name VARCHAR(20))");

  ok_sql(hstmt, "INSERT INTO tabletest VALUES (1,'foo'),(2,'bar'),(3,'baz')");

  ok_sql(hstmt, "SELECT * FROM tabletest");


  SQLCHAR buffer[TEST_BUFFER_SIZE];
  ok_stmt(hstmt, SQLFetch(hstmt));

  ok_stmt(hstmt, SQLGetData(hstmt, 1, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "1", 1);
  ok_stmt(hstmt, SQLGetData(hstmt, 2, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "foo", 3);

  ok_stmt(hstmt, SQLFetch(hstmt));

  ok_stmt(hstmt, SQLGetData(hstmt, 1, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "2", 1);
  ok_stmt(hstmt, SQLGetData(hstmt, 2, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "bar", 3);

  ok_stmt(hstmt, SQLFetch(hstmt));

  ok_stmt(hstmt, SQLGetData(hstmt, 1, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "3", 1);
  ok_stmt(hstmt, SQLGetData(hstmt, 2, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "baz", 3);

  //ok_stmt(hstmt, SQLFreeHandle(SQL_HANDLE_STMT, hstmt));
  return OK;
  
}

DECLARE_TEST(simple_select_block) {
#define BLOCK_SIZE 2

  ok_sql(hstmt, "DROP TABLE IF EXISTS tabletest");

  ok_sql(hstmt, "CREATE TABLE tabletest (id INT PRIMARY KEY, name VARCHAR(20))");

  ok_sql(hstmt, "INSERT INTO tabletest VALUES (1,'foo'),(2,'bar'),(3,'baz')");

  ok_stmt(hstmt,
          SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_BIND_TYPE, SQL_BIND_BY_COLUMN, 0));
  ok_stmt(hstmt, SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_ARRAY_SIZE,
                                (SQLPOINTER)BLOCK_SIZE, 0));

  SQLULEN rowsFetched;
  ok_stmt(hstmt,
          SQLSetStmtAttr(hstmt, SQL_ATTR_ROWS_FETCHED_PTR, &rowsFetched, 0));

  SQLUSMALLINT rowStatus[BLOCK_SIZE];
  ok_stmt(hstmt, SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_STATUS_PTR, rowStatus,
                                0));  // TODO: remove it?

  char col1[BLOCK_SIZE][256];
  SQLLEN col1_lens[BLOCK_SIZE];

  char col2[BLOCK_SIZE][256];
  SQLLEN col2_lens[BLOCK_SIZE];

  ok_stmt(hstmt, SQLCloseCursor(hstmt));

  ok_sql(hstmt, "SELECT * FROM tabletest");

  ok_stmt(hstmt,
          SQLBindCol(hstmt, 1, SQL_C_CHAR, col1, sizeof(col1[0]), col1_lens));
  ok_stmt(hstmt,
          SQLBindCol(hstmt, 2, SQL_C_CHAR, col2, sizeof(col2[0]), col2_lens));

  ok_stmt(hstmt, SQLFetch(hstmt));
  is_num(rowsFetched, 2);

  is_str(col1[0], "1", 1);
  is_str(col2[0], "foo", 3);

  is_str(col1[1], "2", 1);
  is_str(col2[1], "bar", 3);

  ok_stmt(hstmt, SQLFetch(hstmt));
  is_num(rowsFetched, 1);

  is_str(col1[0], "3", 1);
  is_str(col2[0], "baz", 3);

  return OK;
}

DECLARE_TEST(parameter_binding) {
  ok_sql(hstmt, "DROP TABLE IF EXISTS tabletest");

  ok_sql(hstmt, "CREATE TABLE tabletest (id INT PRIMARY KEY, name VARCHAR(20))");

  ok_sql(hstmt, "INSERT INTO tabletest VALUES (1,'foo'),(2,'bar'),(3,'baz')");

  ok_stmt(hstmt,
          SQLPrepare(hstmt, "SELECT * FROM tabletest WHERE id = ? AND name = ?", SQL_NTS));

  int id = 2;
  ok_stmt(hstmt,
          SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_BIGINT,
                           0, 0, &id, 0, NULL));
  char name[256] = "bar";
  ok_stmt(hstmt,
          SQLBindParameter(hstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR,
                           sizeof(name), 0, name, sizeof(name), NULL));

  ok_stmt(hstmt, SQLExecute(hstmt));

  ok_stmt(hstmt, SQLFetch(hstmt));

  SQLCHAR buffer[TEST_BUFFER_SIZE];
  ok_stmt(hstmt, SQLGetData(hstmt, 1, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "2", 1);
  ok_stmt(hstmt, SQLGetData(hstmt, 2, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "bar", 3);

  return OK;
}

DECLARE_TEST(application_variables) {
  ok_sql(hstmt, "DROP TABLE IF EXISTS tabletest");

  ok_sql(hstmt, "CREATE TABLE tabletest (id INT PRIMARY KEY, name VARCHAR(20))");

  ok_sql(hstmt, "INSERT INTO tabletest VALUES (1,'foo'),(2,'bar'),(3,'baz')");

  ok_sql(hstmt, "SELECT * FROM tabletest");

  char snd_column[TEST_BUFFER_SIZE];
  ok_stmt(hstmt, SQLBindCol(hstmt, 2, SQL_C_CHAR, snd_column, TEST_BUFFER_SIZE, NULL));

  ok_stmt(hstmt, SQLFetch(hstmt));
  is_str(snd_column, "foo", 3);

  ok_stmt(hstmt, SQLFetch(hstmt));
  is_str(snd_column, "bar", 3);

  ok_stmt(hstmt, SQLFetch(hstmt));
  is_str(snd_column, "baz", 3);

  return OK;
}

/*
This test is not very useful as we extract the type conversion completely
from MyODBC. We will see a transformation between a string and a uint64
(DES' int)
*/ 
DECLARE_TEST(type_conversion) {
  ok_sql(hstmt, "DROP TABLE IF EXISTS tabletest");

  ok_sql(hstmt, "CREATE TABLE tabletest (number VARCHAR(20))");

  ok_sql(hstmt, "INSERT INTO tabletest VALUES ('123')");

  SQLUBIGINT num;  // we will convert a string to an unsigned bigint (DES integer).

  ok_sql(hstmt, "SELECT * FROM tabletest");

  ok_stmt(hstmt, SQLBindCol(hstmt, 1, SQL_C_UBIGINT, &num, 0, NULL));

  ok_stmt(hstmt, SQLFetch(hstmt));

  is_num(num, 123);

  ok_stmt(hstmt, SQLCloseCursor(hstmt));

  ok_sql(hstmt, "create or replace table birthdays(d date)");
  ok_sql(hstmt, "insert into birthdays values(cast(\'2025-02-13\' as date))");
  ok_sql(hstmt, "select * from birthdays");

  ok_stmt(hstmt, SQLFetch(hstmt));
  
  SQLCHAR datebuf[TEST_BUFFER_SIZE];
  SQLLEN len;

  ok_stmt(hstmt, SQLGetData(hstmt, 1, SQL_C_CHAR, datebuf, sizeof(datebuf),
                   &len));

  is_str("2025-02-13", datebuf, len);

  return OK;
}

DECLARE_TEST(sqlcolumns) {
  ok_sql(hstmt, "DROP TABLE IF EXISTS tabletest");

  ok_sql(hstmt, "CREATE TABLE tabletest (id INT PRIMARY KEY, name VARCHAR(20))");

  expect_odbc(hstmt, SQL_HANDLE_STMT,
              SQLColumnsW(hstmt, L"nonexistentcatalog", SQL_NTS, L"", 0,
                          L"t_b%",
                                SQL_NTS, L"name", SQL_NTS), SQL_ERROR);
  ok_stmt(hstmt,
          SQLColumnsW(
              hstmt, L"$des", SQL_NTS, L"", 0, L"t_b%", SQL_NTS, L"name",
              SQL_NTS));  // we need to escape the first character _ so
                          // that it isn't recognised as pattern at that point.

  SQLCHAR buffer[TEST_BUFFER_SIZE];
  ok_stmt(hstmt, SQLFetch(hstmt));

  ok_stmt(hstmt, SQLGetData(hstmt, 4, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));

  is_str(buffer, "name", 4);

  ok_stmt(hstmt, SQLCloseCursor(hstmt));

  ok_stmt(hstmt, SQLColumnsW(hstmt, L"$des", SQL_NTS, L"", 0, L"tabletest",
                             SQL_NTS, L"%", SQL_NTS));

  ok_stmt(hstmt, SQLFetch(hstmt));
  ok_stmt(hstmt, SQLGetData(hstmt, 4, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "id", 2);

  ok_stmt(hstmt, SQLFetch(hstmt));
  ok_stmt(hstmt, SQLGetData(hstmt, 4, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "name", 4);

  return OK;
}

DECLARE_TEST(sqlgettypeinfo) {
  ok_stmt(hstmt, SQLGetTypeInfo(hstmt, SQL_TYPE_DATE));

  ok_stmt(hstmt, SQLFetch(hstmt));

  SQLCHAR buffer[TEST_BUFFER_SIZE];
  ok_stmt(hstmt, SQLGetData(hstmt, 1, SQL_C_CHAR, buffer, sizeof(buffer),
                            NULL));  // TYPE_NAME col (name of the data type in DES)

  is_str(buffer, "date", 4);

  return OK;
}

DECLARE_TEST(sqlprimarykeys) {
  ok_sql(hstmt, "/process examples/SQLDebugger/awards1.sql");

  expect_odbc(hstmt, SQL_HANDLE_STMT,
          SQLPrimaryKeysW(hstmt, L"nonexistentcatalog", SQL_NTS, "", 0, L"courses", SQL_NTS), SQL_ERROR);

  ok_stmt(hstmt,
          SQLPrimaryKeysW(hstmt, L"$des", SQL_NTS, "", 0, L"courses", SQL_NTS));

  ok_stmt(hstmt, SQLFetch(hstmt));

  SQLCHAR buffer[TEST_BUFFER_SIZE];
  ok_stmt(hstmt, SQLGetData(hstmt, 4, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE,
                            NULL));  // COLUMN_NAME col

  is_str(buffer, "id", 2);

  return OK;
}

DECLARE_TEST(sqlforeignkeys) {
  SQLCHAR buffer[TEST_BUFFER_SIZE];

  ok_sql(hstmt, "/process examples/SQLDebugger/awards1.sql");

  expect_odbc(hstmt, SQL_HANDLE_STMT,
              SQLForeignKeysW(hstmt, L"othercatalog", SQL_NTS, L"", 0, L"courses",
                              SQL_NTS, L"$des", SQL_NTS, L"", 0, L"", 0),
              SQL_ERROR);

  ok_stmt(hstmt, SQLCloseCursor(hstmt));

  ok_stmt(hstmt, SQLForeignKeysW(hstmt, L"$des", SQL_NTS, L"", 0, L"courses",
                                 SQL_NTS, L"$des", SQL_NTS, L"", 0, L"", 0));

  ok_stmt(hstmt, SQLFetch(hstmt));
  ok_stmt(hstmt, SQLGetData(hstmt, 3, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "courses", 7);
  ok_stmt(hstmt, SQLGetData(hstmt, 4, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "id", 2);

  ok_stmt(hstmt, SQLGetData(hstmt, 7, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "registration", 12);
  ok_stmt(hstmt, SQLGetData(hstmt, 8, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "course", 6);

  while (SQLFetch(hstmt) != SQL_NO_DATA);

  ok_stmt(hstmt, SQLCloseCursor(hstmt));

  ok_stmt(hstmt,
          SQLForeignKeysW(hstmt, L"$des", SQL_NTS, L"", 0, L"", 0, L"$des",
                          SQL_NTS, L"", 0, L"registration", SQL_NTS));

  ok_stmt(hstmt, SQLFetch(hstmt));
  ok_stmt(hstmt, SQLGetData(hstmt, 3, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "courses", 7);
  ok_stmt(hstmt, SQLGetData(hstmt, 4, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "id", 2);

  ok_stmt(hstmt, SQLGetData(hstmt, 7, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "registration", 12);
  ok_stmt(hstmt, SQLGetData(hstmt, 8, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "course", 6);

  ok_stmt(hstmt, SQLCloseCursor(hstmt));

  ok_stmt(hstmt,
          SQLForeignKeysW(hstmt, L"$des", SQL_NTS, L"", 0, L"courses", SQL_NTS,
                          L"$des", SQL_NTS, L"", 0, L"registration", SQL_NTS));
  ok_stmt(hstmt, SQLFetch(hstmt));
  ok_stmt(hstmt, SQLGetData(hstmt, 3, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "courses", 7);
  ok_stmt(hstmt, SQLGetData(hstmt, 4, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "id", 2);

  ok_stmt(hstmt, SQLGetData(hstmt, 7, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "registration", 12);
  ok_stmt(hstmt, SQLGetData(hstmt, 8, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE, NULL));
  is_str(buffer, "course", 6);

  return OK;
}

DECLARE_TEST(sqlspecialcolumns) {
  ok_sql(hstmt, "/process examples/SQLDebugger/awards1.sql");

  expect_odbc(
      hstmt, SQL_HANDLE_STMT,
      SQLSpecialColumnsW(hstmt, SQL_BEST_ROWID, L"othercatalog", SQL_NTS, L"", 0,
                         L"courses", SQL_NTS, SQL_SCOPE_SESSION, SQL_NULLABLE), SQL_ERROR);

  ok_stmt(hstmt,
          SQLSpecialColumnsW(hstmt, SQL_BEST_ROWID, L"$des", SQL_NTS, L"", 0,
                            L"courses", SQL_NTS, SQL_SCOPE_SESSION, SQL_NULLABLE));

  ok_stmt(hstmt, SQLFetch(hstmt));

  SQLCHAR buffer[TEST_BUFFER_SIZE];
  ok_stmt(hstmt, SQLGetData(hstmt, 2, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE,
                            NULL));  // COLUMN_NAME col

  is_str(buffer, "id", 2);

  return OK;
}

DECLARE_TEST(sqlstatistics) {
  ok_sql(hstmt, "DROP TABLE IF EXISTS tabletest");

  ok_sql(hstmt, "CREATE TABLE tabletest (id INT PRIMARY KEY, name VARCHAR(20))");

  ok_sql(hstmt, "INSERT INTO tabletest VALUES (1,'foo'),(2,'bar'),(3,'baz')");

  expect_odbc(hstmt, SQL_HANDLE_STMT,
              SQLStatisticsW(hstmt, L"nonexistentcatalog", SQL_NTS, L"", 0, L"tabletest",
                             SQL_NTS, SQL_INDEX_ALL, SQL_ENSURE), SQL_ERROR);

  ok_stmt(hstmt, SQLStatisticsW(hstmt, L"$des", SQL_NTS, L"", 0, L"tabletest", SQL_NTS, SQL_INDEX_ALL, SQL_ENSURE));

  ok_stmt(hstmt, SQLFetch(hstmt));

  SQLCHAR buffer[TEST_BUFFER_SIZE];
  ok_stmt(hstmt, SQLGetData(hstmt, 3, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE,
                            NULL));  // TABLE_NAME col
  is_str(buffer, "tabletest", 7);

  ok_stmt(hstmt, SQLGetData(hstmt, 11, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE,
                            NULL));  // CARDINALITY col
  is_str(buffer, "3", 1);

  return OK;
}

DECLARE_TEST(sqltables) {
  ok_sql(hstmt, "DROP TABLE IF EXISTS tabletest");

  ok_sql(hstmt, "CREATE TABLE tabletest (id INT PRIMARY KEY, name VARCHAR(20))");

  ok_stmt(hstmt, SQLTablesW(hstmt, L"$des", SQL_NTS, L"", 0, L"%", SQL_NTS, L"%",
                           SQL_NTS));

  ok_stmt(hstmt, SQLFetch(hstmt));

  SQLCHAR buffer[TEST_BUFFER_SIZE];
  ok_stmt(hstmt, SQLGetData(hstmt, 3, SQL_C_CHAR, buffer, TEST_BUFFER_SIZE,
                            NULL));  // TABLE_NAME col
  is_str(buffer, "tabletest", 7);

  return OK;
}

DECLARE_TEST(sqlsetpos_standard) {

  ok_stmt(hstmt, SQLSetStmtAttr(hstmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));
  ok_sql(hstmt, "DROP TABLE IF EXISTS tabletest");

  ok_sql(hstmt, "CREATE TABLE tabletest (id INT PRIMARY KEY, name VARCHAR(20))");

  ok_sql(hstmt,
         "INSERT INTO tabletest VALUES (1,'foo'),(2,'bar'),(3,'baz'),(4, 'bax')");

  ok_sql(hstmt, "SELECT * FROM tabletest");

  SQLRETURN rc = SQLFetchScroll(hstmt, SQL_FETCH_ABSOLUTE, 4);

  ok_stmt(hstmt, SQLSetPos(hstmt, 1, SQL_DELETE, SQL_LOCK_NO_CHANGE));

  rc = SQLFetchScroll(hstmt, SQL_FETCH_ABSOLUTE, 2);

  char snd_column[256];
  ok_stmt(hstmt, SQLBindCol(hstmt, 2, SQL_C_CHAR, snd_column,
                            sizeof(snd_column), NULL));
  ok_stmt(hstmt, SQLSetPos(hstmt, 1, SQL_REFRESH, SQL_LOCK_NO_CHANGE));
  is_str(snd_column, "bar", 3);

  ok_stmt(hstmt, SQLSetPos(hstmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE)); //the position is relative to the current (single) rowset

  SQLCHAR buffer[256];
  ok_stmt(hstmt,
          SQLGetData(hstmt, 2, SQL_C_CHAR, buffer, sizeof(buffer), NULL));
  is_str(buffer, "bar", 3);

  return OK;
}


DECLARE_TEST(sqlsetpos_block) {

  ok_stmt(hstmt, SQLSetStmtAttr(hstmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));

  ok_sql(hstmt, "DROP TABLE IF EXISTS tabletest");

  ok_sql(hstmt, "CREATE TABLE tabletest (id INT PRIMARY KEY, name VARCHAR(20))");

  ok_sql(hstmt,
         "INSERT INTO tabletest VALUES "
         "(1,'a'),(2,'b'),(3,'c'),(4,'d'),(5,'e'),(6,'f'),(7,'g'),(8,'h')");

  ok_stmt(hstmt,
          SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_BIND_TYPE, SQL_BIND_BY_COLUMN, 0));
  ok_stmt(hstmt, SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_ARRAY_SIZE,
                                (SQLPOINTER)BLOCK_SIZE, 0));

  SQLULEN rowsFetched;
  ok_stmt(hstmt,
          SQLSetStmtAttr(hstmt, SQL_ATTR_ROWS_FETCHED_PTR, &rowsFetched, 0));

  SQLUSMALLINT rowStatus[BLOCK_SIZE];
  ok_stmt(hstmt, SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_STATUS_PTR, rowStatus,
                                0));  // TODO: remove it?

  char col1[BLOCK_SIZE][256];
  SQLLEN col1_lens[BLOCK_SIZE];

  char col2[BLOCK_SIZE][256];
  SQLLEN col2_lens[BLOCK_SIZE];

  ok_sql(hstmt, "SELECT * FROM tabletest");

  ok_stmt(hstmt,
          SQLBindCol(hstmt, 1, SQL_C_CHAR, col1, sizeof(col1[0]), col1_lens));
  ok_stmt(hstmt,
          SQLBindCol(hstmt, 2, SQL_C_CHAR, col2, sizeof(col2[0]), col2_lens));


  SQLRETURN rc = SQLFetchScroll(hstmt, SQL_FETCH_ABSOLUTE, 4);

  ok_stmt(hstmt, SQLSetPos(hstmt, 2, SQL_DELETE, SQL_LOCK_NO_CHANGE)); //we remove the second row from the size 2 rowset

  rc = SQLFetchScroll(hstmt, SQL_FETCH_ABSOLUTE, 2);

  ok_stmt(hstmt, SQLSetPos(hstmt, 2, SQL_REFRESH, SQL_LOCK_NO_CHANGE)); //we refresh from the 2th row of the rowset -> rows 3 and 4

  is_str(col1[0], "3", 1);
  is_str(col2[0], "c", 1);
  is_str(col1[1], "4", 1);
  is_str(col2[1], "d", 1);

  rc = SQLFetchScroll(hstmt, SQL_FETCH_ABSOLUTE, 3);

  ok_stmt(hstmt, SQLSetPos(hstmt, 1, SQL_POSITION, SQL_LOCK_NO_CHANGE));
  is_str(col1[0], "3", 1);
  is_str(col2[0], "c", 1);

  return OK;
}

/* DESODBC:
    SQL Bookmark Delete using SQLBulkOperations SQL_DELETE_BY_BOOKMARK operation
    Original author: MyODBC (t_bookmark_delete)
*/
DECLARE_TEST(bookmarks) {
  SQLLEN len = 0;
  SQLUSMALLINT rowStatus[4];
  SQLULEN numRowsFetched;
  SQLINTEGER nData[4];
  SQLCHAR szData[4][16];
  SQLCHAR bData[4][10];
  SQLLEN nRowCount;

  ok_sql(hstmt, "drop table if exists t_bookmark");
  ok_sql(hstmt,
         "CREATE TABLE t_bookmark ("
         "tt_int INT PRIMARY KEY,"
         "tt_varchar VARCHAR(128) NOT NULL)");
  ok_sql(hstmt,
         "INSERT INTO t_bookmark VALUES "
         "(1, 'string 1'),"
         "(2, 'string 2'),"
         "(3, 'string 3'),"
         "(4, 'string 4')");

  ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_CLOSE));

  ok_stmt(hstmt, SQLSetStmtAttr(hstmt, SQL_ATTR_USE_BOOKMARKS,
                                (SQLPOINTER)SQL_UB_VARIABLE, 0));
  ok_stmt(hstmt, SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_STATUS_PTR,
                                (SQLPOINTER)rowStatus, 0));
  ok_stmt(hstmt,
          SQLSetStmtAttr(hstmt, SQL_ATTR_ROWS_FETCHED_PTR, &numRowsFetched, 0));
  ok_stmt(hstmt, SQLSetStmtAttr(hstmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));
  ok_stmt(hstmt, SQLSetStmtOption(hstmt, SQL_ROWSET_SIZE, 4));

  ok_sql(hstmt, "select * from t_bookmark order by 1");
  ok_stmt(hstmt, SQLBindCol(hstmt, 0, SQL_C_VARBOOKMARK, bData,
                            sizeof(bData[0]), NULL));
  ok_stmt(hstmt, SQLBindCol(hstmt, 1, SQL_C_LONG, nData, 0, NULL));
  ok_stmt(hstmt,
          SQLBindCol(hstmt, 2, SQL_C_CHAR, szData, sizeof(szData[0]), NULL));

  ok_stmt(hstmt, SQLFetchScroll(hstmt, SQL_FETCH_BOOKMARK, 0));

  is_num(nData[0], 1);
  is_str(szData[0], "string 1", 8);
  is_num(nData[1], 2);
  is_str(szData[1], "string 2", 8);
  is_num(nData[2], 3);
  is_str(szData[2], "string 3", 8);
  is_num(nData[3], 4);
  is_str(szData[3], "string 4", 8);

  ok_stmt(hstmt, SQLSetStmtOption(hstmt, SQL_ROWSET_SIZE, 2));
  ok_stmt(hstmt, SQLBulkOperations(hstmt, SQL_DELETE_BY_BOOKMARK));
  ok_stmt(hstmt, SQLRowCount(hstmt, &nRowCount));
  is_num(nRowCount, 2);

  ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_CLOSE));

  ok_stmt(hstmt, SQLSetStmtOption(hstmt, SQL_ROWSET_SIZE, 4));
  ok_sql(hstmt, "select * from t_bookmark order by 1");
  ok_stmt(hstmt, SQLFetchScroll(hstmt, SQL_FETCH_FIRST, 0));

  is_num(nData[0], 3);
  is_str(szData[0], "string 3", 8);
  is_num(nData[1], 4);
  is_str(szData[1], "string 4", 8);

  memset(nData, 0, sizeof(nData));
  memset(szData, 'x', sizeof(szData));
  ok_stmt(hstmt, SQLSetStmtOption(hstmt, SQL_ROWSET_SIZE, 4));
  expect_stmt(hstmt, SQLBulkOperations(hstmt, SQL_FETCH_BY_BOOKMARK),
              SQL_SUCCESS_WITH_INFO);

  is_num(nData[0], 3);
  is_str(szData[0], "string 3", 8);
  is_num(nData[1], 4);
  is_str(szData[1], "string 4", 8);
  is_num(nData[2], 0);
  is_str(szData[2], "xxxxxxxx", 8);
  is_num(nData[3], 0);
  is_str(szData[3], "xxxxxxxx", 8);

  ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_CLOSE));
  ok_sql(hstmt, "drop table if exists t_bookmark");

  return OK;
}

/* DESODBC:
    Test on bulk operations.
    Original author: MyODBC (t_bulk_insert_bookmark)
    Modified by: DESODBC Developer
*/
DECLARE_TEST(bulk_operations) {
#define MAX_INSERT_COUNT 50
#define MAX_BM_INS_COUNT 20
  SQLINTEGER i, id[MAX_INSERT_COUNT + 1];
  SQLCHAR name[MAX_INSERT_COUNT][40], txt[MAX_INSERT_COUNT][60],
      ltxt[MAX_INSERT_COUNT][70];
  SQLDOUBLE dt, dbl[MAX_INSERT_COUNT];
  SQLLEN name_len[MAX_INSERT_COUNT], txt_len[MAX_INSERT_COUNT],
      ltxt_len[MAX_INSERT_COUNT];

  /*
    DESODBC: duplicates are not allowed by default.
    This will be necessary because we will repeat twice the same SQLBulkOperations.
  */
  ok_sql(hstmt, "/duplicates on");
  ok_sql(hstmt, "DROP TABLE IF EXISTS t_bulk_insert");
  ok_sql(hstmt,
         "CREATE TABLE t_bulk_insert (id INT, v VARCHAR(100),"
         "txt TEXT, ft FLOAT(10), ltxt VARCHAR)");

  ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_CLOSE));

  dt = 0.23456;

  ok_stmt(hstmt, SQLSetStmtAttr(hstmt, SQL_ATTR_CURSOR_TYPE,
                                (SQLPOINTER)SQL_CURSOR_STATIC, 0));
  ok_stmt(hstmt, SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_ARRAY_SIZE,
                                (SQLPOINTER)MAX_INSERT_COUNT, 0));
  /*
  ok_stmt(hstmt, SQLSetStmtAttr(hstmt, SQL_ATTR_CONCURRENCY,
                                (SQLPOINTER)SQL_CONCUR_ROWVER, 0));
    */
  ok_stmt(hstmt, SQLBindCol(hstmt, 1, SQL_C_LONG, id, 0, NULL));
  ok_stmt(hstmt, SQLBindCol(hstmt, 2, SQL_C_CHAR, name, sizeof(name[0]),
                            &name_len[0]));
  ok_stmt(hstmt,
          SQLBindCol(hstmt, 3, SQL_C_CHAR, txt, sizeof(txt[0]), &txt_len[0]));
  ok_stmt(hstmt, SQLBindCol(hstmt, 4, SQL_C_DOUBLE, dbl, 0, NULL));
  ok_stmt(hstmt, SQLBindCol(hstmt, 5, SQL_C_CHAR, ltxt, sizeof(ltxt[0]),
                            &ltxt_len[0]));

  ok_sql(hstmt, "SELECT id, v, txt, ft, ltxt FROM t_bulk_insert");

  expect_stmt(hstmt, SQLFetchScroll(hstmt, SQL_FETCH_NEXT, 0),
              SQL_NO_DATA_FOUND);

  for (i = 0; i < MAX_INSERT_COUNT; i++) {
    id[i] = i;
    dbl[i] = i + dt;
    sprintf((char *)name[i], "Varchar%d", i);
    sprintf((char *)txt[i], "Text%d", i);
    sprintf((char *)ltxt[i], "LongText, id row:%d", i);
    name_len[i] = strlen(name[i]);
    txt_len[i] = strlen(txt[i]);
    ltxt_len[i] = strlen(ltxt[i]);
  }

  ok_stmt(hstmt, SQLBulkOperations(hstmt, SQL_ADD));
  ok_stmt(hstmt, SQLBulkOperations(hstmt, SQL_ADD));

  ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_UNBIND));
  ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_CLOSE));

  ok_stmt(hstmt,
          SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)1, 0));

  ok_sql(hstmt, "SELECT * FROM t_bulk_insert");
  is_num(myrowcount(hstmt), MAX_INSERT_COUNT * 2);

  ok_stmt(hstmt, SQLFreeStmt(hstmt, SQL_CLOSE));

  ok_sql(hstmt, "DROP TABLE IF EXISTS t_bulk_insert");

  return OK;
}

/*
    DESODBC: SQLDisconnect is impliticly tested
    in all of these tests. However, we should test it
    when multiple users are connected.

    Original author: DESODBC Developer
*/
DECLARE_TEST(sqldisconnect) {

    //In this point, we are currently connected.
    //We shall make another connection.
  SQLHDBC newhdbc = NULL;
    expect_odbc(henv, SQL_HANDLE_ENV,
                SQLAllocHandle(SQL_HANDLE_DBC, henv, &newhdbc), SQL_SUCCESS);

  expect_odbc(
        newhdbc, SQL_HANDLE_DBC,
              SQLConnect(newhdbc, mydsn, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS), SQL_SUCCESS);

  expect_odbc(newhdbc, SQL_HANDLE_DBC,
              SQLDisconnect(newhdbc),
              SQL_SUCCESS);

  expect_odbc(newhdbc, SQL_HANDLE_DBC, SQLFreeHandle(SQL_HANDLE_DBC, newhdbc), SQL_SUCCESS);

  ok_sql(hstmt, "create or replace table a(b int)"); //Test whether the main connection is still working fine

  return OK;
}
/*
    DESODBC: SQLFreeHandle is impliticly tested
    in all of these tests. However, we should make a manual
    check.

    Original author: DESODBC Developer
*/
DECLARE_TEST(sqlfreehandle) {
  SQLHDBC newhenv = NULL;
  SQLHDBC newhdbc = NULL;
  SQLHDBC newhstmt = NULL;

  SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &newhenv);
  if (ret != SQL_SUCCESS)
    return FAIL;

  expect_odbc(
      newhenv, SQL_HANDLE_ENV,
      SQLSetEnvAttr(newhenv, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0),
      SQL_SUCCESS);

  expect_odbc(newhenv, SQL_HANDLE_ENV,
              SQLAllocHandle(SQL_HANDLE_DBC, newhenv, &newhdbc), SQL_SUCCESS);


  expect_odbc(newhdbc, SQL_HANDLE_DBC,
              SQLConnect(newhdbc, mydsn, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS),
              SQL_SUCCESS);

  expect_odbc(newhdbc, SQL_HANDLE_STMT,
              SQLAllocHandle(SQL_HANDLE_STMT, newhdbc, &newhstmt), SQL_SUCCESS);

  ok_sql(newhstmt, "DROP TABLE IF EXISTS tabletest");
  ok_stmt(newhstmt, SQLCloseCursor(newhstmt));

  ok_sql(newhstmt,
         "CREATE TABLE tabletest (id INT PRIMARY KEY, name VARCHAR(20))");
  ok_stmt(newhstmt, SQLCloseCursor(newhstmt));

  ok_sql(newhstmt,
         "INSERT INTO tabletest VALUES (1,'foo'),(2,'bar'),(3,'baz')");
  ok_stmt(newhstmt, SQLCloseCursor(newhstmt));

  ok_sql(hstmt, "SELECT * FROM tabletest");
  ok_stmt(newhstmt, SQLCloseCursor(newhstmt));

  ok_stmt(newhstmt, SQLFreeHandle(SQL_HANDLE_STMT, newhstmt));

  expect_odbc(newhdbc, SQL_HANDLE_DBC, SQLDisconnect(newhdbc), SQL_SUCCESS);

  expect_odbc(newhdbc, SQL_HANDLE_DBC, SQLFreeHandle(SQL_HANDLE_DBC, newhdbc),
              SQL_SUCCESS);

  expect_odbc(newhenv, SQL_HANDLE_ENV, SQLFreeHandle(SQL_HANDLE_ENV, newhenv),
              SQL_SUCCESS);

  return OK;

}

DECLARE_TEST(error_handling) {

  expect_odbc(hstmt, SQL_HANDLE_STMT,
      SQLExecDirect(hstmt, (SQLCHAR *)"select * from inexistent_table", SQL_NTS),
            SQL_ERROR);

  SQLCHAR sql_state[1024];
  SQLCHAR sql_msg[1024];

  ok_stmt(hstmt, SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, sql_state, NULL,
                                 sql_msg, 1024, NULL));

  const char *preffix = "Full TAPI output: $error";
  size_t preffix_length = 24;
  if (memcmp(sql_msg, preffix, preffix_length)) return FAIL;

  return OK;
}

DECLARE_TEST(obtain_info) {

  SQLCHAR current_catalog[TEST_BUFFER_SIZE];
  SQLLEN len_curr_cat;

  ok_sql(hstmt, "/open_db postgresql"); //available in my operating system.

  ok_stmt(hdbc, SQLGetConnectAttr(hdbc, SQL_ATTR_CURRENT_CATALOG, current_catalog,
                            sizeof(current_catalog), &len_curr_cat));

  is_str(current_catalog, "postgresql", len_curr_cat);

  SQLINTEGER arr_size;

  ok_stmt(hstmt, SQLGetStmtAttr(hstmt, SQL_ATTR_ROW_ARRAY_SIZE,
                                &arr_size, sizeof(SQLINTEGER), NULL));
  is_num(arr_size, 1);

  ok_stmt(hstmt, SQLSetStmtAttr(hstmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)8,
                                SQL_NTS));
  ok_stmt(hstmt, SQLGetStmtAttr(hstmt, SQL_ATTR_ROW_ARRAY_SIZE, &arr_size,
                                sizeof(SQLINTEGER), NULL));
  is_num(arr_size, 8);

  ok_sql(hstmt, "/use_db $des");

  SQLCHAR db[TEST_BUFFER_SIZE];
  SQLLEN len_db;
  ok_stmt(hstmt, SQLGetInfo(hdbc, SQL_DATABASE_NAME, db, sizeof(db), &len_db));

  is_str(db, "$des", len_db);
  
  return OK;
}

BEGIN_TESTS
ADD_TEST(simple_select_standard)
ADD_TEST(simple_select_block)
ADD_TEST(parameter_binding)
ADD_TEST(application_variables)
ADD_TEST(type_conversion)
ADD_TEST(sqlcolumns)
ADD_TEST(sqlgettypeinfo)
ADD_TEST(sqlprimarykeys)
ADD_TEST(sqlforeignkeys)
ADD_TEST(sqlspecialcolumns)
ADD_TEST(sqlstatistics)
ADD_TEST(sqltables)
ADD_TEST(sqlsetpos_standard)
ADD_TEST(sqlsetpos_block)
ADD_TEST(bookmarks)
ADD_TEST(bulk_operations)
ADD_TEST(sqldisconnect)
ADD_TEST(sqlfreehandle)
ADD_TEST(error_handling)
ADD_TEST(obtain_info)
END_TESTS


RUN_TESTS
