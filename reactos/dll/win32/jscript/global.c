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
#include "engine.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

static const WCHAR NaNW[] = {'N','a','N',0};
static const WCHAR InfinityW[] = {'I','n','f','i','n','i','t','y',0};
static const WCHAR ArrayW[] = {'A','r','r','a','y',0};
static const WCHAR BooleanW[] = {'B','o','o','l','e','a','n',0};
static const WCHAR DateW[] = {'D','a','t','e',0};
static const WCHAR FunctionW[] = {'F','u','n','c','t','i','o','n',0};
static const WCHAR NumberW[] = {'N','u','m','b','e','r',0};
static const WCHAR ObjectW[] = {'O','b','j','e','c','t',0};
static const WCHAR StringW[] = {'S','t','r','i','n','g',0};
static const WCHAR RegExpW[] = {'R','e','g','E','x','p',0};
static const WCHAR ActiveXObjectW[] = {'A','c','t','i','v','e','X','O','b','j','e','c','t',0};
static const WCHAR VBArrayW[] = {'V','B','A','r','r','a','y',0};
static const WCHAR EnumeratorW[] = {'E','n','u','m','e','r','a','t','o','r',0};
static const WCHAR escapeW[] = {'e','s','c','a','p','e',0};
static const WCHAR evalW[] = {'e','v','a','l',0};
static const WCHAR isNaNW[] = {'i','s','N','a','N',0};
static const WCHAR isFiniteW[] = {'i','s','F','i','n','i','t','e',0};
static const WCHAR parseIntW[] = {'p','a','r','s','e','I','n','t',0};
static const WCHAR parseFloatW[] = {'p','a','r','s','e','F','l','o','a','t',0};
static const WCHAR unescapeW[] = {'u','n','e','s','c','a','p','e',0};
static const WCHAR GetObjectW[] = {'G','e','t','O','b','j','e','c','t',0};
static const WCHAR ScriptEngineW[] = {'S','c','r','i','p','t','E','n','g','i','n','e',0};
static const WCHAR ScriptEngineMajorVersionW[] =
    {'S','c','r','i','p','t','E','n','g','i','n','e','M','a','j','o','r','V','e','r','s','i','o','n',0};
static const WCHAR ScriptEngineMinorVersionW[] =
    {'S','c','r','i','p','t','E','n','g','i','n','e','M','i','n','o','r','V','e','r','s','i','o','n',0};
static const WCHAR ScriptEngineBuildVersionW[] =
    {'S','c','r','i','p','t','E','n','g','i','n','e','B','u','i','l','d','V','e','r','s','i','o','n',0};
static const WCHAR CollectGarbageW[] = {'C','o','l','l','e','c','t','G','a','r','b','a','g','e',0};
static const WCHAR MathW[] = {'M','a','t','h',0};

static HRESULT constructor_call(DispatchEx *constr, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    if(flags != DISPATCH_PROPERTYGET)
        return jsdisp_call_value(constr, lcid, flags, dp, retv, ei, sp);

    V_VT(retv) = VT_DISPATCH;
    V_DISPATCH(retv) = (IDispatch*)_IDispatchEx_(constr);
    IDispatchEx_AddRef(_IDispatchEx_(constr));
    return S_OK;
}

