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

#include "jscript.h"

/*
 * This IID is used to get jsdisp_t objecto from interface.
 * We might consider using private interface instead.
 */
static const IID IID_IDispatchJS =
        {0x719c3050,0xf9d3,0x11cf,{0xa4,0x93,0x00,0x40,0x05,0x23,0xa8,0xa6}};

#define FDEX_VERSION_MASK 0xf0000000
#define GOLDEN_RATIO 0x9E3779B9U

typedef enum {
    PROP_JSVAL,
    PROP_BUILTIN,
    PROP_PROTREF,
    PROP_DELETED,
    PROP_IDX
} prop_type_t;

struct _dispex_prop_t {
    WCHAR *name;
    unsigned hash;
    prop_type_t type;
    DWORD flags;

    union {
        jsval_t val;
        const builtin_prop_t *p;
        DWORD ref;
        unsigned idx;
    } u;

    int bucket_head;
    int bucket_next;
};

static inline DISPID prop_to_id(jsdisp_t *This, dispex_prop_t *prop)
{
    return prop - This->props;
}

static inline dispex_prop_t *get_prop(jsdisp_t *This, DISPID id)
{
    if(id < 0 || id >= This->prop_cnt || This->props[id].type == PROP_DELETED)
        return NULL;

    return This->props+id;
}

static DWORD get_flags(jsdisp_t *This, dispex_prop_t *prop)
{
    if(prop->type == PROP_PROTREF) {
        dispex_prop_t *parent = get_prop(This->prototype, prop->u.ref);
        if(!parent) {
            prop->type = PROP_DELETED;
            return 0;
        }

        return get_flags(This->prototype, parent);
    }

    return prop->flags;
}

static const builtin_prop_t *find_builtin_prop(jsdisp_t *This, const WCHAR *name)
{
    int min = 0, max, i, r;

    max = This->builtin_info->props_cnt-1;
    while(min <= max) {
        i = (min+max)/2;

        r = strcmpW(name, This->builtin_info->props[i].name);
        if(!r)
            return This->builtin_info->props + i;

        if(r < 0)
            max = i-1;
        else
            min = i+1;
    }

    return NULL;
}

static inline unsigned string_hash(const WCHAR *name)
{
    unsigned h = 0;
    for(; *name; name++)
        h = (h>>(sizeof(unsigned)*8-4)) ^ (h<<4) ^ tolowerW(*name);
    return h;
}

static inline unsigned get_props_idx(jsdisp_t *This, unsigned hash)
{
    return (hash*GOLDEN_RATIO) & (This->buf_size-1);
}

static inline HRESULT resize_props(jsdisp_t *This)
{
    dispex_prop_t *props;
    int i, bucket;

    if(This->buf_size != This->prop_cnt)
        return S_FALSE;

    props = heap_realloc(This->props, sizeof(dispex_prop_t)*This->buf_size*2);
    if(!props)
        return E_OUTOFMEMORY;
    This->buf_size *= 2;
    This->props = props;

    for(i=0; i<This->buf_size; i++) {
        This->props[i].bucket_head = 0;
        This->props[i].bucket_next = 0;
    }

    for(i=1; i<This->prop_cnt; i++) {
        props = This->props+i;

        bucket = get_props_idx(This, props->hash);
        props->bucket_next = This->props[bucket].bucket_head;
        This->props[bucket].bucket_head = i;
    }

    return S_OK;
}

static inline dispex_prop_t* alloc_prop(jsdisp_t *This, const WCHAR *name, prop_type_t type, DWORD flags)
{
    dispex_prop_t *prop;
    unsigned bucket;

    if(FAILED(resize_props(This)))
        return NULL;

    prop = &This->props[This->prop_cnt];
    prop->name = heap_strdupW(name);
    if(!prop->name)
        return NULL;
    prop->type = type;
    prop->flags = flags;
    prop->hash = string_hash(name);

    bucket = get_props_idx(This, prop->hash);
    prop->bucket_next = This->props[bucket].bucket_head;
    This->props[bucket].bucket_head = This->prop_cnt++;
    return prop;
}

static dispex_prop_t *alloc_protref(jsdisp_t *This, const WCHAR *name, DWORD ref)
{
    dispex_prop_t *ret;

    ret = alloc_prop(This, name, PROP_PROTREF, 0);
    if(!ret)
        return NULL;

    ret->u.ref = ref;
    return ret;
}

static HRESULT find_prop_name(jsdisp_t *This, unsigned hash, const WCHAR *name, dispex_prop_t **ret)
{
    const builtin_prop_t *builtin;
    unsigned bucket, pos, prev = 0;
    dispex_prop_t *prop;

    bucket = get_props_idx(This, hash);
    pos = This->props[bucket].bucket_head;
    while(pos != 0) {
        if(!strcmpW(name, This->props[pos].name)) {
            if(prev != 0) {
                This->props[prev].bucket_next = This->props[pos].bucket_next;
                This->props[pos].bucket_next = This->props[bucket].bucket_head;
                This->props[bucket].bucket_head = pos;
            }

            *ret = &This->props[pos];
            return S_OK;
        }

        prev = pos;
        pos = This->props[pos].bucket_next;
    }

    builtin = find_builtin_prop(This, name);
    if(builtin) {
        prop = alloc_prop(This, name, PROP_BUILTIN, builtin->flags);
        if(!prop)
            return E_OUTOFMEMORY;

        prop->u.p = builtin;
        *ret = prop;
        return S_OK;
    }

    if(This->builtin_info->idx_length) {
        const WCHAR *ptr;
        unsigned idx = 0;

        for(ptr = name; isdigitW(*ptr) && idx < 0x10000; ptr++)
            idx = idx*10 + (*ptr-'0');
        if(!*ptr && idx < This->builtin_info->idx_length(This)) {
            prop = alloc_prop(This, name, PROP_IDX, This->builtin_info->idx_put ? 0 : PROPF_CONST);
            if(!prop)
                return E_OUTOFMEMORY;

            prop->u.idx = idx;
            *ret = prop;
            return S_OK;
        }
    }

    *ret = NULL;
    return S_OK;
}

