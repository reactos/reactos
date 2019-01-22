/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for ShellDimScreen
 * PROGRAMMER:      Mark Jansen
 */

#include <apitest.h>
#include <atlbase.h>
#include <atlcom.h>

#define INITGUID
#include <guiddef.h>
// stolen from com_apitest.h
DEFINE_GUID(CLSID_FadeTask,                0x7EB5FBE4, 0x2100, 0x49E6, 0x85, 0x93, 0x17, 0xE1, 0x30, 0x12, 0x2F, 0x91);

#define INVALID_POINTER ((PVOID)(ULONG_PTR)0xdeadbeefdeadbeefULL)

typedef HRESULT (__stdcall *tShellDimScreen) (IUnknown** Unknown, HWND* hWindow);

tShellDimScreen ShellDimScreen;

static void Test_Dim()
{
    IUnknown* unk = (IUnknown*)INVALID_POINTER;
    HWND wnd = (HWND)INVALID_POINTER;
    ULONG count;

    HRESULT hr = ShellDimScreen(NULL, NULL);
    ok_hex(hr, E_INVALIDARG);

    hr = ShellDimScreen(&unk, &wnd);
    ok_hex(hr, S_OK);
    ok(unk != INVALID_POINTER, "Expected a valid object\n");
    ok(wnd != INVALID_POINTER, "Expected a valid window ptr\n");
    ok(IsWindow(wnd), "Expected a valid window\n");
    ok(IsWindowVisible(wnd), "Expected the window to be visible\n");

    if (unk != ((IUnknown*)INVALID_POINTER) && unk)
    {
        count = unk->Release();
        ok(count == 0, "Expected count to be 0, was: %lu\n", count);
        ok(!IsWindow(wnd), "Expected the window to be destroyed\n");
    }

    unk = (IUnknown*)INVALID_POINTER;
    wnd = (HWND)INVALID_POINTER;
    hr = ShellDimScreen(&unk, &wnd);
    ok_hex(hr, S_OK);
    ok(unk != ((IUnknown*)INVALID_POINTER), "Expected a valid object\n");
    ok(wnd != ((HWND)INVALID_POINTER), "Expected a valid window ptr\n");
    ok(IsWindow(wnd), "Expected a valid window\n");
    ok(IsWindowVisible(wnd), "Expected the window to be visible\n");
    char classname[100] = {0};
    int nRet = GetClassNameA(wnd, classname, 100);
    ok(nRet == 17, "Expected GetClassName to return 3 was %i\n", nRet);
    ok(!strcmp(classname, "DimmedWindowClass"), "Expected classname to be DimmedWindowClass, was %s\n", classname);
    LONG style = GetWindowLong(wnd, GWL_STYLE);
    LONG expectedstyle = WS_POPUP | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS;
    ok(style == expectedstyle, "Expected style to be %lx, was %lx\n", expectedstyle, style);
    style = GetWindowLong(wnd, GWL_EXSTYLE);
    ok(style == WS_EX_TOPMOST, "Expected exstyle to be %x, was %lx\n", WS_EX_TOPMOST, style);

    if (unk != ((IUnknown*)INVALID_POINTER) && unk)
    {
        count = unk->AddRef();
        ok(count == 2, "Expected count to be 2, was: %lu\n", count);
        count = unk->Release();
        ok(count == 1, "Expected count to be 1, was: %lu\n", count);

        IUnknown* unk2;
        hr = unk->QueryInterface(IID_IUnknown, (void**)&unk2);
        ok_hex(hr, S_OK);
        if (SUCCEEDED(hr))
        {
            ok(unk2 == unk, "Expected the object to be the same, was: %p, %p\n", unk, unk2);
            unk2->Release();
        }
        hr = unk->QueryInterface(CLSID_FadeTask, (void**)&unk2);
        ok_hex(hr, E_NOINTERFACE);
        if (SUCCEEDED(hr))
        {
            ok(unk2 == unk, "Expected the object to be the same, was: %p, %p\n", unk, unk2);
            unk2->Release();
        }
    }

    RECT rc;
    GetWindowRect(wnd, &rc);

    ok(rc.left == GetSystemMetrics(SM_XVIRTUALSCREEN), "Expected rc.left to be %u, was %lu\n", GetSystemMetrics(SM_XVIRTUALSCREEN), rc.left);
    ok(rc.top == GetSystemMetrics(SM_YVIRTUALSCREEN), "Expected rc.top to be %u, was %lu\n", GetSystemMetrics(SM_YVIRTUALSCREEN), rc.top);
    ok((rc.right - rc.left) == GetSystemMetrics(SM_CXVIRTUALSCREEN), "Expected rc.left to be %u, was %lu\n", GetSystemMetrics(SM_CXVIRTUALSCREEN), (rc.right - rc.left));
    ok((rc.bottom - rc.top) == GetSystemMetrics(SM_CYVIRTUALSCREEN), "Expected rc.top to be %u, was %lu\n", GetSystemMetrics(SM_CYVIRTUALSCREEN), (rc.bottom - rc.top));

    if (unk != ((IUnknown*)INVALID_POINTER) && unk)
    {
        count = unk->Release();
        ok(count == 0, "Expected count to be 0, was: %lu\n", count);
        ok(!IsWindow(wnd), "Expected the window to be destroyed\n");
    }
}


START_TEST(ShellDimScreen)
{
    HMODULE dll = LoadLibraryA("msgina.dll");
    ShellDimScreen = (tShellDimScreen)GetProcAddress(dll, MAKEINTRESOURCEA(16));
    if (!dll || !ShellDimScreen)
    {
        skip("msgina!#16 not found, skipping tests\n");
        return;
    }
    Test_Dim();
}
