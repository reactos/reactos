/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for RegisterClassEx
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include <apitest.h>
#define WIN32_NO_STATUS
#include <ndk/rtlfuncs.h>

#include <wingdi.h>
#include <winuser.h>
#include "helper.h"
#include <undocuser.h>

static ATOM _RegisterClass(LPCWSTR lpwszClassName, HINSTANCE hInstance, UINT style, WNDPROC lpfnWndProc)
{
    WNDCLASSEXW wcex = {sizeof(WNDCLASSEXW), style, lpfnWndProc};
    wcex.lpszClassName  = lpwszClassName;
    wcex.hInstance      = hInstance;
    return RegisterClassExW(&wcex);
}

static ATOM _GetClassAtom(LPCWSTR lpwszClassName, HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = {sizeof(WNDCLASSEXW)};
    return (ATOM)GetClassInfoEx(hInstance, lpwszClassName, &wcex);
}

static WNDPROC _GetWndproc(LPCWSTR lpwszClassName, HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = {sizeof(WNDCLASSEXW)};
    GetClassInfoEx(hInstance, lpwszClassName, &wcex);
    return wcex.lpfnWndProc;
}

HANDLE _CreateActCtxFromFile(LPCWSTR FileName)
{
    ACTCTXW ActCtx = {sizeof(ACTCTX)};
    WCHAR buffer[MAX_PATH] , *separator;

    ok (GetModuleFileNameW(NULL, buffer, MAX_PATH), "GetModuleFileName failed\n");
    separator = wcsrchr(buffer, L'\\');
    if (separator)
        wcscpy(separator + 1, FileName);

    ActCtx.lpSource = buffer;

    return CreateActCtxW(&ActCtx);
}

VOID TestGlobalClasses(VOID)
{
    HMODULE hmod = GetModuleHandle(NULL);
    ATOM a,b,c,d,e;

    a = _GetClassAtom(L"TestClass1", hmod);
    b = _RegisterClass(L"TestClass1", hmod, 0, DefWindowProcW);
    c = _GetClassAtom(L"TestClass1", hmod);
    UnregisterClass(L"TestClass1", hmod);
    d = _GetClassAtom(L"TestClass1", hmod);
    ok( a == 0, "\n");
    ok( b != 0, "\n");
    ok( c != 0, "\n");
    ok( d == 0, "\n");
    ok (b == c, "\n");
   
    a = _GetClassAtom(L"TestClass2", hmod);
    b = _RegisterClass(L"TestClass2", hmod, CS_GLOBALCLASS, DefWindowProcW);
    c = _GetClassAtom(L"TestClass2", hmod);
    UnregisterClass(L"TestClass2", hmod);
    d = _GetClassAtom(L"TestClass2", hmod);
    ok( a == 0, "\n");
    ok( b != 0, "\n");
    ok( c != 0, "\n");
    ok( d == 0, "\n");
    ok (b == c, "\n");

    a = _RegisterClass(L"TestClass3", hmod, 0, DefWindowProcW);
    b = _RegisterClass(L"TestClass3", hmod, 0, DefWindowProcW);
    c = _RegisterClass(L"TestClass3", hmod, CS_GLOBALCLASS, DefWindowProcW);
    UnregisterClass(L"TestClass3", hmod);
    d = _GetClassAtom(L"TestClass3", hmod);
    ok( a != 0, "\n");
    ok( b == 0, "\n");
    ok( c == 0, "\n");
    ok( d == 0, "\n");

    a = _RegisterClass(L"TestClass4", hmod, CS_GLOBALCLASS, DefWindowProcW);
    b = _RegisterClass(L"TestClass4", hmod, 0, DefWindowProcW);
    c = _RegisterClass(L"TestClass4", hmod, 0, DefWindowProcW);
    UnregisterClass(L"TestClass4", hmod);
    d = _GetClassAtom(L"TestClass4", hmod);
    UnregisterClass(L"TestClass4", hmod);
    e = _GetClassAtom(L"TestClass4", hmod);
    ok( a != 0, "\n");
    ok( b != 0, "\n");
    ok( c == 0, "\n");
    ok( d != 0, "\n");
    ok( e == 0, "\n");

    a = _GetClassAtom(L"ComboBox", hmod);
    b = _RegisterClass(L"ComboBox", hmod, 0, DefWindowProcW);
    c = _RegisterClass(L"ComboBox", hmod, CS_GLOBALCLASS, DefWindowProcW);
    UnregisterClass(L"ComboBox", hmod);
    d = _GetClassAtom(L"ComboBox", hmod);
    UnregisterClass(L"TestClass4", hmod);
    e = _GetClassAtom(L"TestClass4", hmod);
    ok( a != 0, "\n");
    ok( b != 0, "\n");
    ok( c == 0, "\n");
    ok( d != 0, "\n");
    ok( e == 0, "\n");
    
    a = _GetClassAtom(L"ScrollBar", hmod);
    UnregisterClass(L"ScrollBar", hmod);
    b = _GetClassAtom(L"ScrollBar", hmod);
    ok( a != 0, "\n");
    ok( b == 0, "\n");
    
    a = _GetClassAtom(L"ListBox", (HMODULE)0xdead);
    UnregisterClass(L"ListBox", (HMODULE)0xdead);
    b = _GetClassAtom(L"ListBox", (HMODULE)0xdead);
    ok( a != 0, "\n");
    ok( b == 0, "\n");
    
    a = _RegisterClass(L"TestClass5", (HMODULE)0xdead, CS_GLOBALCLASS, DefWindowProcW);
    b = _GetClassAtom(L"TestClass5", hmod);
    UnregisterClass(L"TestClass5", hmod);
    c = _GetClassAtom(L"TestClass5", (HMODULE)0xdead);
    d = _GetClassAtom(L"TestClass5", hmod);
    ok( a != 0, "\n");
    ok( b != 0, "\n");    
    ok( c == 0, "\n");
    ok( d == 0, "\n");    
}

