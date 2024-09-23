/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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

#include <assert.h>

#include "vbscript.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vbscript);

static const GUID GUID_VBScriptTypeInfo = {0xc59c6b12,0xf6c1,0x11cf,{0x88,0x35,0x00,0xa0,0xc9,0x11,0xe8,0xb2}};

#define DISPID_FUNCTION_MASK 0x20000000
#define FDEX_VERSION_MASK 0xf0000000

static inline BOOL is_func_id(vbdisp_t *This, DISPID id)
{
    return id < This->desc->func_cnt;
}

static BOOL get_func_id(vbdisp_t *This, const WCHAR *name, vbdisp_invoke_type_t invoke_type, BOOL search_private, DISPID *id)
{
    unsigned i;

    for(i = invoke_type == VBDISP_ANY ? 0 : 1; i < This->desc->func_cnt; i++) {
        if(invoke_type == VBDISP_ANY) {
            if(!search_private && !This->desc->funcs[i].is_public)
                continue;
            if(!i && !This->desc->funcs[0].name) /* default value may not exist */
                continue;
        }else {
            if(!This->desc->funcs[i].entries[invoke_type]
                || (!search_private && !This->desc->funcs[i].entries[invoke_type]->is_public))
                continue;
        }

        if(!wcsicmp(This->desc->funcs[i].name, name)) {
            *id = i;
            return TRUE;
        }
    }

    return FALSE;
}

HRESULT vbdisp_get_id(vbdisp_t *This, BSTR name, vbdisp_invoke_type_t invoke_type, BOOL search_private, DISPID *id)
{
    unsigned i;

    if(get_func_id(This, name, invoke_type, search_private, id))
        return S_OK;

    for(i=0; i < This->desc->prop_cnt; i++) {
        if(!search_private && !This->desc->props[i].is_public)
            continue;

        if(!wcsicmp(This->desc->props[i].name, name)) {
            *id = i + This->desc->func_cnt;
            return S_OK;
        }
    }

    *id = -1;
    return DISP_E_UNKNOWNNAME;
}

static HRESULT get_propput_arg(script_ctx_t *ctx, const DISPPARAMS *dp, WORD flags, VARIANT *v, BOOL *is_owned)
{
    unsigned i;

    for(i=0; i < dp->cNamedArgs; i++) {
        if(dp->rgdispidNamedArgs[i] == DISPID_PROPERTYPUT)
            break;
    }
    if(i == dp->cNamedArgs) {
        WARN("no value to set\n");
        return DISP_E_PARAMNOTOPTIONAL;
    }

    *v = dp->rgvarg[i];
    if(V_VT(v) == (VT_VARIANT|VT_BYREF))
        *v = *V_VARIANTREF(v);
    *is_owned = FALSE;

    if(V_VT(v) == VT_DISPATCH) {
        if(!(flags & DISPATCH_PROPERTYPUTREF)) {
            HRESULT hres;

            hres = get_disp_value(ctx, V_DISPATCH(v), v);
            if(FAILED(hres))
                return hres;

            *is_owned = TRUE;
        }
    }else if(!(flags & DISPATCH_PROPERTYPUT)) {
        WARN("%s can't be assigned without DISPATCH_PROPERTYPUT flag\n", debugstr_variant(v));
        return DISP_E_EXCEPTION;
    }

    return S_OK;
}

