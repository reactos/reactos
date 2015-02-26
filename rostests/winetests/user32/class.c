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
//#define _WIN32_WINNT 0x0501

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"
#include "commctrl.h"

#define NUMCLASSWORDS 4

#define IS_WNDPROC_HANDLE(x) (((ULONG_PTR)(x) >> 16) == (~0u >> 16))

static LRESULT WINAPI ClassTest_WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE) return 1;
    return DefWindowProcW (hWnd, msg, wParam, lParam);
}

static LRESULT WINAPI ClassTest_WndProc2 (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE) return 1;
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
    WNDCLASSA wc;

    if (GetClassInfoA( 0, name, &wc ))
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

static void check_class_(int line, HINSTANCE inst, const char *name, const char *menu_name)
{
    WNDCLASSA wc;
    UINT atom = GetClassInfoA(inst,name,&wc);
    ok_(__FILE__,line)( atom, "Class %s %p not found\n", name, inst );
    if (atom)
    {
        if (wc.lpszMenuName && menu_name)
            ok_(__FILE__,line)( !strcmp( menu_name, wc.lpszMenuName ),
                                "Wrong name %s/%s for class %s %p\n",
                                wc.lpszMenuName, menu_name, name, inst );
        else
            ok_(__FILE__,line)( !menu_name == !wc.lpszMenuName, "Wrong name %p/%p for class %s %p\n",
                                wc.lpszMenuName, menu_name, name, inst );
    }
}
#define check_class(inst,name,menu) check_class_(__LINE__,inst,name,menu)

static void check_instance_( int line, const char *name, HINSTANCE inst,
                             HINSTANCE info_inst, HINSTANCE gcl_inst )
{
    WNDCLASSA wc;
    HWND hwnd;

    ok_(__FILE__,line)( GetClassInfoA( inst, name, &wc ), "Couldn't find class %s inst %p\n", name, inst );
    ok_(__FILE__,line)( wc.hInstance == info_inst, "Wrong info instance %p/%p for class %s\n",
                        wc.hInstance, info_inst, name );
    hwnd = CreateWindowExA( 0, name, "test_window", 0, 0, 0, 0, 0, 0, 0, inst, 0 );
    ok_(__FILE__,line)( hwnd != NULL, "Couldn't create window for class %s inst %p\n", name, inst );
    ok_(__FILE__,line)( (HINSTANCE)GetClassLongPtrA( hwnd, GCLP_HMODULE ) == gcl_inst,
                        "Wrong GCL instance %p/%p for class %s\n",
        (HINSTANCE)GetClassLongPtrA( hwnd, GCLP_HMODULE ), gcl_inst, name );
    ok_(__FILE__,line)( (HINSTANCE)GetWindowLongPtrA( hwnd, GWLP_HINSTANCE ) == inst,
                        "Wrong GWL instance %p/%p for window %s\n",
        (HINSTANCE)GetWindowLongPtrA( hwnd, GWLP_HINSTANCE ), inst, name );
    ok_(__FILE__,line)(!UnregisterClassA(name, inst),
                       "UnregisterClassA should fail while exists a class window\n");
    ok_(__FILE__,line)(GetLastError() == ERROR_CLASS_HAS_WINDOWS,
                       "GetLastError() should be set to ERROR_CLASS_HAS_WINDOWS not %d\n", GetLastError());
    DestroyWindow(hwnd);
}
#define check_instance(name,inst,info_inst,gcl_inst) check_instance_(__LINE__,name,inst,info_inst,gcl_inst)

struct class_info
{
    const char *name;
    HINSTANCE inst, info_inst, gcl_inst;
};

