/*
 * Copyright 2005-2006 Jacek Caban for CodeWeavers
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

#include "ieframe.h"

#include "exdispid.h"
#include "mshtml.h"
#include "perhist.h"
#include "initguid.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ieframe);

DEFINE_OLEGUID(CGID_DocHostCmdPriv, 0x000214D4L, 0, 0);

#define DOCHOST_DOCCANNAVIGATE  0

/* Undocumented notification, see mshtml tests */
#define CMDID_EXPLORER_UPDATEHISTORY 38

static ATOM doc_view_atom = 0;

void push_dochost_task(DocHost *This, task_header_t *task, task_proc_t proc, task_destr_t destr, BOOL send)
{
    BOOL is_empty;

    task->proc = proc;
    task->destr = destr;

    is_empty = list_empty(&This->task_queue);
    list_add_tail(&This->task_queue, &task->entry);

    if(send)
        SendMessageW(This->frame_hwnd, WM_DOCHOSTTASK, 0, 0);
    else if(is_empty)
        PostMessageW(This->frame_hwnd, WM_DOCHOSTTASK, 0, 0);
}

LRESULT process_dochost_tasks(DocHost *This)
{
    task_header_t *task;

    while(!list_empty(&This->task_queue)) {
        task = LIST_ENTRY(This->task_queue.next, task_header_t, entry);
        list_remove(&task->entry);

        task->proc(This, task);
        task->destr(task);
    }

    return 0;
}

void abort_dochost_tasks(DocHost *This, task_proc_t proc)
{
    task_header_t *task, *cursor;

    LIST_FOR_EACH_ENTRY_SAFE(task, cursor, &This->task_queue, task_header_t, entry) {
        if(proc && proc != task->proc)
            continue;

        list_remove(&task->entry);
        task->destr(task);
    }
}

void on_commandstate_change(DocHost *doc_host, LONG command, BOOL enable)
{
    DISPPARAMS dispparams;
    VARIANTARG params[2];

    TRACE("command=%ld enable=%d\n", command, enable);

    dispparams.cArgs = 2;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = params;

    V_VT(params) = VT_BOOL;
    V_BOOL(params) = enable ? VARIANT_TRUE : VARIANT_FALSE;

    V_VT(params+1) = VT_I4;
    V_I4(params+1) = command;

    call_sink(doc_host->cps.wbe2, DISPID_COMMANDSTATECHANGE, &dispparams);

    doc_host->container_vtbl->on_command_state_change(doc_host, command, enable);
}

void update_navigation_commands(DocHost *dochost)
{
    unsigned pos = dochost->travellog.loading_pos == -1 ? dochost->travellog.position : dochost->travellog.loading_pos;

    on_commandstate_change(dochost, CSC_NAVIGATEBACK, pos > 0);
    on_commandstate_change(dochost, CSC_NAVIGATEFORWARD, pos < dochost->travellog.length);
}

static void notif_complete(DocHost *This, DISPID dispid)
{
    DISPPARAMS dispparams;
    VARIANTARG params[2];
    VARIANT url;

    dispparams.cArgs = 2;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = params;

    V_VT(params) = (VT_BYREF|VT_VARIANT);
    V_BYREF(params) = &url;

    V_VT(params+1) = VT_DISPATCH;
    V_DISPATCH(params+1) = (IDispatch*)This->wb;

    V_VT(&url) = VT_BSTR;
    V_BSTR(&url) = SysAllocString(This->url);

    TRACE("%ld >>>\n", dispid);
    call_sink(This->cps.wbe2, dispid, &dispparams);
    TRACE("%ld <<<\n", dispid);

    SysFreeString(V_BSTR(&url));
}

static void object_available(DocHost *This)
{
    IHlinkTarget *hlink;
    HRESULT hres;

    TRACE("(%p)\n", This);

    if(!This->document) {
        WARN("document == NULL\n");
        return;
    }

    hres = IUnknown_QueryInterface(This->document, &IID_IHlinkTarget, (void**)&hlink);
    if(SUCCEEDED(hres)) {
        hres = IHlinkTarget_Navigate(hlink, 0, NULL);
        IHlinkTarget_Release(hlink);
        if(FAILED(hres))
            FIXME("Navigate failed\n");
    }else {
        IOleObject *ole_object;
        RECT rect;

        TRACE("No IHlink iface\n");

        hres = IUnknown_QueryInterface(This->document, &IID_IOleObject, (void**)&ole_object);
        if(FAILED(hres)) {
            FIXME("Could not get IOleObject iface: %08lx\n", hres);
            return;
        }

        GetClientRect(This->hwnd, &rect);
        hres = IOleObject_DoVerb(ole_object, OLEIVERB_SHOW, NULL, &This->IOleClientSite_iface, -1, This->hwnd, &rect);
        IOleObject_Release(ole_object);
        if(FAILED(hres))
            FIXME("DoVerb failed: %08lx\n", hres);
    }
}

