/*
 * Tests for IRichEditOle and friends.
 *
 * Copyright 2008 Google (Dan Hipschman)
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

#define COBJMACROS

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <initguid.h>
#include <ole2.h>
#include <richedit.h>
#include <richole.h>
#include <tom.h>
#include <imm.h>
#include <textserv.h>
#include <wine/test.h>
#ifdef __REACTOS__
#define debugstr_guid wine_dbgstr_guid
#endif

#define EXPECT_TODO_WINE 0x80000000UL

struct reolecb_obj
{
    IRichEditOleCallback IRichEditOleCallback_iface;
    LONG ref;
    int line;

    ULONG expect_queryinsertobject;

    const CLSID *clsid;
    IStorage *stg;
    LONG cp;
    HRESULT queryinsertobject_result;
};

static inline struct reolecb_obj *impl_from_IRichEditOleCallback(IRichEditOleCallback *iface)
{
    return CONTAINING_RECORD(iface, struct reolecb_obj, IRichEditOleCallback_iface);
}

static HRESULT STDMETHODCALLTYPE reolecb_obj_QueryInterface(IRichEditOleCallback *iface, REFIID riid, void **obj)
{
    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IRichEditOleCallback)) {
        IRichEditOleCallback_AddRef(iface);
        *obj = iface;
        return S_OK;
    }
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE reolecb_obj_AddRef(IRichEditOleCallback *iface)
{
    struct reolecb_obj *This = impl_from_IRichEditOleCallback(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE reolecb_obj_Release(IRichEditOleCallback *iface)
{
    struct reolecb_obj *This = impl_from_IRichEditOleCallback(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    if (!ref) free(This);
    return ref;
}

static HRESULT STDMETHODCALLTYPE reolecb_obj_GetNewStorage(IRichEditOleCallback *iface, IStorage **stg)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE reolecb_obj_GetInPlaceContext(IRichEditOleCallback *iface,
                                                               IOleInPlaceFrame **frame,
                                                               IOleInPlaceUIWindow **doc,
                                                               OLEINPLACEFRAMEINFO *frame_info)
{
    return E_INVALIDARG;
}

static HRESULT STDMETHODCALLTYPE reolecb_obj_ShowContainerUI(IRichEditOleCallback *iface, BOOL show)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE reolecb_obj_QueryInsertObject(IRichEditOleCallback *iface, CLSID *clsid,
                                                               IStorage *stg, LONG cp)
{
    struct reolecb_obj *This = impl_from_IRichEditOleCallback(iface);
    ULONG expect = This->expect_queryinsertobject;

    todo_wine_if(expect & EXPECT_TODO_WINE)
    ok_(__FILE__,This->line)( expect & ~EXPECT_TODO_WINE,
                              "unexpected call to IRichEditOleCallback_QueryInsertObject\n");
    if (!(expect & ~EXPECT_TODO_WINE)) return S_OK;
    This->expect_queryinsertobject--;

    if (This->clsid && clsid)
        ok_(__FILE__,This->line)( IsEqualGUID(This->clsid, clsid),
                                  "QueryInsertObject clsid expected %s, got %s\n",
                                  wine_dbgstr_guid( This->clsid ), wine_dbgstr_guid( clsid ));
    else
        ok_(__FILE__,This->line)( This->clsid == clsid,
                                  "QueryInsertObject clsid expected %p, got %p\n", This->clsid, clsid );
    ok_(__FILE__,This->line)( This->stg == stg, "QueryInsertObject stg expected %p, got %p\n", This->stg, stg );
    ok_(__FILE__,This->line)( This->cp == cp, "QueryInsertObject cp expected %ld, got %ld\n", This->cp, cp );
    return This->queryinsertobject_result;
}

static HRESULT STDMETHODCALLTYPE reolecb_obj_DeleteObject(IRichEditOleCallback *iface, IOleObject *oleobj)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE reolecb_obj_QueryAcceptData(IRichEditOleCallback *iface, IDataObject *dataobj,
                                                             CLIPFORMAT *cf_format, DWORD reco, BOOL really,
                                                             HGLOBAL metapict)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE reolecb_obj_ContextSensitiveHelp(IRichEditOleCallback *iface, BOOL enter_mode)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE reolecb_obj_GetClipboardData(IRichEditOleCallback *iface, CHARRANGE *chrg,
                                                              DWORD reco, IDataObject **dataobj)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE reolecb_obj_GetDragDropEffect(IRichEditOleCallback *iface, BOOL drag,
                                                               DWORD key_state, DWORD *effect)
{
    if (effect) *effect = DROPEFFECT_COPY;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE reolecb_obj_GetContextMenu(IRichEditOleCallback *iface, WORD seltype,
                                                            IOleObject *oleobj, CHARRANGE *chrg, HMENU *hmenu)
{
    return E_NOTIMPL;
}

static const struct IRichEditOleCallbackVtbl reolecb_obj_Vtbl = {
    reolecb_obj_QueryInterface,
    reolecb_obj_AddRef,
    reolecb_obj_Release,
    reolecb_obj_GetNewStorage,
    reolecb_obj_GetInPlaceContext,
    reolecb_obj_ShowContainerUI,
    reolecb_obj_QueryInsertObject,
    reolecb_obj_DeleteObject,
    reolecb_obj_QueryAcceptData,
    reolecb_obj_ContextSensitiveHelp,
    reolecb_obj_GetClipboardData,
    reolecb_obj_GetDragDropEffect,
    reolecb_obj_GetContextMenu,
};

static HRESULT reolecb_obj_Create(struct reolecb_obj **objptr)
{
    struct reolecb_obj *obj;

    obj = calloc(sizeof(struct reolecb_obj), 1);
    if (!obj) return E_OUTOFMEMORY;

#ifdef __REACTOS__
    obj->IRichEditOleCallback_iface.lpVtbl = (IRichEditOleCallbackVtbl*)&reolecb_obj_Vtbl;
#else
    obj->IRichEditOleCallback_iface.lpVtbl = &reolecb_obj_Vtbl;
#endif
    obj->ref = 1;

    *objptr = obj;
    return S_OK;
}

static void olecb_expect_QueryInsertObject(struct reolecb_obj *This, int line, ULONG expect,
                                           const CLSID *clsid, IStorage *stg, LONG cp, HRESULT result)
{
    if (!This) return;

    This->line = line;
    This->expect_queryinsertobject = expect;
    This->clsid = clsid;
    This->stg = stg;
    This->cp = cp;
    This->queryinsertobject_result = result;
}

static void olecb_check_QueryInsertObject(struct reolecb_obj *This, int line)
{
    if (!This) return;

    todo_wine_if(This->expect_queryinsertobject & EXPECT_TODO_WINE)
    ok( !(This->expect_queryinsertobject & ~EXPECT_TODO_WINE),
        "expected IRichEditOleCallback_QueryInsertObject to be called\n" );

    olecb_expect_QueryInsertObject(This, 0, 0, NULL, NULL, 0, S_OK);
}

DEFINE_GUID(CLSID_testoleobj, 0x4484082e, 0x6d18, 0x4932, 0xa0, 0x86, 0x5b, 0x4d, 0xcf, 0x36, 0xb3, 0xde);

struct testoleobj {
    IOleObject IOleObject_iface;
    LONG ref;
    int line;
    int draw_count;

    IOleClientSite *clientsite;
    IOleAdviseHolder *advise_holder;
    SIZEL extent;

    IViewObject IViewObject_iface;
};

static struct testoleobj *impl_from_IOleObject( IOleObject *iface )
{
    return CONTAINING_RECORD( iface, struct testoleobj, IOleObject_iface );
}

static HRESULT STDMETHODCALLTYPE testoleobj_QueryInterface( IOleObject *iface, REFIID riid, void **obj )
{
    struct testoleobj *This = impl_from_IOleObject( iface );

    if (IsEqualGUID( riid, &IID_IUnknown ) || IsEqualGUID( riid, &IID_IOleObject ))
    {
        *obj = iface;
    }
    else if (IsEqualGUID( riid, &IID_IViewObject ))
    {
        *obj = &This->IViewObject_iface;
    }
    else
    {
        if (!IsEqualGUID( riid, &IID_IOleLink ) &&
            !IsEqualGUID( riid, &IID_IRunnableObject ) &&
            !IsEqualGUID( riid, &IID_IMarshal ))
        {
            trace( "Unsupported interface: %s\n", debugstr_guid( riid ));
        }
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef( (IUnknown *)*obj );
    return S_OK;
}

static ULONG STDMETHODCALLTYPE testoleobj_AddRef( IOleObject *iface )
{
    struct testoleobj *This = impl_from_IOleObject( iface );
    ULONG ref = InterlockedIncrement( &This->ref );
    return ref;
}

static ULONG STDMETHODCALLTYPE testoleobj_Release( IOleObject *iface )
{
    struct testoleobj *This = impl_from_IOleObject( iface );
    ULONG ref = InterlockedDecrement( &This->ref );
    if (!ref)
    {
        if (This->advise_holder)
        {
            IOleAdviseHolder_Release( This->advise_holder );
            This->advise_holder = NULL;
        }
        if (This->clientsite)
        {
            IOleClientSite_Release( This->clientsite );
            This->clientsite = NULL;
        }
        free( This );
    }
    return ref;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IOleObject_SetClientSite( IOleObject *iface, IOleClientSite *clientsite )
{
    struct testoleobj *This = impl_from_IOleObject( iface );

    if (This->clientsite != clientsite)
    {
        if (This->clientsite) IOleClientSite_Release( This->clientsite );
        This->clientsite = clientsite;
        if (This->clientsite) IOleClientSite_AddRef( This->clientsite );
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IOleObject_GetClientSite( IOleObject *iface, IOleClientSite **clientsite )
{
    struct testoleobj *This = impl_from_IOleObject( iface );

    if (This->clientsite) IOleClientSite_AddRef( This->clientsite );
    *clientsite = This->clientsite;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IOleObject_SetHostNames( IOleObject *iface,
                                                                     LPCOLESTR container_app,
                                                                     LPCOLESTR container_obj )
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IOleObject_Close( IOleObject *iface, DWORD save_option )
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IOleObject_SetMoniker( IOleObject *iface,
                                                                   DWORD which_moniker, IMoniker *mk )
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IOleObject_GetMoniker( IOleObject *iface, DWORD assign,
                                                                   DWORD which_moniker, IMoniker **mk )
{
    struct testoleobj *This = impl_from_IOleObject( iface );

    *mk = NULL;

    if (!This->clientsite) return E_UNEXPECTED;

    return IOleClientSite_GetMoniker( This->clientsite, assign, which_moniker, mk );
}

static HRESULT STDMETHODCALLTYPE testoleobj_IOleObject_InitFromData( IOleObject *iface, IDataObject *dataobj,
                                                                     BOOL creation, DWORD reserved )
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IOleObject_GetClipboardData( IOleObject *iface, DWORD reserved,
                                                                         IDataObject **dataobj )
{
    *dataobj = NULL;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IOleObject_DoVerb( IOleObject *iface, LONG verb, MSG *msg,
                                                               IOleClientSite *activesite, LONG index,
                                                               HWND parentwnd, LPCRECT posrect )
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IOleObject_EnumVerbs( IOleObject *iface, IEnumOLEVERB **enumoleverb )
{
    *enumoleverb = NULL;
    return OLEOBJ_E_NOVERBS;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IOleObject_Update( IOleObject *iface )
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IOleObject_IsUpToDate( IOleObject *iface )
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IOleObject_GetUserClassID( IOleObject *iface, CLSID *clsid )
{
    *clsid = CLSID_testoleobj;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IOleObject_GetUserType( IOleObject *iface, DWORD form_of_type, LPOLESTR *user_type )
{
    static const OLECHAR typename[] = L"richole testoleobj";

    *user_type = CoTaskMemAlloc( sizeof(typename) );
    if (!*user_type) return E_OUTOFMEMORY;

    memcpy( *user_type, typename, sizeof(typename) );
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IOleObject_SetExtent( IOleObject *iface, DWORD draw_aspect, SIZEL *sizel )
{
    struct testoleobj *This = impl_from_IOleObject( iface );

    if (draw_aspect != DVASPECT_CONTENT) return E_FAIL;

    This->extent = *sizel;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IOleObject_GetExtent( IOleObject *iface, DWORD draw_aspect, SIZEL *sizel )
{
    struct testoleobj *This = impl_from_IOleObject( iface );

    if (draw_aspect != DVASPECT_CONTENT) return E_FAIL;

    *sizel = This->extent;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IOleObject_Advise( IOleObject *iface, IAdviseSink *adv_sink, DWORD *connection )
{
    struct testoleobj *This = impl_from_IOleObject( iface );
    HRESULT hr = S_OK;

    if (!This->advise_holder) hr = CreateOleAdviseHolder( &This->advise_holder );
    if (SUCCEEDED( hr )) hr = IOleAdviseHolder_Advise( This->advise_holder, adv_sink, connection );
    return hr;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IOleObject_Unadvise( IOleObject *iface, DWORD connection )
{
    struct testoleobj *This = impl_from_IOleObject( iface );

    if (!This->advise_holder) return OLE_E_NOCONNECTION;
    return IOleAdviseHolder_Unadvise( This->advise_holder, connection );
}

static HRESULT STDMETHODCALLTYPE testoleobj_IOleObject_EnumAdvise( IOleObject *iface, IEnumSTATDATA **enum_advise )
{
    struct testoleobj *This = impl_from_IOleObject( iface );

    if (!This->advise_holder)
    {
        *enum_advise = NULL;
        return S_OK;
    }
    return IOleAdviseHolder_EnumAdvise( This->advise_holder, enum_advise );
}

static HRESULT STDMETHODCALLTYPE testoleobj_IOleObject_GetMiscStatus( IOleObject *iface, DWORD aspect, DWORD *status )
{
    *status = 0;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IOleObject_SetColorScheme( IOleObject *iface, LOGPALETTE *palette )
{
    return E_NOTIMPL;
}

static const struct IOleObjectVtbl testoleobj_IOleObject_Vtbl = {
    testoleobj_QueryInterface,
    testoleobj_AddRef,
    testoleobj_Release,
    testoleobj_IOleObject_SetClientSite,
    testoleobj_IOleObject_GetClientSite,
    testoleobj_IOleObject_SetHostNames,
    testoleobj_IOleObject_Close,
    testoleobj_IOleObject_SetMoniker,
    testoleobj_IOleObject_GetMoniker,
    testoleobj_IOleObject_InitFromData,
    testoleobj_IOleObject_GetClipboardData,
    testoleobj_IOleObject_DoVerb,
    testoleobj_IOleObject_EnumVerbs,
    testoleobj_IOleObject_Update,
    testoleobj_IOleObject_IsUpToDate,
    testoleobj_IOleObject_GetUserClassID,
    testoleobj_IOleObject_GetUserType,
    testoleobj_IOleObject_SetExtent,
    testoleobj_IOleObject_GetExtent,
    testoleobj_IOleObject_Advise,
    testoleobj_IOleObject_Unadvise,
    testoleobj_IOleObject_EnumAdvise,
    testoleobj_IOleObject_GetMiscStatus,
    testoleobj_IOleObject_SetColorScheme
};

static struct testoleobj *impl_from_IViewObject( IViewObject *iface )
{
    return CONTAINING_RECORD( iface, struct testoleobj, IViewObject_iface );
}

static HRESULT STDMETHODCALLTYPE testoleobj_IViewObject_QueryInterface( IViewObject *iface, REFIID riid, void **obj )
{
    struct testoleobj *This = impl_from_IViewObject( iface );
    return IOleObject_QueryInterface( &This->IOleObject_iface, riid, obj );
}

static ULONG STDMETHODCALLTYPE testoleobj_IViewObject_AddRef( IViewObject *iface )
{
    struct testoleobj *This = impl_from_IViewObject( iface );
    return IOleObject_AddRef( &This->IOleObject_iface );
}

static ULONG STDMETHODCALLTYPE testoleobj_IViewObject_Release( IViewObject *iface )
{
    struct testoleobj *This = impl_from_IViewObject( iface );
    return IOleObject_Release( &This->IOleObject_iface );
}

static HRESULT STDMETHODCALLTYPE testoleobj_IViewObject_Draw( IViewObject *iface, DWORD draw_aspect,
                                                              LONG index, void *aspect, DVTARGETDEVICE *td,
                                                              HDC hdc_target_dev, HDC hdc_draw,
                                                              LPCRECTL bounds, LPCRECTL wbounds,
                                                              BOOL (CALLBACK *fn_continue)(ULONG_PTR),
                                                              ULONG_PTR arg_continue )
{
    struct testoleobj *This = impl_from_IViewObject( iface );
    SIZEL dpi;

    if (draw_aspect != DVASPECT_CONTENT || index != -1) return E_NOTIMPL;

    ok_(__FILE__,This->line)( td == NULL, "expected td to be NULL, got %p\n", td );
    ok_(__FILE__,This->line)( hdc_target_dev == NULL, "expected hdc_target_dev to be NULL, got %p\n", hdc_target_dev );
    ok_(__FILE__,This->line)( wbounds == NULL, "expected wbounds to be NULL, got %p\n", wbounds );

    dpi.cx = GetDeviceCaps(hdc_draw, LOGPIXELSX);
    dpi.cy = GetDeviceCaps(hdc_draw, LOGPIXELSY);

    ok_(__FILE__,This->line)( bounds->right - bounds->left == MulDiv( This->extent.cx, dpi.cx, 2540 ),
                              "bounds->right (= %ld) - bounds->left (= %ld) != "
                              "MulDiv( This->extent.cx (= %ld), dpi.cx (= %ld), 2540 )\n",
                              bounds->right, bounds->left, This->extent.cx, dpi.cx );
    ok_(__FILE__,This->line)( bounds->bottom - bounds->top == MulDiv( This->extent.cy, dpi.cy, 2540 ),
                              "bounds->bottom (= %ld) - bounds->top (= %ld) != "
                              "MulDiv( This->extent.cy (= %ld), dpi.cy (= %ld), 2540 )\n",
                              bounds->bottom, bounds->top, This->extent.cy, dpi.cy );

    FillRect( hdc_draw, (const RECT *)bounds, GetStockObject( DKGRAY_BRUSH ));
    This->draw_count++;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IViewObject_GetColorSet( IViewObject *iface, DWORD draw_aspect,
                                                                     LONG index, void *aspect, DVTARGETDEVICE *td,
                                                                     HDC hdc_target_dev, LOGPALETTE **color_set )
{
    *color_set = NULL;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IViewObject_Freeze( IViewObject *iface, DWORD draw_aspect,
                                                                LONG index, void *aspect, DWORD *freeze )
{
    *freeze = 0;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IViewObject_Unfreeze( IViewObject *iface, DWORD freeze )
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IViewObject_SetAdvise( IViewObject *iface, DWORD aspects,
                                                                   DWORD advf, IAdviseSink *adv_sink )
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE testoleobj_IViewObject_GetAdvise( IViewObject *iface, DWORD *aspects,
                                                                   DWORD *advf, IAdviseSink **adv_sink )
{
    *aspects = 0;
    *advf = 0;
    *adv_sink = NULL;
    return E_NOTIMPL;
}

static const struct IViewObjectVtbl testoleobj_IViewObject_Vtbl = {
    testoleobj_IViewObject_QueryInterface,
    testoleobj_IViewObject_AddRef,
    testoleobj_IViewObject_Release,
    testoleobj_IViewObject_Draw,
    testoleobj_IViewObject_GetColorSet,
    testoleobj_IViewObject_Freeze,
    testoleobj_IViewObject_Unfreeze,
    testoleobj_IViewObject_SetAdvise,
    testoleobj_IViewObject_GetAdvise,
};

static HRESULT testoleobj_Create( struct testoleobj **objptr )
{
    struct testoleobj *obj;

    obj = calloc( sizeof(struct testoleobj), 1 );
    *objptr = obj;
    if (!obj) return E_OUTOFMEMORY;


#ifdef __REACTOS__
    obj->IOleObject_iface.lpVtbl = (IOleObjectVtbl*)&testoleobj_IOleObject_Vtbl;
#else
    obj->IOleObject_iface.lpVtbl = &testoleobj_IOleObject_Vtbl;
#endif
    obj->ref = 1;
#ifdef __REACTOS__
    obj->IViewObject_iface.lpVtbl = (IViewObjectVtbl*)&testoleobj_IViewObject_Vtbl;
#else
    obj->IViewObject_iface.lpVtbl = &testoleobj_IViewObject_Vtbl;
#endif

    return S_OK;
}

static HMODULE hmoduleRichEdit;

DEFINE_GUID(GUID_NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
DEFINE_GUID(IID_ITextServices, 0x8d33f740, 0xcf58, 0x11ce, 0xa8, 0x9d, 0x00, 0xaa, 0x00, 0x6c, 0xad, 0xc5);

static const WCHAR sysW[] = {'S','y','s','t','e','m',0};

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "expected refcount %ld, got %ld\n", ref, rc);
}

static HWND new_window(LPCSTR lpClassName, DWORD dwStyle, HWND parent)
{
  HWND hwnd = CreateWindowA(lpClassName, NULL,
                            dwStyle | WS_POPUP | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE,
                            0, 0, 200, 60, parent, NULL, hmoduleRichEdit, NULL);
  ok(hwnd != NULL, "class: %s, error: %d\n", lpClassName, (int) GetLastError());
  return hwnd;
}

static HWND new_richedit(HWND parent)
{
  return new_window(RICHEDIT_CLASS20A, ES_MULTILINE, parent);
}

static BOOL touch_file(LPCWSTR filename)
{
  HANDLE file;

  file = CreateFileW(filename, GENERIC_READ|GENERIC_WRITE, 0, NULL,
		     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

  if(file == INVALID_HANDLE_VALUE)
    return FALSE;
  CloseHandle(file);
  return TRUE;
}

static BOOL is_existing_file(LPCWSTR filename)
{
  HANDLE file;

  file = CreateFileW(filename, GENERIC_READ, 0, NULL,
		     OPEN_EXISTING, 0, NULL);
  if(file == INVALID_HANDLE_VALUE)
    return FALSE;
  CloseHandle(file);
  return TRUE;
}

static void create_interfaces(HWND *w, IRichEditOle **reOle, ITextDocument **txtDoc,
                              ITextSelection **txtSel)
{
  *w = new_richedit(NULL);
  SendMessageA(*w, EM_GETOLEINTERFACE, 0, (LPARAM)reOle);
  IRichEditOle_QueryInterface(*reOle, &IID_ITextDocument,
                                 (void **) txtDoc);
  ITextDocument_GetSelection(*txtDoc, txtSel);
}

static void release_interfaces(HWND *w, IRichEditOle **reOle, ITextDocument **txtDoc,
                               ITextSelection **txtSel)
{
  if(txtSel)
    ITextSelection_Release(*txtSel);
  ITextDocument_Release(*txtDoc);
  IRichEditOle_Release(*reOle);
  DestroyWindow(*w);
}

static ULONG get_refcount(IUnknown *iface)
{
  IUnknown_AddRef(iface);
  return IUnknown_Release(iface);
}

#define CHECK_TYPEINFO(disp,expected_riid) _check_typeinfo((IDispatch *)disp, expected_riid, __LINE__)
static void _check_typeinfo(IDispatch* disp, REFIID expected_riid, int line)
{
    ITypeInfo *typeinfo;
    TYPEATTR *typeattr;
    UINT count;
    HRESULT hr;

    count = 10;
    hr = IDispatch_GetTypeInfoCount(disp, &count);
    ok_(__FILE__,line)(hr == S_OK, "IDispatch_GetTypeInfoCount failed: 0x%08lx.\n", hr);
    ok_(__FILE__,line)(count == 1, "got wrong count: %u.\n", count);

    hr = IDispatch_GetTypeInfo(disp, 0, LOCALE_SYSTEM_DEFAULT, &typeinfo);
    ok_(__FILE__,line)(hr == S_OK, "IDispatch_GetTypeInfo failed: 0x%08lx.\n", hr);

    hr = ITypeInfo_GetTypeAttr(typeinfo, &typeattr);
    ok_(__FILE__,line)(hr == S_OK, "ITypeInfo_GetTypeAttr failed: 0x%08lx.\n", hr);
    ok_(__FILE__,line)(IsEqualGUID(&typeattr->guid, expected_riid),
                       "Unexpected type guid: %s.\n", wine_dbgstr_guid(&typeattr->guid));

    ITypeInfo_ReleaseTypeAttr(typeinfo, typeattr);
    ITypeInfo_Release(typeinfo);
}

static void test_Interfaces(void)
{
  IRichEditOle *reOle = NULL, *reOle1 = NULL;
  ITextDocument *txtDoc = NULL;
  ITextDocument2Old *txtDoc2Old = NULL;
  ITextSelection *txtSel = NULL, *txtSel2;
  IUnknown *punk;
  HRESULT hres;
  LRESULT res;
  HWND w;
  ULONG refcount;
  IUnknown *unk, *unk2;

  w = new_richedit(NULL);
  if (!w) {
    skip("Couldn't create window\n");
    return;
  }

  res = SendMessageA(w, EM_GETOLEINTERFACE, 0, (LPARAM)&reOle);
  ok(res, "SendMessage\n");
  ok(reOle != NULL, "EM_GETOLEINTERFACE\n");
  EXPECT_REF(reOle, 2);

  res = SendMessageA(w, EM_GETOLEINTERFACE, 0, (LPARAM)&reOle1);
  ok(res == 1, "SendMessage\n");
  ok(reOle1 == reOle, "Should not return a new IRichEditOle interface\n");
  EXPECT_REF(reOle, 3);

  hres = IRichEditOle_QueryInterface(reOle, &IID_ITextDocument,
                                 (void **) &txtDoc);
  ok(hres == S_OK, "IRichEditOle_QueryInterface\n");
  ok(txtDoc != NULL, "IRichEditOle_QueryInterface\n");
  CHECK_TYPEINFO(txtDoc, &IID_ITextDocument);

  hres = ITextDocument_GetSelection(txtDoc, NULL);
  ok(hres == E_INVALIDARG, "ITextDocument_GetSelection: 0x%lx\n", hres);

  EXPECT_REF(txtDoc, 4);

  hres = ITextDocument_GetSelection(txtDoc, &txtSel);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  hres = ITextDocument_QueryInterface(txtDoc, &IID_IUnknown, (void **)&unk);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = ITextSelection_QueryInterface(txtSel, &IID_IUnknown, (void **)&unk2);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(unk != unk2, "unknowns are the same\n");
  IUnknown_Release(unk2);
  IUnknown_Release(unk);

  EXPECT_REF(txtDoc, 4);
  EXPECT_REF(txtSel, 2);

  hres = ITextDocument_GetSelection(txtDoc, &txtSel2);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(txtSel2 == txtSel, "got %p, %p\n", txtSel, txtSel2);

  EXPECT_REF(txtDoc, 4);
  EXPECT_REF(txtSel, 3);

  ITextSelection_Release(txtSel2);

  punk = NULL;
  hres = ITextSelection_QueryInterface(txtSel, &IID_ITextSelection, (void **) &punk);
  ok(hres == S_OK, "ITextSelection_QueryInterface\n");
  ok(punk != NULL, "ITextSelection_QueryInterface\n");
  IUnknown_Release(punk);

  punk = NULL;
  hres = ITextSelection_QueryInterface(txtSel, &IID_ITextRange, (void **) &punk);
  ok(hres == S_OK, "ITextSelection_QueryInterface\n");
  ok(punk != NULL, "ITextSelection_QueryInterface\n");
  IUnknown_Release(punk);

  punk = NULL;
  hres = ITextSelection_QueryInterface(txtSel, &IID_IDispatch, (void **) &punk);
  ok(hres == S_OK, "ITextSelection_QueryInterface\n");
  ok(punk != NULL, "ITextSelection_QueryInterface\n");
  IUnknown_Release(punk);

  punk = NULL;
  hres = IRichEditOle_QueryInterface(reOle, &IID_IOleClientSite, (void **) &punk);
  ok(hres == E_NOINTERFACE, "IRichEditOle_QueryInterface\n");

  punk = NULL;
  hres = IRichEditOle_QueryInterface(reOle, &IID_IOleWindow, (void **) &punk);
  ok(hres == E_NOINTERFACE, "IRichEditOle_QueryInterface\n");

  punk = NULL;
  hres = IRichEditOle_QueryInterface(reOle, &IID_IOleInPlaceSite, (void **) &punk);
  ok(hres == E_NOINTERFACE, "IRichEditOle_QueryInterface\n");

  hres = IRichEditOle_QueryInterface(reOle, &IID_ITextDocument2Old, (void **)&txtDoc2Old);
  ok(hres == S_OK, "IRichEditOle_QueryInterface\n");
  ok(txtDoc2Old != NULL, "IRichEditOle_QueryInterface\n");
  ok((ITextDocument *)txtDoc2Old == txtDoc, "interface pointer isn't equal.\n");
  EXPECT_REF(txtDoc2Old, 5);
  EXPECT_REF(reOle, 5);
  CHECK_TYPEINFO(txtDoc2Old, &IID_ITextDocument);

  ITextDocument2Old_Release(txtDoc2Old);

  ITextDocument_Release(txtDoc);
  IRichEditOle_Release(reOle);
  refcount = IRichEditOle_Release(reOle);
  ok(refcount == 1, "got wrong ref count: %ld\n", refcount);
  DestroyWindow(w);

  /* Methods should return CO_E_RELEASED if the backing document has
     been released.  One test should suffice.  */
  hres = ITextSelection_CanEdit(txtSel, NULL);
  ok(hres == CO_E_RELEASED, "ITextSelection after ITextDocument destroyed\n");

  ITextSelection_Release(txtSel);

  w = new_richedit(NULL);
  res = SendMessageA(w, EM_GETOLEINTERFACE, 0, (LPARAM)&reOle);
  ok(res, "SendMessage\n");
  ok(reOle != NULL, "EM_GETOLEINTERFACE\n");

  hres = IRichEditOle_QueryInterface(reOle, &IID_ITextDocument2Old, (void **)&txtDoc2Old);
  ok(hres == S_OK, "IRichEditOle_QueryInterface failed: 0x%08lx.\n", hres);
  ok(txtDoc2Old != NULL, "IRichEditOle_QueryInterface\n");
  CHECK_TYPEINFO(txtDoc2Old, &IID_ITextDocument);
  ITextDocument2Old_Release(txtDoc2Old);
  IRichEditOle_Release(reOle);
  DestroyWindow(w);
}

