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


#define METHOD_LIST \
    METHOD(DO_EnumFormatEtc), \
    METHOD(DO_QueryGetData), \
    METHOD(EnumFMT_Next), \
    METHOD(EnumFMT_Reset), \
    METHOD(EnumFMT_Skip), \
    METHOD(DS_QueryContinueDrag), \
    METHOD(DS_GiveFeedback), \
    METHOD(DT_DragEnter), \
    METHOD(DT_Drop), \
    METHOD(DT_DragLeave), \
    METHOD(DT_DragOver), \
    METHOD(DoDragDrop_effect_in), \
    METHOD(DoDragDrop_ret), \
    METHOD(DoDragDrop_effect_out), \
    METHOD(end_seq)

#define METHOD(x) x
enum method
{
    METHOD_LIST
};
#undef METHOD

#define METHOD(x) #x
static const char *method_names[] =
{
    METHOD_LIST
};
#undef METHOD
#undef METHOD_LIST

struct method_call
{
    enum method method;
    DWORD expect_param;

    HRESULT set_ret;
    DWORD set_param;

    BOOL called_todo;
};

const struct method_call *call_ptr;

static HRESULT check_expect_(enum method func, DWORD expect_param, DWORD *set_param, const char *file, int line )
{
    HRESULT hr;

    do
    {
        todo_wine_if(call_ptr->called_todo)
            ok_( file, line )( func == call_ptr->method, "unexpected call %s instead of %s\n",
                               method_names[func], method_names[call_ptr->method] );
        if (call_ptr->method == func) break;
    } while ((++call_ptr)->method != end_seq);

    ok_( file, line )( expect_param == call_ptr->expect_param, "%s: unexpected param %08lx expected %08lx\n",
                       method_names[func], expect_param, call_ptr->expect_param );
    if (set_param) *set_param = call_ptr->set_param;
    hr = call_ptr->set_ret;
    if (call_ptr->method != end_seq) call_ptr++;
   return hr;
}

#define check_expect(func, expect_param, set_param) \
    check_expect_((func), (expect_param), (set_param), __FILE__, __LINE__)


