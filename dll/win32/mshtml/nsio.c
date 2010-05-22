/*
 * Copyright 2006-2007 Jacek Caban for CodeWeavers
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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"
#include "shlguid.h"
#include "wininet.h"
#include "shlwapi.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define NS_IOSERVICE_CLASSNAME "nsIOService"
#define NS_IOSERVICE_CONTRACTID "@mozilla.org/network/io-service;1"

static const IID NS_IOSERVICE_CID =
    {0x9ac9e770, 0x18bc, 0x11d3, {0x93, 0x37, 0x00, 0x10, 0x4b, 0xa0, 0xfd, 0x40}};
static const IID IID_nsWineURI =
    {0x5088272e, 0x900b, 0x11da, {0xc6,0x87, 0x00,0x0f,0xea,0x57,0xf2,0x1a}};

static nsIIOService *nsio = NULL;
static nsINetUtil *net_util;

static const WCHAR about_blankW[] = {'a','b','o','u','t',':','b','l','a','n','k',0};

struct  nsWineURI {
    const nsIURLVtbl *lpIURLVtbl;

    LONG ref;

    nsIURI *uri;
    nsIURL *nsurl;
    NSContainer *container;
    windowref_t *window_ref;
    nsChannelBSC *channel_bsc;
    LPWSTR wine_url;
    BOOL is_doc_uri;
    BOOL use_wine_url;
};

#define NSURI(x)  ((nsIURI*)  &(x)->lpIURLVtbl)
#define NSURL(x)  ((nsIURL*)  &(x)->lpIURLVtbl)

static nsresult create_uri(nsIURI*,HTMLWindow*,NSContainer*,nsWineURI**);

static const char *debugstr_nsacstr(const nsACString *nsstr)
{
    const char *data;

    nsACString_GetData(nsstr, &data);
    return debugstr_a(data);
}

HRESULT nsuri_to_url(LPCWSTR nsuri, BOOL ret_empty, BSTR *ret)
{
    const WCHAR *ptr = nsuri;

    static const WCHAR wine_prefixW[] = {'w','i','n','e',':'};

    if(!strncmpW(nsuri, wine_prefixW, sizeof(wine_prefixW)/sizeof(WCHAR)))
        ptr += sizeof(wine_prefixW)/sizeof(WCHAR);

    if(*ptr || ret_empty) {
        *ret = SysAllocString(ptr);
        if(!*ret)
            return E_OUTOFMEMORY;
    }else {
        *ret = NULL;
    }

    TRACE("%s -> %s\n", debugstr_w(nsuri), debugstr_w(*ret));
    return S_OK;
}

static BOOL exec_shldocvw_67(HTMLDocumentObj *doc, LPCWSTR url)
{
    IOleCommandTarget *cmdtrg = NULL;
    HRESULT hres;

    hres = IOleClientSite_QueryInterface(doc->client, &IID_IOleCommandTarget, (void**)&cmdtrg);
    if(SUCCEEDED(hres)) {
        VARIANT varUrl, varRes;

        V_VT(&varUrl) = VT_BSTR;
        V_BSTR(&varUrl) = SysAllocString(url);
        V_VT(&varRes) = VT_BOOL;

        hres = IOleCommandTarget_Exec(cmdtrg, &CGID_ShellDocView, 67, 0, &varUrl, &varRes);

        IOleCommandTarget_Release(cmdtrg);
        SysFreeString(V_BSTR(&varUrl));

        if(SUCCEEDED(hres) && !V_BOOL(&varRes)) {
            TRACE("got VARIANT_FALSE, do not load\n");
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL before_async_open(nsChannel *channel, NSContainer *container)
{
    HTMLDocumentObj *doc = container->doc;
    DWORD hlnf = 0;
    BOOL cancel;
    HRESULT hres;

    if(!doc) {
        NSContainer *container_iter = container;

        hlnf = HLNF_OPENINNEWWINDOW;
        while(!container_iter->doc)
            container_iter = container_iter->parent;
        doc = container_iter->doc;
    }

    if(!doc->client)
        return TRUE;

    if(!hlnf && !exec_shldocvw_67(doc, channel->uri->wine_url))
        return FALSE;

    hres = hlink_frame_navigate(&doc->basedoc, channel->uri->wine_url, channel->post_data_stream, hlnf, &cancel);
    return FAILED(hres) || cancel;
}

HRESULT load_nsuri(HTMLWindow *window, nsWineURI *uri, nsChannelBSC *channelbsc, DWORD flags)
{
    nsIWebNavigation *web_navigation;
    nsIDocShell *doc_shell;
    nsresult nsres;

    nsres = get_nsinterface((nsISupports*)window->nswindow, &IID_nsIWebNavigation, (void**)&web_navigation);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIWebNavigation interface: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIWebNavigation_QueryInterface(web_navigation, &IID_nsIDocShell, (void**)&doc_shell);
    nsIWebNavigation_Release(web_navigation);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDocShell: %08x\n", nsres);
        return E_FAIL;
    }


    uri->channel_bsc = channelbsc;
    nsres = nsIDocShell_LoadURI(doc_shell, NSURI(uri), NULL, flags, FALSE);
    uri->channel_bsc = NULL;
    nsIDocShell_Release(doc_shell);
    if(NS_FAILED(nsres)) {
        WARN("LoadURI failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static BOOL translate_url(HTMLDocumentObj *doc, nsWineURI *uri)
{
    OLECHAR *new_url = NULL, *url;
    BOOL ret = FALSE;
    HRESULT hres;

    if(!doc->hostui)
        return FALSE;

    url = heap_strdupW(uri->wine_url);
    hres = IDocHostUIHandler_TranslateUrl(doc->hostui, 0, url, &new_url);
    heap_free(url);
    if(hres != S_OK || !new_url)
        return FALSE;

    if(strcmpW(url, new_url)) {
        FIXME("TranslateUrl returned new URL %s -> %s\n", debugstr_w(url), debugstr_w(new_url));
        ret = TRUE;
    }

    CoTaskMemFree(new_url);
    return ret;
}

nsresult on_start_uri_open(NSContainer *nscontainer, nsIURI *uri, PRBool *_retval)
{
    nsWineURI *wine_uri;
    nsresult nsres;

    *_retval = FALSE;

    nsres = nsIURI_QueryInterface(uri, &IID_nsWineURI, (void**)&wine_uri);
    if(NS_FAILED(nsres)) {
        WARN("Could not get nsWineURI: %08x\n", nsres);
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    if(!wine_uri->is_doc_uri) {
        if(!wine_uri->container) {
            nsIWebBrowserChrome_AddRef(NSWBCHROME(nscontainer));
            wine_uri->container = nscontainer;
        }

        wine_uri->is_doc_uri = TRUE;
        *_retval = translate_url(nscontainer->doc->basedoc.doc_obj, wine_uri);
    }

    nsIURI_Release(NSURI(wine_uri));
    return NS_OK;
}

HRESULT set_wine_url(nsWineURI *This, LPCWSTR url)
{
    static const WCHAR wszFtp[]   = {'f','t','p',':'};
    static const WCHAR wszHttp[]  = {'h','t','t','p',':'};
    static const WCHAR wszHttps[] = {'h','t','t','p','s',':'};

    TRACE("(%p)->(%s)\n", This, debugstr_w(url));

    if(url) {
        WCHAR *new_url;

        new_url = heap_strdupW(url);
        if(!new_url)
            return E_OUTOFMEMORY;
        heap_free(This->wine_url);
        This->wine_url = new_url;

        if(This->uri) {
            /* FIXME: Always use wine url */
            This->use_wine_url =
                   strncmpW(url, wszFtp,   sizeof(wszFtp)/sizeof(WCHAR))
                && strncmpW(url, wszHttp,  sizeof(wszHttp)/sizeof(WCHAR))
                && strncmpW(url, wszHttps, sizeof(wszHttps)/sizeof(WCHAR));
        }else {
            This->use_wine_url = TRUE;
        }
    }else {
        heap_free(This->wine_url);
        This->wine_url = NULL;
        This->use_wine_url = FALSE;
    }

    return S_OK;
}

static void set_uri_nscontainer(nsWineURI *This, NSContainer *nscontainer)
{
    if(This->container) {
        if(This->container == nscontainer)
            return;
        TRACE("Changing %p -> %p\n", This->container, nscontainer);
        nsIWebBrowserChrome_Release(NSWBCHROME(This->container));
    }

    if(nscontainer)
        nsIWebBrowserChrome_AddRef(NSWBCHROME(nscontainer));
    This->container = nscontainer;
}

static void set_uri_window(nsWineURI *This, HTMLWindow *window)
{
    if(This->window_ref) {
        if(This->window_ref->window == window)
            return;
        TRACE("Changing %p -> %p\n", This->window_ref->window, window);
        windowref_release(This->window_ref);
    }

    if(window) {
        windowref_addref(window->window_ref);
        This->window_ref = window->window_ref;

        if(window->doc_obj)
            set_uri_nscontainer(This, window->doc_obj->nscontainer);
    }else {
        This->window_ref = NULL;
    }
}

static inline BOOL is_http_channel(nsChannel *This)
{
    return This->url_scheme == URL_SCHEME_HTTP || This->url_scheme == URL_SCHEME_HTTPS;
}

#define NSCHANNEL_THIS(iface) DEFINE_THIS(nsChannel, HttpChannel, iface)

