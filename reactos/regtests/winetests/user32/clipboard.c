/*
 * Unit test suite for clipboard functions.
 *
 * Copyright 2002 Dmitry Timoshkov
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
 */

#define _WIN32_WINNT 0x0501

#include "wine/test.h"
#include "windows.h"

static BOOL is_win9x = FALSE;
static DWORD sn; /* to test Secuence Number */

#define test_last_error(expected_error) \
    do \
    { \
        if (!is_win9x) \
            ok(GetLastError() == expected_error, \
               "Last error should be set to %ld, not %ld\n", \
                expected_error, GetLastError()); \
    } while (0)

static void test_ClipboardOwner(void)
{
    HWND hWnd1, hWnd2;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ok(!GetClipboardOwner() && GetLastError() == 0xdeadbeef,
       "could not perform clipboard test: clipboard already owned\n");

    hWnd1 = CreateWindowExA(0, "static", NULL, WS_POPUP,
                                 0, 0, 10, 10, 0, 0, 0, NULL);
    ok(hWnd1 != 0, "CreateWindowExA error %ld\n", GetLastError());
    trace("hWnd1 = %p\n", hWnd1);

    hWnd2 = CreateWindowExA(0, "static", NULL, WS_POPUP,
                                 0, 0, 10, 10, 0, 0, 0, NULL);
    ok(hWnd2 != 0, "CreateWindowExA error %ld\n", GetLastError());
    trace("hWnd2 = %p\n", hWnd2);

    SetLastError(0xdeadbeef);
    ok(!CloseClipboard(), "CloseClipboard should fail if clipboard wasn't open\n");
    test_last_error(ERROR_CLIPBOARD_NOT_OPEN);

    SetLastError(0xdeadbeef);
    ok(OpenClipboard(0), "OpenClipboard failed\n");
    test_last_error((long)0xdeadbeef);
    ok(OpenClipboard(0), "OpenClipboard failed\n");
    test_last_error((long)0xdeadbeef);
    
    ok(!GetClipboardOwner(), "clipboard should still be not owned\n");
    ok(!OpenClipboard(hWnd1), "OpenClipboard should fail since clipboard already opened\n");
    ret = CloseClipboard();
    ok( ret, "CloseClipboard error %ld\n", GetLastError());

    ok(OpenClipboard(hWnd1), "OpenClipboard failed\n");

    SetLastError(0xdeadbeef);
    ok(!OpenClipboard(hWnd2) && GetLastError() == 0xdeadbeef,
       "OpenClipboard should fail without setting last error value\n");

    SetLastError(0xdeadbeef);
    ok(!GetClipboardOwner() && GetLastError() == 0xdeadbeef, "clipboard should still be not owned\n");
    ret = EmptyClipboard();
    ok( ret, "EmptyClipboard error %ld\n", GetLastError());
    ok(GetClipboardOwner() == hWnd1, "clipboard should be owned by %p, not by %p\n", hWnd1, GetClipboardOwner());

    SetLastError(0xdeadbeef);
    ok(!OpenClipboard(hWnd2) && GetLastError() == 0xdeadbeef,
       "OpenClipboard should fail without setting last error value\n");

    ret = CloseClipboard();
    ok(ret, "CloseClipboard error %ld\n", GetLastError());
    ok(GetClipboardOwner() == hWnd1, "clipboard should still be owned\n");
    
    ret = DestroyWindow(hWnd1);
    ok(ret, "DestroyWindow error %ld\n", GetLastError());
    ret = DestroyWindow(hWnd2);
    ok(ret, "DestroyWindow error %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    ok(!GetClipboardOwner() && GetLastError() == 0xdeadbeef, "clipboard should not be owned\n");
    
    hWnd1 = CreateWindowExA(0, "static", NULL, WS_POPUP,
                                 0, 0, 10, 10, 0, 0, 0, NULL);
    ok(hWnd1 != 0, "CreateWindowExA error %ld\n", GetLastError());
    trace("hWnd1 = %p\n", hWnd1);    
    ok(OpenClipboard(hWnd1), "OpenClipboard failed\n");    
    ret = DestroyWindow(hWnd1);
    ok(ret, "DestroyWindow error %ld\n", GetLastError());
    ok(OpenClipboard(0), "OpenClipboard should not fail\n");
    ok(CloseClipboard(), "CloseClipboard error %ld\n", GetLastError());
    
    DWORD asn = GetClipboardSequenceNumber();   
    ok(asn == sn + 2, "GetClipboardSequenceNumber() returns %d, not %d\n", (int)asn, (int)sn + 2);
}

static void test_EnumClipboardFormats(void)
{
   
    SetLastError(0xdeadbeef);
    ok(!EnumClipboardFormats(0), "EnumClipboardFormats failed\n");
	test_last_error(ERROR_CLIPBOARD_NOT_OPEN);
	
	UINT j = 0;
	UINT i = 1;
	/*
	ok( OpenClipboard(NULL), "OpenClipboard error %ld\n", GetLastError());
	ok(!(j = EnumClipboardFormats(0)), "EnumClipboardFormats failed\n");
	while (j!=0) 
	{
	    j = EnumClipboardFormats(j);
	    i++;
	}
	ok(!(j != CountClipboardFormats()), "EnumClipboardFormats failed\n");
	*/
	ok(OpenClipboard(NULL), "OpenClipboard error %ld\n", GetLastError());
    ok(EmptyClipboard(), "EmptyClipboard error %ld\n", GetLastError());
    
	WCHAR *s = L"Pedor";
	HGLOBAL hdst = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, 7 * sizeof(WCHAR));
	PCHAR dst = GlobalLock(hdst);
	memcpy(dst, s, 6 * sizeof(WCHAR));
	GlobalUnlock(hdst);
	
    ok(SetClipboardData(CF_UNICODETEXT, hdst) != 0, "SetClipboardData error %ld\n", GetLastError());
    
	/* this test is very bogus, depends on data entered, need more test ;)
	ok((j = EnumClipboardFormats(0)), "EnumClipboardFormats failed \n");
	while (j!=0) 
	{
	    j = EnumClipboardFormats(j);
	    i++;
	}
	ok(j != CountClipboardFormats(), "EnumClipboardFormats failed\n");
	
	ok(j != 1, "EnumClipboardFormats must be 1\n");
	*/
	
    ok( CloseClipboard(), "CloseClipboard error %ld\n", GetLastError());

    ok(OpenClipboard(NULL), "OpenClipboard error %ld\n", GetLastError());
	ok((j = EnumClipboardFormats(0)), "EnumClipboardFormats failed (%ld)\n", GetLastError());
	while (j!=0) 
	{
	    j = EnumClipboardFormats(j);
	    i++;
	}
	ok(j != CountClipboardFormats(), "EnumClipboardFormats failed\n");
    ok( CloseClipboard(), "CloseClipboard error %ld\n", GetLastError());
    
    DWORD asn = GetClipboardSequenceNumber();   
    ok(asn == sn + 7, "GetClipboardSequenceNumber() returns %d, not %d\n", (int)asn, (int)sn + 7);
    
}