static HRESULT get_doc_ready_state(DocHost *This, READYSTATE *ret)
{
    DISPPARAMS dp = {NULL,NULL,0,0};
    IDispatch *disp;
    EXCEPINFO ei;
    VARIANT var;
    HRESULT hres;

    hres = IUnknown_QueryInterface(This->document, &IID_IDispatch, (void**)&disp);
    if(FAILED(hres))
        return hres;

    hres = IDispatch_Invoke(disp, DISPID_READYSTATE, &IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET,
            &dp, &var, &ei, NULL);
    IDispatch_Release(disp);
    if(FAILED(hres)) {
        WARN("Invoke(DISPID_READYSTATE failed: %08lx\n", hres);
        return hres;
    }

    if(V_VT(&var) != VT_I4) {
        WARN("V_VT(var) = %d\n", V_VT(&var));
        VariantClear(&var);
        return E_FAIL;
    }

    *ret = V_I4(&var);
    return S_OK;
}

static void advise_prop_notif(DocHost *This, BOOL set)
{
    IConnectionPointContainer *cp_container;
    IConnectionPoint *cp;
    HRESULT hres;

    hres = IUnknown_QueryInterface(This->document, &IID_IConnectionPointContainer, (void**)&cp_container);
    if(FAILED(hres))
        return;

    hres = IConnectionPointContainer_FindConnectionPoint(cp_container, &IID_IPropertyNotifySink, &cp);
    IConnectionPointContainer_Release(cp_container);
    if(FAILED(hres))
        return;

    if(set)
        hres = IConnectionPoint_Advise(cp, (IUnknown*)&This->IPropertyNotifySink_iface, &This->prop_notif_cookie);
    else
        hres = IConnectionPoint_Unadvise(cp, This->prop_notif_cookie);
    IConnectionPoint_Release(cp);

    if(SUCCEEDED(hres))
        This->is_prop_notif = set;
}

void set_doc_state(DocHost *This, READYSTATE doc_state)
{
    This->doc_state = doc_state;
    if(doc_state > This->ready_state)
        This->ready_state = doc_state;
}

static void update_ready_state(DocHost *This, READYSTATE ready_state)
{
    if(ready_state > READYSTATE_LOADING && This->travellog.loading_pos != -1) {
        WARN("histupdate not notified\n");
        This->travellog.position = This->travellog.loading_pos;
        This->travellog.loading_pos = -1;
    }

    if(ready_state > READYSTATE_LOADING && This->doc_state <= READYSTATE_LOADING && !This->browser_service /* FIXME */)
        notif_complete(This, DISPID_NAVIGATECOMPLETE2);

    if(ready_state == READYSTATE_COMPLETE && This->doc_state < READYSTATE_COMPLETE) {
        set_doc_state(This, READYSTATE_COMPLETE);
        if(!This->browser_service) /* FIXME: Not fully correct */
            notif_complete(This, DISPID_DOCUMENTCOMPLETE);
    }else {
        set_doc_state(This, ready_state);
    }
}

typedef struct {
    task_header_t header;
    IUnknown *doc;
    READYSTATE ready_state;
} ready_state_task_t;

static void ready_state_task_destr(task_header_t *_task)
{
    ready_state_task_t *task = (ready_state_task_t*)_task;

    IUnknown_Release(task->doc);
    free(task);
}

static void ready_state_proc(DocHost *This, task_header_t *_task)
{
    ready_state_task_t *task = (ready_state_task_t*)_task;

    if(task->doc == This->document)
        update_ready_state(This, task->ready_state);
}

static void push_ready_state_task(DocHost *This, READYSTATE ready_state)
{
    ready_state_task_t *task = malloc(sizeof(ready_state_task_t));

    IUnknown_AddRef(This->document);
    task->doc = This->document;
    task->ready_state = ready_state;

    push_dochost_task(This, &task->header, ready_state_proc, ready_state_task_destr, FALSE);
}

static void object_available_task_destr(task_header_t *task)
{
    free(task);
}

static void object_available_proc(DocHost *This, task_header_t *task)
{
    object_available(This);
}

