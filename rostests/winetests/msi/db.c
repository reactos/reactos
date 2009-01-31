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

#define COBJMACROS

#include <stdio.h>

#include <windows.h>
#include <msi.h>
#include <msiquery.h>

#include <objidl.h>

#include "wine/test.h"

static const char *msifile = "winetest.msi";
static const char *msifile2 = "winetst2.msi";
static const char *mstfile = "winetst.mst";

static void test_msidatabase(void)
{
    MSIHANDLE hdb = 0, hdb2 = 0;
    UINT res;

    DeleteFile(msifile);

    res = MsiOpenDatabase( msifile, msifile2, &hdb );
    ok( res == ERROR_OPEN_FAILED, "expected failure\n");

    res = MsiOpenDatabase( msifile, (LPSTR) 0xff, &hdb );
    ok( res == ERROR_INVALID_PARAMETER, "expected failure\n");

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    /* create an empty database */
    res = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to create database\n" );

    res = MsiDatabaseCommit( hdb );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    ok( INVALID_FILE_ATTRIBUTES != GetFileAttributes( msifile ), "database should exist\n");

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );
    res = MsiOpenDatabase( msifile, msifile2, &hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to open database\n" );

    ok( INVALID_FILE_ATTRIBUTES != GetFileAttributes( msifile2 ), "database should exist\n");

    res = MsiDatabaseCommit( hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    res = MsiCloseHandle( hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    res = MsiOpenDatabase( msifile, msifile2, &hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to open database\n" );

    res = MsiCloseHandle( hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    ok( INVALID_FILE_ATTRIBUTES == GetFileAttributes( msifile2 ), "uncommitted database should not exist\n");

    res = MsiOpenDatabase( msifile, msifile2, &hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    res = MsiDatabaseCommit( hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    res = MsiCloseHandle( hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    ok( INVALID_FILE_ATTRIBUTES != GetFileAttributes( msifile2 ), "committed database should exist\n");

    res = MsiOpenDatabase( msifile, MSIDBOPEN_READONLY, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to open database\n" );

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    res = MsiOpenDatabase( msifile, MSIDBOPEN_DIRECT, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to open database\n" );

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    res = MsiOpenDatabase( msifile, MSIDBOPEN_TRANSACT, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to open database\n" );

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );
    ok( INVALID_FILE_ATTRIBUTES != GetFileAttributes( msifile ), "database should exist\n");

    /* MSIDBOPEN_CREATE deletes the database if MsiCommitDatabase isn't called */
    res = MsiOpenDatabase( msifile, MSIDBOPEN_CREATE, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to open database\n" );

    ok( INVALID_FILE_ATTRIBUTES != GetFileAttributes( msifile ), "database should exist\n");

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    ok( INVALID_FILE_ATTRIBUTES == GetFileAttributes( msifile ), "database should exist\n");

    res = MsiOpenDatabase( msifile, MSIDBOPEN_CREATE, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to open database\n" );

    res = MsiDatabaseCommit( hdb );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    ok( INVALID_FILE_ATTRIBUTES != GetFileAttributes( msifile ), "database should exist\n");

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    res = DeleteFile( msifile2 );
    ok( res == TRUE, "Failed to delete database\n" );

    res = DeleteFile( msifile );
    ok( res == TRUE, "Failed to delete database\n" );
}

static UINT do_query(MSIHANDLE hdb, const char *query, MSIHANDLE *phrec)
{
    MSIHANDLE hview = 0;
    UINT r, ret;

    if (phrec)
        *phrec = 0;

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

static UINT create_component_table( MSIHANDLE hdb )
{
    return run_query( hdb, 0,
            "CREATE TABLE `Component` ( "
            "`Component` CHAR(72) NOT NULL, "
            "`ComponentId` CHAR(38), "
            "`Directory_` CHAR(72) NOT NULL, "
            "`Attributes` SHORT NOT NULL, "
            "`Condition` CHAR(255), "
            "`KeyPath` CHAR(72) "
            "PRIMARY KEY `Component`)" );
}

static UINT create_custom_action_table( MSIHANDLE hdb )
{
    return run_query( hdb, 0,
            "CREATE TABLE `CustomAction` ( "
            "`Action` CHAR(72) NOT NULL, "
            "`Type` SHORT NOT NULL, "
            "`Source` CHAR(72), "
            "`Target` CHAR(255) "
            "PRIMARY KEY `Action`)" );
}

static UINT create_directory_table( MSIHANDLE hdb )
{
    return run_query( hdb, 0,
            "CREATE TABLE `Directory` ( "
            "`Directory` CHAR(255) NOT NULL, "
            "`Directory_Parent` CHAR(255), "
            "`DefaultDir` CHAR(255) NOT NULL "
            "PRIMARY KEY `Directory`)" );
}

static UINT create_feature_components_table( MSIHANDLE hdb )
{
    return run_query( hdb, 0,
            "CREATE TABLE `FeatureComponents` ( "
            "`Feature_` CHAR(38) NOT NULL, "
            "`Component_` CHAR(72) NOT NULL "
            "PRIMARY KEY `Feature_`, `Component_` )" );
}

static UINT create_std_dlls_table( MSIHANDLE hdb )
{
    return run_query( hdb, 0,
            "CREATE TABLE `StdDlls` ( "
            "`File` CHAR(255) NOT NULL, "
            "`Binary_` CHAR(72) NOT NULL "
            "PRIMARY KEY `File` )" );
}

static UINT create_binary_table( MSIHANDLE hdb )
{
    return run_query( hdb, 0,
            "CREATE TABLE `Binary` ( "
            "`Name` CHAR(72) NOT NULL, "
            "`Data` CHAR(72) NOT NULL "
            "PRIMARY KEY `Name` )" );
}

#define make_add_entry(type, qtext) \
    static UINT add##_##type##_##entry( MSIHANDLE hdb, const char *values ) \
    { \
        char insert[] = qtext; \
        char *query; \
        UINT sz, r; \
        sz = strlen(values) + sizeof insert; \
        query = HeapAlloc(GetProcessHeap(),0,sz); \
        sprintf(query,insert,values); \
        r = run_query( hdb, 0, query ); \
        HeapFree(GetProcessHeap(), 0, query); \
        return r; \
    }

make_add_entry(component,
               "INSERT INTO `Component`  "
               "(`Component`, `ComponentId`, `Directory_`, "
               "`Attributes`, `Condition`, `KeyPath`) VALUES( %s )")

make_add_entry(custom_action,
               "INSERT INTO `CustomAction`  "
               "(`Action`, `Type`, `Source`, `Target`) VALUES( %s )")

make_add_entry(feature_components,
               "INSERT INTO `FeatureComponents` "
               "(`Feature_`, `Component_`) VALUES( %s )")

make_add_entry(std_dlls,
               "INSERT INTO `StdDlls` (`File`, `Binary_`) VALUES( %s )")

make_add_entry(binary,
               "INSERT INTO `Binary` (`Name`, `Data`) VALUES( %s )")

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

    /* now try a few bad INSERT xqueries */
    query = "INSERT INTO `phone` ( `id`, `name`, `number` )"
        "VALUES(?, ?)";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "MsiDatabaseOpenView failed\n");

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

    r = try_query( hdb, "select * from c");
    ok(r == ERROR_SUCCESS , "query failed\n");

    r = try_query( hdb, "select * from c where b = 'x");
    todo_wine ok(r == ERROR_BAD_QUERY_SYNTAX, "query failed\n");

    r = try_query( hdb, "select * from 'c'");
    ok(r == ERROR_BAD_QUERY_SYNTAX, "query failed\n");

    r = try_query( hdb, "select * from ''");
    ok(r == ERROR_BAD_QUERY_SYNTAX, "query failed\n");

    r = try_query( hdb, "select * from c where b = x");
    ok(r == ERROR_BAD_QUERY_SYNTAX, "query failed\n");

    r = try_query( hdb, "select * from c where b = \"x\"");
    ok(r == ERROR_BAD_QUERY_SYNTAX, "query failed\n");

    r = try_query( hdb, "select * from c where b = 'x'");
    ok(r == ERROR_SUCCESS, "query failed\n");

    r = try_query( hdb, "select * from c where b = '\"x'");
    ok(r == ERROR_SUCCESS, "query failed\n");

    if (0)  /* FIXME: this query causes trouble with other tests */
    {
        r = try_query( hdb, "select * from c where b = '\\\'x'");
        ok(r == ERROR_BAD_QUERY_SYNTAX, "query failed\n");
    }

    r = try_query( hdb, "select * from 'c'");
    ok(r == ERROR_BAD_QUERY_SYNTAX, "query failed\n");

    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS , "Failed to close database transact\n");

    r = DeleteFile( msifile );
    ok(r == TRUE, "file didn't exist after commit\n");
}

static void test_viewmodify(void)
{
    MSIHANDLE hdb = 0, hview = 0, hrec = 0;
    UINT r;
    MSIDBERROR err;
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
    err = MsiViewGetError( 0, NULL, &sz );
    ok(err == MSIDBERROR_INVALIDARG, "MsiViewGetError return\n");

    /* open a view */
    query = "SELECT * FROM `phone`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");

    /* see what happens with a good hview and bad args */
    err = MsiViewGetError( hview, NULL, NULL );
    ok(err == MSIDBERROR_INVALIDARG || err == MSIDBERROR_NOERROR,
       "MsiViewGetError returns %u (expected -3)\n", err);
    err = MsiViewGetError( hview, buffer, NULL );
    ok(err == MSIDBERROR_INVALIDARG, "MsiViewGetError return\n");

    /* see what happens with a zero length buffer */
    sz = 0;
    buffer[0] = 'x';
    err = MsiViewGetError( hview, buffer, &sz );
    ok(err == MSIDBERROR_MOREDATA, "MsiViewGetError return\n");
    ok(buffer[0] == 'x', "buffer cleared\n");
    ok(sz == 0, "size not zero\n");

    /* ok this one is strange */
    sz = 0;
    err = MsiViewGetError( hview, NULL, &sz );
    ok(err == MSIDBERROR_NOERROR, "MsiViewGetError return\n");
    ok(sz == 0, "size not zero\n");

    /* see if it really has an error */
    sz = sizeof buffer;
    buffer[0] = 'x';
    err = MsiViewGetError( hview, buffer, &sz );
    ok(err == MSIDBERROR_NOERROR, "MsiViewGetError return\n");
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

    r = MsiRecordSetInteger(hrec, 1, 1);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetString(hrec, 2, "bob");
    ok(r == ERROR_SUCCESS, "failed to set string\n");
    r = MsiRecordSetString(hrec, 3, "7654321");
    ok(r == ERROR_SUCCESS, "failed to set string\n");

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewModify(hview, MSIMODIFY_INSERT_TEMPORARY, hrec );
    ok(r == ERROR_SUCCESS, "MsiViewModify failed\n");

    /* insert the same thing again */
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    /* should fail ... */
    r = MsiViewModify(hview, MSIMODIFY_INSERT_TEMPORARY, hrec );
    ok(r == ERROR_FUNCTION_FAILED, "MsiViewModify failed\n");

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    query = "SELECT * FROM `phone`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);

    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "MsiRecordGetString failed\n");
    ok(!lstrcmp(buffer, "bob"), "Expected bob, got %s\n", buffer);

    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 3, buffer, &sz);
    ok(r == ERROR_SUCCESS, "MsiRecordGetString failed\n");
    ok(!lstrcmp(buffer, "7654321"), "Expected 7654321, got %s\n", buffer);

    /* update the view, non-primary key */
    r = MsiRecordSetString(hrec, 3, "3141592");
    ok(r == ERROR_SUCCESS, "MsiRecordSetString failed\n");

    r = MsiViewModify(hview, MSIMODIFY_UPDATE, hrec);
    ok(r == ERROR_SUCCESS, "MsiViewModify failed\n");

    /* do it again */
    r = MsiViewModify(hview, MSIMODIFY_UPDATE, hrec);
    ok(r == ERROR_SUCCESS, "MsiViewModify failed: %d\n", r);

    /* update the view, primary key */
    r = MsiRecordSetInteger(hrec, 1, 5);
    ok(r == ERROR_SUCCESS, "MsiRecordSetInteger failed\n");

    r = MsiViewModify(hview, MSIMODIFY_UPDATE, hrec);
    ok(r == ERROR_FUNCTION_FAILED, "MsiViewModify failed\n");

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    query = "SELECT * FROM `phone`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);

    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "MsiRecordGetString failed\n");
    ok(!lstrcmp(buffer, "bob"), "Expected bob, got %s\n", buffer);

    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 3, buffer, &sz);
    ok(r == ERROR_SUCCESS, "MsiRecordGetString failed\n");
    ok(!lstrcmp(buffer, "3141592"), "Expected 3141592, got %s\n", buffer);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    /* use a record that doesn't come from a view fetch */
    hrec = MsiCreateRecord(3);
    ok(hrec != 0, "MsiCreateRecord failed\n");

    r = MsiRecordSetInteger(hrec, 1, 3);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetString(hrec, 2, "jane");
    ok(r == ERROR_SUCCESS, "failed to set string\n");
    r = MsiRecordSetString(hrec, 3, "112358");
    ok(r == ERROR_SUCCESS, "failed to set string\n");

    r = MsiViewModify(hview, MSIMODIFY_UPDATE, hrec);
    ok(r == ERROR_FUNCTION_FAILED, "Expected ERROR_FUNCTION_FAILED, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    /* use a record that doesn't come from a view fetch, primary key matches */
    hrec = MsiCreateRecord(3);
    ok(hrec != 0, "MsiCreateRecord failed\n");

    r = MsiRecordSetInteger(hrec, 1, 1);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetString(hrec, 2, "jane");
    ok(r == ERROR_SUCCESS, "failed to set string\n");
    r = MsiRecordSetString(hrec, 3, "112358");
    ok(r == ERROR_SUCCESS, "failed to set string\n");

    r = MsiViewModify(hview, MSIMODIFY_UPDATE, hrec);
    todo_wine
    {
        ok(r == ERROR_FUNCTION_FAILED, "MsiViewModify failed\n");
    }

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    hrec = MsiCreateRecord(3);

    r = MsiRecordSetInteger(hrec, 1, 2);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetString(hrec, 2, "nick");
    ok(r == ERROR_SUCCESS, "failed to set string\n");
    r = MsiRecordSetString(hrec, 3, "141421");
    ok(r == ERROR_SUCCESS, "failed to set string\n");

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewModify(hview, MSIMODIFY_INSERT_TEMPORARY, hrec );
    ok(r == ERROR_SUCCESS, "MsiViewModify failed\n");

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    query = "SELECT * FROM `phone` WHERE `id` = 1";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");

    /* change the id to match the second row */
    r = MsiRecordSetInteger(hrec, 1, 2);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetString(hrec, 2, "jerry");
    ok(r == ERROR_SUCCESS, "failed to set string\n");

    r = MsiViewModify(hview, MSIMODIFY_UPDATE, hrec);
    ok(r == ERROR_FUNCTION_FAILED, "MsiViewModify failed\n");

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");
    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* broader search */
    query = "SELECT * FROM `phone` ORDER BY `id`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");

    /* change the id to match the second row */
    r = MsiRecordSetInteger(hrec, 1, 2);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetString(hrec, 2, "jerry");
    ok(r == ERROR_SUCCESS, "failed to set string\n");

    r = MsiViewModify(hview, MSIMODIFY_UPDATE, hrec);
    ok(r == ERROR_FUNCTION_FAILED, "MsiViewModify failed\n");

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
    ok(len == STRING_LENGTH, "string length wrong\n");

    MsiCloseHandle(hrec);
    MsiCloseHandle(hdb);
    DeleteFile(msifile);
}

static void create_file_data(LPCSTR name, LPCSTR data, DWORD size)
{
    HANDLE file;
    DWORD written;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;

    WriteFile(file, data, strlen(data), &written, NULL);
    WriteFile(file, "\n", strlen("\n"), &written, NULL);

    if (size)
    {
        SetFilePointer(file, size, NULL, FILE_BEGIN);
        SetEndOfFile(file);
    }

    CloseHandle(file);
}

#define create_file(name) create_file_data(name, name, 0)
 
static void test_streamtable(void)
{
    MSIHANDLE hdb = 0, rec, view;
    char file[MAX_PATH];
    char buf[MAX_PATH];
    DWORD size;
    UINT r;

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    r = run_query( hdb, 0,
            "CREATE TABLE `Properties` "
            "( `Property` CHAR(255), `Value` CHAR(1)  PRIMARY KEY `Property`)" );
    ok( r == ERROR_SUCCESS , "Failed to create table\n" );

    r = run_query( hdb, 0,
            "INSERT INTO `Properties` "
            "( `Value`, `Property` ) VALUES ( 'Prop', 'value' )" );
    ok( r == ERROR_SUCCESS, "Failed to add to table\n" );

    r = MsiDatabaseCommit( hdb );
    ok( r == ERROR_SUCCESS , "Failed to commit database\n" );

    MsiCloseHandle( hdb );

    r = MsiOpenDatabase(msifile, MSIDBOPEN_TRANSACT, &hdb );
    ok( r == ERROR_SUCCESS , "Failed to open database\n" );

    /* check the column types */
    rec = get_column_info( hdb, "select * from `_Streams`", MSICOLINFO_TYPES );
    ok( rec, "failed to get column info record\n" );

    ok( check_record( rec, 1, "s62"), "wrong record type\n");
    ok( check_record( rec, 2, "V0"), "wrong record type\n");

    MsiCloseHandle( rec );

    /* now try the names */
    rec = get_column_info( hdb, "select * from `_Streams`", MSICOLINFO_NAMES );
    ok( rec, "failed to get column info record\n" );

    ok( check_record( rec, 1, "Name"), "wrong record type\n");
    ok( check_record( rec, 2, "Data"), "wrong record type\n");

    MsiCloseHandle( rec );

    /* insert a file into the _Streams table */
    create_file( "test.txt" );

    rec = MsiCreateRecord( 2 );
    MsiRecordSetString( rec, 1, "data" );

    r = MsiRecordSetStream( rec, 2, "test.txt" );
    ok( r == ERROR_SUCCESS, "Failed to add stream data to the record: %d\n", r);

    DeleteFile("test.txt");

    r = MsiDatabaseOpenView( hdb,
            "INSERT INTO `_Streams` ( `Name`, `Data` ) VALUES ( ?, ? )", &view );
    ok( r == ERROR_SUCCESS, "Failed to open database view: %d\n", r);

    r = MsiViewExecute( view, rec );
    ok( r == ERROR_SUCCESS, "Failed to execute view: %d\n", r);

    MsiCloseHandle( rec );
    MsiCloseHandle( view );

    r = MsiDatabaseOpenView( hdb,
            "SELECT `Name`, `Data` FROM `_Streams`", &view );
    ok( r == ERROR_SUCCESS, "Failed to open database view: %d\n", r);

    r = MsiViewExecute( view, 0 );
    ok( r == ERROR_SUCCESS, "Failed to execute view: %d\n", r);

    r = MsiViewFetch( view, &rec );
    ok( r == ERROR_SUCCESS, "Failed to fetch record: %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString( rec, 1, file, &size );
    ok( r == ERROR_SUCCESS, "Failed to get string: %d\n", r);
    ok( !lstrcmp(file, "data"), "Expected 'data', got %s\n", file);

    size = MAX_PATH;
    memset(buf, 0, MAX_PATH);
    r = MsiRecordReadStream( rec, 2, buf, &size );
    ok( r == ERROR_SUCCESS, "Failed to get stream: %d\n", r);
    ok( !lstrcmp(buf, "test.txt\n"), "Expected 'test.txt\\n', got %s\n", buf);

    MsiCloseHandle( rec );

    r = MsiViewFetch( view, &rec );
    ok( r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiCloseHandle( view );
    MsiCloseHandle( hdb );
    DeleteFile(msifile);
}

static void test_where(void)
{
    MSIHANDLE hdb = 0, rec, view;
    LPCSTR query;
    UINT r;
    DWORD size;
    CHAR buf[MAX_PATH];
    UINT count;

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

    query = "SELECT `DiskId` FROM `Media` WHERE `LastSequence` >= 1 AND DiskId >= 0";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(view, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    r = MsiViewFetch(view, &rec);
    ok( r == ERROR_SUCCESS, "failed to fetch view: %d\n", r );

    count = MsiRecordGetFieldCount( rec );
    ok( count == 1, "Expected 1 record fields, got %d\n", count );

    size = MAX_PATH;
    r = MsiRecordGetString( rec, 1, buf, &size );
    ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
    ok( !lstrcmp( buf, "2" ),
        "For (row %d, column 1) expected '%d', got %s\n", 0, 2, buf );
    MsiCloseHandle( rec );

    r = MsiViewFetch(view, &rec);
    ok( r == ERROR_SUCCESS, "failed to fetch view: %d\n", r );

    size = MAX_PATH;
    r = MsiRecordGetString( rec, 1, buf, &size );
    ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
    ok( !lstrcmp( buf, "3" ),
        "For (row %d, column 1) expected '%d', got %s\n", 1, 3, buf );
    MsiCloseHandle( rec );

    r = MsiViewFetch(view, &rec);
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiViewClose(view);
    MsiCloseHandle(view);

    MsiCloseHandle( rec );

    rec = 0;
    query = "SELECT * FROM `Media` WHERE `DiskPrompt` IS NULL";
    r = do_query(hdb, query, &rec);
    ok( r == ERROR_SUCCESS, "query failed: %d\n", r );
    MsiCloseHandle( rec );

    rec = MsiCreateRecord(1);
    MsiRecordSetString(rec, 1, "");

    query = "SELECT * FROM `Media` WHERE `DiskPrompt` = ?";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(rec);
    MsiViewClose(view);
    MsiCloseHandle(view);

    MsiCloseHandle( hdb );
    DeleteFile(msifile);
}

static CHAR CURR_DIR[MAX_PATH];

static const CHAR test_data[] = "FirstPrimaryColumn\tSecondPrimaryColumn\tShortInt\tShortIntNullable\tLongInt\tLongIntNullable\tString\tLocalizableString\tLocalizableStringNullable\n"
                                "s255\ti2\ti2\tI2\ti4\tI4\tS255\tS0\ts0\n"
                                "TestTable\tFirstPrimaryColumn\n"
                                "stringage\t5\t2\t\t2147483640\t-2147483640\tanother string\tlocalizable\tduh\n";

static const CHAR two_primary[] = "PrimaryOne\tPrimaryTwo\n"
                                  "s255\ts255\n"
                                  "TwoPrimary\tPrimaryOne\tPrimaryTwo\n"
                                  "papaya\tleaf\n"
                                  "papaya\tflower\n";

static const CHAR endlines1[] = "A\tB\tC\tD\tE\tF\r\n"
                                "s72\ts72\ts72\ts72\ts72\ts72\n"
                                "Table\tA\r\n"
                                "a\tb\tc\td\te\tf\n"
                                "g\th\ti\t\rj\tk\tl\r\n";

static const CHAR endlines2[] = "A\tB\tC\tD\tE\tF\r"
                                "s72\ts72\ts72\ts72\ts72\ts72\n"
                                "Table2\tA\r\n"
                                "a\tb\tc\td\te\tf\n"
                                "g\th\ti\tj\tk\tl\r\n";

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
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_table_to_db(hdb, two_primary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_table_to_db(hdb, endlines1);
    if (r == ERROR_FUNCTION_FAILED)
    {
        /* win9x doesn't handle this case */
        skip("endlines not handled correctly.\n");
        MsiCloseHandle(hdb);
        DeleteFileA(msifile);
        return;
    }

    r = add_table_to_db(hdb, endlines2);
    ok(r == ERROR_FUNCTION_FAILED,
       "Expected ERROR_FUNCTION_FAILED, got %d\n", r);

    query = "SELECT * FROM `TestTable`";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_NAMES, &rec);
    count = MsiRecordGetFieldCount(rec);
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
    MsiCloseHandle(rec);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_TYPES, &rec);
    count = MsiRecordGetFieldCount(rec);
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
    MsiCloseHandle(rec);

    query = "SELECT * FROM `TestTable`";
    r = do_query(hdb, query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(check_record(rec, 1, "stringage"), "Expected 'stringage'\n");
    ok(check_record(rec, 7, "another string"), "Expected 'another string'\n");
    ok(check_record(rec, 8, "localizable"), "Expected 'localizable'\n");
    ok(check_record(rec, 9, "duh"), "Expected 'duh'\n");

    i = MsiRecordGetInteger(rec, 2);
    ok(i == 5, "Expected 5, got %d\n", i);

    i = MsiRecordGetInteger(rec, 3);
    ok(i == 2, "Expected 2, got %d\n", i);

    i = MsiRecordGetInteger(rec, 4);
    ok(i == MSI_NULL_INTEGER, "Expected MSI_NULL_INTEGER, got %d\n", i);

    i = MsiRecordGetInteger(rec, 5);
    ok(i == 2147483640, "Expected 2147483640, got %d\n", i);

    i = MsiRecordGetInteger(rec, 6);
    ok(i == -2147483640, "Expected -2147483640, got %d\n", i);

    MsiCloseHandle(rec);
    MsiCloseHandle(view);

    query = "SELECT * FROM `TwoPrimary`";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_NAMES, &rec);
    count = MsiRecordGetFieldCount(rec);
    ok(count == 2, "Expected 2, got %d\n", count);
    ok(check_record(rec, 1, "PrimaryOne"), "Expected PrimaryOne\n");
    ok(check_record(rec, 2, "PrimaryTwo"), "Expected PrimaryTwo\n");

    MsiCloseHandle(rec);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_TYPES, &rec);
    count = MsiRecordGetFieldCount(rec);
    ok(count == 2, "Expected 2, got %d\n", count);
    ok(check_record(rec, 1, "s255"), "Expected s255\n");
    ok(check_record(rec, 2, "s255"), "Expected s255\n");
    MsiCloseHandle(rec);

    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    ok(check_record(rec, 1, "papaya"), "Expected 'papaya'\n");
    ok(check_record(rec, 2, "leaf"), "Expected 'leaf'\n");

    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    ok(check_record(rec, 1, "papaya"), "Expected 'papaya'\n");
    ok(check_record(rec, 2, "flower"), "Expected 'flower'\n");

    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS,
       "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiViewClose(view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(view);

    query = "SELECT * FROM `Table`";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_NAMES, &rec);
    count = MsiRecordGetFieldCount(rec);
    ok(count == 6, "Expected 6, got %d\n", count);
    ok(check_record(rec, 1, "A"), "Expected A\n");
    ok(check_record(rec, 2, "B"), "Expected B\n");
    ok(check_record(rec, 3, "C"), "Expected C\n");
    ok(check_record(rec, 4, "D"), "Expected D\n");
    ok(check_record(rec, 5, "E"), "Expected E\n");
    ok(check_record(rec, 6, "F"), "Expected F\n");
    MsiCloseHandle(rec);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_TYPES, &rec);
    count = MsiRecordGetFieldCount(rec);
    ok(count == 6, "Expected 6, got %d\n", count);
    ok(check_record(rec, 1, "s72"), "Expected s72\n");
    ok(check_record(rec, 2, "s72"), "Expected s72\n");
    ok(check_record(rec, 3, "s72"), "Expected s72\n");
    ok(check_record(rec, 4, "s72"), "Expected s72\n");
    ok(check_record(rec, 5, "s72"), "Expected s72\n");
    ok(check_record(rec, 6, "s72"), "Expected s72\n");
    MsiCloseHandle(rec);

    MsiViewClose(view);
    MsiCloseHandle(view);

    query = "SELECT * FROM `Table`";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(check_record(rec, 1, "a"), "Expected 'a'\n");
    ok(check_record(rec, 2, "b"), "Expected 'b'\n");
    ok(check_record(rec, 3, "c"), "Expected 'c'\n");
    ok(check_record(rec, 4, "d"), "Expected 'd'\n");
    ok(check_record(rec, 5, "e"), "Expected 'e'\n");
    ok(check_record(rec, 6, "f"), "Expected 'f'\n");

    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(check_record(rec, 1, "g"), "Expected 'g'\n");
    ok(check_record(rec, 2, "h"), "Expected 'h'\n");
    ok(check_record(rec, 3, "i"), "Expected 'i'\n");
    ok(check_record(rec, 4, "j"), "Expected 'j'\n");
    ok(check_record(rec, 5, "k"), "Expected 'k'\n");
    ok(check_record(rec, 6, "l"), "Expected 'l'\n");

    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS,
       "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(view);
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

    /* verify that we just created a table called '?', not 'Fable' */
    r = try_query(hdb, "SELECT * from `Fable`");
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    r = try_query(hdb, "SELECT * from `?`");
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
    MsiCloseHandle(rec);

    /* try column names as markers */
    rec = MsiCreateRecord(2);
    MsiRecordSetString(rec, 1, "One");
    MsiRecordSetString(rec, 2, "Two");
    query = "CREATE TABLE `Mable` ( `?` SHORT NOT NULL, `?` CHAR(255) PRIMARY KEY `One`)";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    MsiCloseHandle(rec);

    /* try names with backticks */
    rec = MsiCreateRecord(3);
    MsiRecordSetString(rec, 1, "One");
    MsiRecordSetString(rec, 2, "Two");
    MsiRecordSetString(rec, 3, "One");
    query = "CREATE TABLE `Mable` ( `?` SHORT NOT NULL, `?` CHAR(255) PRIMARY KEY `?`)";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* try names with backticks, minus definitions */
    query = "CREATE TABLE `Mable` ( `?`, `?` PRIMARY KEY `?`)";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* try names without backticks */
    query = "CREATE TABLE `Mable` ( ? SHORT NOT NULL, ? CHAR(255) PRIMARY KEY ?)";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    MsiCloseHandle(rec);

    /* try one long marker */
    rec = MsiCreateRecord(1);
    MsiRecordSetString(rec, 1, "`One` SHORT NOT NULL, `Two` CHAR(255) PRIMARY KEY `One`");
    query = "CREATE TABLE `Mable` ( ? )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    MsiCloseHandle(rec);

    /* try all names as markers */
    rec = MsiCreateRecord(4);
    MsiRecordSetString(rec, 1, "Mable");
    MsiRecordSetString(rec, 2, "One");
    MsiRecordSetString(rec, 3, "Two");
    MsiRecordSetString(rec, 4, "One");
    query = "CREATE TABLE `?` ( `?` SHORT NOT NULL, `?` CHAR(255) PRIMARY KEY `?`)";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    MsiCloseHandle(rec);

    /* try a legit insert */
    query = "INSERT INTO `Table` ( `One`, `Two` ) VALUES ( 5, 'hello' )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = try_query(hdb, "SELECT * from `Table`");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* try values as markers */
    rec = MsiCreateRecord(2);
    MsiRecordSetInteger(rec, 1, 4);
    MsiRecordSetString(rec, 2, "hi");
    query = "INSERT INTO `Table` ( `One`, `Two` ) VALUES ( ?, '?' )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    MsiCloseHandle(rec);

    /* try column names and values as markers */
    rec = MsiCreateRecord(4);
    MsiRecordSetString(rec, 1, "One");
    MsiRecordSetString(rec, 2, "Two");
    MsiRecordSetInteger(rec, 3, 5);
    MsiRecordSetString(rec, 4, "hi");
    query = "INSERT INTO `Table` ( `?`, `?` ) VALUES ( ?, '?' )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    MsiCloseHandle(rec);

    /* try column names as markers */
    rec = MsiCreateRecord(2);
    MsiRecordSetString(rec, 1, "One");
    MsiRecordSetString(rec, 2, "Two");
    query = "INSERT INTO `Table` ( `?`, `?` ) VALUES ( 3, 'yellow' )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    MsiCloseHandle(rec);

    /* try table name as a marker */
    rec = MsiCreateRecord(1);
    MsiRecordSetString(rec, 1, "Table");
    query = "INSERT INTO `?` ( `One`, `Two` ) VALUES ( 2, 'green' )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    MsiCloseHandle(rec);

    /* try table name and values as markers */
    rec = MsiCreateRecord(3);
    MsiRecordSetString(rec, 1, "Table");
    MsiRecordSetInteger(rec, 2, 10);
    MsiRecordSetString(rec, 3, "haha");
    query = "INSERT INTO `?` ( `One`, `Two` ) VALUES ( ?, '?' )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_FUNCTION_FAILED, "Expected ERROR_FUNCTION_FAILED, got %d\n", r);
    MsiCloseHandle(rec);

    /* try all markers */
    rec = MsiCreateRecord(5);
    MsiRecordSetString(rec, 1, "Table");
    MsiRecordSetString(rec, 1, "One");
    MsiRecordSetString(rec, 1, "Two");
    MsiRecordSetInteger(rec, 2, 10);
    MsiRecordSetString(rec, 3, "haha");
    query = "INSERT INTO `?` ( `?`, `?` ) VALUES ( ?, '?' )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    MsiCloseHandle(rec);

    /* insert an integer as a string */
    rec = MsiCreateRecord(2);
    MsiRecordSetString(rec, 1, "11");
    MsiRecordSetString(rec, 2, "hi");
    query = "INSERT INTO `Table` ( `One`, `Two` ) VALUES ( ?, '?' )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    MsiCloseHandle(rec);

    /* leave off the '' for the string */
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

#define MY_NVIEWS 4000    /* Largest installer I've seen uses < 2k */
static void test_handle_limit(void)
{
    int i;
    MSIHANDLE hdb;
    MSIHANDLE hviews[MY_NVIEWS];
    UINT r;

    /* create an empty db */
    hdb = create_db();
    ok( hdb, "failed to create db\n");

    memset(hviews, 0, sizeof(hviews));

    for (i=0; i<MY_NVIEWS; i++) {
        static char szQueryBuf[256] = "SELECT * from `_Tables`";
        hviews[i] = 0xdeadbeeb;
        r = MsiDatabaseOpenView(hdb, szQueryBuf, &hviews[i]);
        if( r != ERROR_SUCCESS || hviews[i] == 0xdeadbeeb || 
            hviews[i] == 0 || (i && (hviews[i] == hviews[i-1])))
            break;
    }

    ok( i == MY_NVIEWS, "problem opening views\n");

    for (i=0; i<MY_NVIEWS; i++) {
        if (hviews[i] != 0 && hviews[i] != 0xdeadbeeb) {
            r = MsiCloseHandle(hviews[i]);
            if (r != ERROR_SUCCESS)
                break;
        }
    }

    ok( i == MY_NVIEWS, "problem closing views\n");

    r = MsiCloseHandle(hdb);
    ok( r == ERROR_SUCCESS, "failed to close database\n");
}

static void generate_transform(void)
{
    MSIHANDLE hdb1, hdb2, hrec;
    LPCSTR query;
    UINT r;

    /* start with two identical databases */
    CopyFile(msifile2, msifile, FALSE);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_TRANSACT, &hdb1 );
    ok( r == ERROR_SUCCESS , "Failed to create database\n" );

    r = MsiDatabaseCommit( hdb1 );
    ok( r == ERROR_SUCCESS , "Failed to commit database\n" );

    r = MsiOpenDatabase(msifile2, MSIDBOPEN_READONLY, &hdb2 );
    ok( r == ERROR_SUCCESS , "Failed to create database\n" );

    /* the transform between two identical database should be empty */
    r = MsiDatabaseGenerateTransform(hdb1, hdb2, NULL, 0, 0);
    todo_wine {
    ok( r == ERROR_NO_DATA, "return code %d, should be ERROR_NO_DATA\n", r );
    }

    query = "CREATE TABLE `AAR` ( `BAR` SHORT NOT NULL, `CAR` CHAR(255) PRIMARY KEY `CAR`)";
    r = run_query(hdb1, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    query = "INSERT INTO `AAR` ( `BAR`, `CAR` ) VALUES ( 1, 'vw' )";
    r = run_query(hdb1, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add row 1\n");

    query = "INSERT INTO `AAR` ( `BAR`, `CAR` ) VALUES ( 2, 'bmw' )";
    r = run_query(hdb1, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add row 2\n");

    query = "UPDATE `MOO` SET `OOO` = 'c' WHERE `NOO` = 1";
    r = run_query(hdb1, 0, query);
    ok(r == ERROR_SUCCESS, "failed to modify row\n");

    query = "DELETE FROM `MOO` WHERE `NOO` = 3";
    r = run_query(hdb1, 0, query);
    ok(r == ERROR_SUCCESS, "failed to delete row\n");

    hrec = MsiCreateRecord(2);
    r = MsiRecordSetInteger(hrec, 1, 1);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");

    write_file("testdata.bin", "naengmyon", 9);
    r = MsiRecordSetStream(hrec, 2, "testdata.bin");
    ok(r == ERROR_SUCCESS, "failed to set stream\n");

    query = "INSERT INTO `BINARY` ( `ID`, `BLOB` ) VALUES ( ?, ? )";
    r = run_query(hdb1, hrec, query);
    ok(r == ERROR_SUCCESS, "failed to add row with blob\n");

    MsiCloseHandle(hrec);

    query = "ALTER TABLE `MOO` ADD `COW` INTEGER";
    r = run_query(hdb1, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add column\n");

    query = "ALTER TABLE `MOO` ADD `PIG` INTEGER";
    r = run_query(hdb1, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add column\n");

    query = "UPDATE `MOO` SET `PIG` = 5 WHERE `NOO` = 1";
    r = run_query(hdb1, 0, query);
    ok(r == ERROR_SUCCESS, "failed to modify row\n");

    query = "CREATE TABLE `Property` ( `Property` CHAR(72) NOT NULL, "
            "`Value` CHAR(0) PRIMARY KEY `Property`)";
    r = run_query(hdb1, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add property table\n");

    query = "INSERT INTO `Property` ( `Property`, `Value` ) VALUES ( 'prop', 'val' )";
    r = run_query(hdb1, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add property\n");

    /* database needs to be committed */
    MsiDatabaseCommit(hdb1);

    r = MsiDatabaseGenerateTransform(hdb1, hdb2, mstfile, 0, 0);
    ok( r == ERROR_SUCCESS, "return code %d, should be ERROR_SUCCESS\n", r );

    MsiCloseHandle( hdb1 );
    MsiCloseHandle( hdb2 );

    DeleteFile("testdata.bin");
}

/* data for generating a transform */

/* tables transform names - encoded as they would be in an msi database file */
static const WCHAR name1[] = { 0x4840, 0x3a8a, 0x481b, 0 }; /* AAR */
static const WCHAR name2[] = { 0x4840, 0x3b3f, 0x43f2, 0x4438, 0x45b1, 0 }; /* _Columns */
static const WCHAR name3[] = { 0x4840, 0x3f7f, 0x4164, 0x422f, 0x4836, 0 }; /* _Tables */
static const WCHAR name4[] = { 0x4840, 0x3f3f, 0x4577, 0x446c, 0x3b6a, 0x45e4, 0x4824, 0 }; /* _StringData */
static const WCHAR name5[] = { 0x4840, 0x3f3f, 0x4577, 0x446c, 0x3e6a, 0x44b2, 0x482f, 0 }; /* _StringPool */
static const WCHAR name6[] = { 0x4840, 0x3e16, 0x4818, 0}; /* MOO */
static const WCHAR name7[] = { 0x4840, 0x3c8b, 0x3a97, 0x409b, 0 }; /* BINARY */
static const WCHAR name8[] = { 0x3c8b, 0x3a97, 0x409b, 0x387e, 0 }; /* BINARY.1 */
static const WCHAR name9[] = { 0x4840, 0x4559, 0x44f2, 0x4568, 0x4737, 0 }; /* Property */

/* data in each table */
static const WCHAR data1[] = { /* AAR */
    0x0201, 0x0008, 0x8001,  /* 0x0201 = add row (1), two shorts */
    0x0201, 0x0009, 0x8002,
};
static const WCHAR data2[] = { /* _Columns */
    0x0401, 0x0001, 0x8003, 0x0002, 0x9502,
    0x0401, 0x0001, 0x8004, 0x0003, 0x9502,
    0x0401, 0x0005, 0x0000, 0x0006, 0xbdff,  /* 0x0401 = add row (1), 4 shorts */
    0x0401, 0x0005, 0x0000, 0x0007, 0x8502,
    0x0401, 0x000a, 0x0000, 0x000a, 0xad48,
    0x0401, 0x000a, 0x0000, 0x000b, 0x9d00,
};
static const WCHAR data3[] = { /* _Tables */
    0x0101, 0x0005, /* 0x0101 = add row (1), 1 short */
    0x0101, 0x000a,
};
static const char data4[] = /* _StringData */
    "MOOCOWPIGcAARCARBARvwbmwPropertyValuepropval";  /* all the strings squashed together */
static const WCHAR data5[] = { /* _StringPool */
/*  len, refs */
    0,   0,    /* string 0 ''    */
    3,   2,    /* string 1 'MOO' */
    3,   1,    /* string 2 'COW' */
    3,   1,    /* string 3 'PIG' */
    1,   1,    /* string 4 'c'   */
    3,   3,    /* string 5 'AAR' */
    3,   1,    /* string 6 'CAR' */
    3,   1,    /* string 7 'BAR' */
    2,   1,    /* string 8 'vw'  */
    3,   1,    /* string 9 'bmw' */
    8,   4,    /* string 10 'Property' */
    5,   1,    /* string 11 'Value' */
    4,   1,    /* string 12 'prop' */
    3,   1,    /* string 13 'val' */
};
/* update row, 0x0002 is a bitmask of present column data, keys are excluded */
static const WCHAR data6[] = { /* MOO */
    0x000a, 0x8001, 0x0004, 0x8005, /* update row */
    0x0000, 0x8003,         /* delete row */
};

static const WCHAR data7[] = { /* BINARY */
    0x0201, 0x8001, 0x0001,
};

static const char data8[] =  /* stream data for the BINARY table */
    "naengmyon";

static const WCHAR data9[] = { /* Property */
    0x0201, 0x000c, 0x000d,
};

static const struct {
    LPCWSTR name;
    const void *data;
    DWORD size;
} table_transform_data[] =
{
    { name1, data1, sizeof data1 },
    { name2, data2, sizeof data2 },
    { name3, data3, sizeof data3 },
    { name4, data4, sizeof data4 - 1 },
    { name5, data5, sizeof data5 },
    { name6, data6, sizeof data6 },
    { name7, data7, sizeof data7 },
    { name8, data8, sizeof data8 - 1 },
    { name9, data9, sizeof data9 },
};

#define NUM_TRANSFORM_TABLES (sizeof table_transform_data/sizeof table_transform_data[0])

static void generate_transform_manual(void)
{
    IStorage *stg = NULL;
    IStream *stm;
    WCHAR name[0x20];
    HRESULT r;
    DWORD i, count;
    const DWORD mode = STGM_CREATE|STGM_READWRITE|STGM_DIRECT|STGM_SHARE_EXCLUSIVE;

    const CLSID CLSID_MsiTransform = { 0xc1082,0,0,{0xc0,0,0,0,0,0,0,0x46}};

    MultiByteToWideChar(CP_ACP, 0, mstfile, -1, name, 0x20);

    r = StgCreateDocfile(name, mode, 0, &stg);
    ok(r == S_OK, "failed to create storage\n");
    if (!stg)
        return;

    r = IStorage_SetClass( stg, &CLSID_MsiTransform );
    ok(r == S_OK, "failed to set storage type\n");

    for (i=0; i<NUM_TRANSFORM_TABLES; i++)
    {
        r = IStorage_CreateStream( stg, table_transform_data[i].name,
                            STGM_WRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stm );
        if (FAILED(r))
        {
            ok(0, "failed to create stream %08x\n", r);
            continue;
        }

        r = IStream_Write( stm, table_transform_data[i].data,
                          table_transform_data[i].size, &count );
        if (FAILED(r) || count != table_transform_data[i].size)
            ok(0, "failed to write stream\n");
        IStream_Release(stm);
    }

    IStorage_Release(stg);
}

static UINT set_summary_info(MSIHANDLE hdb)
{
    UINT res;
    MSIHANDLE suminfo;

    /* build summary info */
    res = MsiGetSummaryInformation(hdb, NULL, 7, &suminfo);
    ok( res == ERROR_SUCCESS , "Failed to open summaryinfo\n" );

    res = MsiSummaryInfoSetProperty(suminfo,2, VT_LPSTR, 0,NULL,
                        "Installation Database");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo,3, VT_LPSTR, 0,NULL,
                        "Installation Database");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo,4, VT_LPSTR, 0,NULL,
                        "Wine Hackers");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo,7, VT_LPSTR, 0,NULL,
                    ";1033");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo,9, VT_LPSTR, 0,NULL,
                    "{913B8D18-FBB6-4CAC-A239-C74C11E3FA74}");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo, 14, VT_I4, 100, NULL, NULL);
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetProperty(suminfo, 15, VT_I4, 0, NULL, NULL);
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoPersist(suminfo);
    ok( res == ERROR_SUCCESS , "Failed to make summary info persist\n" );

    res = MsiCloseHandle( suminfo);
    ok( res == ERROR_SUCCESS , "Failed to close suminfo\n" );

    return res;
}

static MSIHANDLE create_package_db(LPCSTR filename)
{
    MSIHANDLE hdb = 0;
    UINT res;

    DeleteFile(msifile);

    /* create an empty database */
    res = MsiOpenDatabase(filename, MSIDBOPEN_CREATE, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to create database\n" );
    if( res != ERROR_SUCCESS )
        return hdb;

    res = MsiDatabaseCommit( hdb );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    res = set_summary_info(hdb);

    res = create_directory_table(hdb);
    ok( res == ERROR_SUCCESS , "Failed to create directory table\n" );

    return hdb;
}

static MSIHANDLE package_from_db(MSIHANDLE hdb)
{
    UINT res;
    CHAR szPackage[10];
    MSIHANDLE hPackage;

    sprintf(szPackage,"#%i",hdb);
    res = MsiOpenPackage(szPackage,&hPackage);
    if (res != ERROR_SUCCESS)
        return 0;

    res = MsiCloseHandle(hdb);
    if (res != ERROR_SUCCESS)
        return 0;

    return hPackage;
}

static void test_try_transform(void)
{
    MSIHANDLE hdb, hview, hrec, hpkg;
    LPCSTR query;
    UINT r;
    DWORD sz;
    char buffer[MAX_PATH];

    DeleteFile(msifile);
    DeleteFile(mstfile);

    /* create the database */
    hdb = create_package_db(msifile);
    ok(hdb, "Failed to create package db\n");

    query = "CREATE TABLE `MOO` ( `NOO` SHORT NOT NULL, `OOO` CHAR(255) PRIMARY KEY `NOO`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    query = "INSERT INTO `MOO` ( `NOO`, `OOO` ) VALUES ( 1, 'a' )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add row\n");

    query = "INSERT INTO `MOO` ( `NOO`, `OOO` ) VALUES ( 2, 'b' )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add row\n");

    query = "INSERT INTO `MOO` ( `NOO`, `OOO` ) VALUES ( 3, 'c' )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add row\n");

    query = "CREATE TABLE `BINARY` ( `ID` SHORT NOT NULL, `BLOB` OBJECT PRIMARY KEY `ID`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    hrec = MsiCreateRecord(2);
    r = MsiRecordSetInteger(hrec, 1, 2);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");

    write_file("testdata.bin", "lamyon", 6);
    r = MsiRecordSetStream(hrec, 2, "testdata.bin");
    ok(r == ERROR_SUCCESS, "failed to set stream\n");

    query = "INSERT INTO `BINARY` ( `ID`, `BLOB` ) VALUES ( ?, ? )";
    r = run_query(hdb, hrec, query);
    ok(r == ERROR_SUCCESS, "failed to add row with blob\n");

    MsiCloseHandle(hrec);

    r = MsiDatabaseCommit( hdb );
    ok( r == ERROR_SUCCESS , "Failed to commit database\n" );

    MsiCloseHandle( hdb );
    DeleteFileA("testdata.bin");

    /*
     * Both these generate an equivalent transform,
     *  but the first doesn't work in Wine yet
     *  because MsiDatabaseGenerateTransform is unimplemented.
     */
    if (0)
        generate_transform();
    else
        generate_transform_manual();

    r = MsiOpenDatabase(msifile, MSIDBOPEN_DIRECT, &hdb );
    ok( r == ERROR_SUCCESS , "Failed to create database\n" );

    r = MsiDatabaseApplyTransform( hdb, mstfile, 0 );
    ok( r == ERROR_SUCCESS, "return code %d, should be ERROR_SUCCESS\n", r );

    MsiDatabaseCommit( hdb );

    /* check new values */
    hrec = 0;
    query = "select `BAR`,`CAR` from `AAR` where `BAR` = 1 AND `CAR` = 'vw'";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_SUCCESS, "select query failed\n");
    MsiCloseHandle(hrec);

    query = "select `BAR`,`CAR` from `AAR` where `BAR` = 2 AND `CAR` = 'bmw'";
    hrec = 0;
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_SUCCESS, "select query failed\n");
    MsiCloseHandle(hrec);

    /* check updated values */
    hrec = 0;
    query = "select `NOO`,`OOO` from `MOO` where `NOO` = 1 AND `OOO` = 'c'";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_SUCCESS, "select query failed\n");
    MsiCloseHandle(hrec);

    /* check unchanged value */
    hrec = 0;
    query = "select `NOO`,`OOO` from `MOO` where `NOO` = 2 AND `OOO` = 'b'";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_SUCCESS, "select query failed\n");
    MsiCloseHandle(hrec);

    /* check deleted value */
    hrec = 0;
    query = "select * from `MOO` where `NOO` = 3";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "select query failed\n");
    if (hrec) MsiCloseHandle(hrec);

    /* check added stream */
    hrec = 0;
    query = "select `BLOB` from `BINARY` where `ID` = 1";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_SUCCESS, "select query failed\n");

    /* check the contents of the stream */
    sz = sizeof buffer;
    r = MsiRecordReadStream( hrec, 1, buffer, &sz );
    ok(r == ERROR_SUCCESS, "read stream failed\n");
    ok(!memcmp(buffer, "naengmyon", 9), "stream data was wrong\n");
    ok(sz == 9, "stream data was wrong size\n");
    if (hrec) MsiCloseHandle(hrec);

    /* check the validity of the table with a deleted row */
    hrec = 0;
    query = "select * from `MOO`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "open view failed\n");

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "view execute failed\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "view fetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);

    sz = sizeof buffer;
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "record get string failed\n");
    ok(!lstrcmpA(buffer, "c"), "Expected c, got %s\n", buffer);

    r = MsiRecordGetInteger(hrec, 3);
    ok(r == 0x80000000, "Expected 0x80000000, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 4);
    ok(r == 5, "Expected 5, got %d\n", r);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "view fetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 2, "Expected 2, got %d\n", r);

    sz = sizeof buffer;
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "record get string failed\n");
    ok(!lstrcmpA(buffer, "b"), "Expected b, got %s\n", buffer);

    r = MsiRecordGetInteger(hrec, 3);
    ok(r == 0x80000000, "Expected 0x80000000, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 4);
    ok(r == 0x80000000, "Expected 0x80000000, got %d\n", r);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "view fetch succeeded\n");

    MsiCloseHandle(hrec);
    MsiCloseHandle(hview);

    /* check that the property was added */
    hpkg = package_from_db(hdb);
    ok(hpkg != 0, "Expected non-NULL hpkg\n");

    sz = MAX_PATH;
    r = MsiGetProperty(hpkg, "prop", buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(buffer, "val"), "Expected val, got %s\n", buffer);

    MsiCloseHandle(hpkg);
    MsiCloseHandle(hdb);

    DeleteFile(msifile);
    DeleteFile(mstfile);
}

