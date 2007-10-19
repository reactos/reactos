/*
 * tests for Microsoft Installer functionality
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
 * Copyright 2005 Aric Stewart for CodeWeavers
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

#include "wine/test.h"

static const char msifile[] = "winetest.msi";

static UINT run_query( MSIHANDLE hdb, const char *query )
{
    MSIHANDLE hview = 0;
    UINT r;

    r = MsiDatabaseOpenView(hdb, query, &hview);
    if( r != ERROR_SUCCESS )
        return r;

    r = MsiViewExecute(hview, 0);
    if( r == ERROR_SUCCESS )
        r = MsiViewClose(hview);
    MsiCloseHandle(hview);
    return r;
}

static UINT create_component_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `Component` ( "
            "`Component` CHAR(72) NOT NULL, "
            "`ComponentId` CHAR(38), "
            "`Directory_` CHAR(72) NOT NULL, "
            "`Attributes` SHORT NOT NULL, "
            "`Condition` CHAR(255), "
            "`KeyPath` CHAR(72) "
            "PRIMARY KEY `Component`)" );
}

static UINT create_feature_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `Feature` ( "
            "`Feature` CHAR(38) NOT NULL, "
            "`Feature_Parent` CHAR(38), "
            "`Title` CHAR(64), "
            "`Description` CHAR(255), "
            "`Display` SHORT NOT NULL, "
            "`Level` SHORT NOT NULL, "
            "`Directory_` CHAR(72), "
            "`Attributes` SHORT NOT NULL "
            "PRIMARY KEY `Feature`)" );
}

static UINT create_feature_components_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `FeatureComponents` ( "
            "`Feature_` CHAR(38) NOT NULL, "
            "`Component_` CHAR(72) NOT NULL "
            "PRIMARY KEY `Feature_`, `Component_` )" );
}

static UINT create_file_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `File` ("
            "`File` CHAR(72) NOT NULL, "
            "`Component_` CHAR(72) NOT NULL, "
            "`FileName` CHAR(255) NOT NULL, "
            "`FileSize` LONG NOT NULL, "
            "`Version` CHAR(72), "
            "`Language` CHAR(20), "
            "`Attributes` SHORT, "
            "`Sequence` SHORT NOT NULL "
            "PRIMARY KEY `File`)" );
}

static UINT create_remove_file_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `RemoveFile` ("
            "`FileKey` CHAR(72) NOT NULL, "
            "`Component_` CHAR(72) NOT NULL, "
            "`FileName` CHAR(255) LOCALIZABLE, "
            "`DirProperty` CHAR(72) NOT NULL, "
            "`InstallMode` SHORT NOT NULL "
            "PRIMARY KEY `FileKey`)" );
}

static UINT create_appsearch_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `AppSearch` ("
            "`Property` CHAR(72) NOT NULL, "
            "`Signature_` CHAR(72) NOT NULL "
            "PRIMARY KEY `Property`, `Signature_`)" );
}

static UINT create_reglocator_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `RegLocator` ("
            "`Signature_` CHAR(72) NOT NULL, "
            "`Root` SHORT NOT NULL, "
            "`Key` CHAR(255) NOT NULL, "
            "`Name` CHAR(255), "
            "`Type` SHORT "
            "PRIMARY KEY `Signature_`)" );
}

static UINT create_signature_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `Signature` ("
            "`Signature` CHAR(72) NOT NULL, "
            "`FileName` CHAR(255) NOT NULL, "
            "`MinVersion` CHAR(20), "
            "`MaxVersion` CHAR(20), "
            "`MinSize` LONG, "
            "`MaxSize` LONG, "
            "`MinDate` LONG, "
            "`MaxDate` LONG, "
            "`Languages` CHAR(255) "
            "PRIMARY KEY `Signature`)" );
}

static UINT add_component_entry( MSIHANDLE hdb, const char *values )
{
    char insert[] = "INSERT INTO `Component`  "
            "(`Component`, `ComponentId`, `Directory_`, `Attributes`, `Condition`, `KeyPath`) "
            "VALUES( %s )";
    char *query;
    UINT sz, r;

    sz = strlen(values) + sizeof insert;
    query = HeapAlloc(GetProcessHeap(),0,sz);
    sprintf(query,insert,values);
    r = run_query( hdb, query );
    HeapFree(GetProcessHeap(), 0, query);
    return r;
}

static UINT add_feature_entry( MSIHANDLE hdb, const char *values )
{
    char insert[] = "INSERT INTO `Feature` (`Feature`, `Feature_Parent`, "
                    "`Title`, `Description`, `Display`, `Level`, `Directory_`, `Attributes`) VALUES( %s )";
    char *query;
    UINT sz, r;

    sz = strlen(values) + sizeof insert;
    query = HeapAlloc(GetProcessHeap(),0,sz);
    sprintf(query,insert,values);
    r = run_query( hdb, query );
    HeapFree(GetProcessHeap(), 0, query);
    return r;
}

static UINT add_feature_components_entry( MSIHANDLE hdb, const char *values )
{
    char insert[] = "INSERT INTO `FeatureComponents` "
            "(`Feature_`, `Component_`) "
            "VALUES( %s )";
    char *query;
    UINT sz, r;

    sz = strlen(values) + sizeof insert;
    query = HeapAlloc(GetProcessHeap(),0,sz);
    sprintf(query,insert,values);
    r = run_query( hdb, query );
    HeapFree(GetProcessHeap(), 0, query);
    return r;
}

static UINT add_file_entry( MSIHANDLE hdb, const char *values )
{
    char insert[] = "INSERT INTO `File` "
            "(`File`, `Component_`, `FileName`, `FileSize`, `Version`, `Language`, `Attributes`, `Sequence`) "
            "VALUES( %s )";
    char *query;
    UINT sz, r;

    sz = strlen(values) + sizeof insert;
    query = HeapAlloc(GetProcessHeap(),0,sz);
    sprintf(query,insert,values);
    r = run_query( hdb, query );
    HeapFree(GetProcessHeap(), 0, query);
    return r;
}

static UINT add_appsearch_entry( MSIHANDLE hdb, const char *values )
{
    char insert[] = "INSERT INTO `AppSearch` "
            "(`Property`, `Signature_`) "
            "VALUES( %s )";
    char *query;
    UINT sz, r;

    sz = strlen(values) + sizeof insert;
    query = HeapAlloc(GetProcessHeap(),0,sz);
    sprintf(query,insert,values);
    r = run_query( hdb, query );
    HeapFree(GetProcessHeap(), 0, query);
    return r;
}

static UINT add_reglocator_entry( MSIHANDLE hdb, const char *values )
{
    char insert[] = "INSERT INTO `RegLocator` "
            "(`Signature_`, `Root`, `Key`, `Name`, `Type`) "
            "VALUES( %s )";
    char *query;
    UINT sz, r;

    sz = strlen(values) + sizeof insert;
    query = HeapAlloc(GetProcessHeap(),0,sz);
    sprintf(query,insert,values);
    r = run_query( hdb, query );
    HeapFree(GetProcessHeap(), 0, query);
    return r;
}

static UINT add_signature_entry( MSIHANDLE hdb, const char *values )
{
    char insert[] = "INSERT INTO `Signature` "
            "(`Signature`, `FileName`, `MinVersion`, `MaxVersion`,"
            " `MinSize`, `MaxSize`, `MinDate`, `MaxDate`, `Languages`) "
            "VALUES( %s )";
    char *query;
    UINT sz, r;

    sz = strlen(values) + sizeof insert;
    query = HeapAlloc(GetProcessHeap(),0,sz);
    sprintf(query,insert,values);
    r = run_query( hdb, query );
    HeapFree(GetProcessHeap(), 0, query);
    return r;
}

static UINT set_summary_info(MSIHANDLE hdb)
{
    UINT res;
    MSIHANDLE suminfo;

    /* build summmary info */
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


