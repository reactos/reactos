/*
 * Copyright 2005 Jacek Caban
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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "shlguid.h"
#include "idispids.h"
#include "mimeole.h"
#include "shellapi.h"

#define NO_SHLWAPI_REG
#include "shlwapi.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlscript.h"
#include "htmlevent.h"
#include "binding.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

/* Undocumented notification, see tests */
#define CMDID_EXPLORER_UPDATEHISTORY 38

typedef struct {
    task_t header;
    HTMLDocumentObj *doc;
    BOOL set_download;
    LPOLESTR url;
} download_proc_task_t;

static void notify_travellog_update(HTMLDocumentObj *doc)
{
    IOleCommandTarget *cmdtrg;
    HRESULT hres;

    if(!doc->webbrowser)
        return;

    hres = IOleClientSite_QueryInterface(doc->client, &IID_IOleCommandTarget, (void**)&cmdtrg);
    if(SUCCEEDED(hres)) {
        VARIANT vin;

        V_VT(&vin) = VT_I4;
        V_I4(&vin) = 0;

        IOleCommandTarget_Exec(cmdtrg, &CGID_Explorer, CMDID_EXPLORER_UPDATEHISTORY, 0, &vin, NULL);
        IOleCommandTarget_Release(cmdtrg);
    }
}

void set_current_uri(HTMLOuterWindow *window, IUri *uri)
{
    unlink_ref(&window->uri);
    unlink_ref(&window->uri_nofrag);
    SysFreeString(window->url);
    window->url = NULL;

    if(!uri)
        return;

    IUri_AddRef(uri);
    window->uri = uri;

    window->uri_nofrag = get_uri_nofrag(uri);
    if(!window->uri_nofrag) {
        FIXME("get_uri_nofrag failed\n");
        IUri_AddRef(uri);
        window->uri_nofrag = uri;
    }

    IUri_GetDisplayUri(uri, &window->url);
}

void set_current_mon(HTMLOuterWindow *This, IMoniker *mon, DWORD flags)
{
    IUriContainer *uri_container;
    IUri *uri = NULL;
    HRESULT hres;

    if(This->mon) {
        if(This->browser && !(flags & (BINDING_REPLACE|BINDING_REFRESH))) {
            if(is_main_content_window(This))
                notify_travellog_update(This->browser->doc);
            else
                TRACE("Skipping travellog update for frame navigation.\n");
        }
        IMoniker_Release(This->mon);
        This->mon = NULL;
    }

    This->load_flags = flags;
    if(!mon)
        return;

    IMoniker_AddRef(mon);
    This->mon = mon;

    hres = IMoniker_QueryInterface(mon, &IID_IUriContainer, (void**)&uri_container);
    if(SUCCEEDED(hres)) {
        hres = IUriContainer_GetIUri(uri_container, &uri);
        IUriContainer_Release(uri_container);
        if(hres != S_OK) {
            WARN("GetIUri failed: %08lx\n", hres);
            uri = NULL;
        }
    }

    if(!uri) {
        WCHAR *url;

        hres = IMoniker_GetDisplayName(mon, NULL, NULL, &url);
        if(SUCCEEDED(hres)) {
            hres = create_uri(url, 0, &uri);
            if(FAILED(hres)) {
                WARN("CreateUri failed: %08lx\n", hres);
                set_current_uri(This, NULL);
                This->url = SysAllocString(url);
                CoTaskMemFree(url);
                return;
            }
            CoTaskMemFree(url);
        }else {
            WARN("GetDisplayName failed: %08lx\n", hres);
        }
    }

    set_current_uri(This, uri);
    if(uri)
        IUri_Release(uri);

    if(is_main_content_window(This))
        update_browser_script_mode(This->browser, uri);
}

HRESULT create_uri(const WCHAR *uri_str, DWORD flags, IUri **uri)
{
    return CreateUri(uri_str, flags | Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME, 0, uri);
}

HRESULT create_relative_uri(HTMLOuterWindow *window, const WCHAR *rel_uri, IUri **uri)
{
    return window->uri
        ? CoInternetCombineUrlEx(window->uri, rel_uri, URL_ESCAPE_SPACES_ONLY|URL_DONT_ESCAPE_EXTRA_INFO, uri, 0)
        : create_uri(rel_uri, 0, uri);
}

void set_download_state(HTMLDocumentObj *doc, int state)
{
    if(doc->client) {
        IOleCommandTarget *olecmd;
        HRESULT hres;

        hres = IOleClientSite_QueryInterface(doc->client, &IID_IOleCommandTarget, (void**)&olecmd);
        if(SUCCEEDED(hres)) {
            VARIANT var;

            V_VT(&var) = VT_I4;
            V_I4(&var) = state;

            IOleCommandTarget_Exec(olecmd, NULL, OLECMDID_SETDOWNLOADSTATE,
                    OLECMDEXECOPT_DONTPROMPTUSER, &var, NULL);
            IOleCommandTarget_Release(olecmd);
        }
    }

    doc->download_state = state;
}

static void set_progress_proc(task_t *_task)
{
    docobj_task_t *task = (docobj_task_t*)_task;
    IOleCommandTarget *olecmd = NULL;
    HTMLDocumentObj *doc = task->doc;
    HRESULT hres;

    TRACE("(%p)\n", doc);

    IUnknown_AddRef(doc->outer_unk);

    if(doc->client)
        IOleClientSite_QueryInterface(doc->client, &IID_IOleCommandTarget, (void**)&olecmd);

    if(olecmd) {
        VARIANT progress_max, progress;

        V_VT(&progress_max) = VT_I4;
        V_I4(&progress_max) = 0; /* FIXME */
        IOleCommandTarget_Exec(olecmd, NULL, OLECMDID_SETPROGRESSMAX, OLECMDEXECOPT_DONTPROMPTUSER,
                               &progress_max, NULL);

        V_VT(&progress) = VT_I4;
        V_I4(&progress) = 0; /* FIXME */
        IOleCommandTarget_Exec(olecmd, NULL, OLECMDID_SETPROGRESSPOS, OLECMDEXECOPT_DONTPROMPTUSER,
                               &progress, NULL);
        IOleCommandTarget_Release(olecmd);
    }

    if(doc->nscontainer->usermode == EDITMODE && doc->hostui) {
        DOCHOSTUIINFO hostinfo;

        memset(&hostinfo, 0, sizeof(DOCHOSTUIINFO));
        hostinfo.cbSize = sizeof(DOCHOSTUIINFO);
        hres = IDocHostUIHandler_GetHostInfo(doc->hostui, &hostinfo);
        if(SUCCEEDED(hres))
            /* FIXME: use hostinfo */
            TRACE("hostinfo = {%lu %08lx %08lx %s %s}\n",
                    hostinfo.cbSize, hostinfo.dwFlags, hostinfo.dwDoubleClick,
                    debugstr_w(hostinfo.pchHostCss), debugstr_w(hostinfo.pchHostNS));
    }

    IUnknown_Release(doc->outer_unk);
}

