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

static const char msifile[] = "winetest-format.msi";

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

static UINT helper_createpackage( const char *szName, MSIHANDLE *handle )
{
    MSIHANDLE hPackage, suminfo, hdb = 0;
    UINT res;
    WCHAR *nameW;
    int len;

    DeleteFileA(szName);

    len = MultiByteToWideChar( CP_ACP, 0, szName, -1, NULL, 0 );
    if (!(nameW = malloc( len * sizeof(WCHAR) ))) return ERROR_OUTOFMEMORY;
    MultiByteToWideChar( CP_ACP, 0, szName, -1, nameW, len );

    /* create an empty database */
    res = MsiOpenDatabaseW( nameW, MSIDBOPEN_CREATEDIRECT, &hdb );
    free( nameW );
    ok( res == ERROR_SUCCESS , "Failed to create database %u\n", res );
    if (res != ERROR_SUCCESS)
        return res;

    res = MsiDatabaseCommit( hdb );
    ok( res == ERROR_SUCCESS , "Failed to commit database %u\n", res );

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
                    ";1033");
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

    res = package_from_db( hdb, &hPackage );
    MsiCloseHandle(hdb);

    if (res != ERROR_SUCCESS)
        DeleteFileA( szName );
    else
        *handle = hPackage;

    return res;
}

static void test_createpackage(void)
{
    MSIHANDLE hPackage = 0;
    UINT res;

    res = helper_createpackage( msifile, &hPackage );
    if (res == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        return;
    }
    ok( res == ERROR_SUCCESS, "Failed to create package %u\n", res );

    res = MsiCloseHandle( hPackage );
    ok( res == ERROR_SUCCESS , "Failed to close package %u\n", res );

    DeleteFileA( msifile );
}