static void test_ITextDocument_Open(void)
{
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextSelection *txtSel = NULL;
  HRESULT hres;
  HWND w;
  HANDLE hFile;
  VARIANT testfile;
  WCHAR filename[] = {'t', 'e', 's', 't','.','t','x','t', 0};
  int result;
  DWORD dw;
  static const CHAR chACP[] = "TestSomeText";
  static const CHAR chUTF8[] = "\xef\xbb\xbfTextWithUTF8BOM";
  static const WCHAR chUTF16[] = {0xfeff, 'T', 'e', 's', 't', 'S', 'o', 'm',
                                  'e', 'T', 'e', 'x', 't', 0};

#define MAX_BUF_LEN 1024
  CHAR bufACP[MAX_BUF_LEN];
  WCHAR bufUnicode[MAX_BUF_LEN];

  static const int tomConstantsSingle[] =
    {
      tomReadOnly, tomShareDenyRead, tomShareDenyWrite,
      tomCreateAlways, tomOpenExisting, tomOpenAlways,
      tomTruncateExisting, tomRTF, tomText
    };

  static const int tomConstantsMulti[] =
    {
      tomReadOnly|tomShareDenyRead|tomPasteFile, tomReadOnly|tomPasteFile,
      tomReadOnly|tomShareDenyWrite|tomPasteFile,
      tomReadOnly|tomShareDenyRead|tomShareDenyWrite|tomPasteFile, tomShareDenyWrite|tomPasteFile,
      tomShareDenyRead|tomShareDenyWrite|tomPasteFile, tomShareDenyRead|tomPasteFile,
      tomShareDenyRead|tomShareDenyWrite, tomReadOnly|tomShareDenyRead|tomShareDenyWrite,
      tomReadOnly|tomShareDenyWrite, tomReadOnly|tomShareDenyRead
    };

  int tomNumSingle =  ARRAY_SIZE(tomConstantsSingle);
  int tomNumMulti = ARRAY_SIZE(tomConstantsMulti);
  int i;

  V_VT(&testfile) = VT_BSTR;
  V_BSTR(&testfile) = SysAllocString(filename);

  for(i=0; i < tomNumSingle; i++)
    {
      touch_file(filename);
      create_interfaces(&w, &reOle, &txtDoc, &txtSel);
      hres = ITextDocument_Open(txtDoc, &testfile, tomConstantsSingle[i], CP_ACP);
      todo_wine ok(hres == S_OK, "ITextDocument_Open: Filename:test.txt Flags:0x%x Codepage:CP_ACP hres:0x%lx\n",
         tomConstantsSingle[i], hres);
      release_interfaces(&w, &reOle, &txtDoc, &txtSel);
      DeleteFileW(filename);

      touch_file(filename);
      create_interfaces(&w, &reOle, &txtDoc, &txtSel);
      hres = ITextDocument_Open(txtDoc, &testfile, tomConstantsSingle[i], CP_UTF8);
      todo_wine ok(hres == S_OK, "ITextDocument_Open: Filename:test.txt Flags:0x%x Codepage:CP_UTF8 hres:0x%lx\n",
         tomConstantsSingle[i], hres);
      release_interfaces(&w, &reOle, &txtDoc, &txtSel);
      DeleteFileW(filename);
    }

  for(i=0; i < tomNumMulti; i++)
    {
      touch_file(filename);
      create_interfaces(&w, &reOle, &txtDoc, &txtSel);
      hres = ITextDocument_Open(txtDoc, &testfile, tomConstantsMulti[i], CP_ACP);
      todo_wine ok(hres == S_OK, "ITextDocument_Open: Filename:test.txt Flags:0x%x Codepage:CP_ACP hres:0x%lx\n",
         tomConstantsMulti[i], hres);
      release_interfaces(&w, &reOle, &txtDoc, &txtSel);
      DeleteFileW(filename);

      touch_file(filename);
      create_interfaces(&w, &reOle, &txtDoc, &txtSel);
      hres = ITextDocument_Open(txtDoc, &testfile, tomConstantsMulti[i], CP_UTF8);
      todo_wine ok(hres == S_OK, "ITextDocument_Open: Filename:test.txt Flags:0x%x Codepage:CP_UTF8 hres:0x%lx\n",
         tomConstantsMulti[i], hres);
      release_interfaces(&w, &reOle, &txtDoc, &txtSel);
      DeleteFileW(filename);
    }

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomCreateAlways, CP_ACP);
  todo_wine ok(hres == S_OK, "ITextDocument_Open should success Codepage:CP_ACP\n");
  todo_wine ok(is_existing_file(filename), "ITextDocument_Open should create a file\n");
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomCreateAlways, CP_UTF8);
  todo_wine ok(hres == S_OK, "ITextDocument_Open should success Codepage:CP_UTF8\n");
  todo_wine ok(is_existing_file(filename), "ITextDocument_Open should create a file\n");
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomOpenAlways, CP_ACP);
  todo_wine ok(hres == S_OK, "ITextDocument_Open should success Codepage:CP_ACP\n");
  todo_wine ok(is_existing_file(filename), "ITextDocument_Open should create a file\n");
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomOpenAlways, CP_UTF8);
  todo_wine ok(hres == S_OK, "ITextDocument_Open should success Codepage:CP_UTF8\n");
  todo_wine ok(is_existing_file(filename), "ITextDocument_Open should create a file\n");
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomCreateNew, CP_ACP);
  todo_wine ok(hres == S_OK, "ITextDocument_Open should success Codepage:CP_ACP\n");
  todo_wine ok(is_existing_file(filename), "ITextDocument_Open should create a file\n");
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomCreateNew, CP_UTF8);
  todo_wine ok(hres == S_OK, "ITextDocument_Open should success Codepage:CP_UTF8\n");
  todo_wine ok(is_existing_file(filename), "ITextDocument_Open should create a file\n");
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  touch_file(filename);
  hres = ITextDocument_Open(txtDoc, &testfile, tomCreateNew, CP_ACP);
  todo_wine ok(hres == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS), "ITextDocument_Open should fail Codepage:CP_ACP\n");
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  touch_file(filename);
  hres = ITextDocument_Open(txtDoc, &testfile, tomCreateNew, CP_UTF8);
  todo_wine ok(hres == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS), "ITextDocument_Open should fail Codepage:CP_UTF8\n");
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomOpenExisting, CP_ACP);
  todo_wine ok(hres == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "ITextDocument_Open should fail Codepage:CP_ACP\n");
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomOpenExisting, CP_UTF8);
  todo_wine ok(hres == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "ITextDocument_Open should fail Codepage:CP_UTF8\n");
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);
  hres = ITextDocument_Open(txtDoc, &testfile, tomText, CP_ACP);
todo_wine {
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(is_existing_file(filename) == TRUE, "a file should be created default\n");
}
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  /* test of share mode */
  touch_file(filename);
  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomShareDenyRead, CP_ACP);
  todo_wine
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  SetLastError(0xdeadbeef);
  hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, NULL);
  todo_wine ok(GetLastError() == ERROR_SHARING_VIOLATION, "ITextDocument_Open should fail\n");
  CloseHandle(hFile);
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  touch_file(filename);
  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomShareDenyWrite, CP_ACP);
  todo_wine
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  SetLastError(0xdeadbeef);
  hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, NULL);
  todo_wine ok(GetLastError() == ERROR_SHARING_VIOLATION, "ITextDocument_Open should fail\n");
  CloseHandle(hFile);
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  touch_file(filename);
  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  SetLastError(0xdeadbeef);
  hres = ITextDocument_Open(txtDoc, &testfile, tomShareDenyWrite|tomShareDenyRead, CP_ACP);
  todo_wine
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, NULL);
  todo_wine ok(GetLastError() == ERROR_SHARING_VIOLATION, "ITextDocument_Open should fail\n");
  CloseHandle(hFile);
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  /* tests to check the content */
  hFile = CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL, NULL);
  WriteFile(hFile, chACP, sizeof(chACP)-sizeof(CHAR), &dw, NULL);
  CloseHandle(hFile);
  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomReadOnly, CP_ACP);
  todo_wine
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  result = SendMessageA(w, WM_GETTEXT, 1024, (LPARAM)bufACP);
  todo_wine ok(result == 12, "ITextDocument_Open: Test ASCII returned %d, expected 12\n", result);
  result = strcmp(bufACP, chACP);
  todo_wine ok(result == 0, "ITextDocument_Open: Test ASCII set wrong text: Result: %s\n", bufACP);
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  hFile = CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL, NULL);
  WriteFile(hFile, chUTF8, sizeof(chUTF8)-sizeof(CHAR), &dw, NULL);
  CloseHandle(hFile);
  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomReadOnly, CP_UTF8);
  todo_wine
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  result = SendMessageA(w, WM_GETTEXT, 1024, (LPARAM)bufACP);
  todo_wine ok(result == 15, "ITextDocument_Open: Test UTF-8 returned %d, expected 15\n", result);
  result = strcmp(bufACP, &chUTF8[3]);
  todo_wine ok(result == 0, "ITextDocument_Open: Test UTF-8 set wrong text: Result: %s\n", bufACP);
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  hFile = CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL, NULL);
  WriteFile(hFile, chUTF16, sizeof(chUTF16)-sizeof(WCHAR), &dw, NULL);
  CloseHandle(hFile);
  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  hres = ITextDocument_Open(txtDoc, &testfile, tomReadOnly, 1200);
  todo_wine
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  result = SendMessageW(w, WM_GETTEXT, 1024, (LPARAM)bufUnicode);
  todo_wine ok(result == 12, "ITextDocument_Open: Test UTF-16 returned %d, expected 12\n", result);
  result = lstrcmpW(bufUnicode, &chUTF16[1]);
  todo_wine ok(result == 0, "ITextDocument_Open: Test UTF-16 set wrong text: Result: %s\n", wine_dbgstr_w(bufUnicode));
  release_interfaces(&w, &reOle, &txtDoc, &txtSel);
  DeleteFileW(filename);

  VariantClear(&testfile);
}

static void test_GetText(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextSelection *txtSel = NULL;
  HRESULT hres;
  BSTR bstr = NULL;
  int first, lim;
  static const CHAR test_text1[] = "TestSomeText";
  static const WCHAR bufW1[] = {'T', 'e', 's', 't', 0};
  static const WCHAR bufW2[] = {'T', 'e', 'x', 't', '\r', 0};
  static const WCHAR bufW3[] = {'T', 'e', 'x', 't', 0};
  static const WCHAR bufW4[] = {'T', 'e', 's', 't', 'S', 'o', 'm',
                                'e', 'T', 'e', 'x', 't', '\r', 0};
  static const WCHAR bufW5[] = {'\r', 0};
  static const WCHAR bufW6[] = {'T','e','s','t','S','o','m','e','T',0};
  BOOL is64bit = sizeof(void *) > sizeof(int);
  ITextRange *range;

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);

  /* ITextSelection */
  first = 0; lim = 4;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_GetText(txtSel, &bstr);
  ok(hres == S_OK, "ITextSelection_GetText\n");
  ok(!lstrcmpW(bstr, bufW1), "got wrong text: %s\n", wine_dbgstr_w(bstr));
  SysFreeString(bstr);

  first = 4; lim = 0;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_GetText(txtSel, &bstr);
  ok(hres == S_OK, "ITextSelection_GetText\n");
  ok(!lstrcmpW(bstr, bufW1), "got wrong text: %s\n", wine_dbgstr_w(bstr));
  SysFreeString(bstr);

  first = 1; lim = 1;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_GetText(txtSel, &bstr);
  ok(hres == S_OK, "ITextSelection_GetText\n");
  ok(!bstr, "got wrong text: %s\n", wine_dbgstr_w(bstr));

  if (!is64bit)
    {
      hres = ITextSelection_GetText(txtSel, NULL);
      ok(hres == E_INVALIDARG, "ITextSelection_GetText\n");
    }

  first = 8; lim = 12;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_GetText(txtSel, &bstr);
  ok(hres == S_OK, "ITextSelection_GetText\n");
  ok(!lstrcmpW(bstr, bufW3), "got wrong text: %s\n", wine_dbgstr_w(bstr));
  SysFreeString(bstr);

  first = 8; lim = 13;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_GetText(txtSel, &bstr);
  ok(hres == S_OK, "ITextSelection_GetText\n");
  ok(!lstrcmpW(bstr, bufW2), "got wrong text: %s\n", wine_dbgstr_w(bstr));
  SysFreeString(bstr);

  first = 12; lim = 13;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_GetText(txtSel, &bstr);
  ok(hres == S_OK, "ITextSelection_GetText\n");
  ok(!lstrcmpW(bstr, bufW5), "got wrong text: %s\n", wine_dbgstr_w(bstr));
  SysFreeString(bstr);

  first = 0; lim = -1;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_GetText(txtSel, &bstr);
  ok(hres == S_OK, "ITextSelection_GetText\n");
  ok(!lstrcmpW(bstr, bufW4), "got wrong text: %s\n", wine_dbgstr_w(bstr));
  SysFreeString(bstr);

  first = -1; lim = 9;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_GetText(txtSel, &bstr);
  ok(hres == S_OK, "ITextSelection_GetText\n");
  ok(!bstr, "got wrong text: %s\n", wine_dbgstr_w(bstr));

  /* ITextRange */
  hres = ITextDocument_Range(txtDoc, 0, 4, &range);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = ITextRange_GetText(range, &bstr);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(!lstrcmpW(bstr, bufW1), "got wrong text: %s\n", wine_dbgstr_w(bstr));

  SysFreeString(bstr);
  ITextRange_Release(range);

  hres = ITextDocument_Range(txtDoc, 4, 0, &range);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = ITextRange_GetText(range, &bstr);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(!lstrcmpW(bstr, bufW1), "got wrong text: %s\n", wine_dbgstr_w(bstr));

  SysFreeString(bstr);
  ITextRange_Release(range);

  hres = ITextDocument_Range(txtDoc, 1, 1, &range);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = ITextRange_GetText(range, &bstr);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(!bstr, "got wrong text: %s\n", wine_dbgstr_w(bstr));
  if (!is64bit)
  {
    hres = ITextRange_GetText(range, NULL);
    ok(hres == E_INVALIDARG, "got 0x%08lx\n", hres);
  }
  ITextRange_Release(range);

  hres = ITextDocument_Range(txtDoc, 8, 12, &range);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = ITextRange_GetText(range, &bstr);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(!lstrcmpW(bstr, bufW3), "got wrong text: %s\n", wine_dbgstr_w(bstr));

  SysFreeString(bstr);
  ITextRange_Release(range);

  hres = ITextDocument_Range(txtDoc, 8, 13, &range);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = ITextRange_GetText(range, &bstr);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(!lstrcmpW(bstr, bufW2), "got wrong text: %s\n", wine_dbgstr_w(bstr));

  SysFreeString(bstr);
  ITextRange_Release(range);

  hres = ITextDocument_Range(txtDoc, 12, 13, &range);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = ITextRange_GetText(range, &bstr);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(!lstrcmpW(bstr, bufW5), "got wrong text: %s\n", wine_dbgstr_w(bstr));

  SysFreeString(bstr);
  ITextRange_Release(range);

  hres = ITextDocument_Range(txtDoc, 0, -1, &range);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = ITextRange_GetText(range, &bstr);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(!bstr, "got wrong text: %s\n", wine_dbgstr_w(bstr));
  ITextRange_Release(range);

  hres = ITextDocument_Range(txtDoc, -1, 9, &range);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = ITextRange_GetText(range, &bstr);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(!lstrcmpW(bstr, bufW6), "got wrong text: %s\n", wine_dbgstr_w(bstr));

  SysFreeString(bstr);

  release_interfaces(&w, &reOle, &txtDoc, NULL);

  /* detached selection/range */
  if (is64bit) {
    bstr = (void*)0xdeadbeef;
    hres = ITextSelection_GetText(txtSel, &bstr);
    ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);
    todo_wine
    ok(bstr == NULL, "got %p\n", bstr);

    bstr = (void*)0xdeadbeef;
    hres = ITextRange_GetText(range, &bstr);
    ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);
    todo_wine
    ok(bstr == NULL, "got %p\n", bstr);
  }
  else {
    hres = ITextSelection_GetText(txtSel, NULL);
    ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);

    hres = ITextRange_GetText(range, NULL);
    ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);
  }

  ITextRange_Release(range);
  ITextSelection_Release(txtSel);
}

static void test_ITextDocument_Range(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextRange *txtRge, *range2;
  HRESULT hres;
  LONG value;

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  hres = ITextDocument_Range(txtDoc, 0, 0, &txtRge);
  ok(hres == S_OK, "ITextDocument_Range fails 0x%lx.\n", hres);
  EXPECT_REF(txtRge, 1);

  hres = ITextDocument_Range(txtDoc, 0, 0, &range2);
  ok(hres == S_OK, "ITextDocument_Range fails 0x%lx.\n", hres);
  ok(range2 != txtRge, "A new pointer should be returned\n");
  ITextRange_Release(range2);

  hres = ITextDocument_Range(txtDoc, 0, 0, NULL);
  ok(hres == E_INVALIDARG, "ITextDocument_Range should fail 0x%lx.\n", hres);

  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);

  hres = ITextDocument_Range(txtDoc, 8, 30, &range2);
  ok(hres == S_OK, "ITextDocument_Range fails 0x%lx.\n", hres);
  hres = ITextRange_GetStart(range2, &value);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(value == 8, "got %ld\n", value);

  hres = ITextRange_GetEnd(range2, &value);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(value == 13, "got %ld\n", value);
  ITextRange_Release(range2);

  release_interfaces(&w, &reOle, &txtDoc, NULL);
  hres = ITextRange_CanEdit(txtRge, NULL);
  ok(hres == CO_E_RELEASED, "ITextRange after ITextDocument destroyed\n");
  ITextRange_Release(txtRge);
}