static DWORD WINAPI thread_proc(void *param)
{
    struct class_info *class_info = param;

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
    WNDCLASSEXA wcexA;
    HWND hwnd, hwnd2;
    const char *name = "__test__";
    HINSTANCE kernel32 = GetModuleHandleA("kernel32");
    HINSTANCE user32 = GetModuleHandleA("user32");
    HINSTANCE main_module = GetModuleHandleA(NULL);
    HINSTANCE zero_instance = 0;
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

    ZeroMemory(&wcexA, sizeof(wcexA));
    wcexA.lpfnWndProc = DefWindowProcA;
    wcexA.lpszClassName = "__classex_test__";
    SetLastError(0xdeadbeef);
    wcexA.cbSize = sizeof(wcexA) - 1;
    ok( ((RegisterClassExA( &wcexA ) == 0) && (GetLastError() == ERROR_INVALID_PARAMETER)),
          "Succeeded with invalid number of cbSize bytes\n");
    SetLastError(0xdeadbeef);
    wcexA.cbSize = sizeof(wcexA) + 1;
    ok( ((RegisterClassExA( &wcexA ) == 0) && (GetLastError() == ERROR_INVALID_PARAMETER)),
          "Succeeded with invalid number of cbSize bytes\n");
    SetLastError(0xdeadbeef);
    wcexA.cbSize = sizeof(wcexA);
    ok( RegisterClassExA( &wcexA ), "Failed with valid number of cbSize bytes\n");
    wcexA.cbSize = 0xdeadbeef;
    ok( GetClassInfoExA(main_module, wcexA.lpszClassName, &wcexA), "GetClassInfoEx failed\n");
    ok( wcexA.cbSize == 0xdeadbeef, "GetClassInfoEx returned wrong cbSize value %d\n", wcexA.cbSize);
    UnregisterClassA(wcexA.lpszClassName, main_module);

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
    ok( hwnd != 0, "CreateWindow failed error %u\n", GetLastError());
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
    ok( !GetClassInfoA( 0, name, &wc ), "Class found with null instance\n" );
    ok( UnregisterClassA( name, kernel32 ), "Unregister failed for kernel32\n" );

    /* GetClassInfo with instance 0 finds user32 instance */
    SetClassLongPtrA( hwnd, GCLP_HMODULE, (LONG_PTR)user32 );
    ok( RegisterClassA( &cls ), "Failed to register local class for kernel32\n" );
    if (!GetClassInfoA( 0, name, &wc )) zero_instance = user32; /* instance 0 not supported on wow64 */
    else
    {
        check_instance( name, 0, 0, kernel32 );
        check_thread_instance( name, 0, 0, kernel32 );
    }
    check_class( kernel32, name, "kernel32" );
    check_class( user32, name, "main_module" );
    check_class( zero_instance, name, "main_module" );
    check_instance( name, kernel32, kernel32, kernel32 );
    check_instance( name, user32, zero_instance, user32 );
    check_thread_instance( name, kernel32, kernel32, kernel32 );
    check_thread_instance( name, user32, zero_instance, user32 );
    ok( UnregisterClassA( name, kernel32 ), "Unregister failed for kernel32\n" );

    SetClassLongPtrA( hwnd, GCLP_HMODULE, 0x12345678 );
    ok( RegisterClassA( &cls ), "Failed to register local class for kernel32\n" );
    check_class( kernel32, name, "kernel32" );
    check_class( (HINSTANCE)0x12345678, name, "main_module" );
    check_instance( name, kernel32, kernel32, kernel32 );
    check_instance( name, (HINSTANCE)0x12345678, (HINSTANCE)0x12345678, (HINSTANCE)0x12345678 );
    check_thread_instance( name, kernel32, kernel32, kernel32 );
    check_thread_instance( name, (HINSTANCE)0x12345678, (HINSTANCE)0x12345678, (HINSTANCE)0x12345678 );
    ok( !GetClassInfoA( 0, name, &wc ), "Class found with null instance\n" );

    /* creating a window with instance 0 uses the first class found */
    cls.hInstance = (HINSTANCE)0xdeadbeef;
    cls.lpszMenuName = "deadbeef";
    cls.style = 3;
    ok( RegisterClassA( &cls ), "Failed to register local class for deadbeef\n" );
    hwnd2 = CreateWindowExA( 0, name, "test_window", 0, 0, 0, 0, 0, 0, 0, NULL, 0 );
    ok( GetClassLongPtrA( hwnd2, GCLP_HMODULE ) == 0xdeadbeef,
        "Didn't get deadbeef class for null instance\n" );
    DestroyWindow( hwnd2 );
    ok( UnregisterClassA( name, (HINSTANCE)0xdeadbeef ), "Unregister failed for deadbeef\n" );

    hwnd2 = CreateWindowExA( 0, name, "test_window", 0, 0, 0, 0, 0, 0, 0, NULL, 0 );
    ok( (HINSTANCE)GetClassLongPtrA( hwnd2, GCLP_HMODULE ) == kernel32,
        "Didn't get kernel32 class for null instance\n" );
    DestroyWindow( hwnd2 );

    r = GetClassNameA( hwnd, buffer, 4 );
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
    ok( !GetClassInfoA( 0, name, &wc ), "Class found with null instance\n" );
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
    check_instance( "BUTTON", user32, zero_instance, user32 );
    check_thread_instance( "BUTTON", 0, 0, user32 );
    check_thread_instance( "BUTTON", (HINSTANCE)0xdeadbeef, (HINSTANCE)0xdeadbeef, user32 );
    check_thread_instance( "BUTTON", user32, zero_instance, user32 );

    /* we can unregister system classes */
    ok( GetClassInfoA( 0, "BUTTON", &wc ), "Button class not found with null instance\n" );
    ok( GetClassInfoA( kernel32, "BUTTON", &wc ), "Button class not found with kernel32\n" );
    ok( UnregisterClassA( "BUTTON", (HINSTANCE)0x12345678 ), "Failed to unregister button\n" );
    ok( !UnregisterClassA( "BUTTON", (HINSTANCE)0x87654321 ), "Unregistered button a second time\n" );
    ok( GetLastError() == ERROR_CLASS_DOES_NOT_EXIST, "Wrong error code %d\n", GetLastError() );
    ok( !GetClassInfoA( 0, "BUTTON", &wc ), "Button still exists\n" );
    /* last error not set reliably */

    /* we can change the instance of a system class */
    check_instance( "EDIT", (HINSTANCE)0xdeadbeef, (HINSTANCE)0xdeadbeef, user32 );
    check_thread_instance( "EDIT", (HINSTANCE)0xdeadbeef, (HINSTANCE)0xdeadbeef, user32 );
    hwnd = CreateWindowExA( 0, "EDIT", "test", 0, 0, 0, 0, 0, 0, 0, main_module, 0 );
    SetClassLongPtrA( hwnd, GCLP_HMODULE, 0xdeadbeef );
    check_instance( "EDIT", (HINSTANCE)0x12345678, (HINSTANCE)0x12345678, (HINSTANCE)0xdeadbeef );
    check_thread_instance( "EDIT", (HINSTANCE)0x12345678, (HINSTANCE)0x12345678, (HINSTANCE)0xdeadbeef );
    DestroyWindow(hwnd);
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
    WNDPROC pNtdllDefWindowProcA, pNtdllDefWindowProcW;
    WNDPROC oldproc;
    WNDCLASSEXA cls;  /* the memory layout of WNDCLASSEXA and WNDCLASSEXW is the same */
    WCHAR buf[128];
    ATOM atom;
    HWND hwnd;
    int i;

    pDefWindowProcA = (void *)GetProcAddress(GetModuleHandleA("user32.dll"), "DefWindowProcA");
    pDefWindowProcW = (void *)GetProcAddress(GetModuleHandleA("user32.dll"), "DefWindowProcW");
    pNtdllDefWindowProcA = (void *)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtdllDefWindowProc_A");
    pNtdllDefWindowProcW = (void *)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtdllDefWindowProc_W");

    /* On Vista+, the user32.dll export DefWindowProcA/W is forwarded to  */
    /* ntdll.NtdllDefWindowProc_A/W. However, the wndproc returned by     */
    /* GetClassLong/GetWindowLong points to an unexported user32 function */
    if (pDefWindowProcA == pNtdllDefWindowProcA &&
        pDefWindowProcW == pNtdllDefWindowProcW)
        skip("user32.DefWindowProcX forwarded to ntdll.NtdllDefWindowProc_X\n");
    else
    {
        for (i = 0; i < 4; i++)
        {
            ZeroMemory(&cls, sizeof(cls));
            cls.cbSize = sizeof(cls);
            cls.hInstance = GetModuleHandleA(NULL);
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

            hwnd = CreateWindowA(classA, NULL, 0, 0, 0, 100, 100, NULL, NULL, GetModuleHandleA(NULL), NULL);
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
            UnregisterClassA((LPSTR)(DWORD_PTR)atom, GetModuleHandleA(NULL));
        }
    }

    /* built-in winproc - window A/W type automatically detected */
    ZeroMemory(&cls, sizeof(cls));
    cls.cbSize = sizeof(cls);
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hbrBackground = GetStockObject (WHITE_BRUSH);
    cls.lpszClassName = classA;
    cls.lpfnWndProc = pDefWindowProcW;
    atom = RegisterClassExA(&cls);

    hwnd = CreateWindowExW(0, classW, NULL, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 680, 260, NULL, NULL, GetModuleHandleW(NULL), 0);
    ok(IsWindowUnicode(hwnd), "Windows should be Unicode\n");
    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)pDefWindowProcA);
    ok(IsWindowUnicode(hwnd), "Windows should have remained Unicode\n");
    if (GetWindowLongPtrW(hwnd, GWLP_WNDPROC) == (LONG_PTR)pDefWindowProcA)
    {
        /* DefWindowProc isn't magic on wow64 */
        ok(IS_WNDPROC_HANDLE(GetWindowLongPtrA(hwnd, GWLP_WNDPROC)), "Ansi winproc is not a handle\n");
    }
    else
    {
        ok(GetWindowLongPtrW(hwnd, GWLP_WNDPROC) == (LONG_PTR)pDefWindowProcW, "Invalid Unicode winproc\n");
        ok(GetWindowLongPtrA(hwnd, GWLP_WNDPROC) == (LONG_PTR)pDefWindowProcA, "Invalid Ansi winproc\n");
    }
    SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)ClassTest_WndProc);
    ok(IsWindowUnicode(hwnd) == FALSE, "SetWindowLongPtrA should have switched window to ANSI\n");

    DestroyWindow(hwnd);
    UnregisterClassA((LPSTR)(DWORD_PTR)atom, GetModuleHandleA(NULL));

    /* custom winproc - the same function can be used as both A and W*/
    ZeroMemory(&cls, sizeof(cls));
    cls.cbSize = sizeof(cls);
    cls.hInstance = GetModuleHandleA(NULL);
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
    UnregisterClassA((LPSTR)(DWORD_PTR)atom, GetModuleHandleA(NULL));

    /* For most of the builtin controls both GetWindowLongPtrA and W returns a pointer that is executed directly
     * by CallWindowProcA/W */
    for (i = 0; i < NUM_NORMAL_CLASSES; i++)
    {
        WNDPROC procA, procW;
        hwnd = CreateWindowExA(0, NORMAL_CLASSES[i], classA, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 680, 260,
            NULL, NULL, NULL, 0);
        ok(hwnd != NULL, "Couldn't create window of class %s\n", NORMAL_CLASSES[i]);
        SetWindowTextA(hwnd, classA);  /* ComboBox needs this */
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
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)oldproc);
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

    SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)oldproc);

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

    oldproc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)ClassTest_WndProc);
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

    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)oldproc);

    DestroyWindow(hwnd);
}


