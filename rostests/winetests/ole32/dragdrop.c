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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define _WIN32_DCOM
#define COBJMACROS
#define CONST_VTABLE

#include <stdarg.h>
//#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <ole2.h>
//#include "objbase.h"

#include <wine/test.h>

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(DataObject_EnumFormatEtc);
DEFINE_EXPECT(EnumFORMATETC_Next);
DEFINE_EXPECT(EnumFORMATETC_Reset);
DEFINE_EXPECT(DataObject_QueryGetData);
DEFINE_EXPECT(DropSource_QueryContinueDrag);
DEFINE_EXPECT(DropTarget_DragEnter);
DEFINE_EXPECT(DropSource_GiveFeedback);
DEFINE_EXPECT(DropTarget_Drop);
DEFINE_EXPECT(DropTarget_DragLeave);

static int droptarget_refs;

/* helper macros to make tests a bit leaner */
#define ok_ole_success(hr, func) ok(hr == S_OK, func " failed with error 0x%08x\n", hr)

static HRESULT WINAPI DropTarget_QueryInterface(IDropTarget* iface, REFIID riid,
                                                void** ppvObject)
{
    ok(0, "DropTarget_QueryInterface() shouldn't be called\n");
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDropTarget))
    {
        IDropTarget_AddRef(iface);
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
    CHECK_EXPECT(DropTarget_DragEnter);
    *pdwEffect = DROPEFFECT_COPY;
    return S_OK;
}

static HRESULT WINAPI DropTarget_DragOver(IDropTarget* iface,
                                          DWORD grfKeyState,
                                          POINTL pt,
                                          DWORD* pdwEffect)
{
    ok(0, "unexpected call\n");
    *pdwEffect = DROPEFFECT_COPY;
    return S_OK;
}

static HRESULT WINAPI DropTarget_DragLeave(IDropTarget* iface)
{
    CHECK_EXPECT(DropTarget_DragLeave);
    return E_NOTIMPL;
}

static HRESULT WINAPI DropTarget_Drop(IDropTarget* iface,
                                      IDataObject* pDataObj, DWORD grfKeyState,
                                      POINTL pt, DWORD* pdwEffect)
{
    CHECK_EXPECT(DropTarget_Drop);
    return 0xbeefbeef;
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
    CHECK_EXPECT(DropSource_QueryContinueDrag);
    return DRAGDROP_S_DROP;
}

