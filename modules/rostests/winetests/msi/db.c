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
#include <objidl.h>
#include <msi.h>
#include <msidefs.h>
#include <msiquery.h>

#include "wine/test.h"

static const char *msifile = "winetest-db.msi";
static const char *msifile2 = "winetst2-db.msi";
static const char *mstfile = "winetst-db.mst";
static const WCHAR msifileW[] = L"winetest-db.msi";
static const WCHAR msifile2W[] = L"winetst2-db.msi";

static void WINAPIV check_record_(int line, MSIHANDLE rec, UINT count, ...)
{
    va_list args;
    UINT i;

    ok_(__FILE__, line)(count == MsiRecordGetFieldCount(rec),
            "expected %u fields, got %u\n", count, MsiRecordGetFieldCount(rec));

    va_start(args, count);

    for (i = 1; i <= count; ++i)
    {
        const char *expect = va_arg(args, const char *);
        char buffer[200] = "x";
        DWORD sz = sizeof(buffer);
        UINT r = MsiRecordGetStringA(rec, i, buffer, &sz);
        ok_(__FILE__, line)(r == ERROR_SUCCESS, "field %u: got unexpected error %u\n", i, r);
        ok_(__FILE__, line)(!strcmp(buffer, expect),
                "field %u: expected \"%s\", got \"%s\"\n", i, expect, buffer);
    }

    va_end(args);
}
#define check_record(rec, ...) check_record_(__LINE__, rec, __VA_ARGS__)

static void test_msidatabase(void)
{
    MSIHANDLE hdb = 0, hdb2 = 0;
    WCHAR path[MAX_PATH];
    DWORD len;
    UINT res;

    DeleteFileW(msifileW);

    res = MsiOpenDatabaseW( msifileW, msifile2W, &hdb );
    ok( res == ERROR_OPEN_FAILED, "expected failure\n");

    res = MsiOpenDatabaseW( msifileW, (LPWSTR)0xff, &hdb );
    ok( res == ERROR_INVALID_PARAMETER, "expected failure\n");

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    /* create an empty database */
    res = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to create database\n" );

    res = MsiDatabaseCommit( hdb );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    ok( GetFileAttributesA( msifile ) != INVALID_FILE_ATTRIBUTES, "database should exist\n");

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );
    res = MsiOpenDatabaseW( msifileW, msifile2W, &hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to open database\n" );

    ok( GetFileAttributesA( msifile2 ) != INVALID_FILE_ATTRIBUTES, "database should exist\n");

    res = MsiDatabaseCommit( hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    res = MsiCloseHandle( hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    res = MsiOpenDatabaseW( msifileW, msifile2W, &hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to open database\n" );

    res = MsiCloseHandle( hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    ok( GetFileAttributesA( msifile2 ) == INVALID_FILE_ATTRIBUTES, "uncommitted database should not exist\n");

    res = MsiOpenDatabaseW( msifileW, msifile2W, &hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    res = MsiDatabaseCommit( hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    res = MsiCloseHandle( hdb2 );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    ok( GetFileAttributesA( msifile2 ) != INVALID_FILE_ATTRIBUTES, "committed database should exist\n");

    res = MsiOpenDatabaseW( msifileW, MSIDBOPEN_READONLY, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to open database\n" );

    res = MsiDatabaseCommit( hdb );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    res = MsiOpenDatabaseW( msifileW, MSIDBOPEN_DIRECT, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to open database\n" );

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    res = MsiOpenDatabaseW( msifileW, MSIDBOPEN_TRANSACT, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to open database\n" );

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );
    ok( GetFileAttributesA( msifile ) != INVALID_FILE_ATTRIBUTES, "database should exist\n");

    /* MSIDBOPEN_CREATE deletes the database if MsiCommitDatabase isn't called */
    res = MsiOpenDatabaseW( msifileW, MSIDBOPEN_CREATE, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to open database\n" );

    ok( GetFileAttributesA( msifile ) != INVALID_FILE_ATTRIBUTES, "database should exist\n");

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    ok( GetFileAttributesA( msifile ) == INVALID_FILE_ATTRIBUTES, "database should exist\n");

    res = MsiOpenDatabaseW( msifileW, MSIDBOPEN_CREATE, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to open database\n" );

    res = MsiDatabaseCommit( hdb );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    ok( GetFileAttributesA( msifile ) != INVALID_FILE_ATTRIBUTES, "database should exist\n");

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    res = GetCurrentDirectoryW(ARRAY_SIZE(path), path);
    ok ( res, "Got zero res.\n" );
    lstrcatW( path, L"\\");
    lstrcatW( path, msifileW);
    len = lstrlenW(path);
    path[len - 4] = 0;

    res = MsiOpenDatabaseW( path, MSIDBOPEN_READONLY, &hdb );
    ok( res != ERROR_SUCCESS , "Got unexpected res %u.\n", res );

    lstrcpyW( path, msifileW );
    path[lstrlenW(path) - 4] = 0;

    res = MsiOpenDatabaseW( path, MSIDBOPEN_READONLY, &hdb );
    ok( res != ERROR_SUCCESS , "Got unexpected res %u.\n", res );

    res = DeleteFileA( msifile2 );
    ok( res == TRUE, "Failed to delete database\n" );

    res = DeleteFileA( msifile );
    ok( res == TRUE, "Failed to delete database\n" );
}

static UINT do_query(MSIHANDLE hdb, const char *query, MSIHANDLE *phrec)
{
    MSIHANDLE hview = 0;
    UINT r, ret;

    if (phrec)
        *phrec = 0;

    /* open a select query */
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
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

    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    if( r != ERROR_SUCCESS )
        return r;

    r = MsiViewExecute(hview, hrec);
    if( r == ERROR_SUCCESS )
        r = MsiViewClose(hview);
    MsiCloseHandle(hview);
    return r;
}

static UINT run_queryW( MSIHANDLE hdb, MSIHANDLE hrec, const WCHAR *query )
{
    MSIHANDLE hview = 0;
    UINT r;

    r = MsiDatabaseOpenViewW(hdb, query, &hview);
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
    UINT r = run_query( hdb, 0,
            "CREATE TABLE `Component` ( "
            "`Component` CHAR(72) NOT NULL, "
            "`ComponentId` CHAR(38), "
            "`Directory_` CHAR(72) NOT NULL, "
            "`Attributes` SHORT NOT NULL, "
            "`Condition` CHAR(255), "
            "`KeyPath` CHAR(72) "
            "PRIMARY KEY `Component`)" );
    ok(r == ERROR_SUCCESS, "Failed to create Component table: %u\n", r);
    return r;
}

static UINT create_custom_action_table( MSIHANDLE hdb )
{
    UINT r = run_query( hdb, 0,
            "CREATE TABLE `CustomAction` ( "
            "`Action` CHAR(72) NOT NULL, "
            "`Type` SHORT NOT NULL, "
            "`Source` CHAR(72), "
            "`Target` CHAR(255) "
            "PRIMARY KEY `Action`)" );
    ok(r == ERROR_SUCCESS, "Failed to create CustomAction table: %u\n", r);
    return r;
}

static UINT create_directory_table( MSIHANDLE hdb )
{
    UINT r = run_query( hdb, 0,
            "CREATE TABLE `Directory` ( "
            "`Directory` CHAR(255) NOT NULL, "
            "`Directory_Parent` CHAR(255), "
            "`DefaultDir` CHAR(255) NOT NULL "
            "PRIMARY KEY `Directory`)" );
    ok(r == ERROR_SUCCESS, "Failed to create Directory table: %u\n", r);
    return r;
}

static UINT create_feature_components_table( MSIHANDLE hdb )
{
    UINT r = run_query( hdb, 0,
            "CREATE TABLE `FeatureComponents` ( "
            "`Feature_` CHAR(38) NOT NULL, "
            "`Component_` CHAR(72) NOT NULL "
            "PRIMARY KEY `Feature_`, `Component_` )" );
    ok(r == ERROR_SUCCESS, "Failed to create FeatureComponents table: %u\n", r);
    return r;
}

static UINT create_std_dlls_table( MSIHANDLE hdb )
{
    UINT r = run_query( hdb, 0,
            "CREATE TABLE `StdDlls` ( "
            "`File` CHAR(255) NOT NULL, "
            "`Binary_` CHAR(72) NOT NULL "
            "PRIMARY KEY `File` )" );
    ok(r == ERROR_SUCCESS, "Failed to create StdDlls table: %u\n", r);
    return r;
}

static UINT create_binary_table( MSIHANDLE hdb )
{
    UINT r = run_query( hdb, 0,
           "CREATE TABLE `Binary` ( "
            "`Name` CHAR(72) NOT NULL, "
            "`Data` CHAR(72) NOT NULL "
            "PRIMARY KEY `Name` )" );
    ok(r == ERROR_SUCCESS, "Failed to create Binary table: %u\n", r);
    return r;
}

static inline UINT add_entry(const char *file, int line, const char *type, MSIHANDLE hdb, const char *values, const char *insert)
{
    char *query;
    UINT sz, r;

    sz = strlen(values) + strlen(insert) + 1;
    query = malloc(sz);
    sprintf(query, insert, values);
    r = run_query(hdb, 0, query);
    free(query);
    ok_(file, line)(r == ERROR_SUCCESS, "failed to insert into %s table: %u\n", type, r);
    return r;
}

#define add_component_entry(hdb, values) add_entry(__FILE__, __LINE__, "Component", hdb, values, \
               "INSERT INTO `Component`  " \
               "(`Component`, `ComponentId`, `Directory_`, " \
               "`Attributes`, `Condition`, `KeyPath`) VALUES( %s )")

#define add_custom_action_entry(hdb, values) add_entry(__FILE__, __LINE__, "CustomAction", hdb, values, \
               "INSERT INTO `CustomAction`  " \
               "(`Action`, `Type`, `Source`, `Target`) VALUES( %s )")

#define add_feature_components_entry(hdb, values) add_entry(__FILE__, __LINE__, "FeatureComponents", hdb, values, \
               "INSERT INTO `FeatureComponents` " \
               "(`Feature_`, `Component_`) VALUES( %s )")

#define add_std_dlls_entry(hdb, values) add_entry(__FILE__, __LINE__, "StdDlls", hdb, values, \
               "INSERT INTO `StdDlls` (`File`, `Binary_`) VALUES( %s )")

#define add_binary_entry(hdb, values) add_entry(__FILE__, __LINE__, "Binary", hdb, values, \
               "INSERT INTO `Binary` (`Name`, `Data`) VALUES( %s )")

static void test_msiinsert(void)
{
    MSIHANDLE hdb = 0, hview = 0, hview2 = 0, hrec = 0;
    UINT r;
    const char *query;
    char buf[80];
    DWORD sz;

    DeleteFileA(msifile);

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    /* create a table */
    query = "CREATE TABLE `phone` ( "
            "`id` INT, `name` CHAR(32), `number` CHAR(32) "
            "PRIMARY KEY `id`)";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    query = "SELECT * FROM phone WHERE number = '8675309'";
    r = MsiDatabaseOpenViewA(hdb, query, &hview2);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview2, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewFetch(hview2, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "MsiViewFetch produced items\n");

    /* insert a value into it */
    query = "INSERT INTO `phone` ( `id`, `name`, `number` )"
        "VALUES('1', 'Abe', '8675309')";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiViewFetch(hview2, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "MsiViewFetch produced items\n");
    r = MsiViewExecute(hview2, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewFetch(hview2, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed: %u\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");
    r = MsiViewClose(hview2);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview2);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    query = "SELECT * FROM `phone` WHERE `id` = 1";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");

    /* check the record contains what we put in it */
    r = MsiRecordGetFieldCount(hrec);
    ok(r == 3, "record count wrong\n");

    r = MsiRecordIsNull(hrec, 0);
    ok(r == FALSE, "field 0 not null\n");

    r = MsiRecordGetInteger(hrec, 1);
    ok(r == 1, "field 1 contents wrong\n");
    sz = sizeof buf;
    r = MsiRecordGetStringA(hrec, 2, buf, &sz);
    ok(r == ERROR_SUCCESS, "field 2 content fetch failed\n");
    ok(!strcmp(buf,"Abe"), "field 2 content incorrect\n");
    sz = sizeof buf;
    r = MsiRecordGetStringA(hrec, 3, buf, &sz);
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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "MsiDatabaseOpenView failed\n");

    /* construct a record to insert */
    hrec = MsiCreateRecord(4);
    r = MsiRecordSetInteger(hrec, 1, 2);
    ok(r == ERROR_SUCCESS, "MsiRecordSetInteger failed\n");
    r = MsiRecordSetStringA(hrec, 2, "Adam");
    ok(r == ERROR_SUCCESS, "MsiRecordSetString failed\n");
    r = MsiRecordSetStringA(hrec, 3, "96905305");
    ok(r == ERROR_SUCCESS, "MsiRecordSetString failed\n");

    /* insert another value, using a record and wildcards */
    query = "INSERT INTO `phone` ( `id`, `name`, `number` )"
        "VALUES(?, ?, ?)";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
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

    r = DeleteFileA(msifile);
    ok(r == TRUE, "file didn't exist after commit\n");
}

static void test_msidecomposedesc(void)
{
    UINT (WINAPI *pMsiDecomposeDescriptorA)(LPCSTR, LPCSTR, LPSTR, LPSTR, DWORD *);
    char prod[MAX_FEATURE_CHARS+1], comp[MAX_FEATURE_CHARS+1], feature[MAX_FEATURE_CHARS+1];
    const char *desc;
    UINT r;
    DWORD len;
    HMODULE hmod;

    hmod = GetModuleHandleA("msi.dll");
    pMsiDecomposeDescriptorA = (void*)GetProcAddress(hmod, "MsiDecomposeDescriptorA");
    if (!pMsiDecomposeDescriptorA)
        return;

    /* test a valid feature descriptor */
    desc = "']gAVn-}f(ZXfeAR6.jiFollowTheWhiteRabbit>3w2x^IGfe?CxI5heAvk.";
    len = 0;
    prod[0] = feature[0] = comp[0] = 0;
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

    /* test a feature descriptor with < instead of > */
    desc = "']gAVn-}f(ZXfeAR6.jiFollowTheWhiteRabbit<3w2x^IGfe?CxI5heAvk.";
    len = 0;
    prod[0] = feature[0] = 0;
    comp[0] = 0x55;
    r = pMsiDecomposeDescriptorA(desc, prod, feature, comp, &len);
    ok(r == ERROR_SUCCESS, "returned an error\n");
    ok(len == 41, "got %lu\n", len);
    ok(!strcmp(prod,"{90110409-6000-11D3-8CFE-0150048383C9}"), "got '%s'\n", prod);
    ok(!strcmp(feature,"FollowTheWhiteRabbit"), "got '%s'\n", feature);
    ok(!comp[0], "got '%s'\n", comp);

    len = 0;
    prod[0] = feature[0] = 0;
    comp[0] = 0x55;
    r = pMsiDecomposeDescriptorA("yh1BVN)8A$!!!!!MKKSkAlwaysInstalledIntl_1033<", prod, feature, comp, &len);
    ok(r == ERROR_SUCCESS, "got %u\n", r);
    ok(len == 45, "got %lu\n", len);
    ok(!strcmp(prod, "{90150000-006E-0409-0000-0000000FF1CE}"), "got '%s'\n", prod);
    ok(!strcmp(feature, "AlwaysInstalledIntl_1033"), "got '%s'\n", feature);
    ok(!comp[0], "got '%s'\n", comp);

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

    r = pMsiDecomposeDescriptorA(desc, NULL, NULL, NULL, NULL);
    ok(r == ERROR_SUCCESS, "returned wrong error\n");
}

static UINT try_query_param( MSIHANDLE hdb, LPCSTR szQuery, MSIHANDLE hrec )
{
    MSIHANDLE htab = 0;
    UINT res;

    res = MsiDatabaseOpenViewA( hdb, szQuery, &htab );
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
    MsiRecordSetStringA( hrec, 1, "Hello");

    r = try_query_param( hdb, szQuery, hrec );

    MsiCloseHandle( hrec );
    return r;
}

static void test_msibadqueries(void)
{
    MSIHANDLE hdb = 0;
    UINT r;

    DeleteFileA(msifile);

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    r = MsiDatabaseCommit( hdb );
    ok(r == ERROR_SUCCESS , "Failed to commit database\n");

    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS , "Failed to close database\n");

    /* open it readonly */
    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_READONLY, &hdb );
    ok(r == ERROR_SUCCESS , "Failed to open database r/o\n");

    /* add a table to it */
    r = try_query( hdb, "select * from _Tables");
    ok(r == ERROR_SUCCESS , "query 1 failed\n");

    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS , "Failed to close database r/o\n");

    /* open it read/write */
    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_TRANSACT, &hdb );
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
    ok(r == ERROR_BAD_QUERY_SYNTAX, "query failed\n");

    r = try_query( hdb, "select * from c where b = 'x'");
    ok(r == ERROR_SUCCESS, "query failed\n");

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

    r = try_query( hdb, "select `c`.`b` from `c` order by `c`.`order`");
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = try_query( hdb, "select `c`.b` from `c`");
    ok( r == ERROR_SUCCESS, "query failed: %u\n", r );

    r = try_query( hdb, "select `c`.`b from `c`");
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = try_query( hdb, "select `c`.b from `c`");
    ok( r == ERROR_SUCCESS, "query failed: %u\n", r );

    r = try_query( hdb, "select `c.`b` from `c`");
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = try_query( hdb, "select c`.`b` from `c`");
    ok( r == ERROR_SUCCESS, "query failed: %u\n", r );

    r = try_query( hdb, "select c.`b` from `c`");
    ok( r == ERROR_SUCCESS, "query failed: %u\n", r );

    r = try_query( hdb, "select `c`.`b` from c`");
    ok( r == ERROR_SUCCESS, "query failed: %u\n", r );

    r = try_query( hdb, "select `c`.`b` from `c");
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = try_query( hdb, "select `c`.`b` from c");
    ok( r == ERROR_SUCCESS, "query failed: %u\n", r );

    r = try_query( hdb, "CREATE TABLE `\5a` (`b` CHAR NOT NULL PRIMARY KEY `b`)" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "SELECT * FROM \5a" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "CREATE TABLE `a\5` (`b` CHAR NOT NULL PRIMARY KEY `b`)" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "SELECT * FROM a\5" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "CREATE TABLE `-a` (`b` CHAR NOT NULL PRIMARY KEY `b`)" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "SELECT * FROM -a" );
    todo_wine ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "CREATE TABLE `a-` (`b` CHAR NOT NULL PRIMARY KEY `b`)" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "SELECT * FROM a-" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS , "Failed to close database transact\n");

    r = DeleteFileA( msifile );
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

    DeleteFileA(msifile);

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    query = "CREATE TABLE `phone` ( "
            "`id` INT, `name` CHAR(32), `number` CHAR(32) "
            "PRIMARY KEY `id`)";
    r = run_query( hdb, 0, query );
    ok(r == ERROR_SUCCESS, "query failed\n");

    query = "CREATE TABLE `_Validation` ( "
            "`Table` CHAR(32) NOT NULL, `Column` CHAR(32) NOT NULL, "
            "`Nullable` CHAR(4) NOT NULL, `MinValue` INT, `MaxValue` INT, "
            "`KeyTable` CHAR(255), `KeyColumn` SHORT, `Category` CHAR(32), "
            "`Set` CHAR(255), `Description` CHAR(255) PRIMARY KEY `Table`, `Column`)";
    r = run_query( hdb, 0, query );
    ok(r == ERROR_SUCCESS, "query failed\n");

    query = "INSERT INTO `_Validation` ( `Table`, `Column`, `Nullable` ) "
            "VALUES('phone', 'id', 'N')";
    r = run_query( hdb, 0, query );
    ok(r == ERROR_SUCCESS, "query failed\n");

    query = "SELECT * FROM `phone`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");

    /* check what the error function reports without doing anything */
    sz = sizeof(buffer);
    strcpy(buffer, "x");
    err = MsiViewGetErrorA( hview, buffer, &sz );
    ok(err == MSIDBERROR_NOERROR, "got %d\n", err);
    ok(!buffer[0], "got \"%s\"\n", buffer);
    ok(sz == 0, "got size %lu\n", sz);

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

    sz = sizeof(buffer);
    strcpy(buffer, "x");
    err = MsiViewGetErrorA( hview, buffer, &sz );
    ok(err == MSIDBERROR_NOERROR, "got %d\n", err);
    ok(!buffer[0], "got \"%s\"\n", buffer);
    ok(sz == 0, "got size %lu\n", sz);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    /* insert a valid record */
    hrec = MsiCreateRecord(3);

    r = MsiRecordSetInteger(hrec, 1, 1);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetStringA(hrec, 2, "bob");
    ok(r == ERROR_SUCCESS, "failed to set string\n");
    r = MsiRecordSetStringA(hrec, 3, "7654321");
    ok(r == ERROR_SUCCESS, "failed to set string\n");

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewModify(hview, MSIMODIFY_INSERT_TEMPORARY, hrec );
    ok(r == ERROR_SUCCESS, "MsiViewModify failed\n");

    /* validate it */
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    r = MsiViewModify(hview, MSIMODIFY_VALIDATE_NEW, hrec );
    ok(r == ERROR_INVALID_DATA, "MsiViewModify failed %u\n", r);

    sz = sizeof(buffer);
    strcpy(buffer, "x");
    err = MsiViewGetErrorA( hview, buffer, &sz );
    ok(err == MSIDBERROR_DUPLICATEKEY, "got %d\n", err);
    ok(!strcmp(buffer, "id"), "got \"%s\"\n", buffer);
    ok(sz == 2, "got size %lu\n", sz);

    /* insert the same thing again */
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    /* should fail ... */
    r = MsiViewModify(hview, MSIMODIFY_INSERT_TEMPORARY, hrec );
    ok(r == ERROR_FUNCTION_FAILED, "MsiViewModify failed\n");

    /* try to merge the same record */
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewModify(hview, MSIMODIFY_MERGE, hrec );
    ok(r == ERROR_SUCCESS, "MsiViewModify failed\n");

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    /* try merging a new record */
    hrec = MsiCreateRecord(3);

    r = MsiRecordSetInteger(hrec, 1, 10);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetStringA(hrec, 2, "pepe");
    ok(r == ERROR_SUCCESS, "failed to set string\n");
    r = MsiRecordSetStringA(hrec, 3, "7654321");
    ok(r == ERROR_SUCCESS, "failed to set string\n");

    r = MsiViewModify(hview, MSIMODIFY_MERGE, hrec );
    ok(r == ERROR_SUCCESS, "MsiViewModify failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    query = "SELECT * FROM `phone`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    check_record(hrec, 3, "1", "bob", "7654321");

    /* update the view, non-primary key */
    r = MsiRecordSetStringA(hrec, 3, "3141592");
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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    check_record(hrec, 3, "1", "bob", "3141592");
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    /* use a record that doesn't come from a view fetch */
    hrec = MsiCreateRecord(3);
    ok(hrec != 0, "MsiCreateRecord failed\n");

    r = MsiRecordSetInteger(hrec, 1, 3);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetStringA(hrec, 2, "jane");
    ok(r == ERROR_SUCCESS, "failed to set string\n");
    r = MsiRecordSetStringA(hrec, 3, "112358");
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
    r = MsiRecordSetStringA(hrec, 2, "jane");
    ok(r == ERROR_SUCCESS, "failed to set string\n");
    r = MsiRecordSetStringA(hrec, 3, "112358");
    ok(r == ERROR_SUCCESS, "failed to set string\n");

    r = MsiViewModify(hview, MSIMODIFY_UPDATE, hrec);
    ok(r == ERROR_FUNCTION_FAILED, "MsiViewModify failed\n");

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    hrec = MsiCreateRecord(3);

    r = MsiRecordSetInteger(hrec, 1, 2);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetStringA(hrec, 2, "nick");
    ok(r == ERROR_SUCCESS, "failed to set string\n");
    r = MsiRecordSetStringA(hrec, 3, "141421");
    ok(r == ERROR_SUCCESS, "failed to set string\n");

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewModify(hview, MSIMODIFY_INSERT_TEMPORARY, hrec );
    ok(r == ERROR_SUCCESS, "MsiViewModify failed\n");

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");
    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    query = "SELECT * FROM `phone` WHERE `id` = 1";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");

    /* change the id to match the second row */
    r = MsiRecordSetInteger(hrec, 1, 2);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetStringA(hrec, 2, "jerry");
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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");

    /* change the id to match the second row */
    r = MsiRecordSetInteger(hrec, 1, 2);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetStringA(hrec, 2, "jerry");
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

    DeleteFileW(msifileW);

    /* create an empty database */
    res = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb );
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

    /* create an empty db */
    hdb = create_db();
    ok( hdb, "failed to create db\n");

    /* tables should be present */
    r = MsiDatabaseOpenViewA(hdb, "select Name from _Tables", &hview);
    ok( r == ERROR_SUCCESS, "failed to open query\n");

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute query\n");

    /* check that NAMES works */
    rec = 0;
    r = MsiViewGetColumnInfo( hview, MSICOLINFO_NAMES, &rec );
    ok( r == ERROR_SUCCESS, "failed to get names\n");
    check_record(rec, 1, "Name");
    r = MsiCloseHandle( rec );
    ok( r == ERROR_SUCCESS, "failed to close record handle\n");

    /* check that TYPES works */
    rec = 0;
    r = MsiViewGetColumnInfo( hview, MSICOLINFO_TYPES, &rec );
    ok( r == ERROR_SUCCESS, "failed to get names\n");
    check_record(rec, 1, "s64");
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

    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    if( r != ERROR_SUCCESS )
        return r;

    r = MsiViewExecute(hview, 0);
    if( r == ERROR_SUCCESS )
    {
        MsiViewGetColumnInfo( hview, type, &rec );
    }
    MsiViewClose(hview);
    MsiCloseHandle(hview);
    return rec;
}