static HRESULT invoke_variant_prop(script_ctx_t *ctx, VARIANT *v, WORD flags, DISPPARAMS *dp, VARIANT *res)
{
    HRESULT hres;

    switch(flags) {
    case DISPATCH_PROPERTYGET|DISPATCH_METHOD:
    case DISPATCH_PROPERTYGET:
        if(dp->cArgs) {
            WARN("called with arguments\n");
            return DISP_E_MEMBERNOTFOUND; /* That's what tests show */
        }

        hres = VariantCopyInd(res, v);
        break;

    case DISPATCH_PROPERTYPUT:
    case DISPATCH_PROPERTYPUTREF:
    case DISPATCH_PROPERTYPUT|DISPATCH_PROPERTYPUTREF: {
        VARIANT put_val;
        BOOL own_val;

        hres = get_propput_arg(ctx, dp, flags, &put_val, &own_val);
        if(FAILED(hres))
            return hres;

        if(arg_cnt(dp)) {
            FIXME("Arguments not supported\n");
            return E_NOTIMPL;
        }

        if(res)
            V_VT(res) = VT_EMPTY;

        if(own_val)
            *v = put_val;
        else
            hres = VariantCopyInd(v, &put_val);
        break;
    }

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return hres;
}

static HRESULT invoke_vbdisp(vbdisp_t *This, DISPID id, DWORD flags, BOOL extern_caller, DISPPARAMS *params, VARIANT *res)
{
    if(id < 0)
        return DISP_E_MEMBERNOTFOUND;

    if(is_func_id(This, id)) {
        function_t *func;

        TRACE("%p->%s\n", This, debugstr_w(This->desc->funcs[id].name));

        switch(flags) {
        case DISPATCH_PROPERTYGET:
            func = This->desc->funcs[id].entries[VBDISP_CALLGET];
            if(!func || (func->type != FUNC_PROPGET && func->type != FUNC_DEFGET)) {
                WARN("no getter\n");
                return DISP_E_MEMBERNOTFOUND;
            }

            return exec_script(This->desc->ctx, extern_caller, func, This, params, res);

        case DISPATCH_METHOD:
        case DISPATCH_METHOD|DISPATCH_PROPERTYGET:
            func = This->desc->funcs[id].entries[VBDISP_CALLGET];
            if(!func) {
                FIXME("no invoke/getter\n");
                return DISP_E_MEMBERNOTFOUND;
            }

            return exec_script(This->desc->ctx, extern_caller, func, This, params, res);

        case DISPATCH_PROPERTYPUT:
        case DISPATCH_PROPERTYPUTREF:
        case DISPATCH_PROPERTYPUT|DISPATCH_PROPERTYPUTREF: {
            DISPPARAMS dp = {NULL, NULL, 1, 0};
            BOOL needs_release;
            VARIANT put_val;
            HRESULT hres;

            if(arg_cnt(params)) {
                FIXME("arguments not implemented\n");
                return E_NOTIMPL;
            }

            hres = get_propput_arg(This->desc->ctx, params, flags, &put_val, &needs_release);
            if(FAILED(hres))
                return hres;

            dp.rgvarg = &put_val;
            func = This->desc->funcs[id].entries[V_VT(&put_val) == VT_DISPATCH ? VBDISP_SET : VBDISP_LET];
            if(!func) {
                FIXME("no letter/setter\n");
                return DISP_E_MEMBERNOTFOUND;
            }

            hres = exec_script(This->desc->ctx, extern_caller, func, This, &dp, NULL);
            if(needs_release)
                VariantClear(&put_val);
            return hres;
        }
        default:
            FIXME("flags %x\n", flags);
            return DISP_E_MEMBERNOTFOUND;
        }
    }

    if(id >= This->desc->prop_cnt + This->desc->func_cnt)
        return DISP_E_MEMBERNOTFOUND;

    TRACE("%p->%s\n", This, debugstr_w(This->desc->props[id - This->desc->func_cnt].name));
    return invoke_variant_prop(This->desc->ctx, This->props+(id-This->desc->func_cnt), flags, params, res);
}

static BOOL run_terminator(vbdisp_t *This)
{
    DISPPARAMS dp = {0};

    if(This->terminator_ran)
        return TRUE;
    This->terminator_ran = TRUE;

    if(!This->desc->class_terminate_id)
        return TRUE;

    This->ref++;
    exec_script(This->desc->ctx, FALSE, This->desc->funcs[This->desc->class_terminate_id].entries[VBDISP_CALLGET],
            This, &dp, NULL);
    return !--This->ref;
}

static void clean_props(vbdisp_t *This)
{
    unsigned i;

    if(!This->desc)
        return;

    for(i=0; i < This->desc->array_cnt; i++) {
        if(This->arrays[i]) {
            SafeArrayDestroy(This->arrays[i]);
            This->arrays[i] = NULL;
        }
    }

    for(i=0; i < This->desc->prop_cnt; i++)
        VariantClear(This->props+i);
}

static inline vbdisp_t *impl_from_IDispatchEx(IDispatchEx *iface)
{
    return CONTAINING_RECORD(iface, vbdisp_t, IDispatchEx_iface);
}

static HRESULT WINAPI DispatchEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    vbdisp_t *This = impl_from_IDispatchEx(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IDispatchEx_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IDispatchEx_iface;
    }else if(IsEqualGUID(&IID_IDispatchEx, riid)) {
        TRACE("(%p)->(IID_IDispatchEx %p)\n", This, ppv);
        *ppv = &This->IDispatchEx_iface;
    }else {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI DispatchEx_AddRef(IDispatchEx *iface)
{
    vbdisp_t *This = impl_from_IDispatchEx(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI DispatchEx_Release(IDispatchEx *iface)
{
    vbdisp_t *This = impl_from_IDispatchEx(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref && run_terminator(This)) {
        clean_props(This);
        list_remove(&This->entry);
        heap_free(This->arrays);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI DispatchEx_GetTypeInfoCount(IDispatchEx *iface, UINT *pctinfo)
{
    vbdisp_t *This = impl_from_IDispatchEx(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI DispatchEx_GetTypeInfo(IDispatchEx *iface, UINT iTInfo, LCID lcid,
                                              ITypeInfo **ppTInfo)
{
    vbdisp_t *This = impl_from_IDispatchEx(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames, LCID lcid,
                                                DISPID *rgDispId)
{
    vbdisp_t *This = impl_from_IDispatchEx(iface);
    FIXME("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_Invoke(IDispatchEx *iface, DISPID dispIdMember,
                                        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                                        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    vbdisp_t *This = impl_from_IDispatchEx(iface);

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    return IDispatchEx_InvokeEx(&This->IDispatchEx_iface, dispIdMember, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, NULL);
}

static HRESULT WINAPI DispatchEx_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    vbdisp_t *This = impl_from_IDispatchEx(iface);

    TRACE("(%p)->(%s %x %p)\n", This, debugstr_w(bstrName), grfdex, pid);

    grfdex &= ~FDEX_VERSION_MASK;

    if(!This->desc)
        return E_UNEXPECTED;

    /* Tests show that fdexNameCaseSensitive is ignored */

    if(grfdex & ~(fdexNameEnsure|fdexNameCaseInsensitive|fdexNameCaseSensitive)) {
        FIXME("unsupported flags %x\n", grfdex);
        return E_NOTIMPL;
    }

    return vbdisp_get_id(This, bstrName, VBDISP_ANY, FALSE, pid);
}

static HRESULT WINAPI DispatchEx_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    vbdisp_t *This = impl_from_IDispatchEx(iface);

    TRACE("(%p)->(%x %x %x %p %p %p %p)\n", This, id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);

    if(!This->desc)
        return E_UNEXPECTED;

    if(pvarRes)
        V_VT(pvarRes) = VT_EMPTY;

    return invoke_vbdisp(This, id, wFlags, TRUE, pdp, pvarRes);
}

static HRESULT WINAPI DispatchEx_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    vbdisp_t *This = impl_from_IDispatchEx(iface);
    FIXME("(%p)->(%s %x)\n", This, debugstr_w(bstrName), grfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    vbdisp_t *This = impl_from_IDispatchEx(iface);
    FIXME("(%p)->(%x)\n", This, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    vbdisp_t *This = impl_from_IDispatchEx(iface);
    FIXME("(%p)->(%x %x %p)\n", This, id, grfdexFetch, pgrfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *pbstrName)
{
    vbdisp_t *This = impl_from_IDispatchEx(iface);
    FIXME("(%p)->(%x %p)\n", This, id, pbstrName);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    vbdisp_t *This = impl_from_IDispatchEx(iface);
    FIXME("(%p)->(%x %x %p)\n", This, grfdex, id, pid);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    vbdisp_t *This = impl_from_IDispatchEx(iface);
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

static inline vbdisp_t *unsafe_impl_from_IDispatch(IDispatch *iface)
{
    return iface->lpVtbl == (IDispatchVtbl*)&DispatchExVtbl
        ? CONTAINING_RECORD((IDispatchEx *)iface, vbdisp_t, IDispatchEx_iface)
        : NULL;
}

HRESULT create_vbdisp(const class_desc_t *desc, vbdisp_t **ret)
{
    vbdisp_t *vbdisp;
    HRESULT hres = S_OK;

    vbdisp = heap_alloc_zero( FIELD_OFFSET( vbdisp_t, props[desc->prop_cnt] ));
    if(!vbdisp)
        return E_OUTOFMEMORY;

    vbdisp->IDispatchEx_iface.lpVtbl = &DispatchExVtbl;
    vbdisp->ref = 1;
    vbdisp->desc = desc;

    list_add_tail(&desc->ctx->objects, &vbdisp->entry);

    if(desc->array_cnt) {
        vbdisp->arrays = heap_alloc_zero(desc->array_cnt * sizeof(*vbdisp->arrays));
        if(vbdisp->arrays) {
            unsigned i, j;

            for(i=0; i < desc->array_cnt; i++) {
                if(!desc->array_descs[i].dim_cnt)
                    continue;

                vbdisp->arrays[i] = SafeArrayCreate(VT_VARIANT, desc->array_descs[i].dim_cnt, desc->array_descs[i].bounds);
                if(!vbdisp->arrays[i]) {
                    hres = E_OUTOFMEMORY;
                    break;
                }
            }

            if(SUCCEEDED(hres)) {
                for(i=0, j=0; i < desc->prop_cnt; i++) {
                    if(desc->props[i].is_array) {
                        V_VT(vbdisp->props+i) = VT_ARRAY|VT_BYREF|VT_VARIANT;
                        V_ARRAYREF(vbdisp->props+i) = vbdisp->arrays + j++;
                    }
                }
            }
        }else {
            hres = E_OUTOFMEMORY;
        }
    }

    if(SUCCEEDED(hres) && desc->class_initialize_id) {
        DISPPARAMS dp = {0};
        hres = exec_script(desc->ctx, FALSE, desc->funcs[desc->class_initialize_id].entries[VBDISP_CALLGET],
                           vbdisp, &dp, NULL);
    }

    if(FAILED(hres)) {
        IDispatchEx_Release(&vbdisp->IDispatchEx_iface);
        return hres;
    }

    *ret = vbdisp;
    return S_OK;
}

struct typeinfo_func {
    function_t *func;
    MEMBERID memid;
};

typedef struct {
    ITypeInfo ITypeInfo_iface;
    ITypeComp ITypeComp_iface;
    LONG ref;

    UINT num_vars;
    UINT num_funcs;
    struct typeinfo_func *funcs;

    ScriptDisp *disp;
} ScriptTypeInfo;

static function_t *get_func_from_memid(const ScriptTypeInfo *typeinfo, MEMBERID memid)
{
    UINT a = 0, b = typeinfo->num_funcs;

    if (!(memid & DISPID_FUNCTION_MASK)) return NULL;

    while (a < b)
    {
        UINT i = (a + b - 1) / 2;

        if (memid == typeinfo->funcs[i].memid)
            return typeinfo->funcs[i].func;
        else if (memid < typeinfo->funcs[i].memid)
            b = i;
        else
            a = i + 1;
    }
    return NULL;
}

static inline ScriptTypeInfo *ScriptTypeInfo_from_ITypeInfo(ITypeInfo *iface)
{
    return CONTAINING_RECORD(iface, ScriptTypeInfo, ITypeInfo_iface);
}

static inline ScriptTypeInfo *ScriptTypeInfo_from_ITypeComp(ITypeComp *iface)
{
    return CONTAINING_RECORD(iface, ScriptTypeInfo, ITypeComp_iface);
}

static HRESULT WINAPI ScriptTypeInfo_QueryInterface(ITypeInfo *iface, REFIID riid, void **ppv)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);

    if (IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_ITypeInfo, riid))
        *ppv = &This->ITypeInfo_iface;
    else if (IsEqualGUID(&IID_ITypeComp, riid))
        *ppv = &This->ITypeComp_iface;
    else
    {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ScriptTypeInfo_AddRef(ITypeInfo *iface)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI ScriptTypeInfo_Release(ITypeInfo *iface)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    UINT i;

    TRACE("(%p) ref=%d\n", This, ref);

    if (!ref)
    {
        for (i = 0; i < This->num_funcs; i++)
            release_vbscode(This->funcs[i].func->code_ctx);

        IDispatchEx_Release(&This->disp->IDispatchEx_iface);
        heap_free(This->funcs);
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI ScriptTypeInfo_GetTypeAttr(ITypeInfo *iface, TYPEATTR **ppTypeAttr)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);
    TYPEATTR *attr;

    TRACE("(%p)->(%p)\n", This, ppTypeAttr);

    if (!ppTypeAttr) return E_INVALIDARG;

    attr = heap_alloc_zero(sizeof(*attr));
    if (!attr) return E_OUTOFMEMORY;

    attr->guid = GUID_VBScriptTypeInfo;
    attr->lcid = LOCALE_USER_DEFAULT;
    attr->memidConstructor = MEMBERID_NIL;
    attr->memidDestructor = MEMBERID_NIL;
    attr->cbSizeInstance = 4;
    attr->typekind = TKIND_DISPATCH;
    attr->cFuncs = This->num_funcs;
    attr->cVars = This->num_vars;
    attr->cImplTypes = 1;
    attr->cbSizeVft = sizeof(IDispatchVtbl);
    attr->cbAlignment = 4;
    attr->wTypeFlags = TYPEFLAG_FDISPATCHABLE;
    attr->wMajorVerNum = VBSCRIPT_MAJOR_VERSION;
    attr->wMinorVerNum = VBSCRIPT_MINOR_VERSION;

    *ppTypeAttr = attr;
    return S_OK;
}

static HRESULT WINAPI ScriptTypeInfo_GetTypeComp(ITypeInfo *iface, ITypeComp **ppTComp)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);

    TRACE("(%p)->(%p)\n", This, ppTComp);

    if (!ppTComp) return E_INVALIDARG;

    *ppTComp = &This->ITypeComp_iface;
    ITypeInfo_AddRef(iface);
    return S_OK;
}

static HRESULT WINAPI ScriptTypeInfo_GetFuncDesc(ITypeInfo *iface, UINT index, FUNCDESC **ppFuncDesc)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);
    function_t *func;
    FUNCDESC *desc;
    UINT i;

    TRACE("(%p)->(%u %p)\n", This, index, ppFuncDesc);

    if (!ppFuncDesc) return E_INVALIDARG;
    if (index >= This->num_funcs) return TYPE_E_ELEMENTNOTFOUND;
    func = This->funcs[index].func;

    /* Store the parameter array after the FUNCDESC structure */
    desc = heap_alloc_zero(sizeof(*desc) + sizeof(ELEMDESC) * func->arg_cnt);
    if (!desc) return E_OUTOFMEMORY;

    desc->memid = This->funcs[index].memid;
    desc->funckind = FUNC_DISPATCH;
    desc->invkind = INVOKE_FUNC;
    desc->callconv = CC_STDCALL;
    desc->cParams = func->arg_cnt;
    desc->elemdescFunc.tdesc.vt = (func->type == FUNC_SUB) ? VT_VOID : VT_VARIANT;

    if (func->arg_cnt) desc->lprgelemdescParam = (ELEMDESC*)(desc + 1);
    for (i = 0; i < func->arg_cnt; i++)
        desc->lprgelemdescParam[i].tdesc.vt = VT_VARIANT;

    *ppFuncDesc = desc;
    return S_OK;
}

static HRESULT WINAPI ScriptTypeInfo_GetVarDesc(ITypeInfo *iface, UINT index, VARDESC **ppVarDesc)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);
    VARDESC *desc;

    TRACE("(%p)->(%u %p)\n", This, index, ppVarDesc);

    if (!ppVarDesc) return E_INVALIDARG;
    if (index >= This->num_vars) return TYPE_E_ELEMENTNOTFOUND;

    desc = heap_alloc_zero(sizeof(*desc));
    if (!desc) return E_OUTOFMEMORY;

    desc->memid = index + 1;
    desc->varkind = VAR_DISPATCH;
    desc->elemdescVar.tdesc.vt = VT_VARIANT;

    *ppVarDesc = desc;
    return S_OK;
}

static HRESULT WINAPI ScriptTypeInfo_GetNames(ITypeInfo *iface, MEMBERID memid, BSTR *rgBstrNames,
        UINT cMaxNames, UINT *pcNames)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);
    ITypeInfo *disp_typeinfo;
    function_t *func;
    HRESULT hr;
    UINT i = 0;

    TRACE("(%p)->(%d %p %u %p)\n", This, memid, rgBstrNames, cMaxNames, pcNames);

    if (!rgBstrNames || !pcNames) return E_INVALIDARG;
    if (memid <= 0) return TYPE_E_ELEMENTNOTFOUND;

    func = get_func_from_memid(This, memid);
    if (!func && memid > This->num_vars)
    {
        hr = get_dispatch_typeinfo(&disp_typeinfo);
        if (FAILED(hr)) return hr;

        return ITypeInfo_GetNames(disp_typeinfo, memid, rgBstrNames, cMaxNames, pcNames);
    }

    *pcNames = 0;
    if (!cMaxNames) return S_OK;

    if (func)
    {
        UINT num = min(cMaxNames, func->arg_cnt + 1);

        rgBstrNames[0] = SysAllocString(func->name);
        if (!rgBstrNames[0]) return E_OUTOFMEMORY;

        for (i = 1; i < num; i++)
        {
            if (!(rgBstrNames[i] = SysAllocString(func->args[i - 1].name)))
            {
                do SysFreeString(rgBstrNames[--i]); while (i);
                return E_OUTOFMEMORY;
            }
        }
    }
    else
    {
        rgBstrNames[0] = SysAllocString(This->disp->global_vars[memid - 1]->name);
        if (!rgBstrNames[0]) return E_OUTOFMEMORY;
        i++;
    }

    *pcNames = i;
    return S_OK;
}

static HRESULT WINAPI ScriptTypeInfo_GetRefTypeOfImplType(ITypeInfo *iface, UINT index, HREFTYPE *pRefType)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);

    TRACE("(%p)->(%u %p)\n", This, index, pRefType);

    /* We only inherit from IDispatch */
    if (!pRefType) return E_INVALIDARG;
    if (index != 0) return TYPE_E_ELEMENTNOTFOUND;

    *pRefType = 1;
    return S_OK;
}

