/*
 * Unit tests for file functions in Wine
 *
 * Copyright (c) 2002 Jakob Eriksson
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"

LPCSTR filename = "testfile.xxx";
LPCSTR sillytext =
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


static void test__hread( void )
{
    HFILE filehandle;
    char buffer[10000];
    long bytes_read;
    long bytes_wanted;
    long i;

    SetFileAttributesA(filename,FILE_ATTRIBUTE_NORMAL); /* be sure to remove stale files */
    DeleteFileA( filename );
    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%ld)\n",filename,GetLastError());
        return;
    }

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains\n" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    filehandle = _lopen( filename, OF_READ );

    ok( HFILE_ERROR != filehandle, "couldn't open file \"%s\" again (err=%ld)\n", filename, GetLastError(  ) );

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

    ok( DeleteFileA( filename ) != 0, "DeleteFile failed (%ld)\n", GetLastError(  ) );
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

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%ld)\n",filename,GetLastError());
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

    ok( DeleteFileA( filename ) != 0, "DeleteFile failed (%ld)\n", GetLastError(  ) );
}


static void test__lclose( void )
{
    HFILE filehandle;

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%ld)\n",filename,GetLastError());
        return;
    }

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains\n" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    ok( DeleteFileA( filename ) != 0, "DeleteFile failed (%ld)\n", GetLastError(  ) );
}


static void test__lcreat( void )
{
    HFILE filehandle;
    char buffer[10000];
    WIN32_FIND_DATAA search_results;
    char slashname[] = "testfi/";
    int err;
    HANDLE find;

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%ld)\n",filename,GetLastError());
        return;
    }

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains\n" );

    ok( 0 == _llseek( filehandle, 0, FILE_BEGIN ), "_llseek complains\n" );

    ok( _hread( filehandle, buffer, strlen( sillytext ) ) ==  lstrlenA( sillytext ), "erratic _hread return value\n" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    ok( INVALID_HANDLE_VALUE != FindFirstFileA( filename, &search_results ), "should be able to find file\n" );

    ok( DeleteFileA(filename) != 0, "DeleteFile failed (%ld)\n", GetLastError());

    filehandle = _lcreat( filename, 1 ); /* readonly */
    ok( HFILE_ERROR != filehandle, "couldn't create file \"%s\" (err=%ld)\n", filename, GetLastError(  ) );

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite shouldn't be able to write never the less\n" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    ok( INVALID_HANDLE_VALUE != FindFirstFileA( filename, &search_results ), "should be able to find file\n" );

    ok( 0 == DeleteFileA( filename ), "shouldn't be able to delete a readonly file\n" );

    ok( SetFileAttributesA(filename, FILE_ATTRIBUTE_NORMAL ) != 0, "couldn't change attributes on file\n" );

    ok( DeleteFileA( filename ) != 0, "now it should be possible to delete the file!\n" );

    filehandle = _lcreat( filename, 2 );
    ok( HFILE_ERROR != filehandle, "couldn't create file \"%s\" (err=%ld)\n", filename, GetLastError(  ) );

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains\n" );

    ok( 0 == _llseek( filehandle, 0, FILE_BEGIN ), "_llseek complains\n" );

    ok( _hread( filehandle, buffer, strlen( sillytext ) ) ==  lstrlenA( sillytext ), "erratic _hread return value\n" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    ok( INVALID_HANDLE_VALUE != FindFirstFileA( filename, &search_results ), "should STILL be able to find file\n" );

    ok( DeleteFileA( filename ) != 0, "DeleteFile failed (%ld)\n", GetLastError(  ) );

    filehandle = _lcreat( filename, 4 ); /* SYSTEM file */
    ok( HFILE_ERROR != filehandle, "couldn't create file \"%s\" (err=%ld)\n", filename, GetLastError(  ) );

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains\n" );

    ok( 0 == _llseek( filehandle, 0, FILE_BEGIN ), "_llseek complains\n" );

    ok( _hread( filehandle, buffer, strlen( sillytext ) ) ==  lstrlenA( sillytext ), "erratic _hread return value\n" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    ok( INVALID_HANDLE_VALUE != FindFirstFileA( filename, &search_results ), "should STILL be able to find file\n" );

    ok( DeleteFileA( filename ) != 0, "DeleteFile failed (%ld)\n", GetLastError(  ) );

    filehandle=_lcreat (slashname, 0); /* illegal name */
    if (HFILE_ERROR==filehandle) {
      err=GetLastError ();
      ok (err==ERROR_INVALID_NAME || err==ERROR_PATH_NOT_FOUND,
          "creating file \"%s\" failed with error %d\n", slashname, err);
    } else { /* only NT succeeds */
      _lclose(filehandle);
      find=FindFirstFileA (slashname, &search_results);
      if (INVALID_HANDLE_VALUE==find)
        ok (0, "file \"%s\" not found\n", slashname);
      else {
        ok (0!=FindClose (find), "FindClose complains (%ld)\n", GetLastError ());
        slashname[strlen(slashname)-1]=0;
        ok (!strcmp (slashname, search_results.cFileName),
            "found unexpected name \"%s\"\n", search_results.cFileName);
        ok (FILE_ATTRIBUTE_ARCHIVE==search_results.dwFileAttributes,
            "attributes of file \"%s\" are 0x%04lx\n", search_results.cFileName,
            search_results.dwFileAttributes);
      }
      ok (0!=DeleteFileA (slashname), "Can't delete \"%s\" (%ld)\n", slashname,
          GetLastError ());
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
        ok (0!=FindClose (find), "FindClose complains (%ld)\n", GetLastError ());
        ok (!strcmp (filename, search_results.cFileName),
            "found unexpected name \"%s\"\n", search_results.cFileName);
        ok (FILE_ATTRIBUTE_ARCHIVE==search_results.dwFileAttributes,
            "attributes of file \"%s\" are 0x%04lx\n", search_results.cFileName,
            search_results.dwFileAttributes);
      }
      ok (0!=DeleteFileA (filename), "Can't delete \"%s\" (%ld)\n", slashname,
          GetLastError ());
    }
}


static void test__llseek( void )
{
    INT i;
    HFILE filehandle;
    char buffer[1];
    long bytes_read;

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%ld)\n",filename,GetLastError());
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

    ok( DeleteFileA( filename ) != 0, "DeleteFile failed (%ld)\n", GetLastError(  ) );
}