static void test_Owner(void)
{
    HWND hWnd1;
    BOOL ret;
    
    /* get owner */
    ok(OpenClipboard(0), "OpenClipboard should not fail\n");
    ok(CloseClipboard(), "CloseClipboard error %ld\n", GetLastError());
    hWnd1 = CreateWindowExA(0, "static", NULL, WS_POPUP,
                                 0, 0, 10, 10, 0, 0, 0, NULL);
    ok(hWnd1 != 0, "CreateWindowExA error %ld\n", GetLastError());
  
    ok(OpenClipboard(hWnd1), "OpenClipboard failed\n");    
	
	HGLOBAL hdst = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, 7 * sizeof(WCHAR));
	
    ok((int)SetClipboardData(CF_UNICODETEXT, hdst), "SetClipboardData error we can't put data if not be the owner (%ld)\n", GetLastError());
    ok(CloseClipboard(), "CloseClipboard error %ld\n", GetLastError());
    
    ret = DestroyWindow(hWnd1);
    ok(ret, "DestroyWindow error %ld\n", GetLastError());
    
    /* tets secuence number*/
    DWORD asn = GetClipboardSequenceNumber();   
    ok(asn == sn + 8, "GetClipboardSequenceNumber() returns %d, not %d\n", (int)asn, (int)sn + 8);
}