static void test_ITextRange_GetChar(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextRange *txtRge = NULL;
  HRESULT hres;
  LONG pch;
  int first, lim;
  static const CHAR test_text1[] = "TestSomeText";

  first = 0, lim = 4;
  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  pch = 0xdeadbeef;
  hres = ITextRange_GetChar(txtRge, &pch);
  ok(hres == S_OK, "ITextRange_GetChar\n");
  ok(pch == 'T', "got wrong char: %c\n", (char)pch);
  ITextRange_Release(txtRge);
  release_interfaces(&w, &reOle, &txtDoc, NULL);

  first = 0; lim = 0;
  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  pch = 0xdeadbeef;
  hres = ITextRange_GetChar(txtRge, &pch);
  ok(hres == S_OK, "ITextRange_GetChar\n");
  ok(pch == 'T', "got wrong char: %c\n", (char)pch);
  ITextRange_Release(txtRge);
  release_interfaces(&w, &reOle, &txtDoc, NULL);

  first = 12; lim = 12;
  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  pch = 0xdeadbeef;
  hres = ITextRange_GetChar(txtRge, &pch);
  ok(hres == S_OK, "ITextRange_GetChar\n");
  ok(pch == '\r', "got wrong char: %c\n", (char)pch);
  ITextRange_Release(txtRge);
  release_interfaces(&w, &reOle, &txtDoc, NULL);

  first = 13; lim = 13;
  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  pch = 0xdeadbeef;
  hres = ITextRange_GetChar(txtRge, &pch);
  ok(hres == S_OK, "ITextRange_GetChar\n");
  ok(pch == '\r', "got wrong char: %c\n", (char)pch);
  ITextRange_Release(txtRge);
  release_interfaces(&w, &reOle, &txtDoc, NULL);

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);
  first = 12; lim = 12;
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = ITextRange_GetChar(txtRge, NULL);
  ok(hres == E_INVALIDARG, "ITextRange_GetChar\n");

  release_interfaces(&w, &reOle, &txtDoc, NULL);

  hres = ITextRange_GetChar(txtRge, NULL);
  ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);

  hres = ITextRange_GetChar(txtRge, &pch);
  ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);

  ITextRange_Release(txtRge);
}

/* Helper function for testing ITextRange_ScrollIntoView */
static void check_range(HWND w, ITextDocument* doc, int first, int lim,
                        LONG bStart, int expected_nonzero)
{
  SCROLLINFO si;
  ITextRange *txtRge = NULL;
  HRESULT hres;

  si.cbSize = sizeof(SCROLLINFO);
  si.fMask = SIF_POS | SIF_RANGE;

  hres = ITextDocument_Range(doc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = ITextRange_ScrollIntoView(txtRge, bStart);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  GetScrollInfo(w, SB_VERT, &si);
  if (expected_nonzero) {
    ok(si.nPos != 0,
       "Scrollbar at 0, should be >0. (TextRange %d-%d, scroll range %d-%d.)\n",
       first, lim, si.nMin, si.nMax);
  } else {
    ok(si.nPos == 0,
       "Scrollbar at %d, should be 0. (TextRange %d-%d, scroll range %d-%d.)\n",
       si.nPos, first, lim, si.nMin, si.nMax);
  }
}

static void test_ITextRange_ScrollIntoView(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextRange *txtRge = NULL;
  HRESULT hres;
  static const CHAR test_text1[] = "1\n2\n3\n4\n5\n6\n7\n8\n9\n10";

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);

  /* Scroll to the top. */
  check_range(w, txtDoc, 0, 1, tomStart, 0);
  check_range(w, txtDoc, 0, 1, tomEnd, 0);

  /* Scroll to the bottom. */
  check_range(w, txtDoc, 19, 20, tomStart, 1);
  check_range(w, txtDoc, 19, 20, tomEnd, 1);

  /* Back up to the top. */
  check_range(w, txtDoc, 0, 1, tomStart, 0);
  check_range(w, txtDoc, 0, 1, tomEnd, 0);

  /* Large range */
  check_range(w, txtDoc, 0, 20, tomStart, 0);
  check_range(w, txtDoc, 0, 20, tomEnd, 1);

  hres = ITextDocument_Range(txtDoc, 0, 0, &txtRge);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  release_interfaces(&w, &reOle, &txtDoc, NULL);
  hres = ITextRange_ScrollIntoView(txtRge, tomStart);
  ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);
  ITextRange_Release(txtRge);
}

static void test_ITextSelection_GetChar(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextSelection *txtSel = NULL;
  HRESULT hres;
  LONG pch;
  int first, lim;
  static const CHAR test_text1[] = "TestSomeText";

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);

  first = 0; lim = 4;
  SendMessageA(w, EM_SETSEL, first, lim);
  pch = 0xdeadbeef;
  hres = ITextSelection_GetChar(txtSel, &pch);
  ok(hres == S_OK, "ITextSelection_GetChar\n");
  ok(pch == 'T', "got wrong char: %c\n", (char)pch);

  first = 0; lim = 0;
  SendMessageA(w, EM_SETSEL, first, lim);
  pch = 0xdeadbeef;
  hres = ITextSelection_GetChar(txtSel, &pch);
  ok(hres == S_OK, "ITextSelection_GetChar\n");
  ok(pch == 'T', "got wrong char: %c\n", (char)pch);

  first = 12; lim = 12;
  SendMessageA(w, EM_SETSEL, first, lim);
  pch = 0xdeadbeef;
  hres = ITextSelection_GetChar(txtSel, &pch);
  ok(hres == S_OK, "ITextSelection_GetChar\n");
  ok(pch == '\r', "got wrong char: %c\n", (char)pch);

  first = 13; lim = 13;
  SendMessageA(w, EM_SETSEL, first, lim);
  pch = 0xdeadbeef;
  hres = ITextSelection_GetChar(txtSel, &pch);
  ok(hres == S_OK, "ITextSelection_GetChar\n");
  ok(pch == '\r', "got wrong char: %c\n", (char)pch);

  hres = ITextSelection_GetChar(txtSel, NULL);
  ok(hres == E_INVALIDARG, "ITextSelection_GetChar\n");

  release_interfaces(&w, &reOle, &txtDoc, NULL);

  hres = ITextSelection_GetChar(txtSel, NULL);
  ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);

  hres = ITextSelection_GetChar(txtSel, &pch);
  ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);

  ITextSelection_Release(txtSel);
}

static void test_ITextRange_GetStart_GetEnd(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextRange *txtRge = NULL;
  HRESULT hres;
  int first, lim;
  LONG start, end;
  static const CHAR test_text1[] = "TestSomeText";

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);

  first = 1; lim = 6;
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  start = 0xdeadbeef;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "ITextRange_GetStart\n");
  ok(start == 1, "got wrong start value: %ld\n", start);
  end = 0xdeadbeef;
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "ITextRange_GetEnd\n");
  ok(end == 6, "got wrong end value: %ld\n", end);
  ITextRange_Release(txtRge);

  first = 6; lim = 1;
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  start = 0xdeadbeef;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "ITextRange_GetStart\n");
  ok(start == 1, "got wrong start value: %ld\n", start);
  end = 0xdeadbeef;
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "ITextRange_GetEnd\n");
  ok(end == 6, "got wrong end value: %ld\n", end);
  ITextRange_Release(txtRge);

  first = -1; lim = 13;
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  start = 0xdeadbeef;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "ITextRange_GetStart\n");
  ok(start == 0, "got wrong start value: %ld\n", start);
  end = 0xdeadbeef;
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "ITextRange_GetEnd\n");
  ok(end == 13, "got wrong end value: %ld\n", end);
  ITextRange_Release(txtRge);

  first = 13; lim = 13;
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  start = 0xdeadbeef;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "ITextRange_GetStart\n");
  ok(start == 12, "got wrong start value: %ld\n", start);
  end = 0xdeadbeef;
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "ITextRange_GetEnd\n");
  ok(end == 12, "got wrong end value: %ld\n", end);

  /* SetStart */
  hres = ITextRange_SetStart(txtRge, 0);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  /* same value */
  hres = ITextRange_SetStart(txtRge, 0);
  ok(hres == S_FALSE, "got 0x%08lx\n", hres);

  hres = ITextRange_SetStart(txtRge, 1);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  /* negative resets to 0, return value is S_FALSE when
     position wasn't changed */
  hres = ITextRange_SetStart(txtRge, -1);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  hres = ITextRange_SetStart(txtRge, -1);
  ok(hres == S_FALSE, "got 0x%08lx\n", hres);

  hres = ITextRange_SetStart(txtRge, 0);
  ok(hres == S_FALSE, "got 0x%08lx\n", hres);

  start = -1;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == 0, "got %ld\n", start);

  /* greater than initial end, but less than total char count */
  hres = ITextRange_SetStart(txtRge, 1);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  hres = ITextRange_SetEnd(txtRge, 3);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  hres = ITextRange_SetStart(txtRge, 10);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  start = 0;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == 10, "got %ld\n", start);

  end = 0;
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(end == 10, "got %ld\n", end);

  /* more that total text length */
  hres = ITextRange_SetStart(txtRge, 50);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  start = 0;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == 12, "got %ld\n", start);

  end = 0;
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(end == 12, "got %ld\n", end);

  /* SetEnd */
  hres = ITextRange_SetStart(txtRge, 0);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  /* same value */
  hres = ITextRange_SetEnd(txtRge, 5);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  hres = ITextRange_SetEnd(txtRge, 5);
  ok(hres == S_FALSE, "got 0x%08lx\n", hres);

  /* negative resets to 0 */
  hres = ITextRange_SetEnd(txtRge, -1);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  end = -1;
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(end == 0, "got %ld\n", end);

  start = -1;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == 0, "got %ld\n", start);

  /* greater than initial end, but less than total char count */
  hres = ITextRange_SetStart(txtRge, 3);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  hres = ITextRange_SetEnd(txtRge, 1);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  start = 0;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == 1, "got %ld\n", start);

  end = 0;
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(end == 1, "got %ld\n", end);

  /* more than total count */
  hres = ITextRange_SetEnd(txtRge, 50);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  start = 0;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == 1, "got %ld\n", start);

  end = 0;
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(end == 13, "got %ld\n", end);

  /* zero */
  hres = ITextRange_SetEnd(txtRge, 0);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  start = 0;
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == 0, "got %ld\n", start);

  end = 0;
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(end == 0, "got %ld\n", end);

  release_interfaces(&w, &reOle, &txtDoc, NULL);

  /* detached range */
  hres = ITextRange_SetStart(txtRge, 0);
  ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);

  hres = ITextRange_SetEnd(txtRge, 3);
  ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);

  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);

  hres = ITextRange_GetStart(txtRge, NULL);
  ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);

  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);

  hres = ITextRange_GetEnd(txtRge, NULL);
  ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);

  ITextRange_Release(txtRge);
}

static void test_ITextSelection_GetStart_GetEnd(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextSelection *txtSel = NULL;
  HRESULT hres;
  int first, lim;
  LONG start, end;
  static const CHAR test_text1[] = "TestSomeText";

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);

  first = 2; lim = 5;
  SendMessageA(w, EM_SETSEL, first, lim);
  start = 0xdeadbeef;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "ITextSelection_GetStart\n");
  ok(start == 2, "got wrong start value: %ld\n", start);
  end = 0xdeadbeef;
  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == S_OK, "ITextSelection_GetEnd\n");
  ok(end == 5, "got wrong end value: %ld\n", end);

  first = 5; lim = 2;
  SendMessageA(w, EM_SETSEL, first, lim);
  start = 0xdeadbeef;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "ITextSelection_GetStart\n");
  ok(start == 2, "got wrong start value: %ld\n", start);
  end = 0xdeadbeef;
  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == S_OK, "ITextSelection_GetEnd\n");
  ok(end == 5, "got wrong end value: %ld\n", end);

  first = 0; lim = -1;
  SendMessageA(w, EM_SETSEL, first, lim);
  start = 0xdeadbeef;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "ITextSelection_GetStart\n");
  ok(start == 0, "got wrong start value: %ld\n", start);
  end = 0xdeadbeef;
  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == S_OK, "ITextSelection_GetEnd\n");
  ok(end == 13, "got wrong end value: %ld\n", end);

  first = 13; lim = 13;
  SendMessageA(w, EM_SETSEL, first, lim);
  start = 0xdeadbeef;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "ITextSelection_GetStart\n");
  ok(start == 12, "got wrong start value: %ld\n", start);
  end = 0xdeadbeef;
  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == S_OK, "ITextSelection_GetEnd\n");
  ok(end == 12, "got wrong end value: %ld\n", end);

  /* SetStart/SetEnd */
  hres = ITextSelection_SetStart(txtSel, 0);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  /* same value */
  hres = ITextSelection_SetStart(txtSel, 0);
  ok(hres == S_FALSE, "got 0x%08lx\n", hres);

  hres = ITextSelection_SetStart(txtSel, 1);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  /* negative resets to 0, return value is S_FALSE when
     position wasn't changed */
  hres = ITextSelection_SetStart(txtSel, -1);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  hres = ITextSelection_SetStart(txtSel, -1);
  ok(hres == S_FALSE, "got 0x%08lx\n", hres);

  hres = ITextSelection_SetStart(txtSel, 0);
  ok(hres == S_FALSE, "got 0x%08lx\n", hres);

  start = -1;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == 0, "got %ld\n", start);

  /* greater than initial end, but less than total char count */
  hres = ITextSelection_SetStart(txtSel, 1);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  hres = ITextSelection_SetEnd(txtSel, 3);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  hres = ITextSelection_SetStart(txtSel, 10);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  start = 0;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == 10, "got %ld\n", start);

  end = 0;
  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(end == 10, "got %ld\n", end);

  /* more that total text length */
  hres = ITextSelection_SetStart(txtSel, 50);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  start = 0;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == 12, "got %ld\n", start);

  end = 0;
  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(end == 12, "got %ld\n", end);

  /* SetEnd */
  hres = ITextSelection_SetStart(txtSel, 0);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  /* same value */
  hres = ITextSelection_SetEnd(txtSel, 5);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  hres = ITextSelection_SetEnd(txtSel, 5);
  ok(hres == S_FALSE, "got 0x%08lx\n", hres);

  /* negative resets to 0 */
  hres = ITextSelection_SetEnd(txtSel, -1);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  end = -1;
  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(end == 0, "got %ld\n", end);

  start = -1;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == 0, "got %ld\n", start);

  /* greater than initial end, but less than total char count */
  hres = ITextSelection_SetStart(txtSel, 3);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  hres = ITextSelection_SetEnd(txtSel, 1);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  start = 0;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == 1, "got %ld\n", start);

  end = 0;
  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(end == 1, "got %ld\n", end);

  /* more than total count */
  hres = ITextSelection_SetEnd(txtSel, 50);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  start = 0;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == 1, "got %ld\n", start);

  end = 0;
  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(end == 13, "got %ld\n", end);

  /* zero */
  hres = ITextSelection_SetEnd(txtSel, 0);
  ok(hres == S_OK, "got 0x%08lx\n", hres);

  start = 0;
  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == 0, "got %ld\n", start);

  end = 0;
  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(end == 0, "got %ld\n", end);

  release_interfaces(&w, &reOle, &txtDoc, NULL);

  /* detached selection */
  hres = ITextSelection_GetStart(txtSel, NULL);
  ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);

  hres = ITextSelection_GetStart(txtSel, &start);
  ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);

  hres = ITextSelection_GetEnd(txtSel, NULL);
  ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);

  hres = ITextSelection_GetEnd(txtSel, &end);
  ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);

  ITextSelection_Release(txtSel);
}

static void test_ITextRange_GetDuplicate(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextRange *txtRge = NULL;
  ITextRange *txtRgeDup = NULL;
  HRESULT hres;
  LONG first, lim, start, end;
  static const CHAR test_text1[] = "TestSomeText";

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);
  first = 0; lim = 4;
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "ITextDocument_Range fails 0x%lx.\n", hres);

  hres = ITextRange_GetDuplicate(txtRge, &txtRgeDup);
  ok(hres == S_OK, "ITextRange_GetDuplicate\n");
  ok(txtRgeDup != txtRge, "A new pointer should be returned\n");
  hres = ITextRange_GetStart(txtRgeDup, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == first, "got wrong value: %ld\n", start);
  hres = ITextRange_GetEnd(txtRgeDup, &end);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(end == lim, "got wrong value: %ld\n", end);

  ITextRange_Release(txtRgeDup);

  hres = ITextRange_GetDuplicate(txtRge, NULL);
  ok(hres == E_INVALIDARG, "ITextRange_GetDuplicate\n");

  release_interfaces(&w, &reOle, &txtDoc, NULL);

  hres = ITextRange_GetDuplicate(txtRge, NULL);
  ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);

  hres = ITextRange_GetDuplicate(txtRge, &txtRgeDup);
  ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);

  ITextRange_Release(txtRge);
}

static void test_ITextRange_Collapse(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextRange *txtRge = NULL;
  HRESULT hres;
  LONG first, lim, start, end;
  static const CHAR test_text1[] = "TestSomeText";

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);

  first = 4; lim = 8;
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = ITextRange_Collapse(txtRge, tomTrue);
  ok(hres == S_OK, "ITextRange_Collapse\n");
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == 4, "got wrong start value: %ld\n", start);
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(end == 4, "got wrong end value: %ld\n", end);
  ITextRange_Release(txtRge);

  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = ITextRange_Collapse(txtRge, tomStart);
  ok(hres == S_OK, "ITextRange_Collapse\n");
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == 4, "got wrong start value: %ld\n", start);
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(end == 4, "got wrong end value: %ld\n", end);
  ITextRange_Release(txtRge);

  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = ITextRange_Collapse(txtRge, tomFalse);
  ok(hres == S_OK, "ITextRange_Collapse\n");
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == 8, "got wrong start value: %ld\n", start);
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(end == 8, "got wrong end value: %ld\n", end);
  ITextRange_Release(txtRge);

  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = ITextRange_Collapse(txtRge, tomEnd);
  ok(hres == S_OK, "ITextRange_Collapse\n");
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == 8, "got wrong start value: %ld\n", start);
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(end == 8, "got wrong end value: %ld\n", end);
  ITextRange_Release(txtRge);

  /* tomStart is the default */
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = ITextRange_Collapse(txtRge, 256);
  ok(hres == S_OK, "ITextRange_Collapse\n");
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == 4, "got wrong start value: %ld\n", start);
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(end == 4, "got wrong end value: %ld\n", end);
  ITextRange_Release(txtRge);

  first = 6; lim = 6;
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = ITextRange_Collapse(txtRge, tomEnd);
  ok(hres == S_FALSE, "ITextRange_Collapse\n");
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == 6, "got wrong start value: %ld\n", start);
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(end == 6, "got wrong end value: %ld\n", end);
  ITextRange_Release(txtRge);

  first = 8; lim = 8;
  hres = ITextDocument_Range(txtDoc, first, lim, &txtRge);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  hres = ITextRange_Collapse(txtRge, tomStart);
  ok(hres == S_FALSE, "ITextRange_Collapse\n");
  hres = ITextRange_GetStart(txtRge, &start);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(start == 8, "got wrong start value: %ld\n", start);
  hres = ITextRange_GetEnd(txtRge, &end);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(end == 8, "got wrong end value: %ld\n", end);

  release_interfaces(&w, &reOle, &txtDoc, NULL);

  hres = ITextRange_Collapse(txtRge, tomStart);
  ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);

  hres = ITextRange_Collapse(txtRge, tomUndefined);
  ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);

  ITextRange_Release(txtRge);
}

static void test_ITextSelection_Collapse(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  ITextSelection *txtSel = NULL;
  HRESULT hres;
  LONG first, lim, start, end;
  static const CHAR test_text1[] = "TestSomeText";

  create_interfaces(&w, &reOle, &txtDoc, &txtSel);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);

  first = 4; lim = 8;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_Collapse(txtSel, tomTrue);
  ok(hres == S_OK, "ITextSelection_Collapse\n");
  SendMessageA(w, EM_GETSEL, (LPARAM)&start, (WPARAM)&end);
  ok(start == 4, "got wrong start value: %ld\n", start);
  ok(end == 4, "got wrong end value: %ld\n", end);

  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_Collapse(txtSel, tomStart);
  ok(hres == S_OK, "ITextSelection_Collapse\n");
  SendMessageA(w, EM_GETSEL, (LPARAM)&start, (WPARAM)&end);
  ok(start == 4, "got wrong start value: %ld\n", start);
  ok(end == 4, "got wrong end value: %ld\n", end);

  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_Collapse(txtSel, tomFalse);
  ok(hres == S_OK, "ITextSelection_Collapse\n");
  SendMessageA(w, EM_GETSEL, (LPARAM)&start, (WPARAM)&end);
  ok(start == 8, "got wrong start value: %ld\n", start);
  ok(end == 8, "got wrong end value: %ld\n", end);

  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_Collapse(txtSel, tomEnd);
  ok(hres == S_OK, "ITextSelection_Collapse\n");
  SendMessageA(w, EM_GETSEL, (LPARAM)&start, (WPARAM)&end);
  ok(start == 8, "got wrong start value: %ld\n", start);
  ok(end == 8, "got wrong end value: %ld\n", end);

  /* tomStart is the default */
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_Collapse(txtSel, 256);
  ok(hres == S_OK, "ITextSelection_Collapse\n");
  SendMessageA(w, EM_GETSEL, (LPARAM)&start, (WPARAM)&end);
  ok(start == 4, "got wrong start value: %ld\n", start);
  ok(end == 4, "got wrong end value: %ld\n", end);

  first = 6; lim = 6;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_Collapse(txtSel, tomEnd);
  ok(hres == S_FALSE, "ITextSelection_Collapse\n");
  SendMessageA(w, EM_GETSEL, (LPARAM)&start, (WPARAM)&end);
  ok(start == 6, "got wrong start value: %ld\n", start);
  ok(end == 6, "got wrong end value: %ld\n", end);

  first = 8; lim = 8;
  SendMessageA(w, EM_SETSEL, first, lim);
  hres = ITextSelection_Collapse(txtSel, tomStart);
  ok(hres == S_FALSE, "ITextSelection_Collapse\n");
  SendMessageA(w, EM_GETSEL, (LPARAM)&start, (WPARAM)&end);
  ok(start == 8, "got wrong start value: %ld\n", start);
  ok(end == 8, "got wrong end value: %ld\n", end);

  release_interfaces(&w, &reOle, &txtDoc, NULL);

  hres = ITextSelection_Collapse(txtSel, tomStart);
  ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);

  hres = ITextSelection_Collapse(txtSel, tomUndefined);
  ok(hres == CO_E_RELEASED, "got 0x%08lx\n", hres);

  ITextSelection_Release(txtSel);
}

