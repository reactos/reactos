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

static const char *msifile = "winetest-suminfo.msi";
static const WCHAR msifileW[] = L"winetest-suminfo.msi";

static void test_suminfo(void)
{
    MSIHANDLE hdb = 0, hsuminfo;
    UINT r, count, type;
    DWORD sz;
    INT val;
    FILETIME ft;
    char buf[0x10];

    DeleteFileA(msifile);

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_CREATE, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    r = MsiGetSummaryInformationA(hdb, NULL, 0, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "MsiGetSummaryInformation wrong error\n");

    r = MsiGetSummaryInformationA(hdb, NULL, 0, &hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiGetSummaryInformation failed %u\n", r);

    r = MsiCloseHandle(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiGetSummaryInformationA(0, "", 0, &hsuminfo);
    todo_wine
    ok(r == ERROR_INSTALL_PACKAGE_INVALID || r == ERROR_INSTALL_PACKAGE_OPEN_FAILED,
       "MsiGetSummaryInformation failed %u\n", r);

    r = MsiGetSummaryInformationA(hdb, "", 0, &hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiGetSummaryInformation failed %u\n", r);

    r = MsiSummaryInfoGetPropertyCount(0, NULL);
    ok(r == ERROR_INVALID_HANDLE, "getpropcount failed\n");

    r = MsiSummaryInfoGetPropertyCount(hsuminfo, NULL);
    ok(r == ERROR_SUCCESS, "getpropcount failed\n");

    count = -1;
    r = MsiSummaryInfoGetPropertyCount(hsuminfo, &count);
    ok(r == ERROR_SUCCESS, "getpropcount failed\n");
    ok(count == 0, "count should be zero\n");

    r = MsiSummaryInfoGetPropertyA(hsuminfo, 0, NULL, NULL, NULL, 0, NULL);
    ok(r == ERROR_SUCCESS, "getpropcount failed\n");

    r = MsiSummaryInfoGetPropertyA(hsuminfo, -1, NULL, NULL, NULL, 0, NULL);
    ok(r == ERROR_UNKNOWN_PROPERTY, "MsiSummaryInfoGetProperty wrong error\n");

    r = MsiSummaryInfoGetPropertyA(hsuminfo, PID_SECURITY+1, NULL, NULL, NULL, 0, NULL);
    ok(r == ERROR_UNKNOWN_PROPERTY, "MsiSummaryInfoGetProperty wrong error\n");

    type = -1;
    r = MsiSummaryInfoGetPropertyA(hsuminfo, 0, &type, NULL, NULL, 0, NULL);
    ok(r == ERROR_SUCCESS, "getpropcount failed\n");
    ok(type == 0, "wrong type\n");

    type = -1;
    val = 1234;
    r = MsiSummaryInfoGetPropertyA(hsuminfo, 0, &type, &val, NULL, 0, NULL);
    ok(r == ERROR_SUCCESS, "getpropcount failed\n");
    ok(type == 0, "wrong type\n");
    ok(val == 1234, "wrong val\n");

    buf[0]='x';
    buf[1]=0;
    sz = 0x10;
    r = MsiSummaryInfoGetPropertyA(hsuminfo, PID_REVNUMBER, &type, &val, NULL, buf, &sz);
    ok(r == ERROR_SUCCESS, "getpropcount failed\n");
    ok(buf[0]=='x', "cleared buffer\n");
    ok(sz == 0x10, "count wasn't zero\n");
    ok(type == VT_EMPTY, "should be empty\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_TITLE, VT_LPSTR, 0, NULL, "Mike");
    ok(r == ERROR_FUNCTION_FAILED, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_TITLE, VT_LPSTR, 1, NULL, "JungAh");
    ok(r == ERROR_FUNCTION_FAILED, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_TITLE, VT_LPSTR, 1, &ft, "Mike");
    ok(r == ERROR_FUNCTION_FAILED, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_CODEPAGE, VT_I2, 1, &ft, "JungAh");
    ok(r == ERROR_FUNCTION_FAILED, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiCloseHandle(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* try again with the update count set */
    r = MsiGetSummaryInformationA(hdb, NULL, 1, &hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiGetSummaryInformation failed\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, 0, VT_LPSTR, 1, NULL, NULL);
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_CODEPAGE, VT_LPSTR, 1, NULL, NULL);
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_TITLE, VT_I4, 0, NULL, "Mike");
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_AUTHOR, VT_I4, 0, NULL, "JungAh");
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_KEYWORDS, VT_I2, 0, NULL, "Mike");
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_COMMENTS, VT_FILETIME, 0, NULL, "JungAh");
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_TEMPLATE, VT_I2, 0, NULL, "Mike");
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_LASTAUTHOR, VT_LPSTR, 0, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_LASTSAVE_DTM, VT_FILETIME, 0, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_LASTAUTHOR, VT_LPWSTR, 0, NULL, "h\0i\0\0");
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_REVNUMBER, VT_I4, 0, NULL, "Jungah");
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_PAGECOUNT, VT_LPSTR, 1, NULL, NULL);
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_TITLE, VT_LPSTR, 0, NULL, "Mike");
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoSetProperty failed\n");

    sz = 2;
    strcpy(buf,"x");
    r = MsiSummaryInfoGetPropertyA(hsuminfo, PID_TITLE, &type, NULL, NULL, buf, &sz );
    ok(r == ERROR_MORE_DATA, "MsiSummaryInfoSetProperty failed\n");
    ok(sz == 4, "count was wrong\n");
    ok(type == VT_LPSTR, "type was wrong\n");
    ok(!strcmp(buf,"M"), "buffer was wrong\n");

    sz = 4;
    strcpy(buf,"x");
    r = MsiSummaryInfoGetPropertyA(hsuminfo, PID_TITLE, &type, NULL, NULL, buf, &sz );
    ok(r == ERROR_MORE_DATA, "MsiSummaryInfoSetProperty failed\n");
    ok(sz == 4, "count was wrong\n");
    ok(type == VT_LPSTR, "type was wrong\n");
    ok(!strcmp(buf,"Mik"), "buffer was wrong\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_TITLE, VT_LPSTR, 0, NULL, "JungAh");
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoSetProperty failed\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_CODEPAGE, VT_I2, 1, &ft, "Mike");
    ok(r == ERROR_FUNCTION_FAILED, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiCloseHandle(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* try again with a higher update count */
    r = MsiGetSummaryInformationA(hdb, NULL, 10, &hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiGetSummaryInformation failed\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_TITLE, VT_LPSTR, 0, NULL, "JungAh");
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoSetProperty failed\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_CODEPAGE, VT_LPSTR, 1, NULL, NULL);
    ok(r == ERROR_DATATYPE_MISMATCH, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_CODEPAGE, VT_I2, 1, NULL, NULL);
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_CODEPAGE, VT_I2, 1, &ft, "Mike");
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_AUTHOR, VT_LPSTR, 1, &ft, "Mike");
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoPersist(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoPersist failed\n");

    MsiDatabaseCommit(hdb);

    r = MsiCloseHandle(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* filename, non-zero update count */
    r = MsiGetSummaryInformationA(0, msifile, 1, &hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiGetSummaryInformation failed\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_AUTHOR, VT_LPSTR, 1, &ft, "Mike");
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoPersist(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoPersist failed %u\n", r);

    r = MsiCloseHandle(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed %u\n", r);

    /* filename, zero update count */
    r = MsiGetSummaryInformationA(0, msifile, 0, &hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiGetSummaryInformation failed %u\n", r);

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_AUTHOR, VT_LPSTR, 1, &ft, "Mike");
    todo_wine ok(r == ERROR_FUNCTION_FAILED, "MsiSummaryInfoSetProperty wrong error, %u\n", r);

    r = MsiSummaryInfoPersist(hsuminfo);
    ok(r == ERROR_FUNCTION_FAILED, "MsiSummaryInfoPersist wrong error %u\n", r);

    r = MsiCloseHandle(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* try persisting on transacted msi */
    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_TRANSACT, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    r = MsiGetSummaryInformationA(hdb, NULL, 1, &hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiGetSummaryInformation wrong error\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_AUTHOR, VT_LPSTR, 1, &ft, "Fabian");
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoPersist(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoPersist wrong error %u\n", r);

    r = MsiCloseHandle(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_READONLY, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    r = MsiGetSummaryInformationA(hdb, NULL, 1, &hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiGetSummaryInformation wrong error\n");

    sz = 0x10;
    strcpy(buf,"x");
    r = MsiSummaryInfoGetPropertyA(hsuminfo, PID_AUTHOR, &type, NULL, NULL, buf, &sz);
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoGetPropertyA wrong error\n");
    ok(!strcmp(buf,"Mike"), "buffer was wrong: %s\n", buf);

    r = MsiCloseHandle(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* try persisting on transacted msi with commit */
    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_TRANSACT, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    r = MsiGetSummaryInformationA(hdb, NULL, 1, &hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiGetSummaryInformation wrong error\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_AUTHOR, VT_LPSTR, 1, &ft, "Fabian");
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoPersist(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoPersist wrong error %u\n", r);

    r = MsiCloseHandle(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiDatabaseCommit(hdb);
    ok(r == ERROR_SUCCESS, "MsiDatabaseCommit wrong error %u\n", r);

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_READONLY, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    r = MsiGetSummaryInformationA(hdb, NULL, 1, &hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiGetSummaryInformation wrong error\n");

    sz = 0x10;
    strcpy(buf,"x");
    r = MsiSummaryInfoGetPropertyA(hsuminfo, PID_AUTHOR, &type, NULL, NULL, buf, &sz);
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoGetPropertyA wrong error\n");
    ok(!strcmp(buf,"Fabian"), "buffer was wrong: %s\n", buf);

    r = MsiCloseHandle(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* try persisting on direct msi */
    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_DIRECT, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    r = MsiGetSummaryInformationA(hdb, NULL, 1, &hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiGetSummaryInformation wrong error\n");

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_AUTHOR, VT_LPSTR, 1, &ft, "Fabian2");
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoPersist(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoPersist wrong error %u\n", r);

    r = MsiCloseHandle(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_READONLY, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    r = MsiGetSummaryInformationA(hdb, NULL, 1, &hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiGetSummaryInformation wrong error\n");

    sz = 0x10;
    strcpy(buf,"x");
    r = MsiSummaryInfoGetPropertyA(hsuminfo, PID_AUTHOR, &type, NULL, NULL, buf, &sz);
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoGetPropertyA wrong error\n");
    ok(!strcmp(buf,"Fabian2"), "buffer was wrong: %s\n", buf);

    r = MsiCloseHandle(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* try persisting on indirectly opened msi */
    r = MsiGetSummaryInformationA(0, msifile, 2, &hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiGetSummaryInformation wrong error %u\n", r);

    r = MsiSummaryInfoSetPropertyA(hsuminfo, PID_AUTHOR, VT_LPSTR, 1, &ft, "Fabian3");
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoSetProperty wrong error\n");

    r = MsiSummaryInfoPersist(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoPersist wrong error %u\n", r);

    r = MsiCloseHandle(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_READONLY, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    r = MsiGetSummaryInformationA(hdb, NULL, 1, &hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiGetSummaryInformation wrong error\n");

    sz = 0x10;
    strcpy(buf,"x");
    r = MsiSummaryInfoGetPropertyA(hsuminfo, PID_AUTHOR, &type, NULL, NULL, buf, &sz);
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoGetPropertyA wrong error\n");
    ok(!strcmp(buf,"Fabian3"), "buffer was wrong: %s\n", buf);

    r = MsiCloseHandle(hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    r = MsiCloseHandle(hdb);
    ok(r == ERROR_SUCCESS, "MsiCloseHandle failed\n");

    /* Cleanup */
    r = DeleteFileA(msifile);
    ok(r, "DeleteFile failed\n");
}

static const WCHAR tb[] = { 0x4840, 0x3f7f, 0x4164, 0x422f, 0x4836, 0 }; /* _Tables */
static const WCHAR sd[] = { 0x4840, 0x3f3f, 0x4577, 0x446c, 0x3b6a, 0x45e4, 0x4824, 0 }; /* _StringData */
static const WCHAR sp[] = { 0x4840, 0x3f3f, 0x4577, 0x446c, 0x3e6a, 0x44b2, 0x482f, 0 }; /* _StringPool */

#define LOSE_CONST(x) ((LPSTR)(UINT_PTR)(x))

static void test_create_database_binary(void)
{
    static const CLSID CLSID_MsiDatabase =
        { 0xc1084, 0, 0, {0xc0, 0, 0, 0, 0, 0, 0, 0x46 } };
    static const CLSID IID_IPropertySetStorage =
        { 0x13a, 0, 0, {0xc0, 0, 0, 0, 0, 0, 0, 0x46 } };
    static const CLSID FMTID_SummaryInformation =
        { 0xf29f85e0, 0x4ff9, 0x1068, {0xab, 0x91, 0x08, 0x00, 0x2b, 0x27, 0xb3, 0xd9}};
    DWORD mode = STGM_CREATE | STGM_READWRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE;
    IPropertySetStorage *pss = NULL;
    IPropertyStorage *ps = NULL;
    IStorage *stg = NULL;
    IStream *stm = NULL;
    HRESULT r;
    PROPSPEC propspec[10];
    PROPVARIANT propvar[10];
    USHORT data[2] = { 0, 0 };

    r = StgCreateDocfile( msifileW, mode, 0, &stg );
    ok( r == S_OK, "failed to create database\n");

    r = IStorage_SetClass( stg, &CLSID_MsiDatabase );
    ok( r == S_OK, "failed to set clsid\n");

    /* create the _StringData stream */
    r = IStorage_CreateStream( stg, sd, STGM_WRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stm );
    ok( r == S_OK, "failed to create stream\n");

    IStream_Release( stm );

    /* create the _StringPool stream */
    r = IStorage_CreateStream( stg, sp, STGM_WRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stm );
    ok( r == S_OK, "failed to create stream\n");

    r = IStream_Write( stm, data, sizeof data, NULL );
    ok( r == S_OK, "failed to write stream\n");

    IStream_Release( stm );

    /* create the _Tables stream */
    r = IStorage_CreateStream( stg, tb, STGM_WRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stm );
    ok( r == S_OK, "failed to create stream\n");

    IStream_Release( stm );

    r = IStorage_QueryInterface( stg, &IID_IPropertySetStorage, (void**) &pss );
    ok( r == S_OK, "failed to set clsid\n");

    r = IPropertySetStorage_Create( pss, &FMTID_SummaryInformation, NULL, 0, mode, &ps );
    ok( r == S_OK, "failed to create property set\n");

    r = IPropertyStorage_SetClass( ps, &FMTID_SummaryInformation );
    ok( r == S_OK, "failed to set class\n");

    propspec[0].ulKind = PRSPEC_PROPID;
    propspec[0].propid = PID_TITLE;
    propvar[0].vt = VT_LPSTR;
    propvar[0].pszVal = LOSE_CONST("test title");

    propspec[1].ulKind = PRSPEC_PROPID;
    propspec[1].propid = PID_SUBJECT;
    propvar[1].vt = VT_LPSTR;
    propvar[1].pszVal = LOSE_CONST("msi suminfo / property storage test");

    propspec[2].ulKind = PRSPEC_PROPID;
    propspec[2].propid = PID_AUTHOR;
    propvar[2].vt = VT_LPSTR;
    propvar[2].pszVal = LOSE_CONST("mike_m");

    propspec[3].ulKind = PRSPEC_PROPID;
    propspec[3].propid = PID_TEMPLATE;
    propvar[3].vt = VT_LPSTR;
    propvar[3].pszVal = LOSE_CONST(";1033");  /* actually the string table's codepage */

    propspec[4].ulKind = PRSPEC_PROPID;
    propspec[4].propid = PID_REVNUMBER;
    propvar[4].vt = VT_LPSTR;
    propvar[4].pszVal = LOSE_CONST("{913B8D18-FBB6-4CAC-A239-C74C11E3FA74}");

    propspec[5].ulKind = PRSPEC_PROPID;
    propspec[5].propid = PID_PAGECOUNT;
    propvar[5].vt = VT_I4;
    propvar[5].lVal = 100;

    propspec[6].ulKind = PRSPEC_PROPID;
    propspec[6].propid = PID_WORDCOUNT;
    propvar[6].vt = VT_I4;
    propvar[6].lVal = 0;

    /* MSDN says that PID_LASTPRINTED should be a VT_FILETIME... */
    propspec[7].ulKind = PRSPEC_PROPID;
    propspec[7].propid = PID_LASTPRINTED;
    propvar[7].vt = VT_LPSTR;
    propvar[7].pszVal = LOSE_CONST("7/1/1999 5:17");

    r = IPropertyStorage_WriteMultiple( ps, 8, propspec, propvar, PID_FIRST_USABLE );
    ok( r == S_OK, "failed to write properties\n");

    IPropertyStorage_Commit( ps, STGC_DEFAULT );

    IPropertyStorage_Release( ps );
    IPropertySetStorage_Release( pss );

    IStorage_Commit( stg, STGC_DEFAULT );
    IStorage_Release( stg );
}

static void test_summary_binary(void)
{
    MSIHANDLE hdb = 0, hsuminfo = 0;
    UINT r, type, count;
    INT ival;
    DWORD sz;
    char sval[20];

    DeleteFileA( msifile );

    test_create_database_binary();

    ok(GetFileAttributesA(msifile) != INVALID_FILE_ATTRIBUTES, "file doesn't exist!\n");

    /* just MsiOpenDatabase should not create a file */
    r = MsiOpenDatabaseW(msifileW, MSIDBOPEN_READONLY, &hdb);
    ok(r == ERROR_SUCCESS, "MsiOpenDatabase failed\n");

    r = MsiGetSummaryInformationA(hdb, NULL, 0, &hsuminfo);
    ok(r == ERROR_SUCCESS, "MsiGetSummaryInformation failed\n");

    /*
     * Check what reading PID_LASTPRINTED does...
     * The string value is written to the msi file
     * but it appears that we're not allowed to read it back again.
     * We can still read its type though...?
     */
    sz = sizeof sval;
    sval[0] = 0;
    type = 0;
    r = MsiSummaryInfoGetPropertyA(hsuminfo, PID_LASTPRINTED, &type, NULL, NULL, sval, &sz);
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoGetProperty failed\n");
    ok(!lstrcmpA(sval, "") || !lstrcmpA(sval, "7"),
        "Expected empty string or \"7\", got \"%s\"\n", sval);
    todo_wine {
    ok(type == VT_LPSTR, "Expected VT_LPSTR, got %d\n", type);
    ok(sz == 0 || sz == 1, "Expected 0 or 1, got %lu\n", sz);
    }

    ival = -1;
    r = MsiSummaryInfoGetPropertyA(hsuminfo, PID_WORDCOUNT, &type, &ival, NULL, NULL, NULL);
    ok(r == ERROR_SUCCESS, "MsiSummaryInfoGetProperty failed\n");
    todo_wine ok( ival == 0, "value incorrect\n");

    /* looks like msi adds some of its own values in here */
    count = 0;
    r = MsiSummaryInfoGetPropertyCount( hsuminfo, &count );
    ok(r == ERROR_SUCCESS, "getpropcount failed\n");
    todo_wine ok(count == 10, "prop count incorrect\n");

    r = MsiSummaryInfoSetPropertyA( hsuminfo, PID_TITLE, VT_LPSTR, 0, NULL, "Mike" );
    ok(r == ERROR_FUNCTION_FAILED, "MsiSummaryInfoSetProperty failed %u\n", r);

    r = MsiSummaryInfoPersist( hsuminfo );
    ok(r == ERROR_FUNCTION_FAILED, "MsiSummaryInfoPersist failed %u\n", r);

    MsiCloseHandle( hsuminfo );
    MsiCloseHandle( hdb );

    DeleteFileA( msifile );
}

START_TEST(suminfo)
{
    test_suminfo();
    test_summary_binary();
}