HRESULT dochost_object_available(DocHost *This, IUnknown *doc)
{
    READYSTATE ready_state;
    task_header_t *task;
    IOleObject *oleobj;
    HRESULT hres;

    IUnknown_AddRef(doc);
    This->document = doc;

    hres = IUnknown_QueryInterface(doc, &IID_IOleObject, (void**)&oleobj);
    if(SUCCEEDED(hres)) {
        CLSID clsid;

        hres = IOleObject_GetUserClassID(oleobj, &clsid);
        if(SUCCEEDED(hres))
            TRACE("Got clsid %s\n",
                  IsEqualGUID(&clsid, &CLSID_HTMLDocument) ? "CLSID_HTMLDocument" : debugstr_guid(&clsid));

        hres = IOleObject_SetClientSite(oleobj, &This->IOleClientSite_iface);
        if(FAILED(hres))
            FIXME("SetClientSite failed: %08lx\n", hres);

        IOleObject_Release(oleobj);
    }else {
        FIXME("Could not get IOleObject iface: %08lx\n", hres);
    }

    /* FIXME: Call SetAdvise */

    task = malloc(sizeof(*task));
    push_dochost_task(This, task, object_available_proc, object_available_task_destr, FALSE);

    hres = get_doc_ready_state(This, &ready_state);
    if(SUCCEEDED(hres)) {
        if(ready_state == READYSTATE_COMPLETE)
            push_ready_state_task(This, READYSTATE_COMPLETE);
        if(ready_state != READYSTATE_COMPLETE || This->doc_navigate)
            advise_prop_notif(This, TRUE);
    }else if(!This->doc_navigate) {
        /* If we can't get document's ready state, there is not much we can do.
         * Assume that document is complete at this point. */
        push_ready_state_task(This, READYSTATE_COMPLETE);
    }

    return S_OK;
}

static LRESULT resize_document(DocHost *This, LONG width, LONG height)
{
    RECT rect = {0, 0, width, height};

    TRACE("(%p)->(%ld %ld)\n", This, width, height);

    if(This->view)
        IOleDocumentView_SetRect(This->view, &rect);

    return 0;
}

