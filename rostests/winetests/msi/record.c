/*
 * Copyright (C) 2005 Mike McCormack for CodeWeavers
 *
 * A test program for MSI records
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

#include <windows.h>
#include <msi.h>
#include <msiquery.h>

#include "wine/test.h"

static BOOL create_temp_file(char *name)
{
    UINT r;
    unsigned char buffer[26], i;
    DWORD sz;
    HANDLE handle;
    
    r = GetTempFileName(".", "msitest",0,name);
    if(!r)
        return r;
    handle = CreateFile(name, GENERIC_READ|GENERIC_WRITE, 
        0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(handle==INVALID_HANDLE_VALUE)
        return 0;
    for(i=0; i<26; i++)
        buffer[i]=i+'a';
    r = WriteFile(handle,buffer,sizeof buffer,&sz,NULL);
    CloseHandle(handle);
    return r;
}

static void test_msirecord(void)
{
    DWORD r, sz;
    INT i;
    MSIHANDLE h;
    char buf[10];
    WCHAR bufW[10];
    const char str[] = "hello";
    const WCHAR strW[] = { 'h','e','l','l','o',0};
    char filename[MAX_PATH];

    /* check behaviour with an invalid record */
    r = MsiRecordGetFieldCount(0);
    ok(r==-1, "field count for invalid record not -1\n");
    SetLastError(0);
    r = MsiRecordIsNull(0, 0);
    ok(r==0, "invalid handle not considered to be non-null...\n");
    ok(GetLastError()==0, "MsiRecordIsNull set LastError\n");
    r = MsiRecordGetInteger(0,0);
    ok(r == MSI_NULL_INTEGER, "got integer from invalid record\n");
    r = MsiRecordSetInteger(0,0,0);
    ok(r == ERROR_INVALID_HANDLE, "MsiRecordSetInteger returned wrong error\n");
    r = MsiRecordSetInteger(0,-1,0);
    ok(r == ERROR_INVALID_HANDLE, "MsiRecordSetInteger returned wrong error\n");
    SetLastError(0);
    h = MsiCreateRecord(-1);
    ok(h==0, "created record with -1 elements\n");
    h = MsiCreateRecord(0x10000);
    ok(h==0, "created record with 0x10000 elements\n");
    /* doesn't set LastError */
    ok(GetLastError()==0, "MsiCreateRecord set last error\n");
    r = MsiRecordClearData(0);
    ok(r == ERROR_INVALID_HANDLE, "MsiRecordClearData returned wrong error\n");
    r = MsiRecordDataSize(0,0);
    ok(r == 0, "MsiRecordDataSize returned wrong error\n");


    /* check behaviour of a record with 0 elements */
    h = MsiCreateRecord(0);
    ok(h!=0, "couldn't create record with zero elements\n");
    r = MsiRecordGetFieldCount(h);
    ok(r==0, "field count should be zero\n");
    r = MsiRecordIsNull(h,0);
    ok(r, "new record wasn't null\n");
    r = MsiRecordIsNull(h,1);
    ok(r, "out of range record wasn't null\n");
    r = MsiRecordIsNull(h,-1);
    ok(r, "out of range record wasn't null\n");
    r = MsiRecordDataSize(h,0);
    ok(r==0, "size of null record is 0\n");
    sz = sizeof buf;
    strcpy(buf,"x");
    r = MsiRecordGetString(h, 0, buf, &sz);
    ok(r==ERROR_SUCCESS, "failed to get null string\n");
    ok(sz==0, "null string too long\n");
    ok(buf[0]==0, "null string not set\n");

    /* same record, but add an integer to it */
    r = MsiRecordSetInteger(h, 0, 0);
    ok(r == ERROR_SUCCESS, "Failed to set integer at 0 to 0\n");
    r = MsiRecordIsNull(h,0);
    ok(r==0, "new record is null after setting an integer\n");
    r = MsiRecordDataSize(h,0);
    ok(r==sizeof(DWORD), "size of integer record is 4\n");
    r = MsiRecordSetInteger(h, 0, 1);
    ok(r == ERROR_SUCCESS, "Failed to set integer at 0 to 1\n");
    r = MsiRecordSetInteger(h, 1, 1);
    ok(r == ERROR_INVALID_PARAMETER, "set integer at 1\n");
    r = MsiRecordSetInteger(h, -1, 0);
    ok(r == ERROR_INVALID_PARAMETER, "set integer at -1\n");
    r = MsiRecordIsNull(h,0);
    ok(r==0, "new record is null after setting an integer\n");
    r = MsiRecordGetInteger(h, 0);
    ok(r == 1, "failed to get integer\n");

    /* same record, but add a string to it */
    r = MsiRecordSetString(h, 0, NULL);
    ok(r == ERROR_SUCCESS, "Failed to set null string at 0\n");
    r = MsiRecordIsNull(h, 0);
    ok(r == TRUE, "null string not null field\n");
    r = MsiRecordSetString(h, 0, "");
    ok(r == ERROR_SUCCESS, "Failed to set empty string at 0\n");
    r = MsiRecordIsNull(h, 0);
    ok(r == TRUE, "null string not null field\n");
    r = MsiRecordSetString(h,0,str);
    ok(r == ERROR_SUCCESS, "Failed to set string at 0\n");
    r = MsiRecordGetInteger(h, 0);
    ok(r == MSI_NULL_INTEGER, "should get invalid integer\n");
    r = MsiRecordDataSize(h,0);
    ok(r==sizeof str-1, "size of string record is strlen\n");
    buf[0]=0;
    sz = sizeof buf;
    r = MsiRecordGetString(h,0,buf,&sz);
    ok(r == ERROR_SUCCESS, "Failed to get string at 0\n");
    ok(0==strcmp(buf,str), "MsiRecordGetString returned the wrong string\n");
    ok(sz == sizeof str-1, "MsiRecordGetString returned the wrong length\n");
    buf[0]=0;
    sz = sizeof str - 2;
    r = MsiRecordGetString(h,0,buf,&sz);
    ok(r == ERROR_MORE_DATA, "small buffer should yield ERROR_MORE_DATA\n");
    ok(sz == sizeof str-1, "MsiRecordGetString returned the wrong length\n");
    ok(0==strncmp(buf,str,sizeof str-3), "MsiRecordGetString returned the wrong string\n");
    ok(buf[sizeof str - 3]==0, "string wasn't nul terminated\n");

    buf[0]=0;
    sz = sizeof str;
    r = MsiRecordGetString(h,0,buf,&sz);
    ok(r == ERROR_SUCCESS, "wrong error\n");
    ok(sz == sizeof str-1, "MsiRecordGetString returned the wrong length\n");
    ok(0==strcmp(buf,str), "MsiRecordGetString returned the wrong string\n");


    memset(bufW, 0, sizeof bufW);
    sz = 5;
    r = MsiRecordGetStringW(h,0,bufW,&sz);
    ok(r == ERROR_MORE_DATA, "wrong error\n");
    ok(sz == 5, "MsiRecordGetString returned the wrong length\n");
    ok(0==memcmp(bufW,strW,8), "MsiRecordGetString returned the wrong string\n");

    sz = 0;
    bufW[0] = 'x';
    r = MsiRecordGetStringW(h,0,bufW,&sz);
    ok(r == ERROR_MORE_DATA, "wrong error\n");
    ok(sz == 5, "MsiRecordGetString returned the wrong length\n");
    ok('x'==bufW[0], "MsiRecordGetString returned the wrong string\n");

    memset(buf, 0, sizeof buf);
    sz = 5;
    r = MsiRecordGetStringA(h,0,buf,&sz);
    ok(r == ERROR_MORE_DATA, "wrong error\n");
    ok(sz == 5, "MsiRecordGetString returned the wrong length\n");
    ok(0==memcmp(buf,str,4), "MsiRecordGetString returned the wrong string\n");

    sz = 0;
    buf[0] = 'x';
    r = MsiRecordGetStringA(h,0,buf,&sz);
    ok(r == ERROR_MORE_DATA, "wrong error\n");
    ok(sz == 5, "MsiRecordGetString returned the wrong length\n");
    ok('x'==buf[0], "MsiRecordGetString returned the wrong string\n");

    /* same record, check we can wipe all the data */
    r = MsiRecordClearData(h);
    ok(r == ERROR_SUCCESS, "Failed to clear record\n");
    r = MsiRecordClearData(h);
    ok(r == ERROR_SUCCESS, "Failed to clear record again\n");
    r = MsiRecordIsNull(h,0);
    ok(r, "cleared record wasn't null\n");

    /* same record, try converting strings to integers */
    i = MsiRecordSetString(h,0,"42");
    ok(i == ERROR_SUCCESS, "Failed to set string at 0\n");
    i = MsiRecordGetInteger(h, 0);
    ok(i == 42, "should get invalid integer\n");
    i = MsiRecordSetString(h,0,"-42");
    ok(i == ERROR_SUCCESS, "Failed to set string at 0\n");
    i = MsiRecordGetInteger(h, 0);
    ok(i == -42, "should get invalid integer\n");
    i = MsiRecordSetString(h,0," 42");
    ok(i == ERROR_SUCCESS, "Failed to set string at 0\n");
    i = MsiRecordGetInteger(h, 0);
    ok(i == MSI_NULL_INTEGER, "should get invalid integer\n");
    i = MsiRecordSetString(h,0,"42 ");
    ok(i == ERROR_SUCCESS, "Failed to set string at 0\n");
    i = MsiRecordGetInteger(h, 0);
    ok(i == MSI_NULL_INTEGER, "should get invalid integer\n");
    i = MsiRecordSetString(h,0,"42.0");
    ok(i == ERROR_SUCCESS, "Failed to set string at 0\n");
    i = MsiRecordGetInteger(h, 0);
    ok(i == MSI_NULL_INTEGER, "should get invalid integer\n");
    i = MsiRecordSetString(h,0,"0x42");
    ok(i == ERROR_SUCCESS, "Failed to set string at 0\n");
    i = MsiRecordGetInteger(h, 0);
    ok(i == MSI_NULL_INTEGER, "should get invalid integer\n");
    i = MsiRecordSetString(h,0,"1000000000000000");
    ok(i == ERROR_SUCCESS, "Failed to set string at 0\n");
    i = MsiRecordGetInteger(h, 0);
    ok(i == -1530494976, "should get truncated integer\n");
    i = MsiRecordSetString(h,0,"2147483647");
    ok(i == ERROR_SUCCESS, "Failed to set string at 0\n");
    i = MsiRecordGetInteger(h, 0);
    ok(i == 2147483647, "should get maxint\n");
    i = MsiRecordSetString(h,0,"-2147483647");
    ok(i == ERROR_SUCCESS, "Failed to set string at 0\n");
    i = MsiRecordGetInteger(h, 0);
    ok(i == -2147483647, "should get -maxint-1\n");
    i = MsiRecordSetString(h,0,"4294967297");
    ok(i == ERROR_SUCCESS, "Failed to set string at 0\n");
    i = MsiRecordGetInteger(h, 0);
    ok(i == 1, "should get one\n");

    /* same record, try converting integers to strings */
    r = MsiRecordSetInteger(h, 0, 32);
    ok(r == ERROR_SUCCESS, "Failed to set integer at 0 to 32\n");
    buf[0]=0;
    sz = sizeof buf;
    r = MsiRecordGetString(h, 0, buf, &sz);
    ok(r == ERROR_SUCCESS, "failed to get string from integer\n");
    ok(0==strcmp(buf,"32"), "failed to get string from integer\n");
    r = MsiRecordSetInteger(h, 0, -32);
    ok(r == ERROR_SUCCESS, "Failed to set integer at 0 to 32\n");
    buf[0]=0;
    sz = sizeof buf;
    r = MsiRecordGetString(h, 0, buf, &sz);
    ok(r == ERROR_SUCCESS, "failed to get string from integer\n");
    ok(0==strcmp(buf,"-32"), "failed to get string from integer\n");

    /* same record, now try streams */
    r = MsiRecordSetStream(h, 0, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "set NULL stream\n");
    sz = sizeof buf;
    r = MsiRecordReadStream(h, 0, buf, &sz);
    ok(r == ERROR_INVALID_DATATYPE, "read non-stream type\n");
    ok(sz == sizeof buf, "set sz\n");
    r = MsiRecordDataSize( h, -1);
    ok(r == 0,"MsiRecordDataSize returned wrong size\n");
    r = MsiRecordDataSize( h, 0);
    ok(r == 4,"MsiRecordDataSize returned wrong size\n");

    /* same record, now close it */
    r = MsiCloseHandle(h);
    ok(r == ERROR_SUCCESS, "Failed to close handle\n");

    /* now try streams in a new record - need to create a file to play with */
    r = create_temp_file(filename); 
    if(!r)
        return;

    /* streams can't be inserted in field 0 for some reason */
    h = MsiCreateRecord(2);
    ok(h, "couldn't create a two field record\n");
    r = MsiRecordSetStream(h, 0, filename);
    ok(r == ERROR_INVALID_PARAMETER, "added stream to field 0\n");
    r = MsiRecordSetStream(h, 1, filename);
    ok(r == ERROR_SUCCESS, "failed to add stream to record\n");
    r = MsiRecordReadStream(h, 1, buf, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "should return error\n");
    /* http://test.winehq.org/data/200503181000/98_jmelgarejo98casa/msi:record.txt */
    DeleteFile(filename); /* Windows 98 doesn't like this at all, so don't check return. */
    r = MsiRecordReadStream(h, 1, NULL, NULL);
    ok(r == ERROR_INVALID_PARAMETER, "should return error\n");
    sz = sizeof buf;
    r = MsiRecordReadStream(h, 1, NULL, &sz);
    ok(r == ERROR_SUCCESS, "failed to read stream\n");
    ok(sz==26,"couldn't get size of stream\n");
    sz = 0;
    r = MsiRecordReadStream(h, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "failed to read stream\n");
    ok(sz==0,"short read\n");
    sz = sizeof buf;
    r = MsiRecordReadStream(h, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "failed to read stream\n");
    ok(sz==sizeof buf,"short read\n");
    ok(!strncmp(buf,"abcdefghij",10), "read the wrong thing\n");
    sz = sizeof buf;
    r = MsiRecordReadStream(h, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "failed to read stream\n");
    ok(sz==sizeof buf,"short read\n");
    ok(!strncmp(buf,"klmnopqrst",10), "read the wrong thing\n");
    memset(buf,0,sizeof buf);
    sz = sizeof buf;
    r = MsiRecordReadStream(h, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "failed to read stream\n");
    ok(sz==6,"short read\n");
    ok(!strcmp(buf,"uvwxyz"), "read the wrong thing\n");
    memset(buf,0,sizeof buf);
    sz = sizeof buf;
    r = MsiRecordReadStream(h, 1, buf, &sz);
    ok(r == ERROR_SUCCESS, "failed to read stream\n");
    ok(sz==0,"size non-zero at end of stream\n");
    ok(buf[0]==0, "read something at end of the stream\n");
    r = MsiRecordSetStream(h, 1, NULL);
    ok(r == ERROR_SUCCESS, "failed to reset stream\n");
    sz = 0;
    r = MsiRecordReadStream(h, 1, NULL, &sz);
    ok(r == ERROR_SUCCESS, "bytes left wrong after reset\n");
    ok(sz==26,"couldn't get size of stream\n");
    r = MsiRecordDataSize(h,1);
    ok(r == 26,"MsiRecordDataSize returned wrong size\n");

    /* now close the stream record */
    r = MsiCloseHandle(h);
    ok(r == ERROR_SUCCESS, "Failed to close handle\n");
    DeleteFile(filename); /* Delete it for sure, when everything else is closed. */
}

START_TEST(record)
{
    test_msirecord();
}
