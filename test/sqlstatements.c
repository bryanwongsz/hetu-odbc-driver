/*
  Copyright (C) 2018-2020. Huawei Technologies Co., Ltd. All rights reserved.
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation; version 2 of the License.
  
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
  
  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "tap.h"

/* test explain statement */
ODBC_TEST(test_query_explain)
{
    SQLHANDLE   henv1;
    SQLHANDLE   hdbc1;
    SQLHANDLE   hstmt1;

    ODBC_Connect(&henv1, &hdbc1, &hstmt1);
    
    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_query_explain");
    OK_SIMPLE_STMT(hstmt1, "CREATE TABLE test_query_explain (id INTEGER, name CHAR(20))");

    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "INSERT INTO test_query_explain (id, name) VALUES (0, 'name_value')", SQL_NTS));
    
    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_RESET_PARAMS));

    /* expect command execute success and get response from server */
    OK_SIMPLE_STMT(hstmt1, "EXPLAIN SELECT * FROM test_query_explain ORDER BY id");
    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_query_explain");
    
    ODBC_Disconnect(henv1, hdbc1, hstmt1);

    return OK;
}

/* test explain analyze statement */
ODBC_TEST(test_query_explain_analyze)
{
    SQLHANDLE   henv1;
    SQLHANDLE   hdbc1;
    SQLHANDLE   hstmt1;

    ODBC_Connect(&henv1, &hdbc1, &hstmt1);
    
    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_query_explain_analyze");
    OK_SIMPLE_STMT(hstmt1, "CREATE TABLE test_query_explain_analyze (id INTEGER, name CHAR(20))");

    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "INSERT INTO test_query_explain_analyze (id, name) VALUES (0, 'name_value')", SQL_NTS));
    
    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_RESET_PARAMS));

    /* expect command execute success and get response from server */
    OK_SIMPLE_STMT(hstmt1, "EXPLAIN ANALYZE SELECT * FROM test_query_explain_analyze ORDER BY id");
    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_query_explain_analyze");

    ODBC_Disconnect(henv1, hdbc1, hstmt1);

    return OK;
}

/* test basic values */
ODBC_TEST(test_statment_values)
{
#define CREATE_TABLE_AS_RECORD_NAME_LEN    20
#define CREATE_TABLE_AS_RECORD_ARRAY_SIZE  3
    SQLHANDLE   henv1;
    SQLHANDLE   hdbc1;
    SQLHANDLE   hstmt1;
    SQLULEN     rowsFetchedPtr = 0;
    SQLRETURN   rc;

    typedef struct {
        SQLINTEGER id;
        SQLCHAR name[CREATE_TABLE_AS_RECORD_NAME_LEN + 1];
        SQLLEN idLen;
        SQLLEN nameLen;
    }Record;
    SQLULEN      rowcnt = CREATE_TABLE_AS_RECORD_ARRAY_SIZE;
    SQLINTEGER   row_size = sizeof(Record);
    Record       record0[CREATE_TABLE_AS_RECORD_ARRAY_SIZE];
    Record       record[CREATE_TABLE_AS_RECORD_ARRAY_SIZE];

    ODBC_Connect(&henv1, &hdbc1, &hstmt1);
    
    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_statment_values");
    OK_SIMPLE_STMT(hstmt1, "CREATE TABLE test_statment_values (id INTEGER, name CHAR(20))");
    
    /* first prepare data */
    for (int i = 0; i < rowcnt; i++)
    {
        record0[i].id = i;
        rc = _snprintf_s(record0[i].name, sizeof(record0[i].name), sizeof(record0[i].name) - 1, "value%d", i);
    }

    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)rowcnt, 0));
    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_ROW_BIND_TYPE, (SQLPOINTER)sizeof(Record), 0));
    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_ROWS_FETCHED_PTR, &rowsFetchedPtr, 0));

    OK_SIMPLE_STMT(hstmt1, "SELECT * FROM (VALUES (0, 'value0'),(1, 'value1'),(2, 'value2')) AS t(id, name)");

    CHECK_STMT_RC(hstmt1, SQLBindCol(hstmt1, 1, SQL_C_LONG, &record[0].id, sizeof(record[0].id), &record[0].idLen));
    CHECK_STMT_RC(hstmt1, SQLBindCol(hstmt1, 2, SQL_C_CHAR, record[0].name, sizeof(record[0].name), &record[0].nameLen));
    
    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    /* check the result */
    is_num(rowcnt, rowsFetchedPtr);
    for (int i = 0; i < rowcnt; i++)
    {
        is_num(record[i].id, record0[i].id);
        IS_STR(record[i].name, record0[i].name, strlen(record0[i].name));
    }

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_statment_values");
    
    ODBC_Disconnect(henv1, hdbc1, hstmt1);

#undef CREATE_TABLE_AS_RECORD_NAME_LEN
#undef CREATE_TABLE_AS_RECORD_ARRAY_SIZE

    return OK;
}

/* test show catalogs */
ODBC_TEST(test_show_catalogs)
{
#define SHOW_CATALOGS_RECORD_NAME_LEN    512
    SQLHANDLE   henv1;
    SQLHANDLE   hdbc1;
    SQLHANDLE   hstmt1;
    SQLULEN     rowsFetchedPtr = 0;

    SQLCHAR     record[SHOW_CATALOGS_RECORD_NAME_LEN];

    ODBC_Connect(&henv1, &hdbc1, &hstmt1);
    
    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "SHOW CATALOGS", SQL_NTS));

    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    diag("catalog lists:");
    do {
        diag("%s", my_fetch_str(hstmt1, record, 1));
    } while (SQLFetch(hstmt1) != SQL_NO_DATA);
    
    ODBC_Disconnect(henv1, hdbc1, hstmt1);

#undef SHOW_CATALOGS_RECORD_NAME_LEN

    return OK;
}

