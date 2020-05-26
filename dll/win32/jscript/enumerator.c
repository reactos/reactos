/*
 * Copyright 2019 Andreas Maier
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

#include "jscript.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef struct {
    jsdisp_t dispex;
    /* IEnumVARIANT returned by _NewEnum */
    IEnumVARIANT *enumvar;
    /* current item */
    jsval_t item;
    BOOL atend;
} EnumeratorInstance;

static const WCHAR atEndW[] = {'a','t','E','n','d',0};
static const WCHAR itemW[] = {'i','t','e','m',0};
static const WCHAR moveFirstW[] = {'m','o','v','e','F','i','r','s','t',0};
static const WCHAR moveNextW[] = {'m','o','v','e','N','e','x','t',0};

static inline EnumeratorInstance *enumerator_from_jsdisp(jsdisp_t *jsdisp)
{
    return CONTAINING_RECORD(jsdisp, EnumeratorInstance, dispex);
}

static inline EnumeratorInstance *enumerator_from_vdisp(vdisp_t *vdisp)
{
    return enumerator_from_jsdisp(vdisp->u.jsdisp);
}

static inline EnumeratorInstance *enumerator_this(vdisp_t *jsthis)
{
    return is_vclass(jsthis, JSCLASS_ENUMERATOR) ? enumerator_from_vdisp(jsthis) : NULL;
}

static inline HRESULT enumvar_get_next_item(EnumeratorInstance *This)
{
    HRESULT hres;
    VARIANT nextitem;

    if (This->atend)
        return S_OK;

    /* don't leak previous value */
    jsval_release(This->item);

    /* not at end ... get next item */
    VariantInit(&nextitem);
    hres = IEnumVARIANT_Next(This->enumvar, 1, &nextitem, NULL);
    if (hres == S_OK)
    {
        hres = variant_to_jsval(&nextitem, &This->item);
        VariantClear(&nextitem);
        if (FAILED(hres))
        {
            WARN("failed to convert jsval to variant!\n");
            This->item = jsval_undefined();
            return hres;
        }
    }
    else
    {
        This->item = jsval_undefined();
        This->atend = TRUE;
    }

    return S_OK;
}

static void Enumerator_destructor(jsdisp_t *dispex)
{
    EnumeratorInstance *This = enumerator_from_jsdisp(dispex);

    TRACE("\n");

    jsval_release(This->item);
    heap_free(dispex);
}

static HRESULT Enumerator_atEnd(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    EnumeratorInstance *This;

    if (!(This = enumerator_this(jsthis)))
        return throw_type_error(ctx, JS_E_ENUMERATOR_EXPECTED, NULL);

    TRACE("%d\n", This->atend);

    if (r)
        *r = jsval_bool(This->atend);
    return S_OK;
}

static HRESULT Enumerator_item(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    EnumeratorInstance *This;

    TRACE("\n");

    if (!(This = enumerator_this(jsthis)))
        return throw_type_error(ctx, JS_E_ENUMERATOR_EXPECTED, NULL);

    return r ? jsval_copy(This->item, r) : S_OK;
}

static HRESULT Enumerator_moveFirst(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    EnumeratorInstance *This;
    HRESULT hres = S_OK;

    TRACE("\n");

    if (!(This = enumerator_this(jsthis)))
        return throw_type_error(ctx, JS_E_ENUMERATOR_EXPECTED, NULL);

    if (This->enumvar)
    {
        hres = IEnumVARIANT_Reset(This->enumvar);
        if (FAILED(hres))
            return hres;

        This->atend = FALSE;
        hres = enumvar_get_next_item(This);
        if(FAILED(hres))
            return hres;
    }

    if (r)
        *r = jsval_undefined();
    return S_OK;
}

static HRESULT Enumerator_moveNext(script_ctx_t *ctx, vdisp_t *jsthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    EnumeratorInstance *This;
    HRESULT hres = S_OK;

    TRACE("\n");

    if (!(This = enumerator_this(jsthis)))
        return throw_type_error(ctx, JS_E_ENUMERATOR_EXPECTED, NULL);

    if (This->enumvar)
    {
        hres = enumvar_get_next_item(This);
        if (FAILED(hres))
            return hres;
    }

    if (r)
        *r = jsval_undefined();
    return S_OK;
}

static const builtin_prop_t Enumerator_props[] = {
    {atEndW,     Enumerator_atEnd,     PROPF_METHOD},
    {itemW,      Enumerator_item,      PROPF_METHOD},
    {moveFirstW, Enumerator_moveFirst, PROPF_METHOD},
    {moveNextW,  Enumerator_moveNext,  PROPF_METHOD},
};

