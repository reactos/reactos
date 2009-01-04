/*
 * Unit tests for OLE storage
 *
 * Copyright (c) 2004 Mike McCormack
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

#define COBJMACROS

#include <windows.h>
#include "wine/test.h"

#include "ole2.h"
#include "objidl.h"
#include "initguid.h"

DEFINE_GUID( test_stg_cls, 0x88888888, 0x0425, 0x0000, 0,0,0,0,0,0,0,0);

#define ok_ole_success(hr, func) ok(hr == S_OK, func " failed with error 0x%08x\n", hr)

static void test_hglobal_storage_stat(void)
{
    ILockBytes *ilb = NULL;
    IStorage *stg = NULL;
    HRESULT r;
    STATSTG stat;
    DWORD mode, refcount;

    r = CreateILockBytesOnHGlobal( NULL, TRUE, &ilb );
    ok( r == S_OK, "CreateILockBytesOnHGlobal failed\n");

    mode = STGM_CREATE|STGM_SHARE_EXCLUSIVE|STGM_READWRITE;/*0x1012*/
    r = StgCreateDocfileOnILockBytes( ilb, mode, 0,  &stg );
    ok( r == S_OK, "StgCreateDocfileOnILockBytes failed\n");

    r = WriteClassStg( stg, &test_stg_cls );
    ok( r == S_OK, "WriteClassStg failed\n");

    memset( &stat, 0, sizeof stat );
    r = IStorage_Stat( stg, &stat, 0 );

    ok( stat.pwcsName == NULL, "storage name not null\n");
    ok( stat.type == 1, "type is wrong\n");
    ok( stat.grfMode == 0x12, "grf mode is incorrect\n");
    ok( !memcmp(&stat.clsid, &test_stg_cls, sizeof test_stg_cls), "CLSID is wrong\n");

    refcount = IStorage_Release( stg );
    ok( refcount == 0, "IStorage refcount is wrong\n");
    refcount = ILockBytes_Release( ilb );
    ok( refcount == 0, "ILockBytes refcount is wrong\n");
}