static void set_progress_destr(task_t *_task)
{
}

static void set_downloading_proc(task_t *_task)
{
    download_proc_task_t *task = (download_proc_task_t*)_task;
    HTMLDocumentObj *doc = task->doc;
    HRESULT hres;

    TRACE("(%p)\n", doc);

    IUnknown_AddRef(doc->outer_unk);
    set_statustext(doc, IDS_STATUS_DOWNLOADINGFROM, task->url);

    if(task->set_download)
        set_download_state(doc, 1);

    if(!doc->client)
        goto done;

    if(doc->view_sink)
        IAdviseSink_OnViewChange(doc->view_sink, DVASPECT_CONTENT, -1);

    if(doc->hostui) {
        IDropTarget *drop_target = NULL;

        hres = IDocHostUIHandler_GetDropTarget(doc->hostui, NULL /* FIXME */, &drop_target);
        if(SUCCEEDED(hres) && drop_target) {
            FIXME("Use IDropTarget\n");
            IDropTarget_Release(drop_target);
        }
    }

done:
    IUnknown_Release(doc->outer_unk);
}

static void set_downloading_task_destr(task_t *_task)
{
    download_proc_task_t *task = (download_proc_task_t*)_task;

    CoTaskMemFree(task->url);
}

void prepare_for_binding(HTMLDocumentObj *This, IMoniker *mon, DWORD flags)
{
    HRESULT hres;

    if(This->client) {
        VARIANT silent, offline;

        hres = get_client_disp_property(This->client, DISPID_AMBIENT_SILENT, &silent);
        if(SUCCEEDED(hres)) {
            if(V_VT(&silent) != VT_BOOL)
                WARN("silent = %s\n", debugstr_variant(&silent));
            else if(V_BOOL(&silent))
                FIXME("silent == true\n");
        }

        hres = get_client_disp_property(This->client, DISPID_AMBIENT_OFFLINEIFNOTCONNECTED, &offline);
        if(SUCCEEDED(hres)) {
            if(V_VT(&offline) != VT_BOOL)
                WARN("offline = %s\n", debugstr_variant(&offline));
            else if(V_BOOL(&offline))
                FIXME("offline == true\n");
        }
    }

    if(This->window->mon) {
        update_doc(This, UPDATE_TITLE|UPDATE_UI);
    }else {
        update_doc(This, UPDATE_TITLE);
        set_current_mon(This->window, mon, flags);
    }

    if(This->client) {
        IOleCommandTarget *cmdtrg = NULL;

        hres = IOleClientSite_QueryInterface(This->client, &IID_IOleCommandTarget, (void**)&cmdtrg);
        if(SUCCEEDED(hres)) {
            VARIANT var, out;

            if(flags & BINDING_NAVIGATED) {
                V_VT(&var) = VT_UNKNOWN;
                V_UNKNOWN(&var) = (IUnknown*)&This->window->base.IHTMLWindow2_iface;
                V_VT(&out) = VT_EMPTY;
                hres = IOleCommandTarget_Exec(cmdtrg, &CGID_ShellDocView, 63, 0, &var, &out);
                if(SUCCEEDED(hres))
                    VariantClear(&out);
            }else if(!(flags & BINDING_FROMHIST)) {
                V_VT(&var) = VT_I4;
                V_I4(&var) = 0;
                IOleCommandTarget_Exec(cmdtrg, &CGID_ShellDocView, 37, 0, &var, NULL);
            }

            IOleCommandTarget_Release(cmdtrg);
        }
    }
}

HRESULT set_moniker(HTMLOuterWindow *window, IMoniker *mon, IUri *nav_uri, IBindCtx *pibc, nsChannelBSC *async_bsc,
        BOOL set_download)
{
    download_proc_task_t *download_task;
    nsChannelBSC *bscallback;
    nsWineURI *nsuri;
    LPOLESTR url;
    IUri *uri;
    HRESULT hres;

    hres = IMoniker_GetDisplayName(mon, pibc, NULL, &url);
    if(FAILED(hres)) {
        WARN("GetDisplayName failed: %08lx\n", hres);
        return hres;
    }

    if(nav_uri) {
        uri = nav_uri;
    }else {
        hres = create_uri(url, 0, &uri);
        if(FAILED(hres)) {
            CoTaskMemFree(url);
            return hres;
        }
    }

    TRACE("got url: %s\n", debugstr_w(url));

    set_ready_state(window, READYSTATE_LOADING);

    hres = create_doc_uri(uri, &nsuri);
    if(!nav_uri)
        IUri_Release(uri);
    if(FAILED(hres)) {
        CoTaskMemFree(url);
        return hres;
    }

    if(async_bsc)
        bscallback = async_bsc;
    else
        hres = create_channelbsc(mon, NULL, NULL, 0, TRUE, &bscallback);

    if(SUCCEEDED(hres)) {
        abort_window_bindings(window->base.inner_window);

        hres = load_nsuri(window, nsuri, NULL, bscallback, LOAD_FLAGS_BYPASS_CACHE);
        if(SUCCEEDED(hres)) {
            hres = create_pending_window(window, bscallback);
            TRACE("pending window for %p %p %p\n", window, bscallback, window->pending_window);
        }
        if(bscallback != async_bsc)
            IBindStatusCallback_Release(&bscallback->bsc.IBindStatusCallback_iface);
    }
    nsISupports_Release((nsISupports*)nsuri); /* FIXME */

    if(FAILED(hres)) {
        CoTaskMemFree(url);
        return hres;
    }

    if(is_main_content_window(window)) {
        HTMLDocumentObj *doc_obj = window->browser->doc;

        HTMLDocument_LockContainer(doc_obj, TRUE);

        if(doc_obj->frame) {
            docobj_task_t *task;

            task = malloc(sizeof(docobj_task_t));
            task->doc = doc_obj;
            hres = push_task(&task->header, set_progress_proc, set_progress_destr, doc_obj->task_magic);
            if(FAILED(hres)) {
                CoTaskMemFree(url);
                return hres;
            }
        }

        if(doc_obj->nscontainer->usermode == EDITMODE)
            window->load_flags = BINDING_REFRESH;

        download_task = malloc(sizeof(download_proc_task_t));
        download_task->doc = doc_obj;
        download_task->set_download = set_download;
        download_task->url = url;
        return push_task(&download_task->header, set_downloading_proc, set_downloading_task_destr, doc_obj->task_magic);
    }

    return S_OK;
}