struct join_res
{
    const CHAR one[MAX_PATH];
    const CHAR two[MAX_PATH];
};

struct join_res_4col
{
    const CHAR one[MAX_PATH];
    const CHAR two[MAX_PATH];
    const CHAR three[MAX_PATH];
    const CHAR four[MAX_PATH];
};

struct join_res_uint
{
    UINT one;
    UINT two;
    UINT three;
    UINT four;
    UINT five;
    UINT six;
};

static const struct join_res join_res_first[] =
{
    { "alveolar", "procerus" },
    { "septum", "procerus" },
    { "septum", "nasalis" },
    { "ramus", "nasalis" },
    { "malar", "mentalis" },
};

static const struct join_res join_res_second[] =
{
    { "nasal", "septum" },
    { "mandible", "ramus" },
};

static const struct join_res join_res_third[] =
{
    { "msvcp.dll", "abcdefgh" },
    { "msvcr.dll", "ijklmnop" },
};

static const struct join_res join_res_fourth[] =
{
    { "msvcp.dll.01234", "single.dll.31415" },
};

static const struct join_res join_res_fifth[] =
{
    { "malar", "procerus" },
};

static const struct join_res join_res_sixth[] =
{
    { "malar", "procerus" },
    { "malar", "procerus" },
    { "malar", "nasalis" },
    { "malar", "nasalis" },
    { "malar", "nasalis" },
    { "malar", "mentalis" },
};

