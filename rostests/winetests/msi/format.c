/*
 * Copyright (C) 2005 Mike McCormack for CodeWeavers
 * Copyright (C) 2005 Aric Stewart for CodeWeavers
 *
 * A test program for MSI record formatting
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
#include <shlwapi.h>
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

static UINT create_custom_action_table( MSIHANDLE hdb )
{
    return run_query( hdb,
            "CREATE TABLE `CustomAction` ("
            "`Action` CHAR(72) NOT NULL, "
            "`Type` SHORT NOT NULL, "
            "`Source` CHAR(75), "
            "`Target` CHAR(255) "
            "PRIMARY KEY `Action`)" );
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
        r = run_query( hdb, query ); \
        HeapFree(GetProcessHeap(), 0, query); \
        return r; \
    }

make_add_entry(feature,
               "INSERT INTO `Feature` "
               "(`Feature`, `Feature_Parent`, `Title`, `Description`, "
               "`Display`, `Level`, `Directory_`, `Attributes`) VALUES( %s )")

make_add_entry(component,
               "INSERT INTO `Component`  "
               "(`Component`, `ComponentId`, `Directory_`, "
               "`Attributes`, `Condition`, `KeyPath`) VALUES( %s )")

make_add_entry(feature_components,
               "INSERT INTO `FeatureComponents` "
               "(`Feature_`, `Component_`) VALUES( %s )")

make_add_entry(file,
               "INSERT INTO `File` "
               "(`File`, `Component_`, `FileName`, `FileSize`, "
               "`Version`, `Language`, `Attributes`, `Sequence`) VALUES( %s )")

make_add_entry(directory,
               "INSERT INTO `Directory` "
               "(`Directory`,`Directory_Parent`,`DefaultDir`) VALUES( %s )")

make_add_entry(custom_action,
               "INSERT INTO `CustomAction`  "
               "(`Action`, `Type`, `Source`, `Target`) VALUES( %s )")

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

    sprintf(szPackage,"#%i",hdb);
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

static MSIHANDLE helper_createpackage( const char *szName )
{
    MSIHANDLE hdb = 0;
    UINT res;
    CHAR szPackage[10];
    MSIHANDLE hPackage;
    MSIHANDLE suminfo;

    DeleteFile(szName);

    /* create an empty database */
    res = MsiOpenDatabase(szName, MSIDBOPEN_CREATE, &hdb );
    ok( res == ERROR_SUCCESS , "Failed to create database\n" );

    res = MsiDatabaseCommit( hdb );
    ok( res == ERROR_SUCCESS , "Failed to commit database\n" );

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

    sprintf(szPackage,"#%i",hdb);
    res = MsiOpenPackage(szPackage,&hPackage);
    ok( res == ERROR_SUCCESS , "Failed to open package\n" );

    res = MsiCloseHandle( hdb );
    ok( res == ERROR_SUCCESS , "Failed to close database\n" );

    return hPackage;
}

static void test_createpackage(void)
{
    static const CHAR filename[] = "winetest.msi";
    MSIHANDLE hPackage = 0;
    UINT res;

    hPackage = helper_createpackage( filename );
    ok ( hPackage != 0, " Failed to create package\n");

    res = MsiCloseHandle( hPackage);
    ok( res == ERROR_SUCCESS , "Failed to close package\n" );

    DeleteFile( filename );
}