VOID TestVersionedClasses(VOID)
{
    HMODULE hmod = GetModuleHandle(NULL);
    HANDLE h1, h2;
    ULONG_PTR cookie1;
    ATOM a,b,c;
    WNDPROC proc1,proc2,proc3, proc4;

    h1 = _CreateActCtxFromFile(L"verclasstest1.manifest");
    h2 = _CreateActCtxFromFile(L"verclasstest2.manifest");
    if (h1 == INVALID_HANDLE_VALUE || h2 == INVALID_HANDLE_VALUE)
    {
        skip("Loading manifests failed. Skipping TestVersionedClasses\n");
        return;
    }
    
    a = _RegisterClass(L"VersionTestClass1", hmod, 0, DefWindowProcA);
    proc1 = _GetWndproc(L"VersionTestClass1", hmod);
    b = _RegisterClass(L"VersionTestClass1", hmod, 0, DefWindowProcW);
    ActivateActCtx(h1, &cookie1);
    proc2 = _GetWndproc(L"VersionTestClass1", hmod);
    c = _RegisterClass(L"VersionTestClass1", hmod, 0, DefWindowProcW);
    proc3 = _GetWndproc(L"VersionTestClass1", hmod);
    DeactivateActCtx(0, cookie1);
    proc4 = _GetWndproc(L"VersionTestClass1", hmod);
    ok( a != 0, "\n");
    ok( b == 0, "\n");
    ok( c != 0, "\n");
    ok( a == c, "\n");
    ok (proc1 == DefWindowProcA, "\n");
    ok (proc2 == NULL, "\n");
    ok (proc3 == DefWindowProcW, "\n");
    ok (proc4 == DefWindowProcA, "\n");
    
    a = _GetClassAtom(L"Button", hmod);
    b = _RegisterClass(L"Button", hmod, CS_GLOBALCLASS, DefWindowProcA);
    proc1 = _GetWndproc(L"Button", (HMODULE)0xdead);
    ActivateActCtx(h2, &cookie1);
    c = _RegisterClass(L"Button", hmod, CS_GLOBALCLASS, DefWindowProcA);
    proc2 = _GetWndproc(L"Button", (HMODULE)0xdead);
    ok( a != 0, "\n");
    ok( b == 0, "\n");
    ok( c != 0, "\n");
    ok( a == c, "\n");
    ok( proc1 != NULL, "\n");
    ok( proc2 != NULL, "\n");
    ok( proc1 != proc2, "\n");
    ok( proc2 == DefWindowProcA, "\n");
}

START_TEST(RegisterClassEx)
{
    TestGlobalClasses();
    TestVersionedClasses();
}