static void test_create_storage_modes(void)
{
   static const WCHAR szPrefix[] = { 's','t','g',0 };
   static const WCHAR szDot[] = { '.',0 };
   WCHAR filename[MAX_PATH];
   IStorage *stg = NULL;
   HRESULT r;

   if(!GetTempFileNameW(szDot, szPrefix, 0, filename))
      return;

   DeleteFileW(filename);

   /* test with some invalid parameters */
   r = StgCreateDocfile( NULL, 0, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, 0, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_CREATE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_CREATE | STGM_READWRITE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, NULL);
   ok(r==STG_E_INVALIDPOINTER, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 1, &stg);
   ok(r==STG_E_INVALIDPARAMETER, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_DENY_WRITE | STGM_READWRITE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READ, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_PRIORITY, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");

   /* StgCreateDocfile seems to be very particular about the flags it accepts */
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED | STGM_WRITE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED | 8, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED | 0x80, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED | 0x800, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED | 0x8000, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED | 0x80000, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED | 0x800000, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   ok(stg == NULL, "stg was set\n");

   /* check what happens if the file already exists (which is how it's meant to be used) */
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, &stg);
   ok(r==S_OK, "StgCreateDocfile failed\n");
   r = IStorage_Release(stg);
   ok(r == 0, "storage not released\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED, 0, &stg);
   ok(r==STG_E_FILEALREADYEXISTS, "StgCreateDocfile wrong error\n"); /* FAILIFTHERE is default */
   r = StgCreateDocfile( filename, STGM_READ, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n"); /* need at least readmode and sharemode */
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_SHARE_DENY_WRITE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_SHARE_DENY_NONE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile failed\n");
   r = StgCreateDocfile( filename, STGM_SHARE_DENY_NONE | STGM_TRANSACTED, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile failed\n");
   r = StgCreateDocfile( filename, STGM_SHARE_DENY_NONE | STGM_READWRITE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile failed\n");
   r = StgCreateDocfile( filename, STGM_SHARE_DENY_NONE | STGM_WRITE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile failed\n");
   r = StgCreateDocfile( filename, STGM_SHARE_DENY_WRITE | STGM_WRITE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile failed\n");
   r = StgCreateDocfile( filename, STGM_SHARE_DENY_WRITE | STGM_READ, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile wrong error\n");
   r = StgCreateDocfile( filename, STGM_TRANSACTED | STGM_SHARE_DENY_WRITE | STGM_READ, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile wrong error\n");
   ok(DeleteFileW(filename), "failed to delete file\n");

   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED, 0, &stg);
   ok(r==S_OK, "StgCreateDocfile failed\n");
   r = IStorage_Release(stg);
   ok(r == 0, "storage not released\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED |STGM_FAILIFTHERE, 0, &stg);
   ok(r==STG_E_FILEALREADYEXISTS, "StgCreateDocfile wrong error\n");
   r = StgCreateDocfile( filename, STGM_SHARE_EXCLUSIVE | STGM_WRITE, 0, &stg);
   ok(r==STG_E_FILEALREADYEXISTS, "StgCreateDocfile wrong error\n");

   r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_DENY_WRITE | STGM_READWRITE, 0, &stg);
   ok(r==STG_E_INVALIDFLAG, "StgCreateDocfile succeeded\n");
   r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED, 0, &stg);
   ok(r==S_OK, "StgCreateDocfile failed\n");
   r = IStorage_Release(stg);
   ok(r == 0, "storage not released\n");
   ok(DeleteFileW(filename), "failed to delete file\n");

   r = StgCreateDocfile( filename, STGM_CREATE | STGM_READWRITE |STGM_TRANSACTED, 0, &stg);
   ok(r==S_OK, "StgCreateDocfile failed\n");
   r = IStorage_Release(stg);
   ok(r == 0, "storage not released\n");
   ok(DeleteFileW(filename), "failed to delete file\n");

   /* test the way excel uses StgCreateDocFile */
   r = StgCreateDocfile( filename, STGM_TRANSACTED|STGM_CREATE|STGM_SHARE_DENY_WRITE|STGM_READWRITE, 0, &stg);
   ok(r==S_OK, "StgCreateDocfile the excel way failed\n");
   if(r == S_OK)
   {
      r = IStorage_Release(stg);
      ok(r == 0, "storage not released\n");
      ok(DeleteFileW(filename), "failed to delete file\n");
   }

   /* and the way windows media uses it ... */
   r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_DENY_NONE | STGM_READWRITE | STGM_TRANSACTED, 0, &stg);
   ok(r==S_OK, "StgCreateDocfile the windows media way failed\n");
   if (r == S_OK)
   {
      r = IStorage_Release(stg);
      ok(r == 0, "storage not released\n");
      ok(DeleteFileW(filename), "failed to delete file\n");
   }

   /* looks like we need STGM_TRANSACTED or STGM_CREATE */
   r = StgCreateDocfile( filename, STGM_TRANSACTED|STGM_SHARE_DENY_WRITE|STGM_READWRITE, 0, &stg);
   ok(r==S_OK, "StgCreateDocfile the excel way failed\n");
   if(r == S_OK)
   {
      r = IStorage_Release(stg);
      ok(r == 0, "storage not released\n");
      ok(DeleteFileW(filename), "failed to delete file\n");
   }

   r = StgCreateDocfile( filename, STGM_TRANSACTED|STGM_CREATE|STGM_SHARE_DENY_WRITE|STGM_WRITE, 0, &stg);
   ok(r==S_OK, "StgCreateDocfile the excel way failed\n");
   if(r == S_OK)
   {
      r = IStorage_Release(stg);
      ok(r == 0, "storage not released\n");
      ok(DeleteFileW(filename), "failed to delete file\n");
   }

   r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, &stg);
   ok(r==S_OK, "StgCreateDocfile the powerpoint way failed\n");
   if(r == S_OK)
   {
      r = IStorage_Release(stg);
      ok(r == 0, "storage not released\n");
      ok(DeleteFileW(filename), "failed to delete file\n");
   }

   /* test the way msi uses StgCreateDocfile */
   r = StgCreateDocfile( filename, STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &stg);
   ok(r==S_OK, "StgCreateDocFile failed\n");
   r = IStorage_Release(stg);
   ok(r == 0, "storage not released\n");
   ok(DeleteFileW(filename), "failed to delete file\n");
}

static void test_storage_stream(void)
{
    static const WCHAR stmname[] = { 'C','O','N','T','E','N','T','S',0 };
    static const WCHAR szPrefix[] = { 's','t','g',0 };
    static const WCHAR szDot[] = { '.',0 };
    static const WCHAR longname[] = {
        'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
        'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',0
    };
    WCHAR filename[MAX_PATH];
    IStorage *stg = NULL;
    HRESULT r;
    IStream *stm = NULL;
    IStream *stm2 = NULL;
    ULONG count = 0;
    LARGE_INTEGER pos;
    ULARGE_INTEGER p;
    unsigned char buffer[0x100];

    if(!GetTempFileNameW(szDot, szPrefix, 0, filename))
        return;

    DeleteFileW(filename);

    r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED, 0, &stg);
    ok(r==S_OK, "StgCreateDocfile failed\n");

    /* try create some invalid streams */
    r = IStorage_CreateStream(stg, stmname, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 1, 0, &stm );
    ok(r==STG_E_INVALIDPARAMETER, "IStorage->CreateStream wrong error\n");
    r = IStorage_CreateStream(stg, stmname, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 1, &stm );
    ok(r==STG_E_INVALIDPARAMETER, "IStorage->CreateStream wrong error\n");
    r = IStorage_CreateStream(stg, stmname, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, NULL );
    ok(r==STG_E_INVALIDPOINTER, "IStorage->CreateStream wrong error\n");
    r = IStorage_CreateStream(stg, NULL, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm );
    ok(r==STG_E_INVALIDNAME, "IStorage->CreateStream wrong error\n");
    r = IStorage_CreateStream(stg, longname, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm );
    ok(r==STG_E_INVALIDNAME || broken(r==S_OK) /* nt4 */,
       "IStorage->CreateStream wrong error, got %d GetLastError()=%d\n", r, GetLastError());
    r = IStorage_CreateStream(stg, stmname, STGM_READWRITE, 0, 0, &stm );
    ok(r==STG_E_INVALIDFLAG, "IStorage->CreateStream wrong error\n");
    r = IStorage_CreateStream(stg, stmname, STGM_READ, 0, 0, &stm );
    ok(r==STG_E_INVALIDFLAG, "IStorage->CreateStream wrong error\n");
    r = IStorage_CreateStream(stg, stmname, STGM_WRITE, 0, 0, &stm );
    ok(r==STG_E_INVALIDFLAG, "IStorage->CreateStream wrong error\n");
    r = IStorage_CreateStream(stg, stmname, STGM_SHARE_DENY_NONE | STGM_READWRITE, 0, 0, &stm );
    ok(r==STG_E_INVALIDFLAG, "IStorage->CreateStream wrong error\n");
    r = IStorage_CreateStream(stg, stmname, STGM_SHARE_DENY_NONE | STGM_READ, 0, 0, &stm );
    ok(r==STG_E_INVALIDFLAG, "IStorage->CreateStream wrong error\n");

    /* now really create a stream and delete it */
    r = IStorage_CreateStream(stg, stmname, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm );
    ok(r==S_OK, "IStorage->CreateStream failed\n");
    r = IStream_Release(stm);
    ok(r == 0, "wrong ref count\n");
    r = IStorage_CreateStream(stg, stmname, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm );
    ok(r==STG_E_FILEALREADYEXISTS, "IStorage->CreateStream failed\n");
    r = IStorage_DestroyElement(stg,stmname);
    ok(r==S_OK, "IStorage->DestroyElement failed\n");

    /* create a stream and write to it */
    r = IStorage_CreateStream(stg, stmname, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm );
    ok(r==S_OK, "IStorage->CreateStream failed\n");

    r = IStream_Clone(stm, &stm2);
    ok(r==S_OK, "failed to clone stream\n");

    r = IStream_Write(stm, NULL, 0, NULL );
    ok(r==STG_E_INVALIDPOINTER, "IStream->Write wrong error\n");
    r = IStream_Write(stm, "Hello\n", 0, NULL );
    ok(r==S_OK, "failed to write stream\n");
    r = IStream_Write(stm, "Hello\n", 0, &count );
    ok(r==S_OK, "failed to write stream\n");
    r = IStream_Write(stm, "Hello\n", 6, &count );
    ok(r==S_OK, "failed to write stream\n");
    r = IStream_Commit(stm, STGC_DEFAULT );
    ok(r==S_OK, "failed to commit stream\n");
    r = IStream_Commit(stm, STGC_DEFAULT );
    ok(r==S_OK, "failed to commit stream\n");

    /* seek round a bit, reset the stream size */
    pos.QuadPart = 0;
    r = IStream_Seek(stm, pos, 3, &p );
    ok(r==STG_E_INVALIDFUNCTION, "IStream->Seek returned wrong error\n");
    r = IStream_Seek(stm, pos, STREAM_SEEK_SET, NULL);
    ok(r==S_OK, "failed to seek stream\n");
    r = IStream_Seek(stm, pos, STREAM_SEEK_SET, &p );
    ok(r==S_OK, "failed to seek stream\n");
    r = IStream_SetSize(stm,p);
    ok(r==S_OK, "failed to set pos\n");
    pos.QuadPart = 10;
    r = IStream_Seek(stm, pos, STREAM_SEEK_SET, &p );
    ok(r==S_OK, "failed to seek stream\n");
    ok(p.QuadPart == 10, "at wrong place\n");
    pos.QuadPart = 0;
    r = IStream_Seek(stm, pos, STREAM_SEEK_END, &p );
    ok(r==S_OK, "failed to seek stream\n");
    ok(p.QuadPart == 0, "at wrong place\n");
    r = IStream_Read(stm, buffer, sizeof buffer, &count );
    ok(r==S_OK, "failed to set pos\n");
    ok(count == 0, "read bytes from empty stream\n");

    /* wrap up */
    r = IStream_Release(stm2);
    ok(r == 0, "wrong ref count\n");

    /* create a stream and write to it */
    r = IStorage_CreateStream(stg, stmname, STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm2 );
    ok(r==S_OK, "IStorage->CreateStream failed\n");

    r = IStream_Seek(stm, pos, STREAM_SEEK_SET, &p);
    ok(r==STG_E_REVERTED, "overwritten stream should return STG_E_REVERTED instead of 0x%08x\n", r);

    r = IStream_Release(stm2);
    ok(r == 0, "wrong ref count\n");
    r = IStream_Release(stm);
    ok(r == 0, "wrong ref count\n");

    r = IStorage_Release(stg);
    ok(r == 0, "wrong ref count\n");
    r = DeleteFileW(filename);
    ok(r, "file should exist\n");
}

static BOOL touch_file(LPCWSTR filename)
{
    HANDLE file;

    file = CreateFileW(filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, 
                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file==INVALID_HANDLE_VALUE)
        return FALSE;
    CloseHandle(file);
    return TRUE;
}

static BOOL is_zero_length(LPCWSTR filename)
{
    HANDLE file;
    DWORD len;

    file = CreateFileW(filename, GENERIC_READ, 0, NULL, 
                OPEN_EXISTING, 0, NULL);
    if (file==INVALID_HANDLE_VALUE)
        return FALSE;
    len = GetFileSize(file, NULL);
    CloseHandle(file);
    return len == 0;
}

static BOOL is_existing_file(LPCWSTR filename)
{
    HANDLE file;

    file = CreateFileW(filename, GENERIC_READ, 0, NULL,
                       OPEN_EXISTING, 0, NULL);
    if (file==INVALID_HANDLE_VALUE)
        return FALSE;
    CloseHandle(file);
    return TRUE;
}

static void test_open_storage(void)
{
    static const WCHAR szPrefix[] = { 's','t','g',0 };
    static const WCHAR szNonExist[] = { 'n','o','n','e','x','i','s','t',0 };
    static const WCHAR szDot[] = { '.',0 };
    WCHAR filename[MAX_PATH];
    IStorage *stg = NULL, *stg2 = NULL;
    HRESULT r;
    DWORD stgm;

    if(!GetTempFileNameW(szDot, szPrefix, 0, filename))
        return;

    /* try opening a zero length file - it should stay zero length */
    DeleteFileW(filename);
    touch_file(filename);
    stgm = STGM_NOSCRATCH | STGM_TRANSACTED | STGM_SHARE_DENY_WRITE | STGM_READWRITE;
    r = StgOpenStorage( filename, NULL, stgm, NULL, 0, &stg);
    ok(r==STG_E_FILEALREADYEXISTS, "StgOpenStorage didn't fail\n");

    stgm = STGM_SHARE_EXCLUSIVE | STGM_READWRITE;
    r = StgOpenStorage( filename, NULL, stgm, NULL, 0, &stg);
    ok(r==STG_E_FILEALREADYEXISTS, "StgOpenStorage didn't fail\n");
    ok(is_zero_length(filename), "file length changed\n");

    DeleteFileW(filename);

    /* try opening a nonexistent file - it should not create it */
    stgm = STGM_DIRECT | STGM_SHARE_EXCLUSIVE | STGM_READWRITE;
    r = StgOpenStorage( filename, NULL, stgm, NULL, 0, &stg);
    ok(r!=S_OK, "StgOpenStorage failed: 0x%08x\n", r);
    if (r==S_OK) IStorage_Release(stg);
    ok(!is_existing_file(filename), "StgOpenStorage should not create a file\n");
    DeleteFileW(filename);

    /* create the file */
    r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE |STGM_TRANSACTED, 0, &stg);
    ok(r==S_OK, "StgCreateDocfile failed\n");
    IStorage_Release(stg);

    r = StgOpenStorage( filename, NULL, 0, NULL, 0, &stg);
    ok(r==STG_E_INVALIDFLAG, "StgOpenStorage wrong error\n");
    r = StgOpenStorage( NULL, NULL, STGM_SHARE_EXCLUSIVE, NULL, 0, &stg);
    ok(r==STG_E_INVALIDNAME, "StgOpenStorage wrong error\n");
    r = StgOpenStorage( filename, NULL, STGM_SHARE_EXCLUSIVE | STGM_READ, NULL, 0, NULL);
    ok(r==STG_E_INVALIDPOINTER, "StgOpenStorage wrong error\n");
    r = StgOpenStorage( filename, NULL, STGM_SHARE_EXCLUSIVE | STGM_READ, NULL, 1, &stg);
    ok(r==STG_E_INVALIDPARAMETER, "StgOpenStorage wrong error\n");
    r = StgOpenStorage( szNonExist, NULL, STGM_SHARE_EXCLUSIVE | STGM_READ, NULL, 0, &stg);
    ok(r==STG_E_FILENOTFOUND, "StgOpenStorage failed\n");
    r = StgOpenStorage( filename, NULL, STGM_CREATE | STGM_SHARE_EXCLUSIVE | STGM_READ, NULL, 0, &stg);
    ok(r==STG_E_INVALIDFLAG, "StgOpenStorage failed\n");
    r = StgOpenStorage( filename, NULL, STGM_SHARE_DENY_NONE | STGM_READ, NULL, 0, &stg);
    ok(r==STG_E_INVALIDFLAG, "StgOpenStorage failed\n");
    r = StgOpenStorage( filename, NULL, STGM_SHARE_DENY_READ | STGM_READ, NULL, 0, &stg);
    ok(r==STG_E_INVALIDFLAG, "StgOpenStorage failed\n");
    r = StgOpenStorage( filename, NULL, STGM_SHARE_DENY_WRITE | STGM_READWRITE, NULL, 0, &stg);
    ok(r==STG_E_INVALIDFLAG, "StgOpenStorage failed\n");

    /* open it for real */
    r = StgOpenStorage( filename, NULL, STGM_SHARE_DENY_NONE | STGM_READ | STGM_TRANSACTED, NULL, 0, &stg); /* XLViewer 97/2000 */
    ok(r==S_OK, "StgOpenStorage failed\n");
    if(stg)
    {
        r = IStorage_Release(stg);
        ok(r == 0, "wrong ref count\n");
    }

    r = StgOpenStorage( filename, NULL, STGM_SHARE_DENY_WRITE | STGM_READ, NULL, 0, &stg);
    ok(r==S_OK, "StgOpenStorage failed\n");
    if(stg)
    {
        r = IStorage_Release(stg);
        ok(r == 0, "wrong ref count\n");
    }

    /* test the way word opens its custom dictionary */
    r = StgOpenStorage( filename, NULL, STGM_NOSCRATCH | STGM_TRANSACTED |
                        STGM_SHARE_DENY_WRITE | STGM_READWRITE, NULL, 0, &stg);
    ok(r==S_OK, "StgOpenStorage failed\n");
    if(stg)
    {
        r = IStorage_Release(stg);
        ok(r == 0, "wrong ref count\n");
    }

    r = StgOpenStorage( filename, NULL, STGM_SHARE_EXCLUSIVE | STGM_READ, NULL, 0, &stg);
    ok(r==S_OK, "StgOpenStorage failed\n");
    r = StgOpenStorage( filename, NULL, STGM_SHARE_EXCLUSIVE | STGM_READ, NULL, 0, &stg2);
    ok(r==STG_E_SHAREVIOLATION, "StgOpenStorage failed\n");
    if(stg)
    {
        r = IStorage_Release(stg);
        ok(r == 0, "wrong ref count\n");
    }

    /* now try write to a storage file we opened read-only */ 
    r = StgOpenStorage( filename, NULL, STGM_SHARE_EXCLUSIVE | STGM_READ, NULL, 0, &stg);
    ok(r==S_OK, "StgOpenStorage failed\n");
    if(stg)
    { 
        static const WCHAR stmname[] =  { 'w','i','n','e','t','e','s','t',0};
        IStream *stm = NULL;
        IStorage *stg2 = NULL;

        r = IStorage_CreateStream( stg, stmname, STGM_WRITE | STGM_SHARE_EXCLUSIVE,
                                   0, 0, &stm );
        ok(r == STG_E_ACCESSDENIED, "CreateStream should fail\n");
        r = IStorage_CreateStorage( stg, stmname, STGM_WRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stg2);
        ok(r == STG_E_ACCESSDENIED, "CreateStream should fail\n");

        r = IStorage_Release(stg);
        ok(r == 0, "wrong ref count\n");
    }

    /* open like visio 2003 */
    stg = NULL;
    r = StgOpenStorage( filename, NULL, STGM_PRIORITY | STGM_SHARE_DENY_NONE, NULL, 0, &stg);
    ok(r == S_OK, "should succeed\n");
    if (stg)
        IStorage_Release(stg);

    /* test other sharing modes with STGM_PRIORITY */
    stg = NULL;
    r = StgOpenStorage( filename, NULL, STGM_PRIORITY | STGM_SHARE_EXCLUSIVE, NULL, 0, &stg);
    ok(r == S_OK, "should succeed\n");
    if (stg)
        IStorage_Release(stg);

    stg = NULL;
    r = StgOpenStorage( filename, NULL, STGM_PRIORITY | STGM_SHARE_DENY_WRITE, NULL, 0, &stg);
    ok(r == S_OK, "should succeed\n");
    if (stg)
        IStorage_Release(stg);

    stg = NULL;
    r = StgOpenStorage( filename, NULL, STGM_PRIORITY | STGM_SHARE_DENY_READ, NULL, 0, &stg);
    ok(r == S_OK, "should succeed\n");
    if (stg)
        IStorage_Release(stg);

    /* open like Project 2003 */
    stg = NULL;
    r = StgOpenStorage( filename, NULL, STGM_PRIORITY, NULL, 0, &stg);
    ok(r == S_OK, "should succeed\n");
    r = StgOpenStorage( filename, NULL, STGM_PRIORITY, NULL, 0, &stg2);
    ok(r == S_OK, "should succeed\n");
    if (stg2)
        IStorage_Release(stg2);
    if (stg)
        IStorage_Release(stg);

    stg = NULL;
    r = StgOpenStorage( filename, NULL, STGM_PRIORITY | STGM_READWRITE, NULL, 0, &stg);
    ok(r == STG_E_INVALIDFLAG, "should fail\n");

    r = StgOpenStorage( filename, NULL, STGM_TRANSACTED | STGM_PRIORITY, NULL, 0, &stg);
    ok(r == STG_E_INVALIDFLAG, "should fail\n");

    r = StgOpenStorage( filename, NULL, STGM_SIMPLE | STGM_PRIORITY, NULL, 0, &stg);
    ok(r == STG_E_INVALIDFLAG, "should fail\n");

    r = StgOpenStorage( filename, NULL, STGM_DELETEONRELEASE | STGM_PRIORITY, NULL, 0, &stg);
    ok(r == STG_E_INVALIDFUNCTION, "should fail\n");

    r = StgOpenStorage( filename, NULL, STGM_NOSCRATCH | STGM_PRIORITY, NULL, 0, &stg);
    ok(r == STG_E_INVALIDFLAG, "should fail\n");

    r = StgOpenStorage( filename, NULL, STGM_NOSNAPSHOT | STGM_PRIORITY, NULL, 0, &stg);
    ok(r == STG_E_INVALIDFLAG, "should fail\n");

    r = DeleteFileW(filename);
    ok(r, "file didn't exist\n");
}

static void test_storage_suminfo(void)
{
    static const WCHAR szDot[] = { '.',0 };
    static const WCHAR szPrefix[] = { 's','t','g',0 };
    WCHAR filename[MAX_PATH];
    IStorage *stg = NULL;
    IPropertySetStorage *propset = NULL;
    IPropertyStorage *ps = NULL;
    HRESULT r;

    if(!GetTempFileNameW(szDot, szPrefix, 0, filename))
        return;

    DeleteFileW(filename);

    /* create the file */
    r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE | 
                            STGM_READWRITE |STGM_TRANSACTED, 0, &stg);
    ok(r==S_OK, "StgCreateDocfile failed\n");

    r = IStorage_QueryInterface( stg, &IID_IPropertySetStorage, (LPVOID) &propset );
    ok(r == S_OK, "query interface failed\n");

    /* delete it */
    r = IPropertySetStorage_Delete( propset, &FMTID_SummaryInformation );
    ok(r == STG_E_FILENOTFOUND, "deleted property set storage\n");

    r = IPropertySetStorage_Open( propset, &FMTID_SummaryInformation, 
                                STGM_READ | STGM_SHARE_EXCLUSIVE, &ps );
    ok(r == STG_E_FILENOTFOUND, "opened property set storage\n");

    r = IPropertySetStorage_Create( propset, &FMTID_SummaryInformation, NULL, 0,
                                STGM_READ | STGM_SHARE_EXCLUSIVE, &ps );
    ok(r == STG_E_INVALIDFLAG, "created property set storage\n");

    r = IPropertySetStorage_Create( propset, &FMTID_SummaryInformation, NULL, 0,
                                STGM_READ, &ps );
    ok(r == STG_E_INVALIDFLAG, "created property set storage\n");

    r = IPropertySetStorage_Create( propset, &FMTID_SummaryInformation, NULL, 0, 0, &ps );
    ok(r == STG_E_INVALIDFLAG, "created property set storage\n");

    r = IPropertySetStorage_Create( propset, &FMTID_SummaryInformation, NULL, 0,
                                STGM_WRITE|STGM_SHARE_EXCLUSIVE, &ps );
    ok(r == STG_E_INVALIDFLAG, "created property set storage\n");

    r = IPropertySetStorage_Create( propset, &FMTID_SummaryInformation, NULL, 0,
                                STGM_CREATE|STGM_WRITE|STGM_SHARE_EXCLUSIVE, &ps );
    ok(r == STG_E_INVALIDFLAG, "created property set storage\n");

    /* now try really creating a property set */
    r = IPropertySetStorage_Create( propset, &FMTID_SummaryInformation, NULL, 0,
                                STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE, &ps );
    ok(r == S_OK, "failed to create property set storage\n");

    if( ps )
        IPropertyStorage_Release(ps);

    /* now try creating the same thing again */
    r = IPropertySetStorage_Create( propset, &FMTID_SummaryInformation, NULL, 0,
                                STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE, &ps );
    ok(r == S_OK, "failed to create property set storage\n");
    if( ps )
        IPropertyStorage_Release(ps);

    /* should be able to open it */
    r = IPropertySetStorage_Open( propset, &FMTID_SummaryInformation, 
            STGM_READWRITE|STGM_SHARE_EXCLUSIVE, &ps);
    ok(r == S_OK, "open failed\n");
    if(r == S_OK)
        IPropertyStorage_Release(ps);

    /* delete it */
    r = IPropertySetStorage_Delete( propset, &FMTID_SummaryInformation );
    ok(r == S_OK, "failed to delete property set storage\n");

    /* try opening with an invalid FMTID */
    r = IPropertySetStorage_Open( propset, NULL, 
            STGM_READWRITE|STGM_SHARE_EXCLUSIVE, &ps);
    ok(r == E_INVALIDARG, "open succeeded\n");
    if(r == S_OK)
        IPropertyStorage_Release(ps);

    /* try a bad guid */
    r = IPropertySetStorage_Open( propset, &IID_IStorage, 
            STGM_READWRITE|STGM_SHARE_EXCLUSIVE, &ps);
    ok(r == STG_E_FILENOTFOUND, "open succeeded\n");
    if(r == S_OK)
        IPropertyStorage_Release(ps);
    

    /* try some invalid flags */
    r = IPropertySetStorage_Open( propset, &FMTID_SummaryInformation, 
            STGM_CREATE | STGM_READWRITE|STGM_SHARE_EXCLUSIVE, &ps);
    ok(r == STG_E_INVALIDFLAG, "open succeeded\n");
    if(r == S_OK)
        IPropertyStorage_Release(ps);

    /* after deleting it, it should be gone */
    r = IPropertySetStorage_Open( propset, &FMTID_SummaryInformation, 
            STGM_READWRITE|STGM_SHARE_EXCLUSIVE, &ps);
    ok(r == STG_E_FILENOTFOUND, "open failed\n");
    if(r == S_OK)
        IPropertyStorage_Release(ps);

    r = IPropertySetStorage_Release( propset );
    ok(r == 1, "ref count wrong\n");

    r = IStorage_Release(stg);
    ok(r == 0, "ref count wrong\n");

    DeleteFileW(filename);
}

static void test_storage_refcount(void)
{
    static const WCHAR szPrefix[] = { 's','t','g',0 };
    static const WCHAR szDot[] = { '.',0 };
    WCHAR filename[MAX_PATH];
    IStorage *stg = NULL;
    IStorage *stgprio = NULL;
    HRESULT r;
    IStream *stm = NULL;
    static const WCHAR stmname[] = { 'C','O','N','T','E','N','T','S',0 };
    LARGE_INTEGER pos;
    ULARGE_INTEGER upos;
    STATSTG stat;
    char buffer[10];

    if(!GetTempFileNameW(szDot, szPrefix, 0, filename))
        return;

    DeleteFileW(filename);

    /* create the file */
    r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE | 
                            STGM_READWRITE |STGM_TRANSACTED, 0, &stg);
    ok(r==S_OK, "StgCreateDocfile failed\n");

    r = WriteClassStg( stg, &test_stg_cls );
    ok( r == S_OK, "WriteClassStg failed\n");

    r = IStorage_Commit( stg, STGC_DEFAULT );
    ok( r == S_OK, "IStorage_Commit failed\n");

    /* now create a stream */
    r = IStorage_CreateStream(stg, stmname, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm );
    ok(r==S_OK, "IStorage->CreateStream failed\n");

    r = IStorage_Release( stg );
    ok (r == 0, "storage not released\n");

    pos.QuadPart = 0;
    r = IStream_Seek( stm, pos, 0, &upos );
    ok (r == STG_E_REVERTED, "seek should fail\n");

    r = IStream_Stat( stm, &stat, STATFLAG_DEFAULT );
    ok (r == STG_E_REVERTED, "stat should fail\n");

    r = IStream_Write( stm, "Test string", strlen("Test string"), NULL);
    ok (r == STG_E_REVERTED, "IStream_Write should return STG_E_REVERTED instead of 0x%08x\n", r);

    r = IStream_Read( stm, buffer, sizeof(buffer), NULL);
    ok (r == STG_E_REVERTED, "IStream_Read should return STG_E_REVERTED instead of 0x%08x\n", r);

    r = IStream_Release(stm);
    ok (r == 0, "stream not released\n");

    /* tests that STGM_PRIORITY doesn't prevent readwrite access from other
     * StgOpenStorage calls in transacted mode */
    r = StgOpenStorage( filename, NULL, STGM_PRIORITY, NULL, 0, &stgprio);
    ok(r==S_OK, "StgOpenStorage failed with error 0x%08x\n", r);

    todo_wine {
    /* non-transacted mode read/write fails */
    r = StgOpenStorage( filename, NULL, STGM_SHARE_EXCLUSIVE|STGM_READWRITE, NULL, 0, &stg);
    ok(r==STG_E_LOCKVIOLATION, "StgOpenStorage should return STG_E_LOCKVIOLATION instead of 0x%08x\n", r);
    }

    /* non-transacted mode read-only succeeds */
    r = StgOpenStorage( filename, NULL, STGM_SHARE_DENY_WRITE|STGM_READ, NULL, 0, &stg);
    ok(r==S_OK, "StgOpenStorage failed with error 0x%08x\n", r);
    IStorage_Release(stg);

    r = StgOpenStorage( filename, NULL, STGM_TRANSACTED|STGM_SHARE_DENY_WRITE|STGM_READWRITE, NULL, 0, &stg);
    ok(r==S_OK, "StgOpenStorage failed with error 0x%08x\n", r);
    if(stg)
    {
        static const WCHAR stgname[] = { ' ',' ',' ','2','9',0 };
        static const WCHAR stgname2[] = { 'C','V','_','i','e','w',0 };
        static const WCHAR stmname2[] = { 'V','a','r','2','D','a','t','a',0 };
        IStorage *stg2;
        IStorage *stg3;
        STATSTG statstg;

        r = IStorage_Stat( stg, &statstg, STATFLAG_NONAME );
        ok(r == S_OK, "Stat should have succeded instead of returning 0x%08x\n", r);
        ok(statstg.type == STGTY_STORAGE, "Statstg type should have been STGTY_STORAGE instead of %d\n", statstg.type);
        ok(U(statstg.cbSize).LowPart == 0, "Statstg cbSize.LowPart should have been 0 instead of %d\n", U(statstg.cbSize).LowPart);
        ok(U(statstg.cbSize).HighPart == 0, "Statstg cbSize.HighPart should have been 0 instead of %d\n", U(statstg.cbSize).HighPart);
        ok(statstg.grfMode == (STGM_TRANSACTED|STGM_SHARE_DENY_WRITE|STGM_READWRITE),
            "Statstg grfMode should have been 0x10022 instead of 0x%x\n", statstg.grfMode);
        ok(statstg.grfLocksSupported == 0, "Statstg grfLocksSupported should have been 0 instead of %d\n", statstg.grfLocksSupported);
        ok(IsEqualCLSID(&statstg.clsid, &test_stg_cls), "Statstg clsid is not test_stg_cls\n");
        ok(statstg.grfStateBits == 0, "Statstg grfStateBits should have been 0 instead of %d\n", statstg.grfStateBits);
        ok(statstg.reserved == 0, "Statstg reserved should have been 0 instead of %d\n", statstg.reserved);

        r = IStorage_CreateStorage( stg, stgname, STGM_SHARE_EXCLUSIVE, 0, 0, &stg2 );
        ok(r == S_OK, "CreateStorage should have succeeded instead of returning 0x%08x\n", r);

        r = IStorage_Stat( stg2, &statstg, STATFLAG_DEFAULT );
        ok(r == S_OK, "Stat should have succeded instead of returning 0x%08x\n", r);
        ok(!lstrcmpW(statstg.pwcsName, stgname),
            "Statstg pwcsName should have been the name the storage was created with\n");
        ok(statstg.type == STGTY_STORAGE, "Statstg type should have been STGTY_STORAGE instead of %d\n", statstg.type);
        ok(U(statstg.cbSize).LowPart == 0, "Statstg cbSize.LowPart should have been 0 instead of %d\n", U(statstg.cbSize).LowPart);
        ok(U(statstg.cbSize).HighPart == 0, "Statstg cbSize.HighPart should have been 0 instead of %d\n", U(statstg.cbSize).HighPart);
        ok(statstg.grfMode == STGM_SHARE_EXCLUSIVE,
            "Statstg grfMode should have been STGM_SHARE_EXCLUSIVE instead of 0x%x\n", statstg.grfMode);
        ok(statstg.grfLocksSupported == 0, "Statstg grfLocksSupported should have been 0 instead of %d\n", statstg.grfLocksSupported);
        ok(IsEqualCLSID(&statstg.clsid, &CLSID_NULL), "Statstg clsid is not CLSID_NULL\n");
        ok(statstg.grfStateBits == 0, "Statstg grfStateBits should have been 0 instead of %d\n", statstg.grfStateBits);
        ok(statstg.reserved == 0, "Statstg reserved should have been 0 instead of %d\n", statstg.reserved);
        CoTaskMemFree(statstg.pwcsName);

        r = IStorage_CreateStorage( stg2, stgname2, STGM_SHARE_EXCLUSIVE|STGM_READWRITE, 0, 0, &stg3 );
        ok(r == STG_E_ACCESSDENIED, "CreateStorage should have returned STG_E_ACCESSDENIED instead of 0x%08x\n", r);

        r = IStorage_CreateStream( stg2, stmname2, STGM_CREATE|STGM_SHARE_EXCLUSIVE, 0, 0, &stm );
        ok(r == STG_E_ACCESSDENIED, "CreateStream should have returned STG_E_ACCESSDENIED instead of 0x%08x\n", r);

        IStorage_Release(stg2);

        r = IStorage_Release(stg);
        ok(r == 0, "wrong ref count\n");
    }
    IStorage_Release(stgprio);

    DeleteFileW(filename);
}

