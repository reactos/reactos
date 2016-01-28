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

#include "vbscript.h"

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

        if(!strcmpiW(This->desc->funcs[i].name, name)) {
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

        if(!strcmpiW(This->desc->props[i].name, name)) {
            *id = i + This->desc->func_cnt;
            return S_OK;
        }
    }

    if(This->desc->typeinfo) {
        HRESULT hres;

        hres = ITypeInfo_GetIDsOfNames(This->desc->typeinfo, &name, 1, id);
        if(SUCCEEDED(hres))
            return S_OK;
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

static HRESULT invoke_builtin(vbdisp_t *This, const builtin_prop_t *prop, WORD flags, DISPPARAMS *dp, VARIANT *res)
{
    VARIANT args[8];
    unsigned argn, i;

    switch(flags) {
    case DISPATCH_PROPERTYGET:
        if(!(prop->flags & (BP_GET|BP_GETPUT))) {
            FIXME("property does not support DISPATCH_PROPERTYGET\n");
            return E_FAIL;
        }
        break;
    case DISPATCH_PROPERTYGET|DISPATCH_METHOD:
        if(!prop->proc && prop->flags == BP_GET) {
            const int vt = prop->min_args, val = prop->max_args;
            switch(vt) {
            case VT_I2:
                V_VT(res) = VT_I2;
                V_I2(res) = val;
                break;
            case VT_I4:
                V_VT(res) = VT_I4;
                V_I4(res) = val;
                break;
            case VT_BSTR: {
                const string_constant_t *str = (const string_constant_t*)prop->max_args;
                BSTR ret;

                ret = SysAllocStringLen(str->buf, str->len);
                if(!ret)
                    return E_OUTOFMEMORY;

                V_VT(res) = VT_BSTR;
                V_BSTR(res) = ret;
                break;
            }
            DEFAULT_UNREACHABLE;
            }
            return S_OK;
        }
        break;
    case DISPATCH_METHOD:
        if(prop->flags & (BP_GET|BP_GETPUT)) {
            FIXME("Call on property\n");
            return E_FAIL;
        }
        break;
    case DISPATCH_PROPERTYPUT:
        if(!(prop->flags & BP_GETPUT)) {
            FIXME("property does not support DISPATCH_PROPERTYPUT\n");
            return E_FAIL;
        }

        FIXME("call put\n");
        return E_NOTIMPL;
    default:
        FIXME("unsupported flags %x\n", flags);
        return E_NOTIMPL;
    }

    argn = arg_cnt(dp);

    if(argn < prop->min_args || argn > (prop->max_args ? prop->max_args : prop->min_args)) {
        FIXME("invalid number of arguments\n");
        return E_FAIL;
    }

    assert(argn < sizeof(args)/sizeof(*args));

    for(i=0; i < argn; i++) {
        if(V_VT(dp->rgvarg+dp->cArgs-i-1) == (VT_BYREF|VT_VARIANT))
            args[i] = *V_VARIANTREF(dp->rgvarg+dp->cArgs-i-1);
        else
            args[i] = dp->rgvarg[dp->cArgs-i-1];
    }

    return prop->proc(This, args, dp->cArgs, res);
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
    exec_script(This->desc->ctx, This->desc->funcs[This->desc->class_terminate_id].entries[VBDISP_CALLGET],
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

    if(id < 0)
        return DISP_E_MEMBERNOTFOUND;

    if(is_func_id(This, id)) {
        function_t *func;

        switch(wFlags) {
        case DISPATCH_PROPERTYGET:
            func = This->desc->funcs[id].entries[VBDISP_CALLGET];
            if(!func || (func->type != FUNC_PROPGET && func->type != FUNC_DEFGET)) {
                WARN("no getter\n");
                return DISP_E_MEMBERNOTFOUND;
            }

            return exec_script(This->desc->ctx, func, This, pdp, pvarRes);

        case DISPATCH_METHOD:
        case DISPATCH_METHOD|DISPATCH_PROPERTYGET:
            func = This->desc->funcs[id].entries[VBDISP_CALLGET];
            if(!func) {
                FIXME("no invoke/getter\n");
                return DISP_E_MEMBERNOTFOUND;
            }

            return exec_script(This->desc->ctx, func, This, pdp, pvarRes);
        case DISPATCH_PROPERTYPUT:
        case DISPATCH_PROPERTYPUTREF:
        case DISPATCH_PROPERTYPUT|DISPATCH_PROPERTYPUTREF: {
            DISPPARAMS dp = {NULL, NULL, 1, 0};
            BOOL needs_release;
            VARIANT put_val;
            HRESULT hres;

            if(arg_cnt(pdp)) {
                FIXME("arguments not implemented\n");
                return E_NOTIMPL;
            }

            hres = get_propput_arg(This->desc->ctx, pdp, wFlags, &put_val, &needs_release);
            if(FAILED(hres))
                return hres;

            dp.rgvarg = &put_val;
            func = This->desc->funcs[id].entries[V_VT(&put_val) == VT_DISPATCH ? VBDISP_SET : VBDISP_LET];
            if(!func) {
                FIXME("no letter/setter\n");
                return DISP_E_MEMBERNOTFOUND;
            }

            hres = exec_script(This->desc->ctx, func, This, &dp, NULL);
            if(needs_release)
                VariantClear(&put_val);
            return hres;
        }
        default:
            FIXME("flags %x\n", wFlags);
            return DISP_E_MEMBERNOTFOUND;
        }
    }

    if(id < This->desc->prop_cnt + This->desc->func_cnt)
        return invoke_variant_prop(This->desc->ctx, This->props+(id-This->desc->func_cnt), wFlags, pdp, pvarRes);

    if(This->desc->builtin_prop_cnt) {
        unsigned min = 0, max = This->desc->builtin_prop_cnt-1, i;

        while(min <= max) {
            i = (min+max)/2;
            if(This->desc->builtin_props[i].id == id)
                return invoke_builtin(This, This->desc->builtin_props+i, wFlags, pdp, pvarRes);
            if(This->desc->builtin_props[i].id < id)
                min = i+1;
            else
                max = i-1;
        }
    }

    return DISP_E_MEMBERNOTFOUND;
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
        ? CONTAINING_RECORD(iface, vbdisp_t, IDispatchEx_iface)
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
        hres = exec_script(desc->ctx, desc->funcs[desc->class_initialize_id].entries[VBDISP_CALLGET],
                           vbdisp, &dp, NULL);
    }

    if(FAILED(hres)) {
        IDispatchEx_Release(&vbdisp->IDispatchEx_iface);
        return hres;
    }

    *ret = vbdisp;
    return S_OK;
}

static HRESULT Procedure_invoke(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    script_ctx_t *ctx = This->desc->ctx;
    HRESULT hres;

    TRACE("\n");

    IActiveScriptSite_OnEnterScript(ctx->site);
    hres = exec_script(ctx, This->desc->value_func, NULL, NULL, NULL);
    IActiveScriptSite_OnLeaveScript(ctx->site);

    return hres;
}

static const builtin_prop_t procedure_props[] = {
    {DISPID_VALUE,  Procedure_invoke, 0}
};

HRESULT create_procedure_disp(script_ctx_t *ctx, vbscode_t *code, IDispatch **ret)
{
    class_desc_t *desc;
    vbdisp_t *vbdisp;
    HRESULT hres;

    desc = heap_alloc_zero(sizeof(*desc));
    if(!desc)
        return E_OUTOFMEMORY;

    desc->ctx = ctx;
    desc->builtin_prop_cnt = sizeof(procedure_props)/sizeof(*procedure_props);
    desc->builtin_props = procedure_props;
    desc->value_func = &code->main_code;

    hres = create_vbdisp(desc, &vbdisp);
    if(FAILED(hres)) {
        heap_free(desc);
        return hres;
    }

    desc->next = ctx->procs;
    ctx->procs = desc;

    *ret = (IDispatch*)&vbdisp->IDispatchEx_iface;
    return S_OK;
}

struct _ident_map_t {
    const WCHAR *name;
    BOOL is_var;
    union {
        dynamic_var_t *var;
        function_t *func;
    } u;
};

static inline DISPID ident_to_id(ScriptDisp *This, ident_map_t *ident)
{
    return (ident-This->ident_map)+1;
}

static inline ident_map_t *id_to_ident(ScriptDisp *This, DISPID id)
{
    return 0 < id && id <= This->ident_map_cnt ? This->ident_map+id-1 : NULL;
}

static ident_map_t *add_ident(ScriptDisp *This, const WCHAR *name)
{
    ident_map_t *ret;

    if(!This->ident_map_size) {
        This->ident_map = heap_alloc(4 * sizeof(*This->ident_map));
        if(!This->ident_map)
            return NULL;
        This->ident_map_size = 4;
    }else if(This->ident_map_cnt == This->ident_map_size) {
        ident_map_t *new_map;

        new_map = heap_realloc(This->ident_map, 2*This->ident_map_size*sizeof(*new_map));
        if(!new_map)
            return NULL;
        This->ident_map = new_map;
        This->ident_map_size *= 2;
    }

    ret = This->ident_map + This->ident_map_cnt++;
    ret->name = name;
    return ret;
}

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

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        assert(!This->ctx);
        heap_free(This->ident_map);
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

static HRESULT WINAPI ScriptDisp_GetTypeInfo(IDispatchEx *iface, UINT iTInfo, LCID lcid,
                                              ITypeInfo **ppTInfo)
{
    ScriptDisp *This = ScriptDisp_from_IDispatchEx(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
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
    dynamic_var_t *var;
    ident_map_t *ident;
    function_t *func;

    TRACE("(%p)->(%s %x %p)\n", This, debugstr_w(bstrName), grfdex, pid);

    if(!This->ctx)
        return E_UNEXPECTED;

    for(ident = This->ident_map; ident < This->ident_map+This->ident_map_cnt; ident++) {
        if(!strcmpiW(ident->name, bstrName)) {
            *pid = ident_to_id(This, ident);
            return S_OK;
        }
    }

    for(var = This->ctx->global_vars; var; var = var->next) {
        if(!strcmpiW(var->name, bstrName)) {
            ident = add_ident(This, var->name);
            if(!ident)
                return E_OUTOFMEMORY;

            ident->is_var = TRUE;
            ident->u.var = var;
            *pid = ident_to_id(This, ident);
            return S_OK;
        }
    }

    for(func = This->ctx->global_funcs; func; func = func->next) {
        if(!strcmpiW(func->name, bstrName)) {
            ident = add_ident(This, func->name);
            if(!ident)
                return E_OUTOFMEMORY;

            ident->is_var = FALSE;
            ident->u.func = func;
            *pid =  ident_to_id(This, ident);
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
    ident_map_t *ident;
    HRESULT hres;

    TRACE("(%p)->(%x %x %x %p %p %p %p)\n", This, id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);

    ident = id_to_ident(This, id);
    if(!ident)
        return DISP_E_MEMBERNOTFOUND;

    if(ident->is_var) {
        if(ident->u.var->is_const) {
            FIXME("const not supported\n");
            return E_NOTIMPL;
        }

        return invoke_variant_prop(This->ctx, &ident->u.var->v, wFlags, pdp, pvarRes);
    }

    switch(wFlags) {
    case DISPATCH_METHOD:
    case DISPATCH_METHOD|DISPATCH_PROPERTYGET:
        IActiveScriptSite_OnEnterScript(This->ctx->site);
        hres = exec_script(This->ctx, ident->u.func, NULL, pdp, pvarRes);
        IActiveScriptSite_OnLeaveScript(This->ctx->site);
        break;
    default:
        FIXME("Unsupported flags %x\n", wFlags);
        hres = E_NOTIMPL;
    }

    return hres;
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
    EXCEPINFO ei;
    HRESULT hres;

    memset(&ei, 0, sizeof(ei));
    if(retv)
        V_VT(retv) = VT_EMPTY;

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    if(FAILED(hres)) {
        UINT err = 0;

        TRACE("using IDispatch\n");
        return IDispatch_Invoke(disp, id, &IID_NULL, ctx->lcid, flags, dp, retv, &ei, &err);
    }

    hres = IDispatchEx_InvokeEx(dispex, id, ctx->lcid, flags, dp, retv, &ei, NULL /* CALLER_FIXME */);
    IDispatchEx_Release(dispex);
    return hres;
}

HRESULT get_disp_value(script_ctx_t *ctx, IDispatch *disp, VARIANT *v)
{
    DISPPARAMS dp = {NULL};
    return disp_call(ctx, disp, DISPID_VALUE, &dp, v);
}

HRESULT disp_propput(script_ctx_t *ctx, IDispatch *disp, DISPID id, WORD flags, DISPPARAMS *dp)
{
    IDispatchEx *dispex;
    EXCEPINFO ei = {0};
    HRESULT hres;

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    if(SUCCEEDED(hres)) {
        hres = IDispatchEx_InvokeEx(dispex, id, ctx->lcid, flags, dp, NULL, &ei, NULL /* FIXME! */);
        IDispatchEx_Release(dispex);
    }else {
        ULONG err = 0;

        TRACE("using IDispatch\n");
        hres = IDispatch_Invoke(disp, id, &IID_NULL, ctx->lcid, flags, dp, NULL, &ei, &err);
    }

    return hres;
}
