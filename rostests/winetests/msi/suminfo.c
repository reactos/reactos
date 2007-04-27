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

#include "wine/test.h"

/*
 * The following are defined in Windows SDK's msidefs.h
 * but that file doesn't exist in the msvc6 header set.
 *
 * Some are already defined in PropIdl.h - undefine them
 */
#undef PID_DICTIONARY
#undef PID_CODEPAGE
#undef PID_SUBJECT
#undef PID_SECURITY

#define PID_DICTIONARY 0
#define PID_CODEPAGE 1
#define PID_TITLE 2
#define PID_SUBJECT 3
#define PID_AUTHOR 4
#define PID_KEYWORDS 5
#define PID_COMMENTS 6
#define PID_TEMPLATE 7
#define PID_LASTAUTHOR 8
#define PID_REVNUMBER 9
#define PID_EDITTINE 10
#define PID_LASTPRINTED 11
#define PID_CREATE_DTM 12
#define PID_LASTSAVE_DTM 13
#define PID_PAGECOUNT 14
#define PID_WORDCOUNT 15
#define PID_CHARCOUNT 16
#define PID_THUMBNAIL 17
#define PID_APPNAME 18
#define PID_SECURITY 19
#define PID_MSIVERSION PID_PAGECOUNT
#define PID_MSISOURCE PID_WORDCOUNT
#define PID_MSIRESTRICT PID_CHARCOUNT

START_TEST(suminfo)
{
    const char *msifile = "winetest.msi";
    MSIHANDLE hdb = 0, hsuminfo;
    UINT r, count, type;
    DWORD sz;
    INT val;
    FILETIME ft;
    char buf[0x10];

    DeleteFile(msifile);

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabase(msifile, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    r = MsiGetSummaryInformation(hdb, NULL, 0, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "MsiGetSummaryInformation wrong error\n");

    r = MsiGetSummaryInformation(hdb, NULL, 0, &hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiGetSummaryInformation failed\n");

    r = MsiSummaryInfoGetPropertyCount(0, NULL);
    ok(r == ERROR_INVALID_HANDLE, "getpropcount failed\n");

    r = MsiSummaryInfoGetPropertyCount(hsuminfo, NULL);
    ok(r == ERROR_SUCCESS, "getpropcount failed\n");

    count = -1;
    r = MsiSummaryInfoGetPropertyCount(hsuminfo, &count);
    ok(r == ERROR_SUCCESS, "getpropcount failed\n");
    ok(count == 0, "count should be zero\n");

    r = MsiSummaryInfoGetProperty(hsuminfo, 0, NULL, NULL, NULL, 0, NULL);
    ok(r == ERROR_SUCCESS, "getpropcount failed\n");

    type = -1;
    r = MsiSummaryInfoGetProperty(hsuminfo, 0, &type, NULL, NULL, 0, NULL);
    ok(r == ERROR_SUCCESS, "getpropcount failed\n");
    ok(type == 0, "wrong type\n");

    type = -1;
    val = 1234;
    r = MsiSummaryInfoGetProperty(hsuminfo, 0, &type, &val, NULL, 0, NULL);
    ok(r == ERROR_SUCCESS, "getpropcount failed\n");
    ok(type == 0, "wrong type\n");
    ok(val == 1234, "wrong val\n");

    buf[0]='x';
    buf[1]=0;
    sz = 0x10;
    r = MsiSummaryInfoGetProperty(hsuminfo, PID_REVNUMBER, &type, &val, NULL, buf, &sz);
    ok(r == ERROR_SUCCESS, "getpropcount failed\n");
    ok(buf[0]=='x', "cleared buffer\n");
    ok(sz == 0x10, "count wasn't zero\n");
    ok(type == VT_EMPTY, "should be empty\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_TITLE, VT_LPSTR, 0, NULL, "Mike");
    ok(r == ERROR_FUNCTION_FAILED, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_TITLE, VT_LPSTR, 1, NULL, "JungAh");
    ok(r == ERROR_FUNCTION_FAILED, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_TITLE, VT_LPSTR, 1, &ft, "Mike");
    ok(r == ERROR_FUNCTION_FAILED, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_CODEPAGE, VT_I2, 1, &ft, "JungAh");
    ok(r == ERROR_FUNCTION_FAILED, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiCloseHandle(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* try again with the update count set */
    r = MsiGetSummaryInformation(hdb, NULL, 1, &hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiGetSummaryInformation failed\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, 0, VT_LPSTR, 1, NULL, NULL);
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_CODEPAGE, VT_LPSTR, 1, NULL, NULL);
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_TITLE, VT_I4, 0, NULL, "Mike");
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_AUTHOR, VT_I4, 0, NULL, "JungAh");
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_KEYWORDS, VT_I2, 0, NULL, "Mike");
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_COMMENTS, VT_FILETIME, 0, NULL, "JungAh");
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_TEMPLATE, VT_I2, 0, NULL, "Mike");
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_LASTAUTHOR, VT_LPSTR, 0, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_LASTSAVE_DTM, VT_FILETIME, 0, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_LASTAUTHOR, VT_LPWSTR, 0, NULL, "h\0i\0\0");
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_REVNUMBER, VT_I4, 0, NULL, "Jungah");
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_PAGECOUNT, VT_LPSTR, 1, NULL, NULL);
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_TITLE, VT_LPSTR, 0, NULL, "Mike");
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoSetProperty failed\n");

    sz = 2;
    strcpy(buf,"x");
    r = MsiSummaryInfoGetProperty(hsuminfo, PID_TITLE, &type, NULL, NULL, buf, &sz );
    ok(r == ERROR_MORE_DATA, "MsiSummaryInfoSetProperty failed\n");
    ok(sz == 4, "count was wrong\n");
    ok(type == VT_LPSTR, "type was wrong\n");
    ok(!strcmp(buf,"M"), "buffer was wrong\n");

    sz = 4;
    strcpy(buf,"x");
    r = MsiSummaryInfoGetProperty(hsuminfo, PID_TITLE, &type, NULL, NULL, buf, &sz );
    ok(r == ERROR_MORE_DATA, "MsiSummaryInfoSetProperty failed\n");
    ok(sz == 4, "count was wrong\n");
    ok(type == VT_LPSTR, "type was wrong\n");
    ok(!strcmp(buf,"Mik"), "buffer was wrong\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_TITLE, VT_LPSTR, 0, NULL, "JungAh");
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoSetProperty failed\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_CODEPAGE, VT_I2, 1, &ft, "Mike");
    ok(r == ERROR_FUNCTION_FAILED, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiCloseHandle(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* try again with a higher update count */
    r = MsiGetSummaryInformation(hdb, NULL, 10, &hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiGetSummaryInformation failed\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_TITLE, VT_LPSTR, 0, NULL, "JungAh");
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoSetProperty failed\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_CODEPAGE, VT_LPSTR, 1, NULL, NULL);
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_CODEPAGE, VT_I2, 1, NULL, NULL);
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_CODEPAGE, VT_I2, 1, &ft, "Mike");
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetProperty(hsuminfo, PID_AUTHOR, VT_LPSTR, 1, &ft, "Mike");
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoPersist(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoPersist failed\n");

    MsiDatabaseCommit(hdb);

    r = MsiCloseHandle(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = DeleteFile(msifile);
    ok(r, "DeleteFile failed\n");
}