static void test_GetClientSite(void)
{
  HWND w;
  IRichEditOle *reOle = NULL, *reOle1 = NULL;
  ITextDocument *txtDoc = NULL;
  IOleClientSite *clientSite = NULL, *clientSite1 = NULL, *clientSite2 = NULL;
  IOleWindow *oleWin = NULL, *oleWin1 = NULL;
  IOleInPlaceSite *olePlace = NULL, *olePlace1 = NULL;
  HRESULT hres;
  LONG refcount1, refcount2;

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  hres = IRichEditOle_GetClientSite(reOle, &clientSite);
  ok(hres == S_OK, "IRichEditOle_QueryInterface: 0x%08lx\n", hres);
  EXPECT_REF(clientSite, 1);

  hres = IOleClientSite_QueryInterface(clientSite, &IID_IRichEditOle, (void **)&reOle1);
  ok(hres == E_NOINTERFACE, "IOleClientSite_QueryInterface: %lx\n", hres);

  hres = IRichEditOle_GetClientSite(reOle, &clientSite1);
  ok(hres == S_OK, "got 0x%08lx\n", hres);
  ok(clientSite != clientSite1, "got %p, %p\n", clientSite, clientSite1);
  IOleClientSite_Release(clientSite1);

  hres = IOleClientSite_QueryInterface(clientSite, &IID_IOleClientSite, (void **)&clientSite1);
  ok(hres == S_OK, "IOleClientSite_QueryInterface: 0x%08lx\n", hres);
  ok(clientSite == clientSite1, "Should not return a new pointer.\n");
  EXPECT_REF(clientSite, 2);

  /* IOleWindow interface */
  hres = IOleClientSite_QueryInterface(clientSite, &IID_IOleWindow, (void **)&oleWin);
  ok(hres == S_OK, "IOleClientSite_QueryInterface: 0x%08lx\n", hres);
  refcount1 = get_refcount((IUnknown *)clientSite);
  refcount2 = get_refcount((IUnknown *)oleWin);
  ok(refcount1 == refcount2, "got wrong ref count.\n");

  hres = IOleClientSite_QueryInterface(clientSite, &IID_IOleWindow, (void **)&oleWin1);
  ok(hres == S_OK, "IOleClientSite_QueryInterface: 0x%08lx\n", hres);
  ok(oleWin == oleWin1, "Should not return a new pointer.\n");
  refcount1 = get_refcount((IUnknown *)clientSite);
  refcount2 = get_refcount((IUnknown *)oleWin);
  ok(refcount1 == refcount2, "got wrong ref count.\n");

  hres = IOleWindow_QueryInterface(oleWin, &IID_IOleClientSite, (void **)&clientSite2);
  ok(hres == S_OK, "IOleWindow_QueryInterface: 0x%08lx\n", hres);
  ok(clientSite2 == clientSite1, "got wrong pointer\n");

  /* IOleInPlaceSite interface */
  hres = IOleClientSite_QueryInterface(clientSite, &IID_IOleInPlaceSite, (void **)&olePlace);
  ok(hres == S_OK, "IOleClientSite_QueryInterface: 0x%08lx\n", hres);
  refcount1 = get_refcount((IUnknown *)olePlace);
  refcount2 = get_refcount((IUnknown *)clientSite);
  ok(refcount1 == refcount2, "got wrong ref count.\n");

  hres = IOleClientSite_QueryInterface(clientSite, &IID_IOleInPlaceSite, (void **)&olePlace1);
  ok(hres == S_OK, "IOleClientSite_QueryInterface: 0x%08lx\n", hres);
  ok(olePlace == olePlace1, "Should not return a new pointer.\n");
  IOleInPlaceSite_Release(olePlace1);

  hres = IOleWindow_QueryInterface(oleWin, &IID_IOleInPlaceSite, (void **)&olePlace1);
  ok(hres == S_OK, "IOleWindow_QueryInterface: 0x%08lx\n", hres);
  refcount1 = get_refcount((IUnknown *)olePlace1);
  refcount2 = get_refcount((IUnknown *)oleWin);
  ok(refcount1 == refcount2, "got wrong ref count.\n");

  IOleInPlaceSite_Release(olePlace1);
  IOleInPlaceSite_Release(olePlace);
  IOleWindow_Release(oleWin1);
  IOleWindow_Release(oleWin);
  IOleClientSite_Release(clientSite2);
  IOleClientSite_Release(clientSite1);
  IOleClientSite_Release(clientSite);
  release_interfaces(&w, &reOle, &txtDoc, NULL);
}

static void test_IOleWindow_GetWindow(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  IOleClientSite *clientSite = NULL;
  IOleWindow *oleWin = NULL;
  HRESULT hres;
  HWND hwnd;

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  hres = IRichEditOle_GetClientSite(reOle, &clientSite);
  ok(hres == S_OK, "IRichEditOle_QueryInterface: 0x%08lx\n", hres);

  hres = IOleClientSite_QueryInterface(clientSite, &IID_IOleWindow, (void **)&oleWin);
  ok(hres == S_OK, "IOleClientSite_QueryInterface: 0x%08lx\n", hres);
  hres = IOleWindow_GetWindow(oleWin, &hwnd);
  ok(hres == S_OK, "IOleClientSite_GetWindow: 0x%08lx\n", hres);
  ok(w == hwnd, "got wrong pointer\n");

  hres = IOleWindow_GetWindow(oleWin, NULL);
  ok(hres == E_INVALIDARG, "IOleClientSite_GetWindow: 0x%08lx\n", hres);

  IOleWindow_Release(oleWin);
  IOleClientSite_Release(clientSite);
  release_interfaces(&w, &reOle, &txtDoc, NULL);
}

static void test_IOleInPlaceSite_GetWindow(void)
{
  HWND w;
  IRichEditOle *reOle = NULL;
  ITextDocument *txtDoc = NULL;
  IOleClientSite *clientSite = NULL;
  IOleInPlaceSite *olePlace = NULL;
  HRESULT hres;
  HWND hwnd;

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  hres = IRichEditOle_GetClientSite(reOle, &clientSite);
  ok(hres == S_OK, "IRichEditOle_QueryInterface: 0x%08lx\n", hres);

  hres = IOleClientSite_QueryInterface(clientSite, &IID_IOleInPlaceSite, (void **)&olePlace);
  ok(hres == S_OK, "IOleClientSite_QueryInterface: 0x%08lx\n", hres);
  hres = IOleInPlaceSite_GetWindow(olePlace, &hwnd);
  ok(hres == S_OK, "IOleInPlaceSite_GetWindow: 0x%08lx\n", hres);
  ok(w == hwnd, "got wrong pointer.\n");

  hres = IOleInPlaceSite_GetWindow(olePlace, NULL);
  ok(hres == E_INVALIDARG, "IOleInPlaceSite_GetWindow: 0x%08lx\n", hres);

  IOleInPlaceSite_Release(olePlace);
  IOleClientSite_Release(clientSite);
  release_interfaces(&w, &reOle, &txtDoc, NULL);
}