static void test_formatrecord(void)
{
    char buffer[100];
    MSIHANDLE hrec;
    UINT r;
    DWORD sz;

    r = MsiFormatRecord(0, 0, NULL, NULL );
    ok( r == ERROR_INVALID_HANDLE, "wrong error\n");

    hrec = MsiCreateRecord(0);
    ok( hrec, "failed to create record\n");

    /* format an empty record on a record with no parameters */
    sz = sizeof(buffer);
    r = MsiFormatRecordA(0, hrec, buffer, &sz );
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong\n");

    MsiCloseHandle( hrec );

    hrec = MsiCreateRecord(4);
    ok( hrec, "failed to create record\n");

    /* format an empty record */
    r = MsiFormatRecord(0, hrec, NULL, NULL );
    ok( r == ERROR_SUCCESS, "format failed\n");
    buffer[0] = 'x';
    buffer[1] = 0;
    sz=0;
    r = MsiFormatRecord(0, hrec, buffer+1, &sz);
    ok( r == ERROR_MORE_DATA && buffer[0] == 'x', "format failed measuring with buffer\n");
    ok( sz == 16, "size wrong\n");
    sz=100;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed with empty buffer\n");
    ok( sz == 16, "size wrong\n");
    ok( 0 == strcmp(buffer,"1:  2:  3:  4:  "), "wrong output\n");

    r = MsiCloseHandle(hrec);
    ok(r==ERROR_SUCCESS, "Unable to close record\n");

    hrec = MsiCreateRecord(6);
    ok( hrec, "failed to create record\n");

    sz = 100;
    buffer[0] = 'x';
    buffer[1] = 0;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed with empty buffer\n");
    ok( sz == 24, "size wrong\n");
    ok( 0 == strcmp(buffer,"1:  2:  3:  4:  5:  6:  "), "wrong output\n");


    /* format a format string with everything else empty */
    r = MsiRecordSetString(hrec, 0, "%1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecord(0, hrec, NULL, NULL );
    ok( r == ERROR_SUCCESS, "format failed\n");
    r = MsiFormatRecord(0, hrec, buffer, NULL);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error\n");

    sz = 123;
    r = MsiFormatRecord(0, hrec, NULL, &sz);
    ok( r == ERROR_SUCCESS, "format failed with empty buffer\n");
    ok( sz == 2, "size wrong (%i)\n",sz);
    sz = sizeof buffer;
    buffer[0] = 'x';
    buffer[1] = 0;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed with empty buffer\n");
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer,"%1"), "wrong output\n");

    /* make the buffer too small */
    sz = 0;
    buffer[0] = 'x';
    buffer[1] = 0;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_MORE_DATA, "format failed with empty buffer\n");
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer,"x"), "wrong output\n");

    /* make the buffer a little bit bigger */
    sz = 1;
    buffer[0] = 'x';
    buffer[1] = 0;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_MORE_DATA, "format failed with empty buffer\n");
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output (%s)\n",buffer);

    /* and again */
    sz = 2;
    buffer[0] = 'x';
    buffer[1] = 0;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_MORE_DATA, "format failed with empty buffer\n");
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer,"%"), "wrong output\n");

    /* and again */
    sz = 3;
    buffer[0] = 'x';
    buffer[1] = 0;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed with empty buffer\n");
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer,"%1"), "wrong output\n");

    /* now try a real format string */
    r = MsiRecordSetString(hrec, 0, "[1]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");

    /* now put something in the first field */
    r = MsiRecordSetString(hrec, 1, "boo hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo hoo"), "wrong output\n");

    /* now put something in the first field */
    r = MsiRecordSetString(hrec, 0, "[1] [2]");
    r = MsiRecordSetString(hrec, 1, "boo");
    r = MsiRecordSetString(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo hoo"), "wrong output\n");

    /* empty string */
    r = MsiRecordSetString(hrec, 0, "");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 30, "size wrong %i\n",sz);
    ok( 0 == strcmp(buffer,"1: boo 2: hoo 3:  4:  5:  6:  "), 
                    "wrong output(%s)\n",buffer);

    /* play games with recursive lookups */
    r = MsiRecordSetString(hrec, 0, "[[1]] [2]");
    r = MsiRecordSetString(hrec, 1, "2");
    r = MsiRecordSetString(hrec, 2, "hey");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer,"hey hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "[[1]] [2]");
    r = MsiRecordSetString(hrec, 1, "[2]");
    r = MsiRecordSetString(hrec, 2, "hey");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 9, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer,"[[2]] hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "[[[3]]] [2]");
    r = MsiRecordSetString(hrec, 1, "2");
    r = MsiRecordSetString(hrec, 2, "hey");
    r = MsiRecordSetString(hrec, 3, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer,"hey hey"), "wrong output (%s)\n",buffer);

    r = MsiCloseHandle(hrec);
    ok(r==ERROR_SUCCESS, "Unable to close record\n");
    hrec = MsiCreateRecord(12);
    ok( hrec, "failed to create record\n");

    r = MsiRecordSetString(hrec, 0, "[[3][1]] [2]");
    r = MsiRecordSetString(hrec, 1, "2");
    r = MsiRecordSetString(hrec, 2, "hey");
    r = MsiRecordSetString(hrec, 3, "1");
    r = MsiRecordSetString(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer,"big hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "[[3][4][1]] [2]");
    r = MsiRecordSetString(hrec, 1, "2");
    r = MsiRecordSetString(hrec, 2, "hey");
    r = MsiRecordSetString(hrec, 3, "1");
    r = MsiRecordSetString(hrec, 4, NULL);
    r = MsiRecordSetString(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer,"big hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "[[3][[4]][1]] [2]");
    r = MsiRecordSetString(hrec, 1, "2");
    r = MsiRecordSetString(hrec, 2, "hey");
    r = MsiRecordSetString(hrec, 3, "1");
    r = MsiRecordSetString(hrec, 4, NULL);
    r = MsiRecordSetString(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 10, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer,"[1[]2] hey"), "wrong output (%s)\n",buffer);

    /* incorrect  formats */
    r = MsiRecordSetString(hrec, 0, "[[[3][[4]][1]] [2]");
    r = MsiRecordSetString(hrec, 1, "2");
    r = MsiRecordSetString(hrec, 2, "hey");
    r = MsiRecordSetString(hrec, 3, "1");
    r = MsiRecordSetString(hrec, 4, NULL);
    r = MsiRecordSetString(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 18, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer,"[[[3][[4]][1]] [2]"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "[[3][[4]][1]] [2]]");
    r = MsiRecordSetString(hrec, 1, "2");
    r = MsiRecordSetString(hrec, 2, "hey");
    r = MsiRecordSetString(hrec, 3, "1");
    r = MsiRecordSetString(hrec, 4, NULL);
    r = MsiRecordSetString(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 11, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer,"[1[]2] hey]"), "wrong output (%s)\n",buffer);


    /* play games with {} */

    r = MsiRecordSetString(hrec, 0, "{[3][1]} [2]");
    r = MsiRecordSetString(hrec, 1, "2");
    r = MsiRecordSetString(hrec, 2, "hey");
    r = MsiRecordSetString(hrec, 3, "1");
    r = MsiRecordSetString(hrec, 4, NULL);
    r = MsiRecordSetString(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 6, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer,"12 hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "[{[3][1]}] [2]");
    r = MsiRecordSetString(hrec, 1, "2");
    r = MsiRecordSetString(hrec, 2, "hey");
    r = MsiRecordSetString(hrec, 3, "1");
    r = MsiRecordSetString(hrec, 4, NULL);
    r = MsiRecordSetString(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 8, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer,"[12] hey"), "wrong output (%s)\n",buffer);


    r = MsiRecordSetString(hrec, 0, "{test} [2]");
    r = MsiRecordSetString(hrec, 1, "2");
    r = MsiRecordSetString(hrec, 2, "hey");
    r = MsiRecordSetString(hrec, 3, "1");
    r = MsiRecordSetString(hrec, 4, NULL);
    r = MsiRecordSetString(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 10, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer,"{test} hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "{[test]} [2]");
    r = MsiRecordSetString(hrec, 1, "2");
    r = MsiRecordSetString(hrec, 2, "hey");
    r = MsiRecordSetString(hrec, 3, "1");
    r = MsiRecordSetString(hrec, 4, NULL);
    r = MsiRecordSetString(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 12, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer,"{[test]} hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "{[1][2][3][4]} [2]");
    r = MsiRecordSetString(hrec, 1, "2");
    r = MsiRecordSetString(hrec, 2, "hey");
    r = MsiRecordSetString(hrec, 3, "1");
    r = MsiRecordSetString(hrec, 4, NULL);
    r = MsiRecordSetString(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 4, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer," hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "{[1][2][3][dummy]} [2]");
    r = MsiRecordSetString(hrec, 1, "2");
    r = MsiRecordSetString(hrec, 2, "hey");
    r = MsiRecordSetString(hrec, 3, "1");
    r = MsiRecordSetString(hrec, 4, NULL);
    r = MsiRecordSetString(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 18, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer,"{2hey1[dummy]} hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "{[1][2][3][4][dummy]} [2]");
    r = MsiRecordSetString(hrec, 1, "2");
    r = MsiRecordSetString(hrec, 2, "hey");
    r = MsiRecordSetString(hrec, 3, "1");
    r = MsiRecordSetString(hrec, 4, NULL);
    r = MsiRecordSetString(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 18, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer,"{2hey1[dummy]} hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "{{[1][2]}[3][4][dummy]}");
    r = MsiRecordSetString(hrec, 1, "2");
    r = MsiRecordSetString(hrec, 2, "hey");
    r = MsiRecordSetString(hrec, 3, "1");
    r = MsiRecordSetString(hrec, 4, NULL);
    r = MsiRecordSetString(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 16, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer,"{{2hey}1[dummy]}"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "{{[1][2]}[3]{[4][dummy]}}");
    r = MsiRecordSetString(hrec, 1, "2");
    r = MsiRecordSetString(hrec, 2, "hey");
    r = MsiRecordSetString(hrec, 3, "1");
    r = MsiRecordSetString(hrec, 4, NULL);
    r = MsiRecordSetString(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer,""), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "{{[1][2]}[3]} {[1][2]}");
    r = MsiRecordSetString(hrec, 1, "1");
    r = MsiRecordSetString(hrec, 2, "2");
    r = MsiRecordSetString(hrec, 3, "3");
    r = MsiRecordSetString(hrec, 4, NULL);
    r = MsiRecordSetString(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 12, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer,"{{12}3} {12}"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "{[1][2]} {{[1][2]}[3]} {[1][2]}");
    r = MsiRecordSetString(hrec, 1, "1");
    r = MsiRecordSetString(hrec, 2, "2");
    r = MsiRecordSetString(hrec, 3, "3");
    r = MsiRecordSetString(hrec, 4, NULL);
    r = MsiRecordSetString(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 15, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer,"12 {{12}3} {12}"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "{[4]}{[1][2]} {{[1][2]}[3]} {[1][2]}");
    r = MsiRecordSetString(hrec, 1, "1");
    r = MsiRecordSetString(hrec, 2, "2");
    r = MsiRecordSetString(hrec, 3, "3");
    r = MsiRecordSetString(hrec, 4, NULL);
    r = MsiRecordSetString(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 15, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer,"12 {{12}3} {12}"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "{blah} {[4]}{[1][2]} {{[1][2]}[3]} {[1][2]}");
    r = MsiRecordSetString(hrec, 1, "1");
    r = MsiRecordSetString(hrec, 2, "2");
    r = MsiRecordSetString(hrec, 3, "3");
    r = MsiRecordSetString(hrec, 4, NULL);
    r = MsiRecordSetString(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 22, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer,"{blah} 12 {{12}3} {12}"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "{{[1]}[2]} {[4]}{[1][2]}");
    r = MsiRecordSetString(hrec, 1, "1");
    r = MsiRecordSetString(hrec, 2, "2");
    r = MsiRecordSetString(hrec, 3, "3");
    r = MsiRecordSetString(hrec, 4, NULL);
    r = MsiRecordSetString(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 13, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer,"{{1}2} {}{12}"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "{{[1]}} {[4]}{[1][2]}");
    r = MsiRecordSetString(hrec, 1, "1");
    r = MsiRecordSetString(hrec, 2, "2");
    r = MsiRecordSetString(hrec, 3, "3");
    r = MsiRecordSetString(hrec, 4, NULL);
    r = MsiRecordSetString(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 3, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer," 12"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "{{{[1]}} {[4]}{[1][2]}");
    r = MsiRecordSetString(hrec, 1, "1");
    r = MsiRecordSetString(hrec, 2, "2");
    r = MsiRecordSetString(hrec, 3, "3");
    r = MsiRecordSetString(hrec, 4, NULL);
    r = MsiRecordSetString(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    todo_wine{
    ok( sz == 3, "size wrong,(%i)\n",sz);
    ok( 0 == strcmp(buffer," 12"), "wrong output (%s)\n",buffer);
    }
    
    /* now put play games with escaping */
    r = MsiRecordSetString(hrec, 0, "[1] [2] [\\3asdf]");
    r = MsiRecordSetString(hrec, 1, "boo");
    r = MsiRecordSetString(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 16, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo hoo [\\3asdf]"), "wrong output\n");

    /* now put play games with escaping */
    r = MsiRecordSetString(hrec, 0, "[1] [2] [\\x]");
    r = MsiRecordSetString(hrec, 1, "boo");
    r = MsiRecordSetString(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 12, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo hoo [\\x]"), "wrong output\n");

    MsiRecordSetString(hrec, 0, "[\\x]");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 4, "size wrong: %d\n", sz);
    ok( 0 == strcmp(buffer,"[\\x]"), "wrong output: %s\n", buffer);

    MsiRecordSetString(hrec, 0, "{\\x}");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 4, "size wrong: %d\n", sz);
    ok( 0 == strcmp(buffer,"{\\x}"), "wrong output: %s\n", buffer);

    MsiRecordSetString(hrec, 0, "[abc\\x]");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong: %d\n", sz);
    ok( 0 == strcmp(buffer,"[abc\\x]"), "wrong output: %s\n", buffer);

    MsiRecordSetString(hrec, 0, "[\\[]Bracket Text[\\]]");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 20, "size wrong: %d\n", sz);
    ok( 0 == strcmp(buffer,"[\\[]Bracket Text[\\]]"), "wrong output: %s\n", buffer);

    /* now try other formats without a package */
    r = MsiRecordSetString(hrec, 0, "[1] [2] [property]");
    r = MsiRecordSetString(hrec, 1, "boo");
    r = MsiRecordSetString(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 18, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo hoo [property]"), "wrong output\n");

    r = MsiRecordSetString(hrec, 0, "[1] [~] [2]");
    r = MsiRecordSetString(hrec, 1, "boo");
    r = MsiRecordSetString(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 11, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo [~] hoo"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "[1]");
    r = MsiRecordSetInteger(hrec, 1, 123456);
    ok( r == ERROR_SUCCESS, "set integer failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 6, "size wrong\n");
    ok( 0 == strcmp(buffer,"123456"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "[~]");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"[~]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[]");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer,"[]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    /* MsiFormatRecord doesn't seem to handle a negative too well */
    r = MsiRecordSetString(hrec, 0, "[-1]");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 4, "size wrong\n");
    ok( 0 == strcmp(buffer,"[-1]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{[]}");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 4, "size wrong\n");
    ok( 0 == strcmp(buffer,"{[]}"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[0]");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"[0]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[100]");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{[1] [2]}");
    r = MsiRecordSetString(hrec, 1, "boo");
    r = MsiRecordSetString(hrec, 2, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 7, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo hoo"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{}");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{foo}");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 5, "size wrong\n");
    ok( 0 == strcmp(buffer,"{foo}"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{boo [1]}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 7, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo hoo"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{{[1]}}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{ {[1]}}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( 0 == strcmp(buffer," {hoo}"), "wrong output\n");
        ok( sz == 6, "size wrong\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{{[1]} }");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 8, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{hoo} }"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{{ [1]}}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{{[1] }}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{{a}{b}{c }{ d}{any text}}");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{{a} }");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 6, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{a} }"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{{a} {b}}");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    todo_wine {
    r = MsiRecordSetString(hrec, 0, "{{a} b}}");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{{a b}}");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{ }");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"{ }"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, " {{a}}}");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer," }"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    todo_wine {
    r = MsiRecordSetString(hrec, 0, "{{ almost {{ any }} text }}");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 8, "size wrong\n");
    ok( 0 == strcmp(buffer," text }}"), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    todo_wine {
    r = MsiRecordSetString(hrec, 0, "{{ } { hidden ][ [ }}");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[ 1]");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 4, "size wrong\n");
    ok( 0 == strcmp(buffer,"[ 1]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[01]");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"hoo"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    todo_wine {
    r = MsiRecordSetString(hrec, 0, "{{test}} [01");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 4, "size wrong\n");
    ok( 0 == strcmp(buffer," [01"), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[\\[]");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 4, "size wrong\n");
    ok( 0 == strcmp(buffer,"[\\[]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    MsiRecordSetString(hrec, 0, "[\\[]");
    MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof(buffer);
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 4, "Expected 4, got %d\n", sz);
    ok(!lstrcmpA(buffer, "[\\[]"), "Expected \"[\\[]\", got \"%s\"\n", buffer);

    r = MsiRecordSetString(hrec, 0, "[foo]");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 5, "size wrong\n");
    ok( 0 == strcmp(buffer,"[foo]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[01.]");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 5, "size wrong\n");
    ok( 0 == strcmp(buffer,"[01.]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    SetEnvironmentVariable("FOO", "BAR");
    r = MsiRecordSetString(hrec, 0, "[%FOO]");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 6, "size wrong\n");
    ok( 0 == strcmp(buffer,"[%FOO]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{{[1]}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 6, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{hoo}"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{{ {[1]}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 8, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{ {hoo}"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{{ {[1]}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 8, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{ {hoo}"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{{ {{[1]}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 9, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{ {{hoo}"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[1]}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 4, "size wrong\n");
    ok( 0 == strcmp(buffer,"hoo}"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{{ {{a}");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 7, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{ {{a}"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{{ {{a}");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 7, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{ {{a}"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "0{1{2{3{4[1]5}6}7}8}9");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 19, "size wrong\n");
        ok( 0 == strcmp(buffer,"01{2{3{4hoo56}7}8}9"), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "0{1{2[1]3}4");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 9, "size wrong\n");
        ok( 0 == strcmp(buffer,"01{2hoo34"), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "0{1{2[1]3}4");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 9, "size wrong\n");
        ok( 0 == strcmp(buffer,"01{2hoo34"), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{[1.} [1]");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 9, "size wrong\n");
    todo_wine
    {
        ok( 0 == strcmp(buffer,"{[1.} hoo"), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{[{[1]}]}");
    r = MsiRecordSetString(hrec, 1, "2");
    r = MsiRecordSetString(hrec, 2, "foo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 9, "size wrong\n");
        ok( 0 == strcmp(buffer,"{[{[1]}]}"), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{[1][}");
    r = MsiRecordSetString(hrec, 1, "2");
    r = MsiRecordSetString(hrec, 2, "foo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 2, "size wrong\n");
        ok( 0 == strcmp(buffer,"2["), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[1]");
    r = MsiRecordSetString(hrec, 1, "[2]");
    r = MsiRecordSetString(hrec, 2, "foo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"[2]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[{{boo}}1]");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 3, "size wrong\n");
    todo_wine
    {
        ok( 0 == strcmp(buffer,"[1]"), "wrong output: %s\n", buffer);
    }

    r = MsiRecordSetString(hrec, 0, "[{{boo}}1]");
    r = MsiRecordSetString(hrec, 0, "[1{{boo}}]");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"[1]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{[1]{{boo} }}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 11, "size wrong\n");
        ok( 0 == strcmp(buffer,"hoo{{boo }}"), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{[1{{boo}}]}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 12, "size wrong: got %u, expected 12\n", sz);
        ok( 0 == strcmp(buffer,"{[1{{boo}}]}"), "wrong output: got %s, expected [1]\n", buffer);
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{{[1]}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 6, "size wrong: got %u, expected 3\n", sz);
    ok( 0 == strcmp(buffer,"{{hoo}"), "wrong output: got %s, expected [1]\n", buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{[1{{bo}o}}]}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 13, "size wrong\n");
    ok( 0 == strcmp(buffer,"{[1{{bo}o}}]}"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{[1{{b{o}o}}]}");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 14, "size wrong\n");
        ok( 0 == strcmp(buffer,"{[1{{b{o}o}}]}"), "wrong output %s\n",buffer);
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{ {[1]}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 5, "size wrong\n");
        ok( 0 == strcmp(buffer," {hoo"), "wrong output %s\n",buffer);
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    /* {} inside a substitution does strange things... */
    r = MsiRecordSetString(hrec, 0, "[[1]{}]");
    r = MsiRecordSetString(hrec, 1, "2");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 5, "size wrong\n");
        ok( 0 == strcmp(buffer,"[[1]]"), "wrong output %s\n",buffer);
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[[1]{}[1]]");
    r = MsiRecordSetString(hrec, 1, "2");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 6, "size wrong\n");
        ok( 0 == strcmp(buffer,"[[1]2]"), "wrong output %s\n",buffer);
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[a[1]b[1]c{}d[1]e]");
    r = MsiRecordSetString(hrec, 1, "2");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 14, "size wrong\n");
        ok( 0 == strcmp(buffer,"[a[1]b[1]cd2e]"), "wrong output %s\n",buffer);
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[a[1]b");
    r = MsiRecordSetString(hrec, 1, "2");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 6, "size wrong\n");
    ok( 0 == strcmp(buffer,"[a[1]b"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "a[1]b]");
    r = MsiRecordSetString(hrec, 1, "2");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 4, "size wrong\n");
    ok( 0 == strcmp(buffer,"a2b]"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "]a[1]b");
    r = MsiRecordSetString(hrec, 1, "2");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 4, "size wrong\n");
    ok( 0 == strcmp(buffer,"]a2b"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "]a[1]b");
    r = MsiRecordSetString(hrec, 1, "2");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 4, "size wrong\n");
    ok( 0 == strcmp(buffer,"]a2b"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "\\[1]");
    r = MsiRecordSetString(hrec, 1, "2");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer,"\\2"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "\\{[1]}");
    r = MsiRecordSetString(hrec, 1, "2");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer,"\\2"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "a{b[1]c}d");
    r = MsiRecordSetString(hrec, 1, NULL);
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer,"ad"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{a[0]b}");
    r = MsiRecordSetString(hrec, 1, "foo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 9, "size wrong\n");
    ok( 0 == strcmp(buffer,"a{a[0]b}b"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[foo]");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 5, "size wrong\n");
    ok( 0 == strcmp(buffer,"[foo]"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{[1][-1][1]}");
    r = MsiRecordSetString(hrec, 1, "foo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 12, "size wrong\n");
        ok( 0 == strcmp(buffer,"{foo[-1]foo}"), "wrong output %s\n",buffer);
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    /* nested braces */
    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{abcd}");
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 6, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"{abcd}"), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{a[one]bc[two]de[one]f}");
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 23, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"{a[one]bc[two]de[one]f}"), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{a[one]bc[bad]de[two]f}");
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 23, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"{a[one]bc[bad]de[two]f}"), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{[bad]}");
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"{[bad]}"), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{abc{d[one]ef}"); /* missing final brace */
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 14, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"{abc{d[one]ef}"), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{abc{d[one]ef}}");
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 15, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"{abc{d[one]ef}}"), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{abc}{{def}hi{j[one]k}}");
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 5, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"{abc}"), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{{def}hi{j[one]k}}");
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,""), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{{def}hi{jk}}");
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,""), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{{{def}}hi{jk}}");
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    todo_wine
    {
        ok( sz == 7, "size wrong(%i)\n",sz);
        ok( 0 == strcmp(buffer,"hi{jk}}"), "wrong output (%s)\n",buffer);
    }

    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{{def}hi{{jk}}}");
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    todo_wine
    {
        ok( sz == 1, "size wrong(%i)\n",sz);
        ok( 0 == strcmp(buffer,"}"), "wrong output (%s)\n",buffer);
    }

    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{{def}{jk}}");
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,""), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{{def}}");
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,""), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{a{b}c}");
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"{a{b}c}"), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{a{b}}");
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 6, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"{a{b}}"), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{{b}c}");
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 6, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"{{b}c}"), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{{{{}}}}");
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    todo_wine
    {
        ok( sz == 2, "size wrong(%i)\n",sz);
        ok( 0 == strcmp(buffer,"}}"), "wrong output (%s)\n",buffer);
    }

    sz = sizeof buffer;
    MsiRecordSetInteger(hrec, 1, 100);
    MsiRecordSetInteger(hrec, 2, -100);
    MsiRecordSetString(hrec, 0, "[1] [2]");
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 8, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"100 -100"), "wrong output (%s)\n",buffer);

    sz = sizeof(buffer);
    MsiRecordSetString(hrec, 0, "[1] {[noprop] [twoprop]} {abcdef}");
    MsiRecordSetString(hrec, 1, "one");
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 33, "Expected 33, got %d\n",sz);
    ok(!lstrcmpA(buffer, "one {[noprop] [twoprop]} {abcdef}"),
       "Expected \"one {[noprop] [twoprop]} {abcdef}\", got \"%s\"\n", buffer);

    sz = sizeof(buffer);
    MsiRecordSetString(hrec, 0, "[1] {[noprop] [one]} {abcdef}");
    MsiRecordSetString(hrec, 1, "one");
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 29, "Expected 29, got %d\n",sz);
    ok(!lstrcmpA(buffer, "one {[noprop] [one]} {abcdef}"),
       "Expected \"one {[noprop] [one]} {abcdef}\", got \"%s\"\n", buffer);

    sz = sizeof(buffer);
    MsiRecordSetString(hrec, 0, "[1] {[one]} {abcdef}");
    MsiRecordSetString(hrec, 1, "one");
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 20, "Expected 20, got %d\n",sz);
    ok(!lstrcmpA(buffer, "one {[one]} {abcdef}"),
       "Expected \"one {[one]} {abcdef}\", got \"%s\"\n", buffer);

    MsiCloseHandle( hrec );
}

static void test_formatrecord_package(void)
{
    static const CHAR filename[] = "winetest.msi";
    char buffer[100];
    MSIHANDLE hrec;
    MSIHANDLE package;
    UINT r;
    DWORD sz=100;

    package = helper_createpackage( filename );
    ok(package!=0, "Unable to create package\n");

    hrec = MsiCreateRecord(12);
    ok( hrec, "failed to create record\n");

    r = MsiFormatRecord(package, 0, NULL, NULL );
    ok( r == ERROR_INVALID_HANDLE, "wrong error\n");

    r = MsiFormatRecord(package, hrec, NULL, NULL );
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec,0,NULL);
    r = MsiRecordSetString(hrec,1,NULL);
    r = MsiRecordSetString(hrec,2,NULL);
    r = MsiRecordSetString(hrec,3,NULL);
    r = MsiRecordSetString(hrec,4,NULL);
    r = MsiRecordSetString(hrec,5,NULL);
    r = MsiRecordSetString(hrec,6,NULL);
    r = MsiRecordSetString(hrec,7,NULL);
    r = MsiRecordSetString(hrec,8,NULL);
    r = MsiRecordSetString(hrec,9,NULL);
    r = MsiRecordSetString(hrec,10,NULL);
    r = MsiRecordSetString(hrec,11,NULL);
    r = MsiRecordSetString(hrec,12,NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");

    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed with empty buffer (%i)\n",r);
    ok( sz == 51, "size wrong (%i)\n",sz);
    ok( 0 == strcmp(buffer,"1:  2:  3:  4:  5:  6:  7:  8:  9:  10:  11:  12:  "), "wrong output(%s)\n",buffer);

    r = MsiSetProperty(package, "prop", "val");
    ok( r == ERROR_SUCCESS, "failed to set propertY: %d\n", r);

    r = MsiRecordSetString(hrec, 0, NULL);
    r = MsiRecordSetString(hrec, 1, "[2]");
    r = MsiRecordSetString(hrec, 2, "stuff");
    r = MsiRecordSetString(hrec, 3, "prop");
    r = MsiRecordSetString(hrec, 4, "[prop]");
    r = MsiRecordSetString(hrec, 5, "[noprop]");
    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed with empty buffer (%i)\n",r);
    todo_wine
    {
        ok( sz == 66, "size wrong (%i)\n",sz);
        ok( !lstrcmpA(buffer,
            "1: [2] 2: stuff 3: prop 4: val 5:  6:  7:  8:  9:  10:  11:  12:  "),
            "wrong output(%s)\n",buffer);
    }

    /* now put play games with escaping */
    r = MsiRecordSetString(hrec, 0, "[1] [2] [\\3asdf]");
    r = MsiRecordSetString(hrec, 1, "boo");
    r = MsiRecordSetString(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 9, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"boo hoo 3"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "[1] [2] [\\x]");
    r = MsiRecordSetString(hrec, 1, "boo");
    r = MsiRecordSetString(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 9, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"boo hoo x"), "wrong output (%s)\n",buffer);

    MsiRecordSetString(hrec, 0, "[\\x]");
    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 1, "size wrong: %d\n", sz);
    ok( 0 == strcmp(buffer,"x"), "wrong output: %s\n", buffer);

    MsiRecordSetString(hrec, 0, "{\\x}");
    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 4, "size wrong: %d\n", sz);
    ok( 0 == strcmp(buffer,"{\\x}"), "wrong output: %s\n", buffer);

    MsiRecordSetString(hrec, 0, "[abc\\x]");
    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong: %d\n", sz);
    ok( 0 == strcmp(buffer,""), "wrong output: %s\n", buffer);

    MsiRecordSetString(hrec, 0, "[abc\\xdef]");
    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong: %d\n", sz);
    ok( 0 == strcmp(buffer,""), "wrong output: %s\n", buffer);

    MsiRecordSetString(hrec, 0, "\\x");
    sz = sizeof(buffer);
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 2, "Expected 2, got %d\n", sz);
    ok(!lstrcmpA(buffer, "\\x"), "Expected \"\\x\", got \"%s\"\n", buffer);

    MsiRecordSetString(hrec, 0, "[\\[");
    sz = sizeof(buffer);
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 3, "Expected 3, got %d\n", sz);
    ok(!lstrcmpA(buffer, "[\\["), "Expected \"[\\[\", got \"%s\"\n", buffer);

    MsiRecordSetString(hrec, 0, "[\\[]");
    sz = sizeof(buffer);
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 1, "Expected 1, got %d\n", sz);
    ok(!lstrcmpA(buffer, "["), "Expected \"[\", got \"%s\"\n", buffer);

    MsiRecordSetString(hrec, 0, "[[]");
    sz = sizeof(buffer);
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 3, "Expected 3, got %d\n", sz);
    ok(!lstrcmpA(buffer, "[[]"), "Expected \"[]\", got \"%s\"\n", buffer);

    MsiRecordSetString(hrec, 0, "[\\[]]");
    sz = sizeof(buffer);
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 2, "Expected 2, got %d\n", sz);
    ok(!lstrcmpA(buffer, "[]"), "Expected \"[]\", got \"%s\"\n", buffer);

    MsiRecordSetString(hrec, 0, "[\\[a]");
    sz = sizeof(buffer);
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 1, "Expected 1, got %d\n", sz);
    ok(!lstrcmpA(buffer, "["), "Expected \"[\", got \"%s\"\n", buffer);

    MsiRecordSetString(hrec, 0, "[\\a[]");
    sz = sizeof(buffer);
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(sz == 1, "Expected 1, got %d\n", sz);
        ok(!lstrcmpA(buffer, "a"), "Expected \"a\", got \"%s\"\n", buffer);
    }

    MsiRecordSetString(hrec, 0, "[prop]");
    sz = sizeof(buffer);
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 3, "Expected 3, got %d\n", sz);
    ok(!lstrcmpA(buffer, "val"), "Expected \"val\", got \"%s\"\n", buffer);

    MsiRecordSetString(hrec, 0, "[prop] [pro\\pblah] [prop]");
    sz = sizeof(buffer);
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 8, "Expected 8, got %d\n", sz);
    ok(!lstrcmpA(buffer, "val  val"), "Expected \"val  val\", got \"%s\"\n", buffer);

    MsiSetPropertyA(package, "b", "ball");
    MsiRecordSetString(hrec, 0, "[\\b]");
    sz = sizeof(buffer);
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 1, "Expected 1, got %d\n", sz);
    ok(!lstrcmpA(buffer, "b"), "Expected \"b\", got \"%s\"\n", buffer);

    MsiRecordSetString(hrec, 0, "[\\c]");
    sz = sizeof(buffer);
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 1, "Expected 1, got %d\n", sz);
    ok(!lstrcmpA(buffer, "c"), "Expected \"c\", got \"%s\"\n", buffer);

    MsiRecordSetString(hrec, 0, "[\\[]prop]");
    sz = sizeof(buffer);
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 6, "Expected 6, got %d\n", sz);
    ok(!lstrcmpA(buffer, "[prop]"), "Expected \"[prop]\", got \"%s\"\n", buffer);

    MsiRecordSetString(hrec, 0, "[\\a]prop]");
    sz = sizeof(buffer);
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 6, "Expected 6, got %d\n", sz);
    ok(!lstrcmpA(buffer, "aprop]"), "Expected \"aprop]\", got \"%s\"\n", buffer);

    MsiRecordSetString(hrec, 0, "[\\[]Bracket Text[\\]]");
    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 14, "size wrong: %d\n", sz);
    ok( 0 == strcmp(buffer,"[Bracket Text]"), "wrong output: %s\n", buffer);

    /* null characters */
    r = MsiRecordSetString(hrec, 0, "[1] [~] [2]");
    r = MsiRecordSetString(hrec, 1, "boo");
    r = MsiRecordSetString(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 9, "size wrong: %d\n", sz);
    ok( 0 == strcmp(buffer,"boo "), "wrong output: %s\n", buffer);
    ok(!lstrcmpA(&buffer[5], " hoo"),
       "Expected \" hoo\", got \"%s\"\n", &buffer[5]);

    MsiRecordSetString(hrec, 0, "[1] [~abc] [2]");
    MsiRecordSetString(hrec, 1, "boo");
    MsiRecordSetString(hrec, 2, "hoo");
    sz = sizeof(buffer);
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 8, "Expected 8, got %d\n", sz);
    ok(!lstrcmpA(buffer, "boo  hoo"), "Expected \"boo  hoo\", got \"%s\"\n", buffer);

    /* properties */
    r = MsiSetProperty(package,"dummy","Bork");
    ok( r == ERROR_SUCCESS, "set property failed\n");
    r = MsiRecordSetString(hrec, 0, "[1] [dummy] [2]");
    r = MsiRecordSetString(hrec, 1, "boo");
    r = MsiRecordSetString(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 12, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo Bork hoo"), "wrong output\n");

    r = MsiRecordSetString(hrec, 0, "[1] [invalid] [2]");
    r = MsiRecordSetString(hrec, 1, "boo");
    r = MsiRecordSetString(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 8, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo  hoo"), "wrong output\n");

    /* nesting tests */
    r = MsiSetProperty(package,"dummya","foo");
    r = MsiSetProperty(package,"dummyb","baa");
    r = MsiSetProperty(package,"adummyc","whoa");
    ok( r == ERROR_SUCCESS, "set property failed\n");
    r = MsiRecordSetString(hrec, 0, "[dummy[1]] [dummy[2]] [[1]dummy[3]]");
    r = MsiRecordSetString(hrec, 1, "a");
    r = MsiRecordSetString(hrec, 2, "b");
    r = MsiRecordSetString(hrec, 3, "c");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 12, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"foo baa whoa"), "wrong output (%s)\n",buffer);

    r = MsiSetProperty(package,"dummya","1");
    r = MsiSetProperty(package,"dummyb","[2]");
    ok( r == ERROR_SUCCESS, "set property failed\n");
    r = MsiRecordSetString(hrec, 0, "[dummya] [[dummya]] [dummyb]");
    r = MsiRecordSetString(hrec, 1, "aaa");
    r = MsiRecordSetString(hrec, 2, "bbb");
    r = MsiRecordSetString(hrec, 3, "ccc");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    todo_wine
    {
        ok( sz == 9, "size wrong(%i)\n",sz);
        ok( 0 == strcmp(buffer,"1 [1] [2]"), "wrong output (%s)\n",buffer);
    }

    r = MsiSetProperty(package,"dummya","1");
    r = MsiSetProperty(package,"dummyb","a");
    r = MsiSetProperty(package,"dummyc","\\blath");
    r = MsiSetProperty(package,"dummyd","[\\blath]");
    ok( r == ERROR_SUCCESS, "set property failed\n");
    r = MsiRecordSetString(hrec, 0, "[dummyc] [[dummyc]] [dummy[dummyb]]");
    r = MsiRecordSetString(hrec, 1, "aaa");
    r = MsiRecordSetString(hrec, 2, "bbb");
    r = MsiRecordSetString(hrec, 3, "ccc");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 10, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"\\blath b 1"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "[1] [2] [[\\3asdf]]");
    r = MsiRecordSetString(hrec, 1, "boo");
    r = MsiRecordSetString(hrec, 2, "hoo");
    r = MsiRecordSetString(hrec, 3, "yeah");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    todo_wine
    {
        ok( sz == 11, "size wrong(%i)\n",sz);
        ok( 0 == strcmp(buffer,"boo hoo [3]"), "wrong output (%s)\n",buffer);
    }

    r = MsiRecordSetString(hrec, 0, "[1] [2] [[3]]");
    r = MsiRecordSetString(hrec, 1, "boo");
    r = MsiRecordSetString(hrec, 2, "hoo");
    r = MsiRecordSetString(hrec, 3, "\\help");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 9, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"boo hoo h"), "wrong output (%s)\n",buffer);

    /* nested properties */
    MsiSetProperty(package, "PropA", "surprise");
    MsiSetProperty(package, "PropB", "[PropA]");
    MsiSetProperty(package, "PropC", "[PropB]");
    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "[PropC]");
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"[PropB]"), "wrong output (%s)\n",buffer);

    MsiSetProperty(package, "PropA", "surprise");
    MsiSetProperty(package, "PropB", "PropA");
    MsiSetProperty(package, "PropC", "PropB");
    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "[PropC]");
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 5, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"PropB"), "wrong output (%s)\n",buffer);

    MsiSetProperty(package, "PropA", "surprise");
    MsiSetProperty(package, "PropB", "[PropA]");
    MsiSetProperty(package, "PropC", "[PropB]");
    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "[[PropC]]");
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,""), "wrong output (%s)\n",buffer);

    MsiSetProperty(package, "PropA", "surprise");
    MsiSetProperty(package, "PropB", "[PropA]");
    MsiSetProperty(package, "PropC", "PropB");
    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "[[PropC]]");
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"[PropA]"), "wrong output (%s)\n",buffer);

    MsiSetProperty(package, "PropA", "surprise");
    MsiSetProperty(package, "PropB", "PropA");
    MsiSetProperty(package, "PropC", "PropB");
    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "[[PropC]]");
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 5, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"PropA"), "wrong output (%s)\n",buffer);

    MsiSetProperty(package, "PropA", "surprise");
    MsiSetProperty(package, "PropB", "PropA");
    MsiSetProperty(package, "PropC", "PropB");
    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "[[[PropC]]]");
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 8, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"surprise"), "wrong output (%s)\n",buffer);

    /* properties inside braces */
    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{abcd}");
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 6, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"{abcd}"), "wrong output (%s)\n",buffer);

    MsiSetProperty(package, "one", "mercury");
    MsiSetProperty(package, "two", "venus");
    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{a[one]bc[two]de[one]f}");
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed: %d\n", r);
    ok( sz == 25, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"amercurybcvenusdemercuryf"), "wrong output (%s)\n",buffer);

    MsiSetProperty(package, "one", "mercury");
    MsiSetProperty(package, "two", "venus");
    MsiSetProperty(package, "bad", "");
    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{a[one]bc[bad]de[two]f}");
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,""), "wrong output (%s)\n",buffer);

    MsiSetProperty(package, "bad", "");
    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{[bad]}");
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,""), "wrong output (%s)\n",buffer);

    MsiSetProperty(package, "one", "mercury");
    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{abc{d[one]ef}"); /* missing final brace */
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    todo_wine
    {
        ok( sz == 14, "size wrong(%i)\n",sz);
        ok( 0 == strcmp(buffer,"abc{dmercuryef"), "wrong output (%s)\n",buffer);
    }

    MsiSetProperty(package, "one", "mercury");
    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{abc{d[one]ef}}");
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    todo_wine
    {
        ok( sz == 15, "size wrong(%i)\n",sz);
        ok( 0 == strcmp(buffer,"abc{dmercuryef}"), "wrong output (%s)\n",buffer);
    }

    MsiSetProperty(package, "one", "mercury");
    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{abc}{{def}hi{j[one]k}}");
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 5, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,"{abc}"), "wrong output (%s)\n",buffer);

    MsiSetProperty(package, "one", "mercury");

    sz = sizeof buffer;
    MsiRecordSetString(hrec, 0, "{{def}hi{j[one]k}}");
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong(%i)\n",sz);
    ok( 0 == strcmp(buffer,""), "wrong output (%s)\n",buffer);

    sz = sizeof(buffer);
    MsiRecordSetString(hrec, 0, "[1] {[noprop] [twoprop]} {abcdef}");
    MsiRecordSetString(hrec, 1, "one");
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 13, "Expected 13, got %d\n",sz);
    ok(!lstrcmpA(buffer, "one  {abcdef}"),
       "Expected \"one  {abcdef}\", got \"%s\"\n", buffer);

    sz = sizeof(buffer);
    MsiRecordSetString(hrec, 0, "[1] {[noprop] [one]} {abcdef}");
    MsiRecordSetString(hrec, 1, "one");
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 13, "Expected 13, got %d\n",sz);
    ok(!lstrcmpA(buffer, "one  {abcdef}"),
       "Expected \"one  {abcdef}\", got \"%s\"\n", buffer);

    sz = sizeof(buffer);
    MsiRecordSetString(hrec, 0, "[1] {[one]} {abcdef}");
    MsiRecordSetString(hrec, 1, "one");
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 20, "Expected 20, got %d\n",sz);
    ok(!lstrcmpA(buffer, "one mercury {abcdef}"),
       "Expected \"one mercury {abcdef}\", got \"%s\"\n", buffer);

    MsiCloseHandle(hrec);

    r = MsiCloseHandle(package);
    ok(r==ERROR_SUCCESS, "Unable to close package\n");

    DeleteFile( filename );
}

