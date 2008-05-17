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
#include <msidefs.h>
#include <msi.h>
#include <msiquery.h>

#include "wine/test.h"

static const char msifile[] = "winetest.msi";
char CURR_DIR[MAX_PATH];

/* RegDeleteTreeW from dlls/advapi32/registry.c */
LSTATUS WINAPI package_RegDeleteTreeW(HKEY hKey, LPCWSTR lpszSubKey)
{
    LONG ret;
    DWORD dwMaxSubkeyLen, dwMaxValueLen;
    DWORD dwMaxLen, dwSize;
    WCHAR szNameBuf[MAX_PATH], *lpszName = szNameBuf;
    HKEY hSubKey = hKey;

    if(lpszSubKey)
    {
        ret = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
        if (ret) return ret;
    }

    ret = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, NULL,
            &dwMaxSubkeyLen, NULL, NULL, &dwMaxValueLen, NULL, NULL, NULL);
    if (ret) goto cleanup;

    dwMaxSubkeyLen++;
    dwMaxValueLen++;
    dwMaxLen = max(dwMaxSubkeyLen, dwMaxValueLen);
    if (dwMaxLen > sizeof(szNameBuf)/sizeof(WCHAR))
    {
        /* Name too big: alloc a buffer for it */
        if (!(lpszName = HeapAlloc( GetProcessHeap(), 0, dwMaxLen*sizeof(WCHAR))))
        {
            ret = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    /* Recursively delete all the subkeys */
    while (TRUE)
    {
        dwSize = dwMaxLen;
        if (RegEnumKeyExW(hSubKey, 0, lpszName, &dwSize, NULL,
                          NULL, NULL, NULL)) break;

        ret = package_RegDeleteTreeW(hSubKey, lpszName);
        if (ret) goto cleanup;
    }

    if (lpszSubKey)
        ret = RegDeleteKeyW(hKey, lpszSubKey);
    else
        while (TRUE)
        {
            dwSize = dwMaxLen;
            if (RegEnumValueW(hKey, 0, lpszName, &dwSize,
                  NULL, NULL, NULL, NULL)) break;

            ret = RegDeleteValueW(hKey, lpszName);
            if (ret) goto cleanup;
        }

cleanup:
    if (lpszName != szNameBuf)
        HeapFree(GetProcessHeap(), 0, lpszName);
    if(lpszSubKey)
        RegCloseKey(hSubKey);
    return ret;
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

static UINT create_launchcondition_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `LaunchCondition` ("
            "`Condition` CHAR(255) NOT NULL, "
            "`Description` CHAR(255) NOT NULL "
            "PRIMARY KEY `Condition`)" );
}

static UINT create_property_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `Property` ("
            "`Property` CHAR(72) NOT NULL, "
            "`Value` CHAR(0) "
            "PRIMARY KEY `Property`)" );
}

static UINT create_install_execute_sequence_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `InstallExecuteSequence` ("
            "`Action` CHAR(72) NOT NULL, "
            "`Condition` CHAR(255), "
            "`Sequence` SHORT "
            "PRIMARY KEY `Action`)" );
}

static UINT create_media_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `Media` ("
            "`DiskId` SHORT NOT NULL, "
            "`LastSequence` SHORT NOT NULL, "
            "`DiskPrompt` CHAR(64), "
            "`Cabinet` CHAR(255), "
            "`VolumeLabel` CHAR(32), "
            "`Source` CHAR(72) "
            "PRIMARY KEY `DiskId`)" );
}

static UINT create_ccpsearch_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `CCPSearch` ("
            "`Signature_` CHAR(72) NOT NULL "
            "PRIMARY KEY `Signature_`)" );
}

static UINT create_drlocator_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `DrLocator` ("
            "`Signature_` CHAR(72) NOT NULL, "
            "`Parent` CHAR(72), "
            "`Path` CHAR(255), "
            "`Depth` SHORT "
            "PRIMARY KEY `Signature_`, `Parent`, `Path`)" );
}

static UINT create_complocator_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `CompLocator` ("
            "`Signature_` CHAR(72) NOT NULL, "
            "`ComponentId` CHAR(38) NOT NULL, "
            "`Type` SHORT "
            "PRIMARY KEY `Signature_`)" );
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