MSIHANDLE create_package_db(void)
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

    res = set_summary_info(hdb);

    res = run_query( hdb,
            "CREATE TABLE `Directory` ( "
            "`Directory` CHAR(255) NOT NULL, "
            "`Directory_Parent` CHAR(255), "
            "`DefaultDir` CHAR(255) NOT NULL "
            "PRIMARY KEY `Directory`)" );
    ok( res == ERROR_SUCCESS , "Failed to create directory table\n" );

    return hdb;
}

MSIHANDLE package_from_db(MSIHANDLE hdb)
{
    UINT res;
    CHAR szPackage[10];
    MSIHANDLE hPackage;

    sprintf(szPackage,"#%li",hdb);
    res = MsiOpenPackage(szPackage,&hPackage);
    ok( res == ERROR_SUCCESS , "Failed to open package\n" );

    res = MsiCloseHandle(hdb);
    ok( res == ERROR_SUCCESS , "Failed to close db handle\n" );

    return hPackage;
}

static void create_test_file(const CHAR *name)
{
    HANDLE file;
    DWORD written;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failure to open file %s\n", name);
    WriteFile(file, name, strlen(name), &written, NULL);
    WriteFile(file, "\n", strlen("\n"), &written, NULL);
    CloseHandle(file);
}

static void test_createpackage(void)
{
    MSIHANDLE hPackage = 0;
    UINT res;

    hPackage = package_from_db(create_package_db());
    ok( hPackage != 0, " Failed to create package\n");

    res = MsiCloseHandle( hPackage);
    ok( res == ERROR_SUCCESS , "Failed to close package\n" );
    DeleteFile(msifile);
}

static void test_getsourcepath_bad( void )
{
    static const char str[] = { 0 };
    char buffer[0x80];
    DWORD sz;
    UINT r;

    r = MsiGetSourcePath( -1, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "return value wrong\n");

    sz = 0;
    r = MsiGetSourcePath( -1, NULL, buffer, &sz );
    ok( r == ERROR_INVALID_PARAMETER, "return value wrong\n");

    sz = 0;
    r = MsiGetSourcePath( -1, str, NULL, &sz );
    ok( r == ERROR_INVALID_HANDLE, "return value wrong\n");

    sz = 0;
    r = MsiGetSourcePath( -1, str, NULL, NULL );
    ok( r == ERROR_INVALID_HANDLE, "return value wrong\n");

    sz = 0;
    r = MsiGetSourcePath( -1, str, buffer, &sz );
    ok( r == ERROR_INVALID_HANDLE, "return value wrong\n");
}

static UINT add_directory_entry( MSIHANDLE hdb, const char *values )
{
    char insert[] = "INSERT INTO `Directory` (`Directory`,`Directory_Parent`,`DefaultDir`) VALUES( %s )";
    char *query;
    UINT sz, r;

    sz = strlen(values) + sizeof insert;
    query = HeapAlloc(GetProcessHeap(),0,sz);
    sprintf(query,insert,values);
    r = run_query( hdb, query );
    HeapFree(GetProcessHeap(), 0, query);
    return r;
}