static UINT get_columns_table_type(MSIHANDLE hdb, const char *table, UINT field)
{
    MSIHANDLE hview = 0, rec = 0;
    UINT r, type = 0;
    char query[0x100];

    sprintf(query, "select * from `_Columns` where  `Table` = '%s'", table );

    r = MsiDatabaseOpenViewA(hdb, query, &hview);
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
    }
    MsiViewClose(hview);
    MsiCloseHandle(hview);
    return type;
}

static void test_viewgetcolumninfo(void)
{
    MSIHANDLE hdb = 0, rec;
    UINT r;

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    r = run_query( hdb, 0,
            "CREATE TABLE `Properties` "
            "( `Property` CHAR(255), "
            "  `Value` CHAR(1), "
            "  `Intvalue` INT, "
            "  `Integervalue` INTEGER, "
            "  `Shortvalue` SHORT, "
            "  `Longvalue` LONG, "
            "  `Longcharvalue` LONGCHAR, "
            "  `Charvalue` CHAR, "
            "  `Localizablevalue` CHAR LOCALIZABLE "
            "  PRIMARY KEY `Property`)" );
    ok( r == ERROR_SUCCESS , "Failed to create table\n" );

    /* check the column types */
    rec = get_column_info( hdb, "select * from `Properties`", MSICOLINFO_TYPES );
    ok( rec, "failed to get column info record\n" );
    check_record(rec, 9, "S255", "S1", "I2", "I2", "I2", "I4", "S0", "S0", "L0");
    MsiCloseHandle( rec );

    /* check the type in _Columns */
    ok( 0x3dff == get_columns_table_type(hdb, "Properties", 1 ), "_columns table wrong\n");
    ok( 0x1d01 == get_columns_table_type(hdb, "Properties", 2 ), "_columns table wrong\n");
    ok( 0x1502 == get_columns_table_type(hdb, "Properties", 3 ), "_columns table wrong\n");
    ok( 0x1502 == get_columns_table_type(hdb, "Properties", 4 ), "_columns table wrong\n");
    ok( 0x1502 == get_columns_table_type(hdb, "Properties", 5 ), "_columns table wrong\n");
    ok( 0x1104 == get_columns_table_type(hdb, "Properties", 6 ), "_columns table wrong\n");
    ok( 0x1d00 == get_columns_table_type(hdb, "Properties", 7 ), "_columns table wrong\n");
    ok( 0x1d00 == get_columns_table_type(hdb, "Properties", 8 ), "_columns table wrong\n");
    ok( 0x1f00 == get_columns_table_type(hdb, "Properties", 9 ), "_columns table wrong\n");

    /* now try the names */
    rec = get_column_info( hdb, "select * from `Properties`", MSICOLINFO_NAMES );
    ok( rec, "failed to get column info record\n" );
    check_record(rec, 9, "Property", "Value", "Intvalue", "Integervalue", "Shortvalue",
            "Longvalue", "Longcharvalue", "Charvalue", "Localizablevalue");
    MsiCloseHandle( rec );

    r = run_query( hdb, 0,
            "CREATE TABLE `Binary` "
            "( `Name` CHAR(255), `Data` OBJECT  PRIMARY KEY `Name`)" );
    ok( r == ERROR_SUCCESS , "Failed to create table\n" );

    /* check the column types */
    rec = get_column_info( hdb, "select * from `Binary`", MSICOLINFO_TYPES );
    ok( rec, "failed to get column info record\n" );
    check_record(rec, 2, "S255", "V0");
    MsiCloseHandle( rec );

    /* check the type in _Columns */
    ok( 0x3dff == get_columns_table_type(hdb, "Binary", 1 ), "_columns table wrong\n");
    ok( 0x1900 == get_columns_table_type(hdb, "Binary", 2 ), "_columns table wrong\n");

    /* now try the names */
    rec = get_column_info( hdb, "select * from `Binary`", MSICOLINFO_NAMES );
    ok( rec, "failed to get column info record\n" );
    check_record(rec, 2, "Name", "Data");
    MsiCloseHandle( rec );

    r = run_query( hdb, 0,
            "CREATE TABLE `UIText` "
            "( `Key` CHAR(72) NOT NULL, `Text` CHAR(255) LOCALIZABLE PRIMARY KEY `Key`)" );
    ok( r == ERROR_SUCCESS , "Failed to create table\n" );

    ok( 0x2d48 == get_columns_table_type(hdb, "UIText", 1 ), "_columns table wrong\n");
    ok( 0x1fff == get_columns_table_type(hdb, "UIText", 2 ), "_columns table wrong\n");

    rec = get_column_info( hdb, "select * from `UIText`", MSICOLINFO_NAMES );
    ok( rec, "failed to get column info record\n" );
    check_record(rec, 2, "Key", "Text");
    MsiCloseHandle( rec );

    rec = get_column_info( hdb, "select * from `UIText`", MSICOLINFO_TYPES );
    ok( rec, "failed to get column info record\n" );
    check_record(rec, 2, "s72", "L255");
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

    DeleteFileW(msifileW);

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    /* create a table */
    query = "CREATE TABLE `phone` ( "
            "`id` INT, `name` CHAR(32), `number` CHAR(32) "
            "PRIMARY KEY `id`)";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    GetCurrentDirectoryA(MAX_PATH, path);

    r = MsiDatabaseExportA(hdb, "phone", path, file);
    ok(r == ERROR_SUCCESS, "MsiDatabaseExport failed\n");

    MsiCloseHandle(hdb);

    lstrcatA(path, "\\");
    lstrcatA(path, file);

    /* check the data that was written */
    length = 0;
    memset(buffer, 0, sizeof buffer);
    handle = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (handle != INVALID_HANDLE_VALUE)
    {
        ReadFile(handle, buffer, sizeof buffer, &length, NULL);
        CloseHandle(handle);
        DeleteFileA(path);
    }
    else
        ok(0, "failed to open file %s\n", path);

    ok( length == strlen(expected), "length of data wrong\n");
    ok( !lstrcmpA(buffer, expected), "data doesn't match\n");
    DeleteFileA(msifile);
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

    DeleteFileW(msifileW);
    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    /* create a table */
    r = try_query( hdb,
        "CREATE TABLE `strings` ( `id` INT, `val` CHAR(0) PRIMARY KEY `id`)");
    ok(r == ERROR_SUCCESS, "query failed\n");

    /* try to insert a very long string */
    str = malloc(STRING_LENGTH + sizeof insert_query);
    len = strchr(insert_query, 'Z') - insert_query;
    strcpy(str, insert_query);
    memset(str+len, 'Z', STRING_LENGTH);
    strcpy(str+len+STRING_LENGTH, insert_query+len+1);
    r = try_query( hdb, str );
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    free(str);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "MsiDatabaseCommit failed\n");
    MsiCloseHandle(hdb);

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_READONLY, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    r = MsiDatabaseOpenViewA(hdb, "select * from `strings` where `id` = 1", &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    r = MsiRecordGetStringA(hrec, 2, NULL, &len);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    ok(len == STRING_LENGTH, "string length wrong\n");

    MsiCloseHandle(hrec);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
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
    MSIHANDLE hdb = 0, rec, view, hsi;
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

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_TRANSACT, &hdb );
    ok( r == ERROR_SUCCESS , "Failed to open database\n" );

    /* check the column types */
    rec = get_column_info( hdb, "select * from `_Streams`", MSICOLINFO_TYPES );
    ok( rec, "failed to get column info record\n" );
    check_record(rec, 2, "s62", "V0");
    MsiCloseHandle( rec );

    /* now try the names */
    rec = get_column_info( hdb, "select * from `_Streams`", MSICOLINFO_NAMES );
    ok( rec, "failed to get column info record\n" );
    check_record(rec, 2, "Name", "Data");
    MsiCloseHandle( rec );

    r = MsiDatabaseOpenViewA( hdb,
            "SELECT * FROM `_Streams` WHERE `Name` = '\5SummaryInformation'", &view );
    ok( r == ERROR_SUCCESS, "Failed to open database view: %u\n", r );

    r = MsiViewExecute( view, 0 );
    ok( r == ERROR_SUCCESS, "Failed to execute view: %u\n", r );

    r = MsiViewFetch( view, &rec );
    ok( r == ERROR_NO_MORE_ITEMS, "Unexpected result: %u\n", r );

    MsiCloseHandle( rec );
    MsiViewClose( view );
    MsiCloseHandle( view );

    /* create a summary information stream */
    r = MsiGetSummaryInformationA( hdb, NULL, 1, &hsi );
    ok( r == ERROR_SUCCESS, "Failed to get summary information handle: %u\n", r );

    r = MsiSummaryInfoSetPropertyA( hsi, PID_SECURITY, VT_I4, 2, NULL, NULL );
    ok( r == ERROR_SUCCESS, "Failed to set property: %u\n", r );

    r = MsiSummaryInfoPersist( hsi );
    ok( r == ERROR_SUCCESS, "Failed to save summary information: %u\n", r );

    MsiCloseHandle( hsi );

    r = MsiDatabaseOpenViewA( hdb,
            "SELECT * FROM `_Streams` WHERE `Name` = '\5SummaryInformation'", &view );
    ok( r == ERROR_SUCCESS, "Failed to open database view: %u\n", r );

    r = MsiViewExecute( view, 0 );
    ok( r == ERROR_SUCCESS, "Failed to execute view: %u\n", r );

    r = MsiViewFetch( view, &rec );
    ok( r == ERROR_SUCCESS, "Unexpected result: %u\n", r );

    MsiCloseHandle( rec );
    MsiViewClose( view );
    MsiCloseHandle( view );

    /* insert a file into the _Streams table */
    create_file( "test.txt" );

    rec = MsiCreateRecord( 2 );
    MsiRecordSetStringA( rec, 1, "data" );

    r = MsiRecordSetStreamA( rec, 2, "test.txt" );
    ok( r == ERROR_SUCCESS, "Failed to add stream data to the record: %d\n", r);

    DeleteFileA("test.txt");

    r = MsiDatabaseOpenViewA( hdb,
            "INSERT INTO `_Streams` ( `Name`, `Data` ) VALUES ( ?, ? )", &view );
    ok( r == ERROR_SUCCESS, "Failed to open database view: %d\n", r);

    r = MsiViewExecute( view, rec );
    ok( r == ERROR_SUCCESS, "Failed to execute view: %d\n", r);

    MsiCloseHandle( rec );
    MsiViewClose( view );
    MsiCloseHandle( view );

    /* insert another one */
    create_file( "test1.txt" );

    rec = MsiCreateRecord( 2 );
    MsiRecordSetStringA( rec, 1, "data1" );

    r = MsiRecordSetStreamA( rec, 2, "test1.txt" );
    ok( r == ERROR_SUCCESS, "Failed to add stream data to the record: %d\n", r);

    DeleteFileA("test1.txt");

    r = MsiDatabaseOpenViewA( hdb,
            "INSERT INTO `_Streams` ( `Name`, `Data` ) VALUES ( ?, ? )", &view );
    ok( r == ERROR_SUCCESS, "Failed to open database view: %d\n", r);

    r = MsiViewExecute( view, rec );
    ok( r == ERROR_SUCCESS, "Failed to execute view: %d\n", r);

    MsiCloseHandle( rec );
    MsiViewClose( view );
    MsiCloseHandle( view );

    /* try again */
    create_file( "test1.txt" );

    rec = MsiCreateRecord( 2 );
    MsiRecordSetStringA( rec, 1, "data1" );

    r = MsiRecordSetStreamA( rec, 2, "test1.txt" );
    ok( r == ERROR_SUCCESS, "Failed to add stream data to the record: %d\n", r );

    DeleteFileA( "test1.txt" );

    r = MsiDatabaseOpenViewA( hdb,
            "INSERT INTO `_Streams` ( `Name`, `Data` ) VALUES ( ?, ? )", &view );
    ok( r == ERROR_SUCCESS, "Failed to open database view: %d\n", r );

    r = MsiViewExecute( view, rec );
    ok( r == ERROR_FUNCTION_FAILED, "got %u\n", r );

    MsiCloseHandle( rec );
    MsiViewClose( view );
    MsiCloseHandle( view );

    r = MsiDatabaseOpenViewA( hdb,
            "SELECT `Name`, `Data` FROM `_Streams` WHERE `Name` = 'data'", &view );
    ok( r == ERROR_SUCCESS, "Failed to open database view: %d\n", r);

    r = MsiViewExecute( view, 0 );
    ok( r == ERROR_SUCCESS, "Failed to execute view: %d\n", r);

    r = MsiViewFetch( view, &rec );
    ok( r == ERROR_SUCCESS, "Failed to fetch record: %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA( rec, 1, file, &size );
    ok( r == ERROR_SUCCESS, "Failed to get string: %d\n", r);
    ok( !lstrcmpA(file, "data"), "Expected 'data', got %s\n", file);

    size = MAX_PATH;
    memset(buf, 0, MAX_PATH);
    r = MsiRecordReadStream( rec, 2, buf, &size );
    ok( r == ERROR_SUCCESS, "Failed to get stream: %d\n", r);
    ok( !lstrcmpA(buf, "test.txt\n"), "Expected 'test.txt\\n', got %s\n", buf);

    MsiCloseHandle( rec );
    MsiViewClose( view );
    MsiCloseHandle( view );

    r = MsiDatabaseOpenViewA( hdb,
            "SELECT `Name`, `Data` FROM `_Streams` WHERE `Name` = 'data1'", &view );
    ok( r == ERROR_SUCCESS, "Failed to open database view: %d\n", r);

    r = MsiViewExecute( view, 0 );
    ok( r == ERROR_SUCCESS, "Failed to execute view: %d\n", r);

    r = MsiViewFetch( view, &rec );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA( rec, 1, file, &size );
    ok( r == ERROR_SUCCESS, "Failed to get string: %d\n", r);
    ok( !lstrcmpA(file, "data1"), "Expected 'data1', got %s\n", file);

    size = MAX_PATH;
    memset(buf, 0, MAX_PATH);
    r = MsiRecordReadStream( rec, 2, buf, &size );
    ok( r == ERROR_SUCCESS, "Failed to get stream: %d\n", r);
    ok( !lstrcmpA(buf, "test1.txt\n"), "Expected 'test1.txt\\n', got %s\n", buf);

    MsiCloseHandle( rec );
    MsiViewClose( view );
    MsiCloseHandle( view );

    /* perform an update */
    create_file( "test2.txt" );
    rec = MsiCreateRecord( 1 );

    r = MsiRecordSetStreamA( rec, 1, "test2.txt" );
    ok( r == ERROR_SUCCESS, "Failed to add stream data to the record: %d\n", r);

    DeleteFileA("test2.txt");

    r = MsiDatabaseOpenViewA( hdb,
            "UPDATE `_Streams` SET `Data` = ? WHERE `Name` = 'data1'", &view );
    ok( r == ERROR_SUCCESS, "Failed to open database view: %d\n", r);

    r = MsiViewExecute( view, rec );
    ok( r == ERROR_SUCCESS, "Failed to execute view: %d\n", r);

    MsiCloseHandle( rec );
    MsiViewClose( view );
    MsiCloseHandle( view );

    r = MsiDatabaseOpenViewA( hdb,
            "SELECT `Name`, `Data` FROM `_Streams` WHERE `Name` = 'data1'", &view );
    ok( r == ERROR_SUCCESS, "Failed to open database view: %d\n", r);

    r = MsiViewExecute( view, 0 );
    ok( r == ERROR_SUCCESS, "Failed to execute view: %d\n", r);

    r = MsiViewFetch( view, &rec );
    ok( r == ERROR_SUCCESS, "Failed to fetch record: %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA( rec, 1, file, &size );
    ok( r == ERROR_SUCCESS, "Failed to get string: %d\n", r);
    ok( !lstrcmpA(file, "data1"), "Expected 'data1', got %s\n", file);

    size = MAX_PATH;
    memset(buf, 0, MAX_PATH);
    r = MsiRecordReadStream( rec, 2, buf, &size );
    ok( r == ERROR_SUCCESS, "Failed to get stream: %d\n", r);
    ok( !lstrcmpA(buf, "test2.txt\n"), "Expected 'test2.txt\\n', got %s\n", buf);

    MsiCloseHandle( rec );
    MsiViewClose( view );
    MsiCloseHandle( view );
    MsiCloseHandle( hdb );
    DeleteFileA(msifile);

    /* insert a file into the _Streams table */
    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATEDIRECT, &hdb);
    ok(r == ERROR_SUCCESS, "Failed to create database\n");
    ok( hdb, "failed to create db\n");
    create_file( "test.txt" );
    rec = MsiCreateRecord( 2 );
    MsiRecordSetStringA( rec, 1, "data" );
    r = MsiRecordSetStreamA( rec, 2, "test.txt" );
    ok( r == ERROR_SUCCESS, "Failed to add stream data to the record: %d\n", r);
    DeleteFileA("test.txt");
    r = MsiDatabaseOpenViewA( hdb,
            "INSERT INTO `_Streams` ( `Name`, `Data` ) VALUES ( ?, ? )", &view );
    ok( r == ERROR_SUCCESS, "Failed to open database view: %d\n", r);
    r = MsiViewExecute( view, rec );
    ok( r == ERROR_SUCCESS, "Failed to execute view: %d\n", r);
    MsiCloseHandle( rec );
    MsiViewClose( view );
    MsiCloseHandle( view );
    r = MsiDatabaseCommit( hdb );
    ok( r == ERROR_SUCCESS , "Failed to commit database\n" );

    /* open a handle to the "data" stream */
    r = MsiDatabaseOpenViewA( hdb,
            "SELECT `Name`, `Data` FROM `_Streams` WHERE `Name` = 'data'", &view );
    ok( r == ERROR_SUCCESS, "Failed to open database view: %d\n", r);
    r = MsiViewExecute( view, 0 );
    ok( r == ERROR_SUCCESS, "Failed to execute view: %d\n", r);
    r = MsiViewFetch( view, &rec );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    MsiViewClose( view );
    MsiCloseHandle( view );
    /* read the stream while it still exists (normal case) */
    size = MAX_PATH;
    r = MsiRecordGetStringA( rec, 1, file, &size );
    ok( r == ERROR_SUCCESS, "Failed to get string: %d\n", r);
    ok( !lstrcmpA(file, "data"), "Expected 'data', got %s\n", file);
    size = MAX_PATH;
    memset(buf, 0, MAX_PATH);
    r = MsiRecordReadStream( rec, 2, buf, &size );
    ok( r == ERROR_SUCCESS, "Failed to get stream: %d\n", r);
    ok( !lstrcmpA(buf, "test.txt\n"), "Expected 'test.txt\\n', got '%s' (%lu)\n", buf, size);
    MsiCloseHandle( rec );

    /* open a handle to the "data" stream (and keep it open during removal) */
    r = MsiDatabaseOpenViewA( hdb,
            "SELECT `Name`, `Data` FROM `_Streams` WHERE `Name` = 'data'", &view );
    ok( r == ERROR_SUCCESS, "Failed to open database view: %d\n", r);
    r = MsiViewExecute( view, 0 );
    ok( r == ERROR_SUCCESS, "Failed to execute view: %d\n", r);
    r = MsiViewFetch( view, &rec );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    MsiViewClose( view );
    MsiCloseHandle( view );

    /* remove the stream */
    r = MsiDatabaseOpenViewA( hdb,
            "DELETE FROM `_Streams` WHERE `Name` = 'data'", &view );
    ok( r == ERROR_SUCCESS, "Failed to open database view: %d\n", r);
    r = MsiViewExecute( view, 0 );
    ok( r == ERROR_SUCCESS, "Failed to execute view: %d\n", r);
    MsiViewClose( view );
    MsiCloseHandle( view );

    /* attempt to read the stream that no longer exists (abnormal case) */
    size = MAX_PATH;
    r = MsiRecordGetStringA( rec, 1, file, &size );
    ok( r == ERROR_SUCCESS, "Failed to get string: %d\n", r);
    ok( !lstrcmpA(file, "data"), "Expected 'data', got %s\n", file);
    size = MAX_PATH;
    memset(buf, 0, MAX_PATH);
    r = MsiRecordReadStream( rec, 2, buf, &size );
    ok( r == ERROR_SUCCESS, "Failed to get stream: %d\n", r);
    todo_wine ok( size == 0, "Expected empty buffer, got %lu bytes\n", size);
    MsiCloseHandle( rec );

    MsiCloseHandle( hdb );
    DeleteFileA(msifile);
}