static LRESULT WINAPI doc_view_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DocHost *This;

    static const WCHAR wszTHIS[] = {'T','H','I','S',0};

    if(msg == WM_CREATE) {
        This = *(DocHost**)lParam;
        SetPropW(hwnd, wszTHIS, This);
    }else {
        This = GetPropW(hwnd, wszTHIS);
    }

    switch(msg) {
    case WM_SIZE:
        return resize_document(This, LOWORD(lParam), HIWORD(lParam));
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void free_travellog_entry(travellog_entry_t *entry)
{
    if(entry->stream) {
        IStream_Release(entry->stream);
        entry->stream = NULL;
    }
    free(entry->url);
    entry->url = NULL;
}

static IStream *get_travellog_stream(DocHost *This)
{
    IPersistHistory *persist_history;
    IStream *stream;
    HRESULT hres;

    hres = IUnknown_QueryInterface(This->document, &IID_IPersistHistory, (void**)&persist_history);
    if(FAILED(hres))
        return NULL;

    hres = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    if(SUCCEEDED(hres))
        hres = IPersistHistory_SaveHistory(persist_history, stream);
    IPersistHistory_Release(persist_history);
    if(FAILED(hres)) {
        IStream_Release(stream);
        return NULL;
    }

    return stream;
}

static void dump_travellog(DocHost *This)
{
    unsigned i;

    for(i=0; i < This->travellog.length; i++)
        TRACE("%d: %s %s\n", i, i == This->travellog.position ? "=>" : "  ", debugstr_w(This->travellog.log[i].url));
    if(i == This->travellog.position)
        TRACE("%d: =>\n", i);
}

static void update_travellog(DocHost *This)
{
    travellog_entry_t *new_entry;

    static const WCHAR about_schemeW[] = {'a','b','o','u','t',':'};

    if(This->url && !wcsnicmp(This->url, about_schemeW, ARRAY_SIZE(about_schemeW))) {
        TRACE("Skipping about URL\n");
        return;
    }

    if(!This->travellog.log) {
        This->travellog.log = malloc(4 * sizeof(*This->travellog.log));
        if(!This->travellog.log)
            return;

        This->travellog.size = 4;
    }else if(This->travellog.size < This->travellog.position+1) {
        travellog_entry_t *new_travellog;

        new_travellog = realloc(This->travellog.log, This->travellog.size * 2 * sizeof(*This->travellog.log));
        if(!new_travellog)
            return;

        This->travellog.log = new_travellog;
        This->travellog.size *= 2;
    }

    if(This->travellog.loading_pos == -1) {
        /* Clear forward history. */
        while(This->travellog.length > This->travellog.position)
            free_travellog_entry(This->travellog.log + --This->travellog.length);
    }

    new_entry = This->travellog.log + This->travellog.position;

    new_entry->url = wcsdup(This->url);
    TRACE("Adding %s at %d\n", debugstr_w(This->url), This->travellog.position);
    if(!new_entry->url)
        return;

    new_entry->stream = get_travellog_stream(This);

    if(This->travellog.loading_pos == -1) {
        This->travellog.position++;
        This->travellog.log[This->travellog.position].stream = NULL;
        This->travellog.log[This->travellog.position].url = NULL;
    }else {
         This->travellog.position = This->travellog.loading_pos;
         This->travellog.loading_pos = -1;
    }
    if(This->travellog.position > This->travellog.length)
        This->travellog.length = This->travellog.position;

    dump_travellog(This);
}

void create_doc_view_hwnd(DocHost *This)
{
    RECT rect;

    static const WCHAR wszShell_DocObject_View[] =
        {'S','h','e','l','l',' ','D','o','c','O','b','j','e','c','t',' ','V','i','e','w',0};

    if(!doc_view_atom) {
        static WNDCLASSEXW wndclass = {
            sizeof(wndclass),
            CS_PARENTDC,
            doc_view_proc,
            0, 0 /* native uses 4*/, NULL, NULL, NULL,
            (HBRUSH)(COLOR_WINDOW + 1), NULL,
            wszShell_DocObject_View,
            NULL
        };

        wndclass.hInstance = ieframe_instance;

        doc_view_atom = RegisterClassExW(&wndclass);
    }

    This->container_vtbl->get_docobj_rect(This, &rect);
    This->hwnd = CreateWindowExW(0, wszShell_DocObject_View,
         wszShell_DocObject_View,
         WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP,
         rect.left, rect.top, rect.right, rect.bottom, This->frame_hwnd,
         NULL, ieframe_instance, This);
}

void deactivate_document(DocHost *This)
{
    IOleInPlaceObjectWindowless *winobj;
    IOleObject *oleobj = NULL;
    IHlinkTarget *hlink = NULL;
    HRESULT hres;

    if(!This->document) return;

    if(This->doc_navigate) {
        IUnknown_Release(This->doc_navigate);
        This->doc_navigate = NULL;
    }

    if(This->is_prop_notif)
        advise_prop_notif(This, FALSE);

    if(This->view)
        IOleDocumentView_UIActivate(This->view, FALSE);

    hres = IUnknown_QueryInterface(This->document, &IID_IOleInPlaceObjectWindowless,
                                   (void**)&winobj);
    if(SUCCEEDED(hres)) {
        IOleInPlaceObjectWindowless_InPlaceDeactivate(winobj);
        IOleInPlaceObjectWindowless_Release(winobj);
    }

    if(This->view) {
        IOleDocumentView_Show(This->view, FALSE);
        IOleDocumentView_CloseView(This->view, 0);
        IOleDocumentView_SetInPlaceSite(This->view, NULL);
        IOleDocumentView_Release(This->view);
        This->view = NULL;
    }

    hres = IUnknown_QueryInterface(This->document, &IID_IOleObject, (void**)&oleobj);
    if(SUCCEEDED(hres))
        IOleObject_Close(oleobj, OLECLOSE_NOSAVE);

    hres = IUnknown_QueryInterface(This->document, &IID_IHlinkTarget, (void**)&hlink);
    if(SUCCEEDED(hres)) {
        IHlinkTarget_SetBrowseContext(hlink, NULL);
        IHlinkTarget_Release(hlink);
    }

    if(oleobj) {
        IOleClientSite *client_site = NULL;

        IOleObject_GetClientSite(oleobj, &client_site);
        if(client_site) {
            if(client_site == &This->IOleClientSite_iface)
                IOleObject_SetClientSite(oleobj, NULL);
            IOleClientSite_Release(client_site);
        }

        IOleObject_Release(oleobj);
    }

    IUnknown_Release(This->document);
    This->document = NULL;
}

HRESULT refresh_document(DocHost *This, const VARIANT *level)
{
    IOleCommandTarget *cmdtrg;
    VARIANT vin, vout;
    HRESULT hres;

    if(level && (V_VT(level) != VT_I4 || V_I4(level) != REFRESH_NORMAL))
        FIXME("Unsupported refresh level %s\n", debugstr_variant(level));

    if(!This->document) {
        FIXME("no document\n");
        return E_FAIL;
    }

    hres = IUnknown_QueryInterface(This->document, &IID_IOleCommandTarget, (void**)&cmdtrg);
    if(FAILED(hres))
        return hres;

    V_VT(&vin) = VT_EMPTY;
    V_VT(&vout) = VT_EMPTY;
    hres = IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_REFRESH, OLECMDEXECOPT_PROMPTUSER, &vin, &vout);
    IOleCommandTarget_Release(cmdtrg);
    if(FAILED(hres))
        return hres;

    VariantClear(&vout);
    return S_OK;
}

