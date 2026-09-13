/*
 * Unit tests for Active Template Library ActiveX functions
 *
 * Copyright 2010 Andrew Nguyen
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

#define COBJMACROS

#include <wine/test.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <winnls.h>
#include <winerror.h>
#include <winnt.h>
#include <wtypes.h>
#include <olectl.h>
#include <ocidl.h>
#include <exdisp.h>
#include <atlbase.h>

static HRESULT (WINAPI *pAtlAxAttachControl)(IUnknown *, HWND, IUnknown **);

static void init_function_pointers(void)
{
    HMODULE hatl = GetModuleHandleA("atl.dll");

    pAtlAxAttachControl = (void *)GetProcAddress(hatl, "AtlAxAttachControl");
}

static ATOM register_class(void)
{
    WNDCLASSA wndclassA;

    wndclassA.style = 0;
    wndclassA.lpfnWndProc = DefWindowProcA;
    wndclassA.cbClsExtra = 0;
    wndclassA.cbWndExtra = 0;
    wndclassA.hInstance = GetModuleHandleA(NULL);
    wndclassA.hIcon = NULL;
    wndclassA.hCursor = LoadCursorA(NULL, (LPSTR)IDC_ARROW);
    wndclassA.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wndclassA.lpszMenuName = NULL;
    wndclassA.lpszClassName = "WineAtlTestClass";

    return RegisterClassA(&wndclassA);
}

static void test_AtlAxAttachControl(void)
{
    HWND hwnd = CreateWindowA("WineAtlTestClass", "Wine ATL Test Window", 0,
                              CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                              CW_USEDEFAULT, NULL, NULL, NULL, NULL);
    HRESULT hr;
    IUnknown *pObj, *pContainer;

    hr = pAtlAxAttachControl(NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Expected AtlAxAttachControl to return E_INVALIDARG, got 0x%08lx\n", hr);

    pContainer = (IUnknown *)0xdeadbeef;
    hr = pAtlAxAttachControl(NULL, NULL, &pContainer);
    ok(hr == E_INVALIDARG, "Expected AtlAxAttachControl to return E_INVALIDARG, got 0x%08lx\n", hr);
    ok(pContainer == (IUnknown *)0xdeadbeef,
       "Expected the output container pointer to be untouched, got %p\n", pContainer);

    hr = pAtlAxAttachControl(NULL, hwnd, NULL);
    ok(hr == E_INVALIDARG, "Expected AtlAxAttachControl to return E_INVALIDARG, got 0x%08lx\n", hr);

    hr = CoCreateInstance(&CLSID_WebBrowser, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                          &IID_IOleObject, (void **)&pObj);
    ok(hr == S_OK, "Expected CoCreateInstance to return S_OK, got 0x%08lx\n", hr);

    if (FAILED(hr))
    {
        skip("Couldn't obtain a test IOleObject instance\n");
        return;
    }

    hr = pAtlAxAttachControl(pObj, NULL, NULL);
    ok(hr == S_FALSE, "Expected AtlAxAttachControl to return S_FALSE, got 0x%08lx\n", hr);

    pContainer = NULL;
    hr = pAtlAxAttachControl(pObj, NULL, &pContainer);
    ok(hr == S_FALSE, "Expected AtlAxAttachControl to return S_FALSE, got 0x%08lx\n", hr);
    ok(pContainer != NULL, "got %p\n", pContainer);
    IUnknown_Release(pContainer);

    hr = pAtlAxAttachControl(pObj, hwnd, NULL);
    ok(hr == S_OK, "Expected AtlAxAttachControl to return S_OK, got 0x%08lx\n", hr);

    IUnknown_Release(pObj);

    DestroyWindow(hwnd);
}

static void test_ax_win(void)
{
    BOOL ret;
    WNDCLASSEXW wcex;
    static HMODULE hinstance = 0;

    ret = AtlAxWinInit();
    ok(ret, "AtlAxWinInit failed\n");

    hinstance = GetModuleHandleA(NULL);
    memset(&wcex, 0, sizeof(wcex));
    wcex.cbSize = sizeof(wcex);
    ret = GetClassInfoExW(hinstance, L"AtlAxWin", &wcex);
    ok(ret, "AtlAxWin has not registered\n");
    ok(wcex.style == CS_GLOBALCLASS, "wcex.style %08x\n", wcex.style);
}

START_TEST(atl_ax)
{
    init_function_pointers();

    if (!register_class())
        return;

    CoInitialize(NULL);

    if (pAtlAxAttachControl)
        test_AtlAxAttachControl();
    else
        win_skip("AtlAxAttachControl is not available\n");

    test_ax_win();

    CoUninitialize();
}