/* test schemas operation */
ODBC_TEST(test_schemas_operation)
{
#define SCHEMAS_OPERATION_RECORD_NAME_LEN    512    
    SQLHANDLE   henv1;
    SQLHANDLE   hdbc1;
    SQLHANDLE   hstmt1;
    SQLULEN     rowsFetchedPtr = 0;
    SQLLEN      nLen;

    SQLCHAR     record[SCHEMAS_OPERATION_RECORD_NAME_LEN];
    SQLSMALLINT findFlag = 0;
    char       *schemaName = "my_schemas_operation_schema";

    ODBC_Connect(&henv1, &hdbc1, &hstmt1);

    /* first drop exist schema with same name */
    OK_SIMPLE_STMT(hstmt1, "DROP SCHEMA IF EXISTS my_schemas_operation_schema");

    /* create schema and show all schema */
    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "CREATE SCHEMA my_schemas_operation_schema", SQL_NTS));
    
    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "SHOW SCHEMAS", SQL_NTS));

    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    /* search for my schema from show command */
    do {
        CHECK_STMT_RC(hstmt1, SQLGetData(hstmt1, 1, SQL_CHAR, record, sizeof(record), &nLen));
        if (strncmp(record, schemaName, strlen(schemaName)) == 0) {
            findFlag = 1;
        }
    } while (SQLFetch(hstmt1) != SQL_NO_DATA);

    IS(findFlag == 1);

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));
    
    OK_SIMPLE_STMT(hstmt1, "DROP SCHEMA IF EXISTS my_schemas_operation_schema");
    
    ODBC_Disconnect(henv1, hdbc1, hstmt1);

#undef SCHEMAS_OPERATION_RECORD_NAME_LEN

    return OK;
}

ODBC_TEST(test_table_operation)
{
#define TABLE_OPERATION_RECORD_NAME_LEN    512
    SQLHANDLE   henv1;
    SQLHANDLE   hdbc1;
    SQLHANDLE   hstmt1;
    SQLULEN     rowsFetchedPtr = 0;
    SQLLEN      nLen;

    SQLCHAR     record[TABLE_OPERATION_RECORD_NAME_LEN];
    SQLSMALLINT findFlag = 0;

    ODBC_Connect(&henv1, &hdbc1, &hstmt1);

    /* first drop exist table with same name */
    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_tables_operation_table");

    /* create table and show all tables */
    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "CREATE TABLE test_tables_operation_table(id INTEGER)", SQL_NTS));
    
    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "SHOW TABLES", SQL_NTS));

    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    /* search for my table from show command */
    do {
        CHECK_STMT_RC(hstmt1, SQLGetData(hstmt1, 1, SQL_CHAR, record, sizeof(record), &nLen));
        if (strncmp(record, "test_tables_operation_table", strlen("test_tables_operation_table")) == 0) {
            findFlag = 1;
        }
    } while (SQLFetch(hstmt1) != SQL_NO_DATA);

    IS(findFlag == 1);

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_tables_operation_table");
    
    ODBC_Disconnect(henv1, hdbc1, hstmt1);

#undef TABLE_OPERATION_RECORD_NAME_LEN

    return OK;
}

/* test show columns */
ODBC_TEST(test_show_columns)
{
#define SHOW_COLUMNS_RECORD_NAME_LEN    256
#define SHOW_COLUMNS_ARRAY_SIZE         4
    SQLHANDLE   henv1;
    SQLHANDLE   hdbc1;
    SQLHANDLE   hstmt1;
    SQLULEN     rowsFetchedPtr = 0;
    SQLULEN     nLen;

    typedef struct {
        SQLCHAR    columnName[SHOW_COLUMNS_RECORD_NAME_LEN];
        SQLCHAR    type[SHOW_COLUMNS_RECORD_NAME_LEN];
    }COLUMN_INFO;
    
    COLUMN_INFO     record0[SHOW_COLUMNS_RECORD_NAME_LEN] = 
    {
        {"id", "integer"},
        {"name", "char(20)"},
        {"info", "varchar(50)"},
        {"value", "double"},
    };
    COLUMN_INFO     record;

    ODBC_Connect(&henv1, &hdbc1, &hstmt1);

    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_show_columns");
    OK_SIMPLE_STMT(hstmt1, "CREATE TABLE test_show_columns (id INTEGER, name char(20), info varchar(50), value double)");
        
    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "SHOW COLUMNS FROM test_show_columns", SQL_NTS));

    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    diag("columns lists:");
    int i = 0;
    do {
        CHECK_STMT_RC(hstmt1, SQLGetData(hstmt1, 1, SQL_CHAR, record.columnName, sizeof(record.columnName), &nLen));
        CHECK_STMT_RC(hstmt1, SQLGetData(hstmt1, 2, SQL_CHAR, record.type, sizeof(record.type), &nLen));
        IS_STR(record.columnName, record0[i].columnName, strlen(record0[i].columnName));
        IS_STR(record.type, record0[i].type, strlen(record0[i].type));
        i++;
    } while (SQLFetch(hstmt1) != SQL_NO_DATA);

    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_show_columns");
    
    ODBC_Disconnect(henv1, hdbc1, hstmt1);

#undef SHOW_COLUMNS_RECORD_NAME_LEN

    return OK;
}

/* test show create tables */
ODBC_TEST(test_show_create_table)
{
    SQLHANDLE   henv1;
    SQLHANDLE   hdbc1;
    SQLHANDLE   hstmt1;

    ODBC_Connect(&henv1, &hdbc1, &hstmt1);

    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_show_create_table");
    OK_SIMPLE_STMT(hstmt1, "create table test_show_create_table (id INTEGER, name CHAR(20))");
        
    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "SHOW CREATE TABLE test_show_create_table", SQL_NTS));

    /* check if there is a response */
    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_show_create_table");
    
    ODBC_Disconnect(henv1, hdbc1, hstmt1);

    return OK;
}

ODBC_TEST(test_show_stats)
{
#define SHOW_STATS_RECORD_NAME_LEN    512
    SQLHANDLE   henv1;
    SQLHANDLE   hdbc1;
    SQLHANDLE   hstmt1;

    SQLCHAR     record[SHOW_STATS_RECORD_NAME_LEN];

    ODBC_Connect(&henv1, &hdbc1, &hstmt1);

    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_show_stats");
    OK_SIMPLE_STMT(hstmt1, "create table test_show_stats (id INTEGER, name CHAR(20))");
    
    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "SHOW STATS for test_show_stats", SQL_NTS));

    /* expect not null response */
    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    do {
        my_fetch_str(hstmt1, record, 1);
    } while (SQLFetch(hstmt1) != SQL_NO_DATA);

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_show_stats");

    ODBC_Disconnect(henv1, hdbc1, hstmt1);

#undef SHOW_STATS_RECORD_NAME_LEN

    return OK;
}