static void test_formatrecord(void)
{
    char buffer[100];
    MSIHANDLE hrec;
    UINT r;
    DWORD sz;

    r = MsiFormatRecordA(0, 0, NULL, NULL );
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
    r = MsiFormatRecordA(0, hrec, NULL, NULL );
    ok( r == ERROR_SUCCESS, "format failed\n");
    buffer[0] = 'x';
    buffer[1] = 0;
    sz=0;
    r = MsiFormatRecordA(0, hrec, buffer+1, &sz);
    ok( r == ERROR_MORE_DATA && buffer[0] == 'x', "format failed measuring with buffer\n");
    ok( sz == 16, "size wrong\n");
    sz=100;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
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
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed with empty buffer\n");
    ok( sz == 24, "size wrong\n");
    ok( 0 == strcmp(buffer,"1:  2:  3:  4:  5:  6:  "), "wrong output\n");


    /* format a format string with everything else empty */
    r = MsiRecordSetStringA(hrec, 0, "%1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, NULL, NULL );
    ok( r == ERROR_SUCCESS, "format failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, NULL);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error\n");

    sz = 123;
    r = MsiFormatRecordA(0, hrec, NULL, &sz);
    ok( r == ERROR_SUCCESS, "format failed with empty buffer\n");
    ok( sz == 2, "size wrong (%lu)\n",sz);
    sz = sizeof buffer;
    buffer[0] = 'x';
    buffer[1] = 0;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed with empty buffer\n");
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer,"%1"), "wrong output\n");

    /* make the buffer too small */
    sz = 0;
    buffer[0] = 'x';
    buffer[1] = 0;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_MORE_DATA, "format failed with empty buffer\n");
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer,"x"), "wrong output\n");

    /* make the buffer a little bit bigger */
    sz = 1;
    buffer[0] = 'x';
    buffer[1] = 0;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_MORE_DATA, "format failed with empty buffer\n");
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output (%s)\n",buffer);

    /* and again */
    sz = 2;
    buffer[0] = 'x';
    buffer[1] = 0;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_MORE_DATA, "format failed with empty buffer\n");
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer,"%"), "wrong output\n");

    /* and again */
    sz = 3;
    buffer[0] = 'x';
    buffer[1] = 0;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed with empty buffer\n");
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer,"%1"), "wrong output\n");

    /* now try a real format string */
    r = MsiRecordSetStringA(hrec, 0, "[1]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");

    /* now put something in the first field */
    r = MsiRecordSetStringA(hrec, 1, "boo hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo hoo"), "wrong output\n");

    /* now put something in the first field */
    r = MsiRecordSetStringA(hrec, 0, "[1] [2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "boo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo hoo"), "wrong output\n");

    /* self-referential format field */
    r = MsiRecordSetStringA(hrec, 0, "[1] test [0]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 21, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo test [1] test [0]"), "wrong output\n");

    /* empty string */
    r = MsiRecordSetStringA(hrec, 0, "");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 30, "size wrong %lu\n",sz);
    ok( 0 == strcmp(buffer,"1: boo 2: hoo 3:  4:  5:  6:  "), 
                    "wrong output(%s)\n",buffer);

    /* empty string with numbers */
    r = MsiRecordSetStringA(hrec, 1, "123");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetInteger(hrec, 2, 4567);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 31, "size wrong %lu\n",sz);
    ok( 0 == strcmp(buffer,"1: 123 2: 4567 3:  4:  5:  6:  "),
                    "wrong output(%s)\n",buffer);

    /* play games with recursive lookups */
    r = MsiRecordSetStringA(hrec, 0, "[[1]] [2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hey");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"hey hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "[[1]] [2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "[2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hey");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 9, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"[[2]] hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "[[[3]]] [2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hey");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"hey hey"), "wrong output (%s)\n",buffer);

    r = MsiCloseHandle(hrec);
    ok(r==ERROR_SUCCESS, "Unable to close record\n");
    hrec = MsiCreateRecord(12);
    ok( hrec, "failed to create record\n");

    r = MsiRecordSetStringA(hrec, 0, "[[3][1]] [2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hey");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"big hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "[[3][4][1]] [2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hey");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 4, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"big hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "[[3][[4]][1]] [2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hey");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 4, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 10, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"[1[]2] hey"), "wrong output (%s)\n",buffer);

    /* incorrect  formats */
    r = MsiRecordSetStringA(hrec, 0, "[[[3][[4]][1]] [2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hey");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 4, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 18, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"[[[3][[4]][1]] [2]"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "[[3][[4]][1]] [2]]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hey");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 4, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 11, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"[1[]2] hey]"), "wrong output (%s)\n",buffer);


    /* play games with {} */

    r = MsiRecordSetStringA(hrec, 0, "{[3][1]} [2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hey");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 4, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 6, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"12 hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "[{[3][1]}] [2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hey");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 4, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 8, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"[12] hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "{test} [2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hey");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 4, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 10, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"{test} hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "{[test]} [2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hey");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 4, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 12, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"{[test]} hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "{[1][2][3][4]} [2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hey");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 4, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 4, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer," hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "{[1][2][3][dummy]} [2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hey");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 4, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 18, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"{2hey1[dummy]} hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "{[1][2][3][4][dummy]} [2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hey");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 4, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 18, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"{2hey1[dummy]} hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "{{[1][2]}[3][4][dummy]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hey");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 4, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 16, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"{{2hey}1[dummy]}"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "{{[1][2]}[3]{[4][dummy]}}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hey");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 4, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer,""), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "{{[1][2]}[3]} {[1][2]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "3");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 4, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 12, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"{{12}3} {12}"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "{[1][2]} {{[1][2]}[3]} {[1][2]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "3");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 4, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 15, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"12 {{12}3} {12}"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "{[4]}{[1][2]} {{[1][2]}[3]} {[1][2]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "3");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 4, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 15, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"12 {{12}3} {12}"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "{blah} {[4]}{[1][2]} {{[1][2]}[3]} {[1][2]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "3");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 4, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 22, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"{blah} 12 {{12}3} {12}"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "{{[1]}[2]} {[4]}{[1][2]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "3");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 4, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 13, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"{{1}2} {}{12}"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "{{[1]}} {[4]}{[1][2]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "3");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 4, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 3, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer," 12"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "{{{[1]}} {[4]}{[1][2]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "3");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 4, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 12, "big");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    todo_wine{
    ok( sz == 3, "size wrong,(%lu)\n",sz);
    ok( 0 == strcmp(buffer," 12"), "wrong output (%s)\n",buffer);
    }

    /* now put play games with escaping */
    r = MsiRecordSetStringA(hrec, 0, "[1] [2] [\\3asdf]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "boo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 16, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo hoo [\\3asdf]"), "wrong output\n");

    /* now put play games with escaping */
    r = MsiRecordSetStringA(hrec, 0, "[1] [2] [\\x]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "boo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 12, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo hoo [\\x]"), "wrong output\n");

    r = MsiRecordSetStringA(hrec, 0, "[\\x]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 4, "size wrong: %lu\n", sz);
    ok( 0 == strcmp(buffer,"[\\x]"), "wrong output: %s\n", buffer);

    r = MsiRecordSetStringA(hrec, 0, "{\\x}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 4, "size wrong: %lu\n", sz);
    ok( 0 == strcmp(buffer,"{\\x}"), "wrong output: %s\n", buffer);

    r = MsiRecordSetStringA(hrec, 0, "[abc\\x]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong: %lu\n", sz);
    ok( 0 == strcmp(buffer,"[abc\\x]"), "wrong output: %s\n", buffer);

    r = MsiRecordSetStringA(hrec, 0, "[\\[]Bracket Text[\\]]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 20, "size wrong: %lu\n", sz);
    ok( 0 == strcmp(buffer,"[\\[]Bracket Text[\\]]"), "wrong output: %s\n", buffer);

    /* now try other formats without a package */
    r = MsiRecordSetStringA(hrec, 0, "[1] [2] [property]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "boo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 18, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo hoo [property]"), "wrong output\n");

    r = MsiRecordSetStringA(hrec, 0, "[1] [~] [2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "boo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 11, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo [~] hoo"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "[1]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetInteger(hrec, 1, 123456);
    ok( r == ERROR_SUCCESS, "set integer failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 6, "size wrong\n");
    ok( 0 == strcmp(buffer,"123456"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "[~]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"[~]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "[]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer,"[]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    /* MsiFormatRecordA doesn't seem to handle a negative too well */
    r = MsiRecordSetStringA(hrec, 0, "[-1]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 4, "size wrong\n");
    ok( 0 == strcmp(buffer,"[-1]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{[]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 4, "size wrong\n");
    ok( 0 == strcmp(buffer,"{[]}"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "[0]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"[0]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "[100]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{[1] [2]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "boo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 7, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo hoo"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{foo}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 5, "size wrong\n");
    ok( 0 == strcmp(buffer,"{foo}"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{boo [1]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 7, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo hoo"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{{[1]}}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{ {[1]}}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( 0 == strcmp(buffer," {hoo}"), "wrong output\n");
        ok( sz == 6, "size wrong\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{{[1]} }");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 8, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{hoo} }"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{{ [1]}}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{{[1] }}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{{a}{b}{c }{ d}{any text}}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{{a} }");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 6, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{a} }"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{{a} {b}}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{{a} b}}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    todo_wine ok( sz == 0, "size wrong\n");
    todo_wine ok( 0 == strcmp(buffer,""), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{{a b}}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{ }");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"{ }"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, " {{a}}}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer," }"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{{ almost {{ any }} text }}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    todo_wine ok( sz == 8, "size wrong\n");
    todo_wine ok( 0 == strcmp(buffer," text }}"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{{ } { hidden ][ [ }}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    todo_wine ok( sz == 0, "size wrong\n");
    todo_wine ok( 0 == strcmp(buffer,""), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "[ 1]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 4, "size wrong\n");
    ok( 0 == strcmp(buffer,"[ 1]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "[01]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"hoo"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{{test}} [01");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    todo_wine ok( sz == 4, "size wrong\n");
    todo_wine ok( 0 == strcmp(buffer," [01"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "[\\[]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 4, "size wrong\n");
    ok( 0 == strcmp(buffer,"[\\[]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "[\\[]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof(buffer);
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 4, "Expected 4, got %lu\n", sz);
    ok(!lstrcmpA(buffer, "[\\[]"), "Expected \"[\\[]\", got \"%s\"\n", buffer);

    r = MsiRecordSetStringA(hrec, 0, "[foo]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 5, "size wrong\n");
    ok( 0 == strcmp(buffer,"[foo]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "[01.]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 5, "size wrong\n");
    ok( 0 == strcmp(buffer,"[01.]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    SetEnvironmentVariableA("FOO", "BAR");
    r = MsiRecordSetStringA(hrec, 0, "[%FOO]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 6, "size wrong\n");
    ok( 0 == strcmp(buffer,"[%FOO]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{{[1]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 6, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{hoo}"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{{ {[1]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 8, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{ {hoo}"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{{ {[1]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 8, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{ {hoo}"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{{ {{[1]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 9, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{ {{hoo}"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "[1]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 4, "size wrong\n");
    ok( 0 == strcmp(buffer,"hoo}"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{{ {{a}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 7, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{ {{a}"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{{ {{a}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 7, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{ {{a}"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "0{1{2{3{4[1]5}6}7}8}9");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 19, "size wrong\n");
        ok( 0 == strcmp(buffer,"01{2{3{4hoo56}7}8}9"), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "0{1{2[1]3}4");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 9, "size wrong\n");
        ok( 0 == strcmp(buffer,"01{2hoo34"), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "0{1{2[1]3}4");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 9, "size wrong\n");
        ok( 0 == strcmp(buffer,"01{2hoo34"), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{[1.} [1]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 9, "size wrong\n");
    todo_wine
    {
        ok( 0 == strcmp(buffer,"{[1.} hoo"), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{[{[1]}]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "foo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 9, "size wrong\n");
        ok( 0 == strcmp(buffer,"{[{[1]}]}"), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{[1][}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "foo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 2, "size wrong\n");
        ok( 0 == strcmp(buffer,"2["), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "[1]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "[2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "foo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"[2]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "[{{boo}}1]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 3, "size wrong\n");
    todo_wine
    {
        ok( 0 == strcmp(buffer,"[1]"), "wrong output: %s\n", buffer);
    }

    r = MsiRecordSetStringA(hrec, 0, "[{{boo}}1]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 0, "[1{{boo}}]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"[1]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{[1]{{boo} }}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 11, "size wrong\n");
        ok( 0 == strcmp(buffer,"hoo{{boo }}"), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{[1{{boo}}]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 12, "size wrong: got %lu, expected 12\n", sz);
        ok( 0 == strcmp(buffer,"{[1{{boo}}]}"), "wrong output: got %s, expected [1]\n", buffer);
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{{[1]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 6, "size wrong: got %lu, expected 3\n", sz);
    ok( 0 == strcmp(buffer,"{{hoo}"), "wrong output: got %s, expected [1]\n", buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{[1{{bo}o}}]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 13, "size wrong\n");
    ok( 0 == strcmp(buffer,"{[1{{bo}o}}]}"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{[1{{b{o}o}}]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 14, "size wrong\n");
        ok( 0 == strcmp(buffer,"{[1{{b{o}o}}]}"), "wrong output %s\n",buffer);
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{ {[1]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 5, "size wrong\n");
        ok( 0 == strcmp(buffer," {hoo"), "wrong output %s\n",buffer);
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    /* {} inside a substitution does strange things... */
    r = MsiRecordSetStringA(hrec, 0, "[[1]{}]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 5, "size wrong\n");
        ok( 0 == strcmp(buffer,"[[1]]"), "wrong output %s\n",buffer);
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "[[1]{}[1]]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 6, "size wrong\n");
        ok( 0 == strcmp(buffer,"[[1]2]"), "wrong output %s\n",buffer);
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "[a[1]b[1]c{}d[1]e]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 14, "size wrong\n");
        ok( 0 == strcmp(buffer,"[a[1]b[1]cd2e]"), "wrong output %s\n",buffer);
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "[a[1]b");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 6, "size wrong\n");
    ok( 0 == strcmp(buffer,"[a[1]b"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "a[1]b]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 4, "size wrong\n");
    ok( 0 == strcmp(buffer,"a2b]"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "]a[1]b");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 4, "size wrong\n");
    ok( 0 == strcmp(buffer,"]a2b"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "]a[1]b");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 4, "size wrong\n");
    ok( 0 == strcmp(buffer,"]a2b"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "\\[1]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer,"\\2"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "\\{[1]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "2");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer,"\\2"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "a{b[1]c}d");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer,"ad"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{a[0]b}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "foo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 9, "size wrong\n");
    ok( 0 == strcmp(buffer,"a{a[0]b}b"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "[foo]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( sz == 5, "size wrong\n");
    ok( 0 == strcmp(buffer,"[foo]"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec, 0, "{[1][-1][1]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "foo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    todo_wine
    {
        ok( sz == 12, "size wrong\n");
        ok( 0 == strcmp(buffer,"{foo[-1]foo}"), "wrong output %s\n",buffer);
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    /* nested braces */
    sz = sizeof buffer;
    r = MsiRecordSetStringA(hrec, 0, "{abcd}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 6, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"{abcd}"), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    r = MsiRecordSetStringA(hrec, 0, "{a[one]bc[two]de[one]f}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 23, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"{a[one]bc[two]de[one]f}"), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    r = MsiRecordSetStringA(hrec, 0, "{a[one]bc[bad]de[two]f}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 23, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"{a[one]bc[bad]de[two]f}"), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    r = MsiRecordSetStringA(hrec, 0, "{[bad]}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"{[bad]}"), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    r = MsiRecordSetStringA(hrec, 0, "{abc{d[one]ef}"); /* missing final brace */
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 14, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"{abc{d[one]ef}"), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    r = MsiRecordSetStringA(hrec, 0, "{abc{d[one]ef}}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 15, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"{abc{d[one]ef}}"), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    r = MsiRecordSetStringA(hrec, 0, "{abc}{{def}hi{j[one]k}}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 5, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"{abc}"), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    r = MsiRecordSetStringA(hrec, 0, "{{def}hi{j[one]k}}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,""), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    r = MsiRecordSetStringA(hrec, 0, "{{def}hi{jk}}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,""), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    r = MsiRecordSetStringA(hrec, 0, "{{{def}}hi{jk}}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    todo_wine
    {
        ok( sz == 7, "size wrong(%lu)\n",sz);
        ok( 0 == strcmp(buffer,"hi{jk}}"), "wrong output (%s)\n",buffer);
    }

    sz = sizeof buffer;
    r = MsiRecordSetStringA(hrec, 0, "{{def}hi{{jk}}}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    todo_wine
    {
        ok( sz == 1, "size wrong(%lu)\n",sz);
        ok( 0 == strcmp(buffer,"}"), "wrong output (%s)\n",buffer);
    }

    sz = sizeof buffer;
    r = MsiRecordSetStringA(hrec, 0, "{{def}{jk}}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,""), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    r = MsiRecordSetStringA(hrec, 0, "{{def}}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,""), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    r = MsiRecordSetStringA(hrec, 0, "{a{b}c}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"{a{b}c}"), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    r = MsiRecordSetStringA(hrec, 0, "{a{b}}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 6, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"{a{b}}"), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    r = MsiRecordSetStringA(hrec, 0, "{{b}c}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 6, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"{{b}c}"), "wrong output (%s)\n",buffer);

    sz = sizeof buffer;
    r = MsiRecordSetStringA(hrec, 0, "{{{{}}}}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    todo_wine
    {
        ok( sz == 2, "size wrong(%lu)\n",sz);
        ok( 0 == strcmp(buffer,"}}"), "wrong output (%s)\n",buffer);
    }

    sz = sizeof buffer;
    MsiRecordSetInteger(hrec, 1, 100);
    MsiRecordSetInteger(hrec, 2, -100);
    r = MsiRecordSetStringA(hrec, 0, "[1] [2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 8, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"100 -100"), "wrong output (%s)\n",buffer);

    sz = sizeof(buffer);
    r = MsiRecordSetStringA(hrec, 0, "[1] {[noprop] [twoprop]} {abcdef}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "one");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 33, "Expected 33, got %lu\n",sz);
    ok(!lstrcmpA(buffer, "one {[noprop] [twoprop]} {abcdef}"),
       "Expected \"one {[noprop] [twoprop]} {abcdef}\", got \"%s\"\n", buffer);

    sz = sizeof(buffer);
    r = MsiRecordSetStringA(hrec, 0, "[1] {[noprop] [one]} {abcdef}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "one");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 29, "Expected 29, got %lu\n",sz);
    ok(!lstrcmpA(buffer, "one {[noprop] [one]} {abcdef}"),
       "Expected \"one {[noprop] [one]} {abcdef}\", got \"%s\"\n", buffer);

    sz = sizeof(buffer);
    r = MsiRecordSetStringA(hrec, 0, "[1] {[one]} {abcdef}");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "one");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiFormatRecordA(0, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 20, "Expected 20, got %lu\n",sz);
    ok(!lstrcmpA(buffer, "one {[one]} {abcdef}"),
       "Expected \"one {[one]} {abcdef}\", got \"%s\"\n", buffer);

    MsiCloseHandle( hrec );
}

static void test_formatrecord_package(void)
{
    char buffer[100];
    MSIHANDLE hrec;
    MSIHANDLE package;
    UINT r;
    DWORD sz=100;

    r = helper_createpackage( msifile, &package );
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        return;
    }
    ok( r == ERROR_SUCCESS, "Unable to create package %u\n", r );

    hrec = MsiCreateRecord(12);
    ok( hrec, "failed to create record\n");

    r = MsiFormatRecordA(package, 0, NULL, NULL );
    ok( r == ERROR_INVALID_HANDLE, "wrong error\n");

    r = MsiFormatRecordA(package, hrec, NULL, NULL );
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetStringA(hrec,0,NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec,1,NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec,2,NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec,3,NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec,4,NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec,5,NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec,6,NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec,7,NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec,8,NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec,9,NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec,10,NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec,11,NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec,12,NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");

    sz = sizeof buffer;
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed with empty buffer (%i)\n",r);
    ok( sz == 51, "size wrong (%lu)\n",sz);
    ok( 0 == strcmp(buffer,"1:  2:  3:  4:  5:  6:  7:  8:  9:  10:  11:  12:  "), "wrong output(%s)\n",buffer);

    r = MsiSetPropertyA(package, "prop", "val");
    ok( r == ERROR_SUCCESS, "failed to set property: %d\n", r);

    r = MsiRecordSetStringA(hrec, 0, NULL);
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "[2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "stuff");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "prop");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 4, "[prop]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 5, "[noprop]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed with empty buffer (%i)\n",r);
    todo_wine
    {
        ok( sz == 66, "size wrong (%lu)\n",sz);
        ok( !lstrcmpA(buffer,
            "1: [2] 2: stuff 3: prop 4: val 5:  6:  7:  8:  9:  10:  11:  12:  "),
            "wrong output(%s)\n",buffer);
    }

    /* now put play games with escaping */
    r = MsiRecordSetStringA(hrec, 0, "[1] [2] [\\3asdf]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "boo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 9, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"boo hoo 3"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "[1] [2] [\\x]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "boo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 9, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"boo hoo x"), "wrong output (%s)\n",buffer);

    MsiRecordSetStringA(hrec, 0, "[\\x]");
    sz = sizeof buffer;
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 1, "size wrong: %lu\n", sz);
    ok( 0 == strcmp(buffer,"x"), "wrong output: %s\n", buffer);

    MsiRecordSetStringA(hrec, 0, "{\\x}");
    sz = sizeof buffer;
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 4, "size wrong: %lu\n", sz);
    ok( 0 == strcmp(buffer,"{\\x}"), "wrong output: %s\n", buffer);

    MsiRecordSetStringA(hrec, 0, "[abc\\x]");
    sz = sizeof buffer;
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong: %lu\n", sz);
    ok( 0 == strcmp(buffer,""), "wrong output: %s\n", buffer);

    MsiRecordSetStringA(hrec, 0, "[abc\\xdef]");
    sz = sizeof buffer;
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong: %lu\n", sz);
    ok( 0 == strcmp(buffer,""), "wrong output: %s\n", buffer);

    MsiRecordSetStringA(hrec, 0, "\\x");
    sz = sizeof(buffer);
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 2, "Expected 2, got %lu\n", sz);
    ok(!lstrcmpA(buffer, "\\x"), "Expected \"\\x\", got \"%s\"\n", buffer);

    MsiRecordSetStringA(hrec, 0, "[\\[");
    sz = sizeof(buffer);
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 3, "Expected 3, got %lu\n", sz);
    ok(!lstrcmpA(buffer, "[\\["), "Expected \"[\\[\", got \"%s\"\n", buffer);

    MsiRecordSetStringA(hrec, 0, "[\\[]");
    sz = sizeof(buffer);
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 1, "Expected 1, got %lu\n", sz);
    ok(!lstrcmpA(buffer, "["), "Expected \"[\", got \"%s\"\n", buffer);

    MsiRecordSetStringA(hrec, 0, "[[]");
    sz = sizeof(buffer);
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 3, "Expected 3, got %lu\n", sz);
    ok(!lstrcmpA(buffer, "[[]"), "Expected \"[]\", got \"%s\"\n", buffer);

    MsiRecordSetStringA(hrec, 0, "[\\[]]");
    sz = sizeof(buffer);
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 2, "Expected 2, got %lu\n", sz);
    ok(!lstrcmpA(buffer, "[]"), "Expected \"[]\", got \"%s\"\n", buffer);

    MsiRecordSetStringA(hrec, 0, "[\\[a]");
    sz = sizeof(buffer);
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 1, "Expected 1, got %lu\n", sz);
    ok(!lstrcmpA(buffer, "["), "Expected \"[\", got \"%s\"\n", buffer);

    MsiRecordSetStringA(hrec, 0, "[\\a[]");
    sz = sizeof(buffer);
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    todo_wine
    {
        ok(sz == 1, "Expected 1, got %lu\n", sz);
        ok(!lstrcmpA(buffer, "a"), "Expected \"a\", got \"%s\"\n", buffer);
    }

    MsiRecordSetStringA(hrec, 0, "[prop]");
    sz = sizeof(buffer);
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 3, "Expected 3, got %lu\n", sz);
    ok(!lstrcmpA(buffer, "val"), "Expected \"val\", got \"%s\"\n", buffer);

    MsiRecordSetStringA(hrec, 0, "[prop] [pro\\pblah] [prop]");
    sz = sizeof(buffer);
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 8, "Expected 8, got %lu\n", sz);
    ok(!lstrcmpA(buffer, "val  val"), "Expected \"val  val\", got \"%s\"\n", buffer);

    MsiSetPropertyA(package, "b", "ball");
    MsiRecordSetStringA(hrec, 0, "[\\b]");
    sz = sizeof(buffer);
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 1, "Expected 1, got %lu\n", sz);
    ok(!lstrcmpA(buffer, "b"), "Expected \"b\", got \"%s\"\n", buffer);

    MsiRecordSetStringA(hrec, 0, "[\\c]");
    sz = sizeof(buffer);
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 1, "Expected 1, got %lu\n", sz);
    ok(!lstrcmpA(buffer, "c"), "Expected \"c\", got \"%s\"\n", buffer);

    MsiRecordSetStringA(hrec, 0, "[\\[]prop]");
    sz = sizeof(buffer);
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 6, "Expected 6, got %lu\n", sz);
    ok(!lstrcmpA(buffer, "[prop]"), "Expected \"[prop]\", got \"%s\"\n", buffer);

    MsiRecordSetStringA(hrec, 0, "[\\a]prop]");
    sz = sizeof(buffer);
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 6, "Expected 6, got %lu\n", sz);
    ok(!lstrcmpA(buffer, "aprop]"), "Expected \"aprop]\", got \"%s\"\n", buffer);

    MsiRecordSetStringA(hrec, 0, "[\\[]Bracket Text[\\]]");
    sz = sizeof buffer;
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 14, "size wrong: %lu\n", sz);
    ok( 0 == strcmp(buffer,"[Bracket Text]"), "wrong output: %s\n", buffer);

    /* null characters */
    r = MsiRecordSetStringA(hrec, 0, "[1] [~] [2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "boo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 9, "size wrong: %lu\n", sz);
    ok( 0 == strcmp(buffer,"boo "), "wrong output: %s\n", buffer);
    ok(!lstrcmpA(&buffer[5], " hoo"),
       "Expected \" hoo\", got \"%s\"\n", &buffer[5]);

    r = MsiRecordSetStringA(hrec, 0, "[1] [~abc] [2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "boo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof(buffer);
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 8, "Expected 8, got %lu\n", sz);
    ok(!lstrcmpA(buffer, "boo  hoo"), "Expected \"boo  hoo\", got \"%s\"\n", buffer);

    /* properties */
    r = MsiSetPropertyA(package,"dummy","Bork");
    ok( r == ERROR_SUCCESS, "set property failed\n");
    r = MsiRecordSetStringA(hrec, 0, "[1] [dummy] [2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "boo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 12, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo Bork hoo"), "wrong output\n");

    r = MsiRecordSetStringA(hrec, 0, "[1] [invalid] [2]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "boo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 8, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo  hoo"), "wrong output\n");

    /* nesting tests */
    r = MsiSetPropertyA(package,"dummya","foo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiSetPropertyA(package,"dummyb","baa");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiSetPropertyA(package,"adummyc","whoa");
    ok( r == ERROR_SUCCESS, "set property failed\n");
    r = MsiRecordSetStringA(hrec, 0, "[dummy[1]] [dummy[2]] [[1]dummy[3]]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "a");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "b");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "c");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 12, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"foo baa whoa"), "wrong output (%s)\n",buffer);

    r = MsiSetPropertyA(package,"dummya","1");
    ok( r == ERROR_SUCCESS, "set property failed\n");
    r = MsiSetPropertyA(package,"dummyb","[2]");
    ok( r == ERROR_SUCCESS, "set property failed\n");
    r = MsiRecordSetStringA(hrec, 0, "[dummya] [[dummya]] [dummyb]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "aaa");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "bbb");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "ccc");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    todo_wine
    {
        ok( sz == 9, "size wrong(%lu)\n",sz);
        ok( 0 == strcmp(buffer,"1 [1] [2]"), "wrong output (%s)\n",buffer);
    }

    r = MsiSetPropertyA(package,"dummya","1");
    ok( r == ERROR_SUCCESS, "set property failed\n");
    r = MsiSetPropertyA(package,"dummyb","a");
    ok( r == ERROR_SUCCESS, "set property failed\n");
    r = MsiSetPropertyA(package,"dummyc","\\blath");
    ok( r == ERROR_SUCCESS, "set property failed\n");
    r = MsiSetPropertyA(package,"dummyd","[\\blath]");
    ok( r == ERROR_SUCCESS, "set property failed\n");
    r = MsiRecordSetStringA(hrec, 0, "[dummyc] [[dummyc]] [dummy[dummyb]]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "aaa");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "bbb");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "ccc");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 10, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"\\blath b 1"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetStringA(hrec, 0, "[1] [2] [[\\3asdf]]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "boo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "yeah");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    todo_wine
    {
        ok( sz == 11, "size wrong(%lu)\n",sz);
        ok( 0 == strcmp(buffer,"boo hoo [3]"), "wrong output (%s)\n",buffer);
    }

    r = MsiRecordSetStringA(hrec, 0, "[1] [2] [[3]]");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 1, "boo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    r = MsiRecordSetStringA(hrec, 3, "\\help");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 9, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"boo hoo h"), "wrong output (%s)\n",buffer);

    /* nested properties */
    MsiSetPropertyA(package, "PropA", "surprise");
    MsiSetPropertyA(package, "PropB", "[PropA]");
    MsiSetPropertyA(package, "PropC", "[PropB]");
    sz = sizeof buffer;
    MsiRecordSetStringA(hrec, 0, "[PropC]");
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"[PropB]"), "wrong output (%s)\n",buffer);

    MsiSetPropertyA(package, "PropA", "surprise");
    MsiSetPropertyA(package, "PropB", "PropA");
    MsiSetPropertyA(package, "PropC", "PropB");
    sz = sizeof buffer;
    MsiRecordSetStringA(hrec, 0, "[PropC]");
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 5, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"PropB"), "wrong output (%s)\n",buffer);

    MsiSetPropertyA(package, "PropA", "surprise");
    MsiSetPropertyA(package, "PropB", "[PropA]");
    MsiSetPropertyA(package, "PropC", "[PropB]");
    sz = sizeof buffer;
    MsiRecordSetStringA(hrec, 0, "[[PropC]]");
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,""), "wrong output (%s)\n",buffer);

    MsiSetPropertyA(package, "PropA", "surprise");
    MsiSetPropertyA(package, "PropB", "[PropA]");
    MsiSetPropertyA(package, "PropC", "PropB");
    sz = sizeof buffer;
    MsiRecordSetStringA(hrec, 0, "[[PropC]]");
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"[PropA]"), "wrong output (%s)\n",buffer);

    MsiSetPropertyA(package, "PropA", "surprise");
    MsiSetPropertyA(package, "PropB", "PropA");
    MsiSetPropertyA(package, "PropC", "PropB");
    sz = sizeof buffer;
    MsiRecordSetStringA(hrec, 0, "[[PropC]]");
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 5, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"PropA"), "wrong output (%s)\n",buffer);

    MsiSetPropertyA(package, "PropA", "surprise");
    MsiSetPropertyA(package, "PropB", "PropA");
    MsiSetPropertyA(package, "PropC", "PropB");
    sz = sizeof buffer;
    MsiRecordSetStringA(hrec, 0, "[[[PropC]]]");
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 8, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"surprise"), "wrong output (%s)\n",buffer);

    /* properties inside braces */
    sz = sizeof buffer;
    MsiRecordSetStringA(hrec, 0, "{abcd}");
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 6, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"{abcd}"), "wrong output (%s)\n",buffer);

    MsiSetPropertyA(package, "one", "mercury");
    MsiSetPropertyA(package, "two", "venus");
    sz = sizeof buffer;
    MsiRecordSetStringA(hrec, 0, "{a[one]bc[two]de[one]f}");
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed: %d\n", r);
    ok( sz == 25, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"amercurybcvenusdemercuryf"), "wrong output (%s)\n",buffer);

    MsiSetPropertyA(package, "one", "mercury");
    MsiSetPropertyA(package, "two", "venus");
    MsiSetPropertyA(package, "bad", "");
    sz = sizeof buffer;
    MsiRecordSetStringA(hrec, 0, "{a[one]bc[bad]de[two]f}");
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,""), "wrong output (%s)\n",buffer);

    MsiSetPropertyA(package, "bad", "");
    sz = sizeof buffer;
    MsiRecordSetStringA(hrec, 0, "{[bad]}");
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,""), "wrong output (%s)\n",buffer);

    MsiSetPropertyA(package, "one", "mercury");
    sz = sizeof buffer;
    MsiRecordSetStringA(hrec, 0, "{abc{d[one]ef}"); /* missing final brace */
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    todo_wine
    {
        ok( sz == 14, "size wrong(%lu)\n",sz);
        ok( 0 == strcmp(buffer,"abc{dmercuryef"), "wrong output (%s)\n",buffer);
    }

    MsiSetPropertyA(package, "one", "mercury");
    sz = sizeof buffer;
    MsiRecordSetStringA(hrec, 0, "{abc{d[one]ef}}");
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    todo_wine
    {
        ok( sz == 15, "size wrong(%lu)\n",sz);
        ok( 0 == strcmp(buffer,"abc{dmercuryef}"), "wrong output (%s)\n",buffer);
    }

    MsiSetPropertyA(package, "one", "mercury");
    sz = sizeof buffer;
    MsiRecordSetStringA(hrec, 0, "{abc}{{def}hi{j[one]k}}");
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 5, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,"{abc}"), "wrong output (%s)\n",buffer);

    MsiSetPropertyA(package, "one", "mercury");

    sz = sizeof buffer;
    MsiRecordSetStringA(hrec, 0, "{{def}hi{j[one]k}}");
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 0, "size wrong(%lu)\n",sz);
    ok( 0 == strcmp(buffer,""), "wrong output (%s)\n",buffer);

    sz = sizeof(buffer);
    MsiRecordSetStringA(hrec, 0, "[1] {[noprop] [twoprop]} {abcdef}");
    MsiRecordSetStringA(hrec, 1, "one");
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 13, "Expected 13, got %lu\n",sz);
    ok(!lstrcmpA(buffer, "one  {abcdef}"),
       "Expected \"one  {abcdef}\", got \"%s\"\n", buffer);

    sz = sizeof(buffer);
    MsiRecordSetStringA(hrec, 0, "[1] {[noprop] [one]} {abcdef}");
    MsiRecordSetStringA(hrec, 1, "one");
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 13, "Expected 13, got %lu\n",sz);
    ok(!lstrcmpA(buffer, "one  {abcdef}"),
       "Expected \"one  {abcdef}\", got \"%s\"\n", buffer);

    sz = sizeof(buffer);
    MsiRecordSetStringA(hrec, 0, "[1] {[one]} {abcdef}");
    MsiRecordSetStringA(hrec, 1, "one");
    r = MsiFormatRecordA(package, hrec, buffer, &sz);
    ok(r == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %d\n", r);
    ok(sz == 20, "Expected 20, got %lu\n",sz);
    ok(!lstrcmpA(buffer, "one mercury {abcdef}"),
       "Expected \"one mercury {abcdef}\", got \"%s\"\n", buffer);

    MsiCloseHandle(hrec);

    r = MsiCloseHandle(package);
    ok(r==ERROR_SUCCESS, "Unable to close package\n");

    DeleteFileA( msifile );
}

static void test_processmessage(void)
{
    MSIHANDLE hrec, package;
    UINT r;

    MsiSetInternalUI(INSTALLUILEVEL_BASIC, NULL);

    r = helper_createpackage( msifile, &package );
    if (r == ERROR_INSTALL_PACKAGE_REJECTED)
    {
        skip("Not enough rights to perform tests\n");
        return;
    }
    ok( r == ERROR_SUCCESS, "Unable to create package %u\n", r );

    hrec = MsiCreateRecord(3);
    ok( hrec, "failed to create record\n");

    r = MsiRecordSetStringA(hrec, 1, "");
    ok( r == ERROR_SUCCESS, "set string failed\n");

    r = MsiProcessMessage(package, INSTALLMESSAGE_ACTIONSTART, hrec);
    ok( r == IDOK, "expected IDOK, got %i\n", r);

    r = MsiProcessMessage(package, INSTALLMESSAGE_PROGRESS, hrec);
    ok( r == IDOK, "expected IDOK, got %i\n", r);

    r = MsiProcessMessage(package, INSTALLMESSAGE_INFO, hrec);
    ok( r == 0, "expected 0, got %i\n", r);

    r = MsiProcessMessage(package, INSTALLMESSAGE_INITIALIZE, hrec);
    ok( r == -1, "expected -1, got %i\n", r);

    r = MsiRecordSetInteger(hrec, 1, 2);
    ok( r == ERROR_SUCCESS, "set integer failed\n");
    r = MsiRecordSetInteger(hrec, 2, 1);
    ok( r == ERROR_SUCCESS, "set integer failed\n");
    r = MsiProcessMessage(package, INSTALLMESSAGE_COMMONDATA, hrec);
    todo_wine
    ok( r == IDOK, "expected IDOK, got %i\n", r);

    r = MsiRecordSetInteger(hrec, 2, 2);
    ok( r == ERROR_SUCCESS, "set integer failed\n");
    r = MsiProcessMessage(package, INSTALLMESSAGE_COMMONDATA, hrec);
    todo_wine
    ok( r == IDOK, "expected IDOK, got %i\n", r);

    r = MsiRecordSetInteger(hrec, 1, 1);
    ok( r == ERROR_SUCCESS, "set integer failed\n");
    r = MsiProcessMessage(package, INSTALLMESSAGE_COMMONDATA, hrec);
    ok( r == -1, "expected -1, got %i\n", r);

    MsiCloseHandle(package);

    MsiSetInternalUI(INSTALLUILEVEL_BASIC|INSTALLUILEVEL_PROGRESSONLY, NULL);
    helper_createpackage(msifile, &package);

    r = MsiProcessMessage(package, INSTALLMESSAGE_ERROR, hrec);
    ok( r == 0, "expected 0, got %i\n", r);

    MsiCloseHandle(hrec);
    MsiCloseHandle(package);

    DeleteFileA(msifile);
}

START_TEST(format)
{
    test_createpackage();
    test_formatrecord();
    test_formatrecord_package();
    test_processmessage();
}