static HRESULT JSGlobal_NaN(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_Infinity(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_Array(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->array_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_Boolean(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->bool_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_Date(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_Function(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->function_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_Number(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->number_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_Object(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->object_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_String(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->string_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_RegExp(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    TRACE("\n");

    return constructor_call(dispex->ctx->regexp_constr, lcid, flags, dp, retv, ei, sp);
}

static HRESULT JSGlobal_ActiveXObject(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_VBArray(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_Enumerator(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_escape(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/* ECMA-262 3rd Edition    15.1.2.1 */
static HRESULT JSGlobal_eval(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    parser_ctx_t *parser_ctx;
    VARIANT *arg;
    HRESULT hres;

    TRACE("\n");

    if(!arg_cnt(dp)) {
        if(retv)
            V_VT(retv) = VT_EMPTY;
        return S_OK;
    }

    arg = get_arg(dp, 0);
    if(V_VT(arg) != VT_BSTR) {
        if(retv) {
            V_VT(retv) = VT_EMPTY;
            return VariantCopy(retv, arg);
        }
        return S_OK;
    }

    if(!dispex->ctx->exec_ctx) {
        FIXME("No active exec_ctx\n");
        return E_UNEXPECTED;
    }

    TRACE("parsing %s\n", debugstr_w(V_BSTR(arg)));
    hres = script_parse(dispex->ctx, V_BSTR(arg), &parser_ctx);
    if(FAILED(hres)) {
        FIXME("parse failed: %08x\n", hres);
        return hres;
    }

    hres = exec_source(dispex->ctx->exec_ctx, parser_ctx, parser_ctx->source, ei, retv);
    parser_release(parser_ctx);

    return hres;
}

static HRESULT JSGlobal_isNaN(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_isFinite(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_parseInt(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    return E_NOTIMPL;
}

static HRESULT JSGlobal_parseFloat(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_unescape(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_GetObject(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_ScriptEngine(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_ScriptEngineMajorVersion(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_ScriptEngineMinorVersion(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_ScriptEngineBuildVersion(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT JSGlobal_CollectGarbage(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *retv, jsexcept_t *ei, IServiceProvider *sp)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const builtin_prop_t JSGlobal_props[] = {
    {ActiveXObjectW,             JSGlobal_ActiveXObject,             PROPF_METHOD},
    {ArrayW,                     JSGlobal_Array,                     PROPF_CONSTR},
    {BooleanW,                   JSGlobal_Boolean,                   PROPF_CONSTR},
    {CollectGarbageW,            JSGlobal_CollectGarbage,            PROPF_METHOD},
    {DateW,                      JSGlobal_Date,                      PROPF_CONSTR},
    {EnumeratorW,                JSGlobal_Enumerator,                PROPF_METHOD},
    {FunctionW,                  JSGlobal_Function,                  PROPF_CONSTR},
    {GetObjectW,                 JSGlobal_GetObject,                 PROPF_METHOD},
    {InfinityW,                  JSGlobal_Infinity,                  0},
/*  {MathW,                      JSGlobal_Math,                      0},  */
    {NaNW,                       JSGlobal_NaN,                       0},
    {NumberW,                    JSGlobal_Number,                    PROPF_CONSTR},
    {ObjectW,                    JSGlobal_Object,                    PROPF_CONSTR},
    {RegExpW,                    JSGlobal_RegExp,                    PROPF_CONSTR},
    {ScriptEngineW,              JSGlobal_ScriptEngine,              PROPF_METHOD},
    {ScriptEngineBuildVersionW,  JSGlobal_ScriptEngineBuildVersion,  PROPF_METHOD},
    {ScriptEngineMajorVersionW,  JSGlobal_ScriptEngineMajorVersion,  PROPF_METHOD},
    {ScriptEngineMinorVersionW,  JSGlobal_ScriptEngineMinorVersion,  PROPF_METHOD},
    {StringW,                    JSGlobal_String,                    PROPF_CONSTR},
    {VBArrayW,                   JSGlobal_VBArray,                   PROPF_METHOD},
    {escapeW,                    JSGlobal_escape,                    PROPF_METHOD},
    {evalW,                      JSGlobal_eval,                      PROPF_METHOD|1},
    {isFiniteW,                  JSGlobal_isFinite,                  PROPF_METHOD},
    {isNaNW,                     JSGlobal_isNaN,                     PROPF_METHOD},
    {parseFloatW,                JSGlobal_parseFloat,                PROPF_METHOD},
    {parseIntW,                  JSGlobal_parseInt,                  PROPF_METHOD|2},
    {unescapeW,                  JSGlobal_unescape,                  PROPF_METHOD}
};

static const builtin_info_t JSGlobal_info = {
    JSCLASS_GLOBAL,
    {NULL, NULL, 0},
    sizeof(JSGlobal_props)/sizeof(*JSGlobal_props),
    JSGlobal_props,
    NULL,
    NULL
};

static HRESULT init_constructors(script_ctx_t *ctx)
{
    HRESULT hres;

    hres = init_function_constr(ctx);
    if(FAILED(hres))
        return hres;

    hres = create_array_constr(ctx, &ctx->array_constr);
    if(FAILED(hres))
        return hres;

    hres = create_bool_constr(ctx, &ctx->bool_constr);
    if(FAILED(hres))
        return hres;

    hres = create_number_constr(ctx, &ctx->number_constr);
    if(FAILED(hres))
        return hres;

    hres = create_object_constr(ctx, &ctx->object_constr);
    if(FAILED(hres))
        return hres;

    hres = create_object_constr(ctx, &ctx->regexp_constr);
    if(FAILED(hres))
        return hres;

    hres = create_string_constr(ctx, &ctx->string_constr);
    if(FAILED(hres))
        return hres;

    return S_OK;
}

HRESULT init_global(script_ctx_t *ctx)
{
    DispatchEx *math;
    VARIANT var;
    HRESULT hres;

    if(ctx->global)
        return S_OK;

    hres = init_constructors(ctx);
    if(FAILED(hres))
        return hres;

    hres = create_dispex(ctx, &JSGlobal_info, NULL, &ctx->global);
    if(FAILED(hres))
        return hres;

    hres = create_math(ctx, &math);
    if(FAILED(hres))
        return hres;

    V_VT(&var) = VT_DISPATCH;
    V_DISPATCH(&var) = (IDispatch*)_IDispatchEx_(math);
    hres = jsdisp_propput_name(ctx->global, MathW, ctx->lcid, &var, NULL/*FIXME*/, NULL/*FIXME*/);
    jsdisp_release(math);

    return hres;
}