static void notif_readystate(HTMLOuterWindow *window)
{
    HTMLInnerWindow *inner_window = window->base.inner_window;
    HTMLFrameBase *frame_element = window->frame_element;
    DOMEvent *event;
    HRESULT hres;

    window->readystate_pending = FALSE;

    IHTMLWindow2_AddRef(&inner_window->base.IHTMLWindow2_iface);
    if(frame_element)
        IHTMLDOMNode_AddRef(&frame_element->element.node.IHTMLDOMNode_iface);

    if(is_main_content_window(window))
        call_property_onchanged(&window->browser->doc->cp_container, DISPID_READYSTATE);

    hres = create_document_event(inner_window->doc, EVENTID_READYSTATECHANGE, &event);
    if(SUCCEEDED(hres)) {
        event->no_event_obj = TRUE;
        dispatch_event(&inner_window->doc->node.event_target, event);
        IDOMEvent_Release(&event->IDOMEvent_iface);
    }
    IHTMLWindow2_Release(&inner_window->base.IHTMLWindow2_iface);

    if(frame_element) {
        hres = create_document_event(frame_element->element.node.doc, EVENTID_READYSTATECHANGE, &event);
        if(SUCCEEDED(hres)) {
            dispatch_event(&frame_element->element.node.event_target, event);
            IDOMEvent_Release(&event->IDOMEvent_iface);
        }
        IHTMLDOMNode_Release(&frame_element->element.node.IHTMLDOMNode_iface);
    }
}

static void notif_readystate_proc(event_task_t *task)
{
    notif_readystate(task->window->base.outer_window);
}

static void notif_readystate_destr(event_task_t *task)
{
}

void set_ready_state(HTMLOuterWindow *window, READYSTATE readystate)
{
    READYSTATE prev_state = window->readystate;

    window->readystate = readystate;

    if(window->readystate_locked) {
        event_task_t *task;
        HRESULT hres;

        if(window->readystate_pending || prev_state == readystate)
            return;

        task = malloc(sizeof(*task));
        if(!task)
            return;

        hres = push_event_task(task, window->base.inner_window, notif_readystate_proc, notif_readystate_destr, window->task_magic);
        if(SUCCEEDED(hres))
            window->readystate_pending = TRUE;
        return;
    }

    notif_readystate(window);
}

static HRESULT get_doc_string(HTMLDocumentNode *This, char **str)
{
    nsIDOMNode *nsnode;
    LPCWSTR strw;
    nsAString nsstr;
    nsresult nsres;
    HRESULT hres;

    if(!This->dom_document) {
        WARN("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMDocument_QueryInterface(This->dom_document, &IID_nsIDOMNode, (void**)&nsnode);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMNode failed: %08lx\n", nsres);
        return E_FAIL;
    }

    nsAString_Init(&nsstr, NULL);
    hres = nsnode_to_nsstring(nsnode, &nsstr);
    nsIDOMNode_Release(nsnode);
    if(FAILED(hres)) {
        nsAString_Finish(&nsstr);
        return hres;
    }

    nsAString_GetData(&nsstr, &strw);
    TRACE("%s\n", debugstr_w(strw));

    *str = strdupWtoA(strw);

    nsAString_Finish(&nsstr);

    if(!*str)
        return E_OUTOFMEMORY;
    return S_OK;
}


