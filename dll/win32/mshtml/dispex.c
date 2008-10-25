/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    DISPID id;
    BSTR name;
    tid_t tid;
} func_info_t;

struct dispex_data_t {
    DWORD func_cnt;
    func_info_t *funcs;
    func_info_t **name_table;

    struct list entry;
};

typedef struct {
    VARIANT var;
    LPWSTR name;
} dynamic_prop_t;

struct dispex_dynamic_data_t {
    DWORD buf_size;
    DWORD prop_cnt;
    dynamic_prop_t *props;
};

#define DISPID_DYNPROP_0    0x50000000
#define DISPID_DYNPROP_MAX  0x5fffffff

static ITypeLib *typelib;
static ITypeInfo *typeinfos[LAST_tid];
static struct list dispex_data_list = LIST_INIT(dispex_data_list);

static REFIID tid_ids[] = {
    &IID_NULL,
    &DIID_DispDOMChildrenCollection,
    &DIID_DispHTMLBody,
    &DIID_DispHTMLCommentElement,
    &DIID_DispHTMLDocument,
    &DIID_DispHTMLDOMTextNode,
    &DIID_DispHTMLElementCollection,
    &DIID_DispHTMLGenericElement,
    &DIID_DispHTMLImg,
    &DIID_DispHTMLInputElement,
    &DIID_DispHTMLOptionElement,
    &DIID_DispHTMLSelectElement,
    &DIID_DispHTMLStyle,
    &DIID_DispHTMLUnknownElement,
    &DIID_DispHTMLWindow2,
    &IID_IHTMLBodyElement,
    &IID_IHTMLBodyElement2,
    &IID_IHTMLCommentElement,
    &IID_IHTMLControlElement,
    &IID_IHTMLDocument2,
    &IID_IHTMLDocument3,
    &IID_IHTMLDocument4,
    &IID_IHTMLDocument5,
    &IID_IHTMLDOMChildrenCollection,
    &IID_IHTMLDOMNode,
    &IID_IHTMLDOMNode2,
    &IID_IHTMLDOMTextNode,
    &IID_IHTMLElement,
    &IID_IHTMLElement2,
    &IID_IHTMLElement3,
    &IID_IHTMLElement4,
    &IID_IHTMLElementCollection,
    &IID_IHTMLGenericElement,
    &IID_IHTMLImgElement,
    &IID_IHTMLInputElement,
    &IID_IHTMLOptionElement,
    &IID_IHTMLSelectElement,
    &IID_IHTMLStyle,
    &IID_IHTMLTextContainer,
    &IID_IHTMLUniqueName,
    &IID_IHTMLWindow2,
    &IID_IHTMLWindow3,
    &IID_IOmNavigator
};

HRESULT get_typeinfo(tid_t tid, ITypeInfo **typeinfo)
{
    HRESULT hres;

    if(!typelib) {
        ITypeLib *tl;

        hres = LoadRegTypeLib(&LIBID_MSHTML, 4, 0, LOCALE_SYSTEM_DEFAULT, &tl);
        if(FAILED(hres)) {
            ERR("LoadRegTypeLib failed: %08x\n", hres);
            return hres;
        }

        if(InterlockedCompareExchangePointer((void**)&typelib, tl, NULL))
            ITypeLib_Release(tl);
    }

    if(!typeinfos[tid]) {
        ITypeInfo *typeinfo;

        hres = ITypeLib_GetTypeInfoOfGuid(typelib, tid_ids[tid], &typeinfo);
        if(FAILED(hres)) {
            ERR("GetTypeInfoOfGuid(%s) failed: %08x\n", debugstr_guid(tid_ids[tid]), hres);
            return hres;
        }

        if(InterlockedCompareExchangePointer((void**)(typeinfos+tid), typeinfo, NULL))
            ITypeInfo_Release(typeinfo);
    }

    *typeinfo = typeinfos[tid];
    return S_OK;
}

void release_typelib(void)
{
    dispex_data_t *iter;
    unsigned i;

    while(!list_empty(&dispex_data_list)) {
        iter = LIST_ENTRY(list_head(&dispex_data_list), dispex_data_t, entry);
        list_remove(&iter->entry);

        for(i=0; i < iter->func_cnt; i++)
            SysFreeString(iter->funcs[i].name);

        heap_free(iter->funcs);
        heap_free(iter->name_table);
        heap_free(iter);
    }

    if(!typelib)
        return;

    for(i=0; i < sizeof(typeinfos)/sizeof(*typeinfos); i++)
        if(typeinfos[i])
            ITypeInfo_Release(typeinfos[i]);

    ITypeLib_Release(typelib);
}