static const struct join_res join_res_seventh[] =
{
    { "malar", "nasalis" },
    { "malar", "nasalis" },
    { "malar", "nasalis" },
};

static const struct join_res_4col join_res_eighth[] =
{
    { "msvcp.dll", "msvcp.dll.01234", "msvcp.dll.01234", "abcdefgh" },
    { "msvcr.dll", "msvcr.dll.56789", "msvcp.dll.01234", "abcdefgh" },
    { "msvcp.dll", "msvcp.dll.01234", "msvcr.dll.56789", "ijklmnop" },
    { "msvcr.dll", "msvcr.dll.56789", "msvcr.dll.56789", "ijklmnop" },
    { "msvcp.dll", "msvcp.dll.01234", "single.dll.31415", "msvcp.dll" },
    { "msvcr.dll", "msvcr.dll.56789", "single.dll.31415", "msvcp.dll" },
};

static const struct join_res_uint join_res_ninth[] =
{
    { 1, 2, 3, 4, 7, 8 },
    { 1, 2, 5, 6, 7, 8 },
    { 1, 2, 3, 4, 9, 10 },
    { 1, 2, 5, 6, 9, 10 },
    { 1, 2, 3, 4, 11, 12 },
    { 1, 2, 5, 6, 11, 12 },
};

