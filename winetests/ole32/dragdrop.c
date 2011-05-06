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

static int droptarget_refs;

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
    droptarget_refs++;
    return droptarget_refs;
}

static ULONG WINAPI DropTarget_Release(IDropTarget* iface)
{
    droptarget_refs--;
    return droptarget_refs;
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

/** stub IDropSource **/
static HRESULT WINAPI DropSource_QueryInterface(IDropSource *iface, REFIID riid, void **ppObj)
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDropSource))
    {
        *ppObj = iface;
        IDropSource_AddRef(iface);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI DropSource_AddRef(IDropSource *iface)
{
    return 2;
}

static ULONG WINAPI DropSource_Release(IDropSource *iface)
{
    return 1;
}

static HRESULT WINAPI DropSource_QueryContinueDrag(
    IDropSource *iface,
    BOOL fEscapePressed,
    DWORD grfKeyState)
{
    /* always drop */
    return DRAGDROP_S_DROP;
}

static HRESULT WINAPI DropSource_GiveFeedback(
    IDropSource *iface,
    DWORD dwEffect)
{
    return DRAGDROP_S_USEDEFAULTCURSORS;
}

static const IDropSourceVtbl dropsource_vtbl = {
    DropSource_QueryInterface,
    DropSource_AddRef,
    DropSource_Release,
    DropSource_QueryContinueDrag,
    DropSource_GiveFeedback
};

static IDropSource DropSource = { &dropsource_vtbl };

/** IDataObject stub **/
static HRESULT WINAPI DataObject_QueryInterface(
    IDataObject *iface,
    REFIID riid,
    void **pObj)
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDataObject))
    {
        *pObj = iface;
        IDataObject_AddRef(iface);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI DataObject_AddRef(IDataObject *iface)
{
    return 2;
}

static ULONG WINAPI DataObject_Release(IDataObject *iface)
{
    return 1;
}

static HRESULT WINAPI DataObject_GetData(
    IDataObject *iface,
    FORMATETC *pformatetcIn,
    STGMEDIUM *pmedium)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_GetDataHere(
    IDataObject *iface,
    FORMATETC *pformatetc,
    STGMEDIUM *pmedium)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_QueryGetData(
    IDataObject *iface,
    FORMATETC *pformatetc)
{
    return S_OK;
}

static HRESULT WINAPI DataObject_GetCanonicalFormatEtc(
    IDataObject *iface,
    FORMATETC *pformatectIn,
    FORMATETC *pformatetcOut)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_SetData(
    IDataObject *iface,
    FORMATETC *pformatetc,
    STGMEDIUM *pmedium,
    BOOL fRelease)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_EnumFormatEtc(
    IDataObject *iface,
    DWORD dwDirection,
    IEnumFORMATETC **ppenumFormatEtc)
{
    return S_OK;
}

static HRESULT WINAPI DataObject_DAdvise(
    IDataObject *iface,
    FORMATETC *pformatetc,
    DWORD advf,
    IAdviseSink *pAdvSink,
    DWORD *pdwConnection)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_DUnadvise(
    IDataObject *iface,
    DWORD dwConnection)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_EnumDAdvise(
    IDataObject *iface,
    IEnumSTATDATA **ppenumAdvise)
{
    return E_NOTIMPL;
}

static const IDataObjectVtbl dataobject_vtbl = {
    DataObject_QueryInterface,
    DataObject_AddRef,
    DataObject_Release,
    DataObject_GetData,
    DataObject_GetDataHere,
    DataObject_QueryGetData,
    DataObject_GetCanonicalFormatEtc,
    DataObject_SetData,
    DataObject_EnumFormatEtc,
    DataObject_DAdvise,
    DataObject_DUnadvise,
    DataObject_EnumDAdvise
};

static IDataObject DataObject = { &dataobject_vtbl };

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

static void test_Register_Revoke(void)
{
    HANDLE prop;
    HRESULT hr;
    HWND hwnd;

    hwnd = CreateWindowA("WineOleTestClass", "Test", 0,
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

    ok(droptarget_refs == 0, "DropTarget refs should be zero not %d\n", droptarget_refs);
    hr = RegisterDragDrop(hwnd, &DropTarget);
    ok_ole_success(hr, "RegisterDragDrop");
    ok(droptarget_refs >= 1, "DropTarget refs should be at least one\n");

    prop = GetPropA(hwnd, "OleDropTargetInterface");
    ok(prop == &DropTarget, "expected IDropTarget pointer %p, got %p\n", &DropTarget, prop);

    hr = RegisterDragDrop(hwnd, &DropTarget);
    ok(hr == DRAGDROP_E_ALREADYREGISTERED, "RegisterDragDrop with already registered hwnd should return DRAGDROP_E_ALREADYREGISTERED instead of 0x%08x\n", hr);

    ok(droptarget_refs >= 1, "DropTarget refs should be at least one\n");
    OleUninitialize();
    ok(droptarget_refs >= 1, "DropTarget refs should be at least one\n");

    hr = RevokeDragDrop(hwnd);
    ok_ole_success(hr, "RevokeDragDrop");
    ok(droptarget_refs == 0 ||
       broken(droptarget_refs == 1), /* NT4 */
       "DropTarget refs should be zero not %d\n", droptarget_refs);

    hr = RevokeDragDrop(NULL);
    ok(hr == DRAGDROP_E_INVALIDHWND, "RevokeDragDrop with NULL hwnd should return DRAGDROP_E_INVALIDHWND instead of 0x%08x\n", hr);

    DestroyWindow(hwnd);

    /* try to revoke with already destroyed window */
    OleInitialize(NULL);

    hwnd = CreateWindowA("WineOleTestClass", "Test", 0,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL,
        NULL, NULL, NULL);

    hr = RegisterDragDrop(hwnd, &DropTarget);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    DestroyWindow(hwnd);

    hr = RevokeDragDrop(hwnd);
    ok(hr == DRAGDROP_E_INVALIDHWND, "got 0x%08x\n", hr);

    OleUninitialize();
}

static void test_DoDragDrop(void)
{
    DWORD effect;
    HRESULT hr;
    HWND hwnd;

    hwnd = CreateWindowA("WineOleTestClass", "Test", 0,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL,
        NULL, NULL, NULL);
    ok(IsWindow(hwnd), "failed to create window\n");

    hr = OleInitialize(NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = RegisterDragDrop(hwnd, &DropTarget);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* incomplete arguments set */
    hr = DoDragDrop(NULL, NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = DoDragDrop(NULL, &DropSource, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = DoDragDrop(&DataObject, NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = DoDragDrop(NULL, NULL, 0, &effect);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = DoDragDrop(&DataObject, &DropSource, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = DoDragDrop(NULL, &DropSource, 0, &effect);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = DoDragDrop(&DataObject, NULL, 0, &effect);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    OleUninitialize();

    DestroyWindow(hwnd);
}

START_TEST(dragdrop)
{
    register_dummy_class();

    test_Register_Revoke();
    test_DoDragDrop();
}