static void add_func_info(dispex_data_t *data, DWORD *size, tid_t tid, DISPID id, ITypeInfo *dti)
{
    HRESULT hres;

    if(data->func_cnt && data->funcs[data->func_cnt-1].id == id)
        return;

    if(data->func_cnt == *size)
        data->funcs = heap_realloc(data->funcs, (*size <<= 1)*sizeof(func_info_t));

    hres = ITypeInfo_GetDocumentation(dti, id, &data->funcs[data->func_cnt].name, NULL, NULL, NULL);
    if(FAILED(hres))
        return;

    data->funcs[data->func_cnt].id = id;
    data->funcs[data->func_cnt].tid = tid;

    data->func_cnt++;
}

static int dispid_cmp(const void *p1, const void *p2)
{
    return ((func_info_t*)p1)->id - ((func_info_t*)p2)->id;
}

static int func_name_cmp(const void *p1, const void *p2)
{
    return strcmpiW((*(func_info_t**)p1)->name, (*(func_info_t**)p2)->name);
}

static dispex_data_t *preprocess_dispex_data(DispatchEx *This)
{
    const tid_t *tid = This->data->iface_tids;
    FUNCDESC *funcdesc;
    dispex_data_t *data;
    DWORD size = 16, i;
    ITypeInfo *ti, *dti;
    HRESULT hres;

    TRACE("(%p)\n", This);

    hres = get_typeinfo(This->data->disp_tid, &dti);
    if(FAILED(hres)) {
        ERR("Could not get disp type info: %08x\n", hres);
        return NULL;
    }

    data = heap_alloc(sizeof(dispex_data_t));
    data->func_cnt = 0;
    data->funcs = heap_alloc(size*sizeof(func_info_t));
    list_add_tail(&dispex_data_list, &data->entry);

    while(*tid) {
        hres = get_typeinfo(*tid, &ti);
        if(FAILED(hres))
            break;

        i=7;
        while(1) {
            hres = ITypeInfo_GetFuncDesc(ti, i++, &funcdesc);
            if(FAILED(hres))
                break;

            add_func_info(data, &size, *tid, funcdesc->memid, dti);
            ITypeInfo_ReleaseFuncDesc(ti, funcdesc);
        }

        tid++;
    }

    if(!data->func_cnt) {
        heap_free(data->funcs);
        data->funcs = NULL;
    }else if(data->func_cnt != size) {
        data->funcs = heap_realloc(data->funcs, data->func_cnt * sizeof(func_info_t));
    }

    qsort(data->funcs, data->func_cnt, sizeof(func_info_t), dispid_cmp);

    if(data->funcs) {
        data->name_table = heap_alloc(data->func_cnt * sizeof(func_info_t*));
        for(i=0; i < data->func_cnt; i++)
            data->name_table[i] = data->funcs+i;
        qsort(data->name_table, data->func_cnt, sizeof(func_info_t*), func_name_cmp);
    }else {
        data->name_table = NULL;
    }

    return data;
}

static CRITICAL_SECTION cs_dispex_static_data;
static CRITICAL_SECTION_DEBUG cs_dispex_static_data_dbg =
{
    0, 0, &cs_dispex_static_data,
    { &cs_dispex_static_data_dbg.ProcessLocksList, &cs_dispex_static_data_dbg.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": dispex_static_data") }
};
static CRITICAL_SECTION cs_dispex_static_data = { &cs_dispex_static_data_dbg, -1, 0, 0, 0, 0 };


static dispex_data_t *get_dispex_data(DispatchEx *This)
{
    if(This->data->data)
        return This->data->data;

    EnterCriticalSection(&cs_dispex_static_data);

    if(!This->data->data)
        This->data->data = preprocess_dispex_data(This);

    LeaveCriticalSection(&cs_dispex_static_data);

    return This->data->data;
}

void call_disp_func(HTMLDocument *doc, IDispatch *disp)
{
    DISPID named_arg = DISPID_THIS;
    VARIANTARG arg;
    DISPPARAMS params = {&arg, &named_arg, 1, 1};
    EXCEPINFO ei;
    IDispatchEx *dispex;
    VARIANT res;
    HRESULT hres;

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    if(FAILED(hres)) {
        FIXME("Could not get IDispatchEx interface: %08x\n", hres);
        return;
    }

    V_VT(&arg) = VT_DISPATCH;
    V_DISPATCH(&arg) = (IDispatch*)HTMLWINDOW2(doc->window);
    VariantInit(&res);
    memset(&ei, 0, sizeof(ei));

    hres = IDispatchEx_InvokeEx(dispex, 0, GetUserDefaultLCID(), DISPATCH_METHOD, &params, &res, &ei, NULL);
    IDispatchEx_Release(dispex);

    TRACE("%p returned %08x\n", disp, hres);

    VariantClear(&res);
}