static HRESULT WINAPI ScriptTypeInfo_GetImplTypeFlags(ITypeInfo *iface, UINT index, INT *pImplTypeFlags)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);

    TRACE("(%p)->(%u %p)\n", This, index, pImplTypeFlags);

    if (!pImplTypeFlags) return E_INVALIDARG;
    if (index != 0) return TYPE_E_ELEMENTNOTFOUND;

    *pImplTypeFlags = 0;
    return S_OK;
}

static HRESULT WINAPI ScriptTypeInfo_GetIDsOfNames(ITypeInfo *iface, LPOLESTR *rgszNames, UINT cNames,
        MEMBERID *pMemId)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);
    ITypeInfo *disp_typeinfo;
    const WCHAR *name;
    HRESULT hr = S_OK;
    int i, j, arg;

    TRACE("(%p)->(%p %u %p)\n", This, rgszNames, cNames, pMemId);

    if (!rgszNames || !cNames || !pMemId) return E_INVALIDARG;

    for (i = 0; i < cNames; i++) pMemId[i] = MEMBERID_NIL;
    name = rgszNames[0];

    for (i = 0; i < This->num_funcs; i++)
    {
        function_t *func = This->funcs[i].func;

        if (wcsicmp(name, func->name)) continue;
        pMemId[0] = This->funcs[i].memid;

        for (j = 1; j < cNames; j++)
        {
            name = rgszNames[j];
            for (arg = func->arg_cnt; --arg >= 0;)
                if (!wcsicmp(name, func->args[arg].name))
                    break;
            if (arg >= 0)
                pMemId[j] = arg;
            else
                hr = DISP_E_UNKNOWNNAME;
        }
        return hr;
    }

    for (i = 0; i < This->num_vars; i++)
    {
        if (wcsicmp(name, This->disp->global_vars[i]->name)) continue;
        pMemId[0] = i + 1;
        return S_OK;
    }

    /* Look into the inherited IDispatch */
    hr = get_dispatch_typeinfo(&disp_typeinfo);
    if (FAILED(hr)) return hr;

    return ITypeInfo_GetIDsOfNames(disp_typeinfo, rgszNames, cNames, pMemId);
}

