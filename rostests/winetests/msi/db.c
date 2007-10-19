/*
 * Copyright (C) 2005 Mike McCormack for CodeWeavers
 *
 * A test program for MSI database files.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include <stdio.h>

#include <windows.h>
#include <msi.h>
#include <msiquery.h>

#include "wine/test.h"

static const char *msifile = "winetest.msi";

static void test_msidatabase(void)
{
    MSIHANDLE hdb = 0;
    UINT res;

    DeleteFile(msifile);

    /* create an empty database */
    res = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to create database\n" );

    res = MsiDatabaseCommit( hdb );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    res = DeleteFile( msifile );
    ok( res == TRUE, "Failed to delete database\n" );
}

static UINT do_query(MSIHANDLE hdb, const char *query, MSIHANDLE *phrec)
{
    MSIHANDLE hview = 0;
    UINT r, ret;

    /* open a select query */
    r = MsiDatabaseOpenView(hdb, query, &hview);
    if (r != ERROR_SUCCESS)
        return r;
    r = MsiViewExecute(hview, 0);
    if (r != ERROR_SUCCESS)
        return r;
    ret = MsiViewFetch(hview, phrec);
    r = MsiViewClose(hview);
    if (r != ERROR_SUCCESS)
        return r;
    r = MsiCloseHandle(hview);
    if (r != ERROR_SUCCESS)
        return r;
    return ret;
}

static void test_msiinsert(void)
{
    MSIHANDLE hdb = 0, hview = 0, hrec = 0;
    UINT r;
    const char *query;
    char buf[80];
    DWORD sz;

    DeleteFile(msifile);

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    /* create a table */
    query = "CREATE TABLE `phone` ( "
            "`id` INT, `name` CHAR(32), `number` CHAR(32) "
            "PRIMARY KEY `id`)";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* insert a value into it */
    query = "INSERT INTO `phone` ( `id`, `name`, `number` )"
        "VALUES('1', 'Abe', '8675309')";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    query = "SELECT * FROM `phone` WHERE `id` = 1";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");

    /* check the record contains what we put in it */
    r = MsiRecordGetFieldCount(hrec);
    ok(r == 3, "record count wrong\n");

    todo_wine {
    r = MsiRecordIsNull(hrec, 0);
    ok(r == FALSE, "field 0 not null\n");
    }

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "field 1 contents wrong\n");
    sz = sizeof buf;
    r = MsiRecordGetString(hrec, 2, buf, &sz);
    ok(r == ERROR_SUCCESS, "field 2 content fetch failed\n");
    ok(!strcmp(buf,"Abe"), "field 2 content incorrect\n");
    sz = sizeof buf;
    r = MsiRecordGetString(hrec, 3, buf, &sz);
    ok(r == ERROR_SUCCESS, "field 3 content fetch failed\n");
    ok(!strcmp(buf,"8675309"), "field 3 content incorrect\n");

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* open a select query */
    hrec = 100;
    query = "SELECT * FROM `phone` WHERE `id` >= 10";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "MsiViewFetch failed\n");
    ok(hrec == 0, "hrec should be null\n");

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    query = "SELECT * FROM `phone` WHERE `id` < 0";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "MsiViewFetch failed\n");

    query = "SELECT * FROM `phone` WHERE `id` <= 0";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "MsiViewFetch failed\n");

    query = "SELECT * FROM `phone` WHERE `id` <> 1";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "MsiViewFetch failed\n");

    query = "SELECT * FROM `phone` WHERE `id` > 10";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "MsiViewFetch failed\n");

    todo_wine {
    /* now try a few bad INSERT xqueries */
    query = "INSERT INTO `phone` ( `id`, `name`, `number` )"
        "VALUES(?, ?)";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "MsiDatabaseOpenView failed\n");

    if (r == ERROR_SUCCESS)
        r = MsiCloseHandle(hview);
    }

    /* construct a record to insert */
    hrec = MsiCreateRecord(4);
    r = MsiRecordSetInteger(hrec, 1, 2);
    ok(r == ERROR_SUCCESS, "MsiRecordSetInteger failed\n");
    r = MsiRecordSetString(hrec, 2, "Adam");
    ok(r == ERROR_SUCCESS, "MsiRecordSetString failed\n");
    r = MsiRecordSetString(hrec, 3, "96905305");
    ok(r == ERROR_SUCCESS, "MsiRecordSetString failed\n");

    /* insert another value, using a record and wildcards */
    query = "INSERT INTO `phone` ( `id`, `name`, `number` )"
        "VALUES(?, ?, ?)";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");

    if (r == ERROR_SUCCESS)
    {
        r = MsiViewExecute(hview, hrec);
        ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
        r = MsiViewClose(hview);
        ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
        r = MsiCloseHandle(hview);
        ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");
    }
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiViewFetch(0, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "MsiViewFetch failed\n");

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "MsiDatabaseCommit failed\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = DeleteFile(msifile);
    ok(r == TRUE, "file didn't exist after commit\n");
}

typedef UINT (WINAPI *fnMsiDecomposeDescriptorA)(LPCSTR, LPCSTR, LPSTR, LPSTR, DWORD *);
static fnMsiDecomposeDescriptorA pMsiDecomposeDescriptorA;