struct method_call call_lists[][30] =
{
    { /* First QueryContinueDrag rets DRAGDROP_S_DROP */
        { DoDragDrop_effect_in, 0, 0, DROPEFFECT_COPY, 0 },
        { DO_EnumFormatEtc, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_FALSE, 0, 1 },
        { EnumFMT_Reset, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_FALSE, 0, 1 },
        { DO_QueryGetData, 0, S_OK, 0, 1 },

        { DS_QueryContinueDrag, 0, DRAGDROP_S_DROP, 0, 0 },
        { DT_DragEnter, DROPEFFECT_COPY, S_OK, DROPEFFECT_COPY, 0 },
        { DS_GiveFeedback, DROPEFFECT_COPY, DRAGDROP_S_USEDEFAULTCURSORS, 0, 0 },
        { DT_Drop, DROPEFFECT_COPY, 0xbeefbeef, DROPEFFECT_COPY, 0 },

        { DoDragDrop_ret, 0xbeefbeef, 0, 0, 0, },
        { DoDragDrop_effect_out, DROPEFFECT_COPY, 0, 0, 0 },
        { end_seq, 0, 0, 0, 0 }
    },
    { /* As above, but initial effects == 0 */
        { DoDragDrop_effect_in, 0, 0, 0, 0 },
        { DO_EnumFormatEtc, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_FALSE, 0, 1 },
        { EnumFMT_Reset, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_FALSE, 0, 1 },
        { DO_QueryGetData, 0, S_OK, 0, 1 },

        { DS_QueryContinueDrag, 0, DRAGDROP_S_DROP, 0, 0 },
        { DT_DragEnter, 0, S_OK, DROPEFFECT_COPY, 0 },
        { DS_GiveFeedback, 0, DRAGDROP_S_USEDEFAULTCURSORS, 0, 0 },
        { DT_DragLeave, 0, 0, 0, 0 },

        { DoDragDrop_ret, DRAGDROP_S_DROP, 0, 0, 0 },
        { DoDragDrop_effect_out, 0, 0, 0, 0 },
        { end_seq, 0, 0, 0, 0 }
    },
    { /* Multiple initial effects */
        { DoDragDrop_effect_in, 0, 0, DROPEFFECT_COPY | DROPEFFECT_MOVE, 0 },
        { DO_EnumFormatEtc, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_FALSE, 0, 1 },
        { EnumFMT_Reset, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_FALSE, 0, 1 },
        { DO_QueryGetData, 0, S_OK, 0, 1 },

        { DS_QueryContinueDrag, 0, DRAGDROP_S_DROP, 0, 0 },
        { DT_DragEnter, DROPEFFECT_COPY | DROPEFFECT_MOVE, S_OK, DROPEFFECT_COPY, 0 },
        { DS_GiveFeedback, DROPEFFECT_COPY, DRAGDROP_S_USEDEFAULTCURSORS, 0, 0 },
        { DT_Drop, DROPEFFECT_COPY | DROPEFFECT_MOVE, 0, 0, 0 },

        { DoDragDrop_ret, DRAGDROP_S_DROP, 0, 0, 0 },
        { DoDragDrop_effect_out, 0, 0, 0, 0 },
        { end_seq, 0, 0, 0, 0 }
    },
    { /* First couple of QueryContinueDrag return S_OK followed by a drop */
        { DoDragDrop_effect_in, 0, 0, DROPEFFECT_COPY | DROPEFFECT_MOVE, 0 },
        { DO_EnumFormatEtc, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_FALSE, 0, 1 },
        { EnumFMT_Reset, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_FALSE, 0, 1 },
        { DO_QueryGetData, 0, S_OK, 0, 1 },

        { DS_QueryContinueDrag, 0, S_OK, 0, 0 },
        { DT_DragEnter, DROPEFFECT_COPY | DROPEFFECT_MOVE, S_OK, DROPEFFECT_COPY, 0 },
        { DS_GiveFeedback, DROPEFFECT_COPY, DRAGDROP_S_USEDEFAULTCURSORS, 0, 0 },
        { DT_DragOver, DROPEFFECT_COPY | DROPEFFECT_MOVE, S_OK, DROPEFFECT_COPY, 0 },
        { DS_GiveFeedback, DROPEFFECT_COPY, DRAGDROP_S_USEDEFAULTCURSORS, 0, 0 },

        { DS_QueryContinueDrag, 0, S_OK, 0, 0 },
        { DT_DragOver, DROPEFFECT_COPY | DROPEFFECT_MOVE, S_OK, DROPEFFECT_COPY, 0 },
        { DS_GiveFeedback, DROPEFFECT_COPY, DRAGDROP_S_USEDEFAULTCURSORS, 0, 0 },

        { DS_QueryContinueDrag, 0, DRAGDROP_S_DROP, 0, 0 },
        { DT_Drop, DROPEFFECT_COPY | DROPEFFECT_MOVE, 0, 0, 0 },

        { DoDragDrop_ret, DRAGDROP_S_DROP, 0, 0, 0 },
        { DoDragDrop_effect_out, 0, 0, 0, 0 },
        { end_seq, 0, 0, 0, 0 }
    },
    { /* First QueryContinueDrag cancels */
        { DoDragDrop_effect_in, 0, 0, DROPEFFECT_COPY | DROPEFFECT_MOVE, 0 },
        { DO_EnumFormatEtc, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_FALSE, 0, 1 },
        { EnumFMT_Reset, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_FALSE, 0, 1 },
        { DO_QueryGetData, 0, S_OK, 0, 1 },

        { DS_QueryContinueDrag, 0, DRAGDROP_S_CANCEL, 0, 0 },

        { DoDragDrop_ret, DRAGDROP_S_CANCEL, 0, 0, 0 },
        { DoDragDrop_effect_out, 0, 0, 0, 0 },
        { end_seq, 0, 0, 0, 0 }
    },
    { /* First couple of QueryContinueDrag return S_OK followed by a cancel */
        { DoDragDrop_effect_in, 0, 0, DROPEFFECT_COPY | DROPEFFECT_MOVE, 0 },
        { DO_EnumFormatEtc, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_FALSE, 0, 1 },
        { EnumFMT_Reset, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_FALSE, 0, 1 },
        { DO_QueryGetData, 0, S_OK, 0, 1 },

        { DS_QueryContinueDrag, 0, S_OK, 0, 0 },
        { DT_DragEnter, DROPEFFECT_COPY | DROPEFFECT_MOVE, S_OK, DROPEFFECT_COPY, 0 },
        { DS_GiveFeedback, DROPEFFECT_COPY, DRAGDROP_S_USEDEFAULTCURSORS, 0, 0 },
        { DT_DragOver, DROPEFFECT_COPY | DROPEFFECT_MOVE, S_OK, DROPEFFECT_COPY, 0 },
        { DS_GiveFeedback, DROPEFFECT_COPY, DRAGDROP_S_USEDEFAULTCURSORS, 0, 0 },

        { DS_QueryContinueDrag, 0, S_OK, 0, 0 },
        { DT_DragOver, DROPEFFECT_COPY | DROPEFFECT_MOVE, S_OK, DROPEFFECT_COPY, 0 },
        { DS_GiveFeedback, DROPEFFECT_COPY, DRAGDROP_S_USEDEFAULTCURSORS, 0, 0 },

        { DS_QueryContinueDrag, 0, DRAGDROP_S_CANCEL, 0, 0 },
        { DT_DragLeave, 0, 0, 0, 0 },

        { DoDragDrop_ret, DRAGDROP_S_CANCEL, 0, 0, 0 },
        { DoDragDrop_effect_out, 0, 0, 0, 0 },
        { end_seq, 0, 0, 0, 0 }
    },
    { /* First couple of QueryContinueDrag return S_OK followed by a E_FAIL */
        { DoDragDrop_effect_in, 0, 0, DROPEFFECT_COPY | DROPEFFECT_MOVE, 0 },
        { DO_EnumFormatEtc, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_FALSE, 0, 1 },
        { EnumFMT_Reset, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_OK, 0, 1 },
        { EnumFMT_Next, 0, S_FALSE, 0, 1 },
        { DO_QueryGetData, 0, S_OK, 0, 1 },

        { DS_QueryContinueDrag, 0, S_OK, 0, 0 },
        { DT_DragEnter, DROPEFFECT_COPY | DROPEFFECT_MOVE, S_OK, DROPEFFECT_COPY, 0 },
        { DS_GiveFeedback, DROPEFFECT_COPY, DRAGDROP_S_USEDEFAULTCURSORS, 0, 0 },
        { DT_DragOver, DROPEFFECT_COPY | DROPEFFECT_MOVE, S_OK, DROPEFFECT_COPY, 0 },
        { DS_GiveFeedback, DROPEFFECT_COPY, DRAGDROP_S_USEDEFAULTCURSORS, 0, 0 },

        { DS_QueryContinueDrag, 0, S_OK, 0, 0 },
        { DT_DragOver, DROPEFFECT_COPY | DROPEFFECT_MOVE, S_OK, DROPEFFECT_COPY, 0 },
        { DS_GiveFeedback, DROPEFFECT_COPY, DRAGDROP_S_USEDEFAULTCURSORS, 0, 0 },

        { DS_QueryContinueDrag, 0, E_FAIL, 0, 0 },
        { DT_DragLeave, 0, 0, 0, 0 },

        { DoDragDrop_ret, E_FAIL, 0, 0, 0 },
        { DoDragDrop_effect_out, 0, 0, 0, 0 },
        { end_seq, 0, 0, 0, 0 }
    },
};