static HRESULT WINAPI ScriptTypeInfo_Invoke(ITypeInfo *iface, PVOID pvInstance, MEMBERID memid, WORD wFlags,
        DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);
    ITypeInfo *disp_typeinfo;
    IDispatch *disp;
    HRESULT hr;

    TRACE("(%p)->(%p %d %d %p %p %p %p)\n", This, pvInstance, memid, wFlags,
          pDispParams, pVarResult, pExcepInfo, puArgErr);

    if (!pvInstance) return E_INVALIDARG;
    if (memid <= 0) return TYPE_E_ELEMENTNOTFOUND;

    if (!get_func_from_memid(This, memid) && memid > This->num_vars)
    {
        hr = get_dispatch_typeinfo(&disp_typeinfo);
        if (FAILED(hr)) return hr;

        return ITypeInfo_Invoke(disp_typeinfo, pvInstance, memid, wFlags, pDispParams,
                                pVarResult, pExcepInfo, puArgErr);
    }

    hr = IUnknown_QueryInterface((IUnknown*)pvInstance, &IID_IDispatch, (void**)&disp);
    if (FAILED(hr)) return hr;

    hr = IDispatch_Invoke(disp, memid, &IID_NULL, LOCALE_USER_DEFAULT, wFlags,
                          pDispParams, pVarResult, pExcepInfo, puArgErr);
    IDispatch_Release(disp);

    return hr;
}

static HRESULT WINAPI ScriptTypeInfo_GetDocumentation(ITypeInfo *iface, MEMBERID memid, BSTR *pBstrName,
        BSTR *pBstrDocString, DWORD *pdwHelpContext, BSTR *pBstrHelpFile)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);
    ITypeInfo *disp_typeinfo;
    function_t *func;
    HRESULT hr;

    TRACE("(%p)->(%d %p %p %p %p)\n", This, memid, pBstrName, pBstrDocString, pdwHelpContext, pBstrHelpFile);

    if (pBstrDocString) *pBstrDocString = NULL;
    if (pdwHelpContext) *pdwHelpContext = 0;
    if (pBstrHelpFile) *pBstrHelpFile = NULL;

    if (memid == MEMBERID_NIL)
    {
        if (pBstrName && !(*pBstrName = SysAllocString(L"VBScriptTypeInfo")))
            return E_OUTOFMEMORY;
        if (pBstrDocString &&
            !(*pBstrDocString = SysAllocString(L"Visual Basic Scripting Type Info")))
        {
            if (pBstrName) SysFreeString(*pBstrName);
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }
    if (memid <= 0) return TYPE_E_ELEMENTNOTFOUND;

    func = get_func_from_memid(This, memid);
    if (!func && memid > This->num_vars)
    {
        hr = get_dispatch_typeinfo(&disp_typeinfo);
        if (FAILED(hr)) return hr;

        return ITypeInfo_GetDocumentation(disp_typeinfo, memid, pBstrName, pBstrDocString,
                                          pdwHelpContext, pBstrHelpFile);
    }

    if (pBstrName)
    {
        *pBstrName = SysAllocString(func ? func->name : This->disp->global_vars[memid - 1]->name);
        if (!*pBstrName) return E_OUTOFMEMORY;
    }
    return S_OK;
}

static HRESULT WINAPI ScriptTypeInfo_GetDllEntry(ITypeInfo *iface, MEMBERID memid, INVOKEKIND invKind,
        BSTR *pBstrDllName, BSTR *pBstrName, WORD *pwOrdinal)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);
    ITypeInfo *disp_typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %d %p %p %p)\n", This, memid, invKind, pBstrDllName, pBstrName, pwOrdinal);

    if (pBstrDllName) *pBstrDllName = NULL;
    if (pBstrName) *pBstrName = NULL;
    if (pwOrdinal) *pwOrdinal = 0;

    if (!get_func_from_memid(This, memid) && memid > This->num_vars)
    {
        hr = get_dispatch_typeinfo(&disp_typeinfo);
        if (FAILED(hr)) return hr;

        return ITypeInfo_GetDllEntry(disp_typeinfo, memid, invKind, pBstrDllName, pBstrName, pwOrdinal);
    }
    return TYPE_E_BADMODULEKIND;
}