/**********************************************************
 * IPersistMoniker implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IPersistMoniker(IPersistMoniker *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IPersistMoniker_iface);
}

static HRESULT WINAPI DocNodePersistMoniker_QueryInterface(IPersistMoniker *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistMoniker(iface);
    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI DocNodePersistMoniker_AddRef(IPersistMoniker *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistMoniker(iface);
    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI DocNodePersistMoniker_Release(IPersistMoniker *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistMoniker(iface);
    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI DocNodePersistMoniker_GetClassID(IPersistMoniker *iface, CLSID *pClassID)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistMoniker(iface);
    return IPersistFile_GetClassID(&This->IPersistFile_iface, pClassID);
}

static HRESULT WINAPI DocNodePersistMoniker_IsDirty(IPersistMoniker *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistMoniker(iface);

    TRACE("(%p)\n", This);

    return IPersistStreamInit_IsDirty(&This->IPersistStreamInit_iface);
}

static HRESULT WINAPI DocNodePersistMoniker_Load(IPersistMoniker *iface, BOOL fFullyAvailable,
        IMoniker *pimkName, LPBC pibc, DWORD grfMode)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistMoniker(iface);
    return IPersistMoniker_Load(&This->doc_obj->IPersistMoniker_iface, fFullyAvailable, pimkName, pibc, grfMode);
}

static HRESULT WINAPI DocNodePersistMoniker_Save(IPersistMoniker *iface, IMoniker *pimkName,
        LPBC pbc, BOOL fRemember)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistMoniker(iface);
    FIXME("(%p)->(%p %p %x)\n", This, pimkName, pbc, fRemember);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodePersistMoniker_SaveCompleted(IPersistMoniker *iface, IMoniker *pimkName, LPBC pibc)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistMoniker(iface);
    FIXME("(%p)->(%p %p)\n", This, pimkName, pibc);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodePersistMoniker_GetCurMoniker(IPersistMoniker *iface, IMoniker **ppimkName)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistMoniker(iface);
    return IPersistMoniker_GetCurMoniker(&This->doc_obj->IPersistMoniker_iface, ppimkName);
}

static const IPersistMonikerVtbl DocNodePersistMonikerVtbl = {
    DocNodePersistMoniker_QueryInterface,
    DocNodePersistMoniker_AddRef,
    DocNodePersistMoniker_Release,
    DocNodePersistMoniker_GetClassID,
    DocNodePersistMoniker_IsDirty,
    DocNodePersistMoniker_Load,
    DocNodePersistMoniker_Save,
    DocNodePersistMoniker_SaveCompleted,
    DocNodePersistMoniker_GetCurMoniker
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IPersistMoniker(IPersistMoniker *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IPersistMoniker_iface);
}

static HRESULT WINAPI DocObjPersistMoniker_QueryInterface(IPersistMoniker *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistMoniker(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI DocObjPersistMoniker_AddRef(IPersistMoniker *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistMoniker(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI DocObjPersistMoniker_Release(IPersistMoniker *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistMoniker(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI DocObjPersistMoniker_GetClassID(IPersistMoniker *iface, CLSID *pClassID)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistMoniker(iface);
    return IPersistFile_GetClassID(&This->IPersistFile_iface, pClassID);
}

static HRESULT WINAPI DocObjPersistMoniker_IsDirty(IPersistMoniker *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistMoniker(iface);

    TRACE("(%p)\n", This);

    return IPersistStreamInit_IsDirty(&This->IPersistStreamInit_iface);
}

static HRESULT WINAPI DocObjPersistMoniker_Load(IPersistMoniker *iface, BOOL fFullyAvailable,
        IMoniker *pimkName, LPBC pibc, DWORD grfMode)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistMoniker(iface);
    IMoniker *mon;
    HRESULT hres;

    TRACE("(%p)->(%x %p %p %08lx)\n", This, fFullyAvailable, pimkName, pibc, grfMode);

    if(pibc) {
        IUnknown *unk = NULL;

        /* FIXME:
         * Use params:
         * "__PrecreatedObject"
         * "BIND_CONTEXT_PARAM"
         * "__HTMLLOADOPTIONS"
         * "__DWNBINDINFO"
         * "URL Context"
         * "_ITransData_Object_"
         * "_EnumFORMATETC_"
         */

        hres = IBindCtx_GetObjectParam(pibc, (LPOLESTR)SZ_HTML_CLIENTSITE_OBJECTPARAM, &unk);
        if(SUCCEEDED(hres) && unk) {
            IOleClientSite *client = NULL;

            hres = IUnknown_QueryInterface(unk, &IID_IOleClientSite, (void**)&client);
            if(SUCCEEDED(hres)) {
                TRACE("Got client site %p\n", client);
                IOleObject_SetClientSite(&This->IOleObject_iface, client);
                IOleClientSite_Release(client);
            }

            IUnknown_Release(unk);
        }
    }

    if(This->is_mhtml) {
        IUnknown *unk;

        hres = MimeOleObjectFromMoniker(0, pimkName, pibc, &IID_IUnknown, (void**)&unk, &mon);
        if(FAILED(hres))
            return hres;
        IUnknown_Release(unk);
        pibc = NULL;
    }else {
        IMoniker_AddRef(mon = pimkName);
    }

    prepare_for_binding(This, mon, FALSE);
    call_docview_84(This);
    hres = set_moniker(This->window, mon, NULL, pibc, NULL, TRUE);
    IMoniker_Release(mon);
    if(FAILED(hres))
        return hres;

    return start_binding(This->window->pending_window, (BSCallback*)This->window->pending_window->bscallback, pibc);
}

static HRESULT WINAPI DocObjPersistMoniker_Save(IPersistMoniker *iface, IMoniker *pimkName,
        LPBC pbc, BOOL fRemember)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistMoniker(iface);
    FIXME("(%p)->(%p %p %x)\n", This, pimkName, pbc, fRemember);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjPersistMoniker_SaveCompleted(IPersistMoniker *iface, IMoniker *pimkName, LPBC pibc)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistMoniker(iface);
    FIXME("(%p)->(%p %p)\n", This, pimkName, pibc);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjPersistMoniker_GetCurMoniker(IPersistMoniker *iface, IMoniker **ppimkName)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistMoniker(iface);

    TRACE("(%p)->(%p)\n", This, ppimkName);

    if(!This->window || !This->window->mon)
        return E_UNEXPECTED;

    IMoniker_AddRef(This->window->mon);
    *ppimkName = This->window->mon;
    return S_OK;
}

static const IPersistMonikerVtbl DocObjPersistMonikerVtbl = {
    DocObjPersistMoniker_QueryInterface,
    DocObjPersistMoniker_AddRef,
    DocObjPersistMoniker_Release,
    DocObjPersistMoniker_GetClassID,
    DocObjPersistMoniker_IsDirty,
    DocObjPersistMoniker_Load,
    DocObjPersistMoniker_Save,
    DocObjPersistMoniker_SaveCompleted,
    DocObjPersistMoniker_GetCurMoniker
};