static int droptarget_refs;
static int test_reentrance;

/* helper macros to make tests a bit leaner */
#define ok_ole_success(hr, func) ok(hr == S_OK, func " failed with error %#08lx\n", hr)

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
    return check_expect(DT_DragEnter, *pdwEffect, pdwEffect);
}

static HRESULT WINAPI DropTarget_DragOver(IDropTarget* iface,
                                          DWORD grfKeyState,
                                          POINTL pt,
                                          DWORD* pdwEffect)
{
    return check_expect(DT_DragOver, *pdwEffect, pdwEffect);
}

static HRESULT WINAPI DropTarget_DragLeave(IDropTarget* iface)
{
    return check_expect(DT_DragLeave, 0, NULL);
}

static HRESULT WINAPI DropTarget_Drop(IDropTarget* iface,
                                      IDataObject* pDataObj, DWORD grfKeyState,
                                      POINTL pt, DWORD* pdwEffect)
{
    return check_expect(DT_Drop, *pdwEffect, pdwEffect);
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
    HRESULT hr = check_expect(DS_QueryContinueDrag, 0, NULL);
    if (test_reentrance)
    {
        MSG msg;
        BOOL r;
        int num = 0;

        HWND hwnd = GetCapture();
        ok(hwnd != 0, "Expected capture window\n");

        /* send some fake events that should be ignored */
        r = PostMessageA(hwnd, WM_MOUSEMOVE, 0, 0);
        r &= PostMessageA(hwnd, WM_LBUTTONDOWN, 0, 0);
        r &= PostMessageA(hwnd, WM_LBUTTONUP, 0, 0);
        r &= PostMessageA(hwnd, WM_KEYDOWN, VK_ESCAPE, 0);
        ok(r, "Unable to post messages\n");

        /* run the message loop for this thread */
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
            num++;
        }

        ok(num >= 4, "Expected at least 4 messages but %d were processed\n", num);
    }
    return hr;
}