void activate_document(DocHost *This)
{
    IOleObject *oleobj;
    IHlinkTarget *hlink;
    HRESULT hres;

    if(!This->document) return;

    hres = IUnknown_QueryInterface(This->document, &IID_IHlinkTarget, (void**)&hlink);
    if(SUCCEEDED(hres)) {
        IHlinkTarget_Navigate(hlink, 0, NULL);
        IHlinkTarget_Release(hlink);
    }

    hres = IUnknown_QueryInterface(This->document, &IID_IOleObject, (void**)&oleobj);
    if(SUCCEEDED(hres)) {
        IOleObject_DoVerb(oleobj, OLEIVERB_SHOW, NULL, NULL, -1, 0, NULL);
        IOleObject_Release(oleobj);
    }

    refresh_document(This, NULL);

    if(This->view)
        IOleDocumentView_UIActivate(This->view, TRUE);
}

void release_dochost_client(DocHost *This)
{
    if(This->hwnd) {
        DestroyWindow(This->hwnd);
        This->hwnd = NULL;
    }

    if(This->hostui) {
        IDocHostUIHandler_Release(This->hostui);
        This->hostui = NULL;
    }

    if(This->client_disp) {
        IDispatch_Release(This->client_disp);
        This->client_disp = NULL;
    }

    if(This->frame) {
        IOleInPlaceFrame_Release(This->frame);
        This->frame = NULL;
    }

    if(This->olecmd) {
        IOleCommandTarget_Release(This->olecmd);
        This->olecmd = NULL;
    }
}

static inline DocHost *impl_from_IOleCommandTarget(IOleCommandTarget *iface)
{
    return CONTAINING_RECORD(iface, DocHost, IOleCommandTarget_iface);
}