static nsresult NSAPI nsChannel_QueryInterface(nsIHttpChannel *iface, nsIIDRef riid, nsQIResult result)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports %p)\n", This, result);
        *result = NSCHANNEL(This);
    }else if(IsEqualGUID(&IID_nsIRequest, riid)) {
        TRACE("(%p)->(IID_nsIRequest %p)\n", This, result);
        *result = NSCHANNEL(This);
    }else if(IsEqualGUID(&IID_nsIChannel, riid)) {
        TRACE("(%p)->(IID_nsIChannel %p)\n", This, result);
        *result = NSCHANNEL(This);
    }else if(IsEqualGUID(&IID_nsIHttpChannel, riid)) {
        TRACE("(%p)->(IID_nsIHttpChannel %p)\n", This, result);
        *result = is_http_channel(This) ? NSHTTPCHANNEL(This) : NULL;
    }else if(IsEqualGUID(&IID_nsIUploadChannel, riid)) {
        TRACE("(%p)->(IID_nsIUploadChannel %p)\n", This, result);
        *result = NSUPCHANNEL(This);
    }else if(IsEqualGUID(&IID_nsIHttpChannelInternal, riid)) {
        TRACE("(%p)->(IID_nsIHttpChannelInternal %p)\n", This, result);
        *result = is_http_channel(This) ? NSHTTPINTERNAL(This) : NULL;
    }else {
        TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), result);
        *result = NULL;
    }

    if(*result) {
        nsIChannel_AddRef(NSCHANNEL(This));
        return NS_OK;
    }

    return NS_NOINTERFACE;
}

static nsrefcnt NSAPI nsChannel_AddRef(nsIHttpChannel *iface)
{
    nsChannel *This = NSCHANNEL_THIS(iface);
    nsrefcnt ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static nsrefcnt NSAPI nsChannel_Release(nsIHttpChannel *iface)
{
    nsChannel *This = NSCHANNEL_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    if(!ref) {
        struct ResponseHeader *header, *next_hdr;

        nsIURI_Release(NSURI(This->uri));
        if(This->owner)
            nsISupports_Release(This->owner);
        if(This->post_data_stream)
            nsIInputStream_Release(This->post_data_stream);
        if(This->load_group)
            nsILoadGroup_Release(This->load_group);
        if(This->notif_callback)
            nsIInterfaceRequestor_Release(This->notif_callback);
        if(This->original_uri)
            nsIURI_Release(This->original_uri);
        heap_free(This->content_type);
        heap_free(This->charset);

        LIST_FOR_EACH_ENTRY_SAFE(header, next_hdr, &This->response_headers, struct ResponseHeader, entry) {
            list_remove(&header->entry);
            heap_free(header->header);
            heap_free(header->data);
            heap_free(header);
        }

        heap_free(This);
    }

    return ref;
}

static nsresult NSAPI nsChannel_GetName(nsIHttpChannel *iface, nsACString *aName)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%p)\n", This, aName);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_IsPending(nsIHttpChannel *iface, PRBool *_retval)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%p)\n", This, _retval);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetStatus(nsIHttpChannel *iface, nsresult *aStatus)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    WARN("(%p)->(%p) returning NS_OK\n", This, aStatus);

    return *aStatus = NS_OK;
}

static nsresult NSAPI nsChannel_Cancel(nsIHttpChannel *iface, nsresult aStatus)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%08x)\n", This, aStatus);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_Suspend(nsIHttpChannel *iface)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)\n", This);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_Resume(nsIHttpChannel *iface)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)\n", This);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetLoadGroup(nsIHttpChannel *iface, nsILoadGroup **aLoadGroup)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aLoadGroup);

    if(This->load_group)
        nsILoadGroup_AddRef(This->load_group);

    *aLoadGroup = This->load_group;
    return NS_OK;
}

static nsresult NSAPI nsChannel_SetLoadGroup(nsIHttpChannel *iface, nsILoadGroup *aLoadGroup)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aLoadGroup);

    if(This->load_group)
        nsILoadGroup_Release(This->load_group);
    if(aLoadGroup)
        nsILoadGroup_AddRef(aLoadGroup);
    This->load_group = aLoadGroup;

    return NS_OK;
}

static nsresult NSAPI nsChannel_GetLoadFlags(nsIHttpChannel *iface, nsLoadFlags *aLoadFlags)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aLoadFlags);

    *aLoadFlags = This->load_flags;
    return NS_OK;
}

static nsresult NSAPI nsChannel_SetLoadFlags(nsIHttpChannel *iface, nsLoadFlags aLoadFlags)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, aLoadFlags);

    This->load_flags = aLoadFlags;
    return NS_OK;
}

static nsresult NSAPI nsChannel_GetOriginalURI(nsIHttpChannel *iface, nsIURI **aOriginalURI)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aOriginalURI);

    if(This->original_uri)
        nsIURI_AddRef(This->original_uri);

    *aOriginalURI = This->original_uri;
    return NS_OK;
}

static nsresult NSAPI nsChannel_SetOriginalURI(nsIHttpChannel *iface, nsIURI *aOriginalURI)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aOriginalURI);

    if(This->original_uri)
        nsIURI_Release(This->original_uri);

    nsIURI_AddRef(aOriginalURI);
    This->original_uri = aOriginalURI;
    return NS_OK;
}

static nsresult NSAPI nsChannel_GetURI(nsIHttpChannel *iface, nsIURI **aURI)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aURI);

    nsIURI_AddRef(NSURI(This->uri));
    *aURI = (nsIURI*)This->uri;

    return NS_OK;
}

static nsresult NSAPI nsChannel_GetOwner(nsIHttpChannel *iface, nsISupports **aOwner)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aOwner);

    if(This->owner)
        nsISupports_AddRef(This->owner);
    *aOwner = This->owner;

    return NS_OK;
}

static nsresult NSAPI nsChannel_SetOwner(nsIHttpChannel *iface, nsISupports *aOwner)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aOwner);

    if(aOwner)
        nsISupports_AddRef(aOwner);
    if(This->owner)
        nsISupports_Release(This->owner);
    This->owner = aOwner;

    return NS_OK;
}

static nsresult NSAPI nsChannel_GetNotificationCallbacks(nsIHttpChannel *iface,
        nsIInterfaceRequestor **aNotificationCallbacks)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aNotificationCallbacks);

    if(This->notif_callback)
        nsIInterfaceRequestor_AddRef(This->notif_callback);
    *aNotificationCallbacks = This->notif_callback;

    return NS_OK;
}

static nsresult NSAPI nsChannel_SetNotificationCallbacks(nsIHttpChannel *iface,
        nsIInterfaceRequestor *aNotificationCallbacks)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aNotificationCallbacks);

    if(This->notif_callback)
        nsIInterfaceRequestor_Release(This->notif_callback);
    if(aNotificationCallbacks)
        nsIInterfaceRequestor_AddRef(aNotificationCallbacks);

    This->notif_callback = aNotificationCallbacks;

    return NS_OK;
}

static nsresult NSAPI nsChannel_GetSecurityInfo(nsIHttpChannel *iface, nsISupports **aSecurityInfo)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aSecurityInfo);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetContentType(nsIHttpChannel *iface, nsACString *aContentType)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aContentType);

    if(This->content_type) {
        nsACString_SetData(aContentType, This->content_type);
        return S_OK;
    }

    WARN("unknown type\n");
    return NS_ERROR_FAILURE;
}

static nsresult NSAPI nsChannel_SetContentType(nsIHttpChannel *iface,
                                               const nsACString *aContentType)
{
    nsChannel *This = NSCHANNEL_THIS(iface);
    const char *content_type;

    TRACE("(%p)->(%p)\n", This, aContentType);

    nsACString_GetData(aContentType, &content_type);

    TRACE("content_type %s\n", content_type);

    heap_free(This->content_type);
    This->content_type = heap_strdupA(content_type);

    return NS_OK;
}

static nsresult NSAPI nsChannel_GetContentCharset(nsIHttpChannel *iface,
                                                  nsACString *aContentCharset)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aContentCharset);

    if(This->charset) {
        nsACString_SetData(aContentCharset, This->charset);
        return NS_OK;
    }

    nsACString_SetData(aContentCharset, "");
    return NS_OK;
}

static nsresult NSAPI nsChannel_SetContentCharset(nsIHttpChannel *iface,
                                                  const nsACString *aContentCharset)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%p)\n", This, aContentCharset);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetContentLength(nsIHttpChannel *iface, PRInt32 *aContentLength)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%p)\n", This, aContentLength);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetContentLength(nsIHttpChannel *iface, PRInt32 aContentLength)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%d)\n", This, aContentLength);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_Open(nsIHttpChannel *iface, nsIInputStream **_retval)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%p)\n", This, _retval);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static HRESULT create_mon_for_nschannel(nsChannel *channel, IMoniker **mon)
{
    nsWineURI *wine_uri;
    nsresult nsres;
    HRESULT hres;

    if(!channel->original_uri) {
        ERR("original_uri == NULL\n");
        return E_FAIL;
    }

    nsres = nsIURI_QueryInterface(channel->original_uri, &IID_nsWineURI, (void**)&wine_uri);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsWineURI: %08x\n", nsres);
        return E_FAIL;
    }

    if(wine_uri->wine_url) {
        hres = CreateURLMoniker(NULL, wine_uri->wine_url, mon);
        if(FAILED(hres))
            WARN("CreateURLMoniker failed: %08x\n", hres);
    }else {
        TRACE("wine_url == NULL\n");
        hres = E_FAIL;
    }

    nsIURI_Release(NSURI(wine_uri));

    return hres;
}

static HTMLWindow *get_window_from_load_group(nsChannel *This)
{
    HTMLWindow *window;
    nsIChannel *channel;
    nsIRequest *req;
    nsWineURI *wine_uri;
    nsIURI *uri;
    nsresult nsres;

    nsres = nsILoadGroup_GetDefaultLoadRequest(This->load_group, &req);
    if(NS_FAILED(nsres)) {
        ERR("GetDefaultLoadRequest failed: %08x\n", nsres);
        return NULL;
    }

    if(!req)
        return NULL;

    nsres = nsIRequest_QueryInterface(req, &IID_nsIChannel, (void**)&channel);
    nsIRequest_Release(req);
    if(NS_FAILED(nsres)) {
        WARN("Could not get nsIChannel interface: %08x\n", nsres);
        return NULL;
    }

    nsres = nsIChannel_GetURI(channel, &uri);
    nsIChannel_Release(channel);
    if(NS_FAILED(nsres)) {
        ERR("GetURI failed: %08x\n", nsres);
        return NULL;
    }

    nsres = nsIURI_QueryInterface(uri, &IID_nsWineURI, (void**)&wine_uri);
    nsIURI_Release(uri);
    if(NS_FAILED(nsres)) {
        TRACE("Could not get nsWineURI: %08x\n", nsres);
        return NULL;
    }

    window = wine_uri->window_ref ? wine_uri->window_ref->window : NULL;
    if(window)
        IHTMLWindow2_AddRef(HTMLWINDOW2(window));
    nsIURI_Release(NSURI(wine_uri));

    return window;
}