static HRESULT WINAPI ScriptTypeInfo_GetRefTypeInfo(ITypeInfo *iface, HREFTYPE hRefType, ITypeInfo **ppTInfo)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);
    HRESULT hr;

    TRACE("(%p)->(%x %p)\n", This, hRefType, ppTInfo);

    if (!ppTInfo || (INT)hRefType < 0) return E_INVALIDARG;

    if (hRefType & ~3) return E_FAIL;
    if (hRefType & 1)
    {
        hr = get_dispatch_typeinfo(ppTInfo);
        if (FAILED(hr)) return hr;
    }
    else
        *ppTInfo = iface;

    ITypeInfo_AddRef(*ppTInfo);
    return S_OK;
}

static HRESULT WINAPI ScriptTypeInfo_AddressOfMember(ITypeInfo *iface, MEMBERID memid, INVOKEKIND invKind, PVOID *ppv)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);
    ITypeInfo *disp_typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %d %p)\n", This, memid, invKind, ppv);

    if (!ppv) return E_INVALIDARG;
    *ppv = NULL;

    if (!get_func_from_memid(This, memid) && memid > This->num_vars)
    {
        hr = get_dispatch_typeinfo(&disp_typeinfo);
        if (FAILED(hr)) return hr;

        return ITypeInfo_AddressOfMember(disp_typeinfo, memid, invKind, ppv);
    }
    return TYPE_E_BADMODULEKIND;
}

static HRESULT WINAPI ScriptTypeInfo_CreateInstance(ITypeInfo *iface, IUnknown *pUnkOuter, REFIID riid, PVOID *ppvObj)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);

    TRACE("(%p)->(%p %s %p)\n", This, pUnkOuter, debugstr_guid(riid), ppvObj);

    if (!ppvObj) return E_INVALIDARG;

    *ppvObj = NULL;
    return TYPE_E_BADMODULEKIND;
}

static HRESULT WINAPI ScriptTypeInfo_GetMops(ITypeInfo *iface, MEMBERID memid, BSTR *pBstrMops)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);
    ITypeInfo *disp_typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %p)\n", This, memid, pBstrMops);

    if (!pBstrMops) return E_INVALIDARG;

    if (!get_func_from_memid(This, memid) && memid > This->num_vars)
    {
        hr = get_dispatch_typeinfo(&disp_typeinfo);
        if (FAILED(hr)) return hr;

        return ITypeInfo_GetMops(disp_typeinfo, memid, pBstrMops);
    }

    *pBstrMops = NULL;
    return S_OK;
}

static HRESULT WINAPI ScriptTypeInfo_GetContainingTypeLib(ITypeInfo *iface, ITypeLib **ppTLib, UINT *pIndex)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);

    FIXME("(%p)->(%p %p)\n", This, ppTLib, pIndex);

    return E_NOTIMPL;
}

static void WINAPI ScriptTypeInfo_ReleaseTypeAttr(ITypeInfo *iface, TYPEATTR *pTypeAttr)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);

    TRACE("(%p)->(%p)\n", This, pTypeAttr);

    heap_free(pTypeAttr);
}

static void WINAPI ScriptTypeInfo_ReleaseFuncDesc(ITypeInfo *iface, FUNCDESC *pFuncDesc)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);

    TRACE("(%p)->(%p)\n", This, pFuncDesc);

    heap_free(pFuncDesc);
}

static void WINAPI ScriptTypeInfo_ReleaseVarDesc(ITypeInfo *iface, VARDESC *pVarDesc)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeInfo(iface);

    TRACE("(%p)->(%p)\n", This, pVarDesc);

    heap_free(pVarDesc);
}

static const ITypeInfoVtbl ScriptTypeInfoVtbl = {
    ScriptTypeInfo_QueryInterface,
    ScriptTypeInfo_AddRef,
    ScriptTypeInfo_Release,
    ScriptTypeInfo_GetTypeAttr,
    ScriptTypeInfo_GetTypeComp,
    ScriptTypeInfo_GetFuncDesc,
    ScriptTypeInfo_GetVarDesc,
    ScriptTypeInfo_GetNames,
    ScriptTypeInfo_GetRefTypeOfImplType,
    ScriptTypeInfo_GetImplTypeFlags,
    ScriptTypeInfo_GetIDsOfNames,
    ScriptTypeInfo_Invoke,
    ScriptTypeInfo_GetDocumentation,
    ScriptTypeInfo_GetDllEntry,
    ScriptTypeInfo_GetRefTypeInfo,
    ScriptTypeInfo_AddressOfMember,
    ScriptTypeInfo_CreateInstance,
    ScriptTypeInfo_GetMops,
    ScriptTypeInfo_GetContainingTypeLib,
    ScriptTypeInfo_ReleaseTypeAttr,
    ScriptTypeInfo_ReleaseFuncDesc,
    ScriptTypeInfo_ReleaseVarDesc
};

static HRESULT WINAPI ScriptTypeComp_QueryInterface(ITypeComp *iface, REFIID riid, void **ppv)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeComp(iface);
    return ITypeInfo_QueryInterface(&This->ITypeInfo_iface, riid, ppv);
}

static ULONG WINAPI ScriptTypeComp_AddRef(ITypeComp *iface)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeComp(iface);
    return ITypeInfo_AddRef(&This->ITypeInfo_iface);
}

static ULONG WINAPI ScriptTypeComp_Release(ITypeComp *iface)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeComp(iface);
    return ITypeInfo_Release(&This->ITypeInfo_iface);
}

static HRESULT WINAPI ScriptTypeComp_Bind(ITypeComp *iface, LPOLESTR szName, ULONG lHashVal, WORD wFlags,
        ITypeInfo **ppTInfo, DESCKIND *pDescKind, BINDPTR *pBindPtr)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeComp(iface);
    UINT flags = wFlags ? wFlags : ~0;
    ITypeInfo *disp_typeinfo;
    ITypeComp *disp_typecomp;
    HRESULT hr;
    UINT i;

    TRACE("(%p)->(%s %08x %d %p %p %p)\n", This, debugstr_w(szName), lHashVal,
          wFlags, ppTInfo, pDescKind, pBindPtr);

    if (!szName || !ppTInfo || !pDescKind || !pBindPtr)
        return E_INVALIDARG;

    for (i = 0; i < This->num_funcs; i++)
    {
        if (wcsicmp(szName, This->funcs[i].func->name)) continue;
        if (!(flags & INVOKE_FUNC)) return TYPE_E_TYPEMISMATCH;

        hr = ITypeInfo_GetFuncDesc(&This->ITypeInfo_iface, i, &pBindPtr->lpfuncdesc);
        if (FAILED(hr)) return hr;

        *pDescKind = DESCKIND_FUNCDESC;
        *ppTInfo = &This->ITypeInfo_iface;
        ITypeInfo_AddRef(*ppTInfo);
        return S_OK;
    }

    for (i = 0; i < This->num_vars; i++)
    {
        if (wcsicmp(szName, This->disp->global_vars[i]->name)) continue;
        if (!(flags & INVOKE_PROPERTYGET)) return TYPE_E_TYPEMISMATCH;

        hr = ITypeInfo_GetVarDesc(&This->ITypeInfo_iface, i, &pBindPtr->lpvardesc);
        if (FAILED(hr)) return hr;

        *pDescKind = DESCKIND_VARDESC;
        *ppTInfo = &This->ITypeInfo_iface;
        ITypeInfo_AddRef(*ppTInfo);
        return S_OK;
    }

    /* Look into the inherited IDispatch */
    hr = get_dispatch_typeinfo(&disp_typeinfo);
    if (FAILED(hr)) return hr;

    hr = ITypeInfo_GetTypeComp(disp_typeinfo, &disp_typecomp);
    if (FAILED(hr)) return hr;

    hr = ITypeComp_Bind(disp_typecomp, szName, lHashVal, wFlags, ppTInfo, pDescKind, pBindPtr);
    ITypeComp_Release(disp_typecomp);
    return hr;
}