static void test_streamenum(void)
{
    static const WCHAR szPrefix[] = { 's','t','g',0 };
    static const WCHAR szDot[] = { '.',0 };
    WCHAR filename[MAX_PATH];
    IStorage *stg = NULL;
    HRESULT r;
    IStream *stm = NULL;
    static const WCHAR stmname[] = { 'C','O','N','T','E','N','T','S',0 };
    STATSTG stat;
    IEnumSTATSTG *ee = NULL;
    ULONG count;

    if(!GetTempFileNameW(szDot, szPrefix, 0, filename))
        return;

    DeleteFileW(filename);

    /* create the file */
    r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE |
                            STGM_READWRITE |STGM_TRANSACTED, 0, &stg);
    ok(r==S_OK, "StgCreateDocfile failed\n");

    r = WriteClassStg( stg, &test_stg_cls );
    ok( r == S_OK, "WriteClassStg failed\n");

    r = IStorage_Commit( stg, STGC_DEFAULT );
    ok( r == S_OK, "IStorage_Commit failed\n");

    /* now create a stream */
    r = IStorage_CreateStream(stg, stmname, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm );
    ok(r==S_OK, "IStorage->CreateStream failed\n");

    r = IStream_Release(stm);

    /* first enum ... should be 1 stream */
    r = IStorage_EnumElements(stg, 0, NULL, 0, &ee);
    ok(r==S_OK, "IStorage->EnumElements failed\n");

    count = 0xf00;
    r = IEnumSTATSTG_Next(ee, 1, &stat, &count);
    ok(r==S_OK, "IEnumSTATSTG->Next failed\n");
    ok(count == 1, "count wrong\n");

    r = IEnumSTATSTG_Release(ee);

    /* second enum... destroy the stream before reading */
    r = IStorage_EnumElements(stg, 0, NULL, 0, &ee);
    ok(r==S_OK, "IStorage->EnumElements failed\n");

    r = IStorage_DestroyElement(stg, stmname);
    ok(r==S_OK, "IStorage->EnumElements failed\n");

    todo_wine {
    count = 0xf00;
    r = IEnumSTATSTG_Next(ee, 1, &stat, &count);
    ok(r==S_FALSE, "IEnumSTATSTG->Next failed\n");
    ok(count == 0, "count wrong\n");
    }

    /* reset and try again */
    r = IEnumSTATSTG_Reset(ee);
    ok(r==S_OK, "IEnumSTATSTG->Reset failed\n");

    count = 0xf00;
    r = IEnumSTATSTG_Next(ee, 1, &stat, &count);
    ok(r==S_FALSE, "IEnumSTATSTG->Next failed\n");
    ok(count == 0, "count wrong\n");

    r = IEnumSTATSTG_Release(ee);
    ok (r == 0, "enum not released\n");

    r = IStorage_Release( stg );
    ok (r == 0, "storage not released\n");

    DeleteFileW(filename);
}