static void test_binary(void)
{
    MSIHANDLE hdb = 0, rec;
    char file[MAX_PATH];
    char buf[MAX_PATH];
    DWORD size;
    LPCSTR query;
    UINT r;

    /* insert a file into the Binary table */
    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb );
    ok( r == ERROR_SUCCESS , "Failed to open database\n" );

    query = "CREATE TABLE `Binary` ( `Name` CHAR(72) NOT NULL, `ID` INT NOT NULL, `Data` OBJECT  PRIMARY KEY `Name`, `ID`)";
    r = run_query( hdb, 0, query );
    ok( r == ERROR_SUCCESS, "Cannot create Binary table: %d\n", r );

    create_file( "test.txt" );
    rec = MsiCreateRecord( 1 );
    r = MsiRecordSetStreamA( rec, 1, "test.txt" );
    ok( r == ERROR_SUCCESS, "Failed to add stream data to the record: %d\n", r);
    DeleteFileA( "test.txt" );

    /* try a name that exceeds maximum OLE stream name length */
    query = "INSERT INTO `Binary` ( `Name`, `ID`, `Data` ) VALUES ( 'encryption.dll.CB4E6205_F99A_4C51_ADD4_184506EFAB87', 10000, ? )";
    r = run_query( hdb, rec, query );
    ok( r == ERROR_SUCCESS, "Insert into Binary table failed: %d\n", r );

    r = MsiCloseHandle( rec );
    ok( r == ERROR_SUCCESS , "Failed to close record handle\n" );

    r = MsiDatabaseCommit( hdb );
    ok( r == ERROR_FUNCTION_FAILED , "got %u\n", r );

    r = MsiCloseHandle( hdb );
    ok( r == ERROR_SUCCESS , "Failed to close database\n" );

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb );
    ok( r == ERROR_SUCCESS , "Failed to open database\n" );

    query = "CREATE TABLE `Binary` ( `Name` CHAR(72) NOT NULL, `ID` INT NOT NULL, `Data` OBJECT  PRIMARY KEY `Name`, `ID`)";
    r = run_query( hdb, 0, query );
    ok( r == ERROR_SUCCESS, "Cannot create Binary table: %d\n", r );

    create_file( "test.txt" );
    rec = MsiCreateRecord( 1 );
    r = MsiRecordSetStreamA( rec, 1, "test.txt" );
    ok( r == ERROR_SUCCESS, "Failed to add stream data to the record: %d\n", r );
    DeleteFileA( "test.txt" );

    query = "INSERT INTO `Binary` ( `Name`, `ID`, `Data` ) VALUES ( 'filename1', 1, ? )";
    r = run_query( hdb, rec, query );
    ok( r == ERROR_SUCCESS, "Insert into Binary table failed: %d\n", r );

    query = "INSERT INTO `Binary` ( `Name`, `ID`, `Data` ) VALUES ( 'filename1', 1, ? )";
    r = run_query( hdb, rec, query );
    ok( r == ERROR_FUNCTION_FAILED, "got %u\n", r );

    r = MsiCloseHandle( rec );
    ok( r == ERROR_SUCCESS , "Failed to close record handle\n" );

    r = MsiDatabaseCommit( hdb );
    ok( r == ERROR_SUCCESS , "Failed to commit database\n" );

    r = MsiCloseHandle( hdb );
    ok( r == ERROR_SUCCESS , "Failed to close database\n" );

    /* read file from the Stream table */
    r = MsiOpenDatabaseW( msifileW, MSIDBOPEN_READONLY, &hdb );
    ok( r == ERROR_SUCCESS , "Failed to open database\n" );

    query = "SELECT * FROM `_Streams`";
    r = do_query( hdb, query, &rec );
    ok( r == ERROR_SUCCESS, "SELECT query failed: %d\n", r );

    size = MAX_PATH;
    r = MsiRecordGetStringA( rec, 1, file, &size );
    ok( r == ERROR_SUCCESS, "Failed to get string: %d\n", r );
    ok( !lstrcmpA(file, "Binary.filename1.1"), "Expected 'Binary.filename1.1', got %s\n", file );

    size = MAX_PATH;
    memset( buf, 0, MAX_PATH );
    r = MsiRecordReadStream( rec, 2, buf, &size );
    ok( r == ERROR_SUCCESS, "Failed to get stream: %d\n", r );
    ok( !lstrcmpA(buf, "test.txt\n"), "Expected 'test.txt\\n', got %s\n", buf );

    r = MsiCloseHandle( rec );
    ok( r == ERROR_SUCCESS , "Failed to close record handle\n" );

    /* read file from the Binary table */
    query = "SELECT * FROM `Binary`";
    r = do_query( hdb, query, &rec );
    ok( r == ERROR_SUCCESS, "SELECT query failed: %d\n", r );

    size = MAX_PATH;
    r = MsiRecordGetStringA( rec, 1, file, &size );
    ok( r == ERROR_SUCCESS, "Failed to get string: %d\n", r );
    ok( !lstrcmpA(file, "filename1"), "Expected 'filename1', got %s\n", file );

    size = MAX_PATH;
    memset( buf, 0, MAX_PATH );
    r = MsiRecordReadStream( rec, 3, buf, &size );
    ok( r == ERROR_SUCCESS, "Failed to get stream: %d\n", r );
    ok( !lstrcmpA(buf, "test.txt\n"), "Expected 'test.txt\\n', got %s\n", buf );

    r = MsiCloseHandle( rec );
    ok( r == ERROR_SUCCESS , "Failed to close record handle\n" );

    r = MsiCloseHandle( hdb );
    ok( r == ERROR_SUCCESS , "Failed to close database\n" );

    DeleteFileA( msifile );
}

static void test_where_not_in_selected(void)
{
    MSIHANDLE hdb = 0, rec, view;
    LPCSTR query;
    UINT r;

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    r = run_query(hdb, 0,
            "CREATE TABLE `IESTable` ("
            "`Action` CHAR(64), "
            "`Condition` CHAR(64), "
            "`Sequence` LONG PRIMARY KEY `Sequence`)");
    ok( r == S_OK, "Cannot create IESTable table: %d\n", r);

    r = run_query(hdb, 0,
            "CREATE TABLE `CATable` ("
            "`Action` CHAR(64), "
            "`Type` LONG PRIMARY KEY `Type`)");
    ok( r == S_OK, "Cannot create CATable table: %d\n", r);

    r = run_query(hdb, 0, "INSERT INTO `IESTable` "
            "( `Action`, `Condition`, `Sequence`) "
            "VALUES ( 'clean', 'cond4', 4)");
    ok( r == S_OK, "cannot add entry to IESTable table:%d\n", r );

    r = run_query(hdb, 0, "INSERT INTO `IESTable` "
            "( `Action`, `Condition`, `Sequence`) "
            "VALUES ( 'depends', 'cond1', 1)");
    ok( r == S_OK, "cannot add entry to IESTable table:%d\n", r );

    r = run_query(hdb, 0, "INSERT INTO `IESTable` "
            "( `Action`, `Condition`, `Sequence`) "
            "VALUES ( 'build', 'cond2', 2)");
    ok( r == S_OK, "cannot add entry to IESTable table:%d\n", r );

    r = run_query(hdb, 0, "INSERT INTO `IESTable` "
            "( `Action`, `Condition`, `Sequence`) "
            "VALUES ( 'build2', 'cond6', 6)");
    ok( r == S_OK, "cannot add entry to IESTable table:%d\n", r );

    r = run_query(hdb, 0, "INSERT INTO `IESTable` "
            "( `Action`, `Condition`, `Sequence`) "
            "VALUES ( 'build', 'cond3', 3)");
    ok(r == S_OK, "cannot add entry to IESTable table:%d\n", r );

    r = run_query(hdb, 0, "INSERT INTO `CATable` "
            "( `Action`, `Type` ) "
            "VALUES ( 'build', 32)");
    ok(r == S_OK, "cannot add entry to CATable table:%d\n", r );

    r = run_query(hdb, 0, "INSERT INTO `CATable` "
            "( `Action`, `Type` ) "
            "VALUES ( 'depends', 64)");
    ok(r == S_OK, "cannot add entry to CATable table:%d\n", r );

    r = run_query(hdb, 0, "INSERT INTO `CATable` "
            "( `Action`, `Type` ) "
            "VALUES ( 'clean', 63)");
    ok(r == S_OK, "cannot add entry to CATable table:%d\n", r );

    r = run_query(hdb, 0, "INSERT INTO `CATable` "
            "( `Action`, `Type` ) "
            "VALUES ( 'build2', 34)");
    ok(r == S_OK, "cannot add entry to CATable table:%d\n", r );
    query = "Select IESTable.Condition from CATable, IESTable where "
            "CATable.Action = IESTable.Action and CATable.Type = 32";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(view, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    r = MsiViewFetch(view, &rec);
    ok( r == ERROR_SUCCESS, "failed to fetch view: %d\n", r );
    check_record(rec, 1, "cond2");
    MsiCloseHandle( rec );

    r = MsiViewFetch(view, &rec);
    ok( r == ERROR_SUCCESS, "failed to fetch view: %d\n", r );
    check_record(rec, 1, "cond3");
    MsiCloseHandle( rec );

    MsiViewClose(view);
    MsiCloseHandle(view);

    MsiCloseHandle( hdb );
    DeleteFileA(msifile);
}


static void test_where(void)
{
    MSIHANDLE hdb = 0, rec, view;
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
    check_record(rec, 6, "1", "0", "", "zero.cab", "", "");
    MsiCloseHandle( rec );

    query = "SELECT * FROM `Media` WHERE `LastSequence` >= 1";
    r = do_query(hdb, query, &rec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed: %d\n", r);
    check_record(rec, 6, "2", "1", "", "one.cab", "", "");
    MsiCloseHandle( rec );

    query = "SELECT `DiskId` FROM `Media` WHERE `LastSequence` >= 1 AND DiskId >= 0";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(view, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    r = MsiViewFetch(view, &rec);
    ok( r == ERROR_SUCCESS, "failed to fetch view: %d\n", r );
    check_record(rec, 1, "2");
    MsiCloseHandle( rec );

    r = MsiViewFetch(view, &rec);
    ok( r == ERROR_SUCCESS, "failed to fetch view: %d\n", r );
    check_record(rec, 1, "3");
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

    rec = 0;
    query = "SELECT * FROM `Media` WHERE `DiskPrompt` < 'Cabinet'";
    r = do_query(hdb, query, &rec);
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %d\n", r );
    MsiCloseHandle( rec );

    rec = 0;
    query = "SELECT * FROM `Media` WHERE `DiskPrompt` > 'Cabinet'";
    r = do_query(hdb, query, &rec);
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %d\n", r );
    MsiCloseHandle( rec );

    rec = 0;
    query = "SELECT * FROM `Media` WHERE `DiskPrompt` <> 'Cabinet'";
    r = do_query(hdb, query, &rec);
    ok( r == ERROR_SUCCESS, "query failed: %d\n", r );
    MsiCloseHandle( rec );

    rec = 0;
    query = "SELECT * FROM `Media` WHERE `DiskPrompt` = 'Cabinet'";
    r = do_query(hdb, query, &rec);
    ok( r == ERROR_NO_MORE_ITEMS, "query failed: %d\n", r );
    MsiCloseHandle( rec );

    rec = MsiCreateRecord(1);
    MsiRecordSetStringA(rec, 1, "");

    query = "SELECT * FROM `Media` WHERE `DiskPrompt` = ?";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
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
    DeleteFileA(msifile);
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

static const CHAR suminfo[] = "PropertyId\tValue\n"
                              "i2\tl255\n"
                              "_SummaryInformation\tPropertyId\n"
                              "1\t1252\n"
                              "2\tInstaller Database\n"
                              "3\tInstaller description\n"
                              "4\tWineHQ\n"
                              "5\tInstaller\n"
                              "6\tInstaller comments\n"
                              "7\tIntel;1033,2057\n"
                              "9\t{12345678-1234-1234-1234-123456789012}\n"
                              "12\t2009/04/12 15:46:11\n"
                              "13\t2009/04/12 15:46:11\n"
                              "14\t200\n"
                              "15\t2\n"
                              "18\tVim\n"
                              "19\t2\n";

static void write_file(const CHAR *filename, const char *data, int data_size)
{
    DWORD size;

    HANDLE hf = CreateFileA(filename, GENERIC_WRITE, 0, NULL,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    WriteFile(hf, data, data_size, &size, NULL);
    CloseHandle(hf);
}

static UINT add_table_to_db(MSIHANDLE hdb, LPCSTR table_data)
{
    UINT r;

    write_file("temp_file", table_data, (lstrlenA(table_data) - 1) * sizeof(char));
    r = MsiDatabaseImportA(hdb, CURR_DIR, "temp_file");
    DeleteFileA("temp_file");

    return r;
}

static void test_suminfo_import(void)
{
    MSIHANDLE hdb, hsi, view = 0;
    LPCSTR query;
    UINT r, count, type;
    DWORD size;
    char str_value[50];
    INT int_value;
    FILETIME ft_value;

    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = add_table_to_db(hdb, suminfo);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    /* _SummaryInformation is not imported as a regular table... */

    query = "SELECT * FROM `_SummaryInformation`";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %u\n", r);
    MsiCloseHandle(view);

    /* ...its data is added to the special summary information stream */

    r = MsiGetSummaryInformationA(hdb, NULL, 0, &hsi);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSummaryInfoGetPropertyCount(hsi, &count);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(count == 14, "Expected 14, got %u\n", count);

    r = MsiSummaryInfoGetPropertyA(hsi, PID_CODEPAGE, &type, &int_value, NULL, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type ==  VT_I2, "Expected VT_I2, got %u\n", type);
    ok(int_value == 1252, "Expected 1252, got %d\n", int_value);

    size = sizeof(str_value);
    r = MsiSummaryInfoGetPropertyA(hsi, PID_TITLE, &type, NULL, NULL, str_value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type == VT_LPSTR, "Expected VT_LPSTR, got %u\n", type);
    ok(size == 18, "Expected 18, got %lu\n", size);
    ok(!strcmp(str_value, "Installer Database"),
       "Expected \"Installer Database\", got %s\n", str_value);

    size = sizeof(str_value);
    r = MsiSummaryInfoGetPropertyA(hsi, PID_SUBJECT, &type, NULL, NULL, str_value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type == VT_LPSTR, "Expected VT_LPSTR, got %u\n", type);
    ok(!strcmp(str_value, "Installer description"),
       "Expected \"Installer description\", got %s\n", str_value);

    size = sizeof(str_value);
    r = MsiSummaryInfoGetPropertyA(hsi, PID_AUTHOR, &type, NULL, NULL, str_value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type == VT_LPSTR, "Expected VT_LPSTR, got %u\n", type);
    ok(!strcmp(str_value, "WineHQ"),
       "Expected \"WineHQ\", got %s\n", str_value);

    size = sizeof(str_value);
    r = MsiSummaryInfoGetPropertyA(hsi, PID_KEYWORDS, &type, NULL, NULL, str_value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type == VT_LPSTR, "Expected VT_LPSTR, got %u\n", type);
    ok(!strcmp(str_value, "Installer"),
       "Expected \"Installer\", got %s\n", str_value);

    size = sizeof(str_value);
    r = MsiSummaryInfoGetPropertyA(hsi, PID_COMMENTS, &type, NULL, NULL, str_value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type == VT_LPSTR, "Expected VT_LPSTR, got %u\n", type);
    ok(!strcmp(str_value, "Installer comments"),
       "Expected \"Installer comments\", got %s\n", str_value);

    size = sizeof(str_value);
    r = MsiSummaryInfoGetPropertyA(hsi, PID_TEMPLATE, &type, NULL, NULL, str_value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type == VT_LPSTR, "Expected VT_LPSTR, got %u\n", type);
    ok(!strcmp(str_value, "Intel;1033,2057"),
       "Expected \"Intel;1033,2057\", got %s\n", str_value);

    size = sizeof(str_value);
    r = MsiSummaryInfoGetPropertyA(hsi, PID_REVNUMBER, &type, NULL, NULL, str_value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type == VT_LPSTR, "Expected VT_LPSTR, got %u\n", type);
    ok(!strcmp(str_value, "{12345678-1234-1234-1234-123456789012}"),
       "Expected \"{12345678-1234-1234-1234-123456789012}\", got %s\n", str_value);

    r = MsiSummaryInfoGetPropertyA(hsi, PID_CREATE_DTM, &type, NULL, &ft_value, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type == VT_FILETIME, "Expected VT_FILETIME, got %u\n", type);

    r = MsiSummaryInfoGetPropertyA(hsi, PID_LASTSAVE_DTM, &type, NULL, &ft_value, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type == VT_FILETIME, "Expected VT_FILETIME, got %u\n", type);

    r = MsiSummaryInfoGetPropertyA(hsi, PID_PAGECOUNT, &type, &int_value, NULL, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type ==  VT_I4, "Expected VT_I4, got %u\n", type);
    ok(int_value == 200, "Expected 200, got %d\n", int_value);

    r = MsiSummaryInfoGetPropertyA(hsi, PID_WORDCOUNT, &type, &int_value, NULL, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type ==  VT_I4, "Expected VT_I4, got %u\n", type);
    ok(int_value == 2, "Expected 2, got %d\n", int_value);

    r = MsiSummaryInfoGetPropertyA(hsi, PID_SECURITY, &type, &int_value, NULL, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type ==  VT_I4, "Expected VT_I4, got %u\n", type);
    ok(int_value == 2, "Expected 2, got %d\n", int_value);

    size = sizeof(str_value);
    r = MsiSummaryInfoGetPropertyA(hsi, PID_APPNAME, &type, NULL, NULL, str_value, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);
    ok(type == VT_LPSTR, "Expected VT_LPSTR, got %u\n", type);
    ok(!strcmp(str_value, "Vim"), "Expected \"Vim\", got %s\n", str_value);

    MsiCloseHandle(hsi);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_msiimport(void)
{
    MSIHANDLE hdb, view, rec;
    LPCSTR query;
    UINT r;

    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiDatabaseImportA(hdb, CURR_DIR, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    r = MsiDatabaseImportA(hdb, CURR_DIR, "nonexistent");
    ok(r == ERROR_FUNCTION_FAILED, "Expected ERROR_FUNCTION_FAILED, got %d\n", r);

    r = add_table_to_db(hdb, test_data);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_table_to_db(hdb, two_primary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_table_to_db(hdb, endlines1);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_table_to_db(hdb, endlines2);
    ok(r == ERROR_FUNCTION_FAILED, "Expected ERROR_FUNCTION_FAILED, got %d\n", r);

    query = "SELECT * FROM `TestTable`";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_NAMES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 9, "FirstPrimaryColumn", "SecondPrimaryColumn", "ShortInt",
            "ShortIntNullable", "LongInt", "LongIntNullable", "String",
            "LocalizableString", "LocalizableStringNullable");
    MsiCloseHandle(rec);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_TYPES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 9, "s255", "i2", "i2", "I2", "i4", "I4", "S255", "S0", "s0");
    MsiCloseHandle(rec);

    query = "SELECT * FROM `TestTable`";
    r = do_query(hdb, query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 9, "stringage", "5", "2", "", "2147483640", "-2147483640",
            "another string", "localizable", "duh");
    MsiCloseHandle(rec);

    MsiViewClose(view);
    MsiCloseHandle(view);

    query = "SELECT * FROM `TwoPrimary`";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_NAMES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 2, "PrimaryOne", "PrimaryTwo");
    MsiCloseHandle(rec);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_TYPES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 2, "s255", "s255");
    MsiCloseHandle(rec);

    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 2, "papaya", "leaf");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 2, "papaya", "flower");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS,
       "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiViewClose(view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(view);

    query = "SELECT * FROM `Table`";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_NAMES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 6, "A", "B", "C", "D", "E", "F");
    MsiCloseHandle(rec);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_TYPES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 6, "s72", "s72", "s72", "s72", "s72", "s72");
    MsiCloseHandle(rec);

    MsiViewClose(view);
    MsiCloseHandle(view);

    query = "SELECT * FROM `Table`";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 6, "a", "b", "c", "d", "e", "f");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 6, "g", "h", "i", "j", "k", "l");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS,
       "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(view);
    MsiCloseHandle(view);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static const CHAR bin_import_dat[] = "Name\tData\r\n"
                                     "s72\tV0\r\n"
                                     "Binary\tName\r\n"
                                     "filename1\tfilename1.ibd\r\n";

