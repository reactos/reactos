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
#include "objsafe.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vbscript);

#ifdef _WIN64

#define CTXARG_T DWORDLONG
#define IActiveScriptDebugVtbl IActiveScriptDebug64Vtbl
#define IActiveScriptParseVtbl IActiveScriptParse64Vtbl
#define IActiveScriptParseProcedure2Vtbl IActiveScriptParseProcedure2_64Vtbl

#else

#define CTXARG_T DWORD
#define IActiveScriptDebugVtbl IActiveScriptDebug32Vtbl
#define IActiveScriptParseVtbl IActiveScriptParse32Vtbl
#define IActiveScriptParseProcedure2Vtbl IActiveScriptParseProcedure2_32Vtbl

#endif

struct VBScript {
    IActiveScript IActiveScript_iface;
    IActiveScriptDebug IActiveScriptDebug_iface;
    IActiveScriptParse IActiveScriptParse_iface;
    IActiveScriptParseProcedure2 IActiveScriptParseProcedure2_iface;
    IObjectSafety IObjectSafety_iface;

    LONG ref;

    SCRIPTSTATE state;
    script_ctx_t *ctx;
    LONG thread_id;
    BOOL is_initialized;
};

typedef struct {
    IActiveScriptError IActiveScriptError_iface;
    LONG ref;
    EXCEPINFO ei;
    DWORD_PTR cookie;
    unsigned line;
    unsigned character;
} VBScriptError;

static inline WCHAR *heap_pool_strdup(heap_pool_t *heap, const WCHAR *str)
{
    size_t size = (lstrlenW(str) + 1) * sizeof(WCHAR);
    WCHAR *ret = heap_pool_alloc(heap, size);
    if (ret) memcpy(ret, str, size);
    return ret;
}

static void change_state(VBScript *This, SCRIPTSTATE state)
{
    if(This->state == state)
        return;

    This->state = state;
    if(This->ctx->site)
        IActiveScriptSite_OnStateChange(This->ctx->site, state);
}

static inline BOOL is_started(VBScript *This)
{
    return This->state == SCRIPTSTATE_STARTED
        || This->state == SCRIPTSTATE_CONNECTED
        || This->state == SCRIPTSTATE_DISCONNECTED;
}

static HRESULT exec_global_code(script_ctx_t *ctx, vbscode_t *code, VARIANT *res)
{
    ScriptDisp *obj = ctx->script_obj;
    function_t *func_iter, **new_funcs;
    dynamic_var_t *var, **new_vars;
    IServiceProvider *prev_caller;
    size_t cnt, i;
    HRESULT hres;

    if(code->named_item) {
        if(!code->named_item->script_obj) {
            hres = create_script_disp(ctx, &code->named_item->script_obj);
            if(FAILED(hres)) return hres;
        }
        obj = code->named_item->script_obj;
    }

    cnt = obj->global_vars_cnt + code->main_code.var_cnt;
    if (cnt > obj->global_vars_size)
    {
        if (obj->global_vars)
            new_vars = realloc(obj->global_vars, cnt * sizeof(*new_vars));
        else
            new_vars = malloc(cnt * sizeof(*new_vars));
        if (!new_vars)
            return E_OUTOFMEMORY;
        obj->global_vars = new_vars;
        obj->global_vars_size = cnt;
    }

    cnt = obj->global_funcs_cnt;
    for (func_iter = code->funcs; func_iter; func_iter = func_iter->next)
        cnt++;
    if (cnt > obj->global_funcs_size)
    {
        if (obj->global_funcs)
            new_funcs = realloc(obj->global_funcs, cnt * sizeof(*new_funcs));
        else
            new_funcs = malloc(cnt * sizeof(*new_funcs));
        if (!new_funcs)
            return E_OUTOFMEMORY;
        obj->global_funcs = new_funcs;
        obj->global_funcs_size = cnt;
    }

    for (i = 0; i < code->main_code.var_cnt; i++)
    {
        if (!(var = heap_pool_alloc(&obj->heap, sizeof(*var))))
            return E_OUTOFMEMORY;

        var->name = heap_pool_strdup(&obj->heap, code->main_code.vars[i].name);
        if (!var->name)
            return E_OUTOFMEMORY;
        V_VT(&var->v) = VT_EMPTY;
        var->is_const = FALSE;
        var->array = NULL;

        obj->global_vars[obj->global_vars_cnt + i] = var;
    }

    obj->global_vars_cnt += code->main_code.var_cnt;

    for (func_iter = code->funcs; func_iter; func_iter = func_iter->next)
    {
        for (i = 0; i < obj->global_funcs_cnt; i++)
        {
            if (!wcsicmp(obj->global_funcs[i]->name, func_iter->name))
            {
                /* global function already exists, replace it */
                obj->global_funcs[i] = func_iter;
                break;
            }
        }
        if (i == obj->global_funcs_cnt)
            obj->global_funcs[obj->global_funcs_cnt++] = func_iter;
    }

    if (code->classes)
    {
        class_desc_t *class = code->classes;

        while (1)
        {
            class->ctx = ctx;
            if (!class->next)
                break;
            class = class->next;
        }

        class->next = obj->classes;
        obj->classes = code->classes;
        code->last_class = class;
    }

    code->pending_exec = FALSE;

    prev_caller = ctx->vbcaller->caller;
    ctx->vbcaller->caller = SP_CALLER_UNINITIALIZED;
    hres = exec_script(ctx, TRUE, &code->main_code, NULL, NULL, res);
    ctx->vbcaller->caller = prev_caller;
    return hres;
}

