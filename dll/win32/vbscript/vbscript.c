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
} VBScriptError;

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
    code->pending_exec = FALSE;
    return exec_script(ctx, TRUE, &code->main_code, NULL, NULL, res);
}

static void exec_queued_code(script_ctx_t *ctx)
{
    vbscode_t *iter;

    LIST_FOR_EACH_ENTRY(iter, &ctx->code_list, vbscode_t, entry) {
        if(iter->pending_exec)
            exec_global_code(ctx, iter, NULL);
    }
}

IDispatch *lookup_named_item(script_ctx_t *ctx, const WCHAR *name, unsigned flags)
{
    named_item_t *item;
    HRESULT hres;

    LIST_FOR_EACH_ENTRY(item, &ctx->named_items, named_item_t, entry) {
        if((item->flags & flags) == flags && !wcsicmp(item->name, name)) {
            if(!item->disp) {
                IUnknown *unk;

                hres = IActiveScriptSite_GetItemInfo(ctx->site, item->name,
                                                     SCRIPTINFO_IUNKNOWN, &unk, NULL);
                if(FAILED(hres)) {
                    WARN("GetItemInfo failed: %08x\n", hres);
                    continue;
                }

                hres = IUnknown_QueryInterface(unk, &IID_IDispatch, (void**)&item->disp);
                IUnknown_Release(unk);
                if(FAILED(hres)) {
                    WARN("object does not implement IDispatch\n");
                    continue;
                }
            }

            return item->disp;
        }
    }

    return NULL;
}