static HRESULT WINAPI ScriptTypeComp_BindType(ITypeComp *iface, LPOLESTR szName, ULONG lHashVal,
        ITypeInfo **ppTInfo, ITypeComp **ppTComp)
{
    ScriptTypeInfo *This = ScriptTypeInfo_from_ITypeComp(iface);
    ITypeInfo *disp_typeinfo;
    ITypeComp *disp_typecomp;
    HRESULT hr;

    TRACE("(%p)->(%s %08x %p %p)\n", This, debugstr_w(szName), lHashVal, ppTInfo, ppTComp);

    if (!szName || !ppTInfo || !ppTComp)
        return E_INVALIDARG;

    /* Look into the inherited IDispatch */
    hr = get_dispatch_typeinfo(&disp_typeinfo);
    if (FAILED(hr)) return hr;

    hr = ITypeInfo_GetTypeComp(disp_typeinfo, &disp_typecomp);
    if (FAILED(hr)) return hr;

    hr = ITypeComp_BindType(disp_typecomp, szName, lHashVal, ppTInfo, ppTComp);
    ITypeComp_Release(disp_typecomp);
    return hr;
}

static const ITypeCompVtbl ScriptTypeCompVtbl = {
    ScriptTypeComp_QueryInterface,
    ScriptTypeComp_AddRef,
    ScriptTypeComp_Release,
    ScriptTypeComp_Bind,
    ScriptTypeComp_BindType
};

static inline ScriptDisp *ScriptDisp_from_IDispatchEx(IDispatchEx *iface)
{
    return CONTAINING_RECORD(iface, ScriptDisp, IDispatchEx_iface);
}