static void test_binary_import(void)
{
    MSIHANDLE hdb = 0, rec;
    char file[MAX_PATH];
    char buf[MAX_PATH];
    char path[MAX_PATH];
    DWORD size;
    LPCSTR query;
    UINT r;

    /* create files to import */
    write_file("bin_import.idt", bin_import_dat,
          (sizeof(bin_import_dat) - 1) * sizeof(char));
    CreateDirectoryA("bin_import", NULL);
    create_file_data("bin_import/filename1.ibd", "just some words", 15);

    /* import files into database */
    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok( r == ERROR_SUCCESS , "Failed to open database\n");

    GetCurrentDirectoryA(MAX_PATH, path);
    r = MsiDatabaseImportA(hdb, path, "bin_import.idt");
    ok(r == ERROR_SUCCESS , "Failed to import Binary table\n");

    /* read file from the Binary table */
    query = "SELECT * FROM `Binary`";
    r = do_query(hdb, query, &rec);
    ok(r == ERROR_SUCCESS, "SELECT query failed: %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(rec, 1, file, &size);
    ok(r == ERROR_SUCCESS, "Failed to get string: %d\n", r);
    ok(!lstrcmpA(file, "filename1"), "Expected 'filename1', got %s\n", file);

    size = MAX_PATH;
    memset(buf, 0, MAX_PATH);
    r = MsiRecordReadStream(rec, 2, buf, &size);
    ok(r == ERROR_SUCCESS, "Failed to get stream: %d\n", r);
    ok(!lstrcmpA(buf, "just some words"), "Expected 'just some words', got %s\n", buf);

    r = MsiCloseHandle(rec);
    ok(r == ERROR_SUCCESS , "Failed to close record handle\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS , "Failed to close database\n");

    DeleteFileA("bin_import/filename1.ibd");
    RemoveDirectoryA("bin_import");
    DeleteFileA("bin_import.idt");
}

static void test_markers(void)
{
    MSIHANDLE hdb, rec;
    LPCSTR query;
    UINT r;

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    rec = MsiCreateRecord(3);
    MsiRecordSetStringA(rec, 1, "Table");
    MsiRecordSetStringA(rec, 2, "Apples");
    MsiRecordSetStringA(rec, 3, "Oranges");

    /* try a legit create */
    query = "CREATE TABLE `Table` ( `One` SHORT NOT NULL, `Two` CHAR(255) PRIMARY KEY `One`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    MsiCloseHandle(rec);

    /* try table name as marker */
    rec = MsiCreateRecord(1);
    MsiRecordSetStringA(rec, 1, "Fable");
    query = "CREATE TABLE `?` ( `One` SHORT NOT NULL, `Two` CHAR(255) PRIMARY KEY `One`)";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* verify that we just created a table called '?', not 'Fable' */
    r = try_query(hdb, "SELECT * from `Fable`");
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    r = try_query(hdb, "SELECT * from `?`");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* try table name as marker without backticks */
    MsiRecordSetStringA(rec, 1, "Mable");
    query = "CREATE TABLE ? ( `One` SHORT NOT NULL, `Two` CHAR(255) PRIMARY KEY `One`)";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* try one column name as marker */
    MsiRecordSetStringA(rec, 1, "One");
    query = "CREATE TABLE `Mable` ( `?` SHORT NOT NULL, `Two` CHAR(255) PRIMARY KEY `One`)";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    MsiCloseHandle(rec);

    /* try column names as markers */
    rec = MsiCreateRecord(2);
    MsiRecordSetStringA(rec, 1, "One");
    MsiRecordSetStringA(rec, 2, "Two");
    query = "CREATE TABLE `Mable` ( `?` SHORT NOT NULL, `?` CHAR(255) PRIMARY KEY `One`)";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    MsiCloseHandle(rec);

    /* try names with backticks */
    rec = MsiCreateRecord(3);
    MsiRecordSetStringA(rec, 1, "One");
    MsiRecordSetStringA(rec, 2, "Two");
    MsiRecordSetStringA(rec, 3, "One");
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
    MsiRecordSetStringA(rec, 1, "`One` SHORT NOT NULL, `Two` CHAR(255) PRIMARY KEY `One`");
    query = "CREATE TABLE `Mable` ( ? )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    MsiCloseHandle(rec);

    /* try all names as markers */
    rec = MsiCreateRecord(4);
    MsiRecordSetStringA(rec, 1, "Mable");
    MsiRecordSetStringA(rec, 2, "One");
    MsiRecordSetStringA(rec, 3, "Two");
    MsiRecordSetStringA(rec, 4, "One");
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
    MsiRecordSetStringA(rec, 2, "hi");
    query = "INSERT INTO `Table` ( `One`, `Two` ) VALUES ( ?, '?' )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    MsiCloseHandle(rec);

    /* try column names and values as markers */
    rec = MsiCreateRecord(4);
    MsiRecordSetStringA(rec, 1, "One");
    MsiRecordSetStringA(rec, 2, "Two");
    MsiRecordSetInteger(rec, 3, 5);
    MsiRecordSetStringA(rec, 4, "hi");
    query = "INSERT INTO `Table` ( `?`, `?` ) VALUES ( ?, '?' )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    MsiCloseHandle(rec);

    /* try column names as markers */
    rec = MsiCreateRecord(2);
    MsiRecordSetStringA(rec, 1, "One");
    MsiRecordSetStringA(rec, 2, "Two");
    query = "INSERT INTO `Table` ( `?`, `?` ) VALUES ( 3, 'yellow' )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    MsiCloseHandle(rec);

    /* try table name as a marker */
    rec = MsiCreateRecord(1);
    MsiRecordSetStringA(rec, 1, "Table");
    query = "INSERT INTO `?` ( `One`, `Two` ) VALUES ( 2, 'green' )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    MsiCloseHandle(rec);

    /* try table name and values as markers */
    rec = MsiCreateRecord(3);
    MsiRecordSetStringA(rec, 1, "Table");
    MsiRecordSetInteger(rec, 2, 10);
    MsiRecordSetStringA(rec, 3, "haha");
    query = "INSERT INTO `?` ( `One`, `Two` ) VALUES ( ?, '?' )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_FUNCTION_FAILED, "Expected ERROR_FUNCTION_FAILED, got %d\n", r);
    MsiCloseHandle(rec);

    /* try all markers */
    rec = MsiCreateRecord(5);
    MsiRecordSetStringA(rec, 1, "Table");
    MsiRecordSetStringA(rec, 1, "One");
    MsiRecordSetStringA(rec, 1, "Two");
    MsiRecordSetInteger(rec, 2, 10);
    MsiRecordSetStringA(rec, 3, "haha");
    query = "INSERT INTO `?` ( `?`, `?` ) VALUES ( ?, '?' )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);
    MsiCloseHandle(rec);

    /* insert an integer as a string */
    rec = MsiCreateRecord(2);
    MsiRecordSetStringA(rec, 1, "11");
    MsiRecordSetStringA(rec, 2, "hi");
    query = "INSERT INTO `Table` ( `One`, `Two` ) VALUES ( ?, '?' )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    MsiCloseHandle(rec);

    /* leave off the '' for the string */
    rec = MsiCreateRecord(2);
    MsiRecordSetInteger(rec, 1, 12);
    MsiRecordSetStringA(rec, 2, "hi");
    query = "INSERT INTO `Table` ( `One`, `Two` ) VALUES ( ?, ? )";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    MsiCloseHandle(rec);

    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

#define MY_NVIEWS 4000    /* Largest installer I've seen uses < 2000 */
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
        r = MsiDatabaseOpenViewA(hdb, szQueryBuf, &hviews[i]);
        if( r != ERROR_SUCCESS || hviews[i] == 0xdeadbeeb ||
            hviews[i] == 0 || (i && (hviews[i] == hviews[i-1])))
            break;
    }

    ok( i == MY_NVIEWS, "problem opening views\n");

    for (i=0; i<MY_NVIEWS; i++) {
        if (hviews[i] != 0 && hviews[i] != 0xdeadbeeb) {
            MsiViewClose(hviews[i]);
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
    CopyFileA(msifile2, msifile, FALSE);

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_TRANSACT, &hdb1 );
    ok( r == ERROR_SUCCESS , "Failed to create database\n" );

    r = MsiDatabaseCommit( hdb1 );
    ok( r == ERROR_SUCCESS , "Failed to commit database\n" );

    r = MsiOpenDatabaseW(msifile2W, MSIDBOPEN_READONLY, &hdb2 );
    ok( r == ERROR_SUCCESS , "Failed to create database\n" );

    /* the transform between two identical database should be empty */
    r = MsiDatabaseGenerateTransformA(hdb1, hdb2, NULL, 0, 0);
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
    r = MsiRecordSetStreamA(hrec, 2, "testdata.bin");
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

    r = MsiDatabaseGenerateTransformA(hdb1, hdb2, mstfile, 0, 0);
    ok( r == ERROR_SUCCESS, "return code %d, should be ERROR_SUCCESS\n", r );

    MsiCloseHandle( hdb1 );
    MsiCloseHandle( hdb2 );

    DeleteFileA("testdata.bin");
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

    for (i=0; i<ARRAY_SIZE(table_transform_data); i++)
    {
        r = IStorage_CreateStream( stg, table_transform_data[i].name,
                            STGM_WRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stm );
        if (FAILED(r))
        {
            ok(0, "failed to create stream %#lx\n", r);
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
    res = MsiGetSummaryInformationA(hdb, NULL, 7, &suminfo);
    ok( res == ERROR_SUCCESS , "Failed to open summaryinfo\n" );

    res = MsiSummaryInfoSetPropertyA(suminfo,2, VT_LPSTR, 0,NULL,
                        "Installation Database");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetPropertyA(suminfo,3, VT_LPSTR, 0,NULL,
                        "Installation Database");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetPropertyA(suminfo,4, VT_LPSTR, 0,NULL,
                        "Wine Hackers");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetPropertyA(suminfo,7, VT_LPSTR, 0,NULL,
                    ";1033,2057");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetPropertyA(suminfo,9, VT_LPSTR, 0,NULL,
                    "{913B8D18-FBB6-4CAC-A239-C74C11E3FA74}");
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetPropertyA(suminfo, 14, VT_I4, 100, NULL, NULL);
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoSetPropertyA(suminfo, 15, VT_I4, 0, NULL, NULL);
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    res = MsiSummaryInfoPersist(suminfo);
    ok( res == ERROR_SUCCESS , "Failed to make summary info persist\n" );

    res = MsiCloseHandle( suminfo);
    ok( res == ERROR_SUCCESS , "Failed to close suminfo\n" );

    return res;
}

static MSIHANDLE create_package_db(const WCHAR *filename)
{
    MSIHANDLE hdb = 0;
    UINT res;

    DeleteFileW(msifileW);

    /* create an empty database */
    res = MsiOpenDatabaseW(filename, MSIDBOPEN_CREATE, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to create database\n" );
    if( res != ERROR_SUCCESS )
        return hdb;

    res = MsiDatabaseCommit( hdb );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

    res = set_summary_info(hdb);
    ok( res == ERROR_SUCCESS , "Failed to set summary info\n" );

    create_directory_table(hdb);

    return hdb;
}

static UINT package_from_db(MSIHANDLE hdb, MSIHANDLE *handle)
{
    UINT res;
    CHAR szPackage[12];
    MSIHANDLE hPackage;

    sprintf(szPackage, "#%lu", hdb);
    res = MsiOpenPackageA(szPackage, &hPackage);
    if (res != ERROR_SUCCESS)
        return res;

    res = MsiCloseHandle(hdb);
    if (res != ERROR_SUCCESS)
    {
        MsiCloseHandle(hPackage);
        return res;
    }

    *handle = hPackage;
    return ERROR_SUCCESS;
}

static void test_try_transform(void)
{
    static const struct {
        const char *table;
        const char *column;
        const char *row;
        const char *data;
        const char *current;
    } transform_view[] = {
        { "MOO", "OOO", "1", "c", "a" },
        { "MOO", "COW", "", "5378", "3" },
        { "MOO", "PIG", "", "5378", "4" },
        { "MOO", "PIG", "1", "5", "" },
        { "MOO", "DELETE", "3", "", "" },
        { "BINARY", "BLOB", "1", "BINARY.1", "" },
        { "BINARY", "INSERT", "1", "", "" },
        { "AAR", "CREATE", "", "", "" },
        { "AAR", "CAR", "", "15871", "1" },
        { "AAR", "BAR", "", "1282", "2" },
        { "AAR", "BAR", "vw", "1", "" },
        { "AAR", "BAR", "bmw", "2", "" },
        { "AAR", "INSERT", "vw", "", "" },
        { "AAR", "INSERT", "bmw", "", "" },
        { "Property", "CREATE", "", "", "" },
        { "Property", "Property", "", "11592", "1" },
        { "Property", "Value", "", "7424", "2" },
        { "Property", "Value", "prop", "val", "" },
        { "Property", "INSERT", "prop", "", "" }
    };

    MSIHANDLE hdb, hview, hrec, hpkg = 0;
    LPCSTR query;
    UINT r;
    DWORD sz;
    char buffer[MAX_PATH];
    int i, matched;

    DeleteFileA(msifile);
    DeleteFileA(mstfile);

    /* create the database */
    hdb = create_package_db(msifileW);
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
    r = MsiRecordSetStreamA(hrec, 2, "testdata.bin");
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

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_DIRECT, &hdb );
    ok( r == ERROR_SUCCESS , "Failed to create database\n" );

    r = MsiDatabaseApplyTransformA(hdb, mstfile, MSITRANSFORM_ERROR_VIEWTRANSFORM);
    ok(r == ERROR_SUCCESS, "return code %d, should be ERROR_SUCCESS\n", r);

    query = "select * from `_TransformView`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    r = MsiViewGetColumnInfo(hview, MSICOLINFO_NAMES, &hrec);
    ok(r == ERROR_SUCCESS, "error\n");
    check_record(hrec, 5, "Table", "Column", "Row", "Data", "Current");
    MsiCloseHandle(hrec);

    r = MsiViewGetColumnInfo(hview, MSICOLINFO_TYPES, &hrec);
    ok(r == ERROR_SUCCESS, "error\n");
    check_record(hrec, 5, "g0", "g0", "G0", "G0", "G0");
    MsiCloseHandle(hrec);

    matched = 0;
    while (MsiViewFetch(hview, &hrec) == ERROR_SUCCESS)
    {
        char data[5][256];

        for (i = 1; i <= 5; i++) {
            sz = ARRAY_SIZE(data[0]);
            r = MsiRecordGetStringA(hrec, i, data[i-1], &sz);
            ok(r == ERROR_SUCCESS, "%d) MsiRecordGetStringA failed %d\n", i, r);
        }

        for (i = 0; i < ARRAY_SIZE(transform_view); i++)
        {
            if (strcmp(data[0], transform_view[i].table) ||
                    strcmp(data[1], transform_view[i].column) ||
                    strcmp(data[2], transform_view[i].row))
                continue;

            matched++;
            ok(!strcmp(data[3], transform_view[i].data), "%d) data[3] = %s\n", i, data[3]);
            ok(!strcmp(data[4], transform_view[i].current), "%d) data[4] = %s\n", i, data[4]);
            break;
        }
        ok(i != ARRAY_SIZE(transform_view), "invalid row: %s, %s, %s\n",
                wine_dbgstr_a(data[0]), wine_dbgstr_a(data[1]), wine_dbgstr_a(data[2]));
        MsiCloseHandle(hrec);
    }
    ok(matched == ARRAY_SIZE(transform_view), "matched = %d\n", matched);

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    query = "ALTER TABLE `_TransformView` FREE";
    r = run_query( hdb, 0, query );
    ok( r == ERROR_SUCCESS, "cannot free _TransformView table: %d\n", r );
    r = run_query( hdb, 0, query );
    ok( r == ERROR_BAD_QUERY_SYNTAX, "_TransformView table still exist: %d\n", r );

    r = MsiDatabaseApplyTransformA( hdb, mstfile, 0 );
    ok( r == ERROR_SUCCESS, "return code %d, should be ERROR_SUCCESS\n", r );

    r = MsiDatabaseCommit( hdb );
    ok( r == ERROR_SUCCESS , "Failed to commit database\n" );

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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "open view failed\n");

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "view execute failed\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "view fetch failed\n");
    check_record(hrec, 4, "1", "c", "", "5");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "view fetch failed\n");
    check_record(hrec, 4, "2", "b", "", "");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "view fetch succeeded\n");

    MsiCloseHandle(hrec);
    MsiViewClose(hview);
    MsiCloseHandle(hview);

    /* check that the property was added */
    r = package_from_db(hdb, &hpkg);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    sz = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "prop", buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buffer, "val"), "Expected val, got %s\n", buffer);

    MsiCloseHandle(hpkg);

error:
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
    DeleteFileA(mstfile);
}

static const char *join_res_first[][2] =
{
    { "alveolar", "procerus" },
    { "septum", "procerus" },
    { "septum", "nasalis" },
    { "ramus", "nasalis" },
    { "malar", "mentalis" },
};

static const char *join_res_second[][2] =
{
    { "nasal", "septum" },
    { "mandible", "ramus" },
};

static const char *join_res_third[][2] =
{
    { "msvcp.dll", "abcdefgh" },
    { "msvcr.dll", "ijklmnop" },
};

static const char *join_res_fourth[][2] =
{
    { "msvcp.dll.01234", "single.dll.31415" },
};

static const char *join_res_fifth[][2] =
{
    { "malar", "procerus" },
};

static const char *join_res_sixth[][2] =
{
    { "malar", "procerus" },
    { "malar", "procerus" },
    { "malar", "nasalis" },
    { "malar", "nasalis" },
    { "malar", "nasalis" },
    { "malar", "mentalis" },
};

static const char *join_res_seventh[][2] =
{
    { "malar", "nasalis" },
    { "malar", "nasalis" },
    { "malar", "nasalis" },
};

static const char *join_res_eighth[][4] =
{
    { "msvcp.dll", "msvcp.dll.01234", "msvcp.dll.01234", "abcdefgh" },
    { "msvcr.dll", "msvcr.dll.56789", "msvcp.dll.01234", "abcdefgh" },
    { "msvcp.dll", "msvcp.dll.01234", "msvcr.dll.56789", "ijklmnop" },
    { "msvcr.dll", "msvcr.dll.56789", "msvcr.dll.56789", "ijklmnop" },
    { "msvcp.dll", "msvcp.dll.01234", "single.dll.31415", "msvcp.dll" },
    { "msvcr.dll", "msvcr.dll.56789", "single.dll.31415", "msvcp.dll" },
};

static const char *join_res_ninth[][6] =
{
    { "1", "2", "3", "4", "7", "8" },
    { "1", "2", "5", "6", "7", "8" },
    { "1", "2", "3", "4", "9", "10" },
    { "1", "2", "5", "6", "9", "10" },
    { "1", "2", "3", "4", "11", "12" },
    { "1", "2", "5", "6", "11", "12" },
};

static void test_join(void)
{
    MSIHANDLE hdb, hview, hrec;
    LPCSTR query;
    UINT r;
    DWORD i;

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    create_component_table( hdb );
    add_component_entry( hdb, "'zygomatic', 'malar', 'INSTALLDIR', 0, '', ''" );
    add_component_entry( hdb, "'maxilla', 'alveolar', 'INSTALLDIR', 0, '', ''" );
    add_component_entry( hdb, "'nasal', 'septum', 'INSTALLDIR', 0, '', ''" );
    add_component_entry( hdb, "'mandible', 'ramus', 'INSTALLDIR', 0, '', ''" );

    create_feature_components_table( hdb );
    add_feature_components_entry( hdb, "'procerus', 'maxilla'" );
    add_feature_components_entry( hdb, "'procerus', 'nasal'" );
    add_feature_components_entry( hdb, "'nasalis', 'nasal'" );
    add_feature_components_entry( hdb, "'nasalis', 'mandible'" );
    add_feature_components_entry( hdb, "'nasalis', 'notacomponent'" );
    add_feature_components_entry( hdb, "'mentalis', 'zygomatic'" );

    create_std_dlls_table( hdb );
    add_std_dlls_entry( hdb, "'msvcp.dll', 'msvcp.dll.01234'" );
    add_std_dlls_entry( hdb, "'msvcr.dll', 'msvcr.dll.56789'" );

    create_binary_table( hdb );
    add_binary_entry( hdb, "'msvcp.dll.01234', 'abcdefgh'" );
    add_binary_entry( hdb, "'msvcr.dll.56789', 'ijklmnop'" );
    add_binary_entry( hdb, "'single.dll.31415', 'msvcp.dll'" );

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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        check_record(hrec, 2, join_res_first[i][0], join_res_first[i][1]);
        i++;
        MsiCloseHandle(hrec);
    }
    ok( i == 5, "Expected 5 rows, got %lu\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    /* try a join without a WHERE condition */
    query = "SELECT `Component`.`ComponentId`, `FeatureComponents`.`Feature_` "
            "FROM `Component`, `FeatureComponents` ";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        i++;
        MsiCloseHandle(hrec);
    }
    ok( i == 24, "Expected 24 rows, got %lu\n", i );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT DISTINCT Component, ComponentId FROM FeatureComponents, Component "
            "WHERE FeatureComponents.Component_=Component.Component "
            "AND (Feature_='nasalis') ORDER BY Feature_";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        check_record(hrec, 2, join_res_second[i][0], join_res_second[i][1]);
        i++;
        MsiCloseHandle(hrec);
    }

    ok( i == 2, "Expected 2 rows, got %lu\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT `StdDlls`.`File`, `Binary`.`Data` "
            "FROM `StdDlls`, `Binary` "
            "WHERE `StdDlls`.`Binary_` = `Binary`.`Name` "
            "ORDER BY `File`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        check_record(hrec, 2, join_res_third[i][0], join_res_third[i][1]);
        i++;
        MsiCloseHandle(hrec);
    }
    ok( i == 2, "Expected 2 rows, got %lu\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT `StdDlls`.`Binary_`, `Binary`.`Name` "
            "FROM `StdDlls`, `Binary` "
            "WHERE `StdDlls`.`File` = `Binary`.`Data` "
            "ORDER BY `Name`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        check_record(hrec, 2, join_res_fourth[i][0], join_res_fourth[i][1]);
        i++;
        MsiCloseHandle(hrec);
    }
    ok( i == 1, "Expected 1 rows, got %lu\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT `Component`.`ComponentId`, `FeatureComponents`.`Feature_` "
            "FROM `Component`, `FeatureComponents` "
            "WHERE `Component`.`Component` = 'zygomatic' "
            "AND `FeatureComponents`.`Component_` = 'maxilla' "
            "ORDER BY `Feature_`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        check_record(hrec, 2, join_res_fifth[i][0], join_res_fifth[i][1]);
        i++;
        MsiCloseHandle(hrec);
    }
    ok( i == 1, "Expected 1 rows, got %lu\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT `Component`.`ComponentId`, `FeatureComponents`.`Feature_` "
            "FROM `Component`, `FeatureComponents` "
            "WHERE `Component` = 'zygomatic' "
            "ORDER BY `Feature_`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        check_record(hrec, 2, join_res_sixth[i][0], join_res_sixth[i][1]);
        i++;
        MsiCloseHandle(hrec);
    }
    ok( i == 6, "Expected 6 rows, got %lu\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT `Component`.`ComponentId`, `FeatureComponents`.`Feature_` "
            "FROM `Component`, `FeatureComponents` "
            "WHERE `Component` = 'zygomatic' "
            "AND `Feature_` = 'nasalis' "
            "ORDER BY `Feature_`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        check_record(hrec, 2, join_res_seventh[i][0], join_res_seventh[i][1]);
        i++;
        MsiCloseHandle(hrec);
    }
    ok( i == 3, "Expected 3 rows, got %lu\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT `StdDlls`.`File`, `Binary`.`Data` "
            "FROM `StdDlls`, `Binary` ";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        check_record(hrec, 2, join_res_eighth[i][0], join_res_eighth[i][3]);
        i++;
        MsiCloseHandle(hrec);
    }
    ok( i == 6, "Expected 6 rows, got %lu\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT * FROM `StdDlls`, `Binary` ";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        check_record(hrec, 4, join_res_eighth[i][0], join_res_eighth[i][1],
                join_res_eighth[i][2], join_res_eighth[i][3]);
        i++;
        MsiCloseHandle(hrec);
    }
    ok( i == 6, "Expected 6 rows, got %lu\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT * FROM `One`, `Two`, `Three` ";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    i = 0;
    while ((r = MsiViewFetch(hview, &hrec)) == ERROR_SUCCESS)
    {
        check_record(hrec, 6, join_res_ninth[i][0], join_res_ninth[i][1],
                join_res_ninth[i][2], join_res_ninth[i][3],
                join_res_ninth[i][4], join_res_ninth[i][5]);
        i++;
        MsiCloseHandle(hrec);
    }
    ok( i == 6, "Expected 6 rows, got %lu\n", i );
    ok( r == ERROR_NO_MORE_ITEMS, "expected no more items: %d\n", r );

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT * FROM `Four`, `Five`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT * FROM `Nonexistent`, `One`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok( r == ERROR_BAD_QUERY_SYNTAX,
        "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r );

    /* try updating a row in a join table */
    query = "SELECT `Component`.`ComponentId`, `FeatureComponents`.`Feature_` "
            "FROM `Component`, `FeatureComponents` "
            "WHERE `Component`.`Component` = `FeatureComponents`.`Component_` "
            "ORDER BY `Feature_`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok( r == ERROR_SUCCESS, "failed to open view: %d\n", r );

    r = MsiViewExecute(hview, 0);
    ok( r == ERROR_SUCCESS, "failed to execute view: %d\n", r );

    r = MsiViewFetch(hview, &hrec);
    ok( r == ERROR_SUCCESS, "failed to fetch view: %d\n", r );
    check_record(hrec, 2, "alveolar", "procerus");

    r = MsiRecordSetStringA( hrec, 1, "fascia" );
    ok( r == ERROR_SUCCESS, "failed to set string: %d\n", r );
    r = MsiRecordSetStringA( hrec, 2, "pterygoid" );
    ok( r == ERROR_SUCCESS, "failed to set string: %d\n", r );

    r = MsiViewModify(hview, MSIMODIFY_REFRESH, hrec);
    ok( r == ERROR_SUCCESS, "failed to refresh row: %d\n", r );
    check_record(hrec, 2, "alveolar", "procerus");

    r = MsiRecordSetStringA( hrec, 1, "epicranius" );
    ok( r == ERROR_SUCCESS, "failed to set string: %d\n", r );

    r = MsiViewModify(hview, MSIMODIFY_UPDATE, hrec);
    ok( r == ERROR_SUCCESS, "failed to update row: %d\n", r );

    /* primary key cannot be updated */
    r = MsiRecordSetStringA( hrec, 2, "epicranius" );
    ok( r == ERROR_SUCCESS, "failed to set string: %d\n", r );

    r = MsiViewModify(hview, MSIMODIFY_UPDATE, hrec);
    ok( r == ERROR_FUNCTION_FAILED, "failed to update row: %d\n", r );

    /* all other operations are invalid for joins */
    r = MsiViewModify(hview, MSIMODIFY_SEEK, hrec);
    ok( r == ERROR_FUNCTION_FAILED, "unexpected result: %d\n", r );

    r = MsiViewModify(hview, MSIMODIFY_ASSIGN, hrec);
    ok( r == ERROR_FUNCTION_FAILED, "unexpected result: %d\n", r );

    r = MsiViewModify(hview, MSIMODIFY_REPLACE, hrec);
    ok( r == ERROR_FUNCTION_FAILED, "unexpected result: %d\n", r );

    r = MsiViewModify(hview, MSIMODIFY_MERGE, hrec);
    ok( r == ERROR_FUNCTION_FAILED, "unexpected result: %d\n", r );

    r = MsiViewModify(hview, MSIMODIFY_DELETE, hrec);
    ok( r == ERROR_FUNCTION_FAILED, "unexpected result: %d\n", r );

    r = MsiViewModify(hview, MSIMODIFY_VALIDATE, hrec);
    ok( r == ERROR_FUNCTION_FAILED, "unexpected result: %d\n", r );

    r = MsiViewModify(hview, MSIMODIFY_VALIDATE_DELETE, hrec);
    ok( r == ERROR_FUNCTION_FAILED, "unexpected result: %d\n", r );

    MsiRecordSetStringA(hrec, 2, "epicranius");
    r = MsiViewModify(hview, MSIMODIFY_INSERT, hrec);
    ok( r == ERROR_FUNCTION_FAILED, "unexpected result: %d\n", r );

    r = MsiViewModify(hview, MSIMODIFY_INSERT_TEMPORARY, hrec);
    ok( r == ERROR_FUNCTION_FAILED, "unexpected result: %d\n", r );

    r = MsiViewModify(hview, MSIMODIFY_VALIDATE_NEW, hrec);
    ok( r == ERROR_FUNCTION_FAILED, "unexpected result: %d\n", r );

    r = MsiViewModify(hview, MSIMODIFY_VALIDATE_FIELD, hrec);
    ok( r == ERROR_FUNCTION_FAILED, "unexpected result: %d\n", r );

    MsiCloseHandle(hrec);
    MsiViewClose(hview);
    MsiCloseHandle(hview);

    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    check_record(hrec, 2, "epicranius", "procerus");
    MsiCloseHandle(hrec);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_temporary_table(void)
{
    MSICONDITION cond;
    MSIHANDLE hdb = 0, view = 0, rec;
    const char *query;
    UINT r;

    cond = MsiDatabaseIsTablePersistentA(0, NULL);
    ok( cond == MSICONDITION_ERROR, "wrong return condition\n");

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    cond = MsiDatabaseIsTablePersistentA(hdb, NULL);
    ok( cond == MSICONDITION_ERROR, "wrong return condition\n");

    cond = MsiDatabaseIsTablePersistentA(hdb, "_Tables");
    ok( cond == MSICONDITION_NONE, "wrong return condition\n");

    cond = MsiDatabaseIsTablePersistentA(hdb, "_Columns");
    ok( cond == MSICONDITION_NONE, "wrong return condition\n");

    cond = MsiDatabaseIsTablePersistentA(hdb, "_Storages");
    ok( cond == MSICONDITION_NONE, "wrong return condition\n");

    cond = MsiDatabaseIsTablePersistentA(hdb, "_Streams");
    ok( cond == MSICONDITION_NONE, "wrong return condition\n");

    query = "CREATE TABLE `P` ( `B` SHORT NOT NULL, `C` CHAR(255) PRIMARY KEY `C`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    cond = MsiDatabaseIsTablePersistentA(hdb, "P");
    ok( cond == MSICONDITION_TRUE, "wrong return condition\n");

    query = "CREATE TABLE `P2` ( `B` SHORT NOT NULL, `C` CHAR(255) PRIMARY KEY `C`) HOLD";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    cond = MsiDatabaseIsTablePersistentA(hdb, "P2");
    ok( cond == MSICONDITION_TRUE, "wrong return condition\n");

    query = "CREATE TABLE `T` ( `B` SHORT NOT NULL TEMPORARY, `C` CHAR(255) TEMPORARY PRIMARY KEY `C`) HOLD";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    cond = MsiDatabaseIsTablePersistentA(hdb, "T");
    ok( cond == MSICONDITION_FALSE, "wrong return condition\n");

    query = "CREATE TABLE `T2` ( `B` SHORT NOT NULL TEMPORARY, `C` CHAR(255) TEMPORARY PRIMARY KEY `C`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    query = "SELECT * FROM `T2`";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    cond = MsiDatabaseIsTablePersistentA(hdb, "T2");
    ok( cond == MSICONDITION_NONE, "wrong return condition\n");

    query = "CREATE TABLE `T3` ( `B` SHORT NOT NULL TEMPORARY, `C` CHAR(255) PRIMARY KEY `C`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    cond = MsiDatabaseIsTablePersistentA(hdb, "T3");
    ok( cond == MSICONDITION_TRUE, "wrong return condition\n");

    query = "CREATE TABLE `T4` ( `B` SHORT NOT NULL, `C` CHAR(255) TEMPORARY PRIMARY KEY `C`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_FUNCTION_FAILED, "failed to add table\n");

    cond = MsiDatabaseIsTablePersistentA(hdb, "T4");
    ok( cond == MSICONDITION_NONE, "wrong return condition\n");

    query = "CREATE TABLE `T5` ( `B` SHORT NOT NULL TEMP, `C` CHAR(255) TEMP PRIMARY KEY `C`) HOLD";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "failed to add table\n");

    query = "select * from `T`";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "failed to query table\n");
    r = MsiViewGetColumnInfo(view, MSICOLINFO_TYPES, &rec);
    ok(r == ERROR_SUCCESS, "failed to get column info\n");
    check_record(rec, 2, "G255", "j2");
    MsiCloseHandle( rec );

    MsiViewClose( view );
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
    DeleteFileA(msifile);
}