static UINT add_launchcondition_entry( MSIHANDLE hdb, const char *values )
{
    char insert[] = "INSERT INTO `LaunchCondition` "
            "(`Condition`, `Description`) "
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

static UINT add_property_entry( MSIHANDLE hdb, const char *values )
{
    char insert[] = "INSERT INTO `Property` "
            "(`Property`, `Value`) "
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

static UINT add_install_execute_sequence_entry( MSIHANDLE hdb, const char *values )
{
    char insert[] = "INSERT INTO `InstallExecuteSequence` "
            "(`Action`, `Condition`, `Sequence`) "
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

static UINT add_media_entry( MSIHANDLE hdb, const char *values )
{
    char insert[] = "INSERT INTO `Media` "
            "(`DiskId`, `LastSequence`, `DiskPrompt`, `Cabinet`, `VolumeLabel`, `Source`) "
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

static UINT add_ccpsearch_entry( MSIHANDLE hdb, const char *values )
{
    char insert[] = "INSERT INTO `CCPSearch` (`Signature_`) VALUES( %s )";
    char *query;
    UINT sz, r;

    sz = strlen(values) + sizeof insert;
    query = HeapAlloc(GetProcessHeap(),0,sz);
    sprintf(query,insert,values);
    r = run_query( hdb, query );
    HeapFree(GetProcessHeap(), 0, query);
    return r;
}

static UINT add_drlocator_entry( MSIHANDLE hdb, const char *values )
{
    char insert[] = "INSERT INTO `DrLocator` "
            "(`Signature_`, `Parent`, `Path`, `Depth`) "
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

static UINT add_complocator_entry( MSIHANDLE hdb, const char *values )
{
    char insert[] = "INSERT INTO `CompLocator` "
            "(`Signature_`, `ComponentId`, `Type`) "
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


static MSIHANDLE create_package_db(void)
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

static MSIHANDLE package_from_db(MSIHANDLE hdb)
{
    UINT res;
    CHAR szPackage[10];
    MSIHANDLE hPackage;

    sprintf(szPackage,"#%li",hdb);
    res = MsiOpenPackage(szPackage,&hPackage);
    if (res != ERROR_SUCCESS)
        return 0;

    res = MsiCloseHandle(hdb);
    if (res != ERROR_SUCCESS)
        return 0;

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

    sz = sizeof buffer -1;
    buffer[0] = 'x';
    r = MsiGetSourcePath( hpkg, "TARGETDIR", buffer, &sz );
    ok( r == ERROR_SUCCESS, "return value wrong\n");
    ok( sz == strlen(buffer), "returned length wrong\n");

    sz = 0;
    strcpy(buffer,"x bad");
    r = MsiGetSourcePath( hpkg, "TARGETDIR", buffer, &sz );
    ok( r == ERROR_MORE_DATA, "return value wrong\n");
    ok( buffer[0] == 'x', "buffer modified\n");

    r = MsiGetSourcePath( hpkg, "TARGETDIR", NULL, NULL );
    ok( r == ERROR_SUCCESS, "return value wrong\n");

    r = MsiGetSourcePath( hpkg, "TARGETDIR ", NULL, NULL );
    ok( r == ERROR_DIRECTORY, "return value wrong\n");

    r = MsiGetSourcePath( hpkg, "targetdir", NULL, NULL );
    ok( r == ERROR_DIRECTORY, "return value wrong\n");

    r = MsiGetSourcePath( hpkg, "TARGETDIR", buffer, NULL );
    ok( r == ERROR_INVALID_PARAMETER, "return value wrong\n");

    r = MsiGetSourcePath( hpkg, "TARGETDIR", NULL, &sz );
    ok( r == ERROR_SUCCESS, "return value wrong\n");

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
    char tempdir[MAX_PATH+8], buffer[MAX_PATH], file[MAX_PATH];
    DWORD sz;
    MSIHANDLE hpkg;
    UINT r;
    MSIHANDLE hdb;

    hdb = create_package_db();
    ok ( hdb, "failed to create package database\n" );

    r = add_directory_entry( hdb, "'TARGETDIR', '', 'SourceDir'" );
    ok( r == S_OK, "failed to add directory entry: %d\n" , r );

    r = create_component_table( hdb );
    ok( r == S_OK, "cannot create Component table: %d\n", r );

    r = add_component_entry( hdb, "'RootComp', '{83e2694d-0864-4124-9323-6d37630912a1}', 'TARGETDIR', 8, '', 'RootFile'" );
    ok( r == S_OK, "cannot add dummy component: %d\n", r );

    r = add_component_entry( hdb, "'TestComp', '{A3FB59C8-C293-4F7E-B8C5-F0E1D8EEE4E5}', 'TestDir', 0, '', 'TestFile'" );
    ok( r == S_OK, "cannot add test component: %d\n", r );

    r = create_feature_table( hdb );
    ok( r == S_OK, "cannot create Feature table: %d\n", r );

    r = add_feature_entry( hdb, "'TestFeature', '', '', '', 0, 1, '', 0" );
    ok( r == ERROR_SUCCESS, "cannot add TestFeature to Feature table: %d\n", r );

    r = create_feature_components_table( hdb );
    ok( r == S_OK, "cannot create FeatureComponents table: %d\n", r );

    r = add_feature_components_entry( hdb, "'TestFeature', 'RootComp'" );
    ok( r == S_OK, "cannot insert component into FeatureComponents table: %d\n", r );

    r = add_feature_components_entry( hdb, "'TestFeature', 'TestComp'" );
    ok( r == S_OK, "cannot insert component into FeatureComponents table: %d\n", r );

    add_directory_entry( hdb, "'TestParent', 'TARGETDIR', 'TestParent'" );
    add_directory_entry( hdb, "'TestDir', 'TestParent', 'TestDir'" );

    r = create_file_table( hdb );
    ok( r == S_OK, "cannot create File table: %d\n", r );

    r = add_file_entry( hdb, "'RootFile', 'RootComp', 'rootfile.txt', 0, '', '1033', 8192, 1" );
    ok( r == S_OK, "cannot add file to the File table: %d\n", r );

    r = add_file_entry( hdb, "'TestFile', 'TestComp', 'testfile.txt', 0, '', '1033', 8192, 1" );
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
    sprintf( file, "%srootfile.txt", tempdir );
    query_file_path( hpkg, "[#RootFile]", buffer );
    ok( r == ERROR_SUCCESS, "failed to get target path: %d\n", r);
    ok( !lstrcmp(buffer, file), "Expected %s, got %s\n", file, buffer );

    GetTempFileName( tempdir, "_wt", 0, buffer );
    sprintf( tempdir, "%s\\subdir", buffer );

    r = MsiSetTargetPath( hpkg, "TARGETDIR", buffer );
    ok( r == ERROR_SUCCESS || r == ERROR_DIRECTORY,
        "MsiSetTargetPath on file returned %d\n", r );

    r = MsiSetTargetPath( hpkg, "TARGETDIR", tempdir );
    ok( r == ERROR_SUCCESS || r == ERROR_DIRECTORY,
        "MsiSetTargetPath on 'subdir' of file returned %d\n", r );

    DeleteFile( buffer );

    r = MsiSetTargetPath( hpkg, "TARGETDIR", buffer );
    ok( r == ERROR_SUCCESS, "MsiSetTargetPath returned %d\n", r );

    r = GetFileAttributes( buffer );
    ok ( r == INVALID_FILE_ATTRIBUTES, "file/directory exists after MsiSetTargetPath. Attributes: %08X\n", r );

    r = MsiSetTargetPath( hpkg, "TARGETDIR", tempdir );
    ok( r == ERROR_SUCCESS, "MsiSetTargetPath on subsubdir returned %d\n", r );

    sz = sizeof buffer - 1;
    lstrcat( tempdir, "\\" );
    r = MsiGetTargetPath( hpkg, "TARGETDIR", buffer, &sz );
    ok( r == ERROR_SUCCESS, "failed to get target path: %d\n", r);
    ok( !lstrcmp(buffer, tempdir), "Expected %s, got %s\n", tempdir, buffer);

    sprintf( file, "%srootfile.txt", tempdir );
    query_file_path( hpkg, "[#RootFile]", buffer );
    ok( !lstrcmp(buffer, file), "Expected %s, got %s\n", file, buffer);

    r = MsiSetTargetPath( hpkg, "TestParent", "C:\\one\\two" );
    ok( r == ERROR_SUCCESS, "MsiSetTargetPath returned %d\n", r );

    query_file_path( hpkg, "[#TestFile]", buffer );
    ok( !lstrcmp(buffer, "C:\\one\\two\\TestDir\\testfile.txt"),
        "Expected C:\\one\\two\\TestDir\\testfile.txt, got %s\n", buffer );

    sz = sizeof buffer - 1;
    r = MsiGetTargetPath( hpkg, "TestParent", buffer, &sz );
    ok( r == ERROR_SUCCESS, "failed to get target path: %d\n", r);
    ok( !lstrcmp(buffer, "C:\\one\\two\\"), "Expected C:\\one\\two\\, got %s\n", buffer);

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

    r = MsiEvaluateCondition(hpkg, "-1");
    ok( r == MSICONDITION_TRUE, "wrong return val\n");

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

    MsiSetProperty(hpkg, "MsiNetAssemblySupport", NULL);  /* make sure it's empty */

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1.1.4322\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport > \"1.1.4322\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport >= \"1.1.4322\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport <= \"1.1.4322\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport <> \"1.1.4322\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport ~< \"1.1.4322\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"abcd\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"a1.1.4322\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1.1.4322a\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"0000001.1.4322\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1.1.4322.1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1.1.4322.1.1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "\"2\" < \"1.1");
    ok( r == MSICONDITION_ERROR, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "\"2\" < \"1.1\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "\"2\" < \"12.1\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "\"02.1\" < \"2.11\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "\"02.1.1\" < \"2.1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1.1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"0\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"-1\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"a\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"!\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"!\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"/\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \" \"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"azAZ_\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"a[a]\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"a[a]a\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"[a]\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"[a]a\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"{a}\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"{a\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"[a\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"a{\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"a]\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"A\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    MsiSetProperty(hpkg, "MsiNetAssemblySupport", "1.1.4322");
    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1.1.4322\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1.1.14322\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1.1.5\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1.1\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "MsiNetAssemblySupport < \"1\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    MsiSetProperty(hpkg, "one", "1");
    r = MsiEvaluateCondition(hpkg, "one < \"1\"");
    ok( r == MSICONDITION_FALSE, "wrong return val\n");

    MsiSetProperty(hpkg, "X", "5.0");

    r = MsiEvaluateCondition(hpkg, "X != \"\"");
    ok( r == MSICONDITION_ERROR, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "X =\"5.0\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "X =\"5.1\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "X =\"6.0\"");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "X =\"5.0\" or X =\"5.1\" or X =\"6.0\"");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "(X =\"5.0\" or X =\"5.1\" or X =\"6.0\")");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "X !=\"\" and (X =\"5.0\" or X =\"5.1\" or X =\"6.0\")");
    ok( r == MSICONDITION_ERROR, "wrong return val (%d)\n", r);

    /* feature doesn't exist */
    r = MsiEvaluateCondition(hpkg, "&nofeature");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    MsiSetProperty(hpkg, "A", "2");
    MsiSetProperty(hpkg, "X", "50");

    r = MsiEvaluateCondition(hpkg, "2 <= X");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "A <= X");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "A <= 50");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    MsiSetProperty(hpkg, "X", "50val");

    r = MsiEvaluateCondition(hpkg, "2 <= X");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "A <= X");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    MsiSetProperty(hpkg, "A", "7");
    MsiSetProperty(hpkg, "X", "50");

    r = MsiEvaluateCondition(hpkg, "7 <= X");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "A <= X");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "A <= 50");
    ok( r == MSICONDITION_TRUE, "wrong return val (%d)\n", r);

    MsiSetProperty(hpkg, "X", "50val");

    r = MsiEvaluateCondition(hpkg, "2 <= X");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

    r = MsiEvaluateCondition(hpkg, "A <= X");
    ok( r == MSICONDITION_FALSE, "wrong return val (%d)\n", r);

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
    MSIHANDLE hpkg, hdb;
    UINT r;
    DWORD sz;
    char buffer[0x100];

    hdb = create_package_db();
    r = run_query( hdb,
            "CREATE TABLE `Property` ( "
            "`Property` CHAR(255) NOT NULL, "
            "`Value` CHAR(255) "
            "PRIMARY KEY `Property`)" );
    ok( r == ERROR_SUCCESS , "Failed\n" );

    r = run_query(hdb,
            "INSERT INTO `Property` "
            "(`Property`, `Value`) "
            "VALUES( 'MetadataCompName', 'Photoshop.dll' )");
    ok( r == ERROR_SUCCESS , "Failed\n" );

    hpkg = package_from_db( hdb );
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

    r = MsiSetProperty(hpkg, "SourceDir", "foo");
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    sz = 4;
    r = MsiGetProperty(hpkg, "SOURCEDIR", buffer, &sz);
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( !strcmp(buffer,""), "buffer wrong\n");
    ok( sz == 0, "wrong size returned\n");

    sz = 4;
    r = MsiGetProperty(hpkg, "SOMERANDOMNAME", buffer, &sz);
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( !strcmp(buffer,""), "buffer wrong\n");
    ok( sz == 0, "wrong size returned\n");

    sz = 4;
    r = MsiGetProperty(hpkg, "SourceDir", buffer, &sz);
    ok( r == ERROR_SUCCESS, "wrong return val\n");
    ok( !strcmp(buffer,"foo"), "buffer wrong\n");
    ok( sz == 3, "wrong size returned\n");

    r = MsiSetProperty(hpkg, "MetadataCompName", "Photoshop.dll");
    ok( r == ERROR_SUCCESS, "wrong return val\n");

    sz = 0;
    r = MsiGetProperty(hpkg, "MetadataCompName", NULL, &sz );
    ok( r == ERROR_SUCCESS, "return wrong\n");
    ok( sz == 13, "size wrong (%d)\n", sz);

    sz = 13;
    r = MsiGetProperty(hpkg, "MetadataCompName", buffer, &sz );
    ok( r == ERROR_MORE_DATA, "return wrong\n");
    ok( !strcmp(buffer,"Photoshop.dl"), "buffer wrong\n");

    r = MsiSetProperty(hpkg, "property", "value");
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = 6;
    r = MsiGetProperty(hpkg, "property", buffer, &sz);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok( !strcmp(buffer, "value"), "Expected value, got %s\n", buffer);

    r = MsiSetProperty(hpkg, "property", NULL);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sz = 6;
    r = MsiGetProperty(hpkg, "property", buffer, &sz);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok( !strlen(buffer), "Expected empty string, got %s\n", buffer);

    MsiCloseHandle( hpkg );
    DeleteFile(msifile);
}