/**********************************************************
 * IMonikerProp implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IMonikerProp(IMonikerProp *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IMonikerProp_iface);
}

static HRESULT WINAPI DocNodeMonikerProp_QueryInterface(IMonikerProp *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IMonikerProp(iface);
    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI DocNodeMonikerProp_AddRef(IMonikerProp *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IMonikerProp(iface);
    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI DocNodeMonikerProp_Release(IMonikerProp *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IMonikerProp(iface);
    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI DocNodeMonikerProp_PutProperty(IMonikerProp *iface, MONIKERPROPERTY mkp, LPCWSTR val)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IMonikerProp(iface);
    return IMonikerProp_PutProperty(&This->doc_obj->IMonikerProp_iface, mkp, val);
}

static const IMonikerPropVtbl DocNodeMonikerPropVtbl = {
    DocNodeMonikerProp_QueryInterface,
    DocNodeMonikerProp_AddRef,
    DocNodeMonikerProp_Release,
    DocNodeMonikerProp_PutProperty
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IMonikerProp(IMonikerProp *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IMonikerProp_iface);
}

static HRESULT WINAPI DocObjMonikerProp_QueryInterface(IMonikerProp *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IMonikerProp(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI DocObjMonikerProp_AddRef(IMonikerProp *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IMonikerProp(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI DocObjMonikerProp_Release(IMonikerProp *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IMonikerProp(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI DocObjMonikerProp_PutProperty(IMonikerProp *iface, MONIKERPROPERTY mkp, LPCWSTR val)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IMonikerProp(iface);

    TRACE("(%p)->(%d %s)\n", This, mkp, debugstr_w(val));

    switch(mkp) {
    case MIMETYPEPROP:
        free(This->mime);
        This->mime = wcsdup(val);
        break;

    case CLASSIDPROP:
        break;

    default:
        FIXME("mkp %d\n", mkp);
        return E_NOTIMPL;
    }

    return S_OK;
}

static const IMonikerPropVtbl DocObjMonikerPropVtbl = {
    DocObjMonikerProp_QueryInterface,
    DocObjMonikerProp_AddRef,
    DocObjMonikerProp_Release,
    DocObjMonikerProp_PutProperty
};

/**********************************************************
 * IPersistFile implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IPersistFile(IPersistFile *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IPersistFile_iface);
}

static HRESULT WINAPI DocNodePersistFile_QueryInterface(IPersistFile *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistFile(iface);
    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI DocNodePersistFile_AddRef(IPersistFile *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistFile(iface);
    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI DocNodePersistFile_Release(IPersistFile *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistFile(iface);
    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI DocNodePersistFile_GetClassID(IPersistFile *iface, CLSID *pClassID)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistFile(iface);
    return IPersistFile_GetClassID(&This->doc_obj->IPersistFile_iface, pClassID);
}

static HRESULT WINAPI DocNodePersistFile_IsDirty(IPersistFile *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistFile(iface);

    TRACE("(%p)\n", This);

    return IPersistStreamInit_IsDirty(&This->IPersistStreamInit_iface);
}

static HRESULT WINAPI DocNodePersistFile_Load(IPersistFile *iface, LPCOLESTR pszFileName, DWORD dwMode)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistFile(iface);
    FIXME("(%p)->(%s %08lx)\n", This, debugstr_w(pszFileName), dwMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodePersistFile_Save(IPersistFile *iface, LPCOLESTR pszFileName, BOOL fRemember)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistFile(iface);
    char *str;
    DWORD written=0;
    HANDLE file;
    HRESULT hres;

    TRACE("(%p)->(%s %x)\n", This, debugstr_w(pszFileName), fRemember);

    file = CreateFileW(pszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    if(file == INVALID_HANDLE_VALUE) {
        WARN("Could not create file: %lu\n", GetLastError());
        return E_FAIL;
    }

    hres = get_doc_string(This, &str);
    if(SUCCEEDED(hres))
        WriteFile(file, str, strlen(str), &written, NULL);

    CloseHandle(file);
    return hres;
}

static HRESULT WINAPI DocNodePersistFile_SaveCompleted(IPersistFile *iface, LPCOLESTR pszFileName)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistFile(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pszFileName));
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodePersistFile_GetCurFile(IPersistFile *iface, LPOLESTR *pszFileName)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistFile(iface);
    FIXME("(%p)->(%p)\n", This, pszFileName);
    return E_NOTIMPL;
}

static const IPersistFileVtbl DocNodePersistFileVtbl = {
    DocNodePersistFile_QueryInterface,
    DocNodePersistFile_AddRef,
    DocNodePersistFile_Release,
    DocNodePersistFile_GetClassID,
    DocNodePersistFile_IsDirty,
    DocNodePersistFile_Load,
    DocNodePersistFile_Save,
    DocNodePersistFile_SaveCompleted,
    DocNodePersistFile_GetCurFile
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IPersistFile(IPersistFile *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IPersistFile_iface);
}

static HRESULT WINAPI DocObjPersistFile_QueryInterface(IPersistFile *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistFile(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI DocObjPersistFile_AddRef(IPersistFile *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistFile(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI DocObjPersistFile_Release(IPersistFile *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistFile(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI DocObjPersistFile_GetClassID(IPersistFile *iface, CLSID *pClassID)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistFile(iface);

    TRACE("(%p)->(%p)\n", This, pClassID);

    if(!pClassID)
        return E_INVALIDARG;

    *pClassID = CLSID_HTMLDocument;
    return S_OK;
}

static HRESULT WINAPI DocObjPersistFile_IsDirty(IPersistFile *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistFile(iface);

    TRACE("(%p)\n", This);

    return IPersistStreamInit_IsDirty(&This->IPersistStreamInit_iface);
}

static HRESULT WINAPI DocObjPersistFile_Load(IPersistFile *iface, LPCOLESTR pszFileName, DWORD dwMode)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistFile(iface);
    FIXME("(%p)->(%s %08lx)\n", This, debugstr_w(pszFileName), dwMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjPersistFile_Save(IPersistFile *iface, LPCOLESTR pszFileName, BOOL fRemember)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistFile(iface);

    return IPersistFile_Save(&This->doc_node->IPersistFile_iface, pszFileName, fRemember);
}

static HRESULT WINAPI DocObjPersistFile_SaveCompleted(IPersistFile *iface, LPCOLESTR pszFileName)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistFile(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pszFileName));
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjPersistFile_GetCurFile(IPersistFile *iface, LPOLESTR *pszFileName)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistFile(iface);
    FIXME("(%p)->(%p)\n", This, pszFileName);
    return E_NOTIMPL;
}

static const IPersistFileVtbl DocObjPersistFileVtbl = {
    DocObjPersistFile_QueryInterface,
    DocObjPersistFile_AddRef,
    DocObjPersistFile_Release,
    DocObjPersistFile_GetClassID,
    DocObjPersistFile_IsDirty,
    DocObjPersistFile_Load,
    DocObjPersistFile_Save,
    DocObjPersistFile_SaveCompleted,
    DocObjPersistFile_GetCurFile
};

static inline HTMLDocumentNode *HTMLDocumentNode_from_IPersistStreamInit(IPersistStreamInit *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IPersistStreamInit_iface);
}

static HRESULT WINAPI DocNodePersistStreamInit_QueryInterface(IPersistStreamInit *iface,
                                                              REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistStreamInit(iface);
    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI DocNodePersistStreamInit_AddRef(IPersistStreamInit *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistStreamInit(iface);
    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI DocNodePersistStreamInit_Release(IPersistStreamInit *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistStreamInit(iface);
    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI DocNodePersistStreamInit_GetClassID(IPersistStreamInit *iface, CLSID *pClassID)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistStreamInit(iface);
    return IPersistFile_GetClassID(&This->IPersistFile_iface, pClassID);
}

static HRESULT WINAPI DocNodePersistStreamInit_IsDirty(IPersistStreamInit *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistStreamInit(iface);
    return IPersistStreamInit_IsDirty(&This->doc_obj->IPersistStreamInit_iface);
}

static HRESULT WINAPI DocNodePersistStreamInit_Load(IPersistStreamInit *iface, IStream *pStm)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistStreamInit(iface);
    return IPersistStreamInit_Load(&This->doc_obj->IPersistStreamInit_iface, pStm);
}

static HRESULT WINAPI DocNodePersistStreamInit_Save(IPersistStreamInit *iface, IStream *pStm,
                                                    BOOL fClearDirty)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistStreamInit(iface);
    char *str;
    DWORD written=0;
    HRESULT hres;

    TRACE("(%p)->(%p %x)\n", This, pStm, fClearDirty);

    hres = get_doc_string(This, &str);
    if(FAILED(hres))
        return hres;

    hres = IStream_Write(pStm, str, strlen(str), &written);
    if(FAILED(hres))
        FIXME("Write failed: %08lx\n", hres);

    free(str);

    if(fClearDirty)
        set_dirty(This->doc_obj->nscontainer, VARIANT_FALSE);

    return S_OK;
}

static HRESULT WINAPI DocNodePersistStreamInit_GetSizeMax(IPersistStreamInit *iface,
                                                          ULARGE_INTEGER *pcbSize)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistStreamInit(iface);
    FIXME("(%p)->(%p)\n", This, pcbSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodePersistStreamInit_InitNew(IPersistStreamInit *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistStreamInit(iface);
    return IPersistStreamInit_InitNew(&This->doc_obj->IPersistStreamInit_iface);
}

static const IPersistStreamInitVtbl DocNodePersistStreamInitVtbl = {
    DocNodePersistStreamInit_QueryInterface,
    DocNodePersistStreamInit_AddRef,
    DocNodePersistStreamInit_Release,
    DocNodePersistStreamInit_GetClassID,
    DocNodePersistStreamInit_IsDirty,
    DocNodePersistStreamInit_Load,
    DocNodePersistStreamInit_Save,
    DocNodePersistStreamInit_GetSizeMax,
    DocNodePersistStreamInit_InitNew
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IPersistStreamInit(IPersistStreamInit *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IPersistStreamInit_iface);
}

static HRESULT WINAPI DocObjPersistStreamInit_QueryInterface(IPersistStreamInit *iface,
                                                             REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistStreamInit(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI DocObjPersistStreamInit_AddRef(IPersistStreamInit *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistStreamInit(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI DocObjPersistStreamInit_Release(IPersistStreamInit *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistStreamInit(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI DocObjPersistStreamInit_GetClassID(IPersistStreamInit *iface, CLSID *pClassID)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistStreamInit(iface);
    return IPersistFile_GetClassID(&This->IPersistFile_iface, pClassID);
}

static HRESULT WINAPI DocObjPersistStreamInit_IsDirty(IPersistStreamInit *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistStreamInit(iface);

    TRACE("(%p)\n", This);

    return browser_is_dirty(This->nscontainer);
}

static HRESULT WINAPI DocObjPersistStreamInit_Load(IPersistStreamInit *iface, IStream *pStm)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistStreamInit(iface);
    IMoniker *mon;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pStm);

    hres = CreateURLMoniker(NULL, L"about:blank", &mon);
    if(FAILED(hres)) {
        WARN("CreateURLMoniker failed: %08lx\n", hres);
        return hres;
    }

    prepare_for_binding(This, mon, FALSE);
    hres = set_moniker(This->window, mon, NULL, NULL, NULL, TRUE);
    if(SUCCEEDED(hres))
        hres = channelbsc_load_stream(This->window->pending_window, mon, pStm);

    IMoniker_Release(mon);
    return hres;
}

static HRESULT WINAPI DocObjPersistStreamInit_Save(IPersistStreamInit *iface, IStream *pStm,
                                                   BOOL fClearDirty)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistStreamInit(iface);

    return IPersistStreamInit_Save(&This->doc_node->IPersistStreamInit_iface, pStm, fClearDirty);
}

static HRESULT WINAPI DocObjPersistStreamInit_GetSizeMax(IPersistStreamInit *iface,
                                                         ULARGE_INTEGER *pcbSize)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistStreamInit(iface);
    FIXME("(%p)->(%p)\n", This, pcbSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjPersistStreamInit_InitNew(IPersistStreamInit *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistStreamInit(iface);
    IMoniker *mon;
    HRESULT hres;

    TRACE("(%p)\n", This);

    hres = CreateURLMoniker(NULL, L"about:blank", &mon);
    if(FAILED(hres)) {
        WARN("CreateURLMoniker failed: %08lx\n", hres);
        return hres;
    }

    prepare_for_binding(This, mon, FALSE);
    hres = set_moniker(This->window, mon, NULL, NULL, NULL, FALSE);
    if(SUCCEEDED(hres))
        hres = channelbsc_load_stream(This->window->pending_window, mon, NULL);

    IMoniker_Release(mon);
    return hres;
}

static const IPersistStreamInitVtbl DocObjPersistStreamInitVtbl = {
    DocObjPersistStreamInit_QueryInterface,
    DocObjPersistStreamInit_AddRef,
    DocObjPersistStreamInit_Release,
    DocObjPersistStreamInit_GetClassID,
    DocObjPersistStreamInit_IsDirty,
    DocObjPersistStreamInit_Load,
    DocObjPersistStreamInit_Save,
    DocObjPersistStreamInit_GetSizeMax,
    DocObjPersistStreamInit_InitNew
};

/**********************************************************
 * IPersistHistory implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IPersistHistory(IPersistHistory *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IPersistHistory_iface);
}

static HRESULT WINAPI DocNodePersistHistory_QueryInterface(IPersistHistory *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistHistory(iface);
    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI DocNodePersistHistory_AddRef(IPersistHistory *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistHistory(iface);
    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI DocNodePersistHistory_Release(IPersistHistory *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistHistory(iface);
    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI DocNodePersistHistory_GetClassID(IPersistHistory *iface, CLSID *pClassID)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistHistory(iface);
    return IPersistFile_GetClassID(&This->IPersistFile_iface, pClassID);
}

static HRESULT WINAPI DocNodePersistHistory_LoadHistory(IPersistHistory *iface, IStream *pStream, IBindCtx *pbc)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistHistory(iface);
    return IPersistHistory_LoadHistory(&This->doc_obj->IPersistHistory_iface, pStream, pbc);
}

static HRESULT WINAPI DocNodePersistHistory_SaveHistory(IPersistHistory *iface, IStream *pStream)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistHistory(iface);
    return IPersistHistory_SaveHistory(&This->doc_obj->IPersistHistory_iface, pStream);
}

static HRESULT WINAPI DocNodePersistHistory_SetPositionCookie(IPersistHistory *iface, DWORD dwPositioncookie)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistHistory(iface);
    FIXME("(%p)->(%lx)\n", This, dwPositioncookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodePersistHistory_GetPositionCookie(IPersistHistory *iface, DWORD *pdwPositioncookie)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IPersistHistory(iface);
    FIXME("(%p)->(%p)\n", This, pdwPositioncookie);
    return E_NOTIMPL;
}

static const IPersistHistoryVtbl DocNodePersistHistoryVtbl = {
    DocNodePersistHistory_QueryInterface,
    DocNodePersistHistory_AddRef,
    DocNodePersistHistory_Release,
    DocNodePersistHistory_GetClassID,
    DocNodePersistHistory_LoadHistory,
    DocNodePersistHistory_SaveHistory,
    DocNodePersistHistory_SetPositionCookie,
    DocNodePersistHistory_GetPositionCookie
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IPersistHistory(IPersistHistory *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IPersistHistory_iface);
}

static HRESULT WINAPI DocObjPersistHistory_QueryInterface(IPersistHistory *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistHistory(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI DocObjPersistHistory_AddRef(IPersistHistory *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistHistory(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI DocObjPersistHistory_Release(IPersistHistory *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistHistory(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI DocObjPersistHistory_GetClassID(IPersistHistory *iface, CLSID *pClassID)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistHistory(iface);
    return IPersistFile_GetClassID(&This->IPersistFile_iface, pClassID);
}

static HRESULT WINAPI DocObjPersistHistory_LoadHistory(IPersistHistory *iface, IStream *pStream, IBindCtx *pbc)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistHistory(iface);
    ULONG str_len, read;
    WCHAR *uri_str;
    IUri *uri;
    HRESULT hres;

    TRACE("(%p)->(%p %p)\n", This, pStream, pbc);

    if(!This->window) {
        FIXME("No current window\n");
        return E_UNEXPECTED;
    }

    if(pbc)
        FIXME("pbc not supported\n");

    if(This->client) {
        IOleCommandTarget *cmdtrg = NULL;

        hres = IOleClientSite_QueryInterface(This->client, &IID_IOleCommandTarget, (void**)&cmdtrg);
        if(SUCCEEDED(hres)) {
            IOleCommandTarget_Exec(cmdtrg, &CGID_ShellDocView, 138, 0, NULL, NULL);
            IOleCommandTarget_Release(cmdtrg);
        }
    }

    hres = IStream_Read(pStream, &str_len, sizeof(str_len), &read);
    if(FAILED(hres))
        return hres;
    if(read != sizeof(str_len))
        return E_FAIL;

    uri_str = malloc((str_len + 1) * sizeof(WCHAR));
    if(!uri_str)
        return E_OUTOFMEMORY;

    hres = IStream_Read(pStream, uri_str, str_len*sizeof(WCHAR), &read);
    if(SUCCEEDED(hres) && read != str_len*sizeof(WCHAR))
        hres = E_FAIL;
    if(SUCCEEDED(hres)) {
        uri_str[str_len] = 0;
        hres = create_uri(uri_str, 0, &uri);
    }
    free(uri_str);
    if(FAILED(hres))
        return hres;

    hres = load_uri(This->window, uri, BINDING_FROMHIST);
    IUri_Release(uri);
    return hres;
}

static HRESULT WINAPI DocObjPersistHistory_SaveHistory(IPersistHistory *iface, IStream *pStream)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistHistory(iface);
    ULONG len, written;
    BSTR display_uri;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pStream);

    if(!This->window || !This->window->uri) {
        FIXME("No current URI\n");
        return E_FAIL;
    }

    /* NOTE: The format we store is *not* compatible with native MSHTML. We currently
     * store only URI of the page (as a length followed by a string) */
    hres = IUri_GetDisplayUri(This->window->uri, &display_uri);
    if(FAILED(hres))
        return hres;

    len = SysStringLen(display_uri);
    hres = IStream_Write(pStream, &len, sizeof(len), &written);
    if(SUCCEEDED(hres))
        hres = IStream_Write(pStream, display_uri, len*sizeof(WCHAR), &written);
    SysFreeString(display_uri);
    return hres;
}

