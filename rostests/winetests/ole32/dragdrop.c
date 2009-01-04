/*
 * Drag and Drop Tests
 *
 * Copyright 2007 Robert Shearman
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

#define _WIN32_DCOM
#define COBJMACROS
#define CONST_VTABLE

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wine/test.h"

/* functions that are not present on all versions of Windows */
HRESULT (WINAPI * pCoInitializeEx)(LPVOID lpReserved, DWORD dwCoInit);

static int droptarget_addref_called;
static int droptarget_release_called;

/* helper macros to make tests a bit leaner */
#define ok_ole_success(hr, func) ok(hr == S_OK, func " failed with error 0x%08x\n", hr)

static HRESULT WINAPI DropTarget_QueryInterface(IDropTarget* iface, REFIID riid,
                                                void** ppvObject)
{
    trace("DropTarget_QueryInterface\n");
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDropTarget))
    {
        IUnknown_AddRef(iface);
        *ppvObject = iface;
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI DropTarget_AddRef(IDropTarget* iface)
{
    droptarget_addref_called++;
    return 2;
}

static ULONG WINAPI DropTarget_Release(IDropTarget* iface)
{
    droptarget_release_called++;
    return 1;
}

static HRESULT WINAPI DropTarget_DragEnter(IDropTarget* iface,
                                           IDataObject* pDataObj,
                                           DWORD grfKeyState, POINTL pt,
                                           DWORD* pdwEffect)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DropTarget_DragOver(IDropTarget* iface,
                                          DWORD grfKeyState,
                                          POINTL pt,
                                          DWORD* pdwEffect)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DropTarget_DragLeave(IDropTarget* iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DropTarget_Drop(IDropTarget* iface,
                                      IDataObject* pDataObj, DWORD grfKeyState,
                                      POINTL pt, DWORD* pdwEffect)
{
    return E_NOTIMPL;
}

static const IDropTargetVtbl DropTarget_VTbl =
{
    DropTarget_QueryInterface,
    DropTarget_AddRef,
    DropTarget_Release,
    DropTarget_DragEnter,
    DropTarget_DragOver,
    DropTarget_DragLeave,
    DropTarget_Drop
};

static IDropTarget DropTarget = { &DropTarget_VTbl };

static ATOM register_dummy_class(void)
{
    WNDCLASS wc =
    {
        0,
        DefWindowProc,
        0,
        0,
        GetModuleHandle(NULL),
        NULL,
        LoadCursor(NULL, IDC_ARROW),
        (HBRUSH)(COLOR_BTNFACE+1),
        NULL,
        TEXT("WineOleTestClass"),
    };

    return RegisterClass(&wc);
}

START_TEST(dragdrop)
{
    HRESULT hr;
    HWND hwnd;

    hwnd = CreateWindow(MAKEINTATOM(register_dummy_class()), "Test", 0,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL,
        NULL, NULL, NULL);

    hr = RegisterDragDrop(hwnd, &DropTarget);
    ok(hr == E_OUTOFMEMORY ||
        broken(hr == CO_E_NOTINITIALIZED), /* NT4 */
        "RegisterDragDrop without OLE initialized should have returned E_OUTOFMEMORY instead of 0x%08x\n", hr);

    OleInitialize(NULL);

    hr = RegisterDragDrop(hwnd, NULL);
    ok(hr == E_INVALIDARG, "RegisterDragDrop with NULL IDropTarget * should return E_INVALIDARG instead of 0x%08x\n", hr);

    hr = RegisterDragDrop(NULL, &DropTarget);
    ok(hr == DRAGDROP_E_INVALIDHWND, "RegisterDragDrop with NULL hwnd should return DRAGDROP_E_INVALIDHWND instead of 0x%08x\n", hr);

    hr = RegisterDragDrop((HWND)0xdeadbeef, &DropTarget);
    ok(hr == DRAGDROP_E_INVALIDHWND, "RegisterDragDrop with garbage hwnd should return DRAGDROP_E_INVALIDHWND instead of 0x%08x\n", hr);

    ok(droptarget_addref_called == 0, "DropTarget_AddRef shouldn't have been called\n");
    hr = RegisterDragDrop(hwnd, &DropTarget);
    ok_ole_success(hr, "RegisterDragDrop");
    ok(droptarget_addref_called == 1, "DropTarget_AddRef should have been called once, not %d times\n", droptarget_addref_called);

    hr = RegisterDragDrop(hwnd, &DropTarget);
    ok(hr == DRAGDROP_E_ALREADYREGISTERED, "RegisterDragDrop with already registered hwnd should return DRAGDROP_E_ALREADYREGISTERED instead of 0x%08x\n", hr);

    ok(droptarget_release_called == 0, "DropTarget_Release shouldn't have been called\n");
    OleUninitialize();
    ok(droptarget_release_called == 0, "DropTarget_Release shouldn't have been called\n");

    hr = RevokeDragDrop(hwnd);
    ok_ole_success(hr, "RevokeDragDrop");
    ok(droptarget_release_called == 1 ||
        broken(droptarget_release_called == 0), /* NT4 */
        "DropTarget_Release should have been called once, not %d times\n", droptarget_release_called);

    hr = RevokeDragDrop(NULL);
    ok(hr == DRAGDROP_E_INVALIDHWND, "RevokeDragDrop with NULL hwnd should return DRAGDROP_E_INVALIDHWND instead of 0x%08x\n", hr);

    DestroyWindow(hwnd);
}