static LRESULT WINAPI TestDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

static BOOL RegisterTestDialog(HINSTANCE hInstance)
{
    WNDCLASSEXA wcx;
    ATOM atom = 0;

    ZeroMemory(&wcx, sizeof(WNDCLASSEXA));
    wcx.cbSize = sizeof(wcx);
    wcx.lpfnWndProc = TestDlgProc;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = DLGWINDOWEXTRA;
    wcx.hInstance = hInstance;
    wcx.hIcon = LoadIconA(NULL, (LPCSTR)IDI_APPLICATION);
    wcx.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    wcx.hbrBackground = GetStockObject(WHITE_BRUSH);
    wcx.lpszClassName = "TestDialog";
    wcx.lpszMenuName =  "TestDialog";
    wcx.hIconSm = LoadImageA(hInstance, (LPCSTR)MAKEINTRESOURCE(5), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR);

    atom = RegisterClassExA(&wcx);
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
        hWndMain = CreateDialogParamA(hInstance, "CLASS_TEST_DIALOG", NULL, 0, 0);
        ok(hWndMain != NULL, "CreateDialogParam returned NULL\n");
        ShowWindow(hWndMain, SW_SHOW);
        DestroyWindow(hWndMain);
    }
}

static const struct
{
    const char name[9];
    int value;
    int badvalue;
} extra_values[] =
{
    {"#32770",30,30}, /* Dialog */
#ifdef _WIN64
    {"Edit",8,8},
#else
    {"Edit",6,8},     /* Windows XP 64-bit returns 8 also to 32-bit applications */
#endif
};

