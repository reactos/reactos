/* Unit test suite for window classes.
 *
 * Copyright 2002 Mike McCormack
 * Copyright 2003 Alexandre Julliard
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

/* To get CS_DROPSHADOW with the MSVC headers */
#define _WIN32_WINNT 0x0501

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

/* we don't want to include commctrl.h: */
static const CHAR WC_EDITA[] = "Edit";
static const WCHAR WC_EDITW[] = {'E','d','i','t',0};

#define NUMCLASSWORDS 4

#define IS_WNDPROC_HANDLE(x) (((ULONG_PTR)(x) >> 16) == (~((ULONG_PTR)0) >> 16))

static LRESULT WINAPI ClassTest_WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcW (hWnd, msg, wParam, lParam);
}

static LRESULT WINAPI ClassTest_WndProc2 (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcA (hWnd, msg, wParam, lParam);
}

/***********************************************************************
 */
static void ClassTest(HINSTANCE hInstance, BOOL global)
{
    WNDCLASSW cls, wc;
    static const WCHAR className[] = {'T','e','s','t','C','l','a','s','s',0};
    static const WCHAR winName[]   = {'W','i','n','C','l','a','s','s','T','e','s','t',0};
    ATOM test_atom;
    HWND hTestWnd;
    LONG i;
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
    ok(classatom, "failed to register class\n");

    ok(!RegisterClassW (&cls),
        "RegisterClass of the same class should fail for the second time\n");

    /* Setup windows */
    hTestWnd = CreateWindowW (className, winName,
       WS_OVERLAPPEDWINDOW + WS_HSCROLL + WS_VSCROLL,
       CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0,
       0, hInstance, 0);

    ok(hTestWnd!=0, "Failed to create window\n");

    /* test initial values of valid classwords */
    for(i=0; i<NUMCLASSWORDS; i++)
    {
        SetLastError(0);
        ok(!GetClassLongW(hTestWnd,i*sizeof (DWORD)),
            "GetClassLongW initial value nonzero!\n");
        ok(!GetLastError(),
            "GetClassLongW failed!\n");
    }

    if (0)
    {
    /*
     *  GetClassLongW(hTestWnd, NUMCLASSWORDS*sizeof(DWORD))
     *  does not fail on Win 98, though MSDN says it should
     */
    SetLastError(0);
    GetClassLongW(hTestWnd, NUMCLASSWORDS*sizeof(DWORD));
    ok(GetLastError(),
        "GetClassLongW() with invalid offset did not fail\n");
    }

    /* set values of valid class words */
    for(i=0; i<NUMCLASSWORDS; i++)
    {
        SetLastError(0);
        ok(!SetClassLongW(hTestWnd,i*sizeof(DWORD),i+1),
            "GetClassLongW(%d) initial value nonzero!\n",i);
        ok(!GetLastError(),
            "SetClassLongW(%d) failed!\n",i);
    }

    /* test values of valid classwords that we set */
    for(i=0; i<NUMCLASSWORDS; i++)
    {
        SetLastError(0);
        ok( (i+1) == GetClassLongW(hTestWnd,i*sizeof (DWORD)),
            "GetClassLongW value doesn't match what was set!\n");
        ok(!GetLastError(),
            "GetClassLongW failed!\n");
    }

    /* check GetClassName */
    i = GetClassNameW(hTestWnd, str, sizeof(str)/sizeof(str[0]));
    ok(i == lstrlenW(className),
        "GetClassName returned incorrect length\n");
    ok(!lstrcmpW(className,str),
        "GetClassName returned incorrect name for this window's class\n");

    /* check GetClassInfo with our hInstance */
    if((test_atom = GetClassInfoW(hInstance, str, &wc)))
    {
        ok(test_atom == classatom,
            "class atom did not match\n");
        ok(wc.cbClsExtra == cls.cbClsExtra,
            "cbClsExtra did not match\n");
        ok(wc.cbWndExtra == cls.cbWndExtra,
            "cbWndExtra did not match\n");
        ok(wc.hbrBackground == cls.hbrBackground,
            "hbrBackground did not match\n");
        ok(wc.hCursor== cls.hCursor,
            "hCursor did not match\n");
        ok(wc.hInstance== cls.hInstance,
            "hInstance did not match\n");
    }
    else
        ok(FALSE,"GetClassInfo (hinstance) failed!\n");

    /* check GetClassInfo with zero hInstance */
    if(global)
    {
        if((test_atom = GetClassInfoW(0, str, &wc)))
        {
            ok(test_atom == classatom,
                "class atom did not match %x != %x\n", test_atom, classatom);
            ok(wc.cbClsExtra == cls.cbClsExtra,
                "cbClsExtra did not match %x!=%x\n",wc.cbClsExtra,cls.cbClsExtra);
            ok(wc.cbWndExtra == cls.cbWndExtra,
                "cbWndExtra did not match %x!=%x\n",wc.cbWndExtra,cls.cbWndExtra);
            ok(wc.hbrBackground == cls.hbrBackground,
                "hbrBackground did not match %p!=%p\n",wc.hbrBackground,cls.hbrBackground);
            ok(wc.hCursor== cls.hCursor,
                "hCursor did not match %p!=%p\n",wc.hCursor,cls.hCursor);
            ok(!wc.hInstance,
                "hInstance not zero for global class %p\n",wc.hInstance);
        }
        else
            ok(FALSE,"GetClassInfo (0) failed for global class!\n");
    }
    else
    {
        ok(!GetClassInfoW(0, str, &wc),
            "GetClassInfo (0) succeeded for local class!\n");
    }

    ok(!UnregisterClassW(className, hInstance),
        "Unregister class succeeded with window existing\n");

    ok(DestroyWindow(hTestWnd),
        "DestroyWindow() failed!\n");

    ok(UnregisterClassW(className, hInstance),
        "UnregisterClass() failed\n");

    return;
}