static void test_alter(void)
{
    MSICONDITION cond;
    MSIHANDLE hdb = 0, rec;
    const char *query;
    UINT r;

    hdb = create_db();
    ok( hdb, "failed to create db\n");

    query = "CREATE TABLE `T` ( `B` SHORT NOT NULL TEMPORARY, `C` CHAR(255) TEMPORARY PRIMARY KEY `C`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    query = "SELECT * FROM `T`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "CREATE TABLE `T` ( `B` SHORT NOT NULL TEMPORARY, `C` CHAR(255) TEMPORARY PRIMARY KEY `C`) HOLD";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to add table\n");

    query = "SELECT * FROM `T`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %d\n", r);

    cond = MsiDatabaseIsTablePersistentA(hdb, "T");
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

    query = "SELECT * FROM `_Columns` WHERE `Table` = 'U' AND `Name` = 'C'";
    r = do_query(hdb, query, &rec);
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

    query = "SELECT * FROM `_Columns` WHERE `Table` = 'U' AND `Name` = 'D'";
    r = do_query(hdb, query, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

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
    DeleteFileA(msifile);
}

static void test_integers(void)
{
    MSIHANDLE hdb = 0, view = 0, rec = 0;
    DWORD i;
    const char *query;
    UINT r;

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    /* create a table */
    query = "CREATE TABLE `integers` ( "
            "`one` SHORT, `two` INT, `three` INTEGER, `four` LONG, "
            "`five` SHORT NOT NULL, `six` INT NOT NULL, "
            "`seven` INTEGER NOT NULL, `eight` LONG NOT NULL "
            "PRIMARY KEY `one`)";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewClose(view);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(view);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    query = "SELECT * FROM `integers`";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_NAMES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 8, "one", "two", "three", "four", "five", "six", "seven", "eight");
    MsiCloseHandle(rec);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_TYPES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 8, "I2", "I2", "I2", "I4", "i2", "i2", "i2", "i4");
    MsiCloseHandle(rec);

    MsiViewClose(view);
    MsiCloseHandle(view);

    /* insert values into it, NULL where NOT NULL is specified */
    query = "INSERT INTO `integers` ( `one`, `two`, `three`, `four`, `five`, `six`, `seven`, `eight` )"
        "VALUES('', '', '', '', '', '', '', '')";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
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
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `integers`";
    r = do_query(hdb, query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiRecordGetFieldCount(rec);
    ok(r == 8, "record count wrong: %d\n", r);

    i = MsiRecordGetInteger(rec, 1);
    ok(i == MSI_NULL_INTEGER, "Expected MSI_NULL_INTEGER, got %lu\n", i);
    i = MsiRecordGetInteger(rec, 3);
    ok(i == MSI_NULL_INTEGER, "Expected MSI_NULL_INTEGER, got %lu\n", i);
    i = MsiRecordGetInteger(rec, 2);
    ok(i == 2, "Expected 2, got %lu\n", i);
    i = MsiRecordGetInteger(rec, 4);
    ok(i == 4, "Expected 4, got %lu\n", i);
    i = MsiRecordGetInteger(rec, 5);
    ok(i == 5, "Expected 5, got %lu\n", i);
    i = MsiRecordGetInteger(rec, 6);
    ok(i == 6, "Expected 6, got %lu\n", i);
    i = MsiRecordGetInteger(rec, 7);
    ok(i == 7, "Expected 7, got %lu\n", i);
    i = MsiRecordGetInteger(rec, 8);
    ok(i == 8, "Expected 8, got %lu\n", i);

    MsiCloseHandle(rec);
    MsiViewClose(view);
    MsiCloseHandle(view);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "MsiDatabaseCommit failed\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = DeleteFileA(msifile);
    ok(r == TRUE, "file didn't exist after commit\n");
}

static void test_update(void)
{
    MSIHANDLE hdb = 0, view = 0, rec = 0;
    const char *query;
    UINT r;

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    /* create the Control table */
    query = "CREATE TABLE `Control` ( "
        "`Dialog_` CHAR(72) NOT NULL, `Control` CHAR(50) NOT NULL, `Type` SHORT NOT NULL, "
        "`X` SHORT NOT NULL, `Y` SHORT NOT NULL, `Width` SHORT NOT NULL, `Height` SHORT NOT NULL,"
        "`Attributes` LONG, `Property` CHAR(50), `Text` CHAR(0) LOCALIZABLE, "
        "`Control_Next` CHAR(50), `Help` CHAR(50) LOCALIZABLE PRIMARY KEY `Dialog_`, `Control`)";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
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
    r = MsiDatabaseOpenViewA(hdb, query, &view);
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
    r = MsiDatabaseOpenViewA(hdb, query, &view);
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
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewClose(view);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(view);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* bad table */
    query = "UPDATE `NotATable` SET `Text` = 'this is text' WHERE `Dialog_` = 'ErrorDialog'";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* bad set column */
    query = "UPDATE `Control` SET `NotAColumn` = 'this is text' WHERE `Dialog_` = 'ErrorDialog'";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* bad where condition */
    query = "UPDATE `Control` SET `Text` = 'this is text' WHERE `NotAColumn` = 'ErrorDialog'";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    /* just the dialog_ specified */
    query = "UPDATE `Control` SET `Text` = 'this is text' WHERE `Dialog_` = 'ErrorDialog'";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewClose(view);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(view);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* check the modified text */
    query = "SELECT `Text` FROM `Control` WHERE `Control` = 'ErrorText'";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 1, "this is text");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 1, "");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiViewClose(view);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(view);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* dialog_ and control specified */
    query = "UPDATE `Control` SET `Text` = 'this is text' WHERE `Dialog_` = 'ErrorDialog' AND `Control` = 'ErrorText'";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewClose(view);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(view);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* check the modified text */
    query = "SELECT `Text` FROM `Control` WHERE `Control` = 'ErrorText'";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 1, "this is text");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 1, "");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiViewClose(view);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(view);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* no where condition */
    query = "UPDATE `Control` SET `Text` = 'this is text'";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewClose(view);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(view);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* check the modified text */
    query = "SELECT `Text` FROM `Control`";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 1, "this is text");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 1, "this is text");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 1, "this is text");
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
    MsiRecordSetStringA(rec, 2, "two");

    query = "UPDATE `Apple` SET `Pear` = ? WHERE `Orange` = ?";
    r = run_query(hdb, rec, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(rec);

    query = "SELECT `Pear` FROM `Apple` ORDER BY `Orange`";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
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
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(view);
    MsiCloseHandle(view);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "MsiDatabaseCommit failed\n");
    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    DeleteFileA(msifile);
}

static void test_special_tables(void)
{
    const char *query;
    MSIHANDLE hdb = 0;
    UINT r;

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    query = "CREATE TABLE `_Properties` ( "
        "`foo` INT NOT NULL, `bar` INT LOCALIZABLE PRIMARY KEY `foo`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to create table\n");

    query = "CREATE TABLE `_Storages` ( "
        "`foo` INT NOT NULL, `bar` INT LOCALIZABLE PRIMARY KEY `foo`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "created _Streams table\n");

    query = "CREATE TABLE `_Streams` ( "
        "`foo` INT NOT NULL, `bar` INT LOCALIZABLE PRIMARY KEY `foo`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "created _Streams table\n");

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

static void test_tables_order(void)
{
    const char *query;
    MSIHANDLE hdb = 0, hview = 0, hrec = 0;
    UINT r;

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    query = "CREATE TABLE `foo` ( "
        "`baz` INT NOT NULL PRIMARY KEY `baz`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to create table\n");

    query = "CREATE TABLE `bar` ( "
        "`foo` INT NOT NULL PRIMARY KEY `foo`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to create table\n");

    query = "CREATE TABLE `baz` ( "
        "`bar` INT NOT NULL, "
        "`baz` INT NOT NULL, "
        "`foo` INT NOT NULL PRIMARY KEY `bar`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to create table\n");

    /* The names of the tables in the _Tables table must
       be in the same order as these names are created in
       the strings table. */
    query = "SELECT `Name` FROM `_Tables`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    check_record(hrec, 1, "foo");
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    check_record(hrec, 1, "baz");
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    check_record(hrec, 1, "bar");
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* The names of the tables in the _Columns table must
       be in the same order as these names are created in
       the strings table. */
    query = "SELECT `Table`, `Number`, `Name` FROM `_Columns`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    check_record(hrec, 3, "foo", "1", "baz");
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    check_record(hrec, 3, "baz", "1", "bar");
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    check_record(hrec, 3, "baz", "2", "baz");
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    check_record(hrec, 3, "baz", "3", "foo");
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    check_record(hrec, 3, "bar", "1", "foo");
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    DeleteFileA(msifile);
}

static void test_rows_order(void)
{
    const char *query;
    MSIHANDLE hdb = 0, hview = 0, hrec = 0;
    UINT r;

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    query = "CREATE TABLE `foo` ( "
        "`bar` LONGCHAR NOT NULL PRIMARY KEY `bar`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to create table\n");

    r = run_query(hdb, 0, "INSERT INTO `foo` "
            "( `bar` ) VALUES ( 'A' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table\n");

    r = run_query(hdb, 0, "INSERT INTO `foo` "
            "( `bar` ) VALUES ( 'B' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table\n");

    r = run_query(hdb, 0, "INSERT INTO `foo` "
            "( `bar` ) VALUES ( 'C' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table\n");

    r = run_query(hdb, 0, "INSERT INTO `foo` "
            "( `bar` ) VALUES ( 'D' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table\n");

    r = run_query(hdb, 0, "INSERT INTO `foo` "
            "( `bar` ) VALUES ( 'E' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table\n");

    r = run_query(hdb, 0, "INSERT INTO `foo` "
            "( `bar` ) VALUES ( 'F' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table\n");

    query = "CREATE TABLE `bar` ( "
        "`foo` LONGCHAR NOT NULL, "
        "`baz` LONGCHAR NOT NULL "
        "PRIMARY KEY `foo` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to create table\n");

    r = run_query(hdb, 0, "INSERT INTO `bar` "
            "( `foo`, `baz` ) VALUES ( 'C', 'E' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table\n");

    r = run_query(hdb, 0, "INSERT INTO `bar` "
            "( `foo`, `baz` ) VALUES ( 'F', 'A' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table\n");

    r = run_query(hdb, 0, "INSERT INTO `bar` "
            "( `foo`, `baz` ) VALUES ( 'A', 'B' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table\n");

    r = run_query(hdb, 0, "INSERT INTO `bar` "
            "( `foo`, `baz` ) VALUES ( 'D', 'E' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table\n");

    /* The rows of the table must be ordered by the column values of
       each row. For strings, the column value is the string id
       in the string table.  */

    query = "SELECT * FROM `bar`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    check_record(hrec, 2, "A", "B");
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    check_record(hrec, 2, "C", "E");
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    check_record(hrec, 2, "D", "E");
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    check_record(hrec, 2, "F", "A");
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    DeleteFileA(msifile);
}

static void test_collation(void)
{
    const char *query;
    MSIHANDLE hdb = 0, hview = 0, hrec = 0;
    UINT r;
    char buffer[100];
    WCHAR bufferW[100];
    DWORD sz;

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    query = "CREATE TABLE `bar` ( "
        "`foo` LONGCHAR NOT NULL, "
        "`baz` LONGCHAR NOT NULL "
        "PRIMARY KEY `foo` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "failed to create table\n");

    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "wrong error %u\n", r);

    r = run_query(hdb, 0, "INSERT INTO `bar` "
            "( `foo`, `baz` ) VALUES ( '\2', 'A' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table %u\n", r);

    r = run_query(hdb, 0, "INSERT INTO `bar` "
            "( `foo`, `baz` ) VALUES ( '\1', 'B' )");
    ok(r == ERROR_SUCCESS, "cannot add value to table %u\n", r);

    r = run_queryW(hdb, 0, L"INSERT INTO `bar` (`foo`,`baz`) VALUES ('a\x30a','C')");
    ok(r == ERROR_SUCCESS, "cannot add value to table %u\n", r);

    r = run_queryW(hdb, 0, L"INSERT INTO `bar` (`foo`,`baz`) VALUES ('\xe5','D')");
    ok(r == ERROR_SUCCESS, "cannot add value to table %u\n", r);

    r = run_queryW(hdb, 0, L"CREATE TABLE `baz` ( `a\x30a` LONGCHAR NOT NULL, `\xe5` LONGCHAR NOT NULL PRIMARY KEY `a\x30a`)");
    ok(r == ERROR_SUCCESS, "cannot create table %u\n", r);

    r = run_queryW(hdb, 0, L"CREATE TABLE `a\x30a` ( `foo` LONGCHAR NOT NULL PRIMARY KEY `foo`)");
    ok(r == ERROR_SUCCESS, "cannot create table %u\n", r);

    r = run_queryW(hdb, 0, L"CREATE TABLE `\xe5` ( `foo` LONGCHAR NOT NULL PRIMARY KEY `foo`)");
    ok(r == ERROR_SUCCESS, "cannot create table %u\n", r);

    query = "SELECT * FROM `bar`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    sz = sizeof(buffer);
    r = MsiRecordGetStringA(hrec, 1, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buffer, "\2"), "Expected \\2, got '%s'\n", buffer);
    sz = sizeof(buffer);
    r = MsiRecordGetStringA(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buffer, "A"), "Expected A, got '%s'\n", buffer);
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    sz = sizeof(buffer);
    r = MsiRecordGetStringA(hrec, 1, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buffer, "\1"), "Expected \\1, got '%s'\n", buffer);
    sz = sizeof(buffer);
    r = MsiRecordGetStringA(hrec, 2, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buffer, "B"), "Expected B, got '%s'\n", buffer);
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    sz = ARRAY_SIZE(bufferW);
    r = MsiRecordGetStringW(hrec, 1, bufferW, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!memcmp(bufferW, L"a\x30a", sizeof(L"a\x30a")),
       "Expected %s, got %s\n", wine_dbgstr_w(L"a\x30a"), wine_dbgstr_w(bufferW));
    sz = ARRAY_SIZE(bufferW);
    r = MsiRecordGetStringW(hrec, 2, bufferW, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpW(bufferW, L"C"), "Expected C, got %s\n", wine_dbgstr_w(bufferW));
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    sz = ARRAY_SIZE(bufferW);
    r = MsiRecordGetStringW(hrec, 1, bufferW, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!memcmp(bufferW, L"\xe5", sizeof(L"\xe5")),
       "Expected %s, got %s\n", wine_dbgstr_w(L"\xe5"), wine_dbgstr_w(bufferW));
    sz = ARRAY_SIZE(bufferW);
    r = MsiRecordGetStringW(hrec, 2, bufferW, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpW(bufferW, L"D"), "Expected D, got %s\n", wine_dbgstr_w(bufferW));
    MsiCloseHandle(hrec);

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiDatabaseOpenViewW(hdb, L"SELECT * FROM `bar` WHERE `foo` ='\xe5'", &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    sz = ARRAY_SIZE(bufferW);
    r = MsiRecordGetStringW(hrec, 1, bufferW, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!memcmp(bufferW, L"\xe5", sizeof(L"\xe5")),
       "Expected %s, got %s\n", wine_dbgstr_w(L"\xe5"), wine_dbgstr_w(bufferW));
    sz = ARRAY_SIZE(bufferW);
    r = MsiRecordGetStringW(hrec, 2, bufferW, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpW(bufferW, L"D"), "Expected D, got %s\n", wine_dbgstr_w(bufferW));
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "MsiViewFetch failed\n");

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    DeleteFileA(msifile);
}