static void test_extra_values(void)
{
    int i;
    for(i=0; i< sizeof(extra_values)/sizeof(extra_values[0]); i++)
    {
        WNDCLASSEXA wcx;
        BOOL ret = GetClassInfoExA(NULL,extra_values[i].name,&wcx);

        ok( ret, "GetClassInfo (0) failed for global class %s\n", extra_values[i].name);
        if (!ret) continue;
        ok(extra_values[i].value == wcx.cbWndExtra || broken(extra_values[i].badvalue == wcx.cbWndExtra),
           "expected %d, got %d\n", extra_values[i].value, wcx.cbWndExtra);
    }
}

static void test_GetClassInfo(void)
{
    static const WCHAR staticW[] = {'s','t','a','t','i','c',0};
    WNDCLASSA wc;
    WNDCLASSEXA wcx;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = GetClassInfoA(0, "static", &wc);
    ok(ret, "GetClassInfoA() error %d\n", GetLastError());

if (0) { /* crashes under XP */
    SetLastError(0xdeadbeef);
    ret = GetClassInfoA(0, "static", NULL);
    ok(ret, "GetClassInfoA() error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetClassInfoW(0, staticW, NULL);
    ok(ret, "GetClassInfoW() error %d\n", GetLastError());
}

    wcx.cbSize = sizeof(wcx);
    SetLastError(0xdeadbeef);
    ret = GetClassInfoExA(0, "static", &wcx);
    ok(ret, "GetClassInfoExA() error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetClassInfoExA(0, "static", NULL);
    ok(!ret, "GetClassInfoExA() should fail\n");
    ok(GetLastError() == ERROR_NOACCESS ||
       broken(GetLastError() == 0xdeadbeef), /* win9x */
       "expected ERROR_NOACCESS, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetClassInfoExW(0, staticW, NULL);
    ok(!ret, "GetClassInfoExW() should fail\n");
    ok(GetLastError() == ERROR_NOACCESS ||
       broken(GetLastError() == 0xdeadbeef) /* NT4 */ ||
       broken(GetLastError() == ERROR_CALL_NOT_IMPLEMENTED), /* win9x */
       "expected ERROR_NOACCESS, got %d\n", GetLastError());

    wcx.cbSize = 0;
    wcx.lpfnWndProc = NULL;
    SetLastError(0xdeadbeef);
    ret = GetClassInfoExA(0, "static", &wcx);
    ok(ret, "GetClassInfoExA() error %d\n", GetLastError());
    ok(wcx.cbSize == 0, "expected 0, got %u\n", wcx.cbSize);
    ok(wcx.lpfnWndProc != NULL, "got null proc\n");

    wcx.cbSize = sizeof(wcx) - 1;
    wcx.lpfnWndProc = NULL;
    SetLastError(0xdeadbeef);
    ret = GetClassInfoExA(0, "static", &wcx);
    ok(ret, "GetClassInfoExA() error %d\n", GetLastError());
    ok(wcx.cbSize == sizeof(wcx) - 1, "expected sizeof(wcx)-1, got %u\n", wcx.cbSize);
    ok(wcx.lpfnWndProc != NULL, "got null proc\n");

    wcx.cbSize = sizeof(wcx) + 1;
    wcx.lpfnWndProc = NULL;
    SetLastError(0xdeadbeef);
    ret = GetClassInfoExA(0, "static", &wcx);
    ok(ret, "GetClassInfoExA() error %d\n", GetLastError());
    ok(wcx.cbSize == sizeof(wcx) + 1, "expected sizeof(wcx)+1, got %u\n", wcx.cbSize);
    ok(wcx.lpfnWndProc != NULL, "got null proc\n");
}

static void test_icons(void)
{
    WNDCLASSEXW wcex, ret_wcex;
    WCHAR cls_name[] = {'I','c','o','n','T','e','s','t','C','l','a','s','s',0};
    HWND hwnd;
    HINSTANCE hinst = GetModuleHandleW(0);
    HICON hsmicon, hsmallnew;
    ICONINFO icinf;

    memset(&wcex, 0, sizeof wcex);
    wcex.cbSize        = sizeof wcex;
    wcex.lpfnWndProc   = ClassTest_WndProc;
    wcex.hIcon         = LoadIconW(0, (LPCWSTR)IDI_APPLICATION);
    wcex.hInstance     = hinst;
    wcex.lpszClassName = cls_name;
    ok(RegisterClassExW(&wcex), "RegisterClassExW returned 0\n");
    hwnd = CreateWindowExW(0, cls_name, NULL, WS_OVERLAPPEDWINDOW,
                        0, 0, 0, 0, NULL, NULL, hinst, 0);
    ok(hwnd != NULL, "Window was not created\n");

    ok(GetClassInfoExW(hinst, cls_name, &ret_wcex), "Class info was not retrieved\n");
    ok(wcex.hIcon == ret_wcex.hIcon, "Icons don't match\n");
    ok(ret_wcex.hIconSm != NULL, "hIconSm should be non-zero handle\n");

    hsmicon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICONSM);
    ok(hsmicon != NULL, "GetClassLong should return non-zero handle\n");

    hsmallnew = CopyImage(wcex.hIcon, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                                GetSystemMetrics(SM_CYSMICON), 0);
    ok(!SetClassLongPtrW(hwnd, GCLP_HICONSM, (LONG_PTR)hsmallnew),
                    "Previous hIconSm should be zero\n");
    ok(hsmallnew == (HICON)GetClassLongPtrW(hwnd, GCLP_HICONSM),
                    "Should return explicitly assigned small icon\n");
    ok(!GetIconInfo(hsmicon, &icinf), "Previous small icon should be destroyed\n");

    SetClassLongPtrW(hwnd, GCLP_HICONSM, 0);
    hsmicon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICONSM);
    ok( hsmicon != NULL, "GetClassLong should return non-zero handle\n");

    SetClassLongPtrW(hwnd, GCLP_HICON, 0);
    ok(!GetClassLongPtrW(hwnd, GCLP_HICONSM), "GetClassLong should return zero handle\n");

    SetClassLongPtrW(hwnd, GCLP_HICON, (LONG_PTR)LoadIconW(NULL, (LPCWSTR)IDI_QUESTION));
    hsmicon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICONSM);
    ok(hsmicon != NULL, "GetClassLong should return non-zero handle\n");
    UnregisterClassW(cls_name, hinst);
    ok(GetIconInfo(hsmicon, &icinf), "Icon should NOT be destroyed\n");

    DestroyIcon(hsmallnew);
    DestroyWindow(hwnd);
}