ODBC_TEST(test_session_operation)
{
#define SHOW_SESSION_RECORD_NAME_LEN    512
    SQLHANDLE   henv1;
    SQLHANDLE   hdbc1;
    SQLHANDLE   hstmt1;
    SQLLEN      nLen;

    SQLCHAR     record[SHOW_SESSION_RECORD_NAME_LEN];

    ODBC_Connect(&henv1, &hdbc1, &hstmt1);

    /* test set session property */
    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "SET SESSION optimize_hash_generation = true", SQL_NTS));
    
    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "SHOW SESSION", SQL_NTS));

    /* expect not null response */
    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    do {
        CHECK_STMT_RC(hstmt1, SQLGetData(hstmt1, 1, SQL_CHAR, record, sizeof(record), &nLen));
    } while (SQLFetch(hstmt1) != SQL_NO_DATA);

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

    /* expect command(RESET SESSION) execute ok */
    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "RESET SESSION optimize_hash_generation", SQL_NTS));

    ODBC_Disconnect(henv1, hdbc1, hstmt1);

#undef SHOW_SESSION_RECORD_NAME_LEN

    return OK;
}

/* test view operation: create/show record/show create info/drop view */
ODBC_TEST(test_view_operation)
{
#define    TEST_VIEW_RECORD_NAME_LEN      20
#define    TEST_VIEW_RRECORD_ARRAY_SIZE   3
    SQLHANDLE   henv1;
    SQLHANDLE   hdbc1;
    SQLHANDLE   hstmt1;
    int         rc;

    typedef struct {
        SQLINTEGER id;
        SQLCHAR    name[TEST_VIEW_RECORD_NAME_LEN + 1];
        SQLLEN     idLen;
        SQLLEN     nameLen;
    }Record;

    SQLULEN      rowsFetchedPtr = 0;
    Record       record0[TEST_VIEW_RRECORD_ARRAY_SIZE];
    Record       record[TEST_VIEW_RRECORD_ARRAY_SIZE];

    ODBC_Connect(&henv1, &hdbc1, &hstmt1);

    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_view_operation");
    OK_SIMPLE_STMT(hstmt1, "create table test_view_operation (id INTEGER, name CHAR(20))");

    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "INSERT INTO test_view_operation VALUES (0,'name0'),(1,'name1'),(2,'name2')", SQL_NTS));
    for (int i = 0; i < TEST_VIEW_RRECORD_ARRAY_SIZE; i++)
    {
        record0[i].id      = i;
        rc = _snprintf_s(record0[i].name, sizeof(record0[i].name), sizeof(record0[i].name) - 1, "name%d", i);
    }

    /* create view from table */
    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "CREATE VIEW my_view_operation AS SELECT id, name "
                  "FROM test_view_operation ORDER BY id", SQL_NTS));

    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)TEST_VIEW_RRECORD_ARRAY_SIZE, 0));
    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_ROW_BIND_TYPE, (SQLPOINTER)sizeof(Record), 0));
    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_ROWS_FETCHED_PTR, &rowsFetchedPtr, 0));

    /* search view */
    OK_SIMPLE_STMT(hstmt1, "SELECT * FROM my_view_operation ORDER BY id");

    CHECK_STMT_RC(hstmt1, SQLBindCol(hstmt1, 1, SQL_C_LONG, &record[0].id, sizeof(record[0].id), &record[0].idLen));
    CHECK_STMT_RC(hstmt1, SQLBindCol(hstmt1, 2, SQL_C_CHAR, record[0].name, sizeof(record[0].name), &record[0].nameLen));
    
    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    is_num(TEST_VIEW_RRECORD_ARRAY_SIZE, rowsFetchedPtr);
    for (int i = 0; i < TEST_VIEW_RRECORD_ARRAY_SIZE; i++)
    {
        is_num(record[i].id, record0[i].id);
        IS_STR(record[i].name, record0[i].name, strlen(record0[i].name));
    }

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

    /* check create view info */
    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "SHOW CREATE VIEW my_view_operation", SQL_NTS));

    /* check if there is a response */
    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

    OK_SIMPLE_STMT(hstmt1, "DROP VIEW IF EXISTS my_view_operation");
    
    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_view_operation");
    
    ODBC_Disconnect(henv1, hdbc1, hstmt1);

    return OK;
}

/* test show functions */
ODBC_TEST(test_show_functions)
{
#define SHOW_FUNCTIONS_RECORD_LEN    256
    SQLHANDLE   henv1;
    SQLHANDLE   hdbc1;
    SQLHANDLE   hstmt1;
    SQLCHAR     record[SHOW_FUNCTIONS_RECORD_LEN];
    int         max_show_num = 3;

    ODBC_Connect(&henv1, &hdbc1, &hstmt1);

    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "SHOW FUNCTIONS", SQL_NTS));

    /* check if there is a response */
    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    diag("functions lists:");
    int i = 0;
    do {
        my_fetch_str(hstmt1, record, 1);
        i++;
        // totally to many items, we don't need to show all
        if (i >= max_show_num) {
            break;
        }
        
    } while (SQLFetch(hstmt1) != SQL_NO_DATA);
    
    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

    ODBC_Disconnect(henv1, hdbc1, hstmt1);

#undef SHOW_FUNCTIONS_RECORD_LEN
    return OK;
}

