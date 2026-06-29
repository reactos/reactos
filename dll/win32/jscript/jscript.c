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

#include <assert.h>

#include "jscript.h"
#include "engine.h"
#include "objsafe.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

#ifdef _WIN64

#define CTXARG_T DWORDLONG
#define IActiveScriptParseVtbl IActiveScriptParse64Vtbl
#define IActiveScriptParseProcedure2Vtbl IActiveScriptParseProcedure2_64Vtbl

#else

#define CTXARG_T DWORD
#define IActiveScriptParseVtbl IActiveScriptParse32Vtbl
#define IActiveScriptParseProcedure2Vtbl IActiveScriptParseProcedure2_32Vtbl

#endif

typedef struct {
    IActiveScript                IActiveScript_iface;
    IActiveScriptParse           IActiveScriptParse_iface;
    IActiveScriptParseProcedure2 IActiveScriptParseProcedure2_iface;
    IActiveScriptProperty        IActiveScriptProperty_iface;
    IObjectSafety                IObjectSafety_iface;
    IVariantChangeType           IVariantChangeType_iface;
    IWineJScript                 IWineJScript_iface;

    LONG ref;

    DWORD safeopt;
    struct thread_data *thread_data;
    script_ctx_t *ctx;
    LCID lcid;
    DWORD version;
    BOOL html_mode;
    BOOL is_encode;
    BOOL is_initialized;

    IActiveScriptSite *site;

    struct list persistent_code;
    struct list queued_code;
} JScript;

typedef struct {
    IActiveScriptError IActiveScriptError_iface;
    LONG ref;
    jsexcept_t ei;
} JScriptError;

void script_release(script_ctx_t *ctx)
{
    if(--ctx->ref)
        return;

    jsval_release(ctx->acc);
    if(ctx->cc)
        release_cc(ctx->cc);
    heap_pool_free(&ctx->tmp_heap);
    if(ctx->last_match)
        jsstr_release(ctx->last_match);
    assert(!ctx->stack_top);
    free(ctx->stack);

    ctx->jscaller->ctx = NULL;
    IServiceProvider_Release(&ctx->jscaller->IServiceProvider_iface);

    release_thread_data(ctx->thread_data);
    free(ctx);
}

static void script_globals_release(script_ctx_t *ctx)
{
    unsigned i;
    for(i = 0; i < ARRAY_SIZE(ctx->global_objects); i++) {
        if(ctx->global_objects[i]) {
            jsdisp_release(ctx->global_objects[i]);
            ctx->global_objects[i] = NULL;
        }
    }
}

static void change_state(JScript *This, SCRIPTSTATE state)
{
    if(This->ctx->state == state)
        return;

    This->ctx->state = state;
    if(This->site)
        IActiveScriptSite_OnStateChange(This->site, state);
}

static inline BOOL is_started(script_ctx_t *ctx)
{
    return ctx->state == SCRIPTSTATE_STARTED
        || ctx->state == SCRIPTSTATE_CONNECTED
        || ctx->state == SCRIPTSTATE_DISCONNECTED;
}

HRESULT create_named_item_script_obj(script_ctx_t *ctx, named_item_t *item)
{
    static const builtin_info_t disp_info = { .class = JSCLASS_GLOBAL };
    return create_dispex(ctx, &disp_info, NULL, &item->script_obj);
}

static void release_named_item_script_obj(named_item_t *item)
{
    if(!item->script_obj) return;

    jsdisp_release(item->script_obj);
    item->script_obj = NULL;
}

static HRESULT retrieve_named_item_disp(IActiveScriptSite *site, named_item_t *item)
{
    IUnknown *unk;
    HRESULT hr;

    if(!site)
        return E_UNEXPECTED;

    hr = IActiveScriptSite_GetItemInfo(site, item->name, SCRIPTINFO_IUNKNOWN, &unk, NULL);
    if(FAILED(hr)) {
        WARN("GetItemInfo failed: %08lx\n", hr);
        return hr;
    }

    hr = IUnknown_QueryInterface(unk, &IID_IDispatch, (void**)&item->disp);
    IUnknown_Release(unk);
    if(FAILED(hr)) {
        WARN("object does not implement IDispatch\n");
        return hr;
    }

    return S_OK;
}

named_item_t *lookup_named_item(script_ctx_t *ctx, const WCHAR *item_name, unsigned flags)
{
    named_item_t *item;
    HRESULT hr;

    LIST_FOR_EACH_ENTRY(item, &ctx->named_items, named_item_t, entry) {
        if((item->flags & flags) == flags && !wcscmp(item->name, item_name)) {
            if(!item->script_obj && !(item->flags & SCRIPTITEM_GLOBALMEMBERS)) {
                hr = create_named_item_script_obj(ctx, item);
                if(FAILED(hr)) return NULL;
            }

            if(!item->disp && (flags || !(item->flags & SCRIPTITEM_CODEONLY))) {
                hr = retrieve_named_item_disp(ctx->site, item);
                if(FAILED(hr)) continue;
            }

            return item;
        }
    }

    return NULL;
}

void release_named_item(named_item_t *item)
{
    if(--item->ref) return;

    free(item->name);
    free(item);
}

static inline JScriptError *impl_from_IActiveScriptError(IActiveScriptError *iface)
{
    return CONTAINING_RECORD(iface, JScriptError, IActiveScriptError_iface);
}

