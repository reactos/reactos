/*
 * Tests for recycle bin functions
 *
 * Copyright 2011 Jay Yang
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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "shellapi.h"

#include <stdio.h>
#include "wine/test.h"

static int (WINAPI *pSHQueryRecycleBinA)(LPCSTR,LPSHQUERYRBINFO);
static int (WINAPI *pSHFileOperationA)(LPSHFILEOPSTRUCTA);

static char int64_buffer[65];
/* Note: This function uses a single buffer for the return value.*/
static const char* str_from_int64(__int64 ll)
{

    if (sizeof(ll) > sizeof(unsigned long) && ll >> 32)
        sprintf(int64_buffer,"%lx%08lx",(unsigned long)(ll >> 32),(unsigned long)ll);
    else
        sprintf(int64_buffer,"%lx",(unsigned long)ll);
    return int64_buffer;
}

static void setup_pointers(void)
{
    HMODULE hshell32 = GetModuleHandleA("shell32.dll");
    pSHQueryRecycleBinA = (void*)GetProcAddress(hshell32, "SHQueryRecycleBinA");
    pSHFileOperationA = (void*)GetProcAddress(hshell32, "SHFileOperationA");
}

static void test_query_recyclebin(void)
{
    SHQUERYRBINFO info1={sizeof(info1),0xdeadbeef,0xdeadbeef};
    SHQUERYRBINFO info2={sizeof(info2),0xdeadbeef,0xdeadbeef};
    UINT written;
    HRESULT hr;
    HANDLE file;
    SHFILEOPSTRUCTA shfo;
    const CHAR name[] = "test.txt";
    CHAR buf[MAX_PATH + sizeof(name) + 1];
    if(!pSHQueryRecycleBinA)
    {
        skip("SHQueryRecycleBinA does not exist\n");
        return;
    }
    if(!pSHFileOperationA)
    {
        skip("SHFileOperationA does not exist\n");
        return;
    }
    GetCurrentDirectoryA(MAX_PATH, buf);
    strcat(buf,"\\");
    strcat(buf,name);
    buf[strlen(buf) + 1] = '\0';
    hr = pSHQueryRecycleBinA(buf,&info1);
    ok(hr == S_OK, "SHQueryRecycleBinA failed with error 0x%x\n", hr);
    ok(info1.i64Size!=0xdeadbeef,"i64Size not set\n");
    ok(info1.i64NumItems!=0xdeadbeef,"i64NumItems not set\n");
    /*create and send a file to the recycle bin*/
    file = CreateFileA(name,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failure to open file %s\n",name);
    WriteFile(file,name,strlen(name),&written,NULL);
    CloseHandle(file);
    shfo.hwnd = NULL;
    shfo.wFunc = FO_DELETE;
    shfo.pFrom = buf;
    shfo.pTo = NULL;
    shfo.fFlags = FOF_FILESONLY | FOF_NOCONFIRMATION | FOF_SILENT | FOF_ALLOWUNDO;
    shfo.hNameMappings = NULL;
    shfo.lpszProgressTitle = NULL;
    ok(!pSHFileOperationA(&shfo), "Deletion was not successful\n");
    hr = pSHQueryRecycleBinA(buf,&info2);
    ok(hr == S_OK, "SHQueryRecycleBinW failed with error 0x%x\n", hr);
    if(info2.i64Size!=info1.i64Size || info2.i64NumItems!=info1.i64NumItems) {
      ok(info2.i64Size==info1.i64Size+written,"Expected recycle bin to have 0x%s bytes\n",str_from_int64(info1.i64Size+written));
      ok(info2.i64NumItems==info1.i64NumItems+1,"Expected recycle bin to have 0x%s items\n",str_from_int64(info1.i64NumItems+1));
    } else todo_wine {
      ok(info2.i64Size==info1.i64Size+written,"Expected recycle bin to have 0x%s bytes\n",str_from_int64(info1.i64Size+written));
      ok(info2.i64NumItems==info1.i64NumItems+1,"Expected recycle bin to have 0x%s items\n",str_from_int64(info1.i64NumItems+1));
    }
}


START_TEST(recyclebin)
{
    setup_pointers();
    test_query_recyclebin();
}