static void release_script(script_ctx_t *ctx)
{
    class_desc_t *class_desc;

    collect_objects(ctx);
    clear_ei(&ctx->ei);

    release_dynamic_vars(ctx->global_vars);
    ctx->global_vars = NULL;

    while(!list_empty(&ctx->named_items)) {
        named_item_t *iter = LIST_ENTRY(list_head(&ctx->named_items), named_item_t, entry);

        list_remove(&iter->entry);
        if(iter->disp)
            IDispatch_Release(iter->disp);
        heap_free(iter->name);
        heap_free(iter);
    }

    while(ctx->procs) {
        class_desc = ctx->procs;
        ctx->procs = class_desc->next;

        heap_free(class_desc);
    }

    if(ctx->host_global) {
        IDispatch_Release(ctx->host_global);
        ctx->host_global = NULL;
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

    detach_global_objects(ctx);
    heap_pool_free(&ctx->heap);
    heap_pool_init(&ctx->heap);
}

static void destroy_script(script_ctx_t *ctx)
{
    while(!list_empty(&ctx->code_list))
        release_vbscode(LIST_ENTRY(list_head(&ctx->code_list), vbscode_t, entry));

    release_script(ctx);
    heap_free(ctx);
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
        if(This->state == SCRIPTSTATE_DISCONNECTED)
            change_state(This, SCRIPTSTATE_INITIALIZED);
        if(state == SCRIPTSTATE_INITIALIZED)
            break;
        /* FALLTHROUGH */
    case SCRIPTSTATE_INITIALIZED:
    case SCRIPTSTATE_UNINITIALIZED:
        change_state(This, state);
        release_script(This->ctx);
        This->thread_id = 0;
        break;
    case SCRIPTSTATE_CLOSED:
        break;
    DEFAULT_UNREACHABLE;
    }
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

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI VBScriptError_Release(IActiveScriptError *iface)
{
    VBScriptError *This = impl_from_IActiveScriptError(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        heap_free(This);
    }

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

    FIXME("(%p)->(%p %p %p)\n", This, source_context, line, character);

    if(source_context)
        *source_context = 0;
    if(line)
        *line = 0;
    if(character)
        *character = 0;
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

HRESULT report_script_error(script_ctx_t *ctx)
{
    VBScriptError *error;
    HRESULT hres, result;

    if(!(error = heap_alloc(sizeof(*error))))
        return E_OUTOFMEMORY;
    error->IActiveScriptError_iface.lpVtbl = &VBScriptErrorVtbl;

    error->ref = 1;
    error->ei = ctx->ei;
    memset(&ctx->ei, 0, sizeof(ctx->ei));
    result = error->ei.scode;

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
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
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

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI VBScript_Release(IActiveScript *iface)
{
    VBScript *This = impl_from_IActiveScript(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", iface, ref);

    if(!ref) {
        decrease_state(This, SCRIPTSTATE_CLOSED);
        destroy_script(This->ctx);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI VBScript_SetScriptSite(IActiveScript *iface, IActiveScriptSite *pass)
{
    VBScript *This = impl_from_IActiveScript(iface);
    LCID lcid;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pass);

    if(!pass)
        return E_POINTER;

    if(This->ctx->site)
        return E_UNEXPECTED;

    if(InterlockedCompareExchange(&This->thread_id, GetCurrentThreadId(), 0))
        return E_UNEXPECTED;

    hres = create_script_disp(This->ctx, &This->ctx->script_obj);
    if(FAILED(hres))
        return hres;

    This->ctx->site = pass;
    IActiveScriptSite_AddRef(This->ctx->site);

    hres = IActiveScriptSite_GetLCID(This->ctx->site, &lcid);
    if(hres == S_OK)
        This->ctx->lcid = lcid;

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

    if(!This->is_initialized)
        return E_UNEXPECTED;

    switch(ss) {
    case SCRIPTSTATE_STARTED:
    case SCRIPTSTATE_CONNECTED: /* FIXME */
        if(This->state == SCRIPTSTATE_CLOSED)
            return E_UNEXPECTED;

        exec_queued_code(This->ctx);
        break;
    case SCRIPTSTATE_INITIALIZED:
        FIXME("unimplemented SCRIPTSTATE_INITIALIZED\n");
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

    TRACE("(%p)->(%s %x)\n", This, debugstr_w(pstrName), dwFlags);

    if(This->thread_id != GetCurrentThreadId() || !This->ctx->site)
        return E_UNEXPECTED;

    if(dwFlags & SCRIPTITEM_GLOBALMEMBERS) {
        IUnknown *unk;

        hres = IActiveScriptSite_GetItemInfo(This->ctx->site, pstrName, SCRIPTINFO_IUNKNOWN, &unk, NULL);
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

    list_add_tail(&This->ctx->named_items, &item->entry);
    return S_OK;
}

static HRESULT WINAPI VBScript_AddTypeLib(IActiveScript *iface, REFGUID rguidTypeLib,
        DWORD dwMajor, DWORD dwMinor, DWORD dwFlags)
{
    VBScript *This = impl_from_IActiveScript(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI VBScript_GetScriptDispatch(IActiveScript *iface, LPCOLESTR pstrItemName, IDispatch **ppdisp)
{
    VBScript *This = impl_from_IActiveScript(iface);

    TRACE("(%p)->(%p)\n", This, ppdisp);

    if(!ppdisp)
        return E_POINTER;

    if(This->thread_id != GetCurrentThreadId() || !This->ctx->script_obj) {
        *ppdisp = NULL;
        return E_UNEXPECTED;
    }

    *ppdisp = (IDispatch*)&This->ctx->script_obj->IDispatchEx_iface;
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
    FIXME("(%p)->(%s %u %s %#x %p)\n", This, debugstr_w(code), len,
          debugstr_w(delimiter), flags, attr);
    return E_NOTIMPL;
}

static HRESULT WINAPI VBScriptDebug_GetScriptletTextAttributes(IActiveScriptDebug *iface,
        LPCOLESTR code, ULONG len, LPCOLESTR delimiter, DWORD flags, SOURCE_TEXT_ATTR *attr)
{
    VBScript *This = impl_from_IActiveScriptDebug(iface);
    FIXME("(%p)->(%s %u %s %#x %p)\n", This, debugstr_w(code), len,
          debugstr_w(delimiter), flags, attr);
    return E_NOTIMPL;
}

static HRESULT WINAPI VBScriptDebug_EnumCodeContextsOfPosition(IActiveScriptDebug *iface,
        CTXARG_T source, ULONG offset, ULONG len, IEnumDebugCodeContexts **ret)
{
    VBScript *This = impl_from_IActiveScriptDebug(iface);
    FIXME("(%p)->(%s %u %u %p)\n", This, wine_dbgstr_longlong(source), offset, len, ret);
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
    FIXME("(%p)->(%s %s %s %s %s %s %s %u %x %p %p)\n", This, debugstr_w(pstrDefaultName),
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
    IDispatch *context = NULL;
    vbscode_t *code;
    HRESULT hres;

    TRACE("(%p)->(%s %s %p %s %s %u %x %p %p)\n", This, debugstr_w(pstrCode),
          debugstr_w(pstrItemName), punkContext, debugstr_w(pstrDelimiter),
          wine_dbgstr_longlong(dwSourceContextCookie), ulStartingLine, dwFlags, pvarResult, pexcepinfo);

    if(This->thread_id != GetCurrentThreadId() || This->state == SCRIPTSTATE_CLOSED)
        return E_UNEXPECTED;

    if(pstrItemName) {
        context = lookup_named_item(This->ctx, pstrItemName, 0);
        if(!context) {
            WARN("Inknown context %s\n", debugstr_w(pstrItemName));
            return E_INVALIDARG;
        }
    }

    hres = compile_script(This->ctx, pstrCode, pstrDelimiter, dwFlags, &code);
    if(FAILED(hres))
        return hres;

    if(context)
        IDispatch_AddRef(code->context = context);

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

    TRACE("(%p)->(%s %s %s %s %p %s %s %u %x %p)\n", This, debugstr_w(pstrCode), debugstr_w(pstrFormalParams),
          debugstr_w(pstrProcedureName), debugstr_w(pstrItemName), punkContext, debugstr_w(pstrDelimiter),
          wine_dbgstr_longlong(dwSourceContextCookie), ulStartingLineNumber, dwFlags, ppdisp);

    if(This->thread_id != GetCurrentThreadId() || This->state == SCRIPTSTATE_CLOSED)
        return E_UNEXPECTED;

    hres = compile_procedure(This->ctx, pstrCode, pstrDelimiter, dwFlags, &desc);
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

    TRACE("(%p)->(%s %x %x)\n", This, debugstr_guid(riid), dwOptionSetMask, dwEnabledOptions);

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
    script_ctx_t *ctx;
    VBScript *ret;
    HRESULT hres;

    TRACE("(%p %s %p)\n", pUnkOuter, debugstr_guid(riid), ppv);

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IActiveScript_iface.lpVtbl = &VBScriptVtbl;
    ret->IActiveScriptDebug_iface.lpVtbl = &VBScriptDebugVtbl;
    ret->IActiveScriptParse_iface.lpVtbl = &VBScriptParseVtbl;
    ret->IActiveScriptParseProcedure2_iface.lpVtbl = &VBScriptParseProcedureVtbl;
    ret->IObjectSafety_iface.lpVtbl = &VBScriptSafetyVtbl;

    ret->ref = 1;
    ret->state = SCRIPTSTATE_UNINITIALIZED;

    ctx = ret->ctx = heap_alloc_zero(sizeof(*ctx));
    if(!ctx) {
        heap_free(ret);
        return E_OUTOFMEMORY;
    }

    ctx->safeopt = INTERFACE_USES_DISPEX;
    heap_pool_init(&ctx->heap);
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

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI AXSite_Release(IServiceProvider *iface)
{
    AXSite *This = impl_from_IServiceProvider(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

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
        ERR("Could not get IServiceProvider iface: %08x\n", hres);
        return NULL;
    }

    ret = heap_alloc(sizeof(*ret));
    if(!ret) {
        IServiceProvider_Release(sp);
        return NULL;
    }

    ret->IServiceProvider_iface.lpVtbl = &AXSiteVtbl;
    ret->ref = 1;
    ret->sp = sp;

    return (IUnknown*)&ret->IServiceProvider_iface;
}