ODBC_TEST(test_trans_command_operation)
{
#define TRANS_OPERATION_RECORD_ARRAY_SIZE  5
    SQLHANDLE   henv0;
    SQLHANDLE   hdbc0;
    SQLHANDLE   hstmt0;
    SQLHANDLE   henv1;
    SQLHANDLE   hdbc1;
    SQLHANDLE   hstmt1;
    SQLULEN     rowsFetchedPtr = 0;

    typedef struct {
        SQLINTEGER id;
        SQLINTEGER value;
        SQLLEN     idLen;
        SQLLEN     valueLen;
    }Record;
    SQLULEN      rowcnt = TRANS_OPERATION_RECORD_ARRAY_SIZE - 2;
    SQLINTEGER   row_size = sizeof(Record);
    Record       record[TRANS_OPERATION_RECORD_ARRAY_SIZE];
    Record       record1[TRANS_OPERATION_RECORD_ARRAY_SIZE];
    SQLUINTEGER  mode = 0;

    ODBC_Connect(&henv0, &hdbc0, &hstmt0);
    ODBC_Connect(&henv1, &hdbc1, &hstmt1);

    for (int i = 0; i < TRANS_OPERATION_RECORD_ARRAY_SIZE; i++)
    {
        record[i].id      = i;
        record[i].value   = i * 10;
    }

    OK_SIMPLE_STMT(hstmt0, "DROP TABLE IF EXISTS test_trans_command_operation");
    OK_SIMPLE_STMT(hstmt0, "CREATE TABLE test_trans_command_operation (id INTEGER, myValue INTEGER)");

    /* switch to manual commit mode */
    CHECK_DBC_RC(hdbc0, SQLSetConnectAttr(hdbc0, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0));
    CHECK_DBC_RC(hdbc0, SQLGetConnectAttr(hdbc0, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)&mode, 0, NULL));
    is_num(mode, SQL_AUTOCOMMIT_OFF);

    /* begin normal start transaction and commit */
    OK_SIMPLE_STMT(hstmt0, "START TRANSACTION");
    
    FAIL_IF(SQLExecDirect(hstmt0, "INSERT INTO test_trans_command_operation (id, myValue) VALUES (0, 0),(1, 10),(2, 20)", SQL_NTS) != SQL_SUCCESS, "success expected");

    CHECK_STMT_RC(hstmt0, SQLFreeStmt(hstmt0, SQL_RESET_PARAMS));

    OK_SIMPLE_STMT(hstmt0, "COMMIT");

    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)rowcnt, 0));
    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_ROW_BIND_TYPE, (SQLPOINTER)sizeof(Record), 0));
    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_ROWS_FETCHED_PTR, &rowsFetchedPtr, 0));
    
    OK_SIMPLE_STMT(hstmt1, "SELECT * FROM test_trans_command_operation ORDER BY id");

    CHECK_STMT_RC(hstmt1, SQLBindCol(hstmt1, 1, SQL_C_LONG, &record1[0].id, sizeof(record1[0].id), &record1[0].idLen));
    CHECK_STMT_RC(hstmt1, SQLBindCol(hstmt1, 2, SQL_C_LONG, &record1[0].value, sizeof(record1[0].value), &record1[0].valueLen));

    OK_SIMPLE_STMT(hstmt1, "SELECT * FROM test_trans_command_operation ORDER BY id");
    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    is_num(rowcnt, rowsFetchedPtr);
    for (int i = 0; i < rowcnt; i++)
    {
        is_num(record1[i].id, record[i].id);
        is_num(record1[i].value, record[i].value);
    }

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

    /* now test rollback command */
    OK_SIMPLE_STMT(hstmt0, "START TRANSACTION");
    
    FAIL_IF(SQLExecDirect(hstmt0, "INSERT INTO test_trans_command_operation (id, myValue) VALUES (3, 30),(4, 40)", SQL_NTS) != SQL_SUCCESS, "success expected");

    CHECK_STMT_RC(hstmt0, SQLFreeStmt(hstmt0, SQL_RESET_PARAMS));

    OK_SIMPLE_STMT(hstmt0, "ROLLBACK");

    OK_SIMPLE_STMT(hstmt1, "SELECT * FROM test_trans_command_operation ORDER BY id");
    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    /* trans rollback, total records do not change */
    is_num(rowcnt, rowsFetchedPtr);
    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));
    
    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_trans_command_operation");

    ODBC_Disconnect(henv0, hdbc0, hstmt0);
    ODBC_Disconnect(henv1, hdbc1, hstmt1);

#undef TRANS_OPERATION_RECORD_ARRAY_SIZE

    return OK;
}

ODBC_TEST(test_insert_by_select)
{
#define INSERT_BY_SELECT_RECORD_BUFFER_SIZE  20
#define INSERT_BY_SELECT_RECORD_ARRAY_SIZE   5
    SQLHANDLE   henv0;
    SQLHANDLE   hdbc0;
    SQLHANDLE   hstmt0;
    SQLULEN     rowsFetchedPtr = 0;
    int         rc;

    typedef struct {
        SQLINTEGER id;
        SQLCHAR    value[INSERT_BY_SELECT_RECORD_BUFFER_SIZE + 1];
        SQLLEN     idLen;
        SQLLEN     valueLen;
    }Record;

    SQLULEN      rowcnt = INSERT_BY_SELECT_RECORD_ARRAY_SIZE;
    SQLINTEGER   row_size = sizeof(Record);
    Record       record[INSERT_BY_SELECT_RECORD_ARRAY_SIZE];
    Record       record1[INSERT_BY_SELECT_RECORD_ARRAY_SIZE];

    ODBC_Connect(&henv0, &hdbc0, &hstmt0);

    for (int i = 0; i < INSERT_BY_SELECT_RECORD_ARRAY_SIZE; i++)
    {
        record[i].id      = i;
        rc = _snprintf_s(record[i].value, sizeof(record[i].value), sizeof(record[i].value) - 1, "value%d", i);
    }

    OK_SIMPLE_STMT(hstmt0, "DROP TABLE IF EXISTS test_insert_by_select0");
    OK_SIMPLE_STMT(hstmt0, "DROP TABLE IF EXISTS test_insert_by_select");
    OK_SIMPLE_STMT(hstmt0, "CREATE TABLE test_insert_by_select0 (id INTEGER, myValue CHAR(20))");
    OK_SIMPLE_STMT(hstmt0, "CREATE TABLE test_insert_by_select (id INTEGER, myValue CHAR(20))");
    
    FAIL_IF(SQLExecDirect(hstmt0, "INSERT INTO test_insert_by_select0 (id, myValue) VALUES "
            "(0, 'value0'),(1, 'value1'),(2, 'value2'),(3, 'value3'),(4, 'value4')", SQL_NTS) != SQL_SUCCESS, "success expected");

    FAIL_IF(SQLExecDirect(hstmt0, "INSERT INTO test_insert_by_select "
            "SELECT * FROM test_insert_by_select0", SQL_NTS) != SQL_SUCCESS, "success expected");
    
    CHECK_STMT_RC(hstmt0, SQLFreeStmt(hstmt0, SQL_RESET_PARAMS));

    CHECK_STMT_RC(hstmt0, SQLSetStmtAttr(hstmt0, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)rowcnt, 0));
    CHECK_STMT_RC(hstmt0, SQLSetStmtAttr(hstmt0, SQL_ATTR_ROW_BIND_TYPE, (SQLPOINTER)sizeof(Record), 0));
    CHECK_STMT_RC(hstmt0, SQLSetStmtAttr(hstmt0, SQL_ATTR_ROWS_FETCHED_PTR, &rowsFetchedPtr, 0));
    
    CHECK_STMT_RC(hstmt0, SQLBindCol(hstmt0, 1, SQL_C_LONG, &record1[0].id, sizeof(record1[0].id), &record1[0].idLen));
    CHECK_STMT_RC(hstmt0, SQLBindCol(hstmt0, 2, SQL_C_CHAR, &record1[0].value, sizeof(record1[0].value), &record1[0].valueLen));

    OK_SIMPLE_STMT(hstmt0, "SELECT * FROM test_insert_by_select ORDER BY id");
    CHECK_STMT_RC(hstmt0, SQLFetch(hstmt0));

    is_num(rowcnt, rowsFetchedPtr);
    for (int i = 0; i < rowcnt; i++)
    {
        is_num(record1[i].id, record[i].id);
        IS_STR(record1[i].value, record[i].value, strlen(record[i].value));
    }

    CHECK_STMT_RC(hstmt0, SQLFreeStmt(hstmt0, SQL_CLOSE));
    OK_SIMPLE_STMT(hstmt0, "DROP TABLE IF EXISTS test_insert_by_select0");
    OK_SIMPLE_STMT(hstmt0, "DROP TABLE IF EXISTS test_insert_by_select");

    ODBC_Disconnect(henv0, hdbc0, hstmt0);