static void test_RegisterClipboardFormatA(void)
{
    ATOM atom_id;
    UINT format_id, format_id2;
    char buf[256];
    int len;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ok(!(format_id = RegisterClipboardFormatA(0)), "RegisterClipboardFormat should fail\n");
    test_last_error(ERROR_INVALID_PARAMETER);
    
    SetLastError(0xdeadbeef);
    ok(!(format_id = RegisterClipboardFormatA("")), "RegisterClipboardFormat should fail\n");
    test_last_error(ERROR_INVALID_NAME);
    
    format_id = RegisterClipboardFormatA("my_cool_clipboard_format");
    ok(format_id > 0xc000 && format_id < 0xffff, "invalid clipboard format id %04x\n", format_id);

    format_id2 = RegisterClipboardFormatA("MY_COOL_CLIPBOARD_FORMAT");
    ok(format_id2 == format_id, "invalid clipboard format id %04x\n", format_id2);

    len = GetClipboardFormatNameA(format_id, buf, 256);
    ok(len == lstrlenA("my_cool_clipboard_format"), "wrong format name length %d\n", len);
    ok(!lstrcmpA(buf, "my_cool_clipboard_format"), "wrong format name \"%s\"\n", buf);

    WCHAR buf2[256];
    len = GetClipboardFormatNameW(format_id, buf2, 256);
    ok(len == lstrlenW(L"my_cool_clipboard_format"), "wrong format name length %d\n", len);
    ok(!lstrcmpW(buf2, L"my_cool_clipboard_format"), "wrong format name \"%S\"\n", buf2);

    lstrcpyA(buf, "foo");
    SetLastError(0xdeadbeef);
    len = GetAtomNameA((ATOM)format_id, buf, 256);
    ok(len == 0, "GetAtomNameA should fail\n");
    test_last_error(ERROR_INVALID_HANDLE);

todo_wine
{
    lstrcpyA(buf, "foo");
    SetLastError(0xdeadbeef);
    len = GlobalGetAtomNameA((ATOM)format_id, buf, 256);
    ok(len == 0, "GlobalGetAtomNameA should fail\n");
    test_last_error(ERROR_INVALID_HANDLE);
}

    SetLastError(0xdeadbeef);
    atom_id = FindAtomA("my_cool_clipboard_format");
    ok(atom_id == 0, "FindAtomA should fail\n");
    test_last_error(ERROR_FILE_NOT_FOUND);

#if 0
    /* this relies on the clipboard and global atom table being different */
    SetLastError(0xdeadbeef);
    atom_id = GlobalFindAtomA("my_cool_clipboard_format");
    ok(atom_id == 0, "GlobalFindAtomA should fail\n");
    test_last_error(ERROR_FILE_NOT_FOUND);

    for (format_id = 0; format_id < 0xffff; format_id++)
    {
        SetLastError(0xdeadbeef);
        len = GetClipboardFormatNameA(format_id, buf, 256);

        if (format_id < 0xc000)
        {
            ok(!len, "GetClipboardFormatNameA should fail, but it returned %d (%s)\n", len, buf);
            test_last_error(ERROR_INVALID_PARAMETER);
        }
        else
        {
            if (len)
                trace("%04x: %s\n", format_id, len ? buf : "");
            else
                test_last_error(ERROR_INVALID_HANDLE);
        }
    }
#endif

    ok(!GetClipboardFormatNameA(1, buf, 256), "GetClipboardFormatNameA should fail\n");
    
    ret = OpenClipboard(0);
    ok( ret, "OpenClipboard error %ld\n", GetLastError());

    trace("# of formats available: %d\n", CountClipboardFormats());

    format_id = 0;
    while ((format_id = EnumClipboardFormats(format_id)))
    {
        ok(IsClipboardFormatAvailable(format_id), "format %04x was listed as available\n", format_id);
        len = GetClipboardFormatNameA(format_id, buf, 256);
        trace("%04x: %s\n", format_id, len ? buf : "");
    }

    ret = EmptyClipboard();
    ok( ret, "EmptyClipboard error %ld\n", GetLastError());
    ret =CloseClipboard();
    ok( ret, "CloseClipboard error %ld\n", GetLastError());

    if (CountClipboardFormats())
    {
        SetLastError(0xdeadbeef);
        ok(!EnumClipboardFormats(0), "EnumClipboardFormats should fail if clipboard wasn't open\n");
        ok(GetLastError() == ERROR_CLIPBOARD_NOT_OPEN,
           "Last error should be set to ERROR_CLIPBOARD_NOT_OPEN, not %ld\n", GetLastError());
    }

    SetLastError(0xdeadbeef);
    ok(!EmptyClipboard(), "EmptyClipboard should fail if clipboard wasn't open\n");
    test_last_error(ERROR_CLIPBOARD_NOT_OPEN);
    
    DWORD asn = GetClipboardSequenceNumber();
    ok(asn == sn + 1, "GetClipboardSequenceNumber() returns %d, not %d\n", (int)asn, (int)sn + 1);
}