static HRESULT find_prop_name_prot(jsdisp_t *This, unsigned hash, const WCHAR *name, dispex_prop_t **ret)
{
    dispex_prop_t *prop, *del=NULL;
    HRESULT hres;

    hres = find_prop_name(This, hash, name, &prop);
    if(FAILED(hres))
        return hres;
    if(prop && prop->type==PROP_DELETED) {
        del = prop;
    } else if(prop) {
        *ret = prop;
        return S_OK;
    }

    if(This->prototype) {
        hres = find_prop_name_prot(This->prototype, hash, name, &prop);
        if(FAILED(hres))
            return hres;
        if(prop) {
            if(del) {
                del->type = PROP_PROTREF;
                del->flags = 0;
                del->u.ref = prop - This->prototype->props;
                prop = del;
            }else {
                prop = alloc_protref(This, prop->name, prop - This->prototype->props);
                if(!prop)
                    return E_OUTOFMEMORY;
            }

            *ret = prop;
            return S_OK;
        }
    }

    *ret = del;
    return S_OK;
}

static HRESULT ensure_prop_name(jsdisp_t *This, const WCHAR *name, BOOL search_prot, DWORD create_flags, dispex_prop_t **ret)
{
    dispex_prop_t *prop;
    HRESULT hres;

    if(search_prot)
        hres = find_prop_name_prot(This, string_hash(name), name, &prop);
    else
        hres = find_prop_name(This, string_hash(name), name, &prop);
    if(SUCCEEDED(hres) && (!prop || prop->type == PROP_DELETED)) {
        TRACE("creating prop %s flags %x\n", debugstr_w(name), create_flags);

        if(prop) {
            prop->type = PROP_JSVAL;
            prop->flags = create_flags;
            prop->u.val = jsval_undefined();
        }else {
            prop = alloc_prop(This, name, PROP_JSVAL, create_flags);
            if(!prop)
                return E_OUTOFMEMORY;
        }

        prop->u.val = jsval_undefined();
    }

    *ret = prop;
    return hres;
}

static IDispatch *get_this(DISPPARAMS *dp)
{
    DWORD i;

    for(i=0; i < dp->cNamedArgs; i++) {
        if(dp->rgdispidNamedArgs[i] == DISPID_THIS) {
            if(V_VT(dp->rgvarg+i) == VT_DISPATCH)
                return V_DISPATCH(dp->rgvarg+i);

            WARN("This is not VT_DISPATCH\n");
            return NULL;
        }
    }

    TRACE("no this passed\n");
    return NULL;
}

static HRESULT convert_params(const DISPPARAMS *dp, jsval_t *buf, unsigned *argc, jsval_t **ret)
{
    jsval_t *argv;
    unsigned cnt;
    unsigned i;
    HRESULT hres;

    cnt = dp->cArgs - dp->cNamedArgs;

    if(cnt > 6) {
        argv = heap_alloc(cnt * sizeof(*argv));
        if(!argv)
            return E_OUTOFMEMORY;
    }else {
        argv = buf;
    }

    for(i = 0; i < cnt; i++) {
        hres = variant_to_jsval(dp->rgvarg+dp->cArgs-i-1, argv+i);
        if(FAILED(hres)) {
            while(i--)
                jsval_release(argv[i]);
            if(argv != buf)
                heap_free(argv);
            return hres;
        }
    }

    *argc = cnt;
    *ret = argv;
    return S_OK;
}

static HRESULT invoke_prop_func(jsdisp_t *This, IDispatch *jsthis, dispex_prop_t *prop, WORD flags,
        unsigned argc, jsval_t *argv, jsval_t *r, IServiceProvider *caller)
{
    HRESULT hres;

    switch(prop->type) {
    case PROP_BUILTIN: {
        if(flags == DISPATCH_CONSTRUCT && (prop->flags & PROPF_METHOD)) {
            WARN("%s is not a constructor\n", debugstr_w(prop->name));
            return E_INVALIDARG;
        }

        if(prop->name || This->builtin_info->class != JSCLASS_FUNCTION) {
            vdisp_t vthis;

            if(jsthis)
                set_disp(&vthis, jsthis);
            else
                set_jsdisp(&vthis, This);
            hres = prop->u.p->invoke(This->ctx, &vthis, flags, argc, argv, r);
            vdisp_release(&vthis);
        }else {
            /* Function object calls are special case */
            hres = Function_invoke(This, jsthis, flags, argc, argv, r);
        }
        return hres;
    }
    case PROP_PROTREF:
        return invoke_prop_func(This->prototype, jsthis, This->prototype->props+prop->u.ref,
                flags, argc, argv, r, caller);
    case PROP_JSVAL: {
        if(!is_object_instance(prop->u.val)) {
            FIXME("invoke %s\n", debugstr_jsval(prop->u.val));
            return E_FAIL;
        }

        TRACE("call %s %p\n", debugstr_w(prop->name), get_object(prop->u.val));

        return disp_call_value(This->ctx, get_object(prop->u.val), jsthis, flags, argc, argv, r);
    }
    case PROP_IDX:
        FIXME("Invoking PROP_IDX not yet supported\n");
        return E_NOTIMPL;
    case PROP_DELETED:
        assert(0);
    }

    assert(0);
    return E_FAIL;
}

