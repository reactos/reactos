/*
 * Unit tests for profile functions
 *
 * Copyright (c) 2003 Stefan Leichter
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

#include <stdarg.h>
#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "windows.h"

#define KEY      "ProfileInt"
#define SECTION  "Test"
#define TESTFILE ".\\testwine.ini"
#define TESTFILE2 ".\\testwine2.ini"

struct _profileInt { 
    LPCSTR section;
    LPCSTR key;
    LPCSTR value;
    LPCSTR iniFile;
    INT defaultVal;
    UINT result;
    UINT result9x;
};

static void test_profile_int(void)
{
    struct _profileInt profileInt[]={
         { NULL,    NULL, NULL,          NULL,     70, 0          , 0}, /*  0 */
         { NULL,    NULL, NULL,          TESTFILE, -1, 4294967295U, 0},
         { NULL,    NULL, NULL,          TESTFILE,  1, 1          , 0},
         { SECTION, NULL, NULL,          TESTFILE, -1, 4294967295U, 0},
         { SECTION, NULL, NULL,          TESTFILE,  1, 1          , 0},
         { NULL,    KEY,  NULL,          TESTFILE, -1, 4294967295U, 0}, /*  5 */
         { NULL,    KEY,  NULL,          TESTFILE,  1, 1          , 0},
         { SECTION, KEY,  NULL,          TESTFILE, -1, 4294967295U, 4294967295U},
         { SECTION, KEY,  NULL,          TESTFILE,  1, 1          , 1},
         { SECTION, KEY,  "-1",          TESTFILE, -1, 4294967295U, 4294967295U},
         { SECTION, KEY,  "-1",          TESTFILE,  1, 4294967295U, 4294967295U}, /* 10 */
         { SECTION, KEY,  "1",           TESTFILE, -1, 1          , 1},
         { SECTION, KEY,  "1",           TESTFILE,  1, 1          , 1},
         { SECTION, KEY,  "+1",          TESTFILE, -1, 1          , 0},
         { SECTION, KEY,  "+1",          TESTFILE,  1, 1          , 0},
         { SECTION, KEY,  "4294967296",  TESTFILE, -1, 0          , 0}, /* 15 */
         { SECTION, KEY,  "4294967296",  TESTFILE,  1, 0          , 0},
         { SECTION, KEY,  "4294967297",  TESTFILE, -1, 1          , 1},
         { SECTION, KEY,  "4294967297",  TESTFILE,  1, 1          , 1},
         { SECTION, KEY,  "-4294967297", TESTFILE, -1, 4294967295U, 4294967295U},
         { SECTION, KEY,  "-4294967297", TESTFILE,  1, 4294967295U, 4294967295U}, /* 20 */
         { SECTION, KEY,  "42A94967297", TESTFILE, -1, 42         , 42},
         { SECTION, KEY,  "42A94967297", TESTFILE,  1, 42         , 42},
         { SECTION, KEY,  "B4294967297", TESTFILE, -1, 0          , 0},
         { SECTION, KEY,  "B4294967297", TESTFILE,  1, 0          , 0},
    };
    int i, num_test = (sizeof(profileInt)/sizeof(struct _profileInt));
    UINT res;

    DeleteFileA( TESTFILE);

    for (i=0; i < num_test; i++) {
        if (profileInt[i].value)
            WritePrivateProfileStringA(SECTION, KEY, profileInt[i].value, 
                                      profileInt[i].iniFile);

       res = GetPrivateProfileIntA(profileInt[i].section, profileInt[i].key, 
                 profileInt[i].defaultVal, profileInt[i].iniFile); 
       ok((res == profileInt[i].result) || (res == profileInt[i].result9x),
	    "test<%02d>: ret<%010u> exp<%010u><%010u>\n",
            i, res, profileInt[i].result, profileInt[i].result9x);
    }

    DeleteFileA( TESTFILE);
}

static void test_profile_string(void)
{
    static WCHAR emptyW[] = { 0 }; /* if "const", GetPrivateProfileStringW(emptyW, ...) crashes on win2k */
    static const WCHAR keyW[] = { 'k','e','y',0 };
    static const WCHAR sW[] = { 's',0 };
    static const WCHAR TESTFILE2W[] = {'.','\\','t','e','s','t','w','i','n','e','2','.','i','n','i',0};
    static const WCHAR valsectionW[] = {'v','a','l','_','e','_','s','e','c','t','i','o','n',0 };
    static const WCHAR valnokeyW[] = {'v','a','l','_','n','o','_','k','e','y',0};
    HANDLE h;
    int ret;
    DWORD count;
    char buf[100];
    WCHAR bufW[100];
    char *p;
    /* test that lines without an '=' will not be enumerated */
    /* in the case below, name2 is a key while name3 is not. */
    char content[]="[s]\r\nname1=val1\r\nname2=\r\nname3\r\nname4=val4\r\n";
    char content2[]="\r\nkey=val_no_section\r\n[]\r\nkey=val_e_section\r\n"
                    "[s]\r\n=val_no_key\r\n[t]\r\n";
    DeleteFileA( TESTFILE2);
    h = CreateFileA( TESTFILE2, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);
    ok( h != INVALID_HANDLE_VALUE, " cannot create %s\n", TESTFILE2);
    if( h == INVALID_HANDLE_VALUE) return;
    WriteFile( h, content, sizeof(content), &count, NULL);
    CloseHandle( h);

    /* enumerate the keys */
    ret=GetPrivateProfileStringA( "s", NULL, "", buf, sizeof(buf),
        TESTFILE2);
    for( p = buf + strlen(buf) + 1; *p;p += strlen(p)+1) 
        p[-1] = ',';
    /* and test */
    ok( ret == 18 && !strcmp( buf, "name1,name2,name4"), "wrong keys returned(%d): %s\n", ret,
            buf);

    /* add a new key to test that the file is quite usable */
    WritePrivateProfileStringA( "s", "name5", "val5", TESTFILE2); 
    ret=GetPrivateProfileStringA( "s", NULL, "", buf, sizeof(buf),
        TESTFILE2);
    for( p = buf + strlen(buf) + 1; *p;p += strlen(p)+1) 
        p[-1] = ',';
    ok( ret == 24 && !strcmp( buf, "name1,name2,name4,name5"), "wrong keys returned(%d): %s\n",
            ret, buf);

    h = CreateFileA( TESTFILE2, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);
    ok( h != INVALID_HANDLE_VALUE, " cannot create %s\n", TESTFILE2);
    if( h == INVALID_HANDLE_VALUE) return;
    WriteFile( h, content2, sizeof(content2), &count, NULL);
    CloseHandle( h);

    /* works only in unicode, ascii crashes */
    ret=GetPrivateProfileStringW(emptyW, keyW, emptyW, bufW,
                                 sizeof(bufW)/sizeof(bufW[0]), TESTFILE2W);
    todo_wine
    ok(ret == 13, "expected 13, got %u\n", ret);
    todo_wine
    ok(!lstrcmpW(valsectionW,bufW), "expected %s, got %s\n",
        wine_dbgstr_w(valsectionW), wine_dbgstr_w(bufW) );

    /* works only in unicode, ascii crashes */
    ret=GetPrivateProfileStringW(sW, emptyW, emptyW, bufW,
                                 sizeof(bufW)/sizeof(bufW[0]), TESTFILE2W);
    ok(ret == 10, "expected 10, got %u\n", ret);
    ok(!lstrcmpW(valnokeyW,bufW), "expected %s, got %s\n",
        wine_dbgstr_w(valnokeyW), wine_dbgstr_w(bufW) );

    DeleteFileA( TESTFILE2);
}