static void check_style( const char *name, int must_exist, UINT style, UINT ignore )
{
    WNDCLASS wc;

    if (GetClassInfo( 0, name, &wc ))
    {
        ok( !(~wc.style & style & ~ignore), "System class %s is missing bits %x (%08x/%08x)\n",
            name, ~wc.style & style, wc.style, style );
        ok( !(wc.style & ~style), "System class %s has extra bits %x (%08x/%08x)\n",
            name, wc.style & ~style, wc.style, style );
    }
    else
        ok( !must_exist, "System class %s does not exist\n", name );
}

/* test styles of system classes */
static void test_styles(void)
{
    /* check style bits */
    check_style( "Button",     1, CS_PARENTDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW, 0 );
    check_style( "ComboBox",   1, CS_PARENTDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW, 0 );
    check_style( "Edit",       1, CS_PARENTDC | CS_DBLCLKS, 0 );
    check_style( "ListBox",    1, CS_PARENTDC | CS_DBLCLKS, CS_PARENTDC /*FIXME*/ );
    check_style( "MDIClient",  1, 0, 0 );
    check_style( "ScrollBar",  1, CS_PARENTDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW, 0 );
    check_style( "Static",     1, CS_PARENTDC | CS_DBLCLKS, 0 );
    check_style( "ComboLBox",  1, CS_SAVEBITS | CS_DBLCLKS, 0 );
    check_style( "DDEMLEvent", 0, 0, 0 );
    check_style( "Message",    0, 0, 0 );
    check_style( "#32768",     1, CS_DROPSHADOW | CS_SAVEBITS | CS_DBLCLKS, CS_DROPSHADOW );  /* menu */
    check_style( "#32769",     1, CS_DBLCLKS, 0 );  /* desktop */
    check_style( "#32770",     1, CS_SAVEBITS | CS_DBLCLKS, 0 );  /* dialog */
    todo_wine { check_style( "#32771",     1, CS_SAVEBITS | CS_HREDRAW | CS_VREDRAW, 0 ); } /* task switch */
    check_style( "#32772",     1, 0, 0 );  /* icon title */
}

static void check_class(HINSTANCE inst, const char *name, const char *menu_name)
{
    WNDCLASS wc;
    UINT atom = GetClassInfo(inst,name,&wc);
    ok( atom, "Class %s %p not found\n", name, inst );
    if (atom)
    {
        if (wc.lpszMenuName && menu_name)
            ok( !strcmp( menu_name, wc.lpszMenuName ), "Wrong name %s/%s for class %s %p\n",
                wc.lpszMenuName, menu_name, name, inst );
        else
            ok( !menu_name == !wc.lpszMenuName, "Wrong name %p/%p for class %s %p\n",
                wc.lpszMenuName, menu_name, name, inst );
    }
}

static void check_instance( const char *name, HINSTANCE inst, HINSTANCE info_inst, HINSTANCE gcl_inst )
{
    WNDCLASSA wc;
    HWND hwnd;

    ok( GetClassInfo( inst, name, &wc ), "Couldn't find class %s inst %p\n", name, inst );
    ok( wc.hInstance == info_inst, "Wrong info instance %p/%p for class %s\n",
        wc.hInstance, info_inst, name );
    hwnd = CreateWindowExA( 0, name, "test_window", 0, 0, 0, 0, 0, 0, 0, inst, 0 );
    ok( hwnd != NULL, "Couldn't create window for class %s inst %p\n", name, inst );
    ok( (HINSTANCE)GetClassLongPtrA( hwnd, GCLP_HMODULE ) == gcl_inst,
        "Wrong GCL instance %p/%p for class %s\n",
        (HINSTANCE)GetClassLongPtrA( hwnd, GCLP_HMODULE ), gcl_inst, name );
    ok( (HINSTANCE)GetWindowLongPtrA( hwnd, GWLP_HINSTANCE ) == inst,
        "Wrong GWL instance %p/%p for window %s\n",
        (HINSTANCE)GetWindowLongPtrA( hwnd, GWLP_HINSTANCE ), inst, name );
    ok(!UnregisterClassA(name, inst), "UnregisterClassA should fail while exists a class window\n");
    ok(GetLastError() == ERROR_CLASS_HAS_WINDOWS, "GetLastError() should be set to ERROR_CLASS_HAS_WINDOWS not %d\n", GetLastError());
    DestroyWindow(hwnd);
}

struct class_info
{
    const char *name;
    HINSTANCE inst, info_inst, gcl_inst;
};

static DWORD WINAPI thread_proc(void *param)
{
    struct class_info *class_info = (struct class_info *)param;

    check_instance(class_info->name, class_info->inst, class_info->info_inst, class_info->gcl_inst);

    return 0;
}