static HTMLWindow *get_channel_window(nsChannel *This)
{
    nsIRequestObserver *req_observer;
    nsIWebProgress *web_progress;
    nsIDOMWindow *nswindow;
    HTMLWindow *window;
    nsresult nsres;

    if(!This->load_group) {
        ERR("NULL load_group\n");
        return NULL;
    }

    nsres = nsILoadGroup_GetGroupObserver(This->load_group, &req_observer);
    if(NS_FAILED(nsres) || !req_observer) {
        ERR("GetGroupObserver failed: %08x\n", nsres);
        return NULL;
    }

    nsres = nsIRequestObserver_QueryInterface(req_observer, &IID_nsIWebProgress, (void**)&web_progress);
    nsIRequestObserver_Release(req_observer);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIWebProgress iface: %08x\n", nsres);
        return NULL;
    }

    nsres = nsIWebProgress_GetDOMWindow(web_progress, &nswindow);
    nsIWebProgress_Release(web_progress);
    if(NS_FAILED(nsres) || !nswindow) {
        ERR("GetDOMWindow failed: %08x\n", nsres);
        return NULL;
    }

    window = nswindow_to_window(nswindow);
    nsIDOMWindow_Release(nswindow);

    if(window)
        IHTMLWindow2_AddRef(HTMLWINDOW2(window));
    else
        FIXME("NULL window for %p\n", nswindow);
    return window;
}

typedef struct {
    task_t header;
    HTMLDocumentNode *doc;
    nsChannelBSC *bscallback;
} start_binding_task_t;

static void start_binding_proc(task_t *_task)
{
    start_binding_task_t *task = (start_binding_task_t*)_task;

    start_binding(NULL, task->doc, (BSCallback*)task->bscallback, NULL);

    IUnknown_Release((IUnknown*)task->bscallback);
}

static nsresult async_open(nsChannel *This, HTMLWindow *window, BOOL is_doc_channel, nsIStreamListener *listener,
        nsISupports *context)
{
    nsChannelBSC *bscallback;
    IMoniker *mon = NULL;
    HRESULT hres;

    hres = create_mon_for_nschannel(This, &mon);
    if(FAILED(hres))
        return NS_ERROR_UNEXPECTED;

    if(is_doc_channel)
        set_current_mon(window, mon);

    hres = create_channelbsc(mon, NULL, NULL, 0, &bscallback);
    IMoniker_Release(mon);
    if(FAILED(hres))
        return NS_ERROR_UNEXPECTED;

    channelbsc_set_channel(bscallback, This, listener, context);

    if(is_doc_channel) {
        set_window_bscallback(window, bscallback);
        async_start_doc_binding(window, bscallback);
        IUnknown_Release((IUnknown*)bscallback);
    }else {
        start_binding_task_t *task = heap_alloc(sizeof(start_binding_task_t));

        task->doc = window->doc;
        task->bscallback = bscallback;
        push_task(&task->header, start_binding_proc, window->doc->basedoc.task_magic);
    }

    return NS_OK;
}

static nsresult NSAPI nsChannel_AsyncOpen(nsIHttpChannel *iface, nsIStreamListener *aListener,
                                          nsISupports *aContext)
{
    nsChannel *This = NSCHANNEL_THIS(iface);
    HTMLWindow *window = NULL;
    BOOL open = TRUE;
    nsresult nsres = NS_OK;

    TRACE("(%p)->(%p %p) opening %s\n", This, aListener, aContext, debugstr_w(This->uri->wine_url));

    if(This->uri->is_doc_uri) {
        window = get_channel_window(This);
        if(window) {
            set_uri_window(This->uri, window);
        }else if(This->uri->container) {
            BOOL b;

            /* nscontainer->doc should be NULL which means navigation to a new window */
            if(This->uri->container->doc)
                FIXME("nscontainer->doc = %p\n", This->uri->container->doc);

            b = before_async_open(This, This->uri->container);
            if(b)
                FIXME("Navigation not cancelled\n");
            return NS_ERROR_UNEXPECTED;
        }
    }

    if(!window) {
        if(This->uri->window_ref && This->uri->window_ref->window) {
            window = This->uri->window_ref->window;
            IHTMLWindow2_AddRef(HTMLWINDOW2(window));
        }else if(This->load_group) {
            window = get_window_from_load_group(This);
            if(window)
                set_uri_window(This->uri, window);
        }
    }

    if(!window) {
        ERR("window = NULL\n");
        return NS_ERROR_UNEXPECTED;
    }

    if(This->uri->is_doc_uri && window == window->doc_obj->basedoc.window) {
        if(This->uri->channel_bsc) {
            channelbsc_set_channel(This->uri->channel_bsc, This, aListener, aContext);

            if(window->doc_obj->mime) {
                heap_free(This->content_type);
                This->content_type = heap_strdupWtoA(window->doc_obj->mime);
            }

            open = FALSE;
        }else {
            open = !before_async_open(This, window->doc_obj->nscontainer);
            if(!open) {
                TRACE("canceled\n");
                nsres = NS_ERROR_UNEXPECTED;
            }
        }
    }

    if(open)
        nsres = async_open(This, window, This->uri->is_doc_uri, aListener, aContext);

    IHTMLWindow2_Release(HTMLWINDOW2(window));
    return nsres;
}

static nsresult NSAPI nsChannel_GetRequestMethod(nsIHttpChannel *iface, nsACString *aRequestMethod)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%p)\n", This, aRequestMethod);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetRequestMethod(nsIHttpChannel *iface,
                                                 const nsACString *aRequestMethod)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p): Returning NS_OK\n", This, aRequestMethod);

    return NS_OK;
}

static nsresult NSAPI nsChannel_GetReferrer(nsIHttpChannel *iface, nsIURI **aReferrer)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%p)\n", This, aReferrer);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetReferrer(nsIHttpChannel *iface, nsIURI *aReferrer)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%p)\n", This, aReferrer);

    return NS_OK;
}

static nsresult NSAPI nsChannel_GetRequestHeader(nsIHttpChannel *iface,
         const nsACString *aHeader, nsACString *_retval)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%p %p)\n", This, aHeader, _retval);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetRequestHeader(nsIHttpChannel *iface,
         const nsACString *aHeader, const nsACString *aValue, PRBool aMerge)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%p %p %x)\n", This, aHeader, aValue, aMerge);

    return NS_OK;
}

static nsresult NSAPI nsChannel_VisitRequestHeaders(nsIHttpChannel *iface,
                                                    nsIHttpHeaderVisitor *aVisitor)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%p)\n", This, aVisitor);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetAllowPipelining(nsIHttpChannel *iface, PRBool *aAllowPipelining)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%p)\n", This, aAllowPipelining);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetAllowPipelining(nsIHttpChannel *iface, PRBool aAllowPipelining)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%x)\n", This, aAllowPipelining);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetRedirectionLimit(nsIHttpChannel *iface, PRUint32 *aRedirectionLimit)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%p)\n", This, aRedirectionLimit);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetRedirectionLimit(nsIHttpChannel *iface, PRUint32 aRedirectionLimit)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%u)\n", This, aRedirectionLimit);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetResponseStatus(nsIHttpChannel *iface, PRUint32 *aResponseStatus)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aResponseStatus);

    if(This->response_status) {
        *aResponseStatus = This->response_status;
        return NS_OK;
    }

    WARN("No response status\n");
    return NS_ERROR_UNEXPECTED;
}

static nsresult NSAPI nsChannel_GetResponseStatusText(nsIHttpChannel *iface,
                                                      nsACString *aResponseStatusText)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%p)\n", This, aResponseStatusText);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetRequestSucceeded(nsIHttpChannel *iface,
                                                    PRBool *aRequestSucceeded)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aRequestSucceeded);

    if(!This->response_status)
        return NS_ERROR_NOT_AVAILABLE;

    *aRequestSucceeded = This->response_status/100 == 2;

    return NS_OK;
}

static nsresult NSAPI nsChannel_GetResponseHeader(nsIHttpChannel *iface,
         const nsACString *header, nsACString *_retval)
{
    nsChannel *This = NSCHANNEL_THIS(iface);
    const char *header_str;
    WCHAR *header_wstr;
    struct ResponseHeader *this_header;

    nsACString_GetData(header, &header_str);
    TRACE("(%p)->(%p(%s) %p)\n", This, header, header_str, _retval);

    header_wstr = heap_strdupAtoW(header_str);
    if(!header_wstr)
        return NS_ERROR_UNEXPECTED;

    LIST_FOR_EACH_ENTRY(this_header, &This->response_headers, struct ResponseHeader, entry) {
        if(!strcmpW(this_header->header, header_wstr)) {
            char *data = heap_strdupWtoA(this_header->data);
            if(!data) {
                heap_free(header_wstr);
                return NS_ERROR_UNEXPECTED;
            }
            nsACString_SetData(_retval, data);
            heap_free(data);
            heap_free(header_wstr);
            return NS_OK;
        }
    }

    heap_free(header_wstr);

    return NS_ERROR_NOT_AVAILABLE;
}

static nsresult NSAPI nsChannel_SetResponseHeader(nsIHttpChannel *iface,
        const nsACString *header, const nsACString *value, PRBool merge)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%p %p %x)\n", This, header, value, merge);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_VisitResponseHeaders(nsIHttpChannel *iface,
        nsIHttpHeaderVisitor *aVisitor)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%p)\n", This, aVisitor);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_IsNoStoreResponse(nsIHttpChannel *iface, PRBool *_retval)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%p)\n", This, _retval);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_IsNoCacheResponse(nsIHttpChannel *iface, PRBool *_retval)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    FIXME("(%p)->(%p)\n", This, _retval);

    return NS_ERROR_NOT_IMPLEMENTED;
}