static inline BOOL is_custom_dispid(DISPID id)
{
    return MSHTML_DISPID_CUSTOM_MIN <= id && id <= MSHTML_DISPID_CUSTOM_MAX;
}

static inline BOOL is_dynamic_dispid(DISPID id)
{
    return DISPID_DYNPROP_0 <= id && id <= DISPID_DYNPROP_MAX;
}

#define DISPATCHEX_THIS(iface) DEFINE_THIS(DispatchEx, IDispatchEx, iface)

static HRESULT WINAPI DispatchEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);

    return IUnknown_QueryInterface(This->outer, riid, ppv);
}

static ULONG WINAPI DispatchEx_AddRef(IDispatchEx *iface)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);

    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI DispatchEx_Release(IDispatchEx *iface)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);

    return IUnknown_Release(This->outer);
}

static HRESULT WINAPI DispatchEx_GetTypeInfoCount(IDispatchEx *iface, UINT *pctinfo)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI DispatchEx_GetTypeInfo(IDispatchEx *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    HRESULT hres;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hres = get_typeinfo(This->data->disp_tid, ppTInfo);
    if(FAILED(hres))
        return hres;

    ITypeInfo_AddRef(*ppTInfo);
    return S_OK;
}

static HRESULT WINAPI DispatchEx_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    UINT i;
    HRESULT hres;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    for(i=0; i < cNames; i++) {
        hres = IDispatchEx_GetDispID(DISPATCHEX(This), rgszNames[i], 0, rgDispId+i);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

static HRESULT WINAPI DispatchEx_Invoke(IDispatchEx *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    return IDispatchEx_InvokeEx(DISPATCHEX(This), dispIdMember, lcid, wFlags,
                                pDispParams, pVarResult, pExcepInfo, NULL);
}

static HRESULT WINAPI DispatchEx_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    dispex_data_t *data;
    int min, max, n, c;

    TRACE("(%p)->(%s %x %p)\n", This, debugstr_w(bstrName), grfdex, pid);

    if(grfdex & ~(fdexNameCaseSensitive|fdexNameEnsure|fdexNameImplicit))
        FIXME("Unsupported grfdex %x\n", grfdex);

    data = get_dispex_data(This);
    if(!data)
        return E_FAIL;

    min = 0;
    max = data->func_cnt-1;

    while(min <= max) {
        n = (min+max)/2;

        c = strcmpiW(data->name_table[n]->name, bstrName);
        if(!c) {
            if((grfdex & fdexNameCaseSensitive) && strcmpW(data->name_table[n]->name, bstrName))
                break;

            *pid = data->name_table[n]->id;
            return S_OK;
        }

        if(c > 0)
            max = n-1;
        else
            min = n+1;
    }

    if(This->dynamic_data) {
        unsigned i;

        for(i=0; i < This->dynamic_data->prop_cnt; i++) {
            if(!strcmpW(This->dynamic_data->props[i].name, bstrName)) {
                *pid = DISPID_DYNPROP_0 + i;
                return S_OK;
            }
        }
    }

    if(This->data->vtbl && This->data->vtbl->get_dispid) {
        HRESULT hres;

        hres = This->data->vtbl->get_dispid(This->outer, bstrName, grfdex, pid);
        if(hres != DISP_E_UNKNOWNNAME)
            return hres;
    }

    if(grfdex & fdexNameEnsure) {
        TRACE("creating dynamic prop %s\n", debugstr_w(bstrName));

        if(!This->dynamic_data) {
            This->dynamic_data = heap_alloc_zero(sizeof(dispex_dynamic_data_t));
            This->dynamic_data->props = heap_alloc(This->dynamic_data->buf_size = 4);
        }else if(This->dynamic_data->buf_size == This->dynamic_data->prop_cnt) {
            This->dynamic_data->props = heap_realloc(This->dynamic_data->props, This->dynamic_data->buf_size<<=1);
        }

        This->dynamic_data->props[This->dynamic_data->prop_cnt].name = heap_strdupW(bstrName);
        VariantInit(&This->dynamic_data->props[This->dynamic_data->prop_cnt].var);
        *pid = DISPID_DYNPROP_0 + This->dynamic_data->prop_cnt++;

        return S_OK;
    }

    TRACE("not found %s\n", debugstr_w(bstrName));
    return DISP_E_UNKNOWNNAME;
}