static void test_profile_sections(void)
{
    HANDLE h;
    int ret;
    DWORD count;
    char buf[100];
    char *p;
    static const char content[]="[section1]\r\nname1=val1\r\nname2=\r\nname3\r\nname4=val4\r\n[section2]\r\n";
    static const char testfile4[]=".\\testwine4.ini";
    BOOL on_win98 = FALSE;

    DeleteFileA( testfile4 );
    h = CreateFileA( testfile4, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok( h != INVALID_HANDLE_VALUE, " cannot create %s\n", testfile4);
    if( h == INVALID_HANDLE_VALUE) return;
    WriteFile( h, content, sizeof(content), &count, NULL);
    CloseHandle( h);

    /* Some parameter checking */
    SetLastError(0xdeadbeef);
    ret = GetPrivateProfileSectionA( NULL, NULL, 0, NULL );
    ok( ret == 0, "expected return size 0, got %d\n", ret );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == 0xdeadbeef /* Win98 */,
        "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    if (GetLastError() == 0xdeadbeef) on_win98 = TRUE;

    SetLastError(0xdeadbeef);
    ret = GetPrivateProfileSectionA( NULL, NULL, 0, testfile4 );
    ok( ret == 0, "expected return size 0, got %d\n", ret );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        GetLastError() == 0xdeadbeef /* Win98 */,
        "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    if (!on_win98)
    {
        SetLastError(0xdeadbeef);
        ret = GetPrivateProfileSectionA( "section1", NULL, 0, testfile4 );
        ok( ret == 0, "expected return size 0, got %d\n", ret );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    ret = GetPrivateProfileSectionA( NULL, buf, sizeof(buf), testfile4 );
    ok( ret == 0, "expected return size 0, got %d\n", ret );
    ok( GetLastError() == ERROR_INVALID_PARAMETER ||
        broken(GetLastError() == 0xdeadbeef), /* Win9x, WinME */
        "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetPrivateProfileSectionA( "section1", buf, sizeof(buf), NULL );
    ok( ret == 0, "expected return size 0, got %d\n", ret );
    todo_wine
    ok( GetLastError() == ERROR_FILE_NOT_FOUND ||
        broken(GetLastError() == 0xdeadbeef), /* Win9x, WinME */
        "expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());

    /* Existing empty section with no keys */
    SetLastError(0xdeadbeef);
    ret=GetPrivateProfileSectionA("section2", buf, sizeof(buf), testfile4);
    ok( ret == 0, "expected return size 0, got %d\n", ret );
    ok( GetLastError() == ERROR_SUCCESS ||
        broken(GetLastError() == 0xdeadbeef), /* Win9x, WinME */
        "expected ERROR_SUCCESS, got %d\n", GetLastError());

    /* Existing section with keys and values*/
    SetLastError(0xdeadbeef);
    ret=GetPrivateProfileSectionA("section1", buf, sizeof(buf), testfile4);
    for( p = buf + strlen(buf) + 1; *p;p += strlen(p)+1)
        p[-1] = ',';
    ok( ret == 35 && !strcmp( buf, "name1=val1,name2=,name3,name4=val4"), "wrong section returned(%d): %s\n",
            ret, buf);
    ok( buf[ret-1] == 0 && buf[ret] == 0, "returned buffer not terminated with double-null\n" );
    ok( GetLastError() == ERROR_SUCCESS ||
        broken(GetLastError() == 0xdeadbeef), /* Win9x, WinME */
        "expected ERROR_SUCCESS, got %d\n", GetLastError());

    /* Overflow*/
    ret=GetPrivateProfileSectionA("section1", buf, 24, testfile4);
    for( p = buf + strlen(buf) + 1; *p;p += strlen(p)+1)
        p[-1] = ',';
    ok( ret == 22 && !strcmp( buf, "name1=val1,name2=,name"), "wrong section returned(%d): %s\n",
        ret, buf);
    ok( buf[ret] == 0 && buf[ret+1] == 0, "returned buffer not terminated with double-null\n" );

    DeleteFileA( testfile4 );
}

static void test_profile_sections_names(void)
{
    HANDLE h;
    int ret;
    DWORD count;
    char buf[100];
    WCHAR bufW[100];
    static const char content[]="[section1]\r\n[section2]\r\n[section3]\r\n";
    static const char testfile3[]=".\\testwine3.ini";
    static const WCHAR testfile3W[]={ '.','\\','t','e','s','t','w','i','n','e','3','.','i','n','i',0 };
    static const WCHAR not_here[] = {'.','\\','n','o','t','_','h','e','r','e','.','i','n','i',0};
    DeleteFileA( testfile3 );
    h = CreateFileA( testfile3, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);
    ok( h != INVALID_HANDLE_VALUE, " cannot create %s\n", testfile3);
    if( h == INVALID_HANDLE_VALUE) return;
    WriteFile( h, content, sizeof(content), &count, NULL);
    CloseHandle( h);

    /* Test with sufficiently large buffer */
    memset(buf, 0xc, sizeof(buf));
    ret = GetPrivateProfileSectionNamesA( buf, 29, testfile3 );
    ok( ret == 27 ||
        broken(ret == 28), /* Win9x, WinME */
        "expected return size 27, got %d\n", ret );
    ok( (buf[ret-1] == 0 && buf[ret] == 0) ||
        broken(buf[ret-1] == 0 && buf[ret-2] == 0), /* Win9x, WinME */
        "returned buffer not terminated with double-null\n" );

    /* Test with exactly fitting buffer */
    memset(buf, 0xc, sizeof(buf));
    ret = GetPrivateProfileSectionNamesA( buf, 28, testfile3 );
    ok( ret == 26 ||
        broken(ret == 28), /* Win9x, WinME */
        "expected return size 26, got %d\n", ret );
    todo_wine
    ok( (buf[ret+1] == 0 && buf[ret] == 0) || /* W2K3 and higher */
        broken(buf[ret+1] == 0xc && buf[ret] == 0) || /* NT4, W2K, WinXP */
        broken(buf[ret-1] == 0 && buf[ret-2] == 0), /* Win9x, WinME */
        "returned buffer not terminated with double-null\n" );

    /* Test with a buffer too small */
    memset(buf, 0xc, sizeof(buf));
    ret = GetPrivateProfileSectionNamesA( buf, 27, testfile3 );
    ok( ret == 25, "expected return size 25, got %d\n", ret );
    /* Win9x and WinME only fills the buffer with complete section names (double-null terminated) */
    count = strlen("section1") + sizeof(CHAR) + strlen("section2");
    todo_wine
    ok( (buf[ret+1] == 0 && buf[ret] == 0) ||
        broken(buf[count] == 0 && buf[count+1] == 0), /* Win9x, WinME */
        "returned buffer not terminated with double-null\n" );

    /* Tests on nonexistent file */
    memset(buf, 0xc, sizeof(buf));
    ret = GetPrivateProfileSectionNamesA( buf, 10, ".\\not_here.ini" );
    ok( ret == 0 ||
        broken(ret == 1), /* Win9x, WinME */
        "expected return size 0, got %d\n", ret );
    ok( buf[0] == 0, "returned buffer not terminated with null\n" );
    ok( buf[1] != 0, "returned buffer terminated with double-null\n" );
    
    /* Test with sufficiently large buffer */
    SetLastError(0xdeadbeef);
    memset(bufW, 0xcc, sizeof(bufW));
    ret = GetPrivateProfileSectionNamesW( bufW, 29, testfile3W );
    if (ret == 0 && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        win_skip("GetPrivateProfileSectionNamesW is not implemented\n");
        DeleteFileA( testfile3 );
        return;
    }
    ok( ret == 27, "expected return size 27, got %d\n", ret );
    ok( bufW[ret-1] == 0 && bufW[ret] == 0, "returned buffer not terminated with double-null\n" );
    
    /* Test with exactly fitting buffer */
    memset(bufW, 0xcc, sizeof(bufW));
    ret = GetPrivateProfileSectionNamesW( bufW, 28, testfile3W );
    ok( ret == 26, "expected return size 26, got %d\n", ret );
    ok( (bufW[ret+1] == 0 && bufW[ret] == 0) || /* W2K3 and higher */
        broken(bufW[ret+1] == 0xcccc && bufW[ret] == 0), /* NT4, W2K, WinXP */
        "returned buffer not terminated with double-null\n" );

    /* Test with a buffer too small */
    memset(bufW, 0xcc, sizeof(bufW));
    ret = GetPrivateProfileSectionNamesW( bufW, 27, testfile3W );
    ok( ret == 25, "expected return size 25, got %d\n", ret );
    ok( bufW[ret+1] == 0 && bufW[ret] == 0, "returned buffer not terminated with double-null\n" );
    
    DeleteFileA( testfile3 );

    /* Tests on nonexistent file */
    memset(bufW, 0xcc, sizeof(bufW));
    ret = GetPrivateProfileSectionNamesW( bufW, 10, not_here );
    ok( ret == 0, "expected return size 0, got %d\n", ret );
    ok( bufW[0] == 0, "returned buffer not terminated with null\n" );
    ok( bufW[1] != 0, "returned buffer terminated with double-null\n" );
}

/* If the ini-file has already been opened with CreateFile, WritePrivateProfileString failed in wine with an error ERROR_SHARING_VIOLATION,  some testing here */
static void test_profile_existing(void)
{
    static const char *testfile1 = ".\\winesharing1.ini";
    static const char *testfile2 = ".\\winesharing2.ini";

    static const struct {
        DWORD dwDesiredAccess;
        DWORD dwShareMode;
        DWORD write_error;
        BOOL read_error;
        DWORD broken_error;
    } pe[] = {
        {GENERIC_READ,  FILE_SHARE_READ,  ERROR_SHARING_VIOLATION, FALSE },
        {GENERIC_READ,  FILE_SHARE_WRITE, ERROR_SHARING_VIOLATION, TRUE },
        {GENERIC_WRITE, FILE_SHARE_READ,  ERROR_SHARING_VIOLATION, FALSE },
        {GENERIC_WRITE, FILE_SHARE_WRITE, ERROR_SHARING_VIOLATION, TRUE },
        {GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,  ERROR_SHARING_VIOLATION, FALSE },
        {GENERIC_READ|GENERIC_WRITE, FILE_SHARE_WRITE, ERROR_SHARING_VIOLATION, TRUE },
        {GENERIC_READ,  FILE_SHARE_READ|FILE_SHARE_WRITE, 0, FALSE, ERROR_SHARING_VIOLATION /* nt4 */},
        {GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, FALSE, ERROR_SHARING_VIOLATION /* nt4 */},
        /*Thief demo (bug 5024) opens .ini file like this*/
        {GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, FALSE, ERROR_SHARING_VIOLATION /* nt4 */}
    };

    int i;
    BOOL ret;
    DWORD size;
    HANDLE h = 0;
    char buffer[MAX_PATH];

    for (i=0; i < sizeof(pe)/sizeof(pe[0]); i++)
    {
        h = CreateFileA(testfile1, pe[i].dwDesiredAccess, pe[i].dwShareMode, NULL,
                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        ok(INVALID_HANDLE_VALUE != h, "%d: CreateFile failed\n",i);
        SetLastError(0xdeadbeef);

        ret = WritePrivateProfileStringA(SECTION, KEY, "12345", testfile1);
        if (!pe[i].write_error)
        {
            if (!ret)
                ok( broken(GetLastError() == pe[i].broken_error),
                    "%d: WritePrivateProfileString failed with error %u\n", i, GetLastError() );
            CloseHandle(h);
            size = GetPrivateProfileStringA(SECTION, KEY, 0, buffer, MAX_PATH, testfile1);
            if (ret)
                ok( size == 5, "%d: test failed, number of characters copied: %d instead of 5\n", i, size );
            else
                ok( !size, "%d: test failed, number of characters copied: %d instead of 0\n", i, size );
        }
        else
        {
            DWORD err = GetLastError();
            ok( !ret, "%d: WritePrivateProfileString succeeded\n", i );
            if (!ret)
                ok( err == pe[i].write_error, "%d: WritePrivateProfileString failed with error %u/%u\n",
                    i, err, pe[i].write_error );
            CloseHandle(h);
            size = GetPrivateProfileStringA(SECTION, KEY, 0, buffer, MAX_PATH, testfile1);
            ok( !size, "%d: test failed, number of characters copied: %d instead of 0\n", i, size );
        }

        ok( DeleteFileA(testfile1), "delete failed\n" );
    }

    h = CreateFileA(testfile2, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    sprintf( buffer, "[%s]\r\n%s=123\r\n", SECTION, KEY );
    ok( WriteFile( h, buffer, strlen(buffer), &size, NULL ), "failed to write\n" );
    CloseHandle( h );

    for (i=0; i < sizeof(pe)/sizeof(pe[0]); i++)
    {
        h = CreateFileA(testfile2, pe[i].dwDesiredAccess, pe[i].dwShareMode, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        ok(INVALID_HANDLE_VALUE != h, "%d: CreateFile failed\n",i);
        SetLastError(0xdeadbeef);
        ret = GetPrivateProfileStringA(SECTION, KEY, NULL, buffer, MAX_PATH, testfile2);
        /* Win9x and WinME returns 0 for all cases except the first one */
        if (!pe[i].read_error)
            ok( ret ||
                broken(!ret && GetLastError() == 0xdeadbeef), /* Win9x, WinME */
                "%d: GetPrivateProfileString failed with error %u\n", i, GetLastError() );
        else
            ok( !ret, "%d: GetPrivateProfileString succeeded\n", i );
        CloseHandle(h);
    }
    ok( DeleteFileA(testfile2), "delete failed\n" );
}

static void test_profile_delete_on_close(void)
{
    HANDLE h;
    DWORD size, res;
    static const CHAR testfile[] = ".\\testwine5.ini";
    static const char contents[] = "[" SECTION "]\n" KEY "=123\n";

    h = CreateFileA(testfile, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                    CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL);
    res = WriteFile( h, contents, sizeof contents - 1, &size, NULL );
    ok( res, "Cannot write test file: %x\n", GetLastError() );
    ok( size == sizeof contents - 1, "Test file: partial write\n");

    SetLastError(0xdeadbeef);
    res = GetPrivateProfileIntA(SECTION, KEY, 0, testfile);
    ok( res == 123 ||
        broken(res == 0 && GetLastError() == ERROR_SHARING_VIOLATION), /* Win9x, WinME */
        "Got %d instead of 123\n", res);

    /* This also deletes the file */
    CloseHandle(h);
}

static void test_profile_refresh(void)
{
    static const CHAR testfile[] = ".\\winetest4.ini";
    HANDLE h;
    DWORD size, res;
    static const char contents1[] = "[" SECTION "]\n" KEY "=123\n";
    static const char contents2[] = "[" SECTION "]\n" KEY "=124\n";

    h = CreateFileA(testfile, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                    CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL);
    res = WriteFile( h, contents1, sizeof contents1 - 1, &size, NULL );
    ok( res, "Cannot write test file: %x\n", GetLastError() );
    ok( size == sizeof contents1 - 1, "Test file: partial write\n");

    SetLastError(0xdeadbeef);
    res = GetPrivateProfileIntA(SECTION, KEY, 0, testfile);
    ok( res == 123 ||
        broken(res == 0 && GetLastError() == ERROR_SHARING_VIOLATION), /* Win9x, WinME */
        "Got %d instead of 123\n", res);

    CloseHandle(h);

    /* Test proper invalidation of wine's profile file cache */

    h = CreateFileA(testfile, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                    CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL);
    res = WriteFile( h, contents2, sizeof contents2 - 1, &size, NULL );
    ok( res, "Cannot write test file: %x\n", GetLastError() );
    ok( size == sizeof contents2 - 1, "Test file: partial write\n");

    SetLastError(0xdeadbeef);
    res = GetPrivateProfileIntA(SECTION, KEY, 0, testfile);
    ok( res == 124 ||
        broken(res == 0 && GetLastError() == 0xdeadbeef), /* Win9x, WinME */
        "Got %d instead of 124\n", res);

    /* This also deletes the file */
    CloseHandle(h);

    /* Cache must be invalidated if file no longer exists and default must be returned */
    SetLastError(0xdeadbeef);
    res = GetPrivateProfileIntA(SECTION, KEY, 421, testfile);
    ok( res == 421 ||
        broken(res == 0 && GetLastError() == 0xdeadbeef), /* Win9x, WinME */
        "Got %d instead of 421\n", res);
}

static void create_test_file(LPCSTR name, LPCSTR data, DWORD size)
{
    HANDLE hfile;
    DWORD count;

    hfile = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hfile != INVALID_HANDLE_VALUE, "cannot create %s\n", name);
    WriteFile(hfile, data, size, &count, NULL);
    CloseHandle(hfile);
}

static BOOL emptystr_ok(CHAR emptystr[MAX_PATH])
{
    int i;

    for(i = 0;i < MAX_PATH;++i)
        if(emptystr[i] != 0)
        {
            trace("emptystr[%d] = %d\n",i,emptystr[i]);
            return FALSE;
        }

    return TRUE;
}

static void test_GetPrivateProfileString(const char *content, const char *descript)
{
    DWORD ret, len;
    CHAR buf[MAX_PATH];
    CHAR def_val[MAX_PATH];
    CHAR path[MAX_PATH];
    CHAR windir[MAX_PATH];
    /* NT series crashes on r/o empty strings, so pass an r/w
       empty string and check for modification */
    CHAR emptystr[MAX_PATH] = "";
    LPSTR tempfile;

    static const char filename[] = ".\\winetest.ini";

    trace("test_GetPrivateProfileStringA: %s\n", descript);

    if(!lstrcmpA(descript, "CR only"))
    {
        SetLastError(0xdeadbeef);
        ret = GetPrivateProfileStringW(NULL, NULL, NULL,
                                       NULL, 0, NULL);
        if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        {
            win_skip("Win9x and WinME don't handle 'CR only' correctly\n");
            return;
        }
    }

    create_test_file(filename, content, lstrlenA(content));

    /* Run this test series with caching. Wine won't cache profile
       files younger than 2.1 seconds. */
    Sleep(2500);

    /* lpAppName is NULL */
    memset(buf, 0xc, sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA(NULL, "name1", "default",
                                   buf, MAX_PATH, filename);
    ok(ret == 18 ||
       broken(ret == 19), /* Win9x and WinME */
       "Expected 18, got %d\n", ret);
    len = lstrlenA("section1") + sizeof(CHAR) + lstrlenA("section2") + 2 * sizeof(CHAR);
    ok(!memcmp(buf, "section1\0section2\0\0", len),
       "Expected \"section1\\0section2\\0\\0\", got \"%s\"\n", buf);

    /* lpAppName is empty */
    memset(buf, 0xc, sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA(emptystr, "name1", "default",
                                   buf, MAX_PATH, filename);
    ok(ret == 7, "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "default"), "Expected \"default\", got \"%s\"\n", buf);
    ok(emptystr_ok(emptystr), "AppName modified\n");

    /* lpAppName is missing */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("notasection", "name1", "default",
                                   buf, MAX_PATH, filename);
    ok(ret == 7, "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "default"), "Expected \"default\", got \"%s\"\n", buf);

    /* lpAppName is empty, lpDefault is NULL */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA(emptystr, "name1", NULL,
                                   buf, MAX_PATH, filename);
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ok(!lstrcmpA(buf, "") ||
       broken(!lstrcmpA(buf, "kumquat")), /* Win9x, WinME */
       "Expected \"\", got \"%s\"\n", buf);
    ok(emptystr_ok(emptystr), "AppName modified\n");

    /* lpAppName is empty, lpDefault is empty */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA(emptystr, "name1", "",
                                   buf, MAX_PATH, filename);
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(emptystr_ok(emptystr), "AppName modified\n");

    /* lpAppName is empty, lpDefault has trailing blank characters */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    /* lpDefault must be writable (trailing blanks are removed inplace in win9x) */
    lstrcpyA(def_val, "default  ");
    ret = GetPrivateProfileStringA(emptystr, "name1", def_val,
                                   buf, MAX_PATH, filename);
    ok(ret == 7, "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "default"), "Expected \"default\", got \"%s\"\n", buf);
    ok(emptystr_ok(emptystr), "AppName modified\n");

    /* lpAppName is empty, many blank characters in lpDefault */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    /* lpDefault must be writable (trailing blanks are removed inplace in win9x) */
    lstrcpyA(def_val, "one two  ");
    ret = GetPrivateProfileStringA(emptystr, "name1", def_val,
                                   buf, MAX_PATH, filename);
    ok(ret == 7, "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "one two"), "Expected \"one two\", got \"%s\"\n", buf);
    ok(emptystr_ok(emptystr), "AppName modified\n");

    /* lpAppName is empty, blank character but not trailing in lpDefault */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA(emptystr, "name1", "one two",
                                   buf, MAX_PATH, filename);
    ok(ret == 7, "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "one two"), "Expected \"one two\", got \"%s\"\n", buf);
    ok(emptystr_ok(emptystr), "AppName modified\n");

    /* lpKeyName is NULL */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", NULL, "default",
                                   buf, MAX_PATH, filename);
    ok(ret == 18, "Expected 18, got %d\n", ret);
    ok(!memcmp(buf, "name1\0name2\0name4\0", ret + 1),
       "Expected \"name1\\0name2\\0name4\\0\", got \"%s\"\n", buf);

    /* lpKeyName is empty */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", emptystr, "default",
                                   buf, MAX_PATH, filename);
    ok(ret == 7, "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "default"), "Expected \"default\", got \"%s\"\n", buf);
    ok(emptystr_ok(emptystr), "KeyName modified\n");

    /* lpKeyName is missing */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "notakey", "default",
                                   buf, MAX_PATH, filename);
    ok(ret == 7, "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "default"), "Expected \"default\", got \"%s\"\n", buf);

    /* lpKeyName is empty, lpDefault is NULL */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", emptystr, NULL,
                                   buf, MAX_PATH, filename);
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ok(!lstrcmpA(buf, "") ||
       broken(!lstrcmpA(buf, "kumquat")), /* Win9x, WinME */
       "Expected \"\", got \"%s\"\n", buf);
    ok(emptystr_ok(emptystr), "KeyName modified\n");

    /* lpKeyName is empty, lpDefault is empty */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", emptystr, "",
                                   buf, MAX_PATH, filename);
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    ok(emptystr_ok(emptystr), "KeyName modified\n");

    /* lpKeyName is empty, lpDefault has trailing blank characters */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    /* lpDefault must be writable (trailing blanks are removed inplace in win9x) */
    lstrcpyA(def_val, "default  ");
    ret = GetPrivateProfileStringA("section1", emptystr, def_val,
                                   buf, MAX_PATH, filename);
    ok(ret == 7, "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "default"), "Expected \"default\", got \"%s\"\n", buf);
    ok(emptystr_ok(emptystr), "KeyName modified\n");

    if (0) /* crashes */
    {
        /* lpReturnedString is NULL */
        ret = GetPrivateProfileStringA("section1", "name1", "default",
                                       NULL, MAX_PATH, filename);
    }

    /* lpFileName is NULL */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "name1", "default",
                                   buf, MAX_PATH, NULL);
    ok(ret == 7 ||
       broken(ret == 0), /* Win9x, WinME */
       "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "default") ||
       broken(!lstrcmpA(buf, "kumquat")), /* Win9x, WinME */
       "Expected \"default\", got \"%s\"\n", buf);

    /* lpFileName is empty */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "name1", "default",
                                   buf, MAX_PATH, "");
    ok(ret == 7, "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "default"), "Expected \"default\", got \"%s\"\n", buf);

    /* lpFileName is nonexistent */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "name1", "default",
                                   buf, MAX_PATH, "nonexistent");
    ok(ret == 7, "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "default"), "Expected \"default\", got \"%s\"\n", buf);

    /* nSize is 0 */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "name1", "default",
                                   buf, 0, filename);
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ok(!lstrcmpA(buf, "kumquat"), "Expected buf to be unchanged, got \"%s\"\n", buf);

    /* nSize is exact size of output */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "name1", "default",
                                   buf, 4, filename);
    ok(ret == 3, "Expected 3, got %d\n", ret);
    ok(!lstrcmpA(buf, "val"), "Expected \"val\", got \"%s\"\n", buf);

    /* nSize has room for NULL terminator */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "name1", "default",
                                   buf, 5, filename);
    ok(ret == 4, "Expected 4, got %d\n", ret);
    ok(!lstrcmpA(buf, "val1"), "Expected \"val1\", got \"%s\"\n", buf);

    /* output is 1 character */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "name4", "default",
                                   buf, MAX_PATH, filename);
    ok(ret == 1, "Expected 1, got %d\n", ret);
    ok(!lstrcmpA(buf, "a"), "Expected \"a\", got \"%s\"\n", buf);

    /* output is 1 character, no room for NULL terminator */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "name4", "default",
                                   buf, 1, filename);
    ok(ret == 0, "Expected 0, got %d\n", ret);
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);

    /* lpAppName is NULL, not enough room for final section name */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA(NULL, "name1", "default",
                                   buf, 16, filename);
    ok(ret == 14, "Expected 14, got %d\n", ret);
    len = lstrlenA("section1") + 2 * sizeof(CHAR);
    todo_wine
    ok(!memcmp(buf, "section1\0secti\0\0", ret + 2) ||
       broken(!memcmp(buf, "section1\0\0", len)), /* Win9x, WinME */
       "Expected \"section1\\0secti\\0\\0\", got \"%s\"\n", buf);

    /* lpKeyName is NULL, not enough room for final key name */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", NULL, "default",
                                   buf, 16, filename);
    ok(ret == 14, "Expected 14, got %d\n", ret);
    todo_wine
    ok(!memcmp(buf, "name1\0name2\0na\0\0", ret + 2) ||
       broken(!memcmp(buf, "name1\0name2\0n\0\0", ret + 1)), /* Win9x, WinME */
       "Expected \"name1\\0name2\\0na\\0\\0\", got \"%s\"\n", buf);

    /* key value has quotation marks which are stripped */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "name2", "default",
                                   buf, MAX_PATH, filename);
    ok(ret == 4, "Expected 4, got %d\n", ret);
    ok(!lstrcmpA(buf, "val2"), "Expected \"val2\", got \"%s\"\n", buf);

    /* case does not match */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "NaMe1", "default",
                                   buf, MAX_PATH, filename);
    ok(ret == 4, "Expected 4, got %d\n", ret);
    ok(!lstrcmpA(buf, "val1"), "Expected \"val1\", got \"%s\"\n", buf);

    /* only filename is used */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "NaMe1", "default",
                                   buf, MAX_PATH, "winetest.ini");
    ok(ret == 7, "Expected 7, got %d\n", ret);
    ok(!lstrcmpA(buf, "default"), "Expected \"default\", got \"%s\"\n", buf);

    GetWindowsDirectoryA(windir, MAX_PATH);
    SetLastError(0xdeadbeef);
    ret = GetTempFileNameA(windir, "pre", 0, path);
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Not allowed to create a file in the Windows directory\n");
        DeleteFileA(filename);
        return;
    }
    tempfile = strrchr(path, '\\') + 1;
    create_test_file(path, content, lstrlenA(content));

    /* only filename is used, file exists in windows directory */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "NaMe1", "default",
                                   buf, MAX_PATH, tempfile);
    ok(ret == 4, "Expected 4, got %d\n", ret);
    ok(!lstrcmpA(buf, "val1"), "Expected \"val1\", got \"%s\"\n", buf);

    /* successful case */
    memset(buf, 0xc,sizeof(buf));
    lstrcpyA(buf, "kumquat");
    ret = GetPrivateProfileStringA("section1", "name1", "default",
                                   buf, MAX_PATH, filename);
    ok(ret == 4, "Expected 4, got %d\n", ret);
    ok(!lstrcmpA(buf, "val1"), "Expected \"val1\", got \"%s\"\n", buf);

 /* Existing section with no keys in an existing file */
    memset(buf, 0xc,sizeof(buf));
    SetLastError(0xdeadbeef);
    ret=GetPrivateProfileStringA("section2", "DoesntExist", "",
                                 buf, MAX_PATH, filename);
    ok( ret == 0, "expected return size 0, got %d\n", ret );
    ok(!lstrcmpA(buf, ""), "Expected \"\", got \"%s\"\n", buf);
    todo_wine
   ok( GetLastError() == 0xdeadbeef ||
       GetLastError() == ERROR_FILE_NOT_FOUND /* Win 7 */,
       "expected 0xdeadbeef or ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());


    DeleteFileA(path);
    DeleteFileA(filename);
}