static void test_select_markers(void)
{
    MSIHANDLE hdb = 0, rec, view, res;
    LPCSTR query;
    UINT r;

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
    MsiRecordSetStringA(rec, 1, "apple");
    MsiRecordSetStringA(rec, 2, "two");

    query = "SELECT * FROM `Table` WHERE `One`=? AND `Two`=? ORDER BY `Three`";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewExecute(view, rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(view, &res);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(res, 3, "apple", "two", "1");
    MsiCloseHandle(res);

    r = MsiViewFetch(view, &res);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(res, 3, "apple", "two", "2");
    MsiCloseHandle(res);

    r = MsiViewFetch(view, &res);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiCloseHandle(rec);
    MsiViewClose(view);
    MsiCloseHandle(view);

    rec = MsiCreateRecord(2);
    MsiRecordSetStringA(rec, 1, "one");
    MsiRecordSetInteger(rec, 2, 1);

    query = "SELECT * FROM `Table` WHERE `Two`<>? AND `Three`>? ORDER BY `Three`";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(view, &res);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(res, 3, "apple", "two", "2");
    MsiCloseHandle(res);

    r = MsiViewFetch(view, &res);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(res, 3, "banana", "three", "3");
    MsiCloseHandle(res);

    r = MsiViewFetch(view, &res);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiCloseHandle(rec);
    MsiViewClose(view);
    MsiCloseHandle(view);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_viewmodify_update(void)
{
    MSIHANDLE hdb = 0, hview = 0, hrec = 0;
    UINT i, test_max, offset, count;
    const char *query;
    UINT r;

    DeleteFileA(msifile);

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
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

    query = "CREATE TABLE `table2` (`A` INT, `B` INT PRIMARY KEY `A`)";
    r = run_query( hdb, 0, query );
    ok(r == ERROR_SUCCESS, "query failed\n");

    query = "INSERT INTO `table2` (`A`, `B`) VALUES (?, ?)";
    r = MsiDatabaseOpenViewA( hdb, query, &hview );
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");

    test_max = 100;
    offset = 1234;
    for(i = 0; i < test_max; i++)
    {

        hrec = MsiCreateRecord( 2 );
        MsiRecordSetInteger( hrec, 1, test_max - i );
        MsiRecordSetInteger( hrec, 2, i );

        r = MsiViewExecute( hview, hrec );
        ok(r == ERROR_SUCCESS, "Got %d\n", r);

        r = MsiCloseHandle( hrec );
        ok(r == ERROR_SUCCESS, "Got %d\n", r);
    }

    r = MsiViewClose( hview );
    ok(r == ERROR_SUCCESS, "Got %d\n", r);
    r = MsiCloseHandle( hview );
    ok(r == ERROR_SUCCESS, "Got %d\n", r);

    /* Update. */
    query = "SELECT * FROM `table2` ORDER BY `B`";
    r = MsiDatabaseOpenViewA( hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute( hview, 0 );
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    count = 0;
    while (MsiViewFetch( hview, &hrec ) == ERROR_SUCCESS)
    {
        UINT b = MsiRecordGetInteger( hrec, 2 );

        r = MsiRecordSetInteger( hrec, 2, b + offset);
        ok(r == ERROR_SUCCESS, "Got %d\n", r);

        r = MsiViewModify( hview, MSIMODIFY_UPDATE, hrec );
        ok(r == ERROR_SUCCESS, "Got %d\n", r);

        r = MsiCloseHandle(hrec);
        ok(r == ERROR_SUCCESS, "failed to close record\n");
        count++;
    }
    ok(count == test_max, "Got count %d\n", count);

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* Recheck. */
    query = "SELECT * FROM `table2` ORDER BY `B`";
    r = MsiDatabaseOpenViewA( hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute( hview, 0 );
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    count = 0;
    while (MsiViewFetch( hview, &hrec ) == ERROR_SUCCESS)
    {
        UINT a = MsiRecordGetInteger( hrec, 1 );
        UINT b = MsiRecordGetInteger( hrec, 2 );
        ok( ( test_max - a + offset) == b, "Got (%d, %d), expected (%d, %d)\n",
            a, b, test_max - a + offset, b);

        r = MsiCloseHandle(hrec);
        ok(r == ERROR_SUCCESS, "failed to close record\n");
        count++;
    }
    ok(count == test_max, "Got count %d\n", count);

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase close failed\n");
}

static void test_viewmodify_assign(void)
{
    MSIHANDLE hdb = 0, hview = 0, hrec = 0;
    const char *query;
    UINT r;

    /* setup database */
    DeleteFileA(msifile);

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    query = "CREATE TABLE `table` (`A` INT, `B` INT PRIMARY KEY `A`)";
    r = run_query( hdb, 0, query );
    ok(r == ERROR_SUCCESS, "query failed\n");

    /* assign to view, new primary key */
    query = "SELECT * FROM `table`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    hrec = MsiCreateRecord(2);
    ok(hrec != 0, "MsiCreateRecord failed\n");

    r = MsiRecordSetInteger(hrec, 1, 1);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetInteger(hrec, 2, 2);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");

    r = MsiViewModify(hview, MSIMODIFY_ASSIGN, hrec);
    ok(r == ERROR_SUCCESS, "MsiViewModify failed: %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    query = "SELECT * FROM `table`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    check_record(hrec, 2, "1", "2");
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* assign to view, primary key matches */
    query = "SELECT * FROM `table`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    hrec = MsiCreateRecord(2);
    ok(hrec != 0, "MsiCreateRecord failed\n");

    r = MsiRecordSetInteger(hrec, 1, 1);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");
    r = MsiRecordSetInteger(hrec, 2, 4);
    ok(r == ERROR_SUCCESS, "failed to set integer\n");

    r = MsiViewModify(hview, MSIMODIFY_ASSIGN, hrec);
    ok(r == ERROR_SUCCESS, "MsiViewModify failed: %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    query = "SELECT * FROM `table`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");
    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "MsiViewFetch failed\n");
    check_record(hrec, 2, "1", "4");
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "failed to close record\n");

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "MsiViewClose failed\n");
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = run_query(hdb, 0, "CREATE TABLE `table2` (`A` INT, `B` INT, `C` INT, `D` INT PRIMARY KEY `A`,`B`)");
    ok(!r, "got %u\n", r);

    r = MsiDatabaseOpenViewA(hdb, "SELECT * FROM `table2`", &hview);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(hview, 0);
    ok(!r, "got %u\n", r);

    hrec = MsiCreateRecord(4);
    MsiRecordSetInteger(hrec, 1, 1);
    MsiRecordSetInteger(hrec, 2, 2);
    MsiRecordSetInteger(hrec, 3, 3);
    MsiRecordSetInteger(hrec, 4, 4);
    r = MsiViewModify(hview, MSIMODIFY_ASSIGN, hrec);
    ok(!r, "got %u\n", r);
    MsiCloseHandle(hrec);

    MsiCloseHandle(hview);

    r = MsiDatabaseOpenViewA(hdb, "SELECT * FROM `table2`", &hview);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(hview, 0);
    ok(!r, "got %u\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(!r, "got %u\n", r);
    check_record(hrec, 4, "1", "2", "3", "4");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "got %u\n", r);
    MsiCloseHandle(hview);

    r = MsiDatabaseOpenViewA(hdb, "SELECT * FROM `table2`", &hview);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(hview, 0);
    ok(!r, "got %u\n", r);

    hrec = MsiCreateRecord(4);
    MsiRecordSetInteger(hrec, 1, 1);
    MsiRecordSetInteger(hrec, 2, 4);
    MsiRecordSetInteger(hrec, 3, 3);
    MsiRecordSetInteger(hrec, 4, 3);
    r = MsiViewModify(hview, MSIMODIFY_ASSIGN, hrec);
    ok(!r, "got %u\n", r);
    MsiCloseHandle(hrec);

    MsiCloseHandle(hview);

    r = MsiDatabaseOpenViewA(hdb, "SELECT * FROM `table2`", &hview);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(hview, 0);
    ok(!r, "got %u\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(!r, "got %u\n", r);
    check_record(hrec, 4, "1", "2", "3", "4");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(!r, "got %u\n", r);
    check_record(hrec, 4, "1", "4", "3", "3");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "got %u\n", r);
    MsiCloseHandle(hview);

    r = MsiDatabaseOpenViewA(hdb, "SELECT `B`, `C` FROM `table2`", &hview);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(hview, 0);
    ok(!r, "got %u\n", r);

    hrec = MsiCreateRecord(2);
    MsiRecordSetInteger(hrec, 1, 2);
    MsiRecordSetInteger(hrec, 2, 4);
    r = MsiViewModify(hview, MSIMODIFY_ASSIGN, hrec);
    ok(!r, "got %u\n", r);
    MsiRecordSetInteger(hrec, 1, 3);
    r = MsiViewModify(hview, MSIMODIFY_ASSIGN, hrec);
    ok(!r, "got %u\n", r);
    MsiCloseHandle(hrec);

    MsiCloseHandle(hview);

    r = MsiDatabaseOpenViewA(hdb, "SELECT * FROM `table2` ORDER BY `A`", &hview);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(hview, 0);
    ok(!r, "got %u\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(!r, "got %u\n", r);
    check_record(hrec, 4, "", "2", "4", "");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(!r, "got %u\n", r);
    check_record(hrec, 4, "", "3", "4", "");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(!r, "got %u\n", r);
    check_record(hrec, 4, "1", "2", "3", "4");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(!r, "got %u\n", r);
    check_record(hrec, 4, "1", "4", "3", "3");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "got %u\n", r);
    MsiCloseHandle(hview);

    r = MsiDatabaseOpenViewA(hdb, "SELECT `A`, `B`, `C` FROM `table2`", &hview);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(hview, 0);
    ok(!r, "got %u\n", r);

    hrec = MsiCreateRecord(3);
    MsiRecordSetInteger(hrec, 1, 1);
    MsiRecordSetInteger(hrec, 2, 2);
    MsiRecordSetInteger(hrec, 3, 5);
    r = MsiViewModify(hview, MSIMODIFY_ASSIGN, hrec);
    ok(!r, "got %u\n", r);
    MsiCloseHandle(hrec);

    MsiCloseHandle(hview);

    r = MsiDatabaseOpenViewA(hdb, "SELECT * FROM `table2` ORDER BY `A`", &hview);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(hview, 0);
    ok(!r, "got %u\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(!r, "got %u\n", r);
    check_record(hrec, 4, "", "2", "4", "");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(!r, "got %u\n", r);
    check_record(hrec, 4, "", "3", "4", "");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(!r, "got %u\n", r);
    check_record(hrec, 4, "1", "2", "5", "");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(!r, "got %u\n", r);
    check_record(hrec, 4, "1", "4", "3", "3");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "got %u\n", r);
    MsiCloseHandle(hview);

    /* close database */
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
    DWORD read;
    UINT r;

    static const DWORD mode = STGM_DIRECT | STGM_READ | STGM_SHARE_DENY_WRITE;
    static const WCHAR stringdata[] = {0x4840, 0x3f3f, 0x4577, 0x446c, 0x3b6a, 0x45e4, 0x4824, 0}; /* _StringData */
    static const WCHAR stringpool[] = {0x4840, 0x3f3f, 0x4577, 0x446c, 0x3e6a, 0x44b2, 0x482f, 0}; /* _StringPool */
    static const WCHAR moo[] = {0x4840, 0x3e16, 0x4818, 0}; /* MOO */
    static const WCHAR aar[] = {0x4840, 0x3a8a, 0x481b, 0}; /* AAR */

    DeleteFileA(msifile);

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    hrec = MsiCreateRecord(2);

    r = MsiRecordSetInteger(hrec, 1, 3);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiRecordSetStringA(hrec, 2, "three");
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

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_READONLY, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `MOO`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 2, "1", "one");
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

    query = "SELECT * FROM `AAR`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 2, "2", "two");
    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 2, "5", "five");
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
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);
    ok(stg != NULL, "Expected non-NULL storage\n");

    hr = IStorage_OpenStream(stg, moo, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm);
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);
    ok(stm != NULL, "Expected non-NULL stream\n");

    hr = IStream_Read(stm, data, MAX_PATH, &read);
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);
    ok(read == 4, "Expected 4, got %lu\n", read);
    todo_wine ok(!memcmp(data, data10, read), "Unexpected data\n");

    hr = IStream_Release(stm);
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);

    hr = IStorage_OpenStream(stg, aar, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm);
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);
    ok(stm != NULL, "Expected non-NULL stream\n");

    hr = IStream_Read(stm, data, MAX_PATH, &read);
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);
    ok(read == 8, "Expected 8, got %lu\n", read);
    todo_wine
    {
        ok(!memcmp(data, data11, read), "Unexpected data\n");
    }

    hr = IStream_Release(stm);
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);

    hr = IStorage_OpenStream(stg, stringdata, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm);
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);
    ok(stm != NULL, "Expected non-NULL stream\n");

    hr = IStream_Read(stm, buffer, MAX_PATH, &read);
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);
    ok(read == 24, "Expected 24, got %lu\n", read);
    ok(!memcmp(buffer, data12, read), "Unexpected data\n");

    hr = IStream_Release(stm);
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);

    hr = IStorage_OpenStream(stg, stringpool, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm);
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);
    ok(stm != NULL, "Expected non-NULL stream\n");

    hr = IStream_Read(stm, data, MAX_PATH, &read);
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);
    todo_wine
    {
        ok(read == 64, "Expected 64, got %lu\n", read);
        ok(!memcmp(data, data13, read), "Unexpected data\n");
    }

    hr = IStream_Release(stm);
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);

    hr = IStorage_Release(stg);
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);

    DeleteFileA(msifile);
}

static void test_viewmodify_delete(void)
{
    MSIHANDLE hdb = 0, hview = 0, hrec = 0;
    UINT r;
    const char *query;

    DeleteFileA(msifile);

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
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
    MsiRecordSetInteger(hrec, 1, 4);
    r = MsiViewModify(hview, MSIMODIFY_DELETE, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiCloseHandle(hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewClose(hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiCloseHandle(hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `phone`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 3, "3", "Cindy", "2937550");
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
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);

    n = 0;
    while(TRUE)
    {
        count = 0;
        hr = IEnumSTATSTG_Next(stgenum, 1, &stat, &count);
        if(FAILED(hr) || !count)
            break;

        ok(!lstrcmpW(stat.pwcsName, database_table_data[n].name),
           "Expected table %lu name to match\n", n);

        stm = NULL;
        hr = IStorage_OpenStream(stg, stat.pwcsName, NULL,
                                 STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm);
        ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);
        ok(stm != NULL, "Expected non-NULL stream\n");

        CoTaskMemFree(stat.pwcsName);

        sz = MAX_PATH;
        memset(data, 'a', MAX_PATH);
        hr = IStream_Read(stm, data, sz, &count);
        ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);

        ok(count == database_table_data[n].size,
           "Expected %lu, got %lu\n", database_table_data[n].size, count);

        if (!database_table_data[n].size)
            ok(!memcmp(data, check, MAX_PATH), "data should not be changed\n");
        else
            ok(!memcmp(data, database_table_data[n].data, database_table_data[n].size),
               "Expected table %lu data to match\n", n);

        IStream_Release(stm);
        n++;
    }

    ok(n == 3, "Expected 3, got %lu\n", n);

    IEnumSTATSTG_Release(stgenum);
}

static void test_defaultdatabase(void)
{
    UINT r;
    HRESULT hr;
    MSIHANDLE hdb;
    IStorage *stg = NULL;

    DeleteFileA(msifile);

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hdb);

    hr = StgOpenStorage(msifileW, NULL, STGM_READ | STGM_SHARE_DENY_WRITE, NULL, 0, &stg);
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);
    ok(stg != NULL, "Expected non-NULL stg\n");

    enum_stream_names(stg);

    IStorage_Release(stg);
    DeleteFileA(msifile);
}

static void test_order(void)
{
    MSIHANDLE hdb, hview, hrec;
    LPCSTR query;
    int val;
    UINT r;

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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 2, "dos", "3");
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

    DeleteFileA(msifile);

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `Table` ( `A` SHORT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `Table`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
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
    UINT r;

    DeleteFileA(msifile);

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
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

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_READONLY, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `Table`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 1, "two");
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
    UINT r;

    DeleteFileA(msifile);

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
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
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 1, "This is a \"string\" ok");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    write_file("import.idt", import_dat, (sizeof(import_dat) - 1) * sizeof(char));

    r = MsiDatabaseImportA(hdb, CURR_DIR, "import.idt");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    DeleteFileA("import.idt");

    query = "SELECT * FROM `Table`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 1, "This is a new 'string' ok");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_carriagereturn(void)
{
    MSIHANDLE hdb, hview, hrec;
    const char *query;
    UINT r;

    DeleteFileA(msifile);

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `Table`\r ( `A` CHAR(72) NOT NULL PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

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
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

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

    query = "SELECT `Name` FROM `_Tables`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 1, "\rOne");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 1, "Tw\ro");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 1, "Three\r");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_noquotes(void)
{
    MSIHANDLE hdb, hview, hrec;
    const char *query;
    UINT r;

    DeleteFileA(msifile);

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
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

    query = "SELECT `Name` FROM `_Tables`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 1, "Table");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 1, "Table2");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 1, "Table3");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT `Table`, `Number`, `Name` FROM `_Columns`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 3, "Table", "1", "A");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 3, "Table2", "1", "A");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 3, "Table3", "1", "A");
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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT * FROM `Table` WHERE A = 'hi'";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 1, "hi");
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

    DeleteFileA(msifile);
    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
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

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_DIRECT, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `_ForceCodepage`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    r = MsiDatabaseExportA(hdb, "_ForceCodepage", CURR_DIR, "forcecodepage.idt");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    read_file_data("forcecodepage.idt", buffer);
    ok(!lstrcmpA(buffer, "\r\n\r\n0\t_ForceCodepage\r\n"),
       "Expected \"\r\n\r\n0\t_ForceCodepage\r\n\", got \"%s\"\n", buffer);

    create_file_data("forcecodepage.idt", "\r\n\r\n850\t_ForceCodepage\r\n", 0);

    r = MsiDatabaseImportA(hdb, CURR_DIR, "forcecodepage.idt");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiDatabaseExportA(hdb, "_ForceCodepage", CURR_DIR, "forcecodepage.idt");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    read_file_data("forcecodepage.idt", buffer);
    ok(!lstrcmpA(buffer, "\r\n\r\n850\t_ForceCodepage\r\n"),
       "Expected \"\r\n\r\n850\t_ForceCodepage\r\n\", got \"%s\"\n", buffer);

    create_file_data("forcecodepage.idt", "\r\n\r\n9999\t_ForceCodepage\r\n", 0);

    r = MsiDatabaseImportA(hdb, CURR_DIR, "forcecodepage.idt");
    ok(r == ERROR_FUNCTION_FAILED, "Expected ERROR_FUNCTION_FAILED, got %d\n", r);

    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
    DeleteFileA("forcecodepage.idt");
}

static void test_viewmodify_refresh(void)
{
    MSIHANDLE hdb, hview, hrec;
    const char *query;
    UINT r;

    DeleteFileA(msifile);

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `Table` ( `A` CHAR(72) NOT NULL, `B` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `Table` ( `A`, `B` ) VALUES ( 'hi', 1 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `Table`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 2, "hi", "1");

    MsiRecordSetInteger(hrec, 2, 5);
    r = MsiViewModify(hview, MSIMODIFY_REFRESH, hrec);
    ok(!r, "got %u\n", r);
    check_record(hrec, 2, "hi", "1");

    MsiRecordSetStringA(hrec, 1, "foo");
    r = MsiViewModify(hview, MSIMODIFY_REFRESH, hrec);
    ok(!r, "got %u\n", r);
    check_record(hrec, 2, "hi", "1");

    query = "UPDATE `Table` SET `B` = 2 WHERE `A` = 'hi'";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewModify(hview, MSIMODIFY_REFRESH, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 2, "hi", "2");

    r = run_query(hdb, 0, "UPDATE `Table` SET `B` = NULL WHERE `A` = 'hi'");
    ok(!r, "got %u\n", r);

    r = MsiViewModify(hview, MSIMODIFY_REFRESH, hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 2, "hi", "");

    MsiCloseHandle(hrec);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "INSERT INTO `Table` ( `A`, `B` ) VALUES ( 'hello', 3 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `Table` WHERE `B` = 3";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
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
    check_record(hrec, 2, "hello", "2");
    MsiCloseHandle(hrec);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    r = MsiDatabaseOpenViewA(hdb, "SELECT `B` FROM `Table` WHERE `A` = 'hello'", &hview);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(hview, 0);
    ok(!r, "got %u\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(!r, "got %u\n", r);
    check_record(hrec, 1, "2");

    MsiRecordSetInteger(hrec, 1, 8);
    r = MsiViewModify(hview, MSIMODIFY_REFRESH, hrec);
    ok(!r, "got %u\n", r);
    check_record(hrec, 1, "2");

    MsiCloseHandle(hrec);
    MsiCloseHandle(hview);

    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_where_viewmodify(void)
{
    MSIHANDLE hdb, hview, hrec;
    const char *query;
    UINT r;

    DeleteFileA(msifile);

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
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
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
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

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_TRANSACT, &hdb);
    ok(r == ERROR_SUCCESS , "Failed to open database\n");

    /* check the column types */
    hrec = get_column_info(hdb, "SELECT * FROM `_Storages`", MSICOLINFO_TYPES);
    ok(hrec, "failed to get column info hrecord\n");
    check_record(hrec, 2, "s62", "V0");
    MsiCloseHandle(hrec);

    /* now try the names */
    hrec = get_column_info(hdb, "SELECT * FROM `_Storages`", MSICOLINFO_NAMES);
    ok(hrec, "failed to get column info hrecord\n");
    check_record(hrec, 2, "Name", "Data");
    MsiCloseHandle(hrec);

    create_storage("storage.bin");

    hrec = MsiCreateRecord(2);
    MsiRecordSetStringA(hrec, 1, "stgname");

    r = MsiRecordSetStreamA(hrec, 2, "storage.bin");
    ok(r == ERROR_SUCCESS, "Failed to add stream data to the hrecord: %d\n", r);

    DeleteFileA("storage.bin");

    query = "INSERT INTO `_Storages` (`Name`, `Data`) VALUES (?, ?)";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Failed to open database hview: %d\n", r);

    r = MsiViewExecute(hview, hrec);
    ok(r == ERROR_SUCCESS, "Failed to execute hview: %d\n", r);

    MsiCloseHandle(hrec);
    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT `Name`, `Data` FROM `_Storages`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Failed to open database hview: %d\n", r);

    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Failed to execute hview: %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Failed to fetch hrecord: %d\n", r);

    size = MAX_PATH;
    r = MsiRecordGetStringA(hrec, 1, file, &size);
    ok(r == ERROR_SUCCESS, "Failed to get string: %d\n", r);
    ok(!lstrcmpA(file, "stgname"), "Expected \"stgname\", got \"%s\"\n", file);

    size = MAX_PATH;
    lstrcpyA(buf, "apple");
    r = MsiRecordReadStream(hrec, 2, buf, &size);
    ok(r == ERROR_INVALID_DATA, "Expected ERROR_INVALID_DATA, got %d\n", r);
    ok(!lstrcmpA(buf, "apple"), "Expected buf to be unchanged, got %s\n", buf);
    ok(size == 0, "Expected 0, got %lu\n", size);

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
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);
    ok(stg != NULL, "Expected non-NULL storage\n");

    MultiByteToWideChar(CP_ACP, 0, "stgname", -1, name, MAX_PATH);
    hr = IStorage_OpenStorage(stg, name, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE,
                              NULL, 0, &inner);
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);
    ok(inner != NULL, "Expected non-NULL storage\n");

    MultiByteToWideChar(CP_ACP, 0, "storage.bin", -1, name, MAX_PATH);
    hr = IStorage_OpenStream(inner, name, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stm);
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);
    ok(stm != NULL, "Expected non-NULL stream\n");

    hr = IStream_Read(stm, buf, MAX_PATH, &size);
    ok(hr == S_OK, "Expected S_OK, got %#lx\n", hr);
    ok(size == 8, "Expected 8, got %lu\n", size);
    ok(!lstrcmpA(buf, "stgdata"), "Expected \"stgdata\", got \"%s\"\n", buf);

    IStream_Release(stm);
    IStorage_Release(inner);

    IStorage_Release(stg);
    DeleteFileA(msifile);
}

static void test_dbtopackage(void)
{
    MSIHANDLE hdb, hpkg;
    CHAR package[12], buf[MAX_PATH];
    DWORD size;
    UINT r;

    /* create an empty database, transact mode */
    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Failed to create database\n");

    set_summary_info(hdb);

    create_directory_table(hdb);

    create_custom_action_table(hdb);
    add_custom_action_entry(hdb, "'SetProp', 51, 'MYPROP', 'grape'");

    sprintf(package, "#%lu", hdb);
    r = MsiOpenPackageA(package, &hpkg);
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        goto error;
    }
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* property is not set yet */
    size = MAX_PATH;
    lstrcpyA(buf, "kiwi");
    r = MsiGetPropertyA(hpkg, "MYPROP", buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(size == 0, "Expected 0, got %lu\n", size);

    /* run the custom action to set the property */
    r = MsiDoActionA(hpkg, "SetProp");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* property is now set */
    size = MAX_PATH;
    lstrcpyA(buf, "kiwi");
    r = MsiGetPropertyA(hpkg, "MYPROP", buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "grape"), "Expected \"grape\", got \"%s\"\n", buf);
    ok(size == 5, "Expected 5, got %lu\n", size);

    MsiCloseHandle(hpkg);

    /* reset the package */
    r = MsiOpenPackageA(package, &hpkg);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* property is not set anymore */
    size = MAX_PATH;
    lstrcpyA(buf, "kiwi");
    r = MsiGetPropertyA(hpkg, "MYPROP", buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
        ok(size == 0, "Expected 0, got %lu\n", size);
    }

    MsiCloseHandle(hdb);
    MsiCloseHandle(hpkg);

    /* create an empty database, direct mode */
    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATEDIRECT, &hdb);
    ok(r == ERROR_SUCCESS, "Failed to create database\n");

    set_summary_info(hdb);

    create_directory_table(hdb);

    create_custom_action_table(hdb);
    add_custom_action_entry(hdb, "'SetProp', 51, 'MYPROP', 'grape'");

    sprintf(package, "#%lu", hdb);
    r = MsiOpenPackageA(package, &hpkg);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* property is not set yet */
    size = MAX_PATH;
    lstrcpyA(buf, "kiwi");
    r = MsiGetPropertyA(hpkg, "MYPROP", buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(size == 0, "Expected 0, got %lu\n", size);

    /* run the custom action to set the property */
    r = MsiDoActionA(hpkg, "SetProp");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* property is now set */
    size = MAX_PATH;
    lstrcpyA(buf, "kiwi");
    r = MsiGetPropertyA(hpkg, "MYPROP", buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "grape"), "Expected \"grape\", got \"%s\"\n", buf);
    ok(size == 5, "Expected 5, got %lu\n", size);

    MsiCloseHandle(hpkg);

    /* reset the package */
    r = MsiOpenPackageA(package, &hpkg);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* property is not set anymore */
    size = MAX_PATH;
    lstrcpyA(buf, "kiwi");
    r = MsiGetPropertyA(hpkg, "MYPROP", buf, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
        ok(size == 0, "Expected 0, got %lu\n", size);
    }

    MsiCloseHandle(hpkg);