static void test_msidecomposedesc(void)
{
    char prod[MAX_FEATURE_CHARS+1], comp[MAX_FEATURE_CHARS+1], feature[MAX_FEATURE_CHARS+1];
    const char *desc;
    UINT r;
    DWORD len;
    HMODULE hmod;

    hmod = GetModuleHandle("msi.dll");
    if (!hmod)
        return;
    pMsiDecomposeDescriptorA = (fnMsiDecomposeDescriptorA)
        GetProcAddress(hmod, "MsiDecomposeDescriptorA");
    if (!pMsiDecomposeDescriptorA)
        return;

    /* test a valid feature descriptor */
    desc = "']gAVn-}f(ZXfeAR6.jiFollowTheWhiteRabbit>3w2x^IGfe?CxI5heAvk.";
    len = 0;
    r = pMsiDecomposeDescriptorA(desc, prod, feature, comp, &len);
    ok(r == ERROR_SUCCESS, "returned an error\n");
    ok(len == strlen(desc), "length was wrong\n");
    ok(strcmp(prod,"{90110409-6000-11D3-8CFE-0150048383C9}")==0, "product wrong\n");
    ok(strcmp(feature,"FollowTheWhiteRabbit")==0, "feature wrong\n");
    ok(strcmp(comp,"{A7CD68DB-EF74-49C8-FBB2-A7C463B2AC24}")==0,"component wrong\n");

    /* test an invalid feature descriptor with too many characters */
    desc = "']gAVn-}f(ZXfeAR6.ji"
           "ThisWillFailIfTheresMoreThanAGuidsChars>"
           "3w2x^IGfe?CxI5heAvk.";
    len = 0;
    r = pMsiDecomposeDescriptorA(desc, prod, feature, comp, &len);
    ok(r == ERROR_INVALID_PARAMETER, "returned wrong error\n");

    /*
     * Test a valid feature descriptor with the
     * maximum number of characters and some trailing characters.
     */
    desc = "']gAVn-}f(ZXfeAR6.ji"
           "ThisWillWorkIfTheresLTEThanAGuidsChars>"
           "3w2x^IGfe?CxI5heAvk."
           "extra";
    len = 0;
    r = pMsiDecomposeDescriptorA(desc, prod, feature, comp, &len);
    ok(r == ERROR_SUCCESS, "returned wrong error\n");
    ok(len == (strlen(desc) - strlen("extra")), "length wrong\n");

    len = 0;
    r = pMsiDecomposeDescriptorA(desc, prod, feature, NULL, &len);
    ok(r == ERROR_SUCCESS, "returned wrong error\n");
    ok(len == (strlen(desc) - strlen("extra")), "length wrong\n");

    len = 0;
    r = pMsiDecomposeDescriptorA(desc, prod, NULL, NULL, &len);
    ok(r == ERROR_SUCCESS, "returned wrong error\n");
    ok(len == (strlen(desc) - strlen("extra")), "length wrong\n");

    len = 0;
    r = pMsiDecomposeDescriptorA(desc, NULL, NULL, NULL, &len);
    ok(r == ERROR_SUCCESS, "returned wrong error\n");
    ok(len == (strlen(desc) - strlen("extra")), "length wrong\n");

    len = 0;
    r = pMsiDecomposeDescriptorA(NULL, NULL, NULL, NULL, &len);
    ok(r == ERROR_INVALID_PARAMETER, "returned wrong error\n");
    ok(len == 0, "length wrong\n");
}

static UINT try_query_param( MSIHANDLE hdb, LPCSTR szQuery, MSIHANDLE hrec )
{
    MSIHANDLE htab = 0;
    UINT res;

    res = MsiDatabaseOpenView( hdb, szQuery, &htab );
    if(res == ERROR_SUCCESS )
    {
        UINT r;

        r = MsiViewExecute( htab, hrec );
        if(r != ERROR_SUCCESS )
            res = r;

        r = MsiViewClose( htab );
        if(r != ERROR_SUCCESS )
            res = r;

        r = MsiCloseHandle( htab );
        if(r != ERROR_SUCCESS )
            res = r;
    }
    return res;
}

static UINT try_query( MSIHANDLE hdb, LPCSTR szQuery )
{
    return try_query_param( hdb, szQuery, 0 );
}

static UINT try_insert_query( MSIHANDLE hdb, LPCSTR szQuery )
{
    MSIHANDLE hrec = 0;
    UINT r;

    hrec = MsiCreateRecord( 1 );
    MsiRecordSetString( hrec, 1, "Hello");

    r = try_query_param( hdb, szQuery, hrec );

    MsiCloseHandle( hrec );
    return r;
}

