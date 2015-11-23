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

/* ReplaceFile requires Windows 2000 or newer */
#define _WIN32_WINNT 0x0601

#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <wine/test.h>
#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <wine/winternl.h>
#include <winnls.h>
#include <fileapi.h>

#undef DeleteFile  /* needed for FILE_DISPOSITION_INFO */

static HANDLE (WINAPI *pFindFirstFileExA)(LPCSTR,FINDEX_INFO_LEVELS,LPVOID,FINDEX_SEARCH_OPS,LPVOID,DWORD);
static BOOL (WINAPI *pReplaceFileA)(LPCSTR, LPCSTR, LPCSTR, DWORD, LPVOID, LPVOID);
static BOOL (WINAPI *pReplaceFileW)(LPCWSTR, LPCWSTR, LPCWSTR, DWORD, LPVOID, LPVOID);
static UINT (WINAPI *pGetSystemWindowsDirectoryA)(LPSTR, UINT);
static BOOL (WINAPI *pGetVolumeNameForVolumeMountPointA)(LPCSTR, LPSTR, DWORD);
static DWORD (WINAPI *pQueueUserAPC)(PAPCFUNC pfnAPC, HANDLE hThread, ULONG_PTR dwData);
static BOOL (WINAPI *pGetFileInformationByHandleEx)(HANDLE, FILE_INFO_BY_HANDLE_CLASS, LPVOID, DWORD);
static HANDLE (WINAPI *pOpenFileById)(HANDLE, LPFILE_ID_DESCRIPTOR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD);
static BOOL (WINAPI *pSetFileValidData)(HANDLE, LONGLONG);
static HRESULT (WINAPI *pCopyFile2)(PCWSTR,PCWSTR,COPYFILE2_EXTENDED_PARAMETERS*);
static HANDLE (WINAPI *pCreateFile2)(LPCWSTR, DWORD, DWORD, DWORD, CREATEFILE2_EXTENDED_PARAMETERS*);
static DWORD (WINAPI *pGetFinalPathNameByHandleA)(HANDLE, LPSTR, DWORD, DWORD);
static DWORD (WINAPI *pGetFinalPathNameByHandleW)(HANDLE, LPWSTR, DWORD, DWORD);
static NTSTATUS (WINAPI *pNtCreateFile)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK,
                                        PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG);
static BOOL (WINAPI *pRtlDosPathNameToNtPathName_U)(LPCWSTR, PUNICODE_STRING, PWSTR*, CURDIR*);
static NTSTATUS (WINAPI *pRtlAnsiStringToUnicodeString)(PUNICODE_STRING, PCANSI_STRING, BOOLEAN);
static BOOL (WINAPI *pSetFileInformationByHandle)(HANDLE, FILE_INFO_BY_HANDLE_CLASS, void*, DWORD);

static const char filename[] = "testfile.xxx";
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

struct test_list {
    const char *file;           /* file string to test */
    const DWORD err;            /* Win NT and further error code */
    const LONG err2;            /* Win 9x & ME error code  or -1 */
    const DWORD options;        /* option flag to use for open */
    const BOOL todo_flag;       /* todo_wine indicator */
} ;

static void InitFunctionPointers(void)
{
    HMODULE hntdll = GetModuleHandleA("ntdll");
    HMODULE hkernel32 = GetModuleHandleA("kernel32");

    pNtCreateFile = (void *)GetProcAddress(hntdll, "NtCreateFile");
    pRtlDosPathNameToNtPathName_U = (void *)GetProcAddress(hntdll, "RtlDosPathNameToNtPathName_U");
    pRtlAnsiStringToUnicodeString = (void *)GetProcAddress(hntdll, "RtlAnsiStringToUnicodeString");

    pFindFirstFileExA=(void*)GetProcAddress(hkernel32, "FindFirstFileExA");
    pReplaceFileA=(void*)GetProcAddress(hkernel32, "ReplaceFileA");
    pReplaceFileW=(void*)GetProcAddress(hkernel32, "ReplaceFileW");
    pGetSystemWindowsDirectoryA=(void*)GetProcAddress(hkernel32, "GetSystemWindowsDirectoryA");
    pGetVolumeNameForVolumeMountPointA = (void *) GetProcAddress(hkernel32, "GetVolumeNameForVolumeMountPointA");
    pQueueUserAPC = (void *) GetProcAddress(hkernel32, "QueueUserAPC");
    pGetFileInformationByHandleEx = (void *) GetProcAddress(hkernel32, "GetFileInformationByHandleEx");
    pOpenFileById = (void *) GetProcAddress(hkernel32, "OpenFileById");
    pSetFileValidData = (void *) GetProcAddress(hkernel32, "SetFileValidData");
    pCopyFile2 = (void *) GetProcAddress(hkernel32, "CopyFile2");
    pCreateFile2 = (void *) GetProcAddress(hkernel32, "CreateFile2");
    pGetFinalPathNameByHandleA = (void *) GetProcAddress(hkernel32, "GetFinalPathNameByHandleA");
    pGetFinalPathNameByHandleW = (void *) GetProcAddress(hkernel32, "GetFinalPathNameByHandleW");
    pSetFileInformationByHandle = (void *) GetProcAddress(hkernel32, "SetFileInformationByHandle");
}

static void test__hread( void )
{
    HFILE filehandle;
    char buffer[10000];
    LONG bytes_read;
    LONG bytes_wanted;
    LONG i;
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
    LONG bytes_read;
    LONG bytes_written;
    ULONG blocks;
    LONG i;
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
        for (i = 0; i < (LONG)sizeof( buffer ); i++)
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
    ok( NULL != contents, "LocalLock whines\n" );

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

    LocalFree( contents );
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

/* helper function for test__lcreat */
static void get_nt_pathW( const char *name, UNICODE_STRING *nameW )
{
    UNICODE_STRING strW;
    ANSI_STRING str;
    NTSTATUS status;
    BOOLEAN ret;
    RtlInitAnsiString( &str, name );

    status = pRtlAnsiStringToUnicodeString( &strW, &str, TRUE );
    ok( !status, "RtlAnsiStringToUnicodeString failed with %08x\n", status );

    ret = pRtlDosPathNameToNtPathName_U( strW.Buffer, nameW, NULL, NULL );
    ok( ret, "RtlDosPathNameToNtPathName_U failed\n" );

    RtlFreeUnicodeString( &strW );
}

static void test__lcreat( void )
{
    UNICODE_STRING filenameW;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    HFILE filehandle;
    char buffer[10000];
    WIN32_FIND_DATAA search_results;
    char slashname[] = "testfi/";
    int err;
    HANDLE find, file;
    NTSTATUS status;
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

    find = FindFirstFileA( filename, &search_results );
    ok( INVALID_HANDLE_VALUE != find, "should be able to find file\n" );
    FindClose( find );

    ret = DeleteFileA(filename);
    ok( ret != 0, "DeleteFile failed (%d)\n", GetLastError());

    filehandle = _lcreat( filename, 1 ); /* readonly */
    ok( HFILE_ERROR != filehandle, "couldn't create file \"%s\" (err=%d)\n", filename, GetLastError(  ) );

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite shouldn't be able to write never the less\n" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    find = FindFirstFileA( filename, &search_results );
    ok( INVALID_HANDLE_VALUE != find, "should be able to find file\n" );
    FindClose( find );

    SetLastError( 0xdeadbeef );
    ok( 0 == DeleteFileA( filename ), "shouldn't be able to delete a readonly file\n" );
    ok( GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError() );

    ok( SetFileAttributesA(filename, FILE_ATTRIBUTE_NORMAL ) != 0, "couldn't change attributes on file\n" );

    ok( DeleteFileA( filename ) != 0, "now it should be possible to delete the file!\n" );

    filehandle = _lcreat( filename, 1 ); /* readonly */
    ok( HFILE_ERROR != filehandle, "couldn't create file \"%s\" (err=%d)\n", filename, GetLastError() );
    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen(sillytext) ),
        "_hwrite shouldn't be able to write never the less\n" );
    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    find = FindFirstFileA( filename, &search_results );
    ok( INVALID_HANDLE_VALUE != find, "should be able to find file\n" );
    FindClose( find );

    get_nt_pathW( filename, &filenameW );
    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &filenameW;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtCreateFile( &file, GENERIC_READ | GENERIC_WRITE | DELETE, &attr, &io, NULL, 0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN, FILE_DELETE_ON_CLOSE | FILE_NON_DIRECTORY_FILE, NULL, 0 );
    ok( status == STATUS_ACCESS_DENIED, "expected STATUS_ACCESS_DENIED, got %08x\n", status );
    ok( GetFileAttributesA( filename ) != INVALID_FILE_ATTRIBUTES, "file was deleted\n" );

    status = NtCreateFile( &file, DELETE, &attr, &io, NULL, 0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN, FILE_DELETE_ON_CLOSE | FILE_NON_DIRECTORY_FILE, NULL, 0 );
    ok( status == STATUS_CANNOT_DELETE, "expected STATUS_CANNOT_DELETE, got %08x\n", status );

    status = NtCreateFile( &file, DELETE, &attr, &io, NULL, 0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN, FILE_DELETE_ON_CLOSE | FILE_DIRECTORY_FILE, NULL, 0 );
    ok( status == STATUS_NOT_A_DIRECTORY, "expected STATUS_NOT_A_DIRECTORY, got %08x\n", status );

    status = NtCreateFile( &file, DELETE, &attr, &io, NULL, 0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN_IF, FILE_DELETE_ON_CLOSE | FILE_NON_DIRECTORY_FILE, NULL, 0 );
    todo_wine
    ok( status == STATUS_CANNOT_DELETE, "expected STATUS_CANNOT_DELETE, got %08x\n", status );
    if (!status) CloseHandle( file );

    RtlFreeUnicodeString( &filenameW );

    todo_wine
    ok( GetFileAttributesA( filename ) != INVALID_FILE_ATTRIBUTES, "file was deleted\n" );
    todo_wine
    ok( SetFileAttributesA(filename, FILE_ATTRIBUTE_NORMAL ) != 0, "couldn't change attributes on file\n" );
    todo_wine
    ok( DeleteFileA( filename ) != 0, "now it should be possible to delete the file\n" );

    filehandle = _lcreat( filename, 2 );
    ok( HFILE_ERROR != filehandle, "couldn't create file \"%s\" (err=%d)\n", filename, GetLastError(  ) );

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains\n" );

    ok( 0 == _llseek( filehandle, 0, FILE_BEGIN ), "_llseek complains\n" );

    ok( _hread( filehandle, buffer, strlen( sillytext ) ) ==  lstrlenA( sillytext ), "erratic _hread return value\n" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    find = FindFirstFileA( filename, &search_results );
    ok( INVALID_HANDLE_VALUE != find, "should STILL be able to find file\n" );
    FindClose( find );

    ret = DeleteFileA( filename );
    ok( ret, "DeleteFile failed (%d)\n", GetLastError(  ) );

    filehandle = _lcreat( filename, 4 ); /* SYSTEM file */
    ok( HFILE_ERROR != filehandle, "couldn't create file \"%s\" (err=%d)\n", filename, GetLastError(  ) );

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains\n" );

    ok( 0 == _llseek( filehandle, 0, FILE_BEGIN ), "_llseek complains\n" );

    ok( _hread( filehandle, buffer, strlen( sillytext ) ) ==  lstrlenA( sillytext ), "erratic _hread return value\n" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains\n" );

    find = FindFirstFileA( filename, &search_results );
    ok( INVALID_HANDLE_VALUE != find, "should STILL be able to find file\n" );
    FindClose( find );

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
    LONG bytes_read;
    BOOL ret;

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%d)\n",filename,GetLastError());
        return;
    }

    for (i = 0; i < 400; i++)
    {
        ok( _hwrite( filehandle, sillytext, strlen( sillytext ) ) != -1, "_hwrite complains\n" );
    }
    ok( _llseek( filehandle, 400 * strlen( sillytext ), FILE_CURRENT ) != -1, "should be able to seek\n" );
    ok( _llseek( filehandle, 27 + 35 * strlen( sillytext ), FILE_BEGIN ) != -1, "should be able to seek\n" );

    bytes_read = _hread( filehandle, buffer, 1);
    ok( 1 == bytes_read, "file read size error\n" );
    ok( buffer[0] == sillytext[27], "_llseek error, it got lost seeking\n" );
    ok( _llseek( filehandle, -400 * (LONG)strlen( sillytext ), FILE_END ) != -1, "should be able to seek\n" );

    bytes_read = _hread( filehandle, buffer, 1);
    ok( 1 == bytes_read, "file read size error\n" );
    ok( buffer[0] == sillytext[0], "_llseek error, it got lost seeking\n" );
    ok( _llseek( filehandle, 1000000, FILE_END ) != -1, "should be able to seek past file; poor, poor Windows programmers\n" );
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
    UINT bytes_read;
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
    UINT bytes_read;
    UINT bytes_written;
    UINT blocks;
    INT i;
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
        for (i = 0; i < (INT)sizeof( buffer ); i++)
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
    ok( NULL != contents, "LocalLock whines\n" );

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

    LocalFree( contents );
}