#undef NSCHANNEL_THIS

static const nsIHttpChannelVtbl nsChannelVtbl = {
    nsChannel_QueryInterface,
    nsChannel_AddRef,
    nsChannel_Release,
    nsChannel_GetName,
    nsChannel_IsPending,
    nsChannel_GetStatus,
    nsChannel_Cancel,
    nsChannel_Suspend,
    nsChannel_Resume,
    nsChannel_GetLoadGroup,
    nsChannel_SetLoadGroup,
    nsChannel_GetLoadFlags,
    nsChannel_SetLoadFlags,
    nsChannel_GetOriginalURI,
    nsChannel_SetOriginalURI,
    nsChannel_GetURI,
    nsChannel_GetOwner,
    nsChannel_SetOwner,
    nsChannel_GetNotificationCallbacks,
    nsChannel_SetNotificationCallbacks,
    nsChannel_GetSecurityInfo,
    nsChannel_GetContentType,
    nsChannel_SetContentType,
    nsChannel_GetContentCharset,
    nsChannel_SetContentCharset,
    nsChannel_GetContentLength,
    nsChannel_SetContentLength,
    nsChannel_Open,
    nsChannel_AsyncOpen,
    nsChannel_GetRequestMethod,
    nsChannel_SetRequestMethod,
    nsChannel_GetReferrer,
    nsChannel_SetReferrer,
    nsChannel_GetRequestHeader,
    nsChannel_SetRequestHeader,
    nsChannel_VisitRequestHeaders,
    nsChannel_GetAllowPipelining,
    nsChannel_SetAllowPipelining,
    nsChannel_GetRedirectionLimit,
    nsChannel_SetRedirectionLimit,
    nsChannel_GetResponseStatus,
    nsChannel_GetResponseStatusText,
    nsChannel_GetRequestSucceeded,
    nsChannel_GetResponseHeader,
    nsChannel_SetResponseHeader,
    nsChannel_VisitResponseHeaders,
    nsChannel_IsNoStoreResponse,
    nsChannel_IsNoCacheResponse
};

#define NSUPCHANNEL_THIS(iface) DEFINE_THIS(nsChannel, UploadChannel, iface)

static nsresult NSAPI nsUploadChannel_QueryInterface(nsIUploadChannel *iface, nsIIDRef riid,
                                                     nsQIResult result)
{
    nsChannel *This = NSUPCHANNEL_THIS(iface);
    return nsIChannel_QueryInterface(NSCHANNEL(This), riid, result);
}

static nsrefcnt NSAPI nsUploadChannel_AddRef(nsIUploadChannel *iface)
{
    nsChannel *This = NSUPCHANNEL_THIS(iface);
    return nsIChannel_AddRef(NSCHANNEL(This));
}

static nsrefcnt NSAPI nsUploadChannel_Release(nsIUploadChannel *iface)
{
    nsChannel *This = NSUPCHANNEL_THIS(iface);
    return nsIChannel_Release(NSCHANNEL(This));
}

static nsresult NSAPI nsUploadChannel_SetUploadStream(nsIUploadChannel *iface,
        nsIInputStream *aStream, const nsACString *aContentType, PRInt32 aContentLength)
{
    nsChannel *This = NSUPCHANNEL_THIS(iface);
    const char *content_type;

    TRACE("(%p)->(%p %p %d)\n", This, aStream, aContentType, aContentLength);

    if(This->post_data_stream)
        nsIInputStream_Release(This->post_data_stream);

    if(aContentType) {
        nsACString_GetData(aContentType, &content_type);
        if(*content_type)
            FIXME("Unsupported aContentType argument: %s\n", debugstr_a(content_type));
    }

    if(aContentLength != -1)
        FIXME("Unsupported acontentLength = %d\n", aContentLength);

    if(This->post_data_stream)
        nsIInputStream_Release(This->post_data_stream);
    This->post_data_stream = aStream;
    if(aStream)
        nsIInputStream_AddRef(aStream);

    return NS_OK;
}

static nsresult NSAPI nsUploadChannel_GetUploadStream(nsIUploadChannel *iface,
        nsIInputStream **aUploadStream)
{
    nsChannel *This = NSUPCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aUploadStream);

    if(This->post_data_stream)
        nsIInputStream_AddRef(This->post_data_stream);

    *aUploadStream = This->post_data_stream;
    return NS_OK;
}

#undef NSUPCHANNEL_THIS

static const nsIUploadChannelVtbl nsUploadChannelVtbl = {
    nsUploadChannel_QueryInterface,
    nsUploadChannel_AddRef,
    nsUploadChannel_Release,
    nsUploadChannel_SetUploadStream,
    nsUploadChannel_GetUploadStream
};

#define NSHTTPINTERNAL_THIS(iface) DEFINE_THIS(nsChannel, IHttpChannelInternal, iface)

static nsresult NSAPI nsHttpChannelInternal_QueryInterface(nsIHttpChannelInternal *iface, nsIIDRef riid,
        nsQIResult result)
{
    nsChannel *This = NSHTTPINTERNAL_THIS(iface);
    return nsIChannel_QueryInterface(NSCHANNEL(This), riid, result);
}

static nsrefcnt NSAPI nsHttpChannelInternal_AddRef(nsIHttpChannelInternal *iface)
{
    nsChannel *This = NSHTTPINTERNAL_THIS(iface);
    return nsIChannel_AddRef(NSCHANNEL(This));
}

static nsrefcnt NSAPI nsHttpChannelInternal_Release(nsIHttpChannelInternal *iface)
{
    nsChannel *This = NSHTTPINTERNAL_THIS(iface);
    return nsIChannel_Release(NSCHANNEL(This));
}

static nsresult NSAPI nsHttpChannelInternal_GetDocumentURI(nsIHttpChannelInternal *iface, nsIURI **aDocumentURI)
{
    nsChannel *This = NSHTTPINTERNAL_THIS(iface);

    FIXME("(%p)->()\n", This);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_SetDocumentURI(nsIHttpChannelInternal *iface, nsIURI *aDocumentURI)
{
    nsChannel *This = NSHTTPINTERNAL_THIS(iface);

    FIXME("(%p)->()\n", This);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetRequestVersion(nsIHttpChannelInternal *iface, PRUint32 *major, PRUint32 *minor)
{
    nsChannel *This = NSHTTPINTERNAL_THIS(iface);

    FIXME("(%p)->()\n", This);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetResponseVersion(nsIHttpChannelInternal *iface, PRUint32 *major, PRUint32 *minor)
{
    nsChannel *This = NSHTTPINTERNAL_THIS(iface);

    FIXME("(%p)->()\n", This);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_SetCookie(nsIHttpChannelInternal *iface, const char *aCookieHeader)
{
    nsChannel *This = NSHTTPINTERNAL_THIS(iface);

    FIXME("(%p)->()\n", This);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_SetupFallbackChannel(nsIHttpChannelInternal *iface, const char *aFallbackKey)
{
    nsChannel *This = NSHTTPINTERNAL_THIS(iface);

    FIXME("(%p)->()\n", This);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetForceAllowThirdPartyCookie(nsIHttpChannelInternal *iface, PRBool *aForceThirdPartyCookie)
{
    nsChannel *This = NSHTTPINTERNAL_THIS(iface);

    FIXME("(%p)->()\n", This);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_SetForceAllowThirdPartyCookie(nsIHttpChannelInternal *iface, PRBool aForceThirdPartyCookie)
{
    nsChannel *This = NSHTTPINTERNAL_THIS(iface);

    FIXME("(%p)->()\n", This);

    return NS_ERROR_NOT_IMPLEMENTED;
}

#undef NSHTTPINTERNAL_THIS

static const nsIHttpChannelInternalVtbl nsHttpChannelInternalVtbl = {
    nsHttpChannelInternal_QueryInterface,
    nsHttpChannelInternal_AddRef,
    nsHttpChannelInternal_Release,
    nsHttpChannelInternal_GetDocumentURI,
    nsHttpChannelInternal_SetDocumentURI,
    nsHttpChannelInternal_GetRequestVersion,
    nsHttpChannelInternal_GetResponseVersion,
    nsHttpChannelInternal_SetCookie,
    nsHttpChannelInternal_SetupFallbackChannel,
    nsHttpChannelInternal_GetForceAllowThirdPartyCookie,
    nsHttpChannelInternal_SetForceAllowThirdPartyCookie
};

#define NSURI_THIS(iface) DEFINE_THIS(nsWineURI, IURL, iface)

static nsresult NSAPI nsURI_QueryInterface(nsIURL *iface, nsIIDRef riid, nsQIResult result)
{
    nsWineURI *This = NSURI_THIS(iface);

    *result = NULL;

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports %p)\n", This, result);
        *result = NSURI(This);
    }else if(IsEqualGUID(&IID_nsIURI, riid)) {
        TRACE("(%p)->(IID_nsIURI %p)\n", This, result);
        *result = NSURI(This);
    }else if(IsEqualGUID(&IID_nsIURL, riid)) {
        TRACE("(%p)->(IID_nsIURL %p)\n", This, result);
        *result = NSURL(This);
    }else if(IsEqualGUID(&IID_nsWineURI, riid)) {
        TRACE("(%p)->(IID_nsWineURI %p)\n", This, result);
        *result = This;
    }

    if(*result) {
        nsIURI_AddRef(NSURI(This));
        return NS_OK;
    }

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), result);
    return This->uri ? nsIURI_QueryInterface(This->uri, riid, result) : NS_NOINTERFACE;
}

static nsrefcnt NSAPI nsURI_AddRef(nsIURL *iface)
{
    nsWineURI *This = NSURI_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static nsrefcnt NSAPI nsURI_Release(nsIURL *iface)
{
    nsWineURI *This = NSURI_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->window_ref)
            windowref_release(This->window_ref);
        if(This->container)
            nsIWebBrowserChrome_Release(NSWBCHROME(This->container));
        if(This->nsurl)
            nsIURL_Release(This->nsurl);
        if(This->uri)
            nsIURI_Release(This->uri);
        heap_free(This->wine_url);
        heap_free(This);
    }

    return ref;
}

static nsresult NSAPI nsURI_GetSpec(nsIURL *iface, nsACString *aSpec)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aSpec);

    if(This->use_wine_url) {
        char speca[INTERNET_MAX_URL_LENGTH] = "wine:";
        WideCharToMultiByte(CP_ACP, 0, This->wine_url, -1, speca+5, sizeof(speca)-5, NULL, NULL);
        nsACString_SetData(aSpec, speca);

        return NS_OK;
    }

    if(This->uri)
        return nsIURI_GetSpec(This->uri, aSpec);

    TRACE("returning error\n");
    return NS_ERROR_NOT_IMPLEMENTED;

}