static void test_formatrecord_tables(void)
{
    MSIHANDLE hdb, hpkg, hrec;
    CHAR buf[MAX_PATH];
    CHAR curr_dir[MAX_PATH];
    CHAR expected[MAX_PATH];
    CHAR root[MAX_PATH];
    DWORD size;
    UINT r;

    GetCurrentDirectory( MAX_PATH, curr_dir );

    hdb = create_package_db();
    ok ( hdb, "failed to create package database\n");

    r = add_directory_entry( hdb, "'TARGETDIR', '', 'SourceDir'" );
    ok( r == ERROR_SUCCESS, "cannot add directory: %d\n", r);

    r = add_directory_entry( hdb, "'ReallyLongDir', 'TARGETDIR', "
                             "'I am a really long directory'" );
    ok( r == ERROR_SUCCESS, "cannot add directory: %d\n", r);

    r = create_feature_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Feature table: %d\n", r);

    r = add_feature_entry( hdb, "'occipitofrontalis', '', '', '', 2, 1, '', 0" );
    ok( r == ERROR_SUCCESS, "cannot add feature: %d\n", r );

    r = create_component_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create Component table: %d\n", r);

    r = add_component_entry( hdb, "'frontal', '', 'TARGETDIR', 0, '', 'frontal_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r);

    r = add_component_entry( hdb, "'parietal', '', 'TARGETDIR', 1, '', 'parietal_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r);

    r = add_component_entry( hdb, "'temporal', '', 'ReallyLongDir', 0, '', 'temporal_file'" );
    ok( r == ERROR_SUCCESS, "cannot add component: %d\n", r);

    r = create_feature_components_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create FeatureComponents table: %d\n", r);

    r = add_feature_components_entry( hdb, "'occipitofrontalis', 'frontal'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r);

    r = add_feature_components_entry( hdb, "'occipitofrontalis', 'parietal'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r);

    r = add_feature_components_entry( hdb, "'occipitofrontalis', 'temporal'" );
    ok( r == ERROR_SUCCESS, "cannot add feature components: %d\n", r);

    r = create_file_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create File table: %d\n", r);

    r = add_file_entry( hdb, "'frontal_file', 'frontal', 'frontal.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'parietal_file', 'parietal', 'parietal.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = add_file_entry( hdb, "'temporal_file', 'temporal', 'temporal.txt', 0, '', '1033', 8192, 1" );
    ok( r == ERROR_SUCCESS, "cannot add file: %d\n", r);

    r = create_custom_action_table( hdb );
    ok( r == ERROR_SUCCESS, "cannot create CustomAction table: %d\n", r);

    r = add_custom_action_entry( hdb, "'MyCustom', 51, 'prop', '[!temporal_file]'" );
    ok( r == ERROR_SUCCESS, "cannt add custom action: %d\n", r);

    r = add_custom_action_entry( hdb, "'EscapeIt1', 51, 'prop', '[\\[]Bracket Text[\\]]'" );
    ok( r == ERROR_SUCCESS, "cannt add custom action: %d\n", r);

    r = add_custom_action_entry( hdb, "'EscapeIt2', 51, 'prop', '[\\xabcd]'" );
    ok( r == ERROR_SUCCESS, "cannt add custom action: %d\n", r);

    r = add_custom_action_entry( hdb, "'EscapeIt3', 51, 'prop', '[abcd\\xefgh]'" );
    ok( r == ERROR_SUCCESS, "cannt add custom action: %d\n", r);

    hpkg = package_from_db( hdb );
    ok( hpkg, "failed to create package\n");

    MsiCloseHandle( hdb );

    r = MsiSetPropertyA( hpkg, "imaprop", "ringer" );
    ok( r == ERROR_SUCCESS, "cannot set property: %d\n", r);

    hrec = MsiCreateRecord( 1 );

    /* property doesn't exist */
    size = MAX_PATH;
    /*MsiRecordSetString( hrec, 0, "[1]" ); */
    MsiRecordSetString( hrec, 1, "[idontexist]" );
    r = MsiFormatRecord( hpkg, hrec, buf, &size );
    ok( r == ERROR_SUCCESS, "format record failed: %d\n", r);
    ok( !lstrcmp( buf, "1:  " ), "Expected '1:  ', got %s\n", buf );

    /* property exists */
    size = MAX_PATH;
    MsiRecordSetString( hrec, 1, "[imaprop]" );
    r = MsiFormatRecord( hpkg, hrec, buf, &size );
    ok( r == ERROR_SUCCESS, "format record failed: %d\n", r);
    ok( !lstrcmp( buf, "1: ringer " ), "Expected '1: ringer ', got %s\n", buf );

    /* environment variable doesn't exist */
    size = MAX_PATH;
    MsiRecordSetString( hrec, 1, "[%idontexist]" );
    r = MsiFormatRecord( hpkg, hrec, buf, &size );
    ok( r == ERROR_SUCCESS, "format record failed: %d\n", r);
    ok( !lstrcmp( buf, "1:  " ), "Expected '1:  ', got %s\n", buf );

    /* environment variable exists */
    size = MAX_PATH;
    SetEnvironmentVariable( "crazyvar", "crazyval" );
    MsiRecordSetString( hrec, 1, "[%crazyvar]" );
    r = MsiFormatRecord( hpkg, hrec, buf, &size );
    ok( r == ERROR_SUCCESS, "format record failed: %d\n", r);
    ok( !lstrcmp( buf, "1: crazyval " ), "Expected '1: crazyval ', got %s\n", buf );

    /* file key before CostInitialize */
    size = MAX_PATH;
    MsiRecordSetString( hrec, 1, "[#frontal_file]" );
    r = MsiFormatRecord( hpkg, hrec, buf, &size );
    ok( r == ERROR_SUCCESS, "format record failed: %d\n", r);
    ok( !lstrcmp( buf, "1:  " ), "Expected '1:  ', got %s\n", buf );

    r = MsiDoAction(hpkg, "CostInitialize");
    ok( r == ERROR_SUCCESS, "CostInitialize failed: %d\n", r);

    r = MsiDoAction(hpkg, "FileCost");
    ok( r == ERROR_SUCCESS, "FileCost failed: %d\n", r);

    r = MsiDoAction(hpkg, "CostFinalize");
    ok( r == ERROR_SUCCESS, "CostFinalize failed: %d\n", r);

    size = MAX_PATH;
    MsiGetProperty( hpkg, "ROOTDRIVE", root, &size );

    sprintf( expected, "1: %sfrontal.txt ", root);

    /* frontal full file key */
    size = MAX_PATH;
    MsiRecordSetString( hrec, 1, "[#frontal_file]" );
    r = MsiFormatRecord( hpkg, hrec, buf, &size );
    ok( r == ERROR_SUCCESS, "format record failed: %d\n", r);
    ok( !lstrcmp( buf, expected ), "Expected \"%s\", got \"%s\"\n", expected, buf);

    /* frontal short file key */
    size = MAX_PATH;
    MsiRecordSetString( hrec, 1, "[!frontal_file]" );
    r = MsiFormatRecord( hpkg, hrec, buf, &size );
    ok( r == ERROR_SUCCESS, "format record failed: %d\n", r);
    ok( !lstrcmp( buf, expected ), "Expected \"%s\", got \"%s\"\n", expected, buf);

    sprintf( expected, "1: %sI am a really long directory\\temporal.txt ", root);

    /* temporal full file key */
    size = MAX_PATH;
    MsiRecordSetString( hrec, 1, "[#temporal_file]" );
    r = MsiFormatRecord( hpkg, hrec, buf, &size );
    ok( r == ERROR_SUCCESS, "format record failed: %d\n", r);
    ok( !lstrcmp( buf, expected ), "Expected \"%s\", got \"%s\"\n", expected, buf);

    /* temporal short file key */
    size = MAX_PATH;
    MsiRecordSetString( hrec, 1, "[!temporal_file]" );
    r = MsiFormatRecord( hpkg, hrec, buf, &size );
    ok( r == ERROR_SUCCESS, "format record failed: %d\n", r);
    ok( !lstrcmp( buf, expected ), "Expected \"%s\", got \"%s\"\n", expected, buf);

    /* custom action 51, files don't exist */
    r = MsiDoAction( hpkg, "MyCustom" );
    ok( r == ERROR_SUCCESS, "MyCustom failed: %d\n", r);

    sprintf( expected, "%sI am a really long directory\\temporal.txt", root);

    size = MAX_PATH;
    r = MsiGetProperty( hpkg, "prop", buf, &size );
    ok( r == ERROR_SUCCESS, "get property failed: %d\n", r);
    ok( !lstrcmp( buf, expected ), "Expected \"%s\", got \"%s\"\n", expected, buf);

    sprintf( buf, "%sI am a really long directory", root );
    CreateDirectory( buf, NULL );

    lstrcat( buf, "\\temporal.txt" );
    create_test_file( buf );

    /* custom action 51, files exist */
    r = MsiDoAction( hpkg, "MyCustom" );
    ok( r == ERROR_SUCCESS, "MyCustom failed: %d\n", r);

    size = MAX_PATH;
    r = MsiGetProperty( hpkg, "prop", buf, &size );
    ok( r == ERROR_SUCCESS, "get property failed: %d\n", r);
    todo_wine
    {
        ok( !lstrcmp( buf, expected ), "Expected \"%s\", got \"%s\"\n", expected, buf);
    }

    /* custom action 51, escaped text 1 */
    r = MsiDoAction( hpkg, "EscapeIt1" );
    ok( r == ERROR_SUCCESS, "EscapeIt failed: %d\n", r);

    size = MAX_PATH;
    r = MsiGetProperty( hpkg, "prop", buf, &size );
    ok( r == ERROR_SUCCESS, "get property failed: %d\n", r);
    ok( !lstrcmp( buf, "[Bracket Text]" ), "Expected '[Bracket Text]', got %s\n", buf);

    /* custom action 51, escaped text 2 */
    r = MsiDoAction( hpkg, "EscapeIt2" );
    ok( r == ERROR_SUCCESS, "EscapeIt failed: %d\n", r);

    size = MAX_PATH;
    r = MsiGetProperty( hpkg, "prop", buf, &size );
    ok( r == ERROR_SUCCESS, "get property failed: %d\n", r);
    ok( !lstrcmp( buf, "x" ), "Expected 'x', got %s\n", buf);

    /* custom action 51, escaped text 3 */
    r = MsiDoAction( hpkg, "EscapeIt3" );
    ok( r == ERROR_SUCCESS, "EscapeIt failed: %d\n", r);

    size = MAX_PATH;
    r = MsiGetProperty( hpkg, "prop", buf, &size );
    ok( r == ERROR_SUCCESS, "get property failed: %d\n", r);
    ok( !lstrcmp( buf, "" ), "Expected '', got %s\n", buf);

    sprintf( expected, "1: %sI am a really long directory\\ ", root);

    /* component with INSTALLSTATE_LOCAL */
    size = MAX_PATH;
    MsiRecordSetString( hrec, 1, "[$temporal]" );
    r = MsiFormatRecord( hpkg, hrec, buf, &size );
    ok( r == ERROR_SUCCESS, "format record failed: %d\n", r);
    ok( !lstrcmp( buf, expected ), "Expected \"%s\", got \"%s\"\n", expected, buf);

    r = MsiSetComponentState( hpkg, "temporal", INSTALLSTATE_SOURCE );
    ok( r == ERROR_SUCCESS, "failed to set install state: %d\n", r);

    /* component with INSTALLSTATE_SOURCE */
    lstrcpy( expected, "1: " );
    lstrcat( expected, curr_dir );
    if (strlen(curr_dir) > 3)
        lstrcat( expected, "\\" );
    lstrcat( expected, " " );
    size = MAX_PATH;
    MsiRecordSetString( hrec, 1, "[$parietal]" );
    r = MsiFormatRecord( hpkg, hrec, buf, &size );
    ok( r == ERROR_SUCCESS, "format record failed: %d\n", r);
    ok( !lstrcmp( buf, expected ), "Expected '%s', got '%s'\n", expected, buf);

    sprintf( buf, "%sI am a really long directory\\temporal.txt", root );
    DeleteFile( buf );

    sprintf( buf, "%sI am a really long directory", root );
    RemoveDirectory( buf );

    MsiCloseHandle( hrec );
    MsiCloseHandle( hpkg );
    DeleteFile( msifile );
}

static void test_processmessage(void)
{
    static const CHAR filename[] = "winetest.msi";
    MSIHANDLE hrec;
    MSIHANDLE package;
    int r;

    package = helper_createpackage( filename );
    ok(package!=0, "Unable to create package\n");

    hrec = MsiCreateRecord(3);
    ok( hrec, "failed to create record\n");

    r = MsiRecordSetString(hrec, 1, "");
    ok( r == ERROR_SUCCESS, "set string failed\n");

    r = MsiProcessMessage(package, INSTALLMESSAGE_ACTIONSTART, hrec);
    ok( r == IDOK, "expected IDOK, got %i\n", r);

    MsiCloseHandle(hrec);
    MsiCloseHandle(package);

    DeleteFile(filename);
}

START_TEST(format)
{
    test_createpackage();
    test_formatrecord();
    test_formatrecord_package();
    test_formatrecord_tables();
    test_processmessage();
}