static void test_join(void)
{
    MSIHANDLE hdb, hview, hrec;
    LPCSTR query;
    CHAR buf[MAX_PATH];
    UINT r, count;
    DWORD size, i;
    BOOL data_correct;

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    r = create_component_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Component table: %d\n", r );

    r = add_component_entry( hdb, "'zygomatic', 'malar', 'INSTALLDIR', 0, '', ''" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = add_component_entry( hdb, "'maxilla', 'alveolar', 'INSTALLDIR', 0, '', ''" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = add_component_entry( hdb, "'nasal', 'septum', 'INSTALLDIR', 0, '', ''" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = add_component_entry( hdb, "'mandible', 'ramus', 'INSTALLDIR', 0, '', ''" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = create_feature_components_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create FeatureComponents table: %d\n", r );

    r = add_feature_components_entry( hdb, "'procerus', 'maxilla'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'procerus', 'nasal'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'nasalis', 'nasal'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'nasalis', 'mandible'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'nasalis', 'notacomponent'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'mentalis', 'zygomatic'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = create_std_dlls_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create StdDlls table: %d\n", r );

    r = add_std_dlls_entry( hdb, "'msvcp.dll', 'msvcp.dll.01234'" );
    ok( r == ERROR_SUCCESS, "cannot add std dlls: %d\n", r );

    r = add_std_dlls_entry( hdb, "'msvcr.dll', 'msvcr.dll.56789'" );
    ok( r == ERROR_SUCCESS, "cannot add std dlls: %d\n", r );

    r = create_binary_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Binary table: %d\n", r );

    r = add_binary_entry( hdb, "'msvcp.dll.01234', 'abcdefgh'" );
    ok( r == ERROR_SUCCESS, "cannot add binary: %d\n", r );

    r = add_binary_entry( hdb, "'msvcr.dll.56789', 'ijklmnop'" );
    ok( r == ERROR_SUCCESS, "cannot add binary: %d\n", r );

    r = add_binary_entry( hdb, "'single.dll.31415', 'msvcp.dll'" );
    ok( r == ERROR_SUCCESS, "cannot add binary: %d\n", r );

    query = "CREATE TABLE `One` (`A` SHORT, `B` SHORT PRIMARY KEY `A`)";
    r = run_query( hdb, 0, query);
    ok(r == ERROR_SUCCESS, "cannot create table: %d\n", r );

    query = "CREATE TABLE `Two` (`C` SHORT, `D` SHORT PRIMARY KEY `C`)";
    r = run_query( hdb, 0, query);
    ok(r == ERROR_SUCCESS, "cannot create table: %d\n", r );

    query = "CREATE TABLE `Three` (`E` SHORT, `F` SHORT PRIMARY KEY `E`)";
    r = run_query( hdb, 0, query);
    ok(r == ERROR_SUCCESS, "cannot create table: %d\n", r );

    query = "INSERT INTO `One` (`A`, `B`) VALUES (1, 2)";
    r = run_query( hdb, 0, query);
    ok(r == ERROR_SUCCESS, "cannot insert into table: %d\n", r );

    query = "INSERT INTO `Two` (`C`, `D`) VALUES (3, 4)";
    r = run_query( hdb, 0, query);
    ok(r == ERROR_SUCCESS, "cannot insert into table: %d\n", r );

    query = "INSERT INTO `Two` (`C`, `D`) VALUES (5, 6)";
    r = run_query( hdb, 0, query);
    ok(r == ERROR_SUCCESS, "cannot insert into table: %d\n", r );

    query = "INSERT INTO `Three` (`E`, `F`) VALUES (7, 8)";
    r = run_query( hdb, 0, query);
    ok(r == ERROR_SUCCESS, "cannot insert into table: %d\n", r );

    query = "INSERT INTO `Three` (`E`, `F`) VALUES (9, 10)";
    r = run_query( hdb, 0, query);
    ok(r == ERROR_SUCCESS, "cannot insert into table: %d\n", r );

    query = "INSERT INTO `Three` (`E`, `F`) VALUES (11, 12)";
    r = run_query( hdb, 0, query);
    ok(r == ERROR_SUCCESS, "cannot insert into table: %d\n", r );

    query = "CREATE TABLE `Four` (`G` SHORT, `H` SHORT PRIMARY KEY `G`)";
    r = run_query( hdb, 0, query);
    ok(r == ERROR_SUCCESS, "cannot create table: %d\n", r );

    query = "CREATE TABLE `Five` (`I` SHORT, `J` SHORT PRIMARY KEY `I`)";
    r = run_query( hdb, 0, query);
    ok(r == ERROR_SUCCESS, "cannot create table: %d\n", r );

    query = "INSERT INTO `Five` (`I`, `J`) VALUES (13, 14)";
    r = run_query( hdb, 0, query);
    ok(r == ERROR_SUCCESS, "cannot insert into table: %d\n", r );

    query = "INSERT INTO `Five` (`I`, `J`) VALUES (15, 16)";
    r = run_query( hdb, 0, query);
    ok(r == ERROR_SUCCESS, "cannot insert into table: %d\n", r );

    query = "SELECT `Component`.`ComponentId`, `FeatureComponents`.`Feature_` "
            "FROM `Component`, `FeatureComponents` "
            "WHERE `Component`.`Component` = `FeatureComponents`.`Component_` "
            "ORDER BY `Feature_`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        count = MsiRecordGetFieldCount( hrec );
        ok( count == 2, "Expected 2 record fields, got %d\n", count );

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 1, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        ok( !lstrcmp( buf, join_res_first[i].one ),
            "For (row %d, column 1) expected '%s', got %s\n", i, join_res_first[i].one, buf );

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 2, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        ok( !lstrcmp( buf, join_res_first[i].two ),
            "For (row %d, column 2) expected '%s', got %s\n", i, join_res_first[i].two, buf );

        i++;
        MsiCloseHandle(hrec);
    }

    ok( i == 5, "Expected 5 rows, got %d\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    /* try a join without a WHERE condition */
    query = "SELECT `Component`.`ComponentId`, `FeatureComponents`.`Feature_` "
            "FROM `Component`, `FeatureComponents` ";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        i++;
        MsiCloseHandle(hrec);
    }
    ok( i == 24, "Expected 24 rows, got %d\n", i );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT DISTINCT Component, ComponentId FROM FeatureComponents, Component "
            "WHERE FeatureComponents.Component_=Component.Component "
            "AND (Feature_='nasalis') ORDER BY Feature_";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    data_correct = TRUE;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        count = MsiRecordGetFieldCount( hrec );
        ok( count == 2, "Expected 2 record fields, got %d\n", count );

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 1, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( lstrcmp( buf, join_res_second[i].one ))
            data_correct = FALSE;

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 2, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( lstrcmp( buf, join_res_second[i].two ))
            data_correct = FALSE;

        i++;
        MsiCloseHandle(hrec);
    }

    ok( data_correct, "data returned in the wrong order\n");

    ok( i == 2, "Expected 2 rows, got %d\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT `StdDlls`.`File`, `Binary`.`Data` "
            "FROM `StdDlls`, `Binary` "
            "WHERE `StdDlls`.`Binary_` = `Binary`.`Name` "
            "ORDER BY `File`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    data_correct = TRUE;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        count = MsiRecordGetFieldCount( hrec );
        ok( count == 2, "Expected 2 record fields, got %d\n", count );

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 1, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( lstrcmp( buf, join_res_third[i].one ) )
            data_correct = FALSE;

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 2, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( lstrcmp( buf, join_res_third[i].two ) )
            data_correct = FALSE;

        i++;
        MsiCloseHandle(hrec);
    }
    ok( data_correct, "data returned in the wrong order\n");

    ok( i == 2, "Expected 2 rows, got %d\n", i );

    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT `StdDlls`.`Binary_`, `Binary`.`Name` "
            "FROM `StdDlls`, `Binary` "
            "WHERE `StdDlls`.`File` = `Binary`.`Data` "
            "ORDER BY `Name`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    data_correct = TRUE;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        count = MsiRecordGetFieldCount( hrec );
        ok( count == 2, "Expected 2 record fields, got %d\n", count );

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 1, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( lstrcmp( buf, join_res_fourth[i].one ))
            data_correct = FALSE;

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 2, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( lstrcmp( buf, join_res_fourth[i].two ))
            data_correct = FALSE;

        i++;
        MsiCloseHandle(hrec);
    }
    ok( data_correct, "data returned in the wrong order\n");

    ok( i == 1, "Expected 1 rows, got %d\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT `Component`.`ComponentId`, `FeatureComponents`.`Feature_` "
            "FROM `Component`, `FeatureComponents` "
            "WHERE `Component`.`Component` = 'zygomatic' "
            "AND `FeatureComponents`.`Component_` = 'maxilla' "
            "ORDER BY `Feature_`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    data_correct = TRUE;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        count = MsiRecordGetFieldCount( hrec );
        ok( count == 2, "Expected 2 record fields, got %d\n", count );

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 1, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( lstrcmp( buf, join_res_fifth[i].one ))
            data_correct = FALSE;

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 2, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( lstrcmp( buf, join_res_fifth[i].two ))
            data_correct = FALSE;

        i++;
        MsiCloseHandle(hrec);
    }
    ok( data_correct, "data returned in the wrong order\n");

    ok( i == 1, "Expected 1 rows, got %d\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT `Component`.`ComponentId`, `FeatureComponents`.`Feature_` "
            "FROM `Component`, `FeatureComponents` "
            "WHERE `Component` = 'zygomatic' "
            "ORDER BY `Feature_`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    data_correct = TRUE;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        count = MsiRecordGetFieldCount( hrec );
        ok( count == 2, "Expected 2 record fields, got %d\n", count );

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 1, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( lstrcmp( buf, join_res_sixth[i].one ))
            data_correct = FALSE;

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 2, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( lstrcmp( buf, join_res_sixth[i].two ))
            data_correct = FALSE;

        i++;
        MsiCloseHandle(hrec);
    }
    ok( data_correct, "data returned in the wrong order\n");

    ok( i == 6, "Expected 6 rows, got %d\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT `Component`.`ComponentId`, `FeatureComponents`.`Feature_` "
            "FROM `Component`, `FeatureComponents` "
            "WHERE `Component` = 'zygomatic' "
            "AND `Feature_` = 'nasalis' "
            "ORDER BY `Feature_`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    data_correct = TRUE;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        count = MsiRecordGetFieldCount( hrec );
        ok( count == 2, "Expected 2 record fields, got %d\n", count );

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 1, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( lstrcmp( buf, join_res_seventh[i].one ))
            data_correct = FALSE;

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 2, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( lstrcmp( buf, join_res_seventh[i].two ))
            data_correct = FALSE;

        i++;
        MsiCloseHandle(hrec);
    }

    ok( data_correct, "data returned in the wrong order\n");
    ok( i == 3, "Expected 3 rows, got %d\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT `StdDlls`.`File`, `Binary`.`Data` "
            "FROM `StdDlls`, `Binary` ";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    data_correct = TRUE;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        count = MsiRecordGetFieldCount( hrec );
        ok( count == 2, "Expected 2 record fields, got %d\n", count );

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 1, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( lstrcmp( buf, join_res_eighth[i].one ))
            data_correct = FALSE;

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 2, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( lstrcmp( buf, join_res_eighth[i].four ))
            data_correct = FALSE;

        i++;
        MsiCloseHandle(hrec);
    }

    ok( data_correct, "data returned in the wrong order\n");
    ok( i == 6, "Expected 6 rows, got %d\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT * FROM `StdDlls`, `Binary` ";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    data_correct = TRUE;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        count = MsiRecordGetFieldCount( hrec );
        ok( count == 4, "Expected 4 record fields, got %d\n", count );

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 1, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( lstrcmp( buf, join_res_eighth[i].one ))
            data_correct = FALSE;

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 2, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( lstrcmp( buf, join_res_eighth[i].two ))
            data_correct = FALSE;

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 3, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( lstrcmp( buf, join_res_eighth[i].three ))
            data_correct = FALSE;

        size = MAX_PATH;
        r = MsiRecordGetString( hrec, 4, buf, &size );
        ok( r == ERROR_SUCCESS, "failed to get record string: %d\n", r );
        if( lstrcmp( buf, join_res_eighth[i].four ))
            data_correct = FALSE;

        i++;
        MsiCloseHandle(hrec);
    }
    ok( data_correct, "data returned in the wrong order\n");

    ok( i == 6, "Expected 6 rows, got %d\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT * FROM `One`, `Two`, `Three` ";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    data_correct = TRUE;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        count = MsiRecordGetFieldCount( hrec );
        ok( count == 6, "Expected 6 record fields, got %d\n", count );

        r = MsiRecordGetInteger( hrec, 1 );
        if( r != join_res_ninth[i].one )
            data_correct = FALSE;

        r = MsiRecordGetInteger( hrec, 2 );
        if( r != join_res_ninth[i].two )
            data_correct = FALSE;

        r = MsiRecordGetInteger( hrec, 3 );
        if( r != join_res_ninth[i].three )
            data_correct = FALSE;

        r = MsiRecordGetInteger( hrec, 4 );
        if( r != join_res_ninth[i].four )
            data_correct = FALSE;

        r = MsiRecordGetInteger( hrec, 5 );
        if( r != join_res_ninth[i].five )
            data_correct = FALSE;

        r = MsiRecordGetInteger( hrec, 6);
        if( r != join_res_ninth[i].six )
            data_correct = FALSE;

        i++;
        MsiCloseHandle(hrec);
    }
    ok( data_correct, "data returned in the wrong order\n");

    ok( i == 6, "Expected 6 rows, got %d\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT * FROM `Four`, `Five`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT * FROM `Nonexistent`, `One`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok( r == ERROR_BAD_QUERY_SYNTAX,
        "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r );

    MsiCloseHandle(hdb);
    DeleteFile(msifile);
}

static void test_temporary_table(void)
{
    MSICONDITION cond;
    MSIHANDLE hdb = 0, view = 0, rec;
    const char *query;
    UINT r;
    char buf[0x10];
    DWORD sz;

    cond = MsiDatabaseIsTablePersistent(0, NULL);
    ok( cond == MSICONDITION_ERROR, "wrong return condition\n");

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    cond = MsiDatabaseIsTablePersistent(hdb, NULL);
    ok( cond == MSICONDITION_ERROR, "wrong return condition\n");

    todo_wine {
    cond = MsiDatabaseIsTablePersistent(hdb, "_Tables");
    ok( cond == MSICONDITION_NONE, "wrong return condition\n");

    cond = MsiDatabaseIsTablePersistent(hdb, "_Columns");
    ok( cond == MSICONDITION_NONE, "wrong return condition\n");
    }

    cond = MsiDatabaseIsTablePersistent(hdb, "_Storages");
    ok( cond == MSICONDITION_NONE, "wrong return condition\n");

    cond = MsiDatabaseIsTablePersistent(hdb, "_Streams");
    ok( cond == MSICONDITION_NONE, "wrong return condition\n");

    query = "CREATE TABLE `P` ( `B` SHORT NOT NULL, `C` CHAR(255) PRIMARY KEY `C`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    cond = MsiDatabaseIsTablePersistent(hdb, "P");
    ok( cond == MSICONDITION_TRUE, "wrong return condition\n");

    query = "CREATE TABLE `P2` ( `B` SHORT NOT NULL, `C` CHAR(255) PRIMARY KEY `C`) HOLD";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    cond = MsiDatabaseIsTablePersistent(hdb, "P2");
    ok( cond == MSICONDITION_TRUE, "wrong return condition\n");

    query = "CREATE TABLE `T` ( `B` SHORT NOT NULL TEMPORARY, `C` CHAR(255) TEMPORARY PRIMARY KEY `C`) HOLD";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    cond = MsiDatabaseIsTablePersistent(hdb, "T");
    ok( cond == MSICONDITION_FALSE, "wrong return condition\n");

    query = "CREATE TABLE `T2` ( `B` SHORT NOT NULL TEMPORARY, `C` CHAR(255) TEMPORARY PRIMARY KEY `C`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    todo_wine {
    cond = MsiDatabaseIsTablePersistent(hdb, "T2");
    ok( cond == MSICONDITION_NONE, "wrong return condition\n");
    }

    query = "CREATE TABLE `T3` ( `B` SHORT NOT NULL TEMPORARY, `C` CHAR(255) PRIMARY KEY `C`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    cond = MsiDatabaseIsTablePersistent(hdb, "T3");
    ok( cond == MSICONDITION_TRUE, "wrong return condition\n");

    todo_wine {
    query = "CREATE TABLE `T4` ( `B` SHORT NOT NULL, `C` CHAR(255) TEMPORARY PRIMARY KEY `C`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_FUNCTION_FAILED, "failed to add table\n");

    cond = MsiDatabaseIsTablePersistent(hdb, "T4");
    ok( cond == MSICONDITION_NONE, "wrong return condition\n");
    }

    query = "CREATE TABLE `T5` ( `B` SHORT NOT NULL TEMP, `C` CHAR(255) TEMP PRIMARY KEY `C`) HOLD";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "failed to add table\n");

    query = "select * from `T`";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "failed to query table\n");
    r = MsiViewGetColumnInfo(view, MSICOLINFO_TYPES, &rec);
    ok(r == ERROR_SUCCESS, "failed to get column info\n");

    sz = sizeof buf;
    r = MsiRecordGetString(rec, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "failed to get string\n");
    todo_wine ok( 0 == strcmp("G255", buf), "wrong column type\n");

    sz = sizeof buf;
    r = MsiRecordGetString(rec, 2, buf, &sz);
    ok(r == ERROR_SUCCESS, "failed to get string\n");
    todo_wine ok( 0 == strcmp("j2", buf), "wrong column type\n");

    MsiCloseHandle( rec );
    MsiCloseHandle( view );

    /* query the table data */
    rec = 0;
    r = do_query(hdb, "select * from `_Tables` where `Name` = 'T'", &rec);
    ok( r == ERROR_SUCCESS, "temporary table exists in _Tables\n");
    MsiCloseHandle( rec );

    /* query the column data */
    rec = 0;
    r = do_query(hdb, "select * from `_Columns` where `Table` = 'T' AND `Name` = 'B'", &rec);
    ok( r == ERROR_NO_MORE_ITEMS, "temporary table exists in _Columns\n");
    if (rec) MsiCloseHandle( rec );

    r = do_query(hdb, "select * from `_Columns` where `Table` = 'T' AND `Name` = 'C'", &rec);
    ok( r == ERROR_NO_MORE_ITEMS, "temporary table exists in _Columns\n");
    if (rec) MsiCloseHandle( rec );

    MsiCloseHandle( hdb );

    DeleteFile(msifile);
}