static nsresult NSAPI nsURI_SetSpec(nsIURL *iface, const nsACString *aSpec)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aSpec);

    if(This->uri)
        return nsIURI_SetSpec(This->uri, aSpec);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetPrePath(nsIURL *iface, nsACString *aPrePath)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aPrePath);

    if(This->uri)
        return nsIURI_GetPrePath(This->uri, aPrePath);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetScheme(nsIURL *iface, nsACString *aScheme)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aScheme);

    if(This->use_wine_url) {
        /*
         * For Gecko we set scheme to unknown so it won't be handled
         * as any special case.
         */
        nsACString_SetData(aScheme, "wine");
        return NS_OK;
    }

    if(This->uri)
        return nsIURI_GetScheme(This->uri, aScheme);

    TRACE("returning error\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_SetScheme(nsIURL *iface, const nsACString *aScheme)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aScheme);

    if(This->uri)
        return nsIURI_SetScheme(This->uri, aScheme);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetUserPass(nsIURL *iface, nsACString *aUserPass)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aUserPass);

    if(This->uri)
        return nsIURI_GetUserPass(This->uri, aUserPass);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_SetUserPass(nsIURL *iface, const nsACString *aUserPass)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aUserPass);

    if(This->uri)
        return nsIURI_SetUserPass(This->uri, aUserPass);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetUsername(nsIURL *iface, nsACString *aUsername)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aUsername);

    if(This->uri)
        return nsIURI_GetUsername(This->uri, aUsername);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_SetUsername(nsIURL *iface, const nsACString *aUsername)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aUsername);

    if(This->uri)
        return nsIURI_SetUsername(This->uri, aUsername);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetPassword(nsIURL *iface, nsACString *aPassword)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aPassword);

    if(This->uri)
        return nsIURI_GetPassword(This->uri, aPassword);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_SetPassword(nsIURL *iface, const nsACString *aPassword)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aPassword);

    if(This->uri)
        return nsIURI_SetPassword(This->uri, aPassword);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetHostPort(nsIURL *iface, nsACString *aHostPort)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aHostPort);

    if(This->uri)
        return nsIURI_GetHostPort(This->uri, aHostPort);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_SetHostPort(nsIURL *iface, const nsACString *aHostPort)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aHostPort);

    if(This->uri)
        return nsIURI_SetHostPort(This->uri, aHostPort);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetHost(nsIURL *iface, nsACString *aHost)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aHost);

    if(This->uri)
        return nsIURI_GetHost(This->uri, aHost);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_SetHost(nsIURL *iface, const nsACString *aHost)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aHost);

    if(This->uri)
        return nsIURI_SetHost(This->uri, aHost);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetPort(nsIURL *iface, PRInt32 *aPort)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aPort);

    if(This->uri)
        return nsIURI_GetPort(This->uri, aPort);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_SetPort(nsIURL *iface, PRInt32 aPort)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%d)\n", This, aPort);

    if(This->uri)
        return nsIURI_SetPort(This->uri, aPort);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetPath(nsIURL *iface, nsACString *aPath)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aPath);

    if(This->uri)
        return nsIURI_GetPath(This->uri, aPath);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_SetPath(nsIURL *iface, const nsACString *aPath)
{
    nsWineURI *This = NSURI_THIS(iface);
    const char *path;

    nsACString_GetData(aPath, &path);
    TRACE("(%p)->(%p(%s))\n", This, aPath, debugstr_a(path));


    if(This->wine_url) {
        WCHAR new_url[INTERNET_MAX_URL_LENGTH];
        DWORD size = sizeof(new_url)/sizeof(WCHAR);
        LPWSTR pathw;
        HRESULT hres;

        pathw = heap_strdupAtoW(path);
        hres = UrlCombineW(This->wine_url, pathw, new_url, &size, 0);
        heap_free(pathw);
        if(SUCCEEDED(hres))
            set_wine_url(This, new_url);
        else
            WARN("UrlCombine failed: %08x\n", hres);
    }

    if(!This->uri)
        return NS_OK;

    return nsIURI_SetPath(This->uri, aPath);
}

static nsresult NSAPI nsURI_Equals(nsIURL *iface, nsIURI *other, PRBool *_retval)
{
    nsWineURI *This = NSURI_THIS(iface);
    nsWineURI *wine_uri;
    nsresult nsres;

    TRACE("(%p)->(%p %p)\n", This, other, _retval);

    if(This->uri)
        return nsIURI_Equals(This->uri, other, _retval);

    nsres = nsIURI_QueryInterface(other, &IID_nsWineURI, (void**)&wine_uri);
    if(NS_FAILED(nsres)) {
        TRACE("Could not get nsWineURI interface\n");
        *_retval = FALSE;
        return NS_OK;
    }

    *_retval = wine_uri->wine_url && !UrlCompareW(This->wine_url, wine_uri->wine_url, TRUE);
    nsIURI_Release(NSURI(wine_uri));

    return NS_OK;
}

static nsresult NSAPI nsURI_SchemeIs(nsIURL *iface, const char *scheme, PRBool *_retval)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_a(scheme), _retval);

    if(This->use_wine_url) {
        WCHAR buf[INTERNET_MAX_SCHEME_LENGTH];
        int len = MultiByteToWideChar(CP_ACP, 0, scheme, -1, buf, sizeof(buf)/sizeof(WCHAR))-1;

        *_retval = lstrlenW(This->wine_url) > len
            && This->wine_url[len] == ':'
            && !memcmp(buf, This->wine_url, len*sizeof(WCHAR));
        return NS_OK;
    }

    if(This->uri)
        return nsIURI_SchemeIs(This->uri, scheme, _retval);

    TRACE("returning error\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_Clone(nsIURL *iface, nsIURI **_retval)
{
    nsWineURI *This = NSURI_THIS(iface);
    nsIURI *nsuri = NULL;
    nsWineURI *wine_uri;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, _retval);

    if(This->uri) {
        nsres = nsIURI_Clone(This->uri, &nsuri);
        if(NS_FAILED(nsres)) {
            WARN("Clone failed: %08x\n", nsres);
            return nsres;
        }
    }

    nsres = create_uri(nsuri, This->window_ref ? This->window_ref->window : NULL, This->container, &wine_uri);
    if(NS_FAILED(nsres)) {
        WARN("create_uri failed: %08x\n", nsres);
        return nsres;
    }

    set_wine_url(wine_uri, This->wine_url);

    *_retval = NSURI(wine_uri);
    return NS_OK;
}

static nsresult NSAPI nsURI_Resolve(nsIURL *iface, const nsACString *arelativePath,
        nsACString *_retval)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p %p)\n", This, arelativePath, _retval);

    if(This->uri)
        return nsIURI_Resolve(This->uri, arelativePath, _retval);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetAsciiSpec(nsIURL *iface, nsACString *aAsciiSpec)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aAsciiSpec);

    if(This->use_wine_url)
        return nsIURI_GetSpec(NSURI(This), aAsciiSpec);

    if(This->uri)
        return nsIURI_GetAsciiSpec(This->uri, aAsciiSpec);

    TRACE("returning error\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetAsciiHost(nsIURL *iface, nsACString *aAsciiHost)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aAsciiHost);

    if(This->uri)
        return nsIURI_GetAsciiHost(This->uri, aAsciiHost);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetOriginCharset(nsIURL *iface, nsACString *aOriginCharset)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aOriginCharset);

    if(This->uri)
        return nsIURI_GetOriginCharset(This->uri, aOriginCharset);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_GetFilePath(nsIURL *iface, nsACString *aFilePath)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aFilePath);

    if(This->nsurl)
        return nsIURL_GetFilePath(This->nsurl, aFilePath);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_SetFilePath(nsIURL *iface, const nsACString *aFilePath)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_nsacstr(aFilePath));

    if(This->nsurl)
        return nsIURL_SetFilePath(This->nsurl, aFilePath);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_GetParam(nsIURL *iface, nsACString *aParam)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aParam);

    if(This->nsurl)
        return nsIURL_GetParam(This->nsurl, aParam);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_SetParam(nsIURL *iface, const nsACString *aParam)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_nsacstr(aParam));

    if(This->nsurl)
        return nsIURL_SetParam(This->nsurl, aParam);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_GetQuery(nsIURL *iface, nsACString *aQuery)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aQuery);

    if(This->nsurl)
        return nsIURL_GetQuery(This->nsurl, aQuery);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_SetQuery(nsIURL *iface, const nsACString *aQuery)
{
    nsWineURI *This = NSURI_THIS(iface);
    const WCHAR *ptr1, *ptr2;
    const char *query;
    WCHAR *new_url, *ptr;
    DWORD len, size;

    TRACE("(%p)->(%s)\n", This, debugstr_nsacstr(aQuery));

    if(This->nsurl)
        nsIURL_SetQuery(This->nsurl, aQuery);

    if(!This->wine_url)
        return NS_OK;

    nsACString_GetData(aQuery, &query);
    size = len = MultiByteToWideChar(CP_ACP, 0, query, -1, NULL, 0);
    ptr1 = strchrW(This->wine_url, '?');
    if(ptr1) {
        size += ptr1-This->wine_url;
        ptr2 = strchrW(ptr1, '#');
        if(ptr2)
            size += strlenW(ptr2);
    }else {
        ptr1 = This->wine_url + strlenW(This->wine_url);
        ptr2 = NULL;
        size += strlenW(This->wine_url);
    }

    if(*query)
        size++;

    new_url = heap_alloc(size*sizeof(WCHAR));
    memcpy(new_url, This->wine_url, (ptr1-This->wine_url)*sizeof(WCHAR));
    ptr = new_url + (ptr1-This->wine_url);
    if(*query) {
        *ptr++ = '?';
        MultiByteToWideChar(CP_ACP, 0, query, -1, ptr, len);
        ptr += len-1;
    }
    if(ptr2)
        strcpyW(ptr, ptr2);
    else
        *ptr = 0;

    TRACE("setting %s\n", debugstr_w(new_url));

    heap_free(This->wine_url);
    This->wine_url = new_url;
    return NS_OK;
}