static HRESULT WINAPI ClOleCommandTarget_QueryInterface(IOleCommandTarget *iface,
        REFIID riid, void **ppv)
{
    DocHost *This = impl_from_IOleCommandTarget(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
}

static ULONG WINAPI ClOleCommandTarget_AddRef(IOleCommandTarget *iface)
{
    DocHost *This = impl_from_IOleCommandTarget(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI ClOleCommandTarget_Release(IOleCommandTarget *iface)
{
    DocHost *This = impl_from_IOleCommandTarget(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT WINAPI ClOleCommandTarget_QueryStatus(IOleCommandTarget *iface,
        const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    DocHost *This = impl_from_IOleCommandTarget(iface);
    ULONG i;

    TRACE("(%p)->(%s %lu %p %p)\n", This, debugstr_guid(pguidCmdGroup), cCmds, prgCmds,
          pCmdText);
    for(i=0; prgCmds && i < cCmds; i++)
        TRACE("unsupported command %lu (%lx)\n", prgCmds[i].cmdID, prgCmds[i].cmdf);

    return E_NOTIMPL;
}

static HRESULT WINAPI ClOleCommandTarget_Exec(IOleCommandTarget *iface,
        const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn,
        VARIANT *pvaOut)
{
    DocHost *This = impl_from_IOleCommandTarget(iface);

    TRACE("(%p)->(%s %ld %ld %s %s)\n", This, debugstr_guid(pguidCmdGroup), nCmdID, nCmdexecopt,
            debugstr_variant(pvaIn), debugstr_variant(pvaOut));

    if(!pguidCmdGroup) {
        switch(nCmdID) {
        case OLECMDID_UPDATECOMMANDS:
            if(!This->olecmd)
                return E_NOTIMPL;
            return IOleCommandTarget_Exec(This->olecmd, pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
        case OLECMDID_SETDOWNLOADSTATE:
            if(pvaIn && V_VT(pvaIn) == VT_I4)
                This->busy = V_I4(pvaIn) ? VARIANT_TRUE : VARIANT_FALSE;
            if(This->olecmd)
                return IOleCommandTarget_Exec(This->olecmd, pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);

            if(!pvaIn || V_VT(pvaIn) != VT_I4)
                return E_INVALIDARG;

            notify_download_state(This, V_I4(pvaIn));
            return S_OK;
        default:
            TRACE("Unimplemented cmdid %ld\n", nCmdID);
            return E_NOTIMPL;
        }
    }

    if(IsEqualGUID(pguidCmdGroup, &CGID_DocHostCmdPriv)) {
        switch(nCmdID) {
        case DOCHOST_DOCCANNAVIGATE:
            if(!pvaIn || V_VT(pvaIn) != VT_UNKNOWN)
                return E_INVALIDARG;

            if(This->doc_navigate)
                IUnknown_Release(This->doc_navigate);
            IUnknown_AddRef(V_UNKNOWN(pvaIn));
            This->doc_navigate = V_UNKNOWN(pvaIn);
            return S_OK;

        case 1: {
            IHTMLWindow2 *win2;
            SAFEARRAY *sa = V_ARRAY(pvaIn);
            VARIANT status_code, url, htmlwindow;
            LONG ind;
            HRESULT hres;

            if(V_VT(pvaIn) != VT_ARRAY || !sa || (SafeArrayGetDim(sa) != 1))
                return E_INVALIDARG;

            ind = 0;
            hres = SafeArrayGetElement(sa, &ind, &status_code);
            if(FAILED(hres) || V_VT(&status_code)!=VT_I4)
                return E_INVALIDARG;

            ind = 1;
            hres = SafeArrayGetElement(sa, &ind, &url);
            if(FAILED(hres) || V_VT(&url)!=VT_BSTR)
                return E_INVALIDARG;

            ind = 3;
            hres = SafeArrayGetElement(sa, &ind, &htmlwindow);
            if(FAILED(hres) || V_VT(&htmlwindow)!=VT_UNKNOWN || !V_UNKNOWN(&htmlwindow))
                return E_INVALIDARG;

            hres = IUnknown_QueryInterface(V_UNKNOWN(&htmlwindow), &IID_IHTMLWindow2, (void**)&win2);
            if(FAILED(hres))
                return E_INVALIDARG;

            handle_navigation_error(This, V_I4(&status_code), V_BSTR(&url), win2);
            IHTMLWindow2_Release(win2);
            return S_OK;
        }

        default:
            TRACE("unsupported command %ld of CGID_DocHostCmdPriv\n", nCmdID);
            return E_NOTIMPL;
        }
    }

    if(IsEqualGUID(pguidCmdGroup, &CGID_Explorer)) {
        switch(nCmdID) {
        case CMDID_EXPLORER_UPDATEHISTORY:
            update_travellog(This);
            update_navigation_commands(This);
            return S_OK;

        default:
            TRACE("Unimplemented cmdid %ld of CGID_Explorer\n", nCmdID);
            return E_NOTIMPL;
        }
    }

    if(IsEqualGUID(pguidCmdGroup, &CGID_ShellDocView)) {
        switch(nCmdID) {
        default:
            TRACE("Unimplemented cmdid %ld of CGID_ShellDocView\n", nCmdID);
            return E_NOTIMPL;
        }
    }

    if(IsEqualGUID(&CGID_DocHostCommandHandler, pguidCmdGroup)) {
        if(!This->olecmd)
            return E_NOTIMPL;
        return IOleCommandTarget_Exec(This->olecmd, pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
    }

    TRACE("Unimplemented cmdid %ld of group %s\n", nCmdID, debugstr_guid(pguidCmdGroup));
    return E_NOTIMPL;
}

static const IOleCommandTargetVtbl OleCommandTargetVtbl = {
    ClOleCommandTarget_QueryInterface,
    ClOleCommandTarget_AddRef,
    ClOleCommandTarget_Release,
    ClOleCommandTarget_QueryStatus,
    ClOleCommandTarget_Exec
};

static inline DocHost *impl_from_IDocHostUIHandler2(IDocHostUIHandler2 *iface)
{
    return CONTAINING_RECORD(iface, DocHost, IDocHostUIHandler2_iface);
}

static HRESULT WINAPI DocHostUIHandler_QueryInterface(IDocHostUIHandler2 *iface,
                                                      REFIID riid, void **ppv)
{
    DocHost *This = impl_from_IDocHostUIHandler2(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
}

static ULONG WINAPI DocHostUIHandler_AddRef(IDocHostUIHandler2 *iface)
{
    DocHost *This = impl_from_IDocHostUIHandler2(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI DocHostUIHandler_Release(IDocHostUIHandler2 *iface)
{
    DocHost *This = impl_from_IDocHostUIHandler2(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT WINAPI DocHostUIHandler_ShowContextMenu(IDocHostUIHandler2 *iface,
         DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved)
{
    DocHost *This = impl_from_IDocHostUIHandler2(iface);
    HRESULT hres;

    TRACE("(%p)->(%ld %p %p %p)\n", This, dwID, ppt, pcmdtReserved, pdispReserved);

    if(This->hostui) {
        hres = IDocHostUIHandler_ShowContextMenu(This->hostui, dwID, ppt, pcmdtReserved,
                                                 pdispReserved);
        if(hres == S_OK)
            return S_OK;
    }

    FIXME("default action not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetHostInfo(IDocHostUIHandler2 *iface,
        DOCHOSTUIINFO *pInfo)
{
    DocHost *This = impl_from_IDocHostUIHandler2(iface);
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pInfo);

    if(This->hostui) {
        hres = IDocHostUIHandler_GetHostInfo(This->hostui, pInfo);
        if(SUCCEEDED(hres))
            return hres;
    }

    pInfo->dwFlags = DOCHOSTUIFLAG_DISABLE_HELP_MENU | DOCHOSTUIFLAG_OPENNEWWIN
        | DOCHOSTUIFLAG_URL_ENCODING_ENABLE_UTF8 | DOCHOSTUIFLAG_ENABLE_INPLACE_NAVIGATION
        | DOCHOSTUIFLAG_IME_ENABLE_RECONVERSION;
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_ShowUI(IDocHostUIHandler2 *iface, DWORD dwID,
        IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget,
        IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc)
{
    DocHost *This = impl_from_IDocHostUIHandler2(iface);
    FIXME("(%p)->(%ld %p %p %p %p)\n", This, dwID, pActiveObject, pCommandTarget,
          pFrame, pDoc);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_HideUI(IDocHostUIHandler2 *iface)
{
    DocHost *This = impl_from_IDocHostUIHandler2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_UpdateUI(IDocHostUIHandler2 *iface)
{
    DocHost *This = impl_from_IDocHostUIHandler2(iface);

    TRACE("(%p)\n", This);

    if(!This->hostui)
        return S_FALSE;

    return IDocHostUIHandler_UpdateUI(This->hostui);
}

static HRESULT WINAPI DocHostUIHandler_EnableModeless(IDocHostUIHandler2 *iface,
                                                      BOOL fEnable)
{
    DocHost *This = impl_from_IDocHostUIHandler2(iface);
    FIXME("(%p)->(%x)\n", This, fEnable);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_OnDocWindowActivate(IDocHostUIHandler2 *iface,
                                                           BOOL fActivate)
{
    DocHost *This = impl_from_IDocHostUIHandler2(iface);
    FIXME("(%p)->(%x)\n", This, fActivate);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_OnFrameWindowActivate(IDocHostUIHandler2 *iface,
                                                             BOOL fActivate)
{
    DocHost *This = impl_from_IDocHostUIHandler2(iface);
    FIXME("(%p)->(%x)\n", This, fActivate);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_ResizeBorder(IDocHostUIHandler2 *iface,
        LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
    DocHost *This = impl_from_IDocHostUIHandler2(iface);
    FIXME("(%p)->(%p %p %X)\n", This, prcBorder, pUIWindow, fRameWindow);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_TranslateAccelerator(IDocHostUIHandler2 *iface,
        LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID)
{
    DocHost *This = impl_from_IDocHostUIHandler2(iface);
    HRESULT hr = S_FALSE;
    TRACE("(%p)->(%p %p %ld)\n", This, lpMsg, pguidCmdGroup, nCmdID);

    if(This->hostui)
        hr = IDocHostUIHandler_TranslateAccelerator(This->hostui, lpMsg, pguidCmdGroup, nCmdID);

    return hr;
}

static HRESULT WINAPI DocHostUIHandler_GetOptionKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    DocHost *This = impl_from_IDocHostUIHandler2(iface);

    TRACE("(%p)->(%p %ld)\n", This, pchKey, dw);

    if(This->hostui)
        return IDocHostUIHandler_GetOptionKeyPath(This->hostui, pchKey, dw);

    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_GetDropTarget(IDocHostUIHandler2 *iface,
        IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
    DocHost *This = impl_from_IDocHostUIHandler2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetExternal(IDocHostUIHandler2 *iface,
        IDispatch **ppDispatch)
{
    DocHost *This = impl_from_IDocHostUIHandler2(iface);

    TRACE("(%p)->(%p)\n", This, ppDispatch);

    if(This->hostui)
        return IDocHostUIHandler_GetExternal(This->hostui, ppDispatch);

    if(!This->shell_ui_helper) {
        HRESULT hres;

        hres = create_shell_ui_helper(&This->shell_ui_helper);
        if(FAILED(hres))
            return hres;
    }

    *ppDispatch = (IDispatch*)This->shell_ui_helper;
    IDispatch_AddRef(*ppDispatch);
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_TranslateUrl(IDocHostUIHandler2 *iface,
        DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
    DocHost *This = impl_from_IDocHostUIHandler2(iface);

    TRACE("(%p)->(%ld %s %p)\n", This, dwTranslate, debugstr_w(pchURLIn), ppchURLOut);

    if(This->hostui)
        return IDocHostUIHandler_TranslateUrl(This->hostui, dwTranslate,
                                              pchURLIn, ppchURLOut);

    return S_FALSE;
}

static HRESULT WINAPI DocHostUIHandler_FilterDataObject(IDocHostUIHandler2 *iface,
        IDataObject *pDO, IDataObject **ppDORet)
{
    DocHost *This = impl_from_IDocHostUIHandler2(iface);
    FIXME("(%p)->(%p %p)\n", This, pDO, ppDORet);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetOverrideKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    DocHost *This = impl_from_IDocHostUIHandler2(iface);
    IDocHostUIHandler2 *handler;
    HRESULT hres;

    TRACE("(%p)->(%p %ld)\n", This, pchKey, dw);

    if(!This->hostui)
        return S_OK;

    hres = IDocHostUIHandler_QueryInterface(This->hostui, &IID_IDocHostUIHandler2,
                                            (void**)&handler);
    if(SUCCEEDED(hres)) {
        hres = IDocHostUIHandler2_GetOverrideKeyPath(handler, pchKey, dw);
        IDocHostUIHandler2_Release(handler);
        return hres;
    }

    return S_OK;
}

static const IDocHostUIHandler2Vtbl DocHostUIHandler2Vtbl = {
    DocHostUIHandler_QueryInterface,
    DocHostUIHandler_AddRef,
    DocHostUIHandler_Release,
    DocHostUIHandler_ShowContextMenu,
    DocHostUIHandler_GetHostInfo,
    DocHostUIHandler_ShowUI,
    DocHostUIHandler_HideUI,
    DocHostUIHandler_UpdateUI,
    DocHostUIHandler_EnableModeless,
    DocHostUIHandler_OnDocWindowActivate,
    DocHostUIHandler_OnFrameWindowActivate,
    DocHostUIHandler_ResizeBorder,
    DocHostUIHandler_TranslateAccelerator,
    DocHostUIHandler_GetOptionKeyPath,
    DocHostUIHandler_GetDropTarget,
    DocHostUIHandler_GetExternal,
    DocHostUIHandler_TranslateUrl,
    DocHostUIHandler_FilterDataObject,
    DocHostUIHandler_GetOverrideKeyPath
};

static inline DocHost *impl_from_IPropertyNotifySink(IPropertyNotifySink *iface)
{
    return CONTAINING_RECORD(iface, DocHost, IPropertyNotifySink_iface);
}

static HRESULT WINAPI PropertyNotifySink_QueryInterface(IPropertyNotifySink *iface,
        REFIID riid, void **ppv)
{
    DocHost *This = impl_from_IPropertyNotifySink(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
}

static ULONG WINAPI PropertyNotifySink_AddRef(IPropertyNotifySink *iface)
{
    DocHost *This = impl_from_IPropertyNotifySink(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI PropertyNotifySink_Release(IPropertyNotifySink *iface)
{
    DocHost *This = impl_from_IPropertyNotifySink(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT WINAPI PropertyNotifySink_OnChanged(IPropertyNotifySink *iface, DISPID dispID)
{
    DocHost *This = impl_from_IPropertyNotifySink(iface);

    TRACE("(%p)->(%ld)\n", This, dispID);

    switch(dispID) {
    case DISPID_READYSTATE: {
        READYSTATE ready_state;
        HRESULT hres;

        hres = get_doc_ready_state(This, &ready_state);
        if(FAILED(hres))
            return hres;

        if(ready_state == READYSTATE_COMPLETE && !This->doc_navigate)
            advise_prop_notif(This, FALSE);

        update_ready_state(This, ready_state);
        break;
    }
    default:
        FIXME("unimplemented dispid %ld\n", dispID);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT WINAPI PropertyNotifySink_OnRequestEdit(IPropertyNotifySink *iface, DISPID dispID)
{
    DocHost *This = impl_from_IPropertyNotifySink(iface);
    FIXME("(%p)->(%ld)\n", This, dispID);
    return E_NOTIMPL;
}

static const IPropertyNotifySinkVtbl PropertyNotifySinkVtbl = {
    PropertyNotifySink_QueryInterface,
    PropertyNotifySink_AddRef,
    PropertyNotifySink_Release,
    PropertyNotifySink_OnChanged,
    PropertyNotifySink_OnRequestEdit
};

void DocHost_Init(DocHost *This, IWebBrowser2 *wb, const IDocHostContainerVtbl* container)
{
    This->IDocHostUIHandler2_iface.lpVtbl  = &DocHostUIHandler2Vtbl;
    This->IOleCommandTarget_iface.lpVtbl   = &OleCommandTargetVtbl;
    This->IPropertyNotifySink_iface.lpVtbl = &PropertyNotifySinkVtbl;

    This->wb = wb;
    This->container_vtbl = container;

    This->ready_state = READYSTATE_UNINITIALIZED;
    list_init(&This->task_queue);

    This->travellog.loading_pos = -1;

    DocHost_ClientSite_Init(This);
    DocHost_Frame_Init(This);

    ConnectionPointContainer_Init(&This->cps, (IUnknown*)wb);
    IEHTMLWindow_Init(This);
    NewWindowManager_Init(This);
}

void DocHost_Release(DocHost *This)
{
    if(This->shell_ui_helper)
        IShellUIHelper2_Release(This->shell_ui_helper);

    abort_dochost_tasks(This, NULL);
    release_dochost_client(This);
    DocHost_ClientSite_Release(This);

    ConnectionPointContainer_Destroy(&This->cps);

    while(This->travellog.length)
        free_travellog_entry(This->travellog.log + --This->travellog.length);
    free(This->travellog.log);

    free(This->url);
}