static void test__llopen( void )
{
    HFILE filehandle;
    UINT bytes_read;
    char buffer[10000];

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%ld)\n",filename,GetLastError());
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

    ok( DeleteFileA( filename ) != 0, "DeleteFile failed (%ld)\n", GetLastError(  ) );
    /* TODO - add tests for the SHARE modes  -  use two processes to pull this one off */
}


static void test__lread( void )
{
    HFILE filehandle;
    char buffer[10000];
    long bytes_read;
    UINT bytes_wanted;
    UINT i;

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%ld)\n",filename,GetLastError());
        return;
    }

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains\n" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    filehandle = _lopen( filename, OF_READ );

    ok( HFILE_ERROR != filehandle, "couldn't open file \"%s\" again (err=%ld)\n", filename, GetLastError());

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

    ok( DeleteFileA( filename ) != 0, "DeleteFile failed (%ld)\n", GetLastError(  ) );
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

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%ld)\n",filename,GetLastError());
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

    ok( DeleteFileA( filename ) != 0, "DeleteFile failed (%ld)\n", GetLastError(  ) );
}

static void test_CopyFileA(void)
{
    char temp_path[MAX_PATH];
    char source[MAX_PATH], dest[MAX_PATH];
    static const char prefix[] = "pfx";
    DWORD ret;

    ret = GetTempPathA(MAX_PATH, temp_path);
    ok(ret != 0, "GetTempPathA error %ld\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameA(temp_path, prefix, 0, source);
    ok(ret != 0, "GetTempFileNameA error %ld\n", GetLastError());

    ret = GetTempFileNameA(temp_path, prefix, 0, dest);
    ok(ret != 0, "GetTempFileNameA error %ld\n", GetLastError());

    ret = CopyFileA(source, dest, TRUE);
    ok(!ret && GetLastError() == ERROR_FILE_EXISTS,
       "CopyFileA: unexpected error %ld\n", GetLastError());

    ret = CopyFileA(source, dest, FALSE);
    ok(ret,  "CopyFileA: error %ld\n", GetLastError());

    ret = DeleteFileA(source);
    ok(ret, "DeleteFileA: error %ld\n", GetLastError());
    ret = DeleteFileA(dest);
    ok(ret, "DeleteFileA: error %ld\n", GetLastError());
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
    ok(ret != 0, "GetTempPathW error %ld\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameW(temp_path, prefix, 0, source);
    ok(ret != 0, "GetTempFileNameW error %ld\n", GetLastError());

    ret = GetTempFileNameW(temp_path, prefix, 0, dest);
    ok(ret != 0, "GetTempFileNameW error %ld\n", GetLastError());

    ret = CopyFileW(source, dest, TRUE);
    ok(!ret && GetLastError() == ERROR_FILE_EXISTS,
       "CopyFileW: unexpected error %ld\n", GetLastError());

    ret = CopyFileW(source, dest, FALSE);
    ok(ret,  "CopyFileW: error %ld\n", GetLastError());

    ret = DeleteFileW(source);
    ok(ret, "DeleteFileW: error %ld\n", GetLastError());
    ret = DeleteFileW(dest);
    ok(ret, "DeleteFileW: error %ld\n", GetLastError());
}

static void test_CreateFileA(void)
{
    HANDLE hFile;
    char temp_path[MAX_PATH];
    char filename[MAX_PATH];
    static const char prefix[] = "pfx";
    DWORD ret;

    ret = GetTempPathA(MAX_PATH, temp_path);
    ok(ret != 0, "GetTempPathA error %ld\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameA(temp_path, prefix, 0, filename);
    ok(ret != 0, "GetTempFileNameA error %ld\n", GetLastError());

    hFile = CreateFileA(filename, GENERIC_READ, 0, NULL,
                        CREATE_NEW, FILE_FLAG_RANDOM_ACCESS, 0);
    ok(hFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_EXISTS,
        "CREATE_NEW should fail if file exists and last error value should be ERROR_FILE_EXISTS\n");

    ret = DeleteFileA(filename);
    ok(ret, "DeleteFileA: error %ld\n", GetLastError());
}

static void test_CreateFileW(void)
{
    HANDLE hFile;
    WCHAR temp_path[MAX_PATH];
    WCHAR filename[MAX_PATH];
    static const WCHAR prefix[] = {'p','f','x',0};
    DWORD ret;

    ret = GetTempPathW(MAX_PATH, temp_path);
    if (ret==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
        return;
    ok(ret != 0, "GetTempPathW error %ld\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameW(temp_path, prefix, 0, filename);
    ok(ret != 0, "GetTempFileNameW error %ld\n", GetLastError());

    hFile = CreateFileW(filename, GENERIC_READ, 0, NULL,
                        CREATE_NEW, FILE_FLAG_RANDOM_ACCESS, 0);
    ok(hFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_EXISTS,
        "CREATE_NEW should fail if file exists and last error value should be ERROR_FILE_EXISTS\n");

    ret = DeleteFileW(filename);
    ok(ret, "DeleteFileW: error %ld\n", GetLastError());
}


static void test_GetTempFileNameA() {
    UINT result;
    char out[MAX_PATH];
    char *expected = "c:\\windows\\abc2.tmp";

    /* this test may depend on the config file settings */
    result = GetTempFileNameA("C:", "abc", 1, out);
    ok( result != 0, "GetTempFileNameA: error %ld\n", GetLastError() );
    ok( ((out[0] == 'C') && (out[1] == ':')) && (out[2] == '\\'), "GetTempFileNameA: first three characters should be C:\\, string was actually %s\n", out );

    result = GetTempFileNameA("c:\\windows\\", "abc", 2, out);
    ok( result != 0, "GetTempFileNameA: error %ld\n", GetLastError() );
    ok( lstrcmpiA( out, expected ) == 0, "GetTempFileNameA: Unexpected output \"%s\" vs \"%s\"\n", out, expected );
}


static void test_DeleteFileA( void )
{
    BOOL ret;

    ret = DeleteFileA(NULL);
    ok(!ret && (GetLastError() == ERROR_INVALID_PARAMETER ||
                GetLastError() == ERROR_PATH_NOT_FOUND),
       "DeleteFileA(NULL) returned ret=%d error=%ld\n",ret,GetLastError());

    ret = DeleteFileA("");
    ok(!ret && (GetLastError() == ERROR_PATH_NOT_FOUND ||
                GetLastError() == ERROR_BAD_PATHNAME),
       "DeleteFileA(\"\") returned ret=%d error=%ld\n",ret,GetLastError());
}

static void test_DeleteFileW( void )
{
    BOOL ret;
    WCHAR emptyW[]={'\0'};

    ret = DeleteFileW(NULL);
    if (ret==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
        return;
    ok(!ret && GetLastError() == ERROR_PATH_NOT_FOUND,
       "DeleteFileW(NULL) returned ret=%d error=%ld\n",ret,GetLastError());

    ret = DeleteFileW(emptyW);
    ok(!ret && GetLastError() == ERROR_PATH_NOT_FOUND,
       "DeleteFileW(\"\") returned ret=%d error=%ld\n",ret,GetLastError());
}

#define IsDotDir(x)     ((x[0] == '.') && ((x[1] == 0) || ((x[1] == '.') && (x[2] == 0))))

static void test_MoveFileA(void)
{
    char tempdir[MAX_PATH];
    char source[MAX_PATH], dest[MAX_PATH];
    static const char prefix[] = "pfx";
    DWORD ret;

    ret = GetTempPathA(MAX_PATH, tempdir);
    ok(ret != 0, "GetTempPathA error %ld\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameA(tempdir, prefix, 0, source);
    ok(ret != 0, "GetTempFileNameA error %ld\n", GetLastError());

    ret = GetTempFileNameA(tempdir, prefix, 0, dest);
    ok(ret != 0, "GetTempFileNameA error %ld\n", GetLastError());

    ret = MoveFileA(source, dest);
    ok(!ret && GetLastError() == ERROR_ALREADY_EXISTS,
       "MoveFileA: unexpected error %ld\n", GetLastError());

    ret = DeleteFileA(dest);
    ok(ret, "DeleteFileA: error %ld\n", GetLastError());

    ret = MoveFileA(source, dest);
    ok(ret, "MoveFileA: failed, error %ld\n", GetLastError());

    lstrcatA(tempdir, "Remove Me");
    ret = CreateDirectoryA(tempdir, NULL);
    ok(ret == TRUE, "CreateDirectoryA failed\n");

    lstrcpyA(source, dest);
    lstrcpyA(dest, tempdir);
    lstrcatA(dest, "\\wild?.*");
    ret = MoveFileA(source, dest);
    todo_wine {
      ok(!ret, "MoveFileA: shouldn't move to wildcard file\n");
      ok(GetLastError() == ERROR_INVALID_NAME,
              "MoveFileA: with wildcards, unexpected error %ld\n", GetLastError());
#if 0
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
#endif
      ret = DeleteFileA(source);
      ok(ret, "DeleteFileA: error %ld\n", GetLastError());
      ret = DeleteFileA(dest);
      ok(!ret, "DeleteFileA: error %ld\n", GetLastError());
    }

    ret = RemoveDirectoryA(tempdir);
    ok(ret, "DeleteDirectoryA: error %ld\n", GetLastError());
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
    ok(ret != 0, "GetTempPathW error %ld\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameW(temp_path, prefix, 0, source);
    ok(ret != 0, "GetTempFileNameW error %ld\n", GetLastError());

    ret = GetTempFileNameW(temp_path, prefix, 0, dest);
    ok(ret != 0, "GetTempFileNameW error %ld\n", GetLastError());

    ret = MoveFileW(source, dest);
    ok(!ret && GetLastError() == ERROR_ALREADY_EXISTS,
       "CopyFileW: unexpected error %ld\n", GetLastError());

    ret = DeleteFileW(source);
    ok(ret, "DeleteFileW: error %ld\n", GetLastError());
    ret = DeleteFileW(dest);
    ok(ret, "DeleteFileW: error %ld\n", GetLastError());
}

#define PATTERN_OFFSET 0x10

static void test_offset_in_overlapped_structure(void)
{
    HANDLE hFile;
    OVERLAPPED ov;
    DWORD done;
    BOOL rc;
    BYTE buf[256], pattern[] = "TeSt";
    UINT i;
    char temp_path[MAX_PATH], temp_fname[MAX_PATH];

    ok(GetTempPathA(MAX_PATH, temp_path) != 0, "GetTempPathA error %ld\n", GetLastError());
    ok(GetTempFileNameA(temp_path, "pfx", 0, temp_fname) != 0, "GetTempFileNameA error %ld\n", GetLastError());

    /*** Write File *****************************************************/

    hFile = CreateFileA(temp_fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA error %ld\n", GetLastError());

    for(i = 0; i < sizeof(buf); i++) buf[i] = i;
    ok(WriteFile(hFile, buf, sizeof(buf), &done, NULL), "WriteFile error %ld\n", GetLastError());
    ok(done == sizeof(buf), "expected number of bytes written %lu\n", done);

    memset(&ov, 0, sizeof(ov));
    ov.Offset = PATTERN_OFFSET;
    ov.OffsetHigh = 0;
    rc=WriteFile(hFile, pattern, sizeof(pattern), &done, &ov);
    /* Win 9x does not support the overlapped I/O on files */
    if (rc || GetLastError()!=ERROR_INVALID_PARAMETER) {
        ok(rc, "WriteFile error %ld\n", GetLastError());
        ok(done == sizeof(pattern), "expected number of bytes written %lu\n", done);
        trace("Current offset = %04lx\n", SetFilePointer(hFile, 0, NULL, FILE_CURRENT));
        ok(SetFilePointer(hFile, 0, NULL, FILE_CURRENT) == (PATTERN_OFFSET + sizeof(pattern)),
           "expected file offset %d\n", PATTERN_OFFSET + sizeof(pattern));

        ov.Offset = sizeof(buf) * 2;
        ov.OffsetHigh = 0;
        ok(WriteFile(hFile, pattern, sizeof(pattern), &done, &ov), "WriteFile error %ld\n", GetLastError());
        ok(done == sizeof(pattern), "expected number of bytes written %lu\n", done);
        /*trace("Current offset = %04lx\n", SetFilePointer(hFile, 0, NULL, FILE_CURRENT));*/
        ok(SetFilePointer(hFile, 0, NULL, FILE_CURRENT) == (sizeof(buf) * 2 + sizeof(pattern)),
           "expected file offset %d\n", sizeof(buf) * 2 + sizeof(pattern));
    }

    CloseHandle(hFile);

    /*** Read File *****************************************************/

    hFile = CreateFileA(temp_fname, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA error %ld\n", GetLastError());

    memset(buf, 0, sizeof(buf));
    memset(&ov, 0, sizeof(ov));
    ov.Offset = PATTERN_OFFSET;
    ov.OffsetHigh = 0;
    rc=ReadFile(hFile, buf, sizeof(pattern), &done, &ov);
    /* Win 9x does not support the overlapped I/O on files */
    if (rc || GetLastError()!=ERROR_INVALID_PARAMETER) {
        ok(rc, "ReadFile error %ld\n", GetLastError());
        ok(done == sizeof(pattern), "expected number of bytes read %lu\n", done);
        trace("Current offset = %04lx\n", SetFilePointer(hFile, 0, NULL, FILE_CURRENT));
        ok(SetFilePointer(hFile, 0, NULL, FILE_CURRENT) == (PATTERN_OFFSET + sizeof(pattern)),
           "expected file offset %d\n", PATTERN_OFFSET + sizeof(pattern));
        ok(!memcmp(buf, pattern, sizeof(pattern)), "pattern match failed\n");
    }

    CloseHandle(hFile);

    ok(DeleteFileA(temp_fname), "DeleteFileA error %ld\n", GetLastError());
}

static void test_LockFile(void)
{
    HANDLE handle;
    DWORD written;
    OVERLAPPED overlapped;

    handle = CreateFileA( filename, GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                          CREATE_ALWAYS, 0, 0 );
    if (handle == INVALID_HANDLE_VALUE)
    {
        ok(0,"couldn't create file \"%s\" (err=%ld)\n",filename,GetLastError());
        return;
    }
    ok( WriteFile( handle, sillytext, strlen(sillytext), &written, NULL ), "write failed\n" );

    ok( LockFile( handle, 0, 0, 0, 0 ), "LockFile failed\n" );
    ok( UnlockFile( handle, 0, 0, 0, 0 ), "UnlockFile failed\n" );
    ok( !UnlockFile( handle, 0, 0, 0, 0 ), "UnlockFile succeeded\n" );

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

    overlapped.Offset = 100;
    overlapped.OffsetHigh = 0;
    overlapped.hEvent = 0;
    ok( LockFileEx( handle, 0, 0, 100, 0, &overlapped ), "LockFileEx 100,100 failed\n" );
    /* overlapping shared locks are OK */
    overlapped.Offset = 150;
    ok( LockFileEx( handle, 0, 0, 100, 0, &overlapped ), "LockFileEx 150,100 failed\n" );
    /* but exclusive is not */
    ok( !LockFileEx( handle, LOCKFILE_EXCLUSIVE_LOCK|LOCKFILE_FAIL_IMMEDIATELY, 0, 50, 0, &overlapped ),
        "LockFileEx exclusive 150,50 succeeded\n" );
    ok( UnlockFileEx( handle, 0, 100, 0, &overlapped ), "UnlockFileEx 150,100 failed\n" );
    ok( !UnlockFileEx( handle, 0, 100, 0, &overlapped ), "UnlockFileEx 150,100 again succeeded\n" );
    overlapped.Offset = 100;
    ok( UnlockFileEx( handle, 0, 100, 0, &overlapped ), "UnlockFileEx 100,100 failed\n" );
    ok( !UnlockFileEx( handle, 0, 100, 0, &overlapped ), "UnlockFileEx 100,100 again succeeded\n" );

    ok( LockFile( handle, 0, 0x10000000, 0, 0xf0000000 ), "LockFile failed\n" );
    ok( !LockFile( handle, ~0, ~0, 1, 0 ), "LockFile ~0,1 succeeded\n" );
    ok( !LockFile( handle, 0, 0x20000000, 20, 0 ), "LockFile 0x20000000,20 succeeded\n" );
    ok( UnlockFile( handle, 0, 0x10000000, 0, 0xf0000000 ), "UnlockFile failed\n" );

    /* wrap-around lock should not do anything */
    /* (but still succeeds on NT4 so we don't check result) */
    LockFile( handle, 0, 0x10000000, 0, 0xf0000001 );
    ok( LockFile( handle, ~0, ~0, 1, 0 ), "LockFile ~0,1 failed\n" );
    ok( UnlockFile( handle, ~0, ~0, 1, 0 ), "Unlockfile ~0,1 failed\n" );

    /* zero-byte lock */
    ok( LockFile( handle, 100, 0, 0, 0 ), "LockFile 100,0 failed\n" );
    ok( !LockFile( handle, 98, 0, 4, 0 ), "LockFile 98,4 succeeded\n" );
    ok( LockFile( handle, 90, 0, 10, 0 ), "LockFile 90,10 failed\n" );
    ok( LockFile( handle, 100, 0, 10, 0 ), "LockFile 100,10 failed\n" );
    ok( UnlockFile( handle, 90, 0, 10, 0 ), "UnlockFile 90,10 failed\n" );
    ok( UnlockFile( handle, 100, 0, 10, 0 ), "UnlockFile 100,10 failed\n" );
    ok( UnlockFile( handle, 100, 0, 0, 0 ), "UnlockFile 100,0 failed\n" );

    CloseHandle( handle );
    DeleteFileA( filename );
}

static void test_FindFirstFileA()
{
    HANDLE handle;
    WIN32_FIND_DATAA search_results;
    int err;

    handle = FindFirstFileA("C:\\",&search_results);
    err = GetLastError();
    ok ( handle == INVALID_HANDLE_VALUE , "FindFirstFile on Root directory should Fail\n");
    if (handle == INVALID_HANDLE_VALUE)
      ok ( err == ERROR_FILE_NOT_FOUND, "Bad Error number %d\n", err);
    handle = FindFirstFileA("C:\\*",&search_results);
    ok ( handle != INVALID_HANDLE_VALUE, "FindFirstFile on C:\\* should succeed\n" );
    ok ( FindClose(handle) == TRUE, "Failed to close handle\n");
}

static void test_FindNextFileA()
{
    HANDLE handle;
    WIN32_FIND_DATAA search_results;
    int err;

    handle = FindFirstFileA("C:\\*",&search_results);
    ok ( handle != INVALID_HANDLE_VALUE, "FindFirstFile on C:\\* should succeed\n" );
    while (FindNextFile(handle, &search_results))
    {
        /* get to the end of the files */
    }
    ok ( FindClose(handle) == TRUE, "Failed to close handle\n");
    err = GetLastError();
    ok ( err == ERROR_NO_MORE_FILES, "GetLastError should return ERROR_NO_MORE_FILES\n");
}

static void test_MapFile()
{
    HANDLE handle, hmap;

    /* be sure to remove stale files */
    SetFileAttributesA(filename,FILE_ATTRIBUTE_NORMAL);
    DeleteFile(filename);
    handle = CreateFile( filename, GENERIC_READ|GENERIC_WRITE, 0, 0,
                         CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    ok( handle != INVALID_HANDLE_VALUE, "couldn't create test file\n");

    hmap = CreateFileMapping( handle, NULL, PAGE_READWRITE, 0, 0, NULL );
    ok( hmap == NULL, "mapped zero size file\n");
    ok( GetLastError() == ERROR_FILE_INVALID, "not ERROR_FILE_INVALID\n");

    hmap = CreateFileMapping( handle, NULL, PAGE_READWRITE, 0x1000, 0, NULL );
    ok( hmap == NULL, "mapping should fail\n");
    /* GetLastError() varies between win9x and WinNT */

    hmap = CreateFileMapping( handle, NULL, PAGE_READWRITE, 0x1000, 0x10000, NULL );
    ok( hmap == NULL, "mapping should fail\n");
    /* GetLastError() varies between win9x and WinNT */

    hmap = CreateFileMapping( handle, NULL, PAGE_READWRITE, 0, 0x1000, NULL );
    ok( hmap != NULL, "mapping should succeed\n");

    ok( CloseHandle( hmap ), "can't close mapping handle\n");
    ok( CloseHandle( handle ), "can't close file handle\n");
    ok( DeleteFileA( filename ), "DeleteFile failed after map\n" );
}

START_TEST(file)
{
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
    test_LockFile();
    test_offset_in_overlapped_structure();
    test_MapFile();
}