#undef INSERT_BY_SELECT_RECORD_ARRAY_SIZE

    return OK;
}

ODBC_TEST(test_use_command)
{
#define USE_COMMAND_RECORD_NAME_LEN    512
    SQLHANDLE   henv1;
    SQLHANDLE   hdbc1;
    SQLHANDLE   hstmt1;

    SQLCHAR     record[USE_COMMAND_RECORD_NAME_LEN];

    ODBC_Connect(&henv1, &hdbc1, &hstmt1);

    /* first switch to system.information_schema */
    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "USE system.information_schema", SQL_NTS));

    /* there must exist a table named tables, show its record  */
    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "SELECT * from tables", SQL_NTS));

    /* expect non-empty response */
    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    /* try to get one table name and expect non-null string */
    FAIL_IF(my_fetch_str(hstmt1, record, 3) == NULL, "expect a valid table name!");

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

    ODBC_Disconnect(henv1, hdbc1, hstmt1);

#undef USE_COMMAND_RECORD_NAME_LEN

    return OK;
}

/* test basic create table as */
ODBC_TEST(test_create_table_as)
{
#define CREATE_TABLE_AS_RECORD_NAME_LEN    20
#define CREATE_TABLE_AS_RECORD_ARRAY_SIZE  5
    SQLHANDLE   henv1;
    SQLHANDLE   hdbc1;
    SQLHANDLE   hstmt1;
    SQLULEN     rowsFetchedPtr = 0;
    SQLRETURN   rc;

    typedef struct {
        SQLINTEGER id;
        SQLCHAR name[CREATE_TABLE_AS_RECORD_NAME_LEN + 1];
        SQLLEN idLen;
        SQLLEN nameLen;
    }Record;
    SQLULEN      rowcnt = CREATE_TABLE_AS_RECORD_ARRAY_SIZE;
    SQLINTEGER   row_size = sizeof(Record);
    Record       record0[CREATE_TABLE_AS_RECORD_ARRAY_SIZE];
    Record       record[CREATE_TABLE_AS_RECORD_ARRAY_SIZE];

    ODBC_Connect(&henv1, &hdbc1, &hstmt1);
    
    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_create_table_as0");
    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_create_table_as");
    OK_SIMPLE_STMT(hstmt1, "CREATE TABLE test_create_table_as0 (id INTEGER, name CHAR(20))");
    
    /* first prepare data */
    for (int i = 0; i < rowcnt; i++)
    {
        record0[i].id = i;
        rc = _snprintf_s(record0[i].name, sizeof(record0[i].name), sizeof(record0[i].name) - 1, "value%d", i);
    }
    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "INSERT INTO test_create_table_as0 (id, name) VALUES (0, 'value0'),"
                  "(1, 'value1'),(2, 'value2'),(3, 'value3'),(4, 'value4')", SQL_NTS));

    /* create a new table and insert records from an existing table */
    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "CREATE TABLE test_create_table_as AS SELECT * FROM test_create_table_as0", SQL_NTS));

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)rowcnt, 0));
    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_ROW_BIND_TYPE, (SQLPOINTER)sizeof(Record), 0));
    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_ROWS_FETCHED_PTR, &rowsFetchedPtr, 0));

    OK_SIMPLE_STMT(hstmt1, "SELECT * FROM test_create_table_as ORDER BY id");

    CHECK_STMT_RC(hstmt1, SQLBindCol(hstmt1, 1, SQL_C_LONG, &record[0].id, sizeof(record[0].id), &record[0].idLen));
    CHECK_STMT_RC(hstmt1, SQLBindCol(hstmt1, 2, SQL_C_CHAR, record[0].name, sizeof(record[0].name), &record[0].nameLen));
    
    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    /* check the result */
    is_num(rowcnt, rowsFetchedPtr);
    for (int i = 0; i < rowcnt; i++)
    {
        is_num(record[i].id, record0[i].id);
        IS_STR(record[i].name, record0[i].name, strlen(record0[i].name));
    }

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_create_table_as0");
    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_create_table_as");
    
    ODBC_Disconnect(henv1, hdbc1, hstmt1);

#undef CREATE_TABLE_AS_RECORD_NAME_LEN
#undef CREATE_TABLE_AS_RECORD_ARRAY_SIZE

    return OK;
}

ODBC_TEST(test_analyze_operation)
{
    SQLHANDLE   henv1;
    SQLHANDLE   hdbc1;
    SQLHANDLE   hstmt1;

    ODBC_Connect(&henv1, &hdbc1, &hstmt1);

    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_analyze_operation");
    OK_SIMPLE_STMT(hstmt1, "create table test_analyze_operation (id INTEGER, name CHAR(20))");

    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "INSERT INTO test_analyze_operation VALUES(0,'value0')", SQL_NTS));
    
    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "ANALYZE test_analyze_operation", SQL_NTS));

    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "DESCRIBE test_analyze_operation", SQL_NTS));

    /* expect not null response */
    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_analyze_operation");

    ODBC_Disconnect(henv1, hdbc1, hstmt1);

    return OK;
}