static nsresult NSAPI nsURL_GetRef(nsIURL *iface, nsACString *aRef)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aRef);

    if(This->nsurl)
        return nsIURL_GetRef(This->nsurl, aRef);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_SetRef(nsIURL *iface, const nsACString *aRef)
{
    nsWineURI *This = NSURI_THIS(iface);
    const char *refa;

    TRACE("(%p)->(%s)\n", This, debugstr_nsacstr(aRef));

    if(This->nsurl)
        return nsIURL_SetRef(This->nsurl, aRef);

    nsACString_GetData(aRef, &refa);
    if(!*refa)
        return NS_OK;

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_GetDirectory(nsIURL *iface, nsACString *aDirectory)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aDirectory);

    if(This->nsurl)
        return nsIURL_GetDirectory(This->nsurl, aDirectory);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_SetDirectory(nsIURL *iface, const nsACString *aDirectory)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_nsacstr(aDirectory));

    if(This->nsurl)
        return nsIURL_SetDirectory(This->nsurl, aDirectory);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_GetFileName(nsIURL *iface, nsACString *aFileName)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aFileName);

    if(This->nsurl)
        return nsIURL_GetFileName(This->nsurl, aFileName);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_SetFileName(nsIURL *iface, const nsACString *aFileName)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_nsacstr(aFileName));

    if(This->nsurl)
        return nsIURL_SetFileName(This->nsurl, aFileName);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_GetFileBaseName(nsIURL *iface, nsACString *aFileBaseName)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aFileBaseName);

    if(This->nsurl)
        return nsIURL_GetFileBaseName(This->nsurl, aFileBaseName);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_SetFileBaseName(nsIURL *iface, const nsACString *aFileBaseName)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_nsacstr(aFileBaseName));

    if(This->nsurl)
        return nsIURL_SetFileBaseName(This->nsurl, aFileBaseName);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_GetFileExtension(nsIURL *iface, nsACString *aFileExtension)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aFileExtension);

    if(This->nsurl)
        return nsIURL_GetFileExtension(This->nsurl, aFileExtension);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_SetFileExtension(nsIURL *iface, const nsACString *aFileExtension)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_nsacstr(aFileExtension));

    if(This->nsurl)
        return nsIURL_SetFileExtension(This->nsurl, aFileExtension);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_GetCommonBaseSpec(nsIURL *iface, nsIURI *aURIToCompare, nsACString *_retval)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p %p)\n", This, aURIToCompare, _retval);

    if(This->nsurl)
        return nsIURL_GetCommonBaseSpec(This->nsurl, aURIToCompare, _retval);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_GetRelativeSpec(nsIURL *iface, nsIURI *aURIToCompare, nsACString *_retval)
{
    nsWineURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p %p)\n", This, aURIToCompare, _retval);

    if(This->nsurl)
        return nsIURL_GetRelativeSpec(This->nsurl, aURIToCompare, _retval);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

#undef NSURI_THIS

static const nsIURLVtbl nsURLVtbl = {
    nsURI_QueryInterface,
    nsURI_AddRef,
    nsURI_Release,
    nsURI_GetSpec,
    nsURI_SetSpec,
    nsURI_GetPrePath,
    nsURI_GetScheme,
    nsURI_SetScheme,
    nsURI_GetUserPass,
    nsURI_SetUserPass,
    nsURI_GetUsername,
    nsURI_SetUsername,
    nsURI_GetPassword,
    nsURI_SetPassword,
    nsURI_GetHostPort,
    nsURI_SetHostPort,
    nsURI_GetHost,
    nsURI_SetHost,
    nsURI_GetPort,
    nsURI_SetPort,
    nsURI_GetPath,
    nsURI_SetPath,
    nsURI_Equals,
    nsURI_SchemeIs,
    nsURI_Clone,
    nsURI_Resolve,
    nsURI_GetAsciiSpec,
    nsURI_GetAsciiHost,
    nsURI_GetOriginCharset,
    nsURL_GetFilePath,
    nsURL_SetFilePath,
    nsURL_GetParam,
    nsURL_SetParam,
    nsURL_GetQuery,
    nsURL_SetQuery,
    nsURL_GetRef,
    nsURL_SetRef,
    nsURL_GetDirectory,
    nsURL_SetDirectory,
    nsURL_GetFileName,
    nsURL_SetFileName,
    nsURL_GetFileBaseName,
    nsURL_SetFileBaseName,
    nsURL_GetFileExtension,
    nsURL_SetFileExtension,
    nsURL_GetCommonBaseSpec,
    nsURL_GetRelativeSpec
};

static nsresult create_uri(nsIURI *uri, HTMLWindow *window, NSContainer *container, nsWineURI **_retval)
{
    nsWineURI *ret = heap_alloc_zero(sizeof(nsWineURI));

    ret->lpIURLVtbl = &nsURLVtbl;
    ret->ref = 1;
    ret->uri = uri;

    set_uri_nscontainer(ret, container);
    set_uri_window(ret, window);

    if(uri)
        nsIURI_QueryInterface(uri, &IID_nsIURL, (void**)&ret->nsurl);

    TRACE("retval=%p\n", ret);
    *_retval = ret;
    return NS_OK;
}

HRESULT create_doc_uri(HTMLWindow *window, WCHAR *url, nsWineURI **ret)
{
    nsWineURI *uri;
    nsresult nsres;

    nsres = create_uri(NULL, window, window->doc_obj->nscontainer, &uri);
    if(NS_FAILED(nsres))
        return E_FAIL;

    set_wine_url(uri, url);
    uri->is_doc_uri = TRUE;

    *ret = uri;
    return S_OK;
}

typedef struct {
    const nsIProtocolHandlerVtbl  *lpProtocolHandlerVtbl;

    LONG ref;

    nsIProtocolHandler *nshandler;
} nsProtocolHandler;

#define NSPROTHANDLER(x)  ((nsIProtocolHandler*)  &(x)->lpProtocolHandlerVtbl)

#define NSPROTHANDLER_THIS(iface) DEFINE_THIS(nsProtocolHandler, ProtocolHandler, iface)

static nsresult NSAPI nsProtocolHandler_QueryInterface(nsIProtocolHandler *iface, nsIIDRef riid,
        nsQIResult result)
{
    nsProtocolHandler *This = NSPROTHANDLER_THIS(iface);

    *result = NULL;

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports %p)\n", This, result);
        *result = NSPROTHANDLER(This);
    }else if(IsEqualGUID(&IID_nsIProtocolHandler, riid)) {
        TRACE("(%p)->(IID_nsIProtocolHandler %p)\n", This, result);
        *result = NSPROTHANDLER(This);
    }else if(IsEqualGUID(&IID_nsIExternalProtocolHandler, riid)) {
        TRACE("(%p)->(IID_nsIExternalProtocolHandler %p), returning NULL\n", This, result);
        return NS_NOINTERFACE;
    }

    if(*result) {
        nsISupports_AddRef((nsISupports*)*result);
        return NS_OK;
    }

    WARN("(%s %p)\n", debugstr_guid(riid), result);
    return NS_NOINTERFACE;
}

static nsrefcnt NSAPI nsProtocolHandler_AddRef(nsIProtocolHandler *iface)
{
    nsProtocolHandler *This = NSPROTHANDLER_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static nsrefcnt NSAPI nsProtocolHandler_Release(nsIProtocolHandler *iface)
{
    nsProtocolHandler *This = NSPROTHANDLER_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->nshandler)
            nsIProtocolHandler_Release(This->nshandler);
        heap_free(This);
    }

    return ref;
}