static void test_alter(void)
{
    MSICONDITION cond;
    MSIHANDLE hdb = 0;
    const char *query;
    UINT r;

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    query = "CREATE TABLE `T` ( `B` SHORT NOT NULL TEMPORARY, `C` CHAR(255) TEMPORARY PRIMARY KEY `C`) HOLD";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    cond = MsiDatabaseIsTablePersistent(hdb, "T");
    ok( cond == MSICONDITION_FALSE, "wrong return condition\n");

    query = "ALTER TABLE `T` HOLD";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to hold table %d\n", r);

    query = "ALTER TABLE `T` FREE";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to free table\n");

    query = "ALTER TABLE `T` FREE";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to free table\n");

    query = "ALTER TABLE `T` FREE";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "failed to free table\n");

    query = "ALTER TABLE `T` HOLD";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "failed to hold table %d\n", r);

    /* table T is removed */
    query = "SELECT * FROM `T`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* create the table again */
    query = "CREATE TABLE `U` ( `A` INTEGER, `B` INTEGER PRIMARY KEY `B`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* up the ref count */
    query = "ALTER TABLE `U` HOLD";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to free table\n");

    /* add column, no data type */
    query = "ALTER TABLE `U` ADD `C`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "ALTER TABLE `U` ADD `C` INTEGER";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* add column C again */
    query = "ALTER TABLE `U` ADD `C` INTEGER";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "ALTER TABLE `U` ADD `D` INTEGER TEMPORARY";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `U` ( `A`, `B`, `C`, `D` ) VALUES ( 1, 2, 3, 4 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "ALTER TABLE `U` ADD `D` INTEGER TEMPORARY HOLD";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "INSERT INTO `U` ( `A`, `B`, `C`, `D` ) VALUES ( 5, 6, 7, 8 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `U` WHERE `D` = 8";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "ALTER TABLE `U` ADD `D` INTEGER TEMPORARY FREE";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "ALTER COLUMN `D` FREE";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* drop the ref count */
    query = "ALTER TABLE `U` FREE";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* table is not empty */
    query = "SELECT * FROM `U`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* column D is removed */
    query = "SELECT * FROM `U` WHERE `D` = 8";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "INSERT INTO `U` ( `A`, `B`, `C`, `D` ) VALUES ( 9, 10, 11, 12 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* add the column again */
    query = "ALTER TABLE `U` ADD `E` INTEGER TEMPORARY HOLD";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* up the ref count */
    query = "ALTER TABLE `U` HOLD";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `U` ( `A`, `B`, `C`, `E` ) VALUES ( 13, 14, 15, 16 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `U` WHERE `E` = 16";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* drop the ref count */
    query = "ALTER TABLE `U` FREE";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `U` ( `A`, `B`, `C`, `E` ) VALUES ( 17, 18, 19, 20 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `U` WHERE `E` = 20";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* drop the ref count */
    query = "ALTER TABLE `U` FREE";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* table still exists */
    query = "SELECT * FROM `U`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* col E is removed */
    query = "SELECT * FROM `U` WHERE `E` = 20";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "INSERT INTO `U` ( `A`, `B`, `C`, `E` ) VALUES ( 20, 21, 22, 23 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* drop the ref count once more */
    query = "ALTER TABLE `U` FREE";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* table still exists */
    query = "SELECT * FROM `U`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle( hdb );
    DeleteFile(msifile);
}

static void test_integers(void)
{
    MSIHANDLE hdb = 0, view = 0, rec = 0;
    DWORD count, i;
    const char *query;
    UINT r;

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    /* create a table */
    query = "CREATE TABLE `integers` ( "
            "`one` SHORT, `two` INT, `three` INTEGER, `four` LONG, "
            "`five` SHORT NOT NULL, `six` INT NOT NULL, "
            "`seven` INTEGER NOT NULL, `eight` LONG NOT NULL "
            "PRIMARY KEY `one`)";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewClose(view);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(view);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    query = "SELECT * FROM `integers`";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_NAMES, &rec);
    count = MsiRecordGetFieldCount(rec);
    ok(count == 8, "Expected 8, got %d\n", count);
    ok(check_record(rec, 1, "one"), "Expected one\n");
    ok(check_record(rec, 2, "two"), "Expected two\n");
    ok(check_record(rec, 3, "three"), "Expected three\n");
    ok(check_record(rec, 4, "four"), "Expected four\n");
    ok(check_record(rec, 5, "five"), "Expected five\n");
    ok(check_record(rec, 6, "six"), "Expected six\n");
    ok(check_record(rec, 7, "seven"), "Expected seven\n");
    ok(check_record(rec, 8, "eight"), "Expected eight\n");
    MsiCloseHandle(rec);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_TYPES, &rec);
    count = MsiRecordGetFieldCount(rec);
    ok(count == 8, "Expected 8, got %d\n", count);
    ok(check_record(rec, 1, "I2"), "Expected I2\n");
    ok(check_record(rec, 2, "I2"), "Expected I2\n");
    ok(check_record(rec, 3, "I2"), "Expected I2\n");
    ok(check_record(rec, 4, "I4"), "Expected I4\n");
    ok(check_record(rec, 5, "i2"), "Expected i2\n");
    ok(check_record(rec, 6, "i2"), "Expected i2\n");
    ok(check_record(rec, 7, "i2"), "Expected i2\n");
    ok(check_record(rec, 8, "i4"), "Expected i4\n");
    MsiCloseHandle(rec);

    MsiViewClose(view);
    MsiCloseHandle(view);

    /* insert values into it, NULL where NOT NULL is specified */
    query = "INSERT INTO `integers` ( `one`, `two`, `three`, `four`, `five`, `six`, `seven`, `eight` )"
        "VALUES('', '', '', '', '', '', '', '')";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_FUNCTION_FAILED, "Expected ERROR_FUNCTION_FAILED, got %d\n", r);

    MsiViewClose(view);
    MsiCloseHandle(view);

    query = "SELECT * FROM `integers`";
    r = do_query(hdb, query, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiRecordGetFieldCount(rec);
    ok(r == -1, "record count wrong: %d\n", r);

    MsiCloseHandle(rec);

    /* insert legitimate values into it */
    query = "INSERT INTO `integers` ( `one`, `two`, `three`, `four`, `five`, `six`, `seven`, `eight` )"
        "VALUES('', '2', '', '4', '5', '6', '7', '8')";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `integers`";
    r = do_query(hdb, query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetFieldCount(rec);
    ok(r == 8, "record count wrong: %d\n", r);

    i = MsiRecordGetInteger(rec, 1);
    ok(i == MSI_NULL_INTEGER, "Expected MSI_NULL_INTEGER, got %d\n", i);
    i = MsiRecordGetInteger(rec, 3);
    ok(i == MSI_NULL_INTEGER, "Expected MSI_NULL_INTEGER, got %d\n", i);
    i = MsiRecordGetInteger(rec, 2);
    ok(i == 2, "Expected 2, got %d\n", i);
    i = MsiRecordGetInteger(rec, 4);
    ok(i == 4, "Expected 4, got %d\n", i);
    i = MsiRecordGetInteger(rec, 5);
    ok(i == 5, "Expected 5, got %d\n", i);
    i = MsiRecordGetInteger(rec, 6);
    ok(i == 6, "Expected 6, got %d\n", i);
    i = MsiRecordGetInteger(rec, 7);
    ok(i == 7, "Expected 7, got %d\n", i);
    i = MsiRecordGetInteger(rec, 8);
    ok(i == 8, "Expected 8, got %d\n", i);

    MsiCloseHandle(rec);
    MsiViewClose(view);
    MsiCloseHandle(view);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "MsiDatabaseCommit failed\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = DeleteFile(msifile);
    ok(r == TRUE, "file didn't exist after commit\n");
}

static void test_update(void)
{
    MSIHANDLE hdb = 0, view = 0, rec = 0;
    CHAR result[MAX_PATH];
    const char *query;
    DWORD size;
    UINT r;

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    /* create the Control table */
    query = "CREATE TABLE `Control` ( "
        "`Dialog_` CHAR(72) NOT NULL, `Control` CHAR(50) NOT NULL, `Type` SHORT NOT NULL, "
        "`X` SHORT NOT NULL, `Y` SHORT NOT NULL, `Width` SHORT NOT NULL, `Height` SHORT NOT NULL,"
        "`Attributes` LONG, `Property` CHAR(50), `Text` CHAR(0) LOCALIZABLE, "
        "`Control_Next` CHAR(50), `Help` CHAR(50) LOCALIZABLE PRIMARY KEY `Dialog_`, `Control`)";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewClose(view);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(view);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* add a control */
    query = "INSERT INTO `Control` ( "
        "`Dialog_`, `Control`, `Type`, `X`, `Y`, `Width`, `Height`, "
        "`Property`, `Text`, `Control_Next`, `Help` )"
        "VALUES('ErrorDialog', 'ErrorText', '1', '5', '5', '5', '5', '', '', '', '')";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewClose(view);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(view);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* add a second control */
    query = "INSERT INTO `Control` ( "
        "`Dialog_`, `Control`, `Type`, `X`, `Y`, `Width`, `Height`, "
        "`Property`, `Text`, `Control_Next`, `Help` )"
        "VALUES('ErrorDialog', 'Button', '1', '5', '5', '5', '5', '', '', '', '')";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewClose(view);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(view);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* add a third control */
    query = "INSERT INTO `Control` ( "
        "`Dialog_`, `Control`, `Type`, `X`, `Y`, `Width`, `Height`, "
        "`Property`, `Text`, `Control_Next`, `Help` )"
        "VALUES('AnotherDialog', 'ErrorText', '1', '5', '5', '5', '5', '', '', '', '')";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewClose(view);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(view);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* bad table */
    query = "UPDATE `NotATable` SET `Text` = 'this is text' WHERE `Dialog_` = 'ErrorDialog'";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* bad set column */
    query = "UPDATE `Control` SET `NotAColumn` = 'this is text' WHERE `Dialog_` = 'ErrorDialog'";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* bad where condition */
    query = "UPDATE `Control` SET `Text` = 'this is text' WHERE `NotAColumn` = 'ErrorDialog'";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* just the dialog_ specified */
    query = "UPDATE `Control` SET `Text` = 'this is text' WHERE `Dialog_` = 'ErrorDialog'";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewClose(view);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(view);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* check the modified text */
    query = "SELECT `Text` FROM `Control` WHERE `Control` = 'ErrorText'";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(rec, 1, result, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(result, "this is text"), "Expected `this is text`, got %s\n", result);

    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(rec, 1, result, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrlen(result), "Expected an empty string, got %s\n", result);

    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiViewClose(view);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(view);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* dialog_ and control specified */
    query = "UPDATE `Control` SET `Text` = 'this is text' WHERE `Dialog_` = 'ErrorDialog' AND `Control` = 'ErrorText'";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewClose(view);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(view);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* check the modified text */
    query = "SELECT `Text` FROM `Control` WHERE `Control` = 'ErrorText'";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(rec, 1, result, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(result, "this is text"), "Expected `this is text`, got %s\n", result);

    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(rec, 1, result, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrlen(result), "Expected an empty string, got %s\n", result);

    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiViewClose(view);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(view);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* no where condition */
    query = "UPDATE `Control` SET `Text` = 'this is text'";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewClose(view);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(view);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* check the modified text */
    query = "SELECT `Text` FROM `Control`";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(rec, 1, result, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(result, "this is text"), "Expected `this is text`, got %s\n", result);

    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(rec, 1, result, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(result, "this is text"), "Expected `this is text`, got %s\n", result);

    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(rec, 1, result, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(result, "this is text"), "Expected `this is text`, got %s\n", result);

    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiViewClose(view);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(view);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    query = "CREATE TABLE `Apple` ( `Banana` CHAR(72) NOT NULL, "
        "`Orange` CHAR(72),  `Pear` INT PRIMARY KEY `Banana`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Apple` ( `Banana`, `Orange`, `Pear` )"
        "VALUES('one', 'two', 3)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Apple` ( `Banana`, `Orange`, `Pear` )"
        "VALUES('three', 'four', 5)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Apple` ( `Banana`, `Orange`, `Pear` )"
        "VALUES('six', 'two', 7)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    rec = MsiCreateRecord(2);
    MsiRecordSetInteger(rec, 1, 8);
    MsiRecordSetString(rec, 2, "two");

    query = "UPDATE `Apple` SET `Pear` = ? WHERE `Orange` = ?";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(rec);

    query = "SELECT `Pear` FROM `Apple` ORDER BY `Orange`";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(rec, 1);
    ok(r == 8, "Expected 8, got %d\n", r);

    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(rec, 1);
    ok(r == 8, "Expected 8, got %d\n", r);

    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(rec, 1);
    ok(r == 5, "Expected 5, got %d\n", r);

    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expectd ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(view);
    MsiCloseHandle(view);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "MsiDatabaseCommit failed\n");
    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    DeleteFile(msifile);
}

