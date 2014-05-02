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

#include "mshtml_private.h"

/* Undocumented notification, see tests */
#define CMDID_EXPLORER_UPDATEHISTORY 38

static const WCHAR about_blankW[] = {'a','b','o','u','t',':','b','l','a','n','k',0};

typedef struct {
    task_t header;
    HTMLDocumentObj *doc;
    BOOL set_download;
    LPOLESTR url;
} download_proc_task_t;

static BOOL use_gecko_script(HTMLOuterWindow *window)
{
    DWORD zone;
    HRESULT hres;

    hres = IInternetSecurityManager_MapUrlToZone(window->secmgr, window->url, &zone, 0);
    if(FAILED(hres)) {
        WARN("Could not map %s to zone: %08x\n", debugstr_w(window->url), hres);
        return TRUE;
    }

    TRACE("zone %d\n", zone);
    return zone == URLZONE_UNTRUSTED;
}

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
    if(window->uri) {
        IUri_Release(window->uri);
        window->uri = NULL;
    }

    if(window->uri_nofrag) {
        IUri_Release(window->uri_nofrag);
        window->uri_nofrag = NULL;
    }

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
        if(This->doc_obj && !(flags & (BINDING_REPLACE|BINDING_REFRESH)))
            notify_travellog_update(This->doc_obj);
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
            WARN("GetIUri failed: %08x\n", hres);
            uri = NULL;
        }
    }

    if(!uri) {
        WCHAR *url;

        hres = IMoniker_GetDisplayName(mon, NULL, NULL, &url);
        if(SUCCEEDED(hres)) {
            hres = create_uri(url, 0, &uri);
            if(FAILED(hres)) {
                WARN("CrateUri failed: %08x\n", hres);
                set_current_uri(This, NULL);
                This->url = SysAllocString(url);
                CoTaskMemFree(url);
                return;
            }
            CoTaskMemFree(url);
        }else {
            WARN("GetDisplayName failed: %08x\n", hres);
        }
    }

    set_current_uri(This, uri);
    if(uri)
        IUri_Release(uri);
    set_script_mode(This, use_gecko_script(This) ? SCRIPTMODE_GECKO : SCRIPTMODE_ACTIVESCRIPT);
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

    if(doc->usermode == EDITMODE && doc->hostui) {
        DOCHOSTUIINFO hostinfo;

        memset(&hostinfo, 0, sizeof(DOCHOSTUIINFO));
        hostinfo.cbSize = sizeof(DOCHOSTUIINFO);
        hres = IDocHostUIHandler_GetHostInfo(doc->hostui, &hostinfo);
        if(SUCCEEDED(hres))
            /* FIXME: use hostinfo */
            TRACE("hostinfo = {%u %08x %08x %s %s}\n",
                    hostinfo.cbSize, hostinfo.dwFlags, hostinfo.dwDoubleClick,
                    debugstr_w(hostinfo.pchHostCss), debugstr_w(hostinfo.pchHostNS));
    }
}

static void set_downloading_proc(task_t *_task)
{
    download_proc_task_t *task = (download_proc_task_t*)_task;
    HTMLDocumentObj *doc = task->doc;
    HRESULT hres;

    TRACE("(%p)\n", doc);

    set_statustext(doc, IDS_STATUS_DOWNLOADINGFROM, task->url);

    if(task->set_download)
        set_download_state(doc, 1);

    if(!doc->client)
        return;

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
}

static void set_downloading_task_destr(task_t *_task)
{
    download_proc_task_t *task = (download_proc_task_t*)_task;

    CoTaskMemFree(task->url);
    heap_free(task);
}