static HRESULT WINAPI DropSource_GiveFeedback(
    IDropSource *iface,
    DWORD dwEffect)
{
    CHECK_EXPECT(DropSource_GiveFeedback);
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

static HRESULT WINAPI EnumFORMATETC_QueryInterface(IEnumFORMATETC *iface,
        REFIID riid, void **ppvObj)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI EnumFORMATETC_AddRef(IEnumFORMATETC *iface)
{
    return 2;
}

static ULONG WINAPI EnumFORMATETC_Release(IEnumFORMATETC *iface)
{
    return 1;
}

static BOOL formats_enumerated;
static HRESULT WINAPI EnumFORMATETC_Next(IEnumFORMATETC *iface,
        ULONG celt, FORMATETC *rgelt, ULONG *pceltFetched)
{
    static FORMATETC format = { CF_TEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

    CHECK_EXPECT2(EnumFORMATETC_Next);

    ok(celt == 1, "celt = %d\n", celt);
    ok(rgelt != NULL, "rgelt == NULL\n");
    ok(pceltFetched == NULL, "pceltFetched != NULL\n");

    if(formats_enumerated)
        return S_FALSE;

    *rgelt = format;
    formats_enumerated = TRUE;
    return S_OK;
}

static HRESULT WINAPI EnumFORMATETC_Skip(IEnumFORMATETC *iface, ULONG celt)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumFORMATETC_Reset(IEnumFORMATETC *iface)
{
    CHECK_EXPECT(EnumFORMATETC_Reset);
    formats_enumerated = FALSE;
    return S_OK;
}

static HRESULT WINAPI EnumFORMATETC_Clone(IEnumFORMATETC *iface,
        IEnumFORMATETC **ppenum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IEnumFORMATETCVtbl enumformatetc_vtbl = {
    EnumFORMATETC_QueryInterface,
    EnumFORMATETC_AddRef,
    EnumFORMATETC_Release,
    EnumFORMATETC_Next,
    EnumFORMATETC_Skip,
    EnumFORMATETC_Reset,
    EnumFORMATETC_Clone
};

static IEnumFORMATETC EnumFORMATETC = { &enumformatetc_vtbl };

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

    trace("DataObject_QueryInterface: %s\n", wine_dbgstr_guid(riid));
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
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_GetDataHere(
    IDataObject *iface,
    FORMATETC *pformatetc,
    STGMEDIUM *pmedium)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_QueryGetData(
    IDataObject *iface,
    FORMATETC *pformatetc)
{
    CHECK_EXPECT(DataObject_QueryGetData);
    return S_OK;
}

static HRESULT WINAPI DataObject_GetCanonicalFormatEtc(
    IDataObject *iface,
    FORMATETC *pformatectIn,
    FORMATETC *pformatetcOut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_SetData(
    IDataObject *iface,
    FORMATETC *pformatetc,
    STGMEDIUM *pmedium,
    BOOL fRelease)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_EnumFormatEtc(
    IDataObject *iface,
    DWORD dwDirection,
    IEnumFORMATETC **ppenumFormatEtc)
{
    CHECK_EXPECT(DataObject_EnumFormatEtc);
    *ppenumFormatEtc = &EnumFORMATETC;
    formats_enumerated = FALSE;
    return S_OK;
}

static HRESULT WINAPI DataObject_DAdvise(
    IDataObject *iface,
    FORMATETC *pformatetc,
    DWORD advf,
    IAdviseSink *pAdvSink,
    DWORD *pdwConnection)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_DUnadvise(
    IDataObject *iface,
    DWORD dwConnection)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_EnumDAdvise(
    IDataObject *iface,
    IEnumSTATDATA **ppenumAdvise)
{
    ok(0, "unexpected call\n");
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
    WNDCLASSA wc =
    {
        0,
        DefWindowProcA,
        0,
        0,
        GetModuleHandleA(NULL),
        NULL,
        LoadCursorA(NULL, (LPSTR)IDC_ARROW),
        (HBRUSH)(COLOR_BTNFACE+1),
        NULL,
        "WineOleTestClass",
    };

    return RegisterClassA(&wc);
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

    /* Win 8 releases the ref in OleUninitialize() */
    if (droptarget_refs >= 1)
    {
        hr = RevokeDragDrop(hwnd);
        ok_ole_success(hr, "RevokeDragDrop");
        ok(droptarget_refs == 0 ||
           broken(droptarget_refs == 1), /* NT4 */
           "DropTarget refs should be zero not %d\n", droptarget_refs);
    }

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
    RECT rect;

    hwnd = CreateWindowExA(WS_EX_TOPMOST, "WineOleTestClass", "Test", 0,
        CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, NULL,
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

    ShowWindow(hwnd, SW_SHOW);
    GetWindowRect(hwnd, &rect);
    ok(SetCursorPos(rect.left+50, rect.top+50), "SetCursorPos failed\n");
    SET_EXPECT(DataObject_EnumFormatEtc);
    SET_EXPECT(EnumFORMATETC_Next);
    SET_EXPECT(EnumFORMATETC_Reset);
    SET_EXPECT(DataObject_QueryGetData);
    SET_EXPECT(DropSource_QueryContinueDrag);
    SET_EXPECT(DropTarget_DragEnter);
    SET_EXPECT(DropSource_GiveFeedback);
    SET_EXPECT(DropTarget_Drop);
    hr = DoDragDrop(&DataObject, &DropSource, DROPEFFECT_COPY, &effect);
    ok(hr == 0xbeefbeef, "got 0x%08x\n", hr);
    todo_wine CHECK_CALLED(DataObject_EnumFormatEtc);
    todo_wine CHECK_CALLED(EnumFORMATETC_Next);
    todo_wine CHECK_CALLED(EnumFORMATETC_Reset);
    todo_wine CHECK_CALLED(DataObject_QueryGetData);
    CHECK_CALLED(DropSource_QueryContinueDrag);
    CHECK_CALLED(DropTarget_DragEnter);
    CHECK_CALLED(DropSource_GiveFeedback);
    CHECK_CALLED(DropTarget_Drop);

    SET_EXPECT(DataObject_EnumFormatEtc);
    SET_EXPECT(EnumFORMATETC_Next);
    SET_EXPECT(EnumFORMATETC_Reset);
    SET_EXPECT(DataObject_QueryGetData);
    SET_EXPECT(DropSource_QueryContinueDrag);
    SET_EXPECT(DropTarget_DragEnter);
    SET_EXPECT(DropSource_GiveFeedback);
    SET_EXPECT(DropTarget_DragLeave);
    hr = DoDragDrop(&DataObject, &DropSource, 0, &effect);
    ok(hr == DRAGDROP_S_DROP, "got 0x%08x\n", hr);
    todo_wine CHECK_CALLED(DataObject_EnumFormatEtc);
    todo_wine CHECK_CALLED(EnumFORMATETC_Next);
    todo_wine CHECK_CALLED(EnumFORMATETC_Reset);
    todo_wine CHECK_CALLED(DataObject_QueryGetData);
    CHECK_CALLED(DropSource_QueryContinueDrag);
    CHECK_CALLED(DropTarget_DragEnter);
    CHECK_CALLED(DropSource_GiveFeedback);
    CHECK_CALLED(DropTarget_DragLeave);

    OleUninitialize();

    DestroyWindow(hwnd);
}

START_TEST(dragdrop)
{
    register_dummy_class();

    test_Register_Revoke();
#ifdef __REACTOS__
    if (!winetest_interactive &&
        !strcmp(winetest_platform, "windows"))
    {
        skip("ROSTESTS-182: Skipping ole32_winetest:dragdrop test_DoDragDrop because it hangs on WHS-Testbot. Set winetest_interactive to run it anyway.\n");
        return;
    }
#endif
    test_DoDragDrop();
}