static void test_GetFont(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextRange *range = NULL;
  ITextSelection *selection;
  ITextFont *font, *font2;
  CHARFORMAT2A cf;
  LONG value;
  float size;
  HRESULT hr;
  HWND hwnd;
  BOOL ret;

  create_interfaces(&hwnd, &reOle, &doc, NULL);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);

  hr = ITextDocument_GetSelection(doc, &selection);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  hr = ITextSelection_GetFont(selection, &font);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  hr = ITextSelection_GetFont(selection, &font2);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(font != font2, "got %p, %p\n", font, font2);
  ITextFont_Release(font2);
  ITextFont_Release(font);
  ITextSelection_Release(selection);

  EXPECT_REF(reOle, 3);
  EXPECT_REF(doc, 3);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  EXPECT_REF(reOle, 3);
  EXPECT_REF(doc, 3);
  EXPECT_REF(range, 1);

  hr = ITextRange_GetFont(range, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  hr = ITextRange_GetFont(range, &font);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  EXPECT_REF(reOle, 3);
  EXPECT_REF(doc, 3);
  EXPECT_REF(range, 2);
  EXPECT_REF(font, 1);

  hr = ITextRange_GetFont(range, &font2);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(font != font2, "got %p, %p\n", font, font2);

  EXPECT_REF(reOle, 3);
  EXPECT_REF(doc, 3);
  EXPECT_REF(range, 3);
  EXPECT_REF(font, 1);
  EXPECT_REF(font2, 1);

  ITextFont_Release(font2);

  /* set different font style within a range */
  hr = ITextFont_GetItalic(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  hr = ITextFont_GetSize(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  size = 0.0;
  hr = ITextFont_GetSize(font, &size);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(size > 0.0, "size %.2f\n", size);

  value = 0;
  hr = ITextFont_GetLanguageID(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == GetSystemDefaultLCID(), "got lcid %lx, user lcid %lx\n", value,
      GetSystemDefaultLCID());

  /* range is non-italic */
  value = tomTrue;
  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  cf.cbSize = sizeof(CHARFORMAT2A);
  cf.dwMask = CFM_ITALIC|CFM_SIZE;
  cf.dwEffects = CFE_ITALIC;
  cf.yHeight = 24.0;

  SendMessageA(hwnd, EM_SETSEL, 2, 3);
  ret = SendMessageA(hwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
  ok(ret, "got %d\n", ret);

  /* now range is partially italicized */
  value = tomFalse;
  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUndefined, "got %ld\n", value);

  size = 0.0;
  hr = ITextFont_GetSize(font, &size);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(size == tomUndefined, "size %.2f\n", size);

  ITextFont_Release(font);
  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextRange_GetFont(range, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  hr = ITextRange_GetFont(range, &font2);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  ITextRange_Release(range);
}

static void test_GetPara(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextSelection *selection;
  ITextRange *range = NULL;
  ITextPara *para, *para2;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reOle, &doc, &selection);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);

  EXPECT_REF(reOle, 3);
  EXPECT_REF(doc, 3);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  EXPECT_REF(reOle, 3);
  EXPECT_REF(doc, 3);
  EXPECT_REF(range, 1);

  hr = ITextRange_GetPara(range, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  hr = ITextRange_GetPara(range, &para);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  EXPECT_REF(reOle, 3);
  EXPECT_REF(doc, 3);
  EXPECT_REF(range, 2);
  EXPECT_REF(para, 1);

  hr = ITextRange_GetPara(range, &para2);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(para != para2, "got %p, %p\n", para, para2);

  EXPECT_REF(reOle, 3);
  EXPECT_REF(doc, 3);
  EXPECT_REF(range, 3);
  EXPECT_REF(para, 1);
  EXPECT_REF(para2, 1);

  ITextPara_Release(para);
  ITextPara_Release(para2);

  EXPECT_REF(reOle, 3);
  EXPECT_REF(doc, 3);
  EXPECT_REF(selection, 2);

  hr = ITextSelection_GetPara(selection, &para);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  EXPECT_REF(reOle, 3);
  EXPECT_REF(doc, 3);
  EXPECT_REF(selection, 3);
  EXPECT_REF(para, 1);

  hr = ITextSelection_GetPara(selection, &para2);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(para != para2, "got %p, %p\n", para, para2);

  ITextPara_Release(para);
  ITextPara_Release(para2);
  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextRange_GetPara(range, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  hr = ITextRange_GetPara(range, &para);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  hr = ITextSelection_GetPara(selection, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  hr = ITextSelection_GetPara(selection, &para);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  ITextSelection_Release(selection);
  ITextRange_Release(range);
}

static void test_dispatch(void)
{
  static const WCHAR testnameW[] = {'G','e','t','T','e','x','t',0};
  static const WCHAR testname2W[] = {'T','e','x','t',0};
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextRange *range = NULL;
  WCHAR *nameW;
  DISPID dispid;
  HRESULT hr;
  UINT count;
  HWND hwnd;

  create_interfaces(&hwnd, &reOle, &doc, NULL);

  range = NULL;
  hr = ITextDocument_Range(doc, 0, 0, &range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(range != NULL, "got %p\n", range);

  dispid = 123;
  nameW = (WCHAR*)testnameW;
  hr = ITextRange_GetIDsOfNames(range, &IID_NULL, &nameW, 1, LOCALE_USER_DEFAULT, &dispid);
  ok(hr == DISP_E_UNKNOWNNAME, "got 0x%08lx\n", hr);
  ok(dispid == DISPID_UNKNOWN, "got %ld\n", dispid);

  dispid = 123;
  nameW = (WCHAR*)testname2W;
  hr = ITextRange_GetIDsOfNames(range, &IID_NULL, &nameW, 1, LOCALE_USER_DEFAULT, &dispid);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(dispid == DISPID_VALUE, "got %ld\n", dispid);

  release_interfaces(&hwnd, &reOle, &doc, NULL);

  /* try dispatch methods on detached range */
  hr = ITextRange_GetTypeInfoCount(range, &count);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  dispid = 123;
  nameW = (WCHAR*)testname2W;
  hr = ITextRange_GetIDsOfNames(range, &IID_NULL, &nameW, 1, LOCALE_USER_DEFAULT, &dispid);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(dispid == DISPID_VALUE, "got %ld\n", dispid);

  ITextRange_Release(range);
}

static void test_detached_font_getters(ITextFont *font, BOOL duplicate)
{
  HRESULT hr, hrexp = duplicate ? S_OK : CO_E_RELEASED;
  LONG value;
  float size;
  BSTR str;

  hr = ITextFont_GetBold(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  hr = ITextFont_GetBold(font, &value);
  ok(hr == hrexp, "got 0x%08lx\n", hr);

  hr = ITextFont_GetForeColor(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  hr = ITextFont_GetForeColor(font, &value);
  ok(hr == hrexp, "got 0x%08lx\n", hr);

  hr = ITextFont_GetItalic(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  hr = ITextFont_GetItalic(font, &value);
  ok(hr == hrexp, "got 0x%08lx\n", hr);

  hr = ITextFont_GetLanguageID(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  hr = ITextFont_GetLanguageID(font, &value);
  ok(hr == hrexp, "got 0x%08lx\n", hr);

  hr = ITextFont_GetName(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  hr = ITextFont_GetName(font, &str);
  ok(hr == hrexp, "got 0x%08lx\n", hr);

  hr = ITextFont_GetSize(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  hr = ITextFont_GetSize(font, &size);
  ok(hr == hrexp, "got 0x%08lx\n", hr);

  hr = ITextFont_GetStrikeThrough(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  hr = ITextFont_GetStrikeThrough(font, &value);
  ok(hr == hrexp, "got 0x%08lx\n", hr);

  hr = ITextFont_GetSubscript(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  hr = ITextFont_GetSubscript(font, &value);
  ok(hr == hrexp, "got 0x%08lx\n", hr);

  hr = ITextFont_GetSuperscript(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  hr = ITextFont_GetSuperscript(font, &value);
  ok(hr == hrexp, "got 0x%08lx\n", hr);

  hr = ITextFont_GetUnderline(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  hr = ITextFont_GetUnderline(font, &value);
  ok(hr == hrexp, "got 0x%08lx\n", hr);
}

static void test_textfont_global_defaults(ITextFont *font)
{
  float valuef;
  LONG value;
  HRESULT hr;
  BSTR str;

  value = tomUndefined;
  hr = ITextFont_GetAllCaps(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  value = tomUndefined;
  hr = ITextFont_GetAnimation(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  value = tomUndefined;
  hr = ITextFont_GetBackColor(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomAutoColor, "got %ld\n", value);

  value = tomUndefined;
  hr = ITextFont_GetBold(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse || value == tomTrue, "got %ld\n", value);

  value = tomUndefined;
  hr = ITextFont_GetEmboss(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  value = tomUndefined;
  hr = ITextFont_GetForeColor(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomAutoColor, "got %ld\n", value);

  value = tomUndefined;
  hr = ITextFont_GetHidden(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  value = tomUndefined;
  hr = ITextFont_GetEngrave(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  value = tomUndefined;
  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  valuef = 1.0;
  hr = ITextFont_GetKerning(font, &valuef);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(valuef == 0.0, "got %.2f\n", valuef);

  value = tomUndefined;
  hr = ITextFont_GetLanguageID(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == GetSystemDefaultLCID(), "got %ld\n", value);

  str = NULL;
  hr = ITextFont_GetName(font, &str);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(!lstrcmpW(sysW, str), "%s\n", wine_dbgstr_w(str));
  SysFreeString(str);

  value = tomUndefined;
  hr = ITextFont_GetOutline(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  valuef = 1.0;
  hr = ITextFont_GetPosition(font, &valuef);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(valuef == 0.0, "got %.2f\n", valuef);

  value = tomUndefined;
  hr = ITextFont_GetProtected(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  value = tomUndefined;
  hr = ITextFont_GetShadow(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  valuef = 0.0;
  hr = ITextFont_GetSize(font, &valuef);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(valuef >= 0.0, "got %.2f\n", valuef);

  value = tomUndefined;
  hr = ITextFont_GetSmallCaps(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  valuef = 1.0;
  hr = ITextFont_GetSpacing(font, &valuef);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(valuef == 0.0, "got %.2f\n", valuef);

  value = tomUndefined;
  hr = ITextFont_GetStrikeThrough(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  value = tomUndefined;
  hr = ITextFont_GetSubscript(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  value = tomUndefined;
  hr = ITextFont_GetSuperscript(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  value = tomUndefined;
  hr = ITextFont_GetUnderline(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  value = tomUndefined;
  hr = ITextFont_GetWeight(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == FW_NORMAL || value == FW_BOLD, "got %ld\n", value);
}

static void test_textfont_undefined(ITextFont *font)
{
  float valuef;
  LONG value;
  HRESULT hr;

  value = tomFalse;
  hr = ITextFont_GetAllCaps(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUndefined, "got %ld\n", value);

  value = tomFalse;
  hr = ITextFont_GetAnimation(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUndefined, "got %ld\n", value);

  value = tomFalse;
  hr = ITextFont_GetBackColor(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUndefined, "got %ld\n", value);

  value = tomFalse;
  hr = ITextFont_GetBold(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUndefined, "got %ld\n", value);

  value = tomFalse;
  hr = ITextFont_GetEmboss(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUndefined, "got %ld\n", value);

  value = tomFalse;
  hr = ITextFont_GetForeColor(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUndefined, "got %ld\n", value);

  value = tomFalse;
  hr = ITextFont_GetHidden(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUndefined, "got %ld\n", value);

  value = tomFalse;
  hr = ITextFont_GetEngrave(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUndefined, "got %ld\n", value);

  value = tomFalse;
  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUndefined, "got %ld\n", value);

  valuef = 0.0;
  hr = ITextFont_GetKerning(font, &valuef);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(valuef == tomUndefined, "got %.2f\n", valuef);

  value = tomFalse;
  hr = ITextFont_GetLanguageID(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUndefined, "got %ld\n", value);

  value = tomFalse;
  hr = ITextFont_GetOutline(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUndefined, "got %ld\n", value);

  valuef = 0.0;
  hr = ITextFont_GetPosition(font, &valuef);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(valuef == tomUndefined, "got %.2f\n", valuef);

  value = tomFalse;
  hr = ITextFont_GetProtected(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUndefined, "got %ld\n", value);

  value = tomFalse;
  hr = ITextFont_GetShadow(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUndefined, "got %ld\n", value);

  valuef = 0.0;
  hr = ITextFont_GetSize(font, &valuef);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(valuef == tomUndefined, "got %.2f\n", valuef);

  value = tomFalse;
  hr = ITextFont_GetSmallCaps(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUndefined, "got %ld\n", value);

  valuef = 0.0;
  hr = ITextFont_GetSpacing(font, &valuef);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(valuef == tomUndefined, "got %.2f\n", valuef);

  value = tomFalse;
  hr = ITextFont_GetStrikeThrough(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUndefined, "got %ld\n", value);

  value = tomFalse;
  hr = ITextFont_GetSubscript(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUndefined, "got %ld\n", value);

  value = tomFalse;
  hr = ITextFont_GetSuperscript(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUndefined, "got %ld\n", value);

  value = tomFalse;
  hr = ITextFont_GetUnderline(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUndefined, "got %ld\n", value);

  value = tomFalse;
  hr = ITextFont_GetWeight(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUndefined, "got %ld\n", value);
}

static inline FLOAT twips_to_points(LONG value)
{
  return value * 72.0 / 1440;
}

static void test_ITextFont(void)
{
  static const WCHAR arialW[] = {'A','r','i','a','l',0};
  static const CHAR test_text1[] = "TestSomeText";
  ITextFont *font, *font2, *font3;
  FLOAT size, position, kerning;
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextRange *range = NULL;
  CHARFORMAT2A cf;
  LONG value;
  HRESULT hr;
  HWND hwnd;
  BOOL ret;
  BSTR str;

  create_interfaces(&hwnd, &reOle, &doc, NULL);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);

  hr = ITextDocument_Range(doc, 0, 10, &range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextRange_GetFont(range, &font);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextFont_Reset(font, tomUseTwips);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  hr = ITextFont_Reset(font, tomUsePoints);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  hr = ITextFont_GetName(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  /* default font size unit is point */
  size = 0.0;
  hr = ITextFont_GetSize(font, &size);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  /* set to some non-zero values */
  hr = ITextFont_SetPosition(font, 20.0);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextFont_SetKerning(font, 10.0);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  position = 0.0;
  hr = ITextFont_GetPosition(font, &position);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  kerning = 0.0;
  hr = ITextFont_GetKerning(font, &kerning);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  memset(&cf, 0, sizeof(cf));
  cf.cbSize = sizeof(cf);
  cf.dwMask = CFM_SIZE|CFM_OFFSET|CFM_KERNING;

  /* CHARFORMAT members are in twips */
  SendMessageA(hwnd, EM_SETSEL, 0, 10);
  ret = SendMessageA(hwnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
  ok(ret, "got %d\n", ret);
  ok(size == twips_to_points(cf.yHeight), "got yHeight %ld, size %.2f\n", cf.yHeight, size);
  ok(position == twips_to_points(cf.yOffset), "got yOffset %ld, position %.2f\n", cf.yOffset, position);
  ok(kerning == twips_to_points(cf.wKerning), "got wKerning %d, kerning %.2f\n", cf.wKerning, kerning);

  hr = ITextFont_Reset(font, tomUseTwips);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  hr = ITextFont_Reset(font, tomUsePoints);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  hr = ITextFont_GetDuplicate(font, &font2);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextFont_Reset(font2, tomUseTwips);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  hr = ITextFont_Reset(font2, tomUsePoints);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  ITextFont_Release(font2);

  /* default font name */
  str = NULL;
  hr = ITextFont_GetName(font, &str);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(!lstrcmpW(str, sysW), "got %s\n", wine_dbgstr_w(str));
  SysFreeString(str);

  /* change font name for an inner subrange */
  memset(&cf, 0, sizeof(cf));
  cf.cbSize = sizeof(cf);
  cf.dwMask = CFM_FACE;
  strcpy(cf.szFaceName, "Arial");

  SendMessageA(hwnd, EM_SETSEL, 3, 4);
  ret = SendMessageA(hwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
  ok(ret, "got %d\n", ret);

  /* still original name */
  str = NULL;
  hr = ITextFont_GetName(font, &str);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(!lstrcmpW(str, sysW), "got %s\n", wine_dbgstr_w(str));
  SysFreeString(str);

  SendMessageA(hwnd, EM_SETSEL, 1, 2);
  ret = SendMessageA(hwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
  ok(ret, "got %d\n", ret);

  str = NULL;
  hr = ITextFont_GetName(font, &str);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(!lstrcmpW(str, sysW), "got %s\n", wine_dbgstr_w(str));
  SysFreeString(str);

  /* name is returned for first position within a range */
  SendMessageA(hwnd, EM_SETSEL, 0, 1);
  ret = SendMessageA(hwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
  ok(ret, "got %d\n", ret);

  str = NULL;
  hr = ITextFont_GetName(font, &str);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(!lstrcmpW(str, arialW), "got %s\n", wine_dbgstr_w(str));
  SysFreeString(str);

  /* GetDuplicate() */
  hr = ITextFont_GetDuplicate(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  EXPECT_REF(range, 2);
  font2 = NULL;
  hr = ITextFont_GetDuplicate(font, &font2);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  EXPECT_REF(range, 2);

  /* set whole range to italic */
  cf.cbSize = sizeof(CHARFORMAT2A);
  cf.dwMask = CFM_ITALIC;
  cf.dwEffects = CFE_ITALIC;

  SendMessageA(hwnd, EM_SETSEL, 0, 10);
  ret = SendMessageA(hwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
  ok(ret, "got %d\n", ret);

  value = tomFalse;
  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomTrue, "got %ld\n", value);

  /* duplicate retains original value */
  value = tomTrue;
  hr = ITextFont_GetItalic(font2, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  /* get a duplicate from a cloned font */
  hr = ITextFont_GetDuplicate(font2, &font3);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ITextFont_Release(font3);

  ITextRange_Release(range);
  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextFont_GetDuplicate(font, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  test_detached_font_getters(font, FALSE);
  test_detached_font_getters(font2, TRUE);

  /* get a duplicate of detached font */
  hr = ITextFont_GetDuplicate(font2, &font3);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ITextFont_Release(font3);

  /* reset detached font to undefined */
  value = tomUndefined;
  hr = ITextFont_GetBold(font2, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value != tomUndefined, "got %ld\n", value);

  /* reset to undefined for detached font */
  hr = ITextFont_Reset(font2, tomUndefined);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  test_textfont_undefined(font2);

  /* font is detached, default means global TOM defaults */
  hr = ITextFont_Reset(font2, tomDefault);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  test_textfont_global_defaults(font2);

  hr = ITextFont_GetDuplicate(font2, &font3);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  test_textfont_global_defaults(font2);

  hr = ITextFont_Reset(font2, tomApplyNow);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  test_textfont_global_defaults(font2);

  hr = ITextFont_Reset(font2, tomApplyLater);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  test_textfont_global_defaults(font2);

  hr = ITextFont_Reset(font2, tomTrackParms);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  test_textfont_global_defaults(font2);

  hr = ITextFont_SetItalic(font2, tomUndefined);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextFont_GetItalic(font2, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  hr = ITextFont_Reset(font2, tomCacheParms);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  test_textfont_global_defaults(font2);

  ITextFont_Release(font3);
  ITextFont_Release(font2);

  font2 = (void*)0xdeadbeef;
  hr = ITextFont_GetDuplicate(font, &font2);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);
  ok(font2 == NULL, "got %p\n", font2);

  hr = ITextFont_Reset(font, tomDefault);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  ITextFont_Release(font);

  /* Reset() */
  create_interfaces(&hwnd, &reOle, &doc, NULL);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);

  hr = ITextDocument_Range(doc, 0, 10, &range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextRange_GetFont(range, &font);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  value = tomUndefined;
  hr = ITextFont_GetBold(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value != tomUndefined, "got %ld\n", value);

  /* reset to undefined for attached font */
  hr = ITextFont_Reset(font, tomUndefined);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  value = tomUndefined;
  hr = ITextFont_GetBold(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value != tomUndefined, "got %ld\n", value);

  /* tomCacheParms/tomTrackParms */
  hr = ITextFont_Reset(font, tomCacheParms);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  memset(&cf, 0, sizeof(cf));
  cf.cbSize = sizeof(CHARFORMAT2A);
  cf.dwMask = CFM_ITALIC;

  cf.dwEffects = CFE_ITALIC;
  SendMessageA(hwnd, EM_SETSEL, 0, 10);
  ret = SendMessageA(hwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
  ok(ret, "got %d\n", ret);

  /* still cached value */
  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  hr = ITextFont_Reset(font, tomTrackParms);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomTrue, "got %ld\n", value);

  /* switch back to cache - value retained */
  hr = ITextFont_Reset(font, tomCacheParms);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomTrue, "got %ld\n", value);

  /* tomApplyLater */
  hr = ITextFont_Reset(font, tomApplyLater);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextFont_SetItalic(font, tomFalse);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  cf.dwEffects = 0;
  SendMessageA(hwnd, EM_SETSEL, 0, 10);
  ret = SendMessageA(hwnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
  ok(ret, "got %d\n", ret);
  ok((cf.dwEffects & CFE_ITALIC) == CFE_ITALIC, "got 0x%08lx\n", cf.dwEffects);

  hr = ITextFont_Reset(font, tomApplyNow);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  cf.dwEffects = 0;
  SendMessageA(hwnd, EM_SETSEL, 0, 10);
  ret = SendMessageA(hwnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
  ok(ret, "got %d\n", ret);
  ok((cf.dwEffects & CFE_ITALIC) == 0, "got 0x%08lx\n", cf.dwEffects);

  hr = ITextFont_SetItalic(font, tomUndefined);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  hr = ITextFont_SetItalic(font, tomAutoColor);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  cf.dwEffects = 0;
  SendMessageA(hwnd, EM_SETSEL, 0, 10);
  ret = SendMessageA(hwnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
  ok(ret, "got %d\n", ret);
  ok((cf.dwEffects & CFE_ITALIC) == 0, "got 0x%08lx\n", cf.dwEffects);

  ITextRange_Release(range);
  ITextFont_Release(font);
  release_interfaces(&hwnd, &reOle, &doc, NULL);
}

static void test_Delete(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextRange *range, *range2;
  LONG value;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reOle, &doc, NULL);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextDocument_Range(doc, 1, 2, &range2);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextRange_GetEnd(range, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 4, "got %ld\n", value);

  /* unit type doesn't matter is count is 0 */
  value = 0;
  hr = ITextRange_Delete(range2, tomSentence, 0, &value);
todo_wine {
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 1, "got %ld\n", value);
}
  value = 1;
  hr = ITextRange_Delete(range2, tomCharacter, 0, &value);
todo_wine {
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);
  ok(value == 0, "got %ld\n", value);
}
  hr = ITextRange_GetEnd(range, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  todo_wine
  ok(value == 3, "got %ld\n", value);

  hr = ITextRange_GetStart(range2, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 1, "got %ld\n", value);

  hr = ITextRange_GetEnd(range2, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  todo_wine
  ok(value == 1, "got %ld\n", value);

  ITextRange_Release(range);
  ITextRange_Release(range2);
  release_interfaces(&hwnd, &reOle, &doc, NULL);
}

static void test_SetText(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  static const WCHAR textW[] = {'a','b','c','d','e','f','g','h','i',0};
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextRange *range, *range2;
  LONG value;
  HRESULT hr;
  HWND hwnd;
  BSTR str;

  create_interfaces(&hwnd, &reOle, &doc, NULL);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextDocument_Range(doc, 0, 4, &range2);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  value = 1;
  hr = ITextRange_GetStart(range2, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 0, "got %ld\n", value);

  value = 0;
  hr = ITextRange_GetEnd(range2, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 4, "got %ld\n", value);

  hr = ITextRange_SetText(range, NULL);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  value = 1;
  hr = ITextRange_GetEnd(range2, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 0, "got %ld\n", value);

  str = SysAllocString(textW);
  hr = ITextRange_SetText(range, str);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  SysFreeString(str);

  value = 1;
  hr = ITextRange_GetStart(range, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 0, "got %ld\n", value);

  value = 0;
  hr = ITextRange_GetEnd(range, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 9, "got %ld\n", value);

  value = 1;
  hr = ITextRange_GetStart(range2, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 0, "got %ld\n", value);

  value = 0;
  hr = ITextRange_GetEnd(range2, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 0, "got %ld\n", value);

  str = SysAllocStringLen(NULL, 0);
  hr = ITextRange_SetText(range, str);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  value = 1;
  hr = ITextRange_GetEnd(range, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 0, "got %ld\n", value);
  SysFreeString(str);

  ITextRange_Release(range2);
  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextRange_SetText(range, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  str = SysAllocStringLen(NULL, 0);
  hr = ITextRange_SetText(range, str);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);
  SysFreeString(str);

  ITextRange_Release(range);
}

static void test_InRange(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  ITextRange *range, *range2, *range3;
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextSelection *selection;
  LONG value;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reOle, &doc, &selection);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);
  SendMessageA(hwnd, EM_SETSEL, 1, 2);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextDocument_Range(doc, 0, 4, &range2);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  /* matches selection */
  hr = ITextDocument_Range(doc, 1, 2, &range3);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextRange_InRange(range, NULL, NULL);
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);

  value = tomTrue;
  hr = ITextRange_InRange(range, NULL, &value);
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  hr = ITextRange_InRange(range, range2, NULL);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  value = tomFalse;
  hr = ITextRange_InRange(range, range2, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomTrue, "got %ld\n", value);

  /* selection */
  hr = ITextSelection_InRange(selection, NULL, NULL);
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);

  value = tomTrue;
  hr = ITextSelection_InRange(selection, NULL, &value);
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  hr = ITextSelection_InRange(selection, range2, NULL);
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);

  value = tomTrue;
  hr = ITextSelection_InRange(selection, range2, &value);
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  value = tomTrue;
  hr = ITextSelection_InRange(selection, range3, &value);
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  /* seems to work on ITextSelection ranges only */
  value = tomFalse;
  hr = ITextSelection_InRange(selection, (ITextRange*)selection, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomTrue, "got %ld\n", value);

  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextRange_InRange(range, NULL, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  value = tomTrue;
  hr = ITextRange_InRange(range, NULL, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  hr = ITextRange_InRange(range, range2, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  value = tomTrue;
  hr = ITextRange_InRange(range, range2, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  /* selection */
  hr = ITextSelection_InRange(selection, NULL, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  value = tomTrue;
  hr = ITextSelection_InRange(selection, NULL, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  hr = ITextSelection_InRange(selection, range2, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  value = tomTrue;
  hr = ITextSelection_InRange(selection, range2, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  ITextRange_Release(range);
  ITextRange_Release(range2);
  ITextRange_Release(range3);
  ITextSelection_Release(selection);
}

static void test_ITextRange_IsEqual(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  ITextRange *range, *range2, *range3;
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextSelection *selection;
  LONG value;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reOle, &doc, &selection);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);
  SendMessageA(hwnd, EM_SETSEL, 1, 2);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextDocument_Range(doc, 0, 4, &range2);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  /* matches selection */
  hr = ITextDocument_Range(doc, 1, 2, &range3);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextRange_IsEqual(range, NULL, NULL);
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);

  value = tomTrue;
  hr = ITextRange_IsEqual(range, NULL, &value);
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  hr = ITextRange_IsEqual(range, range2, NULL);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  value = tomFalse;
  hr = ITextRange_IsEqual(range, range2, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomTrue, "got %ld\n", value);

  value = tomTrue;
  hr = ITextRange_IsEqual(range, range3, &value);
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  /* selection */
  hr = ITextSelection_IsEqual(selection, NULL, NULL);
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);

  value = tomTrue;
  hr = ITextSelection_IsEqual(selection, NULL, &value);
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  hr = ITextSelection_IsEqual(selection, range2, NULL);
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);

  value = tomTrue;
  hr = ITextSelection_IsEqual(selection, range2, &value);
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  value = tomTrue;
  hr = ITextSelection_IsEqual(selection, range3, &value);
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  /* seems to work on ITextSelection ranges only */
  value = tomFalse;
  hr = ITextSelection_IsEqual(selection, (ITextRange*)selection, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomTrue, "got %ld\n", value);

  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextRange_IsEqual(range, NULL, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  value = tomTrue;
  hr = ITextRange_IsEqual(range, NULL, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  hr = ITextRange_IsEqual(range, range2, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  value = tomTrue;
  hr = ITextRange_IsEqual(range, range2, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  /* selection */
  hr = ITextSelection_IsEqual(selection, NULL, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  value = tomTrue;
  hr = ITextSelection_IsEqual(selection, NULL, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  hr = ITextSelection_IsEqual(selection, range2, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  value = tomTrue;
  hr = ITextSelection_IsEqual(selection, range2, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  ITextRange_Release(range);
  ITextRange_Release(range2);
  ITextRange_Release(range3);
  ITextSelection_Release(selection);
}

static int get_scroll_pos_y(HWND hwnd)
{
    POINT p = {-1, -1};
    SendMessageA(hwnd, EM_GETSCROLLPOS, 0, (LPARAM)&p);
    ok(p.x != -1 && p.y != -1, "p.x:%ld p.y:%ld\n", p.x, p.y);
    return p.y;
}

static void test_Select(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  static const CHAR test_text2[] = "text\nwith\nbreak\n"
                                   "lines\ntest\ntest\n";
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextSelection *selection;
  ITextRange *range;
  int scroll_pos1;
  int scroll_pos2;
  LONG value;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reOle, &doc, &selection);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);
  SendMessageA(hwnd, EM_SETSEL, 1, 2);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  scroll_pos1 = get_scroll_pos_y(hwnd);
  hr = ITextRange_Select(range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  scroll_pos2 = get_scroll_pos_y(hwnd);
  ok(scroll_pos1 == scroll_pos2, "%d != %d\n", scroll_pos1, scroll_pos2);

  value = 1;
  hr = ITextSelection_GetStart(selection, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 0, "got %ld\n", value);

  hr = ITextRange_Select(range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextSelection_Select(selection);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text2);
  SendMessageA(hwnd, EM_SETSEL, 1, 2);
  hr = ITextDocument_Range(doc, 10, 16, &range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  scroll_pos1 = get_scroll_pos_y(hwnd);
  hr = ITextRange_Select(range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  scroll_pos2 = get_scroll_pos_y(hwnd);
  ok(scroll_pos1 != scroll_pos2, "%d == %d\n", scroll_pos1, scroll_pos2);

  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextRange_Select(range);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  hr = ITextSelection_Select(selection);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  ITextRange_Release(range);
  ITextSelection_Release(selection);
}

static void test_GetStoryType(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextSelection *selection;
  ITextRange *range;
  LONG value;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reOle, &doc, &selection);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);
  SendMessageA(hwnd, EM_SETSEL, 1, 2);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextRange_GetStoryType(range, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  value = tomTextFrameStory;
  hr = ITextRange_GetStoryType(range, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUnknownStory, "got %ld\n", value);

  hr = ITextSelection_GetStoryType(selection, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  value = tomTextFrameStory;
  hr = ITextSelection_GetStoryType(selection, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomUnknownStory, "got %ld\n", value);

  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextRange_GetStoryType(range, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  value = 123;
  hr = ITextRange_GetStoryType(range, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);
  ok(value == 123, "got %ld\n", value);

  hr = ITextSelection_GetStoryType(selection, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  value = 123;
  hr = ITextSelection_GetStoryType(selection, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);
  ok(value == 123, "got %ld\n", value);

  ITextRange_Release(range);
  ITextSelection_Release(selection);
}

static void test_SetFont(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextSelection *selection;
  ITextRange *range, *range2;
  ITextFont *font, *font2;
  LONG value;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reOle, &doc, &selection);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);
  SendMessageA(hwnd, EM_SETSEL, 1, 2);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextDocument_Range(doc, 5, 2, &range2);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  EXPECT_REF(range, 1);
  hr = ITextRange_GetFont(range, &font);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  EXPECT_REF(range, 2);

  EXPECT_REF(range2, 1);
  hr = ITextRange_GetFont(range2, &font2);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  EXPECT_REF(range2, 2);

  hr = ITextRange_SetFont(range, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  /* setting same font, no-op */
  EXPECT_REF(range, 2);
  hr = ITextRange_SetFont(range, font);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  EXPECT_REF(range, 2);

  EXPECT_REF(range2, 2);
  EXPECT_REF(range, 2);
  hr = ITextRange_SetFont(range, font2);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  EXPECT_REF(range2, 2);
  EXPECT_REF(range, 2);

  /* originally range 0-4 is non-italic */
  value = tomTrue;
  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomFalse, "got %ld\n", value);

  /* set range 5-2 to italic, then set this font to range 0-4 */
  hr = ITextFont_SetItalic(font2, tomTrue);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextRange_SetFont(range, font2);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  value = tomFalse;
  hr = ITextFont_GetItalic(font, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == tomTrue, "got %ld\n", value);

  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextRange_SetFont(range, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  hr = ITextRange_SetFont(range, font);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  hr = ITextSelection_SetFont(selection, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  hr = ITextSelection_SetFont(selection, font);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  ITextFont_Release(font);
  ITextFont_Release(font2);
  ITextRange_Release(range);
  ITextRange_Release(range2);
  ITextSelection_Release(selection);
}

static void fill_reobject_struct(REOBJECT *reobj, LONG cp, LPOLEOBJECT poleobj,
                                 LPSTORAGE pstg, LPOLECLIENTSITE polesite, LONG sizel_cx,
                                 LONG sizel_cy, DWORD aspect, DWORD flags, DWORD user)
{
  reobj->cbStruct = sizeof(*reobj);
  reobj->clsid = CLSID_NULL;
  reobj->cp = cp;
  reobj->poleobj = poleobj;
  reobj->pstg = pstg;
  reobj->polesite = polesite;
  reobj->sizel.cx = sizel_cx;
  reobj->sizel.cy = sizel_cy;
  reobj->dvaspect = aspect;
  reobj->dwFlags = flags;
  reobj->dwUser = user;
}

#define CHECK_REOBJECT_STRUCT(reole,index,flags,cp,cp_check,poleobj,pstg,polesite,user) \
  _check_reobject_struct(reole, index, flags, cp, cp_check, poleobj, pstg, polesite, user, __LINE__)
static void _check_reobject_struct(IRichEditOle *reole, LONG index, DWORD flags, LONG cp, LONG cp_check,
    LPOLEOBJECT poleobj, LPSTORAGE pstg, LPOLECLIENTSITE polesite, DWORD user, int line)
{
  REOBJECT reobj;
  HRESULT hr;

  reobj.cbStruct = sizeof(reobj);
  reobj.cp = cp;
  hr = IRichEditOle_GetObject(reole, index, &reobj, flags);
  ok(hr == S_OK, "IRichEditOle_GetObject failed: %#lx.\n", hr);
  ok_(__FILE__,line)(reobj.cp == cp_check, "expected cp = %ld, got %ld.\n", cp_check, reobj.cp);
  ok_(__FILE__,line)(reobj.poleobj == poleobj, "got wrong object interface.\n");
  ok_(__FILE__,line)(reobj.pstg == pstg, "got wrong storage interface.\n");
  ok_(__FILE__,line)(reobj.polesite == polesite, "got wrong site interface.\n");
  ok_(__FILE__,line)(reobj.dwUser == user, "got wrong user-defined value.\n");
}

#define INSERT_REOBJECT(callback,reole,reobj,cp,user) \
  _insert_reobject(callback, reole, reobj, cp, user, __LINE__)
static void _insert_reobject(struct reolecb_obj *callback, IRichEditOle *reole,
                             REOBJECT *reobj, LONG cp, DWORD user, int line)
{
  IOleClientSite *clientsite;
  HRESULT hr;

  olecb_expect_QueryInsertObject(callback, line, 1,
                                 &CLSID_NULL, NULL, REO_CP_SELECTION /* cp overridden */, S_OK);
  hr = IRichEditOle_GetClientSite(reole, &clientsite);
  ok_(__FILE__,line)(hr == S_OK, "IRichEditOle_GetClientSite got hr %#lx.\n", hr);
  fill_reobject_struct(reobj, cp, NULL, NULL, clientsite, 10, 10, DVASPECT_CONTENT, 0, user);
  hr = IRichEditOle_InsertObject(reole, reobj);
  ok_(__FILE__,line)(hr == S_OK, "IRichEditOle_InsertObject got hr %#lx.\n", hr);
  IOleClientSite_Release(clientsite);
  olecb_check_QueryInsertObject(callback, line);
}

static void flush_dispatch_messages(void)
{
    MSG msg;
    while (PeekMessageW( &msg, NULL, 0, 0, PM_REMOVE ))
    {
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
    }
}

static void subtest_InsertObject(struct reolecb_obj *callback)
{
  static CHAR test_text1[] = "abcdefg";
  IRichEditOle *reole = NULL;
  ITextDocument *doc = NULL;
  REOBJECT reo1, reo2, reo3, received_reo;
  HRESULT hr;
  HWND hwnd;
  const WCHAR *expected_string, *string;
  const CHAR *expected_stringA;
  ITextSelection *selection;
  IDataObject *dataobject;
  TEXTRANGEA textrange;
  FORMATETC formatetc;
  CHARRANGE charrange;
  GETTEXTEX gettextex;
  STGMEDIUM stgmedium;
  WCHAR buffer[1024];
  CHAR bufferA[1024];
  LONG count, result;
  ITextRange *range;
  BSTR bstr;
  struct testoleobj *testobj;
  IOleClientSite *clientsite;
  REOBJECT reobj;
  BOOL bad_getsel;
  DWORD gle;

  create_interfaces(&hwnd, &reole, &doc, &selection);
  if (callback)
  {
    LRESULT sendres = SendMessageA(hwnd, EM_SETOLECALLBACK, 0, (LPARAM)&callback->IRichEditOleCallback_iface);
    ok( !!sendres, "EM_SETOLECALLBACK should succeed\n" );
  }

  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)"a");
  SendMessageA(hwnd, EM_SETSEL, 0, -1);
  *bufferA = '\0';
  SetLastError(0xdeadbeef);
  result = SendMessageA(hwnd, EM_GETSELTEXT, 0, (LPARAM)bufferA);
  gle = GetLastError();
  ok((result > 0 && gle == 0xdeadbeef) ||
     broken(result == 0 && gle == ERROR_INVALID_PARAMETER /* Hindi */),
     "EM_GETSELTEXT returned %ld gle=%lu\n", result, gle);
  bad_getsel = (gle != 0xdeadbeef);
  if (bad_getsel)
      trace("EM_GETSELTEXT is broken, some tests will be ignored\n");

  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);

  hr = IRichEditOle_InsertObject(reole, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  /* insert object1 in (0, 1)*/
  SendMessageA(hwnd, EM_SETSEL, 0, 1);
  INSERT_REOBJECT(callback, reole, &reo1, REO_CP_SELECTION, 1);
  count = IRichEditOle_GetObjectCount(reole);
  ok(count == 1, "got wrong object count: %ld\n", count);

  /* insert object2 in (2, 3)*/
  SendMessageA(hwnd, EM_SETSEL, 2, 3);
  INSERT_REOBJECT(callback, reole, &reo2, REO_CP_SELECTION, 2);
  count = IRichEditOle_GetObjectCount(reole);
  ok(count == 2, "got wrong object count: %ld\n", count);

  /* insert object3 in (1, 2)*/
  SendMessageA(hwnd, EM_SETSEL, 1, 2);
  INSERT_REOBJECT(callback, reole, &reo3, REO_CP_SELECTION, 3);
  count = IRichEditOle_GetObjectCount(reole);
  ok(count == 3, "got wrong object count: %ld\n", count);

  if (callback)
  {
    /* (fail to) insert object1 in (3, 4)*/
    SendMessageA(hwnd, EM_SETSEL, 3, 4);

    hr = IRichEditOle_GetClientSite(reole, &clientsite);
    ok(hr == S_OK, "IRichEditOle_GetClientSite got hr %#lx.\n", hr);

    olecb_expect_QueryInsertObject(callback, __LINE__, 1,
                                   &CLSID_NULL, NULL, REO_CP_SELECTION, S_FALSE);
    fill_reobject_struct(&reobj, REO_CP_SELECTION, NULL, NULL, clientsite, 10, 10, DVASPECT_CONTENT, 0, 0);
    hr = IRichEditOle_InsertObject(reole, &reobj);
    ok(hr == S_FALSE, "IRichEditOle_InsertObject got hr %#lx.\n", hr);
    olecb_check_QueryInsertObject(callback, __LINE__);

    IOleClientSite_Release(clientsite);

    count = IRichEditOle_GetObjectCount(reole);
    ok(count == 3, "got wrong object count: %ld\n", count);
  }

  /* tests below show that order of rebject (from 0 to 2) is: reo1,reo3,reo2 */
  CHECK_REOBJECT_STRUCT(reole, 0, REO_GETOBJ_ALL_INTERFACES, 0, 0, NULL, NULL, reo1.polesite, 1);
  CHECK_REOBJECT_STRUCT(reole, 1, REO_GETOBJ_ALL_INTERFACES, 0, 1, NULL, NULL, reo3.polesite, 3);
  CHECK_REOBJECT_STRUCT(reole, 2, REO_GETOBJ_ALL_INTERFACES, 0, 2, NULL, NULL, reo2.polesite, 2);

  hr = IRichEditOle_GetObject(reole, 2, NULL, REO_GETOBJ_ALL_INTERFACES);
  ok(hr == E_INVALIDARG, "IRichEditOle_GetObject should fail: 0x%08lx\n", hr);

  received_reo.cbStruct = 0;
  hr = IRichEditOle_GetObject(reole, 2, &received_reo, REO_GETOBJ_ALL_INTERFACES);
  ok(hr == E_INVALIDARG, "IRichEditOle_GetObject should fail: 0x%08lx\n", hr);

  CHECK_REOBJECT_STRUCT(reole, 2, REO_GETOBJ_PSTG, 0, 2, NULL, NULL, NULL, 2);
  CHECK_REOBJECT_STRUCT(reole, 2, REO_GETOBJ_POLESITE, 0, 2, NULL, NULL, reo2.polesite, 2);

  hr = IRichEditOle_GetObject(reole, 3, &received_reo, REO_GETOBJ_POLESITE);
  ok(hr == E_INVALIDARG, "IRichEditOle_GetObject should fail: 0x%08lx\n", hr);

  hr = IRichEditOle_GetObject(reole, 4, &received_reo, REO_GETOBJ_POLESITE);
  ok(hr == E_INVALIDARG, "IRichEditOle_GetObject should fail: 0x%08lx\n", hr);

  hr = IRichEditOle_GetObject(reole, 1024, &received_reo, REO_GETOBJ_POLESITE);
  ok(hr == E_INVALIDARG, "IRichEditOle_GetObject should fail: 0x%08lx\n", hr);

  hr = IRichEditOle_GetObject(reole, -10, &received_reo, REO_GETOBJ_POLESITE);
  ok(hr == E_INVALIDARG, "IRichEditOle_GetObject should fail: 0x%08lx\n", hr);

  /* received_reo will be zeroed before be used */
  received_reo.cbStruct = sizeof(received_reo);
  received_reo.polesite = (IOleClientSite *)0xdeadbeef;
  hr = IRichEditOle_GetObject(reole, 2, &received_reo, REO_GETOBJ_NO_INTERFACES);
  ok(hr == S_OK, "IRichEditOle_GetObject failed: 0x%08lx\n", hr);
  ok(received_reo.polesite == NULL, "Got wrong site interface.\n");

  CHECK_REOBJECT_STRUCT(reole, REO_IOB_USE_CP, REO_GETOBJ_ALL_INTERFACES, 0, 0, NULL, NULL, reo1.polesite, 1);
  CHECK_REOBJECT_STRUCT(reole, REO_IOB_USE_CP, REO_GETOBJ_ALL_INTERFACES, 1, 1, NULL, NULL, reo3.polesite, 3);
  CHECK_REOBJECT_STRUCT(reole, REO_IOB_USE_CP, REO_GETOBJ_ALL_INTERFACES, 2, 2, NULL, NULL, reo2.polesite, 2);

  received_reo.cbStruct = sizeof(received_reo);
  received_reo.polesite = (IOleClientSite *)0xdeadbeef;
  received_reo.dwUser = 4;
  received_reo.cp = 4;
  hr = IRichEditOle_GetObject(reole, REO_IOB_USE_CP, &received_reo, REO_GETOBJ_ALL_INTERFACES);
  ok(hr == E_INVALIDARG, "IRichEditOle_GetObject should fail: 0x%08lx\n", hr);
  ok(received_reo.polesite == (IOleClientSite *)0xdeadbeef, "Got wrong site interface.\n");
  ok(received_reo.dwUser == 4, "Got wrong user-defined value: %ld.\n", received_reo.dwUser);

  SendMessageA(hwnd, EM_SETSEL, 0, 1);
  CHECK_REOBJECT_STRUCT(reole, REO_IOB_SELECTION, REO_GETOBJ_ALL_INTERFACES, 0, 0, NULL, NULL, reo1.polesite, 1);

  SendMessageA(hwnd, EM_SETSEL, 1, 2);
  CHECK_REOBJECT_STRUCT(reole, REO_IOB_SELECTION, REO_GETOBJ_ALL_INTERFACES, 0, 1, NULL, NULL, reo3.polesite, 3);

  SendMessageA(hwnd, EM_SETSEL, 2, 3);
  CHECK_REOBJECT_STRUCT(reole, REO_IOB_SELECTION, REO_GETOBJ_ALL_INTERFACES, 0, 2, NULL, NULL, reo2.polesite, 2);

  SendMessageA(hwnd, EM_SETSEL, 0, 2);
  CHECK_REOBJECT_STRUCT(reole, REO_IOB_SELECTION, REO_GETOBJ_ALL_INTERFACES, 0, 0, NULL, NULL, reo1.polesite, 1);

  SendMessageA(hwnd, EM_SETSEL, 1, 3);
  CHECK_REOBJECT_STRUCT(reole, REO_IOB_SELECTION, REO_GETOBJ_ALL_INTERFACES, 0, 1, NULL, NULL, reo3.polesite, 3);

  SendMessageA(hwnd, EM_SETSEL, 2, 0);
  CHECK_REOBJECT_STRUCT(reole, REO_IOB_SELECTION, REO_GETOBJ_ALL_INTERFACES, 0, 0, NULL, NULL, reo1.polesite, 1);

  SendMessageA(hwnd, EM_SETSEL, 0, 6);
  CHECK_REOBJECT_STRUCT(reole, REO_IOB_SELECTION, REO_GETOBJ_ALL_INTERFACES, 0, 0, NULL, NULL, reo1.polesite, 1);

  SendMessageA(hwnd, EM_SETSEL, 4, 5);
  received_reo.cbStruct = sizeof(received_reo);
  received_reo.cp = 0;
  hr = IRichEditOle_GetObject(reole, REO_IOB_SELECTION, &received_reo, REO_GETOBJ_ALL_INTERFACES);
  ok(hr == E_INVALIDARG, "IRichEditOle_GetObject should fail: 0x%08lx\n", hr);

  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);

  /* "abc|d|efg" */
  INSERT_REOBJECT(callback, reole, &reo1, 3, 1);
  INSERT_REOBJECT(callback, reole, &reo2, 5, 2);

  SendMessageW(hwnd, EM_SETSEL, 2, 3);
  result = SendMessageW(hwnd, EM_SELECTIONTYPE, 0, 0);
  ok(result == SEL_TEXT, "Got selection type: %lx.\n", result);

  SendMessageW(hwnd, EM_SETSEL, 3, 4);
  result = SendMessageW(hwnd, EM_SELECTIONTYPE, 0, 0);
  ok(result == SEL_OBJECT, "Got selection type: %lx.\n", result);
  CHECK_REOBJECT_STRUCT(reole, REO_IOB_SELECTION, REO_GETOBJ_ALL_INTERFACES, 1, 3, NULL, NULL, reo1.polesite, 1);

  SendMessageW(hwnd, EM_SETSEL, 2, 4);
  result = SendMessageW(hwnd, EM_SELECTIONTYPE, 0, 0);
  ok(result == (SEL_TEXT | SEL_OBJECT), "Got selection type: %lx.\n", result);

  SendMessageW(hwnd, EM_SETSEL, 5, 6);
  CHECK_REOBJECT_STRUCT(reole, REO_IOB_SELECTION, REO_GETOBJ_ALL_INTERFACES, 1, 5, NULL, NULL, reo2.polesite, 2);

#ifdef __REACTOS__
  expected_string = L"abc\xfffc"L"d\xfffc"L"efg";
#else
  expected_string = L"abc\xfffc""d\xfffc""efg";
#endif

  gettextex.cb = sizeof(buffer);
  gettextex.flags = GT_DEFAULT;
  gettextex.codepage = 1200;
  gettextex.lpDefaultChar = NULL;
  gettextex.lpUsedDefChar = NULL;
  result = SendMessageW(hwnd, EM_GETTEXTEX, (WPARAM)&gettextex, (LPARAM)buffer);
  ok(result == lstrlenW(expected_string), "Got wrong length: %ld.\n", result);
  todo_wine ok(!lstrcmpW(buffer, expected_string), "Got wrong content: %s.\n", debugstr_w(buffer));

  gettextex.flags = GT_RAWTEXT;
  memset(buffer, 0, sizeof(buffer));
  result = SendMessageW(hwnd, EM_GETTEXTEX, (WPARAM)&gettextex, (LPARAM)buffer);
  ok(result == lstrlenW(expected_string), "Got wrong length: %ld.\n", result);
  todo_wine ok(!lstrcmpW(buffer, expected_string), "Got wrong content: %s.\n", debugstr_w(buffer));

  gettextex.flags = GT_NOHIDDENTEXT;
  memset(buffer, 0, sizeof(buffer));
  result = SendMessageW(hwnd, EM_GETTEXTEX, (WPARAM)&gettextex, (LPARAM)buffer);
  ok(result == lstrlenW(expected_string), "Got wrong length: %ld.\n", result);
  todo_wine ok(!lstrcmpW(buffer, expected_string), "Got wrong content: %s.\n", debugstr_w(buffer));

  gettextex.flags = GT_SELECTION;
  memset(buffer, 0, sizeof(buffer));
  SendMessageW(hwnd, EM_SETSEL, 0, -1);
  result = SendMessageW(hwnd, EM_GETTEXTEX, (WPARAM)&gettextex, (LPARAM)buffer);
  ok(result == lstrlenW(expected_string), "Got wrong length: %ld.\n", result);
  todo_wine ok(!lstrcmpW(buffer, expected_string), "Got wrong content: %s.\n", debugstr_w(buffer));

  expected_string = L"abc d efg";
  gettextex.flags = GT_USECRLF;
  memset(buffer, 0, sizeof(buffer));
  result = SendMessageW(hwnd, EM_GETTEXTEX, (WPARAM)&gettextex, (LPARAM)buffer);
  ok(result == lstrlenW(expected_string), "Got wrong length: %ld.\n", result);
  ok(!lstrcmpW(buffer, expected_string), "Got wrong content: %s.\n", debugstr_w(buffer));

  expected_stringA = "abc d efg";
  memset(bufferA, 0, sizeof(bufferA));
  SendMessageA(hwnd, EM_SETSEL, 0, -1);
  SetLastError(0xdeadbeef);
  result = SendMessageA(hwnd, EM_GETSELTEXT, 0, (LPARAM)bufferA);
  gle = GetLastError();
  ok(result == strlen(expected_stringA) || broken(bad_getsel && result == 0),
     "Got wrong length: %ld (gle %lu)\n", result, gle);
  ok(!strcmp(bufferA, expected_stringA) || broken(bad_getsel && !*bufferA),
     "Got wrong content: %s (gle %lu)\n", bufferA, gle);

  memset(bufferA, 0, sizeof(bufferA));
  textrange.lpstrText = bufferA;
  textrange.chrg.cpMin = 0;
  textrange.chrg.cpMax = 11;
  result = SendMessageA(hwnd, EM_GETTEXTRANGE, 0, (LPARAM)&textrange);
  ok(result == strlen(expected_stringA), "Got wrong length: %ld.\n", result);
  ok(!strcmp(bufferA, expected_stringA), "Got wrong content: %s.\n", bufferA);

#ifdef __REACTOS__
  expected_string = L"abc\xfffc"L"d\xfffc"L"efg\r";
#else
  expected_string = L"abc\xfffc""d\xfffc""efg\r";
#endif

  hr = ITextDocument_Range(doc, 0, 11, &range);
  ok(hr == S_OK, "Got hr %#lx.\n", hr);
  hr = ITextRange_GetText(range, &bstr);
  ok(hr == S_OK, "Got hr %#lx.\n", hr);
  ok(lstrlenW(bstr) == lstrlenW(expected_string), "Got wrong length: %d.\n", lstrlenW(bstr));
  todo_wine ok(!lstrcmpW(bstr, expected_string), "Got text: %s.\n", wine_dbgstr_w(bstr));
  SysFreeString(bstr);
  hr = ITextRange_SetRange(range, 3, 4);
  ok(hr == S_OK, "Got hr %#lx.\n", hr);
  hr = ITextRange_GetChar(range, &result);
  ok(hr == S_OK, "Got hr %#lx.\n", hr);
  todo_wine ok(result == 0xfffc, "Got char: %lc\n", (WCHAR)result);
  ITextRange_Release(range);

  SendMessageW(hwnd, EM_SETSEL, 0, -1);
  hr = ITextSelection_GetText(selection, &bstr);
  ok(hr == S_OK, "Got hr %#lx.\n", hr);
  ok(lstrlenW(bstr) == lstrlenW(expected_string), "Got wrong length: %d.\n", lstrlenW(bstr));
  todo_wine ok(!lstrcmpW(bstr, expected_string), "Got text: %s.\n", wine_dbgstr_w(bstr));
  SysFreeString(bstr);
  SendMessageW(hwnd, EM_SETSEL, 3, 4);
  result = 0;
  hr = ITextSelection_GetChar(selection, &result);
  ok(hr == S_OK, "Got hr %#lx.\n", hr);
  todo_wine ok(result == 0xfffc, "Got char: %lc\n", (WCHAR)result);

  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)"");
  result = SendMessageW(hwnd, EM_SETTEXTMODE, (WPARAM)TM_PLAINTEXT, 0);
  ok(!result, "Got result %lx.\n", result);
  /* "abc|d|efg" */
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);
  INSERT_REOBJECT(callback, reole, &reo1, 3, 1);
  INSERT_REOBJECT(callback, reole, &reo2, 5, 2);

  expected_string = L"abc d efg";
  charrange.cpMin = 0;
  charrange.cpMax = 11;
  hr = IRichEditOle_GetClipboardData(reole, &charrange, 1, &dataobject);
  ok(hr == S_OK, "Got hr %#lx.\n", hr);
  formatetc.cfFormat = CF_UNICODETEXT;
  formatetc.dwAspect = DVASPECT_CONTENT;
  formatetc.ptd = NULL;
  formatetc.tymed = TYMED_HGLOBAL;
  formatetc.lindex = -1;
  hr = IDataObject_GetData(dataobject, &formatetc, &stgmedium);
  ok(hr == S_OK, "Got hr %#lx.\n", hr);
  string = GlobalLock(stgmedium.hGlobal);
  ok(lstrlenW(string) == lstrlenW(expected_string), "Got wrong length: %d.\n", lstrlenW(string));
  ok(!lstrcmpW(string, expected_string), "Got wrong content: %s.\n", debugstr_w(string));
  GlobalUnlock(stgmedium.hGlobal);

#ifdef __REACTOS__
  expected_string = L"abc\xfffc"L"d\xfffc"L"efg";
#else
  expected_string = L"abc\xfffc""d\xfffc""efg";
#endif

  gettextex.cb = sizeof(buffer);
  gettextex.flags = GT_DEFAULT;
  gettextex.codepage = 1200;
  gettextex.lpDefaultChar = NULL;
  gettextex.lpUsedDefChar = NULL;
  result = SendMessageW(hwnd, EM_GETTEXTEX, (WPARAM)&gettextex, (LPARAM)buffer);
  ok(result == lstrlenW(expected_string), "Got wrong length: %ld.\n", result);
  todo_wine ok(!lstrcmpW(buffer, expected_string), "Got wrong content: %s.\n", debugstr_w(buffer));

  gettextex.flags = GT_RAWTEXT;
  memset(buffer, 0, sizeof(buffer));
  result = SendMessageW(hwnd, EM_GETTEXTEX, (WPARAM)&gettextex, (LPARAM)buffer);
  ok(result == lstrlenW(expected_string), "Got wrong length: %ld.\n", result);
  todo_wine ok(!lstrcmpW(buffer, expected_string), "Got wrong content: %s.\n", debugstr_w(buffer));

  expected_stringA = "abc d efg";
  memset(bufferA, 0, sizeof(bufferA));
  SendMessageA(hwnd, EM_SETSEL, 0, -1);
  SetLastError(0xdeadbeef);
  result = SendMessageA(hwnd, EM_GETSELTEXT, 0, (LPARAM)bufferA);
  gle = GetLastError();
  ok(result == strlen(expected_stringA) || broken(bad_getsel && result == 0),
     "Got wrong length: %ld (gle %lu)\n", result, gle);
  ok(!strcmp(bufferA, expected_stringA) || broken(bad_getsel && !*bufferA),
     "Got wrong content: %s (gle %lu)\n", bufferA, gle);

  memset(bufferA, 0, sizeof(bufferA));
  textrange.lpstrText = bufferA;
  textrange.chrg.cpMin = 0;
  textrange.chrg.cpMax = 11;
  result = SendMessageA(hwnd, EM_GETTEXTRANGE, 0, (LPARAM)&textrange);
  ok(result == strlen(expected_stringA), "Got wrong length: %ld.\n", result);
  ok(!strcmp(bufferA, expected_stringA), "Got wrong content: %s.\n", bufferA);

#ifdef __REACTOS__
  expected_string = L"abc\xfffc"L"d\xfffc"L"efg";
#else
  expected_string = L"abc\xfffc""d\xfffc""efg";
#endif

  hr = ITextDocument_Range(doc, 0, 11, &range);
  ok(hr == S_OK, "Got hr %#lx.\n", hr);
  hr = ITextRange_GetText(range, &bstr);
  ok(hr == S_OK, "Got hr %#lx.\n", hr);
  todo_wine ok(lstrlenW(bstr) == lstrlenW(expected_string), "Got wrong length: %d.\n", lstrlenW(bstr));
  todo_wine ok(!lstrcmpW(bstr, expected_string), "Got text: %s.\n", wine_dbgstr_w(bstr));
  SysFreeString(bstr);
  hr = ITextRange_SetRange(range, 3, 4);
  ok(hr == S_OK, "Got hr %#lx.\n", hr);
  hr = ITextRange_GetChar(range, &result);
  ok(hr == S_OK, "Got hr %#lx.\n", hr);
  todo_wine ok(result == 0xfffc, "Got char: %lc\n", (WCHAR)result);
  ITextRange_Release(range);

  SendMessageW(hwnd, EM_SETSEL, 0, -1);
  hr = ITextSelection_GetText(selection, &bstr);
  ok(hr == S_OK, "Got hr %#lx.\n", hr);
  todo_wine ok(lstrlenW(bstr) == lstrlenW(expected_string), "Got wrong length: %d.\n", lstrlenW(bstr));
  todo_wine ok(!lstrcmpW(bstr, expected_string), "Got text: %s.\n", wine_dbgstr_w(bstr));
  SysFreeString(bstr);
  SendMessageW(hwnd, EM_SETSEL, 3, 4);
  result = 0;
  hr = ITextSelection_GetChar(selection, &result);
  ok(hr == S_OK, "Got hr %#lx.\n", hr);
  todo_wine ok(result == 0xfffc, "Got char: %lc\n", (WCHAR)result);

  hr = testoleobj_Create(&testobj);
  ok(hr == S_OK, "testoleobj_Create got hr %#lx.\n", hr);
  testobj->extent.cx = 800;
  testobj->extent.cy = 400;

  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)"");
  testobj->draw_count = 0;
  testobj->line = __LINE__;

  hr = IRichEditOle_GetClientSite(reole, &clientsite);
  ok(hr == S_OK, "IRichEditOle_GetClientSite got hr %#lx.\n", hr);
  hr = IOleObject_SetClientSite(&testobj->IOleObject_iface, clientsite);
  ok(hr == S_OK, "IOleObject_SetClientSite got hr %#lx.\n", hr);

  olecb_expect_QueryInsertObject(callback, __LINE__, 1,
                                 &CLSID_testoleobj, NULL, REO_CP_SELECTION, S_OK);
  fill_reobject_struct(&reobj, REO_CP_SELECTION, &testobj->IOleObject_iface, NULL, clientsite, 800, 400, DVASPECT_CONTENT, 0, 0);
  reobj.clsid = CLSID_testoleobj;
  hr = IRichEditOle_InsertObject(reole, &reobj);
  ok(hr == S_OK, "IRichEditOle_InsertObject got hr %#lx.\n", hr);
  olecb_check_QueryInsertObject(callback, __LINE__);

  IOleClientSite_Release(clientsite);

  testobj->line = __LINE__;
  UpdateWindow(hwnd);
  testobj->line = __LINE__;
  flush_dispatch_messages();
  ok(testobj->draw_count != 0, "expected draw_count to be nonzero, got %d\n", testobj->draw_count);

  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)"");
  testobj->draw_count = 0;
  testobj->line = __LINE__;

  hr = IRichEditOle_GetClientSite(reole, &clientsite);
  ok(hr == S_OK, "IRichEditOle_GetClientSite got hr %#lx.\n", hr);
  hr = IOleObject_SetClientSite(&testobj->IOleObject_iface, clientsite);
  ok(hr == S_OK, "IOleObject_SetClientSite got hr %#lx.\n", hr);

  olecb_expect_QueryInsertObject(callback, __LINE__, 1,
                                 &CLSID_testoleobj, NULL, REO_CP_SELECTION, S_OK);
  fill_reobject_struct(&reobj, REO_CP_SELECTION, &testobj->IOleObject_iface, NULL, clientsite, 0, 0, DVASPECT_CONTENT, 0, 0);
  reobj.clsid = CLSID_testoleobj;
  hr = IRichEditOle_InsertObject(reole, &reobj);
  ok(hr == S_OK, "IRichEditOle_InsertObject got hr %#lx.\n", hr);
  olecb_check_QueryInsertObject(callback, __LINE__);

  memset(&reobj, 0xcc, sizeof(reobj));
  reobj.cbStruct = sizeof(reobj);
  hr = IRichEditOle_GetObject(reole, 0, &reobj, REO_GETOBJ_NO_INTERFACES);
  ok(hr == S_OK, "IRichEditOle_GetObject got hr %#lx.\n", hr);
  ok(reobj.sizel.cx == 800, "expected reobj.sizel.cx to be %ld, got %ld\n", 800L, reobj.sizel.cx);
  ok(reobj.sizel.cy == 400, "expected reobj.sizel.cy to be %ld, got %ld\n", 400L, reobj.sizel.cy);
  IOleClientSite_Release(clientsite);

  testobj->line = __LINE__;
  UpdateWindow(hwnd);
  testobj->line = __LINE__;
  flush_dispatch_messages();
  ok(testobj->draw_count != 0, "expected draw_count to be nonzero, got %d\n", testobj->draw_count);

  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)"");
  IOleObject_Release(&testobj->IOleObject_iface);

  if (callback)
  {
    LRESULT sendres = SendMessageA(hwnd, EM_SETOLECALLBACK, 0, 0);
    ok( !!sendres, "EM_SETOLECALLBACK should succeed\n" );
  }

  release_interfaces(&hwnd, &reole, &doc, &selection);
}

static void test_InsertObject(void)
{
  struct reolecb_obj *callback;
  HRESULT hr;
  ULONG ref;

  subtest_InsertObject(NULL);

  hr = reolecb_obj_Create(&callback);
  ok(SUCCEEDED(hr), "reolecb_obj_Create returned %#lx\n", hr);
  if (SUCCEEDED(hr))
  {
    subtest_InsertObject(callback);
    ref = IRichEditOleCallback_Release(&callback->IRichEditOleCallback_iface);
    ok(ref == 0, "expected IRichEditOleCallback recount to be 0, got %lu\n", ref);
  }
  else
  {
    skip("cannot test InsertObject with callback\n");
  }
}

static void test_GetStoryLength(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextSelection *selection;
  ITextRange *range;
  LONG value;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reOle, &doc, &selection);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);
  SendMessageA(hwnd, EM_SETSEL, 1, 2);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextRange_GetStoryLength(range, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  value = 0;
  hr = ITextRange_GetStoryLength(range, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 13, "got %ld\n", value);

  hr = ITextSelection_GetStoryLength(selection, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  value = 0;
  hr = ITextSelection_GetStoryLength(selection, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 13, "got %ld\n", value);

  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)"");

  value = 0;
  hr = ITextRange_GetStoryLength(range, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 1, "got %ld\n", value);

  value = 0;
  hr = ITextSelection_GetStoryLength(selection, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 1, "got %ld\n", value);

  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextRange_GetStoryLength(range, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  value = 100;
  hr = ITextRange_GetStoryLength(range, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);
  ok(value == 100, "got %ld\n", value);

  hr = ITextSelection_GetStoryLength(selection, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  value = 100;
  hr = ITextSelection_GetStoryLength(selection, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);
  ok(value == 100, "got %ld\n", value);

  ITextSelection_Release(selection);
  ITextRange_Release(range);
}

static void test_ITextSelection_GetDuplicate(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  IRichEditOle *reOle = NULL;
  ITextDocument *doc = NULL;
  ITextSelection *selection, *sel2;
  ITextRange *range, *range2;
  ITextFont *font;
  LONG value;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reOle, &doc, &selection);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);
  SendMessageA(hwnd, EM_SETSEL, 1, 2);

  hr = ITextSelection_GetDuplicate(selection, NULL);
  ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

  EXPECT_REF(selection, 2);

  hr = ITextSelection_GetDuplicate(selection, &range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextSelection_GetDuplicate(selection, &range2);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(range != range2, "got %p, %p\n", range, range2);

  EXPECT_REF(selection, 2);
  EXPECT_REF(range, 1);
  EXPECT_REF(range2, 1);

  ITextRange_Release(range2);

  value = 0;
  hr = ITextRange_GetStart(range, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 1, "got %ld\n", value);

  value = 0;
  hr = ITextRange_GetEnd(range, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 2, "got %ld\n", value);

  SendMessageA(hwnd, EM_SETSEL, 2, 3);

  value = 0;
  hr = ITextRange_GetStart(range, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 1, "got %ld\n", value);

  value = 0;
  hr = ITextRange_GetEnd(range, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 2, "got %ld\n", value);

  hr = ITextRange_QueryInterface(range, &IID_ITextSelection, (void**)&sel2);
  ok(hr == E_NOINTERFACE, "got 0x%08lx\n", hr);

  release_interfaces(&hwnd, &reOle, &doc, NULL);

  hr = ITextSelection_GetDuplicate(selection, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  hr = ITextSelection_GetDuplicate(selection, &range);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  hr = ITextRange_GetFont(range, &font);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  ITextSelection_Release(selection);
  ITextRange_Release(range);
}

#define RESET_RANGE(range,start,end) \
  _reset_range(range, start, end, __LINE__)
static void _reset_range(ITextRange *range, LONG start, LONG end, int line)
{
  HRESULT hr;

  hr = ITextRange_SetStart(range, start);
  ok_(__FILE__,line)(hr == S_OK, "SetStart failed: 0x%08lx\n", hr);
  hr = ITextRange_SetEnd(range, end);
  ok_(__FILE__,line)(hr == S_OK, "SetEnd failed: 0x%08lx\n", hr);
}

#define CHECK_RANGE(range,expected_start,expected_end) \
  _check_range(range, expected_start, expected_end, __LINE__)
static void _check_range(ITextRange* range, LONG expected_start, LONG expected_end, int line)
{
  HRESULT hr;
  LONG value;

  hr = ITextRange_GetStart(range, &value);
  ok_(__FILE__,line)(hr == S_OK, "GetStart failed: 0x%08lx\n", hr);
  ok_(__FILE__,line)(value == expected_start, "Expected start %ld got %ld\n",
                     expected_start, value);
  hr = ITextRange_GetEnd(range, &value);
  ok_(__FILE__,line)(hr == S_OK, "GetEnd failed: 0x%08lx\n", hr);
  ok_(__FILE__,line)(value == expected_end, "Expected end %ld got %ld\n",
                     expected_end, value);
}

#define RESET_SELECTION(selection,start,end) \
  _reset_selection(selection, start, end, __LINE__)
static void _reset_selection(ITextSelection *selection, LONG start, LONG end, int line)
{
  HRESULT hr;

  hr = ITextSelection_SetStart(selection, start);
  ok_(__FILE__,line)(hr == S_OK, "SetStart failed: 0x%08lx\n", hr);
  hr = ITextSelection_SetEnd(selection, end);
  ok_(__FILE__,line)(hr == S_OK, "SetEnd failed: 0x%08lx\n", hr);
}

#define CHECK_SELECTION(selection,expected_start,expected_end) \
  _check_selection(selection, expected_start, expected_end, __LINE__)
static void _check_selection(ITextSelection *selection, LONG expected_start, LONG expected_end, int line)
{
  HRESULT hr;
  LONG value;

  hr = ITextSelection_GetStart(selection, &value);
  ok_(__FILE__,line)(hr == S_OK, "GetStart failed: 0x%08lx\n", hr);
  ok_(__FILE__,line)(value == expected_start, "Expected start %ld got %ld\n",
                     expected_start, value);
  hr = ITextSelection_GetEnd(selection, &value);
  ok_(__FILE__,line)(hr == S_OK, "GetEnd failed: 0x%08lx\n", hr);
  ok_(__FILE__,line)(value == expected_end, "Expected end %ld got %ld\n",
                     expected_end, value);
}

static void test_ITextRange_SetRange(void)
{
  static const CHAR test_text1[] = "TestSomeText";
  ITextDocument *txtDoc = NULL;
  IRichEditOle *reOle = NULL;
  ITextRange *txtRge = NULL;
  HRESULT hr;
  HWND w;

  create_interfaces(&w, &reOle, &txtDoc, NULL);
  SendMessageA(w, WM_SETTEXT, 0, (LPARAM)test_text1);
  ITextDocument_Range(txtDoc, 0, 0, &txtRge);

  hr = ITextRange_SetRange(txtRge, 2, 4);
  ok(hr == S_OK, "got 0x%08lx.\n", hr);
  CHECK_RANGE(txtRge, 2, 4);

  hr = ITextRange_SetRange(txtRge, 2, 4);
  ok(hr == S_FALSE, "got 0x%08lx.\n", hr);
  CHECK_RANGE(txtRge, 2, 4);

  hr = ITextRange_SetRange(txtRge, 4, 2);
  ok(hr == S_FALSE, "got 0x%08lx.\n", hr);
  CHECK_RANGE(txtRge, 2, 4);

  hr = ITextRange_SetRange(txtRge, 14, 14);
  ok(hr == S_OK, "got 0x%08lx.\n", hr);
  CHECK_RANGE(txtRge, 12, 12);

  hr = ITextRange_SetRange(txtRge, 15, 15);
  ok(hr == S_FALSE, "got 0x%08lx.\n", hr);
  CHECK_RANGE(txtRge, 12, 12);

  hr = ITextRange_SetRange(txtRge, 14, 1);
  ok(hr == S_OK, "got 0x%08lx.\n", hr);
  CHECK_RANGE(txtRge, 1, 13);

  hr = ITextRange_SetRange(txtRge, -1, 4);
  ok(hr == S_OK, "got 0x%08lx.\n", hr);
  CHECK_RANGE(txtRge, 0, 4);

  ITextRange_Release(txtRge);
  release_interfaces(&w, &reOle, &txtDoc, NULL);
}

static void test_Expand(void)
{
  static const char test_text1[] = "TestSomeText";
  IRichEditOle *reole = NULL;
  ITextDocument *doc = NULL;
  ITextSelection *selection;
  ITextRange *range;
  LONG value;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reole, &doc, &selection);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);
  SendMessageA(hwnd, EM_SETSEL, 1, 2);

  hr = ITextDocument_Range(doc, 0, 4, &range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextRange_Expand(range, tomStory, NULL);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  CHECK_RANGE(range, 0, 13);

  hr = ITextSelection_Expand(selection, tomStory, NULL);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  CHECK_SELECTION(selection, 0, 13);

  RESET_RANGE(range, 1, 2);
  RESET_SELECTION(selection, 1, 2);

  value = 0;
  hr = ITextRange_Expand(range, tomStory, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 12, "got %ld\n", value);
  CHECK_RANGE(range, 0, 13);

  value = 0;
  hr = ITextSelection_Expand(selection, tomStory, &value);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(value == 12, "got %ld\n", value);
  CHECK_SELECTION(selection, 0, 13);

  release_interfaces(&hwnd, &reole, &doc, NULL);

  hr = ITextRange_Expand(range, tomStory, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  hr = ITextRange_Expand(range, tomStory, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  hr = ITextSelection_Expand(selection, tomStory, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  hr = ITextSelection_Expand(selection, tomStory, &value);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  ITextSelection_Release(selection);
  ITextRange_Release(range);
}

static void test_MoveEnd_story(void)
{
  static const char test_text1[] = "Word1 Word2";
  IRichEditOle *reole = NULL;
  ITextDocument *doc = NULL;
  ITextSelection *selection;
  ITextRange *range;
  LONG delta;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reole, &doc, &selection);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);
  SendMessageA(hwnd, EM_SETSEL, 1, 2);

  hr = ITextDocument_Range(doc, 1, 2, &range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  hr = ITextRange_MoveEnd(range, tomStory, 0, &delta);
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);
  ok(delta == 0, "got %ld\n", delta);
  CHECK_RANGE(range, 1, 2);

  hr = ITextRange_MoveEnd(range, tomStory, -1, &delta);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(delta == -1, "got %ld\n", delta);
  CHECK_RANGE(range, 0, 0);

  hr = ITextRange_MoveEnd(range, tomStory, 1, &delta);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(delta == 1, "got %ld\n", delta);
  CHECK_RANGE(range, 0, 12);

  hr = ITextRange_MoveEnd(range, tomStory, 1, &delta);
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);
  ok(delta == 0, "got %ld\n", delta);
  CHECK_RANGE(range, 0, 12);

  RESET_RANGE(range, 1, 2);

  hr = ITextRange_MoveEnd(range, tomStory, 3, &delta);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(delta == 1, "got %ld\n", delta);
  CHECK_RANGE(range, 1, 12);

  RESET_RANGE(range, 2, 3);

  hr = ITextRange_MoveEnd(range, tomStory, -3, &delta);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(delta == -1, "got %ld\n", delta);
  CHECK_RANGE(range, 0, 0);

  hr = ITextRange_MoveEnd(range, tomStory, -1, &delta);
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);
  ok(delta == 0, "got %ld\n", delta);
  CHECK_RANGE(range, 0, 0);

  hr = ITextSelection_MoveEnd(selection, tomStory, 0, &delta);
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);
  ok(delta == 0, "got %ld\n", delta);
  CHECK_SELECTION(selection, 1, 2);

  hr = ITextSelection_MoveEnd(selection, tomStory, -1, &delta);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(delta == -1, "got %ld\n", delta);
  CHECK_SELECTION(selection, 0, 0);

  hr = ITextSelection_MoveEnd(selection, tomStory, 1, &delta);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(delta == 1, "got %ld\n", delta);
  CHECK_SELECTION(selection, 0, 12);

  hr = ITextSelection_MoveEnd(selection, tomStory, 1, &delta);
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);
  ok(delta == 0, "got %ld\n", delta);
  CHECK_SELECTION(selection, 0, 12);

  RESET_SELECTION(selection, 1, 2);

  hr = ITextSelection_MoveEnd(selection, tomStory, 3, &delta);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(delta == 1, "got %ld\n", delta);
  CHECK_SELECTION(selection, 1, 12);

  RESET_SELECTION(selection, 2, 3);

  hr = ITextSelection_MoveEnd(selection, tomStory, -3, &delta);
  ok(hr == S_OK, "got 0x%08lx\n", hr);
  ok(delta == -1, "got %ld\n", delta);
  CHECK_SELECTION(selection, 0, 0);

  hr = ITextSelection_MoveEnd(selection, tomStory, -1, &delta);
  ok(hr == S_FALSE, "got 0x%08lx\n", hr);
  ok(delta == 0, "got %ld\n", delta);
  CHECK_SELECTION(selection, 0, 0);

  release_interfaces(&hwnd, &reole, &doc, NULL);

  hr = ITextRange_MoveEnd(range, tomStory, 1, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  hr = ITextRange_MoveEnd(range, tomStory, 1, &delta);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  hr = ITextSelection_MoveEnd(selection, tomStory, 1, NULL);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  hr = ITextSelection_MoveEnd(selection, tomStory, 1, &delta);
  ok(hr == CO_E_RELEASED, "got 0x%08lx\n", hr);

  ITextSelection_Release(selection);
  ITextRange_Release(range);
}

static void test_character_movestart(ITextRange *range, int textlen, int i, int j, LONG target)
{
    HRESULT hr;
    LONG delta = 0;
    LONG expected_delta;
    LONG expected_start = target;

    if (expected_start < 0)
        expected_start = 0;
    else if (expected_start > textlen)
        expected_start = textlen;
    expected_delta = expected_start - i;
    hr = ITextRange_SetRange(range, i, j);
    ok(SUCCEEDED(hr), "got 0x%08lx\n", hr);
    hr = ITextRange_MoveStart(range, tomCharacter, target - i, &delta);
    if (expected_start == i) {
        ok(hr == S_FALSE, "(%d,%d) move by %ld got hr=0x%08lx\n", i, j, target - i, hr);
        ok(delta == 0, "(%d,%d) move by %ld got delta %ld\n", i, j, target - i, delta);
        CHECK_RANGE(range, i, j);
    } else {
        ok(hr == S_OK, "(%d,%d) move by %ld got hr=0x%08lx\n", i, j, target - i, hr);
        ok(delta == expected_delta, "(%d,%d) move by %ld got delta %ld\n", i, j, target - i, delta);
        if (expected_start <= j)
            CHECK_RANGE(range, expected_start, j);
        else
            CHECK_RANGE(range, expected_start, expected_start);
    }
}

static void test_character_moveend(ITextRange *range, int textlen, int i, int j, LONG target)
{
    HRESULT hr;
    LONG delta;
    LONG expected_delta;
    LONG expected_end = target;

    if (expected_end < 0)
        expected_end = 0;
    else if (expected_end > textlen + 1)
        expected_end = textlen + 1;
    expected_delta = expected_end - j;
    hr = ITextRange_SetRange(range, i, j);
    ok(SUCCEEDED(hr), "got 0x%08lx\n", hr);
    hr = ITextRange_MoveEnd(range, tomCharacter, target - j, &delta);
    if (expected_end == j) {
        ok(hr == S_FALSE, "(%d,%d) move by %ld got hr=0x%08lx\n", i, j, target - j, hr);
        ok(delta == 0, "(%d,%d) move by %ld got delta %ld\n", i, j, target - j, delta);
        CHECK_RANGE(range, i, j);
    } else {
        ok(hr == S_OK, "(%d,%d) move by %ld got hr=0x%08lx\n", i, j, target - j, hr);
        ok(delta == expected_delta, "(%d,%d) move by %ld got delta %ld\n", i, j, target - j, delta);
        if (i <= expected_end)
            CHECK_RANGE(range, i, expected_end);
        else
            CHECK_RANGE(range, expected_end, expected_end);
    }
}

static void test_character_move(ITextRange *range, int textlen, int i, int j, LONG target)
{
    HRESULT hr;
    LONG move_by;
    LONG delta = 0;
    LONG expected_delta;
    LONG expected_location = target;

    if (expected_location < 0)
        expected_location = 0;
    else if (expected_location > textlen)
        expected_location = textlen;

    if (target <= i) {
        move_by = target - i;
        expected_delta = expected_location - i;
        if (i != j) {
            --move_by;
            --expected_delta;
        }
    } else if (j <= target) {
        move_by = target - j;
        expected_delta = expected_location - j;
        if (i != j) {
            ++move_by;
            ++expected_delta;
        }
    } else {
        /* There's no way to move to a point between start and end: */
        return;
    }

    hr = ITextRange_SetRange(range, i, j);
    ok(SUCCEEDED(hr), "got 0x%08lx\n", hr);
    hr = ITextRange_Move(range, tomCharacter, move_by, &delta);
    if (expected_delta == 0) {
        ok(hr == S_FALSE, "(%d,%d) move by %ld got hr=0x%08lx\n", i, j, move_by, hr);
        ok(delta == 0, "(%d,%d) move by %ld got delta %ld\n", i, j, move_by, delta);
        CHECK_RANGE(range, expected_location, expected_location);
    } else {
        ok(hr == S_OK, "(%d,%d) move by %ld got hr=0x%08lx\n", i, j, move_by, hr);
        ok(delta == expected_delta, "(%d,%d) move by %ld got delta %ld\n", i, j, move_by, delta);
        CHECK_RANGE(range, expected_location, expected_location);
    }
}

static void test_character_startof(ITextRange *range, int textlen, int i, int j)
{
    HRESULT hr;
    LONG delta;

    hr = ITextRange_SetRange(range, i, j);
    ok(SUCCEEDED(hr), "got 0x%08lx\n", hr);
    hr = ITextRange_StartOf(range, tomCharacter, tomMove, &delta);
    if (i == j) {
        ok(hr == S_FALSE, "(%d,%d) tomMove got hr=0x%08lx\n", i, j, hr);
        ok(delta == 0, "(%d,%d) tomMove got delta %ld\n", i, j, delta);
    } else {
        ok(hr == S_OK, "(%d,%d) tomMove got hr=0x%08lx\n", i, j, hr);
        ok(delta == -1, "(%d,%d) tomMove got delta %ld\n", i, j, delta);
    }
    CHECK_RANGE(range, i, i);

    hr = ITextRange_SetRange(range, i, j);
    ok(SUCCEEDED(hr), "got 0x%08lx\n", hr);
    hr = ITextRange_StartOf(range, tomCharacter, tomExtend, &delta);
    ok(hr == S_FALSE, "(%d,%d) tomExtend got hr=0x%08lx\n", i, j, hr);
    ok(delta == 0, "(%d,%d) tomExtend got delta %ld\n", i, j, delta);
    CHECK_RANGE(range, i, j);
}

static void test_character_endof(ITextRange *range, int textlen, int i, int j)
{
    HRESULT hr;
    LONG end;
    LONG delta;

    hr = ITextRange_SetRange(range, i, j);
    ok(SUCCEEDED(hr), "got 0x%08lx\n", hr);
    hr = ITextRange_EndOf(range, tomCharacter, tomMove, &delta);

    /* A character "end", apparently cannot be before the very first character */
    end = j;
    if (j == 0)
        ++end;

    if (i == end) {
        ok(hr == S_FALSE, "(%d,%d) tomMove got hr=0x%08lx\n", i, j, hr);
        ok(delta == 0, "(%d,%d) tomMove got delta %ld\n", i, j, delta);
    } else {
        ok(hr == S_OK, "(%d,%d) tomMove got hr=0x%08lx\n", i, j, hr);
        ok(delta == 1, "(%d,%d) tomMove got delta %ld\n", i, j, delta);
    }
    CHECK_RANGE(range, end, end);

    hr = ITextRange_SetRange(range, i, j);
    ok(SUCCEEDED(hr), "got 0x%08lx\n", hr);
    hr = ITextRange_EndOf(range, tomCharacter, tomExtend, &delta);
    if (0 < j) {
        ok(hr == S_FALSE, "(%d,%d) tomExtend got hr=0x%08lx\n", i, j, hr);
        ok(delta == 0, "(%d,%d) tomExtend got delta %ld\n", i, j, delta);
    } else {
        ok(hr == S_OK, "(%d,%d) tomExtend got hr=0x%08lx\n", i, j, hr);
        ok(delta == 1, "(%d,%d) tomExtend got delta %ld\n", i, j, delta);
    }
    CHECK_RANGE(range, i, end);
}

static void test_character_movement(void)
{
  static const char test_text1[] = "ab\n c";
  IRichEditOle *reole = NULL;
  ITextDocument *doc = NULL;
  ITextRange *range;
  ITextSelection *selection;
  HRESULT hr;
  HWND hwnd;
  int i, j;
  const int textlen = strlen(test_text1);

  create_interfaces(&hwnd, &reole, &doc, &selection);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)test_text1);

  hr = ITextDocument_Range(doc, 0, 0, &range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  /* Exhaustive test of every possible combination of (start,end) locations,
   * against every possible target location to move to. */
  for (i = 0; i <= textlen; i++) {
      for (j = i; j <= textlen; j++) {
          LONG target;
          for (target = -2; target <= textlen + 3; target++) {
              test_character_moveend(range, textlen, i, j, target);
              test_character_movestart(range, textlen, i, j, target);
              test_character_move(range, textlen, i, j, target);
          }
          test_character_startof(range, textlen, i, j);
          test_character_endof(range, textlen, i, j);
      }
  }

  release_interfaces(&hwnd, &reole, &doc, NULL);
  ITextSelection_Release(selection);
  ITextRange_Release(range);
}

static BOOL open_clipboard(HWND hwnd)
{
    DWORD start = GetTickCount();
    while (1)
    {
        BOOL ret = OpenClipboard(hwnd);
        if (ret || GetLastError() != ERROR_ACCESS_DENIED)
            return ret;
        if (GetTickCount() - start > 100)
        {
            char classname[256];
            DWORD le = GetLastError();
            HWND clipwnd = GetOpenClipboardWindow();
            /* Provide a hint as to the source of interference:
             * - The class name would typically be CLIPBRDWNDCLASS if the
             *   clipboard was opened by a Windows application using the
             *   ole32 API.
             * - And it would be __wine_clipboard_manager if it was opened in
             *   response to a native application.
             */
            GetClassNameA(clipwnd, classname, ARRAY_SIZE(classname));
            trace("%p (%s) opened the clipboard\n", clipwnd, classname);
            SetLastError(le);
            return ret;
        }
        Sleep(15);
    }
}

#define CLIPBOARD_RANGE_CONTAINS(range, start, end, expected) _clipboard_range_contains(range, start, end, expected, __LINE__, 0);
#define TODO_CLIPBOARD_RANGE_CONTAINS(range, start, end, expected) _clipboard_range_contains(range, start, end, expected, __LINE__, 1);
static void _clipboard_range_contains(ITextRange *range, LONG start, LONG end, const char *expected, int line, int todo)
{
  HRESULT hr;
  BOOL clipboard_open;
  HGLOBAL global;
  const char *clipboard_text;

  hr = ITextRange_SetRange(range, start, end);
  ok_(__FILE__,line)(SUCCEEDED(hr), "SetRange failed: 0x%08lx\n", hr);
  hr = ITextRange_Copy(range, NULL);
  ok_(__FILE__,line)(hr == S_OK, "Copy failed: 0x%08lx\n", hr);

  clipboard_open = open_clipboard(NULL);
  ok_(__FILE__,line)(clipboard_open, "OpenClipboard failed: %ld\n", GetLastError());
  global = GetClipboardData(CF_TEXT);
  ok_(__FILE__,line)(global != NULL, "GetClipboardData failed: %p\n", global);
  clipboard_text = GlobalLock(global);
  ok_(__FILE__,line)(clipboard_text != NULL, "GlobalLock failed: %p\n", clipboard_text);
#ifdef __REACTOS__
  if (expected != NULL && clipboard_text != NULL)
    todo_wine_if(todo) ok_(__FILE__,line)(!strcmp(expected, clipboard_text), "unexpected contents: %s\n", wine_dbgstr_a(clipboard_text));
  else
    todo_wine_if(todo) ok_(__FILE__,line)(FALSE, "Either 'expected' or 'clipboard_text' was NULL\n");
#else
  todo_wine_if(todo) ok_(__FILE__,line)(!strcmp(expected, clipboard_text), "unexpected contents: %s\n", wine_dbgstr_a(clipboard_text));
#endif
  GlobalUnlock(global);
  CloseClipboard();
}

static void test_clipboard(void)
{
  static const char text_in[] = "ab\n c";
  IRichEditOle *reole = NULL;
  ITextDocument *doc = NULL;
  ITextRange *range;
  ITextSelection *selection;
  HRESULT hr;
  HWND hwnd;

  create_interfaces(&hwnd, &reole, &doc, &selection);
  SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)text_in);

  hr = ITextDocument_Range(doc, 0, 0, &range);
  ok(hr == S_OK, "got 0x%08lx\n", hr);

  CLIPBOARD_RANGE_CONTAINS(range, 0, 5, "ab\r\n c")
  CLIPBOARD_RANGE_CONTAINS(range, 0, 0, "ab\r\n c")
  CLIPBOARD_RANGE_CONTAINS(range, 1, 1, "ab\r\n c")
  CLIPBOARD_RANGE_CONTAINS(range, 0, 1, "a")
  CLIPBOARD_RANGE_CONTAINS(range, 5, 6, "")

  /* Setting password char does not stop Copy */
  SendMessageA(hwnd, EM_SETPASSWORDCHAR, '*', 0);
  CLIPBOARD_RANGE_CONTAINS(range, 0, 1, "a")

  /* Cut can be undone */
  hr = ITextRange_SetRange(range, 0, 1);
  ok(SUCCEEDED(hr), "SetRange failed: 0x%08lx\n", hr);
  hr = ITextRange_Cut(range, NULL);
  ok(hr == S_OK, "Cut failed: 0x%08lx\n", hr);
  CLIPBOARD_RANGE_CONTAINS(range, 0, 4, "b\r\n c");
  hr = ITextDocument_Undo(doc, 1, NULL);
  ok(hr == S_OK, "Undo failed: 0x%08lx\n", hr);
  CLIPBOARD_RANGE_CONTAINS(range, 0, 5, "ab\r\n c");

  /* Cannot cut when read-only */
  SendMessageA(hwnd, EM_SETREADONLY, TRUE, 0);
  hr = ITextRange_SetRange(range, 0, 1);
  ok(SUCCEEDED(hr), "SetRange failed: 0x%08lx\n", hr);
  hr = ITextRange_Cut(range, NULL);
  ok(hr == E_ACCESSDENIED, "got 0x%08lx\n", hr);

  release_interfaces(&hwnd, &reole, &doc, NULL);
  ITextSelection_Release(selection);
  ITextRange_Release(range);
}

static void subtest_undo(const char *dummy_text)
{
  static const char *text_seq[] = {
    "",
    "1-alpha",
    "2-beta",
    "3-gamma",
    "4-delta",
    "5-epsilon",
    "6-zeta",
    "7-eta",
  };
  static LONG seq[] = { -1, -2, -3, -1, 1, 2, 3, 1, -5, 2, -1, 3, 1, 0 };
  LONG i = 0, stack_pos = 0;
  IRichEditOle *reole = NULL;
  ITextDocument *doc = NULL;
  ITextSelection *selection;
  char buffer[1024] = "";
  HRESULT hr;
  HWND hwnd;
  LONG count = 0;

  winetest_push_context("(%Iu)", dummy_text ? strlen(dummy_text) : 0);

  create_interfaces(&hwnd, &reole, &doc, &selection);

  for (i = -2; i <= 2; i++)
  {
    if (i != tomFalse && i != tomTrue)
    {
      hr = ITextDocument_Undo(doc, i, NULL);
      todo_wine_if(i >= 1)
      ok(hr == (i >= 1 ? S_OK : S_FALSE), "(%ld@0) Undo: %#lx\n", i, hr);

      count = 0xcccccccc;
      hr = ITextDocument_Undo(doc, i, &count);
      todo_wine_if(i >= 1)
      ok(hr == (i >= 1 ? S_OK : S_FALSE), "(%ld@0) Undo: %#lx\n", i, hr);
      todo_wine_if(i >= 1)
      ok(count == (i >= 1 ? i : 0), "(%ld@0) Expected %ld, got %ld\n", i, i >= 0 ? i : 0, count);
    }

    hr = ITextDocument_Redo(doc, i, NULL);
    ok(hr == (i == 0 ? S_OK : S_FALSE), "(%ld@0) Redo: %#lx\n", i, hr);

    count = 0xcccccccc;
    hr = ITextDocument_Redo(doc, i, &count);
    ok(hr == (i == 0 ? S_OK : S_FALSE), "(%ld@0) Redo: %#lx\n", i, hr);
    ok(count == 0, "(%ld@0) got %ld\n", i, count);
  }

  while (stack_pos < ARRAY_SIZE(text_seq) - 1)
  {
    stack_pos++;
    if (dummy_text)
    {
      hr = ITextDocument_Undo(doc, tomSuspend, NULL);
      ok(hr == S_FALSE, "(@%ld) Undo: %#lx\n", stack_pos, hr);
      if (SUCCEEDED(hr))
      {
        SendMessageA(hwnd, EM_SETSEL, 0, 0);
        SendMessageA(hwnd, EM_REPLACESEL, TRUE, (LPARAM)dummy_text);
        SendMessageA(hwnd, EM_SETSEL, 0, strlen(dummy_text));
        SendMessageA(hwnd, EM_REPLACESEL, TRUE, (LPARAM)"");
        hr = ITextDocument_Undo(doc, tomResume, NULL);
        ok(hr == S_FALSE, "(@%ld) Undo: %#lx\n", stack_pos, hr);
      }
    }
    SendMessageA(hwnd, EM_SETSEL, 0, -1);
    SendMessageA(hwnd, EM_REPLACESEL, TRUE, (LPARAM)text_seq[stack_pos]);
  }

  for (i = 0; i < ARRAY_SIZE(seq); i++)
  {
    LONG expect_count;

    memset(buffer, 0, sizeof(buffer));
    SendMessageA(hwnd, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    ok(strcmp(buffer, text_seq[stack_pos]) == 0, "Expected %s, got %s\n",
       wine_dbgstr_a(text_seq[stack_pos]), wine_dbgstr_a(buffer));

    if (!seq[i]) break;

    count = 0xcccccccc;
    expect_count = labs(stack_pos - min(max(stack_pos + seq[i], 0), (LONG)ARRAY_SIZE(seq) - 1));
    if (seq[i] < 0)
    {
      hr = ITextDocument_Undo(doc, -seq[i], &count);
      ok(hr == S_OK, "(%ld@%ld) Undo: %#lx\n", i, stack_pos, hr);
      ok(count == expect_count, "(%ld@%ld) Expected %ld, got %ld\n", i, stack_pos, expect_count, count);
      stack_pos -= count;
    }
    else
    {
      hr = ITextDocument_Redo(doc, seq[i], &count);
      ok(hr == (expect_count ? S_OK : S_FALSE), "(%ld@%ld) Redo: %#lx\n", i, stack_pos, hr);
      ok(count == expect_count, "(%ld@%ld) Expected %ld, got %ld\n", i, stack_pos, expect_count, count);
      stack_pos += count;
    }

    if (FAILED(hr) || count <= 0) break;
  }

  release_interfaces(&hwnd, &reole, &doc, &selection);
  winetest_pop_context();
}

static void test_undo(void)
{
  subtest_undo(NULL);
  subtest_undo("dummy 12345");
}

#define ok_msg_result(h,m,w,l,r) ok_msg_result_(__LINE__,#m,h,m,w,l,r)
static void ok_msg_result_(int line, const char *desc, HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, LRESULT expect)
{
  LRESULT lresult = SendMessageA(hwnd, message, wparam, lparam);
  ok_(__FILE__,line)(lresult == expect, "%s: Expected %Id, got %Id\n", desc, expect, lresult);
}

enum editorUndoState {
  firstUndoState = 0,
  undoStateActive = 0,
  undoStateSuspended = 1,
  undoStateDisabled = 2,
  numUndoStates = 3,
};

enum editorUndoStateAction {
  firstUndoAction = 0,
  undoActionNoOp = 0,
  undoActionEnable = 1,
  undoActionDisable = 2,
  undoActionSuspend = 3,
  undoActionResume = 4,
  numUndoActions = 5
};

enum editorUndoStateTestFlags {
  undoTestUseWindowMessages = 0x1,
  undoTestResetUndoLimit = 0x2,
  undoTestDoFirstUndo = 0x4,
  undoTestDoFirstRedo = 0x8,
  undoTestDoSecondUndoAfterEnable = 0x10,
  undoTestMaxFlag = 0x20,
};

struct undo_test
{
  HWND hwnd;
  ITextDocument *doc;
  int test_flags;
  enum editorUndoState undo_ctl_state;
  LONG_PTR undo_limit;
  BOOL last_undo_status;
  BOOL last_redo_status;
};

static HRESULT perform_editor_undo_state_action(struct undo_test *inst, enum editorUndoStateAction action, LONG *count)
{
  HRESULT hr = S_OK;

  if (count) *count = 0xcccccccc;

  switch (action)
  {
    case undoActionNoOp:
      if (count) *count = 0;
      break;
    case undoActionEnable:
      if (inst->test_flags & undoTestResetUndoLimit)
      {
        LONG_PTR cur_undo_limit = SendMessageA(inst->hwnd, EM_SETUNDOLIMIT, inst->undo_limit, 0);
        ok(cur_undo_limit == inst->undo_limit, "Expected undo limit %Id, got %Id\n",
           inst->undo_limit, cur_undo_limit);
        if (count) *count = 0;
      }
      else
      {
        hr = ITextDocument_Undo(inst->doc, tomTrue, count);
        ok(hr == S_FALSE, "Undo: %#lx\n", hr);
      }
      if (SUCCEEDED(hr))
      {
        if (inst->undo_ctl_state == undoStateDisabled)
        {
          inst->undo_ctl_state = undoStateActive;
        }
        inst->last_undo_status = TRUE;
      }
      break;
    case undoActionDisable:
      hr = ITextDocument_Undo(inst->doc, tomFalse, count);
      ok(hr == S_OK, "Undo: %#lx\n", hr);
      if (SUCCEEDED(hr))
      {
        inst->undo_ctl_state = undoStateDisabled;
        inst->last_undo_status = FALSE;
        inst->last_redo_status = FALSE;
      }
      break;
    case undoActionSuspend:
      hr = ITextDocument_Undo(inst->doc, tomSuspend, count);
      ok(hr == S_FALSE, "Undo: %#lx\n", hr);
      if (SUCCEEDED(hr) && inst->undo_ctl_state == undoStateActive)
      {
        inst->undo_ctl_state = undoStateSuspended;
      }
      break;
    case undoActionResume:
      hr = ITextDocument_Undo(inst->doc, tomResume, count);
      ok(hr == S_FALSE, "Undo: %#lx\n", hr);
      if (SUCCEEDED(hr))
      {
        inst->undo_ctl_state = undoStateActive;
      }
      break;
    default:
      ok(0, "unreachable\n");
      break;
  }

  if (count)
  {
    ok(*count == 0, "Got %ld\n", *count);
  }
  return hr;
}

static HRESULT set_editor_undo_state(struct undo_test *inst, enum editorUndoState state)
{
  HRESULT hr = S_OK;
  if (inst->undo_ctl_state == state) return hr;
  switch (state)
  {
    case undoStateActive:
      if (FAILED(hr = perform_editor_undo_state_action(inst, undoActionEnable, NULL))) break;
      if (FAILED(hr = perform_editor_undo_state_action(inst, undoActionResume, NULL))) break;
      break;
    case undoStateSuspended:
      if (FAILED(hr = perform_editor_undo_state_action(inst, undoActionEnable, NULL))) break;
      if (FAILED(hr = perform_editor_undo_state_action(inst, undoActionSuspend, NULL))) break;
      break;
    case undoStateDisabled:
      if (FAILED(hr = perform_editor_undo_state_action(inst, undoActionDisable, NULL))) break;
      break;
    default:
      ok(0, "unreachable\n");
      break;
  }
  ok(inst->undo_ctl_state == state, "expected state %d, got %d\n", state, inst->undo_ctl_state);
  ok(SUCCEEDED(hr), "cannot set state to %d: %#lx\n", undoStateActive, hr);
  return hr;
}

#define perform_undo(i,c) perform_undo_(i,c,__LINE__)
static BOOL perform_undo_(struct undo_test *inst, BOOL can_undo, int line)
{
  LONG count;
  HRESULT hr;
  BOOL result, expect;

  if (inst->test_flags & undoTestUseWindowMessages)
  {
    LRESULT lres = SendMessageA(inst->hwnd, EM_UNDO, 0, 0);
    ok_(__FILE__, line)(lres == FALSE || lres == TRUE, "unexpected LRESULT %#Ix\n", lres);
    result = lres;
  }
  else
  {
    count = 0xcccccccc;
    hr = ITextDocument_Undo(inst->doc, 1, &count);
    ok_(__FILE__, line)(SUCCEEDED(hr), "got hr %#lx\n", hr);
    ok_(__FILE__, line)(count == (hr == S_OK), "expected count %d, got %ld\n", hr == S_OK, count);
    result = hr == S_OK && count > 0;
  }

  expect = FALSE;
  if (inst->undo_ctl_state == undoStateActive)
  {
    if (can_undo) inst->last_undo_status = TRUE;
    expect = inst->last_undo_status;
  }
  todo_wine_if(!can_undo && !result && expect)
  ok_(__FILE__, line)(result == expect, "state %d: expected %d, got %d\n", inst->undo_ctl_state, expect, result);

  return can_undo && result;
}

#define perform_redo(i,c) perform_redo_(i,c,__LINE__)
static BOOL perform_redo_(struct undo_test *inst, BOOL can_redo, int line)
{
  LONG count;
  HRESULT hr;
  BOOL result, expect;

  if (inst->test_flags & undoTestUseWindowMessages)
  {
    LRESULT lres = SendMessageA(inst->hwnd, EM_REDO, 0, 0);
    ok_(__FILE__, line)(lres == FALSE || lres == TRUE, "unexpected LRESULT %#Ix\n", lres);
    result = lres;
  }
  else
  {
    count = 0xcccccccc;
    hr = ITextDocument_Redo(inst->doc, 1, &count);
    ok_(__FILE__, line)(SUCCEEDED(hr), "got hr %#lx\n", hr);
    ok_(__FILE__, line)(count == (hr == S_OK), "expected count %d, got %ld\n", hr == S_OK, count);
    result = hr == S_OK && count > 0;
  }

  expect = FALSE;
  if (inst->undo_ctl_state == undoStateActive)
  {
    if (can_redo) inst->last_redo_status = TRUE;
    expect = inst->last_redo_status;
  }
  todo_wine_if(!can_redo && !result && expect)
  ok_(__FILE__, line)(result == expect, "state %d: expected %d, got %d\n", inst->undo_ctl_state, expect, result);

  return can_redo && result;
}

static HRESULT subtest_undo_control(struct undo_test *inst, enum editorUndoStateAction action)
{
  LONG undo_count, redo_count, count;
  static const char text_foo[] = "foo";
  static const char text_bar[] = "bar";
  static const char *last_text, *last_text2;
  char buffer[1024] = "";
  HRESULT hr;

  SendMessageA(inst->hwnd, EM_EMPTYUNDOBUFFER, 0, 0);
  undo_count = redo_count = 0;
  ok_msg_result(inst->hwnd, EM_CANUNDO, 0, 0, undo_count > 0);
  ok_msg_result(inst->hwnd, EM_CANREDO, 0, 0, redo_count > 0);

  SendMessageA(inst->hwnd, WM_SETTEXT, 0, (LPARAM)(last_text = ""));
  SendMessageA(inst->hwnd, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
  last_text = "";
  ok(strcmp(buffer, last_text) == 0,
     "Expected %s, got %s\n", wine_dbgstr_a(""), wine_dbgstr_a(buffer));

  SendMessageA(inst->hwnd, EM_SETSEL, 0, -1);
  SendMessageA(inst->hwnd, EM_REPLACESEL, TRUE, (LPARAM)(last_text = text_foo));
  if (inst->undo_ctl_state == undoStateActive) undo_count++, redo_count = 0;
  ok_msg_result(inst->hwnd, EM_CANUNDO, 0, 0, undo_count > 0);
  ok_msg_result(inst->hwnd, EM_CANREDO, 0, 0, redo_count > 0);
  SendMessageA(inst->hwnd, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
  ok(strcmp(buffer, last_text) == 0, "Expected %s, got %s\n", wine_dbgstr_a(text_foo), wine_dbgstr_a(buffer));

  hr = perform_editor_undo_state_action(inst, action, &count);
  ok(SUCCEEDED(hr), "got %#lx\n", hr);
  if (FAILED(hr)) return hr;
  if (inst->undo_ctl_state == undoStateDisabled) undo_count = redo_count = 0;
  ok_msg_result(inst->hwnd, EM_CANUNDO, 0, 0, undo_count > 0);
  ok_msg_result(inst->hwnd, EM_CANREDO, 0, 0, redo_count > 0);

  if (inst->test_flags & undoTestDoFirstUndo)
  {
    if (perform_undo(inst, undo_count > 0)) undo_count--, redo_count++, last_text = "";
    ok_msg_result(inst->hwnd, EM_CANUNDO, 0, 0, undo_count > 0);
    ok_msg_result(inst->hwnd, EM_CANREDO, 0, 0, redo_count > 0);
    SendMessageA(inst->hwnd, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    ok(strcmp(buffer, last_text) == 0, "Expected %s, got %s\n", wine_dbgstr_a(last_text), wine_dbgstr_a(buffer));
  }

  if (inst->test_flags & undoTestDoSecondUndoAfterEnable)
  {
    hr = perform_editor_undo_state_action(inst, undoActionEnable, &count);
    ok(SUCCEEDED(hr), "got %#lx\n", hr);
    if (FAILED(hr)) return hr;
    ok_msg_result(inst->hwnd, EM_CANUNDO, 0, 0, undo_count > 0);
    ok_msg_result(inst->hwnd, EM_CANREDO, 0, 0, redo_count > 0);

    if (perform_undo(inst, undo_count > 0)) undo_count--, redo_count++, last_text = "";
    ok_msg_result(inst->hwnd, EM_CANUNDO, 0, 0, undo_count > 0);
    ok_msg_result(inst->hwnd, EM_CANREDO, 0, 0, redo_count > 0);
    SendMessageA(inst->hwnd, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    ok(strcmp(buffer, last_text) == 0, "Expected %s, got %s\n", wine_dbgstr_a(last_text), wine_dbgstr_a(buffer));
  }

  if (inst->test_flags & undoTestDoFirstRedo)
  {
    if (perform_redo(inst, redo_count > 0)) undo_count++, redo_count--, last_text = text_foo;
    ok_msg_result(inst->hwnd, EM_CANUNDO, 0, 0, undo_count > 0);
    ok_msg_result(inst->hwnd, EM_CANREDO, 0, 0, redo_count > 0);
    SendMessageA(inst->hwnd, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
    ok(strcmp(buffer, last_text) == 0, "Expected %s, got %s\n", wine_dbgstr_a(last_text), wine_dbgstr_a(buffer));
  }

  SendMessageA(inst->hwnd, EM_SETSEL, 0, -1);
  SendMessageA(inst->hwnd, EM_REPLACESEL, TRUE, (LPARAM)(last_text2 = text_bar));
  if (inst->undo_ctl_state == undoStateActive) undo_count++, redo_count = 0;
  ok_msg_result(inst->hwnd, EM_CANUNDO, 0, 0, undo_count > 0);
  ok_msg_result(inst->hwnd, EM_CANREDO, 0, 0, redo_count > 0);
  SendMessageA(inst->hwnd, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
  ok(strcmp(buffer, last_text2) == 0, "Expected %s, got %s\n", wine_dbgstr_a(last_text2), wine_dbgstr_a(buffer));

  if (perform_undo(inst, undo_count > 0)) undo_count--, redo_count++, last_text2 = last_text;
  ok_msg_result(inst->hwnd, EM_CANUNDO, 0, 0, undo_count > 0);
  ok_msg_result(inst->hwnd, EM_CANREDO, 0, 0, redo_count > 0);
  SendMessageA(inst->hwnd, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
  ok(strcmp(buffer, last_text2) == 0, "Expected %s, got %s\n", wine_dbgstr_a(last_text2), wine_dbgstr_a(buffer));

  if (perform_redo(inst, redo_count > 0)) undo_count++, redo_count--, last_text2 = text_bar;
  ok_msg_result(inst->hwnd, EM_CANUNDO, 0, 0, undo_count > 0);
  ok_msg_result(inst->hwnd, EM_CANREDO, 0, 0, redo_count > 0);
  SendMessageA(inst->hwnd, WM_GETTEXT, ARRAY_SIZE(buffer), (LPARAM)buffer);
  ok(strcmp(buffer, last_text2) == 0, "Expected %s, got %s\n", wine_dbgstr_a(last_text2), wine_dbgstr_a(buffer));

  return S_OK;
}

static void test_undo_control(void)
{
  enum editorUndoState state0;
  enum editorUndoStateAction action0, action1;
  IRichEditOle *reole = NULL;
  ITextSelection *selection;
  struct undo_test inst = { NULL };
  HRESULT hr;

  create_interfaces(&inst.hwnd, &reole, &inst.doc, &selection);
  inst.undo_ctl_state = undoStateActive;
  inst.last_undo_status = TRUE;
  inst.last_redo_status = TRUE;
  inst.undo_limit = SendMessageA(inst.hwnd, EM_SETUNDOLIMIT, 100, 0);
  ok(inst.undo_limit >= 1, "Message EM_SETUNDOLIMIT returned %#Ix\n", inst.undo_limit);

  if (SUCCEEDED(ITextDocument_Undo(inst.doc, 1, NULL)))
  {
    for (inst.test_flags = 0; inst.test_flags < undoTestMaxFlag; inst.test_flags++)
    {
      for (state0 = firstUndoState; state0 < numUndoStates; state0++)
      {
        for (action0 = firstUndoAction; action0 < numUndoActions; action0++)
        {
          for (action1 = firstUndoAction; action1 < numUndoActions; action1++)
          {
            winetest_push_context("%x:%d:%d >?:%d", inst.test_flags, state0, action0, action1);
            hr = set_editor_undo_state(&inst, state0);
            winetest_pop_context();

            if (FAILED(hr)) continue;

            winetest_push_context("%x:%d:%d+>?:%d", inst.test_flags, state0, action0, action1);
            hr = subtest_undo_control(&inst, action0);
            winetest_pop_context();

            if (FAILED(hr)) continue;

            winetest_push_context("%x:%d:%d>%d:%d+", inst.test_flags, state0, action0, inst.undo_ctl_state, action1);
            subtest_undo_control(&inst, action1);
            winetest_pop_context();
          }
        }
      }
    }
  }

  release_interfaces(&inst.hwnd, &reole, &inst.doc, &selection);
}

static void test_freeze(void)
{
  ITextSelection *selection = NULL;
  DWORD lasterr, style1, style2;
  IRichEditOle *reole = NULL;
  ITextDocument *doc = NULL;
  HRESULT hr;
  LONG count;
  HWND hwnd;

  create_interfaces(&hwnd, &reole, &doc, &selection);

  SetLastError(0xdeadbeef);
  style1 = GetWindowLongW(hwnd, GWL_STYLE);
  lasterr = GetLastError();
  ok(lasterr == 0xdeadbeefUL, "GetLastError() returned %#lx\n", lasterr);

  count = 0xdeadbeef;
  hr = ITextDocument_Freeze(doc, &count);
  ok(hr == S_OK, "ITextDocument_Freeze returned %#lx\n", hr);
  ok(count == 1, "expected count to be %d, got %ld\n", 1, count);

  style2 = GetWindowLongW(hwnd, GWL_STYLE);
  ok(style2 == style1, "expected window style to not change from %#lx, got %#lx\n", style1, style2);

  count = 0xdeadbeef;
  hr = ITextDocument_Freeze(doc, &count);
  ok(hr == S_OK, "ITextDocument_Freeze returned %#lx\n", hr);
  ok(count == 2, "expected count to be %d, got %ld\n", 2, count);

  count = 0xdeadbeef;
  hr = ITextDocument_Unfreeze(doc, &count);
  ok(hr == S_FALSE, "ITextDocument_Unfreeze returned %#lx\n", hr);
  ok(count == 1, "expected count to be %d, got %ld\n", 1, count);

  count = 0xdeadbeef;
  hr = ITextDocument_Unfreeze(doc, &count);
  ok(hr == S_OK, "ITextDocument_Unfreeze returned %#lx\n", hr);
  ok(count == 0, "expected count to be %d, got %ld\n", 0, count);

  count = 0xdeadbeef;
  hr = ITextDocument_Unfreeze(doc, &count);
  ok(hr == S_OK, "ITextDocument_Unfreeze returned %#lx\n", hr);
  ok(count == 0, "expected count to be %d, got %ld\n", 0, count);

  count = 0xdeadbeef;
  hr = ITextDocument_Freeze(doc, &count);
  ok(hr == S_OK, "ITextDocument_Freeze returned %#lx\n", hr);
  ok(count == 1, "expected count to be %d, got %ld\n", 1, count);

  count = 0xdeadbeef;
  hr = ITextDocument_Unfreeze(doc, &count);
  ok(hr == S_OK, "ITextDocument_Unfreeze returned %#lx\n", hr);
  ok(count == 0, "expected count to be %d, got %ld\n", 0, count);

  count = 0xdeadbeef;
  hr = ITextDocument_Freeze(doc, NULL);
  ok(hr == S_OK, "ITextDocument_Freeze returned %#lx\n", hr);

  count = 0xdeadbeef;
  hr = ITextDocument_Unfreeze(doc, NULL);
  ok(hr == S_OK, "ITextDocument_Unfreeze returned %#lx\n", hr);

  release_interfaces(&hwnd, &reole, &doc, &selection);
}

START_TEST(richole)
{
  /* Must explicitly LoadLibrary(). The test has no references to functions in
   * RICHED20.DLL, so the linker doesn't actually link to it. */
  hmoduleRichEdit = LoadLibraryA("riched20.dll");
  ok(hmoduleRichEdit != NULL, "error: %d\n", (int) GetLastError());

  test_Interfaces();
  test_ITextDocument_Open();
  test_GetText();
  test_ITextSelection_GetChar();
  test_ITextSelection_GetStart_GetEnd();
  test_ITextSelection_Collapse();
  test_ITextDocument_Range();
  test_ITextRange_GetChar();
  test_ITextRange_ScrollIntoView();
  test_ITextRange_GetStart_GetEnd();
  test_ITextRange_SetRange();
  test_ITextRange_GetDuplicate();
  test_ITextRange_Collapse();
  test_GetClientSite();
  test_IOleWindow_GetWindow();
  test_IOleInPlaceSite_GetWindow();
  test_GetFont();
  test_GetPara();
  test_dispatch();
  test_ITextFont();
  test_Delete();
  test_SetText();
  test_InRange();
  test_ITextRange_IsEqual();
  test_Select();
  test_GetStoryType();
  test_SetFont();
  test_InsertObject();
  test_GetStoryLength();
  test_ITextSelection_GetDuplicate();
  test_Expand();
  test_MoveEnd_story();
  test_character_movement();
  test_clipboard();
  test_undo();
  test_undo_control();
  test_freeze();
}