static HRESULT WINAPI JScriptError_QueryInterface(IActiveScriptError *iface, REFIID riid, void **ppv)
{
    JScriptError *This = impl_from_IActiveScriptError(iface);

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

static ULONG WINAPI JScriptError_AddRef(IActiveScriptError *iface)
{
    JScriptError *This = impl_from_IActiveScriptError(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI JScriptError_Release(IActiveScriptError *iface)
{
    JScriptError *This = impl_from_IActiveScriptError(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        reset_ei(&This->ei);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI JScriptError_GetExceptionInfo(IActiveScriptError *iface, EXCEPINFO *excepinfo)
{
    JScriptError *This = impl_from_IActiveScriptError(iface);

    TRACE("(%p)->(%p)\n", This, excepinfo);

    if(!excepinfo)
        return E_POINTER;

    memset(excepinfo, 0, sizeof(*excepinfo));
    excepinfo->scode = This->ei.error;
    if(This->ei.source)
        jsstr_to_bstr(This->ei.source, &excepinfo->bstrSource);
    if(This->ei.message)
        jsstr_to_bstr(This->ei.message, &excepinfo->bstrDescription);
    return S_OK;
}

static HRESULT WINAPI JScriptError_GetSourcePosition(IActiveScriptError *iface, DWORD *source_context, ULONG *line, LONG *character)
{
    JScriptError *This = impl_from_IActiveScriptError(iface);
    bytecode_t *code = This->ei.code;
    unsigned line_pos, char_pos;

    TRACE("(%p)->(%p %p %p)\n", This, source_context, line, character);

    if(!This->ei.code) {
        FIXME("unknown position\n");
        return E_FAIL;
    }

    if(source_context)
        *source_context = This->ei.code->source_context;
    if(!line && !character)
        return S_OK;

    line_pos = get_location_line(code, This->ei.loc, &char_pos);
    if(line)
        *line = line_pos;
    if(character)
        *character = char_pos;
    return S_OK;
}

static HRESULT WINAPI JScriptError_GetSourceLineText(IActiveScriptError *iface, BSTR *source)
{
    JScriptError *This = impl_from_IActiveScriptError(iface);

    TRACE("(%p)->(%p)\n", This, source);

    if(!source)
        return E_POINTER;

    if(!This->ei.line) {
        *source = NULL;
        return E_FAIL;
    }

    return jsstr_to_bstr(This->ei.line, source);
}

static const IActiveScriptErrorVtbl JScriptErrorVtbl = {
    JScriptError_QueryInterface,
    JScriptError_AddRef,
    JScriptError_Release,
    JScriptError_GetExceptionInfo,
    JScriptError_GetSourcePosition,
    JScriptError_GetSourceLineText
};

void reset_ei(jsexcept_t *ei)
{
    ei->error = S_OK;
    if(ei->valid_value) {
        jsval_release(ei->value);
        ei->valid_value = FALSE;
    }
    if(ei->code) {
        release_bytecode(ei->code);
        ei->code = NULL;
        ei->loc = 0;
    }
    if(ei->source) {
        jsstr_release(ei->source);
        ei->source = NULL;
    }
    if(ei->message) {
        jsstr_release(ei->message);
        ei->message = NULL;
    }
    if(ei->line) {
        jsstr_release(ei->line);
        ei->line = NULL;
    }
}

void enter_script(script_ctx_t *ctx, jsexcept_t *ei)
{
    memset(ei, 0, sizeof(*ei));
    ei->prev = ctx->ei;
    ctx->ei = ei;
    TRACE("ctx %p ei %p prev %p\n", ctx, ei, ei->prev);
}

HRESULT leave_script(script_ctx_t *ctx, HRESULT result)
{
    jsexcept_t *ei = ctx->ei;
    BOOL enter_notified = ei->enter_notified;
    JScriptError *error;

    TRACE("ctx %p ei %p prev %p\n", ctx, ei, ei->prev);

    ctx->ei = ei->prev;
    if(result == DISP_E_EXCEPTION) {
        result = ei->error;
    }else {
        reset_ei(ei);
        ei->error = result;
    }
    if(FAILED(result)) {
        WARN("%08lx\n", result);
        if(ctx->site && (error = malloc(sizeof(*error)))) {
            HRESULT hres;

            error->IActiveScriptError_iface.lpVtbl = &JScriptErrorVtbl;
            error->ref = 1;
            error->ei = *ei;
            memset(ei, 0, sizeof(*ei));

            hres = IActiveScriptSite_OnScriptError(ctx->site, &error->IActiveScriptError_iface);
            IActiveScriptError_Release(&error->IActiveScriptError_iface);
            if(hres == S_OK)
                result = SCRIPT_E_REPORTED;
        }
    }
    if(enter_notified && ctx->site)
        IActiveScriptSite_OnLeaveScript(ctx->site);
    reset_ei(ei);
    return result;
}

static void clear_script_queue(JScript *This)
{
    while(!list_empty(&This->queued_code))
    {
        bytecode_t *iter = LIST_ENTRY(list_head(&This->queued_code), bytecode_t, entry);
        list_remove(&iter->entry);
        if (iter->is_persistent)
            list_add_tail(&This->persistent_code, &iter->entry);
        else
            release_bytecode(iter);
    }
}

static void clear_persistent_code_list(JScript *This)
{
    while(!list_empty(&This->persistent_code))
    {
        bytecode_t *iter = LIST_ENTRY(list_head(&This->persistent_code), bytecode_t, entry);
        list_remove(&iter->entry);
        release_bytecode(iter);
    }
}

static void release_persistent_script_objs(JScript *This)
{
    bytecode_t *iter;

    LIST_FOR_EACH_ENTRY(iter, &This->persistent_code, bytecode_t, entry)
        if(iter->named_item)
            release_named_item_script_obj(iter->named_item);
}

static void release_named_item_list(JScript *This)
{
    while(!list_empty(&This->ctx->named_items)) {
        named_item_t *iter = LIST_ENTRY(list_head(&This->ctx->named_items), named_item_t, entry);
        list_remove(&iter->entry);
        release_named_item(iter);
    }
}

static HRESULT exec_global_code(script_ctx_t *ctx, bytecode_t *code, jsval_t *r)
{
    IServiceProvider *prev_caller = ctx->jscaller->caller;
    HRESULT hres;

    ctx->jscaller->caller = SP_CALLER_UNINITIALIZED;
    hres = exec_source(ctx, EXEC_GLOBAL, code, &code->global_code, NULL, NULL, NULL, 0, NULL, r);
    ctx->jscaller->caller = prev_caller;
    return hres;
}

static void exec_queued_code(JScript *This)
{
    bytecode_t *iter;
    jsexcept_t ei;
    HRESULT hres = S_OK;

    LIST_FOR_EACH_ENTRY(iter, &This->queued_code, bytecode_t, entry) {
        enter_script(This->ctx, &ei);
        hres = exec_global_code(This->ctx, iter, NULL);
        leave_script(This->ctx, hres);
        if(FAILED(hres))
            break;
    }

    clear_script_queue(This);
}

static void decrease_state(JScript *This, SCRIPTSTATE state)
{
    named_item_t *item, *item_next;

    if(This->ctx) {
        switch(This->ctx->state) {
        case SCRIPTSTATE_CONNECTED:
            change_state(This, SCRIPTSTATE_DISCONNECTED);
            if(state == SCRIPTSTATE_DISCONNECTED)
                return;
            /* FALLTHROUGH */
        case SCRIPTSTATE_STARTED:
        case SCRIPTSTATE_DISCONNECTED:
            clear_script_queue(This);

            if(This->ctx->state == SCRIPTSTATE_DISCONNECTED)
                change_state(This, SCRIPTSTATE_INITIALIZED);
            if(state == SCRIPTSTATE_INITIALIZED)
                return;
            /* FALLTHROUGH */
        case SCRIPTSTATE_INITIALIZED:
            clear_script_queue(This);
            release_persistent_script_objs(This);

            LIST_FOR_EACH_ENTRY_SAFE(item, item_next, &This->ctx->named_items, named_item_t, entry)
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

            if(This->ctx->secmgr) {
                IInternetHostSecurityManager_Release(This->ctx->secmgr);
                This->ctx->secmgr = NULL;
            }

            if(This->ctx->site) {
                IActiveScriptSite_Release(This->ctx->site);
                This->ctx->site = NULL;
            }

            script_globals_release(This->ctx);
            gc_run(This->ctx);

            /* FALLTHROUGH */
        case SCRIPTSTATE_UNINITIALIZED:
            change_state(This, state);
            break;
        default:
            assert(0);
        }

        change_state(This, state);
    }else if(state == SCRIPTSTATE_UNINITIALIZED) {
        if(This->site)
            IActiveScriptSite_OnStateChange(This->site, state);
    }else {
        FIXME("NULL ctx\n");
    }

    if((state == SCRIPTSTATE_UNINITIALIZED || state == SCRIPTSTATE_CLOSED) && This->thread_data) {
        release_thread_data(This->thread_data);
        This->thread_data = NULL;
    }

    if(This->site) {
        IActiveScriptSite_Release(This->site);
        This->site = NULL;
    }
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
    {
        if(This->sp)
            IServiceProvider_Release(This->sp);

        free(This);
    }

    return ref;
}

static HRESULT WINAPI AXSite_QueryService(IServiceProvider *iface,
        REFGUID guidService, REFIID riid, void **ppv)
{
    AXSite *This = impl_from_IServiceProvider(iface);

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);

    if(!This->sp)
        return E_NOINTERFACE;

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
    IServiceProvider *sp = NULL;
    AXSite *ret;
    HRESULT hres;

    hres = IActiveScriptSite_QueryInterface(ctx->site, &IID_IServiceProvider, (void**)&sp);
    if(FAILED(hres)) {
        TRACE("Could not get IServiceProvider iface: %08lx\n", hres);
    }

    ret = malloc(sizeof(AXSite));
    if(!ret) {
        IServiceProvider_Release(sp);
        return NULL;
    }

    ret->IServiceProvider_iface.lpVtbl = &AXSiteVtbl;
    ret->ref = 1;
    ret->sp = sp;

    return (IUnknown*)&ret->IServiceProvider_iface;
}

static inline JScript *impl_from_IActiveScript(IActiveScript *iface)
{
    return CONTAINING_RECORD(iface, JScript, IActiveScript_iface);
}

static HRESULT WINAPI JScript_QueryInterface(IActiveScript *iface, REFIID riid, void **ppv)
{
    JScript *This = impl_from_IActiveScript(iface);

    *ppv = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IActiveScript_iface;
    }else if(IsEqualGUID(riid, &IID_IActiveScript)) {
        TRACE("(%p)->(IID_IActiveScript %p)\n", This, ppv);
        *ppv = &This->IActiveScript_iface;
    }else if(IsEqualGUID(riid, &IID_IActiveScriptParse)) {
        TRACE("(%p)->(IID_IActiveScriptParse %p)\n", This, ppv);
        *ppv = &This->IActiveScriptParse_iface;
    }else if(IsEqualGUID(riid, &IID_IActiveScriptParseProcedure)) {
        TRACE("(%p)->(IID_IActiveScriptParseProcedure %p)\n", This, ppv);
        *ppv = &This->IActiveScriptParseProcedure2_iface;
    }else if(IsEqualGUID(riid, &IID_IActiveScriptParseProcedure2)) {
        TRACE("(%p)->(IID_IActiveScriptParseProcedure2 %p)\n", This, ppv);
        *ppv = &This->IActiveScriptParseProcedure2_iface;
    }else if(IsEqualGUID(riid, &IID_IActiveScriptProperty)) {
        TRACE("(%p)->(IID_IActiveScriptProperty %p)\n", This, ppv);
        *ppv = &This->IActiveScriptProperty_iface;
    }else if(IsEqualGUID(riid, &IID_IObjectSafety)) {
        TRACE("(%p)->(IID_IObjectSafety %p)\n", This, ppv);
        *ppv = &This->IObjectSafety_iface;
    }else if(IsEqualGUID(riid, &IID_IVariantChangeType)) {
        TRACE("(%p)->(IID_IVariantChangeType %p)\n", This, ppv);
        *ppv = &This->IVariantChangeType_iface;
    }else if(IsEqualGUID(riid, &IID_IWineJScript)) {
        TRACE("(%p)->(IID_IWineJScript %p)\n", This, ppv);
        *ppv = &This->IWineJScript_iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI JScript_AddRef(IActiveScript *iface)
{
    JScript *This = impl_from_IActiveScript(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI JScript_Release(IActiveScript *iface)
{
    JScript *This = impl_from_IActiveScript(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", iface, ref);

    if(!ref) {
        if(This->ctx && This->ctx->state != SCRIPTSTATE_CLOSED)
            IActiveScript_Close(&This->IActiveScript_iface);
        if(This->ctx) {
            This->ctx->active_script = NULL;
            script_release(This->ctx);
        }
        if(This->thread_data)
            release_thread_data(This->thread_data);
        free(This);
        unlock_module();
    }

    return ref;
}

static HRESULT WINAPI JScript_SetScriptSite(IActiveScript *iface,
                                            IActiveScriptSite *pass)
{
    JScript *This = impl_from_IActiveScript(iface);
    struct thread_data *thread_data;
    named_item_t *item;
    LCID lcid;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pass);

    if(!pass)
        return E_POINTER;

    if(This->site)
        return E_UNEXPECTED;

    if(!(thread_data = get_thread_data()))
        return E_OUTOFMEMORY;

    if(InterlockedCompareExchangePointer((void**)&This->thread_data, thread_data, NULL)) {
        release_thread_data(thread_data);
        return E_UNEXPECTED;
    }

    if(!This->ctx) {
        script_ctx_t *ctx = calloc(1, sizeof(script_ctx_t));
        if(!ctx)
            return E_OUTOFMEMORY;

        ctx->ref = 1;
        ctx->state = SCRIPTSTATE_UNINITIALIZED;
        ctx->active_script = &This->IActiveScript_iface;
        ctx->safeopt = This->safeopt;
        ctx->version = This->version;
        ctx->html_mode = This->html_mode;
        ctx->acc = jsval_undefined();
        list_init(&ctx->named_items);
        heap_pool_init(&ctx->tmp_heap);

        hres = create_jscaller(ctx);
        if(FAILED(hres)) {
            free(ctx);
            return hres;
        }

        thread_data->ref++;
        ctx->thread_data = thread_data;
        ctx->last_match = jsstr_empty();

        This->ctx = ctx;
    }

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

    This->site = pass;
    IActiveScriptSite_AddRef(This->site);

    hres = IActiveScriptSite_GetLCID(This->site, &lcid);
    if(hres == S_OK)
        This->lcid = lcid;

    This->ctx->lcid = This->lcid;

    hres = init_global(This->ctx);
    if(FAILED(hres))
        return hres;

    IActiveScriptSite_AddRef(This->site);
    This->ctx->site = This->site;

    if(This->is_initialized)
        change_state(This, SCRIPTSTATE_INITIALIZED);
    return S_OK;
}

static HRESULT WINAPI JScript_GetScriptSite(IActiveScript *iface, REFIID riid,
                                            void **ppvObject)
{
    JScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_SetScriptState(IActiveScript *iface, SCRIPTSTATE ss)
{
    JScript *This = impl_from_IActiveScript(iface);

    TRACE("(%p)->(%d)\n", This, ss);

    if(This->thread_data && This->thread_data->thread_id != GetCurrentThreadId())
        return E_UNEXPECTED;

    if(ss == SCRIPTSTATE_UNINITIALIZED) {
        if(This->ctx && This->ctx->state == SCRIPTSTATE_CLOSED)
            return E_UNEXPECTED;

        decrease_state(This, SCRIPTSTATE_UNINITIALIZED);
        list_move_tail(&This->queued_code, &This->persistent_code);
        return S_OK;
    }

    if(!This->is_initialized || !This->ctx)
        return E_UNEXPECTED;

    switch(ss) {
    case SCRIPTSTATE_STARTED:
    case SCRIPTSTATE_CONNECTED: /* FIXME */
        if(This->ctx->state == SCRIPTSTATE_UNINITIALIZED || This->ctx->state == SCRIPTSTATE_CLOSED)
            return E_UNEXPECTED;

        exec_queued_code(This);
        break;
    case SCRIPTSTATE_INITIALIZED:
        FIXME("unimplemented SCRIPTSTATE_INITIALIZED\n");
        return S_OK;
    default:
        FIXME("unimplemented state %d\n", ss);
        return E_NOTIMPL;
    }

    change_state(This, ss);
    return S_OK;
}

static HRESULT WINAPI JScript_GetScriptState(IActiveScript *iface, SCRIPTSTATE *pssState)
{
    JScript *This = impl_from_IActiveScript(iface);

    TRACE("(%p)->(%p)\n", This, pssState);

    if(!pssState)
        return E_POINTER;

    if(This->thread_data && This->thread_data->thread_id != GetCurrentThreadId())
        return E_UNEXPECTED;

    *pssState = This->ctx ? This->ctx->state : SCRIPTSTATE_UNINITIALIZED;
    return S_OK;
}

static HRESULT WINAPI JScript_Close(IActiveScript *iface)
{
    JScript *This = impl_from_IActiveScript(iface);

    TRACE("(%p)->()\n", This);

    if(This->thread_data && This->thread_data->thread_id != GetCurrentThreadId())
        return E_UNEXPECTED;

    decrease_state(This, SCRIPTSTATE_CLOSED);
    clear_persistent_code_list(This);
    release_named_item_list(This);
    return S_OK;
}

static HRESULT WINAPI JScript_AddNamedItem(IActiveScript *iface,
                                           LPCOLESTR pstrName, DWORD dwFlags)
{
    JScript *This = impl_from_IActiveScript(iface);
    named_item_t *item;
    IDispatch *disp = NULL;
    HRESULT hres;

    TRACE("(%p)->(%s %lx)\n", This, debugstr_w(pstrName), dwFlags);

    if(!This->thread_data || This->thread_data->thread_id != GetCurrentThreadId() || !This->ctx || This->ctx->state == SCRIPTSTATE_CLOSED)
        return E_UNEXPECTED;

    if(dwFlags & SCRIPTITEM_GLOBALMEMBERS) {
        IUnknown *unk;

        hres = IActiveScriptSite_GetItemInfo(This->site, pstrName, SCRIPTINFO_IUNKNOWN, &unk, NULL);
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

static HRESULT WINAPI JScript_AddTypeLib(IActiveScript *iface, REFGUID rguidTypeLib,
                                         DWORD dwMajor, DWORD dwMinor, DWORD dwFlags)
{
    JScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_GetScriptDispatch(IActiveScript *iface, LPCOLESTR pstrItemName,
                                                IDispatch **ppdisp)
{
    JScript *This = impl_from_IActiveScript(iface);
    jsdisp_t *script_obj;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(pstrItemName), ppdisp);

    if(!ppdisp)
        return E_POINTER;

    if(!This->thread_data || This->thread_data->thread_id != GetCurrentThreadId() || !This->ctx->global) {
        *ppdisp = NULL;
        return E_UNEXPECTED;
    }

    script_obj = This->ctx->global;
    if(pstrItemName) {
        named_item_t *item = lookup_named_item(This->ctx, pstrItemName, 0);
        if(!item) return E_INVALIDARG;
        if(item->script_obj) script_obj = item->script_obj;
    }

    *ppdisp = to_disp(script_obj);
    IDispatch_AddRef(*ppdisp);
    return S_OK;
}

static HRESULT WINAPI JScript_GetCurrentScriptThreadID(IActiveScript *iface,
                                                       SCRIPTTHREADID *pstridThread)
{
    JScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_GetScriptThreadID(IActiveScript *iface,
                                                DWORD dwWin32ThreadId, SCRIPTTHREADID *pstidThread)
{
    JScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_GetScriptThreadState(IActiveScript *iface,
        SCRIPTTHREADID stidThread, SCRIPTTHREADSTATE *pstsState)
{
    JScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_InterruptScriptThread(IActiveScript *iface,
        SCRIPTTHREADID stidThread, const EXCEPINFO *pexcepinfo, DWORD dwFlags)
{
    JScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_Clone(IActiveScript *iface, IActiveScript **ppscript)
{
    JScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static const IActiveScriptVtbl JScriptVtbl = {
    JScript_QueryInterface,
    JScript_AddRef,
    JScript_Release,
    JScript_SetScriptSite,
    JScript_GetScriptSite,
    JScript_SetScriptState,
    JScript_GetScriptState,
    JScript_Close,
    JScript_AddNamedItem,
    JScript_AddTypeLib,
    JScript_GetScriptDispatch,
    JScript_GetCurrentScriptThreadID,
    JScript_GetScriptThreadID,
    JScript_GetScriptThreadState,
    JScript_InterruptScriptThread,
    JScript_Clone
};

static inline JScript *impl_from_IActiveScriptParse(IActiveScriptParse *iface)
{
    return CONTAINING_RECORD(iface, JScript, IActiveScriptParse_iface);
}

static HRESULT WINAPI JScriptParse_QueryInterface(IActiveScriptParse *iface, REFIID riid, void **ppv)
{
    JScript *This = impl_from_IActiveScriptParse(iface);
    return IActiveScript_QueryInterface(&This->IActiveScript_iface, riid, ppv);
}

static ULONG WINAPI JScriptParse_AddRef(IActiveScriptParse *iface)
{
    JScript *This = impl_from_IActiveScriptParse(iface);
    return IActiveScript_AddRef(&This->IActiveScript_iface);
}

static ULONG WINAPI JScriptParse_Release(IActiveScriptParse *iface)
{
    JScript *This = impl_from_IActiveScriptParse(iface);
    return IActiveScript_Release(&This->IActiveScript_iface);
}

static HRESULT WINAPI JScriptParse_InitNew(IActiveScriptParse *iface)
{
    JScript *This = impl_from_IActiveScriptParse(iface);

    TRACE("(%p)\n", This);

    if(This->is_initialized)
        return E_UNEXPECTED;
    This->is_initialized = TRUE;

    if(This->site)
        change_state(This, SCRIPTSTATE_INITIALIZED);
    return S_OK;
}

static HRESULT WINAPI JScriptParse_AddScriptlet(IActiveScriptParse *iface,
        LPCOLESTR pstrDefaultName, LPCOLESTR pstrCode, LPCOLESTR pstrItemName,
        LPCOLESTR pstrSubItemName, LPCOLESTR pstrEventName, LPCOLESTR pstrDelimiter,
        CTXARG_T dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags,
        BSTR *pbstrName, EXCEPINFO *pexcepinfo)
{
    JScript *This = impl_from_IActiveScriptParse(iface);
    FIXME("(%p)->(%s %s %s %s %s %s %s %lu %lx %p %p)\n", This, debugstr_w(pstrDefaultName),
          debugstr_w(pstrCode), debugstr_w(pstrItemName), debugstr_w(pstrSubItemName),
          debugstr_w(pstrEventName), debugstr_w(pstrDelimiter), wine_dbgstr_longlong(dwSourceContextCookie),
          ulStartingLineNumber, dwFlags, pbstrName, pexcepinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScriptParse_ParseScriptText(IActiveScriptParse *iface,
        LPCOLESTR pstrCode, LPCOLESTR pstrItemName, IUnknown *punkContext,
        LPCOLESTR pstrDelimiter, CTXARG_T dwSourceContextCookie, ULONG ulStartingLine,
        DWORD dwFlags, VARIANT *pvarResult, EXCEPINFO *pexcepinfo)
{
    JScript *This = impl_from_IActiveScriptParse(iface);
    named_item_t *item = NULL;
    bytecode_t *code;
    jsexcept_t ei;
    HRESULT hres;

    TRACE("(%p)->(%s %s %p %s %s %lu %lx %p %p)\n", This, debugstr_w(pstrCode),
          debugstr_w(pstrItemName), punkContext, debugstr_w(pstrDelimiter),
          wine_dbgstr_longlong(dwSourceContextCookie), ulStartingLine, dwFlags, pvarResult, pexcepinfo);

    if(!This->thread_data || This->thread_data->thread_id != GetCurrentThreadId() || This->ctx->state == SCRIPTSTATE_CLOSED)
        return E_UNEXPECTED;

    if(pstrItemName) {
        item = lookup_named_item(This->ctx, pstrItemName, 0);
        if(!item) {
            WARN("Unknown context %s\n", debugstr_w(pstrItemName));
            return E_INVALIDARG;
        }
        if(!item->script_obj) item = NULL;
    }

    enter_script(This->ctx, &ei);
    hres = compile_script(This->ctx, pstrCode, dwSourceContextCookie, ulStartingLine, NULL, pstrDelimiter,
            (dwFlags & SCRIPTTEXT_ISEXPRESSION) != 0, This->is_encode, item, &code);
    if(FAILED(hres))
        return leave_script(This->ctx, hres);

    if(dwFlags & SCRIPTTEXT_ISEXPRESSION) {
        jsval_t r;

        hres = exec_global_code(This->ctx, code, &r);
        if(SUCCEEDED(hres)) {
            if(pvarResult)
                hres = jsval_to_variant(r, pvarResult);
            jsval_release(r);
        }

        return leave_script(This->ctx, hres);
    }

    code->is_persistent = (dwFlags & SCRIPTTEXT_ISPERSISTENT) != 0;

    /*
     * Although pvarResult is not really used without SCRIPTTEXT_ISEXPRESSION flag, if it's not NULL,
     * script is executed immediately, even if it's not in started state yet.
     */
    if(!pvarResult && !is_started(This->ctx)) {
        list_add_tail(&This->queued_code, &code->entry);
    }else {
        hres = exec_global_code(This->ctx, code, NULL);
        if(code->is_persistent)
            list_add_tail(&This->persistent_code, &code->entry);
        else
            release_bytecode(code);
    }

    if(FAILED(hres = leave_script(This->ctx, hres)))
        return hres;

    if(pvarResult)
        V_VT(pvarResult) = VT_EMPTY;
    return S_OK;
}

static const IActiveScriptParseVtbl JScriptParseVtbl = {
    JScriptParse_QueryInterface,
    JScriptParse_AddRef,
    JScriptParse_Release,
    JScriptParse_InitNew,
    JScriptParse_AddScriptlet,
    JScriptParse_ParseScriptText
};

static inline JScript *impl_from_IActiveScriptParseProcedure2(IActiveScriptParseProcedure2 *iface)
{
    return CONTAINING_RECORD(iface, JScript, IActiveScriptParseProcedure2_iface);
}

static HRESULT WINAPI JScriptParseProcedure_QueryInterface(IActiveScriptParseProcedure2 *iface, REFIID riid, void **ppv)
{
    JScript *This = impl_from_IActiveScriptParseProcedure2(iface);
    return IActiveScript_QueryInterface(&This->IActiveScript_iface, riid, ppv);
}

static ULONG WINAPI JScriptParseProcedure_AddRef(IActiveScriptParseProcedure2 *iface)
{
    JScript *This = impl_from_IActiveScriptParseProcedure2(iface);
    return IActiveScript_AddRef(&This->IActiveScript_iface);
}

static ULONG WINAPI JScriptParseProcedure_Release(IActiveScriptParseProcedure2 *iface)
{
    JScript *This = impl_from_IActiveScriptParseProcedure2(iface);
    return IActiveScript_Release(&This->IActiveScript_iface);
}

static HRESULT WINAPI JScriptParseProcedure_ParseProcedureText(IActiveScriptParseProcedure2 *iface,
        LPCOLESTR pstrCode, LPCOLESTR pstrFormalParams, LPCOLESTR pstrProcedureName,
        LPCOLESTR pstrItemName, IUnknown *punkContext, LPCOLESTR pstrDelimiter,
        CTXARG_T dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags, IDispatch **ppdisp)
{
    JScript *This = impl_from_IActiveScriptParseProcedure2(iface);
    named_item_t *item = NULL;
    bytecode_t *code;
    jsdisp_t *dispex;
    jsexcept_t ei;
    HRESULT hres;

    TRACE("(%p)->(%s %s %s %s %p %s %s %lu %lx %p)\n", This, debugstr_w(pstrCode), debugstr_w(pstrFormalParams),
          debugstr_w(pstrProcedureName), debugstr_w(pstrItemName), punkContext, debugstr_w(pstrDelimiter),
          wine_dbgstr_longlong(dwSourceContextCookie), ulStartingLineNumber, dwFlags, ppdisp);

    if(!This->thread_data || This->thread_data->thread_id != GetCurrentThreadId() || This->ctx->state == SCRIPTSTATE_CLOSED)
        return E_UNEXPECTED;

    if(pstrItemName) {
        item = lookup_named_item(This->ctx, pstrItemName, 0);
        if(!item) {
            WARN("Unknown context %s\n", debugstr_w(pstrItemName));
            return E_INVALIDARG;
        }
        if(!item->script_obj) item = NULL;
    }

    enter_script(This->ctx, &ei);
    hres = compile_script(This->ctx, pstrCode, dwSourceContextCookie, ulStartingLineNumber, pstrFormalParams,
                          pstrDelimiter, FALSE, This->is_encode, item, &code);
    if(FAILED(hres))
        return leave_script(This->ctx, hres);

    hres = create_source_function(This->ctx, code, &code->global_code, NULL, &dispex);
    release_bytecode(code);

    hres = leave_script(This->ctx, hres);
    if(FAILED(hres))
        return hres;

    *ppdisp = to_disp(dispex);
    return S_OK;
}

static const IActiveScriptParseProcedure2Vtbl JScriptParseProcedureVtbl = {
    JScriptParseProcedure_QueryInterface,
    JScriptParseProcedure_AddRef,
    JScriptParseProcedure_Release,
    JScriptParseProcedure_ParseProcedureText,
};

static inline JScript *impl_from_IActiveScriptProperty(IActiveScriptProperty *iface)
{
    return CONTAINING_RECORD(iface, JScript, IActiveScriptProperty_iface);
}

static HRESULT WINAPI JScriptProperty_QueryInterface(IActiveScriptProperty *iface, REFIID riid, void **ppv)
{
    JScript *This = impl_from_IActiveScriptProperty(iface);
    return IActiveScript_QueryInterface(&This->IActiveScript_iface, riid, ppv);
}

static ULONG WINAPI JScriptProperty_AddRef(IActiveScriptProperty *iface)
{
    JScript *This = impl_from_IActiveScriptProperty(iface);
    return IActiveScript_AddRef(&This->IActiveScript_iface);
}

static ULONG WINAPI JScriptProperty_Release(IActiveScriptProperty *iface)
{
    JScript *This = impl_from_IActiveScriptProperty(iface);
    return IActiveScript_Release(&This->IActiveScript_iface);
}

static HRESULT WINAPI JScriptProperty_GetProperty(IActiveScriptProperty *iface, DWORD dwProperty,
        VARIANT *pvarIndex, VARIANT *pvarValue)
{
    JScript *This = impl_from_IActiveScriptProperty(iface);
    FIXME("(%p)->(%lx %p %p)\n", This, dwProperty, pvarIndex, pvarValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScriptProperty_SetProperty(IActiveScriptProperty *iface, DWORD dwProperty,
        VARIANT *pvarIndex, VARIANT *pvarValue)
{
    JScript *This = impl_from_IActiveScriptProperty(iface);

    TRACE("(%p)->(%lx %s %s)\n", This, dwProperty, debugstr_variant(pvarIndex), debugstr_variant(pvarValue));

    if(pvarIndex)
        FIXME("unsupported pvarIndex\n");

    switch(dwProperty) {
    case SCRIPTPROP_INVOKEVERSIONING:
        if(V_VT(pvarValue) != VT_I4 || V_I4(pvarValue) < 0
           || (V_I4(pvarValue) > 15 && !(V_I4(pvarValue) & SCRIPTLANGUAGEVERSION_HTML))) {
            WARN("invalid value %s\n", debugstr_variant(pvarValue));
            return E_INVALIDARG;
        }

        This->version = V_I4(pvarValue) & 0x1ff;
        This->html_mode = (V_I4(pvarValue) & SCRIPTLANGUAGEVERSION_HTML) != 0;
        break;
    default:
        FIXME("Unimplemented property %lx\n", dwProperty);
        return E_NOTIMPL;
    }

    return S_OK;
}

static const IActiveScriptPropertyVtbl JScriptPropertyVtbl = {
    JScriptProperty_QueryInterface,
    JScriptProperty_AddRef,
    JScriptProperty_Release,
    JScriptProperty_GetProperty,
    JScriptProperty_SetProperty
};

static inline JScript *impl_from_IObjectSafety(IObjectSafety *iface)
{
    return CONTAINING_RECORD(iface, JScript, IObjectSafety_iface);
}

static HRESULT WINAPI JScriptSafety_QueryInterface(IObjectSafety *iface, REFIID riid, void **ppv)
{
    JScript *This = impl_from_IObjectSafety(iface);
    return IActiveScript_QueryInterface(&This->IActiveScript_iface, riid, ppv);
}

static ULONG WINAPI JScriptSafety_AddRef(IObjectSafety *iface)
{
    JScript *This = impl_from_IObjectSafety(iface);
    return IActiveScript_AddRef(&This->IActiveScript_iface);
}

static ULONG WINAPI JScriptSafety_Release(IObjectSafety *iface)
{
    JScript *This = impl_from_IObjectSafety(iface);
    return IActiveScript_Release(&This->IActiveScript_iface);
}

#define SUPPORTED_OPTIONS (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER)

static HRESULT WINAPI JScriptSafety_GetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    JScript *This = impl_from_IObjectSafety(iface);

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_guid(riid), pdwSupportedOptions, pdwEnabledOptions);

    if(!pdwSupportedOptions || !pdwEnabledOptions)
        return E_POINTER;

    *pdwSupportedOptions = SUPPORTED_OPTIONS;
    *pdwEnabledOptions = This->safeopt;

    return S_OK;
}

static HRESULT WINAPI JScriptSafety_SetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
    JScript *This = impl_from_IObjectSafety(iface);

    TRACE("(%p)->(%s %lx %lx)\n", This, debugstr_guid(riid), dwOptionSetMask, dwEnabledOptions);

    if(dwOptionSetMask & ~SUPPORTED_OPTIONS)
        return E_FAIL;

    This->safeopt = (dwEnabledOptions & dwOptionSetMask) | (This->safeopt & ~dwOptionSetMask) | INTERFACE_USES_DISPEX;
    return S_OK;
}

static const IObjectSafetyVtbl JScriptSafetyVtbl = {
    JScriptSafety_QueryInterface,
    JScriptSafety_AddRef,
    JScriptSafety_Release,
    JScriptSafety_GetInterfaceSafetyOptions,
    JScriptSafety_SetInterfaceSafetyOptions
};

static inline JScript *impl_from_IVariantChangeType(IVariantChangeType *iface)
{
    return CONTAINING_RECORD(iface, JScript, IVariantChangeType_iface);
}

static HRESULT WINAPI VariantChangeType_QueryInterface(IVariantChangeType *iface, REFIID riid, void **ppv)
{
    JScript *This = impl_from_IVariantChangeType(iface);
    return IActiveScript_QueryInterface(&This->IActiveScript_iface, riid, ppv);
}

static ULONG WINAPI VariantChangeType_AddRef(IVariantChangeType *iface)
{
    JScript *This = impl_from_IVariantChangeType(iface);
    return IActiveScript_AddRef(&This->IActiveScript_iface);
}

static ULONG WINAPI VariantChangeType_Release(IVariantChangeType *iface)
{
    JScript *This = impl_from_IVariantChangeType(iface);
    return IActiveScript_Release(&This->IActiveScript_iface);
}

static HRESULT WINAPI VariantChangeType_ChangeType(IVariantChangeType *iface, VARIANT *dst, VARIANT *src, LCID lcid, VARTYPE vt)
{
    JScript *This = impl_from_IVariantChangeType(iface);
    jsexcept_t ei;
    VARIANT res;
    HRESULT hres;

    TRACE("(%p)->(%p %s %lx %s)\n", This, dst, debugstr_variant(src), lcid, debugstr_vt(vt));

    if(!This->ctx) {
        FIXME("Object uninitialized\n");
        return E_UNEXPECTED;
    }

    enter_script(This->ctx, &ei);
    hres = variant_change_type(This->ctx, &res, src, vt);
    hres = leave_script(This->ctx, hres);
    if(FAILED(hres))
        return hres;

    hres = VariantClear(dst);
    if(FAILED(hres)) {
        VariantClear(&res);
        return hres;
    }

    *dst = res;
    return S_OK;
}

static const IVariantChangeTypeVtbl VariantChangeTypeVtbl = {
    VariantChangeType_QueryInterface,
    VariantChangeType_AddRef,
    VariantChangeType_Release,
    VariantChangeType_ChangeType
};

static inline JScript *impl_from_IWineJScript(IWineJScript *iface)
{
    return CONTAINING_RECORD(iface, JScript, IWineJScript_iface);
}

static HRESULT WINAPI WineJScript_QueryInterface(IWineJScript *iface, REFIID riid, void **ppv)
{
    JScript *This = impl_from_IWineJScript(iface);
    return IActiveScript_QueryInterface(&This->IActiveScript_iface, riid, ppv);
}

static ULONG WINAPI WineJScript_AddRef(IWineJScript *iface)
{
    JScript *This = impl_from_IWineJScript(iface);
    return IActiveScript_AddRef(&This->IActiveScript_iface);
}

static ULONG WINAPI WineJScript_Release(IWineJScript *iface)
{
    JScript *This = impl_from_IWineJScript(iface);
    return IActiveScript_Release(&This->IActiveScript_iface);
}

static HRESULT WINAPI WineJScript_InitHostObject(IWineJScript *iface, IWineJSDispatchHost *host_obj,
                                                 IWineJSDispatch *prototype, UINT32 flags, IWineJSDispatch **ret)
{
    JScript *This = impl_from_IWineJScript(iface);
    return init_host_object(This->ctx, host_obj, prototype, flags, ret);
}

static HRESULT WINAPI WineJScript_InitHostConstructor(IWineJScript *iface, IWineJSDispatchHost *constr,
                                                      IWineJSDispatch *prototype, IWineJSDispatch **ret)
{
    JScript *This = impl_from_IWineJScript(iface);
    return init_host_constructor(This->ctx, constr, prototype, ret);
}

static const IWineJScriptVtbl WineJScriptVtbl = {
    WineJScript_QueryInterface,
    WineJScript_AddRef,
    WineJScript_Release,
    WineJScript_InitHostObject,
    WineJScript_InitHostConstructor,
};

HRESULT create_jscript_object(BOOL is_encode, REFIID riid, void **ppv)
{
    JScript *ret;
    HRESULT hres;

    ret = calloc(1, sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    lock_module();

    ret->IActiveScript_iface.lpVtbl = &JScriptVtbl;
    ret->IActiveScriptParse_iface.lpVtbl = &JScriptParseVtbl;
    ret->IActiveScriptParseProcedure2_iface.lpVtbl = &JScriptParseProcedureVtbl;
    ret->IActiveScriptProperty_iface.lpVtbl = &JScriptPropertyVtbl;
    ret->IObjectSafety_iface.lpVtbl = &JScriptSafetyVtbl;
    ret->IVariantChangeType_iface.lpVtbl = &VariantChangeTypeVtbl;
    ret->IWineJScript_iface.lpVtbl = &WineJScriptVtbl;
    ret->ref = 1;
    ret->safeopt = INTERFACE_USES_DISPEX;
    ret->is_encode = is_encode;
    list_init(&ret->persistent_code);
    list_init(&ret->queued_code);

    hres = IActiveScript_QueryInterface(&ret->IActiveScript_iface, riid, ppv);
    IActiveScript_Release(&ret->IActiveScript_iface);
    return hres;
}