static void test_special_tables(void)
{
    const char *query;
    MSIHANDLE hdb = 0;
    UINT r;

    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    query = "CREATE TABLE `_Properties` ( "
        "`foo` INT NOT NULL, `bar` INT LOCALIZABLE PRIMARY KEY `foo`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to create table\n");

    query = "CREATE TABLE `_Storages` ( "
        "`foo` INT NOT NULL, `bar` INT LOCALIZABLE PRIMARY KEY `foo`)";
    r = run_query(hdb, 0, query);
    todo_wine ok(r == ERROR_BAD_QUERY_SYNTAX, "created _Streams table\n");

    query = "CREATE TABLE `_Streams` ( "
        "`foo` INT NOT NULL, `bar` INT LOCALIZABLE PRIMARY KEY `foo`)";
    r = run_query(hdb, 0, query);
    todo_wine ok(r == ERROR_BAD_QUERY_SYNTAX, "created _Streams table\n");

    query = "CREATE TABLE `_Tables` ( "
        "`foo` INT NOT NULL, `bar` INT LOCALIZABLE PRIMARY KEY `foo`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "created _Tables table\n");

    query = "CREATE TABLE `_Columns` ( "
        "`foo` INT NOT NULL, `bar` INT LOCALIZABLE PRIMARY KEY `foo`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "created _Columns table\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");
}

static void test_select_markers(void)
{
    MSIHANDLE hdb = 0, rec, view, res;
    LPCSTR query;
    UINT r;
    DWORD size;
    CHAR buf[MAX_PATH];

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    r = run_query(hdb, 0,
            "CREATE TABLE `Table` (`One` CHAR(72), `Two` CHAR(72), `Three` SHORT PRIMARY KEY `One`, `Two`, `Three`)");
    ok(r == S_OK, "cannot create table: %d\n", r);

    r = run_query(hdb, 0, "INSERT INTO `Table` "
            "( `One`, `Two`, `Three` ) VALUES ( 'apple', 'one', 1 )");
    ok(r == S_OK, "cannot add file to the Media table: %d\n", r);

    r = run_query(hdb, 0, "INSERT INTO `Table` "
            "( `One`, `Two`, `Three` ) VALUES ( 'apple', 'two', 1 )");
    ok(r == S_OK, "cannot add file to the Media table: %d\n", r);

    r = run_query(hdb, 0, "INSERT INTO `Table` "
            "( `One`, `Two`, `Three` ) VALUES ( 'apple', 'two', 2 )");
    ok(r == S_OK, "cannot add file to the Media table: %d\n", r);

    r = run_query(hdb, 0, "INSERT INTO `Table` "
            "( `One`, `Two`, `Three` ) VALUES ( 'banana', 'three', 3 )");
    ok(r == S_OK, "cannot add file to the Media table: %d\n", r);

    rec = MsiCreateRecord(2);
    MsiRecordSetString(rec, 1, "apple");
    MsiRecordSetString(rec, 2, "two");

    query = "SELECT * FROM `Table` WHERE `One`=? AND `Two`=? ORDER BY `Three`";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewExecute(view, rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(view, &res);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(res, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(buf, "apple"), "Expected apple, got %s\n", buf);

    size = MAX_PATH;
    r = MsiRecordGetString(res, 2, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(buf, "two"), "Expected two, got %s\n", buf);

    r = MsiRecordGetInteger(res, 3);
    ok(r == 1, "Expected 1, got %d\n", r);

    MsiCloseHandle(res);

    r = MsiViewFetch(view, &res);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(res, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(buf, "apple"), "Expected apple, got %s\n", buf);

    size = MAX_PATH;
    r = MsiRecordGetString(res, 2, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(buf, "two"), "Expected two, got %s\n", buf);

    r = MsiRecordGetInteger(res, 3);
    ok(r == 2, "Expected 2, got %d\n", r);

    MsiCloseHandle(res);

    r = MsiViewFetch(view, &res);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiCloseHandle(rec);
    MsiViewClose(view);
    MsiCloseHandle(view);

    rec = MsiCreateRecord(2);
    MsiRecordSetString(rec, 1, "one");
    MsiRecordSetInteger(rec, 2, 1);

    query = "SELECT * FROM `Table` WHERE `Two`<>? AND `Three`>? ORDER BY `Three`";
    r = MsiDatabaseOpenView(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(view, &res);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(res, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(buf, "apple"), "Expected apple, got %s\n", buf);

    size = MAX_PATH;
    r = MsiRecordGetString(res, 2, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(buf, "two"), "Expected two, got %s\n", buf);

    r = MsiRecordGetInteger(res, 3);
    ok(r == 2, "Expected 2, got %d\n", r);

    MsiCloseHandle(res);

    r = MsiViewFetch(view, &res);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(res, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(buf, "banana"), "Expected banana, got %s\n", buf);

    size = MAX_PATH;
    r = MsiRecordGetString(res, 2, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(buf, "three"), "Expected three, got %s\n", buf);

    r = MsiRecordGetInteger(res, 3);
    ok(r == 3, "Expected 3, got %d\n", r);

    MsiCloseHandle(res);

    r = MsiViewFetch(view, &res);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiCloseHandle(rec);
    MsiViewClose(view);
    MsiCloseHandle(view);
    MsiCloseHandle(hdb);
    DeleteFile(msifile);
}

static void test_viewmodify_update(void)
{
    MSIHANDLE hdb = 0, hview = 0, hrec = 0;
    const char *query;
    UINT r;

    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    query = "CREATE TABLE `table` (`A` INT, `B` INT PRIMARY KEY `A`)";
    r = run_query( hdb, 0, query );
    ok(r == ERROR_SUCCESS, "query failed\n");

    query = "INSERT INTO `table` (`A`, `B`) VALUES (1, 2)";
    r = run_query( hdb, 0, query );
    ok(r == ERROR_SUCCESS, "query failed\n");

    query = "INSERT INTO `table` (`A`, `B`) VALUES (3, 4)";
    r = run_query( hdb, 0, query );
    ok(r == ERROR_SUCCESS, "query failed\n");

    query = "INSERT INTO `table` (`A`, `B`) VALUES (5, 6)";
    r = run_query( hdb, 0, query );
    ok(r == ERROR_SUCCESS, "query failed\n");

    query = "SELECT `B` FROM `table`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");

    r = MsiRecordSetInteger(hrec, 1, 0);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");

    r = MsiViewModify(hview, MSIMODIFY_UPDATE, hrec);
    ok(r == ERROR_SUCCESS, "MsiViewModify failed: %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    query = "SELECT * FROM `table`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);
    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 0, "Expected 0, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 3, "Expected 3, got %d\n", r);
    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 4, "Expected 4, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 5, "Expected 5, got %d\n", r);
    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 6, "Expected 6, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* loop through all elements */
    query = "SELECT `B` FROM `table`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    while (TRUE)
    {
        r = MsiViewFetch(hview, &hrec);
        if (r != ERROR_SUCCESS)
            break;

        r = MsiRecordSetInteger(hrec, 1, 0);
        ok(r == ERROR_SUCCESS, "failed to set integer\n");

        r = MsiViewModify(hview, MSIMODIFY_UPDATE, hrec);
        ok(r == ERROR_SUCCESS, "MsiViewModify failed: %d\n", r);

        r = MsiCloseHandle(hrec);
        ok(r == ERROR_SUCCESS, "failed to close record\n");
    }

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    query = "SELECT * FROM `table`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);
    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 0, "Expected 0, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 3, "Expected 3, got %d\n", r);
    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 0, "Expected 0, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 5, "Expected 5, got %d\n", r);
    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 0, "Expected 0, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase close failed\n");
}

static const WCHAR data10[] = { /* MOO */
    0x8001, 0x000b,
};
static const WCHAR data11[] = { /* AAR */
    0x8002, 0x8005,
    0x000c, 0x000f,
};
static const char data12[] = /* _StringData */
    "MOOABAARCDonetwofourfive";
static const WCHAR data13[] = { /* _StringPool */
/*  len, refs */
    0,   0,    /* string 0 ''     */
    0,   0,    /* string 1 ''     */
    0,   0,    /* string 2 ''     */
    0,   0,    /* string 3 ''     */
    0,   0,    /* string 4 ''     */
    3,   3,    /* string 5 'MOO'  */
    1,   1,    /* string 6 'A'    */
    1,   1,    /* string 7 'B'    */
    3,   3,    /* string 8 'AAR'  */
    1,   1,    /* string 9 'C'    */
    1,   1,    /* string a 'D'    */
    3,   1,    /* string b 'one'  */
    3,   1,    /* string c 'two'  */
    0,   0,    /* string d ''     */
    4,   1,    /* string e 'four' */
    4,   1,    /* string f 'five' */
};

static void test_stringtable(void)
{
    MSIHANDLE hdb = 0, hview = 0, hrec = 0;
    IStorage *stg = NULL;
    IStream *stm;
    WCHAR name[0x20];
    HRESULT hr;
    const char *query;
    char buffer[MAX_PATH];
    WCHAR data[MAX_PATH];
    DWORD sz, read;
    UINT r;

    static const DWORD mode = STGM_DIRECT | STGM_READ | STGM_SHARE_DENY_WRITE;
    static const WCHAR stringdata[] = {0x4840, 0x3f3f, 0x4577, 0x446c, 0x3b6a, 0x45e4, 0x4824, 0}; /* _StringData */
    static const WCHAR stringpool[] = {0x4840, 0x3f3f, 0x4577, 0x446c, 0x3e6a, 0x44b2, 0x482f, 0}; /* _StringPool */
    static const WCHAR moo[] = {0x4840, 0x3e16, 0x4818, 0}; /* MOO */
    static const WCHAR aar[] = {0x4840, 0x3a8a, 0x481b, 0}; /* AAR */

    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `MOO` (`A` INT, `B` CHAR(72) PRIMARY KEY `A`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `AAR` (`C` INT, `D` CHAR(72) PRIMARY KEY `C`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* insert persistent row */
    query = "INSERT INTO `MOO` (`A`, `B`) VALUES (1, 'one')";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* insert persistent row */
    query = "INSERT INTO `AAR` (`C`, `D`) VALUES (2, 'two')";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* open a view */
    query = "SELECT * FROM `MOO`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    hrec = MsiCreateRecord(2);

    r = MsiRecordSetInteger(hrec, 1, 3);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiRecordSetString(hrec, 2, "three");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* insert a nonpersistent row */
    r = MsiViewModify(hview, MSIMODIFY_INSERT_TEMPORARY, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* insert persistent row */
    query = "INSERT INTO `MOO` (`A`, `B`) VALUES (4, 'four')";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* insert persistent row */
    query = "INSERT INTO `AAR` (`C`, `D`) VALUES (5, 'five')";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_READONLY, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `MOO`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetFieldCount(hrec);
    ok(r == 2, "Expected 2, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);

    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(buffer, "one"), "Expected one, got %s\n", buffer);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    todo_wine
    {
        ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    }

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `AAR`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetFieldCount(hrec);
    ok(r == 2, "Expected 2, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 2, "Expected 2, got %d\n", r);

    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(buffer, "two"), "Expected two, got %s\n", buffer);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetFieldCount(hrec);
    ok(r == 2, "Expected 2, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 5, "Expected 5, got %d\n", r);

    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(buffer, "five"), "Expected five, got %s\n", buffer);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MultiByteToWideChar(CP_ACP, 0, msifile, -1, name, 0x20);
    hr = StgOpenStorage(name, NULL, mode, NULL, 0, &stg);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);
    ok(stg != NULL, "Expected non-NULL storage\n");

    hr = IStorage_OpenStream(stg, moo, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);
    ok(stm != NULL, "Expected non-NULL stream\n");

    hr = IStream_Read(stm, data, MAX_PATH, &read);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);
    todo_wine
    {
        ok(read == 4, "Expected 4, got %d\n", read);
        ok(!memcmp(data, data10, read), "Unexpected data\n");
    }

    hr = IStream_Release(stm);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);

    hr = IStorage_OpenStream(stg, aar, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);
    ok(stm != NULL, "Expected non-NULL stream\n");

    hr = IStream_Read(stm, data, MAX_PATH, &read);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);
    ok(read == 8, "Expected 8, got %d\n", read);
    todo_wine
    {
        ok(!memcmp(data, data11, read), "Unexpected data\n");
    }

    hr = IStream_Release(stm);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);

    hr = IStorage_OpenStream(stg, stringdata, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);
    ok(stm != NULL, "Expected non-NULL stream\n");

    hr = IStream_Read(stm, buffer, MAX_PATH, &read);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);
    ok(read == 24, "Expected 24, got %d\n", read);
    ok(!memcmp(buffer, data12, read), "Unexpected data\n");

    hr = IStream_Release(stm);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);

    hr = IStorage_OpenStream(stg, stringpool, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);
    ok(stm != NULL, "Expected non-NULL stream\n");

    hr = IStream_Read(stm, data, MAX_PATH, &read);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);
    todo_wine
    {
        ok(read == 64, "Expected 64, got %d\n", read);
        ok(!memcmp(data, data13, read), "Unexpected data\n");
    }

    hr = IStream_Release(stm);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);

    hr = IStorage_Release(stg);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);

    DeleteFileA(msifile);
}

static void test_viewmodify_delete(void)
{
    MSIHANDLE hdb = 0, hview = 0, hrec = 0;
    UINT r;
    const char *query;
    char buffer[0x100];
    DWORD sz;

    DeleteFile(msifile);

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `phone` ( "
            "`id` INT, `name` CHAR(32), `number` CHAR(32) "
            "PRIMARY KEY `id`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `phone` ( `id`, `name`, `number` )"
        "VALUES('1', 'Alan', '5030581')";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `phone` ( `id`, `name`, `number` )"
        "VALUES('2', 'Barry', '928440')";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `phone` ( `id`, `name`, `number` )"
        "VALUES('3', 'Cindy', '2937550')";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `phone` WHERE `id` <= 2";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* delete 1 */
    r = MsiViewModify(hview, MSIMODIFY_DELETE, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* delete 2 */
    r = MsiViewModify(hview, MSIMODIFY_DELETE, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `phone`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 3, "Expected 3, got %d\n", r);

    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(buffer, "Cindy"), "Expected Cindy, got %s\n", buffer);

    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 3, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(buffer, "2937550"), "Expected 2937550, got %s\n", buffer);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
}

static const WCHAR _Tables[] = {0x4840, 0x3f7f, 0x4164, 0x422f, 0x4836, 0};
static const WCHAR _StringData[] = {0x4840, 0x3f3f, 0x4577, 0x446c, 0x3b6a, 0x45e4, 0x4824, 0};
static const WCHAR _StringPool[] = {0x4840, 0x3f3f, 0x4577, 0x446c, 0x3e6a, 0x44b2, 0x482f, 0};

static const WCHAR data14[] = { /* _StringPool */
/*  len, refs */
    0,   0,    /* string 0 ''    */
};