static HRESULT WINAPI DropSource_GiveFeedback(
    IDropSource *iface,
    DWORD dwEffect)
{
    return check_expect(DS_GiveFeedback, dwEffect, NULL);
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

static HRESULT WINAPI EnumFORMATETC_Next(IEnumFORMATETC *iface,
        ULONG celt, FORMATETC *rgelt, ULONG *pceltFetched)
{
    static FORMATETC format = { CF_TEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    HRESULT hr = check_expect(EnumFMT_Next, 0, NULL);

    ok(celt == 1, "celt = %ld\n", celt);
    ok(rgelt != NULL, "rgelt == NULL\n");
    ok(pceltFetched == NULL, "pceltFetched != NULL\n");

    *rgelt = format;
    return hr;
}

static HRESULT WINAPI EnumFORMATETC_Skip(IEnumFORMATETC *iface, ULONG celt)
{
    return check_expect(EnumFMT_Skip, 0, NULL);
}

static HRESULT WINAPI EnumFORMATETC_Reset(IEnumFORMATETC *iface)
{
    return check_expect(EnumFMT_Reset, 0, NULL);
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
    return check_expect(DO_QueryGetData, 0, NULL);
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
    HRESULT hr = check_expect(DO_EnumFormatEtc, 0, NULL);
    *ppenumFormatEtc = &EnumFORMATETC;
    return hr;
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
        "RegisterDragDrop without OLE initialized should have returned E_OUTOFMEMORY instead of 0x%08lx\n", hr);

    OleInitialize(NULL);

    hr = RegisterDragDrop(hwnd, NULL);
    ok(hr == E_INVALIDARG, "RegisterDragDrop with NULL IDropTarget * should return E_INVALIDARG instead of 0x%08lx\n", hr);

    hr = RegisterDragDrop(NULL, &DropTarget);
    ok(hr == DRAGDROP_E_INVALIDHWND, "RegisterDragDrop with NULL hwnd should return DRAGDROP_E_INVALIDHWND instead of 0x%08lx\n", hr);

    hr = RegisterDragDrop((HWND)0xdeadbeef, &DropTarget);
    ok(hr == DRAGDROP_E_INVALIDHWND, "RegisterDragDrop with garbage hwnd should return DRAGDROP_E_INVALIDHWND instead of 0x%08lx\n", hr);

    ok(droptarget_refs == 0, "DropTarget refs should be zero not %d\n", droptarget_refs);
    hr = RegisterDragDrop(hwnd, &DropTarget);
    ok_ole_success(hr, "RegisterDragDrop");
    ok(droptarget_refs >= 1, "DropTarget refs should be at least one\n");

    prop = GetPropA(hwnd, "OleDropTargetInterface");
    ok(prop == &DropTarget, "expected IDropTarget pointer %p, got %p\n", &DropTarget, prop);

    hr = RegisterDragDrop(hwnd, &DropTarget);
    ok(hr == DRAGDROP_E_ALREADYREGISTERED, "RegisterDragDrop with already registered hwnd should return DRAGDROP_E_ALREADYREGISTERED instead of 0x%08lx\n", hr);

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
    ok(hr == DRAGDROP_E_INVALIDHWND, "RevokeDragDrop with NULL hwnd should return DRAGDROP_E_INVALIDHWND instead of 0x%08lx\n", hr);

    DestroyWindow(hwnd);

    /* try to revoke with already destroyed window */
    OleInitialize(NULL);

    hwnd = CreateWindowA("WineOleTestClass", "Test", 0,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL,
        NULL, NULL, NULL);

    hr = RegisterDragDrop(hwnd, &DropTarget);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    DestroyWindow(hwnd);

    hr = RevokeDragDrop(hwnd);
    ok(hr == DRAGDROP_E_INVALIDHWND, "got 0x%08lx\n", hr);

    OleUninitialize();
}

static void test_DoDragDrop(void)
{
    DWORD effect;
    HRESULT hr;
    HWND hwnd;
    RECT rect;
    int seq;

    hwnd = CreateWindowExA(WS_EX_TOPMOST, "WineOleTestClass", "Test", 0,
        CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, NULL,
        NULL, NULL, NULL);
    ok(IsWindow(hwnd), "failed to create window\n");

    hr = OleInitialize(NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = RegisterDragDrop(hwnd, &DropTarget);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    /* incomplete arguments set */
    hr = DoDragDrop(NULL, NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    hr = DoDragDrop(NULL, &DropSource, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    hr = DoDragDrop(&DataObject, NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    hr = DoDragDrop(NULL, NULL, 0, &effect);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    hr = DoDragDrop(&DataObject, &DropSource, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    hr = DoDragDrop(NULL, &DropSource, 0, &effect);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    hr = DoDragDrop(&DataObject, NULL, 0, &effect);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    ShowWindow(hwnd, SW_SHOW);
    GetWindowRect(hwnd, &rect);
    ok(SetCursorPos(rect.left+50, rect.top+50), "SetCursorPos failed\n");

    for (test_reentrance = 0; test_reentrance < 2; test_reentrance++)
    {
        for (seq = 0; seq < ARRAY_SIZE(call_lists); seq++)
        {
            DWORD effect_in;
            trace("%d\n", seq);
            call_ptr = call_lists[seq];
            effect_in = call_ptr->set_param;
            call_ptr++;

            hr = DoDragDrop(&DataObject, &DropSource, effect_in, &effect);
            check_expect(DoDragDrop_ret, hr, NULL);
            check_expect(DoDragDrop_effect_out, effect, NULL);
        }
    }

    OleUninitialize();

    DestroyWindow(hwnd);
}

START_TEST(dragdrop)
{
    register_dummy_class();

    test_Register_Revoke();
    test_DoDragDrop();
}