static void check_thread_instance( const char *name, HINSTANCE inst, HINSTANCE info_inst, HINSTANCE gcl_inst )
{
    HANDLE hThread;
    DWORD tid;
    struct class_info class_info;

    class_info.name = name;
    class_info.inst = inst;
    class_info.info_inst = info_inst;
    class_info.gcl_inst = gcl_inst;

    hThread = CreateThread(NULL, 0, thread_proc, &class_info, 0, &tid);
    ok(hThread != NULL, "CreateThread failed, error %d\n", GetLastError());
    ok(WaitForSingleObject(hThread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(hThread);
}

/* test various instance parameters */
static void test_instances(void)
{
    WNDCLASSA cls, wc;
    HWND hwnd, hwnd2;
    const char *name = "__test__";
    HINSTANCE kernel32 = GetModuleHandleA("kernel32");
    HINSTANCE user32 = GetModuleHandleA("user32");
    HINSTANCE main_module = GetModuleHandleA(NULL);
    DWORD r;
    char buffer[0x10];

    memset( &cls, 0, sizeof(cls) );
    cls.style         = CS_HREDRAW | CS_VREDRAW;
    cls.lpfnWndProc   = ClassTest_WndProc;
    cls.cbClsExtra    = 0;
    cls.cbWndExtra    = 0;
    cls.lpszClassName = name;

    cls.lpszMenuName  = "main_module";
    cls.hInstance = main_module;

    ok( RegisterClassA( &cls ), "Failed to register local class for main module\n" );
    check_class( main_module, name, "main_module" );
    check_instance( name, main_module, main_module, main_module );
    check_thread_instance( name, main_module, main_module, main_module );

    cls.lpszMenuName  = "kernel32";
    cls.hInstance = kernel32;
    ok( RegisterClassA( &cls ), "Failed to register local class for kernel32\n" );
    check_class( kernel32, name, "kernel32" );
    check_class( main_module, name, "main_module" );
    check_instance( name, kernel32, kernel32, kernel32 );
    check_thread_instance( name, kernel32, kernel32, kernel32 );
    ok( UnregisterClassA( name, kernel32 ), "Unregister failed for kernel32\n" );

    /* Bug 2631 - Supplying an invalid number of bytes fails */
    cls.cbClsExtra    = 0;
    cls.cbWndExtra    = -1;
    SetLastError(0xdeadbeef);
    ok( ((RegisterClassA( &cls ) == 0) && (GetLastError() == ERROR_INVALID_PARAMETER)),
          "Failed with invalid number of WndExtra bytes\n");

    cls.cbClsExtra    = -1;
    cls.cbWndExtra    = 0;
    SetLastError(0xdeadbeef);
    ok( ((RegisterClassA( &cls ) == 0) && (GetLastError() == ERROR_INVALID_PARAMETER)),
          "Failed with invalid number of ClsExtra bytes\n");

    cls.cbClsExtra    = -1;
    cls.cbWndExtra    = -1;
    SetLastError(0xdeadbeef);
    ok( ((RegisterClassA( &cls ) == 0) && (GetLastError() == ERROR_INVALID_PARAMETER)),
          "Failed with invalid number of ClsExtra and cbWndExtra bytes\n");

    cls.cbClsExtra    = 0;
    cls.cbWndExtra    = 0;
    SetLastError(0xdeadbeef);

    /* setting global flag doesn't change status of class */
    hwnd = CreateWindowExA( 0, name, "test", 0, 0, 0, 0, 0, 0, 0, main_module, 0 );
    SetClassLongA( hwnd, GCL_STYLE, CS_GLOBALCLASS );
    cls.lpszMenuName  = "kernel32";
    cls.hInstance = kernel32;
    ok( RegisterClassA( &cls ), "Failed to register local class for kernel32\n" );
    check_class( kernel32, name, "kernel32" );
    check_class( main_module, name, "main_module" );
    check_instance( name, kernel32, kernel32, kernel32 );
    check_instance( name, main_module, main_module, main_module );
    check_thread_instance( name, kernel32, kernel32, kernel32 );
    check_thread_instance( name, main_module, main_module, main_module );
    ok( UnregisterClassA( name, kernel32 ), "Unregister failed for kernel32\n" );

    /* changing the instance doesn't make it global */
    SetClassLongPtrA( hwnd, GCLP_HMODULE, 0 );
    ok( RegisterClassA( &cls ), "Failed to register local class for kernel32\n" );
    check_class( kernel32, name, "kernel32" );
    check_instance( name, kernel32, kernel32, kernel32 );
    check_thread_instance( name, kernel32, kernel32, kernel32 );
    ok( !GetClassInfo( 0, name, &wc ), "Class found with null instance\n" );
    ok( UnregisterClassA( name, kernel32 ), "Unregister failed for kernel32\n" );

    /* GetClassInfo with instance 0 finds user32 instance */
    SetClassLongPtrA( hwnd, GCLP_HMODULE, (LONG_PTR)user32 );
    ok( RegisterClassA( &cls ), "Failed to register local class for kernel32\n" );
    check_class( kernel32, name, "kernel32" );
    check_class( user32, name, "main_module" );
    check_class( 0, name, "main_module" );
    check_instance( name, kernel32, kernel32, kernel32 );
    check_instance( name, user32, 0, user32 );
    check_instance( name, 0, 0, kernel32 );
    check_thread_instance( name, kernel32, kernel32, kernel32 );
    check_thread_instance( name, user32, 0, user32 );
    check_thread_instance( name, 0, 0, kernel32 );
    ok( UnregisterClassA( name, kernel32 ), "Unregister failed for kernel32\n" );

    SetClassLongPtrA( hwnd, GCLP_HMODULE, 0x12345678 );
    ok( RegisterClassA( &cls ), "Failed to register local class for kernel32\n" );
    check_class( kernel32, name, "kernel32" );
    check_class( (HINSTANCE)0x12345678, name, "main_module" );
    check_instance( name, kernel32, kernel32, kernel32 );
    check_instance( name, (HINSTANCE)0x12345678, (HINSTANCE)0x12345678, (HINSTANCE)0x12345678 );
    check_thread_instance( name, kernel32, kernel32, kernel32 );
    check_thread_instance( name, (HINSTANCE)0x12345678, (HINSTANCE)0x12345678, (HINSTANCE)0x12345678 );
    ok( !GetClassInfo( 0, name, &wc ), "Class found with null instance\n" );

    /* creating a window with instance 0 uses the first class found */
    cls.hInstance = (HINSTANCE)0xdeadbeef;
    cls.lpszMenuName = "deadbeef";
    cls.style = 3;
    ok( RegisterClassA( &cls ), "Failed to register local class for deadbeef\n" );
    hwnd2 = CreateWindowExA( 0, name, "test_window", 0, 0, 0, 0, 0, 0, 0, NULL, 0 );
    ok( (HINSTANCE)GetClassLongPtrA( hwnd2, GCLP_HMODULE ) == (HINSTANCE)0xdeadbeef,
        "Didn't get deadbeef class for null instance\n" );
    DestroyWindow( hwnd2 );
    ok( UnregisterClassA( name, (HINSTANCE)0xdeadbeef ), "Unregister failed for deadbeef\n" );

    hwnd2 = CreateWindowExA( 0, name, "test_window", 0, 0, 0, 0, 0, 0, 0, NULL, 0 );
    ok( (HINSTANCE)GetClassLongPtrA( hwnd2, GCLP_HMODULE ) == kernel32,
        "Didn't get kernel32 class for null instance\n" );
    DestroyWindow( hwnd2 );

    r = GetClassName( hwnd, buffer, 4 );
    ok( r == 3, "expected 3, got %d\n", r );
    ok( !strcmp( buffer, "__t"), "name wrong: %s\n", buffer );

    ok( UnregisterClassA( name, kernel32 ), "Unregister failed for kernel32\n" );

    hwnd2 = CreateWindowExA( 0, name, "test_window", 0, 0, 0, 0, 0, 0, 0, NULL, 0 );
    ok( GetClassLongPtrA( hwnd2, GCLP_HMODULE ) == 0x12345678,
        "Didn't get 12345678 class for null instance\n" );
    DestroyWindow( hwnd2 );

    SetClassLongPtrA( hwnd, GCLP_HMODULE, (LONG_PTR)main_module );
    DestroyWindow( hwnd );

    /* null handle means the same thing as main module */
    cls.lpszMenuName  = "null";
    cls.hInstance = 0;
    ok( !RegisterClassA( &cls ), "Succeeded registering local class for null instance\n" );
    ok( GetLastError() == ERROR_CLASS_ALREADY_EXISTS, "Wrong error code %d\n", GetLastError() );
    ok( UnregisterClassA( name, main_module ), "Unregister failed for main module\n" );

    ok( RegisterClassA( &cls ), "Failed to register local class for null instance\n" );
    /* must be found with main module handle */
    check_class( main_module, name, "null" );
    check_instance( name, main_module, main_module, main_module );
    check_thread_instance( name, main_module, main_module, main_module );
    ok( !GetClassInfo( 0, name, &wc ), "Class found with null instance\n" );
    ok( GetLastError() == ERROR_CLASS_DOES_NOT_EXIST, "Wrong error code %d\n", GetLastError() );
    ok( UnregisterClassA( name, 0 ), "Unregister failed for null instance\n" );

    /* registering for user32 always fails */
    cls.lpszMenuName = "user32";
    cls.hInstance = user32;
    ok( !RegisterClassA( &cls ), "Succeeded registering local class for user32\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "Wrong error code %d\n", GetLastError() );
    cls.style |= CS_GLOBALCLASS;
    ok( !RegisterClassA( &cls ), "Succeeded registering global class for user32\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "Wrong error code %d\n", GetLastError() );

    /* unregister is OK though */
    cls.hInstance = main_module;
    ok( RegisterClassA( &cls ), "Failed to register global class for main module\n" );
    ok( UnregisterClassA( name, user32 ), "Unregister failed for user32\n" );

    /* instance doesn't matter for global class */
    cls.style |= CS_GLOBALCLASS;
    cls.lpszMenuName  = "main_module";
    cls.hInstance = main_module;
    ok( RegisterClassA( &cls ), "Failed to register global class for main module\n" );
    cls.lpszMenuName  = "kernel32";
    cls.hInstance = kernel32;
    ok( !RegisterClassA( &cls ), "Succeeded registering local class for kernel32\n" );
    ok( GetLastError() == ERROR_CLASS_ALREADY_EXISTS, "Wrong error code %d\n", GetLastError() );
    /* even if global flag is cleared */
    hwnd = CreateWindowExA( 0, name, "test", 0, 0, 0, 0, 0, 0, 0, main_module, 0 );
    SetClassLongA( hwnd, GCL_STYLE, 0 );
    ok( !RegisterClassA( &cls ), "Succeeded registering local class for kernel32\n" );
    ok( GetLastError() == ERROR_CLASS_ALREADY_EXISTS, "Wrong error code %d\n", GetLastError() );

    check_class( main_module, name, "main_module" );
    check_class( kernel32, name, "main_module" );
    check_class( 0, name, "main_module" );
    check_class( (HINSTANCE)0x12345678, name, "main_module" );
    check_instance( name, main_module, main_module, main_module );
    check_instance( name, (HINSTANCE)0xdeadbeef, (HINSTANCE)0xdeadbeef, main_module );
    check_thread_instance( name, main_module, main_module, main_module );
    check_thread_instance( name, (HINSTANCE)0xdeadbeef, (HINSTANCE)0xdeadbeef, main_module );

    /* changing the instance for global class doesn't make much difference */
    SetClassLongPtrA( hwnd, GCLP_HMODULE, 0xdeadbeef );
    check_instance( name, main_module, main_module, (HINSTANCE)0xdeadbeef );
    check_instance( name, (HINSTANCE)0xdeadbeef, (HINSTANCE)0xdeadbeef, (HINSTANCE)0xdeadbeef );
    check_thread_instance( name, main_module, main_module, (HINSTANCE)0xdeadbeef );
    check_thread_instance( name, (HINSTANCE)0xdeadbeef, (HINSTANCE)0xdeadbeef, (HINSTANCE)0xdeadbeef );

    DestroyWindow( hwnd );
    ok( UnregisterClassA( name, (HINSTANCE)0x87654321 ), "Unregister failed for main module global\n" );
    ok( !UnregisterClassA( name, (HINSTANCE)0x87654321 ), "Unregister succeeded the second time\n" );
    ok( GetLastError() == ERROR_CLASS_DOES_NOT_EXIST, "Wrong error code %d\n", GetLastError() );

    cls.hInstance = (HINSTANCE)0x12345678;
    ok( RegisterClassA( &cls ), "Failed to register global class for dummy instance\n" );
    check_instance( name, main_module, main_module, (HINSTANCE)0x12345678 );
    check_instance( name, (HINSTANCE)0xdeadbeef, (HINSTANCE)0xdeadbeef, (HINSTANCE)0x12345678 );
    check_thread_instance( name, main_module, main_module, (HINSTANCE)0x12345678 );
    check_thread_instance( name, (HINSTANCE)0xdeadbeef, (HINSTANCE)0xdeadbeef, (HINSTANCE)0x12345678 );
    ok( UnregisterClassA( name, (HINSTANCE)0x87654321 ), "Unregister failed for main module global\n" );

    /* check system classes */

    /* we cannot register a global class with the name of a system class */
    cls.style |= CS_GLOBALCLASS;
    cls.lpszMenuName  = "button_main_module";
    cls.lpszClassName = "BUTTON";
    cls.hInstance = main_module;
    ok( !RegisterClassA( &cls ), "Succeeded registering global button class for main module\n" );
    ok( GetLastError() == ERROR_CLASS_ALREADY_EXISTS, "Wrong error code %d\n", GetLastError() );
    cls.hInstance = kernel32;
    ok( !RegisterClassA( &cls ), "Succeeded registering global button class for kernel32\n" );
    ok( GetLastError() == ERROR_CLASS_ALREADY_EXISTS, "Wrong error code %d\n", GetLastError() );

    /* local class is OK however */
    cls.style &= ~CS_GLOBALCLASS;
    cls.lpszMenuName  = "button_main_module";
    cls.hInstance = main_module;
    ok( RegisterClassA( &cls ), "Failed to register local button class for main module\n" );
    check_class( main_module, "BUTTON", "button_main_module" );
    cls.lpszMenuName  = "button_kernel32";
    cls.hInstance = kernel32;
    ok( RegisterClassA( &cls ), "Failed to register local button class for kernel32\n" );
    check_class( kernel32, "BUTTON", "button_kernel32" );
    check_class( main_module, "BUTTON", "button_main_module" );
    ok( UnregisterClassA( "BUTTON", kernel32 ), "Unregister failed for kernel32 button\n" );
    ok( UnregisterClassA( "BUTTON", main_module ), "Unregister failed for main module button\n" );
    /* GetClassInfo sets instance to passed value for global classes */
    check_instance( "BUTTON", 0, 0, user32 );
    check_instance( "BUTTON", (HINSTANCE)0xdeadbeef, (HINSTANCE)0xdeadbeef, user32 );
    check_instance( "BUTTON", user32, 0, user32 );
    check_thread_instance( "BUTTON", 0, 0, user32 );
    check_thread_instance( "BUTTON", (HINSTANCE)0xdeadbeef, (HINSTANCE)0xdeadbeef, user32 );
    check_thread_instance( "BUTTON", user32, 0, user32 );

    /* we can unregister system classes */
    ok( GetClassInfo( 0, "BUTTON", &wc ), "Button class not found with null instance\n" );
    ok( GetClassInfo( kernel32, "BUTTON", &wc ), "Button class not found with kernel32\n" );
    ok( UnregisterClass( "BUTTON", (HINSTANCE)0x12345678 ), "Failed to unregister button\n" );
    ok( !UnregisterClass( "BUTTON", (HINSTANCE)0x87654321 ), "Unregistered button a second time\n" );
    ok( GetLastError() == ERROR_CLASS_DOES_NOT_EXIST, "Wrong error code %d\n", GetLastError() );
    ok( !GetClassInfo( 0, "BUTTON", &wc ), "Button still exists\n" );
    ok( GetLastError() == ERROR_CLASS_DOES_NOT_EXIST ||
        GetLastError() == ERROR_INVALID_PARAMETER || /* W2K3 */
        GetLastError() == ERROR_SUCCESS /* Vista */,
        "Wrong error code %d\n", GetLastError() );

    /* we can change the instance of a system class */
    check_instance( "EDIT", (HINSTANCE)0xdeadbeef, (HINSTANCE)0xdeadbeef, user32 );
    check_thread_instance( "EDIT", (HINSTANCE)0xdeadbeef, (HINSTANCE)0xdeadbeef, user32 );
    hwnd = CreateWindowExA( 0, "EDIT", "test", 0, 0, 0, 0, 0, 0, 0, main_module, 0 );
    SetClassLongPtrA( hwnd, GCLP_HMODULE, 0xdeadbeef );
    check_instance( "EDIT", (HINSTANCE)0x12345678, (HINSTANCE)0x12345678, (HINSTANCE)0xdeadbeef );
    check_thread_instance( "EDIT", (HINSTANCE)0x12345678, (HINSTANCE)0x12345678, (HINSTANCE)0xdeadbeef );
}

static void test_builtinproc(void)
{
    /* Edit behaves differently */
    static const CHAR NORMAL_CLASSES[][10] = {
        "Button",
        "Static",
        "ComboBox",
        "ComboLBox",
        "ListBox",
        "ScrollBar",
        "#32770",  /* dialog */
    };
    static const int NUM_NORMAL_CLASSES = (sizeof(NORMAL_CLASSES)/sizeof(NORMAL_CLASSES[0]));
    static const char classA[] = "deftest";
    static const WCHAR classW[] = {'d','e','f','t','e','s','t',0};
    WCHAR unistring[] = {0x142, 0x40e, 0x3b4, 0};  /* a string that would be destroyed by a W->A->W conversion */
    WNDPROC pDefWindowProcA, pDefWindowProcW;
    WNDPROC oldproc;
    WNDCLASSEXA cls;  /* the memory layout of WNDCLASSEXA and WNDCLASSEXW is the same */
    WCHAR buf[128];
    ATOM atom;
    HWND hwnd;
    int i;

    pDefWindowProcA = (void *)GetProcAddress(GetModuleHandle("user32.dll"), "DefWindowProcA");
    pDefWindowProcW = (void *)GetProcAddress(GetModuleHandle("user32.dll"), "DefWindowProcW");

    for (i = 0; i < 4; i++)
    {
        ZeroMemory(&cls, sizeof(cls));
        cls.cbSize = sizeof(cls);
        cls.hInstance = GetModuleHandle(NULL);
        cls.hbrBackground = GetStockObject (WHITE_BRUSH);
        if (i & 1)
            cls.lpfnWndProc = pDefWindowProcA;
        else
            cls.lpfnWndProc = pDefWindowProcW;

        if (i & 2)
        {
            cls.lpszClassName = classA;
            atom = RegisterClassExA(&cls);
        }
        else
        {
            cls.lpszClassName = (LPCSTR)classW;
            atom = RegisterClassExW((WNDCLASSEXW *)&cls);
        }
        ok(atom != 0, "Couldn't register class, i=%d, %d\n", i, GetLastError());

        hwnd = CreateWindowA(classA, NULL, 0, 0, 0, 100, 100, NULL, NULL, GetModuleHandle(NULL), NULL);
        ok(hwnd != NULL, "Couldn't create window i=%d\n", i);

        ok(GetWindowLongPtrA(hwnd, GWLP_WNDPROC) == (LONG_PTR)pDefWindowProcA, "Wrong ANSI wndproc: %p vs %p\n",
            (void *)GetWindowLongPtrA(hwnd, GWLP_WNDPROC), pDefWindowProcA);
        ok(GetClassLongPtrA(hwnd, GCLP_WNDPROC) == (ULONG_PTR)pDefWindowProcA, "Wrong ANSI wndproc: %p vs %p\n",
            (void *)GetClassLongPtrA(hwnd, GCLP_WNDPROC), pDefWindowProcA);

        ok(GetWindowLongPtrW(hwnd, GWLP_WNDPROC) == (LONG_PTR)pDefWindowProcW, "Wrong Unicode wndproc: %p vs %p\n",
            (void *)GetWindowLongPtrW(hwnd, GWLP_WNDPROC), pDefWindowProcW);
        ok(GetClassLongPtrW(hwnd, GCLP_WNDPROC) == (ULONG_PTR)pDefWindowProcW, "Wrong Unicode wndproc: %p vs %p\n",
            (void *)GetClassLongPtrW(hwnd, GCLP_WNDPROC), pDefWindowProcW);

        DestroyWindow(hwnd);
        UnregisterClass((LPSTR)(DWORD_PTR)atom, GetModuleHandle(NULL));
    }

    /* built-in winproc - window A/W type automatically detected */
    ZeroMemory(&cls, sizeof(cls));
    cls.cbSize = sizeof(cls);
    cls.hInstance = GetModuleHandle(NULL);
    cls.hbrBackground = GetStockObject (WHITE_BRUSH);
    cls.lpszClassName = classA;
    cls.lpfnWndProc = pDefWindowProcW;
    atom = RegisterClassExA(&cls);

    hwnd = CreateWindowExW(0, classW, NULL, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 680, 260, NULL, NULL, GetModuleHandleA(NULL), 0);
    ok(IsWindowUnicode(hwnd), "Windows should be Unicode\n");
    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)pDefWindowProcA);
    ok(IsWindowUnicode(hwnd), "Windows should have remained Unicode\n");
    ok(GetWindowLongPtrW(hwnd, GWLP_WNDPROC) == (LONG_PTR)pDefWindowProcW, "Invalid ANSI winproc\n");
    ok(GetWindowLongPtrA(hwnd, GWLP_WNDPROC) == (LONG_PTR)pDefWindowProcA, "Invalid Unicode winproc\n");
    SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)ClassTest_WndProc);
    ok(IsWindowUnicode(hwnd) == FALSE, "SetWindowLongPtrA should have switched window to ANSI\n");

    DestroyWindow(hwnd);
    UnregisterClass((LPSTR)(DWORD_PTR)atom, GetModuleHandle(NULL));

    /* custom winproc - the same function can be used as both A and W*/
    ZeroMemory(&cls, sizeof(cls));
    cls.cbSize = sizeof(cls);
    cls.hInstance = GetModuleHandle(NULL);
    cls.hbrBackground = GetStockObject (WHITE_BRUSH);
    cls.lpszClassName = classA;
    cls.lpfnWndProc = ClassTest_WndProc2;
    atom = RegisterClassExA(&cls);

    hwnd = CreateWindowExW(0, classW, NULL, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 680, 260, NULL, NULL, GetModuleHandleA(NULL), 0);
    ok(IsWindowUnicode(hwnd) == FALSE, "Window should be ANSI\n");
    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)ClassTest_WndProc);
    ok(IsWindowUnicode(hwnd), "SetWindowLongPtrW should have changed window to Unicode\n");
    SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)ClassTest_WndProc);
    ok(IsWindowUnicode(hwnd) == FALSE, "SetWindowLongPtrA should have changed window to ANSI\n");

    DestroyWindow(hwnd);
    UnregisterClass((LPSTR)(DWORD_PTR)atom, GetModuleHandle(NULL));

    /* For most of the builtin controls both GetWindowLongPtrA and W returns a pointer that is executed directly
     * by CallWindowProcA/W */
    for (i = 0; i < NUM_NORMAL_CLASSES; i++)
    {
        WNDPROC procA, procW;
        hwnd = CreateWindowExA(0, NORMAL_CLASSES[i], classA, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 680, 260,
            NULL, NULL, NULL, 0);
        ok(hwnd != NULL, "Couldn't create window of class %s\n", NORMAL_CLASSES[i]);
        SetWindowText(hwnd, classA);  /* ComboBox needs this */
        procA = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_WNDPROC);
        procW = (WNDPROC)GetWindowLongPtrW(hwnd, GWLP_WNDPROC);
        ok(!IS_WNDPROC_HANDLE(procA), "procA should not be a handle for %s (%p)\n", NORMAL_CLASSES[i], procA);
        ok(!IS_WNDPROC_HANDLE(procW), "procW should not be a handle for %s (%p)\n", NORMAL_CLASSES[i], procW);
        CallWindowProcA(procA, hwnd, WM_GETTEXT, 120, (LPARAM)buf);
        ok(memcmp(buf, classA, sizeof(classA)) == 0, "WM_GETTEXT A/A invalid return for class %s\n", NORMAL_CLASSES[i]);
        CallWindowProcA(procW, hwnd, WM_GETTEXT, 120, (LPARAM)buf);
        ok(memcmp(buf, classW, sizeof(classW)) == 0, "WM_GETTEXT A/W invalid return for class %s\n", NORMAL_CLASSES[i]);
        CallWindowProcW(procA, hwnd, WM_GETTEXT, 120, (LPARAM)buf);
        ok(memcmp(buf, classA, sizeof(classA)) == 0, "WM_GETTEXT W/A invalid return for class %s\n", NORMAL_CLASSES[i]);
        CallWindowProcW(procW, hwnd, WM_GETTEXT, 120, (LPARAM)buf);
        ok(memcmp(buf, classW, sizeof(classW)) == 0, "WM_GETTEXT W/W invalid return for class %s\n", NORMAL_CLASSES[i]);

        oldproc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)ClassTest_WndProc);
        ok(IS_WNDPROC_HANDLE(oldproc) == FALSE, "Class %s shouldn't return a handle\n", NORMAL_CLASSES[i]);
        DestroyWindow(hwnd);
    }

    /* Edit controls are special - they return a wndproc handle when GetWindowLongPtr is called with a different A/W.
     * On the other hand there is no W->A->W conversion so this control is treated specially. */
    hwnd = CreateWindowW(WC_EDITW, unistring, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 680, 260, NULL, NULL, NULL, 0);
    /* GetClassLongPtr returns that both the Unicode and ANSI wndproc */
    ok(IS_WNDPROC_HANDLE(GetClassLongPtrA(hwnd, GCLP_WNDPROC)) == FALSE, "Edit control class should have a Unicode wndproc\n");
    ok(IS_WNDPROC_HANDLE(GetClassLongPtrW(hwnd, GCLP_WNDPROC)) == FALSE, "Edit control class should have a ANSI wndproc\n");
    /* But GetWindowLongPtr returns only a handle for the ANSI one */
    ok(IS_WNDPROC_HANDLE(GetWindowLongPtrA(hwnd, GWLP_WNDPROC)), "Edit control should return a wndproc handle\n");
    ok(!IS_WNDPROC_HANDLE(GetWindowLongPtrW(hwnd, GWLP_WNDPROC)), "Edit control shouldn't return a W wndproc handle\n");
    CallWindowProcW((WNDPROC)GetWindowLongPtrW(hwnd, GWLP_WNDPROC), hwnd, WM_GETTEXT, 120, (LPARAM)buf);
    ok(memcmp(buf, unistring, sizeof(unistring)) == 0, "WM_GETTEXT invalid return\n");
    CallWindowProcA((WNDPROC)GetWindowLongPtrW(hwnd, GWLP_WNDPROC), hwnd, WM_GETTEXT, 120, (LPARAM)buf);
    ok(memcmp(buf, unistring, sizeof(unistring)) == 0, "WM_GETTEXT invalid return\n");
    CallWindowProcW((WNDPROC)GetWindowLongPtrA(hwnd, GWLP_WNDPROC), hwnd, WM_GETTEXT, 120, (LPARAM)buf);
    ok(memcmp(buf, unistring, sizeof(unistring)) == 0, "WM_GETTEXT invalid return\n");

    SetWindowTextW(hwnd, classW);
    CallWindowProcA((WNDPROC)GetWindowLongPtrA(hwnd, GWLP_WNDPROC), hwnd, WM_GETTEXT, 120, (LPARAM)buf);
    ok(memcmp(buf, classA, sizeof(classA)) == 0, "WM_GETTEXT invalid return\n");

    oldproc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)ClassTest_WndProc2);
    /* SetWindowLongPtr returns a wndproc handle - like GetWindowLongPtr */
    ok(IS_WNDPROC_HANDLE(oldproc), "Edit control should return a wndproc handle\n");
    ok(IsWindowUnicode(hwnd) == FALSE, "SetWindowLongPtrA should have changed window to ANSI\n");
    SetWindowTextA(hwnd, classA);  /* Windows resets the title to WideStringToMultiByte(unistring) */
    memset(buf, 0, sizeof(buf));
    CallWindowProcA((WNDPROC)GetWindowLongPtrA(hwnd, GWLP_WNDPROC), hwnd, WM_GETTEXT, 120, (LPARAM)buf);
    ok(memcmp(buf, classA, sizeof(classA)) == 0, "WM_GETTEXT invalid return\n");
    CallWindowProcA((WNDPROC)GetWindowLongPtrW(hwnd, GWLP_WNDPROC), hwnd, WM_GETTEXT, 120, (LPARAM)buf);
    ok(memcmp(buf, classA, sizeof(classA)) == 0, "WM_GETTEXT invalid return\n");
    CallWindowProcW((WNDPROC)GetWindowLongPtrA(hwnd, GWLP_WNDPROC), hwnd, WM_GETTEXT, 120, (LPARAM)buf);
    ok(memcmp(buf, classA, sizeof(classA)) == 0, "WM_GETTEXT invalid return\n");

    CallWindowProcW((WNDPROC)GetWindowLongPtrW(hwnd, GWLP_WNDPROC), hwnd, WM_GETTEXT, 120, (LPARAM)buf);
    ok(memcmp(buf, classW, sizeof(classW)) == 0, "WM_GETTEXT invalid return\n");

    DestroyWindow(hwnd);

    hwnd = CreateWindowA(WC_EDITA, classA, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 680, 260, NULL, NULL, NULL, 0);

    /* GetClassLongPtr returns that both the Unicode and ANSI wndproc */
    ok(!IS_WNDPROC_HANDLE(GetClassLongPtrA(hwnd, GCLP_WNDPROC)), "Edit control class should have a Unicode wndproc\n");
    ok(!IS_WNDPROC_HANDLE(GetClassLongPtrW(hwnd, GCLP_WNDPROC)), "Edit control class should have a ANSI wndproc\n");
    /* But GetWindowLongPtr returns only a handle for the Unicode one */
    ok(!IS_WNDPROC_HANDLE(GetWindowLongPtrA(hwnd, GWLP_WNDPROC)), "Edit control shouldn't return an A wndproc handle\n");
    ok(IS_WNDPROC_HANDLE(GetWindowLongPtrW(hwnd, GWLP_WNDPROC)), "Edit control should return a wndproc handle\n");
    CallWindowProcA((WNDPROC)GetWindowLongPtrA(hwnd, GWLP_WNDPROC), hwnd, WM_GETTEXT, 120, (LPARAM)buf);
    ok(memcmp(buf, classA, sizeof(classA)) == 0, "WM_GETTEXT invalid return\n");
    CallWindowProcA((WNDPROC)GetWindowLongPtrW(hwnd, GWLP_WNDPROC), hwnd, WM_GETTEXT, 120, (LPARAM)buf);
    ok(memcmp(buf, classA, sizeof(classA)) == 0, "WM_GETTEXT invalid return\n");
    CallWindowProcW((WNDPROC)GetWindowLongPtrA(hwnd, GWLP_WNDPROC), hwnd, WM_GETTEXT, 120, (LPARAM)buf);
    ok(memcmp(buf, classA, sizeof(classA)) == 0, "WM_GETTEXT invalid return\n");

    CallWindowProcW((WNDPROC)GetWindowLongPtrW(hwnd, GWLP_WNDPROC), hwnd, WM_GETTEXT, 120, (LPARAM)buf);
    ok(memcmp(buf, classW, sizeof(classW)) == 0, "WM_GETTEXT invalid return\n");

    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)ClassTest_WndProc);
    SetWindowTextW(hwnd, unistring);
    CallWindowProcW((WNDPROC)GetWindowLongPtrW(hwnd, GWLP_WNDPROC), hwnd, WM_GETTEXT, 120, (LPARAM)buf);
    ok(memcmp(buf, unistring, sizeof(unistring)) == 0, "WM_GETTEXT invalid return\n");
    CallWindowProcA((WNDPROC)GetWindowLongPtrW(hwnd, GWLP_WNDPROC), hwnd, WM_GETTEXT, 120, (LPARAM)buf);
    ok(memcmp(buf, unistring, sizeof(unistring)) == 0, "WM_GETTEXT invalid return\n");
    CallWindowProcW((WNDPROC)GetWindowLongPtrA(hwnd, GWLP_WNDPROC), hwnd, WM_GETTEXT, 120, (LPARAM)buf);
    ok(memcmp(buf, unistring, sizeof(unistring)) == 0, "WM_GETTEXT invalid return\n");

    SetWindowTextW(hwnd, classW);
    CallWindowProcA((WNDPROC)GetWindowLongPtrA(hwnd, GWLP_WNDPROC), hwnd, WM_GETTEXT, 120, (LPARAM)buf);
    ok(memcmp(buf, classA, sizeof(classA)) == 0, "WM_GETTEXT invalid return\n");

    DestroyWindow(hwnd);
}