static void test_msibadqueries(void)
{
    MSIHANDLE hdb = 0;
    UINT r;

    DeleteFile(msifile);

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    r = MsiDatabaseCommit( hdb );
    ok(r == ERROR_SUCCESS , "Failed to commit database\n");

    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS , "Failed to close database\n");

    /* open it readonly */
    r = MsiOpenDatabase(msifile, MSIDBOPEN_READONLY, &hdb );
    ok(r == ERROR_SUCCESS , "Failed to open database r/o\n");

    /* add a table to it */
    r = try_query( hdb, "select * from _Tables");
    ok(r == ERROR_SUCCESS , "query 1 failed\n");

    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS , "Failed to close database r/o\n");

    /* open it read/write */
    r = MsiOpenDatabase(msifile, MSIDBOPEN_TRANSACT, &hdb );
    ok(r == ERROR_SUCCESS , "Failed to open database r/w\n");

    /* a bunch of test queries that fail with the native MSI */

    r = try_query( hdb, "CREATE TABLE");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2a return code\n");

    r = try_query( hdb, "CREATE TABLE `a`");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2b return code\n");

    r = try_query( hdb, "CREATE TABLE `a` ()");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2c return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b`)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2d return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) )");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2e return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2f return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2g return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY KEY)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2h return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY KEY)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2i return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY KEY 'b')");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2j return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY KEY `b')");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2k return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY KEY `b')");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2l return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHA(72) NOT NULL PRIMARY KEY `b`)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2m return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(-1) NOT NULL PRIMARY KEY `b`)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2n return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(720) NOT NULL PRIMARY KEY `b`)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2o return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL KEY `b`)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2p return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`` CHAR(72) NOT NULL PRIMARY KEY `b`)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "invalid query 2p return code\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY KEY `b`)");
    ok(r == ERROR_SUCCESS , "valid query 2z failed\n");

    r = try_query( hdb, "CREATE TABLE `a` (`b` CHAR(72) NOT NULL PRIMARY KEY `b`)");
    ok(r == ERROR_BAD_QUERY_SYNTAX , "created same table again\n");

    r = try_query( hdb, "CREATE TABLE `aa` (`b` CHAR(72) NOT NULL, `c` "
          "CHAR(72), `d` CHAR(255) NOT NULL LOCALIZABLE PRIMARY KEY `b`)");
    ok(r == ERROR_SUCCESS , "query 4 failed\n");

    r = MsiDatabaseCommit( hdb );
    ok(r == ERROR_SUCCESS , "Failed to commit database after write\n");

    r = try_query( hdb, "CREATE TABLE `blah` (`foo` CHAR(72) NOT NULL "
                          "PRIMARY KEY `foo`)");
    ok(r == ERROR_SUCCESS , "query 4 failed\n");

    r = try_insert_query( hdb, "insert into a  ( `b` ) VALUES ( ? )");
    ok(r == ERROR_SUCCESS , "failed to insert record in db\n");

    r = MsiDatabaseCommit( hdb );
    ok(r == ERROR_SUCCESS , "Failed to commit database after write\n");

    r = try_query( hdb, "CREATE TABLE `boo` (`foo` CHAR(72) NOT NULL "
                          "PRIMARY KEY `ba`)");
    ok(r != ERROR_SUCCESS , "query 5 succeeded\n");

    r = try_query( hdb,"CREATE TABLE `bee` (`foo` CHAR(72) NOT NULL )");
    ok(r != ERROR_SUCCESS , "query 6 succeeded\n");

    r = try_query( hdb, "CREATE TABLE `temp` (`t` CHAR(72) NOT NULL "
                          "PRIMARY KEY `t`)");
    ok(r == ERROR_SUCCESS , "query 7 failed\n");

    r = try_query( hdb, "CREATE TABLE `c` (`b` CHAR NOT NULL PRIMARY KEY `b`)");
    ok(r == ERROR_SUCCESS , "query 8 failed\n");

    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS , "Failed to close database transact\n");

    r = DeleteFile( msifile );
    ok(r == TRUE, "file didn't exist after commit\n");
}

static UINT run_query( MSIHANDLE hdb, MSIHANDLE hrec, const char *query )
{
    MSIHANDLE hview = 0;
    UINT r;

    r = MsiDatabaseOpenView(hdb, query, &hview);
    if( r != ERROR_SUCCESS )
        return r;

    r = MsiViewExecute(hview, hrec);
    if( r == ERROR_SUCCESS )
        r = MsiViewClose(hview);
    MsiCloseHandle(hview);
    return r;
}

static void test_viewmodify(void)
{
    MSIHANDLE hdb = 0, hview = 0, hrec = 0;
    UINT r;
    const char *query;
    char buffer[0x100];
    DWORD sz;

    DeleteFile(msifile);

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    query = "CREATE TABLE `phone` ( "
            "`id` INT, `name` CHAR(32), `number` CHAR(32) "
            "PRIMARY KEY `id`)";
    r = run_query( hdb, 0, query );
    ok(r == ERROR_SUCCESS, "query failed\n");

    /* check what the error function reports without doing anything */
    sz = 0;
    /* passing NULL as the 3rd param make function to crash on older platforms */
    r = MsiViewGetError( 0, NULL, &sz );
    ok(r == MSIDBERROR_INVALIDARG, "MsiViewGetError return\n");

    /* open a view */
    query = "SELECT * FROM `phone`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");

    /* see what happens with a good hview and bad args */
    r = MsiViewGetError( hview, NULL, NULL );
    ok(r == MSIDBERROR_INVALIDARG || r == MSIDBERROR_NOERROR,
       "MsiViewGetError returns %u (expected -3)\n", r);
    r = MsiViewGetError( hview, buffer, NULL );
    ok(r == MSIDBERROR_INVALIDARG, "MsiViewGetError return\n");

    /* see what happens with a zero length buffer */
    sz = 0;
    buffer[0] = 'x';
    r = MsiViewGetError( hview, buffer, &sz );
    ok(r == MSIDBERROR_MOREDATA, "MsiViewGetError return\n");
    ok(buffer[0] == 'x', "buffer cleared\n");
    ok(sz == 0, "size not zero\n");

    /* ok this one is strange */
    sz = 0;
    r = MsiViewGetError( hview, NULL, &sz );
    ok(r == MSIDBERROR_NOERROR, "MsiViewGetError return\n");
    ok(sz == 0, "size not zero\n");

    /* see if it really has an error */
    sz = sizeof buffer;
    buffer[0] = 'x';
    r = MsiViewGetError( hview, buffer, &sz );
    ok(r == MSIDBERROR_NOERROR, "MsiViewGetError return\n");
    ok(buffer[0] == 0, "buffer not cleared\n");
    ok(sz == 0, "size not zero\n");

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    /* try some invalid records */
    r = MsiViewModify(hview, MSIMODIFY_INSERT, 0 );
    ok(r == ERROR_INVALID_HANDLE, "MsiViewModify failed\n");
    r = MsiViewModify(hview, -1, 0 );
    ok(r == ERROR_INVALID_HANDLE, "MsiViewModify failed\n");

    /* try an small record */
    hrec = MsiCreateRecord(1);
    r = MsiViewModify(hview, -1, hrec );
    ok(r == ERROR_INVALID_DATA, "MsiViewModify failed\n");

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    /* insert a valid record */
    hrec = MsiCreateRecord(3);

    r = MsiRecordSetInteger(hrec, 2, 1);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetString(hrec, 2, "bob");
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetString(hrec, 3, "7654321");
    ok(r == ERROR_SUCCESS, "failed to set integer\n");

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewModify(hview, MSIMODIFY_INSERT_TEMPORARY, hrec );
    ok(r == ERROR_SUCCESS, "MsiViewModify failed\n");

    /* insert the same thing again */
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    /* should fail ... */
    todo_wine {
    r = MsiViewModify(hview, MSIMODIFY_INSERT_TEMPORARY, hrec );
    ok(r == ERROR_FUNCTION_FAILED, "MsiViewModify failed\n");
    }

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase close failed\n");
}