void prepare_for_binding(HTMLDocument *This, IMoniker *mon, DWORD flags)
{
    HRESULT hres;

    if(This->doc_obj->client) {
        VARIANT silent, offline;

        hres = get_client_disp_property(This->doc_obj->client, DISPID_AMBIENT_SILENT, &silent);
        if(SUCCEEDED(hres)) {
            if(V_VT(&silent) != VT_BOOL)
                WARN("silent = %s\n", debugstr_variant(&silent));
            else if(V_BOOL(&silent))
                FIXME("silent == true\n");
        }

        hres = get_client_disp_property(This->doc_obj->client,
                DISPID_AMBIENT_OFFLINEIFNOTCONNECTED, &offline);
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

    if(This->doc_obj->client) {
        IOleCommandTarget *cmdtrg = NULL;

        hres = IOleClientSite_QueryInterface(This->doc_obj->client, &IID_IOleCommandTarget,
                (void**)&cmdtrg);
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
    HTMLDocumentObj *doc_obj = NULL;
    nsChannelBSC *bscallback;
    nsWineURI *nsuri;
    LPOLESTR url;
    IUri *uri;
    HRESULT hres;

    if(window->doc_obj && window->doc_obj->basedoc.window == window)
        doc_obj = window->doc_obj;

    hres = IMoniker_GetDisplayName(mon, pibc, NULL, &url);
    if(FAILED(hres)) {
        WARN("GetDiaplayName failed: %08x\n", hres);
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

    hres = create_doc_uri(window, uri, &nsuri);
    if(!nav_uri)
        IUri_Release(uri);
    if(SUCCEEDED(hres)) {
        if(async_bsc)
            bscallback = async_bsc;
        else
            hres = create_channelbsc(mon, NULL, NULL, 0, TRUE, &bscallback);
    }

    if(SUCCEEDED(hres)) {
        if(window->base.inner_window->doc)
            remove_target_tasks(window->base.inner_window->task_magic);
        abort_window_bindings(window->base.inner_window);

        hres = load_nsuri(window, nsuri, bscallback, LOAD_FLAGS_BYPASS_CACHE);
        nsISupports_Release((nsISupports*)nsuri); /* FIXME */
        if(SUCCEEDED(hres)) {
            hres = create_pending_window(window, bscallback);
            TRACE("pending window for %p %p %p\n", window, bscallback, window->pending_window);
        }
        if(bscallback != async_bsc)
            IBindStatusCallback_Release(&bscallback->bsc.IBindStatusCallback_iface);
    }

    if(FAILED(hres)) {
        CoTaskMemFree(url);
        return hres;
    }

    if(doc_obj) {
        HTMLDocument_LockContainer(doc_obj, TRUE);

        if(doc_obj->frame) {
            docobj_task_t *task;

            task = heap_alloc(sizeof(docobj_task_t));
            task->doc = doc_obj;
            hres = push_task(&task->header, set_progress_proc, NULL, doc_obj->basedoc.task_magic);
            if(FAILED(hres)) {
                CoTaskMemFree(url);
                return hres;
            }
        }

        download_task = heap_alloc(sizeof(download_proc_task_t));
        download_task->doc = doc_obj;
        download_task->set_download = set_download;
        download_task->url = url;
        return push_task(&download_task->header, set_downloading_proc, set_downloading_task_destr, doc_obj->basedoc.task_magic);
    }

    return S_OK;
}

static void notif_readystate(HTMLOuterWindow *window)
{
    window->readystate_pending = FALSE;

    if(window->doc_obj && window->doc_obj->basedoc.window == window)
        call_property_onchanged(&window->doc_obj->basedoc.cp_container, DISPID_READYSTATE);

    fire_event(window->base.inner_window->doc, EVENTID_READYSTATECHANGE, FALSE,
            window->base.inner_window->doc->node.nsnode, NULL, NULL);

    if(window->frame_element)
        fire_event(window->frame_element->element.node.doc, EVENTID_READYSTATECHANGE,
                   TRUE, window->frame_element->element.node.nsnode, NULL, NULL);
}

typedef struct {
    task_t header;
    HTMLOuterWindow *window;
} readystate_task_t;

static void notif_readystate_proc(task_t *_task)
{
    readystate_task_t *task = (readystate_task_t*)_task;
    notif_readystate(task->window);
}

static void notif_readystate_destr(task_t *_task)
{
    readystate_task_t *task = (readystate_task_t*)_task;
    IHTMLWindow2_Release(&task->window->base.IHTMLWindow2_iface);
}

void set_ready_state(HTMLOuterWindow *window, READYSTATE readystate)
{
    READYSTATE prev_state = window->readystate;

    window->readystate = readystate;

    if(window->readystate_locked) {
        readystate_task_t *task;
        HRESULT hres;

        if(window->readystate_pending || prev_state == readystate)
            return;

        task = heap_alloc(sizeof(*task));
        if(!task)
            return;

        IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);
        task->window = window;

        hres = push_task(&task->header, notif_readystate_proc, notif_readystate_destr, window->task_magic);
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

    if(!This->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_QueryInterface(This->nsdoc, &IID_nsIDOMNode, (void**)&nsnode);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMNode failed: %08x\n", nsres);
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

    *str = heap_strdupWtoA(strw);

    nsAString_Finish(&nsstr);

    if(!*str)
        return E_OUTOFMEMORY;
    return S_OK;
}


/**********************************************************
 * IPersistMoniker implementation
 */

static inline HTMLDocument *impl_from_IPersistMoniker(IPersistMoniker *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IPersistMoniker_iface);
}

static HRESULT WINAPI PersistMoniker_QueryInterface(IPersistMoniker *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IPersistMoniker(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI PersistMoniker_AddRef(IPersistMoniker *iface)
{
    HTMLDocument *This = impl_from_IPersistMoniker(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI PersistMoniker_Release(IPersistMoniker *iface)
{
    HTMLDocument *This = impl_from_IPersistMoniker(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI PersistMoniker_GetClassID(IPersistMoniker *iface, CLSID *pClassID)
{
    HTMLDocument *This = impl_from_IPersistMoniker(iface);
    return IPersistFile_GetClassID(&This->IPersistFile_iface, pClassID);
}

static HRESULT WINAPI PersistMoniker_IsDirty(IPersistMoniker *iface)
{
    HTMLDocument *This = impl_from_IPersistMoniker(iface);

    TRACE("(%p)\n", This);

    return IPersistStreamInit_IsDirty(&This->IPersistStreamInit_iface);
}

static HRESULT WINAPI PersistMoniker_Load(IPersistMoniker *iface, BOOL fFullyAvailable,
        IMoniker *pimkName, LPBC pibc, DWORD grfMode)
{
    HTMLDocument *This = impl_from_IPersistMoniker(iface);
    HRESULT hres;

    TRACE("(%p)->(%x %p %p %08x)\n", This, fFullyAvailable, pimkName, pibc, grfMode);

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

    prepare_for_binding(This, pimkName, FALSE);
    call_docview_84(This->doc_obj);
    hres = set_moniker(This->window, pimkName, NULL, pibc, NULL, TRUE);
    if(FAILED(hres))
        return hres;

    return start_binding(This->window->pending_window, (BSCallback*)This->window->pending_window->bscallback, pibc);
}

static HRESULT WINAPI PersistMoniker_Save(IPersistMoniker *iface, IMoniker *pimkName,
        LPBC pbc, BOOL fRemember)
{
    HTMLDocument *This = impl_from_IPersistMoniker(iface);
    FIXME("(%p)->(%p %p %x)\n", This, pimkName, pbc, fRemember);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistMoniker_SaveCompleted(IPersistMoniker *iface, IMoniker *pimkName, LPBC pibc)
{
    HTMLDocument *This = impl_from_IPersistMoniker(iface);
    FIXME("(%p)->(%p %p)\n", This, pimkName, pibc);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistMoniker_GetCurMoniker(IPersistMoniker *iface, IMoniker **ppimkName)
{
    HTMLDocument *This = impl_from_IPersistMoniker(iface);

    TRACE("(%p)->(%p)\n", This, ppimkName);

    if(!This->window || !This->window->mon)
        return E_UNEXPECTED;

    IMoniker_AddRef(This->window->mon);
    *ppimkName = This->window->mon;
    return S_OK;
}

static const IPersistMonikerVtbl PersistMonikerVtbl = {
    PersistMoniker_QueryInterface,
    PersistMoniker_AddRef,
    PersistMoniker_Release,
    PersistMoniker_GetClassID,
    PersistMoniker_IsDirty,
    PersistMoniker_Load,
    PersistMoniker_Save,
    PersistMoniker_SaveCompleted,
    PersistMoniker_GetCurMoniker
};

/**********************************************************
 * IMonikerProp implementation
 */

static inline HTMLDocument *impl_from_IMonikerProp(IMonikerProp *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IMonikerProp_iface);
}

static HRESULT WINAPI MonikerProp_QueryInterface(IMonikerProp *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IMonikerProp(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI MonikerProp_AddRef(IMonikerProp *iface)
{
    HTMLDocument *This = impl_from_IMonikerProp(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI MonikerProp_Release(IMonikerProp *iface)
{
    HTMLDocument *This = impl_from_IMonikerProp(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI MonikerProp_PutProperty(IMonikerProp *iface, MONIKERPROPERTY mkp, LPCWSTR val)
{
    HTMLDocument *This = impl_from_IMonikerProp(iface);

    TRACE("(%p)->(%d %s)\n", This, mkp, debugstr_w(val));

    switch(mkp) {
    case MIMETYPEPROP:
        heap_free(This->doc_obj->mime);
        This->doc_obj->mime = heap_strdupW(val);
        break;

    case CLASSIDPROP:
        break;

    default:
        FIXME("mkp %d\n", mkp);
        return E_NOTIMPL;
    }

    return S_OK;
}

static const IMonikerPropVtbl MonikerPropVtbl = {
    MonikerProp_QueryInterface,
    MonikerProp_AddRef,
    MonikerProp_Release,
    MonikerProp_PutProperty
};

/**********************************************************
 * IPersistFile implementation
 */

static inline HTMLDocument *impl_from_IPersistFile(IPersistFile *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IPersistFile_iface);
}

static HRESULT WINAPI PersistFile_QueryInterface(IPersistFile *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IPersistFile(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI PersistFile_AddRef(IPersistFile *iface)
{
    HTMLDocument *This = impl_from_IPersistFile(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI PersistFile_Release(IPersistFile *iface)
{
    HTMLDocument *This = impl_from_IPersistFile(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI PersistFile_GetClassID(IPersistFile *iface, CLSID *pClassID)
{
    HTMLDocument *This = impl_from_IPersistFile(iface);

    TRACE("(%p)->(%p)\n", This, pClassID);

    if(!pClassID)
        return E_INVALIDARG;

    *pClassID = CLSID_HTMLDocument;
    return S_OK;
}

static HRESULT WINAPI PersistFile_IsDirty(IPersistFile *iface)
{
    HTMLDocument *This = impl_from_IPersistFile(iface);

    TRACE("(%p)\n", This);

    return IPersistStreamInit_IsDirty(&This->IPersistStreamInit_iface);
}

static HRESULT WINAPI PersistFile_Load(IPersistFile *iface, LPCOLESTR pszFileName, DWORD dwMode)
{
    HTMLDocument *This = impl_from_IPersistFile(iface);
    FIXME("(%p)->(%s %08x)\n", This, debugstr_w(pszFileName), dwMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistFile_Save(IPersistFile *iface, LPCOLESTR pszFileName, BOOL fRemember)
{
    HTMLDocument *This = impl_from_IPersistFile(iface);
    char *str;
    DWORD written=0;
    HANDLE file;
    HRESULT hres;

    TRACE("(%p)->(%s %x)\n", This, debugstr_w(pszFileName), fRemember);

    file = CreateFileW(pszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    if(file == INVALID_HANDLE_VALUE) {
        WARN("Could not create file: %u\n", GetLastError());
        return E_FAIL;
    }

    hres = get_doc_string(This->doc_node, &str);
    if(SUCCEEDED(hres))
        WriteFile(file, str, strlen(str), &written, NULL);

    CloseHandle(file);
    return hres;
}

static HRESULT WINAPI PersistFile_SaveCompleted(IPersistFile *iface, LPCOLESTR pszFileName)
{
    HTMLDocument *This = impl_from_IPersistFile(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pszFileName));
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistFile_GetCurFile(IPersistFile *iface, LPOLESTR *pszFileName)
{
    HTMLDocument *This = impl_from_IPersistFile(iface);
    FIXME("(%p)->(%p)\n", This, pszFileName);
    return E_NOTIMPL;
}

static const IPersistFileVtbl PersistFileVtbl = {
    PersistFile_QueryInterface,
    PersistFile_AddRef,
    PersistFile_Release,
    PersistFile_GetClassID,
    PersistFile_IsDirty,
    PersistFile_Load,
    PersistFile_Save,
    PersistFile_SaveCompleted,
    PersistFile_GetCurFile
};

static inline HTMLDocument *impl_from_IPersistStreamInit(IPersistStreamInit *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IPersistStreamInit_iface);
}

static HRESULT WINAPI PersistStreamInit_QueryInterface(IPersistStreamInit *iface,
                                                       REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IPersistStreamInit(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI PersistStreamInit_AddRef(IPersistStreamInit *iface)
{
    HTMLDocument *This = impl_from_IPersistStreamInit(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI PersistStreamInit_Release(IPersistStreamInit *iface)
{
    HTMLDocument *This = impl_from_IPersistStreamInit(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI PersistStreamInit_GetClassID(IPersistStreamInit *iface, CLSID *pClassID)
{
    HTMLDocument *This = impl_from_IPersistStreamInit(iface);
    return IPersistFile_GetClassID(&This->IPersistFile_iface, pClassID);
}

static HRESULT WINAPI PersistStreamInit_IsDirty(IPersistStreamInit *iface)
{
    HTMLDocument *This = impl_from_IPersistStreamInit(iface);

    TRACE("(%p)\n", This);

    if(This->doc_obj->usermode == EDITMODE)
        return editor_is_dirty(This);

    return S_FALSE;
}

static HRESULT WINAPI PersistStreamInit_Load(IPersistStreamInit *iface, LPSTREAM pStm)
{
    HTMLDocument *This = impl_from_IPersistStreamInit(iface);
    IMoniker *mon;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pStm);

    hres = CreateURLMoniker(NULL, about_blankW, &mon);
    if(FAILED(hres)) {
        WARN("CreateURLMoniker failed: %08x\n", hres);
        return hres;
    }

    prepare_for_binding(This, mon, FALSE);
    hres = set_moniker(This->window, mon, NULL, NULL, NULL, TRUE);
    if(FAILED(hres))
        return hres;

    hres = channelbsc_load_stream(This->window->pending_window, mon, pStm);
    IMoniker_Release(mon);
    return hres;
}

static HRESULT WINAPI PersistStreamInit_Save(IPersistStreamInit *iface, LPSTREAM pStm,
                                             BOOL fClearDirty)
{
    HTMLDocument *This = impl_from_IPersistStreamInit(iface);
    char *str;
    DWORD written=0;
    HRESULT hres;

    TRACE("(%p)->(%p %x)\n", This, pStm, fClearDirty);

    hres = get_doc_string(This->doc_node, &str);
    if(FAILED(hres))
        return hres;

    hres = IStream_Write(pStm, str, strlen(str), &written);
    if(FAILED(hres))
        FIXME("Write failed: %08x\n", hres);

    heap_free(str);

    if(fClearDirty)
        set_dirty(This, VARIANT_FALSE);

    return S_OK;
}

static HRESULT WINAPI PersistStreamInit_GetSizeMax(IPersistStreamInit *iface,
                                                   ULARGE_INTEGER *pcbSize)
{
    HTMLDocument *This = impl_from_IPersistStreamInit(iface);
    FIXME("(%p)->(%p)\n", This, pcbSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_InitNew(IPersistStreamInit *iface)
{
    HTMLDocument *This = impl_from_IPersistStreamInit(iface);
    IMoniker *mon;
    HRESULT hres;

    TRACE("(%p)\n", This);

    hres = CreateURLMoniker(NULL, about_blankW, &mon);
    if(FAILED(hres)) {
        WARN("CreateURLMoniker failed: %08x\n", hres);
        return hres;
    }

    prepare_for_binding(This, mon, FALSE);
    hres = set_moniker(This->window, mon, NULL, NULL, NULL, FALSE);
    if(FAILED(hres))
        return hres;

    hres = channelbsc_load_stream(This->window->pending_window, mon, NULL);
    IMoniker_Release(mon);
    return hres;
}

static const IPersistStreamInitVtbl PersistStreamInitVtbl = {
    PersistStreamInit_QueryInterface,
    PersistStreamInit_AddRef,
    PersistStreamInit_Release,
    PersistStreamInit_GetClassID,
    PersistStreamInit_IsDirty,
    PersistStreamInit_Load,
    PersistStreamInit_Save,
    PersistStreamInit_GetSizeMax,
    PersistStreamInit_InitNew
};

/**********************************************************
 * IPersistHistory implementation
 */

static inline HTMLDocument *impl_from_IPersistHistory(IPersistHistory *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IPersistHistory_iface);
}

static HRESULT WINAPI PersistHistory_QueryInterface(IPersistHistory *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IPersistHistory(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI PersistHistory_AddRef(IPersistHistory *iface)
{
    HTMLDocument *This = impl_from_IPersistHistory(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI PersistHistory_Release(IPersistHistory *iface)
{
    HTMLDocument *This = impl_from_IPersistHistory(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI PersistHistory_GetClassID(IPersistHistory *iface, CLSID *pClassID)
{
    HTMLDocument *This = impl_from_IPersistHistory(iface);
    return IPersistFile_GetClassID(&This->IPersistFile_iface, pClassID);
}

static HRESULT WINAPI PersistHistory_LoadHistory(IPersistHistory *iface, IStream *pStream, IBindCtx *pbc)
{
    HTMLDocument *This = impl_from_IPersistHistory(iface);
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

    if(This->doc_obj->client) {
        IOleCommandTarget *cmdtrg = NULL;

        hres = IOleClientSite_QueryInterface(This->doc_obj->client, &IID_IOleCommandTarget,
                (void**)&cmdtrg);
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

    uri_str = heap_alloc((str_len+1)*sizeof(WCHAR));
    if(!uri_str)
        return E_OUTOFMEMORY;

    hres = IStream_Read(pStream, uri_str, str_len*sizeof(WCHAR), &read);
    if(SUCCEEDED(hres) && read != str_len*sizeof(WCHAR))
        hres = E_FAIL;
    if(SUCCEEDED(hres)) {
        uri_str[str_len] = 0;
        hres = create_uri(uri_str, 0, &uri);
    }
    heap_free(uri_str);
    if(FAILED(hres))
        return hres;

    hres = load_uri(This->window, uri, BINDING_FROMHIST);
    IUri_Release(uri);
    return hres;
}

static HRESULT WINAPI PersistHistory_SaveHistory(IPersistHistory *iface, IStream *pStream)
{
    HTMLDocument *This = impl_from_IPersistHistory(iface);
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

static HRESULT WINAPI PersistHistory_SetPositionCookie(IPersistHistory *iface, DWORD dwPositioncookie)
{
    HTMLDocument *This = impl_from_IPersistHistory(iface);
    FIXME("(%p)->(%x)\n", This, dwPositioncookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistHistory_GetPositionCookie(IPersistHistory *iface, DWORD *pdwPositioncookie)
{
    HTMLDocument *This = impl_from_IPersistHistory(iface);
    FIXME("(%p)->(%p)\n", This, pdwPositioncookie);
    return E_NOTIMPL;
}

static const IPersistHistoryVtbl PersistHistoryVtbl = {
    PersistHistory_QueryInterface,
    PersistHistory_AddRef,
    PersistHistory_Release,
    PersistHistory_GetClassID,
    PersistHistory_LoadHistory,
    PersistHistory_SaveHistory,
    PersistHistory_SetPositionCookie,
    PersistHistory_GetPositionCookie
};

void HTMLDocument_Persist_Init(HTMLDocument *This)
{
    This->IPersistMoniker_iface.lpVtbl = &PersistMonikerVtbl;
    This->IPersistFile_iface.lpVtbl = &PersistFileVtbl;
    This->IMonikerProp_iface.lpVtbl = &MonikerPropVtbl;
    This->IPersistStreamInit_iface.lpVtbl = &PersistStreamInitVtbl;
    This->IPersistHistory_iface.lpVtbl = &PersistHistoryVtbl;
}