static void exec_queued_code(script_ctx_t *ctx)
{
    vbscode_t *iter;

    LIST_FOR_EACH_ENTRY(iter, &ctx->code_list, vbscode_t, entry) {
        if(iter->pending_exec)
            exec_global_code(ctx, iter, NULL);
    }
}

static HRESULT retrieve_named_item_disp(IActiveScriptSite *site, named_item_t *item)
{
    IUnknown *unk;
    HRESULT hres;

    hres = IActiveScriptSite_GetItemInfo(site, item->name, SCRIPTINFO_IUNKNOWN, &unk, NULL);
    if(FAILED(hres)) {
        WARN("GetItemInfo failed: %08lx\n", hres);
        return hres;
    }

    hres = IUnknown_QueryInterface(unk, &IID_IDispatch, (void**)&item->disp);
    IUnknown_Release(unk);
    if(FAILED(hres)) {
        WARN("object does not implement IDispatch\n");
        return hres;
    }

    return S_OK;
}

named_item_t *lookup_named_item(script_ctx_t *ctx, const WCHAR *name, unsigned flags)
{
    named_item_t *item;
    HRESULT hres;

    LIST_FOR_EACH_ENTRY(item, &ctx->named_items, named_item_t, entry) {
        if((item->flags & flags) == flags && !wcsicmp(item->name, name)) {
            if(!item->script_obj && !(item->flags & SCRIPTITEM_GLOBALMEMBERS)) {
                hres = create_script_disp(ctx, &item->script_obj);
                if(FAILED(hres)) return NULL;
            }

            if(!item->disp && (flags || !(item->flags & SCRIPTITEM_CODEONLY))) {
                hres = retrieve_named_item_disp(ctx->site, item);
                if(FAILED(hres)) continue;
            }

            return item;
        }
    }

    return NULL;
}

static void release_named_item_script_obj(named_item_t *item)
{
    if(!item->script_obj) return;

    item->script_obj->ctx = NULL;
    IDispatchEx_Release(&item->script_obj->IDispatchEx_iface);
    item->script_obj = NULL;
}

void release_named_item(named_item_t *item)
{
    if(--item->ref) return;

    free(item->name);
    free(item);
}

static void release_script(script_ctx_t *ctx)
{
    named_item_t *item, *item_next;
    vbscode_t *code, *code_next;

    collect_objects(ctx);
    clear_ei(&ctx->ei);

    LIST_FOR_EACH_ENTRY_SAFE(code, code_next, &ctx->code_list, vbscode_t, entry)
    {
        if(code->is_persistent)
        {
            code->pending_exec = TRUE;
            if(code->last_class) code->last_class->next = NULL;
            if(code->named_item) release_named_item_script_obj(code->named_item);
        }
        else
        {
            list_remove(&code->entry);
            release_vbscode(code);
        }
    }

    LIST_FOR_EACH_ENTRY_SAFE(item, item_next, &ctx->named_items, named_item_t, entry)
    {
        if(item->disp)
        {
            IDispatch_Release(item->disp);
            item->disp = NULL;
        }
        release_named_item_script_obj(item);
        if(!(item->flags & SCRIPTITEM_ISPERSISTENT))
        {
            list_remove(&item->entry);
            release_named_item(item);
        }
    }

    if(ctx->secmgr) {
        IInternetHostSecurityManager_Release(ctx->secmgr);
        ctx->secmgr = NULL;
    }

    if(ctx->site) {
        IActiveScriptSite_Release(ctx->site);
        ctx->site = NULL;
    }

    if(ctx->script_obj) {
        ScriptDisp *script_obj = ctx->script_obj;

        ctx->script_obj = NULL;
        script_obj->ctx = NULL;
        IDispatchEx_Release(&script_obj->IDispatchEx_iface);
    }
}

static void release_code_list(script_ctx_t *ctx)
{
    while(!list_empty(&ctx->code_list)) {
        vbscode_t *iter = LIST_ENTRY(list_head(&ctx->code_list), vbscode_t, entry);

        list_remove(&iter->entry);
        release_vbscode(iter);
    }
}

static void release_named_item_list(script_ctx_t *ctx)
{
    while(!list_empty(&ctx->named_items)) {
        named_item_t *iter = LIST_ENTRY(list_head(&ctx->named_items), named_item_t, entry);
        list_remove(&iter->entry);
        release_named_item(iter);
    }
}

static void decrease_state(VBScript *This, SCRIPTSTATE state)
{
    switch(This->state) {
    case SCRIPTSTATE_CONNECTED:
        change_state(This, SCRIPTSTATE_DISCONNECTED);
        if(state == SCRIPTSTATE_DISCONNECTED)
            return;
        /* FALLTHROUGH */
    case SCRIPTSTATE_STARTED:
    case SCRIPTSTATE_DISCONNECTED:
        change_state(This, SCRIPTSTATE_INITIALIZED);
        /* FALLTHROUGH */
    case SCRIPTSTATE_INITIALIZED:
    case SCRIPTSTATE_UNINITIALIZED:
        change_state(This, state);
        if(state == SCRIPTSTATE_INITIALIZED)
            break;
        release_script(This->ctx);
        This->thread_id = 0;
        if(state == SCRIPTSTATE_CLOSED) {
            release_code_list(This->ctx);
            release_named_item_list(This->ctx);
        }
        break;
    case SCRIPTSTATE_CLOSED:
        break;
    DEFAULT_UNREACHABLE;
    }
}

