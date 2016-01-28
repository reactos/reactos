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

    LONG ref;

    DWORD safeopt;
    script_ctx_t *ctx;
    LONG thread_id;
    LCID lcid;
    DWORD version;
    BOOL is_encode;

    IActiveScriptSite *site;

    bytecode_t *queue_head;
    bytecode_t *queue_tail;
} JScript;

void script_release(script_ctx_t *ctx)
{
    if(--ctx->ref)
        return;

    clear_ei(ctx);
    if(ctx->cc)
        release_cc(ctx->cc);
    heap_pool_free(&ctx->tmp_heap);
    if(ctx->last_match)
        jsstr_release(ctx->last_match);

    ctx->jscaller->ctx = NULL;
    IServiceProvider_Release(&ctx->jscaller->IServiceProvider_iface);

    heap_free(ctx);
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

static HRESULT exec_global_code(JScript *This, bytecode_t *code)
{
    exec_ctx_t *exec_ctx;
    HRESULT hres;

    hres = create_exec_ctx(This->ctx, NULL, This->ctx->global, NULL, TRUE, &exec_ctx);
    if(FAILED(hres))
        return hres;

    IActiveScriptSite_OnEnterScript(This->site);

    clear_ei(This->ctx);
    hres = exec_source(exec_ctx, code, &code->global_code, FALSE, NULL);
    exec_release(exec_ctx);

    IActiveScriptSite_OnLeaveScript(This->site);
    return hres;
}

static void clear_script_queue(JScript *This)
{
    bytecode_t *iter, *iter2;

    if(!This->queue_head)
        return;

    iter = This->queue_head;
    while(iter) {
        iter2 = iter->next;
        iter->next = NULL;
        release_bytecode(iter);
        iter = iter2;
    }

    This->queue_head = This->queue_tail = NULL;
}

static void exec_queued_code(JScript *This)
{
    bytecode_t *iter;

    for(iter = This->queue_head; iter; iter = iter->next)
        exec_global_code(This, iter);

    clear_script_queue(This);
}

static HRESULT set_ctx_site(JScript *This)
{
    HRESULT hres;

    This->ctx->lcid = This->lcid;

    hres = init_global(This->ctx);
    if(FAILED(hres))
        return hres;

    IActiveScriptSite_AddRef(This->site);
    This->ctx->site = This->site;

    change_state(This, SCRIPTSTATE_INITIALIZED);
    return S_OK;
}

static void decrease_state(JScript *This, SCRIPTSTATE state)
{
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
            if(This->ctx->host_global) {
                IDispatch_Release(This->ctx->host_global);
                This->ctx->host_global = NULL;
            }

            if(This->ctx->named_items) {
                named_item_t *iter, *iter2;

                iter = This->ctx->named_items;
                while(iter) {
                    iter2 = iter->next;

                    if(iter->disp)
                        IDispatch_Release(iter->disp);
                    heap_free(iter->name);
                    heap_free(iter);
                    iter = iter2;
                }

                This->ctx->named_items = NULL;
            }

            if(This->ctx->secmgr) {
                IInternetHostSecurityManager_Release(This->ctx->secmgr);
                This->ctx->secmgr = NULL;
            }

            if(This->ctx->site) {
                IActiveScriptSite_Release(This->ctx->site);
                This->ctx->site = NULL;
            }

            if(This->ctx->global) {
                jsdisp_release(This->ctx->global);
                This->ctx->global = NULL;
            }
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

    if(state == SCRIPTSTATE_UNINITIALIZED)
        This->thread_id = 0;

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

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI AXSite_Release(IServiceProvider *iface)
{
    AXSite *This = impl_from_IServiceProvider(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
    {
        if(This->sp)
            IServiceProvider_Release(This->sp);

        heap_free(This);
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
        TRACE("Could not get IServiceProvider iface: %08x\n", hres);
    }

    ret = heap_alloc(sizeof(AXSite));
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

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI JScript_Release(IActiveScript *iface)
{
    JScript *This = impl_from_IActiveScript(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", iface, ref);

    if(!ref) {
        if(This->ctx && This->ctx->state != SCRIPTSTATE_CLOSED)
            IActiveScript_Close(&This->IActiveScript_iface);
        if(This->ctx) {
            This->ctx->active_script = NULL;
            script_release(This->ctx);
        }
        heap_free(This);
        unlock_module();
    }

    return ref;
}

static HRESULT WINAPI JScript_SetScriptSite(IActiveScript *iface,
                                            IActiveScriptSite *pass)
{
    JScript *This = impl_from_IActiveScript(iface);
    LCID lcid;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pass);

    if(!pass)
        return E_POINTER;

    if(This->site)
        return E_UNEXPECTED;

    if(InterlockedCompareExchange(&This->thread_id, GetCurrentThreadId(), 0))
        return E_UNEXPECTED;

    This->site = pass;
    IActiveScriptSite_AddRef(This->site);

    hres = IActiveScriptSite_GetLCID(This->site, &lcid);
    if(hres == S_OK)
        This->lcid = lcid;

    return This->ctx ? set_ctx_site(This) : S_OK;
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

    if(This->thread_id && GetCurrentThreadId() != This->thread_id)
        return E_UNEXPECTED;

    if(ss == SCRIPTSTATE_UNINITIALIZED) {
        if(This->ctx && This->ctx->state == SCRIPTSTATE_CLOSED)
            return E_UNEXPECTED;

        decrease_state(This, SCRIPTSTATE_UNINITIALIZED);
        return S_OK;
    }

    if(!This->ctx)
        return E_UNEXPECTED;

    switch(ss) {
    case SCRIPTSTATE_STARTED:
    case SCRIPTSTATE_CONNECTED: /* FIXME */
        if(This->ctx->state == SCRIPTSTATE_CLOSED)
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

    if(This->thread_id && This->thread_id != GetCurrentThreadId())
        return E_UNEXPECTED;

    *pssState = This->ctx ? This->ctx->state : SCRIPTSTATE_UNINITIALIZED;
    return S_OK;
}

static HRESULT WINAPI JScript_Close(IActiveScript *iface)
{
    JScript *This = impl_from_IActiveScript(iface);

    TRACE("(%p)->()\n", This);

    if(This->thread_id && This->thread_id != GetCurrentThreadId())
        return E_UNEXPECTED;

    decrease_state(This, SCRIPTSTATE_CLOSED);
    return S_OK;
}

static HRESULT WINAPI JScript_AddNamedItem(IActiveScript *iface,
                                           LPCOLESTR pstrName, DWORD dwFlags)
{
    JScript *This = impl_from_IActiveScript(iface);
    named_item_t *item;
    IDispatch *disp = NULL;
    HRESULT hres;

    TRACE("(%p)->(%s %x)\n", This, debugstr_w(pstrName), dwFlags);

    if(This->thread_id != GetCurrentThreadId() || !This->ctx || This->ctx->state == SCRIPTSTATE_CLOSED)
        return E_UNEXPECTED;

    if(dwFlags & SCRIPTITEM_GLOBALMEMBERS) {
        IUnknown *unk;

        hres = IActiveScriptSite_GetItemInfo(This->site, pstrName, SCRIPTINFO_IUNKNOWN, &unk, NULL);
        if(FAILED(hres)) {
            WARN("GetItemInfo failed: %08x\n", hres);
            return hres;
        }

        hres = IUnknown_QueryInterface(unk, &IID_IDispatch, (void**)&disp);
        IUnknown_Release(unk);
        if(FAILED(hres)) {
            WARN("object does not implement IDispatch\n");
            return hres;
        }

        if(This->ctx->host_global)
            IDispatch_Release(This->ctx->host_global);
        IDispatch_AddRef(disp);
        This->ctx->host_global = disp;
    }

    item = heap_alloc(sizeof(*item));
    if(!item) {
        if(disp)
            IDispatch_Release(disp);
        return E_OUTOFMEMORY;
    }

    item->disp = disp;
    item->flags = dwFlags;
    item->name = heap_strdupW(pstrName);
    if(!item->name) {
        if(disp)
            IDispatch_Release(disp);
        heap_free(item);
        return E_OUTOFMEMORY;
    }

    item->next = This->ctx->named_items;
    This->ctx->named_items = item;

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

    TRACE("(%p)->(%p)\n", This, ppdisp);

    if(!ppdisp)
        return E_POINTER;

    if(This->thread_id != GetCurrentThreadId() || !This->ctx->global) {
        *ppdisp = NULL;
        return E_UNEXPECTED;
    }

    *ppdisp = to_disp(This->ctx->global);
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
    script_ctx_t *ctx;
    HRESULT hres;

    TRACE("(%p)\n", This);

    if(This->ctx)
        return E_UNEXPECTED;

    ctx = heap_alloc_zero(sizeof(script_ctx_t));
    if(!ctx)
        return E_OUTOFMEMORY;

    ctx->ref = 1;
    ctx->state = SCRIPTSTATE_UNINITIALIZED;
    ctx->active_script = &This->IActiveScript_iface;
    ctx->safeopt = This->safeopt;
    ctx->version = This->version;
    ctx->ei.val = jsval_undefined();
    heap_pool_init(&ctx->tmp_heap);

    hres = create_jscaller(ctx);
    if(FAILED(hres)) {
        heap_free(ctx);
        return hres;
    }

    ctx->last_match = jsstr_empty();

    ctx = InterlockedCompareExchangePointer((void**)&This->ctx, ctx, NULL);
    if(ctx) {
        script_release(ctx);
        return E_UNEXPECTED;
    }

    return This->site ? set_ctx_site(This) : S_OK;
}

static HRESULT WINAPI JScriptParse_AddScriptlet(IActiveScriptParse *iface,
        LPCOLESTR pstrDefaultName, LPCOLESTR pstrCode, LPCOLESTR pstrItemName,
        LPCOLESTR pstrSubItemName, LPCOLESTR pstrEventName, LPCOLESTR pstrDelimiter,
        CTXARG_T dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags,
        BSTR *pbstrName, EXCEPINFO *pexcepinfo)
{
    JScript *This = impl_from_IActiveScriptParse(iface);
    FIXME("(%p)->(%s %s %s %s %s %s %s %u %x %p %p)\n", This, debugstr_w(pstrDefaultName),
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
    bytecode_t *code;
    HRESULT hres;

    TRACE("(%p)->(%s %s %p %s %s %u %x %p %p)\n", This, debugstr_w(pstrCode),
          debugstr_w(pstrItemName), punkContext, debugstr_w(pstrDelimiter),
          wine_dbgstr_longlong(dwSourceContextCookie), ulStartingLine, dwFlags, pvarResult, pexcepinfo);

    if(This->thread_id != GetCurrentThreadId() || This->ctx->state == SCRIPTSTATE_CLOSED)
        return E_UNEXPECTED;

    hres = compile_script(This->ctx, pstrCode, NULL, pstrDelimiter, (dwFlags & SCRIPTTEXT_ISEXPRESSION) != 0,
            This->is_encode, &code);
    if(FAILED(hres))
        return hres;

    if(dwFlags & SCRIPTTEXT_ISEXPRESSION) {
        exec_ctx_t *exec_ctx;

        hres = create_exec_ctx(This->ctx, NULL, This->ctx->global, NULL, TRUE, &exec_ctx);
        if(SUCCEEDED(hres)) {
            jsval_t r;

            IActiveScriptSite_OnEnterScript(This->site);

            clear_ei(This->ctx);
            hres = exec_source(exec_ctx, code, &code->global_code, TRUE, &r);
            if(SUCCEEDED(hres)) {
                if(pvarResult)
                    hres = jsval_to_variant(r, pvarResult);
                jsval_release(r);
            }
            exec_release(exec_ctx);

            IActiveScriptSite_OnLeaveScript(This->site);
        }

        return hres;
    }

    /*
     * Although pvarResult is not really used without SCRIPTTEXT_ISEXPRESSION flag, if it's not NULL,
     * script is executed immediately, even if it's not in started state yet.
     */
    if(!pvarResult && !is_started(This->ctx)) {
        if(This->queue_tail)
            This->queue_tail = This->queue_tail->next = code;
        else
            This->queue_head = This->queue_tail = code;
        return S_OK;
    }

    hres = exec_global_code(This, code);
    release_bytecode(code);
    if(FAILED(hres))
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
    bytecode_t *code;
    jsdisp_t *dispex;
    HRESULT hres;

    TRACE("(%p)->(%s %s %s %s %p %s %s %u %x %p)\n", This, debugstr_w(pstrCode), debugstr_w(pstrFormalParams),
          debugstr_w(pstrProcedureName), debugstr_w(pstrItemName), punkContext, debugstr_w(pstrDelimiter),
          wine_dbgstr_longlong(dwSourceContextCookie), ulStartingLineNumber, dwFlags, ppdisp);

    if(This->thread_id != GetCurrentThreadId() || This->ctx->state == SCRIPTSTATE_CLOSED)
        return E_UNEXPECTED;

    hres = compile_script(This->ctx, pstrCode, pstrFormalParams, pstrDelimiter, FALSE, This->is_encode, &code);
    if(FAILED(hres)) {
        WARN("Parse failed %08x\n", hres);
        return hres;
    }

    hres = create_source_function(This->ctx, code, &code->global_code, NULL,  &dispex);
    release_bytecode(code);
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
    FIXME("(%p)->(%x %p %p)\n", This, dwProperty, pvarIndex, pvarValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScriptProperty_SetProperty(IActiveScriptProperty *iface, DWORD dwProperty,
        VARIANT *pvarIndex, VARIANT *pvarValue)
{
    JScript *This = impl_from_IActiveScriptProperty(iface);

    TRACE("(%p)->(%x %s %s)\n", This, dwProperty, debugstr_variant(pvarIndex), debugstr_variant(pvarValue));

    if(pvarIndex)
        FIXME("unsupported pvarIndex\n");

    switch(dwProperty) {
    case SCRIPTPROP_INVOKEVERSIONING:
        if(V_VT(pvarValue) != VT_I4 || V_I4(pvarValue) < 0 || V_I4(pvarValue) > 15) {
            WARN("invalid value %s\n", debugstr_variant(pvarValue));
            return E_INVALIDARG;
        }

        This->version = V_I4(pvarValue);
        break;
    default:
        FIXME("Unimplemented property %x\n", dwProperty);
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

    TRACE("(%p)->(%s %x %x)\n", This, debugstr_guid(riid), dwOptionSetMask, dwEnabledOptions);

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
    VARIANT res;
    HRESULT hres;

    TRACE("(%p)->(%p %p%s %x %d)\n", This, dst, src, debugstr_variant(src), lcid, vt);

    if(!This->ctx) {
        FIXME("Object uninitialized\n");
        return E_UNEXPECTED;
    }

    hres = variant_change_type(This->ctx, &res, src, vt);
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

HRESULT create_jscript_object(BOOL is_encode, REFIID riid, void **ppv)
{
    JScript *ret;
    HRESULT hres;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    lock_module();

    ret->IActiveScript_iface.lpVtbl = &JScriptVtbl;
    ret->IActiveScriptParse_iface.lpVtbl = &JScriptParseVtbl;
    ret->IActiveScriptParseProcedure2_iface.lpVtbl = &JScriptParseProcedureVtbl;
    ret->IActiveScriptProperty_iface.lpVtbl = &JScriptPropertyVtbl;
    ret->IObjectSafety_iface.lpVtbl = &JScriptSafetyVtbl;
    ret->IVariantChangeType_iface.lpVtbl = &VariantChangeTypeVtbl;
    ret->ref = 1;
    ret->safeopt = INTERFACE_USES_DISPEX;
    ret->is_encode = is_encode;

    hres = IActiveScript_QueryInterface(&ret->IActiveScript_iface, riid, ppv);
    IActiveScript_Release(&ret->IActiveScript_iface);
    return hres;
}