static HRESULT WINAPI ScriptDisp_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    ScriptDisp *This = ScriptDisp_from_IDispatchEx(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IDispatchEx_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IDispatchEx_iface;
    }else if(IsEqualGUID(&IID_IDispatchEx, riid)) {
        TRACE("(%p)->(IID_IDispatchEx %p)\n", This, ppv);
        *ppv = &This->IDispatchEx_iface;
    }else {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ScriptDisp_AddRef(IDispatchEx *iface)
{
    ScriptDisp *This = ScriptDisp_from_IDispatchEx(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI ScriptDisp_Release(IDispatchEx *iface)
{
    ScriptDisp *This = ScriptDisp_from_IDispatchEx(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    unsigned i;

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        assert(!This->ctx);

        for (i = 0; i < This->global_vars_cnt; i++)
            release_dynamic_var(This->global_vars[i]);

        heap_pool_free(&This->heap);
        heap_free(This->global_vars);
        heap_free(This->global_funcs);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI ScriptDisp_GetTypeInfoCount(IDispatchEx *iface, UINT *pctinfo)
{
    ScriptDisp *This = ScriptDisp_from_IDispatchEx(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI ScriptDisp_GetTypeInfo(IDispatchEx *iface, UINT iTInfo, LCID lcid, ITypeInfo **ret)
{
    ScriptDisp *This = ScriptDisp_from_IDispatchEx(iface);
    ScriptTypeInfo *type_info;
    UINT num_funcs = 0;
    unsigned i, j;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ret);

    if(iTInfo)
        return DISP_E_BADINDEX;

    if(!(type_info = heap_alloc(sizeof(*type_info))))
        return E_OUTOFMEMORY;

    for(i = 0; i < This->global_funcs_cnt; i++)
        if(This->global_funcs[i]->is_public)
            num_funcs++;

    type_info->ITypeInfo_iface.lpVtbl = &ScriptTypeInfoVtbl;
    type_info->ITypeComp_iface.lpVtbl = &ScriptTypeCompVtbl;
    type_info->ref = 1;
    type_info->num_funcs = num_funcs;
    type_info->num_vars = This->global_vars_cnt;
    type_info->disp = This;

    type_info->funcs = heap_alloc(sizeof(*type_info->funcs) * num_funcs);
    if(!type_info->funcs)
    {
        heap_free(type_info);
        return E_OUTOFMEMORY;
    }

    for(j = 0, i = 0; i < This->global_funcs_cnt; i++)
    {
        if(!This->global_funcs[i]->is_public) continue;

        type_info->funcs[j].memid = i + 1 + DISPID_FUNCTION_MASK;
        type_info->funcs[j].func = This->global_funcs[i];
        grab_vbscode(This->global_funcs[i]->code_ctx);
        j++;
    }

    IDispatchEx_AddRef(&This->IDispatchEx_iface);

    *ret = &type_info->ITypeInfo_iface;
    return S_OK;
}

static HRESULT WINAPI ScriptDisp_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ScriptDisp *This = ScriptDisp_from_IDispatchEx(iface);
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

static HRESULT WINAPI ScriptDisp_Invoke(IDispatchEx *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
         WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ScriptDisp *This = ScriptDisp_from_IDispatchEx(iface);

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    return IDispatchEx_InvokeEx(&This->IDispatchEx_iface, dispIdMember, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, NULL);
}

static HRESULT WINAPI ScriptDisp_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    ScriptDisp *This = ScriptDisp_from_IDispatchEx(iface);
    unsigned i;

    TRACE("(%p)->(%s %x %p)\n", This, debugstr_w(bstrName), grfdex, pid);

    if(!This->ctx)
        return E_UNEXPECTED;

    for(i = 0; i < This->global_vars_cnt; i++) {
        if(!wcsicmp(This->global_vars[i]->name, bstrName)) {
            *pid = i + 1;
            return S_OK;
        }
    }

    for(i = 0; i < This->global_funcs_cnt; i++) {
        if(!wcsicmp(This->global_funcs[i]->name, bstrName)) {
            *pid = i + 1 + DISPID_FUNCTION_MASK;
            return S_OK;
        }
    }

    *pid = -1;
    return DISP_E_UNKNOWNNAME;
}

static HRESULT WINAPI ScriptDisp_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    ScriptDisp *This = ScriptDisp_from_IDispatchEx(iface);
    HRESULT hres;

    TRACE("(%p)->(%x %x %x %p %p %p %p)\n", This, id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);

    if (!This->ctx)
        return E_UNEXPECTED;

    if (id & DISPID_FUNCTION_MASK)
    {
        id &= ~DISPID_FUNCTION_MASK;
        if (id > This->global_funcs_cnt)
            return DISP_E_MEMBERNOTFOUND;

        switch (wFlags)
        {
        case DISPATCH_METHOD:
        case DISPATCH_METHOD | DISPATCH_PROPERTYGET:
            hres = exec_script(This->ctx, TRUE, This->global_funcs[id - 1], NULL, pdp, pvarRes);
            break;
        default:
            FIXME("Unsupported flags %x\n", wFlags);
            hres = E_NOTIMPL;
        }

        return hres;
    }

    if (id > This->global_vars_cnt)
        return DISP_E_MEMBERNOTFOUND;

    if (This->global_vars[id - 1]->is_const)
    {
        FIXME("const not supported\n");
        return E_NOTIMPL;
    }

    return invoke_variant_prop(This->ctx, &This->global_vars[id - 1]->v, wFlags, pdp, pvarRes);
}

static HRESULT WINAPI ScriptDisp_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    ScriptDisp *This = ScriptDisp_from_IDispatchEx(iface);
    FIXME("(%p)->(%s %x)\n", This, debugstr_w(bstrName), grfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI ScriptDisp_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    ScriptDisp *This = ScriptDisp_from_IDispatchEx(iface);
    FIXME("(%p)->(%x)\n", This, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI ScriptDisp_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    ScriptDisp *This = ScriptDisp_from_IDispatchEx(iface);
    FIXME("(%p)->(%x %x %p)\n", This, id, grfdexFetch, pgrfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI ScriptDisp_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *pbstrName)
{
    ScriptDisp *This = ScriptDisp_from_IDispatchEx(iface);
    FIXME("(%p)->(%x %p)\n", This, id, pbstrName);
    return E_NOTIMPL;
}

static HRESULT WINAPI ScriptDisp_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    ScriptDisp *This = ScriptDisp_from_IDispatchEx(iface);
    FIXME("(%p)->(%x %x %p)\n", This, grfdex, id, pid);
    return E_NOTIMPL;
}

static HRESULT WINAPI ScriptDisp_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    ScriptDisp *This = ScriptDisp_from_IDispatchEx(iface);
    FIXME("(%p)->(%p)\n", This, ppunk);
    return E_NOTIMPL;
}

static IDispatchExVtbl ScriptDispVtbl = {
    ScriptDisp_QueryInterface,
    ScriptDisp_AddRef,
    ScriptDisp_Release,
    ScriptDisp_GetTypeInfoCount,
    ScriptDisp_GetTypeInfo,
    ScriptDisp_GetIDsOfNames,
    ScriptDisp_Invoke,
    ScriptDisp_GetDispID,
    ScriptDisp_InvokeEx,
    ScriptDisp_DeleteMemberByName,
    ScriptDisp_DeleteMemberByDispID,
    ScriptDisp_GetMemberProperties,
    ScriptDisp_GetMemberName,
    ScriptDisp_GetNextDispID,
    ScriptDisp_GetNameSpaceParent
};

HRESULT create_script_disp(script_ctx_t *ctx, ScriptDisp **ret)
{
    ScriptDisp *script_disp;

    script_disp = heap_alloc_zero(sizeof(*script_disp));
    if(!script_disp)
        return E_OUTOFMEMORY;

    script_disp->IDispatchEx_iface.lpVtbl = &ScriptDispVtbl;
    script_disp->ref = 1;
    script_disp->ctx = ctx;
    heap_pool_init(&script_disp->heap);

    *ret = script_disp;
    return S_OK;
}

void collect_objects(script_ctx_t *ctx)
{
    vbdisp_t *iter, *iter2;

    LIST_FOR_EACH_ENTRY_SAFE(iter, iter2, &ctx->objects, vbdisp_t, entry)
        run_terminator(iter);

    while(!list_empty(&ctx->objects)) {
        iter = LIST_ENTRY(list_head(&ctx->objects), vbdisp_t, entry);

        IDispatchEx_AddRef(&iter->IDispatchEx_iface);
        clean_props(iter);
        iter->desc = NULL;
        list_remove(&iter->entry);
        list_init(&iter->entry);
        IDispatchEx_Release(&iter->IDispatchEx_iface);
    }
}

HRESULT disp_get_id(IDispatch *disp, BSTR name, vbdisp_invoke_type_t invoke_type, BOOL search_private, DISPID *id)
{
    IDispatchEx *dispex;
    vbdisp_t *vbdisp;
    HRESULT hres;

    vbdisp = unsafe_impl_from_IDispatch(disp);
    if(vbdisp)
        return vbdisp_get_id(vbdisp, name, invoke_type, search_private, id);

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    if(FAILED(hres)) {
        TRACE("using IDispatch\n");
        return IDispatch_GetIDsOfNames(disp, &IID_NULL, &name, 1, 0, id);
    }

    hres = IDispatchEx_GetDispID(dispex, name, fdexNameCaseInsensitive, id);
    IDispatchEx_Release(dispex);
    return hres;
}

#define RPC_E_SERVER_UNAVAILABLE 0x800706ba

HRESULT map_hres(HRESULT hres)
{
    if(SUCCEEDED(hres) || HRESULT_FACILITY(hres) == FACILITY_VBS)
        return hres;

    switch(hres) {
    case E_NOTIMPL:                  return MAKE_VBSERROR(VBSE_ACTION_NOT_SUPPORTED);
    case E_NOINTERFACE:              return MAKE_VBSERROR(VBSE_OLE_NOT_SUPPORTED);
    case DISP_E_UNKNOWNINTERFACE:    return MAKE_VBSERROR(VBSE_OLE_NO_PROP_OR_METHOD);
    case DISP_E_MEMBERNOTFOUND:      return MAKE_VBSERROR(VBSE_OLE_NO_PROP_OR_METHOD);
    case DISP_E_PARAMNOTFOUND:       return MAKE_VBSERROR(VBSE_NAMED_PARAM_NOT_FOUND);
    case DISP_E_TYPEMISMATCH:        return MAKE_VBSERROR(VBSE_TYPE_MISMATCH);
    case DISP_E_UNKNOWNNAME:         return MAKE_VBSERROR(VBSE_OLE_NO_PROP_OR_METHOD);
    case DISP_E_NONAMEDARGS:         return MAKE_VBSERROR(VBSE_NAMED_ARGS_NOT_SUPPORTED);
    case DISP_E_BADVARTYPE:          return MAKE_VBSERROR(VBSE_INVALID_TYPELIB_VARIABLE);
    case DISP_E_OVERFLOW:            return MAKE_VBSERROR(VBSE_OVERFLOW);
    case DISP_E_BADINDEX:            return MAKE_VBSERROR(VBSE_OUT_OF_BOUNDS);
    case DISP_E_UNKNOWNLCID:         return MAKE_VBSERROR(VBSE_LOCALE_SETTING_NOT_SUPPORTED);
    case DISP_E_ARRAYISLOCKED:       return MAKE_VBSERROR(VBSE_ARRAY_LOCKED);
    case DISP_E_BADPARAMCOUNT:       return MAKE_VBSERROR(VBSE_FUNC_ARITY_MISMATCH);
    case DISP_E_PARAMNOTOPTIONAL:    return MAKE_VBSERROR(VBSE_PARAMETER_NOT_OPTIONAL);
    case DISP_E_NOTACOLLECTION:      return MAKE_VBSERROR(VBSE_NOT_ENUM);
    case TYPE_E_DLLFUNCTIONNOTFOUND: return MAKE_VBSERROR(VBSE_INVALID_DLL_FUNCTION_NAME);
    case TYPE_E_TYPEMISMATCH:        return MAKE_VBSERROR(VBSE_TYPE_MISMATCH);
    case TYPE_E_OUTOFBOUNDS:         return MAKE_VBSERROR(VBSE_OUT_OF_BOUNDS);
    case TYPE_E_IOERROR:             return MAKE_VBSERROR(VBSE_IO_ERROR);
    case TYPE_E_CANTCREATETMPFILE:   return MAKE_VBSERROR(VBSE_CANT_CREATE_TMP_FILE);
    case STG_E_FILENOTFOUND:         return MAKE_VBSERROR(VBSE_OLE_FILE_NOT_FOUND);
    case STG_E_PATHNOTFOUND:         return MAKE_VBSERROR(VBSE_PATH_NOT_FOUND);
    case STG_E_TOOMANYOPENFILES:     return MAKE_VBSERROR(VBSE_TOO_MANY_FILES);
    case STG_E_ACCESSDENIED:         return MAKE_VBSERROR(VBSE_PERMISSION_DENIED);
    case STG_E_INSUFFICIENTMEMORY:   return MAKE_VBSERROR(VBSE_OUT_OF_MEMORY);
    case STG_E_NOMOREFILES:          return MAKE_VBSERROR(VBSE_TOO_MANY_FILES);
    case STG_E_DISKISWRITEPROTECTED: return MAKE_VBSERROR(VBSE_PERMISSION_DENIED);
    case STG_E_WRITEFAULT:           return MAKE_VBSERROR(VBSE_IO_ERROR);
    case STG_E_READFAULT:            return MAKE_VBSERROR(VBSE_IO_ERROR);
    case STG_E_SHAREVIOLATION:       return MAKE_VBSERROR(VBSE_PATH_FILE_ACCESS);
    case STG_E_LOCKVIOLATION:        return MAKE_VBSERROR(VBSE_PERMISSION_DENIED);
    case STG_E_FILEALREADYEXISTS:    return MAKE_VBSERROR(VBSE_FILE_ALREADY_EXISTS);
    case STG_E_MEDIUMFULL:           return MAKE_VBSERROR(VBSE_DISK_FULL);
    case STG_E_INVALIDNAME:          return MAKE_VBSERROR(VBSE_FILE_NOT_FOUND);
    case STG_E_INUSE:                return MAKE_VBSERROR(VBSE_PERMISSION_DENIED);
    case STG_E_NOTCURRENT:           return MAKE_VBSERROR(VBSE_PERMISSION_DENIED);
    case STG_E_CANTSAVE:             return MAKE_VBSERROR(VBSE_IO_ERROR);
    case REGDB_E_CLASSNOTREG:        return MAKE_VBSERROR(VBSE_CANT_CREATE_OBJECT);
    case MK_E_UNAVAILABLE:           return MAKE_VBSERROR(VBSE_CANT_CREATE_OBJECT);
    case MK_E_INVALIDEXTENSION:      return MAKE_VBSERROR(VBSE_OLE_FILE_NOT_FOUND);
    case MK_E_CANTOPENFILE:          return MAKE_VBSERROR(VBSE_OLE_FILE_NOT_FOUND);
    case CO_E_CLASSSTRING:           return MAKE_VBSERROR(VBSE_CANT_CREATE_OBJECT);
    case CO_E_APPNOTFOUND:           return MAKE_VBSERROR(VBSE_CANT_CREATE_OBJECT);
    case CO_E_APPDIDNTREG:           return MAKE_VBSERROR(VBSE_CANT_CREATE_OBJECT);
    case E_ACCESSDENIED:             return MAKE_VBSERROR(VBSE_PERMISSION_DENIED);
    case E_OUTOFMEMORY:              return MAKE_VBSERROR(VBSE_OUT_OF_MEMORY);
    case E_INVALIDARG:               return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
    case RPC_E_SERVER_UNAVAILABLE:   return MAKE_VBSERROR(VBSE_SERVER_NOT_FOUND);
    case CO_E_SERVER_EXEC_FAILURE:   return MAKE_VBSERROR(VBSE_CANT_CREATE_OBJECT);
    }

    return hres;
}

HRESULT disp_call(script_ctx_t *ctx, IDispatch *disp, DISPID id, DISPPARAMS *dp, VARIANT *retv)
{
    const WORD flags = DISPATCH_METHOD|(retv ? DISPATCH_PROPERTYGET : 0);
    IDispatchEx *dispex;
    vbdisp_t *vbdisp;
    EXCEPINFO ei;
    HRESULT hres;

    memset(&ei, 0, sizeof(ei));
    if(retv)
        V_VT(retv) = VT_EMPTY;

    vbdisp = unsafe_impl_from_IDispatch(disp);
    if(vbdisp && vbdisp->desc && vbdisp->desc->ctx == ctx)
        return invoke_vbdisp(vbdisp, id, flags, FALSE, dp, retv);

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    if(SUCCEEDED(hres)) {
        hres = IDispatchEx_InvokeEx(dispex, id, ctx->lcid, flags, dp, retv, &ei, NULL /* CALLER_FIXME */);
        IDispatchEx_Release(dispex);
    }else {
        UINT err = 0;

        TRACE("using IDispatch\n");
        hres = IDispatch_Invoke(disp, id, &IID_NULL, ctx->lcid, flags, dp, retv, &ei, &err);
    }

    if(hres == DISP_E_EXCEPTION) {
        clear_ei(&ctx->ei);
        ctx->ei = ei;
        hres = SCRIPT_E_RECORDED;
    }
    return hres;
}

HRESULT get_disp_value(script_ctx_t *ctx, IDispatch *disp, VARIANT *v)
{
    DISPPARAMS dp = {NULL};
    if(!disp)
        return MAKE_VBSERROR(VBSE_OBJECT_VARIABLE_NOT_SET);
    return disp_call(ctx, disp, DISPID_VALUE, &dp, v);
}

HRESULT disp_propput(script_ctx_t *ctx, IDispatch *disp, DISPID id, WORD flags, DISPPARAMS *dp)
{
    IDispatchEx *dispex;
    vbdisp_t *vbdisp;
    EXCEPINFO ei = {0};
    HRESULT hres;

    vbdisp = unsafe_impl_from_IDispatch(disp);
    if(vbdisp && vbdisp->desc && vbdisp->desc->ctx == ctx)
        return invoke_vbdisp(vbdisp, id, flags, FALSE, dp, NULL);

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    if(SUCCEEDED(hres)) {
        hres = IDispatchEx_InvokeEx(dispex, id, ctx->lcid, flags, dp, NULL, &ei, NULL /* FIXME! */);
        IDispatchEx_Release(dispex);
    }else {
        ULONG err = 0;

        TRACE("using IDispatch\n");
        hres = IDispatch_Invoke(disp, id, &IID_NULL, ctx->lcid, flags, dp, NULL, &ei, &err);
    }

    if(hres == DISP_E_EXCEPTION) {
        clear_ei(&ctx->ei);
        ctx->ei = ei;
        hres = SCRIPT_E_RECORDED;
    }
    return hres;
}