static void test_getsourcepath( void )
{
    static const char str[] = { 0 };
    char buffer[0x80];
    DWORD sz;
    UINT r;
    MSIHANDLE hpkg, hdb;

    hpkg = package_from_db(create_package_db());
    ok( hpkg, "failed to create package\n");

    sz = 0;
    buffer[0] = 'x';
    r = MsiGetSourcePath( hpkg, str, buffer, &sz );
    ok( r == ERROR_DIRECTORY, "return value wrong\n");
    ok( buffer[0] == 'x', "buffer modified\n");

    sz = 1;
    buffer[0] = 'x';
    r = MsiGetSourcePath( hpkg, str, buffer, &sz );
    ok( r == ERROR_DIRECTORY, "return value wrong\n");
    ok( buffer[0] == 'x', "buffer modified\n");

    MsiCloseHandle( hpkg );


    /* another test but try create a directory this time */
    hdb = create_package_db();
    ok( hdb, "failed to create database\n");

    r = add_directory_entry( hdb, "'TARGETDIR', '', 'SourceDir'");
    ok( r == S_OK, "failed\n");

    hpkg = package_from_db(hdb);
    ok( hpkg, "failed to create package\n");

    sz = sizeof buffer -1;
    strcpy(buffer,"x bad");
    r = MsiGetSourcePath( hpkg, "TARGETDIR", buffer, &sz );
    ok( r == ERROR_DIRECTORY, "return value wrong\n");

    r = MsiDoAction( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");
    r = MsiDoAction( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed\n");

    todo_wine {
    sz = sizeof buffer -1;
    buffer[0] = 'x';
    r = MsiGetSourcePath( hpkg, "TARGETDIR", buffer, &sz );
    ok( r == ERROR_SUCCESS, "return value wrong\n");
    ok( sz == strlen(buffer), "returned length wrong\n");

    sz = 0;
    strcpy(buffer,"x bad");
    r = MsiGetSourcePath( hpkg, "TARGETDIR", buffer, &sz );
    ok( r == ERROR_MORE_DATA, "return value wrong\n");
    }
    ok( buffer[0] == 'x', "buffer modified\n");

    todo_wine {
    r = MsiGetSourcePath( hpkg, "TARGETDIR", NULL, NULL );
    ok( r == ERROR_SUCCESS, "return value wrong\n");
    }

    r = MsiGetSourcePath( hpkg, "TARGETDIR ", NULL, NULL );
    ok( r == ERROR_DIRECTORY, "return value wrong\n");

    r = MsiGetSourcePath( hpkg, "targetdir", NULL, NULL );
    ok( r == ERROR_DIRECTORY, "return value wrong\n");

    r = MsiGetSourcePath( hpkg, "TARGETDIR", buffer, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "return value wrong\n");

    todo_wine {
    r = MsiGetSourcePath( hpkg, "TARGETDIR", NULL, &sz );
    ok( r == ERROR_SUCCESS, "return value wrong\n");
    }

    MsiCloseHandle( hpkg );
    DeleteFile(msifile);
}

static void test_doaction( void )
{
    MSIHANDLE hpkg;
    UINT r;

    r = MsiDoAction( -1, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    hpkg = package_from_db(create_package_db());
    ok( hpkg, "failed to create package\n");

    r = MsiDoAction(hpkg, NULL);
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiDoAction(0, "boo");
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiDoAction(hpkg, "boo");
    ok( r == ERROR_FUNCTION_NOT_CALLED, "wrong return val\n");

    MsiCloseHandle( hpkg );
    DeleteFile(msifile);
}

static void test_gettargetpath_bad(void)
{
    char buffer[0x80];
    MSIHANDLE hpkg;
    DWORD sz;
    UINT r;

    hpkg = package_from_db(create_package_db());
    ok( hpkg, "failed to create package\n");

    r = MsiGetTargetPath( 0, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiGetTargetPath( 0, NULL, NULL, &sz );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiGetTargetPath( 0, "boo", NULL, NULL );
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiGetTargetPath( 0, "boo", NULL, NULL );
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiGetTargetPath( hpkg, "boo", NULL, NULL );
    ok( r == ERROR_DIRECTORY, "wrong return val\n");

    r = MsiGetTargetPath( hpkg, "boo", buffer, NULL );
    ok( r == ERROR_DIRECTORY, "wrong return val\n");

    MsiCloseHandle( hpkg );
    DeleteFile(msifile);
}

static void query_file_path(MSIHANDLE hpkg, LPCSTR file, LPSTR buff)
{
    UINT r;
    DWORD size;
    MSIHANDLE rec;

    rec = MsiCreateRecord( 1 );
    ok(rec, "MsiCreate record failed\n");

    r = MsiRecordSetString( rec, 0, file );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r );

    size = MAX_PATH;
    r = MsiFormatRecord( hpkg, rec, buff, &size );
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r );

    MsiCloseHandle( rec );
}

static void test_settargetpath(void)
{
    char tempdir[MAX_PATH+8], buffer[MAX_PATH];
    DWORD sz;
    MSIHANDLE hpkg;
    UINT r;
    MSIHANDLE hdb;

    hdb = create_package_db();
    ok ( hdb, "failed to create package database\n" );

    r = add_directory_entry( hdb, "'TARGETDIR', '', 'SourceDir'" );
    ok( r == S_OK, "failed to add directory entry: %d\n" , r );

    r = run_query( hdb, /* these tables required by Windows Installer for MsiSetTargetPath */
            "CREATE TABLE `Component` ( "
            "`Component` CHAR(72) NOT NULL, "
            "`ComponentId` CHAR(38), "
            "`Directory_` CHAR(72) NOT NULL, "
            "`Attributes` SHORT NOT NULL, "
            "`Condition` CHAR(255), "
            "`KeyPath` CHAR(72) "
            "PRIMARY KEY `Component`)" );
    ok( r == S_OK, "cannot create Component table: %d\n", r );

    r = run_query( hdb,
            "INSERT INTO `Component`  "
            "(`Component`, `ComponentId`, `Directory_`, `Attributes`, `Condition`, `KeyPath`) "
            "VALUES( 'WinWorkAround', '{83e2694d-0864-4124-9323-6d37630912a1}', 'TARGETDIR', 8, '', 'FL_dummycomponent')" );
    ok( r == S_OK, "cannot add dummy component: %d\n", r );

    r = run_query( hdb,
            "INSERT INTO `Component`  "
            "(`Component`, `ComponentId`, `Directory_`, `Attributes`, `Condition`, `KeyPath`) "
            "VALUES( 'TestComp', '{A3FB59C8-C293-4F7E-B8C5-F0E1D8EEE4E5}', 'TestDir', 0, '', 'TestFile')" );
    ok( r == S_OK, "cannot add test component: %d\n", r );

    r = run_query( hdb,
            "CREATE TABLE `Feature` ( "
            "`Feature` CHAR(38) NOT NULL, "
            "`Feature_Parent` CHAR(38), "
            "`Title` CHAR(64), "
            "`Description` CHAR(255), "
            "`Display` SHORT NOT NULL, "
            "`Level` SHORT NOT NULL, "
            "`Directory_` CHAR(72), "
            "`Attributes` SHORT NOT NULL "
            "PRIMARY KEY `Feature`)" );
    ok( r == S_OK, "cannot create Feature table: %d\n", r );

    r = run_query( hdb,
            "INSERT INTO `Feature` "
            "(`Feature`, `Feature_Parent`, `Display`, `Level`, `Attributes`) "
            "VALUES( 'TestFeature', '', 0, 1, 0 )" );
    ok( r == ERROR_SUCCESS, "cannot add TestFeature to Feature table: %d\n", r );

    r = run_query( hdb,
            "CREATE TABLE `FeatureComponents` ( "
            "`Feature_` CHAR(38) NOT NULL, "
            "`Component_` CHAR(72) NOT NULL "
            "PRIMARY KEY `Feature_` )" );
    ok( r == S_OK, "cannot create FeatureComponents table: %d\n", r );

    r = run_query( hdb,
            "INSERT INTO `FeatureComponents` "
            "(`Feature_`, `Component_`) "
            "VALUES( 'TestFeature', 'TestComp' )" );
    ok( r == S_OK, "cannot insert component into FeatureComponents table: %d\n", r );

    add_directory_entry( hdb, "'TestParent', 'TARGETDIR', 'TestParent'" );
    add_directory_entry( hdb, "'TestDir', 'TestParent', 'TestDir'" );

    r = run_query( hdb,
            "CREATE TABLE `File` ("
            "`File` CHAR(72) NOT NULL, "
            "`Component_` CHAR(72) NOT NULL, "
            "`FileName` CHAR(255) NOT NULL, "
            "`FileSize` LONG NOT NULL, "
            "`Version` CHAR(72), "
            "`Language` CHAR(20), "
            "`Attributes` SHORT, "
            "`Sequence` SHORT NOT NULL "
            "PRIMARY KEY `File`)" );
    ok( r == S_OK, "cannot create File table: %d\n", r );

    r = run_query( hdb,
            "INSERT INTO `File` "
            "(`File`, `Component_`, `FileName`, `FileSize`, `Version`, `Language`, `Attributes`, `Sequence`) "
            "VALUES( 'TestFile', 'TestComp', 'testfile.txt', 0, '', '1033', 8192, 1 )" );
    ok( r == S_OK, "cannot add file to the File table: %d\n", r );

    hpkg = package_from_db( hdb );
    ok( hpkg, "failed to create package\n");

    r = MsiDoAction( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    r = MsiDoAction( hpkg, "FileCost");
    ok( r == ERROR_SUCCESS, "file cost failed\n");

    r = MsiDoAction( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed\n");

    r = MsiSetTargetPath( 0, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiSetTargetPath( 0, "boo", "C:\\bogusx" );
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiSetTargetPath( hpkg, "boo", NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiSetTargetPath( hpkg, "boo", "c:\\bogusx" );
    ok( r == ERROR_DIRECTORY, "wrong return val\n");

    sz = sizeof tempdir - 1;
    r = MsiGetTargetPath( hpkg, "TARGETDIR", tempdir, &sz );
    if ( r == S_OK )
    {
        if ( GetTempFileName( tempdir, "_wt", 0, buffer ) )
        {
            sprintf( tempdir, "%s\\subdir", buffer );
            r = MsiSetTargetPath( hpkg, "TARGETDIR", buffer );
            ok( r == ERROR_SUCCESS, "MsiSetTargetPath on file returned %d\n", r );

            r = MsiSetTargetPath( hpkg, "TARGETDIR", tempdir );
            ok( r == ERROR_SUCCESS, "MsiSetTargetPath on 'subdir' of file returned %d\n", r );

            DeleteFile( buffer );

            r = MsiSetTargetPath( hpkg, "TARGETDIR", buffer );
            ok( r == ERROR_SUCCESS, "MsiSetTargetPath returned %d\n", r );

            r = GetFileAttributes( buffer );
            ok ( r == INVALID_FILE_ATTRIBUTES, "file/directory exists after MsiSetTargetPath. Attributes: %08X\n", r );

            r = MsiSetTargetPath( hpkg, "TARGETDIR", tempdir );
            ok( r == ERROR_SUCCESS, "MsiSetTargetPath on subsubdir returned %d\n", r );
        } else {
            trace("GetTempFileName failed, cannot do some tests\n");
        }
    } else {
        trace( "MsiGetTargetPath failed: %d\n", r );
    }

    r = MsiSetTargetPath( hpkg, "TestParent", "C:\\one\\two" );
    ok( r == ERROR_SUCCESS, "MsiSetTargetPath returned %d\n", r );

    query_file_path( hpkg, "[#TestFile]", buffer );
    ok( !lstrcmp(buffer, "C:\\one\\two\\TestDir\\testfile.txt"),
        "Expected C:\\one\\two\\TestDir\\testfile.txt, got %s\n", buffer );

    MsiCloseHandle( hpkg );
}

static void test_condition(void)
{
    MSICONDITION r;
    MSIHANDLE hpkg;

    hpkg = package_from_db(create_package_db());
    ok( hpkg, "failed to create package\n");

    r = MsiEvaluateCondition(0, NULL);
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, NULL);
    ok( r == MSICONDITION_NONE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "");
    ok( r == MSICONDITION_NONE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 = 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 <> 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 = 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 > 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 ~> 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 > 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 ~> 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 >= 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 ~>= 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 >= 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 ~>= 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 < 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 ~< 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 < 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 ~< 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 <= 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 ~<= 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 <= 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 ~<= 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 >=");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " ");
    ok( r == MSICONDITION_NONE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "LicView <> \"1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "LicView <> \"0\"");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "LicView <> LicView");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "not 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "not LicView");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "not \"A\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "~not \"A\"");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "\"0\"");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 and 2");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "not 0 and 3");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "not 0 and 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "not 0 or 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "(0)");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "(((((1))))))");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "(((((1)))))");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"A\" < \"B\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"A\" > \"B\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"1\" > \"12\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"100\" < \"21\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 < > 0");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "(1<<1) == 2");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"A\" = \"a\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"A\" ~ = \"a\" ");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"A\" ~= \"a\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"A\" ~= 1 ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " \"A\" = 1 ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " 1 ~= 1 ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " 1 ~= \"1\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " 1 = \"1\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " 0 = \"1\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " 0 < \"100\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, " 100 > \"0\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 XOR 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 IMP 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 IMP 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 IMP 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 EQV 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 EQV 1");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 IMP 1 OR 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 IMPL 1");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "\"ASFD\" >< \"S\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "\"ASFD\" ~>< \"s\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "\"ASFD\" ~>< \"\" ");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "\"ASFD\" ~>< \"sss\" ");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "mm", "5" );

    r = MsiEvaluateCondition(hpkg, "mm = 5");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "mm < 6");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "mm <= 5");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "mm > 4");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "mm < 12");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "mm = \"5\"");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 = \"\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 AND \"\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 AND \"\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "1 AND \"1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "3 >< 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "3 >< 4");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "NOT 0 AND 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "NOT 0 AND 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "NOT 1 OR 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 AND 1 OR 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "0 AND 0 OR 1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "NOT 0 AND 1 OR 0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "_1 = _1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "( 1 AND 1 ) = 2");
    ok( r == MSICONDITION_ERROR, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "NOT ( 1 AND 1 )");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "NOT A AND (BBBBBBBBBB=2 OR CCC=1) AND Ddddddddd");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "Installed<>\"\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "NOT 1 AND 0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "bandalmael<>0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "bandalmael<0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "bandalmael>0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "bandalmael>=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "bandalmael<=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    r = MsiEvaluateCondition(hpkg, "bandalmael~<>0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "0" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "asdf" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "0asdf" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "0 " );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "-0" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "0000000000000" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "--0" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "0x00" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "-" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "+0" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "bandalmael", "0.0" );
    r = MsiEvaluateCondition(hpkg, "bandalmael=0");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");
    r = MsiEvaluateCondition(hpkg, "bandalmael<>0");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hi");
    MsiSetProperty(hpkg, "two", "hithere");
    r = MsiEvaluateCondition(hpkg, "one >< two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hithere");
    MsiSetProperty(hpkg, "two", "hi");
    r = MsiEvaluateCondition(hpkg, "one >< two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hello");
    MsiSetProperty(hpkg, "two", "hi");
    r = MsiEvaluateCondition(hpkg, "one >< two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hellohithere");
    MsiSetProperty(hpkg, "two", "hi");
    r = MsiEvaluateCondition(hpkg, "one >< two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "");
    MsiSetProperty(hpkg, "two", "hi");
    r = MsiEvaluateCondition(hpkg, "one >< two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hi");
    MsiSetProperty(hpkg, "two", "");
    r = MsiEvaluateCondition(hpkg, "one >< two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "");
    MsiSetProperty(hpkg, "two", "");
    r = MsiEvaluateCondition(hpkg, "one >< two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "1234");
    MsiSetProperty(hpkg, "two", "1");
    r = MsiEvaluateCondition(hpkg, "one >< two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "one 1234");
    MsiSetProperty(hpkg, "two", "1");
    r = MsiEvaluateCondition(hpkg, "one >< two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hithere");
    MsiSetProperty(hpkg, "two", "hi");
    r = MsiEvaluateCondition(hpkg, "one << two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hi");
    MsiSetProperty(hpkg, "two", "hithere");
    r = MsiEvaluateCondition(hpkg, "one << two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hi");
    MsiSetProperty(hpkg, "two", "hi");
    r = MsiEvaluateCondition(hpkg, "one << two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "abcdhithere");
    MsiSetProperty(hpkg, "two", "hi");
    r = MsiEvaluateCondition(hpkg, "one << two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "");
    MsiSetProperty(hpkg, "two", "hi");
    r = MsiEvaluateCondition(hpkg, "one << two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hithere");
    MsiSetProperty(hpkg, "two", "");
    r = MsiEvaluateCondition(hpkg, "one << two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "");
    MsiSetProperty(hpkg, "two", "");
    r = MsiEvaluateCondition(hpkg, "one << two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "1234");
    MsiSetProperty(hpkg, "two", "1");
    r = MsiEvaluateCondition(hpkg, "one << two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "1234 one");
    MsiSetProperty(hpkg, "two", "1");
    r = MsiEvaluateCondition(hpkg, "one << two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hithere");
    MsiSetProperty(hpkg, "two", "there");
    r = MsiEvaluateCondition(hpkg, "one >> two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "hithere");
    MsiSetProperty(hpkg, "two", "hi");
    r = MsiEvaluateCondition(hpkg, "one >> two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "there");
    MsiSetProperty(hpkg, "two", "hithere");
    r = MsiEvaluateCondition(hpkg, "one >> two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "there");
    MsiSetProperty(hpkg, "two", "there");
    r = MsiEvaluateCondition(hpkg, "one >> two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "abcdhithere");
    MsiSetProperty(hpkg, "two", "hi");
    r = MsiEvaluateCondition(hpkg, "one >> two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "");
    MsiSetProperty(hpkg, "two", "there");
    r = MsiEvaluateCondition(hpkg, "one >> two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "there");
    MsiSetProperty(hpkg, "two", "");
    r = MsiEvaluateCondition(hpkg, "one >> two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "");
    MsiSetProperty(hpkg, "two", "");
    r = MsiEvaluateCondition(hpkg, "one >> two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "1234");
    MsiSetProperty(hpkg, "two", "4");
    r = MsiEvaluateCondition(hpkg, "one >> two");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "one", "one 1234");
    MsiSetProperty(hpkg, "two", "4");
    r = MsiEvaluateCondition(hpkg, "one >> two");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

    MsiCloseHandle( hpkg );
    DeleteFile(msifile);
}

static BOOL check_prop_empty( MSIHANDLE hpkg, const char * prop)
{
    UINT r;
    DWORD sz;
    char buffer[2];

    sz = sizeof buffer;
    strcpy(buffer,"x");
    r = MsiGetProperty( hpkg, prop, buffer, &sz );
    return r == ERROR_SUCCESS && buffer[0] == 0 && sz == 0;
}

static void test_props(void)
{
    MSIHANDLE hpkg;
    UINT r;
    DWORD sz;
    char buffer[0x100];

    hpkg = package_from_db(create_package_db());
    ok( hpkg, "failed to create package\n");

    /* test invalid values */
    r = MsiGetProperty( 0, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiGetProperty( hpkg, NULL, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiGetProperty( hpkg, "boo", NULL, NULL );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    r = MsiGetProperty( hpkg, "boo", buffer, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    /* test retrieving an empty/nonexistent property */
    sz = sizeof buffer;
    r = MsiGetProperty( hpkg, "boo", NULL, &sz );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( sz == 0, "wrong size returned\n");

    check_prop_empty( hpkg, "boo");
    sz = 0;
    strcpy(buffer,"x");
    r = MsiGetProperty( hpkg, "boo", buffer, &sz );
    ok( r == ERROR_MORE_DATA, "wrong return val\n");
    ok( !strcmp(buffer,"x"), "buffer was changed\n");
    ok( sz == 0, "wrong size returned\n");

    sz = 1;
    strcpy(buffer,"x");
    r = MsiGetProperty( hpkg, "boo", buffer, &sz );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( buffer[0] == 0, "buffer was not changed\n");
    ok( sz == 0, "wrong size returned\n");

    /* set the property to something */
    r = MsiSetProperty( 0, NULL, NULL );
    ok( r == ERROR_INVALID_HANDLE, "wrong return val\n");

    r = MsiSetProperty( hpkg, NULL, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "wrong return val\n");

    r = MsiSetProperty( hpkg, "", NULL );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    /* try set and get some illegal property identifiers */
    r = MsiSetProperty( hpkg, "", "asdf" );
    ok( r == ERROR_FUNCTION_FAILED, "wrong return val\n");

    r = MsiSetProperty( hpkg, "=", "asdf" );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    r = MsiSetProperty( hpkg, " ", "asdf" );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    r = MsiSetProperty( hpkg, "'", "asdf" );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    sz = sizeof buffer;
    buffer[0]=0;
    r = MsiGetProperty( hpkg, "'", buffer, &sz );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( !strcmp(buffer,"asdf"), "buffer was not changed\n");

    /* set empty values */
    r = MsiSetProperty( hpkg, "boo", NULL );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( check_prop_empty( hpkg, "boo"), "prop wasn't empty\n");

    r = MsiSetProperty( hpkg, "boo", "" );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( check_prop_empty( hpkg, "boo"), "prop wasn't empty\n");

    /* set a non-empty value */
    r = MsiSetProperty( hpkg, "boo", "xyz" );
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    sz = 1;
    strcpy(buffer,"x");
    r = MsiGetProperty( hpkg, "boo", buffer, &sz );
    ok( r == ERROR_MORE_DATA, "wrong return val\n");
    ok( buffer[0] == 0, "buffer was not changed\n");
    ok( sz == 3, "wrong size returned\n");

    sz = 4;
    strcpy(buffer,"x");
    r = MsiGetProperty( hpkg, "boo", buffer, &sz );
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( !strcmp(buffer,"xyz"), "buffer was not changed\n");
    ok( sz == 3, "wrong size returned\n");

    sz = 3;
    strcpy(buffer,"x");
    r = MsiGetProperty( hpkg, "boo", buffer, &sz );
    ok( r == ERROR_MORE_DATA, "wrong return val\n");
    ok( !strcmp(buffer,"xy"), "buffer was not changed\n");
    ok( sz == 3, "wrong size returned\n");

    MsiCloseHandle( hpkg );
    DeleteFile(msifile);
}

static UINT try_query_param( MSIHANDLE hdb, LPCSTR szQuery, MSIHANDLE hrec )
{
    MSIHANDLE htab = 0;
    UINT res;

    res = MsiDatabaseOpenView( hdb, szQuery, &htab );
    if( res == ERROR_SUCCESS )
    {
        UINT r;

        r = MsiViewExecute( htab, hrec );
        if( r != ERROR_SUCCESS )
        {
            res = r;
            fprintf(stderr,"MsiViewExecute failed %08x\n", res);
        }

        r = MsiViewClose( htab );
        if( r != ERROR_SUCCESS )
            res = r;

        r = MsiCloseHandle( htab );
        if( r != ERROR_SUCCESS )
            res = r;
    }
    return res;
}

static UINT try_query( MSIHANDLE hdb, LPCSTR szQuery )
{
    return try_query_param( hdb, szQuery, 0 );
}

static void test_msipackage(void)
{
    MSIHANDLE hdb = 0, hpack = 100;
    UINT r;
    const char *query;
    char name[10];

    DeleteFile(msifile);

    todo_wine {
    name[0] = 0;
    r = MsiOpenPackage(name, &hpack);
    ok(r == ERROR_SUCCESS, "failed to open package with no name\n");
    r = MsiCloseHandle(hpack);
    ok(r == ERROR_SUCCESS, "failed to close package\n");
    }

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    name[0]='#';
    name[1]=0;
    r = MsiOpenPackage(name, &hpack);
    ok(r == ERROR_INVALID_HANDLE, "MsiOpenPackage returned wrong code\n");

    todo_wine {
    /* now try again with our empty database */
    sprintf(name, "#%ld", hdb);
    r = MsiOpenPackage(name, &hpack);
    ok(r == ERROR_INSTALL_PACKAGE_INVALID, "MsiOpenPackage returned wrong code\n");
    if (!r)    MsiCloseHandle(hpack);
    }

    /* create a table */
    query = "CREATE TABLE `Property` ( "
            "`Property` CHAR(72), `Value` CHAR(0) "
            "PRIMARY KEY `Property`)";
    r = try_query(hdb, query);
    ok(r == ERROR_SUCCESS, "failed to create Properties table\n");

    todo_wine {
    query = "CREATE TABLE `InstallExecuteSequence` ("
            "`Action` CHAR(72), `Condition` CHAR(0), `Sequence` INTEGER "
            "PRIMARY KEY `Action`)";
    r = try_query(hdb, query);
    ok(r == ERROR_SUCCESS, "failed to create InstallExecuteSequence table\n");

    sprintf(name, "#%ld", hdb);
    r = MsiOpenPackage(name, &hpack);
    ok(r == ERROR_INSTALL_PACKAGE_INVALID, "MsiOpenPackage returned wrong code\n");
    if (!r)    MsiCloseHandle(hpack);
    }

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle(database) failed\n");
    DeleteFile(msifile);
}

static void test_formatrecord2(void)
{
    MSIHANDLE hpkg, hrec ;
    char buffer[0x100];
    DWORD sz;
    UINT r;

    hpkg = package_from_db(create_package_db());
    ok( hpkg, "failed to create package\n");

    r = MsiSetProperty(hpkg, "Manufacturer", " " );
    ok( r == ERROR_SUCCESS, "set property failed\n");

    hrec = MsiCreateRecord(2);
    ok(hrec, "create record failed\n");

    r = MsiRecordSetString( hrec, 0, "[ProgramFilesFolder][Manufacturer]\\asdf");
    ok( r == ERROR_SUCCESS, "format record failed\n");

    buffer[0] = 0;
    sz = sizeof buffer;
    r = MsiFormatRecord( hpkg, hrec, buffer, &sz );

    r = MsiRecordSetString(hrec, 0, "[foo][1]");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(hpkg, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"hoo"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "x[~]x");
    sz = sizeof buffer;
    r = MsiFormatRecord(hpkg, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"x"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[foo.$%}][1]");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(hpkg, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"hoo"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[\\[]");
    sz = sizeof buffer;
    r = MsiFormatRecord(hpkg, hrec, buffer, &sz);
    ok( sz == 1, "size wrong\n");
    ok( 0 == strcmp(buffer,"["), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    SetEnvironmentVariable("FOO", "BAR");
    r = MsiRecordSetString(hrec, 0, "[%FOO]");
    sz = sizeof buffer;
    r = MsiFormatRecord(hpkg, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"BAR"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[[1]]");
    r = MsiRecordSetString(hrec, 1, "%FOO");
    sz = sizeof buffer;
    r = MsiFormatRecord(hpkg, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"BAR"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    MsiCloseHandle( hrec );
    MsiCloseHandle( hpkg );
    DeleteFile(msifile);
}

static void test_states(void)
{
    MSIHANDLE hpkg;
    UINT r;
    MSIHANDLE hdb;
    INSTALLSTATE state, action;

    hdb = create_package_db();
    ok ( hdb, "failed to create package database\n" );

    r = add_directory_entry( hdb, "'TARGETDIR', '', 'SourceDir'");
    ok( r == ERROR_SUCCESS, "cannot add directory: %d\n", r );

    r = create_feature_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Feature table: %d\n", r );

    r = create_component_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Component table: %d\n", r );

    /* msidbFeatureAttributesFavorLocal */
    r = add_feature_entry( hdb, "'one', '', '', '', 2, 1, '', 0" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:msidbComponentAttributesLocalOnly */
    r = add_component_entry( hdb, "'alpha', '{467EC132-739D-4784-A37B-677AA43DBC94}', 'TARGETDIR', 0, '', 'alpha_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:msidbComponentAttributesSourceOnly */
    r = add_component_entry( hdb, "'beta', '{2C1F189C-24A6-4C34-B26B-994A6C026506}', 'TARGETDIR', 1, '', 'beta_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:msidbComponentAttributesOptional */
    r = add_component_entry( hdb, "'gamma', '{C271E2A4-DE2E-4F70-86D1-6984AF7DE2CA}', 'TARGETDIR', 2, '', 'gamma_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:msidbComponentAttributesSharedDllRefCount */
    r = add_component_entry( hdb, "'theta', '{4EB3129D-81A8-48D5-9801-75600FED3DD9}', 'TARGETDIR', 8, '', 'theta_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource */
    r = add_feature_entry( hdb, "'two', '', '', '', 2, 1, '', 1" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* msidbFeatureAttributesFavorSource:msidbComponentAttributesLocalOnly */
    r = add_component_entry( hdb, "'delta', '{938FD4F2-C648-4259-A03C-7AA3B45643F3}', 'TARGETDIR', 0, '', 'delta_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource:msidbComponentAttributesSourceOnly */
    r = add_component_entry( hdb, "'epsilon', '{D59713B6-C11D-47F2-A395-1E5321781190}', 'TARGETDIR', 1, '', 'epsilon_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource:msidbComponentAttributesOptional */
    r = add_component_entry( hdb, "'zeta', '{377D33AB-2FAA-42B9-A629-0C0DAE9B9C7A}', 'TARGETDIR', 2, '', 'zeta_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource:msidbComponentAttributesSharedDllRefCount */
    r = add_component_entry( hdb, "'iota', '{5D36F871-B5ED-4801-9E0F-C46B9E5C9669}', 'TARGETDIR', 8, '', 'iota_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource */
    r = add_feature_entry( hdb, "'three', '', '', '', 2, 1, '', 1" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* msidbFeatureAttributesFavorSource:msidbComponentAttributesSourceOnly */
    r = add_component_entry( hdb, "'eta', '{DD89003F-0DD4-41B8-81C0-3411A7DA2695}', 'TARGETDIR', 1, '', 'eta_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = create_feature_components_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create FeatureComponents table: %d\n", r );

    r = add_feature_components_entry( hdb, "'one', 'alpha'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'one', 'beta'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'one', 'gamma'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'one', 'theta'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'two', 'delta'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'two', 'epsilon'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'two', 'zeta'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'two', 'iota'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'three', 'eta'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = create_file_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create File table: %d\n", r );

    r = add_file_entry( hdb, "'alpha_file', 'alpha', 'alpha.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'beta_file', 'beta', 'beta.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'gamma_file', 'gamma', 'gamma.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'theta_file', 'theta', 'theta.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'delta_file', 'delta', 'delta.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'epsilon_file', 'epsilon', 'epsilon.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'zeta_file', 'zeta', 'zeta.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'iota_file', 'iota', 'iota.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    /* compressed file */
    r = add_file_entry( hdb, "'eta_file', 'eta', 'eta.txt', 0, '', '1033', 16384, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    hpkg = package_from_db( hdb );
    ok( hpkg, "failed to create package\n");

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    r = MsiDoAction( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    }
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    }
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    }
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    r = MsiDoAction( hpkg, "FileCost");
    ok( r == ERROR_SUCCESS, "file cost failed\n");

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    }
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    }
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    }
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    r = MsiDoAction( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed: %d\n", r);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    MsiCloseHandle( hpkg );
}

static void test_getproperty(void)
{
    MSIHANDLE hPackage = 0;
    char prop[100];
    static CHAR empty[] = "";
    DWORD size;
    UINT r;

    hPackage = package_from_db(create_package_db());
    ok( hPackage != 0, " Failed to create package\n");

    /* set the property */
    r = MsiSetProperty(hPackage, "Name", "Value");
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* retrieve the size, NULL pointer */
    size = 0;
    r = MsiGetProperty(hPackage, "Name", NULL, &size);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok( size == 5, "Expected 5, got %ld\n", size);

    /* retrieve the size, empty string */
    size = 0;
    r = MsiGetProperty(hPackage, "Name", empty, &size);
    ok( r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok( size == 5, "Expected 5, got %ld\n", size);

    /* don't change size */
    r = MsiGetProperty(hPackage, "Name", prop, &size);
    ok( r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok( size == 5, "Expected 5, got %ld\n", size);
    ok( !lstrcmp(prop, "Valu"), "Expected Valu, got %s\n", prop);

    /* increase the size by 1 */
    size++;
    r = MsiGetProperty(hPackage, "Name", prop, &size);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok( size == 5, "Expected 5, got %ld\n", size);
    ok( !lstrcmp(prop, "Value"), "Expected Value, got %s\n", prop);

    r = MsiCloseHandle( hPackage);
    ok( r == ERROR_SUCCESS , "Failed to close package\n" );
    DeleteFile(msifile);
}

static void test_removefiles(void)
{
    MSIHANDLE hpkg;
    UINT r;
    MSIHANDLE hdb;
    char CURR_DIR[MAX_PATH];

    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);

    hdb = create_package_db();
    ok ( hdb, "failed to create package database\n" );

    r = add_directory_entry( hdb, "'TARGETDIR', '', 'SourceDir'");
    ok( r == ERROR_SUCCESS, "cannot add directory: %d\n", r );

    r = create_feature_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Feature table: %d\n", r );

    r = create_component_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Component table: %d\n", r );

    r = add_feature_entry( hdb, "'one', '', '', '', 2, 1, '', 0" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    r = add_component_entry( hdb, "'hydrogen', '', 'TARGETDIR', 0, '', 'hydrogen_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = add_component_entry( hdb, "'helium', '', 'TARGETDIR', 0, '', 'helium_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = add_component_entry( hdb, "'lithium', '', 'TARGETDIR', 0, '', 'lithium_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = add_component_entry( hdb, "'beryllium', '', 'TARGETDIR', 0, '', 'beryllium_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = add_component_entry( hdb, "'boron', '', 'TARGETDIR', 0, '', 'boron_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = add_component_entry( hdb, "'carbon', '', 'TARGETDIR', 0, '', 'carbon_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    r = create_feature_components_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create FeatureComponents table: %d\n", r );

    r = add_feature_components_entry( hdb, "'one', 'hydrogen'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'one', 'helium'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'one', 'lithium'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'one', 'beryllium'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'one', 'boron'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'one', 'carbon'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = create_file_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create File table: %d\n", r );

    r = add_file_entry( hdb, "'hydrogen_file', 'hydrogen', 'hydrogen.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'helium_file', 'helium', 'helium.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'lithium_file', 'lithium', 'lithium.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'beryllium_file', 'beryllium', 'beryllium.txt', 0, '', '1033', 16384, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'boron_file', 'boron', 'boron.txt', 0, '', '1033', 16384, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'carbon_file', 'carbon', 'carbon.txt', 0, '', '1033', 16384, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = create_remove_file_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Remove File table: %d\n", r);

    hpkg = package_from_db( hdb );
    ok( hpkg, "failed to create package\n");

    create_test_file( "hydrogen.txt" );
    create_test_file( "helium.txt" );
    create_test_file( "lithium.txt" );
    create_test_file( "beryllium.txt" );
    create_test_file( "boron.txt" );
    create_test_file( "carbon.txt" );

    r = MsiSetProperty( hpkg, "TARGETDIR", CURR_DIR );
    ok( r == ERROR_SUCCESS, "set property failed\n");

    r = MsiDoAction( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    r = MsiDoAction( hpkg, "FileCost");
    ok( r == ERROR_SUCCESS, "cost finalize failed\n");

    r = MsiDoAction( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed\n");

    r = MsiDoAction( hpkg, "InstallValidate");
    ok( r == ERROR_SUCCESS, "cost finalize failed\n");

    r = MsiSetComponentState( hpkg, "hydrogen", INSTALLSTATE_ABSENT );
    ok( r == ERROR_SUCCESS, "failed to set component state: %d\n", r);

    r = MsiSetComponentState( hpkg, "helium", INSTALLSTATE_LOCAL );
    ok( r == ERROR_SUCCESS, "failed to set component state: %d\n", r);

    r = MsiSetComponentState( hpkg, "lithium", INSTALLSTATE_SOURCE );
    ok( r == ERROR_SUCCESS, "failed to set component state: %d\n", r);

    r = MsiSetComponentState( hpkg, "beryllium", INSTALLSTATE_ABSENT );
    ok( r == ERROR_SUCCESS, "failed to set component state: %d\n", r);

    r = MsiSetComponentState( hpkg, "boron", INSTALLSTATE_LOCAL );
    ok( r == ERROR_SUCCESS, "failed to set component state: %d\n", r);

    r = MsiSetComponentState( hpkg, "carbon", INSTALLSTATE_SOURCE );
    ok( r == ERROR_SUCCESS, "failed to set component state: %d\n", r);

    r = MsiDoAction( hpkg, "RemoveFiles");
    ok( r == ERROR_SUCCESS, "remove files failed\n");

    ok(DeleteFileA("hydrogen.txt"), "Expected hydrogen.txt to exist\n");
    ok(DeleteFileA("lithium.txt"), "Expected lithium.txt to exist\n");
    ok(DeleteFileA("beryllium.txt"), "Expected beryllium.txt to exist\n");
    ok(DeleteFileA("carbon.txt"), "Expected carbon.txt to exist\n");
    ok(DeleteFileA("helium.txt"), "Expected helium.txt to exist\n");
    ok(DeleteFileA("boron.txt"), "Expected boron.txt to exist\n");

    MsiCloseHandle( hpkg );
    DeleteFileA(msifile);
}

static void test_appsearch(void)
{
    MSIHANDLE hpkg;
    UINT r;
    MSIHANDLE hdb;
    CHAR prop[MAX_PATH];
    DWORD size = MAX_PATH;

    hdb = create_package_db();
    ok ( hdb, "failed to create package database\n" );

    r = create_appsearch_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create AppSearch table: %d\n", r );

    r = add_appsearch_entry( hdb, "'WEBBROWSERPROG', 'NewSignature1'" );
    ok( r == ERROR_SUCCESS, "cannot add entry: %d\n", r );

    r = create_reglocator_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create RegLocator table: %d\n", r );

    r = add_reglocator_entry( hdb, "'NewSignature1', 0, 'htmlfile\\shell\\open\\command', '', 1" );
    ok( r == ERROR_SUCCESS, "cannot create RegLocator table: %d\n", r );

    r = create_signature_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Signature table: %d\n", r );

    r = add_signature_entry( hdb, "'NewSignature1', 'FileName', '', '', '', '', '', '', ''" );
    ok( r == ERROR_SUCCESS, "cannot create Signature table: %d\n", r );

    hpkg = package_from_db( hdb );
    ok( hpkg, "failed to create package\n");

    r = MsiDoAction( hpkg, "AppSearch" );
    ok( r == ERROR_SUCCESS, "AppSearch failed: %d\n", r);

    r = MsiGetPropertyA( hpkg, "WEBBROWSERPROG", prop, &size );
    ok( r == ERROR_SUCCESS, "get property failed: %d\n", r);
    todo_wine
    {
        ok( lstrlenA(prop) != 0, "Expected non-zero length\n");
    }

    MsiCloseHandle( hpkg );
    DeleteFileA(msifile);
}

START_TEST(package)
{
    test_createpackage();
    test_getsourcepath_bad();
    test_getsourcepath();
    test_doaction();
    test_gettargetpath_bad();
    test_settargetpath();
    test_props();
    test_condition();
    test_msipackage();
    test_formatrecord2();
    test_states();
    test_getproperty();
    test_removefiles();
    test_appsearch();
}