static const builtin_info_t Enumerator_info = {
    JSCLASS_ENUMERATOR,
    {NULL, NULL, 0},
    ARRAY_SIZE(Enumerator_props),
    Enumerator_props,
    NULL,
    NULL
};

static const builtin_info_t EnumeratorInst_info = {
    JSCLASS_ENUMERATOR,
    {NULL, NULL, 0, NULL},
    0,
    NULL,
    Enumerator_destructor,
    NULL
};

static HRESULT alloc_enumerator(script_ctx_t *ctx, jsdisp_t *object_prototype, EnumeratorInstance **ret)
{
    EnumeratorInstance *enumerator;
    HRESULT hres;

    enumerator = heap_alloc_zero(sizeof(EnumeratorInstance));
    if(!enumerator)
        return E_OUTOFMEMORY;

    if(object_prototype)
        hres = init_dispex(&enumerator->dispex, ctx, &Enumerator_info, object_prototype);
    else
        hres = init_dispex_from_constr(&enumerator->dispex, ctx, &EnumeratorInst_info,
                                       ctx->enumerator_constr);

    if(FAILED(hres))
    {
        heap_free(enumerator);
        return hres;
    }

    *ret = enumerator;
    return S_OK;
}

static HRESULT create_enumerator(script_ctx_t *ctx, jsval_t *argv, jsdisp_t **ret)
{
    EnumeratorInstance *enumerator;
    HRESULT hres;
    IDispatch *obj;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    IEnumVARIANT *enumvar = NULL;

    if (argv)
    {
        VARIANT varresult;

        if (!is_object_instance(*argv))
        {
            FIXME("I don't know how to handle this type!\n");
            return E_NOTIMPL;
        }

        obj = get_object(*argv);

        /* Try to get a IEnumVARIANT by _NewEnum */
        VariantInit(&varresult);
        hres = IDispatch_Invoke(obj, DISPID_NEWENUM, &IID_NULL, LOCALE_NEUTRAL,
                DISPATCH_METHOD, &dispparams, &varresult, NULL, NULL);
        if (FAILED(hres))
        {
            WARN("Enumerator: no DISPID_NEWENUM.\n");
            return E_INVALIDARG;
        }

        if ((V_VT(&varresult) == VT_DISPATCH) || (V_VT(&varresult) == VT_UNKNOWN))
        {
            hres = IUnknown_QueryInterface(V_UNKNOWN(&varresult),
                &IID_IEnumVARIANT, (void**)&enumvar);
        }
        else
        {
            FIXME("Enumerator: NewEnum unexpected type of varresult (%d).\n", V_VT(&varresult));
            hres = E_INVALIDARG;
        }
        VariantClear(&varresult);
        if (FAILED(hres))
            return hres;
    }

    hres = alloc_enumerator(ctx, NULL, &enumerator);
    if (FAILED(hres))
    {
        if (enumvar)
            IEnumVARIANT_Release(enumvar);
        return hres;
    }

    enumerator->enumvar = enumvar;
    enumerator->atend = !enumvar;
    hres = enumvar_get_next_item(enumerator);
    if (FAILED(hres))
    {
        jsdisp_release(&enumerator->dispex);
        return hres;
    }

    *ret = &enumerator->dispex;
    return S_OK;
}

static HRESULT EnumeratorConstr_value(script_ctx_t *ctx, vdisp_t *vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsdisp_t *obj;
    HRESULT hres;

    TRACE("\n");

    switch(flags) {
    case DISPATCH_CONSTRUCT: {
        if (argc > 1)
            return throw_syntax_error(ctx, JS_E_INVALIDARG, NULL);

        hres = create_enumerator(ctx, (argc == 1) ? &argv[0] : 0, &obj);
        if(FAILED(hres))
            return hres;

        *r = jsval_obj(obj);
        break;
    }
    default:
        FIXME("unimplemented flags: %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static const builtin_info_t EnumeratorConstr_info = {
    JSCLASS_FUNCTION,
    DEFAULT_FUNCTION_VALUE,
    0,
    NULL,
    NULL,
    NULL
};

HRESULT create_enumerator_constr(script_ctx_t *ctx, jsdisp_t *object_prototype, jsdisp_t **ret)
{
    EnumeratorInstance *enumerator;
    HRESULT hres;
    static const WCHAR EnumeratorW[] = {'E','n','u','m','e','r','a','t','o','r',0};

    hres = alloc_enumerator(ctx, object_prototype, &enumerator);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_constructor(ctx, EnumeratorConstr_value,
                                     EnumeratorW, &EnumeratorConstr_info,
                                     PROPF_CONSTR|7, &enumerator->dispex, ret);
    jsdisp_release(&enumerator->dispex);

    return hres;
}