static HRESULT WINAPI DispatchEx_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    IUnknown *unk;
    ITypeInfo *ti;
    dispex_data_t *data;
    UINT argerr=0;
    int min, max, n;
    HRESULT hres;

    TRACE("(%p)->(%x %x %x %p %p %p %p)\n", This, id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);

    if(is_custom_dispid(id) && This->data->vtbl && This->data->vtbl->invoke)
        return This->data->vtbl->invoke(This->outer, id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);

    if(wFlags == DISPATCH_CONSTRUCT) {
        FIXME("DISPATCH_CONSTRUCT not implemented\n");
        return E_NOTIMPL;
    }

    if(is_dynamic_dispid(id)) {
        DWORD idx = id - DISPID_DYNPROP_0;
        VARIANT *var;

        if(!This->dynamic_data || This->dynamic_data->prop_cnt <= idx)
            return DISP_E_UNKNOWNNAME;

        var = &This->dynamic_data->props[idx].var;

        switch(wFlags) {
        case INVOKE_PROPERTYGET:
            return VariantCopy(pvarRes, var);
        case INVOKE_PROPERTYPUT:
            VariantClear(var);
            return VariantCopy(var, pdp->rgvarg);
        default:
            FIXME("unhandled wFlags %x\n", wFlags);
            return E_NOTIMPL;
        }
    }

    data = get_dispex_data(This);
    if(!data)
        return E_FAIL;

    min = 0;
    max = data->func_cnt-1;

    while(min <= max) {
        n = (min+max)/2;

        if(data->funcs[n].id == id)
            break;

        if(data->funcs[n].id < id)
            min = n+1;
        else
            max = n-1;
    }

    if(min > max) {
        WARN("invalid id %x\n", id);
        return DISP_E_UNKNOWNNAME;
    }

    hres = get_typeinfo(data->funcs[n].tid, &ti);
    if(FAILED(hres)) {
        ERR("Could not get type info: %08x\n", hres);
        return hres;
    }

    hres = IUnknown_QueryInterface(This->outer, tid_ids[data->funcs[n].tid], (void**)&unk);
    if(FAILED(hres)) {
        ERR("Could not get iface: %08x\n", hres);
        return E_FAIL;
    }

    hres = ITypeInfo_Invoke(ti, unk, id, wFlags, pdp, pvarRes, pei, &argerr);

    IUnknown_Release(unk);
    return hres;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    FIXME("(%p)->(%s %x)\n", This, debugstr_w(bstrName), grfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    FIXME("(%p)->(%x)\n", This, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    FIXME("(%p)->(%x %x %p)\n", This, id, grfdexFetch, pgrfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *pbstrName)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    FIXME("(%p)->(%x %p)\n", This, id, pbstrName);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    FIXME("(%p)->(%x %x %p)\n", This, grfdex, id, pid);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    DispatchEx *This = DISPATCHEX_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppunk);
    return E_NOTIMPL;
}

#undef DISPATCHEX_THIS

static IDispatchExVtbl DispatchExVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    DispatchEx_GetDispID,
    DispatchEx_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

BOOL dispex_query_interface(DispatchEx *This, REFIID riid, void **ppv)
{
    static const IID IID_UndocumentedScriptIface =
        {0x719c3050,0xf9d3,0x11cf,{0xa4,0x93,0x00,0x40,0x05,0x23,0xa8,0xa0}};

    if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = DISPATCHEX(This);
    }else if(IsEqualGUID(&IID_IDispatchEx, riid)) {
        TRACE("(%p)->(IID_IDispatchEx %p)\n", This, ppv);
        *ppv = DISPATCHEX(This);
    }else if(IsEqualGUID(&IID_UndocumentedScriptIface, riid)) {
        TRACE("(%p)->(IID_UndocumentedScriptIface %p) returning NULL\n", This, ppv);
        *ppv = NULL;
    }else {
        return FALSE;
    }

    if(*ppv)
        IUnknown_AddRef((IUnknown*)*ppv);
    return TRUE;
}

void init_dispex(DispatchEx *dispex, IUnknown *outer, dispex_static_data_t *data)
{
    dispex->lpIDispatchExVtbl = &DispatchExVtbl;
    dispex->outer = outer;
    dispex->data = data;
}