static HRESULT WINAPI DocObjPersistHistory_SetPositionCookie(IPersistHistory *iface, DWORD dwPositioncookie)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistHistory(iface);
    FIXME("(%p)->(%lx)\n", This, dwPositioncookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjPersistHistory_GetPositionCookie(IPersistHistory *iface, DWORD *pdwPositioncookie)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IPersistHistory(iface);
    FIXME("(%p)->(%p)\n", This, pdwPositioncookie);
    return E_NOTIMPL;
}

static const IPersistHistoryVtbl DocObjPersistHistoryVtbl = {
    DocObjPersistHistory_QueryInterface,
    DocObjPersistHistory_AddRef,
    DocObjPersistHistory_Release,
    DocObjPersistHistory_GetClassID,
    DocObjPersistHistory_LoadHistory,
    DocObjPersistHistory_SaveHistory,
    DocObjPersistHistory_SetPositionCookie,
    DocObjPersistHistory_GetPositionCookie
};

/**********************************************************
 * IHlinkTarget implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IHlinkTarget(IHlinkTarget *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IHlinkTarget_iface);
}

static HRESULT WINAPI DocNodeHlinkTarget_QueryInterface(IHlinkTarget *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IHlinkTarget(iface);
    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI DocNodeHlinkTarget_AddRef(IHlinkTarget *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IHlinkTarget(iface);
    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI DocNodeHlinkTarget_Release(IHlinkTarget *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IHlinkTarget(iface);
    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI DocNodeHlinkTarget_SetBrowseContext(IHlinkTarget *iface, IHlinkBrowseContext *pihlbc)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IHlinkTarget(iface);
    FIXME("(%p)->(%p)\n", This, pihlbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeHlinkTarget_GetBrowseContext(IHlinkTarget *iface, IHlinkBrowseContext **ppihlbc)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IHlinkTarget(iface);
    FIXME("(%p)->(%p)\n", This, ppihlbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeHlinkTarget_Navigate(IHlinkTarget *iface, DWORD grfHLNF, LPCWSTR pwzJumpLocation)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IHlinkTarget(iface);

    TRACE("(%p)->(%08lx %s)\n", This, grfHLNF, debugstr_w(pwzJumpLocation));

    if(grfHLNF)
        FIXME("Unsupported grfHLNF=%08lx\n", grfHLNF);
    if(pwzJumpLocation)
        FIXME("JumpLocation not supported\n");

    if(This->doc_obj->client)
        return IOleObject_DoVerb(&This->IOleObject_iface, OLEIVERB_SHOW, NULL, NULL, -1, NULL, NULL);

    return IHlinkTarget_Navigate(&This->doc_obj->IHlinkTarget_iface, grfHLNF, pwzJumpLocation);
}

static HRESULT WINAPI DocNodeHlinkTarget_GetMoniker(IHlinkTarget *iface, LPCWSTR pwzLocation, DWORD dwAssign,
        IMoniker **ppimkLocation)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IHlinkTarget(iface);
    FIXME("(%p)->(%s %08lx %p)\n", This, debugstr_w(pwzLocation), dwAssign, ppimkLocation);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeHlinkTarget_GetFriendlyName(IHlinkTarget *iface, LPCWSTR pwzLocation,
        LPWSTR *ppwzFriendlyName)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IHlinkTarget(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(pwzLocation), ppwzFriendlyName);
    return E_NOTIMPL;
}

static const IHlinkTargetVtbl DocNodeHlinkTargetVtbl = {
    DocNodeHlinkTarget_QueryInterface,
    DocNodeHlinkTarget_AddRef,
    DocNodeHlinkTarget_Release,
    DocNodeHlinkTarget_SetBrowseContext,
    DocNodeHlinkTarget_GetBrowseContext,
    DocNodeHlinkTarget_Navigate,
    DocNodeHlinkTarget_GetMoniker,
    DocNodeHlinkTarget_GetFriendlyName
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IHlinkTarget(IHlinkTarget *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IHlinkTarget_iface);
}

static HRESULT WINAPI DocObjHlinkTarget_QueryInterface(IHlinkTarget *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IHlinkTarget(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI DocObjHlinkTarget_AddRef(IHlinkTarget *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IHlinkTarget(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI DocObjHlinkTarget_Release(IHlinkTarget *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IHlinkTarget(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI DocObjHlinkTarget_SetBrowseContext(IHlinkTarget *iface, IHlinkBrowseContext *pihlbc)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IHlinkTarget(iface);
    FIXME("(%p)->(%p)\n", This, pihlbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjHlinkTarget_GetBrowseContext(IHlinkTarget *iface, IHlinkBrowseContext **ppihlbc)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IHlinkTarget(iface);
    FIXME("(%p)->(%p)\n", This, ppihlbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjHlinkTarget_Navigate(IHlinkTarget *iface, DWORD grfHLNF, LPCWSTR pwzJumpLocation)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IHlinkTarget(iface);

    TRACE("(%p)->(%08lx %s)\n", This, grfHLNF, debugstr_w(pwzJumpLocation));

    if(grfHLNF)
        FIXME("Unsupported grfHLNF=%08lx\n", grfHLNF);
    if(pwzJumpLocation)
        FIXME("JumpLocation not supported\n");

    if(!This->client) {
        HRESULT hres;
        BSTR uri;

        hres = IUri_GetAbsoluteUri(This->window->uri, &uri);
        if(FAILED(hres))
            return hres;

        if(hres == S_OK)
            ShellExecuteW(NULL, L"open", uri, NULL, NULL, SW_SHOW);
        SysFreeString(uri);
        return S_OK;
    }

    return IOleObject_DoVerb(&This->IOleObject_iface, OLEIVERB_SHOW, NULL, NULL, -1, NULL, NULL);
}

static HRESULT WINAPI DocObjHlinkTarget_GetMoniker(IHlinkTarget *iface, LPCWSTR pwzLocation, DWORD dwAssign,
        IMoniker **ppimkLocation)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IHlinkTarget(iface);
    FIXME("(%p)->(%s %08lx %p)\n", This, debugstr_w(pwzLocation), dwAssign, ppimkLocation);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjHlinkTarget_GetFriendlyName(IHlinkTarget *iface, LPCWSTR pwzLocation,
        LPWSTR *ppwzFriendlyName)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IHlinkTarget(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(pwzLocation), ppwzFriendlyName);
    return E_NOTIMPL;
}

static const IHlinkTargetVtbl DocObjHlinkTargetVtbl = {
    DocObjHlinkTarget_QueryInterface,
    DocObjHlinkTarget_AddRef,
    DocObjHlinkTarget_Release,
    DocObjHlinkTarget_SetBrowseContext,
    DocObjHlinkTarget_GetBrowseContext,
    DocObjHlinkTarget_Navigate,
    DocObjHlinkTarget_GetMoniker,
    DocObjHlinkTarget_GetFriendlyName
};

void HTMLDocumentNode_Persist_Init(HTMLDocumentNode *This)
{
    This->IPersistMoniker_iface.lpVtbl = &DocNodePersistMonikerVtbl;
    This->IPersistFile_iface.lpVtbl = &DocNodePersistFileVtbl;
    This->IMonikerProp_iface.lpVtbl = &DocNodeMonikerPropVtbl;
    This->IPersistStreamInit_iface.lpVtbl = &DocNodePersistStreamInitVtbl;
    This->IPersistHistory_iface.lpVtbl = &DocNodePersistHistoryVtbl;
    This->IHlinkTarget_iface.lpVtbl = &DocNodeHlinkTargetVtbl;
}

void HTMLDocumentObj_Persist_Init(HTMLDocumentObj *This)
{
    This->IPersistMoniker_iface.lpVtbl = &DocObjPersistMonikerVtbl;
    This->IPersistFile_iface.lpVtbl = &DocObjPersistFileVtbl;
    This->IMonikerProp_iface.lpVtbl = &DocObjMonikerPropVtbl;
    This->IPersistStreamInit_iface.lpVtbl = &DocObjPersistStreamInitVtbl;
    This->IPersistHistory_iface.lpVtbl = &DocObjPersistHistoryVtbl;
    This->IHlinkTarget_iface.lpVtbl = &DocObjHlinkTargetVtbl;
}