static LRESULT WINAPI TestDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static BOOL RegisterTestDialog(HINSTANCE hInstance)
{
    WNDCLASSEX wcx;
    ATOM atom = 0;

    ZeroMemory(&wcx, sizeof(WNDCLASSEX));
    wcx.cbSize = sizeof(wcx);
    wcx.lpfnWndProc = TestDlgProc;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = DLGWINDOWEXTRA;
    wcx.hInstance = hInstance;
    wcx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcx.hbrBackground = GetStockObject(WHITE_BRUSH);
    wcx.lpszClassName = "TestDialog";
    wcx.lpszMenuName =  "TestDialog";
    wcx.hIconSm = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(5),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR);

    atom = RegisterClassEx(&wcx);
    ok(atom != 0, "RegisterClassEx returned 0\n");

    return atom;
}

/* test registering a dialog box created by using the CLASS directive in a
   resource file, then test creating the dialog using CreateDialogParam. */
static void CreateDialogParamTest(HINSTANCE hInstance)
{
    HWND hWndMain;

    if (RegisterTestDialog(hInstance))
    {
        hWndMain = CreateDialogParam(hInstance, "CLASS_TEST_DIALOG", NULL, 0, 0);
        ok(hWndMain != NULL, "CreateDialogParam returned NULL\n");
        ShowWindow(hWndMain, SW_SHOW);
        DestroyWindow(hWndMain);
    }
}

START_TEST(class)
{
    HANDLE hInstance = GetModuleHandleA( NULL );

    if (!GetModuleHandleW(0))
    {
        trace("Class test is incompatible with Win9x implementation, skipping\n");
        return;
    }

    ClassTest(hInstance,FALSE);
    ClassTest(hInstance,TRUE);
    CreateDialogParamTest(hInstance);
    test_styles();
    test_builtinproc();

    /* this test unregisters the Button class so it should be executed at the end */
    test_instances();
}