static void test_transact(void)
{
    static const WCHAR szPrefix[] = { 's','t','g',0 };
    static const WCHAR szDot[] = { '.',0 };
    WCHAR filename[MAX_PATH];
    IStorage *stg = NULL, *stg2 = NULL;
    HRESULT r;
    IStream *stm = NULL;
    static const WCHAR stmname[] = { 'C','O','N','T','E','N','T','S',0 };
    static const WCHAR stmname2[] = { 'F','O','O',0 };

    if(!GetTempFileNameW(szDot, szPrefix, 0, filename))
        return;

    DeleteFileW(filename);

    /* create the file */
    r = StgCreateDocfile( filename, STGM_CREATE | STGM_SHARE_EXCLUSIVE | 
                            STGM_READWRITE |STGM_TRANSACTED, 0, &stg);
    ok(r==S_OK, "StgCreateDocfile failed\n");

    /* now create a stream, but don't commit it */
    r = IStorage_CreateStream(stg, stmname2, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm );
    ok(r==S_OK, "IStorage->CreateStream failed\n");

    r = IStream_Write(stm, "this is stream 1\n", 16, NULL);
    ok(r==S_OK, "IStream->Write failed\n");

    r = IStream_Release(stm);

    r = IStorage_Commit(stg, 0);
    ok(r==S_OK, "IStorage->Commit failed\n");

    /* now create a stream, but don't commit it */
    stm = NULL;
    r = IStorage_CreateStream(stg, stmname, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm );
    ok(r==S_OK, "IStorage->CreateStream failed\n");

    r = IStream_Write(stm, "this is stream 2\n", 16, NULL);
    ok(r==S_OK, "IStream->Write failed\n");

    r = IStream_Commit(stm, STGC_ONLYIFCURRENT | STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE);
    ok(r==S_OK, "IStream->Commit failed\n");

    r = IStream_Release(stm);

    IStorage_Release(stg);

    stm = NULL;
    stg = NULL;
    r = StgOpenStorage( filename, NULL, STGM_SHARE_DENY_NONE | STGM_READ | STGM_TRANSACTED, NULL, 0, &stg);
    ok(r==S_OK, "StgOpenStorage failed\n");

    if (!stg)
        return;

    r = IStorage_OpenStream(stg, stmname, NULL, STGM_SHARE_DENY_NONE|STGM_READ, 0, &stm );
    ok(r==STG_E_INVALIDFLAG, "IStorage->OpenStream failed %08x\n", r);

    r = IStorage_OpenStream(stg, stmname, NULL, STGM_DELETEONRELEASE|STGM_SHARE_EXCLUSIVE|STGM_READWRITE, 0, &stm );
    ok(r==STG_E_INVALIDFUNCTION, "IStorage->OpenStream failed %08x\n", r);

    r = IStorage_OpenStream(stg, stmname, NULL, STGM_TRANSACTED|STGM_SHARE_EXCLUSIVE|STGM_READWRITE, 0, &stm );
    ok(r==STG_E_INVALIDFUNCTION, "IStorage->OpenStream failed %08x\n", r);

    r = IStorage_OpenStorage(stg, stmname, NULL, STGM_TRANSACTED|STGM_SHARE_EXCLUSIVE|STGM_READWRITE, NULL, 0, &stg2 );
    ok(r==STG_E_FILENOTFOUND, "IStorage->OpenStream failed %08x\n", r);

    todo_wine {
    r = IStorage_OpenStream(stg, stmname, NULL, STGM_SHARE_EXCLUSIVE|STGM_READWRITE, 0, &stm );
    ok(r==STG_E_FILENOTFOUND, "IStorage->OpenStream should fail %08x\n", r);
    }
    if (stm)
        r = IStream_Release(stm);

    r = IStorage_OpenStorage(stg, stmname2, NULL, STGM_TRANSACTED|STGM_SHARE_EXCLUSIVE|STGM_READWRITE, NULL, 0, &stg2 );
    ok(r==STG_E_FILENOTFOUND, "IStorage->OpenStream failed %08x\n", r);

    r = IStorage_OpenStream(stg, stmname2, NULL, STGM_SHARE_EXCLUSIVE|STGM_READWRITE, 0, &stm );
    ok(r==S_OK, "IStorage->OpenStream should fail %08x\n", r);
    if (stm)
        r = IStream_Release(stm);

    IStorage_Release(stg);

    r = DeleteFileW(filename);
    ok( r == TRUE, "deleted file\n");
}