static const struct {
    LPCWSTR name;
    const void *data;
    DWORD size;
} database_table_data[] =
{
    {_Tables, NULL, 0},
    {_StringData, NULL, 0},
    {_StringPool, data14, sizeof data14},
};

static void enum_stream_names(IStorage *stg)
{
    IEnumSTATSTG *stgenum = NULL;
    IStream *stm;
    HRESULT hr;
    STATSTG stat;
    ULONG n, count;
    BYTE data[MAX_PATH];
    BYTE check[MAX_PATH];
    DWORD sz;

    memset(check, 'a', MAX_PATH);

    hr = IStorage_EnumElements(stg, 0, NULL, 0, &stgenum);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    n = 0;
    while(TRUE)
    {
        count = 0;
        hr = IEnumSTATSTG_Next(stgenum, 1, &stat, &count);
        if(FAILED(hr) || !count)
            break;

        ok(!lstrcmpW(stat.pwcsName, database_table_data[n].name),
           "Expected table %d name to match\n", n);

        stm = NULL;
        hr = IStorage_OpenStream(stg, stat.pwcsName, NULL,
                                 STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm);
        ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
        ok(stm != NULL, "Expected non-NULL stream\n");

        sz = MAX_PATH;
        memset(data, 'a', MAX_PATH);
        hr = IStream_Read(stm, data, sz, &count);
        ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

        ok(count == database_table_data[n].size,
           "Expected %d, got %d\n", database_table_data[n].size, count);

        if (!database_table_data[n].size)
            ok(!memcmp(data, check, MAX_PATH), "data should not be changed\n");
        else
            ok(!memcmp(data, database_table_data[n].data, database_table_data[n].size),
               "Expected table %d data to match\n", n);

        IStream_Release(stm);
        n++;
    }

    ok(n == 3, "Expected 3, got %d\n", n);

    IEnumSTATSTG_Release(stgenum);
}

static void test_defaultdatabase(void)
{
    UINT r;
    HRESULT hr;
    MSIHANDLE hdb;
    IStorage *stg = NULL;

    static const WCHAR msifileW[] = {'w','i','n','e','t','e','s','t','.','m','s','i',0};

    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hdb);

    hr = StgOpenStorage(msifileW, NULL, STGM_READ | STGM_SHARE_DENY_WRITE, NULL, 0, &stg);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(stg != NULL, "Expected non-NULL stg\n");

    enum_stream_names(stg);

    IStorage_Release(stg);
    DeleteFileA(msifile);
}

static void test_order(void)
{
    MSIHANDLE hdb, hview, hrec;
    CHAR buffer[MAX_PATH];
    LPCSTR query;
    UINT r, sz;
    int val;

    hdb = create_db();
    ok(hdb, "failed to create db\n");

    query = "CREATE TABLE `Empty` ( `A` SHORT NOT NULL PRIMARY KEY `A`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `Mesa` ( `A` SHORT NOT NULL, `B` SHORT, `C` SHORT PRIMARY KEY `A`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Mesa` ( `A`, `B`, `C` ) VALUES ( 1, 2, 9 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Mesa` ( `A`, `B`, `C` ) VALUES ( 3, 4, 7 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Mesa` ( `A`, `B`, `C` ) VALUES ( 5, 6, 8 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `Sideboard` ( `D` SHORT NOT NULL, `E` SHORT, `F` SHORT PRIMARY KEY `D`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Sideboard` ( `D`, `E`, `F` ) VALUES ( 10, 11, 18 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Sideboard` ( `D`, `E`, `F` ) VALUES ( 12, 13, 16 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Sideboard` ( `D`, `E`, `F` ) VALUES ( 14, 15, 17 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT `A`, `B` FROM `Mesa` ORDER BY `C`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 3, "Expected 3, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 4, "Expected 3, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 5, "Expected 5, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 6, "Expected 6, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 1, "Expected 1, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 2, "Expected 2, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT `A`, `D` FROM `Mesa`, `Sideboard` ORDER BY `F`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 1, "Expected 1, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 12, "Expected 12, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 3, "Expected 3, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 12, "Expected 12, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 5, "Expected 5, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 12, "Expected 12, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 1, "Expected 1, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 14, "Expected 14, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 3, "Expected 3, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 14, "Expected 14, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 5, "Expected 5, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 14, "Expected 14, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 1, "Expected 1, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 10, "Expected 10, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 3, "Expected 3, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 10, "Expected 10, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    val = MsiRecordGetInteger(hrec, 1);
    ok(val == 5, "Expected 5, got %d\n", val);

    val = MsiRecordGetInteger(hrec, 2);
    ok(val == 10, "Expected 10, got %d\n", val);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT * FROM `Empty` ORDER BY `A`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "CREATE TABLE `Buffet` ( `One` CHAR(72), `Two` SHORT PRIMARY KEY `One`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Buffet` ( `One`, `Two` ) VALUES ( 'uno',  2)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Buffet` ( `One`, `Two` ) VALUES ( 'dos',  3)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Buffet` ( `One`, `Two` ) VALUES ( 'tres',  1)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `Buffet` WHERE `One` = 'dos' ORDER BY `Two`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = sizeof(buffer);
    r = MsiRecordGetString(hrec, 1, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(buffer, "dos"), "Expected \"dos\", got \"%s\"\n", buffer);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 3, "Expected 3, got %d\n", r);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);
    MsiCloseHandle(hdb);
}