static BOOL check_binary_file_data(LPCSTR path, const VOID *data, DWORD size)
{
    HANDLE file;
    CHAR buf[MAX_PATH];
    BOOL ret;

    file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    if (file == INVALID_HANDLE_VALUE)
      return FALSE;

    if(size != GetFileSize(file, NULL) )
    {
        CloseHandle(file);
        return FALSE;
    }

    ret = ReadFile(file, buf, size, &size, NULL);
    CloseHandle(file);
    if (!ret)
      return FALSE;

    return !memcmp(buf, data, size);
}

static BOOL check_file_data(LPCSTR path, LPCSTR data)
{
    return check_binary_file_data(path, data, lstrlenA(data));
}

static void test_WritePrivateProfileString(void)
{
    BOOL ret;
    LPCSTR data;
    CHAR path[MAX_PATH];
    CHAR temp[MAX_PATH];

    SetLastError(0xdeadbeef);
    ret = WritePrivateProfileStringW(NULL, NULL, NULL, NULL);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        /* Win9x/WinME needs (variable) timeouts between tests and even long timeouts don't
         * guarantee a correct result.
         * Win9x/WinMe also produces different ini files where there is always a newline before
         * a section start (except for the first one).
         */
        win_skip("WritePrivateProfileString on Win9x/WinME is hard to test reliably\n");
        return;
    }

    GetTempPathA(MAX_PATH, temp);
    GetTempFileNameA(temp, "wine", 0, path);
    DeleteFileA(path);

    /* path is not created yet */

    /* NULL lpAppName */
    SetLastError(0xdeadbeef);
    ret = WritePrivateProfileStringA(NULL, "key", "string", path);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND ||
       broken(GetLastError() == ERROR_INVALID_PARAMETER) || /* NT4 */
       broken(GetLastError() == 0xdeadbeef), /* Win9x and WinME */
       "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    ok(GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES,
       "Expected path to not exist\n");

    GetTempFileNameA(temp, "wine", 0, path);

    /* NULL lpAppName, path exists */
    data = "";
    SetLastError(0xdeadbeef);
    ret = WritePrivateProfileStringA(NULL, "key", "string", path);
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND ||
       broken(GetLastError() == ERROR_INVALID_PARAMETER) || /* NT4 */
       broken(GetLastError() == 0xdeadbeef), /* Win9x and WinME */
       "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    ok(check_file_data(path, data), "File doesn't match\n");
    DeleteFileA(path);

    if (0)
    {
    /* empty lpAppName, crashes on NT4 and higher */
    data = "[]\r\n"
           "key=string\r\n";
    ret = WritePrivateProfileStringA("", "key", "string", path);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(check_file_data(path, data), "File doesn't match\n");
    DeleteFileA(path);
    }

    /* NULL lpKeyName */
    data = "";
    ret = WritePrivateProfileStringA("App", NULL, "string", path);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    todo_wine
    {
        ok(check_file_data(path, data), "File doesn't match\n");
    }
    DeleteFileA(path);

    if (0)
    {
    /* empty lpKeyName, crashes on NT4 and higher */
    data = "[App]\r\n"
           "=string\r\n";
    ret = WritePrivateProfileStringA("App", "", "string", path);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    todo_wine
    {
        ok(check_file_data(path, data), "File doesn't match\n");
    }
    DeleteFileA(path);
    }

    /* NULL lpString */
    data = "";
    ret = WritePrivateProfileStringA("App", "key", NULL, path);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    todo_wine
    {
        ok(check_file_data(path, data) ||
           (broken(GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES)), /* Win9x and WinME */
           "File doesn't match\n");
    }
    DeleteFileA(path);

    /* empty lpString */
    data = "[App]\r\n"
           "key=\r\n";
    ret = WritePrivateProfileStringA("App", "key", "", path);
    ok(ret == TRUE ||
       broken(!ret), /* Win9x and WinME */
       "Expected TRUE, got %d\n", ret);
    ok(check_file_data(path, data) ||
       (broken(GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES)), /* Win9x and WinME */
       "File doesn't match\n");
    DeleteFileA(path);

    /* empty lpFileName */
    SetLastError(0xdeadbeef);
    ret = WritePrivateProfileStringA("App", "key", "string", "");
    ok(ret == FALSE, "Expected FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_ACCESS_DENIED ||
       broken(GetLastError() == ERROR_PATH_NOT_FOUND), /* Win9x and WinME */
       "Expected ERROR_ACCESS_DENIED, got %d\n", GetLastError());

    /* Relative paths are relative to X:\\%WINDIR% */
    GetWindowsDirectoryA(temp, MAX_PATH);
    GetTempFileNameA(temp, "win", 1, path);
    if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES)
        skip("Not allowed to create a file in the Windows directory\n");
    else
    {
        DeleteFileA(path);

        data = "[App]\r\n"
               "key=string\r\n";
        ret = WritePrivateProfileStringA("App", "key", "string", "win1.tmp");
        ok(ret == TRUE, "Expected TRUE, got %d, le=%u\n", ret, GetLastError());
        ok(check_file_data(path, data), "File doesn't match\n");
        DeleteFileA(path);
    }

    GetTempPathA(MAX_PATH, temp);
    GetTempFileNameA(temp, "wine", 0, path);

    /* build up an INI file */
    WritePrivateProfileStringA("App1", "key1", "string1", path);
    WritePrivateProfileStringA("App1", "key2", "string2", path);
    WritePrivateProfileStringA("App1", "key3", "string3", path);
    WritePrivateProfileStringA("App2", "key4", "string4", path);

    /* make an addition and verify the INI */
    data = "[App1]\r\n"
           "key1=string1\r\n"
           "key2=string2\r\n"
           "key3=string3\r\n"
           "[App2]\r\n"
           "key4=string4\r\n"
           "[App3]\r\n"
           "key5=string5\r\n";
    ret = WritePrivateProfileStringA("App3", "key5", "string5", path);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(check_file_data(path, data), "File doesn't match\n");

    /* lpString is NULL, key2 key is deleted */
    data = "[App1]\r\n"
           "key1=string1\r\n"
           "key3=string3\r\n"
           "[App2]\r\n"
           "key4=string4\r\n"
           "[App3]\r\n"
           "key5=string5\r\n";
    ret = WritePrivateProfileStringA("App1", "key2", NULL, path);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(check_file_data(path, data), "File doesn't match\n");

    /* try to delete key2 again */
    data = "[App1]\r\n"
           "key1=string1\r\n"
           "key3=string3\r\n"
           "[App2]\r\n"
           "key4=string4\r\n"
           "[App3]\r\n"
           "key5=string5\r\n";
    ret = WritePrivateProfileStringA("App1", "key2", NULL, path);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(check_file_data(path, data), "File doesn't match\n");

    /* lpKeyName is NULL, App1 section is deleted */
    data = "[App2]\r\n"
           "key4=string4\r\n"
           "[App3]\r\n"
           "key5=string5\r\n";
    ret = WritePrivateProfileStringA("App1", NULL, "string1", path);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(check_file_data(path, data), "File doesn't match\n");

    /* lpString is not needed to delete a section */
    data = "[App3]\r\n"
           "key5=string5\r\n";
    ret = WritePrivateProfileStringA("App2", NULL, NULL, path);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(check_file_data(path, data), "File doesn't match\n");

    /* leave just the section */
    data = "[App3]\r\n";
    ret = WritePrivateProfileStringA("App3", "key5", NULL, path);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    ok(check_file_data(path, data), "File doesn't match\n");
    DeleteFileA(path);

    /* NULLs in file before first section. Should be preserved in output */
    data = "Data \0 before \0 first \0 section"    /* 31 bytes */
           "\r\n[section1]\r\n"                    /* 14 bytes */
           "key1=string1\r\n";                     /* 14 bytes */
    GetTempFileNameA(temp, "wine", 0, path);
    create_test_file(path, data, 31);
    ret = WritePrivateProfileStringA("section1", "key1", "string1", path);
    ok(ret == TRUE, "Expected TRUE, got %d\n", ret);
    todo_wine
    ok( check_binary_file_data(path, data, 59) ||
        broken( check_binary_file_data(path,        /* Windows 9x */
            "Data \0 before \0 first \0 section"    /* 31 bytes */
            "\r\n\r\n[section1]\r\n"                /* 14 bytes */
            "key1=string1"                          /* 14 bytes */
            , 59)), "File doesn't match\n");
    DeleteFileA(path);
}

START_TEST(profile)
{
    test_profile_int();
    test_profile_string();
    test_profile_sections();
    test_profile_sections_names();
    test_profile_existing();
    test_profile_delete_on_close();
    test_profile_refresh();
    test_GetPrivateProfileString(
        "[section1]\r\n"
        "name1=val1\r\n"
        "name2=\"val2\"\r\n"
        "name3\r\n"
        "name4=a\r\n"
        "[section2]\r\n",
        "CR+LF");
    test_GetPrivateProfileString(
        "[section1]\r"
        "name1=val1\r"
        "name2=\"val2\"\r"
        "name3\r"
        "name4=a\r"
        "[section2]\r",
        "CR only");
    test_WritePrivateProfileString();
}
