/* Unit test suite for window classes.
 *
 * Copyright 2002 Mike McCormack
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

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"

#define NUMCLASSWORDS 4

LRESULT WINAPI ClassTest_WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcW (hWnd, msg, wParam, lParam);
}

/***********************************************************************
 *
 *           WinMain
 */
void ClassTest(HINSTANCE hInstance, BOOL global)
{
    WNDCLASSW cls, wc;
    WCHAR className[] = {'T','e','s','t','C','l','a','s','s',0};
    WCHAR winName[]   = {'W','i','n','C','l','a','s','s','T','e','s','t',0};
    ATOM test_atom;
    HWND hTestWnd;
    DWORD i;
    WCHAR str[20];
    ATOM classatom;

    cls.style         = CS_HREDRAW | CS_VREDRAW | (global?CS_GLOBALCLASS:0);
    cls.lpfnWndProc   = ClassTest_WndProc;
    cls.cbClsExtra    = NUMCLASSWORDS*sizeof(DWORD);
    cls.cbWndExtra    = 12;
    cls.hInstance     = hInstance;
    cls.hIcon         = LoadIconW (0, (LPWSTR)IDI_APPLICATION);
    cls.hCursor       = LoadCursorW (0, (LPWSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject (WHITE_BRUSH);
    cls.lpszMenuName  = 0;
    cls.lpszClassName = className;

    classatom=RegisterClassW(&cls);
    if (!classatom && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
        return;
    ok(classatom, "failed to register class");

    ok(!RegisterClassW (&cls),
        "RegisterClass of the same class should fail for the second time");

#if 0
    /* these succeeds on Wine, but shouldn't cause any trouble ... */
    ok(!GlobalFindAtomW(className),
        "Found class as global atom");

    ok(!FindAtomW(className),
        "Found class as global atom");
#endif

    /* Setup windows */
    hTestWnd = CreateWindowW (className, winName,
       WS_OVERLAPPEDWINDOW + WS_HSCROLL + WS_VSCROLL,
       CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0,
       0, hInstance, 0);

    ok(hTestWnd!=0, "Failed to create window");

    /* test initial values of valid classwords */
    for(i=0; i<NUMCLASSWORDS; i++)
    {
        SetLastError(0);
        ok(!GetClassLongW(hTestWnd,i*sizeof (DWORD)),
            "GetClassLongW initial value nonzero!");
        ok(!GetLastError(),
            "GetClassLongW failed!");
    }

#if 0
    /*
     *  GetClassLongW(hTestWnd, NUMCLASSWORDS*sizeof(DWORD))
     *  does not fail on Win 98, though MSDN says it should
     */
    SetLastError(0);
    GetClassLongW(hTestWnd, NUMCLASSWORDS*sizeof(DWORD));
    ok(GetLastError(),
        "GetClassLongW() with invalid offset did not fail");
#endif

    /* set values of valid class words */
    for(i=0; i<NUMCLASSWORDS; i++)
    {
        SetLastError(0);
        ok(!SetClassLongW(hTestWnd,i*sizeof(DWORD),i+1),
            "GetClassLongW(%ld) initial value nonzero!",i*sizeof(DWORD));
        ok(!GetLastError(),
            "SetClassLongW(%ld) failed!",i*sizeof(DWORD));
    }

    /* test values of valid classwords that we set */
    for(i=0; i<NUMCLASSWORDS; i++)
    {
        SetLastError(0);
        ok( (i+1) == GetClassLongW(hTestWnd,i*sizeof (DWORD)),
            "GetClassLongW value doesn't match what was set!");
        ok(!GetLastError(),
            "GetClassLongW failed!");
    }

    /* check GetClassName */
    i = GetClassNameW(hTestWnd, str, sizeof(str));
    ok(i == lstrlenW(className),
        "GetClassName returned incorrect length");
    ok(!lstrcmpW(className,str),
        "GetClassName returned incorrect name for this window's class");

    /* check GetClassInfo with our hInstance */
    if((test_atom = GetClassInfoW(hInstance, str, &wc)))
    {
        ok(test_atom == classatom,
            "class atom did not match");
        ok(wc.cbClsExtra == cls.cbClsExtra,
            "cbClsExtra did not match");
        ok(wc.cbWndExtra == cls.cbWndExtra,
            "cbWndExtra did not match");
        ok(wc.hbrBackground == cls.hbrBackground,
            "hbrBackground did not match");
        ok(wc.hCursor== cls.hCursor,
            "hCursor did not match");
        ok(wc.hInstance== cls.hInstance,
            "hInstance did not match");
    }
    else
        ok(FALSE,"GetClassInfo (hinstance) failed!");

    /* check GetClassInfo with zero hInstance */
    if(global)
    {
        if((test_atom = GetClassInfoW(0, str, &wc)))
        {
            ok(test_atom == classatom,
                "class atom did not match %x != %x", test_atom, classatom);
            ok(wc.cbClsExtra == cls.cbClsExtra,
                "cbClsExtra did not match %x!=%x",wc.cbClsExtra,cls.cbClsExtra);
            ok(wc.cbWndExtra == cls.cbWndExtra,
                "cbWndExtra did not match %x!=%x",wc.cbWndExtra,cls.cbWndExtra);
            ok(wc.hbrBackground == cls.hbrBackground,
                "hbrBackground did not match %p!=%p",wc.hbrBackground,cls.hbrBackground);
            ok(wc.hCursor== cls.hCursor,
                "hCursor did not match %p!=%p",wc.hCursor,cls.hCursor);
            ok(!wc.hInstance,
                "hInstance not zero for global class %p",wc.hInstance);
        }
        else
            ok(FALSE,"GetClassInfo (0) failed for global class!");
    }
    else
    {
        ok(!GetClassInfoW(0, str, &wc),
            "GetClassInfo (0) succeeded for local class!");
    }

    ok(!UnregisterClassW(className, hInstance),
        "Unregister class succeeded with window existing");

    ok(DestroyWindow(hTestWnd),
        "DestroyWindow() failed!");

    ok(UnregisterClassW(className, hInstance),
        "UnregisterClass() failed");

    return;
}

START_TEST(class)
{
    HANDLE hInstance = GetModuleHandleA( NULL );

    ClassTest(hInstance,FALSE);
    ClassTest(hInstance,TRUE);
}