static void test_comctl32_class( const char *name )
{
    WNDCLASSA wcA;
    WNDCLASSW wcW;
    BOOL ret;
    HMODULE module;
    WCHAR nameW[20];
    HWND hwnd;

    module = GetModuleHandleA( "comctl32" );
    ok( !module, "comctl32 already loaded\n" );
    ret = GetClassInfoA( 0, name, &wcA );
    ok( ret || broken(!ret) /* <= winxp */, "GetClassInfoA failed for %s\n", name );
    if (!ret) return;
    MultiByteToWideChar( CP_ACP, 0, name, -1, nameW, sizeof(nameW)/sizeof(WCHAR) );
    ret = GetClassInfoW( 0, nameW, &wcW );
    ok( ret, "GetClassInfoW failed for %s\n", name );
    module = GetModuleHandleA( "comctl32" );
    ok( module != 0, "comctl32 not loaded\n" );
    FreeLibrary( module );
    module = GetModuleHandleA( "comctl32" );
    ok( !module, "comctl32 still loaded\n" );
    hwnd = CreateWindowA( name, "test", WS_OVERLAPPEDWINDOW, 0, 0, 10, 10, NULL, NULL, NULL, 0 );
    ok( hwnd != 0, "failed to create window for %s\n", name );
    module = GetModuleHandleA( "comctl32" );
    ok( module != 0, "comctl32 not loaded\n" );
}