static void test_CopyFileA(void)
{
    char temp_path[MAX_PATH];
    char source[MAX_PATH], dest[MAX_PATH];
    static const char prefix[] = "pfx";
    HANDLE hfile;
    HANDLE hmapfile;
    FILETIME ft1, ft2;
    char buf[10];
    DWORD ret;
    BOOL retok;

    ret = GetTempPathA(MAX_PATH, temp_path);
    ok(ret != 0, "GetTempPathA error %d\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameA(temp_path, prefix, 0, source);
    ok(ret != 0, "GetTempFileNameA error %d\n", GetLastError());

    ret = MoveFileA(source, source);
    todo_wine ok(ret, "MoveFileA: failed, error %d\n", GetLastError());

    /* copying a file to itself must fail */
    retok = CopyFileA(source, source, FALSE);
    ok( !retok && (GetLastError() == ERROR_SHARING_VIOLATION || broken(GetLastError() == ERROR_FILE_EXISTS) /* Win 9x */),
        "copying a file to itself didn't fail (ret=%d, err=%d)\n", retok, GetLastError());

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

    /* copying from a read-locked source fails */
    hfile = CreateFileA(source, GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open source file, error %d\n", GetLastError());
    retok = CopyFileA(source, dest, FALSE);
    ok(!retok && GetLastError() == ERROR_SHARING_VIOLATION,
        "copying from a read-locked file succeeded when it shouldn't have\n");
    /* in addition, the source is opened before the destination */
    retok = CopyFileA("25f99d3b-4ba4-4f66-88f5-2906886993cc", dest, FALSE);
    ok(!retok && GetLastError() == ERROR_FILE_NOT_FOUND,
        "copying from a file that doesn't exist failed in an unexpected way (ret=%d, err=%d)\n", retok, GetLastError());
    CloseHandle(hfile);

    /* copying from a r+w opened, r shared source succeeds */
    hfile = CreateFileA(source, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open source file, error %d\n", GetLastError());
    retok = CopyFileA(source, dest, FALSE);
    ok(retok,
        "copying from an r+w opened and r shared file failed (ret=%d, err=%d)\n", retok, GetLastError());
    CloseHandle(hfile);

    /* copying from a delete-locked source mostly succeeds */
    hfile = CreateFileA(source, DELETE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open source file, error %d\n", GetLastError());
    retok = CopyFileA(source, dest, FALSE);
    ok(retok || broken(!retok && GetLastError() == ERROR_SHARING_VIOLATION) /* NT, 2000, XP */,
        "copying from a delete-locked file failed (ret=%d, err=%d)\n", retok, GetLastError());
    CloseHandle(hfile);

    /* copying to a write-locked destination fails */
    hfile = CreateFileA(dest, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open destination file, error %d\n", GetLastError());
    retok = CopyFileA(source, dest, FALSE);
    ok(!retok && GetLastError() == ERROR_SHARING_VIOLATION,
        "copying to a write-locked file didn't fail (ret=%d, err=%d)\n", retok, GetLastError());
    CloseHandle(hfile);

    /* copying to a r+w opened, w shared destination mostly succeeds */
    hfile = CreateFileA(dest, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open destination file, error %d\n", GetLastError());
    retok = CopyFileA(source, dest, FALSE);
    ok(retok || broken(!retok && GetLastError() == ERROR_SHARING_VIOLATION) /* Win 9x */,
        "copying to a r+w opened and w shared file failed (ret=%d, err=%d)\n", retok, GetLastError());
    CloseHandle(hfile);

    /* copying to a delete-locked destination fails, even when the destination is delete-shared */
    hfile = CreateFileA(dest, DELETE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE || broken(GetLastError() == ERROR_INVALID_PARAMETER) /* Win 9x */,
        "failed to open destination file, error %d\n", GetLastError());
    if (hfile != INVALID_HANDLE_VALUE)
    {
        retok = CopyFileA(source, dest, FALSE);
        ok(!retok && GetLastError() == ERROR_SHARING_VIOLATION,
            "copying to a delete-locked shared file didn't fail (ret=%d, err=%d)\n", retok, GetLastError());
        CloseHandle(hfile);
    }

    /* copy to a file that's opened the way Wine opens the source */
    hfile = CreateFileA(dest, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open destination file, error %d\n", GetLastError());
    retok = CopyFileA(source, dest, FALSE);
    ok(retok || broken(GetLastError() == ERROR_SHARING_VIOLATION) /* Win 9x */,
        "copying to a file opened the way Wine opens the source failed (ret=%d, err=%d)\n", retok, GetLastError());
    CloseHandle(hfile);

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

    /* check error on copying over a mapped file that was opened with FILE_SHARE_READ */
    hmapfile = CreateFileMappingW(hfile, NULL, PAGE_READONLY | SEC_COMMIT, 0, 0, NULL);
    ok(hmapfile != NULL, "CreateFileMapping: error %d\n", GetLastError());

    ret = CopyFileA(source, dest, FALSE);
    ok(!ret && GetLastError() == ERROR_SHARING_VIOLATION,
       "CopyFileA with mapped dest file: expected ERROR_SHARING_VIOLATION, got %d\n", GetLastError());

    CloseHandle(hmapfile);
    CloseHandle(hfile);

    hfile = CreateFileA(dest, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open destination file\n");

    /* check error on copying over a mapped file that was opened with FILE_SHARE_WRITE */
    hmapfile = CreateFileMappingW(hfile, NULL, PAGE_READONLY | SEC_COMMIT, 0, 0, NULL);
    ok(hmapfile != NULL, "CreateFileMapping: error %d\n", GetLastError());

    ret = CopyFileA(source, dest, FALSE);
    ok(!ret, "CopyFileA: expected failure\n");
    ok(GetLastError() == ERROR_USER_MAPPED_FILE ||
       broken(GetLastError() == ERROR_SHARING_VIOLATION), /* Win9x */
       "CopyFileA with mapped dest file: expected ERROR_USER_MAPPED_FILE, got %d\n", GetLastError());

    CloseHandle(hmapfile);
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
    if (ret == 0 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("GetTempPathW is not available\n");
        return;
    }
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

static void test_CopyFile2(void)
{
    static const WCHAR doesntexistW[] = {'d','o','e','s','n','t','e','x','i','s','t',0};
    static const WCHAR prefix[] = {'p','f','x',0};
    WCHAR source[MAX_PATH], dest[MAX_PATH], temp_path[MAX_PATH];
    COPYFILE2_EXTENDED_PARAMETERS params;
    HANDLE hfile, hmapfile;
    FILETIME ft1, ft2;
    DWORD ret, len;
    char buf[10];
    HRESULT hr;

    if (!pCopyFile2)
    {
        skip("CopyFile2 is not available\n");
        return;
    }

    ret = GetTempPathW(MAX_PATH, temp_path);
    ok(ret != 0, "GetTempPathW error %d\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameW(temp_path, prefix, 0, source);
    ok(ret != 0, "GetTempFileNameW error %d\n", GetLastError());

    ret = GetTempFileNameW(temp_path, prefix, 0, dest);
    ok(ret != 0, "GetTempFileNameW error %d\n", GetLastError());

    /* fail if exists */
    memset(&params, 0, sizeof(params));
    params.dwSize = sizeof(params);
    params.dwCopyFlags = COPY_FILE_FAIL_IF_EXISTS;

    SetLastError(0xdeadbeef);
    hr = pCopyFile2(source, dest, &params);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS), "CopyFile2: unexpected error 0x%08x\n", hr);
    ok(GetLastError() == ERROR_FILE_EXISTS, "CopyFile2: last error %d\n", GetLastError());

    /* don't fail if exists */
    params.dwSize = sizeof(params);
    params.dwCopyFlags = 0;

    hr = pCopyFile2(source, dest, &params);
    ok(hr == S_OK, "CopyFile2: error 0x%08x\n", hr);

    /* copying a file to itself must fail */
    params.dwSize = sizeof(params);
    params.dwCopyFlags = 0;

    SetLastError(0xdeadbeef);
    hr = pCopyFile2(source, source, &params);
    ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "CopyFile2: copying a file to itself didn't fail, 0x%08x\n", hr);
    ok(GetLastError() == ERROR_SHARING_VIOLATION, "CopyFile2: last error %d\n", GetLastError());

    /* make the source have not zero size */
    hfile = CreateFileW(source, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open source file\n");
    ret = WriteFile(hfile, prefix, sizeof(prefix), &len, NULL );
    ok(ret && len == sizeof(prefix), "WriteFile error %d\n", GetLastError());
    ok(GetFileSize(hfile, NULL) == sizeof(prefix), "source file has wrong size\n");

    /* get the file time and change it to prove the difference */
    ret = GetFileTime(hfile, NULL, NULL, &ft1);
    ok(ret, "GetFileTime error %d\n", GetLastError());
    ft1.dwLowDateTime -= 600000000; /* 60 second */
    ret = SetFileTime(hfile, NULL, NULL, &ft1);
    ok(ret, "SetFileTime error %d\n", GetLastError());
    GetFileTime(hfile, NULL, NULL, &ft1);  /* get the actual time back */
    CloseHandle(hfile);

    ret = GetTempFileNameW(temp_path, prefix, 0, dest);
    ok(ret != 0, "GetTempFileNameA error %d\n", GetLastError());

    params.dwSize = sizeof(params);
    params.dwCopyFlags = COPY_FILE_FAIL_IF_EXISTS;

    SetLastError(0xdeadbeef);
    hr = pCopyFile2(source, dest, &params);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS), "CopyFile2: unexpected error 0x%08x\n", hr);
    ok(GetLastError() == ERROR_FILE_EXISTS, "CopyFile2: last error %d\n", GetLastError());

    params.dwSize = sizeof(params);
    params.dwCopyFlags = 0;
    hr = pCopyFile2(source, dest, &params);
    ok(ret, "CopyFile2: error 0x%08x\n", hr);

    /* copying from a read-locked source fails */
    hfile = CreateFileW(source, GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open source file, error %d\n", GetLastError());

    params.dwSize = sizeof(params);
    params.dwCopyFlags = 0;
    SetLastError(0xdeadbeef);
    hr = pCopyFile2(source, dest, &params);
    ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "CopyFile2: unexpected error 0x%08x\n", hr);
    ok(GetLastError() == ERROR_SHARING_VIOLATION, "CopyFile2: last error %d\n", GetLastError());

    /* in addition, the source is opened before the destination */
    params.dwSize = sizeof(params);
    params.dwCopyFlags = 0;
    SetLastError(0xdeadbeef);
    hr = pCopyFile2(doesntexistW, dest, &params);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "got 0x%08x\n", hr);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "CopyFile2: last error %d\n", GetLastError());
    CloseHandle(hfile);

    /* copying from a r+w opened, r shared source succeeds */
    hfile = CreateFileW(source, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open source file, error %d\n", GetLastError());

    params.dwSize = sizeof(params);
    params.dwCopyFlags = 0;
    hr = pCopyFile2(source, dest, &params);
    ok(hr == S_OK, "failed 0x%08x\n", hr);
    CloseHandle(hfile);

    /* copying from a delete-locked source mostly succeeds */
    hfile = CreateFileW(source, DELETE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open source file, error %d\n", GetLastError());

    params.dwSize = sizeof(params);
    params.dwCopyFlags = 0;
    hr = pCopyFile2(source, dest, &params);
    ok(hr == S_OK, "failed 0x%08x\n", hr);
    CloseHandle(hfile);

    /* copying to a write-locked destination fails */
    hfile = CreateFileW(dest, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open destination file, error %d\n", GetLastError());

    params.dwSize = sizeof(params);
    params.dwCopyFlags = 0;
    SetLastError(0xdeadbeef);
    hr = pCopyFile2(source, dest, FALSE);
    ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "CopyFile2: unexpected error 0x%08x\n", hr);
    ok(GetLastError() == ERROR_SHARING_VIOLATION, "CopyFile2: last error %d\n", GetLastError());
    CloseHandle(hfile);

    /* copying to a r+w opened, w shared destination mostly succeeds */
    hfile = CreateFileW(dest, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open destination file, error %d\n", GetLastError());

    params.dwSize = sizeof(params);
    params.dwCopyFlags = 0;
    hr = pCopyFile2(source, dest, FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    CloseHandle(hfile);

    /* copying to a delete-locked destination fails, even when the destination is delete-shared */
    hfile = CreateFileW(dest, DELETE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open destination file, error %d\n", GetLastError());

    params.dwSize = sizeof(params);
    params.dwCopyFlags = 0;
    SetLastError(0xdeadbeef);
    hr = pCopyFile2(source, dest, &params);
    ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "CopyFile2: unexpected error 0x%08x\n", hr);
    ok(GetLastError() == ERROR_SHARING_VIOLATION, "CopyFile2: last error %d\n", GetLastError());
    CloseHandle(hfile);

    /* copy to a file that's opened the way Wine opens the source */
    hfile = CreateFileW(dest, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open destination file, error %d\n", GetLastError());

    params.dwSize = sizeof(params);
    params.dwCopyFlags = 0;
    hr = pCopyFile2(source, dest, &params);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    CloseHandle(hfile);

    /* make sure that destination has correct size */
    hfile = CreateFileW(dest, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open destination file\n");
    ret = GetFileSize(hfile, NULL);
    ok(ret == sizeof(prefix), "destination file has wrong size %d\n", ret);

    /* make sure that destination has the same filetime */
    ret = GetFileTime(hfile, NULL, NULL, &ft2);
    ok(ret, "GetFileTime error %d\n", GetLastError());
    ok(CompareFileTime(&ft1, &ft2) == 0, "destination file has wrong filetime\n");

    params.dwSize = sizeof(params);
    params.dwCopyFlags = 0;
    SetLastError(0xdeadbeef);
    hr = pCopyFile2(source, dest, &params);
    ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "CopyFile2: unexpected error 0x%08x\n", hr);
    ok(GetLastError() == ERROR_SHARING_VIOLATION, "CopyFile2: last error %d\n", GetLastError());

    /* make sure that destination still has correct size */
    ret = GetFileSize(hfile, NULL);
    ok(ret == sizeof(prefix), "destination file has wrong size %d\n", ret);
    ret = ReadFile(hfile, buf, sizeof(buf), &len, NULL);
    ok(ret && len == sizeof(prefix), "ReadFile: error %d\n", GetLastError());
    ok(!memcmp(prefix, buf, sizeof(prefix)), "buffer contents mismatch\n");

    /* check error on copying over a mapped file that was opened with FILE_SHARE_READ */
    hmapfile = CreateFileMappingW(hfile, NULL, PAGE_READONLY | SEC_COMMIT, 0, 0, NULL);
    ok(hmapfile != NULL, "CreateFileMapping: error %d\n", GetLastError());

    params.dwSize = sizeof(params);
    params.dwCopyFlags = 0;
    SetLastError(0xdeadbeef);
    hr = pCopyFile2(source, dest, &params);
    ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "CopyFile2: unexpected error 0x%08x\n", hr);
    ok(GetLastError() == ERROR_SHARING_VIOLATION, "CopyFile2: last error %d\n", GetLastError());

    CloseHandle(hmapfile);
    CloseHandle(hfile);

    hfile = CreateFileW(dest, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open destination file\n");

    /* check error on copying over a mapped file that was opened with FILE_SHARE_WRITE */
    hmapfile = CreateFileMappingW(hfile, NULL, PAGE_READONLY | SEC_COMMIT, 0, 0, NULL);
    ok(hmapfile != NULL, "CreateFileMapping: error %d\n", GetLastError());

    params.dwSize = sizeof(params);
    params.dwCopyFlags = 0;
    hr = pCopyFile2(source, dest, &params);
    ok(hr == HRESULT_FROM_WIN32(ERROR_USER_MAPPED_FILE), "CopyFile2: unexpected error 0x%08x\n", hr);
    ok(GetLastError() == ERROR_USER_MAPPED_FILE, "CopyFile2: last error %d\n", GetLastError());

    CloseHandle(hmapfile);
    CloseHandle(hfile);

    DeleteFileW(source);
    DeleteFileW(dest);
}

static DWORD WINAPI copy_progress_cb(LARGE_INTEGER total_size, LARGE_INTEGER total_transferred,
                                     LARGE_INTEGER stream_size, LARGE_INTEGER stream_transferred,
                                     DWORD stream, DWORD reason, HANDLE source, HANDLE dest, LPVOID userdata)
{
    ok(reason == CALLBACK_STREAM_SWITCH, "expected CALLBACK_STREAM_SWITCH, got %u\n", reason);
    CloseHandle(userdata);
    return PROGRESS_CANCEL;
}

static void test_CopyFileEx(void)
{
    char temp_path[MAX_PATH];
    char source[MAX_PATH], dest[MAX_PATH];
    static const char prefix[] = "pfx";
    HANDLE hfile;
    DWORD ret;
    BOOL retok;

    ret = GetTempPathA(MAX_PATH, temp_path);
    ok(ret != 0, "GetTempPathA error %d\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameA(temp_path, prefix, 0, source);
    ok(ret != 0, "GetTempFileNameA error %d\n", GetLastError());

    ret = GetTempFileNameA(temp_path, prefix, 0, dest);
    ok(ret != 0, "GetTempFileNameA error %d\n", GetLastError());

    hfile = CreateFileA(dest, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open destination file, error %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    retok = CopyFileExA(source, dest, copy_progress_cb, hfile, NULL, 0);
    ok(!retok, "CopyFileExA unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_REQUEST_ABORTED, "expected ERROR_REQUEST_ABORTED, got %d\n", GetLastError());
    ok(GetFileAttributesA(dest) != INVALID_FILE_ATTRIBUTES, "file was deleted\n");

    hfile = CreateFileA(dest, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open destination file, error %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    retok = CopyFileExA(source, dest, copy_progress_cb, hfile, NULL, 0);
    ok(!retok, "CopyFileExA unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_REQUEST_ABORTED, "expected ERROR_REQUEST_ABORTED, got %d\n", GetLastError());
    ok(GetFileAttributesA(dest) == INVALID_FILE_ATTRIBUTES, "file was not deleted\n");

    ret = DeleteFileA(source);
    ok(ret, "DeleteFileA failed with error %d\n", GetLastError());
    ret = DeleteFileA(dest);
    ok(!ret, "DeleteFileA unexpectedly succeeded\n");
}

/*
 *   Debugging routine to dump a buffer in a hexdump-like fashion.
 */
static void dumpmem(unsigned char *mem, int len)
{
    int x = 0;
    char hex[49], *p;
    char txt[17], *c;

    while (x < len)
    {
        p = hex;
        c = txt;
        do {
            p += sprintf(p, "%02x ", mem[x]);
            *c++ = (mem[x] >= 32 && mem[x] <= 127) ? mem[x] : '.';
        } while (++x % 16 && x < len);
        *c = '\0';
        trace("%04x: %-48s- %s\n", x, hex, txt);
    }
}

static void test_CreateFileA(void)
{
    HANDLE hFile;
    char temp_path[MAX_PATH], dirname[MAX_PATH];
    char filename[MAX_PATH];
    static const char prefix[] = "pfx";
    char windowsdir[MAX_PATH];
    char Volume_1[MAX_PATH];
    unsigned char buffer[512];
    char directory[] = "removeme";
    static const char nt_drive[] = "\\\\?\\A:";
    DWORD i, ret, len;
    struct test_list p[] = {
    {"", ERROR_PATH_NOT_FOUND, -1, FILE_ATTRIBUTE_NORMAL, TRUE }, /* dir as file w \ */
    {"", ERROR_SUCCESS, ERROR_PATH_NOT_FOUND, FILE_FLAG_BACKUP_SEMANTICS, FALSE }, /* dir as dir w \ */
    {"a", ERROR_FILE_NOT_FOUND, -1, FILE_ATTRIBUTE_NORMAL, FALSE }, /* non-exist file */
    {"a\\", ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND, FILE_ATTRIBUTE_NORMAL, FALSE }, /* non-exist dir */
    {"removeme", ERROR_ACCESS_DENIED, -1, FILE_ATTRIBUTE_NORMAL, FALSE }, /* exist dir w/o \ */
    {"removeme\\", ERROR_PATH_NOT_FOUND, -1, FILE_ATTRIBUTE_NORMAL, TRUE }, /* exst dir w \ */
    {"c:", ERROR_ACCESS_DENIED, ERROR_PATH_NOT_FOUND, FILE_ATTRIBUTE_NORMAL, FALSE }, /* device in file namespace */
    {"c:", ERROR_SUCCESS, ERROR_PATH_NOT_FOUND, FILE_FLAG_BACKUP_SEMANTICS, FALSE }, /* device in file namespace as dir */
    {"c:\\", ERROR_PATH_NOT_FOUND, ERROR_ACCESS_DENIED, FILE_ATTRIBUTE_NORMAL, TRUE }, /* root dir w \ */
    {"c:\\", ERROR_SUCCESS, ERROR_ACCESS_DENIED, FILE_FLAG_BACKUP_SEMANTICS, FALSE }, /* root dir w \ as dir */
    {"\\\\?\\c:", ERROR_SUCCESS, ERROR_BAD_NETPATH, FILE_ATTRIBUTE_NORMAL,FALSE }, /* dev namespace drive */
    {"\\\\?\\c:\\", ERROR_PATH_NOT_FOUND, ERROR_BAD_NETPATH, FILE_ATTRIBUTE_NORMAL, TRUE }, /* dev namespace drive w \ */
    {NULL, 0, -1, 0, FALSE}
    };
    BY_HANDLE_FILE_INFORMATION  Finfo;

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

    SetLastError(0xdeadbeef);
    hFile = CreateFileA("c:\\*.*", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hFile == INVALID_HANDLE_VALUE, "hFile should have been INVALID_HANDLE_VALUE\n");
    ok(GetLastError() == ERROR_INVALID_NAME ||
        broken(GetLastError() == ERROR_FILE_NOT_FOUND), /* Win98 */
        "LastError should have been ERROR_INVALID_NAME or ERROR_FILE_NOT_FOUND but got %u\n", GetLastError());

    /* get windows drive letter */
    ret = GetWindowsDirectoryA(windowsdir, sizeof(windowsdir));
    ok(ret < sizeof(windowsdir), "windowsdir is abnormally long!\n");
    ok(ret != 0, "GetWindowsDirectory: error %d\n", GetLastError());

    /* test error return codes from CreateFile for some cases */
    ret = GetTempPathA(MAX_PATH, temp_path);
    ok(ret != 0, "GetTempPathA error %d\n", GetLastError());
    strcpy(dirname, temp_path);
    strcat(dirname, directory);
    ret = CreateDirectoryA(dirname, NULL);
    ok( ret, "Createdirectory failed, gle=%d\n", GetLastError() );
    /* set current drive & directory to known location */
    SetCurrentDirectoryA( temp_path );
    i = 0;
    while (p[i].file)
    {
        filename[0] = 0;
        /* update the drive id in the table entry with the current one */
        if (p[i].file[1] == ':')
        {
            strcpy(filename, p[i].file);
            filename[0] = windowsdir[0];
        }
        else if (p[i].file[0] == '\\' && p[i].file[5] == ':')
        {
            strcpy(filename, p[i].file);
            filename[4] = windowsdir[0];
        }
        else
        {
            /* prefix the table entry with the current temp directory */
            strcpy(filename, temp_path);
            strcat(filename, p[i].file);
        }
        hFile = CreateFileA( filename, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING,
                        p[i].options, NULL );
        /* if we get ACCESS_DENIED when we do not expect it, assume
         * no access to the volume
         */
        if (hFile == INVALID_HANDLE_VALUE &&
            GetLastError() == ERROR_ACCESS_DENIED &&
            p[i].err != ERROR_ACCESS_DENIED)
        {
            if (p[i].todo_flag)
                skip("Either no authority to volume, or is todo_wine for %s err=%d should be %d\n", filename, GetLastError(), p[i].err);
            else
                skip("Do not have authority to access volumes. Test for %s skipped\n", filename);
        }
        /* otherwise validate results with expectations */
        else if (p[i].todo_flag)
            todo_wine ok(
                (hFile == INVALID_HANDLE_VALUE &&
                  (p[i].err == GetLastError() || p[i].err2 == GetLastError())) ||
                (hFile != INVALID_HANDLE_VALUE && p[i].err == ERROR_SUCCESS),
                "CreateFileA failed on %s, hFile %p, err=%u, should be %u\n",
                filename, hFile, GetLastError(), p[i].err);
        else
            ok(
                (hFile == INVALID_HANDLE_VALUE &&
                 (p[i].err == GetLastError() || p[i].err2 == GetLastError())) ||
                (hFile != INVALID_HANDLE_VALUE && p[i].err == ERROR_SUCCESS),
                "CreateFileA failed on %s, hFile %p, err=%u, should be %u\n",
                filename, hFile, GetLastError(), p[i].err);
        if (hFile != INVALID_HANDLE_VALUE)
            CloseHandle( hFile );
        i++;
    }
    ret = RemoveDirectoryA(dirname);
    ok(ret, "RemoveDirectoryA: error %d\n", GetLastError());


    /* test opening directory as a directory */
    hFile = CreateFileA( temp_path, GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_BACKUP_SEMANTICS, NULL );
    if (hFile != INVALID_HANDLE_VALUE && GetLastError() != ERROR_PATH_NOT_FOUND)
    {
        ok(hFile != INVALID_HANDLE_VALUE && GetLastError() == ERROR_SUCCESS,
            "CreateFileA did not work, last error %u on volume <%s>\n",
             GetLastError(), temp_path );

        if (hFile != INVALID_HANDLE_VALUE)
        {
            ret = GetFileInformationByHandle( hFile, &Finfo );
            if (ret)
            {
                ok(Finfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY,
                    "CreateFileA probably did not open temp directory %s correctly\n   file information does not include FILE_ATTRIBUTE_DIRECTORY, actual=0x%08x\n",
                temp_path, Finfo.dwFileAttributes);
            }
            CloseHandle( hFile );
        }
    }
    else
        skip("Probable Win9x, got ERROR_PATH_NOT_FOUND w/ FILE_FLAG_BACKUP_SEMANTICS or %s\n", temp_path);


    /* ***  Test opening volumes/devices using drive letter  ***         */

    /* test using drive letter in non-rewrite format without trailing \  */
    /* this should work                                                  */
    strcpy(filename, nt_drive);
    filename[4] = windowsdir[0];
    hFile = CreateFileA( filename, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, NULL );
    if (hFile != INVALID_HANDLE_VALUE ||
        (GetLastError() != ERROR_ACCESS_DENIED && GetLastError() != ERROR_BAD_NETPATH))
    {
        /* if we have adm rights to volume, then try rest of tests */
        ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA did not open %s, last error=%u\n",
            filename, GetLastError());
        if (hFile != INVALID_HANDLE_VALUE)
        {
            /* if we opened the volume/device, try to read it. Since it  */
            /* opened, we should be able to read it.  We don't care about*/
            /* what the data is at this time.                            */
            len = 512;
            ret = ReadFile( hFile, buffer, len, &len, NULL );
            todo_wine ok(ret, "Failed to read volume, last error %u, %u, for %s\n",
                GetLastError(), ret, filename);
            if (ret)
            {
                trace("buffer is\n");
                dumpmem(buffer, 64);
            }
            CloseHandle( hFile );
        }

        /* test using drive letter with trailing \ and in non-rewrite   */
        /* this should not work                                         */
        strcpy(filename, nt_drive);
        filename[4] = windowsdir[0];
        strcat( filename, "\\" );
        hFile = CreateFileA( filename, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, NULL );
        todo_wine
        ok(hFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_PATH_NOT_FOUND,
            "CreateFileA should have returned ERROR_PATH_NOT_FOUND on %s, but got %u\n",
            filename, GetLastError());
        if (hFile != INVALID_HANDLE_VALUE)
            CloseHandle( hFile );

        /* test using temp path with trailing \ and in non-rewrite as dir */
        /* this should work                                               */
        strcpy(filename, nt_drive);
        filename[4] = 0;
        strcat( filename, temp_path );
        hFile = CreateFileA( filename, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING,
                        FILE_FLAG_BACKUP_SEMANTICS, NULL );
        ok(hFile != INVALID_HANDLE_VALUE,
            "CreateFileA should have worked on %s, but got %u\n",
            filename, GetLastError());
        if (hFile != INVALID_HANDLE_VALUE)
            CloseHandle( hFile );

        /* test using drive letter without trailing \ and in device ns  */
        /* this should work                                             */
        strcpy(filename, nt_drive);
        filename[4] = windowsdir[0];
        filename[2] = '.';
        hFile = CreateFileA( filename, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, NULL );
        ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA did not open %s, last error=%u\n",
            filename, GetLastError());
        if (hFile != INVALID_HANDLE_VALUE)
            CloseHandle( hFile );
    }
    /* If we see ERROR_BAD_NETPATH then on Win9x or WinME, so skip */
    else if (GetLastError() == ERROR_BAD_NETPATH)
        skip("Probable Win9x, got ERROR_BAD_NETPATH (53)\n");
    else
        skip("Do not have authority to access volumes. Tests skipped\n");


    /* ***  Test opening volumes/devices using GUID  ***           */

    if (pGetVolumeNameForVolumeMountPointA)
    {
        strcpy(filename, "c:\\");
        filename[0] = windowsdir[0];
        ret = pGetVolumeNameForVolumeMountPointA( filename, Volume_1, MAX_PATH );
        ok(ret, "GetVolumeNameForVolumeMountPointA failed, for %s, last error=%d\n", filename, GetLastError());
        if (ret)
        {
            ok(strlen(Volume_1) == 49, "GetVolumeNameForVolumeMountPointA returned wrong length name <%s>\n", Volume_1);

            /* test the result of opening a unique volume name (GUID)
             *  with the trailing \
             *  this should error out
             */
            strcpy(filename, Volume_1);
            hFile = CreateFileA( filename, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, NULL );
            todo_wine
            ok(hFile == INVALID_HANDLE_VALUE,
                "CreateFileA should not have opened %s, hFile %p\n",
                filename, hFile);
            todo_wine
            ok(hFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_PATH_NOT_FOUND,
                "CreateFileA should have returned ERROR_PATH_NOT_FOUND on %s, but got %u\n",
                filename, GetLastError());
            if (hFile != INVALID_HANDLE_VALUE)
                CloseHandle( hFile );

            /* test the result of opening a unique volume name (GUID)
             * with the temp path string as dir
             * this should work
             */
            strcpy(filename, Volume_1);
            strcat(filename, temp_path+3);
            hFile = CreateFileA( filename, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING,
                        FILE_FLAG_BACKUP_SEMANTICS, NULL );
            todo_wine
            ok(hFile != INVALID_HANDLE_VALUE,
                "CreateFileA should have opened %s, but got %u\n",
                filename, GetLastError());
            if (hFile != INVALID_HANDLE_VALUE)
                CloseHandle( hFile );

            /* test the result of opening a unique volume name (GUID)
             * without the trailing \ and in device namespace
             * this should work
             */
            strcpy(filename, Volume_1);
            filename[2] = '.';
            filename[48] = 0;
            hFile = CreateFileA( filename, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, NULL );
            if (hFile != INVALID_HANDLE_VALUE || GetLastError() != ERROR_ACCESS_DENIED)
            {
                /* if we have adm rights to volume, then try rest of tests */
                ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA did not open %s, last error=%u\n",
                    filename, GetLastError());
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    /* if we opened the volume/device, try to read it. Since it  */
                    /* opened, we should be able to read it.  We don't care about*/
                    /* what the data is at this time.                            */
                    len = 512;
                    ret = ReadFile( hFile, buffer, len, &len, NULL );
                    todo_wine ok(ret, "Failed to read volume, last error %u, %u, for %s\n",
                        GetLastError(), ret, filename);
                    if (ret)
                    {
                        trace("buffer is\n");
                        dumpmem(buffer, 64);
                    }
                    CloseHandle( hFile );
	        }
            }
            else
                skip("Do not have authority to access volumes. Tests skipped\n");
        }
        else
            win_skip("GetVolumeNameForVolumeMountPointA not functioning\n");
    }
    else
        win_skip("GetVolumeNameForVolumeMountPointA not found\n");
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
    if (ret == 0 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("GetTempPathW is not available\n");
        return;
    }
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

static void test_CreateFile2(void)
{
    HANDLE hFile;
    WCHAR temp_path[MAX_PATH];
    WCHAR filename[MAX_PATH];
    CREATEFILE2_EXTENDED_PARAMETERS exparams;
    static const WCHAR emptyW[]={'\0'};
    static const WCHAR prefix[] = {'p','f','x',0};
    static const WCHAR bogus[] = { '\\', '\\', '.', '\\', 'B', 'O', 'G', 'U', 'S', 0 };
    DWORD ret;

    if (!pCreateFile2)
    {
        win_skip("CreateFile2 is missing\n");
        return;
    }

    ret = GetTempPathW(MAX_PATH, temp_path);
    ok(ret != 0, "GetTempPathW error %d\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameW(temp_path, prefix, 0, filename);
    ok(ret != 0, "GetTempFileNameW error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    exparams.dwSize = sizeof(exparams);
    exparams.dwFileAttributes = FILE_FLAG_RANDOM_ACCESS;
    exparams.dwFileFlags = 0;
    exparams.dwSecurityQosFlags = 0;
    exparams.lpSecurityAttributes = NULL;
    exparams.hTemplateFile = 0;
    hFile = pCreateFile2(filename, GENERIC_READ, 0, CREATE_NEW, &exparams);
    ok(hFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_EXISTS,
       "CREATE_NEW should fail if file exists and last error value should be ERROR_FILE_EXISTS\n");

    SetLastError(0xdeadbeef);
    hFile = pCreateFile2(filename, GENERIC_READ, FILE_SHARE_READ, CREATE_ALWAYS, &exparams);
    ok(hFile != INVALID_HANDLE_VALUE && GetLastError() == ERROR_ALREADY_EXISTS,
       "hFile %p, last error %u\n", hFile, GetLastError());
    CloseHandle(hFile);

    SetLastError(0xdeadbeef);
    hFile = pCreateFile2(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_ALWAYS, &exparams);
    ok(hFile != INVALID_HANDLE_VALUE && GetLastError() == ERROR_ALREADY_EXISTS,
       "hFile %p, last error %u\n", hFile, GetLastError());
    CloseHandle(hFile);

    ret = DeleteFileW(filename);
    ok(ret, "DeleteFileW: error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    hFile = pCreateFile2(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_ALWAYS, &exparams);
    ok(hFile != INVALID_HANDLE_VALUE && GetLastError() == 0,
       "hFile %p, last error %u\n", hFile, GetLastError());
    CloseHandle(hFile);

    ret = DeleteFileW(filename);
    ok(ret, "DeleteFileW: error %d\n", GetLastError());

    hFile = pCreateFile2(emptyW, GENERIC_READ, 0, CREATE_NEW, &exparams);
    ok(hFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_PATH_NOT_FOUND,
       "CreateFile2(\"\") returned ret=%p error=%d\n",hFile,GetLastError());

    /* test the result of opening a nonexistent driver name */
    exparams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    hFile = pCreateFile2(bogus, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING, &exparams);
    ok(hFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND,
       "CreateFile2 on invalid VxD name returned ret=%p error=%d\n",hFile,GetLastError());

    ret = CreateDirectoryW(filename, NULL);
    ok(ret == TRUE, "couldn't create temporary directory\n");
    exparams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS;
    hFile = pCreateFile2(filename, GENERIC_READ | GENERIC_WRITE, 0, OPEN_ALWAYS, &exparams);
    todo_wine
    ok(hFile == INVALID_HANDLE_VALUE,
       "expected CreateFile2 to fail on existing directory, error: %d\n", GetLastError());
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

    result = GetWindowsDirectoryA(windowsdir, sizeof(windowsdir));
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
    char temp_path[MAX_PATH], temp_file[MAX_PATH];
    HANDLE hfile;

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

    ret = DeleteFileA("nonexist.txt");
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND, "DeleteFileA(\"nonexist.txt\") returned ret=%d error=%d\n",ret,GetLastError());

    GetTempPathA(MAX_PATH, temp_path);
    GetTempFileNameA(temp_path, "tst", 0, temp_file);

    SetLastError(0xdeadbeef);
    hfile = CreateFileA(temp_file, GENERIC_READ, FILE_SHARE_DELETE | FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "CreateFile error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = DeleteFileA(temp_file);
    ok(ret, "DeleteFile error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = CloseHandle(hfile);
    ok(ret, "CloseHandle error %d\n", GetLastError());
    ret = DeleteFileA(temp_file);
    ok(!ret, "DeleteFile should fail\n");

    SetLastError(0xdeadbeef);
    ret = CreateDirectoryA("testdir", NULL);
    ok(ret, "CreateDirectory failed, got err %d\n", GetLastError());
    ret = DeleteFileA("testdir");
    ok(!ret && GetLastError() == ERROR_ACCESS_DENIED,
        "Expected ERROR_ACCESS_DENIED, got error %d\n", GetLastError());
    ret = RemoveDirectoryA("testdir");
    ok(ret, "Remove a directory failed, got error %d\n", GetLastError());
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
    if (ret == 0 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("DeleteFileW is not available\n");
        return;
    }
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
    HANDLE hfile;
    HANDLE hmapfile;
    DWORD ret;
    BOOL retok;

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

    hfile = CreateFileA(source, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open source file\n");

    retok = WriteFile(hfile, prefix, sizeof(prefix), &ret, NULL );
    ok( retok && ret == sizeof(prefix),
       "WriteFile error %d\n", GetLastError());

    hmapfile = CreateFileMappingW(hfile, NULL, PAGE_READONLY | SEC_COMMIT, 0, 0, NULL);
    ok(hmapfile != NULL, "CreateFileMapping: error %d\n", GetLastError());

    ret = MoveFileA(source, dest);
    todo_wine {
        ok(!ret, "MoveFileA: expected failure\n");
        ok(GetLastError() == ERROR_SHARING_VIOLATION ||
           broken(GetLastError() == ERROR_ACCESS_DENIED), /* Win9x and WinMe */
           "MoveFileA: expected ERROR_SHARING_VIOLATION, got %d\n", GetLastError());
    }

    CloseHandle(hmapfile);
    CloseHandle(hfile);

    /* if MoveFile succeeded, move back to dest */
    if (ret) MoveFileA(dest, source);

    hfile = CreateFileA(source, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to open source file\n");

    hmapfile = CreateFileMappingW(hfile, NULL, PAGE_READONLY | SEC_COMMIT, 0, 0, NULL);
    ok(hmapfile != NULL, "CreateFileMapping: error %d\n", GetLastError());

    ret = MoveFileA(source, dest);
    todo_wine {
        ok(!ret, "MoveFileA: expected failure\n");
        ok(GetLastError() == ERROR_SHARING_VIOLATION ||
           broken(GetLastError() == ERROR_ACCESS_DENIED), /* Win9x and WinMe */
           "MoveFileA: expected ERROR_SHARING_VIOLATION, got %d\n", GetLastError());
    }

    CloseHandle(hmapfile);
    CloseHandle(hfile);

    /* if MoveFile succeeded, move back to dest */
    if (ret) MoveFileA(dest, source);

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
    if (ret == 0 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("GetTempPathW is not available\n");
        return;
    }
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
    HANDLE handle, handle2;
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
    handle2 = CreateFileA( filename, GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           OPEN_EXISTING, 0, 0 );
    if (handle2 == INVALID_HANDLE_VALUE)
    {
        ok( 0, "couldn't open file \"%s\" (err=%d)\n", filename, GetLastError() );
        goto cleanup;
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

    /* shared lock can overlap exclusive if handles are equal */
    S(U(overlapped)).Offset = 300;
    ok( LockFileEx( handle, LOCKFILE_EXCLUSIVE_LOCK, 0, 100, 0, &overlapped ),
        "LockFileEx exclusive 300,100 failed\n" );
    ok( !LockFileEx( handle2, LOCKFILE_FAIL_IMMEDIATELY, 0, 100, 0, &overlapped ),
        "LockFileEx handle2 300,100 succeeded\n" );
    ret = LockFileEx( handle, LOCKFILE_FAIL_IMMEDIATELY, 0, 100, 0, &overlapped );
    ok( ret, "LockFileEx 300,100 failed\n" );
    ok( UnlockFileEx( handle, 0, 100, 0, &overlapped ), "UnlockFileEx 300,100 failed\n" );
    /* exclusive lock is removed first */
    ok( LockFileEx( handle2, LOCKFILE_FAIL_IMMEDIATELY, 0, 100, 0, &overlapped ),
        "LockFileEx handle2 300,100 failed\n" );
    ok( UnlockFileEx( handle2, 0, 100, 0, &overlapped ), "UnlockFileEx 300,100 failed\n" );
    if (ret)
        ok( UnlockFileEx( handle, 0, 100, 0, &overlapped ), "UnlockFileEx 300,100 failed\n" );

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
    if (!limited_LockFile) ok( !LockFile( handle, 98, 0, 4, 0 ), "LockFile 98,4 succeeded\n" );
    ok( LockFile( handle, 90, 0, 10, 0 ), "LockFile 90,10 failed\n" );
    if (!limited_LockFile) ok( !LockFile( handle, 100, 0, 10, 0 ), "LockFile 100,10 failed\n" );

    ok( UnlockFile( handle, 90, 0, 10, 0 ), "UnlockFile 90,10 failed\n" );
    ok( !UnlockFile( handle, 100, 0, 10, 0 ), "UnlockFile 100,10 succeeded\n" );

    ok( UnlockFile( handle, 100, 0, 0, 0 ), "UnlockFile 100,0 failed\n" );

    CloseHandle( handle2 );
cleanup:
    CloseHandle( handle );
    DeleteFileA( filename );
}

static BOOL create_fake_dll( LPCSTR filename )
{
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER *sec;
    BYTE *buffer;
    DWORD lfanew = sizeof(*dos);
    DWORD size = lfanew + sizeof(*nt) + sizeof(*sec);
    DWORD written;
    BOOL ret;

    HANDLE file = CreateFileA( filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    if (file == INVALID_HANDLE_VALUE) return FALSE;

    buffer = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size );

    dos = (IMAGE_DOS_HEADER *)buffer;
    dos->e_magic    = IMAGE_DOS_SIGNATURE;
    dos->e_cblp     = sizeof(*dos);
    dos->e_cp       = 1;
    dos->e_cparhdr  = lfanew / 16;
    dos->e_minalloc = 0;
    dos->e_maxalloc = 0xffff;
    dos->e_ss       = 0x0000;
    dos->e_sp       = 0x00b8;
    dos->e_lfarlc   = lfanew;
    dos->e_lfanew   = lfanew;

    nt = (IMAGE_NT_HEADERS *)(buffer + lfanew);
    nt->Signature = IMAGE_NT_SIGNATURE;
#if defined __i386__
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
#elif defined __x86_64__
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64;
#elif defined __powerpc__
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_POWERPC;
#elif defined __arm__
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_ARMNT;
#elif defined __aarch64__
    nt->FileHeader.Machine = IMAGE_FILE_MACHINE_ARM64;
#else
# error You must specify the machine type
#endif
    nt->FileHeader.NumberOfSections = 1;
    nt->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);
    nt->FileHeader.Characteristics = IMAGE_FILE_DLL | IMAGE_FILE_EXECUTABLE_IMAGE;
    nt->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR_MAGIC;
    nt->OptionalHeader.MajorLinkerVersion = 1;
    nt->OptionalHeader.MinorLinkerVersion = 0;
    nt->OptionalHeader.ImageBase = 0x10000000;
    nt->OptionalHeader.SectionAlignment = 0x1000;
    nt->OptionalHeader.FileAlignment = 0x1000;
    nt->OptionalHeader.MajorOperatingSystemVersion = 1;
    nt->OptionalHeader.MinorOperatingSystemVersion = 0;
    nt->OptionalHeader.MajorImageVersion = 1;
    nt->OptionalHeader.MinorImageVersion = 0;
    nt->OptionalHeader.MajorSubsystemVersion = 4;
    nt->OptionalHeader.MinorSubsystemVersion = 0;
    nt->OptionalHeader.SizeOfImage = 0x2000;
    nt->OptionalHeader.SizeOfHeaders = size;
    nt->OptionalHeader.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
    nt->OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;

    sec = (IMAGE_SECTION_HEADER *)(nt + 1);
    memcpy( sec->Name, ".rodata", sizeof(".rodata") );
    sec->Misc.VirtualSize = 0x1000;
    sec->VirtualAddress   = 0x1000;
    sec->SizeOfRawData    = 0;
    sec->PointerToRawData = 0;
    sec->Characteristics  = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;

    ret = WriteFile( file, buffer, size, &written, NULL ) && written == size;
    HeapFree( GetProcessHeap(), 0, buffer );
    CloseHandle( file );
    return ret;
}

static unsigned int map_file_access( unsigned int access )
{
    if (access & GENERIC_READ)    access |= FILE_GENERIC_READ;
    if (access & GENERIC_WRITE)   access |= FILE_GENERIC_WRITE;
    if (access & GENERIC_EXECUTE) access |= FILE_GENERIC_EXECUTE;
    if (access & GENERIC_ALL)     access |= FILE_ALL_ACCESS;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

static BOOL is_sharing_compatible( DWORD access1, DWORD sharing1, DWORD access2, DWORD sharing2 )
{
    access1 = map_file_access( access1 );
    access2 = map_file_access( access2 );
    access1 &= FILE_READ_DATA | FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_EXECUTE | DELETE;
    access2 &= FILE_READ_DATA | FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_EXECUTE | DELETE;

    if (!access1) sharing1 = FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE;
    if (!access2) sharing2 = FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE;

    if ((access1 & (FILE_READ_DATA|FILE_EXECUTE)) && !(sharing2 & FILE_SHARE_READ)) return FALSE;
    if ((access1 & (FILE_WRITE_DATA|FILE_APPEND_DATA)) && !(sharing2 & FILE_SHARE_WRITE)) return FALSE;
    if ((access1 & DELETE) && !(sharing2 & FILE_SHARE_DELETE)) return FALSE;
    if ((access2 & (FILE_READ_DATA|FILE_EXECUTE)) && !(sharing1 & FILE_SHARE_READ)) return FALSE;
    if ((access2 & (FILE_WRITE_DATA|FILE_APPEND_DATA)) && !(sharing1 & FILE_SHARE_WRITE)) return FALSE;
    if ((access2 & DELETE) && !(sharing1 & FILE_SHARE_DELETE)) return FALSE;
    return TRUE;
}

static BOOL is_sharing_map_compatible( DWORD map_access, DWORD access2, DWORD sharing2 )
{
    if ((map_access == PAGE_READWRITE || map_access == PAGE_EXECUTE_READWRITE) &&
        !(sharing2 & FILE_SHARE_WRITE)) return FALSE;
    access2 = map_file_access( access2 );
    if ((map_access & SEC_IMAGE) && (access2 & FILE_WRITE_DATA)) return FALSE;
    return TRUE;
}

static void test_file_sharing(void)
{
    static const DWORD access_modes[] =
        { 0, GENERIC_READ, GENERIC_WRITE, GENERIC_READ|GENERIC_WRITE,
          DELETE, GENERIC_READ|DELETE, GENERIC_WRITE|DELETE, GENERIC_READ|GENERIC_WRITE|DELETE,
          GENERIC_EXECUTE, GENERIC_EXECUTE | DELETE,
          FILE_READ_DATA, FILE_WRITE_DATA, FILE_APPEND_DATA, FILE_READ_EA, FILE_WRITE_EA,
          FILE_READ_DATA | FILE_EXECUTE, FILE_WRITE_DATA | FILE_EXECUTE, FILE_APPEND_DATA | FILE_EXECUTE,
          FILE_READ_EA | FILE_EXECUTE, FILE_WRITE_EA | FILE_EXECUTE, FILE_EXECUTE,
          FILE_DELETE_CHILD, FILE_READ_ATTRIBUTES, FILE_WRITE_ATTRIBUTES };
    static const DWORD sharing_modes[] =
        { 0, FILE_SHARE_READ,
          FILE_SHARE_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
          FILE_SHARE_DELETE, FILE_SHARE_READ|FILE_SHARE_DELETE,
          FILE_SHARE_WRITE|FILE_SHARE_DELETE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE };
    static const DWORD mapping_modes[] =
        { PAGE_READONLY, PAGE_WRITECOPY, PAGE_READWRITE, SEC_IMAGE | PAGE_WRITECOPY };
    int a1, s1, a2, s2;
    int ret;
    HANDLE h, h2;

    /* make sure the file exists */
    if (!create_fake_dll( filename ))
    {
        ok(0, "couldn't create file \"%s\" (err=%d)\n", filename, GetLastError());
        return;
    }

    for (a1 = 0; a1 < sizeof(access_modes)/sizeof(access_modes[0]); a1++)
    {
        for (s1 = 0; s1 < sizeof(sharing_modes)/sizeof(sharing_modes[0]); s1++)
        {
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
                    SetLastError(0xdeadbeef);
                    h2 = CreateFileA( filename, access_modes[a2], sharing_modes[s2],
                                      NULL, OPEN_EXISTING, 0, 0 );
                    ret = GetLastError();
                    if (is_sharing_compatible( access_modes[a1], sharing_modes[s1],
                                               access_modes[a2], sharing_modes[s2] ))
                    {
                        ok( h2 != INVALID_HANDLE_VALUE,
                            "open failed for modes %x/%x/%x/%x\n",
                            access_modes[a1], sharing_modes[s1],
                            access_modes[a2], sharing_modes[s2] );
                        ok( ret == 0, "wrong error code %d\n", ret );
                    }
                    else
                    {
                        ok( h2 == INVALID_HANDLE_VALUE,
                            "open succeeded for modes %x/%x/%x/%x\n",
                            access_modes[a1], sharing_modes[s1],
                            access_modes[a2], sharing_modes[s2] );
                         ok( ret == ERROR_SHARING_VIOLATION,
                             "wrong error code %d\n", ret );
                    }
                    if (h2 != INVALID_HANDLE_VALUE) CloseHandle( h2 );
                }
            }
            CloseHandle( h );
        }
    }

    for (a1 = 0; a1 < sizeof(mapping_modes)/sizeof(mapping_modes[0]); a1++)
    {
        HANDLE m;

        create_fake_dll( filename );
        SetLastError(0xdeadbeef);
        h = CreateFileA( filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0 );
        if (h == INVALID_HANDLE_VALUE)
        {
            ok(0,"couldn't create file \"%s\" (err=%d)\n",filename,GetLastError());
            return;
        }
        m = CreateFileMappingA( h, NULL, mapping_modes[a1], 0, 0, NULL );
        ok( m != 0, "failed to create mapping %x err %u\n", mapping_modes[a1], GetLastError() );
        CloseHandle( h );
        if (!m) continue;

        for (a2 = 0; a2 < sizeof(access_modes)/sizeof(access_modes[0]); a2++)
        {
            for (s2 = 0; s2 < sizeof(sharing_modes)/sizeof(sharing_modes[0]); s2++)
            {
                SetLastError(0xdeadbeef);
                h2 = CreateFileA( filename, access_modes[a2], sharing_modes[s2],
                                  NULL, OPEN_EXISTING, 0, 0 );

                ret = GetLastError();
                if (h2 == INVALID_HANDLE_VALUE)
                {
                    ok( !is_sharing_map_compatible(mapping_modes[a1], access_modes[a2], sharing_modes[s2]),
                        "open failed for modes map %x/%x/%x\n",
                        mapping_modes[a1], access_modes[a2], sharing_modes[s2] );
                    ok( ret == ERROR_SHARING_VIOLATION,
                        "wrong error code %d\n", ret );
                }
                else
                {
                    if (!is_sharing_map_compatible(mapping_modes[a1], access_modes[a2], sharing_modes[s2]))
                        ok( broken(1),  /* no checking on nt4 */
                            "open succeeded for modes map %x/%x/%x\n",
                            mapping_modes[a1], access_modes[a2], sharing_modes[s2] );
                    ok( ret == 0xdeadbeef /* Win9x */ ||
                        ret == 0, /* XP */
                        "wrong error code %d\n", ret );
                    CloseHandle( h2 );
                }
            }
        }

        /* try CREATE_ALWAYS over an existing mapping */
        SetLastError(0xdeadbeef);
        h2 = CreateFileA( filename, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL, CREATE_ALWAYS, 0, 0 );
        ret = GetLastError();
        if (mapping_modes[a1] & SEC_IMAGE)
        {
            ok( h2 == INVALID_HANDLE_VALUE, "create succeeded for map %x\n", mapping_modes[a1] );
            ok( ret == ERROR_SHARING_VIOLATION, "wrong error code %d for %x\n", ret, mapping_modes[a1] );
        }
        else
        {
            ok( h2 == INVALID_HANDLE_VALUE, "create succeeded for map %x\n", mapping_modes[a1] );
            ok( ret == ERROR_USER_MAPPED_FILE, "wrong error code %d for %x\n", ret, mapping_modes[a1] );
        }
        if (h2 != INVALID_HANDLE_VALUE) CloseHandle( h2 );

        /* try DELETE_ON_CLOSE over an existing mapping */
        SetLastError(0xdeadbeef);
        h2 = CreateFileA( filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, 0 );
        ret = GetLastError();
        if (mapping_modes[a1] & SEC_IMAGE)
        {
            ok( h2 == INVALID_HANDLE_VALUE, "create succeeded for map %x\n", mapping_modes[a1] );
            ok( ret == ERROR_ACCESS_DENIED, "wrong error code %d for %x\n", ret, mapping_modes[a1] );
        }
        else
        {
            ok( h2 != INVALID_HANDLE_VALUE, "open failed for map %x err %u\n", mapping_modes[a1], ret );
        }
        if (h2 != INVALID_HANDLE_VALUE) CloseHandle( h2 );

        CloseHandle( m );
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
    GetWindowsDirectoryA(windowsdir, sizeof(windowsdir));
    return windowsdir[0];
}

static void test_FindFirstFileA(void)
{
    HANDLE handle;
    WIN32_FIND_DATAA data;
    int err;
    char buffer[5] = "C:\\";
    char buffer2[100];
    char nonexistent[MAX_PATH];

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
    GetWindowsDirectoryA( buffer2, sizeof(buffer2) );
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
    if (!GetTempFileNameA( buffer, "foo", 0, nonexistent ) && GetLastError() == ERROR_ACCESS_DENIED)
    {
        char tmp[MAX_PATH];
        GetTempPathA( sizeof(tmp), tmp );
        GetTempFileNameA( tmp, "foo", 0, nonexistent );
    }
    DeleteFileA( nonexistent );
    strcpy(buffer2, nonexistent);
    strcat(buffer2, "\\");
    handle = FindFirstFileA(buffer2, &data);
    err = GetLastError();
    ok ( handle == INVALID_HANDLE_VALUE, "FindFirstFile on %s should fail\n", buffer2 );
    todo_wine {
        ok ( err == ERROR_PATH_NOT_FOUND, "Bad Error number %d\n", err );
    }

    /* try FindFirstFileA without trailing backslash */
    SetLastError( 0xdeadbeaf );
    strcpy(buffer2, nonexistent);
    handle = FindFirstFileA(buffer2, &data);
    err = GetLastError();
    ok ( handle == INVALID_HANDLE_VALUE, "FindFirstFile on %s should fail\n", buffer2 );
    ok ( err == ERROR_FILE_NOT_FOUND, "Bad Error number %d\n", err );

    /* try FindFirstFileA on "C:\foo\bar.txt" */
    SetLastError( 0xdeadbeaf );
    strcpy(buffer2, nonexistent);
    strcat(buffer2, "\\bar.txt");
    handle = FindFirstFileA(buffer2, &data);
    err = GetLastError();
    ok ( handle == INVALID_HANDLE_VALUE, "FindFirstFile on %s should fail\n", buffer2 );
    ok ( err == ERROR_PATH_NOT_FOUND, "Bad Error number %d\n", err );

    /* try FindFirstFileA on "C:\foo\*.*" */
    SetLastError( 0xdeadbeaf );
    strcpy(buffer2, nonexistent);
    strcat(buffer2, "\\*.*");
    handle = FindFirstFileA(buffer2, &data);
    err = GetLastError();
    ok ( handle == INVALID_HANDLE_VALUE, "FindFirstFile on %s should fail\n", buffer2 );
    ok ( err == ERROR_PATH_NOT_FOUND, "Bad Error number %d\n", err );

    /* try FindFirstFileA on "foo\bar.txt" */
    SetLastError( 0xdeadbeaf );
    strcpy(buffer2, nonexistent + 3);
    strcat(buffer2, "\\bar.txt");
    handle = FindFirstFileA(buffer2, &data);
    err = GetLastError();
    ok ( handle == INVALID_HANDLE_VALUE, "FindFirstFile on %s should fail\n", buffer2 );
    ok ( err == ERROR_PATH_NOT_FOUND, "Bad Error number %d\n", err );

    /* try FindFirstFileA on "c:\nul" */
    SetLastError( 0xdeadbeaf );
    strcpy(buffer2, buffer);
    strcat(buffer2, "nul");
    handle = FindFirstFileA(buffer2, &data);
    err = GetLastError();
    ok( handle != INVALID_HANDLE_VALUE, "FindFirstFile on %s failed: %d\n", buffer2, err );
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
    ok( handle != INVALID_HANDLE_VALUE, "FindFirstFile on %s failed: %d\n", buffer2, err );
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
    ok ( handle == INVALID_HANDLE_VALUE, "FindFirstFile on %s should fail\n", buffer2 );
    ok ( err == ERROR_PATH_NOT_FOUND, "Bad Error number %d\n", err );

    /* try FindFirstFileA on "c:\nul*" */
    SetLastError( 0xdeadbeaf );
    strcpy(buffer2, buffer);
    strcat(buffer2, "nul*");
    handle = FindFirstFileA(buffer2, &data);
    err = GetLastError();
    ok ( handle == INVALID_HANDLE_VALUE, "FindFirstFile on %s should fail\n", buffer2 );
    ok ( err == ERROR_FILE_NOT_FOUND, "Bad Error number %d\n", err );

    /* try FindFirstFileA on "c:\foo\bar\nul" */
    SetLastError( 0xdeadbeaf );
    strcpy(buffer2, buffer);
    strcat(buffer2, "foo\\bar\\nul");
    handle = FindFirstFileA(buffer2, &data);
    err = GetLastError();
    ok ( handle == INVALID_HANDLE_VALUE, "FindFirstFile on %s should fail\n", buffer2 );
    ok ( err == ERROR_PATH_NOT_FOUND, "Bad Error number %d\n", err );

    /* try FindFirstFileA on "c:\foo\nul\bar" */
    SetLastError( 0xdeadbeaf );
    strcpy(buffer2, buffer);
    strcat(buffer2, "foo\\nul\\bar");
    handle = FindFirstFileA(buffer2, &data);
    err = GetLastError();
    ok ( handle == INVALID_HANDLE_VALUE, "FindFirstFile on %s should fail\n", buffer2 );
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
    while (FindNextFileA(handle, &search_results))
    {
        /* get to the end of the files */
    }
    ok ( FindClose(handle) == TRUE, "Failed to close handle\n");
    err = GetLastError();
    ok ( err == ERROR_NO_MORE_FILES, "GetLastError should return ERROR_NO_MORE_FILES\n");
}

static void test_FindFirstFileExA(FINDEX_INFO_LEVELS level, FINDEX_SEARCH_OPS search_ops, DWORD flags)
{
    WIN32_FIND_DATAA search_results;
    HANDLE handle;
    BOOL ret;

    if (!pFindFirstFileExA)
    {
        win_skip("FindFirstFileExA() is missing\n");
        return;
    }

    trace("Running FindFirstFileExA tests with level=%d, search_ops=%d, flags=%u\n",
          level, search_ops, flags);

    CreateDirectoryA("test-dir", NULL);
    _lclose(_lcreat("test-dir\\file1", 0));
    _lclose(_lcreat("test-dir\\file2", 0));
    CreateDirectoryA("test-dir\\dir1", NULL);
    SetLastError(0xdeadbeef);
    handle = pFindFirstFileExA("test-dir\\*", level, &search_results, search_ops, NULL, flags);
    if (handle == INVALID_HANDLE_VALUE && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("FindFirstFileExA is not implemented\n");
        goto cleanup;
    }
    if ((flags & FIND_FIRST_EX_LARGE_FETCH) && handle == INVALID_HANDLE_VALUE && GetLastError() == ERROR_INVALID_PARAMETER)
    {
        win_skip("FindFirstFileExA flag FIND_FIRST_EX_LARGE_FETCH not supported, skipping test\n");
        goto cleanup;
    }
    if ((level == FindExInfoBasic) && handle == INVALID_HANDLE_VALUE && GetLastError() == ERROR_INVALID_PARAMETER)
    {
        win_skip("FindFirstFileExA level FindExInfoBasic not supported, skipping test\n");
        goto cleanup;
    }

#define CHECK_NAME(fn) (strcmp((fn), "file1") == 0 || strcmp((fn), "file2") == 0 || strcmp((fn), "dir1") == 0)
#define CHECK_LEVEL(fn) (level != FindExInfoBasic || !(fn)[0])

    ok(handle != INVALID_HANDLE_VALUE, "FindFirstFile failed (err=%u)\n", GetLastError());
    ok(strcmp(search_results.cFileName, ".") == 0, "First entry should be '.', is %s\n", search_results.cFileName);
    ok(CHECK_LEVEL(search_results.cAlternateFileName), "FindFirstFile unexpectedly returned an alternate filename\n");

    ok(FindNextFileA(handle, &search_results), "Fetching second file failed\n");
    ok(strcmp(search_results.cFileName, "..") == 0, "Second entry should be '..' is %s\n", search_results.cFileName);
    ok(CHECK_LEVEL(search_results.cAlternateFileName), "FindFirstFile unexpectedly returned an alternate filename\n");

    ok(FindNextFileA(handle, &search_results), "Fetching third file failed\n");
    ok(CHECK_NAME(search_results.cFileName), "Invalid third entry - %s\n", search_results.cFileName);
    ok(CHECK_LEVEL(search_results.cAlternateFileName), "FindFirstFile unexpectedly returned an alternate filename\n");

    SetLastError(0xdeadbeef);
    ret = FindNextFileA(handle, &search_results);
    if (!ret && (GetLastError() == ERROR_NO_MORE_FILES) && (search_ops == FindExSearchLimitToDirectories))
    {
        skip("File system supports directory filtering\n");
        /* Results from the previous call are not cleared */
        ok(strcmp(search_results.cFileName, "dir1") == 0, "Third entry should be 'dir1' is %s\n", search_results.cFileName);
        ok(CHECK_LEVEL(search_results.cAlternateFileName), "FindFirstFile unexpectedly returned an alternate filename\n");

    }
    else
    {
        ok(ret, "Fetching fourth file failed\n");
        ok(CHECK_NAME(search_results.cFileName), "Invalid fourth entry - %s\n", search_results.cFileName);
        ok(CHECK_LEVEL(search_results.cAlternateFileName), "FindFirstFile unexpectedly returned an alternate filename\n");

        ok(FindNextFileA(handle, &search_results), "Fetching fifth file failed\n");
        ok(CHECK_NAME(search_results.cFileName), "Invalid fifth entry - %s\n", search_results.cFileName);
        ok(CHECK_LEVEL(search_results.cAlternateFileName), "FindFirstFile unexpectedly returned an alternate filename\n");

        ok(FindNextFileA(handle, &search_results) == FALSE, "Fetching sixth file should fail\n");
    }

#undef CHECK_NAME
#undef CHECK_LEVEL

    FindClose( handle );

    /* Most Windows systems seem to ignore the FIND_FIRST_EX_CASE_SENSITIVE flag. Unofficial documentation
     * suggests that there are registry keys and that it might depend on the used filesystem. */
    SetLastError(0xdeadbeef);
    handle = pFindFirstFileExA("TEST-DIR\\*", level, &search_results, search_ops, NULL, flags);
    if (flags & FIND_FIRST_EX_CASE_SENSITIVE)
    {
        ok(handle != INVALID_HANDLE_VALUE || GetLastError() == ERROR_PATH_NOT_FOUND,
           "Unexpected error %x, expected valid handle or ERROR_PATH_NOT_FOUND\n", GetLastError());
        trace("FindFirstFileExA flag FIND_FIRST_EX_CASE_SENSITIVE is %signored\n",
              (handle == INVALID_HANDLE_VALUE) ? "not " : "");
    }
    else
        ok(handle != INVALID_HANDLE_VALUE, "Unexpected error %x, expected valid handle\n", GetLastError());
    if (handle != INVALID_HANDLE_VALUE)
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
    DeleteFileA(filename);
    *handle = CreateFileA(filename, GENERIC_READ|GENERIC_WRITE, 0, 0,
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

    hmap = CreateFileMappingA( handle, NULL, PAGE_READWRITE, 0, 0x1000, "named_file_map" );
    ok( hmap != NULL, "mapping should work, I named it!\n" );

    ok( CloseHandle( hmap ), "can't close mapping handle\n");

    /* We have to close file before we try new stuff with mapping again.
       Else we would always succeed on XP or block descriptors on 95. */
    hmap = CreateFileMappingA( handle, NULL, PAGE_READWRITE, 0, 0, NULL );
    ok( hmap != NULL, "We should still be able to map!\n" );
    ok( CloseHandle( hmap ), "can't close mapping handle\n");
    ok( CloseHandle( handle ), "can't close file handle\n");
    handle = NULL;

    ok(test_Mapfile_createtemp(&handle), "Couldn't create test file.\n");

    hmap = CreateFileMappingA( handle, NULL, PAGE_READWRITE, 0, 0, NULL );
    ok( hmap == NULL, "mapped zero size file\n");
    ok( GetLastError() == ERROR_FILE_INVALID, "not ERROR_FILE_INVALID\n");

    hmap = CreateFileMappingA( handle, NULL, PAGE_READWRITE, 0x80000000, 0, NULL );
    ok( hmap == NULL || broken(hmap != NULL) /* NT4 */, "mapping should fail\n");
    /* GetLastError() varies between win9x and WinNT and also depends on the filesystem */
    if ( hmap )
        CloseHandle( hmap );

    hmap = CreateFileMappingA( handle, NULL, PAGE_READWRITE, 0x80000000, 0x10000, NULL );
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
    DWORD type, type2;
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
    h = GetStdHandle( STD_OUTPUT_HANDLE );
    ok( h != INVALID_HANDLE_VALUE, "GetStdHandle failed\n" );
    type = GetFileType( (HANDLE)STD_OUTPUT_HANDLE );
    type2 = GetFileType( h );
    ok(type == type2, "expected type %d for STD_OUTPUT_HANDLE got %d\n", type2, type);
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

static BOOL user_apc_ran;
static void CALLBACK user_apc(ULONG_PTR param)
{
    user_apc_ran = TRUE;
}

static void test_read_write(void)
{
    DWORD bytes, ret, old_prot;
    HANDLE hFile;
    char temp_path[MAX_PATH];
    char filename[MAX_PATH];
    char *mem;
    static const char prefix[] = "pfx";

    ret = GetTempPathA(MAX_PATH, temp_path);
    ok(ret != 0, "GetTempPathA error %d\n", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetTempFileNameA(temp_path, prefix, 0, filename);
    ok(ret != 0, "GetTempFileNameA error %d\n", GetLastError());

    hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA: error %d\n", GetLastError());

    user_apc_ran = FALSE;
    if (pQueueUserAPC) {
        trace("Queueing an user APC\n"); /* verify the file is non alerable */
        ret = pQueueUserAPC(&user_apc, GetCurrentThread(), 0);
        ok(ret, "QueueUserAPC failed: %d\n", GetLastError());
    }

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

    ok(user_apc_ran == FALSE, "UserAPC ran, file using alertable io mode\n");
    if (pQueueUserAPC)
        SleepEx(0, TRUE); /* get rid of apc */

    /* test passing protected memory as buffer */

    mem = VirtualAlloc( NULL, 0x4000, MEM_COMMIT, PAGE_READWRITE );
    ok( mem != NULL, "failed to allocate virtual mem error %u\n", GetLastError() );

    ret = WriteFile( hFile, mem, 0x4000, &bytes, NULL );
    ok( ret, "WriteFile failed error %u\n", GetLastError() );
    ok( bytes == 0x4000, "only wrote %x bytes\n", bytes );

    ret = VirtualProtect( mem + 0x2000, 0x2000, PAGE_NOACCESS, &old_prot );
    ok( ret, "VirtualProtect failed error %u\n", GetLastError() );

    ret = WriteFile( hFile, mem, 0x4000, &bytes, NULL );
    ok( !ret, "WriteFile succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_USER_BUFFER ||
        GetLastError() == ERROR_INVALID_PARAMETER,  /* win9x */
        "wrong error %u\n", GetLastError() );
    ok( bytes == 0, "wrote %x bytes\n", bytes );

    ret = WriteFile( (HANDLE)0xdead, mem, 0x4000, &bytes, NULL );
    ok( !ret, "WriteFile succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_HANDLE || /* handle is checked before buffer on NT */
        GetLastError() == ERROR_INVALID_PARAMETER,  /* win9x */
        "wrong error %u\n", GetLastError() );
    ok( bytes == 0, "wrote %x bytes\n", bytes );

    ret = VirtualProtect( mem, 0x2000, PAGE_NOACCESS, &old_prot );
    ok( ret, "VirtualProtect failed error %u\n", GetLastError() );

    ret = WriteFile( hFile, mem, 0x4000, &bytes, NULL );
    ok( !ret, "WriteFile succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_USER_BUFFER ||
        GetLastError() == ERROR_INVALID_PARAMETER,  /* win9x */
        "wrong error %u\n", GetLastError() );
    ok( bytes == 0, "wrote %x bytes\n", bytes );

    SetFilePointer( hFile, 0, NULL, FILE_BEGIN );

    ret = ReadFile( hFile, mem, 0x4000, &bytes, NULL );
    ok( !ret, "ReadFile succeeded\n" );
    ok( GetLastError() == ERROR_NOACCESS ||
        GetLastError() == ERROR_INVALID_PARAMETER,  /* win9x */
        "wrong error %u\n", GetLastError() );
    ok( bytes == 0, "read %x bytes\n", bytes );

    ret = VirtualProtect( mem, 0x2000, PAGE_READONLY, &old_prot );
    ok( ret, "VirtualProtect failed error %u\n", GetLastError() );

    ret = ReadFile( hFile, mem, 0x4000, &bytes, NULL );
    ok( !ret, "ReadFile succeeded\n" );
    ok( GetLastError() == ERROR_NOACCESS ||
        GetLastError() == ERROR_INVALID_PARAMETER,  /* win9x */
        "wrong error %u\n", GetLastError() );
    ok( bytes == 0, "read %x bytes\n", bytes );

    ret = VirtualProtect( mem, 0x2000, PAGE_READWRITE, &old_prot );
    ok( ret, "VirtualProtect failed error %u\n", GetLastError() );

    ret = ReadFile( hFile, mem, 0x4000, &bytes, NULL );
    ok( !ret, "ReadFile succeeded\n" );
    ok( GetLastError() == ERROR_NOACCESS ||
        GetLastError() == ERROR_INVALID_PARAMETER,  /* win9x */
        "wrong error %u\n", GetLastError() );
    ok( bytes == 0, "read %x bytes\n", bytes );

    SetFilePointer( hFile, 0x1234, NULL, FILE_BEGIN );
    SetEndOfFile( hFile );
    SetFilePointer( hFile, 0, NULL, FILE_BEGIN );

    ret = ReadFile( hFile, mem, 0x4000, &bytes, NULL );
    ok( !ret, "ReadFile succeeded\n" );
    ok( GetLastError() == ERROR_NOACCESS ||
        GetLastError() == ERROR_INVALID_PARAMETER,  /* win9x */
        "wrong error %u\n", GetLastError() );
    ok( bytes == 0, "read %x bytes\n", bytes );

    ret = ReadFile( hFile, mem, 0x2000, &bytes, NULL );
    ok( ret, "ReadFile failed error %u\n", GetLastError() );
    ok( bytes == 0x1234, "read %x bytes\n", bytes );

    ret = ReadFile( hFile, NULL, 1, &bytes, NULL );
    ok( !ret, "ReadFile succeeded\n" );
    ok( GetLastError() == ERROR_NOACCESS ||
        GetLastError() == ERROR_INVALID_PARAMETER,  /* win9x */
        "wrong error %u\n", GetLastError() );
    ok( bytes == 0, "read %x bytes\n", bytes );

    VirtualFree( mem, 0, MEM_FREE );

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
    if (!pGetSystemWindowsDirectoryA)
        length = GetWindowsDirectoryA(buff, MAX_PATH);
    else
        length = pGetSystemWindowsDirectoryA(buff, MAX_PATH);

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
    ret = _lclose(hFile);
    ok( !ret, "_lclose() returns %d\n", ret );
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
    ret = _lclose(hFile);
    ok( !ret, "_lclose() returns %d\n", ret );

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
    ret = _lclose(hFile);
    ok( !ret, "_lclose() returns %d\n", ret );

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
    ret = _lclose(hFile);
    ok( !ret, "_lclose() returns %d\n", ret );

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
    if (0) /* tested: WinXP */
    {
        GetOverlappedResult(0, NULL, &result, FALSE);
        GetOverlappedResult(0, &ov, NULL, FALSE);
        GetOverlappedResult(0, NULL, NULL, FALSE);
    }

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
    ov.hEvent = CreateEventW( NULL, 1, 1, NULL );
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

    rc = CreateDirectoryA(directory, NULL);
    ok( rc, "Createdirectory failed, gle=%d\n", GetLastError() );

    rc = SetCurrentDirectoryA(directory);
    ok( rc, "SetCurrentDirectory failed, gle=%d\n", GetLastError() );

    rc = RemoveDirectoryA(".");
    if (!rc)
    {
        rc = SetCurrentDirectoryA("..");
        ok( rc, "SetCurrentDirectory failed, gle=%d\n", GetLastError() );

        rc = RemoveDirectoryA(directory);
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
    BOOL retok, removeBackup = FALSE;

    if (!pReplaceFileA)
    {
        win_skip("ReplaceFileA() is missing\n");
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
        "failed to open replaced file: %d\n", GetLastError());
    if (hReplacedFile != INVALID_HANDLE_VALUE)
    {
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
    }
    else
        skip("couldn't open replacement file, skipping tests\n");

    /* re-create replacement file for pass w/o backup (blank) */
    ret = GetTempFileNameA(temp_path, prefix, 0, replacement);
    ok(ret != 0, "GetTempFileNameA error (replacement) %d\n", GetLastError());
    /* perform replacement w/o backup
     * TODO: flags are not implemented
     */
    SetLastError(0xdeadbeef);
    ret = pReplaceFileA(replaced, replacement, NULL, 0, 0, 0);
    ok(ret || GetLastError() == ERROR_ACCESS_DENIED,
       "ReplaceFileA: unexpected error %d\n", GetLastError());

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
    ok(ret || GetLastError() == ERROR_ACCESS_DENIED,
       "ReplaceFileA: unexpected error %d\n", GetLastError());
    if (ret)
        removeBackup = TRUE;

    /* re-create replacement file for pass w/ no permissions to "replaced" */
    ret = GetTempFileNameA(temp_path, prefix, 0, replacement);
    ok(ret != 0, "GetTempFileNameA error (replacement) %d\n", GetLastError());
    ret = SetFileAttributesA(replaced, FILE_ATTRIBUTE_READONLY);
    ok(ret || GetLastError() == ERROR_ACCESS_DENIED,
       "SetFileAttributesA: error setting to read only %d\n", GetLastError());
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
    ok(ret || GetLastError() == ERROR_ACCESS_DENIED,
       "SetFileAttributesA: error setting to normal %d\n", GetLastError());

    /* replacement file still exists, make pass w/o "replaced" */
    ret = DeleteFileA(replaced);
    ok(ret || GetLastError() == ERROR_ACCESS_DENIED,
       "DeleteFileA: error (replaced) %d\n", GetLastError());
    /* perform replacement w/ backup (no pre-existing backup or "replaced")
     * TODO: flags are not implemented
     */
    SetLastError(0xdeadbeef);
    ret = pReplaceFileA(replaced, replacement, backup, 0, 0, 0);
    ok(!ret && (GetLastError() == ERROR_FILE_NOT_FOUND ||
       GetLastError() == ERROR_ACCESS_DENIED),
       "ReplaceFileA: unexpected error %d\n", GetLastError());

    /* perform replacement w/o existing "replacement" file
     * TODO: flags are not implemented
     */
    SetLastError(0xdeadbeef);
    ret = pReplaceFileA(replaced, replacement, NULL, 0, 0, 0);
    ok(!ret && (GetLastError() == ERROR_FILE_NOT_FOUND ||
        GetLastError() == ERROR_ACCESS_DENIED),
        "ReplaceFileA: unexpected error %d\n", GetLastError());
    DeleteFileA( replacement );

    /*
     * if the first round (w/ backup) worked then as long as there is no
     * failure then there is no need to check this round (w/ backup is the
     * more complete case)
     */

    /* delete temporary files, replacement and replaced are already deleted */
    if (removeBackup)
    {
        ret = DeleteFileA(backup);
        ok(ret ||
           broken(GetLastError() == ERROR_ACCESS_DENIED), /* win2k */
           "DeleteFileA: error (backup) %d\n", GetLastError());
    }
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
    BOOL removeBackup = FALSE;

    if (!pReplaceFileW)
    {
        win_skip("ReplaceFileW() is missing\n");
        return;
    }

    ret = GetTempPathW(MAX_PATH, temp_path);
    if (ret == 0 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("GetTempPathW is not available\n");
        return;
    }
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
    ok(ret || GetLastError() == ERROR_ACCESS_DENIED,
       "ReplaceFileW: error %d\n", GetLastError());

    ret = GetTempFileNameW(temp_path, prefix, 0, replacement);
    ok(ret != 0, "GetTempFileNameW error (replacement) %d\n", GetLastError());
    ret = DeleteFileW(backup);
    ok(ret, "DeleteFileW: error (backup) %d\n", GetLastError());
    ret = pReplaceFileW(replaced, replacement, backup, 0, 0, 0);
    ok(ret || GetLastError() == ERROR_ACCESS_DENIED,
       "ReplaceFileW: error %d\n", GetLastError());

    ret = GetTempFileNameW(temp_path, prefix, 0, replacement);
    ok(ret != 0, "GetTempFileNameW error (replacement) %d\n", GetLastError());
    ret = SetFileAttributesW(replaced, FILE_ATTRIBUTE_READONLY);
    ok(ret || GetLastError() == ERROR_ACCESS_DENIED,
       "SetFileAttributesW: error setting to read only %d\n", GetLastError());

    ret = pReplaceFileW(replaced, replacement, backup, 0, 0, 0);
    ok(ret != ERROR_UNABLE_TO_REMOVE_REPLACED,
        "ReplaceFileW: unexpected error %d\n", GetLastError());
    ret = SetFileAttributesW(replaced, FILE_ATTRIBUTE_NORMAL);
    ok(ret || GetLastError() == ERROR_ACCESS_DENIED,
       "SetFileAttributesW: error setting to normal %d\n", GetLastError());
    if (ret)
        removeBackup = TRUE;

    ret = DeleteFileW(replaced);
    ok(ret, "DeleteFileW: error (replaced) %d\n", GetLastError());
    ret = pReplaceFileW(replaced, replacement, backup, 0, 0, 0);
    ok(!ret, "ReplaceFileW: error %d\n", GetLastError());

    ret = pReplaceFileW(replaced, replacement, NULL, 0, 0, 0);
    ok(!ret && (GetLastError() == ERROR_FILE_NOT_FOUND ||
       GetLastError() == ERROR_ACCESS_DENIED),
        "ReplaceFileW: unexpected error %d\n", GetLastError());
    DeleteFileW( replacement );

    if (removeBackup)
    {
        ret = DeleteFileW(backup);
        ok(ret ||
           broken(GetLastError() == ERROR_ACCESS_DENIED), /* win2k */
           "DeleteFileW: error (backup) %d\n", GetLastError());
    }
}

static void test_CreateFile(void)
{
    static const struct test_data
    {
        DWORD disposition, access, error, clean_up;
    } td[] =
    {
    /* 0 */ { 0, 0, ERROR_INVALID_PARAMETER, 0 },
    /* 1 */ { 0, GENERIC_READ, ERROR_INVALID_PARAMETER, 0 },
    /* 2 */ { 0, GENERIC_READ|GENERIC_WRITE, ERROR_INVALID_PARAMETER, 0 },
    /* 3 */ { CREATE_NEW, 0, ERROR_FILE_EXISTS, 1 },
    /* 4 */ { CREATE_NEW, 0, 0, 1 },
    /* 5 */ { CREATE_NEW, GENERIC_READ, 0, 1 },
    /* 6 */ { CREATE_NEW, GENERIC_WRITE, 0, 1 },
    /* 7 */ { CREATE_NEW, GENERIC_READ|GENERIC_WRITE, 0, 0 },
    /* 8 */ { CREATE_ALWAYS, 0, 0, 0 },
    /* 9 */ { CREATE_ALWAYS, GENERIC_READ, 0, 0 },
    /* 10*/ { CREATE_ALWAYS, GENERIC_WRITE, 0, 0 },
    /* 11*/ { CREATE_ALWAYS, GENERIC_READ|GENERIC_WRITE, 0, 1 },
    /* 12*/ { OPEN_EXISTING, 0, ERROR_FILE_NOT_FOUND, 0 },
    /* 13*/ { CREATE_ALWAYS, 0, 0, 0 },
    /* 14*/ { OPEN_EXISTING, 0, 0, 0 },
    /* 15*/ { OPEN_EXISTING, GENERIC_READ, 0, 0 },
    /* 16*/ { OPEN_EXISTING, GENERIC_WRITE, 0, 0 },
    /* 17*/ { OPEN_EXISTING, GENERIC_READ|GENERIC_WRITE, 0, 1 },
    /* 18*/ { OPEN_ALWAYS, 0, 0, 0 },
    /* 19*/ { OPEN_ALWAYS, GENERIC_READ, 0, 0 },
    /* 20*/ { OPEN_ALWAYS, GENERIC_WRITE, 0, 0 },
    /* 21*/ { OPEN_ALWAYS, GENERIC_READ|GENERIC_WRITE, 0, 0 },
    /* 22*/ { TRUNCATE_EXISTING, 0, ERROR_INVALID_PARAMETER, 0 },
    /* 23*/ { TRUNCATE_EXISTING, GENERIC_READ, ERROR_INVALID_PARAMETER, 0 },
    /* 24*/ { TRUNCATE_EXISTING, GENERIC_WRITE, 0, 0 },
    /* 25*/ { TRUNCATE_EXISTING, GENERIC_READ|GENERIC_WRITE, 0, 0 },
    /* 26*/ { TRUNCATE_EXISTING, FILE_WRITE_DATA, ERROR_INVALID_PARAMETER, 0 }
    };
    char temp_path[MAX_PATH];
    char file_name[MAX_PATH];
    DWORD i, ret, written;
    HANDLE hfile;

    GetTempPathA(MAX_PATH, temp_path);
    GetTempFileNameA(temp_path, "tmp", 0, file_name);

    i = strlen(temp_path);
    if (i && temp_path[i - 1] == '\\') temp_path[i - 1] = 0;

    for (i = 0; i <= 5; i++)
    {
        SetLastError(0xdeadbeef);
        hfile = CreateFileA(temp_path, GENERIC_READ, 0, NULL, i, 0, 0);
        ok(hfile == INVALID_HANDLE_VALUE, "CreateFile should fail\n");
        if (i == 0 || i == 5)
        {
/* FIXME: remove once Wine is fixed */
if (i == 5) todo_wine
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "%d: expected ERROR_INVALID_PARAMETER, got %d\n", i, GetLastError());
else
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "%d: expected ERROR_INVALID_PARAMETER, got %d\n", i, GetLastError());
        }
        else
        {
/* FIXME: remove once Wine is fixed */
if (i == 1) todo_wine
            ok(GetLastError() == ERROR_ACCESS_DENIED, "%d: expected ERROR_ACCESS_DENIED, got %d\n", i, GetLastError());
else
            ok(GetLastError() == ERROR_ACCESS_DENIED, "%d: expected ERROR_ACCESS_DENIED, got %d\n", i, GetLastError());
        }

        SetLastError(0xdeadbeef);
        hfile = CreateFileA(temp_path, GENERIC_WRITE, 0, NULL, i, 0, 0);
        ok(hfile == INVALID_HANDLE_VALUE, "CreateFile should fail\n");
        if (i == 0)
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "%d: expected ERROR_INVALID_PARAMETER, got %d\n", i, GetLastError());
        else
        {
/* FIXME: remove once Wine is fixed */
if (i == 1) todo_wine
            ok(GetLastError() == ERROR_ACCESS_DENIED, "%d: expected ERROR_ACCESS_DENIED, got %d\n", i, GetLastError());
else
            ok(GetLastError() == ERROR_ACCESS_DENIED, "%d: expected ERROR_ACCESS_DENIED, got %d\n", i, GetLastError());
        }
    }

    for (i = 0; i < sizeof(td)/sizeof(td[0]); i++)
    {
        SetLastError(0xdeadbeef);
        hfile = CreateFileA(file_name, td[i].access, 0, NULL, td[i].disposition, 0, 0);
        if (!td[i].error)
        {
            ok(hfile != INVALID_HANDLE_VALUE, "%d: CreateFile error %d\n", i, GetLastError());
            written = 0xdeadbeef;
            SetLastError(0xdeadbeef);
            ret = WriteFile(hfile, &td[i].error, sizeof(td[i].error), &written, NULL);
            if (td[i].access & GENERIC_WRITE)
                ok(ret, "%d: WriteFile error %d\n", i, GetLastError());
            else
            {
                ok(!ret, "%d: WriteFile should fail\n", i);
                ok(GetLastError() == ERROR_ACCESS_DENIED, "%d: expected ERROR_ACCESS_DENIED, got %d\n", i, GetLastError());
            }
            CloseHandle(hfile);
        }
        else
        {
            /* FIXME: remove the condition below once Wine is fixed */
            if (td[i].disposition == TRUNCATE_EXISTING && !(td[i].access & GENERIC_WRITE))
            {
                todo_wine
                {
                ok(hfile == INVALID_HANDLE_VALUE, "%d: CreateFile should fail\n", i);
                ok(GetLastError() == td[i].error, "%d: expected %d, got %d\n", i, td[i].error, GetLastError());
                }
                CloseHandle(hfile);
            }
            else
            {
            ok(hfile == INVALID_HANDLE_VALUE, "%d: CreateFile should fail\n", i);
            ok(GetLastError() == td[i].error, "%d: expected %d, got %d\n", i, td[i].error, GetLastError());
            }
        }

        if (td[i].clean_up) DeleteFileA(file_name);
    }

    DeleteFileA(file_name);
}

static void test_GetFileInformationByHandleEx(void)
{
    int i;
    char tempPath[MAX_PATH], tempFileName[MAX_PATH], buffer[1024], *strPtr;
    BOOL ret;
    DWORD ret2, written;
    HANDLE directory, file;
    FILE_ID_BOTH_DIR_INFO *bothDirInfo;
    FILE_BASIC_INFO *basicInfo;
    FILE_STANDARD_INFO *standardInfo;
    FILE_NAME_INFO *nameInfo;
    LARGE_INTEGER prevWrite;
    FILE_IO_PRIORITY_HINT_INFO priohintinfo;
    FILE_ALLOCATION_INFO allocinfo;
    FILE_DISPOSITION_INFO dispinfo;
    FILE_END_OF_FILE_INFO eofinfo;
    FILE_RENAME_INFO renameinfo;

    struct {
        FILE_INFO_BY_HANDLE_CLASS handleClass;
        void *ptr;
        DWORD size;
        DWORD errorCode;
    } checks[] = {
        {0xdeadbeef, NULL, 0, ERROR_INVALID_PARAMETER},
        {FileIdBothDirectoryInfo, NULL, 0, ERROR_BAD_LENGTH},
        {FileIdBothDirectoryInfo, NULL, sizeof(buffer), ERROR_NOACCESS},
        {FileIdBothDirectoryInfo, buffer, 0, ERROR_BAD_LENGTH}};

    if (!pGetFileInformationByHandleEx)
    {
        win_skip("GetFileInformationByHandleEx is missing.\n");
        return;
    }

    ret2 = GetTempPathA(sizeof(tempPath), tempPath);
    ok(ret2, "GetFileInformationByHandleEx: GetTempPathA failed, got error %u.\n", GetLastError());

    /* ensure the existence of a file in the temp folder */
    ret2 = GetTempFileNameA(tempPath, "abc", 0, tempFileName);
    ok(ret2, "GetFileInformationByHandleEx: GetTempFileNameA failed, got error %u.\n", GetLastError());
    ret2 = GetFileAttributesA(tempFileName);
    ok(ret2 != INVALID_FILE_ATTRIBUTES, "GetFileInformationByHandleEx: "
        "GetFileAttributesA failed to find the temp file, got error %u.\n", GetLastError());

    directory = CreateFileA(tempPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    ok(directory != INVALID_HANDLE_VALUE, "GetFileInformationByHandleEx: failed to open the temp folder, "
        "got error %u.\n", GetLastError());

    for (i = 0; i < sizeof(checks) / sizeof(checks[0]); i += 1)
    {
        SetLastError(0xdeadbeef);
        ret = pGetFileInformationByHandleEx(directory, checks[i].handleClass, checks[i].ptr, checks[i].size);
        ok(!ret && GetLastError() == checks[i].errorCode, "GetFileInformationByHandleEx: expected error %u, "
           "got %u.\n", checks[i].errorCode, GetLastError());
    }

    while (TRUE)
    {
        memset(buffer, 0xff, sizeof(buffer));
        ret = pGetFileInformationByHandleEx(directory, FileIdBothDirectoryInfo, buffer, sizeof(buffer));
        if (!ret && GetLastError() == ERROR_NO_MORE_FILES)
            break;
        ok(ret, "GetFileInformationByHandleEx: failed to query for FileIdBothDirectoryInfo, got error %u.\n", GetLastError());
        if (!ret)
            break;
        bothDirInfo = (FILE_ID_BOTH_DIR_INFO *)buffer;
        while (TRUE)
        {
            ok(bothDirInfo->FileAttributes != 0xffffffff, "GetFileInformationByHandleEx: returned invalid file attributes.\n");
            ok(bothDirInfo->FileId.u.LowPart != 0xffffffff, "GetFileInformationByHandleEx: returned invalid file id.\n");
            ok(bothDirInfo->FileNameLength != 0xffffffff, "GetFileInformationByHandleEx: returned invalid file name length.\n");
            if (!bothDirInfo->NextEntryOffset)
                break;
            bothDirInfo = (FILE_ID_BOTH_DIR_INFO *)(((char *)bothDirInfo) + bothDirInfo->NextEntryOffset);
        }
    }

    CloseHandle(directory);

    file = CreateFileA(tempFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "GetFileInformationByHandleEx: failed to open the temp file, "
        "got error %u.\n", GetLastError());

    /* Test FileBasicInfo; make sure the write time changes when a file is updated */
    memset(buffer, 0xff, sizeof(buffer));
    ret = pGetFileInformationByHandleEx(file, FileBasicInfo, buffer, sizeof(buffer));
    ok(ret, "GetFileInformationByHandleEx: failed to get FileBasicInfo, %u\n", GetLastError());
    basicInfo = (FILE_BASIC_INFO *)buffer;
    prevWrite = basicInfo->LastWriteTime;
    CloseHandle(file);

    Sleep(30); /* Make sure a new write time is different from the previous */

    /* Write something to the file, to make sure the write time has changed */
    file = CreateFileA(tempFileName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "GetFileInformationByHandleEx: failed to open the temp file, "
        "got error %u.\n", GetLastError());
    ret = WriteFile(file, tempFileName, strlen(tempFileName), &written, NULL);
    ok(ret, "GetFileInformationByHandleEx: Write failed\n");
    CloseHandle(file);

    file = CreateFileA(tempFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "GetFileInformationByHandleEx: failed to open the temp file, "
        "got error %u.\n", GetLastError());

    memset(buffer, 0xff, sizeof(buffer));
    ret = pGetFileInformationByHandleEx(file, FileBasicInfo, buffer, sizeof(buffer));
    ok(ret, "GetFileInformationByHandleEx: failed to get FileBasicInfo, %u\n", GetLastError());
    basicInfo = (FILE_BASIC_INFO *)buffer;
    /* Could also check that the creation time didn't change - on windows
     * it doesn't, but on wine, it does change even if it shouldn't. */
    ok(basicInfo->LastWriteTime.QuadPart != prevWrite.QuadPart,
        "GetFileInformationByHandleEx: last write time didn't change\n");

    /* Test FileStandardInfo, check some basic parameters */
    memset(buffer, 0xff, sizeof(buffer));
    ret = pGetFileInformationByHandleEx(file, FileStandardInfo, buffer, sizeof(buffer));
    ok(ret, "GetFileInformationByHandleEx: failed to get FileStandardInfo, %u\n", GetLastError());
    standardInfo = (FILE_STANDARD_INFO *)buffer;
    ok(standardInfo->NumberOfLinks == 1, "GetFileInformationByHandleEx: Unexpcted number of links\n");
    ok(standardInfo->DeletePending == FALSE, "GetFileInformationByHandleEx: Unexpcted pending delete\n");
    ok(standardInfo->Directory == FALSE, "GetFileInformationByHandleEx: Incorrect directory flag\n");

    /* Test FileNameInfo */
    memset(buffer, 0xff, sizeof(buffer));
    ret = pGetFileInformationByHandleEx(file, FileNameInfo, buffer, sizeof(buffer));
    ok(ret, "GetFileInformationByHandleEx: failed to get FileNameInfo, %u\n", GetLastError());
    nameInfo = (FILE_NAME_INFO *)buffer;
    strPtr = strchr(tempFileName, '\\');
    ok(strPtr != NULL, "GetFileInformationByHandleEx: Temp filename didn't contain backslash\n");
    ok(nameInfo->FileNameLength == strlen(strPtr) * 2,
        "GetFileInformationByHandleEx: Incorrect file name length\n");
    for (i = 0; i < nameInfo->FileNameLength/2; i++)
        ok(strPtr[i] == nameInfo->FileName[i], "Incorrect filename char %d: %c vs %c\n",
            i, strPtr[i], nameInfo->FileName[i]);

    /* invalid classes */
    SetLastError(0xdeadbeef);
    ret = pGetFileInformationByHandleEx(file, FileEndOfFileInfo, &eofinfo, sizeof(eofinfo));
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "got %d, error %d\n", ret, GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetFileInformationByHandleEx(file, FileIoPriorityHintInfo, &priohintinfo, sizeof(priohintinfo));
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "got %d, error %d\n", ret, GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetFileInformationByHandleEx(file, FileAllocationInfo, &allocinfo, sizeof(allocinfo));
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "got %d, error %d\n", ret, GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetFileInformationByHandleEx(file, FileDispositionInfo, &dispinfo, sizeof(dispinfo));
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "got %d, error %d\n", ret, GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetFileInformationByHandleEx(file, FileRenameInfo, &renameinfo, sizeof(renameinfo));
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "got %d, error %d\n", ret, GetLastError());

    CloseHandle(file);
    DeleteFileA(tempFileName);
}

static void test_OpenFileById(void)
{
    char tempPath[MAX_PATH], tempFileName[MAX_PATH], buffer[256], tickCount[256];
    WCHAR tempFileNameW[MAX_PATH];
    BOOL ret, found;
    DWORD ret2, count, tempFileNameLen;
    HANDLE directory, handle, tempFile;
    FILE_ID_BOTH_DIR_INFO *bothDirInfo;
    FILE_ID_DESCRIPTOR fileIdDescr;

    if (!pGetFileInformationByHandleEx || !pOpenFileById)
    {
        win_skip("GetFileInformationByHandleEx or OpenFileById is missing.\n");
        return;
    }

    ret2 = GetTempPathA(sizeof(tempPath), tempPath);
    ok(ret2, "OpenFileById: GetTempPath failed, got error %u.\n", GetLastError());

    /* ensure the existence of a file in the temp folder */
    ret2 = GetTempFileNameA(tempPath, "abc", 0, tempFileName);
    ok(ret2, "OpenFileById: GetTempFileNameA failed, got error %u.\n", GetLastError());
    ret2 = GetFileAttributesA(tempFileName);
    ok(ret2 != INVALID_FILE_ATTRIBUTES,
        "OpenFileById: GetFileAttributesA failed to find the temp file, got error %u\n", GetLastError());

    ret2 = MultiByteToWideChar(CP_ACP, 0, tempFileName + strlen(tempPath), -1, tempFileNameW, sizeof(tempFileNameW)/sizeof(tempFileNameW[0]));
    ok(ret2, "OpenFileById: MultiByteToWideChar failed to convert tempFileName, got error %u.\n", GetLastError());
    tempFileNameLen = ret2 - 1;

    tempFile = CreateFileA(tempFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(tempFile != INVALID_HANDLE_VALUE, "OpenFileById: failed to create a temp file, "
	    "got error %u.\n", GetLastError());
    ret2 = sprintf(tickCount, "%u", GetTickCount());
    ret = WriteFile(tempFile, tickCount, ret2, &count, NULL);
    ok(ret, "OpenFileById: WriteFile failed, got error %u.\n", GetLastError());
    CloseHandle(tempFile);

    directory = CreateFileA(tempPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    ok(directory != INVALID_HANDLE_VALUE, "OpenFileById: failed to open the temp folder, "
        "got error %u.\n", GetLastError());

    /* get info about the temp folder itself */
    bothDirInfo = (FILE_ID_BOTH_DIR_INFO *)buffer;
    ret = pGetFileInformationByHandleEx(directory, FileIdBothDirectoryInfo, buffer, sizeof(buffer));
    ok(ret, "OpenFileById: failed to query for FileIdBothDirectoryInfo, got error %u.\n", GetLastError());
    ok(bothDirInfo->FileNameLength == sizeof(WCHAR) && bothDirInfo->FileName[0] == '.',
        "OpenFileById: failed to return the temp folder at the first entry, got error %u.\n", GetLastError());

    /* open the temp folder itself */
    fileIdDescr.dwSize    = sizeof(fileIdDescr);
    fileIdDescr.Type      = FileIdType;
    U(fileIdDescr).FileId = bothDirInfo->FileId;
    handle = pOpenFileById(directory, &fileIdDescr, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, 0);
    todo_wine
    ok(handle != INVALID_HANDLE_VALUE, "OpenFileById: failed to open the temp folder itself, got error %u.\n", GetLastError());
    CloseHandle(handle);

    /* find the temp file in the temp folder */
    found = FALSE;
    while (!found)
    {
        ret = pGetFileInformationByHandleEx(directory, FileIdBothDirectoryInfo, buffer, sizeof(buffer));
        ok(ret, "OpenFileById: failed to query for FileIdBothDirectoryInfo, got error %u.\n", GetLastError());
        if (!ret)
            break;
        bothDirInfo = (FILE_ID_BOTH_DIR_INFO *)buffer;
        while (TRUE)
        {
            if (tempFileNameLen == bothDirInfo->FileNameLength / sizeof(WCHAR) &&
                memcmp(tempFileNameW, bothDirInfo->FileName, bothDirInfo->FileNameLength) == 0)
            {
                found = TRUE;
                break;
            }
            if (!bothDirInfo->NextEntryOffset)
                break;
            bothDirInfo = (FILE_ID_BOTH_DIR_INFO *)(((char *)bothDirInfo) + bothDirInfo->NextEntryOffset);
        }
    }
    ok(found, "OpenFileById: failed to find the temp file in the temp folder.\n");

    SetLastError(0xdeadbeef);
    handle = pOpenFileById(directory, NULL, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, 0);
    ok(handle == INVALID_HANDLE_VALUE && GetLastError() == ERROR_INVALID_PARAMETER,
        "OpenFileById: expected ERROR_INVALID_PARAMETER, got error %u.\n", GetLastError());

    fileIdDescr.dwSize    = sizeof(fileIdDescr);
    fileIdDescr.Type      = FileIdType;
    U(fileIdDescr).FileId = bothDirInfo->FileId;
    handle = pOpenFileById(directory, &fileIdDescr, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, 0);
    ok(handle != INVALID_HANDLE_VALUE, "OpenFileById: failed to open the file, got error %u.\n", GetLastError());

    ret = ReadFile(handle, buffer, sizeof(buffer), &count, NULL);
    buffer[count] = 0;
    ok(ret, "OpenFileById: ReadFile failed, got error %u.\n", GetLastError());
    ok(strcmp(tickCount, buffer) == 0, "OpenFileById: invalid contents of the temp file.\n");

    CloseHandle(handle);
    CloseHandle(directory);
    DeleteFileA(tempFileName);
}

static void test_SetFileValidData(void)
{
    BOOL ret;
    HANDLE handle;
    DWORD error, count;
    char path[MAX_PATH], filename[MAX_PATH];
    TOKEN_PRIVILEGES privs;
    HANDLE token = NULL;

    if (!pSetFileValidData)
    {
        win_skip("SetFileValidData is missing\n");
        return;
    }
    GetTempPathA(sizeof(path), path);
    GetTempFileNameA(path, "tst", 0, filename);
    handle = CreateFileA(filename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    WriteFile(handle, "test", sizeof("test") - 1, &count, NULL);
    CloseHandle(handle);

    SetLastError(0xdeadbeef);
    ret = pSetFileValidData(INVALID_HANDLE_VALUE, 0);
    error = GetLastError();
    ok(!ret, "SetFileValidData succeeded\n");
    ok(error == ERROR_INVALID_HANDLE, "got %u\n", error);

    SetLastError(0xdeadbeef);
    ret = pSetFileValidData(INVALID_HANDLE_VALUE, -1);
    error = GetLastError();
    ok(!ret, "SetFileValidData succeeded\n");
    ok(error == ERROR_INVALID_HANDLE, "got %u\n", error);

    /* file opened for reading */
    handle = CreateFileA(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

    SetLastError(0xdeadbeef);
    ret = pSetFileValidData(handle, 0);
    ok(!ret, "SetFileValidData succeeded\n");
    error = GetLastError();
    ok(error == ERROR_ACCESS_DENIED, "got %u\n", error);

    SetLastError(0xdeadbeef);
    ret = pSetFileValidData(handle, -1);
    error = GetLastError();
    ok(!ret, "SetFileValidData succeeded\n");
    ok(error == ERROR_ACCESS_DENIED, "got %u\n", error);
    CloseHandle(handle);

    handle = CreateFileA(filename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    SetLastError(0xdeadbeef);
    ret = pSetFileValidData(handle, 0);
    error = GetLastError();
    ok(!ret, "SetFileValidData succeeded\n");
    todo_wine ok(error == ERROR_PRIVILEGE_NOT_HELD, "got %u\n", error);
    CloseHandle(handle);

    privs.PrivilegeCount = 1;
    privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token) ||
        !LookupPrivilegeValueA(NULL, SE_MANAGE_VOLUME_NAME, &privs.Privileges[0].Luid) ||
        !AdjustTokenPrivileges(token, FALSE, &privs, sizeof(privs), NULL, NULL) ||
        GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    {
        win_skip("cannot enable SE_MANAGE_VOLUME_NAME privilege\n");
        CloseHandle(token);
        DeleteFileA(filename);
        return;
    }
    handle = CreateFileA(filename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    SetLastError(0xdeadbeef);
    ret = pSetFileValidData(handle, 0);
    error = GetLastError();
    ok(!ret, "SetFileValidData succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "got %u\n", error);

    SetLastError(0xdeadbeef);
    ret = pSetFileValidData(handle, -1);
    error = GetLastError();
    ok(!ret, "SetFileValidData succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "got %u\n", error);

    SetLastError(0xdeadbeef);
    ret = pSetFileValidData(handle, 2);
    error = GetLastError();
    todo_wine ok(!ret, "SetFileValidData succeeded\n");
    todo_wine ok(error == ERROR_INVALID_PARAMETER, "got %u\n", error);

    ret = pSetFileValidData(handle, 4);
    ok(ret, "SetFileValidData failed %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pSetFileValidData(handle, 8);
    error = GetLastError();
    ok(!ret, "SetFileValidData succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "got %u\n", error);

    count = SetFilePointer(handle, 1024, NULL, FILE_END);
    ok(count != INVALID_SET_FILE_POINTER, "SetFilePointer failed %u\n", GetLastError());
    ret = SetEndOfFile(handle);
    ok(ret, "SetEndOfFile failed %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pSetFileValidData(handle, 2);
    error = GetLastError();
    todo_wine ok(!ret, "SetFileValidData succeeded\n");
    todo_wine ok(error == ERROR_INVALID_PARAMETER, "got %u\n", error);

    ret = pSetFileValidData(handle, 4);
    ok(ret, "SetFileValidData failed %u\n", GetLastError());

    ret = pSetFileValidData(handle, 8);
    ok(ret, "SetFileValidData failed %u\n", GetLastError());

    ret = pSetFileValidData(handle, 4);
    error = GetLastError();
    todo_wine ok(!ret, "SetFileValidData succeeded\n");
    todo_wine ok(error == ERROR_INVALID_PARAMETER, "got %u\n", error);

    ret = pSetFileValidData(handle, 1024);
    ok(ret, "SetFileValidData failed %u\n", GetLastError());

    ret = pSetFileValidData(handle, 2048);
    error = GetLastError();
    ok(!ret, "SetFileValidData succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "got %u\n", error);

    privs.Privileges[0].Attributes = 0;
    AdjustTokenPrivileges(token, FALSE, &privs, sizeof(privs), NULL, NULL);

    CloseHandle(token);
    CloseHandle(handle);
    DeleteFileA(filename);
}

static void test_WriteFileGather(void)
{
    char temp_path[MAX_PATH], filename[MAX_PATH];
    HANDLE hfile, hiocp1, hiocp2;
    DWORD ret, size;
    ULONG_PTR key;
    FILE_SEGMENT_ELEMENT fse[2];
    OVERLAPPED ovl, *povl = NULL;
    SYSTEM_INFO si;
    LPVOID buf = NULL;

    ret = GetTempPathA( MAX_PATH, temp_path );
    ok( ret != 0, "GetTempPathA error %d\n", GetLastError() );
    ok( ret < MAX_PATH, "temp path should fit into MAX_PATH\n" );
    ret = GetTempFileNameA( temp_path, "wfg", 0, filename );
    ok( ret != 0, "GetTempFileNameA error %d\n", GetLastError() );

    hfile = CreateFileA( filename, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
                         FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL, 0 );
    ok( hfile != INVALID_HANDLE_VALUE, "CreateFile failed err %u\n", GetLastError() );
    if (hfile == INVALID_HANDLE_VALUE) return;

    hiocp1 = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 999, 0 );
    hiocp2 = CreateIoCompletionPort( hfile, hiocp1, 999, 0 );
    ok( hiocp2 != 0, "CreateIoCompletionPort failed err %u\n", GetLastError() );

    GetSystemInfo( &si );
    buf = VirtualAlloc( NULL, si.dwPageSize, MEM_COMMIT, PAGE_READWRITE );
    ok( buf != NULL, "VirtualAlloc failed err %u\n", GetLastError() );

    memset( &ovl, 0, sizeof(ovl) );
    memset( fse, 0, sizeof(fse) );
    fse[0].Buffer = buf;
    if (!WriteFileGather( hfile, fse, si.dwPageSize, NULL, &ovl ))
        ok( GetLastError() == ERROR_IO_PENDING, "WriteFileGather failed err %u\n", GetLastError() );

    ret = GetQueuedCompletionStatus( hiocp2, &size, &key, &povl, 1000 );
    ok( ret, "GetQueuedCompletionStatus failed err %u\n", GetLastError());
    ok( povl == &ovl, "wrong ovl %p\n", povl );

    memset( &ovl, 0, sizeof(ovl) );
    memset( fse, 0, sizeof(fse) );
    fse[0].Buffer = buf;
    if (!ReadFileScatter( hfile, fse, si.dwPageSize, NULL, &ovl ))
        ok( GetLastError() == ERROR_IO_PENDING, "ReadFileScatter failed err %u\n", GetLastError() );

    ret = GetQueuedCompletionStatus( hiocp2, &size, &key, &povl, 1000 );
    ok( ret, "GetQueuedCompletionStatus failed err %u\n", GetLastError());
    ok( povl == &ovl, "wrong ovl %p\n", povl );

    CloseHandle( hfile );
    CloseHandle( hiocp1 );
    CloseHandle( hiocp2 );
    VirtualFree( buf, 0, MEM_RELEASE );
    DeleteFileA( filename );
}

static unsigned file_map_access(unsigned access)
{
    if (access & GENERIC_READ)    access |= FILE_GENERIC_READ;
    if (access & GENERIC_WRITE)   access |= FILE_GENERIC_WRITE;
    if (access & GENERIC_EXECUTE) access |= FILE_GENERIC_EXECUTE;
    if (access & GENERIC_ALL)     access |= FILE_ALL_ACCESS;
    return access & ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

static BOOL is_access_compatible(unsigned obj_access, unsigned desired_access)
{
    obj_access = file_map_access(obj_access);
    desired_access = file_map_access(desired_access);
    return (obj_access & desired_access) == desired_access;
}

static void test_file_access(void)
{
    static const struct
    {
        unsigned access, create_error, write_error, read_error;
    } td[] =
    {
        { GENERIC_READ | GENERIC_WRITE, 0, 0, 0 },
        { GENERIC_WRITE, 0, 0, ERROR_ACCESS_DENIED },
        { GENERIC_READ, 0, ERROR_ACCESS_DENIED, 0 },
        { FILE_READ_DATA | FILE_WRITE_DATA, 0, 0, 0 },
        { FILE_WRITE_DATA, 0, 0, ERROR_ACCESS_DENIED },
        { FILE_READ_DATA, 0, ERROR_ACCESS_DENIED, 0 },
        { FILE_APPEND_DATA, 0, 0, ERROR_ACCESS_DENIED },
        { FILE_READ_DATA | FILE_APPEND_DATA, 0, 0, 0 },
        { FILE_WRITE_DATA | FILE_APPEND_DATA, 0, 0, ERROR_ACCESS_DENIED },
        { 0, 0, ERROR_ACCESS_DENIED, ERROR_ACCESS_DENIED },
    };
    char path[MAX_PATH], fname[MAX_PATH];
    unsigned char buf[16];
    HANDLE hfile, hdup;
    DWORD i, j, ret, bytes;

    GetTempPathA(MAX_PATH, path);
    GetTempFileNameA(path, "foo", 0, fname);

    for (i = 0; i < sizeof(td)/sizeof(td[0]); i++)
    {
        SetLastError(0xdeadbeef);
        hfile = CreateFileA(fname, td[i].access, 0, NULL, CREATE_ALWAYS,
                           FILE_FLAG_DELETE_ON_CLOSE, 0);
        if (td[i].create_error)
        {
            ok(hfile == INVALID_HANDLE_VALUE, "%d: CreateFile should fail\n", i);
            ok(td[i].create_error == GetLastError(), "%d: expected %d, got %d\n", i, td[i].create_error, GetLastError());
            continue;
        }
        else
            ok(hfile != INVALID_HANDLE_VALUE, "%d: CreateFile error %d\n", i, GetLastError());

        for (j = 0; j < sizeof(td)/sizeof(td[0]); j++)
        {
            SetLastError(0xdeadbeef);
            ret = DuplicateHandle(GetCurrentProcess(), hfile, GetCurrentProcess(), &hdup,
                                  td[j].access, 0, 0);
            if (is_access_compatible(td[i].access, td[j].access))
                ok(ret, "DuplicateHandle(%#x => %#x) error %d\n", td[i].access, td[j].access, GetLastError());
            else
            {
                /* FIXME: Remove once Wine is fixed */
                if ((td[j].access & (GENERIC_READ | GENERIC_WRITE)) ||
                    (!(td[i].access & (GENERIC_WRITE | FILE_WRITE_DATA)) && (td[j].access & FILE_WRITE_DATA)) ||
                    (!(td[i].access & (GENERIC_READ | FILE_READ_DATA)) && (td[j].access & FILE_READ_DATA)) ||
                    (!(td[i].access & (GENERIC_WRITE)) && (td[j].access & FILE_APPEND_DATA)))
                {
todo_wine
                ok(!ret, "DuplicateHandle(%#x => %#x) should fail\n", td[i].access, td[j].access);
todo_wine
                ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());
                }
                else
                {
                ok(!ret, "DuplicateHandle(%#x => %#x) should fail\n", td[i].access, td[j].access);
                ok(GetLastError() == ERROR_ACCESS_DENIED, "expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());
                }
            }
            if (ret) CloseHandle(hdup);
        }

        SetLastError(0xdeadbeef);
        bytes = 0xdeadbeef;
        ret = WriteFile(hfile, "\x5e\xa7", 2, &bytes, NULL);
        if (td[i].write_error)
        {
            ok(!ret, "%d: WriteFile should fail\n", i);
            ok(td[i].write_error == GetLastError(), "%d: expected %d, got %d\n", i, td[i].write_error, GetLastError());
            ok(bytes == 0, "%d: expected 0, got %u\n", i, bytes);
        }
        else
        {
            ok(ret, "%d: WriteFile error %d\n", i, GetLastError());
            ok(bytes == 2, "%d: expected 2, got %u\n", i, bytes);
        }

        SetLastError(0xdeadbeef);
        ret = SetFilePointer(hfile, 0, NULL, FILE_BEGIN);
        ok(ret != INVALID_SET_FILE_POINTER, "SetFilePointer error %d\n", GetLastError());

        SetLastError(0xdeadbeef);
        bytes = 0xdeadbeef;
        ret = ReadFile(hfile, buf, sizeof(buf), &bytes, NULL);
        if (td[i].read_error)
        {
            ok(!ret, "%d: ReadFile should fail\n", i);
            ok(td[i].read_error == GetLastError(), "%d: expected %d, got %d\n", i, td[i].read_error, GetLastError());
            ok(bytes == 0, "%d: expected 0, got %u\n", i, bytes);
        }
        else
        {
            ok(ret, "%d: ReadFile error %d\n", i, GetLastError());
            if (td[i].write_error)
                ok(bytes == 0, "%d: expected 0, got %u\n", i, bytes);
            else
            {
                ok(bytes == 2, "%d: expected 2, got %u\n", i, bytes);
                ok(buf[0] == 0x5e && buf[1] == 0xa7, "%d: expected 5ea7, got %02x%02x\n", i, buf[0], buf[1]);
            }
        }

        CloseHandle(hfile);
    }
}

static void test_GetFinalPathNameByHandleA(void)
{
    static char prefix[] = "GetFinalPathNameByHandleA";
    static char dos_prefix[] = "\\\\?\\";
    char temp_path[MAX_PATH], test_path[MAX_PATH];
    char long_path[MAX_PATH], result_path[MAX_PATH];
    char dos_path[MAX_PATH + sizeof(dos_prefix)];
    HANDLE file;
    DWORD count;
    UINT ret;

    if (!pGetFinalPathNameByHandleA)
    {
        skip("GetFinalPathNameByHandleA is missing\n");
        return;
    }

    /* Test calling with INVALID_HANDLE_VALUE */
    SetLastError(0xdeadbeaf);
    count = pGetFinalPathNameByHandleA(INVALID_HANDLE_VALUE, result_path, MAX_PATH, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    ok(count == 0, "Expected length 0, got %u\n", count);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %u\n", GetLastError());

    count = GetTempPathA(MAX_PATH, temp_path);
    ok(count, "Failed to get temp path, error %u\n", GetLastError());
    ret = GetTempFileNameA(temp_path, prefix, 0, test_path);
    ok(ret != 0, "GetTempFileNameA error %u\n", GetLastError());
    ret = GetLongPathNameA(test_path, long_path, MAX_PATH);
    ok(ret != 0, "GetLongPathNameA error %u\n", GetLastError());
    strcpy(dos_path, dos_prefix);
    strcat(dos_path, long_path);

    file = CreateFileA(test_path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                       CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA error %u\n", GetLastError());

    /* Test VOLUME_NAME_DOS with sufficient buffer size */
    memset(result_path, 0x11, sizeof(result_path));
    count = pGetFinalPathNameByHandleA(file, result_path, MAX_PATH, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    ok(count == strlen(dos_path), "Expected length %u, got %u\n", (DWORD)strlen(dos_path), count);
    ok(lstrcmpiA(dos_path, result_path) == 0, "Expected %s, got %s\n", dos_path, result_path);

    /* Test VOLUME_NAME_DOS with insufficient buffer size */
    memset(result_path, 0x11, sizeof(result_path));
    count = pGetFinalPathNameByHandleA(file, result_path, strlen(dos_path)-2, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    ok(count == strlen(dos_path), "Expected length %u, got %u\n", (DWORD)strlen(dos_path), count);
    ok(result_path[0] == 0x11, "Result path was modified\n");

    memset(result_path, 0x11, sizeof(result_path));
    count = pGetFinalPathNameByHandleA(file, result_path, strlen(dos_path)-1, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    ok(count == strlen(dos_path), "Expected length %u, got %u\n", (DWORD)strlen(dos_path), count);
    ok(result_path[0] == 0x11, "Result path was modified\n");

    memset(result_path, 0x11, sizeof(result_path));
    count = pGetFinalPathNameByHandleA(file, result_path, strlen(dos_path), FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    ok(count == strlen(dos_path), "Expected length %u, got %u\n", (DWORD)strlen(dos_path), count);
    ok(result_path[0] == 0x11, "Result path was modified\n");

    memset(result_path, 0x11, sizeof(result_path));
    count = pGetFinalPathNameByHandleA(file, result_path, strlen(dos_path)+1, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    ok(count == strlen(dos_path), "Expected length %u, got %u\n", (DWORD)strlen(dos_path), count);
    ok(result_path[0] != 0x11, "Result path was not modified\n");
    ok(!result_path[strlen(dos_path)], "Expected nullterminated string\n");
    ok(result_path[strlen(dos_path)+1] == 0x11, "Buffer overflow\n");

    CloseHandle(file);
}

static void test_GetFinalPathNameByHandleW(void)
{
    static WCHAR prefix[] = {'G','e','t','F','i','n','a','l','P','a','t','h',
                             'N','a','m','e','B','y','H','a','n','d','l','e','W','\0'};
    static WCHAR dos_prefix[] = {'\\','\\','?','\\','\0'};
    WCHAR temp_path[MAX_PATH], test_path[MAX_PATH];
    WCHAR long_path[MAX_PATH], result_path[MAX_PATH];
    WCHAR dos_path[MAX_PATH + sizeof(dos_prefix)];
    WCHAR drive_part[MAX_PATH];
    WCHAR *file_part;
    WCHAR volume_path[MAX_PATH + 50];
    WCHAR nt_path[2 * MAX_PATH];
    BOOL success;
    HANDLE file;
    DWORD count;
    UINT ret;

    if (!pGetFinalPathNameByHandleW)
    {
        skip("GetFinalPathNameByHandleW is missing\n");
        return;
    }

    /* Test calling with INVALID_HANDLE_VALUE */
    SetLastError(0xdeadbeaf);
    count = pGetFinalPathNameByHandleW(INVALID_HANDLE_VALUE, result_path, MAX_PATH, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    ok(count == 0, "Expected length 0, got %u\n", count);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %u\n", GetLastError());

    count = GetTempPathW(MAX_PATH, temp_path);
    ok(count, "Failed to get temp path, error %u\n", GetLastError());
    ret = GetTempFileNameW(temp_path, prefix, 0, test_path);
    ok(ret != 0, "GetTempFileNameW error %u\n", GetLastError());
    ret = GetLongPathNameW(test_path, long_path, MAX_PATH);
    ok(ret != 0, "GetLongPathNameW error %u\n", GetLastError());
    lstrcpyW(dos_path, dos_prefix);
    lstrcatW(dos_path, long_path);

    file = CreateFileW(test_path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                       CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileW error %u\n", GetLastError());

    /* Test VOLUME_NAME_DOS with sufficient buffer size */
    memset(result_path, 0x11, sizeof(result_path));
    count = pGetFinalPathNameByHandleW(file, result_path, MAX_PATH, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    ok(count == lstrlenW(dos_path), "Expected length %u, got %u\n", lstrlenW(dos_path), count);
    ok(lstrcmpiW(dos_path, result_path) == 0, "Expected %s, got %s\n", wine_dbgstr_w(dos_path), wine_dbgstr_w(result_path));

    /* Test VOLUME_NAME_DOS with insufficient buffer size */
    memset(result_path, 0x11, sizeof(result_path));
    count = pGetFinalPathNameByHandleW(file, result_path, lstrlenW(dos_path)-1, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    ok(count == lstrlenW(dos_path) + 1, "Expected length %u, got %u\n", lstrlenW(dos_path) + 1, count);
    ok(result_path[0] == 0x1111, "Result path was modified\n");

    memset(result_path, 0x11, sizeof(result_path));
    count = pGetFinalPathNameByHandleW(file, result_path, lstrlenW(dos_path), FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    ok(count == lstrlenW(dos_path) + 1, "Expected length %u, got %u\n", lstrlenW(dos_path) + 1, count);
    ok(result_path[0] == 0x1111, "Result path was modified\n");

    memset(result_path, 0x11, sizeof(result_path));
    count = pGetFinalPathNameByHandleW(file, result_path, lstrlenW(dos_path)+1, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
    ok(count == lstrlenW(dos_path), "Expected length %u, got %u\n", lstrlenW(dos_path), count);
    ok(result_path[0] != 0x1111, "Result path was not modified\n");
    ok(!result_path[lstrlenW(dos_path)], "Expected nullterminated string\n");
    ok(result_path[lstrlenW(dos_path)+1] == 0x1111, "Buffer overflow\n");

    success = GetVolumePathNameW(long_path, drive_part, MAX_PATH);
    ok(success, "GetVolumePathNameW error %u\n", GetLastError());
    success = GetVolumeNameForVolumeMountPointW(drive_part, volume_path, sizeof(volume_path) / sizeof(WCHAR));
    ok(success, "GetVolumeNameForVolumeMountPointW error %u\n", GetLastError());

    /* Test for VOLUME_NAME_GUID */
    lstrcatW(volume_path, long_path + lstrlenW(drive_part));
    memset(result_path, 0x11, sizeof(result_path));
    count = pGetFinalPathNameByHandleW(file, result_path, MAX_PATH, FILE_NAME_NORMALIZED | VOLUME_NAME_GUID);
    ok(count == lstrlenW(volume_path), "Expected length %u, got %u\n", lstrlenW(volume_path), count);
    ok(lstrcmpiW(volume_path, result_path) == 0, "Expected %s, got %s\n",
       wine_dbgstr_w(volume_path), wine_dbgstr_w(result_path));

    /* Test for VOLUME_NAME_NONE */
    file_part = long_path + lstrlenW(drive_part) - 1;
    memset(result_path, 0x11, sizeof(result_path));
    count = pGetFinalPathNameByHandleW(file, result_path, MAX_PATH, FILE_NAME_NORMALIZED | VOLUME_NAME_NONE);
    ok(count == lstrlenW(file_part), "Expected length %u, got %u\n", lstrlenW(file_part), count);
    ok(lstrcmpiW(file_part, result_path) == 0, "Expected %s, got %s\n",
       wine_dbgstr_w(file_part), wine_dbgstr_w(result_path));

    drive_part[lstrlenW(drive_part)-1] = 0;
    success = QueryDosDeviceW(drive_part, nt_path, sizeof(nt_path) / sizeof(WCHAR));
    ok(success, "QueryDosDeviceW error %u\n", GetLastError());

    /* Test for VOLUME_NAME_NT */
    lstrcatW(nt_path, file_part);
    memset(result_path, 0x11, sizeof(result_path));
    count = pGetFinalPathNameByHandleW(file, result_path, MAX_PATH, FILE_NAME_NORMALIZED | VOLUME_NAME_NT);
    ok(count == lstrlenW(nt_path), "Expected length %u, got %u\n", lstrlenW(nt_path), count);
    ok(lstrcmpiW(nt_path, result_path) == 0, "Expected %s, got %s\n",
       wine_dbgstr_w(nt_path), wine_dbgstr_w(result_path));

    CloseHandle(file);
}

static void test_SetFileInformationByHandle(void)
{
    FILE_ATTRIBUTE_TAG_INFO fileattrinfo = { 0 };
    FILE_REMOTE_PROTOCOL_INFO protinfo = { 0 };
    FILE_STANDARD_INFO stdinfo;
    FILE_COMPRESSION_INFO compressinfo;
    FILE_DISPOSITION_INFO dispinfo;
    char tempFileName[MAX_PATH];
    char tempPath[MAX_PATH];
    HANDLE file;
    BOOL ret;

    if (!pSetFileInformationByHandle)
    {
        win_skip("SetFileInformationByHandle is not supported\n");
        return;
    }

    ret = GetTempPathA(sizeof(tempPath), tempPath);
    ok(ret, "GetTempPathA failed, got error %u.\n", GetLastError());

    /* ensure the existence of a file in the temp folder */
    ret = GetTempFileNameA(tempPath, "abc", 0, tempFileName);
    ok(ret, "GetTempFileNameA failed, got error %u.\n", GetLastError());

    file = CreateFileA(tempFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, NULL);
    ok(file != INVALID_HANDLE_VALUE, "failed to open the temp file, error %u.\n", GetLastError());

    /* invalid classes */
    SetLastError(0xdeadbeef);
    ret = pSetFileInformationByHandle(file, FileStandardInfo, &stdinfo, sizeof(stdinfo));
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "got %d, error %d\n", ret, GetLastError());

    memset(&compressinfo, 0, sizeof(compressinfo));
    SetLastError(0xdeadbeef);
    ret = pSetFileInformationByHandle(file, FileCompressionInfo, &compressinfo, sizeof(compressinfo));
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "got %d, error %d\n", ret, GetLastError());

    SetLastError(0xdeadbeef);
    ret = pSetFileInformationByHandle(file, FileAttributeTagInfo, &fileattrinfo, sizeof(fileattrinfo));
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "got %d, error %d\n", ret, GetLastError());

    memset(&protinfo, 0, sizeof(protinfo));
    protinfo.StructureVersion = 1;
    protinfo.StructureSize = sizeof(protinfo);
    SetLastError(0xdeadbeef);
    ret = pSetFileInformationByHandle(file, FileRemoteProtocolInfo, &protinfo, sizeof(protinfo));
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "got %d, error %d\n", ret, GetLastError());

    /* test FileDispositionInfo, additional details already covered by ntdll tests */
    SetLastError(0xdeadbeef);
    ret = pSetFileInformationByHandle(file, FileDispositionInfo, &dispinfo, 0);
todo_wine
    ok(!ret && GetLastError() == ERROR_BAD_LENGTH, "got %d, error %d\n", ret, GetLastError());

    dispinfo.DeleteFile = TRUE;
    ret = pSetFileInformationByHandle(file, FileDispositionInfo, &dispinfo, sizeof(dispinfo));
    ok(ret, "setting FileDispositionInfo failed, error %d\n", GetLastError());

    CloseHandle(file);
}

static void test_GetFileAttributesExW(void)
{
    static const WCHAR path1[] = {'\\','\\','?','\\',0};
    static const WCHAR path2[] = {'\\','?','?','\\',0};
    static const WCHAR path3[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\',0};
    WIN32_FILE_ATTRIBUTE_DATA info;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = GetFileAttributesExW(path1, GetFileExInfoStandard, &info);
    ok(!ret, "GetFileAttributesExW succeeded\n");
    ok(GetLastError() == ERROR_INVALID_NAME, "Expected error ERROR_INVALID_NAME, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetFileAttributesExW(path2, GetFileExInfoStandard, &info);
    ok(!ret, "GetFileAttributesExW succeeded\n");
    ok(GetLastError() == ERROR_INVALID_NAME, "Expected error ERROR_INVALID_NAME, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetFileAttributesExW(path3, GetFileExInfoStandard, &info);
    ok(!ret, "GetFileAttributesExW succeeded\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Expected error ERROR_FILE_NOT_FOUND, got %u\n", GetLastError());
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
    test_CopyFile2();
    test_CopyFileEx();
    test_CreateFile();
    test_CreateFileA();
    test_CreateFileW();
    test_CreateFile2();
    test_DeleteFileA();
    test_DeleteFileW();
    test_MoveFileA();
    test_MoveFileW();
    test_FindFirstFileA();
    test_FindNextFileA();
    test_FindFirstFileExA(FindExInfoStandard, 0, 0);
    test_FindFirstFileExA(FindExInfoStandard, 0, FIND_FIRST_EX_CASE_SENSITIVE);
    test_FindFirstFileExA(FindExInfoStandard, 0, FIND_FIRST_EX_LARGE_FETCH);
    test_FindFirstFileExA(FindExInfoBasic, 0, 0);
    /* FindExLimitToDirectories is ignored if the file system doesn't support directory filtering */
    test_FindFirstFileExA(FindExInfoStandard, FindExSearchLimitToDirectories, 0);
    test_FindFirstFileExA(FindExInfoStandard, FindExSearchLimitToDirectories, FIND_FIRST_EX_CASE_SENSITIVE);
    test_FindFirstFileExA(FindExInfoStandard, FindExSearchLimitToDirectories, FIND_FIRST_EX_LARGE_FETCH);
    test_FindFirstFileExA(FindExInfoBasic, FindExSearchLimitToDirectories, 0);
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
    test_GetFileInformationByHandleEx();
    test_OpenFileById();
    test_SetFileValidData();
    test_WriteFileGather();
    test_file_access();
    test_GetFinalPathNameByHandleA();
    test_GetFinalPathNameByHandleW();
    test_SetFileInformationByHandle();
    test_GetFileAttributesExW();
}