static BOOL find_prop_in_property(MSIHANDLE hdb, LPCSTR prop, LPCSTR val)
{
    MSIHANDLE hview, hrec;
    BOOL found;
    CHAR buffer[MAX_PATH];
    DWORD sz;
    UINT r;

    r = MsiDatabaseOpenView(hdb, "SELECT * FROM `_Property`", &hview);
    ok(r == ERROR_SUCCESS, "MsiDatabaseOpenView failed\n");
    r = MsiViewExecute(hview, 0);
    ok(r == ERROR_SUCCESS, "MsiViewExecute failed\n");

    found = FALSE;
    while (r == ERROR_SUCCESS && !found)
    {
        r = MsiViewFetch(hview, &hrec);
        if (r != ERROR_SUCCESS) break;

        sz = MAX_PATH;
        r = MsiRecordGetString(hrec, 1, buffer, &sz);
        if (r == ERROR_SUCCESS && !lstrcmpA(buffer, prop))
        {
            sz = MAX_PATH;
            r = MsiRecordGetString(hrec, 2, buffer, &sz);
            if (r == ERROR_SUCCESS && !lstrcmpA(buffer, val))
                found = TRUE;
        }

        MsiCloseHandle(hrec);
    }

    MsiViewClose(hview);
    MsiCloseHandle(hview);

    return found;
}