static HRESULT prop_get(jsdisp_t *This, dispex_prop_t *prop, DISPPARAMS *dp,
        jsval_t *r, IServiceProvider *caller)
{
    HRESULT hres;

    switch(prop->type) {
    case PROP_BUILTIN:
        if(prop->u.p->getter) {
            hres = prop->u.p->getter(This->ctx, This, r);
        }else {
            jsdisp_t *obj;

            assert(prop->u.p->invoke != NULL);
            hres = create_builtin_function(This->ctx, prop->u.p->invoke, prop->u.p->name, NULL,
                    prop->u.p->flags, NULL, &obj);
            if(FAILED(hres))
                break;

            prop->type = PROP_JSVAL;
            prop->u.val = jsval_obj(obj);

            jsdisp_addref(obj);
            *r = jsval_obj(obj);
        }
        break;
    case PROP_PROTREF:
        hres = prop_get(This->prototype, This->prototype->props+prop->u.ref, dp, r, caller);
        break;
    case PROP_JSVAL:
        hres = jsval_copy(prop->u.val, r);
        break;
    case PROP_IDX:
        hres = This->builtin_info->idx_get(This, prop->u.idx, r);
        break;
    default:
        ERR("type %d\n", prop->type);
        return E_FAIL;
    }

    if(FAILED(hres)) {
        TRACE("fail %08x\n", hres);
        return hres;
    }

    TRACE("%s ret %s\n", debugstr_w(prop->name), debugstr_jsval(*r));
    return hres;
}

static HRESULT prop_put(jsdisp_t *This, dispex_prop_t *prop, jsval_t val, IServiceProvider *caller)
{
    HRESULT hres;

    if(prop->flags & PROPF_CONST)
        return S_OK;

    switch(prop->type) {
    case PROP_BUILTIN:
        if(prop->u.p->setter)
            return prop->u.p->setter(This->ctx, This, val);

        if(prop->u.p->setter) {
            FIXME("getter with no setter\n");
            return E_FAIL;
        }
        /* fall through */
    case PROP_PROTREF:
        prop->type = PROP_JSVAL;
        prop->flags = PROPF_ENUM;
        prop->u.val = jsval_undefined();
        break;
    case PROP_JSVAL:
        jsval_release(prop->u.val);
        break;
    case PROP_IDX:
        return This->builtin_info->idx_put(This, prop->u.idx, val);
    default:
        ERR("type %d\n", prop->type);
        return E_FAIL;
    }

    TRACE("%s = %s\n", debugstr_w(prop->name), debugstr_jsval(val));

    hres = jsval_copy(val, &prop->u.val);
    if(FAILED(hres)) {
        prop->u.val = jsval_undefined();
        return hres;
    }

    if(This->builtin_info->on_put)
        This->builtin_info->on_put(This, prop->name);

    return S_OK;
}

HRESULT builtin_set_const(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t value)
{
    TRACE("%p %s\n", jsthis, debugstr_jsval(value));
    return S_OK;
}

static HRESULT fill_protrefs(jsdisp_t *This)
{
    dispex_prop_t *iter, *prop;
    HRESULT hres;

    if(!This->prototype)
        return S_OK;

    fill_protrefs(This->prototype);

    for(iter = This->prototype->props; iter < This->prototype->props+This->prototype->prop_cnt; iter++) {
        if(!iter->name)
            continue;
        hres = find_prop_name(This, iter->hash, iter->name, &prop);
        if(FAILED(hres))
            return hres;
        if(!prop || prop->type==PROP_DELETED) {
            if(prop) {
                prop->type = PROP_PROTREF;
                prop->flags = 0;
                prop->u.ref = iter - This->prototype->props;
            }else {
                prop = alloc_protref(This, iter->name, iter - This->prototype->props);
                if(!prop)
                    return E_OUTOFMEMORY;
            }
        }
    }

    return S_OK;
}

static inline jsdisp_t *impl_from_IDispatchEx(IDispatchEx *iface)
{
    return CONTAINING_RECORD(iface, jsdisp_t, IDispatchEx_iface);
}

static HRESULT WINAPI DispatchEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    jsdisp_t *This = impl_from_IDispatchEx(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IDispatchEx_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IDispatchEx_iface;
    }else if(IsEqualGUID(&IID_IDispatchEx, riid)) {
        TRACE("(%p)->(IID_IDispatchEx %p)\n", This, ppv);
        *ppv = &This->IDispatchEx_iface;
    }else if(IsEqualGUID(&IID_IDispatchJS, riid)) {
        TRACE("(%p)->(IID_IDispatchJS %p)\n", This, ppv);
        jsdisp_addref(This);
        *ppv = This;
        return S_OK;
    }else {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    jsdisp_addref(This);
    return S_OK;
}

static ULONG WINAPI DispatchEx_AddRef(IDispatchEx *iface)
{
    jsdisp_t *This = impl_from_IDispatchEx(iface);
    jsdisp_addref(This);
    return This->ref;
}

static ULONG WINAPI DispatchEx_Release(IDispatchEx *iface)
{
    jsdisp_t *This = impl_from_IDispatchEx(iface);
    ULONG ref = --This->ref;
    if(!ref)
        jsdisp_free(This);
    return ref;
}

