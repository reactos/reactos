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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#define NUMCLASSWORDS 4

static LRESULT WINAPI ClassTest_WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcW (hWnd, msg, wParam, lParam);
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

#if 0
    /*
     *  GetClassLongW(hTestWnd, NUMCLASSWORDS*sizeof(DWORD))
     *  does not fail on Win 98, though MSDN says it should
     */
    SetLastError(0);
    GetClassLongW(hTestWnd, NUMCLASSWORDS*sizeof(DWORD));
    ok(GetLastError(),
        "GetClassLongW() with invalid offset did not fail\n");
#endif

    /* set values of valid class words */
    for(i=0; i<NUMCLASSWORDS; i++)
    {
        SetLastError(0);
        ok(!SetClassLongW(hTestWnd,i*sizeof(DWORD),i+1),
            "GetClassLongW(%ld) initial value nonzero!\n",i*sizeof(DWORD));
        ok(!GetLastError(),
            "SetClassLongW(%ld) failed!\n",i*sizeof(DWORD));
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
    i = GetClassNameW(hTestWnd, str, sizeof(str));
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
    ok( (HINSTANCE)GetClassLongA( hwnd, GCL_HMODULE ) == gcl_inst,
        "Wrong GCL instance %p/%p for class %s\n",
        (HINSTANCE)GetClassLongA( hwnd, GCL_HMODULE ), gcl_inst, name );
    ok( (HINSTANCE)GetWindowLongA( hwnd, GWL_HINSTANCE ) == inst,
        "Wrong GWL instance %p/%p for window %s\n",
        (HINSTANCE)GetWindowLongA( hwnd, GWL_HINSTANCE ), inst, name );
    DestroyWindow(hwnd);
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

    cls.lpszMenuName  = "kernel32";
    cls.hInstance = kernel32;
    ok( RegisterClassA( &cls ), "Failed to register local class for kernel32\n" );
    check_class( kernel32, name, "kernel32" );
    check_class( main_module, name, "main_module" );
    check_instance( name, kernel32, kernel32, kernel32 );
    ok( UnregisterClassA( name, kernel32 ), "Unregister failed for kernel32\n" );

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
    ok( UnregisterClassA( name, kernel32 ), "Unregister failed for kernel32\n" );

    /* changing the instance doesn't make it global */
    SetClassLongA( hwnd, GCL_HMODULE, 0 );
    ok( RegisterClassA( &cls ), "Failed to register local class for kernel32\n" );
    check_class( kernel32, name, "kernel32" );
    check_instance( name, kernel32, kernel32, kernel32 );
    ok( !GetClassInfo( 0, name, &wc ), "Class found with null instance\n" );
    ok( UnregisterClassA( name, kernel32 ), "Unregister failed for kernel32\n" );

    /* GetClassInfo with instance 0 finds user32 instance */
    SetClassLongA( hwnd, GCL_HMODULE, (LONG)user32 );
    ok( RegisterClassA( &cls ), "Failed to register local class for kernel32\n" );
    check_class( kernel32, name, "kernel32" );
    check_class( user32, name, "main_module" );
    check_class( 0, name, "main_module" );
    check_instance( name, kernel32, kernel32, kernel32 );
    check_instance( name, user32, 0, user32 );
    check_instance( name, 0, 0, kernel32 );
    ok( UnregisterClassA( name, kernel32 ), "Unregister failed for kernel32\n" );

    SetClassLongA( hwnd, GCL_HMODULE, 0x12345678 );
    ok( RegisterClassA( &cls ), "Failed to register local class for kernel32\n" );
    check_class( kernel32, name, "kernel32" );
    check_class( (HINSTANCE)0x12345678, name, "main_module" );
    check_instance( name, kernel32, kernel32, kernel32 );
    check_instance( name, (HINSTANCE)0x12345678, (HINSTANCE)0x12345678, (HINSTANCE)0x12345678 );
    ok( !GetClassInfo( 0, name, &wc ), "Class found with null instance\n" );

    /* creating a window with instance 0 uses the first class found */
    cls.hInstance = (HINSTANCE)0xdeadbeef;
    cls.lpszMenuName = "deadbeef";
    cls.style = 3;
    ok( RegisterClassA( &cls ), "Failed to register local class for deadbeef\n" );
    hwnd2 = CreateWindowExA( 0, name, "test_window", 0, 0, 0, 0, 0, 0, 0, NULL, 0 );
    ok( GetClassLong( hwnd2, GCL_HMODULE ) == 0xdeadbeef,
        "Didn't get deadbeef class for null instance\n" );
    DestroyWindow( hwnd2 );
    ok( UnregisterClassA( name, (HINSTANCE)0xdeadbeef ), "Unregister failed for deadbeef\n" );

    hwnd2 = CreateWindowExA( 0, name, "test_window", 0, 0, 0, 0, 0, 0, 0, NULL, 0 );
    ok( (HINSTANCE)GetClassLong( hwnd2, GCL_HMODULE ) == kernel32,
        "Didn't get kernel32 class for null instance\n" );
    DestroyWindow( hwnd2 );

    ok( UnregisterClassA( name, kernel32 ), "Unregister failed for kernel32\n" );

    hwnd2 = CreateWindowExA( 0, name, "test_window", 0, 0, 0, 0, 0, 0, 0, NULL, 0 );
    ok( GetClassLong( hwnd2, GCL_HMODULE ) == 0x12345678,
        "Didn't get 12345678 class for null instance\n" );
    DestroyWindow( hwnd2 );

    SetClassLongA( hwnd, GCL_HMODULE, (LONG)main_module );
    DestroyWindow( hwnd );

    /* null handle means the same thing as main module */
    cls.lpszMenuName  = "null";
    cls.hInstance = 0;
    ok( !RegisterClassA( &cls ), "Succeeded registering local class for null instance\n" );
    ok( GetLastError() == ERROR_CLASS_ALREADY_EXISTS, "Wrong error code %ld\n", GetLastError() );
    ok( UnregisterClassA( name, main_module ), "Unregister failed for main module\n" );

    ok( RegisterClassA( &cls ), "Failed to register local class for null instance\n" );
    /* must be found with main module handle */
    check_class( main_module, name, "null" );
    check_instance( name, main_module, main_module, main_module );
    ok( !GetClassInfo( 0, name, &wc ), "Class found with null instance\n" );
    ok( GetLastError() == ERROR_CLASS_DOES_NOT_EXIST, "Wrong error code %ld\n", GetLastError() );
    ok( UnregisterClassA( name, 0 ), "Unregister failed for null instance\n" );

    /* registering for user32 always fails */
    cls.lpszMenuName = "user32";
    cls.hInstance = user32;
    ok( !RegisterClassA( &cls ), "Succeeded registering local class for user32\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "Wrong error code %ld\n", GetLastError() );
    cls.style |= CS_GLOBALCLASS;
    ok( !RegisterClassA( &cls ), "Succeeded registering global class for user32\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "Wrong error code %ld\n", GetLastError() );

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
    ok( GetLastError() == ERROR_CLASS_ALREADY_EXISTS, "Wrong error code %ld\n", GetLastError() );
    /* even if global flag is cleared */
    hwnd = CreateWindowExA( 0, name, "test", 0, 0, 0, 0, 0, 0, 0, main_module, 0 );
    SetClassLongA( hwnd, GCL_STYLE, 0 );
    ok( !RegisterClassA( &cls ), "Succeeded registering local class for kernel32\n" );
    ok( GetLastError() == ERROR_CLASS_ALREADY_EXISTS, "Wrong error code %ld\n", GetLastError() );

    check_class( main_module, name, "main_module" );
    check_class( kernel32, name, "main_module" );
    check_class( 0, name, "main_module" );
    check_class( (HINSTANCE)0x12345678, name, "main_module" );
    check_instance( name, main_module, main_module, main_module );
    check_instance( name, (HINSTANCE)0xdeadbeef, (HINSTANCE)0xdeadbeef, main_module );

    /* changing the instance for global class doesn't make much difference */
    SetClassLongA( hwnd, GCL_HMODULE, 0xdeadbeef );
    check_instance( name, main_module, main_module, (HINSTANCE)0xdeadbeef );
    check_instance( name, (HINSTANCE)0xdeadbeef, (HINSTANCE)0xdeadbeef, (HINSTANCE)0xdeadbeef );

    DestroyWindow( hwnd );
    ok( UnregisterClassA( name, (HINSTANCE)0x87654321 ), "Unregister failed for main module global\n" );
    ok( !UnregisterClassA( name, (HINSTANCE)0x87654321 ), "Unregister succeeded the second time\n" );
    ok( GetLastError() == ERROR_CLASS_DOES_NOT_EXIST, "Wrong error code %ld\n", GetLastError() );

    cls.hInstance = (HINSTANCE)0x12345678;
    ok( RegisterClassA( &cls ), "Failed to register global class for dummy instance\n" );
    check_instance( name, main_module, main_module, (HINSTANCE)0x12345678 );
    check_instance( name, (HINSTANCE)0xdeadbeef, (HINSTANCE)0xdeadbeef, (HINSTANCE)0x12345678 );
    ok( UnregisterClassA( name, (HINSTANCE)0x87654321 ), "Unregister failed for main module global\n" );

    /* check system classes */

    /* we cannot register a global class with the name of a system class */
    cls.style |= CS_GLOBALCLASS;
    cls.lpszMenuName  = "button_main_module";
    cls.lpszClassName = "BUTTON";
    cls.hInstance = main_module;
    ok( !RegisterClassA( &cls ), "Succeeded registering global button class for main module\n" );
    ok( GetLastError() == ERROR_CLASS_ALREADY_EXISTS, "Wrong error code %ld\n", GetLastError() );
    cls.hInstance = kernel32;
    ok( !RegisterClassA( &cls ), "Succeeded registering global button class for kernel32\n" );
    ok( GetLastError() == ERROR_CLASS_ALREADY_EXISTS, "Wrong error code %ld\n", GetLastError() );

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

    /* we can unregister system classes */
    ok( GetClassInfo( 0, "BUTTON", &wc ), "Button class not found with null instance\n" );
    ok( GetClassInfo( kernel32, "BUTTON", &wc ), "Button class not found with kernel32\n" );
    ok( UnregisterClass( "BUTTON", (HINSTANCE)0x12345678 ), "Failed to unregister button\n" );
    ok( !UnregisterClass( "BUTTON", (HINSTANCE)0x87654321 ), "Unregistered button a second time\n" );
    ok( GetLastError() == ERROR_CLASS_DOES_NOT_EXIST, "Wrong error code %ld\n", GetLastError() );
    ok( !GetClassInfo( 0, "BUTTON", &wc ), "Button still exists\n" );
    ok( GetLastError() == ERROR_CLASS_DOES_NOT_EXIST, "Wrong error code %ld\n", GetLastError() );

    /* we can change the instance of a system class */
    check_instance( "EDIT", (HINSTANCE)0xdeadbeef, (HINSTANCE)0xdeadbeef, user32 );
    hwnd = CreateWindowExA( 0, "EDIT", "test", 0, 0, 0, 0, 0, 0, 0, main_module, 0 );
    SetClassLongA( hwnd, GCL_HMODULE, 0xdeadbeef );
    check_instance( "EDIT", (HINSTANCE)0x12345678, (HINSTANCE)0x12345678, (HINSTANCE)0xdeadbeef );
}

START_TEST(class)
{
    HANDLE hInstance = GetModuleHandleA( NULL );

    ClassTest(hInstance,FALSE);
    ClassTest(hInstance,TRUE);
    test_styles();
    test_instances();
}