/* test alert table */
// Access Denied: Cannot rename table from test.test_alert_table0 to test.test_alert_table
ODBC_TEST(test_alert_table)
{
#define ALERT_TABLE_RECORD_NAME_LEN    20
#define ALERT_TABLE_RECORD_ARRAY_SIZE  5
#define ROLE_OPERTATE_STATE_CODE_LEN     6
#define ALERT_TABLE_EXPECT_RETURN_NOT_SUPPORT_INFO(hstmt,expect) \
    do {\
    SQLCHAR SQLState[ROLE_OPERTATE_STATE_CODE_LEN];\
    SQLINTEGER NativeError;\
    SQLCHAR SQLMessage[SQL_MAX_MESSAGE_LENGTH];\
    SQLSMALLINT TextLengthPtr;\
    CHECK_STMT_RC(hstmt, SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, SQLState, &NativeError, \
                  SQLMessage, SQL_MAX_MESSAGE_LENGTH, &TextLengthPtr));\
        if (strstr(SQLMessage, expect) == NULL)\
        {\
            fprintf(stdout, "[%s] (%d) %s\n", SQLState, NativeError, SQLMessage);\
            return FAIL;\
        }\
    } while(0);
    
    SQLHANDLE   henv1;
    SQLHANDLE   hdbc1;
    SQLHANDLE   hstmt1;
    SQLULEN     rowsFetchedPtr = 0;
    SQLRETURN   rc;

    typedef struct {
        SQLINTEGER id;
        SQLCHAR name[ALERT_TABLE_RECORD_NAME_LEN + 1];
        SQLINTEGER value;
        SQLLEN idLen;
        SQLLEN nameLen;
        SQLLEN valueLen;
    }Record;
    SQLULEN      rowcnt = ALERT_TABLE_RECORD_ARRAY_SIZE;
    SQLINTEGER   row_size = sizeof(Record);
    Record       record0[ALERT_TABLE_RECORD_ARRAY_SIZE];
    Record       record[ALERT_TABLE_RECORD_ARRAY_SIZE];

    ODBC_Connect(&henv1, &hdbc1, &hstmt1);
    
    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_alert_table");
    OK_SIMPLE_STMT(hstmt1, "CREATE TABLE test_alert_table (id INTEGER, name CHAR(20)) WITH (transactional=true)");
    
    /* first prepare data */
    for (int i = 0; i < rowcnt; i++)
    {
        record0[i].id = i;
        rc = _snprintf_s(record0[i].name, sizeof(record0[i].name), sizeof(record0[i].name) - 1, "name%d", i);
    }

    rc = SQLExecDirect(hstmt1, "ALTER TABLE test_alert_table ADD COLUMN value INTEGER", SQL_NTS);
    if (rc != SQL_SUCCESS)
    {
        ALERT_TABLE_EXPECT_RETURN_NOT_SUPPORT_INFO(hstmt1, "Access Denied");
        CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));
        ODBC_Disconnect(henv1, hdbc1, hstmt1);
        diag("Has no right to alert table, so terminate test.");
        return OK;
    }

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)rowcnt, 0));
    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_ROW_BIND_TYPE, (SQLPOINTER)sizeof(Record), 0));
    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_ROWS_FETCHED_PTR, &rowsFetchedPtr, 0));

    OK_SIMPLE_STMT(hstmt1, "SELECT * FROM test_alert_table ORDER BY id");

    CHECK_STMT_RC(hstmt1, SQLBindCol(hstmt1, 1, SQL_C_LONG, &record[0].id, sizeof(record[0].id), &record[0].idLen));
    CHECK_STMT_RC(hstmt1, SQLBindCol(hstmt1, 2, SQL_C_CHAR, record[0].name, sizeof(record[0].name), &record[0].nameLen));
    CHECK_STMT_RC(hstmt1, SQLBindCol(hstmt1, 3, SQL_C_LONG, &record[0].value, sizeof(record[0].value), &record[0].valueLen));
    
    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    /* check the result */
    is_num(rowcnt, rowsFetchedPtr);
    for (int i = 0; i < rowcnt; i++)
    {
        is_num(record[i].id, record0[i].id);
        IS_STR(record[i].name, record0[i].name, strlen(record0[i].name));
    }

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_alert_table");
    
    ODBC_Disconnect(henv1, hdbc1, hstmt1);

#undef ALERT_TABLE_RECORD_NAME_LEN
#undef ALERT_TABLE_RECORD_ARRAY_SIZE
#undef ALERT_TABLE_EXPECT_RETURN_NOT_SUPPORT_INFO

    return OK;
}

/* simple update test
   NOTE: requires that the server data source support update operation
*/
ODBC_TEST(test_update_data)
{
#define UPDATE_DATA_RECORD_ARRAY_SIZE  3
    SQLHANDLE   henv0;
    SQLHANDLE   hdbc0;
    SQLHANDLE   hstmt0;
    SQLHANDLE   henv1;
    SQLHANDLE   hdbc1;
    SQLHANDLE   hstmt1;
    SQLULEN     rowsFetchedPtr = 0;

    typedef struct {
        SQLINTEGER id;
        SQLINTEGER value;
        SQLLEN     idLen;
        SQLLEN     valueLen;
    }Record;
    SQLULEN      rowcnt = UPDATE_DATA_RECORD_ARRAY_SIZE;
    SQLINTEGER   row_size = sizeof(Record);
    Record       record[UPDATE_DATA_RECORD_ARRAY_SIZE];
    Record       record1[UPDATE_DATA_RECORD_ARRAY_SIZE];
    SQLUINTEGER  mode = 0;
    SQLRETURN    ret;

    ODBC_Connect(&henv0, &hdbc0, &hstmt0);
    ODBC_Connect(&henv1, &hdbc1, &hstmt1);

    for (int i = 0; i < UPDATE_DATA_RECORD_ARRAY_SIZE; i++)
    {
        record[i].id      = i;
        record[i].value   = i * 10 + 5;
    }

    OK_SIMPLE_STMT(hstmt0, "DROP TABLE IF EXISTS test_update_data");
    OK_SIMPLE_STMT(hstmt0, "CREATE TABLE test_update_data (id INTEGER, myValue INTEGER) WITH (transactional=true)");

    /* insert to table, each value equals (id * 10 + 5) except the row value of id==1 */
    ret = SQLExecDirect(hstmt0, "INSERT INTO test_update_data (id, myValue) VALUES (0, 5),(1, 10),(2, 25)", SQL_NTS);
    if (ret != SQL_SUCCESS) {
        diag("Data source does not support trans, skip update data.");
        ODBC_Disconnect(henv0, hdbc0, hstmt0);
        ODBC_Disconnect(henv1, hdbc1, hstmt1);
        return OK;
    }

    CHECK_STMT_RC(hstmt0, SQLFreeStmt(hstmt0, SQL_RESET_PARAMS));

    /* switch to manual commit mode */
    CHECK_DBC_RC(hdbc0, SQLSetConnectAttr(hdbc0, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0));
    CHECK_DBC_RC(hdbc0, SQLGetConnectAttr(hdbc0, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)&mode, 0, NULL));
    is_num(mode, SQL_AUTOCOMMIT_OFF);

    /* update the value of id==1 */
    CHECK_STMT_RC(hstmt0, SQLExecDirect(hstmt0, "UPDATE test_update_data SET myValue=15 where id=1", SQL_NTS));

    CHECK_DBC_RC(hdbc0, SQLEndTran(SQL_HANDLE_DBC, hdbc0, SQL_COMMIT));
    
    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)rowcnt, 0));
    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_ROW_BIND_TYPE, (SQLPOINTER)sizeof(Record), 0));
    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_ROWS_FETCHED_PTR, &rowsFetchedPtr, 0));
    
    OK_SIMPLE_STMT(hstmt1, "SELECT * FROM test_update_data ORDER BY id");

    CHECK_STMT_RC(hstmt1, SQLBindCol(hstmt1, 1, SQL_C_LONG, &record1[0].id, sizeof(record1[0].id), &record1[0].idLen));
    CHECK_STMT_RC(hstmt1, SQLBindCol(hstmt1, 2, SQL_C_LONG, &record1[0].value, sizeof(record1[0].value), &record1[0].valueLen));

    OK_SIMPLE_STMT(hstmt1, "SELECT * FROM test_update_data ORDER BY id");
    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    /* check update result */
    is_num(rowcnt, rowsFetchedPtr);
    for (int i = 0; i < rowcnt; i++)
    {
        is_num(record1[i].id, record[i].id);
        is_num(record1[i].value, record[i].value);
    }

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));
    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_update_data");

    ODBC_Disconnect(henv0, hdbc0, hstmt0);
    ODBC_Disconnect(henv1, hdbc1, hstmt1);