static void test_property_table(void)
{
    const char *query;
    UINT r;
    MSIHANDLE hpkg, hdb, hrec;
    char buffer[MAX_PATH];
    DWORD sz;
    BOOL found;

    hdb = create_package_db();
    ok( hdb, "failed to create package\n");

    hpkg = package_from_db(hdb);
    ok( hpkg, "failed to create package\n");

    MsiCloseHandle(hdb);

    hdb = MsiGetActiveDatabase(hpkg);

    query = "CREATE TABLE `_Property` ( "
        "`foo` INT NOT NULL, `bar` INT LOCALIZABLE PRIMARY KEY `foo`)";
    r = run_query(hdb, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    MsiCloseHandle(hdb);
    MsiCloseHandle(hpkg);
    DeleteFile(msifile);

    hdb = create_package_db();
    ok( hdb, "failed to create package\n");

    query = "CREATE TABLE `_Property` ( "
        "`foo` INT NOT NULL, `bar` INT LOCALIZABLE PRIMARY KEY `foo`)";
    r = run_query(hdb, query);
    ok(r == ERROR_SUCCESS, "failed to create table\n");

    query = "ALTER `_Property` ADD `foo` INTEGER";
    r = run_query(hdb, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "failed to add column\n");

    query = "ALTER TABLE `_Property` ADD `foo` INTEGER";
    r = run_query(hdb, query);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "failed to add column\n");

    query = "ALTER TABLE `_Property` ADD `extra` INTEGER";
    r = run_query(hdb, query);
    ok(r == ERROR_SUCCESS, "failed to add column\n");

    hpkg = package_from_db(hdb);
    todo_wine
    {
        ok(!hpkg, "package should not be created\n");
    }

    MsiCloseHandle(hdb);
    MsiCloseHandle(hpkg);
    DeleteFile(msifile);

    hdb = create_package_db();
    ok (hdb, "failed to create package database\n");

    r = create_property_table(hdb);
    ok(r == ERROR_SUCCESS, "cannot create Property table: %d\n", r);

    r = add_property_entry(hdb, "'prop', 'val'");
    ok(r == ERROR_SUCCESS, "cannot add property: %d\n", r);

    hpkg = package_from_db(hdb);
    ok(hpkg, "failed to create package\n");

    MsiCloseHandle(hdb);

    sz = MAX_PATH;
    r = MsiGetProperty(hpkg, "prop", buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmp(buffer, "val"), "Expected val, got %s\n", buffer);

    hdb = MsiGetActiveDatabase(hpkg);

    found = find_prop_in_property(hdb, "prop", "val");
    ok(found, "prop should be in the _Property table\n");

    r = add_property_entry(hdb, "'dantes', 'mercedes'");
    ok(r == ERROR_SUCCESS, "cannot add property: %d\n", r);

    query = "SELECT * FROM `_Property` WHERE `Property` = 'dantes'";
    r = do_query(hdb, query, &hrec);
    ok(r == ERROR_BAD_QUERY_SYNTAX, "Expected ERROR_BAD_QUERY_SYNTAX, got %d\n", r);

    found = find_prop_in_property(hdb, "dantes", "mercedes");
    ok(found == FALSE, "dantes should not be in the _Property table\n");

    sz = MAX_PATH;
    lstrcpy(buffer, "aaa");
    r = MsiGetProperty(hpkg, "dantes", buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(lstrlenA(buffer) == 0, "Expected empty string, got %s\n", buffer);

    r = MsiSetProperty(hpkg, "dantes", "mercedes");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    found = find_prop_in_property(hdb, "dantes", "mercedes");
    ok(found == TRUE, "dantes should be in the _Property table\n");

    MsiCloseHandle(hdb);
    MsiCloseHandle(hpkg);
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

static void set_summary_str(MSIHANDLE hdb, DWORD pid, LPCSTR value)
{
    MSIHANDLE summary;
    UINT r;

    r = MsiGetSummaryInformationA(hdb, NULL, 1, &summary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiSummaryInfoSetPropertyA(summary, pid, VT_LPSTR, 0, NULL, value);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSummaryInfoPersist(summary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    MsiCloseHandle(summary);
}

static void set_summary_dword(MSIHANDLE hdb, DWORD pid, DWORD value)
{
    MSIHANDLE summary;
    UINT r;

    r = MsiGetSummaryInformationA(hdb, NULL, 1, &summary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiSummaryInfoSetPropertyA(summary, pid, VT_I4, value, NULL, NULL);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    r = MsiSummaryInfoPersist(summary);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %u\n", r);

    MsiCloseHandle(summary);
}

static void test_msipackage(void)
{
    MSIHANDLE hdb = 0, hpack = 100;
    UINT r;
    const char *query;
    char name[10];

    /* NULL szPackagePath */
    r = MsiOpenPackage(NULL, &hpack);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    /* empty szPackagePath */
    r = MsiOpenPackage("", &hpack);
    todo_wine
    {
        ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    }

    if (r == ERROR_SUCCESS)
        MsiCloseHandle(hpack);

    /* nonexistent szPackagePath */
    r = MsiOpenPackage("nonexistent", &hpack);
    ok(r == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", r);

    /* NULL hProduct */
    r = MsiOpenPackage(msifile, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", r);

    name[0]='#';
    name[1]=0;
    r = MsiOpenPackage(name, &hpack);
    ok(r == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", r);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    /* database exists, but is emtpy */
    sprintf(name, "#%ld", hdb);
    r = MsiOpenPackage(name, &hpack);
    ok(r == ERROR_INSTALL_PACKAGE_INVALID,
       "Expected ERROR_INSTALL_PACKAGE_INVALID, got %d\n", r);

    query = "CREATE TABLE `Property` ( "
            "`Property` CHAR(72), `Value` CHAR(0) "
            "PRIMARY KEY `Property`)";
    r = try_query(hdb, query);
    ok(r == ERROR_SUCCESS, "failed to create Properties table\n");

    query = "CREATE TABLE `InstallExecuteSequence` ("
            "`Action` CHAR(72), `Condition` CHAR(0), `Sequence` INTEGER "
            "PRIMARY KEY `Action`)";
    r = try_query(hdb, query);
    ok(r == ERROR_SUCCESS, "failed to create InstallExecuteSequence table\n");

    /* a few key tables exist */
    sprintf(name, "#%ld", hdb);
    r = MsiOpenPackage(name, &hpack);
    ok(r == ERROR_INSTALL_PACKAGE_INVALID,
       "Expected ERROR_INSTALL_PACKAGE_INVALID, got %d\n", r);

    MsiCloseHandle(hdb);
    DeleteFile(msifile);

    /* start with a clean database to show what constitutes a valid package */
    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    sprintf(name, "#%ld", hdb);

    /* The following summary information props must exist:
     *  - PID_REVNUMBER
     *  - PID_PAGECOUNT
     */

    set_summary_dword(hdb, PID_PAGECOUNT, 100);
    r = MsiOpenPackage(name, &hpack);
    ok(r == ERROR_INSTALL_PACKAGE_INVALID,
       "Expected ERROR_INSTALL_PACKAGE_INVALID, got %d\n", r);

    set_summary_str(hdb, PID_REVNUMBER, "{004757CD-5092-49c2-AD20-28E1CE0DF5F2}");
    r = MsiOpenPackage(name, &hpack);
    ok(r == ERROR_SUCCESS,
       "Expected ERROR_SUCCESS, got %d\n", r);

    MsiCloseHandle(hpack);
    MsiCloseHandle(hdb);
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

/* FIXME: state is INSTALLSTATE_UNKNOWN if any features are removed and the
 * feature in question is not in ADD*
 */
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

    r = create_property_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Property table: %d\n", r );

    r = add_property_entry( hdb, "'ProductCode', '{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}'" );
    ok( r == ERROR_SUCCESS, "cannot add property entry: %d\n", r );

    r = add_property_entry( hdb, "'ProductLanguage', '1033'" );
    ok( r == ERROR_SUCCESS, "cannot add property entry: %d\n", r );

    r = add_property_entry( hdb, "'ProductName', 'MSITEST'" );
    ok( r == ERROR_SUCCESS, "cannot add property entry: %d\n", r );

    r = add_property_entry( hdb, "'ProductVersion', '1.1.1'" );
    ok( r == ERROR_SUCCESS, "cannot add property entry: %d\n", r );

    r = create_install_execute_sequence_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create InstallExecuteSequence table: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'CostInitialize', '', '800'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'FileCost', '', '900'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'CostFinalize', '', '1000'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'InstallValidate', '', '1400'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'InstallInitialize', '', '1500'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'ProcessComponents', '', '1600'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'UnpublishFeatures', '', '1800'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'RegisterProduct', '', '6100'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'PublishFeatures', '', '6300'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'PublishProduct', '', '6400'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = add_install_execute_sequence_entry( hdb, "'InstallFinalize', '', '6600'" );
    ok( r == ERROR_SUCCESS, "cannot add install execute sequence entry: %d\n", r );

    r = create_media_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create media table: %d\n", r );

    r = add_media_entry( hdb, "'1', '3', '', '', 'DISK1', ''");
    ok( r == ERROR_SUCCESS, "cannot add media entry: %d\n", r );

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

    /* msidbFeatureAttributesFavorLocal */
    r = add_feature_entry( hdb, "'four', '', '', '', 2, 1, '', 0" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* disabled */
    r = add_feature_entry( hdb, "'five', '', '', '', 2, 0, '', 1" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* msidbFeatureAttributesFavorSource:msidbComponentAttributesSourceOnly */
    r = add_component_entry( hdb, "'eta', '{DD89003F-0DD4-41B8-81C0-3411A7DA2695}', 'TARGETDIR', 1, '', 'eta_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* no feature parent:msidbComponentAttributesLocalOnly */
    r = add_component_entry( hdb, "'kappa', '{D6B93DC3-8DA5-4769-9888-42BFE156BB8B}', 'TARGETDIR', 1, '', 'kappa_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:removed */
    r = add_feature_entry( hdb, "'six', '', '', '', 2, 1, '', 0" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:removed:msidbComponentAttributesLocalOnly */
    r = add_component_entry( hdb, "'lambda', '{6528C5E4-02A4-4636-A214-7A66A6C35B64}', 'TARGETDIR', 0, '', 'lambda_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:removed:msidbComponentAttributesSourceOnly */
    r = add_component_entry( hdb, "'mu', '{97014BAB-6C56-4013-9A63-2BF913B42519}', 'TARGETDIR', 1, '', 'mu_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:removed:msidbComponentAttributesOptional */
    r = add_component_entry( hdb, "'nu', '{943DD0D8-5808-4954-8526-3B8493FEDDCD}', 'TARGETDIR', 2, '', 'nu_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:removed:msidbComponentAttributesSharedDllRefCount */
    r = add_component_entry( hdb, "'xi', '{D6CF9EF7-6FCF-4930-B34B-F938AEFF9BDB}', 'TARGETDIR', 8, '', 'xi_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource:removed */
    r = add_feature_entry( hdb, "'seven', '', '', '', 2, 1, '', 1" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* msidbFeatureAttributesFavorSource:removed:msidbComponentAttributesLocalOnly */
    r = add_component_entry( hdb, "'omicron', '{7B57521D-15DB-4141-9AA6-01D934A4433F}', 'TARGETDIR', 0, '', 'omicron_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource:removed:msidbComponentAttributesSourceOnly */
    r = add_component_entry( hdb, "'pi', '{FB85346B-378E-4492-8769-792305471C81}', 'TARGETDIR', 1, '', 'pi_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource:removed:msidbComponentAttributesOptional */
    r = add_component_entry( hdb, "'rho', '{798F2047-7B0C-4783-8BB0-D703E554114B}', 'TARGETDIR', 2, '', 'rho_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource:removed:msidbComponentAttributesSharedDllRefCount */
    r = add_component_entry( hdb, "'sigma', '{5CE9DDA8-B67B-4736-9D93-99D61C5B93E7}', 'TARGETDIR', 8, '', 'sigma_file'" );
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

    r = add_feature_components_entry( hdb, "'four', 'eta'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'five', 'eta'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'six', 'lambda'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'six', 'mu'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'six', 'nu'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'six', 'xi'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'seven', 'omicron'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'seven', 'pi'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'seven', 'rho'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'seven', 'sigma'" );
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

    r = add_file_entry( hdb, "'kappa_file', 'kappa', 'kappa.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'lambda_file', 'lambda', 'lambda.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'mu_file', 'mu', 'mu.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'nu_file', 'nu', 'nu.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'xi_file', 'xi', 'xi.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'omicron_file', 'omicron', 'omicron.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'pi_file', 'pi', 'pi.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'rho_file', 'rho', 'rho.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'sigma_file', 'sigma', 'sigma.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    MsiDatabaseCommit(hdb);

    /* these properties must not be in the saved msi file */
    r = add_property_entry( hdb, "'ADDLOCAL', 'one,four'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    r = add_property_entry( hdb, "'ADDSOURCE', 'two,three'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    r = add_property_entry( hdb, "'REMOVE', 'six,seven'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    hpkg = package_from_db( hdb );
    ok( hpkg, "failed to create package\n");

    MsiCloseHandle(hdb);

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
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
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

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    r = MsiDoAction( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    r = MsiDoAction( hpkg, "FileCost");
    ok( r == ERROR_SUCCESS, "file cost failed\n");

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

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
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    todo_wine
    {
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    todo_wine
    {
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

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

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    todo_wine
    {
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    todo_wine
    {
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    todo_wine
    {
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    todo_wine
    {
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    todo_wine
    {
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    todo_wine
    {
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    todo_wine
    {
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    todo_wine
    {
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    MsiCloseHandle( hpkg );

    /* publish the features and components */
    r = MsiInstallProduct(msifile, "ADDLOCAL=one,four ADDSOURCE=two,three REMOVE=six,seven");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiOpenDatabase(msifile, MSIDBOPEN_DIRECT, &hdb);
    ok(r == ERROR_SUCCESS, "failed to open database: %d\n", r);

    /* these properties must not be in the saved msi file */
    r = add_property_entry( hdb, "'ADDLOCAL', 'one,four'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    r = add_property_entry( hdb, "'ADDSOURCE', 'two,three'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    r = add_property_entry( hdb, "'REMOVE', 'six,seven'");
    ok( r == ERROR_SUCCESS, "cannot add property: %d\n", r );

    hpkg = package_from_db( hdb );
    ok( hpkg, "failed to create package\n");

    MsiCloseHandle(hdb);

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
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_UNKNOWN_FEATURE, "Expected ERROR_UNKNOWN_FEATURE, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
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

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_UNKNOWN_COMPONENT, "Expected ERROR_UNKNOWN_COMPONENT, got %d\n", r );
    ok( state == 0xdeadbee, "Expected 0xdeadbee, got %d\n", state);
    ok( action == 0xdeadbee, "Expected 0xdeadbee, got %d\n", action);

    r = MsiDoAction( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    r = MsiDoAction( hpkg, "FileCost");
    ok( r == ERROR_SUCCESS, "file cost failed\n");

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    r = MsiDoAction( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed: %d\n", r);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "one", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    }
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "two", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    }
    ok( action == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "three", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    }
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "four", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    }
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "five", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "six", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    todo_wine
    {
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "seven", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    todo_wine
    {
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "alpha", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    }
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "beta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
    }
    ok( action == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "gamma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    }
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "theta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    }
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    }
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "epsilon", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "zeta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", state);
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "iota", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    }
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "eta", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    todo_wine
    {
        ok( state == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", state);
    }
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "kappa", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lambda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    todo_wine
    {
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "mu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    todo_wine
    {
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "nu", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    todo_wine
    {
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "xi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    todo_wine
    {
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "omicron", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    todo_wine
    {
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "pi", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    todo_wine
    {
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "rho", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    todo_wine
    {
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "sigma", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    todo_wine
    {
        ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);
    }

    MsiCloseHandle(hpkg);

    /* uninstall the product */
    r = MsiInstallProduct(msifile, "REMOVE=ALL");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    DeleteFileA(msifile);
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
    ok( size == 5, "Expected 5, got %d\n", size);

    /* retrieve the size, empty string */
    size = 0;
    r = MsiGetProperty(hPackage, "Name", empty, &size);
    ok( r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok( size == 5, "Expected 5, got %d\n", size);

    /* don't change size */
    r = MsiGetProperty(hPackage, "Name", prop, &size);
    ok( r == ERROR_MORE_DATA, "Expected ERROR_MORE_DATA, got %d\n", r);
    ok( size == 5, "Expected 5, got %d\n", size);
    ok( !lstrcmp(prop, "Valu"), "Expected Valu, got %s\n", prop);

    /* increase the size by 1 */
    size++;
    r = MsiGetProperty(hPackage, "Name", prop, &size);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok( size == 5, "Expected 5, got %d\n", size);
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

    MsiCloseHandle( hdb );

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

    MsiCloseHandle( hdb );

    r = MsiDoAction( hpkg, "AppSearch" );
    ok( r == ERROR_SUCCESS, "AppSearch failed: %d\n", r);

    r = MsiGetPropertyA( hpkg, "WEBBROWSERPROG", prop, &size );
    ok( r == ERROR_SUCCESS, "get property failed: %d\n", r);
    ok( lstrlenA(prop) != 0, "Expected non-zero length\n");

    MsiCloseHandle( hpkg );
    DeleteFileA(msifile);
}

static void test_featureparents(void)
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

    r = create_feature_components_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create FeatureComponents table: %d\n", r );

    r = create_file_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create File table: %d\n", r );

    /* msidbFeatureAttributesFavorLocal */
    r = add_feature_entry( hdb, "'zodiac', '', '', '', 2, 1, '', 0" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* msidbFeatureAttributesFavorSource */
    r = add_feature_entry( hdb, "'perseus', '', '', '', 2, 1, '', 1" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* msidbFeatureAttributesFavorLocal */
    r = add_feature_entry( hdb, "'orion', '', '', '', 2, 1, '', 0" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* disabled because of install level */
    r = add_feature_entry( hdb, "'waters', '', '', '', 15, 101, '', 9" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* child feature of disabled feature */
    r = add_feature_entry( hdb, "'bayer', 'waters', '', '', 14, 1, '', 9" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    /* component of disabled feature (install level) */
    r = add_component_entry( hdb, "'delphinus', '', 'TARGETDIR', 0, '', 'delphinus_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* component of disabled child feature (install level) */
    r = add_component_entry( hdb, "'hydrus', '', 'TARGETDIR', 0, '', 'hydrus_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:msidbComponentAttributesLocalOnly */
    r = add_component_entry( hdb, "'leo', '', 'TARGETDIR', 0, '', 'leo_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:msidbComponentAttributesSourceOnly */
    r = add_component_entry( hdb, "'virgo', '', 'TARGETDIR', 1, '', 'virgo_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:msidbComponentAttributesOptional */
    r = add_component_entry( hdb, "'libra', '', 'TARGETDIR', 2, '', 'libra_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource:msidbComponentAttributesLocalOnly */
    r = add_component_entry( hdb, "'cassiopeia', '', 'TARGETDIR', 0, '', 'cassiopeia_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource:msidbComponentAttributesSourceOnly */
    r = add_component_entry( hdb, "'cepheus', '', 'TARGETDIR', 1, '', 'cepheus_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorSource:msidbComponentAttributesOptional */
    r = add_component_entry( hdb, "'andromeda', '', 'TARGETDIR', 2, '', 'andromeda_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:msidbComponentAttributesLocalOnly */
    r = add_component_entry( hdb, "'canis', '', 'TARGETDIR', 0, '', 'canis_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:msidbComponentAttributesSourceOnly */
    r = add_component_entry( hdb, "'monoceros', '', 'TARGETDIR', 1, '', 'monoceros_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r );

    /* msidbFeatureAttributesFavorLocal:msidbComponentAttributesOptional */
    r = add_component_entry( hdb, "'lepus', '', 'TARGETDIR', 2, '', 'lepus_file'" );

    r = add_feature_components_entry( hdb, "'zodiac', 'leo'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'zodiac', 'virgo'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'zodiac', 'libra'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'perseus', 'cassiopeia'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'perseus', 'cepheus'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'perseus', 'andromeda'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'orion', 'leo'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'orion', 'virgo'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'orion', 'libra'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'orion', 'cassiopeia'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'orion', 'cepheus'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'orion', 'andromeda'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'orion', 'canis'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'orion', 'monoceros'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'orion', 'lepus'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'waters', 'delphinus'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_feature_components_entry( hdb, "'bayer', 'hydrus'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r );

    r = add_file_entry( hdb, "'leo_file', 'leo', 'leo.txt', 100, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'virgo_file', 'virgo', 'virgo.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'libra_file', 'libra', 'libra.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'cassiopeia_file', 'cassiopeia', 'cassiopeia.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'cepheus_file', 'cepheus', 'cepheus.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'andromeda_file', 'andromeda', 'andromeda.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'canis_file', 'canis', 'canis.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'monoceros_file', 'monoceros', 'monoceros.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'lepus_file', 'lepus', 'lepus.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'delphinus_file', 'delphinus', 'delphinus.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'hydrus_file', 'hydrus', 'hydrus.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    hpkg = package_from_db( hdb );
    ok( hpkg, "failed to create package\n");

    MsiCloseHandle( hdb );

    r = MsiDoAction( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    r = MsiDoAction( hpkg, "FileCost");
    ok( r == ERROR_SUCCESS, "file cost failed\n");

    r = MsiDoAction( hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "cost finalize failed\n");

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "zodiac", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "perseus", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "orion", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "waters", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "bayer", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "leo", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "virgo", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected virgo INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected virgo INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "libra", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected libra INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected libra INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "cassiopeia", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected cassiopeia INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected cassiopeia INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "cepheus", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected cepheus INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected cepheus INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "andromeda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected andromeda INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected andromeda INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "canis", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected canis INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected canis INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "monoceros", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected monoceros INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected monoceros INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lepus", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected lepus INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected lepus INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delphinus", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected delphinus INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected delphinus INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "hydrus", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected hydrus INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected hydrus INSTALLSTATE_UNKNOWN, got %d\n", action);

    r = MsiSetFeatureState(hpkg, "orion", INSTALLSTATE_ABSENT);
    ok( r == ERROR_SUCCESS, "failed to set feature state: %d\n", r);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "zodiac", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected zodiac INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected zodiac INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "perseus", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected perseus INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected perseus INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetFeatureState(hpkg, "orion", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_ABSENT, "Expected orion INSTALLSTATE_ABSENT, got %d\n", state);
    ok( action == INSTALLSTATE_ABSENT, "Expected orion INSTALLSTATE_ABSENT, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "leo", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected leo INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected leo INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "virgo", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected virgo INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected virgo INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "libra", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected libra INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected libra INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "cassiopeia", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected cassiopeia INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_LOCAL, "Expected cassiopeia INSTALLSTATE_LOCAL, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "cepheus", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected cepheus INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected cepheus INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "andromeda", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected andromeda INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_SOURCE, "Expected andromeda INSTALLSTATE_SOURCE, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "canis", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected canis INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected canis INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "monoceros", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected monoceros INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected monoceros INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "lepus", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected lepus INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected lepus INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "delphinus", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected delphinus INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected delphinus INSTALLSTATE_UNKNOWN, got %d\n", action);

    state = 0xdeadbee;
    action = 0xdeadbee;
    r = MsiGetComponentState(hpkg, "hydrus", &state, &action);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r );
    ok( state == INSTALLSTATE_UNKNOWN, "Expected hydrus INSTALLSTATE_UNKNOWN, got %d\n", state);
    ok( action == INSTALLSTATE_UNKNOWN, "Expected hydrus INSTALLSTATE_UNKNOWN, got %d\n", action);
    
    MsiCloseHandle(hpkg);
    DeleteFileA(msifile);
}

static void test_installprops(void)
{
    MSIHANDLE hpkg, hdb;
    CHAR path[MAX_PATH];
    CHAR buf[MAX_PATH];
    DWORD size, type;
    LANGID langid;
    HKEY hkey1, hkey2;
    int res;
    UINT r;

    GetCurrentDirectory(MAX_PATH, path);
    lstrcat(path, "\\");
    lstrcat(path, msifile);

    hdb = create_package_db();
    ok( hdb, "failed to create database\n");

    hpkg = package_from_db(hdb);
    ok( hpkg, "failed to create package\n");

    MsiCloseHandle(hdb);

    size = MAX_PATH;
    r = MsiGetProperty(hpkg, "DATABASE", buf, &size);
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    ok( !lstrcmp(buf, path), "Expected %s, got %s\n", path, buf);

    RegOpenKey(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\MS Setup (ACME)\\User Info", &hkey1);

    RegOpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", &hkey2);

    size = MAX_PATH;
    type = REG_SZ;
    *path = '\0';
    if (RegQueryValueEx(hkey1, "DefName", NULL, &type, (LPBYTE)path, &size) != ERROR_SUCCESS)
    {
        size = MAX_PATH;
        type = REG_SZ;
        RegQueryValueEx(hkey2, "RegisteredOwner", NULL, &type, (LPBYTE)path, &size);
    }

    /* win9x doesn't set this */
    if (*path)
    {
        size = MAX_PATH;
        r = MsiGetProperty(hpkg, "USERNAME", buf, &size);
        ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
        ok( !lstrcmp(buf, path), "Expected %s, got %s\n", path, buf);
    }

    size = MAX_PATH;
    type = REG_SZ;
    *path = '\0';
    if (RegQueryValueEx(hkey1, "DefCompany", NULL, &type, (LPBYTE)path, &size) != ERROR_SUCCESS)
    {
        size = MAX_PATH;
        type = REG_SZ;
        RegQueryValueEx(hkey2, "RegisteredOrganization", NULL, &type, (LPBYTE)path, &size);
    }

    if (*path)
    {
        size = MAX_PATH;
        r = MsiGetProperty(hpkg, "COMPANYNAME", buf, &size);
        ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
        ok( !lstrcmp(buf, path), "Expected %s, got %s\n", path, buf);
    }

    size = MAX_PATH;
    r = MsiGetProperty(hpkg, "VersionDatabase", buf, &size);
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    trace("VersionDatabase = %s\n", buf);

    size = MAX_PATH;
    r = MsiGetProperty(hpkg, "VersionMsi", buf, &size);
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    trace("VersionMsi = %s\n", buf);

    size = MAX_PATH;
    r = MsiGetProperty(hpkg, "Date", buf, &size);
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    trace("Date = %s\n", buf);

    size = MAX_PATH;
    r = MsiGetProperty(hpkg, "Time", buf, &size);
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    trace("Time = %s\n", buf);

    size = MAX_PATH;
    r = MsiGetProperty(hpkg, "PackageCode", buf, &size);
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    trace("PackageCode = %s\n", buf);

    langid = GetUserDefaultLangID();
    sprintf(path, "%d", langid);

    size = MAX_PATH;
    r = MsiGetProperty(hpkg, "UserLanguageID", buf, &size);
    ok( r == ERROR_SUCCESS, "Expected ERROR_SUCCESS< got %d\n", r);
    ok( !lstrcmpA(buf, path), "Expected \"%s\", got \"%s\"\n", path, buf);

    res = GetSystemMetrics(SM_CXSCREEN);
    size = MAX_PATH;
    r = MsiGetProperty(hpkg, "ScreenX", buf, &size);
    ok(atol(buf) == res, "Expected %d, got %ld\n", res, atol(buf));

    res = GetSystemMetrics(SM_CYSCREEN);
    size = MAX_PATH;
    r = MsiGetProperty(hpkg, "ScreenY", buf, &size);
    ok(atol(buf) == res, "Expected %d, got %ld\n", res, atol(buf));

    CloseHandle(hkey1);
    CloseHandle(hkey2);
    MsiCloseHandle(hpkg);
    DeleteFile(msifile);
}

static void test_sourcedirprop(void)
{
    MSIHANDLE hpkg, hdb;
    CHAR source_dir[MAX_PATH];
    CHAR path[MAX_PATH];
    DWORD size;
    UINT r;

    hdb = create_package_db();
    ok ( hdb, "failed to create package database\n" );

    r = add_directory_entry( hdb, "'TARGETDIR', '', 'SourceDir'");
    ok( r == ERROR_SUCCESS, "cannot add directory: %d\n", r );

    hpkg = package_from_db( hdb );
    ok( hpkg, "failed to create package\n");

    MsiCloseHandle( hdb );

    size = MAX_PATH;
    r = MsiGetProperty( hpkg, "SourceDir", source_dir, &size );
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    ok( !lstrlenA(source_dir), "Expected emtpy source dir, got %s\n", source_dir);

    size = MAX_PATH;
    r = MsiGetProperty( hpkg, "SOURCEDIR", source_dir, &size );
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    ok( !lstrlenA(source_dir), "Expected emtpy source dir, got %s\n", source_dir);

    r = MsiDoAction( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    size = MAX_PATH;
    r = MsiGetProperty( hpkg, "SourceDir", source_dir, &size );
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    ok( !lstrlenA(source_dir), "Expected emtpy source dir, got %s\n", source_dir);

    size = MAX_PATH;
    r = MsiGetProperty( hpkg, "SOURCEDIR", source_dir, &size );
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    ok( !lstrlenA(source_dir), "Expected emtpy source dir, got %s\n", source_dir);

    r = MsiDoAction( hpkg, "ResolveSource");
    ok( r == ERROR_SUCCESS, "file cost failed\n");

    GetCurrentDirectory(MAX_PATH, path);
    lstrcatA(path, "\\");

    size = MAX_PATH;
    r = MsiGetProperty( hpkg, "SourceDir", source_dir, &size );
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    ok( !lstrcmpA(source_dir, path), "Expected %s, got %s\n", path, source_dir);

    size = MAX_PATH;
    r = MsiGetProperty( hpkg, "SOURCEDIR", source_dir, &size );
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    ok( !lstrcmpA(source_dir, path), "Expected %s, got %s\n", path, source_dir);

    size = MAX_PATH;
    r = MsiGetProperty( hpkg, "SoUrCeDiR", source_dir, &size );
    ok( r == ERROR_SUCCESS, "failed to get property: %d\n", r);
    ok( !lstrlenA(source_dir), "Expected emtpy source dir, got %s\n", source_dir);

    MsiCloseHandle(hpkg);
    DeleteFileA(msifile);
}

static void test_prop_path(void)
{
    MSIHANDLE hpkg, hdb;
    char buffer[MAX_PATH], cwd[MAX_PATH];
    DWORD sz;
    UINT r;

    GetCurrentDirectory(MAX_PATH, cwd);
    strcat(cwd, "\\");

    hdb = create_package_db();
    ok( hdb, "failed to create database\n");

    r = add_directory_entry( hdb, "'TARGETDIR','','SourceDir'" );
    ok( r == ERROR_SUCCESS, "cannot add directory: %d\n", r );

    r = add_directory_entry( hdb, "'foo','TARGETDIR','foosrc:footgt'" );
    ok( r == ERROR_SUCCESS, "cannot add directory: %d\n", r );

    hpkg = package_from_db(hdb);
    ok( hpkg, "failed to create package\n");

    r = MsiGetSourcePath(hpkg, "SourceDir", buffer, &sz );
    ok( r == ERROR_DIRECTORY, "failed to get source path\n");

    r = MsiGetSourcePath(hpkg, "SOURCEDIR", buffer, &sz );
    ok( r == ERROR_DIRECTORY, "failed to get source path\n");

    r = MsiDoAction( hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "cost init failed\n");

    sz = sizeof buffer;
    buffer[0] = 0;
    r = MsiGetProperty(hpkg, "SourceDir", buffer, &sz);
    ok( r == ERROR_SUCCESS, "property not set\n");
    ok( !buffer[0], "SourceDir should be empty\n");

    sz = sizeof buffer;
    buffer[0] = 0;
    r = MsiGetProperty(hpkg, "SOURCEDIR", buffer, &sz);
    ok( r == ERROR_SUCCESS, "property not set\n");
    ok( !buffer[0], "SourceDir should be empty\n");

    sz = sizeof buffer;
    buffer[0] = 0;
    r = MsiGetSourcePath(hpkg, "SourceDir", buffer, &sz );
    ok( r == ERROR_SUCCESS, "failed to get source path\n");
    ok( !lstrcmpi(cwd, buffer), "SourceDir (%s) should be current dir (%s)\n", buffer, cwd);

    sz = sizeof buffer;
    buffer[0] = 0;
    r = MsiGetProperty(hpkg, "SourceDir", buffer, &sz);
    ok( r == ERROR_SUCCESS, "property not set\n");
    todo_wine {
    ok( !lstrcmpi(cwd, buffer), "SourceDir (%s) should be current dir (%s)\n", buffer, cwd);
    }

    sz = sizeof buffer;
    buffer[0] = 0;
    r = MsiGetSourcePath(hpkg, "SOURCEDIR", buffer, &sz );
    ok( r == ERROR_DIRECTORY, "failed to get source path\n");

    sz = sizeof buffer;
    buffer[0] = 0;
    r = MsiGetProperty(hpkg, "SOURCEDIR", buffer, &sz);
    ok( r == ERROR_SUCCESS, "property not set\n");
    todo_wine {
    ok( !lstrcmpi(cwd, buffer), "SourceDir (%s) should be current dir (%s)\n", buffer, cwd);
    }

    r = MsiSetProperty(hpkg, "SourceDir", "goo");
    ok( r == ERROR_SUCCESS, "property not set\n");

    sz = sizeof buffer;
    buffer[0] = 0;
    r = MsiGetProperty(hpkg, "SourceDir", buffer, &sz);
    ok( r == ERROR_SUCCESS, "property not set\n");
    ok( !lstrcmpi(buffer, "goo"), "SourceDir (%s) should be goo\n", buffer);

    sz = sizeof buffer;
    buffer[0] = 0;
    r = MsiGetSourcePath(hpkg, "SourceDir", buffer, &sz );
    ok( r == ERROR_SUCCESS, "failed to get source path\n");
    ok( !lstrcmpi(buffer, cwd), "SourceDir (%s) should be goo\n", buffer);

    sz = sizeof buffer;
    buffer[0] = 0;
    r = MsiGetProperty(hpkg, "SourceDir", buffer, &sz);
    ok( r == ERROR_SUCCESS, "property not set\n");
    ok( !lstrcmpi(buffer, "goo"), "SourceDir (%s) should be goo\n", buffer);

    MsiCloseHandle( hpkg );
    DeleteFile(msifile);
}

static void test_launchconditions(void)
{
    MSIHANDLE hpkg;
    MSIHANDLE hdb;
    UINT r;

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    hdb = create_package_db();
    ok( hdb, "failed to create package database\n" );

    r = create_launchcondition_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create LaunchCondition table: %d\n", r );

    r = add_launchcondition_entry( hdb, "'X = \"1\"', 'one'" );
    ok( r == ERROR_SUCCESS, "cannot add launch condition: %d\n", r );

    /* invalid condition */
    r = add_launchcondition_entry( hdb, "'X != \"1\"', 'one'" );
    ok( r == ERROR_SUCCESS, "cannot add launch condition: %d\n", r );

    hpkg = package_from_db( hdb );
    ok( hpkg, "failed to create package\n");

    MsiCloseHandle( hdb );

    r = MsiSetProperty( hpkg, "X", "1" );
    ok( r == ERROR_SUCCESS, "failed to set property\n" );

    /* invalid conditions are ignored */
    r = MsiDoAction( hpkg, "LaunchConditions" );
    ok( r == ERROR_SUCCESS, "cost init failed\n" );

    /* verify LaunchConditions still does some verification */
    r = MsiSetProperty( hpkg, "X", "2" );
    ok( r == ERROR_SUCCESS, "failed to set property\n" );

    r = MsiDoAction( hpkg, "LaunchConditions" );
    ok( r == ERROR_INSTALL_FAILURE, "Expected ERROR_INSTALL_FAILURE, got %d\n", r );

    MsiCloseHandle( hpkg );
    DeleteFile( msifile );
}

static void test_ccpsearch(void)
{
    MSIHANDLE hdb, hpkg;
    CHAR prop[MAX_PATH];
    DWORD size = MAX_PATH;
    UINT r;

    MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

    hdb = create_package_db();
    ok(hdb, "failed to create package database\n");

    r = create_ccpsearch_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_ccpsearch_entry(hdb, "'CCP_random'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_ccpsearch_entry(hdb, "'RMCCP_random'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_reglocator_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_reglocator_entry(hdb, "'CCP_random', 0, 'htmlfile\\shell\\open\\nonexistent', '', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_drlocator_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_drlocator_entry(hdb, "'RMCCP_random', '', 'C:\\', '0'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_signature_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    hpkg = package_from_db(hdb);
    ok(hpkg, "failed to create package\n");

    MsiCloseHandle(hdb);

    r = MsiDoAction(hpkg, "CCPSearch");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = MsiGetPropertyA(hpkg, "CCP_Success", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, "1"), "Expected 1, got %s\n", prop);

    MsiCloseHandle(hpkg);
    DeleteFileA(msifile);
}

static BOOL squash_guid(LPCWSTR in, LPWSTR out)
{
    DWORD i,n=1;
    GUID guid;

    if (FAILED(CLSIDFromString((LPOLESTR)in, &guid)))
        return FALSE;

    for(i=0; i<8; i++)
        out[7-i] = in[n++];
    n++;
    for(i=0; i<4; i++)
        out[11-i] = in[n++];
    n++;
    for(i=0; i<4; i++)
        out[15-i] = in[n++];
    n++;
    for(i=0; i<2; i++)
    {
        out[17+i*2] = in[n++];
        out[16+i*2] = in[n++];
    }
    n++;
    for( ; i<8; i++)
    {
        out[17+i*2] = in[n++];
        out[16+i*2] = in[n++];
    }
    out[32]=0;
    return TRUE;
}

static void set_component_path(LPCSTR filename, LPCSTR guid)
{
    WCHAR guidW[MAX_PATH];
    WCHAR squashedW[MAX_PATH];
    CHAR squashed[MAX_PATH];
    CHAR substr[MAX_PATH];
    CHAR path[MAX_PATH];
    HKEY hkey;

    MultiByteToWideChar(CP_ACP, 0, guid, -1, guidW, MAX_PATH);
    squash_guid(guidW, squashedW);
    WideCharToMultiByte(CP_ACP, 0, squashedW, -1, squashed, MAX_PATH, NULL, NULL);

    lstrcpyA(substr, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
                    "Installer\\UserData\\S-1-5-18\\Components\\");
    lstrcatA(substr, squashed);

    RegCreateKeyA(HKEY_LOCAL_MACHINE, substr, &hkey);

    lstrcpyA(path, CURR_DIR);
    lstrcatA(path, "\\");
    lstrcatA(path, filename);

    /* just using a random squashed product code */
    RegSetValueExA(hkey, "7D2F387510109040002000060BECB6AB", 0,
                   REG_SZ, (LPBYTE)path, lstrlenA(path));

    RegCloseKey(hkey);

    lstrcpyA(substr, "SOFTWARE\\Classes\\Installer\\Products\\7D2F387510109040002000060BECB6AB");
    RegCreateKeyA(HKEY_LOCAL_MACHINE, substr, &hkey);
}

static void delete_component_path(LPCSTR guid)
{
    WCHAR guidW[MAX_PATH];
    WCHAR squashedW[MAX_PATH];
    WCHAR substrW[MAX_PATH];
    CHAR squashed[MAX_PATH];
    CHAR substr[MAX_PATH];

    MultiByteToWideChar(CP_ACP, 0, guid, -1, guidW, MAX_PATH);
    squash_guid(guidW, squashedW);
    WideCharToMultiByte(CP_ACP, 0, squashedW, -1, squashed, MAX_PATH, NULL, NULL);

    lstrcpyA(substr, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
                    "Installer\\UserData\\S-1-5-18\\Components\\");
    lstrcatA(substr, squashed);

    MultiByteToWideChar(CP_ACP, 0, substr, -1, substrW, MAX_PATH);
    package_RegDeleteTreeW(HKEY_LOCAL_MACHINE, substrW);

    lstrcpyA(substr, "SOFTWARE\\Classes\\Installer\\Products\\7D2F387510109040002000060BECB6AB");
    MultiByteToWideChar(CP_ACP, 0, substr, -1, substrW, MAX_PATH);
    package_RegDeleteTreeW(HKEY_LOCAL_MACHINE, substrW);
}

static void test_complocator(void)
{
    MSIHANDLE hdb, hpkg;
    UINT r;
    CHAR prop[MAX_PATH];
    CHAR expected[MAX_PATH];
    DWORD size = MAX_PATH;

    hdb = create_package_db();
    ok(hdb, "failed to create package database\n");

    r = create_appsearch_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'ABELISAURUS', 'abelisaurus'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'BACTROSAURUS', 'bactrosaurus'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'CAMELOTIA', 'camelotia'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'DICLONIUS', 'diclonius'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'ECHINODON', 'echinodon'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'FALCARIUS', 'falcarius'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'GALLIMIMUS', 'gallimimus'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'HAGRYPHUS', 'hagryphus'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'IGUANODON', 'iguanodon'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'JOBARIA', 'jobaria'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'KAKURU', 'kakuru'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'LABOCANIA', 'labocania'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'MEGARAPTOR', 'megaraptor'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'NEOSODON', 'neosodon'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'OLOROTITAN', 'olorotitan'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_appsearch_entry(hdb, "'PANTYDRACO', 'pantydraco'");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_complocator_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'abelisaurus', '{E3619EED-305A-418C-B9C7-F7D7377F0934}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'bactrosaurus', '{D56B688D-542F-42Ef-90FD-B6DA76EE8119}', 0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'camelotia', '{8211BE36-2466-47E3-AFB7-6AC72E51AED2}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'diclonius', '{5C767B20-A33C-45A4-B80B-555E512F01AE}', 0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'echinodon', '{A19E16C5-C75D-4699-8111-C4338C40C3CB}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'falcarius', '{17762FA1-A7AE-4CC6-8827-62873C35361D}', 0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'gallimimus', '{75EBF568-C959-41E0-A99E-9050638CF5FB}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'hagrphus', '{D4969B72-17D9-4AB6-BE49-78F2FEE857AC}', 0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'iguanodon', '{8E0DA02E-F6A7-4A8F-B25D-6F564C492308}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'jobaria', '{243C22B1-8C51-4151-B9D1-1AE5265E079E}', 0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'kakuru', '{5D0F03BA-50BC-44F2-ABB1-72C972F4E514}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'labocania', '{C7DDB60C-7828-4046-A6F8-699D5E92F1ED}', 0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'megaraptor', '{8B1034B7-BD5E-41ac-B52C-0105D3DFD74D}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'neosodon', '{0B499649-197A-48EF-93D2-AF1C17ED6E90}', 0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'olorotitan', '{54E9E91F-AED2-46D5-A25A-7E50AFA24513}', 1");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_complocator_entry(hdb, "'pantydraco', '{2A989951-5565-4FA7-93A7-E800A3E67D71}', 0");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = create_signature_table(hdb);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'abelisaurus', 'abelisaurus', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'bactrosaurus', 'bactrosaurus', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'camelotia', 'camelotia', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'diclonius', 'diclonius', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'iguanodon', 'iguanodon', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'jobaria', 'jobaria', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'kakuru', 'kakuru', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    r = add_signature_entry(hdb, "'labocania', 'labocania', '', '', '', '', '', '', ''");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    hpkg = package_from_db(hdb);
    ok(hpkg, "failed to create package\n");

    MsiCloseHandle(hdb);

    create_test_file("abelisaurus");
    create_test_file("bactrosaurus");
    create_test_file("camelotia");
    create_test_file("diclonius");
    create_test_file("echinodon");
    create_test_file("falcarius");
    create_test_file("gallimimus");
    create_test_file("hagryphus");
    CreateDirectoryA("iguanodon", NULL);
    CreateDirectoryA("jobaria", NULL);
    CreateDirectoryA("kakuru", NULL);
    CreateDirectoryA("labocania", NULL);
    CreateDirectoryA("megaraptor", NULL);
    CreateDirectoryA("neosodon", NULL);
    CreateDirectoryA("olorotitan", NULL);
    CreateDirectoryA("pantydraco", NULL);

    set_component_path("abelisaurus", "{E3619EED-305A-418C-B9C7-F7D7377F0934}");
    set_component_path("bactrosaurus", "{D56B688D-542F-42Ef-90FD-B6DA76EE8119}");
    set_component_path("echinodon", "{A19E16C5-C75D-4699-8111-C4338C40C3CB}");
    set_component_path("falcarius", "{17762FA1-A7AE-4CC6-8827-62873C35361D}");
    set_component_path("iguanodon", "{8E0DA02E-F6A7-4A8F-B25D-6F564C492308}");
    set_component_path("jobaria", "{243C22B1-8C51-4151-B9D1-1AE5265E079E}");
    set_component_path("megaraptor", "{8B1034B7-BD5E-41ac-B52C-0105D3DFD74D}");
    set_component_path("neosodon", "{0B499649-197A-48EF-93D2-AF1C17ED6E90}");

    r = MsiDoAction(hpkg, "AppSearch");
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "ABELISAURUS", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    lstrcpyA(expected, CURR_DIR);
    lstrcatA(expected, "\\abelisaurus");
    ok(!lstrcmpA(prop, expected) || !lstrcmpA(prop, ""),
       "Expected %s or empty string, got %s\n", expected, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "BACTROSAURUS", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "CAMELOTIA", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "DICLONIUS", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "ECHINODON", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    lstrcpyA(expected, CURR_DIR);
    lstrcatA(expected, "\\");
    ok(!lstrcmpA(prop, expected) || !lstrcmpA(prop, ""),
       "Expected %s or empty string, got %s\n", expected, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "FALCARIUS", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "GALLIMIMUS", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "HAGRYPHUS", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "IGUANODON", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "JOBARIA", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "KAKURU", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "LABOCANIA", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "MEGARAPTOR", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    lstrcpyA(expected, CURR_DIR);
    lstrcatA(expected, "\\");
    ok(!lstrcmpA(prop, expected) || !lstrcmpA(prop, ""),
       "Expected %s or empty string, got %s\n", expected, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "NEOSODON", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);

    lstrcpyA(expected, CURR_DIR);
    lstrcatA(expected, "\\neosodon\\");
    ok(!lstrcmpA(prop, expected) || !lstrcmpA(prop, ""),
       "Expected %s or empty string, got %s\n", expected, prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "OLOROTITAN", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    size = MAX_PATH;
    r = MsiGetPropertyA(hpkg, "PANTYDRACO", prop, &size);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(!lstrcmpA(prop, ""), "Expected , got %s\n", prop);

    MsiCloseHandle(hpkg);
    DeleteFileA("abelisaurus");
    DeleteFileA("bactrosaurus");
    DeleteFileA("camelotia");
    DeleteFileA("diclonius");
    DeleteFileA("echinodon");
    DeleteFileA("falcarius");
    DeleteFileA("gallimimus");
    DeleteFileA("hagryphus");
    RemoveDirectoryA("iguanodon");
    RemoveDirectoryA("jobaria");
    RemoveDirectoryA("kakuru");
    RemoveDirectoryA("labocania");
    RemoveDirectoryA("megaraptor");
    RemoveDirectoryA("neosodon");
    RemoveDirectoryA("olorotitan");
    RemoveDirectoryA("pantydraco");
    delete_component_path("{E3619EED-305A-418C-B9C7-F7D7377F0934}");
    delete_component_path("{D56B688D-542F-42Ef-90FD-B6DA76EE8119}");
    delete_component_path("{A19E16C5-C75D-4699-8111-C4338C40C3CB}");
    delete_component_path("{17762FA1-A7AE-4CC6-8827-62873C35361D}");
    delete_component_path("{8E0DA02E-F6A7-4A8F-B25D-6F564C492308}");
    delete_component_path("{243C22B1-8C51-4151-B9D1-1AE5265E079E}");
    delete_component_path("{8B1034B7-BD5E-41ac-B52C-0105D3DFD74D}");
    delete_component_path("{0B499649-197A-48EF-93D2-AF1C17ED6E90}");
    DeleteFileA(msifile);
}

START_TEST(package)
{
    GetCurrentDirectoryA(MAX_PATH, CURR_DIR);

    test_createpackage();
    test_getsourcepath_bad();
    test_getsourcepath();
    test_doaction();
    test_gettargetpath_bad();
    test_settargetpath();
    test_props();
    test_property_table();
    test_condition();
    test_msipackage();
    test_formatrecord2();
    test_states();
    test_getproperty();
    test_removefiles();
    test_appsearch();
    test_featureparents();
    test_installprops();
    test_sourcedirprop();
    test_prop_path();
    test_launchconditions();
    test_ccpsearch();
    test_complocator();
}