static void test_viewmodify_delete_temporary(void)
{
    MSIHANDLE hdb, hview, hrec;
    const char *query;
    UINT r;

    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `Table` ( `A` SHORT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `Table`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    hrec = MsiCreateRecord(1);
    MsiRecordSetInteger(hrec, 1, 1);

    r = MsiViewModify(hview, MSIMODIFY_INSERT, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hrec);

    hrec = MsiCreateRecord(1);
    MsiRecordSetInteger(hrec, 1, 2);

    r = MsiViewModify(hview, MSIMODIFY_INSERT_TEMPORARY, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hrec);

    hrec = MsiCreateRecord(1);
    MsiRecordSetInteger(hrec, 1, 3);

    r = MsiViewModify(hview, MSIMODIFY_INSERT, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hrec);

    hrec = MsiCreateRecord(1);
    MsiRecordSetInteger(hrec, 1, 4);

    r = MsiViewModify(hview, MSIMODIFY_INSERT_TEMPORARY, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hrec);
    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT * FROM `Table` WHERE  `A` = 2";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewModify(hview, MSIMODIFY_DELETE, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hrec);
    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT * FROM `Table` WHERE  `A` = 3";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewModify(hview, MSIMODIFY_DELETE, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hrec);
    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT * FROM `Table` ORDER BY `A`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 4, "Expected 4, got %d\n", r);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_deleterow(void)
{
    MSIHANDLE hdb, hview, hrec;
    const char *query;
    char buf[MAX_PATH];
    UINT r;
    DWORD size;

    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Table` (`A`) VALUES ('one')";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Table` (`A`) VALUES ('two')";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "DELETE FROM `Table` WHERE `A` = 'one'";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hdb);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_READONLY, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `Table`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "two"), "Expected two, got %s\n", buf);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static const CHAR import_dat[] = "A\n"
                                 "s72\n"
                                 "Table\tA\n"
                                 "This is a new 'string' ok\n";

static void test_quotes(void)
{
    MSIHANDLE hdb, hview, hrec;
    const char *query;
    char buf[MAX_PATH];
    UINT r;
    DWORD size;

    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Table` ( `A` ) VALUES ( 'This is a 'string' ok' )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "INSERT INTO `Table` ( `A` ) VALUES ( \"This is a 'string' ok\" )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "INSERT INTO `Table` ( `A` ) VALUES ( \"test\" )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "INSERT INTO `Table` ( `A` ) VALUES ( 'This is a ''string'' ok' )";
    r = run_query(hdb, 0, query);
    todo_wine
    {
        ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    }

    query = "INSERT INTO `Table` ( `A` ) VALUES ( 'This is a '''string''' ok' )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "INSERT INTO `Table` ( `A` ) VALUES ( 'This is a \'string\' ok' )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "INSERT INTO `Table` ( `A` ) VALUES ( 'This is a \"string\" ok' )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `Table`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(!lstrcmp(buf, "This is a \"string\" ok"),
           "Expected \"This is a \"string\" ok\", got %s\n", buf);
    }

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    todo_wine
    {
        ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    }

    MsiCloseHandle(hview);

    write_file("import.idt", import_dat, (sizeof(import_dat) - 1) * sizeof(char));

    r = MsiDatabaseImportA(hdb, CURR_DIR, "import.idt");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    DeleteFileA("import.idt");

    query = "SELECT * FROM `Table`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(buf, "This is a new 'string' ok"),
       "Expected \"This is a new 'string' ok\", got %s\n", buf);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiCloseHandle(hview);

    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_carriagereturn(void)
{
    MSIHANDLE hdb, hview, hrec;
    const char *query;
    char buf[MAX_PATH];
    UINT r;
    DWORD size;

    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `Table`\r ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    todo_wine
    {
        ok(r == ERROR_BAD_QUERY_SYNTAX,
           "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    }

    query = "CREATE TABLE `Table` \r( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE\r TABLE `Table` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE\r `Table` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `Table` (\r `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `Table` ( `A`\r CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `Table` ( `A` CHAR(72)\r NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `Table` ( `A` CHAR(72) NOT\r NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `Table` ( `A` CHAR(72) NOT \rNULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL\r PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL \rPRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL PRIMARY\r KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL PRIMARY \rKEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL PRIMARY KEY\r `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A`\r )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )\r";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `\rOne` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `Tw\ro` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `Three\r` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `Four` ( `A\r` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `Four` ( `\rA` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `Four` ( `A` CHAR(72\r) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    todo_wine
    {
        ok(r == ERROR_BAD_QUERY_SYNTAX,
           "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    }

    query = "CREATE TABLE `Four` ( `A` CHAR(\r72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `Four` ( `A` CHAR(72) NOT NULL PRIMARY KEY `\rA` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `Four` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A\r` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `Four` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A\r` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "SELECT * FROM `_Tables`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(!lstrcmpA(buf, "\rOne"), "Expected \"\\rOne\", got \"%s\"\n", buf);
    }

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(!lstrcmpA(buf, "Tw\ro"), "Expected \"Tw\\ro\", got \"%s\"\n", buf);
    }

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(!lstrcmpA(buf, "Three\r"),
           "Expected \"Three\r\", got \"%s\"\n", buf);
    }

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    todo_wine
    {
        ok(r == ERROR_NO_MORE_ITEMS,
           "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);
    }

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_noquotes(void)
{
    MSIHANDLE hdb, hview, hrec;
    const char *query;
    char buf[MAX_PATH];
    UINT r;
    DWORD size;

    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE Table ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `Table` ( A CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `Table2` ( `A` CHAR(72) NOT NULL PRIMARY KEY A )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `Table3` ( A CHAR(72) NOT NULL PRIMARY KEY A )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `_Tables`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "Table"), "Expected \"Table\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "Table2"), "Expected \"Table2\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "Table3"), "Expected \"Table3\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT * FROM `_Columns`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "Table"), "Expected \"Table\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 1, "Expected 1, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 3, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "A"), "Expected \"A\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "Table2"), "Expected \"Table2\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 1, "Expected 1, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 3, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "A"), "Expected \"A\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "Table3"), "Expected \"Table3\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 1, "Expected 1, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 3, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "A"), "Expected \"A\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "INSERT INTO Table ( `A` ) VALUES ( 'hi' )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "INSERT INTO `Table` ( A ) VALUES ( 'hi' )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Table` ( `A` ) VALUES ( hi )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "SELECT * FROM Table WHERE `A` = 'hi'";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "SELECT * FROM `Table` WHERE `A` = hi";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "SELECT * FROM Table";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "SELECT * FROM Table2";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT * FROM `Table` WHERE A = 'hi'";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "hi"), "Expected \"hi\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);


    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void read_file_data(LPCSTR filename, LPSTR buffer)
{
    HANDLE file;
    DWORD read;

    file = CreateFileA( filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    ZeroMemory(buffer, MAX_PATH);
    ReadFile(file, buffer, MAX_PATH, &read, NULL);
    CloseHandle(file);
}

static void test_forcecodepage(void)
{
    MSIHANDLE hdb;
    const char *query;
    char buffer[MAX_PATH];
    UINT r;

    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `_ForceCodepage`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `_ForceCodepage`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `_ForceCodepage`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    MsiCloseHandle(hdb);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_DIRECT, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `_ForceCodepage`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    r = MsiDatabaseExport(hdb, "_ForceCodepage", CURR_DIR, "forcecodepage.idt");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    read_file_data("forcecodepage.idt", buffer);
    ok(!lstrcmpA(buffer, "\r\n\r\n0\t_ForceCodepage\r\n"),
       "Expected \"\r\n\r\n0\t_ForceCodepage\r\n\", got \"%s\"", buffer);

    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
    DeleteFileA("forcecodepage.idt");
}

static void test_viewmodify_refresh(void)
{
    MSIHANDLE hdb, hview, hrec;
    const char *query;
    char buffer[MAX_PATH];
    UINT r;
    DWORD size;

    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL, `B` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Table` ( `A`, `B` ) VALUES ( 'hi', 1 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `Table`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "UPDATE `Table` SET `B` = 2 WHERE `A` = 'hi'";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewModify(hview, MSIMODIFY_REFRESH, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, buffer, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buffer, "hi"), "Expected \"hi\", got \"%s\"\n", buffer);
    ok(size == 2, "Expected 2, got %d\n", size);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 2, "Expected 2, got %d\n", r);

    MsiCloseHandle(hrec);
    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "INSERT INTO `Table` ( `A`, `B` ) VALUES ( 'hello', 3 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `Table` WHERE `B` = 3";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "UPDATE `Table` SET `B` = 2 WHERE `A` = 'hello'";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Table` ( `A`, `B` ) VALUES ( 'hithere', 3 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewModify(hview, MSIMODIFY_REFRESH, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, buffer, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buffer, "hello"), "Expected \"hello\", got \"%s\"\n", buffer);
    ok(size == 5, "Expected 5, got %d\n", size);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 2, "Expected 2, got %d\n", r);

    MsiCloseHandle(hrec);
    MsiViewClose(hview);
    MsiCloseHandle(hview);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_where_viewmodify(void)
{
    MSIHANDLE hdb, hview, hrec;
    const char *query;
    UINT r;

    DeleteFile(msifile);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `Table` ( `A` INT, `B` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Table` ( `A`, `B` ) VALUES ( 1, 2 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Table` ( `A`, `B` ) VALUES ( 3, 4 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Table` ( `A`, `B` ) VALUES ( 5, 6 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* `B` = 3 doesn't match, but the view shouldn't be executed */
    query = "SELECT * FROM `Table` WHERE `B` = 3";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    hrec = MsiCreateRecord(2);
    MsiRecordSetInteger(hrec, 1, 7);
    MsiRecordSetInteger(hrec, 2, 8);

    r = MsiViewModify(hview, MSIMODIFY_INSERT_TEMPORARY, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hrec);
    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT * FROM `Table` WHERE `A` = 7";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 7, "Expected 7, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 8, "Expected 8, got %d\n", r);

    MsiRecordSetInteger(hrec, 2, 9);

    r = MsiViewModify(hview, MSIMODIFY_UPDATE, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hrec);
    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT * FROM `Table` WHERE `A` = 7";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 7, "Expected 7, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 9, "Expected 9, got %d\n", r);

    query = "UPDATE `Table` SET `B` = 10 WHERE `A` = 7";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewModify(hview, MSIMODIFY_REFRESH, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 7, "Expected 7, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 10, "Expected 10, got %d\n", r);

    MsiCloseHandle(hrec);
    MsiViewClose(hview);
    MsiCloseHandle(hview);
    MsiCloseHandle(hdb);
}

static BOOL create_storage(LPCSTR name)
{
    WCHAR nameW[MAX_PATH];
    IStorage *stg;
    IStream *stm;
    HRESULT hr;
    DWORD count;
    BOOL res = FALSE;

    MultiByteToWideChar(CP_ACP, 0, name, -1, nameW, MAX_PATH);
    hr = StgCreateDocfile(nameW, STGM_CREATE | STGM_READWRITE |
                          STGM_DIRECT | STGM_SHARE_EXCLUSIVE, 0, &stg);
    if (FAILED(hr))
        return FALSE;

    hr = IStorage_CreateStream(stg, nameW, STGM_WRITE | STGM_SHARE_EXCLUSIVE,
                               0, 0, &stm);
    if (FAILED(hr))
        goto done;

    hr = IStream_Write(stm, "stgdata", 8, &count);
    if (SUCCEEDED(hr))
        res = TRUE;

done:
    IStream_Release(stm);
    IStorage_Release(stg);

    return res;
}

static void test_storages_table(void)
{
    MSIHANDLE hdb, hview, hrec;
    IStorage *stg, *inner;
    IStream *stm;
    char file[MAX_PATH];
    char buf[MAX_PATH];
    WCHAR name[MAX_PATH];
    LPCSTR query;
    HRESULT hr;
    DWORD size;
    UINT r;

    hdb = create_db();
    ok(hdb, "failed to create db\n");

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS , "Failed to commit database\n");

    MsiCloseHandle(hdb);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_TRANSACT, &hdb);
    ok(r == ERROR_SUCCESS , "Failed to open database\n");

    /* check the column types */
    hrec = get_column_info(hdb, "SELECT * FROM `_Storages`", MSICOLINFO_TYPES);
    ok(hrec, "failed to get column info hrecord\n");
    ok(check_record(hrec, 1, "s62"), "wrong hrecord type\n");
    ok(check_record(hrec, 2, "V0"), "wrong hrecord type\n");

    MsiCloseHandle(hrec);

    /* now try the names */
    hrec = get_column_info(hdb, "SELECT * FROM `_Storages`", MSICOLINFO_NAMES);
    ok(hrec, "failed to get column info hrecord\n");
    ok(check_record(hrec, 1, "Name"), "wrong hrecord type\n");
    ok(check_record(hrec, 2, "Data"), "wrong hrecord type\n");

    MsiCloseHandle(hrec);

    create_storage("storage.bin");

    hrec = MsiCreateRecord(2);
    MsiRecordSetString(hrec, 1, "stgname");

    r = MsiRecordSetStream(hrec, 2, "storage.bin");
    ok(r == ERROR_SUCCESS, "Failed to add stream data to the hrecord: %d\n", r);

    DeleteFileA("storage.bin");

    query = "INSERT INTO `_Storages` (`Name`, `Data`) VALUES (?, ?)";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Failed to open database hview: %d\n", r);

    r = MsiViewExecute(hview, hrec);
    ok(r == ERROR_SUCCESS, "Failed to execute hview: %d\n", r);

    MsiCloseHandle(hrec);
    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT `Name`, `Data` FROM `_Storages`";
    r = MsiDatabaseOpenView(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Failed to open database hview: %d\n", r);

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Failed to execute hview: %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Failed to fetch hrecord: %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, file, &size);
    ok(r == ERROR_SUCCESS, "Failed to get string: %d\n", r);
    ok(!lstrcmp(file, "stgname"), "Expected \"stgname\", got \"%s\"\n", file);

    size = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiRecordReadStream(hrec, 2, buf, &size);
    ok(!lstrcmp(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    todo_wine
    {
        ok(r == ERROR_INVALID_DATA, "Expected ERROR_INVALID_DATA, got %d\n", r);
        ok(size == 0, "Expected 0, got %d\n", size);
    }

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    MsiDatabaseCommit(hdb);
    MsiCloseHandle(hdb);

    MultiByteToWideChar(CP_ACP, 0, msifile, -1, name, MAX_PATH);
    hr = StgOpenStorage(name, NULL, STGM_DIRECT | STGM_READ |
                        STGM_SHARE_DENY_WRITE, NULL, 0, &stg);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(stg != NULL, "Expected non-NULL storage\n");

    MultiByteToWideChar(CP_ACP, 0, "stgname", -1, name, MAX_PATH);
    hr = IStorage_OpenStorage(stg, name, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE,
                              NULL, 0, &inner);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(inner != NULL, "Expected non-NULL storage\n");

    MultiByteToWideChar(CP_ACP, 0, "storage.bin", -1, name, MAX_PATH);
    hr = IStorage_OpenStream(inner, name, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(stm != NULL, "Expected non-NULL stream\n");

    hr = IStream_Read(stm, buf, MAX_PATH, &size);
    ok(hr == S_OK, "Expected S_OK, got %d\n", hr);
    ok(size == 8, "Expected 8, got %d\n", size);
    ok(!lstrcmpA(buf, "stgdata"), "Expected \"stgdata\", got \"%s\"\n", buf);

    IStream_Release(stm);
    IStorage_Release(inner);

    IStorage_Release(stg);
    DeleteFileA(msifile);
}

static void test_dbtopackage(void)
{
    MSIHANDLE hdb, hpkg;
    CHAR package[10];
    CHAR buf[MAX_PATH];
    DWORD size;
    UINT r;

    /* create an empty database, transact mode */
    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Failed to create database\n");

    set_summary_info(hdb);

    r = create_directory_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_custom_action_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_custom_action_entry(hdb, "'SetProp', 51, 'MYPROP', 'grape'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sprintf(package, "#%i", hdb);
    r = MsiOpenPackage(package, &hpkg);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* property is not set yet */
    size = MAX_PATH;
    lstrcpyA(buf, "kiwi");
    r = MsiGetProperty(hpkg, "MYPROP", buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(size == 0, "Expected 0, got %d\n", size);

    /* run the custom action to set the property */
    r = MsiDoAction(hpkg, "SetProp");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* property is now set */
    size = MAX_PATH;
    lstrcpyA(buf, "kiwi");
    r = MsiGetProperty(hpkg, "MYPROP", buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "grape"), "Expected \"grape\", got \"%s\"\n", buf);
    ok(size == 5, "Expected 5, got %d\n", size);

    MsiCloseHandle(hpkg);

    /* reset the package */
    r = MsiOpenPackage(package, &hpkg);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* property is not set anymore */
    size = MAX_PATH;
    lstrcpyA(buf, "kiwi");
    r = MsiGetProperty(hpkg, "MYPROP", buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
        ok(size == 0, "Expected 0, got %d\n", size);
    }

    MsiCloseHandle(hdb);
    MsiCloseHandle(hpkg);

    /* create an empty database, direct mode */
    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATEDIRECT, &hdb);
    ok(r == ERROR_SUCCESS, "Failed to create database\n");

    set_summary_info(hdb);

    r = create_directory_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_custom_action_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_custom_action_entry(hdb, "'SetProp', 51, 'MYPROP', 'grape'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sprintf(package, "#%i", hdb);
    r = MsiOpenPackage(package, &hpkg);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* property is not set yet */
    size = MAX_PATH;
    lstrcpyA(buf, "kiwi");
    r = MsiGetProperty(hpkg, "MYPROP", buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(size == 0, "Expected 0, got %d\n", size);

    /* run the custom action to set the property */
    r = MsiDoAction(hpkg, "SetProp");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* property is now set */
    size = MAX_PATH;
    lstrcpyA(buf, "kiwi");
    r = MsiGetProperty(hpkg, "MYPROP", buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "grape"), "Expected \"grape\", got \"%s\"\n", buf);
    ok(size == 5, "Expected 5, got %d\n", size);

    MsiCloseHandle(hpkg);

    /* reset the package */
    r = MsiOpenPackage(package, &hpkg);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* property is not set anymore */
    size = MAX_PATH;
    lstrcpyA(buf, "kiwi");
    r = MsiGetProperty(hpkg, "MYPROP", buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
        ok(size == 0, "Expected 0, got %d\n", size);
    }

    MsiCloseHandle(hdb);
    MsiCloseHandle(hpkg);
    DeleteFileA(msifile);
}

static void test_droptable(void)
{
    MSIHANDLE hdb, hview, hrec;
    CHAR buf[MAX_PATH];
    LPCSTR query;
    DWORD size;
    UINT r;

    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( `A` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `One`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    query = "SELECT * FROM `_Tables` WHERE `Name` = 'One'";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "One"), "Expected \"One\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);
    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT * FROM `_Columns` WHERE `Table` = 'One'";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "One"), "Expected \"One\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 1, "Expected 1, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 3, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "A"), "Expected \"A\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS,
       "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "DROP `One`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "DROP TABLE";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "DROP TABLE `One`";
    hview = 0;
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_FUNCTION_FAILED,
       "Expected ERROR_FUNCTION_FAILED, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT * FROM `IDontExist`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "SELECT * FROM `One`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `One` ( `A` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "DROP TABLE One";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `One`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "SELECT * FROM `_Tables` WHERE `Name` = 'One'";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    query = "SELECT * FROM `_Columns` WHERE `Table` = 'One'";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    query = "CREATE TABLE `One` ( `B` INT, `C` INT PRIMARY KEY `B` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `One`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    query = "SELECT * FROM `_Tables` WHERE `Name` = 'One'";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "One"), "Expected \"One\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);
    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT * FROM `_Columns` WHERE `Table` = 'One'";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "One"), "Expected \"One\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 1, "Expected 1, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 3, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "B"), "Expected \"B\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "One"), "Expected \"One\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 2, "Expected 2, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 3, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "C"), "Expected \"C\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS,
       "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "DROP TABLE One";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `One`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "SELECT * FROM `_Tables` WHERE `Name` = 'One'";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    query = "SELECT * FROM `_Columns` WHERE `Table` = 'One'";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_dbmerge(void)
{
    MSIHANDLE hdb, href, hview, hrec;
    CHAR buf[MAX_PATH];
    LPCSTR query;
    DWORD size;
    UINT r;

    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiOpenDatabase("refdb.msi", MSIDBOPEN_CREATE, &href);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* hDatabase is invalid */
    r = MsiDatabaseMergeA(0, href, "MergeErrors");
    ok(r == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", r);

    /* hDatabaseMerge is invalid */
    r = MsiDatabaseMergeA(hdb, 0, "MergeErrors");
    ok(r == ERROR_INVALID_HANDLE,
       "Expected ERROR_INVALID_HANDLE, got %d\n", r);

    /* szTableName is NULL */
    r = MsiDatabaseMergeA(hdb, href, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* szTableName is empty */
    r = MsiDatabaseMergeA(hdb, href, "");
    ok(r == ERROR_INVALID_TABLE, "Expected ERROR_INVALID_TABLE, got %d\n", r);

    /* both DBs empty, szTableName is valid */
    r = MsiDatabaseMergeA(hdb, href, "MergeErrors");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( `A` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( `A` CHAR(72) PRIMARY KEY `A` )";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* column types don't match */
    r = MsiDatabaseMergeA(hdb, href, "MergeErrors");
    ok(r == ERROR_DATATYPE_MISMATCH,
       "Expected ERROR_DATATYPE_MISMATCH, got %d\n", r);

    /* nothing in MergeErrors */
    query = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "DROP TABLE `One`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "DROP TABLE `One`";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( `A` INT, `B` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( `A` INT, `C` INT PRIMARY KEY `A` )";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* column names don't match */
    r = MsiDatabaseMergeA(hdb, href, "MergeErrors");
    ok(r == ERROR_DATATYPE_MISMATCH,
       "Expected ERROR_DATATYPE_MISMATCH, got %d\n", r);

    /* nothing in MergeErrors */
    query = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "DROP TABLE `One`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "DROP TABLE `One`";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( `A` INT, `B` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( `A` INT, `B` INT PRIMARY KEY `B` )";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* primary keys don't match */
    r = MsiDatabaseMergeA(hdb, href, "MergeErrors");
    ok(r == ERROR_DATATYPE_MISMATCH,
       "Expected ERROR_DATATYPE_MISMATCH, got %d\n", r);

    /* nothing in MergeErrors */
    query = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "DROP TABLE `One`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "DROP TABLE `One`";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( `A` INT, `B` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( `A` INT, `B` INT PRIMARY KEY `A`, `B` )";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* number of primary keys doesn't match */
    r = MsiDatabaseMergeA(hdb, href, "MergeErrors");
    ok(r == ERROR_DATATYPE_MISMATCH,
       "Expected ERROR_DATATYPE_MISMATCH, got %d\n", r);

    /* nothing in MergeErrors */
    query = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "DROP TABLE `One`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "DROP TABLE `One`";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( `A` INT, `B` INT, `C` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( `A` INT, `B` INT PRIMARY KEY `A` )";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 1, 2 )";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* number of columns doesn't match */
    r = MsiDatabaseMergeA(hdb, href, "MergeErrors");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `One`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 2, "Expected 2, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 3);
    ok(r == MSI_NULL_INTEGER, "Expected MSI_NULL_INTEGER, got %d\n", r);

    MsiCloseHandle(hrec);

    /* nothing in MergeErrors */
    query = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "DROP TABLE `One`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "DROP TABLE `One`";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( `A` INT, `B` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( `A` INT, `B` INT, `C` INT PRIMARY KEY `A` )";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `One` ( `A`, `B`, `C` ) VALUES ( 1, 2, 3 )";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* number of columns doesn't match */
    r = MsiDatabaseMergeA(hdb, href, "MergeErrors");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `One`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 2, "Expected 2, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 3);
    ok(r == MSI_NULL_INTEGER, "Expected MSI_NULL_INTEGER, got %d\n", r);

    MsiCloseHandle(hrec);

    /* nothing in MergeErrors */
    query = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "DROP TABLE `One`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "DROP TABLE `One`";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( `A` INT, `B` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 1, 1 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 2, 2 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( `A` INT, `B` INT PRIMARY KEY `A` )";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 1, 2 )";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 2, 3 )";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* primary keys match, rows do not */
    r = MsiDatabaseMergeA(hdb, href, "MergeErrors");
    ok(r == ERROR_FUNCTION_FAILED,
       "Expected ERROR_FUNCTION_FAILED, got %d\n", r);

    /* nothing in MergeErrors */
    query = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "One"), "Expected \"One\", got \"%s\"\n", buf);

    r = MsiRecordGetInteger(hrec, 2);
    ok(r == 2, "Expected 2, got %d\n", r);

    MsiCloseHandle(hrec);

    r = MsiDatabaseOpenViewA(hdb, "SELECT * FROM `MergeErrors`", &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewGetColumnInfo(hview, MSICOLINFO_NAMES, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(!lstrcmpA(buf, "Table"), "Expected \"Table\", got \"%s\"\n", buf);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 2, buf, &size);
    ok(!lstrcmpA(buf, "NumRowMergeConflicts"),
       "Expected \"NumRowMergeConflicts\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    r = MsiViewGetColumnInfo(hview, MSICOLINFO_TYPES, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 1, buf, &size);
    ok(!lstrcmpA(buf, "s255"), "Expected \"s255\", got \"%s\"\n", buf);

    size = MAX_PATH;
    r = MsiRecordGetString(hrec, 2, buf, &size);
    ok(!lstrcmpA(buf, "i2"), "Expected \"i2\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);
    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "DROP TABLE `MergeErrors`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    create_file_data("codepage.idt", "\r\n\r\n850\t_ForceCodepage\r\n", 0);

    GetCurrentDirectoryA(MAX_PATH, buf);
    r = MsiDatabaseImportA(hdb, buf, "codepage.idt");
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    }

    query = "DROP TABLE `One`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "DROP TABLE `One`";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( "
            "`A` INT, `B` CHAR(72) LOCALIZABLE PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( "
            "`A` INT, `B` CHAR(72) LOCALIZABLE PRIMARY KEY `A` )";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 1, 'hi' )";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* code page does not match */
    r = MsiDatabaseMergeA(hdb, href, "MergeErrors");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `One`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 2, buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "hi"), "Expected \"hi\", got \"%s\"\n", buf);

    MsiCloseHandle(hrec);

    /* nothing in MergeErrors */
    query = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "DROP TABLE `One`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "DROP TABLE `One`";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( `A` INT, `B` OBJECT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( `A` INT, `B` OBJECT PRIMARY KEY `A` )";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    create_file("binary.dat");
    hrec = MsiCreateRecord(1);
    MsiRecordSetStreamA(hrec, 1, "binary.dat");

    query = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 1, ? )";
    r = run_query(href, hrec, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hrec);

    /* binary data to merge */
    r = MsiDatabaseMergeA(hdb, href, "MergeErrors");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `One`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "Expected 1, got %d\n", r);

    size = MAX_PATH;
    ZeroMemory(buf, MAX_PATH);
    r = MsiRecordReadStream(hrec, 2, buf, &size);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
        ok(!lstrcmpA(buf, "binary.dat\n"),
           "Expected \"binary.dat\\n\", got \"%s\"\n", buf);
    }

    MsiCloseHandle(hrec);

    /* nothing in MergeErrors */
    query = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    MsiCloseHandle(hdb);
    MsiCloseHandle(href);
    DeleteFileA(msifile);
    DeleteFileA("refdb.msi");
    DeleteFileA("codepage.idt");
    DeleteFileA("binary.dat");
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
    test_handle_limit();
    test_try_transform();
    test_join();
    test_temporary_table();
    test_alter();
    test_integers();
    test_update();
    test_special_tables();
    test_select_markers();
    test_viewmodify_update();
    test_stringtable();
    test_viewmodify_delete();
    test_defaultdatabase();
    test_order();
    test_viewmodify_delete_temporary();
    test_deleterow();
    test_quotes();
    test_carriagereturn();
    test_noquotes();
    test_forcecodepage();
    test_viewmodify_refresh();
    test_where_viewmodify();
    test_storages_table();
    test_dbtopackage();
    test_droptable();
    test_dbmerge();
}