#undef UPDATE_DATA_RECORD_ARRAY_SIZE

    return OK;
}

/* simple delete row test 
NOTE: requires that the server data source support delete operation
*/
ODBC_TEST(test_delete_data)
{
#define DELETE_DATA_RECORD_ARRAY_SIZE  6
    SQLHANDLE   henv0;
    SQLHANDLE   hdbc0;
    SQLHANDLE   hstmt0;
    SQLHANDLE   henv1;
    SQLHANDLE   hdbc1;
    SQLHANDLE   hstmt1;
    SQLULEN     rowsFetchedPtr = 0;

    typedef struct {
        SQLINTEGER id;
        SQLINTEGER value;
        SQLLEN     idLen;
        SQLLEN     valueLen;
    }Record;
    SQLULEN      rowcnt = DELETE_DATA_RECORD_ARRAY_SIZE;
    SQLINTEGER   row_size = sizeof(Record);
    Record       record[DELETE_DATA_RECORD_ARRAY_SIZE];
    Record       record1[DELETE_DATA_RECORD_ARRAY_SIZE];
    SQLUINTEGER  mode = 0;
    SQLRETURN    ret;

    ODBC_Connect(&henv0, &hdbc0, &hstmt0);
    ODBC_Connect(&henv1, &hdbc1, &hstmt1);

    for (int i = 0; i < DELETE_DATA_RECORD_ARRAY_SIZE; i++)
    {
        record[i].id      = i;
        record[i].value   = i * 10;
    }

    OK_SIMPLE_STMT(hstmt0, "DROP TABLE IF EXISTS test_delete_data");
    OK_SIMPLE_STMT(hstmt0, "CREATE TABLE test_delete_data (id INTEGER, myValue INTEGER) WITH (transactional=true)");
    
    ret = SQLExecDirect(hstmt0, "INSERT INTO test_delete_data (id, myValue) VALUES (0, 0),(1, 10),(2, 20),(3, 30),(4, 40),(5, 50)", SQL_NTS);
    if (ret != SQL_SUCCESS) {
        diag("Data source does not support trans, skip delete data.");
        ODBC_Disconnect(henv0, hdbc0, hstmt0);
        ODBC_Disconnect(henv1, hdbc1, hstmt1);
        return OK;
    }
    
    CHECK_STMT_RC(hstmt0, SQLFreeStmt(hstmt0, SQL_RESET_PARAMS));

    /* now delete the records where id equals times of 2 */
    CHECK_STMT_RC(hstmt0, SQLExecDirect(hstmt0, "DELETE FROM test_delete_data WHERE id%2=0", SQL_NTS));
    
    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)rowcnt, 0));
    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_ROW_BIND_TYPE, (SQLPOINTER)sizeof(Record), 0));
    CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_ROWS_FETCHED_PTR, &rowsFetchedPtr, 0));
    
    OK_SIMPLE_STMT(hstmt1, "SELECT * FROM test_delete_data ORDER BY id");

    CHECK_STMT_RC(hstmt1, SQLBindCol(hstmt1, 1, SQL_C_LONG, &record1[0].id, sizeof(record1[0].id), &record1[0].idLen));
    CHECK_STMT_RC(hstmt1, SQLBindCol(hstmt1, 2, SQL_C_LONG, &record1[0].value, sizeof(record1[0].value), &record1[0].valueLen));

    OK_SIMPLE_STMT(hstmt1, "SELECT * FROM test_delete_data ORDER BY id");
    CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));

    /* check delete result */
    is_num(rowcnt / 2, rowsFetchedPtr); /* even number of id has been deleted, half records left */
    for (int i = 0; i < rowcnt / 2; i++)
    {
        is_num(record1[i].id, record[i * 2 + 1].id);
        is_num(record1[i].value, record[i * 2 + 1].value);
    }

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));
    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_delete_data");

    ODBC_Disconnect(henv0, hdbc0, hstmt0);
    ODBC_Disconnect(henv1, hdbc1, hstmt1);

#undef DELETE_DATA_RECORD_ARRAY_SIZE

    return OK;
}

ODBC_TEST(test_describe_operation)
{
#define DESCRIBE_OPERATION_RECORD_NAME_LEN    512
#define DESCRIBE_OPERATION_RECORD_ROW_COUNT   2
    SQLHANDLE   henv1;
    SQLHANDLE   hdbc1;
    SQLHANDLE   hstmt1;
    SQLULEN     rowsFetchedPtr = 0;
    SQLLEN      nLen;

    SQLCHAR     record[DESCRIBE_OPERATION_RECORD_NAME_LEN];
    SQLSMALLINT findFlag = 0;
    char        colName[][DESCRIBE_OPERATION_RECORD_NAME_LEN] = {"id", "value"};

    ODBC_Connect(&henv1, &hdbc1, &hstmt1);

    /* first drop exist table with same name */
    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_describe_operation");

    /* create table and execute decribe */
    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "CREATE TABLE test_describe_operation(id INTEGER, value VARCHAR(30))", SQL_NTS));

    CHECK_STMT_RC(hstmt1, SQLExecDirect(hstmt1, "DESCRIBE test_describe_operation", SQL_NTS));

    /* search for my table from show command */
    for (int i = 0; i < DESCRIBE_OPERATION_RECORD_ROW_COUNT; i++) {
        CHECK_STMT_RC(hstmt1, SQLFetch(hstmt1));
        CHECK_STMT_RC(hstmt1, SQLGetData(hstmt1, 1, SQL_CHAR, record, sizeof(record), &nLen));
        IS_STR(record, colName[i], strlen(colName[i]));
    }
    FAIL_IF(SQLFetch(hstmt1) != SQL_NO_DATA, "Return column count larger than table columns.");

    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));

    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_describe_operation");
    
    ODBC_Disconnect(henv1, hdbc1, hstmt1);