error:
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_droptable(void)
{
    MSIHANDLE hdb, hview, hrec;
    LPCSTR query;
    UINT r;

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( `A` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `One`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    query = "SELECT `Name` FROM `_Tables` WHERE `Name` = 'One'";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 1, "One");
    MsiCloseHandle(hrec);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT `Table`, `Number`, `Name` FROM `_Columns` WHERE `Table` = 'One'";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 3, "One", "1", "A");
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

    query = "SELECT `Name` FROM `_Tables` WHERE `Name` = 'One'";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 1, "One");
    MsiCloseHandle(hrec);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "SELECT `Table`, `Number`, `Name` FROM `_Columns` WHERE `Table` = 'One'";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 3, "One", "1", "B");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 3, "One", "2", "C");
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

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiOpenDatabaseW(L"refdb.msi", MSIDBOPEN_CREATE, &href);
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

    query = "CREATE TABLE `One` ( "
        "`A` CHAR(72), "
        "`B` CHAR(56), "
        "`C` CHAR(64) LOCALIZABLE, "
        "`D` LONGCHAR, "
        "`E` CHAR(72) NOT NULL, "
        "`F` CHAR(56) NOT NULL, "
        "`G` CHAR(64) NOT NULL LOCALIZABLE, "
        "`H` LONGCHAR NOT NULL "
        "PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( "
        "`A` CHAR(64), "
        "`B` CHAR(64), "
        "`C` CHAR(64), "
        "`D` CHAR(64), "
        "`E` CHAR(64) NOT NULL, "
        "`F` CHAR(64) NOT NULL, "
        "`G` CHAR(64) NOT NULL, "
        "`H` CHAR(64) NOT NULL "
        "PRIMARY KEY `A` )";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* column string types don't match exactly */
    r = MsiDatabaseMergeA(hdb, href, "MergeErrors");
    ok(r == ERROR_SUCCESS,
       "Expected ERROR_SUCCESS, got %d\n", r);

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
    check_record(hrec, 2, "One", "2");
    MsiCloseHandle(hrec);

    r = MsiDatabaseOpenViewA(hdb, "SELECT * FROM `MergeErrors`", &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewGetColumnInfo(hview, MSICOLINFO_NAMES, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 2, "Table", "NumRowMergeConflicts");
    MsiCloseHandle(hrec);

    r = MsiViewGetColumnInfo(hview, MSICOLINFO_TYPES, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 2, "s255", "i2");
    MsiCloseHandle(hrec);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    query = "DROP TABLE `MergeErrors`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "DROP TABLE `One`";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "DROP TABLE `One`";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( `A` INT, `B` CHAR(72) PRIMARY KEY `A` )";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 1, 'hi' )";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* table from merged database is not in target database */
    r = MsiDatabaseMergeA(hdb, href, "MergeErrors");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `One`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 2, "1", "hi");
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

    query = "CREATE TABLE `One` ( "
            "`A` CHAR(72), `B` INT PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `One` ( "
            "`A` CHAR(72), `B` INT PRIMARY KEY `A` )";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 'hi', 1 )";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* primary key is string */
    r = MsiDatabaseMergeA(hdb, href, "MergeErrors");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `One`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 2, "hi", "1");
    MsiCloseHandle(hrec);

    /* nothing in MergeErrors */
    query = "SELECT * FROM `MergeErrors`";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    create_file_data("codepage.idt", "\r\n\r\n850\t_ForceCodepage\r\n", 0);

    GetCurrentDirectoryA(MAX_PATH, buf);
    r = MsiDatabaseImportA(hdb, buf, "codepage.idt");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

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
    check_record(hrec, 2, "1", "hi");
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
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(buf, "binary.dat\n"),
       "Expected \"binary.dat\\n\", got \"%s\"\n", buf);

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

    query = "CREATE TABLE `One` ( `A` INT, `B` CHAR(72) PRIMARY KEY `A` )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 1, 'foo' )";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `One` ( `A`, `B` ) VALUES ( 2, 'bar' )";
    r = run_query(href, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiDatabaseMergeA(hdb, href, "MergeErrors");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `One`";
    r = MsiDatabaseOpenViewA(hdb, query, &hview);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 2, "1", "foo");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(hrec, 2, "2", "bar");
    MsiCloseHandle(hrec);

    r = MsiViewFetch(hview, &hrec);
    ok(r == ERROR_NO_MORE_ITEMS,
       "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    MsiCloseHandle(hdb);
    MsiCloseHandle(href);
    DeleteFileA(msifile);
    DeleteFileW(L"refdb.msi");
    DeleteFileA("codepage.idt");
    DeleteFileA("binary.dat");
}

static void test_select_with_tablenames(void)
{
    MSIHANDLE hdb, view, rec;
    LPCSTR query;
    UINT r;
    int i;

    int vals[4][2] = {
        {1,12},
        {4,12},
        {1,15},
        {4,15}};

    hdb = create_db();
    ok(hdb, "failed to create db\n");

    /* Build a pair of tables with the same column names, but unique data */
    query = "CREATE TABLE `T1` ( `A` SHORT, `B` SHORT PRIMARY KEY `A`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `T1` ( `A`, `B` ) VALUES ( 1, 2 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `T1` ( `A`, `B` ) VALUES ( 4, 5 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "CREATE TABLE `T2` ( `A` SHORT, `B` SHORT PRIMARY KEY `A`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `T2` ( `A`, `B` ) VALUES ( 11, 12 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `T2` ( `A`, `B` ) VALUES ( 14, 15 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);


    /* Test that selection based on prefixing the column with the table
     * actually selects the right data */

    query = "SELECT T1.A, T2.B FROM T1,T2";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    for (i = 0; i < 4; i++)
    {
        r = MsiViewFetch(view, &rec);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

        r = MsiRecordGetInteger(rec, 1);
        ok(r == vals[i][0], "Expected %d, got %d\n", vals[i][0], r);

        r = MsiRecordGetInteger(rec, 2);
        ok(r == vals[i][1], "Expected %d, got %d\n", vals[i][1], r);

        MsiCloseHandle(rec);
    }

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(view);
    MsiCloseHandle(view);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static const UINT ordervals[6][3] =
{
    { MSI_NULL_INTEGER, 12, 13 },
    { 1, 2, 3 },
    { 6, 4, 5 },
    { 8, 9, 7 },
    { 10, 11, MSI_NULL_INTEGER },
    { 14, MSI_NULL_INTEGER, 15 }
};

static void test_insertorder(void)
{
    MSIHANDLE hdb, view, rec;
    LPCSTR query;
    UINT r;
    int i;

    hdb = create_db();
    ok(hdb, "failed to create db\n");

    query = "CREATE TABLE `T` ( `A` SHORT, `B` SHORT, `C` SHORT PRIMARY KEY `A`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `T` ( `A`, `B`, `C` ) VALUES ( 1, 2, 3 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `T` ( `B`, `C`, `A` ) VALUES ( 4, 5, 6 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `T` ( `C`, `A`, `B` ) VALUES ( 7, 8, 9 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `T` ( `A`, `B` ) VALUES ( 10, 11 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `T` ( `B`, `C` ) VALUES ( 12, 13 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* fails because the primary key already
     * has an MSI_NULL_INTEGER value set above
     */
    query = "INSERT INTO `T` ( `C` ) VALUES ( 14 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_FUNCTION_FAILED,
       "Expected ERROR_FUNCTION_FAILED, got %d\n", r);

    /* replicate the error where primary key is set twice */
    query = "INSERT INTO `T` ( `A`, `C` ) VALUES ( 1, 14 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_FUNCTION_FAILED,
       "Expected ERROR_FUNCTION_FAILED, got %d\n", r);

    query = "INSERT INTO `T` ( `A`, `C` ) VALUES ( 14, 15 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `T` VALUES ( 16 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "INSERT INTO `T` VALUES ( 17, 18 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "INSERT INTO `T` VALUES ( 19, 20, 21 )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX,
       "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    query = "SELECT * FROM `T`";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    for (i = 0; i < 6; i++)
    {
        r = MsiViewFetch(view, &rec);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

        r = MsiRecordGetInteger(rec, 1);
        ok(r == ordervals[i][0], "Expected %d, got %d\n", ordervals[i][0], r);

        r = MsiRecordGetInteger(rec, 2);
        ok(r == ordervals[i][1], "Expected %d, got %d\n", ordervals[i][1], r);

        r = MsiRecordGetInteger(rec, 3);
        ok(r == ordervals[i][2], "Expected %d, got %d\n", ordervals[i][2], r);

        MsiCloseHandle(rec);
    }

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(view);
    MsiCloseHandle(view);

    query = "DELETE FROM `T` WHERE `A` IS NULL";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "INSERT INTO `T` ( `B`, `C` ) VALUES ( 12, 13 ) TEMPORARY";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `T`";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    for (i = 0; i < 6; i++)
    {
        r = MsiViewFetch(view, &rec);
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

        r = MsiRecordGetInteger(rec, 1);
        ok(r == ordervals[i][0], "Expected %d, got %d\n", ordervals[i][0], r);

        r = MsiRecordGetInteger(rec, 2);
        ok(r == ordervals[i][1], "Expected %d, got %d\n", ordervals[i][1], r);

        r = MsiRecordGetInteger(rec, 3);
        ok(r == ordervals[i][2], "Expected %d, got %d\n", ordervals[i][2], r);

        MsiCloseHandle(rec);
    }

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(view);
    MsiCloseHandle(view);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_columnorder(void)
{
    MSIHANDLE hdb, view, rec;
    LPCSTR query;
    UINT r;

    hdb = create_db();
    ok(hdb, "failed to create db\n");

    /* Each column is a slot:
     * ---------------------
     * | B | C | A | E | D |
     * ---------------------
     *
     * When a column is selected as a primary key,
     * the column occupying the nth primary key slot is swapped
     * with the current position of the primary key in question:
     *
     * set primary key `D`
     * ---------------------    ---------------------
     * | B | C | A | E | D | -> | D | C | A | E | B |
     * ---------------------    ---------------------
     *
     * set primary key `E`
     * ---------------------    ---------------------
     * | D | C | A | E | B | -> | D | E | A | C | B |
     * ---------------------    ---------------------
     */

    query = "CREATE TABLE `T` ( `B` SHORT NOT NULL, `C` SHORT NOT NULL, "
            "`A` CHAR(255), `E` INT, `D` CHAR(255) NOT NULL "
            "PRIMARY KEY `D`, `E`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `T`";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_TYPES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 5, "s255", "I2", "S255", "i2", "i2");
    MsiCloseHandle(rec);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_NAMES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 5, "D", "E", "A", "C", "B");
    MsiCloseHandle(rec);

    MsiViewClose(view);
    MsiCloseHandle(view);

    query = "INSERT INTO `T` ( `B`, `C`, `A`, `E`, `D` ) "
            "VALUES ( 1, 2, 'a', 3, 'bc' )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `T`";
    r = do_query(hdb, query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 5, "bc", "3", "a", "2", "1");
    MsiCloseHandle(rec);

    query = "SELECT `Table`, `Number`, `Name` FROM `_Columns` WHERE `Table` = 'T'";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 3, "T", "1", "D");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 3, "T", "2", "E");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 3, "T", "3", "A");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 3, "T", "4", "C");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 3, "T", "5", "B");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(view);
    MsiCloseHandle(view);

    query = "CREATE TABLE `Z` ( `B` SHORT NOT NULL, `C` SHORT NOT NULL, "
            "`A` CHAR(255), `E` INT, `D` CHAR(255) NOT NULL "
            "PRIMARY KEY `C`, `A`, `D`)";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `Z`";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_TYPES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 5, "i2", "S255", "s255", "I2", "i2");
    MsiCloseHandle(rec);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_NAMES, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 5, "C", "A", "D", "E", "B");
    MsiCloseHandle(rec);

    MsiViewClose(view);
    MsiCloseHandle(view);

    query = "INSERT INTO `Z` ( `B`, `C`, `A`, `E`, `D` ) "
            "VALUES ( 1, 2, 'a', 3, 'bc' )";
    r = run_query(hdb, 0, query);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    query = "SELECT * FROM `Z`";
    r = do_query(hdb, query, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 5, "2", "a", "bc", "3", "1");
    MsiCloseHandle(rec);

    query = "SELECT `Table`, `Number`, `Name` FROM `_Columns` WHERE `Table` = 'T'";
    r = MsiDatabaseOpenViewA(hdb, query, &view);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    r = MsiViewExecute(view, 0);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 3, "T", "1", "D");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 3, "T", "2", "E");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 3, "T", "3", "A");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 3, "T", "4", "C");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    check_record(rec, 3, "T", "5", "B");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

    MsiViewClose(view);
    MsiCloseHandle(view);

    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_createtable(void)
{
    MSIHANDLE hdb, htab = 0, hrec = 0;
    LPCSTR query;
    UINT res;
    DWORD size;
    char buffer[0x20];

    hdb = create_db();
    ok(hdb, "failed to create db\n");

    query = "CREATE TABLE `blah` (`foo` CHAR(72) NOT NULL PRIMARY KEY `foo`)";
    res = MsiDatabaseOpenViewA( hdb, query, &htab );
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    if(res == ERROR_SUCCESS )
    {
        res = MsiViewExecute( htab, hrec );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        res = MsiViewGetColumnInfo( htab, MSICOLINFO_NAMES, &hrec );
        todo_wine ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        size = sizeof(buffer);
        res = MsiRecordGetStringA(hrec, 1, buffer, &size );
        todo_wine ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
        MsiCloseHandle( hrec );

        res = MsiViewClose( htab );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        res = MsiCloseHandle( htab );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    }

    query = "CREATE TABLE `a` (`b` INT PRIMARY KEY `b`)";
    res = MsiDatabaseOpenViewA( hdb, query, &htab );
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    if(res == ERROR_SUCCESS )
    {
        res = MsiViewExecute( htab, 0 );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        res = MsiViewClose( htab );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        res = MsiCloseHandle( htab );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        query = "SELECT * FROM `a`";
        res = MsiDatabaseOpenViewA( hdb, query, &htab );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        res = MsiViewGetColumnInfo( htab, MSICOLINFO_NAMES, &hrec );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
        check_record(hrec, 1, "b");
        MsiCloseHandle( hrec );

        res = MsiViewClose( htab );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        res = MsiCloseHandle( htab );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        res = MsiDatabaseCommit(hdb);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        res = MsiCloseHandle(hdb);
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        res = MsiOpenDatabaseW(msifileW, MSIDBOPEN_TRANSACT, &hdb );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        query = "SELECT * FROM `a`";
        res = MsiDatabaseOpenViewA( hdb, query, &htab );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        res = MsiViewGetColumnInfo( htab, MSICOLINFO_NAMES, &hrec );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
        check_record(hrec, 1, "b");
        res = MsiCloseHandle( hrec );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        res = MsiViewClose( htab );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

        res = MsiCloseHandle( htab );
        ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);
    }

    res = MsiDatabaseCommit(hdb);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    res = MsiCloseHandle(hdb);
    ok(res == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", res);

    DeleteFileA(msifile);
}

static void test_embedded_nulls(void)
{
    static const char control_table[] =
        "Dialog\tText\n"
        "s72\tL0\n"
        "Control\tDialog\n"
        "LicenseAgreementDlg\ttext\x11\x19text\0text";
    static const char export_expected[] =
        "Dialog\tText\r\n"
        "s72\tL0\r\n"
        "Control\tDialog\r\n"
        "LicenseAgreementDlg\ttext\x11\x19text\x19text";
    /* newlines have alternate representation in idt files */
    static const char control_table2[] =
        "Dialog\tText\n"
        "s72\tL0\n"
        "Control\tDialog\n"
        "LicenseAgreementDlg\ttext\x11\x19te\nxt\0text";
    char data[1024];
    UINT r;
    DWORD sz;
    MSIHANDLE hdb, hrec;
    char buffer[32];

    r = MsiOpenDatabaseW( msifileW, MSIDBOPEN_CREATE, &hdb );
    ok( r == ERROR_SUCCESS, "failed to open database %u\n", r );

    GetCurrentDirectoryA( MAX_PATH, CURR_DIR );
    write_file( "temp_file", control_table, sizeof(control_table) );
    r = MsiDatabaseImportA( hdb, CURR_DIR, "temp_file" );
    ok( r == ERROR_SUCCESS, "failed to import table %u\n", r );
    DeleteFileA( "temp_file" );

    r = do_query( hdb, "SELECT `Text` FROM `Control` WHERE `Dialog` = 'LicenseAgreementDlg'", &hrec );
    ok( r == ERROR_SUCCESS, "query failed %u\n", r );

    buffer[0] = 0;
    sz = sizeof(buffer);
    r = MsiRecordGetStringA( hrec, 1, buffer, &sz );
    ok( r == ERROR_SUCCESS, "failed to get string %u\n", r );
    ok( !memcmp( "text\r\ntext\ntext", buffer, sizeof("text\r\ntext\ntext") - 1 ), "wrong buffer contents \"%s\"\n", buffer );

    r = MsiDatabaseExportA( hdb, "Control", CURR_DIR, "temp_file1");
    ok( r == ERROR_SUCCESS, "failed to export table %u\n", r );
    read_file_data( "temp_file1", data );
    ok( !memcmp( data, export_expected, sizeof(export_expected) - 1), "expected: \"%s\" got: \"%s\"\n", export_expected, data );
    DeleteFileA( "temp_file1" );

    MsiCloseHandle( hrec );
    MsiCloseHandle( hdb );
    DeleteFileA( msifile );

    r = MsiOpenDatabaseW( msifileW, MSIDBOPEN_CREATE, &hdb );
    ok( r == ERROR_SUCCESS, "failed to open database %u\n", r );

    GetCurrentDirectoryA( MAX_PATH, CURR_DIR );
    write_file( "temp_file", control_table2, sizeof(control_table2) );
    r = MsiDatabaseImportA( hdb, CURR_DIR, "temp_file" );
    ok( r == ERROR_FUNCTION_FAILED, "failed to import table %u\n", r );
    DeleteFileA( "temp_file" );

    MsiCloseHandle( hdb );
    DeleteFileA( msifile );
}

static void test_select_column_names(void)
{
    MSIHANDLE hdb = 0, rec, view;
    UINT r;

    DeleteFileA(msifile);

    r = MsiOpenDatabaseW( msifileW, MSIDBOPEN_CREATE, &hdb );
    ok( r == ERROR_SUCCESS , "failed to open database: %u\n", r );

    r = try_query( hdb, "CREATE TABLE `t` (`a` CHAR NOT NULL, `b` CHAR PRIMARY KEY `a`)");
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "SELECT `t`.`b` FROM `t` WHERE `t`.`b` = `x`" );
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = try_query( hdb, "SELECT '', `t`.`b` FROM `t` WHERE `t`.`b` = 'x'" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "SELECT *, `t`.`b` FROM `t` WHERE `t`.`b` = 'x'" );
    todo_wine ok( r == ERROR_SUCCESS, "query failed: %u\n", r );

    r = try_query( hdb, "SELECT 'b', `t`.`b` FROM `t` WHERE `t`.`b` = 'x'" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "SELECT `t`.`b`, '' FROM `t` WHERE `t`.`b` = 'x'" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "SELECT `t`.`b`, '' FROM `t` WHERE `t`.`b` = 'x' ORDER BY `b`" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "SELECT `t`.`b`, '' FROM `t` WHERE `t`.`b` = 'x' ORDER BY 'b'" );
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = try_query( hdb, "SELECT 't'.'b' FROM `t` WHERE `t`.`b` = 'x'" );
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = try_query( hdb, "SELECT 'b' FROM `t` WHERE `t`.`b` = 'x'" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "INSERT INTO `t` ( `a`, `b` ) VALUES( '1', '2' )" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "INSERT INTO `t` ( `a`, `b` ) VALUES( '3', '4' )" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = MsiDatabaseOpenViewA( hdb, "SELECT '' FROM `t`", &view );
    ok( r == ERROR_SUCCESS, "failed to open database view: %u\n", r );

    r = MsiViewExecute( view, 0 );
    ok( r == ERROR_SUCCESS, "failed to execute view: %u\n", r );

    r = MsiViewFetch( view, &rec );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    check_record(rec, 1, "");
    MsiCloseHandle(rec);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_NAMES, &rec);
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    check_record(rec, 1, "");
    MsiCloseHandle(rec);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_TYPES, &rec);
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    check_record(rec, 1, "f0");
    MsiCloseHandle(rec);

    r = MsiViewFetch( view, &rec );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    check_record(rec, 1, "");
    MsiCloseHandle( rec );

    r = MsiViewFetch( view, &rec );
    ok( r == ERROR_NO_MORE_ITEMS, "unexpected result: %u\n", r );
    MsiCloseHandle( rec );

    MsiViewClose( view );
    MsiCloseHandle( view );

    r = MsiDatabaseOpenViewA( hdb, "SELECT `a`, '' FROM `t`", &view );
    ok( r == ERROR_SUCCESS, "failed to open database view: %u\n", r );

    r = MsiViewExecute( view, 0 );
    ok( r == ERROR_SUCCESS, "failed to execute view: %u\n", r );

    r = MsiViewFetch( view, &rec );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    check_record(rec, 2, "1", "");
    MsiCloseHandle( rec );

    r = MsiViewFetch( view, &rec );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    check_record(rec, 2, "3", "");
    MsiCloseHandle( rec );

    r = MsiViewFetch( view, &rec );
    ok( r == ERROR_NO_MORE_ITEMS, "unexpected result: %u\n", r );
    MsiCloseHandle( rec );

    MsiViewClose( view );
    MsiCloseHandle( view );

    r = MsiDatabaseOpenViewA( hdb, "SELECT '', `a` FROM `t`", &view );
    ok( r == ERROR_SUCCESS, "failed to open database view: %u\n", r );

    r = MsiViewExecute( view, 0 );
    ok( r == ERROR_SUCCESS, "failed to execute view: %u\n", r );

    r = MsiViewFetch( view, &rec );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    check_record(rec, 2, "", "1");
    MsiCloseHandle( rec );

    r = MsiViewFetch( view, &rec );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    check_record(rec, 2, "", "3");
    MsiCloseHandle( rec );

    r = MsiViewFetch( view, &rec );
    ok( r == ERROR_NO_MORE_ITEMS, "unexpected result: %u\n", r );
    MsiCloseHandle( rec );

    MsiViewClose( view );
    MsiCloseHandle( view );

    r = MsiDatabaseOpenViewA( hdb, "SELECT `a`, '', `b` FROM `t`", &view );
    ok( r == ERROR_SUCCESS, "failed to open database view: %u\n", r );

    r = MsiViewExecute( view, 0 );
    ok( r == ERROR_SUCCESS, "failed to execute view: %u\n", r );

    r = MsiViewFetch( view, &rec );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    check_record(rec, 3, "1", "", "2");
    MsiCloseHandle( rec );

    r = MsiViewFetch( view, &rec );
    ok( r == ERROR_SUCCESS, "unexpected result: %u\n", r );
    check_record(rec, 3, "3", "", "4");
    MsiCloseHandle( rec );

    r = MsiViewFetch( view, &rec );
    ok( r == ERROR_NO_MORE_ITEMS, "unexpected result: %u\n", r );
    MsiCloseHandle( rec );

    MsiViewClose( view );
    MsiCloseHandle( view );

    r = try_query( hdb, "SELECT '' FROM `t` WHERE `t`.`b` = 'x'" );
    ok( r == ERROR_SUCCESS , "query failed: %u\n", r );

    r = try_query( hdb, "SELECT `` FROM `t` WHERE `t`.`b` = 'x'" );
    todo_wine ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = try_query( hdb, "SELECT `b` FROM 't' WHERE `t`.`b` = 'x'" );
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = try_query( hdb, "SELECT `b` FROM `t` WHERE 'b' = 'x'" );
    ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = try_query( hdb, "SELECT `t`.`b`, `` FROM `t` WHERE `t`.`b` = 'x'" );
    todo_wine ok( r == ERROR_BAD_QUERY_SYNTAX, "query failed: %u\n", r );

    r = MsiCloseHandle( hdb );
    ok(r == ERROR_SUCCESS , "failed to close database: %u\n", r);
}

