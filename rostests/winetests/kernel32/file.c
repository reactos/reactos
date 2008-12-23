/*
 * Unit tests for file functions in Wine
 *
 * Copyright (c) 2002, 2004 Jakob Eriksson
 * Copyright (c) 2008 Jeff Zaroyko
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
 *
 */

#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"

static HANDLE (WINAPI *pFindFirstFileExA)(LPCSTR,FINDEX_INFO_LEVELS,LPVOID,FINDEX_SEARCH_OPS,LPVOID,DWORD);
static BOOL (WINAPI *pReplaceFileA)(LPCSTR, LPCSTR, LPCSTR, DWORD, LPVOID, LPVOID);
static BOOL (WINAPI *pReplaceFileW)(LPCWSTR, LPCWSTR, LPCWSTR, DWORD, LPVOID, LPVOID);

/* keep filename and filenameW the same */
static const char filename[] = "testfile.xxx";
static const WCHAR filenameW[] = { 't','e','s','t','f','i','l','e','.','x','x','x',0 };
static const char sillytext[] =
"en larvig liten text dx \033 gx hej 84 hej 4484 ! \001\033 bla bl\na.. bla bla."
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"sdlkfjasdlkfj a dslkj adsklf  \n  \nasdklf askldfa sdlkf \nsadklf asdklf asdf ";

static void InitFunctionPointers(void)
{
    HMODULE hkernel32 = GetModuleHandleA("kernel32");

    pFindFirstFileExA=(void*)GetProcAddress(hkernel32, "FindFirstFileExA");
    pReplaceFileA=(void*)GetProcAddress(hkernel32, "ReplaceFileA");
    pReplaceFileW=(void*)GetProcAddress(hkernel32, "ReplaceFileW");
}

static void test__hread( void )
{
    HFILE filehandle;
    char buffer[10000];
    long bytes_read;
    long bytes_wanted;
    long i;
    BOOL ret;

    SetFileAttributesA(filename,FILE_ATTRIBUTE_NORMAL); /* be sure to remove stale files */
    DeleteFileA( filename );
    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%d)\n",filename,GetLastError());
        return;
    }

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains\n" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    filehandle = _lopen( filename, OF_READ );

    ok( HFILE_ERROR != filehandle, "couldn't open file \"%s\" again (err=%d)\n", filename, GetLastError(  ) );

    bytes_read = _hread( filehandle, buffer, 2 * strlen( sillytext ) );

    ok( lstrlenA( sillytext ) == bytes_read, "file read size error\n" );

    for (bytes_wanted = 0; bytes_wanted < lstrlenA( sillytext ); bytes_wanted++)
    {
        ok( 0 == _llseek( filehandle, 0, FILE_BEGIN ), "_llseek complains\n" );
        ok( _hread( filehandle, buffer, bytes_wanted ) == bytes_wanted, "erratic _hread return value\n" );
        for (i = 0; i < bytes_wanted; i++)
        {
            ok( buffer[i] == sillytext[i], "that's not what's written\n" );
        }
    }

    ok( HFILE_ERROR != _lclose( filehandle ), "_lclose complains\n" );

    ret = DeleteFileA( filename );
    ok( ret != 0, "DeleteFile failed (%d)\n", GetLastError(  ) );
}


static void test__hwrite( void )
{
    HFILE filehandle;
    char buffer[10000];
    long bytes_read;
    long bytes_written;
    long blocks;
    long i;
    char *contents;
    HLOCAL memory_object;
    char checksum[1];
    BOOL ret;

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%d)\n",filename,GetLastError());
        return;
    }

    ok( HFILE_ERROR != _hwrite( filehandle, "", 0 ), "_hwrite complains\n" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    filehandle = _lopen( filename, OF_READ );

    bytes_read = _hread( filehandle, buffer, 1);

    ok( 0 == bytes_read, "file read size error\n" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    filehandle = _lopen( filename, OF_READWRITE );

    bytes_written = 0;
    checksum[0] = '\0';
    srand( (unsigned)time( NULL ) );
    for (blocks = 0; blocks < 100; blocks++)
    {
        for (i = 0; i < (long)sizeof( buffer ); i++)
        {
            buffer[i] = rand(  );
            checksum[0] = checksum[0] + buffer[i];
        }
        ok( HFILE_ERROR != _hwrite( filehandle, buffer, sizeof( buffer ) ), "_hwrite complains\n" );
        bytes_written = bytes_written + sizeof( buffer );
    }

    ok( HFILE_ERROR != _hwrite( filehandle, checksum, 1 ), "_hwrite complains\n" );
    bytes_written++;

    ok( HFILE_ERROR != _lclose( filehandle ), "_lclose complains\n" );

    memory_object = LocalAlloc( LPTR, bytes_written );

    ok( 0 != memory_object, "LocalAlloc fails. (Could be out of memory.)\n" );

    contents = LocalLock( memory_object );

    filehandle = _lopen( filename, OF_READ );

    contents = LocalLock( memory_object );

    ok( NULL != contents, "LocalLock whines\n" );

    ok( bytes_written == _hread( filehandle, contents, bytes_written), "read length differ from write length\n" );

    checksum[0] = '\0';
    i = 0;
    do
    {
        checksum[0] = checksum[0] + contents[i];
        i++;
    }
    while (i < bytes_written - 1);

    ok( checksum[0] == contents[i], "stored checksum differ from computed checksum\n" );

    ok( HFILE_ERROR != _lclose( filehandle ), "_lclose complains\n" );

    ret = DeleteFileA( filename );
    ok( ret != 0, "DeleteFile failed (%d)\n", GetLastError(  ) );
}


static void test__lclose( void )
{
    HFILE filehandle;
    BOOL ret;

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%d)\n",filename,GetLastError());
        return;
    }

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains\n" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    ret = DeleteFileA( filename );
    ok( ret != 0, "DeleteFile failed (%d)\n", GetLastError(  ) );
}


static void test__lcreat( void )
{
    HFILE filehandle;
    char buffer[10000];
    WIN32_FIND_DATAA search_results;
    char slashname[] = "testfi/";
    int err;
    HANDLE find;
    BOOL ret;

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%d)\n",filename,GetLastError());
        return;
    }

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains\n" );

    ok( 0 == _llseek( filehandle, 0, FILE_BEGIN ), "_llseek complains\n" );

    ok( _hread( filehandle, buffer, strlen( sillytext ) ) ==  lstrlenA( sillytext ), "erratic _hread return value\n" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    ok( INVALID_HANDLE_VALUE != FindFirstFileA( filename, &search_results ), "should be able to find file\n" );

    ret = DeleteFileA(filename);
    ok( ret != 0, "DeleteFile failed (%d)\n", GetLastError());

    filehandle = _lcreat( filename, 1 ); /* readonly */
    ok( HFILE_ERROR != filehandle, "couldn't create file \"%s\" (err=%d)\n", filename, GetLastError(  ) );

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite shouldn't be able to write never the less\n" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    ok( INVALID_HANDLE_VALUE != FindFirstFileA( filename, &search_results ), "should be able to find file\n" );

    ok( 0 == DeleteFileA( filename ), "shouldn't be able to delete a readonly file\n" );

    ok( SetFileAttributesA(filename, FILE_ATTRIBUTE_NORMAL ) != 0, "couldn't change attributes on file\n" );

    ok( DeleteFileA( filename ) != 0, "now it should be possible to delete the file!\n" );

    filehandle = _lcreat( filename, 2 );
    ok( HFILE_ERROR != filehandle, "couldn't create file \"%s\" (err=%d)\n", filename, GetLastError(  ) );

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains\n" );

    ok( 0 == _llseek( filehandle, 0, FILE_BEGIN ), "_llseek complains\n" );

    ok( _hread( filehandle, buffer, strlen( sillytext ) ) ==  lstrlenA( sillytext ), "erratic _hread return value\n" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    ok( INVALID_HANDLE_VALUE != FindFirstFileA( filename, &search_results ), "should STILL be able to find file\n" );

    ret = DeleteFileA( filename );
    ok( ret, "DeleteFile failed (%d)\n", GetLastError(  ) );

    filehandle = _lcreat( filename, 4 ); /* SYSTEM file */
    ok( HFILE_ERROR != filehandle, "couldn't create file \"%s\" (err=%d)\n", filename, GetLastError(  ) );

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains\n" );

    ok( 0 == _llseek( filehandle, 0, FILE_BEGIN ), "_llseek complains\n" );

    ok( _hread( filehandle, buffer, strlen( sillytext ) ) ==  lstrlenA( sillytext ), "erratic _hread return value\n" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    ok( INVALID_HANDLE_VALUE != FindFirstFileA( filename, &search_results ), "should STILL be able to find file\n" );

    ret = DeleteFileA( filename );
    ok( ret, "DeleteFile failed (%d)\n", GetLastError(  ) );

    filehandle=_lcreat (slashname, 0); /* illegal name */
    if (HFILE_ERROR==filehandle) {
      err=GetLastError ();
      ok (err==ERROR_INVALID_NAME || err==ERROR_PATH_NOT_FOUND,
          "creating file \"%s\" failed with error %d\n", slashname, err);
    } else { /* only NT succeeds */
      _lclose(filehandle);
      find=FindFirstFileA (slashname, &search_results);
      if (INVALID_HANDLE_VALUE!=find)
      {
        ret = FindClose (find);
        ok (0 != ret, "FindClose complains (%d)\n", GetLastError ());
        slashname[strlen(slashname)-1]=0;
        ok (!strcmp (slashname, search_results.cFileName),
            "found unexpected name \"%s\"\n", search_results.cFileName);
        ok (FILE_ATTRIBUTE_ARCHIVE==search_results.dwFileAttributes,
            "attributes of file \"%s\" are 0x%04x\n", search_results.cFileName,
            search_results.dwFileAttributes);
      }
    ret = DeleteFileA( slashname );
    ok( ret, "DeleteFile failed (%d)\n", GetLastError(  ) );
    }

    filehandle=_lcreat (filename, 8); /* illegal attribute */
    if (HFILE_ERROR==filehandle)
      ok (0, "couldn't create volume label \"%s\"\n", filename);
    else {
      _lclose(filehandle);
      find=FindFirstFileA (filename, &search_results);
      if (INVALID_HANDLE_VALUE==find)
        ok (0, "file \"%s\" not found\n", filename);
      else {
        ret = FindClose(find);
        ok ( 0 != ret, "FindClose complains (%d)\n", GetLastError ());
        ok (!strcmp (filename, search_results.cFileName),
            "found unexpected name \"%s\"\n", search_results.cFileName);
        search_results.dwFileAttributes &= ~FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
        ok (FILE_ATTRIBUTE_ARCHIVE==search_results.dwFileAttributes,
            "attributes of file \"%s\" are 0x%04x\n", search_results.cFileName,
            search_results.dwFileAttributes);
      }
    ret = DeleteFileA( filename );
    ok( ret, "DeleteFile failed (%d)\n", GetLastError(  ) );
    }
}


static void test__llseek( void )
{
    INT i;
    HFILE filehandle;
    char buffer[1];
    long bytes_read;
    BOOL ret;

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%d)\n",filename,GetLastError());
        return;
    }

    for (i = 0; i < 400; i++)
    {
        ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains\n" );
    }
    ok( HFILE_ERROR != _llseek( filehandle, 400 * strlen( sillytext ), FILE_CURRENT ), "should be able to seek\n" );
    ok( HFILE_ERROR != _llseek( filehandle, 27 + 35 * strlen( sillytext ), FILE_BEGIN ), "should be able to seek\n" );

    bytes_read = _hread( filehandle, buffer, 1);
    ok( 1 == bytes_read, "file read size error\n" );
    ok( buffer[0] == sillytext[27], "_llseek error, it got lost seeking\n" );
    ok( HFILE_ERROR != _llseek( filehandle, -400 * strlen( sillytext ), FILE_END ), "should be able to seek\n" );

    bytes_read = _hread( filehandle, buffer, 1);
    ok( 1 == bytes_read, "file read size error\n" );
    ok( buffer[0] == sillytext[0], "_llseek error, it got lost seeking\n" );
    ok( HFILE_ERROR != _llseek( filehandle, 1000000, FILE_END ), "should be able to seek past file; poor, poor Windows programmers\n" );
    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    ret = DeleteFileA( filename );
    ok( ret, "DeleteFile failed (%d)\n", GetLastError(  ) );
}


