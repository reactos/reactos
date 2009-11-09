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
    const IActiveScriptVtbl                 *lpIActiveScriptVtbl;
    const IActiveScriptParseVtbl            *lpIActiveScriptParseVtbl;
    const IActiveScriptParseProcedure2Vtbl  *lpIActiveScriptParseProcedure2Vtbl;
    const IActiveScriptPropertyVtbl         *lpIActiveScriptPropertyVtbl;
    const IObjectSafetyVtbl                 *lpIObjectSafetyVtbl;

    LONG ref;

    DWORD safeopt;
    script_ctx_t *ctx;
    LONG thread_id;
    LCID lcid;

    IActiveScriptSite *site;

    parser_ctx_t *queue_head;
    parser_ctx_t *queue_tail;
} JScript;

#define ACTSCRIPT(x)    ((IActiveScript*) &(x)->lpIActiveScriptVtbl)
#define ASPARSE(x)      (&(x)->lpIActiveScriptParseVtbl)
#define ASPARSEPROC(x)  (&(x)->lpIActiveScriptParseProcedure2Vtbl)
#define ACTSCPPROP(x)   (&(x)->lpIActiveScriptPropertyVtbl)
#define OBJSAFETY(x)    (&(x)->lpIObjectSafetyVtbl)

void script_release(script_ctx_t *ctx)
{
    if(--ctx->ref)
        return;

    jsheap_free(&ctx->tmp_heap);
    heap_free(ctx);
}

static void change_state(JScript *This, SCRIPTSTATE state)
{
    if(This->ctx->state == state)
        return;

    This->ctx->state = state;
    IActiveScriptSite_OnStateChange(This->site, state);
}

static inline BOOL is_started(script_ctx_t *ctx)
{
    return ctx->state == SCRIPTSTATE_STARTED
        || ctx->state == SCRIPTSTATE_CONNECTED
        || ctx->state == SCRIPTSTATE_DISCONNECTED;
}

static HRESULT exec_global_code(JScript *This, parser_ctx_t *parser_ctx)
{
    exec_ctx_t *exec_ctx;
    jsexcept_t jsexcept;
    VARIANT var;
    HRESULT hres;

    hres = create_exec_ctx((IDispatch*)_IDispatchEx_(This->ctx->script_disp), This->ctx->script_disp, NULL, &exec_ctx);
    if(FAILED(hres))
        return hres;

    IActiveScriptSite_OnEnterScript(This->site);

    memset(&jsexcept, 0, sizeof(jsexcept));
    hres = exec_source(exec_ctx, parser_ctx, parser_ctx->source, &jsexcept, &var);
    VariantClear(&jsexcept.var);
    exec_release(exec_ctx);
    if(SUCCEEDED(hres))
        VariantClear(&var);

    IActiveScriptSite_OnLeaveScript(This->site);

    return hres;
}

static void clear_script_queue(JScript *This)
{
    parser_ctx_t *iter, *iter2;

    if(!This->queue_head)
        return;

    iter = This->queue_head;
    while(iter) {
        iter2 = iter->next;
        iter->next = NULL;
        parser_release(iter);
        iter = iter2;
    }

    This->queue_head = This->queue_tail = NULL;
}

static void exec_queued_code(JScript *This)
{
    parser_ctx_t *iter;

    for(iter = This->queue_head; iter; iter = iter->next)
        exec_global_code(This, iter);

    clear_script_queue(This);
}

static HRESULT set_ctx_site(JScript *This)
{
    HRESULT hres;

    This->ctx->lcid = This->lcid;

    if(!This->ctx->script_disp) {
        hres = create_dispex(This->ctx, NULL, NULL, &This->ctx->script_disp);
        if(FAILED(hres))
            return hres;
    }

    hres = init_global(This->ctx);
    if(FAILED(hres))
        return hres;

    IActiveScriptSite_AddRef(This->site);
    This->ctx->site = This->site;

    change_state(This, SCRIPTSTATE_INITIALIZED);
    return S_OK;
}

#define ACTSCRIPT_THIS(iface) DEFINE_THIS(JScript, IActiveScript, iface)