static void test_RegisterClipboardFormatW(void)
{
    ATOM atom_id;
    UINT format_id, format_id2;
    WCHAR buf[256];
    int len;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ok(!(format_id = RegisterClipboardFormatW(0)), "RegisterClipboardFormatW should fail\n");
    test_last_error(ERROR_INVALID_PARAMETER);
    
    SetLastError(0xdeadbeef);
    ok(!(format_id = RegisterClipboardFormatW(L"")), "RegisterClipboardFormatW should fail\n");
    test_last_error(ERROR_INVALID_NAME);
    
    format_id = RegisterClipboardFormatW(L"my_cool_clipboard_format");
    ok(format_id > 0xc000 && format_id < 0xffff, "invalid clipboard format id %04x\n", format_id);
    
    format_id2 = RegisterClipboardFormatW(L"MY_COOL_CLIPBOARD_FORMAT");
    ok(format_id2 == format_id, "invalid clipboard format id %04x\n", format_id2);
    
    len = GetClipboardFormatNameW(format_id, buf, 256);
    ok(len == lstrlenW(L"my_cool_clipboard_format"), "wrong format name length %d\n", len);
    ok(!lstrcmpW(buf, L"my_cool_clipboard_format"), "wrong format name \"%S\"\n", buf);
    
    lstrcpyW(buf, L"foo");
    SetLastError(0xdeadbeef);
    len = GetAtomNameW((ATOM)format_id, buf, 256);
    ok(len == 0, "GetAtomNameA should fail\n");
    test_last_error(ERROR_INVALID_HANDLE);
    
    
    lstrcpyW(buf, L"foo");
    SetLastError(0xdeadbeef);
    len = GlobalGetAtomNameW((ATOM)format_id, buf, 256);
    ok(len == 0, "GlobalGetAtomNameA should fail\n");
    test_last_error(ERROR_INVALID_HANDLE);

    SetLastError(0xdeadbeef);
    atom_id = FindAtomW(L"my_cool_clipboard_format");
    ok(atom_id == 0, "FindAtomA should fail\n");
    test_last_error(ERROR_FILE_NOT_FOUND);

    ok(!GetClipboardFormatNameW(1, buf, 256), "GetClipboardFormatNameA should fail\n");
    
    ret = OpenClipboard(0);
    ok( ret, "OpenClipboard error %ld\n", GetLastError());

    trace("# of formats available: %d\n", CountClipboardFormats());

    format_id = 0;
    while ((format_id = EnumClipboardFormats(format_id)))
    {
        ok(IsClipboardFormatAvailable(format_id), "format %04x was listed as available\n", format_id);
        len = GetClipboardFormatNameW(format_id, buf, 256);
        trace("%04x: %S\n", format_id, len ? buf : L"");
    }

    ret = EmptyClipboard();
    ok(ret, "EmptyClipboard error %ld\n", GetLastError());
    ret = CloseClipboard();
    ok(ret, "CloseClipboard error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ok(!EmptyClipboard(), "EmptyClipboard should fail if clipboard wasn't open\n");
    test_last_error(ERROR_CLIPBOARD_NOT_OPEN);

    DWORD asn = GetClipboardSequenceNumber();
    ok(asn == sn + 9, "GetClipboardSequenceNumber() returns %d, not %d\n", (int)asn, (int)sn + 9);
}


START_TEST(clipboard)
{
    SetLastError(0xdeadbeef);
    FindAtomW(NULL);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) is_win9x = TRUE;

    sn = GetClipboardSequenceNumber();
    trace("Current Secuence Number: %d\n", (int)sn);   
    
    test_RegisterClipboardFormatA();
    test_ClipboardOwner();
    test_EnumClipboardFormats();
    test_Owner();
    test_RegisterClipboardFormatW();
    
    /*
    ok(0, "GetClipboardSequenceNumber() returns 0x%x\n", GetClipboardSequenceNumber());
    */
}