static void test_ReadClassStm(void)
{
    CLSID clsid;
    HRESULT hr;
    IStream *pStream;
    static const LARGE_INTEGER llZero;

    hr = ReadClassStm(NULL, &clsid);
    ok(hr == E_INVALIDARG, "ReadClassStm should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, "CreateStreamOnHGlobal");
    hr = WriteClassStm(pStream, &test_stg_cls);
    ok_ole_success(hr, "WriteClassStm");

    hr = ReadClassStm(pStream, NULL);
    ok(hr == E_INVALIDARG, "ReadClassStm should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    /* test not rewound stream */
    hr = ReadClassStm(pStream, &clsid);
    ok(hr == STG_E_READFAULT, "ReadClassStm should have returned STG_E_READFAULT instead of 0x%08x\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_NULL), "clsid should have been zeroed\n");

    hr = IStream_Seek(pStream, llZero, STREAM_SEEK_SET, NULL);
    ok_ole_success(hr, "IStream_Seek");
    hr = ReadClassStm(pStream, &clsid);
    ok_ole_success(hr, "ReadClassStm");
    ok(IsEqualCLSID(&clsid, &test_stg_cls), "clsid should have been set to CLSID_WineTest\n");
}

struct access_res
{
    BOOL gothandle;
    DWORD lasterr;
    BOOL ignore;
};

static const struct access_res create[16] =
{
    { TRUE, ERROR_SUCCESS, TRUE },
    { TRUE, ERROR_SUCCESS, TRUE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { TRUE, ERROR_SUCCESS, TRUE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { TRUE, ERROR_SUCCESS, TRUE }
};

static const struct access_res create_commit[16] =
{
    { TRUE, ERROR_SUCCESS, TRUE },
    { TRUE, ERROR_SUCCESS, TRUE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { TRUE, ERROR_SUCCESS, TRUE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { FALSE, ERROR_SHARING_VIOLATION, FALSE },
    { TRUE, ERROR_SUCCESS, TRUE }
};

static const struct access_res create_close[16] =
{
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS, FALSE },
    { TRUE, ERROR_SUCCESS }
};

static void _test_file_access(LPCSTR file, const struct access_res *ares, DWORD line)
{
    DWORD access = 0, share = 0;
    DWORD lasterr;
    HANDLE hfile;
    int i, j, idx = 0;

    for (i = 0; i < 4; i++)
    {
        if (i == 0) access = 0;
        if (i == 1) access = GENERIC_READ;
        if (i == 2) access = GENERIC_WRITE;
        if (i == 3) access = GENERIC_READ | GENERIC_WRITE;

        for (j = 0; j < 4; j++)
        {
            if (ares[idx].ignore)
                continue;

            if (j == 0) share = 0;
            if (j == 1) share = FILE_SHARE_READ;
            if (j == 2) share = FILE_SHARE_WRITE;
            if (j == 3) share = FILE_SHARE_READ | FILE_SHARE_WRITE;

            SetLastError(0xdeadbeef);
            hfile = CreateFileA(file, access, share, NULL, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, 0);
            lasterr = GetLastError();

            ok((hfile != INVALID_HANDLE_VALUE) == ares[idx].gothandle,
               "(%d, handle, %d): Expected %d, got %d\n",
               line, idx, ares[idx].gothandle,
               (hfile != INVALID_HANDLE_VALUE));

            ok(lasterr == ares[idx].lasterr ||
               broken(lasterr == 0xdeadbeef) /* win9x */,
               "(%d, lasterr, %d): Expected %d, got %d\n",
               line, idx, ares[idx].lasterr, lasterr);

            CloseHandle(hfile);
            idx++;
        }
    }
}

#define test_file_access(file, ares) _test_file_access(file, ares, __LINE__)

static void test_access(void)
{
    IStorage *stg;
    HRESULT hr;

    static const WCHAR fileW[] = {'w','i','n','e','t','e','s','t',0};

    /* STGM_TRANSACTED */

    hr = StgCreateDocfile(fileW, STGM_CREATE | STGM_READWRITE |
                          STGM_SHARE_EXCLUSIVE | STGM_TRANSACTED, 0, &stg);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    test_file_access("winetest", create);

    hr = IStorage_Commit(stg, STGC_DEFAULT);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    test_file_access("winetest", create_commit);

    IStorage_Release(stg);

    test_file_access("winetest", create_close);

    DeleteFileA("winetest");

    /* STGM_DIRECT */

    hr = StgCreateDocfile(fileW, STGM_CREATE | STGM_READWRITE |
                          STGM_SHARE_EXCLUSIVE | STGM_DIRECT, 0, &stg);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    test_file_access("winetest", create);

    hr = IStorage_Commit(stg, STGC_DEFAULT);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    test_file_access("winetest", create_commit);

    IStorage_Release(stg);

    test_file_access("winetest", create_close);

    DeleteFileA("winetest");

    /* STGM_SHARE_DENY_NONE */

    hr = StgCreateDocfile(fileW, STGM_CREATE | STGM_READWRITE |
                          STGM_SHARE_DENY_NONE | STGM_TRANSACTED, 0, &stg);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    test_file_access("winetest", create);

    hr = IStorage_Commit(stg, STGC_DEFAULT);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    test_file_access("winetest", create_commit);

    IStorage_Release(stg);

    test_file_access("winetest", create_close);

    DeleteFileA("winetest");

    /* STGM_SHARE_DENY_READ */

    hr = StgCreateDocfile(fileW, STGM_CREATE | STGM_READWRITE |
                          STGM_SHARE_DENY_READ | STGM_TRANSACTED, 0, &stg);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    test_file_access("winetest", create);

    hr = IStorage_Commit(stg, STGC_DEFAULT);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    test_file_access("winetest", create_commit);

    IStorage_Release(stg);

    test_file_access("winetest", create_close);

    DeleteFileA("winetest");

    /* STGM_SHARE_DENY_WRITE */

    hr = StgCreateDocfile(fileW, STGM_CREATE | STGM_READWRITE |
                          STGM_SHARE_DENY_WRITE | STGM_TRANSACTED, 0, &stg);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    test_file_access("winetest", create);

    hr = IStorage_Commit(stg, STGC_DEFAULT);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    test_file_access("winetest", create_commit);

    IStorage_Release(stg);

    test_file_access("winetest", create_close);

    DeleteFileA("winetest");
}

START_TEST(storage32)
{
    test_hglobal_storage_stat();
    test_create_storage_modes();
    test_storage_stream();
    test_open_storage();
    test_storage_suminfo();
    test_storage_refcount();
    test_streamenum();
    test_transact();
    test_ReadClassStm();
    test_access();
}