/* verify that comctl32 classes are automatically loaded by user32 */
static void test_comctl32_classes(void)
{
    char path_name[MAX_PATH];
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    char **argv;
    int i;

    static const char *classes[] =
    {
        ANIMATE_CLASSA,
        WC_COMBOBOXEXA,
        DATETIMEPICK_CLASSA,
        WC_HEADERA,
        HOTKEY_CLASSA,
        WC_IPADDRESSA,
        WC_LISTVIEWA,
        MONTHCAL_CLASSA,
        WC_NATIVEFONTCTLA,
        WC_PAGESCROLLERA,
        PROGRESS_CLASSA,
        REBARCLASSNAMEA,
        STATUSCLASSNAMEA,
        WC_TABCONTROLA,
        TOOLBARCLASSNAMEA,
        TOOLTIPS_CLASSA,
        TRACKBAR_CLASSA,
        WC_TREEVIEWA,
        UPDOWN_CLASSA
    };

    winetest_get_mainargs( &argv );
    for (i = 0; i < sizeof(classes) / sizeof(classes[0]); i++)
    {
        memset( &startup, 0, sizeof(startup) );
        startup.cb = sizeof( startup );
        sprintf( path_name, "%s class %s", argv[0], classes[i] );
        ok( CreateProcessA( NULL, path_name, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info ),
            "CreateProcess failed.\n" );
        winetest_wait_child_process( info.hProcess );
        CloseHandle( info.hProcess );
        CloseHandle( info.hThread );
    }
}