#undef DESCRIBE_OPERATION_RECORD_NAME_LEN

    return OK;
}

ODBC_TEST(test_role_operation)
{
#define ROLE_OPERTATE_STATE_CODE_LEN     6
#define EXPECT_RETURN_NOT_SUPPORT_INFO(hstmt,expect) \
    do {\
    SQLCHAR SQLState[ROLE_OPERTATE_STATE_CODE_LEN];\
    SQLINTEGER NativeError;\
    SQLCHAR SQLMessage[SQL_MAX_MESSAGE_LENGTH];\
    SQLSMALLINT TextLengthPtr;\
    CHECK_STMT_RC(hstmt, SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, SQLState, &NativeError, \
                  SQLMessage, SQL_MAX_MESSAGE_LENGTH, &TextLengthPtr));\
        if (strstr(SQLMessage, expect) == NULL)\
        {\
            fprintf(stdout, "[%s] (%d) %s\n", SQLState, NativeError, SQLMessage);\
            return FAIL;\
        }\
    } while(0);

    SQLHANDLE   henv1;
    SQLHANDLE   hdbc1;
    SQLHANDLE   hstmt1;
    SQLRETURN   ret;
    char       *expectMsg = "This connector does not support";

    ODBC_Connect(&henv1, &hdbc1, &hstmt1);

    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_role_operation");
    OK_SIMPLE_STMT(hstmt1, "CREATE TABLE test_role_operation (id INTEGER, value VARCHAR(300))");

    /* test role and grant statement, expect success, 
       or command pass through ODBC driver/gateway, returns not support message by server 
    */
    
    ret = SQLExecDirect(hstmt1, "SET ROLE ALL", SQL_NTS);
    if (ret != SQL_SUCCESS){
        EXPECT_RETURN_NOT_SUPPORT_INFO(hstmt1, expectMsg);
    }

    ret = SQLExecDirect(hstmt1, "GRANT ADMIN TO USER usr1", SQL_NTS); //GRANT ROLES
    if (ret != SQL_SUCCESS)
    {
        EXPECT_RETURN_NOT_SUPPORT_INFO(hstmt1, expectMsg);
    }
    
    ret = SQLExecDirect(hstmt1, "REVOKE ADMIN OPTION FOR usr1 FROM system", SQL_NTS);
    if (ret != SQL_SUCCESS){
        EXPECT_RETURN_NOT_SUPPORT_INFO(hstmt1, expectMsg);
    }
    
    ret = SQLExecDirect(hstmt1, "CREATE ROLE test_role_operation", SQL_NTS); //CREATE ROLE creates the specified role in the current catalog.
    if (ret != SQL_SUCCESS){
        EXPECT_RETURN_NOT_SUPPORT_INFO(hstmt1, expectMsg);
    }
    
    ret = SQLExecDirect(hstmt1, "DROP ROLE my_role", SQL_NTS); //DROP ROLE drops the specified role in the current catalog
    if (ret != SQL_SUCCESS){
        EXPECT_RETURN_NOT_SUPPORT_INFO(hstmt1, expectMsg);
    }
    
    ret = SQLExecDirect(hstmt1, "GRANT INSERT, SELECT ON test_role_operation TO user1", SQL_NTS);
    if (ret != SQL_SUCCESS){
        EXPECT_RETURN_NOT_SUPPORT_INFO(hstmt1, expectMsg);
    }

    //REVOKE ROLES, Revokes the specified role(s) from the specified principal(s) in the current catalog
    ret = SQLExecDirect(hstmt1, "REVOKE ALL PRIVILEGES ON test_role_operation FROM user1", SQL_NTS);
    if (ret != SQL_SUCCESS){
        EXPECT_RETURN_NOT_SUPPORT_INFO(hstmt1, expectMsg);
    }
    
    CHECK_STMT_RC(hstmt1, SQLFreeStmt(hstmt1, SQL_CLOSE));
    
    OK_SIMPLE_STMT(hstmt1, "DROP TABLE IF EXISTS test_role_operation");
    
    ODBC_Disconnect(henv1, hdbc1, hstmt1);

#undef ROLE_OPERTATE_STATE_CODE_LEN
#undef EXPECT_RETURN_NOT_SUPPORT_INFO

    return OK;
}

MA_ODBC_TESTS my_tests[]=
{
    {test_query_explain,               "test_query_explain"},
    {test_query_explain_analyze,       "test_query_explain_analyze"},
    {test_statment_values,             "test_statment_values"},
    {test_show_catalogs,               "test_show_catalogs"},
    {test_schemas_operation,           "test_schemas_operation"},
    {test_table_operation,             "test_table_operation"},
    {test_show_columns,                "test_show_columns"},
    {test_show_create_table,           "test_show_create_table"},
    {test_show_stats,                  "test_show_stats"},
    {test_session_operation,           "test_session_operation"},
    {test_view_operation,              "test_view_operation"},
    {test_show_functions,              "test_show_functions"},
    {test_trans_command_operation,     "test_trans_command_operation"},
    {test_insert_by_select,            "test_insert_by_select"},
    {test_use_command,                 "test_use_command"},
    {test_create_table_as,             "test_command_create_table_as"},
    {test_role_operation,              "test_role_operation"},
    {test_describe_operation,          "test_describe_operation"},
    {test_analyze_operation,           "test_analyze_operation"},
    {test_alert_table,                 "test_alert_table"},
    {test_update_data,                 "test_update_data"},
    {test_delete_data,                 "test_delete_data"},
    {NULL, NULL}
};

int main(int argc, char **argv)
{
    int ret;
    int count = sizeof(my_tests) / sizeof(MA_ODBC_TESTS) - 1;
    
    get_options(argc, argv);
    plan(count);
    mark_all_tests_normal(my_tests);
    
    ret = run_tests(my_tests);

    return ret;
}