static void test__llopen( void )
{
    HFILE filehandle;
    UINT bytes_read;
    char buffer[10000];
    BOOL ret;

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%d)\n",filename,GetLastError());
        return;
    }

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains\n" );
    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    filehandle = _lopen( filename, OF_READ );
    ok( HFILE_ERROR == _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite shouldn't be able to write!\n" );
    bytes_read = _hread( filehandle, buffer, strlen( sillytext ) );
    ok( strlen( sillytext )  == bytes_read, "file read size error\n" );
    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    filehandle = _lopen( filename, OF_READWRITE );
    bytes_read = _hread( filehandle, buffer, 2 * strlen( sillytext ) );
    ok( strlen( sillytext )  == bytes_read, "file read size error\n" );
    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite should write just fine\n" );
    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    filehandle = _lopen( filename, OF_WRITE );
    ok( HFILE_ERROR == _hread( filehandle, buffer, 1 ), "you should only be able to write this file\n" );
    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite should write just fine\n" );
    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    ret = DeleteFileA( filename );
    ok( ret, "DeleteFile failed (%d)\n", GetLastError(  ) );
    /* TODO - add tests for the SHARE modes  -  use two processes to pull this one off */
}


static void test__lread( void )
{
    HFILE filehandle;
    char buffer[10000];
    long bytes_read;
    UINT bytes_wanted;
    UINT i;
    BOOL ret;

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%d)\n",filename,GetLastError());
        return;
    }

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains\n" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    filehandle = _lopen( filename, OF_READ );

    ok( HFILE_ERROR != filehandle, "couldn't open file \"%s\" again (err=%d)\n", filename, GetLastError());

    bytes_read = _lread( filehandle, buffer, 2 * strlen( sillytext ) );

    ok( lstrlenA( sillytext ) == bytes_read, "file read size error\n" );

    for (bytes_wanted = 0; bytes_wanted < strlen( sillytext ); bytes_wanted++)
    {
        ok( 0 == _llseek( filehandle, 0, FILE_BEGIN ), "_llseek complains\n" );
        ok( _lread( filehandle, buffer, bytes_wanted ) == bytes_wanted, "erratic _hread return value\n" );
        for (i = 0; i < bytes_wanted; i++)
        {
            ok( buffer[i] == sillytext[i], "that's not what's written\n" );
        }
    }

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    ret = DeleteFileA( filename );
    ok( ret, "DeleteFile failed (%d)\n", GetLastError(  ) );
}


