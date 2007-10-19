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
#include <msi.h>
#include <msiquery.h>

#include "wine/test.h"

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

    sprintf(szPackage,"#%li",(DWORD)hdb);
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
    ok( sz == 2, "size wrong (%li)\n",sz);
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
    ok( sz == 30, "size wrong %li\n",sz);
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
    ok( sz == 7, "size wrong,(%li)\n",sz);
    ok( 0 == strcmp(buffer,"hey hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "[[1]] [2]");
    r = MsiRecordSetString(hrec, 1, "[2]");
    r = MsiRecordSetString(hrec, 2, "hey");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 9, "size wrong,(%li)\n",sz);
    ok( 0 == strcmp(buffer,"[[2]] hey"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "[[[3]]] [2]");
    r = MsiRecordSetString(hrec, 1, "2");
    r = MsiRecordSetString(hrec, 2, "hey");
    r = MsiRecordSetString(hrec, 3, "1");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 7, "size wrong,(%li)\n",sz);
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
    ok( sz == 7, "size wrong,(%li)\n",sz);
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
    ok( sz == 7, "size wrong,(%li)\n",sz);
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
    ok( sz == 10, "size wrong,(%li)\n",sz);
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
    ok( sz == 18, "size wrong,(%li)\n",sz);
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
    ok( sz == 11, "size wrong,(%li)\n",sz);
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
    ok( sz == 6, "size wrong,(%li)\n",sz);
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
    ok( sz == 8, "size wrong,(%li)\n",sz);
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
    ok( sz == 10, "size wrong,(%li)\n",sz);
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
    ok( sz == 12, "size wrong,(%li)\n",sz);
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
    ok( sz == 4, "size wrong,(%li)\n",sz);
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
    ok( sz == 18, "size wrong,(%li)\n",sz);
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
    ok( sz == 18, "size wrong,(%li)\n",sz);
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

    todo_wine{
    ok( sz == 16, "size wrong,(%li)\n",sz);
    ok( 0 == strcmp(buffer,"{{2hey}1[dummy]}"), "wrong output (%s)\n",buffer);
    }

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
    todo_wine{
    ok( sz == 0, "size wrong,(%li)\n",sz);
    ok( 0 == strcmp(buffer,""), "wrong output (%s)\n",buffer);
    }

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
    todo_wine{
    ok( sz == 12, "size wrong,(%li)\n",sz);
    ok( 0 == strcmp(buffer,"{{12}3} {12}"), "wrong output (%s)\n",buffer);
    }

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
    todo_wine{
    ok( sz == 15, "size wrong,(%li)\n",sz);
    ok( 0 == strcmp(buffer,"12 {{12}3} {12}"), "wrong output (%s)\n",buffer);
    }

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
    todo_wine{
    ok( sz == 15, "size wrong,(%li)\n",sz);
    ok( 0 == strcmp(buffer,"12 {{12}3} {12}"), "wrong output (%s)\n",buffer);
    }


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
    todo_wine{
    ok( sz == 22, "size wrong,(%li)\n",sz);
    ok( 0 == strcmp(buffer,"{blah} 12 {{12}3} {12}"), "wrong output (%s)\n",buffer);
    }

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
    todo_wine{
    ok( sz == 13, "size wrong,(%li)\n",sz);
    ok( 0 == strcmp(buffer,"{{1}2} {}{12}"), "wrong output (%s)\n",buffer);
    }

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
    todo_wine{
    ok( sz == 3, "size wrong,(%li)\n",sz);
    ok( 0 == strcmp(buffer," 12"), "wrong output (%s)\n",buffer);
    }

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
    ok( sz == 3, "size wrong,(%li)\n",sz);
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
    todo_wine{
    ok( sz == 6, "size wrong\n");
    ok( 0 == strcmp(buffer,"123456"), "wrong output (%s)\n",buffer);
    }

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

    todo_wine {
    r = MsiRecordSetString(hrec, 0, "{{[1]}}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{ {[1]}}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( 0 == strcmp(buffer," {hoo}"), "wrong output\n");
    ok( sz == 6, "size wrong\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    todo_wine {
    r = MsiRecordSetString(hrec, 0, "{{[1]} }");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 8, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{hoo} }"), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    todo_wine {
    r = MsiRecordSetString(hrec, 0, "{{ [1]}}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    todo_wine {
    r = MsiRecordSetString(hrec, 0, "{{[1] }}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    todo_wine {
    r = MsiRecordSetString(hrec, 0, "{{a}{b}{c }{ d}{any text}}");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{{a} }");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 6, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{a} }"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    todo_wine {
    r = MsiRecordSetString(hrec, 0, "{{a} {b}}");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    todo_wine {
    r = MsiRecordSetString(hrec, 0, "{{a} b}}");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    todo_wine {
    r = MsiRecordSetString(hrec, 0, "{{a b}}");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 0, "size wrong\n");
    ok( 0 == strcmp(buffer,""), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{ }");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"{ }"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    todo_wine {
    r = MsiRecordSetString(hrec, 0, " {{a}}}");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer," }"), "wrong output\n");
    }
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

    todo_wine {
    r = MsiRecordSetString(hrec, 0, "{{[1]}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 6, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{hoo}"), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    todo_wine {
    r = MsiRecordSetString(hrec, 0, "{{ {[1]}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 8, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{ {hoo}"), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    todo_wine {
    r = MsiRecordSetString(hrec, 0, "{{ {[1]}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 8, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{ {hoo}"), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    todo_wine {
    r = MsiRecordSetString(hrec, 0, "{{ {{[1]}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 9, "size wrong\n");
    ok( 0 == strcmp(buffer,"{{ {{hoo}"), "wrong output\n");
    }
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
    ok( sz == 19, "size wrong\n");
    ok( 0 == strcmp(buffer,"01{2{3{4hoo56}7}8}9"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "0{1{2[1]3}4");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 9, "size wrong\n");
    ok( 0 == strcmp(buffer,"01{2hoo34"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "0{1{2[1]3}4");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 9, "size wrong\n");
    ok( 0 == strcmp(buffer,"01{2hoo34"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{[1.} [1]");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 9, "size wrong\n");
    ok( 0 == strcmp(buffer,"{[1.} hoo"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{[{[1]}]}");
    r = MsiRecordSetString(hrec, 1, "2");
    r = MsiRecordSetString(hrec, 2, "foo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 9, "size wrong\n");
    ok( 0 == strcmp(buffer,"{[{[1]}]}"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{[1][}");
    r = MsiRecordSetString(hrec, 1, "2");
    r = MsiRecordSetString(hrec, 2, "foo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 2, "size wrong\n");
    ok( 0 == strcmp(buffer,"2["), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[1]");
    r = MsiRecordSetString(hrec, 1, "[2]");
    r = MsiRecordSetString(hrec, 2, "foo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"[2]"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    todo_wine {
    r = MsiRecordSetString(hrec, 0, "[{{boo}}1]");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"[1]"), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    todo_wine {
    r = MsiRecordSetString(hrec, 0, "[{{boo}}1]");
    r = MsiRecordSetString(hrec, 0, "[1{{boo}}]");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 3, "size wrong\n");
    ok( 0 == strcmp(buffer,"[1]"), "wrong output\n");
    }
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{[1]{{boo} }}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 11, "size wrong\n");
    ok( 0 == strcmp(buffer,"hoo{{boo }}"), "wrong output\n");
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{[1{{boo}}]}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 12, "size wrong: got %lu, expected 12\n", sz);
    ok( 0 == strcmp(buffer,"{[1{{boo}}]}"), "wrong output: got %s, expected [1]\n", buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    todo_wine {
    r = MsiRecordSetString(hrec, 0, "{{[1]}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 6, "size wrong: got %lu, expected 3\n", sz);
    ok( 0 == strcmp(buffer,"{{hoo}"), "wrong output: got %s, expected [1]\n", buffer);
    }
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
    ok( sz == 14, "size wrong\n");
    ok( 0 == strcmp(buffer,"{[1{{b{o}o}}]}"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "{ {[1]}");
    r = MsiRecordSetString(hrec, 1, "hoo");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 5, "size wrong\n");
    ok( 0 == strcmp(buffer," {hoo"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    /* {} inside a substitution does strange things... */
    r = MsiRecordSetString(hrec, 0, "[[1]{}]");
    r = MsiRecordSetString(hrec, 1, "2");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 5, "size wrong\n");
    ok( 0 == strcmp(buffer,"[[1]]"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[[1]{}[1]]");
    r = MsiRecordSetString(hrec, 1, "2");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 6, "size wrong\n");
    ok( 0 == strcmp(buffer,"[[1]2]"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

    r = MsiRecordSetString(hrec, 0, "[a[1]b[1]c{}d[1]e]");
    r = MsiRecordSetString(hrec, 1, "2");
    sz = sizeof buffer;
    r = MsiFormatRecord(0, hrec, buffer, &sz);
    ok( sz == 14, "size wrong\n");
    ok( 0 == strcmp(buffer,"[a[1]b[1]cd2e]"), "wrong output %s\n",buffer);
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
    ok( sz == 12, "size wrong\n");
    ok( 0 == strcmp(buffer,"{foo[-1]foo}"), "wrong output %s\n",buffer);
    ok( r == ERROR_SUCCESS, "format failed\n");

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
    ok( sz == 51, "size wrong (%li)\n",sz);
    ok( 0 == strcmp(buffer,"1:  2:  3:  4:  5:  6:  7:  8:  9:  10:  11:  12:  "), "wrong output(%s)\n",buffer);

    /* now put play games with escaping */
    r = MsiRecordSetString(hrec, 0, "[1] [2] [\\3asdf]");
    r = MsiRecordSetString(hrec, 1, "boo");
    r = MsiRecordSetString(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 9, "size wrong(%li)\n",sz);
    ok( 0 == strcmp(buffer,"boo hoo 3"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "[1] [2] [\\x]");
    r = MsiRecordSetString(hrec, 1, "boo");
    r = MsiRecordSetString(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 9, "size wrong(%li)\n",sz);
    ok( 0 == strcmp(buffer,"boo hoo x"), "wrong output (%s)\n",buffer);

    /* null characters */
    r = MsiRecordSetString(hrec, 0, "[1] [~] [2]");
    r = MsiRecordSetString(hrec, 1, "boo");
    r = MsiRecordSetString(hrec, 2, "hoo");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 9, "size wrong\n");
    ok( 0 == strcmp(buffer,"boo "), "wrong output\n");

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
    ok( sz == 12, "size wrong(%li)\n",sz);
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
    ok( sz == 9, "size wrong(%li)\n",sz);
    ok( 0 == strcmp(buffer,"1 [1] [2]"), "wrong output (%s)\n",buffer);

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
    ok( sz == 10, "size wrong(%li)\n",sz);
    ok( 0 == strcmp(buffer,"\\blath b 1"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "[1] [2] [[\\3asdf]]");
    r = MsiRecordSetString(hrec, 1, "boo");
    r = MsiRecordSetString(hrec, 2, "hoo");
    r = MsiRecordSetString(hrec, 3, "yeah");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 11, "size wrong(%li)\n",sz);
    ok( 0 == strcmp(buffer,"boo hoo [3]"), "wrong output (%s)\n",buffer);

    r = MsiRecordSetString(hrec, 0, "[1] [2] [[3]]");
    r = MsiRecordSetString(hrec, 1, "boo");
    r = MsiRecordSetString(hrec, 2, "hoo");
    r = MsiRecordSetString(hrec, 3, "\\help");
    ok( r == ERROR_SUCCESS, "set string failed\n");
    sz = sizeof buffer;
    r = MsiFormatRecord(package, hrec, buffer, &sz);
    ok( r == ERROR_SUCCESS, "format failed\n");
    ok( sz == 9, "size wrong(%li)\n",sz);
    ok( 0 == strcmp(buffer,"boo hoo h"), "wrong output (%s)\n",buffer);

    MsiCloseHandle(hrec);

    r = MsiCloseHandle(package);
    ok(r==ERROR_SUCCESS, "Unable to close package\n");

    DeleteFile( filename );
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
    test_processmessage();
}