static MSIHANDLE create_db(void)
{
    MSIHANDLE hdb = 0;
    UINT res;

    DeleteFile(msifile);

    /* create an empty database */
    res = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to create database\n" );
    if( res != ERROR_SUCCESS )
        return hdb;

    res = MsiDatabaseCommit( hdb );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    return hdb;
}

static void test_getcolinfo(void)
{
    MSIHANDLE hdb, hview = 0, rec = 0;
    UINT r;
    DWORD sz;
    char buffer[0x20];

    /* create an empty db */
    hdb = create_db();
    ok( hdb, "failed to create db\n");

    /* tables should be present */
    r = MsiDatabaseOpenView(hdb, "select * from _Tables", &hview);
    ok( r == ERROR_SUCCESS, "failed to open query\n");

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute query\n");

    /* check that NAMES works */
    rec = 0;
    r = MsiViewGetColumnInfo( hview, MSICOLINFO_NAMES, &rec );
    ok( r == ERROR_SUCCESS, "failed to get names\n");
    sz = sizeof buffer;
    r = MsiRecordGetString(rec, 1, buffer, &sz );
    ok( r == ERROR_SUCCESS, "failed to get string\n");
    ok( !strcmp(buffer,"Name"), "_Tables has wrong column name\n");
    r = MsiCloseHandle( rec );
    ok( r == ERROR_SUCCESS, "failed to close record handle\n");

    /* check that TYPES works */
    rec = 0;
    r = MsiViewGetColumnInfo( hview, MSICOLINFO_TYPES, &rec );
    ok( r == ERROR_SUCCESS, "failed to get names\n");
    sz = sizeof buffer;
    r = MsiRecordGetString(rec, 1, buffer, &sz );
    ok( r == ERROR_SUCCESS, "failed to get string\n");
    ok( !strcmp(buffer,"s64"), "_Tables has wrong column type\n");
    r = MsiCloseHandle( rec );
    ok( r == ERROR_SUCCESS, "failed to close record handle\n");

    /* check that invalid values fail */
    rec = 0;
    r = MsiViewGetColumnInfo( hview, 100, &rec );
    ok( r == ERROR_INVALID_PARAMETER, "wrong error code\n");
    ok( rec == 0, "returned a record\n");

    r = MsiViewGetColumnInfo( hview, MSICOLINFO_TYPES, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong error code\n");

    r = MsiViewGetColumnInfo( 0, MSICOLINFO_TYPES, &rec );
    ok( r == ERROR_INVALID_HANDLE, "wrong error code\n");

    r = MsiViewClose(hview);
    ok( r == ERROR_SUCCESS, "failed to close view\n");
    r = MsiCloseHandle(hview);
    ok( r == ERROR_SUCCESS, "failed to close view handle\n");
    r = MsiCloseHandle(hdb);
    ok( r == ERROR_SUCCESS, "failed to close database\n");
}

static MSIHANDLE get_column_info(MSIHANDLE hdb, const char *query, MSICOLINFO type)
{
    MSIHANDLE hview = 0, rec = 0;
    UINT r;

    r = MsiDatabaseOpenView(hdb, query, &hview);
    if( r != ERROR_SUCCESS )
        return r;

    r = MsiViewExecute(hview, 0);
    if( r == ERROR_SUCCESS )
    {
        MsiViewGetColumnInfo( hview, type, &rec );
        MsiViewClose(hview);
    }
    MsiCloseHandle(hview);
    return rec;
}

static UINT get_columns_table_type(MSIHANDLE hdb, const char *table, UINT field)
{
    MSIHANDLE hview = 0, rec = 0;
    UINT r, type = 0;
    char query[0x100];

    sprintf(query, "select * from `_Columns` where  `Table` = '%s'", table );

    r = MsiDatabaseOpenView(hdb, query, &hview);
    if( r != ERROR_SUCCESS )
        return r;

    r = MsiViewExecute(hview, 0);
    if( r == ERROR_SUCCESS )
    {
        while (1)
        {
            r = MsiViewFetch( hview, &rec );
            if( r != ERROR_SUCCESS)
                break;
            r = MsiRecordGetInteger( rec, 2 );
            if (r == field)
                type = MsiRecordGetInteger( rec, 4 );
            MsiCloseHandle( rec );
        }

        MsiViewClose(hview);
    }
    MsiCloseHandle(hview);
    return type;
}

static BOOL check_record( MSIHANDLE rec, UINT field, LPCSTR val )
{
    CHAR buffer[0x20];
    UINT r;
    DWORD sz;

    sz = sizeof buffer;
    r = MsiRecordGetString( rec, field, buffer, &sz );
    return (r == ERROR_SUCCESS ) && !strcmp(val, buffer);
}

static void test_viewgetcolumninfo(void)
{
    MSIHANDLE hdb = 0, rec;
    UINT r;

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    r = run_query( hdb, 0,
            "CREATE TABLE `Properties` "
            "( `Property` CHAR(255), `Value` CHAR(1)  PRIMARY KEY `Property`)" );
    ok( r == ERROR_SUCCESS , "Failed to create table\n" );

    /* check the column types */
    rec = get_column_info( hdb, "select * from `Properties`", MSICOLINFO_TYPES );
    ok( rec, "failed to get column info record\n" );

    ok( check_record( rec, 1, "S255"), "wrong record type\n");
    ok( check_record( rec, 2, "S1"), "wrong record type\n");

    MsiCloseHandle( rec );

    /* check the type in _Columns */
    ok( 0x3dff == get_columns_table_type(hdb, "Properties", 1 ), "_columns table wrong\n");
    ok( 0x1d01 == get_columns_table_type(hdb, "Properties", 2 ), "_columns table wrong\n");

    /* now try the names */
    rec = get_column_info( hdb, "select * from `Properties`", MSICOLINFO_NAMES );
    ok( rec, "failed to get column info record\n" );

    ok( check_record( rec, 1, "Property"), "wrong record type\n");
    ok( check_record( rec, 2, "Value"), "wrong record type\n");

    MsiCloseHandle( rec );

    r = run_query( hdb, 0,
            "CREATE TABLE `Binary` "
            "( `Name` CHAR(255), `Data` OBJECT  PRIMARY KEY `Name`)" );
    ok( r == ERROR_SUCCESS , "Failed to create table\n" );

    /* check the column types */
    rec = get_column_info( hdb, "select * from `Binary`", MSICOLINFO_TYPES );
    ok( rec, "failed to get column info record\n" );

    ok( check_record( rec, 1, "S255"), "wrong record type\n");
    ok( check_record( rec, 2, "V0"), "wrong record type\n");

    MsiCloseHandle( rec );

    /* check the type in _Columns */
    ok( 0x3dff == get_columns_table_type(hdb, "Binary", 1 ), "_columns table wrong\n");
    ok( 0x1900 == get_columns_table_type(hdb, "Binary", 2 ), "_columns table wrong\n");

    /* now try the names */
    rec = get_column_info( hdb, "select * from `Binary`", MSICOLINFO_NAMES );
    ok( rec, "failed to get column info record\n" );

    ok( check_record( rec, 1, "Name"), "wrong record type\n");
    ok( check_record( rec, 2, "Data"), "wrong record type\n");
    MsiCloseHandle( rec );

    r = run_query( hdb, 0,
            "CREATE TABLE `UIText` "
            "( `Key` CHAR(72) NOT NULL, `Text` CHAR(255) LOCALIZABLE PRIMARY KEY `Key`)" );
    ok( r == ERROR_SUCCESS , "Failed to create table\n" );

    ok( 0x2d48 == get_columns_table_type(hdb, "UIText", 1 ), "_columns table wrong\n");
    ok( 0x1fff == get_columns_table_type(hdb, "UIText", 2 ), "_columns table wrong\n");

    rec = get_column_info( hdb, "select * from `UIText`", MSICOLINFO_NAMES );
    ok( rec, "failed to get column info record\n" );
    ok( check_record( rec, 1, "Key"), "wrong record type\n");
    ok( check_record( rec, 2, "Text"), "wrong record type\n");
    MsiCloseHandle( rec );

    rec = get_column_info( hdb, "select * from `UIText`", MSICOLINFO_TYPES );
    ok( rec, "failed to get column info record\n" );
    ok( check_record( rec, 1, "s72"), "wrong record type\n");
    ok( check_record( rec, 2, "L255"), "wrong record type\n");
    MsiCloseHandle( rec );

    MsiCloseHandle( hdb );
}

static void test_msiexport(void)
{
    MSIHANDLE hdb = 0, hview = 0;
    UINT r;
    const char *query;
    char path[MAX_PATH];
    const char file[] = "phone.txt";
    HANDLE handle;
    char buffer[0x100];
    DWORD length;
    const char expected[] =
        "id\tname\tnumber\r\n"
        "I2\tS32\tS32\r\n"
        "phone\tid\r\n"
        "1\tAbe\t8675309\r\n";

    DeleteFile(msifile);

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    /* create a table */
    query = "CREATE TABLE `phone` ( "
            "`id` INT, `name` CHAR(32), `number` CHAR(32) "
            "PRIMARY KEY `id`)";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* insert a value into it */
    query = "INSERT INTO `phone` ( `id`, `name`, `number` )"
        "VALUES('1', 'Abe', '8675309')";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    GetCurrentDirectory(MAX_PATH, path);

    todo_wine {
    r = MsiDatabaseExport(hdb, "phone", path, file);
    ok(r == ERROR_SUCCESS, "MsiDatabaseExport failed\n");

    MsiCloseHandle(hdb);

    lstrcat(path, "\\");
    lstrcat(path, file);

    /* check the data that was written */
    length = 0;
    memset(buffer, 0, sizeof buffer);
    handle = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (handle != INVALID_HANDLE_VALUE)
    {
        ReadFile(handle, buffer, sizeof buffer, &length, NULL);
        CloseHandle(handle);
        DeleteFile(path);
    }
    else
        ok(0, "failed to open file %s\n", path);

    ok( length == strlen(expected), "length of data wrong\n");
    ok( !lstrcmp(buffer, expected), "data doesn't match\n");
    }
    DeleteFile(msifile);
}

static void test_longstrings(void)
{
    const char insert_query[] =
        "INSERT INTO `strings` ( `id`, `val` ) VALUES('1', 'Z')";
    char *str;
    MSIHANDLE hdb = 0, hview = 0, hrec = 0;
    DWORD len;
    UINT r;
    const DWORD STRING_LENGTH = 0x10005;

    DeleteFile(msifile);
    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    /* create a table */
    r = try_query( hdb,
        "CREATE TABLE `strings` ( `id` INT, `val` CHAR(0) PRIMARY KEY `id`)");
    ok(r == ERROR_SUCCESS, "query failed\n");

    /* try a insert a very long string */
    str = HeapAlloc(GetProcessHeap(), 0, STRING_LENGTH+sizeof insert_query);
    len = strchr(insert_query, 'Z') - insert_query;
    strcpy(str, insert_query);
    memset(str+len, 'Z', STRING_LENGTH);
    strcpy(str+len+STRING_LENGTH, insert_query+len+1);
    r = try_query( hdb, str );
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");

    HeapFree(GetProcessHeap(), 0, str);

    MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "MsiDatabaseCommit failed\n");
    MsiCloseHandle(hdb);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_READONLY, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    r = MsiDatabaseOpenView(hdb, "select * from `strings` where `id` = 1", &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");

    MsiCloseHandle(hview);

    r = MsiRecordGetString(hrec, 2, NULL, &len);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    todo_wine {
    ok(len == STRING_LENGTH, "string length wrong\n");
    }

    MsiCloseHandle(hrec);
    MsiCloseHandle(hdb);
    DeleteFile(msifile);
}

static void test_streamtable(void)
{
    MSIHANDLE hdb = 0, rec;
    UINT r;

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    r = run_query( hdb, 0,
            "CREATE TABLE `Properties` "
            "( `Property` CHAR(255), `Value` CHAR(1)  PRIMARY KEY `Property`)" );
    ok( r == ERROR_SUCCESS , "Failed to create table\n" );

    /* check the column types */
    rec = get_column_info( hdb, "select * from `_Streams`", MSICOLINFO_TYPES );
    ok( rec, "failed to get column info record\n" );

    todo_wine {
    ok( check_record( rec, 1, "s62"), "wrong record type\n");
    ok( check_record( rec, 2, "V0"), "wrong record type\n");
    }

    MsiCloseHandle( rec );

    /* now try the names */
    rec = get_column_info( hdb, "select * from `_Streams`", MSICOLINFO_NAMES );
    ok( rec, "failed to get column info record\n" );

    todo_wine {
    ok( check_record( rec, 1, "Name"), "wrong record type\n");
    ok( check_record( rec, 2, "Data"), "wrong record type\n");
    }

    MsiCloseHandle( rec );
    MsiCloseHandle( hdb );
    DeleteFile(msifile);
}

static void test_where(void)
{
    MSIHANDLE hdb = 0, rec;
    LPCSTR query;
    UINT r;

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    r = run_query( hdb, 0,
            "CREATE TABLE `Media` ("
            "`DiskId` SHORT NOT NULL, "
            "`LastSequence` LONG, "
            "`DiskPrompt` CHAR(64) LOCALIZABLE, "
            "`Cabinet` CHAR(255), "
            "`VolumeLabel` CHAR(32), "
            "`Source` CHAR(72) "
            "PRIMARY KEY `DiskId`)" );
    ok( r == S_OK, "cannot create Media table: %d\n", r );

    r = run_query( hdb, 0, "INSERT INTO `Media` "
            "( `DiskId`, `LastSequence`, `DiskPrompt`, `Cabinet`, `VolumeLabel`, `Source` ) "
            "VALUES ( 1, 0, '', 'zero.cab', '', '' )" );
    ok( r == S_OK, "cannot add file to the Media table: %d\n", r );

    r = run_query( hdb, 0, "INSERT INTO `Media` "
            "( `DiskId`, `LastSequence`, `DiskPrompt`, `Cabinet`, `VolumeLabel`, `Source` ) "
            "VALUES ( 2, 1, '', 'one.cab', '', '' )" );
    ok( r == S_OK, "cannot add file to the Media table: %d\n", r );

    r = run_query( hdb, 0, "INSERT INTO `Media` "
            "( `DiskId`, `LastSequence`, `DiskPrompt`, `Cabinet`, `VolumeLabel`, `Source` ) "
            "VALUES ( 3, 2, '', 'two.cab', '', '' )" );
    ok( r == S_OK, "cannot add file to the Media table: %d\n", r );

    query = "SELECT * FROM `Media`";
    r = do_query(hdb, query, &rec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed: %d\n", r);
    ok( check_record( rec, 4, "zero.cab"), "wrong cabinet\n");
    MsiCloseHandle( rec );

    query = "SELECT * FROM `Media` WHERE `LastSequence` >= 1";
    r = do_query(hdb, query, &rec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed: %d\n", r);
    ok( check_record( rec, 4, "one.cab"), "wrong cabinet\n");

    r = MsiRecordGetInteger(rec, 1);
    ok( 2 == r, "field wrong\n");
    r = MsiRecordGetInteger(rec, 2);
    ok( 1 == r, "field wrong\n");

    MsiCloseHandle( rec );

    MsiCloseHandle( hdb );
    DeleteFile(msifile);
}

static CHAR CURR_DIR[MAX_PATH];

static const CHAR test_data[] = "FirstPrimaryColumn\tSecondPrimaryColumn\tShortInt\tShortIntNullable\tLongInt\tLongIntNullable\tString\tLocalizableString\tLocalizableStringNullable\n"
                                "s255\ti2\ti2\tI2\ti4\tI4\tS255\tS0\ts0\n"
                                "TestTable\tFirstPrimaryColumn\n"
                                "stringage\t5\t2\t\t2147483640\t-2147483640\tanother string\tlocalizable\tduh\n";

static void write_file(const CHAR *filename, const char *data, int data_size)
{
    DWORD size;

    HANDLE hf = CreateFile(filename, GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    WriteFile(hf, data, data_size, &size, NULL);
    CloseHandle(hf);
}

static UINT add_table_to_db(MSIHANDLE hdb, LPCSTR table_data)
{
    UINT r;

    write_file("temp_file", table_data, (lstrlen(table_data) - 1) * sizeof(char));
    r = MsiDatabaseImportA(hdb, CURR_DIR, "temp_file");
    DeleteFileA("temp_file");

    return r;
}

static void test_msiimport(void)
{
    MSIHANDLE hdb, view, rec;
    LPCSTR query;
    UINT r, count;
    signed int i;

    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);

    r = MsiOpenDatabaseA(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_table_to_db(hdb, test_data);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    }

    query = "SELECT * FROM `TestTable`";
    r = MsiDatabaseOpenView(hdb, query, &view);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    }

    r = MsiViewGetColumnInfo(view, MSICOLINFO_NAMES, &rec);
    count = MsiRecordGetFieldCount(rec);
    todo_wine
    {
        ok(count == 9, "Expected 9, got %d\n", count);
        ok(check_record(rec, 1, "FirstPrimaryColumn"), "Expected FirstPrimaryColumn\n");
        ok(check_record(rec, 2, "SecondPrimaryColumn"), "Expected SecondPrimaryColumn\n");
        ok(check_record(rec, 3, "ShortInt"), "Expected ShortInt\n");
        ok(check_record(rec, 4, "ShortIntNullable"), "Expected ShortIntNullalble\n");
        ok(check_record(rec, 5, "LongInt"), "Expected LongInt\n");
        ok(check_record(rec, 6, "LongIntNullable"), "Expected LongIntNullalble\n");
        ok(check_record(rec, 7, "String"), "Expected String\n");
        ok(check_record(rec, 8, "LocalizableString"), "Expected LocalizableString\n");
        ok(check_record(rec, 9, "LocalizableStringNullable"), "Expected LocalizableStringNullable\n");
    }

    r = MsiViewGetColumnInfo(view, MSICOLINFO_TYPES, &rec);
    count = MsiRecordGetFieldCount(rec);
    todo_wine
    {
        ok(count == 9, "Expected 9, got %d\n", count);
        ok(check_record(rec, 1, "s255"), "Expected s255\n");
        ok(check_record(rec, 2, "i2"), "Expected i2\n");
        ok(check_record(rec, 3, "i2"), "Expected i2\n");
        ok(check_record(rec, 4, "I2"), "Expected I2\n");
        ok(check_record(rec, 5, "i4"), "Expected i4\n");
        ok(check_record(rec, 6, "I4"), "Expected I4\n");
        ok(check_record(rec, 7, "S255"), "Expected S255\n");
        ok(check_record(rec, 8, "S0"), "Expected S0\n");
        ok(check_record(rec, 9, "s0"), "Expected s0\n");
    }

    query = "SELECT * FROM `TestTable`";
    r = do_query(hdb, query, &rec);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    }

    todo_wine
    {
        ok(check_record(rec, 1, "stringage"), "Expected 'stringage'\n");
        ok(check_record(rec, 7, "another string"), "Expected 'another string'\n");
        ok(check_record(rec, 8, "localizable"), "Expected 'localizable'\n");
        ok(check_record(rec, 9, "duh"), "Expected 'duh'\n");
    }

    i = MsiRecordGetInteger(rec, 2);
    todo_wine
    {
        ok(i == 5, "Expected 5, got %d\n", i);
    }

    i = MsiRecordGetInteger(rec, 3);
    todo_wine
    {
        ok(i == 2, "Expected 2, got %d\n", i);
    }

    i = MsiRecordGetInteger(rec, 4);
    ok(i == 0x80000000, "Expected 0x80000000, got %d\n", i);

    i = MsiRecordGetInteger(rec, 5);
    todo_wine
    {
        ok(i == 2147483640, "Expected 2147483640, got %d\n", i);
    }

    i = MsiRecordGetInteger(rec, 6);
    todo_wine
    {
        ok(i == -2147483640, "Expected -2147483640, got %d\n", i);
    }

    MsiCloseHandle(rec);
    MsiCloseHandle(view);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_markers(void)
{
    MSIHANDLE hdb, rec;
    LPCSTR query;
    UINT r;

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    rec = MsiCreateRecord(3);
    MsiRecordSetString(rec, 1, "Table");
    MsiRecordSetString(rec, 2, "Apples");
    MsiRecordSetString(rec, 3, "Oranges");

    /* try a legit create */
    query = "CREATE TABLE `Table` ( `One` SHORT NOT NULL, `Two` CHAR(255) PRIMARY KEY `One`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    MsiCloseHandle(rec);

    /* try table name as marker */
    rec = MsiCreateRecord(1);
    MsiRecordSetString(rec, 1, "Fable");
    query = "CREATE TABLE `?` ( `One` SHORT NOT NULL, `Two` CHAR(255) PRIMARY KEY `One`)";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* try table name as marker without backticks */
    MsiRecordSetString(rec, 1, "Mable");
    query = "CREATE TABLE ? ( `One` SHORT NOT NULL, `Two` CHAR(255) PRIMARY KEY `One`)";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* try one column name as marker */
    MsiRecordSetString(rec, 1, "One");
    query = "CREATE TABLE `Mable` ( `?` SHORT NOT NULL, `Two` CHAR(255) PRIMARY KEY `One`)";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* try column names as markers */
    MsiCloseHandle(rec);
    rec = MsiCreateRecord(2);
    MsiRecordSetString(rec, 1, "One");
    MsiRecordSetString(rec, 2, "Two");
    query = "CREATE TABLE `Mable` ( `?` SHORT NOT NULL, `?` CHAR(255) PRIMARY KEY `One`)";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* try names with backticks */
    MsiCloseHandle(rec);
    rec = MsiCreateRecord(3);
    MsiRecordSetString(rec, 1, "One");
    MsiRecordSetString(rec, 2, "Two");
    MsiRecordSetString(rec, 3, "One");
    query = "CREATE TABLE `Mable` ( `?` SHORT NOT NULL, `?` CHAR(255) PRIMARY KEY `?`)";
    r = run_query(hdb, rec, query);
    todo_wine
    {
        ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    }

    /* try names with backticks, minus definitions */
    query = "CREATE TABLE `Mable` ( `?`, `?` PRIMARY KEY `?`)";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* try names without backticks */
    query = "CREATE TABLE `Mable` ( ? SHORT NOT NULL, ? CHAR(255) PRIMARY KEY ?)";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* try one long marker */
    MsiCloseHandle(rec);
    rec = MsiCreateRecord(1);
    MsiRecordSetString(rec, 1, "`One` SHORT NOT NULL, `Two` CHAR(255) PRIMARY KEY `One`");
    query = "CREATE TABLE `Mable` ( ? )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* try all names as markers */
    MsiCloseHandle(rec);
    rec = MsiCreateRecord(4);
    MsiRecordSetString(rec, 1, "Mable");
    MsiRecordSetString(rec, 2, "One");
    MsiRecordSetString(rec, 3, "Two");
    MsiRecordSetString(rec, 4, "One");
    query = "CREATE TABLE `?` ( `?` SHORT NOT NULL, `?` CHAR(255) PRIMARY KEY `?`)";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* try a legit insert */
    query = "INSERT INTO `Table` ( `One`, `Two` ) VALUES ( 5, 'hello' )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* try values as markers */
    MsiCloseHandle(rec);
    rec = MsiCreateRecord(2);
    MsiRecordSetInteger(rec, 1, 4);
    MsiRecordSetString(rec, 2, "hi");
    query = "INSERT INTO `Table` ( `One`, `Two` ) VALUES ( ?, '?' )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* try column names and values as markers */
    MsiCloseHandle(rec);
    rec = MsiCreateRecord(4);
    MsiRecordSetString(rec, 1, "One");
    MsiRecordSetString(rec, 2, "Two");
    MsiRecordSetInteger(rec, 3, 5);
    MsiRecordSetString(rec, 4, "hi");
    query = "INSERT INTO `Table` ( `?`, `?` ) VALUES ( ?, '?' )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* try column names as markers */
    MsiCloseHandle(rec);
    rec = MsiCreateRecord(2);
    MsiRecordSetString(rec, 1, "One");
    MsiRecordSetString(rec, 2, "Two");
    query = "INSERT INTO `Table` ( `?`, `?` ) VALUES ( 3, 'yellow' )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* try table name as a marker */
    MsiCloseHandle(rec);
    rec = MsiCreateRecord(1);
    MsiRecordSetString(rec, 1, "Table");
    query = "INSERT INTO `?` ( `One`, `Two` ) VALUES ( 2, 'green' )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* try table name and values as markers */
    MsiCloseHandle(rec);
    rec = MsiCreateRecord(3);
    MsiRecordSetString(rec, 1, "Table");
    MsiRecordSetInteger(rec, 2, 10);
    MsiRecordSetString(rec, 3, "haha");
    query = "INSERT INTO `?` ( `One`, `Two` ) VALUES ( ?, '?' )";
    r = run_query(hdb, rec, query);
    todo_wine
    {
        ok(r == ERROR_FUNCTION_FAILED, "Expected ERROR_FUNCTION_FAILED, got %d\n", r);
    }

    /* try all markers */
    MsiCloseHandle(rec);
    rec = MsiCreateRecord(5);
    MsiRecordSetString(rec, 1, "Table");
    MsiRecordSetString(rec, 1, "One");
    MsiRecordSetString(rec, 1, "Two");
    MsiRecordSetInteger(rec, 2, 10);
    MsiRecordSetString(rec, 3, "haha");
    query = "INSERT INTO `?` ( `?`, `?` ) VALUES ( ?, '?' )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* insert an integer as a string */
    MsiCloseHandle(rec);
    rec = MsiCreateRecord(2);
    MsiRecordSetString(rec, 1, "11");
    MsiRecordSetString(rec, 2, "hi");
    query = "INSERT INTO `Table` ( `One`, `Two` ) VALUES ( ?, '?' )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* leave off the '' for the string */
    MsiCloseHandle(rec);
    rec = MsiCreateRecord(2);
    MsiRecordSetInteger(rec, 1, 12);
    MsiRecordSetString(rec, 2, "hi");
    query = "INSERT INTO `Table` ( `One`, `Two` ) VALUES ( ?, ? )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    MsiCloseHandle(rec);

    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

START_TEST(db)
{
    test_msidatabase();
    test_msiinsert();
    test_msidecomposedesc();
    test_msibadqueries();
    test_viewmodify();
    test_viewgetcolumninfo();
    test_getcolinfo();
    test_msiexport();
    test_longstrings();
    test_streamtable();
    test_where();
    test_msiimport();
    test_markers();
}