static void test_primary_keys(void)
{
    MSIHANDLE hdb, keys;
    char buffer[5];
    DWORD size;
    UINT r;

    hdb = create_db();

    r = MsiDatabaseGetPrimaryKeysA(hdb, "T", &keys);
    ok(r == ERROR_INVALID_TABLE, "got %u\n", r);

    r = run_query(hdb, 0, "CREATE TABLE `T` (`A` SHORT, `B` SHORT, `C` SHORT PRIMARY KEY `A`)");
    ok(!r, "got %u\n", r);

    r = MsiDatabaseGetPrimaryKeysA(hdb, "T", &keys);
    ok(!r, "got %u\n", r);
    check_record(keys, 1, "A");
    size = sizeof(buffer);
    r = MsiRecordGetStringA(keys, 0, buffer, &size);
    ok(!r, "got %u\n", r);
    ok(!strcmp(buffer, "T"), "got \"%s\"\n", buffer);
    MsiCloseHandle(keys);

    r = run_query(hdb, 0, "CREATE TABLE `U` (`A` SHORT, `B` SHORT, `C` SHORT PRIMARY KEY `B`, `C`)");
    ok(!r, "got %u\n", r);

    r = MsiDatabaseGetPrimaryKeysA(hdb, "U", &keys);
    ok(!r, "got %u\n", r);
    check_record(keys, 2, "B", "C");
    size = sizeof(buffer);
    r = MsiRecordGetStringA(keys, 0, buffer, &size);
    ok(!r, "got %u\n", r);
    ok(!strcmp(buffer, "U"), "got \"%s\"\n", buffer);
    MsiCloseHandle(keys);
    MsiCloseHandle(hdb);
    DeleteFileA(msifile);
}

static void test_viewmodify_merge(void)
{
    MSIHANDLE view, rec, db = create_db();
    UINT r;

    r = run_query(db, 0, "CREATE TABLE `T` (`A` SHORT, `B` SHORT PRIMARY KEY `A`)");
    ok(!r, "got %u\n", r);
    r = run_query(db, 0, "INSERT INTO `T` (`A`, `B`) VALUES (1, 2)");
    ok(!r, "got %u\n", r);

    r = MsiDatabaseOpenViewA(db, "SELECT * FROM `T`", &view);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(view, 0);
    ok(!r, "got %u\n", r);

    rec = MsiCreateRecord(2);
    MsiRecordSetInteger(rec, 1, 1);
    MsiRecordSetInteger(rec, 2, 2);
    r = MsiViewModify(view, MSIMODIFY_MERGE, rec);
    ok(!r, "got %u\n", r);

    MsiCloseHandle(rec);
    MsiCloseHandle(view);

    r = MsiDatabaseOpenViewA(db, "SELECT * FROM `T`", &view);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(view, 0);
    ok(!r, "got %u\n", r);

    r = MsiViewFetch(view, &rec);
    ok(!r, "got %u\n", r);
    check_record(rec, 2, "1", "2");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "got %u\n", r);
    MsiCloseHandle(view);

    r = MsiDatabaseOpenViewA(db, "SELECT * FROM `T`", &view);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(view, 0);
    ok(!r, "got %u\n", r);

    rec = MsiCreateRecord(2);
    MsiRecordSetInteger(rec, 1, 1);
    MsiRecordSetInteger(rec, 2, 3);
    r = MsiViewModify(view, MSIMODIFY_MERGE, rec);
    todo_wine
    ok(r == ERROR_FUNCTION_FAILED, "got %u\n", r);

    MsiRecordSetInteger(rec, 1, 2);
    r = MsiViewModify(view, MSIMODIFY_MERGE, rec);
    ok(!r, "got %u\n", r);

    MsiCloseHandle(rec);
    MsiCloseHandle(view);

    r = MsiDatabaseOpenViewA(db, "SELECT * FROM `T`", &view);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(view, 0);
    ok(!r, "got %u\n", r);

    r = MsiViewFetch(view, &rec);
    ok(!r, "got %u\n", r);
    check_record(rec, 2, "1", "2");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(!r, "got %u\n", r);
    check_record(rec, 2, "2", "3");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "got %u\n", r);
    MsiCloseHandle(view);

    r = run_query(db, 0, "CREATE TABLE `U` (`A` SHORT, `B` SHORT, `C` SHORT, `D` SHORT PRIMARY KEY `A`, `B`)");
    ok(!r, "got %u\n", r);
    r = run_query(db, 0, "INSERT INTO `U` (`A`, `B`, `C`, `D`) VALUES (1, 2, 3, 4)");
    ok(!r, "got %u\n", r);

    r = MsiDatabaseOpenViewA(db, "SELECT * FROM `U`", &view);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(view, 0);
    ok(!r, "got %u\n", r);

    rec = MsiCreateRecord(4);
    MsiRecordSetInteger(rec, 1, 1);
    MsiRecordSetInteger(rec, 2, 2);
    MsiRecordSetInteger(rec, 3, 3);
    MsiRecordSetInteger(rec, 4, 4);
    r = MsiViewModify(view, MSIMODIFY_MERGE, rec);
    ok(!r, "got %u\n", r);

    MsiRecordSetInteger(rec, 3, 4);
    r = MsiViewModify(view, MSIMODIFY_MERGE, rec);
    todo_wine
    ok(r == ERROR_FUNCTION_FAILED, "got %u\n", r);

    MsiRecordSetInteger(rec, 2, 4);
    r = MsiViewModify(view, MSIMODIFY_MERGE, rec);
    ok(!r, "got %u\n", r);

    MsiCloseHandle(rec);
    MsiCloseHandle(view);

    r = MsiDatabaseOpenViewA(db, "SELECT * FROM `U`", &view);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(view, 0);
    ok(!r, "got %u\n", r);

    r = MsiViewFetch(view, &rec);
    ok(!r, "got %u\n", r);
    check_record(rec, 4, "1", "2", "3", "4");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(!r, "got %u\n", r);
    check_record(rec, 4, "1", "4", "4", "4");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "got %u\n", r);
    MsiCloseHandle(view);

    r = MsiDatabaseOpenViewA(db, "SELECT `A`,`C` FROM `U`", &view);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(view, 0);
    ok(!r, "got %u\n", r);

    rec = MsiCreateRecord(2);
    MsiRecordSetInteger(rec, 1, 1);
    MsiRecordSetInteger(rec, 2, 2);
    r = MsiViewModify(view, MSIMODIFY_MERGE, rec);
    ok(!r, "got %u\n", r);

    r = MsiViewModify(view, MSIMODIFY_MERGE, rec);
    ok(!r, "got %u\n", r);

    MsiRecordSetInteger(rec, 2, 3);
    r = MsiViewModify(view, MSIMODIFY_MERGE, rec);
    todo_wine
    ok(r == ERROR_FUNCTION_FAILED, "got %u\n", r);

    MsiCloseHandle(rec);
    MsiCloseHandle(view);

    r = MsiDatabaseOpenViewA(db, "SELECT * FROM `U` ORDER BY `B`", &view);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(view, 0);
    ok(!r, "got %u\n", r);

    r = MsiViewFetch(view, &rec);
    ok(!r, "got %u\n", r);
    check_record(rec, 4, "1", "", "2", "");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(!r, "got %u\n", r);
    check_record(rec, 4, "1", "2", "3", "4");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(!r, "got %u\n", r);
    check_record(rec, 4, "1", "4", "4", "4");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "got %u\n", r);
    MsiCloseHandle(view);

    r = MsiDatabaseOpenViewA(db, "SELECT `A`,`B`,`C` FROM `U`", &view);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(view, 0);
    ok(!r, "got %u\n", r);

    rec = MsiCreateRecord(3);
    MsiRecordSetInteger(rec, 1, 1);
    MsiRecordSetInteger(rec, 2, 2);
    MsiRecordSetInteger(rec, 3, 3);
    r = MsiViewModify(view, MSIMODIFY_MERGE, rec);
    todo_wine
    ok(r == ERROR_FUNCTION_FAILED, "got %u\n", r);

    MsiRecordSetInteger(rec, 1, 1);
    MsiRecordSetInteger(rec, 2, MSI_NULL_INTEGER);
    MsiRecordSetInteger(rec, 3, 2);
    r = MsiViewModify(view, MSIMODIFY_MERGE, rec);
    ok(!r, "got %u\n", r);

    MsiCloseHandle(rec);
    MsiCloseHandle(view);

    MsiCloseHandle(db);
    DeleteFileA(msifile);
}

static void test_viewmodify_insert(void)
{
    MSIHANDLE view, rec, db = create_db();
    UINT r;

    r = run_query(db, 0, "CREATE TABLE `T` (`A` SHORT, `B` SHORT PRIMARY KEY `A`)");
    ok(!r, "got %u\n", r);

    r = MsiDatabaseOpenViewA(db, "SELECT * FROM `T`", &view);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(view, 0);
    ok(!r, "got %u\n", r);

    rec = MsiCreateRecord(2);
    MsiRecordSetInteger(rec, 1, 1);
    MsiRecordSetInteger(rec, 2, 2);
    r = MsiViewModify(view, MSIMODIFY_INSERT, rec);
    ok(!r, "got %u\n", r);

    MsiCloseHandle(rec);
    MsiCloseHandle(view);

    r = MsiDatabaseOpenViewA(db, "SELECT * FROM `T`", &view);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(view, 0);
    ok(!r, "got %u\n", r);

    r = MsiViewFetch(view, &rec);
    ok(!r, "got %u\n", r);
    check_record(rec, 2, "1", "2");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "got %u\n", r);
    MsiCloseHandle(view);

    r = MsiDatabaseOpenViewA(db, "SELECT * FROM `T`", &view);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(view, 0);
    ok(!r, "got %u\n", r);

    rec = MsiCreateRecord(2);
    MsiRecordSetInteger(rec, 1, 1);
    MsiRecordSetInteger(rec, 2, 2);
    r = MsiViewModify(view, MSIMODIFY_INSERT, rec);
    ok(r == ERROR_FUNCTION_FAILED, "got %u\n", r);

    MsiRecordSetInteger(rec, 2, 3);
    r = MsiViewModify(view, MSIMODIFY_INSERT, rec);
    ok(r == ERROR_FUNCTION_FAILED, "got %u\n", r);

    MsiRecordSetInteger(rec, 1, 3);
    r = MsiViewModify(view, MSIMODIFY_INSERT, rec);
    ok(!r, "got %u\n", r);

    MsiCloseHandle(rec);
    MsiCloseHandle(view);

    r = MsiDatabaseOpenViewA(db, "SELECT * FROM `T`", &view);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(view, 0);
    ok(!r, "got %u\n", r);

    r = MsiViewFetch(view, &rec);
    ok(!r, "got %u\n", r);
    check_record(rec, 2, "1", "2");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(!r, "got %u\n", r);
    check_record(rec, 2, "3", "3");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "got %u\n", r);
    MsiCloseHandle(view);

    r = run_query(db, 0, "CREATE TABLE `U` (`A` SHORT, `B` SHORT, `C` SHORT, `D` SHORT PRIMARY KEY `A`, `B`)");
    ok(!r, "got %u\n", r);

    r = MsiDatabaseOpenViewA(db, "SELECT * FROM `U`", &view);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(view, 0);
    ok(!r, "got %u\n", r);

    rec = MsiCreateRecord(4);
    MsiRecordSetInteger(rec, 1, 1);
    MsiRecordSetInteger(rec, 2, 2);
    MsiRecordSetInteger(rec, 3, 3);
    MsiRecordSetInteger(rec, 4, 4);
    r = MsiViewModify(view, MSIMODIFY_INSERT, rec);
    ok(!r, "got %u\n", r);

    MsiRecordSetInteger(rec, 2, 4);
    r = MsiViewModify(view, MSIMODIFY_INSERT, rec);
    ok(!r, "got %u\n", r);

    MsiCloseHandle(rec);
    MsiCloseHandle(view);

    r = MsiDatabaseOpenViewA(db, "SELECT * FROM `U`", &view);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(view, 0);
    ok(!r, "got %u\n", r);

    r = MsiViewFetch(view, &rec);
    ok(!r, "got %u\n", r);
    check_record(rec, 4, "1", "2", "3", "4");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(!r, "got %u\n", r);
    check_record(rec, 4, "1", "4", "3", "4");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "got %u\n", r);
    MsiCloseHandle(view);

    r = MsiDatabaseOpenViewA(db, "SELECT `A`,`C` FROM `U`", &view);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(view, 0);
    ok(!r, "got %u\n", r);

    rec = MsiCreateRecord(2);
    MsiRecordSetInteger(rec, 1, 1);
    MsiRecordSetInteger(rec, 2, 2);
    r = MsiViewModify(view, MSIMODIFY_INSERT, rec);
    ok(!r, "got %u\n", r);

    r = MsiViewModify(view, MSIMODIFY_INSERT, rec);
    ok(r == ERROR_FUNCTION_FAILED, "got %u\n", r);

    MsiCloseHandle(rec);
    MsiCloseHandle(view);

    r = MsiDatabaseOpenViewA(db, "SELECT * FROM `U` ORDER BY `B`", &view);
    ok(!r, "got %u\n", r);
    r = MsiViewExecute(view, 0);
    ok(!r, "got %u\n", r);

    r = MsiViewFetch(view, &rec);
    ok(!r, "got %u\n", r);
    check_record(rec, 4, "1", "", "2", "");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(!r, "got %u\n", r);
    check_record(rec, 4, "1", "2", "3", "4");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(!r, "got %u\n", r);
    check_record(rec, 4, "1", "4", "3", "4");
    MsiCloseHandle(rec);

    r = MsiViewFetch(view, &rec);
    ok(r == ERROR_NO_MORE_ITEMS, "got %u\n", r);
    MsiCloseHandle(view);

    MsiCloseHandle(db);
    DeleteFileA(msifile);
}

static void test_view_get_error(void)
{
    MSIHANDLE view, rec, db = create_db();
    MSIDBERROR err;
    char buffer[5];
    DWORD sz;
    UINT r;

    r = run_query(db, 0, "CREATE TABLE `T` (`A` SHORT, `B` SHORT NOT NULL PRIMARY KEY `A`)");
    ok(!r, "got %u\n", r);
    r = run_query(db, 0, "INSERT INTO `T` (`A`, `B`) VALUES (1, 2)");
    r = run_query(db, 0, "CREATE TABLE `_Validation` ("
            "`Table` CHAR(32) NOT NULL, `Column` CHAR(32) NOT NULL, "
            "`Nullable` CHAR(4) NOT NULL, `MinValue` INT, `MaxValue` INT, "
            "`KeyTable` CHAR(255), `KeyColumn` SHORT, `Category` CHAR(32), "
            "`Set` CHAR(255), `Description` CHAR(255) PRIMARY KEY `Table`, `Column`)");
    ok(!r, "got %u\n", r);
    r = run_query(db, 0, "INSERT INTO `_Validation` (`Table`, `Column`, `Nullable`) VALUES ('T', 'A', 'N')");
    ok(!r, "got %u\n", r);
    r = run_query(db, 0, "INSERT INTO `_Validation` (`Table`, `Column`, `Nullable`) VALUES ('T', 'B', 'N')");
    ok(!r, "got %u\n", r);

    r = MsiDatabaseOpenViewA(db, "SELECT * FROM `T`", &view);
    ok(!r, "got %u\n", r);

    r = MsiViewExecute(view, 0);
    ok(!r, "got %u\n", r);

    sz = 0;
    err = MsiViewGetErrorA(0, NULL, &sz);
    ok(err == MSIDBERROR_INVALIDARG, "got %d\n", err);
    ok(sz == 0, "got size %lu\n", sz);

    err = MsiViewGetErrorA(view, NULL, NULL);
    ok(err == MSIDBERROR_INVALIDARG, "got %d\n", err);

    sz = 0;
    err = MsiViewGetErrorA(view, NULL, &sz);
    ok(err == MSIDBERROR_NOERROR, "got %d\n", err);
    ok(sz == 0, "got size %lu\n", sz);

    sz = 0;
    strcpy(buffer, "x");
    err = MsiViewGetErrorA(view, buffer, &sz);
    ok(err == MSIDBERROR_MOREDATA, "got %d\n", err);
    ok(!strcmp(buffer, "x"), "got \"%s\"\n", buffer);
    ok(sz == 0, "got size %lu\n", sz);

    sz = 1;
    strcpy(buffer, "x");
    err = MsiViewGetErrorA(view, buffer, &sz);
    ok(err == MSIDBERROR_NOERROR, "got %d\n", err);
    ok(!buffer[0], "got \"%s\"\n", buffer);
    ok(sz == 0, "got size %lu\n", sz);

    rec = MsiCreateRecord(2);
    MsiRecordSetInteger(rec, 1, 1);
    MsiRecordSetInteger(rec, 2, 2);
    r = MsiViewModify(view, MSIMODIFY_VALIDATE_NEW, rec);
    ok(r == ERROR_INVALID_DATA, "got %u\n", r);

    sz = 2;
    strcpy(buffer, "x");
    err = MsiViewGetErrorA(view, buffer, &sz);
    ok(err == MSIDBERROR_DUPLICATEKEY, "got %d\n", err);
    ok(!strcmp(buffer, "A"), "got \"%s\"\n", buffer);
    ok(sz == 1, "got size %lu\n", sz);

    sz = 2;
    strcpy(buffer, "x");
    err = MsiViewGetErrorA(view, buffer, &sz);
    todo_wine ok(err == MSIDBERROR_NOERROR, "got %d\n", err);
    todo_wine ok(!buffer[0], "got \"%s\"\n", buffer);
    todo_wine ok(sz == 0, "got size %lu\n", sz);

    r = MsiViewModify(view, MSIMODIFY_VALIDATE_NEW, rec);
    ok(r == ERROR_INVALID_DATA, "got %u\n", r);

    sz = 1;
    strcpy(buffer, "x");
    err = MsiViewGetErrorA(view, buffer, &sz);
    ok(err == MSIDBERROR_MOREDATA, "got %d\n", err);
    ok(!buffer[0], "got \"%s\"\n", buffer);
    ok(sz == 1, "got size %lu\n", sz);

    sz = 1;
    strcpy(buffer, "x");
    err = MsiViewGetErrorA(view, buffer, &sz);
    todo_wine ok(err == MSIDBERROR_NOERROR, "got %d\n", err);
    ok(!buffer[0], "got \"%s\"\n", buffer);
    todo_wine ok(sz == 0, "got size %lu\n", sz);

    r = MsiViewModify(view, MSIMODIFY_VALIDATE_NEW, rec);
    ok(r == ERROR_INVALID_DATA, "got %u\n", r);

    sz = 0;
    strcpy(buffer, "x");
    err = MsiViewGetErrorA(view, buffer, &sz);
    ok(err == MSIDBERROR_MOREDATA, "got %d\n", err);
    ok(!strcmp(buffer, "x"), "got \"%s\"\n", buffer);
    ok(sz == 1, "got size %lu\n", sz);

    sz = 0;
    strcpy(buffer, "x");
    err = MsiViewGetErrorA(view, buffer, &sz);
    ok(err == MSIDBERROR_MOREDATA, "got %d\n", err);
    ok(!strcmp(buffer, "x"), "got \"%s\"\n", buffer);
    todo_wine ok(sz == 0, "got size %lu\n", sz);

    MsiCloseHandle(rec);
    MsiCloseHandle(view);
    MsiCloseHandle(db);
    DeleteFileA(msifile);
}

static void test_viewfetch_wraparound(void)
{
    MSIHANDLE db = 0, view = 0, rec = 0;
    UINT r, i, idset, tries;
    const char *query;

    DeleteFileA(msifile);

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabaseW( msifileW, MSIDBOPEN_CREATE, &db );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );

    query = "CREATE TABLE `phone` ( "
            "`id` INT, `name` CHAR(32), `number` CHAR(32) "
            "PRIMARY KEY `id`)";
    r = run_query( db, 0, query );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );

    query = "INSERT INTO `phone` ( `id`, `name`, `number` )"
        "VALUES('1', 'Alan', '5030581')";
    r = run_query( db, 0, query );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );

    query = "INSERT INTO `phone` ( `id`, `name`, `number` )"
        "VALUES('2', 'Barry', '928440')";
    r = run_query( db, 0, query );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );

    query = "INSERT INTO `phone` ( `id`, `name`, `number` )"
        "VALUES('3', 'Cindy', '2937550')";
    r = run_query( db, 0, query );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );

    query = "SELECT * FROM `phone`";
    r = MsiDatabaseOpenViewA( db, query, &view );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );

    r = MsiViewExecute( view, 0 );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );

    for (tries = 0; tries < 3; tries++)
    {
        winetest_push_context( "Wraparound attempt #%d", tries );
        idset = 0;

        for (i = 0; i < 3; i++)
        {
            winetest_push_context( "Record #%d", i );

            r = MsiViewFetch( view, &rec );
            ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
            if (r != ERROR_SUCCESS)
            {
                winetest_pop_context();
                break;
            }

            r = MsiRecordGetInteger(rec, 1);
            ok(r >= 1 && r <= 3, "Expected 1 <= id <= 3, got %d\n", r);
            if (r < sizeof(idset) * 8)
            {
                ok(!(idset & (1 << r)), "Duplicate id %d\n", r);
                idset |= 1 << r;
            }

            MsiCloseHandle(rec);

            winetest_pop_context();
        }

        r = MsiViewFetch(view, &rec);
        ok(r == ERROR_NO_MORE_ITEMS, "Expected ERROR_NO_MORE_ITEMS, got %d\n", r);

        winetest_pop_context();
    }

    MsiViewClose(view);
    MsiCloseHandle(view);
    MsiCloseHandle(db);
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
    test_binary();
    test_where_not_in_selected();
    test_where();
    test_msiimport();
    test_binary_import();
    test_markers();
    test_handle_limit();
    test_try_transform();
    test_join();
    test_temporary_table();
    test_alter();
    test_integers();
    test_update();
    test_special_tables();
    test_tables_order();
    test_rows_order();
    test_select_markers();
    test_viewmodify_update();
    test_viewmodify_assign();
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
    test_select_with_tablenames();
    test_insertorder();
    test_columnorder();
    test_suminfo_import();
    test_createtable();
    test_collation();
    test_embedded_nulls();
    test_select_column_names();
    test_primary_keys();
    test_viewmodify_merge();
    test_viewmodify_insert();
    test_view_get_error();
    test_viewfetch_wraparound();
}
