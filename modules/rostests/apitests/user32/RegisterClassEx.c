/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for RegisterClassEx
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include "precomp.h"

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
    return (ATOM)GetClassInfoExW(hInstance, lpwszClassName, &wcex);
}

static WNDPROC _GetWndproc(LPCWSTR lpwszClassName, HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = {sizeof(WNDCLASSEXW)};
    BOOL ret = GetClassInfoExW(hInstance, lpwszClassName, &wcex);
    return ret ? wcex.lpfnWndProc : NULL;
}

static ATOM _RegisterClassA(LPCSTR lpzClassName, HINSTANCE hInstance, UINT style, WNDPROC lpfnWndProc)
{
    WNDCLASSEXA wcex = {sizeof(WNDCLASSEX), style, lpfnWndProc};
    wcex.lpszClassName  = lpzClassName;
    wcex.hInstance      = hInstance;
    return RegisterClassExA(&wcex);
}

static ATOM _GetClassAtomA(LPCSTR lpszClassName, HINSTANCE hInstance)
{
    WNDCLASSEXA wcex = {sizeof(WNDCLASSEX)};
    return (ATOM)GetClassInfoExA(hInstance, lpszClassName, &wcex);
}

static WNDPROC _GetWndprocA(LPCSTR lpszClassName, HINSTANCE hInstance)
{
    WNDCLASSEXA wcex = {sizeof(WNDCLASSEX)};
    BOOL ret = GetClassInfoExA(hInstance, lpszClassName, &wcex);
    return ret ? wcex.lpfnWndProc : NULL;
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
    UnregisterClassW(L"TestClass1", hmod);
    d = _GetClassAtom(L"TestClass1", hmod);
    ok( a == 0, "\n");
    ok( b != 0, "\n");
    ok( c != 0, "\n");
    ok( d == 0, "\n");
    ok (b == c, "\n");

    a = _GetClassAtom(L"TestClass2", hmod);
    b = _RegisterClass(L"TestClass2", hmod, CS_GLOBALCLASS, DefWindowProcW);
    c = _GetClassAtom(L"TestClass2", hmod);
    UnregisterClassW(L"TestClass2", hmod);
    d = _GetClassAtom(L"TestClass2", hmod);
    ok( a == 0, "\n");
    ok( b != 0, "\n");
    ok( c != 0, "\n");
    ok( d == 0, "\n");
    ok (b == c, "\n");

    a = _RegisterClass(L"TestClass3", hmod, 0, DefWindowProcW);
    b = _RegisterClass(L"TestClass3", hmod, 0, DefWindowProcW);
    c = _RegisterClass(L"TestClass3", hmod, CS_GLOBALCLASS, DefWindowProcW);
    UnregisterClassW(L"TestClass3", hmod);
    d = _GetClassAtom(L"TestClass3", hmod);
    ok( a != 0, "\n");
    ok( b == 0, "\n");
    ok( c == 0, "\n");
    ok( d == 0, "\n");

    a = _RegisterClass(L"TestClass4", hmod, CS_GLOBALCLASS, DefWindowProcW);
    b = _RegisterClass(L"TestClass4", hmod, 0, DefWindowProcW);
    c = _RegisterClass(L"TestClass4", hmod, 0, DefWindowProcW);
    UnregisterClassW(L"TestClass4", hmod);
    d = _GetClassAtom(L"TestClass4", hmod);
    UnregisterClassW(L"TestClass4", hmod);
    e = _GetClassAtom(L"TestClass4", hmod);
    ok( a != 0, "\n");
    ok( b != 0, "\n");
    ok( c == 0, "\n");
    ok( d != 0, "\n");
    ok( e == 0, "\n");

    a = _GetClassAtom(L"ComboBox", hmod);
    b = _RegisterClass(L"ComboBox", hmod, 0, DefWindowProcW);
    c = _RegisterClass(L"ComboBox", hmod, CS_GLOBALCLASS, DefWindowProcW);
    UnregisterClassW(L"ComboBox", hmod);
    d = _GetClassAtom(L"ComboBox", hmod);
    UnregisterClassW(L"TestClass4", hmod);
    e = _GetClassAtom(L"TestClass4", hmod);
    ok( a != 0, "\n");
    ok( b != 0, "\n");
    ok( c == 0, "\n");
    ok( d != 0, "\n");
    ok( e == 0, "\n");

    a = _GetClassAtom(L"ScrollBar", hmod);
    UnregisterClassW(L"ScrollBar", hmod);
    b = _GetClassAtom(L"ScrollBar", hmod);
    c = _RegisterClass(L"ScrollBar", hmod, CS_GLOBALCLASS, DefWindowProcW);
    d = _GetClassAtom(L"ScrollBar", hmod);
    ok( a != 0, "Wrong value for a. Expected != 0, got 0\n");
    ok( b == 0, "Wrong value for b. Expected == 0, got %d\n", b);
    //ok( c != 0, "Wrong value for c. Expected != 0, got 0\n");
    //ok( d != 0, "Wrong value for d. Expected != 0, got 0\n");
    //ok_int(a, c);
    //ok_int(a, d); /* In Windows 10 and WHS testbot the last 4 tests fail */

    a = _GetClassAtom(L"ListBox", (HMODULE)0xdead);
    UnregisterClassW(L"ListBox", (HMODULE)0xdead);
    b = _GetClassAtom(L"ListBox", (HMODULE)0xdead);
    ok( a != 0, "\n");
    ok( b == 0, "\n");

    a = _RegisterClass(L"TestClass5", (HMODULE)0xdead, CS_GLOBALCLASS, DefWindowProcW);
    b = _GetClassAtom(L"TestClass5", hmod);
    UnregisterClassW(L"TestClass5", hmod);
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
    ULONG_PTR cookie1, cookie2;
    ATOM a,b,c,d;
    WNDPROC proc1,proc2,proc3, proc4, proc5;
    WCHAR buffer[50];


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
    d = _GetClassAtom(L"VersionTestClass1", hmod);
    proc3 = _GetWndproc(L"VersionTestClass1", hmod);
    proc4 = _GetWndproc((LPCWSTR)(DWORD_PTR)a, hmod);
    DeactivateActCtx(0, cookie1);
    proc5 = _GetWndproc(L"VersionTestClass1", hmod);
    ok( a != 0, "\n");
    ok( b == 0, "\n");
    ok( c != 0, "\n");
    ok( d != 0, "\n");
    ok( a == c, "\n");
    ok( a == d, "\n");
    ok (proc1 == DefWindowProcA, "\n");
    ok (proc2 == NULL, "Got 0x%p, expected NULL\n", proc2);
    ok (proc3 == DefWindowProcW, "Got 0x%p, expected 0x%p\n", proc3, DefWindowProcW);
    ok (proc4 == DefWindowProcW, "Got 0x%p, expected 0x%p\n", proc4, DefWindowProcW);
    ok (proc5 == DefWindowProcA, "\n");

    a = _GetClassAtom(L"Button", hmod);
    b = _RegisterClass(L"Button", hmod, CS_GLOBALCLASS, DefWindowProcA);
    proc1 = _GetWndproc(L"Button", (HMODULE)0xdead);
    ActivateActCtx(h2, &cookie1);
    c = _RegisterClass(L"Button", hmod, CS_GLOBALCLASS, DefWindowProcA);
    proc2 = _GetWndproc(L"Button", (HMODULE)0xdead);
    d = _GetClassAtom(L"3.3.3.3!Button", (HMODULE)0xdead);
    proc3 = _GetWndproc(L"3.3.3.3!Button", (HMODULE)0xdead);
    ok( a != 0, "\n");
    ok( b == 0, "\n");
    ok( c != 0, "\n");
    ok( d != 0, "\n");
    ok( a == c, "\n");
    ok( d == a, "\n");
    ok( proc1 != NULL, "\n");
    ok( proc1 != DefWindowProcA, "Got 0x%p, expected not 0x%p\n", proc1, DefWindowProcA);
    ok( proc2 == DefWindowProcA, "Got 0x%p, expected 0x%p\n", proc2, DefWindowProcA);
    ok( proc3 == DefWindowProcA, "Got 0x%p, expected 0x%p\n", proc3, DefWindowProcA);

    a = _RegisterClass(L"VersionTestClass2", hmod, CS_GLOBALCLASS, DefWindowProcW);
    proc1 = _GetWndproc(L"VersionTestClass2", (HMODULE)0xdead);
    b = _RegisterClass(L"VersionTestClass2", hmod, 0, DefWindowProcA);
    proc2 = _GetWndproc(L"VersionTestClass2", hmod);
    proc3 = _GetWndproc(L"VersionTestClass2", (HMODULE)0xdead);
    ok (a != 0, "\n");
    ok (b != 0, "\n");
    ok (a == b, "\n");
    ok (proc1 == DefWindowProcW, "Got 0x%p, expected 0x%p\n", proc1, DefWindowProcW);
    ok (proc2 == DefWindowProcA, "Got 0x%p, expected 0x%p\n", proc2, DefWindowProcA);
    ok (proc3 == DefWindowProcW, "Got 0x%p, expected 0x%p\n", proc2, DefWindowProcA);

    a = _RegisterClass(L"VersionTestClass3", hmod, 0, DefWindowProcW);
    swprintf(buffer, L"#%d", a);
    proc1 = _GetWndproc((LPCWSTR)(DWORD_PTR)a, hmod);
    proc2 = _GetWndproc(buffer, hmod);
    ok (a != 0, "\n");
    ok (proc1 == DefWindowProcW, "\n");
    ok (proc2 == 0, "Got 0x%p for %S, expected 0\n", proc2, buffer);
    DeactivateActCtx(0, cookie1);

    a = _RegisterClass(L"VersionTestClass3", hmod, 0, DefWindowProcW);
    swprintf(buffer, L"#%d", a);
    proc1 = _GetWndproc((LPCWSTR)(DWORD_PTR)a, hmod);
    proc2 = _GetWndproc(buffer, hmod);
    ok (a != 0, "\n");
    ok (proc1 == DefWindowProcW, "\n");
    ok (proc2 == 0, "Got 0x%p for %S, expected 0\n", proc2, buffer);

    ActivateActCtx(h2, &cookie1);
    a = _RegisterClassA("VersionTestClass7", hmod, 0, DefWindowProcW);
    b = _GetClassAtomA("VersionTestClass7", hmod);
    proc1 = _GetWndprocA("VersionTestClass7", hmod);
    proc2 = _GetWndprocA((LPCSTR)(DWORD_PTR)a, hmod);
    ok(a != 0, "\n");
    ok(b != 0, "\n");
    ok(a == b, "\n");
    ok (proc1 == DefWindowProcW, "\n");
    ok (proc2 == DefWindowProcW, "\n");

    DeactivateActCtx(0, cookie1);

    proc1 = _GetWndproc(L"Button", 0);
    ActivateActCtx(h2, &cookie1);
    ActivateActCtx(h1, &cookie2);
    proc2 = _GetWndproc(L"Button", 0);
    DeactivateActCtx(0, cookie2);
    ActivateActCtx(0, &cookie2);
    proc3 = _GetWndproc(L"Button", 0);
    DeactivateActCtx(0, cookie2);
    DeactivateActCtx(0, cookie1);
    ok (proc1 != 0, "\n");
    ok (proc2 != 0, "\n");
    ok (proc4 != 0, "\n");
    ok (proc1 == proc2, "\n");
    ok (proc1 == proc3, "\n");

}

START_TEST(RegisterClassEx)
{
    TestGlobalClasses();
    TestVersionedClasses();
}