static void test__lwrite( void )
{
    HFILE filehandle;
    char buffer[10000];
    long bytes_read;
    long bytes_written;
    long blocks;
    long i;
    char *contents;
    HLOCAL memory_object;
    char checksum[1];
    BOOL ret;

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%d)\n",filename,GetLastError());
        return;
    }

    ok( HFILE_ERROR != _lwrite( filehandle, "", 0 ), "_hwrite complains\n" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    filehandle = _lopen( filename, OF_READ );

    bytes_read = _hread( filehandle, buffer, 1);

    ok( 0 == bytes_read, "file read size error\n" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    filehandle = _lopen( filename, OF_READWRITE );

    bytes_written = 0;
    checksum[0] = '\0';
    srand( (unsigned)time( NULL ) );
    for (blocks = 0; blocks < 100; blocks++)
    {
        for (i = 0; i < (long)sizeof( buffer ); i++)
        {
            buffer[i] = rand(  );
            checksum[0] = checksum[0] + buffer[i];
        }
        ok( HFILE_ERROR != _lwrite( filehandle, buffer, sizeof( buffer ) ), "_hwrite complains\n" );
        bytes_written = bytes_written + sizeof( buffer );
    }

    ok( HFILE_ERROR != _lwrite( filehandle, checksum, 1 ), "_hwrite complains\n" );
    bytes_written++;

    ok( HFILE_ERROR != _lclose( filehandle ), "_lclose complains\n" );

    memory_object = LocalAlloc( LPTR, bytes_written );

    ok( 0 != memory_object, "LocalAlloc fails, could be out of memory\n" );

    contents = LocalLock( memory_object );

    filehandle = _lopen( filename, OF_READ );

    contents = LocalLock( memory_object );

    ok( NULL != contents, "LocalLock whines\n" );

    ok( bytes_written == _hread( filehandle, contents, bytes_written), "read length differ from write length\n" );

    checksum[0] = '\0';
    i = 0;
    do
    {
        checksum[0] += contents[i];
        i++;
    }
    while (i < bytes_written - 1);

    ok( checksum[0] == contents[i], "stored checksum differ from computed checksum\n" );

    ok( HFILE_ERROR != _lclose( filehandle ), "_lclose complains\n" );

    ret = DeleteFileA( filename );
    ok( ret, "DeleteFile failed (%d)\n", GetLastError(  ) );
}

static void test_CopyFileA(void)
{
    char temp_path[MAX_PATH];
    char source[MAX_PATH], dest[MAX_PATH];
    static const char prefix[] = "pfx";
    HANDLE hfile;
    FILETIME ft1, ft2;
    char buf[10];
    DWORD ret;
    BOOL retok;

    ret = GetTempPathA(MAX_PATH, temp_path);
    ok(ret != 0, "GetTempPathA error %d\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameA(temp_path, prefix, 0, source);
    ok(ret != 0, "GetTempFileNameA error %d\n", GetLastError());

    /* make the source have not zero size */
    hfile = CreateFileA(source, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open source file\n");
    retok = WriteFile(hfile, prefix, sizeof(prefix), &ret, NULL );
    ok( retok && ret == sizeof(prefix),
       "WriteFile error %d\n", GetLastError());
    ok(GetFileSize(hfile, NULL) == sizeof(prefix), "source file has wrong size\n");
    /* get the file time and change it to prove the difference */
    ret = GetFileTime(hfile, NULL, NULL, &ft1);
    ok( ret, "GetFileTime error %d\n", GetLastError());
    ft1.dwLowDateTime -= 600000000; /* 60 second */
    ret = SetFileTime(hfile, NULL, NULL, &ft1);
    ok( ret, "SetFileTime error %d\n", GetLastError());
    GetFileTime(hfile, NULL, NULL, &ft1);  /* get the actual time back */
    CloseHandle(hfile);

    ret = GetTempFileNameA(temp_path, prefix, 0, dest);
    ok(ret != 0, "GetTempFileNameA error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = CopyFileA(source, dest, TRUE);
    ok(!ret && GetLastError() == ERROR_FILE_EXISTS,
       "CopyFileA: unexpected error %d\n", GetLastError());

    ret = CopyFileA(source, dest, FALSE);
    ok(ret, "CopyFileA: error %d\n", GetLastError());

    /* make sure that destination has correct size */
    hfile = CreateFileA(dest, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open destination file\n");
    ret = GetFileSize(hfile, NULL);
    ok(ret == sizeof(prefix), "destination file has wrong size %d\n", ret);

    /* make sure that destination has the same filetime */
    ret = GetFileTime(hfile, NULL, NULL, &ft2);
    ok( ret, "GetFileTime error %d\n", GetLastError());
    ok(CompareFileTime(&ft1, &ft2) == 0, "destination file has wrong filetime\n");

    SetLastError(0xdeadbeef);
    ret = CopyFileA(source, dest, FALSE);
    ok(!ret && GetLastError() == ERROR_SHARING_VIOLATION,
       "CopyFileA: ret = %d, unexpected error %d\n", ret, GetLastError());

    /* make sure that destination still has correct size */
    ret = GetFileSize(hfile, NULL);
    ok(ret == sizeof(prefix), "destination file has wrong size %d\n", ret);
    retok = ReadFile(hfile, buf, sizeof(buf), &ret, NULL);
    ok( retok && ret == sizeof(prefix),
       "ReadFile: error %d\n", GetLastError());
    ok(!memcmp(prefix, buf, sizeof(prefix)), "buffer contents mismatch\n");
    CloseHandle(hfile);

    ret = DeleteFileA(source);
    ok(ret, "DeleteFileA: error %d\n", GetLastError());
    ret = DeleteFileA(dest);
    ok(ret, "DeleteFileA: error %d\n", GetLastError());
}

static void test_CopyFileW(void)
{
    WCHAR temp_path[MAX_PATH];
    WCHAR source[MAX_PATH], dest[MAX_PATH];
    static const WCHAR prefix[] = {'p','f','x',0};
    DWORD ret;

    ret = GetTempPathW(MAX_PATH, temp_path);
    if (ret==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
        return;
    ok(ret != 0, "GetTempPathW error %d\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameW(temp_path, prefix, 0, source);
    ok(ret != 0, "GetTempFileNameW error %d\n", GetLastError());

    ret = GetTempFileNameW(temp_path, prefix, 0, dest);
    ok(ret != 0, "GetTempFileNameW error %d\n", GetLastError());

    ret = CopyFileW(source, dest, TRUE);
    ok(!ret && GetLastError() == ERROR_FILE_EXISTS,
       "CopyFileW: unexpected error %d\n", GetLastError());

    ret = CopyFileW(source, dest, FALSE);
    ok(ret, "CopyFileW: error %d\n", GetLastError());

    ret = DeleteFileW(source);
    ok(ret, "DeleteFileW: error %d\n", GetLastError());
    ret = DeleteFileW(dest);
    ok(ret, "DeleteFileW: error %d\n", GetLastError());
}

static void test_CreateFileA(void)
{
    HANDLE hFile;
    char temp_path[MAX_PATH];
    char filename[MAX_PATH];
    static const char prefix[] = "pfx";
    DWORD ret;

    ret = GetTempPathA(MAX_PATH, temp_path);
    ok(ret != 0, "GetTempPathA error %d\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameA(temp_path, prefix, 0, filename);
    ok(ret != 0, "GetTempFileNameA error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    hFile = CreateFileA(filename, GENERIC_READ, 0, NULL,
                        CREATE_NEW, FILE_FLAG_RANDOM_ACCESS, 0);
    ok(hFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_EXISTS,
        "CREATE_NEW should fail if file exists and last error value should be ERROR_FILE_EXISTS\n");

    SetLastError(0xdeadbeef);
    hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                        CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, 0);
    ok(hFile != INVALID_HANDLE_VALUE && GetLastError() == ERROR_ALREADY_EXISTS,
       "hFile %p, last error %u\n", hFile, GetLastError());

    CloseHandle(hFile);

    SetLastError(0xdeadbeef);
    hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS, 0);
    ok(hFile != INVALID_HANDLE_VALUE && GetLastError() == ERROR_ALREADY_EXISTS,
       "hFile %p, last error %u\n", hFile, GetLastError());

    CloseHandle(hFile);

    ret = DeleteFileA(filename);
    ok(ret, "DeleteFileA: error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS, 0);
    ok(hFile != INVALID_HANDLE_VALUE && GetLastError() == 0,
       "hFile %p, last error %u\n", hFile, GetLastError());

    CloseHandle(hFile);

    ret = DeleteFileA(filename);
    ok(ret, "DeleteFileA: error %d\n", GetLastError());
}

static void test_CreateFileW(void)
{
    HANDLE hFile;
    WCHAR temp_path[MAX_PATH];
    WCHAR filename[MAX_PATH];
    static const WCHAR emptyW[]={'\0'};
    static const WCHAR prefix[] = {'p','f','x',0};
    static const WCHAR bogus[] = { '\\', '\\', '.', '\\', 'B', 'O', 'G', 'U', 'S', 0 };
    DWORD ret;

    ret = GetTempPathW(MAX_PATH, temp_path);
    if (ret==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
        return;
    ok(ret != 0, "GetTempPathW error %d\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameW(temp_path, prefix, 0, filename);
    ok(ret != 0, "GetTempFileNameW error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    hFile = CreateFileW(filename, GENERIC_READ, 0, NULL,
                        CREATE_NEW, FILE_FLAG_RANDOM_ACCESS, 0);
    ok(hFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_EXISTS,
        "CREATE_NEW should fail if file exists and last error value should be ERROR_FILE_EXISTS\n");

    SetLastError(0xdeadbeef);
    hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                        CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, 0);
    ok(hFile != INVALID_HANDLE_VALUE && GetLastError() == ERROR_ALREADY_EXISTS,
       "hFile %p, last error %u\n", hFile, GetLastError());

    CloseHandle(hFile);

    SetLastError(0xdeadbeef);
    hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS, 0);
    ok(hFile != INVALID_HANDLE_VALUE && GetLastError() == ERROR_ALREADY_EXISTS,
       "hFile %p, last error %u\n", hFile, GetLastError());

    CloseHandle(hFile);

    ret = DeleteFileW(filename);
    ok(ret, "DeleteFileW: error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS, 0);
    ok(hFile != INVALID_HANDLE_VALUE && GetLastError() == 0,
       "hFile %p, last error %u\n", hFile, GetLastError());

    CloseHandle(hFile);

    ret = DeleteFileW(filename);
    ok(ret, "DeleteFileW: error %d\n", GetLastError());

    if (0)
    {
        /* this crashes on NT4.0 */
        hFile = CreateFileW(NULL, GENERIC_READ, 0, NULL,
                            CREATE_NEW, FILE_FLAG_RANDOM_ACCESS, 0);
        ok(hFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_PATH_NOT_FOUND,
           "CreateFileW(NULL) returned ret=%p error=%u\n",hFile,GetLastError());
    }

    hFile = CreateFileW(emptyW, GENERIC_READ, 0, NULL,
                        CREATE_NEW, FILE_FLAG_RANDOM_ACCESS, 0);
    ok(hFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_PATH_NOT_FOUND,
       "CreateFileW(\"\") returned ret=%p error=%d\n",hFile,GetLastError());

    /* test the result of opening a nonexistent driver name */
    hFile = CreateFileW(bogus, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND,
       "CreateFileW on invalid VxD name returned ret=%p error=%d\n",hFile,GetLastError());

    ret = CreateDirectoryW(filename, NULL);
    ok(ret == TRUE, "couldn't create temporary directory\n");
    hFile = CreateFileW(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, NULL);
    ok(hFile != INVALID_HANDLE_VALUE,
       "expected CreateFile to succeed on existing directory, error: %d\n", GetLastError());
    CloseHandle(hFile);
    ret = RemoveDirectoryW(filename);
    ok(ret, "DeleteFileW: error %d\n", GetLastError());
}

static void test_GetTempFileNameA(void)
{
    UINT result;
    char out[MAX_PATH];
    char expected[MAX_PATH + 10];
    char windowsdir[MAX_PATH + 10];
    char windowsdrive[3];

    result = GetWindowsDirectory(windowsdir, sizeof(windowsdir));
    ok(result < sizeof(windowsdir), "windowsdir is abnormally long!\n");
    ok(result != 0, "GetWindowsDirectory: error %d\n", GetLastError());

    /* If the Windows directory is the root directory, it ends in backslash, not else. */
    if (strlen(windowsdir) != 3) /* As in  "C:\"  or  "F:\"  */
    {
        strcat(windowsdir, "\\");
    }

    windowsdrive[0] = windowsdir[0];
    windowsdrive[1] = windowsdir[1];
    windowsdrive[2] = '\0';

    result = GetTempFileNameA(windowsdrive, "abc", 1, out);
    ok(result != 0, "GetTempFileNameA: error %d\n", GetLastError());
    ok(((out[0] == windowsdrive[0]) && (out[1] == ':')) && (out[2] == '\\'),
       "GetTempFileNameA: first three characters should be %c:\\, string was actually %s\n",
       windowsdrive[0], out);

    result = GetTempFileNameA(windowsdir, "abc", 2, out);
    ok(result != 0, "GetTempFileNameA: error %d\n", GetLastError());
    expected[0] = '\0';
    strcat(expected, windowsdir);
    strcat(expected, "abc2.tmp");
    ok(lstrcmpiA(out, expected) == 0, "GetTempFileNameA: Unexpected output \"%s\" vs \"%s\"\n",
       out, expected);
}

static void test_DeleteFileA( void )
{
    BOOL ret;

    ret = DeleteFileA(NULL);
    ok(!ret && (GetLastError() == ERROR_INVALID_PARAMETER ||
                GetLastError() == ERROR_PATH_NOT_FOUND),
       "DeleteFileA(NULL) returned ret=%d error=%d\n",ret,GetLastError());

    ret = DeleteFileA("");
    ok(!ret && (GetLastError() == ERROR_PATH_NOT_FOUND ||
                GetLastError() == ERROR_BAD_PATHNAME),
       "DeleteFileA(\"\") returned ret=%d error=%d\n",ret,GetLastError());

    ret = DeleteFileA("nul");
    ok(!ret && (GetLastError() == ERROR_FILE_NOT_FOUND ||
                GetLastError() == ERROR_INVALID_PARAMETER ||
                GetLastError() == ERROR_ACCESS_DENIED ||
                GetLastError() == ERROR_INVALID_FUNCTION),
       "DeleteFileA(\"nul\") returned ret=%d error=%d\n",ret,GetLastError());
}

static void test_DeleteFileW( void )
{
    BOOL ret;
    WCHAR pathW[MAX_PATH];
    WCHAR pathsubW[MAX_PATH];
    static const WCHAR dirW[] = {'d','e','l','e','t','e','f','i','l','e',0};
    static const WCHAR subdirW[] = {'\\','s','u','b',0};
    static const WCHAR emptyW[]={'\0'};

    ret = DeleteFileW(NULL);
    if (ret==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
        return;
    ok(!ret && GetLastError() == ERROR_PATH_NOT_FOUND,
       "DeleteFileW(NULL) returned ret=%d error=%d\n",ret,GetLastError());

    ret = DeleteFileW(emptyW);
    ok(!ret && GetLastError() == ERROR_PATH_NOT_FOUND,
       "DeleteFileW(\"\") returned ret=%d error=%d\n",ret,GetLastError());

    /* test DeleteFile on empty directory */
    ret = GetTempPathW(MAX_PATH, pathW);
    if (ret + sizeof(dirW)/sizeof(WCHAR)-1 + sizeof(subdirW)/sizeof(WCHAR)-1 >= MAX_PATH)
    {
        ok(0, "MAX_PATH exceeded in constructing paths\n");
        return;
    }
    lstrcatW(pathW, dirW);
    lstrcpyW(pathsubW, pathW);
    lstrcatW(pathsubW, subdirW);
    ret = CreateDirectoryW(pathW, NULL);
    ok(ret == TRUE, "couldn't create directory deletefile\n");
    ret = DeleteFileW(pathW);
    ok(ret == FALSE, "DeleteFile should fail for empty directories\n");
    ret = RemoveDirectoryW(pathW);
    ok(ret == TRUE, "expected to remove directory deletefile\n");

    /* test DeleteFile on non-empty directory */
    ret = CreateDirectoryW(pathW, NULL);
    ok(ret == TRUE, "couldn't create directory deletefile\n");
    ret = CreateDirectoryW(pathsubW, NULL);
    ok(ret == TRUE, "couldn't create directory deletefile\\sub\n");
    ret = DeleteFileW(pathW);
    ok(ret == FALSE, "DeleteFile should fail for non-empty directories\n");
    ret = RemoveDirectoryW(pathsubW);
    ok(ret == TRUE, "expected to remove directory deletefile\\sub\n");
    ret = RemoveDirectoryW(pathW);
    ok(ret == TRUE, "expected to remove directory deletefile\n");
}

#define IsDotDir(x)     ((x[0] == '.') && ((x[1] == 0) || ((x[1] == '.') && (x[2] == 0))))

static void test_MoveFileA(void)
{
    char tempdir[MAX_PATH];
    char source[MAX_PATH], dest[MAX_PATH];
    static const char prefix[] = "pfx";
    DWORD ret;

    ret = GetTempPathA(MAX_PATH, tempdir);
    ok(ret != 0, "GetTempPathA error %d\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameA(tempdir, prefix, 0, source);
    ok(ret != 0, "GetTempFileNameA error %d\n", GetLastError());

    ret = GetTempFileNameA(tempdir, prefix, 0, dest);
    ok(ret != 0, "GetTempFileNameA error %d\n", GetLastError());

    ret = MoveFileA(source, dest);
    ok(!ret && GetLastError() == ERROR_ALREADY_EXISTS,
       "MoveFileA: unexpected error %d\n", GetLastError());

    ret = DeleteFileA(dest);
    ok(ret, "DeleteFileA: error %d\n", GetLastError());

    ret = MoveFileA(source, dest);
    ok(ret, "MoveFileA: failed, error %d\n", GetLastError());

    lstrcatA(tempdir, "Remove Me");
    ret = CreateDirectoryA(tempdir, NULL);
    ok(ret == TRUE, "CreateDirectoryA failed\n");

    lstrcpyA(source, dest);
    lstrcpyA(dest, tempdir);
    lstrcatA(dest, "\\wild?.*");
    /* FIXME: if we create a file with wildcards we can't delete it now that DeleteFile works correctly */
    ret = MoveFileA(source, dest);
    ok(!ret, "MoveFileA: shouldn't move to wildcard file\n");
    ok(GetLastError() == ERROR_INVALID_NAME || /* NT */
       GetLastError() == ERROR_FILE_NOT_FOUND, /* Win9x */
       "MoveFileA: with wildcards, unexpected error %d\n", GetLastError());
    if (ret || (GetLastError() != ERROR_INVALID_NAME))
    {
        WIN32_FIND_DATAA fd;
        char temppath[MAX_PATH];
        HANDLE hFind;

        lstrcpyA(temppath, tempdir);
        lstrcatA(temppath, "\\*.*");
        hFind = FindFirstFileA(temppath, &fd);
        if (INVALID_HANDLE_VALUE != hFind)
        {
          LPSTR lpName;
          do
          {
            lpName = fd.cAlternateFileName;
            if (!lpName[0])
              lpName = fd.cFileName;
            ok(IsDotDir(lpName), "MoveFileA: wildcards file created!\n");
          }
          while (FindNextFileA(hFind, &fd));
          FindClose(hFind);
        }
    }
    ret = DeleteFileA(source);
    ok(ret, "DeleteFileA: error %d\n", GetLastError());
    ret = DeleteFileA(dest);
    ok(!ret, "DeleteFileA: error %d\n", GetLastError());
    ret = RemoveDirectoryA(tempdir);
    ok(ret, "DeleteDirectoryA: error %d\n", GetLastError());
}

static void test_MoveFileW(void)
{
    WCHAR temp_path[MAX_PATH];
    WCHAR source[MAX_PATH], dest[MAX_PATH];
    static const WCHAR prefix[] = {'p','f','x',0};
    DWORD ret;

    ret = GetTempPathW(MAX_PATH, temp_path);
    if (ret==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
        return;
    ok(ret != 0, "GetTempPathW error %d\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameW(temp_path, prefix, 0, source);
    ok(ret != 0, "GetTempFileNameW error %d\n", GetLastError());

    ret = GetTempFileNameW(temp_path, prefix, 0, dest);
    ok(ret != 0, "GetTempFileNameW error %d\n", GetLastError());

    ret = MoveFileW(source, dest);
    ok(!ret && GetLastError() == ERROR_ALREADY_EXISTS,
       "CopyFileW: unexpected error %d\n", GetLastError());

    ret = DeleteFileW(source);
    ok(ret, "DeleteFileW: error %d\n", GetLastError());
    ret = DeleteFileW(dest);
    ok(ret, "DeleteFileW: error %d\n", GetLastError());
}

#define PATTERN_OFFSET 0x10

static void test_offset_in_overlapped_structure(void)
{
    HANDLE hFile;
    OVERLAPPED ov;
    DWORD done, offset;
    BOOL rc;
    BYTE buf[256], pattern[] = "TeSt";
    UINT i;
    char temp_path[MAX_PATH], temp_fname[MAX_PATH];
    BOOL ret;

    ret =GetTempPathA(MAX_PATH, temp_path);
    ok( ret, "GetTempPathA error %d\n", GetLastError());
    ret =GetTempFileNameA(temp_path, "pfx", 0, temp_fname);
    ok( ret, "GetTempFileNameA error %d\n", GetLastError());

    /*** Write File *****************************************************/

    hFile = CreateFileA(temp_fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA error %d\n", GetLastError());

    for(i = 0; i < sizeof(buf); i++) buf[i] = i;
    ret = WriteFile(hFile, buf, sizeof(buf), &done, NULL);
    ok( ret, "WriteFile error %d\n", GetLastError());
    ok(done == sizeof(buf), "expected number of bytes written %u\n", done);

    memset(&ov, 0, sizeof(ov));
    S(U(ov)).Offset = PATTERN_OFFSET;
    S(U(ov)).OffsetHigh = 0;
    rc=WriteFile(hFile, pattern, sizeof(pattern), &done, &ov);
    /* Win 9x does not support the overlapped I/O on files */
    if (rc || GetLastError()!=ERROR_INVALID_PARAMETER) {
        ok(rc, "WriteFile error %d\n", GetLastError());
        ok(done == sizeof(pattern), "expected number of bytes written %u\n", done);
        offset = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
        ok(offset == PATTERN_OFFSET + sizeof(pattern), "wrong file offset %d\n", offset);

        S(U(ov)).Offset = sizeof(buf) * 2;
        S(U(ov)).OffsetHigh = 0;
        ret = WriteFile(hFile, pattern, sizeof(pattern), &done, &ov);
        ok( ret, "WriteFile error %d\n", GetLastError());
        ok(done == sizeof(pattern), "expected number of bytes written %u\n", done);
        offset = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
        ok(offset == sizeof(buf) * 2 + sizeof(pattern), "wrong file offset %d\n", offset);
    }

    CloseHandle(hFile);

    /*** Read File *****************************************************/

    hFile = CreateFileA(temp_fname, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA error %d\n", GetLastError());

    memset(buf, 0, sizeof(buf));
    memset(&ov, 0, sizeof(ov));
    S(U(ov)).Offset = PATTERN_OFFSET;
    S(U(ov)).OffsetHigh = 0;
    rc=ReadFile(hFile, buf, sizeof(pattern), &done, &ov);
    /* Win 9x does not support the overlapped I/O on files */
    if (rc || GetLastError()!=ERROR_INVALID_PARAMETER) {
        ok(rc, "ReadFile error %d\n", GetLastError());
        ok(done == sizeof(pattern), "expected number of bytes read %u\n", done);
        offset = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
        ok(offset == PATTERN_OFFSET + sizeof(pattern), "wrong file offset %d\n", offset);
        ok(!memcmp(buf, pattern, sizeof(pattern)), "pattern match failed\n");
    }

    CloseHandle(hFile);

    ret = DeleteFileA(temp_fname);
    ok( ret, "DeleteFileA error %d\n", GetLastError());
}

static void test_LockFile(void)
{
    HANDLE handle;
    DWORD written;
    OVERLAPPED overlapped;
    int limited_LockFile;
    int limited_UnLockFile;
    BOOL ret;

    handle = CreateFileA( filename, GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                          CREATE_ALWAYS, 0, 0 );
    if (handle == INVALID_HANDLE_VALUE)
    {
        ok(0,"couldn't create file \"%s\" (err=%d)\n",filename,GetLastError());
        return;
    }
    ok( WriteFile( handle, sillytext, strlen(sillytext), &written, NULL ), "write failed\n" );

    ok( LockFile( handle, 0, 0, 0, 0 ), "LockFile failed\n" );
    ok( UnlockFile( handle, 0, 0, 0, 0 ), "UnlockFile failed\n" );

    limited_UnLockFile = 0;
    if (UnlockFile( handle, 0, 0, 0, 0 ))
    {
        limited_UnLockFile = 1;
    }

    ok( LockFile( handle, 10, 0, 20, 0 ), "LockFile 10,20 failed\n" );
    /* overlapping locks must fail */
    ok( !LockFile( handle, 12, 0, 10, 0 ), "LockFile 12,10 succeeded\n" );
    ok( !LockFile( handle, 5, 0, 6, 0 ), "LockFile 5,6 succeeded\n" );
    /* non-overlapping locks must succeed */
    ok( LockFile( handle, 5, 0, 5, 0 ), "LockFile 5,5 failed\n" );

    ok( !UnlockFile( handle, 10, 0, 10, 0 ), "UnlockFile 10,10 succeeded\n" );
    ok( UnlockFile( handle, 10, 0, 20, 0 ), "UnlockFile 10,20 failed\n" );
    ok( !UnlockFile( handle, 10, 0, 20, 0 ), "UnlockFile 10,20 again succeeded\n" );
    ok( UnlockFile( handle, 5, 0, 5, 0 ), "UnlockFile 5,5 failed\n" );

    S(U(overlapped)).Offset = 100;
    S(U(overlapped)).OffsetHigh = 0;
    overlapped.hEvent = 0;

    /* Test for broken LockFileEx a la Windows 95 OSR2. */
    if (LockFileEx( handle, 0, 0, 100, 0, &overlapped ))
    {
        /* LockFileEx is probably OK, test it more. */
        ok( LockFileEx( handle, 0, 0, 100, 0, &overlapped ),
            "LockFileEx 100,100 failed\n" );
    }

    /* overlapping shared locks are OK */
    S(U(overlapped)).Offset = 150;
    limited_UnLockFile || ok( LockFileEx( handle, 0, 0, 100, 0, &overlapped ), "LockFileEx 150,100 failed\n" );

    /* but exclusive is not */
    ok( !LockFileEx( handle, LOCKFILE_EXCLUSIVE_LOCK|LOCKFILE_FAIL_IMMEDIATELY,
                     0, 50, 0, &overlapped ),
        "LockFileEx exclusive 150,50 succeeded\n" );
    if (!UnlockFileEx( handle, 0, 100, 0, &overlapped ))
    { /* UnLockFile is capable. */
        S(U(overlapped)).Offset = 100;
        ok( !UnlockFileEx( handle, 0, 100, 0, &overlapped ),
            "UnlockFileEx 150,100 again succeeded\n" );
    }

    ret = LockFile( handle, 0, 0x10000000, 0, 0xf0000000 );
    if (ret)
    {
        ok( !LockFile( handle, ~0, ~0, 1, 0 ), "LockFile ~0,1 succeeded\n" );
        ok( !LockFile( handle, 0, 0x20000000, 20, 0 ), "LockFile 0x20000000,20 succeeded\n" );
        ok( UnlockFile( handle, 0, 0x10000000, 0, 0xf0000000 ), "UnlockFile failed\n" );
    }
    else  /* win9x */
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong LockFile error %u\n", GetLastError() );

    /* wrap-around lock should not do anything */
    /* (but still succeeds on NT4 so we don't check result) */
    LockFile( handle, 0, 0x10000000, 0, 0xf0000001 );

    limited_LockFile = 0;
    if (!LockFile( handle, ~0, ~0, 1, 0 ))
    {
        limited_LockFile = 1;
    }

    limited_UnLockFile || ok( UnlockFile( handle, ~0, ~0, 1, 0 ), "Unlockfile ~0,1 failed\n" );

    /* zero-byte lock */
    ok( LockFile( handle, 100, 0, 0, 0 ), "LockFile 100,0 failed\n" );
    limited_LockFile || ok( !LockFile( handle, 98, 0, 4, 0 ), "LockFile 98,4 succeeded\n" );
    ok( LockFile( handle, 90, 0, 10, 0 ), "LockFile 90,10 failed\n" );
    limited_LockFile || ok( !LockFile( handle, 100, 0, 10, 0 ), "LockFile 100,10 failed\n" );

    ok( UnlockFile( handle, 90, 0, 10, 0 ), "UnlockFile 90,10 failed\n" );
    !ok( UnlockFile( handle, 100, 0, 10, 0 ), "UnlockFile 100,10 failed\n" );

    ok( UnlockFile( handle, 100, 0, 0, 0 ), "UnlockFile 100,0 failed\n" );

    CloseHandle( handle );
    DeleteFileA( filename );
}

static inline int is_sharing_compatible( DWORD access1, DWORD sharing1, DWORD access2, DWORD sharing2, BOOL is_win9x )
{
    if (!is_win9x)
    {
        if (!access1) sharing1 = FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE;
        if (!access2) sharing2 = FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE;
    }
    else
    {
        access1 &= ~DELETE;
        if (!access1) access1 = GENERIC_READ;

        access2 &= ~DELETE;
        if (!access2) access2 = GENERIC_READ;
    }

    if ((access1 & GENERIC_READ) && !(sharing2 & FILE_SHARE_READ)) return 0;
    if ((access1 & GENERIC_WRITE) && !(sharing2 & FILE_SHARE_WRITE)) return 0;
    if ((access1 & DELETE) && !(sharing2 & FILE_SHARE_DELETE)) return 0;
    if ((access2 & GENERIC_READ) && !(sharing1 & FILE_SHARE_READ)) return 0;
    if ((access2 & GENERIC_WRITE) && !(sharing1 & FILE_SHARE_WRITE)) return 0;
    if ((access2 & DELETE) && !(sharing1 & FILE_SHARE_DELETE)) return 0;
    return 1;
}

static void test_file_sharing(void)
{
    static const DWORD access_modes[] =
        { 0, GENERIC_READ, GENERIC_WRITE, GENERIC_READ|GENERIC_WRITE,
          DELETE, GENERIC_READ|DELETE, GENERIC_WRITE|DELETE, GENERIC_READ|GENERIC_WRITE|DELETE };
    static const DWORD sharing_modes[] =
        { 0, FILE_SHARE_READ,
          FILE_SHARE_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
          FILE_SHARE_DELETE, FILE_SHARE_READ|FILE_SHARE_DELETE,
          FILE_SHARE_WRITE|FILE_SHARE_DELETE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE };
    int a1, s1, a2, s2;
    int ret;
    HANDLE h, h2;
    BOOL is_win9x = FALSE;

    /* make sure the file exists */
    h = CreateFileA( filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    if (h == INVALID_HANDLE_VALUE)
    {
        ok(0, "couldn't create file \"%s\" (err=%d)\n", filename, GetLastError());
        return;
    }
    is_win9x = GetFileAttributesW(filenameW) == INVALID_FILE_ATTRIBUTES;
    CloseHandle( h );

    for (a1 = 0; a1 < sizeof(access_modes)/sizeof(access_modes[0]); a1++)
    {
        for (s1 = 0; s1 < sizeof(sharing_modes)/sizeof(sharing_modes[0]); s1++)
        {
            /* Win9x doesn't support FILE_SHARE_DELETE */
            if (is_win9x && (sharing_modes[s1] & FILE_SHARE_DELETE))
                continue;

            SetLastError(0xdeadbeef);
            h = CreateFileA( filename, access_modes[a1], sharing_modes[s1],
                             NULL, OPEN_EXISTING, 0, 0 );
            if (h == INVALID_HANDLE_VALUE)
            {
                ok(0,"couldn't create file \"%s\" (err=%d)\n",filename,GetLastError());
                return;
            }
            for (a2 = 0; a2 < sizeof(access_modes)/sizeof(access_modes[0]); a2++)
            {
                for (s2 = 0; s2 < sizeof(sharing_modes)/sizeof(sharing_modes[0]); s2++)
                {
                    /* Win9x doesn't support FILE_SHARE_DELETE */
                    if (is_win9x && (sharing_modes[s2] & FILE_SHARE_DELETE))
                        continue;

                    SetLastError(0xdeadbeef);
                    h2 = CreateFileA( filename, access_modes[a2], sharing_modes[s2],
                                      NULL, OPEN_EXISTING, 0, 0 );

                    if (is_sharing_compatible( access_modes[a1], sharing_modes[s1],
                                               access_modes[a2], sharing_modes[s2], is_win9x ))
                    {
                        ret = GetLastError();

                        ok( h2 != INVALID_HANDLE_VALUE,
                            "open failed for modes %x/%x/%x/%x\n",
                            access_modes[a1], sharing_modes[s1],
                            access_modes[a2], sharing_modes[s2] );
                        ok( ret == 0xdeadbeef /* Win9x */ ||
                            ret == 0, /* XP */
                             "wrong error code %d\n", ret );

                        CloseHandle( h2 );
                    }
                    else
                    {
                        ret = GetLastError();

                        ok( h2 == INVALID_HANDLE_VALUE,
                            "open succeeded for modes %x/%x/%x/%x\n",
                            access_modes[a1], sharing_modes[s1],
                            access_modes[a2], sharing_modes[s2] );
                         ok( ret == ERROR_SHARING_VIOLATION,
                             "wrong error code %d\n", ret );
                    }
                }
            }
            CloseHandle( h );
        }
    }

    SetLastError(0xdeadbeef);
    h = CreateFileA( filename, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, 0 );
    ok( h != INVALID_HANDLE_VALUE, "CreateFileA error %d\n", GetLastError() );

    SetLastError(0xdeadbeef);
    h2 = CreateFileA( filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
    ok( h2 == INVALID_HANDLE_VALUE, "CreateFileA should fail\n");
    ok( GetLastError() == ERROR_SHARING_VIOLATION, "wrong error code %d\n", GetLastError() );

    h2 = CreateFileA( filename, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
    ok( h2 != INVALID_HANDLE_VALUE, "CreateFileA error %d\n", GetLastError() );

    CloseHandle(h);
    CloseHandle(h2);

    DeleteFileA( filename );
}

static char get_windows_drive(void)
{
    char windowsdir[MAX_PATH];
    GetWindowsDirectory(windowsdir, sizeof(windowsdir));
    return windowsdir[0];
}

static void test_FindFirstFileA(void)
{
    HANDLE handle;
    WIN32_FIND_DATAA data;
    int err;
    char buffer[5] = "C:\\";
    char buffer2[100];

    /* try FindFirstFileA on "C:\" */
    buffer[0] = get_windows_drive();
    
    SetLastError( 0xdeadbeaf );
    handle = FindFirstFileA(buffer, &data);
    err = GetLastError();
    ok ( handle == INVALID_HANDLE_VALUE, "FindFirstFile on root directory should fail\n" );
    ok ( err == ERROR_FILE_NOT_FOUND, "Bad Error number %d\n", err );

    /* try FindFirstFileA on "C:\*" */
    strcpy(buffer2, buffer);
    strcat(buffer2, "*");
    handle = FindFirstFileA(buffer2, &data);
    ok ( handle != INVALID_HANDLE_VALUE, "FindFirstFile on %s should succeed\n", buffer2 );
    ok ( strcmp( data.cFileName, "." ) && strcmp( data.cFileName, ".." ),
         "FindFirstFile shouldn't return '%s' in drive root\n", data.cFileName );
    if (FindNextFileA( handle, &data ))
        ok ( strcmp( data.cFileName, "." ) && strcmp( data.cFileName, ".." ),
             "FindNextFile shouldn't return '%s' in drive root\n", data.cFileName );
    ok ( FindClose(handle) == TRUE, "Failed to close handle %s\n", buffer2 );

    /* try FindFirstFileA on windows dir */
    GetWindowsDirectory( buffer2, sizeof(buffer2) );
    strcat(buffer2, "\\*");
    handle = FindFirstFileA(buffer2, &data);
    ok( handle != INVALID_HANDLE_VALUE, "FindFirstFile on %s should succeed\n", buffer2 );
    ok( !strcmp( data.cFileName, "." ), "FindFirstFile should return '.' first\n" );
    ok( FindNextFileA( handle, &data ), "FindNextFile failed\n" );
    ok( !strcmp( data.cFileName, ".." ), "FindNextFile should return '..' as second entry\n" );
    while (FindNextFileA( handle, &data ))
        ok ( strcmp( data.cFileName, "." ) && strcmp( data.cFileName, ".." ),
             "FindNextFile shouldn't return '%s'\n", data.cFileName );
    ok ( FindClose(handle) == TRUE, "Failed to close handle %s\n", buffer2 );

    /* try FindFirstFileA on "C:\foo\" */
    SetLastError( 0xdeadbeaf );
    strcpy(buffer2, buffer);
    strcat(buffer2, "foo\\");
    handle = FindFirstFileA(buffer2, &data);
    err = GetLastError();
    ok ( handle == INVALID_HANDLE_VALUE, "FindFirstFile on %s should Fail\n", buffer2 );
    todo_wine {
        ok ( err == ERROR_PATH_NOT_FOUND, "Bad Error number %d\n", err );
    }

    /* try FindFirstFileA on "C:\foo\bar.txt" */
    SetLastError( 0xdeadbeaf );
    strcpy(buffer2, buffer);
    strcat(buffer2, "foo\\bar.txt");
    handle = FindFirstFileA(buffer2, &data);
    err = GetLastError();
    ok ( handle == INVALID_HANDLE_VALUE, "FindFirstFile on %s should Fail\n", buffer2 );
    ok ( err == ERROR_PATH_NOT_FOUND, "Bad Error number %d\n", err );

    /* try FindFirstFileA on "C:\foo\*.*" */
    SetLastError( 0xdeadbeaf );
    strcpy(buffer2, buffer);
    strcat(buffer2, "foo\\*.*");
    handle = FindFirstFileA(buffer2, &data);
    err = GetLastError();
    ok ( handle == INVALID_HANDLE_VALUE, "FindFirstFile on %s should Fail\n", buffer2 );
    ok ( err == ERROR_PATH_NOT_FOUND, "Bad Error number %d\n", err );

    /* try FindFirstFileA on "foo\bar.txt" */
    SetLastError( 0xdeadbeaf );
    strcpy(buffer2, "foo\\bar.txt");
    handle = FindFirstFileA(buffer2, &data);
    err = GetLastError();
    ok ( handle == INVALID_HANDLE_VALUE, "FindFirstFile on %s should Fail\n", buffer2 );
    ok ( err == ERROR_PATH_NOT_FOUND, "Bad Error number %d\n", err );

    /* try FindFirstFileA on "c:\nul" */
    SetLastError( 0xdeadbeaf );
    strcpy(buffer2, buffer);
    strcat(buffer2, "nul");
    handle = FindFirstFileA(buffer2, &data);
    err = GetLastError();
    ok( handle != INVALID_HANDLE_VALUE, "FindFirstFile on %s failed\n", buffer2 );
    ok( 0 == lstrcmpiA(data.cFileName, "nul"), "wrong name %s\n", data.cFileName );
    ok( FILE_ATTRIBUTE_ARCHIVE == data.dwFileAttributes ||
        FILE_ATTRIBUTE_DEVICE == data.dwFileAttributes /* Win9x */,
        "wrong attributes %x\n", data.dwFileAttributes );
    if (data.dwFileAttributes == FILE_ATTRIBUTE_ARCHIVE)
    {
        ok( 0 == data.nFileSizeHigh, "wrong size %d\n", data.nFileSizeHigh );
        ok( 0 == data.nFileSizeLow, "wrong size %d\n", data.nFileSizeLow );
    }
    SetLastError( 0xdeadbeaf );
    ok( !FindNextFileA( handle, &data ), "FindNextFileA succeeded\n" );
    ok( GetLastError() == ERROR_NO_MORE_FILES, "bad error %d\n", GetLastError() );
    ok( FindClose( handle ), "failed to close handle\n" );

    /* try FindFirstFileA on "lpt1" */
    SetLastError( 0xdeadbeaf );
    strcpy(buffer2, "lpt1");
    handle = FindFirstFileA(buffer2, &data);
    err = GetLastError();
    ok( handle != INVALID_HANDLE_VALUE, "FindFirstFile on %s failed\n", buffer2 );
    ok( 0 == lstrcmpiA(data.cFileName, "lpt1"), "wrong name %s\n", data.cFileName );
    ok( FILE_ATTRIBUTE_ARCHIVE == data.dwFileAttributes ||
        FILE_ATTRIBUTE_DEVICE == data.dwFileAttributes /* Win9x */,
        "wrong attributes %x\n", data.dwFileAttributes );
    if (data.dwFileAttributes == FILE_ATTRIBUTE_ARCHIVE)
    {
        ok( 0 == data.nFileSizeHigh, "wrong size %d\n", data.nFileSizeHigh );
        ok( 0 == data.nFileSizeLow, "wrong size %d\n", data.nFileSizeLow );
    }
    SetLastError( 0xdeadbeaf );
    ok( !FindNextFileA( handle, &data ), "FindNextFileA succeeded\n" );
    ok( GetLastError() == ERROR_NO_MORE_FILES, "bad error %d\n", GetLastError() );
    ok( FindClose( handle ), "failed to close handle\n" );

    /* try FindFirstFileA on "c:\nul\*" */
    SetLastError( 0xdeadbeaf );
    strcpy(buffer2, buffer);
    strcat(buffer2, "nul\\*");
    handle = FindFirstFileA(buffer2, &data);
    err = GetLastError();
    ok ( handle == INVALID_HANDLE_VALUE, "FindFirstFile on %s should Fail\n", buffer2 );
    ok ( err == ERROR_PATH_NOT_FOUND, "Bad Error number %d\n", err );

    /* try FindFirstFileA on "c:\nul*" */
    SetLastError( 0xdeadbeaf );
    strcpy(buffer2, buffer);
    strcat(buffer2, "nul*");
    handle = FindFirstFileA(buffer2, &data);
    err = GetLastError();
    ok ( handle == INVALID_HANDLE_VALUE, "FindFirstFile on %s should Fail\n", buffer2 );
    ok ( err == ERROR_FILE_NOT_FOUND, "Bad Error number %d\n", err );

    /* try FindFirstFileA on "c:\foo\bar\nul" */
    SetLastError( 0xdeadbeaf );
    strcpy(buffer2, buffer);
    strcat(buffer2, "foo\\bar\\nul");
    handle = FindFirstFileA(buffer2, &data);
    err = GetLastError();
    ok ( handle == INVALID_HANDLE_VALUE, "FindFirstFile on %s should Fail\n", buffer2 );
    ok ( err == ERROR_PATH_NOT_FOUND, "Bad Error number %d\n", err );

    /* try FindFirstFileA on "c:\foo\nul\bar" */
    SetLastError( 0xdeadbeaf );
    strcpy(buffer2, buffer);
    strcat(buffer2, "foo\\nul\\bar");
    handle = FindFirstFileA(buffer2, &data);
    err = GetLastError();
    ok ( handle == INVALID_HANDLE_VALUE, "FindFirstFile on %s should Fail\n", buffer2 );
    ok ( err == ERROR_PATH_NOT_FOUND, "Bad Error number %d\n", err );
}

static void test_FindNextFileA(void)
{
    HANDLE handle;
    WIN32_FIND_DATAA search_results;
    int err;
    char buffer[5] = "C:\\*";

    buffer[0] = get_windows_drive();
    handle = FindFirstFileA(buffer,&search_results);
    ok ( handle != INVALID_HANDLE_VALUE, "FindFirstFile on C:\\* should succeed\n" );
    while (FindNextFile(handle, &search_results))
    {
        /* get to the end of the files */
    }
    ok ( FindClose(handle) == TRUE, "Failed to close handle\n");
    err = GetLastError();
    ok ( err == ERROR_NO_MORE_FILES, "GetLastError should return ERROR_NO_MORE_FILES\n");
}

static void test_FindFirstFileExA(void)
{
    WIN32_FIND_DATAA search_results;
    HANDLE handle;

    if (!pFindFirstFileExA)
    {
        skip("FindFirstFileExA() is missing\n");
        return;
    }

    CreateDirectoryA("test-dir", NULL);
    _lclose(_lcreat("test-dir\\file1", 0));
    _lclose(_lcreat("test-dir\\file2", 0));
    CreateDirectoryA("test-dir\\dir1", NULL);
    /* FindExLimitToDirectories is ignored */
    SetLastError(0xdeadbeef);
    handle = pFindFirstFileExA("test-dir\\*", FindExInfoStandard, &search_results, FindExSearchLimitToDirectories, NULL, 0);
    if (handle == INVALID_HANDLE_VALUE && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        skip("FindFirstFileExA is not implemented\n");
        goto cleanup;
    }
    ok(handle != INVALID_HANDLE_VALUE, "FindFirstFile failed (err=%u)\n", GetLastError());
    ok(strcmp(search_results.cFileName, ".") == 0, "First entry should be '.', is %s\n", search_results.cFileName);

#define CHECK_NAME(fn) (strcmp((fn), "file1") == 0 || strcmp((fn), "file2") == 0 || strcmp((fn), "dir1") == 0)

    ok(FindNextFile(handle, &search_results), "Fetching second file failed\n");
    ok(strcmp(search_results.cFileName, "..") == 0, "Second entry should be '..' is %s\n", search_results.cFileName);

    ok(FindNextFile(handle, &search_results), "Fetching third file failed\n");
    ok(CHECK_NAME(search_results.cFileName), "Invalid third entry - %s\n", search_results.cFileName);

    ok(FindNextFile(handle, &search_results), "Fetching fourth file failed\n");
    ok(CHECK_NAME(search_results.cFileName), "Invalid fourth entry - %s\n", search_results.cFileName);

    ok(FindNextFile(handle, &search_results), "Fetching fifth file failed\n");
    ok(CHECK_NAME(search_results.cFileName), "Invalid fifth entry - %s\n", search_results.cFileName);

#undef CHECK_NAME

    ok(FindNextFile(handle, &search_results) == FALSE, "Fetching sixth file should failed\n");

    FindClose( handle );

cleanup:
    DeleteFileA("test-dir\\file1");
    DeleteFileA("test-dir\\file2");
    RemoveDirectoryA("test-dir\\dir1");
    RemoveDirectoryA("test-dir");
}

static int test_Mapfile_createtemp(HANDLE *handle)
{
    SetFileAttributesA(filename,FILE_ATTRIBUTE_NORMAL);
    DeleteFile(filename);
    *handle = CreateFile(filename, GENERIC_READ|GENERIC_WRITE, 0, 0,
                         CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (*handle != INVALID_HANDLE_VALUE) {

        return 1;
    }

    return 0;
}

static void test_MapFile(void)
{
    HANDLE handle;
    HANDLE hmap;

    ok(test_Mapfile_createtemp(&handle), "Couldn't create test file.\n");

    hmap = CreateFileMapping( handle, NULL, PAGE_READWRITE, 0, 0x1000, "named_file_map" );
    ok( hmap != NULL, "mapping should work, I named it!\n" );

    ok( CloseHandle( hmap ), "can't close mapping handle\n");

    /* We have to close file before we try new stuff with mapping again.
       Else we would always succeed on XP or block descriptors on 95. */
    hmap = CreateFileMapping( handle, NULL, PAGE_READWRITE, 0, 0, NULL );
    ok( hmap != NULL, "We should still be able to map!\n" );
    ok( CloseHandle( hmap ), "can't close mapping handle\n");
    ok( CloseHandle( handle ), "can't close file handle\n");
    handle = NULL;

    ok(test_Mapfile_createtemp(&handle), "Couldn't create test file.\n");

    hmap = CreateFileMapping( handle, NULL, PAGE_READWRITE, 0, 0, NULL );
    ok( hmap == NULL, "mapped zero size file\n");
    ok( GetLastError() == ERROR_FILE_INVALID, "not ERROR_FILE_INVALID\n");

    hmap = CreateFileMapping( handle, NULL, PAGE_READWRITE, 0x80000000, 0, NULL );
    ok( hmap == NULL || broken(hmap != NULL) /* NT4 */, "mapping should fail\n");
    /* GetLastError() varies between win9x and WinNT and also depends on the filesystem */
    if ( hmap )
        CloseHandle( hmap );

    hmap = CreateFileMapping( handle, NULL, PAGE_READWRITE, 0x80000000, 0x10000, NULL );
    ok( hmap == NULL || broken(hmap != NULL) /* NT4 */, "mapping should fail\n");
    /* GetLastError() varies between win9x and WinNT and also depends on the filesystem */
    if ( hmap )
        CloseHandle( hmap );

    /* On XP you can now map again, on Win 95 you cannot. */

    ok( CloseHandle( handle ), "can't close file handle\n");
    ok( DeleteFileA( filename ), "DeleteFile failed after map\n" );
}

static void test_GetFileType(void)
{
    DWORD type;
    HANDLE h = CreateFileA( filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( h != INVALID_HANDLE_VALUE, "open %s failed\n", filename );
    type = GetFileType(h);
    ok( type == FILE_TYPE_DISK, "expected type disk got %d\n", type );
    CloseHandle( h );
    h = CreateFileA( "nul", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( h != INVALID_HANDLE_VALUE, "open nul failed\n" );
    type = GetFileType(h);
    ok( type == FILE_TYPE_CHAR, "expected type char for nul got %d\n", type );
    CloseHandle( h );
    DeleteFileA( filename );
}

static int completion_count;

static void CALLBACK FileIOComplete(DWORD dwError, DWORD dwBytes, LPOVERLAPPED ovl)
{
/*	printf("(%ld, %ld, %p { %ld, %ld, %ld, %ld, %p })\n", dwError, dwBytes, ovl, ovl->Internal, ovl->InternalHigh, ovl->Offset, ovl->OffsetHigh, ovl->hEvent);*/
	ReleaseSemaphore(ovl->hEvent, 1, NULL);
	completion_count++;
}

static void test_async_file_errors(void)
{
    char szFile[MAX_PATH];
    HANDLE hSem = CreateSemaphoreW(NULL, 1, 1, NULL);
    HANDLE hFile;
    LPVOID lpBuffer = HeapAlloc(GetProcessHeap(), 0, 4096);
    OVERLAPPED ovl;
    S(U(ovl)).Offset = 0;
    S(U(ovl)).OffsetHigh = 0;
    ovl.hEvent = hSem;
    completion_count = 0;
    szFile[0] = '\0';
    GetWindowsDirectoryA(szFile, sizeof(szFile)/sizeof(szFile[0])-1-strlen("\\win.ini"));
    strcat(szFile, "\\win.ini");
    hFile = CreateFileA(szFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
    if (hFile == INVALID_HANDLE_VALUE)  /* win9x doesn't like FILE_SHARE_DELETE */
        hFile = CreateFileA(szFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA(%s ...) failed\n", szFile);
    while (TRUE)
    {
        BOOL res;
        DWORD count;
        while (WaitForSingleObjectEx(hSem, INFINITE, TRUE) == WAIT_IO_COMPLETION)
            ;
        res = ReadFileEx(hFile, lpBuffer, 4096, &ovl, FileIOComplete);
        /*printf("Offset = %ld, result = %s\n", ovl.Offset, res ? "TRUE" : "FALSE");*/
        if (!res)
            break;
        if (!GetOverlappedResult(hFile, &ovl, &count, FALSE))
            break;
        S(U(ovl)).Offset += count;
        /* i/o completion routine only called if ReadFileEx returned success.
         * we only care about violations of this rule so undo what should have
         * been done */
        completion_count--;
    }
    ok(completion_count == 0, "completion routine should only be called when ReadFileEx succeeds (this rule was violated %d times)\n", completion_count);
    /*printf("Error = %ld\n", GetLastError());*/
    HeapFree(GetProcessHeap(), 0, lpBuffer);
}

static void test_read_write(void)
{
    DWORD bytes, ret;
    HANDLE hFile;
    char temp_path[MAX_PATH];
    char filename[MAX_PATH];
    static const char prefix[] = "pfx";

    ret = GetTempPathA(MAX_PATH, temp_path);
    ok(ret != 0, "GetTempPathA error %d\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameA(temp_path, prefix, 0, filename);
    ok(ret != 0, "GetTempFileNameA error %d\n", GetLastError());

    hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA: error %d\n", GetLastError());

    SetLastError(12345678);
    bytes = 12345678;
    ret = WriteFile(hFile, NULL, 0, &bytes, NULL);
    ok(ret && GetLastError() == 12345678,
	"ret = %d, error %d\n", ret, GetLastError());
    ok(!bytes, "bytes = %d\n", bytes);

    SetLastError(12345678);
    bytes = 12345678;
    ret = WriteFile(hFile, NULL, 10, &bytes, NULL);
    ok((!ret && GetLastError() == ERROR_INVALID_USER_BUFFER) || /* Win2k */
	(ret && GetLastError() == 12345678), /* Win9x */
	"ret = %d, error %d\n", ret, GetLastError());
    ok(!bytes || /* Win2k */
	bytes == 10, /* Win9x */
	"bytes = %d\n", bytes);

    /* make sure the file contains data */
    WriteFile(hFile, "this is the test data", 21, &bytes, NULL);
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

    SetLastError(12345678);
    bytes = 12345678;
    ret = ReadFile(hFile, NULL, 0, &bytes, NULL);
    ok(ret && GetLastError() == 12345678,
	"ret = %d, error %d\n", ret, GetLastError());
    ok(!bytes, "bytes = %d\n", bytes);

    SetLastError(12345678);
    bytes = 12345678;
    ret = ReadFile(hFile, NULL, 10, &bytes, NULL);
    ok(!ret && (GetLastError() == ERROR_NOACCESS || /* Win2k */
		GetLastError() == ERROR_INVALID_PARAMETER), /* Win9x */
	"ret = %d, error %d\n", ret, GetLastError());
    ok(!bytes, "bytes = %d\n", bytes);

    ret = CloseHandle(hFile);
    ok( ret, "CloseHandle: error %d\n", GetLastError());
    ret = DeleteFileA(filename);
    ok( ret, "DeleteFileA: error %d\n", GetLastError());
}

static void test_OpenFile(void)
{
    HFILE hFile;
    OFSTRUCT ofs;
    BOOL ret;
    DWORD retval;
    
    static const char file[] = "regedit.exe";
    static const char foo[] = ".\\foo-bar-foo.baz";
    static const char *foo_too_long = ".\\foo-bar-foo.baz+++++++++++++++"
        "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
        "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
        "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
        "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
        "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++";
    char buff[MAX_PATH];
    char buff_long[4*MAX_PATH];
    char filled_0xA5[OFS_MAXPATHNAME];
    char *p;
    UINT length;
    
    /* Check for existing file */
    length = GetWindowsDirectoryA(buff, MAX_PATH);

    if (length + sizeof(file) < MAX_PATH)
    {
        p = buff + strlen(buff);
        if (p > buff && p[-1] != '\\') *p++ = '\\';
        strcpy( p, file );
        memset(&ofs, 0xA5, sizeof(ofs));
        SetLastError(0xfaceabee);

        hFile = OpenFile(buff, &ofs, OF_EXIST);
        ok( hFile == TRUE, "%s not found : %d\n", buff, GetLastError() );
        ok( GetLastError() == 0xfaceabee || GetLastError() == ERROR_SUCCESS, 
            "GetLastError() returns %d\n", GetLastError() );
        ok( ofs.cBytes == sizeof(ofs), "OpenFile set ofs.cBytes to %d\n", ofs.cBytes );
        ok( ofs.nErrCode == ERROR_SUCCESS, "OpenFile set ofs.nErrCode to %d\n", ofs.nErrCode );
        ok( lstrcmpiA(ofs.szPathName, buff) == 0,
            "OpenFile returned '%s', but was expected to return '%s' or string filled with 0xA5\n",
            ofs.szPathName, buff );
    }

    memset(&filled_0xA5, 0xA5, OFS_MAXPATHNAME);
    length = GetCurrentDirectoryA(MAX_PATH, buff);

    /* Check for nonexistent file */
    if (length + sizeof(foo) < MAX_PATH)
    {
        p = buff + strlen(buff);
        if (p > buff && p[-1] != '\\') *p++ = '\\';
        strcpy( p, foo + 2 );
        memset(&ofs, 0xA5, sizeof(ofs));
        SetLastError(0xfaceabee);

        hFile = OpenFile(foo, &ofs, OF_EXIST);
        ok( hFile == HFILE_ERROR, "hFile != HFILE_ERROR : %d\n", GetLastError());
        ok( GetLastError() == ERROR_FILE_NOT_FOUND, "GetLastError() returns %d\n", GetLastError() );
        todo_wine
        ok( ofs.cBytes == 0xA5, "OpenFile set ofs.cBytes to %d\n", ofs.cBytes );
        ok( ofs.nErrCode == ERROR_FILE_NOT_FOUND, "OpenFile set ofs.nErrCode to %d\n", ofs.nErrCode );
        ok( lstrcmpiA(ofs.szPathName, buff) == 0 || strncmp(ofs.szPathName, filled_0xA5, OFS_MAXPATHNAME) == 0,
            "OpenFile returned '%s', but was expected to return '%s' or string filled with 0xA5\n", 
            ofs.szPathName, buff );
    }

    length = GetCurrentDirectoryA(MAX_PATH, buff_long);
    length += lstrlenA(foo_too_long + 1);

    /* Check for nonexistent file with too long filename */ 
    if (length >= OFS_MAXPATHNAME && length < sizeof(buff_long)) 
    {
        lstrcatA(buff_long, foo_too_long + 1); /* Avoid '.' during concatenation */
        memset(&ofs, 0xA5, sizeof(ofs));
        SetLastError(0xfaceabee);

        hFile = OpenFile(foo_too_long, &ofs, OF_EXIST);
        ok( hFile == HFILE_ERROR, "hFile != HFILE_ERROR : %d\n", GetLastError());
        ok( GetLastError() == ERROR_INVALID_DATA || GetLastError() == ERROR_FILENAME_EXCED_RANGE, 
            "GetLastError() returns %d\n", GetLastError() );
        todo_wine
        ok( ofs.cBytes == 0xA5, "OpenFile set ofs.cBytes to %d\n", ofs.cBytes );
        ok( ofs.nErrCode == ERROR_INVALID_DATA || ofs.nErrCode == ERROR_FILENAME_EXCED_RANGE,
            "OpenFile set ofs.nErrCode to %d\n", ofs.nErrCode );
        ok( strncmp(ofs.szPathName, filled_0xA5, OFS_MAXPATHNAME) == 0, 
            "OpenFile returned '%s', but was expected to return string filled with 0xA5\n", 
            ofs.szPathName );
    }

    length = GetCurrentDirectoryA(MAX_PATH, buff) + sizeof(filename);

    if (length >= MAX_PATH) 
    {
        trace("Buffer too small, requested length = %d, but MAX_PATH = %d.  Skipping test.\n", length, MAX_PATH);
        return;
    }
    p = buff + strlen(buff);
    if (p > buff && p[-1] != '\\') *p++ = '\\';
    strcpy( p, filename );

    memset(&ofs, 0xA5, sizeof(ofs));
    SetLastError(0xfaceabee);
    /* Create an empty file */
    hFile = OpenFile(filename, &ofs, OF_CREATE);
    ok( hFile != HFILE_ERROR, "OpenFile failed to create nonexistent file\n" );
    ok( GetLastError() == 0xfaceabee || GetLastError() == ERROR_SUCCESS, 
        "GetLastError() returns %d\n", GetLastError() );
    ok( ofs.cBytes == sizeof(OFSTRUCT), "OpenFile set ofs.cBytes to %d\n", ofs.cBytes );
    ok( ofs.nErrCode == ERROR_SUCCESS || broken(ofs.nErrCode != ERROR_SUCCESS) /* win9x */,
        "OpenFile set ofs.nErrCode to %d\n", ofs.nErrCode );
    ret = CloseHandle((HANDLE)hFile);
    ok( ret == TRUE, "CloseHandle() returns %d\n", ret );
    retval = GetFileAttributesA(filename);
    ok( retval != INVALID_FILE_ATTRIBUTES, "GetFileAttributesA: error %d\n", GetLastError() );

    memset(&ofs, 0xA5, sizeof(ofs));
    SetLastError(0xfaceabee);
    /* Check various opening options: */
    /* for reading only, */
    hFile = OpenFile(filename, &ofs, OF_READ);
    ok( hFile != HFILE_ERROR, "OpenFile failed on read\n" );
    ok( GetLastError() == 0xfaceabee || GetLastError() == ERROR_SUCCESS, 
        "GetLastError() returns %d\n", GetLastError() );
    ok( ofs.cBytes == sizeof(OFSTRUCT), "OpenFile set ofs.cBytes to %d\n", ofs.cBytes );
    ok( ofs.nErrCode == ERROR_SUCCESS || broken(ofs.nErrCode != ERROR_SUCCESS) /* win9x */,
        "OpenFile set ofs.nErrCode to %d\n", ofs.nErrCode );
    ok( lstrcmpiA(ofs.szPathName, buff) == 0,
        "OpenFile returned '%s', but was expected to return '%s'\n", ofs.szPathName, buff );
    ret = CloseHandle((HANDLE)hFile);
    ok( ret == TRUE, "CloseHandle() returns %d\n", ret );

    memset(&ofs, 0xA5, sizeof(ofs));
    SetLastError(0xfaceabee);
    /* for writing only, */
    hFile = OpenFile(filename, &ofs, OF_WRITE);
    ok( hFile != HFILE_ERROR, "OpenFile failed on write\n" );
    ok( GetLastError() == 0xfaceabee || GetLastError() == ERROR_SUCCESS, 
        "GetLastError() returns %d\n", GetLastError() );
    ok( ofs.cBytes == sizeof(OFSTRUCT), "OpenFile set ofs.cBytes to %d\n", ofs.cBytes );
    ok( ofs.nErrCode == ERROR_SUCCESS || broken(ofs.nErrCode != ERROR_SUCCESS) /* win9x */,
        "OpenFile set ofs.nErrCode to %d\n", ofs.nErrCode );
    ok( lstrcmpiA(ofs.szPathName, buff) == 0,
        "OpenFile returned '%s', but was expected to return '%s'\n", ofs.szPathName, buff );
    ret = CloseHandle((HANDLE)hFile);
    ok( ret == TRUE, "CloseHandle() returns %d\n", ret );

    memset(&ofs, 0xA5, sizeof(ofs));
    SetLastError(0xfaceabee);
    /* for reading and writing, */
    hFile = OpenFile(filename, &ofs, OF_READWRITE);
    ok( hFile != HFILE_ERROR, "OpenFile failed on read/write\n" );
    ok( GetLastError() == 0xfaceabee || GetLastError() == ERROR_SUCCESS, 
        "GetLastError() returns %d\n", GetLastError() );
    ok( ofs.cBytes == sizeof(OFSTRUCT), "OpenFile set ofs.cBytes to %d\n", ofs.cBytes );
    ok( ofs.nErrCode == ERROR_SUCCESS || broken(ofs.nErrCode != ERROR_SUCCESS) /* win9x */,
        "OpenFile set ofs.nErrCode to %d\n", ofs.nErrCode );
    ok( lstrcmpiA(ofs.szPathName, buff) == 0,
        "OpenFile returned '%s', but was expected to return '%s'\n", ofs.szPathName, buff );
    ret = CloseHandle((HANDLE)hFile);
    ok( ret == TRUE, "CloseHandle() returns %d\n", ret );

    memset(&ofs, 0xA5, sizeof(ofs));
    SetLastError(0xfaceabee);
    /* for checking file presence. */
    hFile = OpenFile(filename, &ofs, OF_EXIST);
    ok( hFile == 1, "OpenFile failed on finding our created file\n" );
    ok( GetLastError() == 0xfaceabee || GetLastError() == ERROR_SUCCESS, 
        "GetLastError() returns %d\n", GetLastError() );
    ok( ofs.cBytes == sizeof(OFSTRUCT), "OpenFile set ofs.cBytes to %d\n", ofs.cBytes );
    ok( ofs.nErrCode == ERROR_SUCCESS || broken(ofs.nErrCode != ERROR_SUCCESS) /* win9x */,
        "OpenFile set ofs.nErrCode to %d\n", ofs.nErrCode );
    ok( lstrcmpiA(ofs.szPathName, buff) == 0,
        "OpenFile returned '%s', but was expected to return '%s'\n", ofs.szPathName, buff );

    memset(&ofs, 0xA5, sizeof(ofs));
    SetLastError(0xfaceabee);
    /* Delete the file and make sure it doesn't exist anymore */
    hFile = OpenFile(filename, &ofs, OF_DELETE);
    ok( hFile == 1, "OpenFile failed on delete (%d)\n", hFile );
    ok( GetLastError() == 0xfaceabee || GetLastError() == ERROR_SUCCESS, 
        "GetLastError() returns %d\n", GetLastError() );
    ok( ofs.cBytes == sizeof(OFSTRUCT), "OpenFile set ofs.cBytes to %d\n", ofs.cBytes );
    ok( ofs.nErrCode == ERROR_SUCCESS || broken(ofs.nErrCode != ERROR_SUCCESS) /* win9x */,
        "OpenFile set ofs.nErrCode to %d\n", ofs.nErrCode );
    ok( lstrcmpiA(ofs.szPathName, buff) == 0,
        "OpenFile returned '%s', but was expected to return '%s'\n", ofs.szPathName, buff );

    retval = GetFileAttributesA(filename);
    ok( retval == INVALID_FILE_ATTRIBUTES, "GetFileAttributesA succeeded on deleted file\n" );
}

static void test_overlapped(void)
{
    OVERLAPPED ov;
    DWORD r, result;

    /* GetOverlappedResult crashes if the 2nd or 3rd param are NULL */

    memset( &ov, 0,  sizeof ov );
    result = 1;
    r = GetOverlappedResult(0, &ov, &result, 0);
    if (r)
        ok( result == 0, "wrong result %u\n", result );
    else  /* win9x */
        ok( GetLastError() == ERROR_INVALID_HANDLE, "wrong error %u\n", GetLastError() );

    result = 0;
    ov.Internal = 0;
    ov.InternalHigh = 0xabcd;
    r = GetOverlappedResult(0, &ov, &result, 0);
    if (r)
        ok( result == 0xabcd, "wrong result %u\n", result );
    else  /* win9x */
        ok( GetLastError() == ERROR_INVALID_HANDLE, "wrong error %u\n", GetLastError() );

    SetLastError( 0xb00 );
    result = 0;
    ov.Internal = STATUS_INVALID_HANDLE;
    ov.InternalHigh = 0xabcd;
    r = GetOverlappedResult(0, &ov, &result, 0);
    ok( GetLastError() == ERROR_INVALID_HANDLE, "wrong error %u\n", GetLastError() );
    ok( r == FALSE, "should return false\n");
    ok( result == 0xabcd || result == 0 /* win9x */, "wrong result %u\n", result );

    SetLastError( 0xb00 );
    result = 0;
    ov.Internal = STATUS_PENDING;
    ov.InternalHigh = 0xabcd;
    r = GetOverlappedResult(0, &ov, &result, 0);
    ok( GetLastError() == ERROR_IO_INCOMPLETE || GetLastError() == ERROR_INVALID_HANDLE /* win9x */,
        "wrong error %u\n", GetLastError() );
    ok( r == FALSE, "should return false\n");
    ok( result == 0, "wrong result %u\n", result );

    SetLastError( 0xb00 );
    ov.hEvent = CreateEvent( NULL, 1, 1, NULL );
    ov.Internal = STATUS_PENDING;
    ov.InternalHigh = 0xabcd;
    r = GetOverlappedResult(0, &ov, &result, 0);
    ok( GetLastError() == ERROR_IO_INCOMPLETE || GetLastError() == ERROR_INVALID_HANDLE /* win9x */,
        "wrong error %u\n", GetLastError() );
    ok( r == FALSE, "should return false\n");

    ResetEvent( ov.hEvent );

    SetLastError( 0xb00 );
    ov.Internal = STATUS_PENDING;
    ov.InternalHigh = 0;
    r = GetOverlappedResult(0, &ov, &result, 0);
    ok( GetLastError() == ERROR_IO_INCOMPLETE || GetLastError() == ERROR_INVALID_HANDLE /* win9x */,
        "wrong error %u\n", GetLastError() );
    ok( r == FALSE, "should return false\n");

    r = CloseHandle( ov.hEvent );
    ok( r == TRUE, "close handle failed\n");
}

static void test_RemoveDirectory(void)
{
    int rc;
    char directory[] = "removeme";

    rc = CreateDirectory(directory, NULL);
    ok( rc, "Createdirectory failed, gle=%d\n", GetLastError() );

    rc = SetCurrentDirectory(directory);
    ok( rc, "SetCurrentDirectory failed, gle=%d\n", GetLastError() );

    rc = RemoveDirectory(".");
    if (!rc)
    {
        rc = SetCurrentDirectory("..");
        ok( rc, "SetCurrentDirectory failed, gle=%d\n", GetLastError() );

        rc = RemoveDirectory(directory);
        ok( rc, "RemoveDirectory failed, gle=%d\n", GetLastError() );
    }
}

static BOOL check_file_time( const FILETIME *ft1, const FILETIME *ft2, UINT tolerance )
{
    ULONGLONG t1 = ((ULONGLONG)ft1->dwHighDateTime << 32) | ft1->dwLowDateTime;
    ULONGLONG t2 = ((ULONGLONG)ft2->dwHighDateTime << 32) | ft2->dwLowDateTime;
    return abs(t1 - t2) <= tolerance;
}

static void test_ReplaceFileA(void)
{
    char replaced[MAX_PATH], replacement[MAX_PATH], backup[MAX_PATH];
    HANDLE hReplacedFile, hReplacementFile, hBackupFile;
    static const char replacedData[] = "file-to-replace";
    static const char replacementData[] = "new-file";
    static const char backupData[] = "backup-file";
    FILETIME ftReplaced, ftReplacement, ftBackup;
    static const char prefix[] = "pfx";
    char temp_path[MAX_PATH];
    DWORD ret;
    BOOL retok;

    if (!pReplaceFileA)
    {
        skip("ReplaceFileA() is missing\n");
        return;
    }

    ret = GetTempPathA(MAX_PATH, temp_path);
    ok(ret != 0, "GetTempPathA error %d\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameA(temp_path, prefix, 0, replaced);
    ok(ret != 0, "GetTempFileNameA error (replaced) %d\n", GetLastError());

    ret = GetTempFileNameA(temp_path, prefix, 0, replacement);
    ok(ret != 0, "GetTempFileNameA error (replacement) %d\n", GetLastError());

    ret = GetTempFileNameA(temp_path, prefix, 0, backup);
    ok(ret != 0, "GetTempFileNameA error (backup) %d\n", GetLastError());

    /* place predictable data in the file to be replaced */
    hReplacedFile = CreateFileA(replaced, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok(hReplacedFile != INVALID_HANDLE_VALUE,
        "failed to open replaced file\n");
    retok = WriteFile(hReplacedFile, replacedData, sizeof(replacedData), &ret, NULL );
    ok( retok && ret == sizeof(replacedData),
       "WriteFile error (replaced) %d\n", GetLastError());
    ok(GetFileSize(hReplacedFile, NULL) == sizeof(replacedData),
        "replaced file has wrong size\n");
    /* place predictable data in the file to be the replacement */
    hReplacementFile = CreateFileA(replacement, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok(hReplacementFile != INVALID_HANDLE_VALUE,
        "failed to open replacement file\n");
    retok = WriteFile(hReplacementFile, replacementData, sizeof(replacementData), &ret, NULL );
    ok( retok && ret == sizeof(replacementData),
       "WriteFile error (replacement) %d\n", GetLastError());
    ok(GetFileSize(hReplacementFile, NULL) == sizeof(replacementData),
        "replacement file has wrong size\n");
    /* place predictable data in the backup file (to be over-written) */
    hBackupFile = CreateFileA(backup, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok(hBackupFile != INVALID_HANDLE_VALUE,
        "failed to open backup file\n");
    retok = WriteFile(hBackupFile, backupData, sizeof(backupData), &ret, NULL );
    ok( retok && ret == sizeof(backupData),
       "WriteFile error (replacement) %d\n", GetLastError());
    ok(GetFileSize(hBackupFile, NULL) == sizeof(backupData),
        "backup file has wrong size\n");
    /* change the filetime on the "replaced" file to ensure that it changes */
    ret = GetFileTime(hReplacedFile, NULL, NULL, &ftReplaced);
    ok( ret, "GetFileTime error (replaced) %d\n", GetLastError());
    ftReplaced.dwLowDateTime -= 600000000; /* 60 second */
    ret = SetFileTime(hReplacedFile, NULL, NULL, &ftReplaced);
    ok( ret, "SetFileTime error (replaced) %d\n", GetLastError());
    GetFileTime(hReplacedFile, NULL, NULL, &ftReplaced);  /* get the actual time back */
    CloseHandle(hReplacedFile);
    /* change the filetime on the backup to ensure that it changes */
    ret = GetFileTime(hBackupFile, NULL, NULL, &ftBackup);
    ok( ret, "GetFileTime error (backup) %d\n", GetLastError());
    ftBackup.dwLowDateTime -= 1200000000; /* 120 second */
    ret = SetFileTime(hBackupFile, NULL, NULL, &ftBackup);
    ok( ret, "SetFileTime error (backup) %d\n", GetLastError());
    GetFileTime(hBackupFile, NULL, NULL, &ftBackup);  /* get the actual time back */
    CloseHandle(hBackupFile);
    /* get the filetime on the replacement file to perform checks */
    ret = GetFileTime(hReplacementFile, NULL, NULL, &ftReplacement);
    ok( ret, "GetFileTime error (replacement) %d\n", GetLastError());
    CloseHandle(hReplacementFile);

    /* perform replacement w/ backup
     * TODO: flags are not implemented
     */
    SetLastError(0xdeadbeef);
    ret = pReplaceFileA(replaced, replacement, backup, 0, 0, 0);
    ok(ret, "ReplaceFileA: unexpected error %d\n", GetLastError());
    /* make sure that the backup has the size of the old "replaced" file */
    hBackupFile = CreateFileA(backup, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    ok(hBackupFile != INVALID_HANDLE_VALUE,
        "failed to open backup file\n");
    ret = GetFileSize(hBackupFile, NULL);
    ok(ret == sizeof(replacedData),
        "backup file has wrong size %d\n", ret);
    /* make sure that the "replaced" file has the size of the replacement file */
    hReplacedFile = CreateFileA(replaced, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    ok(hReplacedFile != INVALID_HANDLE_VALUE,
        "failed to open replaced file\n");
    ret = GetFileSize(hReplacedFile, NULL);
    ok(ret == sizeof(replacementData),
        "replaced file has wrong size %d\n", ret);
    /* make sure that the replacement file no-longer exists */
    hReplacementFile = CreateFileA(replacement, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    ok(hReplacementFile == INVALID_HANDLE_VALUE,
       "unexpected error, replacement file should not exist %d\n", GetLastError());
    /* make sure that the backup has the old "replaced" filetime */
    ret = GetFileTime(hBackupFile, NULL, NULL, &ftBackup);
    ok( ret, "GetFileTime error (backup %d\n", GetLastError());
    ok(check_file_time(&ftBackup, &ftReplaced, 20000000), "backup file has wrong filetime\n");
    CloseHandle(hBackupFile);
    /* make sure that the "replaced" has the old replacement filetime */
    ret = GetFileTime(hReplacedFile, NULL, NULL, &ftReplaced);
    ok( ret, "GetFileTime error (backup %d\n", GetLastError());
    ok(check_file_time(&ftReplaced, &ftReplacement, 20000000),
       "replaced file has wrong filetime %x%08x / %x%08x\n",
       ftReplaced.dwHighDateTime, ftReplaced.dwLowDateTime,
       ftReplacement.dwHighDateTime, ftReplacement.dwLowDateTime );
    CloseHandle(hReplacedFile);

    /* re-create replacement file for pass w/o backup (blank) */
    ret = GetTempFileNameA(temp_path, prefix, 0, replacement);
    ok(ret != 0, "GetTempFileNameA error (replacement) %d\n", GetLastError());
    /* perform replacement w/o backup
     * TODO: flags are not implemented
     */
    SetLastError(0xdeadbeef);
    ret = pReplaceFileA(replaced, replacement, NULL, 0, 0, 0);
    ok(ret, "ReplaceFileA: unexpected error %d\n", GetLastError());

    /* re-create replacement file for pass w/ backup (backup-file not existing) */
    ret = GetTempFileNameA(temp_path, prefix, 0, replacement);
    ok(ret != 0, "GetTempFileNameA error (replacement) %d\n", GetLastError());
    ret = DeleteFileA(backup);
    ok(ret, "DeleteFileA: error (backup) %d\n", GetLastError());
    /* perform replacement w/ backup (no pre-existing backup)
     * TODO: flags are not implemented
     */
    SetLastError(0xdeadbeef);
    ret = pReplaceFileA(replaced, replacement, backup, 0, 0, 0);
    ok(ret, "ReplaceFileA: unexpected error %d\n", GetLastError());

    /* re-create replacement file for pass w/ no permissions to "replaced" */
    ret = GetTempFileNameA(temp_path, prefix, 0, replacement);
    ok(ret != 0, "GetTempFileNameA error (replacement) %d\n", GetLastError());
    ret = SetFileAttributesA(replaced, FILE_ATTRIBUTE_READONLY);
    ok(ret, "SetFileAttributesA: error setting to read only %d\n", GetLastError());
    /* perform replacement w/ backup (no permission to "replaced")
     * TODO: flags are not implemented
     */
    SetLastError(0xdeadbeef);
    ret = pReplaceFileA(replaced, replacement, backup, 0, 0, 0);
    ok(ret != ERROR_UNABLE_TO_REMOVE_REPLACED, "ReplaceFileA: unexpected error %d\n", GetLastError());
    /* make sure that the replacement file still exists */
    hReplacementFile = CreateFileA(replacement, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    ok(hReplacementFile != INVALID_HANDLE_VALUE ||
       broken(GetLastError() == ERROR_FILE_NOT_FOUND), /* win2k */
       "unexpected error, replacement file should still exist %d\n", GetLastError());
    CloseHandle(hReplacementFile);
    ret = SetFileAttributesA(replaced, FILE_ATTRIBUTE_NORMAL);
    ok(ret, "SetFileAttributesA: error setting to normal %d\n", GetLastError());

    /* replacement file still exists, make pass w/o "replaced" */
    ret = DeleteFileA(replaced);
    ok(ret, "DeleteFileA: error (replaced) %d\n", GetLastError());
    /* perform replacement w/ backup (no pre-existing backup or "replaced")
     * TODO: flags are not implemented
     */
    SetLastError(0xdeadbeef);
    ret = pReplaceFileA(replaced, replacement, backup, 0, 0, 0);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
			"ReplaceFileA: unexpected error %d\n", GetLastError());

    /* perform replacement w/o existing "replacement" file
     * TODO: flags are not implemented
     */
    SetLastError(0xdeadbeef);
    ret = pReplaceFileA(replaced, replacement, NULL, 0, 0, 0);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
        "ReplaceFileA: unexpected error %d\n", GetLastError());

    /*
     * if the first round (w/ backup) worked then as long as there is no
     * failure then there is no need to check this round (w/ backup is the
     * more complete case)
     */

    /* delete temporary files, replacement and replaced are already deleted */
    ret = DeleteFileA(backup);
    ok(ret ||
       broken(GetLastError() == ERROR_ACCESS_DENIED), /* win2k */
       "DeleteFileA: error (backup) %d\n", GetLastError());
}

/*
 * ReplaceFileW is a simpler case of ReplaceFileA, there is no
 * need to be as thorough.
 */
static void test_ReplaceFileW(void)
{
    WCHAR replaced[MAX_PATH], replacement[MAX_PATH], backup[MAX_PATH];
    static const WCHAR prefix[] = {'p','f','x',0};
    WCHAR temp_path[MAX_PATH];
    DWORD ret;

    if (!pReplaceFileW)
    {
        skip("ReplaceFileW() is missing\n");
        return;
    }

    ret = GetTempPathW(MAX_PATH, temp_path);
    if (ret==0 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        return;
    ok(ret != 0, "GetTempPathW error %d\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameW(temp_path, prefix, 0, replaced);
    ok(ret != 0, "GetTempFileNameW error (replaced) %d\n", GetLastError());

    ret = GetTempFileNameW(temp_path, prefix, 0, replacement);
    ok(ret != 0, "GetTempFileNameW error (replacement) %d\n", GetLastError());

    ret = GetTempFileNameW(temp_path, prefix, 0, backup);
    ok(ret != 0, "GetTempFileNameW error (backup) %d\n", GetLastError());

    ret = pReplaceFileW(replaced, replacement, backup, 0, 0, 0);
    ok(ret, "ReplaceFileW: error %d\n", GetLastError());

    ret = GetTempFileNameW(temp_path, prefix, 0, replacement);
    ok(ret != 0, "GetTempFileNameW error (replacement) %d\n", GetLastError());
    ret = pReplaceFileW(replaced, replacement, NULL, 0, 0, 0);
    ok(ret, "ReplaceFileW: error %d\n", GetLastError());

    ret = GetTempFileNameW(temp_path, prefix, 0, replacement);
    ok(ret != 0, "GetTempFileNameW error (replacement) %d\n", GetLastError());
    ret = DeleteFileW(backup);
    ok(ret, "DeleteFileW: error (backup) %d\n", GetLastError());
    ret = pReplaceFileW(replaced, replacement, backup, 0, 0, 0);
    ok(ret, "ReplaceFileW: error %d\n", GetLastError());

    ret = GetTempFileNameW(temp_path, prefix, 0, replacement);
    ok(ret != 0, "GetTempFileNameW error (replacement) %d\n", GetLastError());
    ret = SetFileAttributesW(replaced, FILE_ATTRIBUTE_READONLY);
    ok(ret, "SetFileAttributesW: error setting to read only %d\n", GetLastError());

    ret = pReplaceFileW(replaced, replacement, backup, 0, 0, 0);
    ok(ret != ERROR_UNABLE_TO_REMOVE_REPLACED,
        "ReplaceFileW: unexpected error %d\n", GetLastError());
    ret = SetFileAttributesW(replaced, FILE_ATTRIBUTE_NORMAL);
    ok(ret, "SetFileAttributesW: error setting to normal %d\n", GetLastError());

    ret = DeleteFileW(replaced);
    ok(ret, "DeleteFileW: error (replaced) %d\n", GetLastError());
    ret = pReplaceFileW(replaced, replacement, backup, 0, 0, 0);
    ok(!ret, "ReplaceFileW: error %d\n", GetLastError());

    ret = pReplaceFileW(replaced, replacement, NULL, 0, 0, 0);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
        "ReplaceFileW: unexpected error %d\n", GetLastError());

    ret = DeleteFileW(backup);
    ok(ret ||
       broken(GetLastError() == ERROR_ACCESS_DENIED), /* win2k */
       "DeleteFileW: error (backup) %d\n", GetLastError());
}

START_TEST(file)
{
    InitFunctionPointers();

    test__hread(  );
    test__hwrite(  );
    test__lclose(  );
    test__lcreat(  );
    test__llseek(  );
    test__llopen(  );
    test__lread(  );
    test__lwrite(  );
    test_GetTempFileNameA();
    test_CopyFileA();
    test_CopyFileW();
    test_CreateFileA();
    test_CreateFileW();
    test_DeleteFileA();
    test_DeleteFileW();
    test_MoveFileA();
    test_MoveFileW();
    test_FindFirstFileA();
    test_FindNextFileA();
    test_FindFirstFileExA();
    test_LockFile();
    test_file_sharing();
    test_offset_in_overlapped_structure();
    test_MapFile();
    test_GetFileType();
    test_async_file_errors();
    test_read_write();
    test_OpenFile();
    test_overlapped();
    test_RemoveDirectory();
    test_ReplaceFileA();
    test_ReplaceFileW();
}