static nsresult NSAPI nsProtocolHandler_GetScheme(nsIProtocolHandler *iface, nsACString *aScheme)
{
    nsProtocolHandler *This = NSPROTHANDLER_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aScheme);

    if(This->nshandler)
        return nsIProtocolHandler_GetScheme(This->nshandler, aScheme);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsProtocolHandler_GetDefaultPort(nsIProtocolHandler *iface,
        PRInt32 *aDefaultPort)
{
    nsProtocolHandler *This = NSPROTHANDLER_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aDefaultPort);

    if(This->nshandler)
        return nsIProtocolHandler_GetDefaultPort(This->nshandler, aDefaultPort);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsProtocolHandler_GetProtocolFlags(nsIProtocolHandler *iface,
                                                         PRUint32 *aProtocolFlags)
{
    nsProtocolHandler *This = NSPROTHANDLER_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aProtocolFlags);

    if(This->nshandler)
        return nsIProtocolHandler_GetProtocolFlags(This->nshandler, aProtocolFlags);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsProtocolHandler_NewURI(nsIProtocolHandler *iface,
        const nsACString *aSpec, const char *aOriginCharset, nsIURI *aBaseURI, nsIURI **_retval)
{
    nsProtocolHandler *This = NSPROTHANDLER_THIS(iface);

    TRACE("((%p)->%p %s %p %p)\n", This, aSpec, debugstr_a(aOriginCharset), aBaseURI, _retval);

    if(This->nshandler)
        return nsIProtocolHandler_NewURI(This->nshandler, aSpec, aOriginCharset, aBaseURI, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsProtocolHandler_NewChannel(nsIProtocolHandler *iface,
        nsIURI *aURI, nsIChannel **_retval)
{
    nsProtocolHandler *This = NSPROTHANDLER_THIS(iface);

    TRACE("(%p)->(%p %p)\n", This, aURI, _retval);

    if(This->nshandler)
        return nsIProtocolHandler_NewChannel(This->nshandler, aURI, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsProtocolHandler_AllowPort(nsIProtocolHandler *iface,
        PRInt32 port, const char *scheme, PRBool *_retval)
{
    nsProtocolHandler *This = NSPROTHANDLER_THIS(iface);

    TRACE("(%p)->(%d %s %p)\n", This, port, debugstr_a(scheme), _retval);

    if(This->nshandler)
        return nsIProtocolHandler_AllowPort(This->nshandler, port, scheme, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

#undef NSPROTHANDLER_THIS

static const nsIProtocolHandlerVtbl nsProtocolHandlerVtbl = {
    nsProtocolHandler_QueryInterface,
    nsProtocolHandler_AddRef,
    nsProtocolHandler_Release,
    nsProtocolHandler_GetScheme,
    nsProtocolHandler_GetDefaultPort,
    nsProtocolHandler_GetProtocolFlags,
    nsProtocolHandler_NewURI,
    nsProtocolHandler_NewChannel,
    nsProtocolHandler_AllowPort
};

static nsIProtocolHandler *create_protocol_handler(nsIProtocolHandler *nshandler)
{
    nsProtocolHandler *ret = heap_alloc(sizeof(nsProtocolHandler));

    ret->lpProtocolHandlerVtbl = &nsProtocolHandlerVtbl;
    ret->ref = 1;
    ret->nshandler = nshandler;

    return NSPROTHANDLER(ret);
}

static nsresult NSAPI nsIOService_QueryInterface(nsIIOService*,nsIIDRef,nsQIResult);

static nsrefcnt NSAPI nsIOService_AddRef(nsIIOService *iface)
{
    return 2;
}

static nsrefcnt NSAPI nsIOService_Release(nsIIOService *iface)
{
    return 1;
}

static nsresult NSAPI nsIOService_GetProtocolHandler(nsIIOService *iface, const char *aScheme,
                                                     nsIProtocolHandler **_retval)
{
    nsIExternalProtocolHandler *nsexthandler;
    nsIProtocolHandler *nshandler;
    nsresult nsres;

    TRACE("(%s %p)\n", debugstr_a(aScheme), _retval);

    nsres = nsIIOService_GetProtocolHandler(nsio, aScheme, &nshandler);
    if(NS_FAILED(nsres)) {
        WARN("GetProtocolHandler failed: %08x\n", nsres);
        return nsres;
    }

    nsres = nsIProtocolHandler_QueryInterface(nshandler, &IID_nsIExternalProtocolHandler,
                                              (void**)&nsexthandler);
    if(NS_FAILED(nsres)) {
        *_retval = nshandler;
        return NS_OK;
    }

    nsIExternalProtocolHandler_Release(nsexthandler);
    *_retval = create_protocol_handler(nshandler);
    TRACE("return %p\n", *_retval);
    return NS_OK;
}

static nsresult NSAPI nsIOService_GetProtocolFlags(nsIIOService *iface, const char *aScheme,
                                                    PRUint32 *_retval)
{
    TRACE("(%s %p)\n", debugstr_a(aScheme), _retval);
    return nsIIOService_GetProtocolFlags(nsio, aScheme, _retval);
}

static BOOL is_gecko_special_uri(const char *spec)
{
    static const char *special_schemes[] = {"chrome:", "jar:", "resource:", "javascript:", "wyciwyg:"};
    int i;

    for(i=0; i < sizeof(special_schemes)/sizeof(*special_schemes); i++) {
        if(!strncasecmp(spec, special_schemes[i], strlen(special_schemes[i])))
            return TRUE;
    }

    return FALSE;
}

static nsresult NSAPI nsIOService_NewURI(nsIIOService *iface, const nsACString *aSpec,
        const char *aOriginCharset, nsIURI *aBaseURI, nsIURI **_retval)
{
    nsWineURI *wine_uri, *base_wine_uri = NULL;
    const char *spec = NULL;
    HTMLWindow *window = NULL;
    nsIURI *uri = NULL;
    LPCWSTR base_wine_url = NULL;
    nsresult nsres;

    nsACString_GetData(aSpec, &spec);

    TRACE("(%p(%s) %s %p %p)\n", aSpec, debugstr_a(spec), debugstr_a(aOriginCharset),
          aBaseURI, _retval);

    if(is_gecko_special_uri(spec))
        return nsIIOService_NewURI(nsio, aSpec, aOriginCharset, aBaseURI, _retval);

    if(!strncmp(spec, "wine:", 5))
        spec += 5;

    if(aBaseURI) {
        PARSEDURLA parsed_url = {sizeof(PARSEDURLA)};

        nsres = nsIURI_QueryInterface(aBaseURI, &IID_nsWineURI, (void**)&base_wine_uri);
        if(NS_SUCCEEDED(nsres)) {
            base_wine_url = base_wine_uri->wine_url;
            if(base_wine_uri->window_ref && base_wine_uri->window_ref->window) {
                window = base_wine_uri->window_ref->window;
                IHTMLWindow2_AddRef(HTMLWINDOW2(window));
            }
            TRACE("base url: %s window: %p\n", debugstr_w(base_wine_url), window);
        }else if(FAILED(ParseURLA(spec, &parsed_url))) {
            TRACE("not wraping\n");
            return nsIIOService_NewURI(nsio, aSpec, aOriginCharset, aBaseURI, _retval);
        }else {
            WARN("Could not get base nsWineURI: %08x\n", nsres);
        }
    }

    nsres = nsIIOService_NewURI(nsio, aSpec, aOriginCharset, aBaseURI, &uri);
    if(NS_FAILED(nsres))
        TRACE("NewURI failed: %08x\n", nsres);

    nsres = create_uri(uri, window, NULL, &wine_uri);
    *_retval = (nsIURI*)wine_uri;

    if(window)
        IHTMLWindow2_Release(HTMLWINDOW2(window));

    if(base_wine_url) {
        WCHAR url[INTERNET_MAX_URL_LENGTH], rel_url[INTERNET_MAX_URL_LENGTH];
        DWORD len;
        HRESULT hres;

        MultiByteToWideChar(CP_ACP, 0, spec, -1, rel_url, sizeof(rel_url)/sizeof(WCHAR));

        hres = CoInternetCombineUrl(base_wine_url, rel_url,
                                    URL_ESCAPE_SPACES_ONLY|URL_DONT_ESCAPE_EXTRA_INFO,
                                    url, sizeof(url)/sizeof(WCHAR), &len, 0);
        if(SUCCEEDED(hres))
            set_wine_url(wine_uri, url);
        else
             WARN("CoCombineUrl failed: %08x\n", hres);
    }else {
        WCHAR url[INTERNET_MAX_URL_LENGTH];

        MultiByteToWideChar(CP_ACP, 0, spec, -1, url, sizeof(url)/sizeof(WCHAR));
        set_wine_url(wine_uri, url);
    }

    if(base_wine_uri)
        nsIURI_Release(NSURI(base_wine_uri));

    return nsres;
}

static nsresult NSAPI nsIOService_NewFileURI(nsIIOService *iface, nsIFile *aFile,
                                             nsIURI **_retval)
{
    TRACE("(%p %p)\n", aFile, _retval);
    return nsIIOService_NewFileURI(nsio, aFile, _retval);
}

static nsresult NSAPI nsIOService_NewChannelFromURI(nsIIOService *iface, nsIURI *aURI,
                                                     nsIChannel **_retval)
{
    PARSEDURLW parsed_url = {sizeof(PARSEDURLW)};
    nsChannel *ret;
    nsWineURI *wine_uri;
    nsresult nsres;

    TRACE("(%p %p)\n", aURI, _retval);

    nsres = nsIURI_QueryInterface(aURI, &IID_nsWineURI, (void**)&wine_uri);
    if(NS_FAILED(nsres)) {
        TRACE("Could not get nsWineURI: %08x\n", nsres);
        return nsIIOService_NewChannelFromURI(nsio, aURI, _retval);
    }

    ret = heap_alloc_zero(sizeof(nsChannel));

    ret->lpHttpChannelVtbl = &nsChannelVtbl;
    ret->lpUploadChannelVtbl = &nsUploadChannelVtbl;
    ret->lpIHttpChannelInternalVtbl = &nsHttpChannelInternalVtbl;
    ret->ref = 1;
    ret->uri = wine_uri;
    list_init(&ret->response_headers);

    nsIURI_AddRef(aURI);
    ret->original_uri = aURI;
    ret->url_scheme = wine_uri->wine_url && SUCCEEDED(ParseURLW(wine_uri->wine_url, &parsed_url))
        ? parsed_url.nScheme : URL_SCHEME_UNKNOWN;

    *_retval = NSCHANNEL(ret);
    return NS_OK;
}

static nsresult NSAPI nsIOService_NewChannel(nsIIOService *iface, const nsACString *aSpec,
        const char *aOriginCharset, nsIURI *aBaseURI, nsIChannel **_retval)
{
    TRACE("(%p %s %p %p)\n", aSpec, debugstr_a(aOriginCharset), aBaseURI, _retval);
    return nsIIOService_NewChannel(nsio, aSpec, aOriginCharset, aBaseURI, _retval);
}

static nsresult NSAPI nsIOService_GetOffline(nsIIOService *iface, PRBool *aOffline)
{
    TRACE("(%p)\n", aOffline);
    return nsIIOService_GetOffline(nsio, aOffline);
}

static nsresult NSAPI nsIOService_SetOffline(nsIIOService *iface, PRBool aOffline)
{
    TRACE("(%x)\n", aOffline);
    return nsIIOService_SetOffline(nsio, aOffline);
}

static nsresult NSAPI nsIOService_AllowPort(nsIIOService *iface, PRInt32 aPort,
                                             const char *aScheme, PRBool *_retval)
{
    TRACE("(%d %s %p)\n", aPort, debugstr_a(aScheme), _retval);
    return nsIIOService_AllowPort(nsio, aPort, debugstr_a(aScheme), _retval);
}

static nsresult NSAPI nsIOService_ExtractScheme(nsIIOService *iface, const nsACString *urlString,
                                                 nsACString * _retval)
{
    TRACE("(%p %p)\n", urlString, _retval);
    return nsIIOService_ExtractScheme(nsio, urlString, _retval);
}

static const nsIIOServiceVtbl nsIOServiceVtbl = {
    nsIOService_QueryInterface,
    nsIOService_AddRef,
    nsIOService_Release,
    nsIOService_GetProtocolHandler,
    nsIOService_GetProtocolFlags,
    nsIOService_NewURI,
    nsIOService_NewFileURI,
    nsIOService_NewChannelFromURI,
    nsIOService_NewChannel,
    nsIOService_GetOffline,
    nsIOService_SetOffline,
    nsIOService_AllowPort,
    nsIOService_ExtractScheme
};

static nsIIOService nsIOService = { &nsIOServiceVtbl };

static nsresult NSAPI nsNetUtil_QueryInterface(nsINetUtil *iface, nsIIDRef riid,
                                               nsQIResult result)
{
    return nsIIOService_QueryInterface(&nsIOService, riid, result);
}

static nsrefcnt NSAPI nsNetUtil_AddRef(nsINetUtil *iface)
{
    return 2;
}

static nsrefcnt NSAPI nsNetUtil_Release(nsINetUtil *iface)
{
    return 1;
}

static nsresult NSAPI nsNetUtil_ParseContentType(nsINetUtil *iface, const nsACString *aTypeHeader,
        nsACString *aCharset, PRBool *aHadCharset, nsACString *aContentType)
{
    TRACE("(%p %p %p %p)\n", aTypeHeader, aCharset, aHadCharset, aContentType);

    return nsINetUtil_ParseContentType(net_util, aTypeHeader, aCharset, aHadCharset, aContentType);
}

static nsresult NSAPI nsNetUtil_ProtocolHasFlags(nsINetUtil *iface, nsIURI *aURI, PRUint32 aFlags, PRBool *_retval)
{
    TRACE("()\n");

    return nsINetUtil_ProtocolHasFlags(net_util, aURI, aFlags, _retval);
}

static nsresult NSAPI nsNetUtil_URIChainHasFlags(nsINetUtil *iface, nsIURI *aURI, PRUint32 aFlags, PRBool *_retval)
{
    TRACE("(%p %08x %p)\n", aURI, aFlags, _retval);

    if(aFlags == (1<<11)) {
        *_retval = FALSE;
        return NS_OK;
    }

    return nsINetUtil_URIChainHasFlags(net_util, aURI, aFlags, _retval);
}

static nsresult NSAPI nsNetUtil_ToImmutableURI(nsINetUtil *iface, nsIURI *aURI, nsIURI **_retval)
{
    TRACE("(%p %p)\n", aURI, _retval);

    return nsINetUtil_ToImmutableURI(net_util, aURI, _retval);
}

static nsresult NSAPI nsNetUtil_EscapeString(nsINetUtil *iface, const nsACString *aString,
                                             PRUint32 aEscapeType, nsACString *_retval)
{
    TRACE("(%p %x %p)\n", aString, aEscapeType, _retval);

    return nsINetUtil_EscapeString(net_util, aString, aEscapeType, _retval);
}

static nsresult NSAPI nsNetUtil_EscapeURL(nsINetUtil *iface, const nsACString *aStr, PRUint32 aFlags,
                                          nsACString *_retval)
{
    TRACE("(%p %08x %p)\n", aStr, aFlags, _retval);

    return nsINetUtil_EscapeURL(net_util, aStr, aFlags, _retval);
}

static nsresult NSAPI nsNetUtil_UnescapeString(nsINetUtil *iface, const nsACString *aStr,
                                               PRUint32 aFlags, nsACString *_retval)
{
    TRACE("(%p %08x %p)\n", aStr, aFlags, _retval);

    return nsINetUtil_UnescapeString(net_util, aStr, aFlags, _retval);
}

static nsresult NSAPI nsNetUtil_ExtractCharsetFromContentType(nsINetUtil *iface, const nsACString *aTypeHeader,
        nsACString *aCharset, PRInt32 *aCharsetStart, PRInt32 *aCharsetEnd, PRBool *_retval)
{
    TRACE("(%p %p %p %p %p)\n", aTypeHeader, aCharset, aCharsetStart, aCharsetEnd, _retval);

    return nsINetUtil_ExtractCharsetFromContentType(net_util, aTypeHeader, aCharset, aCharsetStart, aCharsetEnd, _retval);
}

static const nsINetUtilVtbl nsNetUtilVtbl = {
    nsNetUtil_QueryInterface,
    nsNetUtil_AddRef,
    nsNetUtil_Release,
    nsNetUtil_ParseContentType,
    nsNetUtil_ProtocolHasFlags,
    nsNetUtil_URIChainHasFlags,
    nsNetUtil_ToImmutableURI,
    nsNetUtil_EscapeString,
    nsNetUtil_EscapeURL,
    nsNetUtil_UnescapeString,
    nsNetUtil_ExtractCharsetFromContentType
};

static nsINetUtil nsNetUtil = { &nsNetUtilVtbl };

static nsresult NSAPI nsIOService_QueryInterface(nsIIOService *iface, nsIIDRef riid,
                                                 nsQIResult result)
{
    *result = NULL;

    if(IsEqualGUID(&IID_nsISupports, riid))
        *result = &nsIOService;
    else if(IsEqualGUID(&IID_nsIIOService, riid))
        *result = &nsIOService;
    else if(IsEqualGUID(&IID_nsINetUtil, riid))
        *result = &nsNetUtil;

    if(*result) {
        nsISupports_AddRef((nsISupports*)*result);
        return NS_OK;
    }

    FIXME("(%s %p)\n", debugstr_guid(riid), result);
    return NS_NOINTERFACE;
}

static nsresult NSAPI nsIOServiceFactory_QueryInterface(nsIFactory *iface, nsIIDRef riid,
                                                        nsQIResult result)
{
    *result = NULL;

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(IID_nsISupports %p)\n", result);
        *result = iface;
    }else if(IsEqualGUID(&IID_nsIFactory, riid)) {
        TRACE("(IID_nsIFactory %p)\n", result);
        *result = iface;
    }

    if(*result) {
        nsIFactory_AddRef(iface);
        return NS_OK;
    }

    WARN("(%s %p)\n", debugstr_guid(riid), result);
    return NS_NOINTERFACE;
}

static nsrefcnt NSAPI nsIOServiceFactory_AddRef(nsIFactory *iface)
{
    return 2;
}

static nsrefcnt NSAPI nsIOServiceFactory_Release(nsIFactory *iface)
{
    return 1;
}

static nsresult NSAPI nsIOServiceFactory_CreateInstance(nsIFactory *iface,
        nsISupports *aOuter, const nsIID *iid, void **result)
{
    return nsIIOService_QueryInterface(&nsIOService, iid, result);
}

static nsresult NSAPI nsIOServiceFactory_LockFactory(nsIFactory *iface, PRBool lock)
{
    WARN("(%x)\n", lock);
    return NS_OK;
}

static const nsIFactoryVtbl nsIOServiceFactoryVtbl = {
    nsIOServiceFactory_QueryInterface,
    nsIOServiceFactory_AddRef,
    nsIOServiceFactory_Release,
    nsIOServiceFactory_CreateInstance,
    nsIOServiceFactory_LockFactory
};

static nsIFactory nsIOServiceFactory = { &nsIOServiceFactoryVtbl };

void init_nsio(nsIComponentManager *component_manager, nsIComponentRegistrar *registrar)
{
    nsIFactory *old_factory = NULL;
    nsresult nsres;

    nsres = nsIComponentManager_GetClassObject(component_manager, &NS_IOSERVICE_CID,
                                               &IID_nsIFactory, (void**)&old_factory);
    if(NS_FAILED(nsres)) {
        ERR("Could not get factory: %08x\n", nsres);
        return;
    }

    nsres = nsIFactory_CreateInstance(old_factory, NULL, &IID_nsIIOService, (void**)&nsio);
    if(NS_FAILED(nsres)) {
        ERR("Couldn not create nsIOService instance %08x\n", nsres);
        nsIFactory_Release(old_factory);
        return;
    }

    nsres = nsIIOService_QueryInterface(nsio, &IID_nsINetUtil, (void**)&net_util);
    if(NS_FAILED(nsres)) {
        WARN("Could not get nsINetUtil interface: %08x\n", nsres);
        nsIIOService_Release(nsio);
        return;
    }

    nsres = nsIComponentRegistrar_UnregisterFactory(registrar, &NS_IOSERVICE_CID, old_factory);
    nsIFactory_Release(old_factory);
    if(NS_FAILED(nsres))
        ERR("UnregisterFactory failed: %08x\n", nsres);

    nsres = nsIComponentRegistrar_RegisterFactory(registrar, &NS_IOSERVICE_CID,
            NS_IOSERVICE_CLASSNAME, NS_IOSERVICE_CONTRACTID, &nsIOServiceFactory);
    if(NS_FAILED(nsres))
        ERR("RegisterFactory failed: %08x\n", nsres);
}

void release_nsio(void)
{
    if(net_util) {
        nsINetUtil_Release(net_util);
        net_util = NULL;
    }

    if(nsio) {
        nsIIOService_Release(nsio);
        nsio = NULL;
    }
}