static HRESULT WINAPI DispatchEx_GetTypeInfoCount(IDispatchEx *iface, UINT *pctinfo)
{
    jsdisp_t *This = impl_from_IDispatchEx(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI DispatchEx_GetTypeInfo(IDispatchEx *iface, UINT iTInfo, LCID lcid,
                                              ITypeInfo **ppTInfo)
{
    jsdisp_t *This = impl_from_IDispatchEx(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames, LCID lcid,
                                                DISPID *rgDispId)
{
    jsdisp_t *This = impl_from_IDispatchEx(iface);
    UINT i;
    HRESULT hres;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    for(i=0; i < cNames; i++) {
        hres = IDispatchEx_GetDispID(&This->IDispatchEx_iface, rgszNames[i], 0, rgDispId+i);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

static HRESULT WINAPI DispatchEx_Invoke(IDispatchEx *iface, DISPID dispIdMember,
                                        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    jsdisp_t *This = impl_from_IDispatchEx(iface);

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    return IDispatchEx_InvokeEx(&This->IDispatchEx_iface, dispIdMember, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, NULL);
}

static HRESULT WINAPI DispatchEx_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    jsdisp_t *This = impl_from_IDispatchEx(iface);

    TRACE("(%p)->(%s %x %p)\n", This, debugstr_w(bstrName), grfdex, pid);

    if(grfdex & ~(fdexNameCaseSensitive|fdexNameEnsure|fdexNameImplicit|FDEX_VERSION_MASK)) {
        FIXME("Unsupported grfdex %x\n", grfdex);
        return E_NOTIMPL;
    }

    return jsdisp_get_id(This, bstrName, grfdex, pid);
}

static HRESULT WINAPI DispatchEx_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    jsdisp_t *This = impl_from_IDispatchEx(iface);
    dispex_prop_t *prop;
    HRESULT hres;

    TRACE("(%p)->(%x %x %x %p %p %p %p)\n", This, id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);

    if(pvarRes)
        V_VT(pvarRes) = VT_EMPTY;

    prop = get_prop(This, id);
    if(!prop || prop->type == PROP_DELETED) {
        TRACE("invalid id\n");
        return DISP_E_MEMBERNOTFOUND;
    }

    clear_ei(This->ctx);

    switch(wFlags) {
    case DISPATCH_METHOD|DISPATCH_PROPERTYGET:
        wFlags = DISPATCH_METHOD;
        /* fall through */
    case DISPATCH_METHOD:
    case DISPATCH_CONSTRUCT: {
        jsval_t *argv, buf[6], r;
        unsigned argc;

        hres = convert_params(pdp, buf, &argc, &argv);
        if(FAILED(hres))
            return hres;

        hres = invoke_prop_func(This, get_this(pdp), prop, wFlags, argc, argv, pvarRes ? &r : NULL, pspCaller);
        if(argv != buf)
            heap_free(argv);
        if(SUCCEEDED(hres) && pvarRes) {
            hres = jsval_to_variant(r, pvarRes);
            jsval_release(r);
        }
        break;
    }
    case DISPATCH_PROPERTYGET: {
        jsval_t r;

        hres = prop_get(This, prop, pdp, &r, pspCaller);
        if(SUCCEEDED(hres)) {
            hres = jsval_to_variant(r, pvarRes);
            jsval_release(r);
        }
        break;
    }
    case DISPATCH_PROPERTYPUT: {
        jsval_t val;
        DWORD i;

        for(i=0; i < pdp->cNamedArgs; i++) {
            if(pdp->rgdispidNamedArgs[i] == DISPID_PROPERTYPUT)
                break;
        }

        if(i == pdp->cNamedArgs) {
            TRACE("no value to set\n");
            return DISP_E_PARAMNOTOPTIONAL;
        }

        hres = variant_to_jsval(pdp->rgvarg+i, &val);
        if(FAILED(hres))
            return hres;

        hres = prop_put(This, prop, val, pspCaller);
        jsval_release(val);
        break;
    }
    default:
        FIXME("Unimplemented flags %x\n", wFlags);
        return E_INVALIDARG;
    }

    if(pei)
        *pei = This->ctx->ei.ei;
    return hres;
}

static HRESULT delete_prop(dispex_prop_t *prop, BOOL *ret)
{
    if(prop->flags & PROPF_DONTDELETE) {
        *ret = FALSE;
        return S_OK;
    }

    *ret = TRUE; /* FIXME: not exactly right */

    if(prop->type == PROP_JSVAL) {
        jsval_release(prop->u.val);
        prop->type = PROP_DELETED;
    }
    return S_OK;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    jsdisp_t *This = impl_from_IDispatchEx(iface);
    dispex_prop_t *prop;
    BOOL b;
    HRESULT hres;

    TRACE("(%p)->(%s %x)\n", This, debugstr_w(bstrName), grfdex);

    if(grfdex & ~(fdexNameCaseSensitive|fdexNameEnsure|fdexNameImplicit|FDEX_VERSION_MASK))
        FIXME("Unsupported grfdex %x\n", grfdex);

    hres = find_prop_name(This, string_hash(bstrName), bstrName, &prop);
    if(FAILED(hres))
        return hres;
    if(!prop) {
        TRACE("not found\n");
        return S_OK;
    }

    return delete_prop(prop, &b);
}

static HRESULT WINAPI DispatchEx_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    jsdisp_t *This = impl_from_IDispatchEx(iface);
    dispex_prop_t *prop;
    BOOL b;

    TRACE("(%p)->(%x)\n", This, id);

    prop = get_prop(This, id);
    if(!prop) {
        WARN("invalid id\n");
        return DISP_E_MEMBERNOTFOUND;
    }

    return delete_prop(prop, &b);
}

static HRESULT WINAPI DispatchEx_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    jsdisp_t *This = impl_from_IDispatchEx(iface);
    FIXME("(%p)->(%x %x %p)\n", This, id, grfdexFetch, pgrfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *pbstrName)
{
    jsdisp_t *This = impl_from_IDispatchEx(iface);
    dispex_prop_t *prop;

    TRACE("(%p)->(%x %p)\n", This, id, pbstrName);

    prop = get_prop(This, id);
    if(!prop || !prop->name || prop->type == PROP_DELETED)
        return DISP_E_MEMBERNOTFOUND;

    *pbstrName = SysAllocString(prop->name);
    if(!*pbstrName)
        return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI DispatchEx_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    jsdisp_t *This = impl_from_IDispatchEx(iface);
    dispex_prop_t *iter;
    HRESULT hres;

    TRACE("(%p)->(%x %x %p)\n", This, grfdex, id, pid);

    if(id == DISPID_STARTENUM) {
        hres = fill_protrefs(This);
        if(FAILED(hres))
            return hres;
    }

    if(id+1>=0 && id+1<This->prop_cnt) {
        iter = &This->props[id+1];
    }else {
        *pid = DISPID_STARTENUM;
        return S_FALSE;
    }

    while(iter < This->props + This->prop_cnt) {
        if(iter->name && (get_flags(This, iter) & PROPF_ENUM) && iter->type!=PROP_DELETED) {
            *pid = prop_to_id(This, iter);
            return S_OK;
        }
        iter++;
    }

    *pid = DISPID_STARTENUM;
    return S_FALSE;
}

static HRESULT WINAPI DispatchEx_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    jsdisp_t *This = impl_from_IDispatchEx(iface);
    FIXME("(%p)->(%p)\n", This, ppunk);
    return E_NOTIMPL;
}

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

jsdisp_t *as_jsdisp(IDispatch *disp)
{
    assert(disp->lpVtbl == (IDispatchVtbl*)&DispatchExVtbl);
    return impl_from_IDispatchEx((IDispatchEx*)disp);
}

jsdisp_t *to_jsdisp(IDispatch *disp)
{
    return disp->lpVtbl == (IDispatchVtbl*)&DispatchExVtbl ? impl_from_IDispatchEx((IDispatchEx*)disp) : NULL;
}

HRESULT init_dispex(jsdisp_t *dispex, script_ctx_t *ctx, const builtin_info_t *builtin_info, jsdisp_t *prototype)
{
    TRACE("%p (%p)\n", dispex, prototype);

    dispex->IDispatchEx_iface.lpVtbl = &DispatchExVtbl;
    dispex->ref = 1;
    dispex->builtin_info = builtin_info;

    dispex->props = heap_alloc_zero(sizeof(dispex_prop_t)*(dispex->buf_size=4));
    if(!dispex->props)
        return E_OUTOFMEMORY;

    dispex->prototype = prototype;
    if(prototype)
        jsdisp_addref(prototype);

    dispex->prop_cnt = 1;
    if(builtin_info->value_prop.invoke || builtin_info->value_prop.getter) {
        dispex->props[0].type = PROP_BUILTIN;
        dispex->props[0].u.p = &builtin_info->value_prop;
    }else {
        dispex->props[0].type = PROP_DELETED;
    }

    script_addref(ctx);
    dispex->ctx = ctx;

    return S_OK;
}

static const builtin_info_t dispex_info = {
    JSCLASS_NONE,
    {NULL, NULL, 0},
    0, NULL,
    NULL,
    NULL
};

HRESULT create_dispex(script_ctx_t *ctx, const builtin_info_t *builtin_info, jsdisp_t *prototype, jsdisp_t **dispex)
{
    jsdisp_t *ret;
    HRESULT hres;

    ret = heap_alloc_zero(sizeof(jsdisp_t));
    if(!ret)
        return E_OUTOFMEMORY;

    hres = init_dispex(ret, ctx, builtin_info ? builtin_info : &dispex_info, prototype);
    if(FAILED(hres)) {
        heap_free(ret);
        return hres;
    }

    *dispex = ret;
    return S_OK;
}

void jsdisp_free(jsdisp_t *obj)
{
    dispex_prop_t *prop;

    TRACE("(%p)\n", obj);

    for(prop = obj->props; prop < obj->props+obj->prop_cnt; prop++) {
        if(prop->type == PROP_JSVAL)
            jsval_release(prop->u.val);
        heap_free(prop->name);
    }
    heap_free(obj->props);
    script_release(obj->ctx);
    if(obj->prototype)
        jsdisp_release(obj->prototype);

    if(obj->builtin_info->destructor)
        obj->builtin_info->destructor(obj);
    else
        heap_free(obj);
}

#ifdef TRACE_REFCNT

jsdisp_t *jsdisp_addref(jsdisp_t *jsdisp)
{
    ULONG ref = ++jsdisp->ref;
    TRACE("(%p) ref=%d\n", jsdisp, ref);
    return jsdisp;
}

void jsdisp_release(jsdisp_t *jsdisp)
{
    ULONG ref = --jsdisp->ref;

    TRACE("(%p) ref=%d\n", jsdisp, ref);

    if(!ref)
        jsdisp_free(jsdisp);
}

#endif

HRESULT init_dispex_from_constr(jsdisp_t *dispex, script_ctx_t *ctx, const builtin_info_t *builtin_info, jsdisp_t *constr)
{
    jsdisp_t *prot = NULL;
    dispex_prop_t *prop;
    HRESULT hres;

    static const WCHAR prototypeW[] = {'p','r','o','t','o','t','y','p','e',0};

    hres = find_prop_name_prot(constr, string_hash(prototypeW), prototypeW, &prop);
    if(SUCCEEDED(hres) && prop && prop->type!=PROP_DELETED) {
        jsval_t val;

        hres = prop_get(constr, prop, NULL, &val, NULL);
        if(FAILED(hres)) {
            ERR("Could not get prototype\n");
            return hres;
        }

        if(is_object_instance(val))
            prot = iface_to_jsdisp((IUnknown*)get_object(val));
        jsval_release(val);
    }

    hres = init_dispex(dispex, ctx, builtin_info, prot);

    if(prot)
        jsdisp_release(prot);
    return hres;
}

jsdisp_t *iface_to_jsdisp(IUnknown *iface)
{
    jsdisp_t *ret;
    HRESULT hres;

    hres = IUnknown_QueryInterface(iface, &IID_IDispatchJS, (void**)&ret);
    if(FAILED(hres))
        return NULL;

    return ret;
}

HRESULT jsdisp_get_id(jsdisp_t *jsdisp, const WCHAR *name, DWORD flags, DISPID *id)
{
    dispex_prop_t *prop;
    HRESULT hres;

    if(flags & fdexNameEnsure)
        hres = ensure_prop_name(jsdisp, name, TRUE, PROPF_ENUM, &prop);
    else
        hres = find_prop_name_prot(jsdisp, string_hash(name), name, &prop);
    if(FAILED(hres))
        return hres;

    if(prop && prop->type!=PROP_DELETED) {
        *id = prop_to_id(jsdisp, prop);
        return S_OK;
    }

    TRACE("not found %s\n", debugstr_w(name));
    return DISP_E_UNKNOWNNAME;
}

HRESULT jsdisp_call_value(jsdisp_t *jsfunc, IDispatch *jsthis, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    HRESULT hres;

    assert(!(flags & ~(DISPATCH_METHOD|DISPATCH_CONSTRUCT)));

    if(is_class(jsfunc, JSCLASS_FUNCTION)) {
        hres = Function_invoke(jsfunc, jsthis, flags, argc, argv, r);
    }else {
        vdisp_t vdisp;

        if(!jsfunc->builtin_info->value_prop.invoke) {
            WARN("Not a function\n");
            return throw_type_error(jsfunc->ctx, JS_E_FUNCTION_EXPECTED, NULL);
        }

        set_disp(&vdisp, jsthis);
        hres = jsfunc->builtin_info->value_prop.invoke(jsfunc->ctx, &vdisp, flags, argc, argv, r);
        vdisp_release(&vdisp);
    }
    return hres;
}

HRESULT jsdisp_call(jsdisp_t *disp, DISPID id, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    dispex_prop_t *prop;

    prop = get_prop(disp, id);
    if(!prop)
        return DISP_E_MEMBERNOTFOUND;

    return invoke_prop_func(disp, to_disp(disp), prop, flags, argc, argv, r, NULL);
}

HRESULT jsdisp_call_name(jsdisp_t *disp, const WCHAR *name, WORD flags, unsigned argc, jsval_t *argv, jsval_t *r)
{
    dispex_prop_t *prop;
    HRESULT hres;

    hres = find_prop_name_prot(disp, string_hash(name), name, &prop);
    if(FAILED(hres))
        return hres;

    return invoke_prop_func(disp, to_disp(disp), prop, flags, argc, argv, r, NULL);
}

HRESULT disp_call(script_ctx_t *ctx, IDispatch *disp, DISPID id, WORD flags, unsigned argc, jsval_t *argv, jsval_t *ret)
{
    IDispatchEx *dispex;
    jsdisp_t *jsdisp;
    VARIANT buf[6], retv;
    DISPPARAMS dp;
    unsigned i;
    HRESULT hres;

    jsdisp = iface_to_jsdisp((IUnknown*)disp);
    if(jsdisp) {
        if(flags & DISPATCH_PROPERTYPUT) {
            FIXME("disp_call(propput) on builtin object\n");
            return E_FAIL;
        }

        hres = jsdisp_call(jsdisp, id, flags, argc, argv, ret);
        jsdisp_release(jsdisp);
        return hres;
    }

    if(ret && argc)
        flags |= DISPATCH_PROPERTYGET;

    dp.cArgs = argc;

    if(flags & DISPATCH_PROPERTYPUT) {
        static DISPID propput_dispid = DISPID_PROPERTYPUT;

        dp.cNamedArgs = 1;
        dp.rgdispidNamedArgs = &propput_dispid;
    }else {
        dp.cNamedArgs = 0;
        dp.rgdispidNamedArgs = NULL;
    }

    if(argc > 6) {
        dp.rgvarg = heap_alloc(argc*sizeof(VARIANT));
        if(!dp.rgvarg)
            return E_OUTOFMEMORY;
    }else {
        dp.rgvarg = buf;
    }

    for(i=0; i<argc; i++) {
        hres = jsval_to_variant(argv[i], dp.rgvarg+argc-i-1);
        if(FAILED(hres)) {
            while(i--)
                VariantClear(dp.rgvarg+argc-i-1);
            if(dp.rgvarg != buf)
                heap_free(dp.rgvarg);
            return hres;
        }
    }

    V_VT(&retv) = VT_EMPTY;
    clear_ei(ctx);
    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    if(SUCCEEDED(hres)) {
        hres = IDispatchEx_InvokeEx(dispex, id, ctx->lcid, flags, &dp, ret ? &retv : NULL, &ctx->ei.ei,
                &ctx->jscaller->IServiceProvider_iface);
        IDispatchEx_Release(dispex);
    }else {
        UINT err = 0;

        if(flags == DISPATCH_CONSTRUCT) {
            WARN("IDispatch cannot be constructor\n");
            return DISP_E_MEMBERNOTFOUND;
        }

        TRACE("using IDispatch\n");
        hres = IDispatch_Invoke(disp, id, &IID_NULL, ctx->lcid, flags, &dp, ret ? &retv : NULL, &ctx->ei.ei, &err);
    }

    for(i=0; i<argc; i++)
        VariantClear(dp.rgvarg+argc-i-1);
    if(dp.rgvarg != buf)
        heap_free(dp.rgvarg);
    if(FAILED(hres))
        return hres;

    if(ret) {
        hres = variant_to_jsval(&retv, ret);
        VariantClear(&retv);
    }
    return hres;
}

HRESULT disp_call_value(script_ctx_t *ctx, IDispatch *disp, IDispatch *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *jsdisp;
    IDispatchEx *dispex;
    VARIANT buf[6], retv;
    DISPPARAMS dp;
    unsigned i;
    HRESULT hres;

    assert(!(flags & ~(DISPATCH_METHOD|DISPATCH_CONSTRUCT)));

    jsdisp = iface_to_jsdisp((IUnknown*)disp);
    if(jsdisp) {
        hres = jsdisp_call_value(jsdisp, jsthis, flags, argc, argv, r);
        jsdisp_release(jsdisp);
        return hres;
    }

    if(r && argc && flags == DISPATCH_METHOD)
        flags |= DISPATCH_PROPERTYGET;

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    if(FAILED(hres)) {
        TRACE("using IDispatch\n");
        dispex = NULL;
        jsthis = NULL;
    }

    if(jsthis) {
        static DISPID this_id = DISPID_THIS;

        dp.cArgs = argc+1;
        dp.cNamedArgs = 1;
        dp.rgdispidNamedArgs = &this_id;
    }else {
        dp.cArgs = argc;
        dp.cNamedArgs = 0;
        dp.rgdispidNamedArgs = NULL;
    }

    if(dp.cArgs > sizeof(buf)/sizeof(*buf)) {
        dp.rgvarg = heap_alloc(dp.cArgs*sizeof(VARIANT));
        if(!dp.rgvarg) {
            if(dispex)
                IDispatchEx_Release(dispex);
            return E_OUTOFMEMORY;
        }
    }else {
        dp.rgvarg = buf;
    }

    for(i=0; i<argc; i++) {
        hres = jsval_to_variant(argv[i], dp.rgvarg+dp.cArgs-i-1);
        if(FAILED(hres)) {
            while(i--)
                VariantClear(dp.rgvarg+dp.cArgs-i-1);
            if(dp.rgvarg != buf)
                heap_free(dp.rgvarg);
            if(dispex)
                IDispatchEx_Release(dispex);
            return hres;
        }
    }
    if(jsthis) {
        V_VT(dp.rgvarg) = VT_DISPATCH;
        V_DISPATCH(dp.rgvarg) = jsthis;
    }

    V_VT(&retv) = VT_EMPTY;
    clear_ei(ctx);
    if(dispex) {
        hres = IDispatchEx_InvokeEx(dispex, DISPID_VALUE, ctx->lcid, flags, &dp, r ? &retv : NULL, &ctx->ei.ei,
                &ctx->jscaller->IServiceProvider_iface);
        IDispatchEx_Release(dispex);
    }else {
        UINT err = 0;

        if(flags == DISPATCH_CONSTRUCT) {
            WARN("IDispatch cannot be constructor\n");
            return DISP_E_MEMBERNOTFOUND;
        }

        hres = IDispatch_Invoke(disp, DISPID_VALUE, &IID_NULL, ctx->lcid, flags, &dp, r ? &retv : NULL, &ctx->ei.ei, &err);
    }

    for(i=0; i<argc; i++)
        VariantClear(dp.rgvarg+dp.cArgs-i-1);
    if(dp.rgvarg != buf)
        heap_free(dp.rgvarg);
    if(FAILED(hres))
        return hres;

    if(!r)
        return S_OK;

    hres = variant_to_jsval(&retv, r);
    VariantClear(&retv);
    return hres;
}

HRESULT jsdisp_propput(jsdisp_t *obj, const WCHAR *name, DWORD flags, jsval_t val)
{
    dispex_prop_t *prop;
    HRESULT hres;

    hres = ensure_prop_name(obj, name, FALSE, flags, &prop);
    if(FAILED(hres))
        return hres;

    return prop_put(obj, prop, val, NULL);
}

HRESULT jsdisp_propput_name(jsdisp_t *obj, const WCHAR *name, jsval_t val)
{
    return jsdisp_propput(obj, name, PROPF_ENUM, val);
}

HRESULT jsdisp_propput_const(jsdisp_t *obj, const WCHAR *name, jsval_t val)
{
    dispex_prop_t *prop;
    HRESULT hres;

    hres = ensure_prop_name(obj, name, FALSE, PROPF_CONST, &prop);
    if(FAILED(hres))
        return hres;

    return jsval_copy(val, &prop->u.val);
}

HRESULT jsdisp_propput_dontenum(jsdisp_t *obj, const WCHAR *name, jsval_t val)
{
    return jsdisp_propput(obj, name, 0, val);
}

HRESULT jsdisp_propput_idx(jsdisp_t *obj, DWORD idx, jsval_t val)
{
    WCHAR buf[12];

    static const WCHAR formatW[] = {'%','d',0};

    sprintfW(buf, formatW, idx);
    return jsdisp_propput_name(obj, buf, val);
}

HRESULT disp_propput(script_ctx_t *ctx, IDispatch *disp, DISPID id, jsval_t val)
{
    jsdisp_t *jsdisp;
    HRESULT hres;

    jsdisp = iface_to_jsdisp((IUnknown*)disp);
    if(jsdisp) {
        dispex_prop_t *prop;

        prop = get_prop(jsdisp, id);
        if(prop)
            hres = prop_put(jsdisp, prop, val, NULL);
        else
            hres = DISP_E_MEMBERNOTFOUND;

        jsdisp_release(jsdisp);
    }else {
        DISPID dispid = DISPID_PROPERTYPUT;
        DWORD flags = DISPATCH_PROPERTYPUT;
        VARIANT var;
        DISPPARAMS dp  = {&var, &dispid, 1, 1};
        IDispatchEx *dispex;

        hres = jsval_to_variant(val, &var);
        if(FAILED(hres))
            return hres;

        if(V_VT(&var) == VT_DISPATCH)
            flags |= DISPATCH_PROPERTYPUTREF;

        clear_ei(ctx);
        hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
        if(SUCCEEDED(hres)) {
            hres = IDispatchEx_InvokeEx(dispex, id, ctx->lcid, flags, &dp, NULL, &ctx->ei.ei,
                    &ctx->jscaller->IServiceProvider_iface);
            IDispatchEx_Release(dispex);
        }else {
            ULONG err = 0;

            TRACE("using IDispatch\n");
            hres = IDispatch_Invoke(disp, id, &IID_NULL, ctx->lcid, flags, &dp, NULL, &ctx->ei.ei, &err);
        }

        VariantClear(&var);
    }

    return hres;
}

HRESULT jsdisp_propget_name(jsdisp_t *obj, const WCHAR *name, jsval_t *val)
{
    DISPPARAMS dp = {NULL, NULL, 0, 0};
    dispex_prop_t *prop;
    HRESULT hres;

    hres = find_prop_name_prot(obj, string_hash(name), name, &prop);
    if(FAILED(hres))
        return hres;

    if(!prop || prop->type==PROP_DELETED) {
        *val = jsval_undefined();
        return S_OK;
    }

    return prop_get(obj, prop, &dp, val, NULL);
}

HRESULT jsdisp_get_idx(jsdisp_t *obj, DWORD idx, jsval_t *r)
{
    WCHAR name[12];
    DISPPARAMS dp = {NULL, NULL, 0, 0};
    dispex_prop_t *prop;
    HRESULT hres;

    static const WCHAR formatW[] = {'%','d',0};

    sprintfW(name, formatW, idx);

    hres = find_prop_name_prot(obj, string_hash(name), name, &prop);
    if(FAILED(hres))
        return hres;

    if(!prop || prop->type==PROP_DELETED) {
        *r = jsval_undefined();
        return DISP_E_UNKNOWNNAME;
    }

    return prop_get(obj, prop, &dp, r, NULL);
}

HRESULT jsdisp_propget(jsdisp_t *jsdisp, DISPID id, jsval_t *val)
{
    DISPPARAMS dp  = {NULL,NULL,0,0};
    dispex_prop_t *prop;

    prop = get_prop(jsdisp, id);
    if(!prop)
        return DISP_E_MEMBERNOTFOUND;

    return prop_get(jsdisp, prop, &dp, val, NULL);
}

HRESULT disp_propget(script_ctx_t *ctx, IDispatch *disp, DISPID id, jsval_t *val)
{
    DISPPARAMS dp  = {NULL,NULL,0,0};
    IDispatchEx *dispex;
    jsdisp_t *jsdisp;
    VARIANT var;
    HRESULT hres;

    jsdisp = iface_to_jsdisp((IUnknown*)disp);
    if(jsdisp) {
        hres = jsdisp_propget(jsdisp, id, val);
        jsdisp_release(jsdisp);
        return hres;
    }

    V_VT(&var) = VT_EMPTY;
    clear_ei(ctx);
    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    if(SUCCEEDED(hres)) {
        hres = IDispatchEx_InvokeEx(dispex, id, ctx->lcid, INVOKE_PROPERTYGET, &dp, &var, &ctx->ei.ei,
                &ctx->jscaller->IServiceProvider_iface);
        IDispatchEx_Release(dispex);
    }else {
        ULONG err = 0;

        TRACE("using IDispatch\n");
        hres = IDispatch_Invoke(disp, id, &IID_NULL, ctx->lcid, INVOKE_PROPERTYGET, &dp, &var, &ctx->ei.ei, &err);
    }
    if(FAILED(hres))
        return hres;

    hres = variant_to_jsval(&var, val);
    VariantClear(&var);
    return hres;
}

HRESULT jsdisp_delete_idx(jsdisp_t *obj, DWORD idx)
{
    static const WCHAR formatW[] = {'%','d',0};
    WCHAR buf[12];
    dispex_prop_t *prop;
    BOOL b;
    HRESULT hres;

    sprintfW(buf, formatW, idx);

    hres = find_prop_name(obj, string_hash(buf), buf, &prop);
    if(FAILED(hres) || !prop)
        return hres;

    return delete_prop(prop, &b);
}

HRESULT disp_delete(IDispatch *disp, DISPID id, BOOL *ret)
{
    IDispatchEx *dispex;
    jsdisp_t *jsdisp;
    HRESULT hres;

    jsdisp = iface_to_jsdisp((IUnknown*)disp);
    if(jsdisp) {
        dispex_prop_t *prop;

        prop = get_prop(jsdisp, id);
        if(prop)
            hres = delete_prop(prop, ret);
        else
            hres = DISP_E_MEMBERNOTFOUND;

        jsdisp_release(jsdisp);
        return hres;
    }

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    if(FAILED(hres)) {
        *ret = FALSE;
        return S_OK;
    }

    hres = IDispatchEx_DeleteMemberByDispID(dispex, id);
    IDispatchEx_Release(dispex);
    if(FAILED(hres))
        return hres;

    *ret = hres == S_OK;
    return S_OK;
}

HRESULT disp_delete_name(script_ctx_t *ctx, IDispatch *disp, jsstr_t *name, BOOL *ret)
{
    IDispatchEx *dispex;
    jsdisp_t *jsdisp;
    BSTR bstr;
    HRESULT hres;

    jsdisp = iface_to_jsdisp((IUnknown*)disp);
    if(jsdisp) {
        dispex_prop_t *prop;
        const WCHAR *ptr;

        ptr = jsstr_flatten(name);
        if(!ptr) {
            jsdisp_release(jsdisp);
            return E_OUTOFMEMORY;
        }

        hres = find_prop_name(jsdisp, string_hash(ptr), ptr, &prop);
        if(prop) {
            hres = delete_prop(prop, ret);
        }else {
            *ret = TRUE;
            hres = S_OK;
        }

        jsdisp_release(jsdisp);
        return hres;
    }

    bstr = SysAllocStringLen(NULL, jsstr_length(name));
    if(!bstr)
        return E_OUTOFMEMORY;
    jsstr_flush(name, bstr);

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    if(SUCCEEDED(hres)) {
        hres = IDispatchEx_DeleteMemberByName(dispex, bstr, make_grfdex(ctx, fdexNameCaseSensitive));
        if(SUCCEEDED(hres))
            *ret = hres == S_OK;
        IDispatchEx_Release(dispex);
    }else {
        DISPID id;

        hres = IDispatch_GetIDsOfNames(disp, &IID_NULL, &bstr, 1, 0, &id);
        if(SUCCEEDED(hres)) {
            /* Property exists and we can't delete it from pure IDispatch interface, so return false. */
            *ret = FALSE;
        }else if(hres == DISP_E_UNKNOWNNAME) {
            /* Property doesn't exist, so nothing to delete */
            *ret = TRUE;
            hres = S_OK;
        }
    }

    SysFreeString(bstr);
    return hres;
}

HRESULT jsdisp_is_own_prop(jsdisp_t *obj, const WCHAR *name, BOOL *ret)
{
    dispex_prop_t *prop;
    HRESULT hres;

    hres = find_prop_name(obj, string_hash(name), name, &prop);
    if(FAILED(hres))
        return hres;

    *ret = prop && (prop->type == PROP_JSVAL || prop->type == PROP_BUILTIN);
    return S_OK;
}

HRESULT jsdisp_is_enumerable(jsdisp_t *obj, const WCHAR *name, BOOL *ret)
{
    dispex_prop_t *prop;
    HRESULT hres;

    hres = find_prop_name(obj, string_hash(name), name, &prop);
    if(FAILED(hres))
        return hres;

    *ret = prop && (prop->flags & PROPF_ENUM) && prop->type != PROP_PROTREF;
    return S_OK;
}