static void test_IME(void)
{
    static const WCHAR ime_classW[] = {'I','M','E',0};

    char module_name[MAX_PATH], *ptr;
    MEMORY_BASIC_INFORMATION mbi;
    WNDCLASSW wnd_classw;
    WNDCLASSA wnd_class;
    SIZE_T size;
    BOOL ret;

    if (!GetProcAddress(GetModuleHandleA("user32.dll"), "BroadcastSystemMessageExA"))
    {
        win_skip("BroadcastSystemMessageExA not available, skipping IME class test\n");
        return;
    }

    ok(GetModuleHandleA("imm32") != 0, "imm32.dll is not loaded\n");

    ret = GetClassInfoA(NULL, "IME", &wnd_class);
    ok(ret, "GetClassInfo failed: %d\n", GetLastError());

    size = VirtualQuery(wnd_class.lpfnWndProc, &mbi, sizeof(mbi));
    ok(size == sizeof(mbi), "VirtualQuery returned %ld\n", size);
    if (size == sizeof(mbi)) {
        size = GetModuleFileNameA(mbi.AllocationBase, module_name, sizeof(module_name));
        ok(size, "GetModuleFileName failed\n");
        for (ptr = module_name+size-1; ptr > module_name; ptr--)
            if (*ptr == '\\' || *ptr == '/') break;
        if (*ptr == '\\' || *ptr=='/') ptr++;
        ok(!lstrcmpiA(ptr, "user32.dll") || !lstrcmpiA(ptr, "ntdll.dll"), "IME window proc implemented in %s\n", ptr);
    }

    ret = GetClassInfoW(NULL, ime_classW, &wnd_classw);
    ok(ret, "GetClassInfo failed: %d\n", GetLastError());

    size = VirtualQuery(wnd_classw.lpfnWndProc, &mbi, sizeof(mbi));
    ok(size == sizeof(mbi), "VirtualQuery returned %ld\n", size);
    size = GetModuleFileNameA(mbi.AllocationBase, module_name, sizeof(module_name));
    ok(size, "GetModuleFileName failed\n");
    for (ptr = module_name+size-1; ptr > module_name; ptr--)
        if (*ptr == '\\' || *ptr == '/') break;
    if (*ptr == '\\' || *ptr=='/') ptr++;
    ok(!lstrcmpiA(ptr, "user32.dll") || !lstrcmpiA(ptr, "ntdll.dll"), "IME window proc implemented in %s\n", ptr);
}

START_TEST(class)
{
    char **argv;
    HANDLE hInstance = GetModuleHandleA( NULL );
    int argc = winetest_get_mainargs( &argv );

    if (argc >= 3)
    {
        test_comctl32_class( argv[2] );
        return;
    }

    test_IME();
    test_GetClassInfo();
    test_extra_values();

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
    test_icons();
    test_comctl32_classes();

    /* this test unregisters the Button class so it should be executed at the end */
    test_instances();
}