static HRESULT WINAPI JScript_QueryInterface(IActiveScript *iface, REFIID riid, void **ppv)
{
    JScript *This = ACTSCRIPT_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = ACTSCRIPT(This);
    }else if(IsEqualGUID(riid, &IID_IActiveScript)) {
        TRACE("(%p)->(IID_IActiveScript %p)\n", This, ppv);
        *ppv = ACTSCRIPT(This);
    }else if(IsEqualGUID(riid, &IID_IActiveScriptParse)) {
        TRACE("(%p)->(IID_IActiveScriptParse %p)\n", This, ppv);
        *ppv = ASPARSE(This);
    }else if(IsEqualGUID(riid, &IID_IActiveScriptParseProcedure)) {
        TRACE("(%p)->(IID_IActiveScriptParseProcedure %p)\n", This, ppv);
        *ppv = ASPARSEPROC(This);
    }else if(IsEqualGUID(riid, &IID_IActiveScriptParseProcedure2)) {
        TRACE("(%p)->(IID_IActiveScriptParseProcedure2 %p)\n", This, ppv);
        *ppv = ASPARSEPROC(This);
    }else if(IsEqualGUID(riid, &IID_IActiveScriptProperty)) {
        TRACE("(%p)->(IID_IActiveScriptProperty %p)\n", This, ppv);
        *ppv = ACTSCPPROP(This);
    }else if(IsEqualGUID(riid, &IID_IObjectSafety)) {
        TRACE("(%p)->(IID_IObjectSafety %p)\n", This, ppv);
        *ppv = OBJSAFETY(This);
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
    JScript *This = ACTSCRIPT_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI JScript_Release(IActiveScript *iface)
{
    JScript *This = ACTSCRIPT_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", iface, ref);

    if(!ref) {
        if(This->ctx && This->ctx->state != SCRIPTSTATE_CLOSED)
            IActiveScript_Close(ACTSCRIPT(This));
        if(This->ctx)
            script_release(This->ctx);
        heap_free(This);
        unlock_module();
    }

    return ref;
}

static HRESULT WINAPI JScript_SetScriptSite(IActiveScript *iface,
                                            IActiveScriptSite *pass)
{
    JScript *This = ACTSCRIPT_THIS(iface);
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
    JScript *This = ACTSCRIPT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_SetScriptState(IActiveScript *iface, SCRIPTSTATE ss)
{
    JScript *This = ACTSCRIPT_THIS(iface);

    TRACE("(%p)->(%d)\n", This, ss);

    if(!This->ctx || GetCurrentThreadId() != This->thread_id)
        return E_UNEXPECTED;

    switch(ss) {
    case SCRIPTSTATE_STARTED:
    case SCRIPTSTATE_CONNECTED: /* FIXME */
        if(This->ctx->state == SCRIPTSTATE_CLOSED)
            return E_UNEXPECTED;

        exec_queued_code(This);
        break;
    default:
        FIXME("unimplemented state %d\n", ss);
        return E_NOTIMPL;
    }

    change_state(This, ss);
    return S_OK;
}

static HRESULT WINAPI JScript_GetScriptState(IActiveScript *iface, SCRIPTSTATE *pssState)
{
    JScript *This = ACTSCRIPT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pssState);

    if(!pssState)
        return E_POINTER;

    if(!This->thread_id) {
        *pssState = SCRIPTSTATE_UNINITIALIZED;
        return S_OK;
    }

    if(This->thread_id != GetCurrentThreadId())
        return E_UNEXPECTED;

    *pssState = This->ctx ? This->ctx->state : SCRIPTSTATE_UNINITIALIZED;
    return S_OK;
}

static HRESULT WINAPI JScript_Close(IActiveScript *iface)
{
    JScript *This = ACTSCRIPT_THIS(iface);

    TRACE("(%p)->()\n", This);

    if(This->thread_id != GetCurrentThreadId())
        return E_UNEXPECTED;

    if(This->ctx) {
        if(This->ctx->state == SCRIPTSTATE_CONNECTED)
            change_state(This, SCRIPTSTATE_DISCONNECTED);

        clear_script_queue(This);

        if(This->ctx->state == SCRIPTSTATE_DISCONNECTED)
            change_state(This, SCRIPTSTATE_INITIALIZED);

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

        if(This->ctx->site) {
            IActiveScriptSite_Release(This->ctx->site);
            This->ctx->site = NULL;
        }

        if (This->site)
            change_state(This, SCRIPTSTATE_CLOSED);

        if(This->ctx->script_disp) {
            IDispatchEx_Release(_IDispatchEx_(This->ctx->script_disp));
            This->ctx->script_disp = NULL;
        }

        if(This->ctx->global) {
            IDispatchEx_Release(_IDispatchEx_(This->ctx->global));
            This->ctx->global = NULL;
        }
    }

    if(This->site) {
        IActiveScriptSite_Release(This->site);
        This->site = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI JScript_AddNamedItem(IActiveScript *iface,
                                           LPCOLESTR pstrName, DWORD dwFlags)
{
    JScript *This = ACTSCRIPT_THIS(iface);
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
    JScript *This = ACTSCRIPT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_GetScriptDispatch(IActiveScript *iface, LPCOLESTR pstrItemName,
                                                IDispatch **ppdisp)
{
    JScript *This = ACTSCRIPT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, ppdisp);

    if(!ppdisp)
        return E_POINTER;

    if(This->thread_id != GetCurrentThreadId() || !This->ctx->script_disp) {
        *ppdisp = NULL;
        return E_UNEXPECTED;
    }

    *ppdisp = (IDispatch*)_IDispatchEx_(This->ctx->script_disp);
    IDispatch_AddRef(*ppdisp);
    return S_OK;
}

static HRESULT WINAPI JScript_GetCurrentScriptThreadID(IActiveScript *iface,
                                                       SCRIPTTHREADID *pstridThread)
{
    JScript *This = ACTSCRIPT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_GetScriptThreadID(IActiveScript *iface,
                                                DWORD dwWin32ThreadId, SCRIPTTHREADID *pstidThread)
{
    JScript *This = ACTSCRIPT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_GetScriptThreadState(IActiveScript *iface,
        SCRIPTTHREADID stidThread, SCRIPTTHREADSTATE *pstsState)
{
    JScript *This = ACTSCRIPT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_InterruptScriptThread(IActiveScript *iface,
        SCRIPTTHREADID stidThread, const EXCEPINFO *pexcepinfo, DWORD dwFlags)
{
    JScript *This = ACTSCRIPT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScript_Clone(IActiveScript *iface, IActiveScript **ppscript)
{
    JScript *This = ACTSCRIPT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

#undef ACTSCRIPT_THIS

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

#define ASPARSE_THIS(iface) DEFINE_THIS(JScript, IActiveScriptParse, iface)

static HRESULT WINAPI JScriptParse_QueryInterface(IActiveScriptParse *iface, REFIID riid, void **ppv)
{
    JScript *This = ASPARSE_THIS(iface);
    return IActiveScript_QueryInterface(ACTSCRIPT(This), riid, ppv);
}

static ULONG WINAPI JScriptParse_AddRef(IActiveScriptParse *iface)
{
    JScript *This = ASPARSE_THIS(iface);
    return IActiveScript_AddRef(ACTSCRIPT(This));
}

static ULONG WINAPI JScriptParse_Release(IActiveScriptParse *iface)
{
    JScript *This = ASPARSE_THIS(iface);
    return IActiveScript_Release(ACTSCRIPT(This));
}

static HRESULT WINAPI JScriptParse_InitNew(IActiveScriptParse *iface)
{
    JScript *This = ASPARSE_THIS(iface);
    script_ctx_t *ctx;

    TRACE("(%p)\n", This);

    if(This->ctx)
        return E_UNEXPECTED;

    ctx = heap_alloc_zero(sizeof(script_ctx_t));
    if(!ctx)
        return E_OUTOFMEMORY;

    ctx->ref = 1;
    ctx->state = SCRIPTSTATE_UNINITIALIZED;
    jsheap_init(&ctx->tmp_heap);

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
    JScript *This = ASPARSE_THIS(iface);
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
    JScript *This = ASPARSE_THIS(iface);
    parser_ctx_t *parser_ctx;
    HRESULT hres;

    TRACE("(%p)->(%s %s %p %s %s %u %x %p %p)\n", This, debugstr_w(pstrCode),
          debugstr_w(pstrItemName), punkContext, debugstr_w(pstrDelimiter),
          wine_dbgstr_longlong(dwSourceContextCookie), ulStartingLine, dwFlags, pvarResult, pexcepinfo);

    if(This->thread_id != GetCurrentThreadId() || This->ctx->state == SCRIPTSTATE_CLOSED)
        return E_UNEXPECTED;

    hres = script_parse(This->ctx, pstrCode, pstrDelimiter, &parser_ctx);
    if(FAILED(hres))
        return hres;

    if(!is_started(This->ctx)) {
        if(This->queue_tail)
            This->queue_tail = This->queue_tail->next = parser_ctx;
        else
            This->queue_head = This->queue_tail = parser_ctx;
        return S_OK;
    }

    hres = exec_global_code(This, parser_ctx);
    parser_release(parser_ctx);

    return hres;
}

#undef ASPARSE_THIS

static const IActiveScriptParseVtbl JScriptParseVtbl = {
    JScriptParse_QueryInterface,
    JScriptParse_AddRef,
    JScriptParse_Release,
    JScriptParse_InitNew,
    JScriptParse_AddScriptlet,
    JScriptParse_ParseScriptText
};

#define ASPARSEPROC_THIS(iface) DEFINE_THIS(JScript, IActiveScriptParseProcedure2, iface)

static HRESULT WINAPI JScriptParseProcedure_QueryInterface(IActiveScriptParseProcedure2 *iface, REFIID riid, void **ppv)
{
    JScript *This = ASPARSEPROC_THIS(iface);
    return IActiveScript_QueryInterface(ACTSCRIPT(This), riid, ppv);
}

static ULONG WINAPI JScriptParseProcedure_AddRef(IActiveScriptParseProcedure2 *iface)
{
    JScript *This = ASPARSEPROC_THIS(iface);
    return IActiveScript_AddRef(ACTSCRIPT(This));
}

static ULONG WINAPI JScriptParseProcedure_Release(IActiveScriptParseProcedure2 *iface)
{
    JScript *This = ASPARSEPROC_THIS(iface);
    return IActiveScript_Release(ACTSCRIPT(This));
}

static HRESULT WINAPI JScriptParseProcedure_ParseProcedureText(IActiveScriptParseProcedure2 *iface,
        LPCOLESTR pstrCode, LPCOLESTR pstrFormalParams, LPCOLESTR pstrProcedureName,
        LPCOLESTR pstrItemName, IUnknown *punkContext, LPCOLESTR pstrDelimiter,
        CTXARG_T dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags, IDispatch **ppdisp)
{
    JScript *This = ASPARSEPROC_THIS(iface);
    parser_ctx_t *parser_ctx;
    DispatchEx *dispex;
    HRESULT hres;

    TRACE("(%p)->(%s %s %s %s %p %s %s %u %x %p)\n", This, debugstr_w(pstrCode), debugstr_w(pstrFormalParams),
          debugstr_w(pstrProcedureName), debugstr_w(pstrItemName), punkContext, debugstr_w(pstrDelimiter),
          wine_dbgstr_longlong(dwSourceContextCookie), ulStartingLineNumber, dwFlags, ppdisp);

    if(This->thread_id != GetCurrentThreadId() || This->ctx->state == SCRIPTSTATE_CLOSED)
        return E_UNEXPECTED;

    hres = script_parse(This->ctx, pstrCode, pstrDelimiter, &parser_ctx);
    if(FAILED(hres)) {
        WARN("Parse failed %08x\n", hres);
        return hres;
    }

    hres = create_source_function(parser_ctx, NULL, parser_ctx->source, NULL, NULL, 0, &dispex);
    parser_release(parser_ctx);
    if(FAILED(hres))
        return hres;

    *ppdisp = (IDispatch*)_IDispatchEx_(dispex);
    return S_OK;
}

#undef ASPARSEPROC_THIS

static const IActiveScriptParseProcedure2Vtbl JScriptParseProcedureVtbl = {
    JScriptParseProcedure_QueryInterface,
    JScriptParseProcedure_AddRef,
    JScriptParseProcedure_Release,
    JScriptParseProcedure_ParseProcedureText,
};

#define ACTSCPPROP_THIS(iface) DEFINE_THIS(JScript, IActiveScriptProperty, iface)

static HRESULT WINAPI JScriptProperty_QueryInterface(IActiveScriptProperty *iface, REFIID riid, void **ppv)
{
    JScript *This = ACTSCPPROP_THIS(iface);
    return IActiveScript_QueryInterface(ACTSCRIPT(This), riid, ppv);
}

static ULONG WINAPI JScriptProperty_AddRef(IActiveScriptProperty *iface)
{
    JScript *This = ACTSCPPROP_THIS(iface);
    return IActiveScript_AddRef(ACTSCRIPT(This));
}

static ULONG WINAPI JScriptProperty_Release(IActiveScriptProperty *iface)
{
    JScript *This = ACTSCPPROP_THIS(iface);
    return IActiveScript_Release(ACTSCRIPT(This));
}

static HRESULT WINAPI JScriptProperty_GetProperty(IActiveScriptProperty *iface, DWORD dwProperty,
        VARIANT *pvarIndex, VARIANT *pvarValue)
{
    JScript *This = ACTSCPPROP_THIS(iface);
    FIXME("(%p)->(%x %p %p)\n", This, dwProperty, pvarIndex, pvarValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI JScriptProperty_SetProperty(IActiveScriptProperty *iface, DWORD dwProperty,
        VARIANT *pvarIndex, VARIANT *pvarValue)
{
    JScript *This = ACTSCPPROP_THIS(iface);
    FIXME("(%p)->(%x %p %p)\n", This, dwProperty, pvarIndex, pvarValue);
    return E_NOTIMPL;
}

#undef ACTSCPPROP_THIS

static const IActiveScriptPropertyVtbl JScriptPropertyVtbl = {
    JScriptProperty_QueryInterface,
    JScriptProperty_AddRef,
    JScriptProperty_Release,
    JScriptProperty_GetProperty,
    JScriptProperty_SetProperty
};

#define OBJSAFETY_THIS(iface) DEFINE_THIS(JScript, IObjectSafety, iface)

static HRESULT WINAPI JScriptSafety_QueryInterface(IObjectSafety *iface, REFIID riid, void **ppv)
{
    JScript *This = OBJSAFETY_THIS(iface);
    return IActiveScript_QueryInterface(ACTSCRIPT(This), riid, ppv);
}

static ULONG WINAPI JScriptSafety_AddRef(IObjectSafety *iface)
{
    JScript *This = OBJSAFETY_THIS(iface);
    return IActiveScript_AddRef(ACTSCRIPT(This));
}

static ULONG WINAPI JScriptSafety_Release(IObjectSafety *iface)
{
    JScript *This = OBJSAFETY_THIS(iface);
    return IActiveScript_Release(ACTSCRIPT(This));
}

#define SUPPORTED_OPTIONS (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER)

static HRESULT WINAPI JScriptSafety_GetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    JScript *This = OBJSAFETY_THIS(iface);

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
    JScript *This = OBJSAFETY_THIS(iface);

    TRACE("(%p)->(%s %x %x)\n", This, debugstr_guid(riid), dwOptionSetMask, dwEnabledOptions);

    if(dwOptionSetMask & ~SUPPORTED_OPTIONS)
        return E_FAIL;

    This->safeopt = dwEnabledOptions & dwEnabledOptions;
    return S_OK;
}

#undef OBJSAFETY_THIS

static const IObjectSafetyVtbl JScriptSafetyVtbl = {
    JScriptSafety_QueryInterface,
    JScriptSafety_AddRef,
    JScriptSafety_Release,
    JScriptSafety_GetInterfaceSafetyOptions,
    JScriptSafety_SetInterfaceSafetyOptions
};

HRESULT WINAPI JScriptFactory_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter,
                                             REFIID riid, void **ppv)
{
    JScript *ret;
    HRESULT hres;

    TRACE("(%p %s %p)\n", pUnkOuter, debugstr_guid(riid), ppv);

    lock_module();

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->lpIActiveScriptVtbl                 = &JScriptVtbl;
    ret->lpIActiveScriptParseVtbl            = &JScriptParseVtbl;
    ret->lpIActiveScriptParseProcedure2Vtbl  = &JScriptParseProcedureVtbl;
    ret->lpIActiveScriptPropertyVtbl         = &JScriptPropertyVtbl;
    ret->lpIObjectSafetyVtbl                 = &JScriptSafetyVtbl;
    ret->ref = 1;
    ret->safeopt = INTERFACE_USES_DISPEX;

    hres = IActiveScript_QueryInterface(ACTSCRIPT(ret), riid, ppv);
    IActiveScript_Release(ACTSCRIPT(ret));
    return hres;
}