static inline struct vbcaller *vbcaller_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, struct vbcaller, IServiceProvider_iface);
}

static HRESULT WINAPI vbcaller_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    struct vbcaller *This = vbcaller_from_IServiceProvider(iface);

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IServiceProvider, riid)) {
        *ppv = &This->IServiceProvider_iface;
    }else {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI vbcaller_AddRef(IServiceProvider *iface)
{
    struct vbcaller *This = vbcaller_from_IServiceProvider(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI vbcaller_Release(IServiceProvider *iface)
{
    struct vbcaller *This = vbcaller_from_IServiceProvider(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref)
        free(This);

    return ref;
}

static HRESULT WINAPI vbcaller_QueryService(IServiceProvider *iface, REFGUID guidService,
        REFIID riid, void **ppv)
{
    struct vbcaller *This = vbcaller_from_IServiceProvider(iface);

    if(IsEqualGUID(guidService, &IID_IActiveScriptSite)) {
        TRACE("(%p)->(IID_IActiveScriptSite)\n", This);
        if(This->ctx->site)
            return IActiveScriptSite_QueryInterface(This->ctx->site, riid, ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    if(IsEqualGUID(guidService, &SID_GetCaller)) {
        TRACE("(%p)->(SID_GetCaller)\n", This);
        *ppv = NULL;
        if(!This->caller)
            return S_OK;
        return (This->caller == SP_CALLER_UNINITIALIZED) ? E_NOINTERFACE : IServiceProvider_QueryInterface(This->caller, riid, ppv);
    }

    FIXME("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);

    *ppv = NULL;
    return E_NOINTERFACE;
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    vbcaller_QueryInterface,
    vbcaller_AddRef,
    vbcaller_Release,
    vbcaller_QueryService
};

static struct vbcaller *create_vbcaller(void)
{
    struct vbcaller *ret;

    ret = malloc(sizeof(*ret));
    if(ret) {
        ret->IServiceProvider_iface.lpVtbl = &ServiceProviderVtbl;
        ret->ref = 1;
        ret->caller = SP_CALLER_UNINITIALIZED;
    }
    return ret;
}

static inline VBScriptError *impl_from_IActiveScriptError(IActiveScriptError *iface)
{
    return CONTAINING_RECORD(iface, VBScriptError, IActiveScriptError_iface);
}

static HRESULT WINAPI VBScriptError_QueryInterface(IActiveScriptError *iface, REFIID riid, void **ppv)
{
    VBScriptError *This = impl_from_IActiveScriptError(iface);

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IActiveScriptError_iface;
    }else if(IsEqualGUID(riid, &IID_IActiveScriptError)) {
        TRACE("(%p)->(IID_IActiveScriptError %p)\n", This, ppv);
        *ppv = &This->IActiveScriptError_iface;
    }else {
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI VBScriptError_AddRef(IActiveScriptError *iface)
{
    VBScriptError *This = impl_from_IActiveScriptError(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI VBScriptError_Release(IActiveScriptError *iface)
{
    VBScriptError *This = impl_from_IActiveScriptError(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref)
        free(This);

    return ref;
}

static HRESULT WINAPI VBScriptError_GetExceptionInfo(IActiveScriptError *iface, EXCEPINFO *excepinfo)
{
    VBScriptError *This = impl_from_IActiveScriptError(iface);

    TRACE("(%p)->(%p)\n", This, excepinfo);

    *excepinfo = This->ei;
    excepinfo->bstrSource = SysAllocString(This->ei.bstrSource);
    excepinfo->bstrDescription = SysAllocString(This->ei.bstrDescription);
    excepinfo->bstrHelpFile = SysAllocString(This->ei.bstrHelpFile);
    return S_OK;
}

static HRESULT WINAPI VBScriptError_GetSourcePosition(IActiveScriptError *iface, DWORD *source_context, ULONG *line, LONG *character)
{
    VBScriptError *This = impl_from_IActiveScriptError(iface);

    TRACE("(%p)->(%p %p %p)\n", This, source_context, line, character);

    if(source_context)
        *source_context = This->cookie;
    if(line)
        *line = This->line;
    if(character)
        *character = This->character;
    return S_OK;
}

static HRESULT WINAPI VBScriptError_GetSourceLineText(IActiveScriptError *iface, BSTR *source)
{
    VBScriptError *This = impl_from_IActiveScriptError(iface);
    FIXME("(%p)->(%p)\n", This, source);
    return E_NOTIMPL;
}

static const IActiveScriptErrorVtbl VBScriptErrorVtbl = {
    VBScriptError_QueryInterface,
    VBScriptError_AddRef,
    VBScriptError_Release,
    VBScriptError_GetExceptionInfo,
    VBScriptError_GetSourcePosition,
    VBScriptError_GetSourceLineText
};

HRESULT report_script_error(script_ctx_t *ctx, const vbscode_t *code, unsigned loc)
{
    VBScriptError *error;
    const WCHAR *p, *nl;
    HRESULT hres, result;

    if(!(error = malloc(sizeof(*error))))
        return E_OUTOFMEMORY;
    error->IActiveScriptError_iface.lpVtbl = &VBScriptErrorVtbl;

    error->ref = 1;
    error->ei = ctx->ei;
    memset(&ctx->ei, 0, sizeof(ctx->ei));
    result = error->ei.scode;

    p = code->source;
    error->cookie = code->cookie;
    error->line = code->start_line;
    for(nl = p = code->source; p < code->source + loc; p++) {
        if(*p != '\n') continue;
        error->line++;
        nl = p + 1;
    }
    error->character = code->source + loc - nl;

    hres = IActiveScriptSite_OnScriptError(ctx->site, &error->IActiveScriptError_iface);
    IActiveScriptError_Release(&error->IActiveScriptError_iface);
    return hres == S_OK ? SCRIPT_E_REPORTED : result;
}

static inline VBScript *impl_from_IActiveScript(IActiveScript *iface)
{
    return CONTAINING_RECORD(iface, VBScript, IActiveScript_iface);
}

static HRESULT WINAPI VBScript_QueryInterface(IActiveScript *iface, REFIID riid, void **ppv)
{
    VBScript *This = impl_from_IActiveScript(iface);

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IActiveScript_iface;
    }else if(IsEqualGUID(riid, &IID_IActiveScript)) {
        TRACE("(%p)->(IID_IActiveScript %p)\n", This, ppv);
        *ppv = &This->IActiveScript_iface;
    }else if(IsEqualGUID(riid, &IID_IActiveScriptDebug)) {
        TRACE("(%p)->(IID_IActiveScriptDebug %p)\n", This, ppv);
        *ppv = &This->IActiveScriptDebug_iface;
    }else if(IsEqualGUID(riid, &IID_IActiveScriptParse)) {
        TRACE("(%p)->(IID_IActiveScriptParse %p)\n", This, ppv);
        *ppv = &This->IActiveScriptParse_iface;
    }else if(IsEqualGUID(riid, &IID_IActiveScriptParseProcedure2)) {
        TRACE("(%p)->(IID_IActiveScriptParseProcedure2 %p)\n", This, ppv);
        *ppv = &This->IActiveScriptParseProcedure2_iface;
    }else if(IsEqualGUID(riid, &IID_IObjectSafety)) {
        TRACE("(%p)->(IID_IObjectSafety %p)\n", This, ppv);
        *ppv = &This->IObjectSafety_iface;
    }else {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI VBScript_AddRef(IActiveScript *iface)
{
    VBScript *This = impl_from_IActiveScript(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI VBScript_Release(IActiveScript *iface)
{
    VBScript *This = impl_from_IActiveScript(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", iface, ref);

    if(!ref) {
        decrease_state(This, SCRIPTSTATE_CLOSED);
        detach_global_objects(This->ctx);
        IServiceProvider_Release(&This->ctx->vbcaller->IServiceProvider_iface);
        free(This->ctx);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI VBScript_SetScriptSite(IActiveScript *iface, IActiveScriptSite *pass)
{
    VBScript *This = impl_from_IActiveScript(iface);
    LCID lcid = LOCALE_USER_DEFAULT;
    named_item_t *item;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pass);

    if(!pass)
        return E_POINTER;

    if(This->ctx->site)
        return E_UNEXPECTED;

    if(InterlockedCompareExchange(&This->thread_id, GetCurrentThreadId(), 0))
        return E_UNEXPECTED;

    /* Retrieve new dispatches for persistent named items */
    LIST_FOR_EACH_ENTRY(item, &This->ctx->named_items, named_item_t, entry)
    {
        if(!item->disp)
        {
            hres = retrieve_named_item_disp(pass, item);
            if(FAILED(hres)) return hres;
        }

        /* For some reason, CODEONLY flag is lost in re-initialized scripts */
        item->flags &= ~SCRIPTITEM_CODEONLY;
    }

    hres = create_script_disp(This->ctx, &This->ctx->script_obj);
    if(FAILED(hres))
        return hres;

    This->ctx->site = pass;
    IActiveScriptSite_AddRef(This->ctx->site);

    IActiveScriptSite_GetLCID(This->ctx->site, &lcid);
    This->ctx->lcid = IsValidLocale(lcid, 0) ? lcid : GetUserDefaultLCID();
    GetLocaleInfoW(lcid, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER, (WCHAR *)&This->ctx->codepage,
            sizeof(This->ctx->codepage)/sizeof(WCHAR));
    if (!This->ctx->codepage)
        This->ctx->codepage = CP_UTF8;

    if(This->is_initialized)
        change_state(This, SCRIPTSTATE_INITIALIZED);
    return S_OK;
}

static HRESULT WINAPI VBScript_GetScriptSite(IActiveScript *iface, REFIID riid,
                                            void **ppvObject)
{
    VBScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI VBScript_SetScriptState(IActiveScript *iface, SCRIPTSTATE ss)
{
    VBScript *This = impl_from_IActiveScript(iface);

    TRACE("(%p)->(%d)\n", This, ss);

    if(This->thread_id && GetCurrentThreadId() != This->thread_id)
        return E_UNEXPECTED;

    if(ss == SCRIPTSTATE_UNINITIALIZED) {
        if(This->state == SCRIPTSTATE_CLOSED)
            return E_UNEXPECTED;

        decrease_state(This, SCRIPTSTATE_UNINITIALIZED);
        return S_OK;
    }

    if(!This->is_initialized || (!This->ctx->site && ss != SCRIPTSTATE_CLOSED))
        return E_UNEXPECTED;

    switch(ss) {
    case SCRIPTSTATE_STARTED:
    case SCRIPTSTATE_CONNECTED: /* FIXME */
        if(This->state == SCRIPTSTATE_CLOSED)
            return E_UNEXPECTED;

        exec_queued_code(This->ctx);
        break;
    case SCRIPTSTATE_INITIALIZED:
        decrease_state(This, SCRIPTSTATE_INITIALIZED);
        return S_OK;
    case SCRIPTSTATE_CLOSED:
        decrease_state(This, SCRIPTSTATE_CLOSED);
        return S_OK;
    case SCRIPTSTATE_DISCONNECTED:
        FIXME("unimplemented SCRIPTSTATE_DISCONNECTED\n");
        return S_OK;
    default:
        FIXME("unimplemented state %d\n", ss);
        return E_NOTIMPL;
    }

    change_state(This, ss);
    return S_OK;
}

static HRESULT WINAPI VBScript_GetScriptState(IActiveScript *iface, SCRIPTSTATE *pssState)
{
    VBScript *This = impl_from_IActiveScript(iface);

    TRACE("(%p)->(%p)\n", This, pssState);

    if(!pssState)
        return E_POINTER;

    if(This->thread_id && This->thread_id != GetCurrentThreadId())
        return E_UNEXPECTED;

    *pssState = This->state;
    return S_OK;
}

static HRESULT WINAPI VBScript_Close(IActiveScript *iface)
{
    VBScript *This = impl_from_IActiveScript(iface);

    TRACE("(%p)->()\n", This);

    if(This->thread_id && This->thread_id != GetCurrentThreadId())
        return E_UNEXPECTED;

    decrease_state(This, SCRIPTSTATE_CLOSED);
    return S_OK;
}

static HRESULT WINAPI VBScript_AddNamedItem(IActiveScript *iface, LPCOLESTR pstrName, DWORD dwFlags)
{
    VBScript *This = impl_from_IActiveScript(iface);
    named_item_t *item;
    IDispatch *disp = NULL;
    HRESULT hres;

    TRACE("(%p)->(%s %lx)\n", This, debugstr_w(pstrName), dwFlags);

    if(This->thread_id != GetCurrentThreadId() || !This->ctx->site)
        return E_UNEXPECTED;

    if(dwFlags & SCRIPTITEM_GLOBALMEMBERS) {
        IUnknown *unk;

        hres = IActiveScriptSite_GetItemInfo(This->ctx->site, pstrName, SCRIPTINFO_IUNKNOWN, &unk, NULL);
        if(FAILED(hres)) {
            WARN("GetItemInfo failed: %08lx\n", hres);
            return hres;
        }

        hres = IUnknown_QueryInterface(unk, &IID_IDispatch, (void**)&disp);
        IUnknown_Release(unk);
        if(FAILED(hres)) {
            WARN("object does not implement IDispatch\n");
            return hres;
        }
    }

    item = malloc(sizeof(*item));
    if(!item) {
        if(disp)
            IDispatch_Release(disp);
        return E_OUTOFMEMORY;
    }

    item->ref = 1;
    item->disp = disp;
    item->flags = dwFlags;
    item->script_obj = NULL;
    item->name = wcsdup(pstrName);
    if(!item->name) {
        if(disp)
            IDispatch_Release(disp);
        free(item);
        return E_OUTOFMEMORY;
    }

    list_add_tail(&This->ctx->named_items, &item->entry);
    return S_OK;
}

static HRESULT WINAPI VBScript_AddTypeLib(IActiveScript *iface, REFGUID rguidTypeLib,
        DWORD dwMajor, DWORD dwMinor, DWORD dwFlags)
{
    VBScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->(%s %ld %ld %ld)\n", This, debugstr_guid(rguidTypeLib), dwMajor, dwMinor, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI VBScript_GetScriptDispatch(IActiveScript *iface, LPCOLESTR pstrItemName, IDispatch **ppdisp)
{
    VBScript *This = impl_from_IActiveScript(iface);
    ScriptDisp *script_obj;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(pstrItemName), ppdisp);

    if(!ppdisp)
        return E_POINTER;

    if(This->thread_id != GetCurrentThreadId() || !This->ctx->script_obj) {
        *ppdisp = NULL;
        return E_UNEXPECTED;
    }

    script_obj = This->ctx->script_obj;
    if(pstrItemName) {
        named_item_t *item = lookup_named_item(This->ctx, pstrItemName, 0);
        if(!item) return E_INVALIDARG;
        if(item->script_obj) script_obj = item->script_obj;
    }

    *ppdisp = (IDispatch*)&script_obj->IDispatchEx_iface;
    IDispatch_AddRef(*ppdisp);
    return S_OK;
}

static HRESULT WINAPI VBScript_GetCurrentScriptThreadID(IActiveScript *iface,
                                                       SCRIPTTHREADID *pstridThread)
{
    VBScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI VBScript_GetScriptThreadID(IActiveScript *iface,
                                                DWORD dwWin32ThreadId, SCRIPTTHREADID *pstidThread)
{
    VBScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI VBScript_GetScriptThreadState(IActiveScript *iface,
        SCRIPTTHREADID stidThread, SCRIPTTHREADSTATE *pstsState)
{
    VBScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI VBScript_InterruptScriptThread(IActiveScript *iface,
        SCRIPTTHREADID stidThread, const EXCEPINFO *pexcepinfo, DWORD dwFlags)
{
    VBScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI VBScript_Clone(IActiveScript *iface, IActiveScript **ppscript)
{
    VBScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static const IActiveScriptVtbl VBScriptVtbl = {
    VBScript_QueryInterface,
    VBScript_AddRef,
    VBScript_Release,
    VBScript_SetScriptSite,
    VBScript_GetScriptSite,
    VBScript_SetScriptState,
    VBScript_GetScriptState,
    VBScript_Close,
    VBScript_AddNamedItem,
    VBScript_AddTypeLib,
    VBScript_GetScriptDispatch,
    VBScript_GetCurrentScriptThreadID,
    VBScript_GetScriptThreadID,
    VBScript_GetScriptThreadState,
    VBScript_InterruptScriptThread,
    VBScript_Clone
};

static inline VBScript *impl_from_IActiveScriptDebug(IActiveScriptDebug *iface)
{
    return CONTAINING_RECORD(iface, VBScript, IActiveScriptDebug_iface);
}

static HRESULT WINAPI VBScriptDebug_QueryInterface(IActiveScriptDebug *iface, REFIID riid, void **ppv)
{
    VBScript *This = impl_from_IActiveScriptDebug(iface);
    return IActiveScript_QueryInterface(&This->IActiveScript_iface, riid, ppv);
}

static ULONG WINAPI VBScriptDebug_AddRef(IActiveScriptDebug *iface)
{
    VBScript *This = impl_from_IActiveScriptDebug(iface);
    return IActiveScript_AddRef(&This->IActiveScript_iface);
}

static ULONG WINAPI VBScriptDebug_Release(IActiveScriptDebug *iface)
{
    VBScript *This = impl_from_IActiveScriptDebug(iface);
    return IActiveScript_Release(&This->IActiveScript_iface);
}

static HRESULT WINAPI VBScriptDebug_GetScriptTextAttributes(IActiveScriptDebug *iface,
        LPCOLESTR code, ULONG len, LPCOLESTR delimiter, DWORD flags, SOURCE_TEXT_ATTR *attr)
{
    VBScript *This = impl_from_IActiveScriptDebug(iface);
    FIXME("(%p)->(%s %lu %s %#lx %p)\n", This, debugstr_w(code), len,
          debugstr_w(delimiter), flags, attr);
    return E_NOTIMPL;
}

static HRESULT WINAPI VBScriptDebug_GetScriptletTextAttributes(IActiveScriptDebug *iface,
        LPCOLESTR code, ULONG len, LPCOLESTR delimiter, DWORD flags, SOURCE_TEXT_ATTR *attr)
{
    VBScript *This = impl_from_IActiveScriptDebug(iface);
    FIXME("(%p)->(%s %lu %s %#lx %p)\n", This, debugstr_w(code), len,
          debugstr_w(delimiter), flags, attr);
    return E_NOTIMPL;
}

static HRESULT WINAPI VBScriptDebug_EnumCodeContextsOfPosition(IActiveScriptDebug *iface,
        CTXARG_T source, ULONG offset, ULONG len, IEnumDebugCodeContexts **ret)
{
    VBScript *This = impl_from_IActiveScriptDebug(iface);
    FIXME("(%p)->(%s %lu %lu %p)\n", This, wine_dbgstr_longlong(source), offset, len, ret);
    return E_NOTIMPL;
}

static const IActiveScriptDebugVtbl VBScriptDebugVtbl = {
    VBScriptDebug_QueryInterface,
    VBScriptDebug_AddRef,
    VBScriptDebug_Release,
    VBScriptDebug_GetScriptTextAttributes,
    VBScriptDebug_GetScriptletTextAttributes,
    VBScriptDebug_EnumCodeContextsOfPosition,
};

static inline VBScript *impl_from_IActiveScriptParse(IActiveScriptParse *iface)
{
    return CONTAINING_RECORD(iface, VBScript, IActiveScriptParse_iface);
}

static HRESULT WINAPI VBScriptParse_QueryInterface(IActiveScriptParse *iface, REFIID riid, void **ppv)
{
    VBScript *This = impl_from_IActiveScriptParse(iface);
    return IActiveScript_QueryInterface(&This->IActiveScript_iface, riid, ppv);
}

static ULONG WINAPI VBScriptParse_AddRef(IActiveScriptParse *iface)
{
    VBScript *This = impl_from_IActiveScriptParse(iface);
    return IActiveScript_AddRef(&This->IActiveScript_iface);
}

static ULONG WINAPI VBScriptParse_Release(IActiveScriptParse *iface)
{
    VBScript *This = impl_from_IActiveScriptParse(iface);
    return IActiveScript_Release(&This->IActiveScript_iface);
}

static HRESULT WINAPI VBScriptParse_InitNew(IActiveScriptParse *iface)
{
    VBScript *This = impl_from_IActiveScriptParse(iface);

    TRACE("(%p)\n", This);

    if(This->is_initialized)
        return E_UNEXPECTED;
    This->is_initialized = TRUE;

    if(This->ctx->site)
        change_state(This, SCRIPTSTATE_INITIALIZED);
    return S_OK;
}

static HRESULT WINAPI VBScriptParse_AddScriptlet(IActiveScriptParse *iface,
        LPCOLESTR pstrDefaultName, LPCOLESTR pstrCode, LPCOLESTR pstrItemName,
        LPCOLESTR pstrSubItemName, LPCOLESTR pstrEventName, LPCOLESTR pstrDelimiter,
        CTXARG_T dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags,
        BSTR *pbstrName, EXCEPINFO *pexcepinfo)
{
    VBScript *This = impl_from_IActiveScriptParse(iface);
    FIXME("(%p)->(%s %s %s %s %s %s %s %lu %lx %p %p)\n", This, debugstr_w(pstrDefaultName),
          debugstr_w(pstrCode), debugstr_w(pstrItemName), debugstr_w(pstrSubItemName),
          debugstr_w(pstrEventName), debugstr_w(pstrDelimiter), wine_dbgstr_longlong(dwSourceContextCookie),
          ulStartingLineNumber, dwFlags, pbstrName, pexcepinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI VBScriptParse_ParseScriptText(IActiveScriptParse *iface,
        LPCOLESTR pstrCode, LPCOLESTR pstrItemName, IUnknown *punkContext,
        LPCOLESTR pstrDelimiter, CTXARG_T dwSourceContextCookie, ULONG ulStartingLine,
        DWORD dwFlags, VARIANT *pvarResult, EXCEPINFO *pexcepinfo)
{
    VBScript *This = impl_from_IActiveScriptParse(iface);
    vbscode_t *code;
    HRESULT hres;

    TRACE("(%p)->(%s %s %p %s %s %lu %lx %p %p)\n", This, debugstr_w(pstrCode),
          debugstr_w(pstrItemName), punkContext, debugstr_w(pstrDelimiter),
          wine_dbgstr_longlong(dwSourceContextCookie), ulStartingLine, dwFlags, pvarResult, pexcepinfo);

    if(This->thread_id != GetCurrentThreadId() || This->state == SCRIPTSTATE_CLOSED)
        return E_UNEXPECTED;

    hres = compile_script(This->ctx, pstrCode, pstrItemName, pstrDelimiter, dwSourceContextCookie,
                          ulStartingLine, dwFlags, &code);
    if(FAILED(hres))
        return hres;

    if(!(dwFlags & SCRIPTTEXT_ISEXPRESSION) && !is_started(This)) {
        code->pending_exec = TRUE;
        return S_OK;
    }

    return exec_global_code(This->ctx, code, pvarResult);
}

static const IActiveScriptParseVtbl VBScriptParseVtbl = {
    VBScriptParse_QueryInterface,
    VBScriptParse_AddRef,
    VBScriptParse_Release,
    VBScriptParse_InitNew,
    VBScriptParse_AddScriptlet,
    VBScriptParse_ParseScriptText
};

static inline VBScript *impl_from_IActiveScriptParseProcedure2(IActiveScriptParseProcedure2 *iface)
{
    return CONTAINING_RECORD(iface, VBScript, IActiveScriptParseProcedure2_iface);
}

static HRESULT WINAPI VBScriptParseProcedure_QueryInterface(IActiveScriptParseProcedure2 *iface, REFIID riid, void **ppv)
{
    VBScript *This = impl_from_IActiveScriptParseProcedure2(iface);
    return IActiveScript_QueryInterface(&This->IActiveScript_iface, riid, ppv);
}

static ULONG WINAPI VBScriptParseProcedure_AddRef(IActiveScriptParseProcedure2 *iface)
{
    VBScript *This = impl_from_IActiveScriptParseProcedure2(iface);
    return IActiveScript_AddRef(&This->IActiveScript_iface);
}

static ULONG WINAPI VBScriptParseProcedure_Release(IActiveScriptParseProcedure2 *iface)
{
    VBScript *This = impl_from_IActiveScriptParseProcedure2(iface);
    return IActiveScript_Release(&This->IActiveScript_iface);
}

static HRESULT WINAPI VBScriptParseProcedure_ParseProcedureText(IActiveScriptParseProcedure2 *iface,
        LPCOLESTR pstrCode, LPCOLESTR pstrFormalParams, LPCOLESTR pstrProcedureName,
        LPCOLESTR pstrItemName, IUnknown *punkContext, LPCOLESTR pstrDelimiter,
        CTXARG_T dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags, IDispatch **ppdisp)
{
    VBScript *This = impl_from_IActiveScriptParseProcedure2(iface);
    class_desc_t *desc;
    vbdisp_t *vbdisp;
    HRESULT hres;

    TRACE("(%p)->(%s %s %s %s %p %s %s %lu %lx %p)\n", This, debugstr_w(pstrCode), debugstr_w(pstrFormalParams),
          debugstr_w(pstrProcedureName), debugstr_w(pstrItemName), punkContext, debugstr_w(pstrDelimiter),
          wine_dbgstr_longlong(dwSourceContextCookie), ulStartingLineNumber, dwFlags, ppdisp);

    if(This->thread_id != GetCurrentThreadId() || This->state == SCRIPTSTATE_CLOSED)
        return E_UNEXPECTED;

    hres = compile_procedure(This->ctx, pstrCode, pstrItemName, pstrDelimiter, dwSourceContextCookie,
                             ulStartingLineNumber, dwFlags, &desc);
    if(FAILED(hres))
        return hres;

    hres = create_vbdisp(desc, &vbdisp);
    if(FAILED(hres))
        return hres;

    *ppdisp = (IDispatch*)&vbdisp->IDispatchEx_iface;
    return S_OK;
}

static const IActiveScriptParseProcedure2Vtbl VBScriptParseProcedureVtbl = {
    VBScriptParseProcedure_QueryInterface,
    VBScriptParseProcedure_AddRef,
    VBScriptParseProcedure_Release,
    VBScriptParseProcedure_ParseProcedureText,
};

static inline VBScript *impl_from_IObjectSafety(IObjectSafety *iface)
{
    return CONTAINING_RECORD(iface, VBScript, IObjectSafety_iface);
}

static HRESULT WINAPI VBScriptSafety_QueryInterface(IObjectSafety *iface, REFIID riid, void **ppv)
{
    VBScript *This = impl_from_IObjectSafety(iface);
    return IActiveScript_QueryInterface(&This->IActiveScript_iface, riid, ppv);
}

static ULONG WINAPI VBScriptSafety_AddRef(IObjectSafety *iface)
{
    VBScript *This = impl_from_IObjectSafety(iface);
    return IActiveScript_AddRef(&This->IActiveScript_iface);
}

static ULONG WINAPI VBScriptSafety_Release(IObjectSafety *iface)
{
    VBScript *This = impl_from_IObjectSafety(iface);
    return IActiveScript_Release(&This->IActiveScript_iface);
}

#define SUPPORTED_OPTIONS (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER)

static HRESULT WINAPI VBScriptSafety_GetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    VBScript *This = impl_from_IObjectSafety(iface);

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_guid(riid), pdwSupportedOptions, pdwEnabledOptions);

    if(!pdwSupportedOptions || !pdwEnabledOptions)
        return E_POINTER;

    *pdwSupportedOptions = SUPPORTED_OPTIONS;
    *pdwEnabledOptions = This->ctx->safeopt;
    return S_OK;
}

static HRESULT WINAPI VBScriptSafety_SetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
    VBScript *This = impl_from_IObjectSafety(iface);

    TRACE("(%p)->(%s %lx %lx)\n", This, debugstr_guid(riid), dwOptionSetMask, dwEnabledOptions);

    if(dwOptionSetMask & ~SUPPORTED_OPTIONS)
        return E_FAIL;

    This->ctx->safeopt = (dwEnabledOptions & dwOptionSetMask) | (This->ctx->safeopt & ~dwOptionSetMask) | INTERFACE_USES_DISPEX;
    return S_OK;
}

static const IObjectSafetyVtbl VBScriptSafetyVtbl = {
    VBScriptSafety_QueryInterface,
    VBScriptSafety_AddRef,
    VBScriptSafety_Release,
    VBScriptSafety_GetInterfaceSafetyOptions,
    VBScriptSafety_SetInterfaceSafetyOptions
};

HRESULT WINAPI VBScriptFactory_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    struct vbcaller *vbcaller;
    script_ctx_t *ctx;
    VBScript *ret;
    HRESULT hres;

    TRACE("(%p %s %p)\n", pUnkOuter, debugstr_guid(riid), ppv);

    ret = calloc(1, sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    if(!(vbcaller = create_vbcaller())) {
        free(ret);
        return E_OUTOFMEMORY;
    }

    ret->IActiveScript_iface.lpVtbl = &VBScriptVtbl;
    ret->IActiveScriptDebug_iface.lpVtbl = &VBScriptDebugVtbl;
    ret->IActiveScriptParse_iface.lpVtbl = &VBScriptParseVtbl;
    ret->IActiveScriptParseProcedure2_iface.lpVtbl = &VBScriptParseProcedureVtbl;
    ret->IObjectSafety_iface.lpVtbl = &VBScriptSafetyVtbl;

    ret->ref = 1;
    ret->state = SCRIPTSTATE_UNINITIALIZED;

    ctx = ret->ctx = calloc(1, sizeof(*ctx));
    if(!ctx) {
        IServiceProvider_Release(&vbcaller->IServiceProvider_iface);
        free(ret);
        return E_OUTOFMEMORY;
    }

    vbcaller->ctx = ctx;
    ctx->vbcaller = vbcaller;
    ctx->safeopt = INTERFACE_USES_DISPEX;
    list_init(&ctx->objects);
    list_init(&ctx->code_list);
    list_init(&ctx->named_items);

    hres = init_global(ctx);
    if(FAILED(hres)) {
        IActiveScript_Release(&ret->IActiveScript_iface);
        return hres;
    }

    hres = IActiveScript_QueryInterface(&ret->IActiveScript_iface, riid, ppv);
    IActiveScript_Release(&ret->IActiveScript_iface);
    return hres;
}

typedef struct {
    IServiceProvider IServiceProvider_iface;

    LONG ref;

    IServiceProvider *sp;
} AXSite;

static inline AXSite *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, AXSite, IServiceProvider_iface);
}

static HRESULT WINAPI AXSite_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    AXSite *This = impl_from_IServiceProvider(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IServiceProvider_iface;
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = &This->IServiceProvider_iface;
    }else {
        TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI AXSite_AddRef(IServiceProvider *iface)
{
    AXSite *This = impl_from_IServiceProvider(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI AXSite_Release(IServiceProvider *iface)
{
    AXSite *This = impl_from_IServiceProvider(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref)
        free(This);

    return ref;
}

static HRESULT WINAPI AXSite_QueryService(IServiceProvider *iface,
        REFGUID guidService, REFIID riid, void **ppv)
{
    AXSite *This = impl_from_IServiceProvider(iface);

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);

    return IServiceProvider_QueryService(This->sp, guidService, riid, ppv);
}

static IServiceProviderVtbl AXSiteVtbl = {
    AXSite_QueryInterface,
    AXSite_AddRef,
    AXSite_Release,
    AXSite_QueryService
};

IUnknown *create_ax_site(script_ctx_t *ctx)
{
    IServiceProvider *sp;
    AXSite *ret;
    HRESULT hres;

    hres = IActiveScriptSite_QueryInterface(ctx->site, &IID_IServiceProvider, (void**)&sp);
    if(FAILED(hres)) {
        ERR("Could not get IServiceProvider iface: %08lx\n", hres);
        return NULL;
    }

    ret = malloc(sizeof(*ret));
    if(!ret) {
        IServiceProvider_Release(sp);
        return NULL;
    }

    ret->IServiceProvider_iface.lpVtbl = &AXSiteVtbl;
    ret->ref = 1;
    ret->sp = sp;

    return (IUnknown*)&ret->IServiceProvider_iface;
}
