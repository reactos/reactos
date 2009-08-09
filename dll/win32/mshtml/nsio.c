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

#define LOAD_INITIAL_DOCUMENT_URI 0x80000

#define NS_IOSERVICE_CLASSNAME "nsIOService"
#define NS_IOSERVICE_CONTRACTID "@mozilla.org/network/io-service;1"

static const IID NS_IOSERVICE_CID =
    {0x9ac9e770, 0x18bc, 0x11d3, {0x93, 0x37, 0x00, 0x10, 0x4b, 0xa0, 0xfd, 0x40}};

static nsIIOService *nsio = NULL;
static nsINetUtil *net_util;

static const WCHAR about_blankW[] = {'a','b','o','u','t',':','b','l','a','n','k',0};

typedef struct {
    const nsIWineURIVtbl *lpWineURIVtbl;

    LONG ref;

    nsIURI *uri;
    NSContainer *container;
    LPWSTR wine_url;
    PRBool is_doc_uri;
    BOOL use_wine_url;
} nsURI;

#define NSURI(x)         ((nsIURI*)            &(x)->lpWineURIVtbl)
#define NSWINEURI(x)     ((nsIWineURI*)        &(x)->lpWineURIVtbl)

static nsresult create_uri(nsIURI*,NSContainer*,nsIWineURI**);

HRESULT nsuri_to_url(LPCWSTR nsuri, BSTR *ret)
{
    const WCHAR *ptr = nsuri;

    static const WCHAR wine_prefixW[] = {'w','i','n','e',':'};

    if(!strncmpW(nsuri, wine_prefixW, sizeof(wine_prefixW)/sizeof(WCHAR)))
        ptr += sizeof(wine_prefixW)/sizeof(WCHAR);

    *ret = SysAllocString(ptr);
    if(!*ret)
        return E_OUTOFMEMORY;

    TRACE("%s -> %s\n", debugstr_w(nsuri), debugstr_w(*ret));
    return S_OK;
}

static BOOL exec_shldocvw_67(HTMLDocument *doc, LPCWSTR url)
{
    IOleCommandTarget *cmdtrg = NULL;
    HRESULT hres;

    hres = IOleClientSite_QueryInterface(doc->client, &IID_IOleCommandTarget,
                                         (void**)&cmdtrg);
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
    IServiceProvider *service_provider;
    HTMLDocument *doc = container->doc;
    DWORD hlnf = 0;
    LPCWSTR uri;
    HRESULT hres;

    nsIWineURI_GetWineURL(channel->uri, &uri);
    if(!uri) {
        ERR("GetWineURL returned NULL\n");
        return TRUE;
    }

    if(!doc) {
        NSContainer *container_iter = container;

        hlnf = HLNF_OPENINNEWWINDOW;
        while(!container_iter->doc)
            container_iter = container_iter->parent;
        doc = container_iter->doc;
    }

    if(!doc->client)
        return TRUE;

    if(!hlnf && !exec_shldocvw_67(doc, uri))
        return FALSE;

    hres = IOleClientSite_QueryInterface(doc->client, &IID_IServiceProvider,
                                         (void**)&service_provider);
    if(SUCCEEDED(hres)) {
        IHlinkFrame *hlink_frame;

        hres = IServiceProvider_QueryService(service_provider, &IID_IHlinkFrame,
                                             &IID_IHlinkFrame, (void**)&hlink_frame);
        IServiceProvider_Release(service_provider);
        if(SUCCEEDED(hres)) {
            hlink_frame_navigate(doc, hlink_frame, uri, channel->post_data_stream, hlnf);
            IHlinkFrame_Release(hlink_frame);

            return FALSE;
        }
    }

    return TRUE;
}

#define NSCHANNEL_THIS(iface) DEFINE_THIS(nsChannel, HttpChannel, iface)

static nsresult NSAPI nsChannel_QueryInterface(nsIHttpChannel *iface, nsIIDRef riid, nsQIResult result)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    *result = NULL;

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports %p)\n", This, result);
        *result = NSCHANNEL(This);
    }else if(IsEqualGUID(&IID_nsIRequest, riid)) {
        TRACE("(%p)->(IID_nsIRequest %p)\n", This, result);
        *result = NSCHANNEL(This);
    }else if(IsEqualGUID(&IID_nsIChannel, riid)) {
        TRACE("(%p)->(IID_nsIChannel %p)\n", This, result);
        *result = NSCHANNEL(This);
    }else if(This->http_channel && IsEqualGUID(&IID_nsIHttpChannel, riid)) {
        TRACE("(%p)->(IID_nsIHttpChannel %p)\n", This, result);
        *result = NSHTTPCHANNEL(This);
    }else if(IsEqualGUID(&IID_nsIUploadChannel, riid)) {
        TRACE("(%p)->(IID_nsIUploadChannel %p)\n", This, result);
        *result = NSUPCHANNEL(This);
    }

    if(*result) {
        nsIChannel_AddRef(NSCHANNEL(This));
        return NS_OK;
    }

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), result);

    if(This->channel)
        return nsIChannel_QueryInterface(This->channel, riid, result);
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
        nsIWineURI_Release(This->uri);
        if(This->channel)
            nsIChannel_Release(This->channel);
        if(This->http_channel)
            nsIHttpChannel_Release(This->http_channel);
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
        heap_free(This);
    }

    return ref;
}

static nsresult NSAPI nsChannel_GetName(nsIHttpChannel *iface, nsACString *aName)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aName);

    if(This->channel)
        return nsIChannel_GetName(This->channel, aName);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_IsPending(nsIHttpChannel *iface, PRBool *_retval)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, _retval);

    if(This->channel)
        return nsIChannel_IsPending(This->channel, _retval);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetStatus(nsIHttpChannel *iface, nsresult *aStatus)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aStatus);

    if(This->channel)
        return nsIChannel_GetStatus(This->channel, aStatus);

    TRACE("returning NS_OK\n");
    return *aStatus = NS_OK;
}

static nsresult NSAPI nsChannel_Cancel(nsIHttpChannel *iface, nsresult aStatus)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, aStatus);

    if(This->channel)
        return nsIChannel_Cancel(This->channel, aStatus);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_Suspend(nsIHttpChannel *iface)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)\n", This);

    if(This->channel)
        return nsIChannel_Suspend(This->channel);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_Resume(nsIHttpChannel *iface)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)\n", This);

    if(This->channel)
        return nsIChannel_Resume(This->channel);

    FIXME("default action not implemented\n");
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

    if(This->channel)
        return nsIChannel_SetLoadGroup(This->channel, aLoadGroup);
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

    if(This->channel)
        return nsIChannel_SetLoadFlags(This->channel, aLoadFlags);
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

    if(This->channel)
        return nsIChannel_SetOriginalURI(This->channel, aOriginalURI);
    return NS_OK;
}

static nsresult NSAPI nsChannel_GetURI(nsIHttpChannel *iface, nsIURI **aURI)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aURI);

    nsIWineURI_AddRef(This->uri);
    *aURI = (nsIURI*)This->uri;

    return NS_OK;
}

static nsresult NSAPI nsChannel_GetOwner(nsIHttpChannel *iface, nsISupports **aOwner)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aOwner);

    if(This->channel)
        return nsIChannel_GetOwner(This->channel, aOwner);

    if(This->owner)
        nsISupports_AddRef(This->owner);
    *aOwner = This->owner;

    return NS_OK;
}

static nsresult NSAPI nsChannel_SetOwner(nsIHttpChannel *iface, nsISupports *aOwner)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aOwner);

    if(This->channel)
        return nsIChannel_SetOwner(This->channel, aOwner);

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

    if(This->channel)
        return nsIChannel_SetNotificationCallbacks(This->channel, aNotificationCallbacks);
    return NS_OK;
}

static nsresult NSAPI nsChannel_GetSecurityInfo(nsIHttpChannel *iface, nsISupports **aSecurityInfo)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aSecurityInfo);

    if(This->channel)
        return nsIChannel_GetSecurityInfo(This->channel, aSecurityInfo);

    FIXME("default action not implemented\n");
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

    if(This->channel)
        return nsIChannel_GetContentType(This->channel, aContentType);

    TRACE("returning default text/html\n");
    nsACString_SetData(aContentType, "text/html");
    return NS_OK;
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

    if(This->channel)
        return nsIChannel_SetContentType(This->channel, aContentType);

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

    if(This->channel) {
        nsresult nsres = nsIChannel_GetContentCharset(This->channel, aContentCharset);
        const char *ch;
        nsACString_GetData(aContentCharset, &ch);
        return nsres;
    }

    nsACString_SetData(aContentCharset, "");
    return NS_OK;
}

static nsresult NSAPI nsChannel_SetContentCharset(nsIHttpChannel *iface,
                                                  const nsACString *aContentCharset)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aContentCharset);

    if(This->channel)
        return nsIChannel_SetContentCharset(This->channel, aContentCharset);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetContentLength(nsIHttpChannel *iface, PRInt32 *aContentLength)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aContentLength);

    if(This->channel)
        return nsIChannel_GetContentLength(This->channel, aContentLength);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetContentLength(nsIHttpChannel *iface, PRInt32 aContentLength)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%d)\n", This, aContentLength);

    if(This->channel)
        return nsIChannel_SetContentLength(This->channel, aContentLength);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_Open(nsIHttpChannel *iface, nsIInputStream **_retval)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, _retval);

    if(This->channel)
        return nsIChannel_Open(This->channel, _retval);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static BOOL do_load_from_moniker_hack(nsChannel *This)
{
    nsACString scheme_str;
    nsresult nsres;
    BOOL ret = TRUE;

    /* 
     * We should always load the page from IMoniker, but Wine is not yet
     * ready for this. This function is a heuristic, that decides which
     * way of loading is better (Gecko implementation or IMoniker). The
     * aim is to always return TRUE.
     */

    /* Load from moniker if there is no Gecko channel available */
    if(!This->channel)
        return TRUE;

    nsACString_Init(&scheme_str, NULL);
    nsres = nsIWineURI_GetScheme(This->uri, &scheme_str);

    if(NS_SUCCEEDED(nsres)) {
        const char *scheme;

        nsACString_GetData(&scheme_str, &scheme);
        ret = !strcmp(scheme, "wine") || !strcmp(scheme, "about");
    }

    nsACString_Finish(&scheme_str);
    return ret;
}

static HRESULT create_mon_for_nschannel(nsChannel *channel, IMoniker **mon)
{
    nsIWineURI *wine_uri;
    LPCWSTR wine_url;
    nsresult nsres;
    HRESULT hres;

    if(!channel->original_uri) {
        ERR("original_uri == NULL\n");
        return E_FAIL;
    }

    nsres = nsIURI_QueryInterface(channel->original_uri, &IID_nsIWineURI, (void**)&wine_uri);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIWineURI: %08x\n", nsres);
        return E_FAIL;
    }

    nsIWineURI_GetWineURL(wine_uri, &wine_url);
    nsIWineURI_Release(wine_uri);
    if(!wine_url) {
        TRACE("wine_url == NULL\n");
        return E_FAIL;
    }

    hres = CreateURLMoniker(NULL, wine_url, mon);
    if(FAILED(hres))
        WARN("CreateURLMonikrer failed: %08x\n", hres);

    return hres;
}

static NSContainer *get_nscontainer_from_load_group(nsChannel *This)
{
    NSContainer *container;
    nsIChannel *channel;
    nsIRequest *req;
    nsIWineURI *wine_uri;
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

    nsres = nsIURI_QueryInterface(uri, &IID_nsIWineURI, (void**)&wine_uri);
    nsIURI_Release(uri);
    if(NS_FAILED(nsres)) {
        TRACE("Could not get nsIWineURI: %08x\n", nsres);
        return NULL;
    }

    nsIWineURI_GetNSContainer(wine_uri, &container);
    nsIWineURI_Release(wine_uri);

    return container;
}

static nsresult async_open_doc_uri(nsChannel *This, NSContainer *container,
        nsIStreamListener *listener, nsISupports *context, BOOL *open)
{
    IMoniker *mon;
    HRESULT hres;

    *open = FALSE;

    if(container->bscallback) {
        channelbsc_set_channel(container->bscallback, This, listener, context);

        if(container->doc && container->doc->mime) {
            heap_free(This->content_type);
            This->content_type = heap_strdupWtoA(container->doc->mime);
        }

        if(do_load_from_moniker_hack(This))
            return WINE_NS_LOAD_FROM_MONIKER;
    }else  {
        BOOL cont = before_async_open(This, container);

        if(!cont) {
            TRACE("canceled\n");
            return NS_ERROR_UNEXPECTED;
        }

        if(!container->doc) {
            return This->channel
                ?  nsIChannel_AsyncOpen(This->channel, listener, context)
                : NS_ERROR_UNEXPECTED;
        }

        hres = create_mon_for_nschannel(This, &mon);
        if(FAILED(hres)) {
            return NS_ERROR_UNEXPECTED;
        }
        set_current_mon(container->doc, mon);
    }

    *open = TRUE;
    return NS_OK;
}

static nsresult async_open(nsChannel *This, NSContainer *container, nsIStreamListener *listener,
        nsISupports *context)
{
    nsChannelBSC *bscallback;
    IMoniker *mon = NULL;
    nsresult nsres;
    task_t *task;
    HRESULT hres;

    if(This->channel) {
        nsres = nsIChannel_AsyncOpen(This->channel, listener, context);

        if(mon)
            IMoniker_Release(mon);

        if(NS_FAILED(nsres) && (This->load_flags & LOAD_INITIAL_DOCUMENT_URI))
            return WINE_NS_LOAD_FROM_MONIKER;
        return nsres;
    }

    TRACE("channel == NULL\n");

    hres = create_mon_for_nschannel(This, &mon);
    if(FAILED(hres))
        return NS_ERROR_UNEXPECTED;

    bscallback = create_channelbsc(mon);
    IMoniker_Release(mon);

    channelbsc_set_channel(bscallback, This, listener, context);

    task = heap_alloc(sizeof(task_t));

    task->doc = container->doc;
    task->task_id = TASK_START_BINDING;
    task->next = NULL;
    task->bscallback = bscallback;

    push_task(task);

    return NS_OK;
}

static nsresult NSAPI nsChannel_AsyncOpen(nsIHttpChannel *iface, nsIStreamListener *aListener,
                                          nsISupports *aContext)
{
    nsChannel *This = NSCHANNEL_THIS(iface);
    NSContainer *container;
    PRBool is_doc_uri;
    BOOL open = TRUE;
    nsresult nsres = NS_OK;

    TRACE("(%p)->(%p %p)\n", This, aListener, aContext);

    nsIWineURI_GetNSContainer(This->uri, &container);

    if(!container && This->load_group) {
        container = get_nscontainer_from_load_group(This);
        if(container)
            nsIWineURI_SetNSContainer(This->uri, container);
    }

    if(!container) {
        TRACE("container = NULL\n");
        return This->channel
            ? nsIChannel_AsyncOpen(This->channel, aListener, aContext)
            : NS_ERROR_UNEXPECTED;
    }

    nsIWineURI_GetIsDocumentURI(This->uri, &is_doc_uri);

    if(is_doc_uri && (This->load_flags & LOAD_INITIAL_DOCUMENT_URI))
        nsres = async_open_doc_uri(This, container, aListener, aContext, &open);

    if(open)
        nsres = async_open(This, container, aListener, aContext);

    nsIWebBrowserChrome_Release(NSWBCHROME(container));
    return nsres;
}

static nsresult NSAPI nsChannel_GetRequestMethod(nsIHttpChannel *iface, nsACString *aRequestMethod)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aRequestMethod);

    if(This->http_channel)
        return nsIHttpChannel_GetRequestMethod(This->http_channel, aRequestMethod);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetRequestMethod(nsIHttpChannel *iface,
                                                 const nsACString *aRequestMethod)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aRequestMethod);

    if(This->http_channel)
        return nsIHttpChannel_SetRequestMethod(This->http_channel, aRequestMethod);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetReferrer(nsIHttpChannel *iface, nsIURI **aReferrer)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aReferrer);

    if(This->http_channel)
        return nsIHttpChannel_GetReferrer(This->http_channel, aReferrer);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetReferrer(nsIHttpChannel *iface, nsIURI *aReferrer)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aReferrer);

    if(This->http_channel)
        return nsIHttpChannel_SetReferrer(This->http_channel, aReferrer);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetRequestHeader(nsIHttpChannel *iface,
         const nsACString *aHeader, nsACString *_retval)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p %p)\n", This, aHeader, _retval);

    if(This->http_channel)
        return nsIHttpChannel_GetRequestHeader(This->http_channel, aHeader, _retval);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetRequestHeader(nsIHttpChannel *iface,
         const nsACString *aHeader, const nsACString *aValue, PRBool aMerge)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p %p %x)\n", This, aHeader, aValue, aMerge);

    if(This->http_channel)
        return nsIHttpChannel_SetRequestHeader(This->http_channel, aHeader, aValue, aMerge);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_VisitRequestHeaders(nsIHttpChannel *iface,
                                                    nsIHttpHeaderVisitor *aVisitor)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aVisitor);

    if(This->http_channel)
        return nsIHttpChannel_VisitRequestHeaders(This->http_channel, aVisitor);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetAllowPipelining(nsIHttpChannel *iface, PRBool *aAllowPipelining)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aAllowPipelining);

    if(This->http_channel)
        return nsIHttpChannel_GetAllowPipelining(This->http_channel, aAllowPipelining);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetAllowPipelining(nsIHttpChannel *iface, PRBool aAllowPipelining)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%x)\n", This, aAllowPipelining);

    if(This->http_channel)
        return nsIHttpChannel_SetAllowPipelining(This->http_channel, aAllowPipelining);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetRedirectionLimit(nsIHttpChannel *iface, PRUint32 *aRedirectionLimit)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aRedirectionLimit);

    if(This->http_channel)
        return nsIHttpChannel_GetRedirectionLimit(This->http_channel, aRedirectionLimit);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetRedirectionLimit(nsIHttpChannel *iface, PRUint32 aRedirectionLimit)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%u)\n", This, aRedirectionLimit);

    if(This->http_channel)
        return nsIHttpChannel_SetRedirectionLimit(This->http_channel, aRedirectionLimit);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetResponseStatus(nsIHttpChannel *iface, PRUint32 *aResponseStatus)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aResponseStatus);

    if(This->http_channel)
        return nsIHttpChannel_GetResponseStatus(This->http_channel, aResponseStatus);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetResponseStatusText(nsIHttpChannel *iface,
                                                      nsACString *aResponseStatusText)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aResponseStatusText);

    if(This->http_channel)
        return nsIHttpChannel_GetResponseStatusText(This->http_channel, aResponseStatusText);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetRequestSucceeded(nsIHttpChannel *iface,
                                                    PRBool *aRequestSucceeded)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aRequestSucceeded);

    if(This->http_channel)
        return nsIHttpChannel_GetRequestSucceeded(This->http_channel, aRequestSucceeded);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetResponseHeader(nsIHttpChannel *iface,
         const nsACString *header, nsACString *_retval)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p %p)\n", This, header, _retval);

    if(This->http_channel)
        return nsIHttpChannel_GetResponseHeader(This->http_channel, header, _retval);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetResponseHeader(nsIHttpChannel *iface,
        const nsACString *header, const nsACString *value, PRBool merge)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p %p %x)\n", This, header, value, merge);

    if(This->http_channel)
        return nsIHttpChannel_SetResponseHeader(This->http_channel, header, value, merge);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_VisitResponseHeaders(nsIHttpChannel *iface,
        nsIHttpHeaderVisitor *aVisitor)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aVisitor);

    if(This->http_channel)
        return nsIHttpChannel_VisitResponseHeaders(This->http_channel, aVisitor);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_IsNoStoreResponse(nsIHttpChannel *iface, PRBool *_retval)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, _retval);

    if(This->http_channel)
        return nsIHttpChannel_IsNoStoreResponse(This->http_channel, _retval);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_IsNoCacheResponse(nsIHttpChannel *iface, PRBool *_retval)
{
    nsChannel *This = NSCHANNEL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, _retval);

    if(This->http_channel)
        return nsIHttpChannel_IsNoCacheResponse(This->http_channel, _retval);

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
    nsresult nsres;

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

    if(This->post_data_stream) {
        nsIUploadChannel *upload_channel;

        nsres = nsIChannel_QueryInterface(This->channel, &IID_nsIUploadChannel,
                (void**)&upload_channel);
        if(NS_SUCCEEDED(nsres)) {
            nsres = nsIUploadChannel_SetUploadStream(upload_channel, aStream, aContentType, aContentLength);
            nsIUploadChannel_Release(upload_channel);
            if(NS_FAILED(nsres))
                WARN("SetUploadStream failed: %08x\n", nsres);

        }
    }

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

#define NSURI_THIS(iface) DEFINE_THIS(nsURI, WineURI, iface)

static nsresult NSAPI nsURI_QueryInterface(nsIWineURI *iface, nsIIDRef riid, nsQIResult result)
{
    nsURI *This = NSURI_THIS(iface);

    *result = NULL;

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports %p)\n", This, result);
        *result = NSURI(This);
    }else if(IsEqualGUID(&IID_nsIURI, riid)) {
        TRACE("(%p)->(IID_nsIURI %p)\n", This, result);
        *result = NSURI(This);
    }else if(IsEqualGUID(&IID_nsIWineURI, riid)) {
        TRACE("(%p)->(IID_nsIWineURI %p)\n", This, result);
        *result = NSURI(This);
    }

    if(*result) {
        nsIURI_AddRef(NSURI(This));
        return NS_OK;
    }

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), result);
    return This->uri ? nsIURI_QueryInterface(This->uri, riid, result) : NS_NOINTERFACE;
}

static nsrefcnt NSAPI nsURI_AddRef(nsIWineURI *iface)
{
    nsURI *This = NSURI_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static nsrefcnt NSAPI nsURI_Release(nsIWineURI *iface)
{
    nsURI *This = NSURI_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->container)
            nsIWebBrowserChrome_Release(NSWBCHROME(This->container));
        if(This->uri)
            nsIURI_Release(This->uri);
        heap_free(This->wine_url);
        heap_free(This);
    }

    return ref;
}

static nsresult NSAPI nsURI_GetSpec(nsIWineURI *iface, nsACString *aSpec)
{
    nsURI *This = NSURI_THIS(iface);

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

static nsresult NSAPI nsURI_SetSpec(nsIWineURI *iface, const nsACString *aSpec)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aSpec);

    if(This->uri)
        return nsIURI_SetSpec(This->uri, aSpec);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetPrePath(nsIWineURI *iface, nsACString *aPrePath)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aPrePath);

    if(This->uri)
        return nsIURI_GetPrePath(This->uri, aPrePath);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetScheme(nsIWineURI *iface, nsACString *aScheme)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aScheme);

    if(This->use_wine_url && strcmpW(This->wine_url, about_blankW)) {
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

static nsresult NSAPI nsURI_SetScheme(nsIWineURI *iface, const nsACString *aScheme)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aScheme);

    if(This->uri)
        return nsIURI_SetScheme(This->uri, aScheme);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetUserPass(nsIWineURI *iface, nsACString *aUserPass)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aUserPass);

    if(This->uri)
        return nsIURI_GetUserPass(This->uri, aUserPass);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_SetUserPass(nsIWineURI *iface, const nsACString *aUserPass)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aUserPass);

    if(This->uri)
        return nsIURI_SetUserPass(This->uri, aUserPass);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetUsername(nsIWineURI *iface, nsACString *aUsername)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aUsername);

    if(This->uri)
        return nsIURI_GetUsername(This->uri, aUsername);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_SetUsername(nsIWineURI *iface, const nsACString *aUsername)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aUsername);

    if(This->uri)
        return nsIURI_SetUsername(This->uri, aUsername);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetPassword(nsIWineURI *iface, nsACString *aPassword)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aPassword);

    if(This->uri)
        return nsIURI_GetPassword(This->uri, aPassword);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_SetPassword(nsIWineURI *iface, const nsACString *aPassword)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aPassword);

    if(This->uri)
        return nsIURI_SetPassword(This->uri, aPassword);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetHostPort(nsIWineURI *iface, nsACString *aHostPort)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aHostPort);

    if(This->uri)
        return nsIURI_GetHostPort(This->uri, aHostPort);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_SetHostPort(nsIWineURI *iface, const nsACString *aHostPort)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aHostPort);

    if(This->uri)
        return nsIURI_SetHostPort(This->uri, aHostPort);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetHost(nsIWineURI *iface, nsACString *aHost)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aHost);

    if(This->uri)
        return nsIURI_GetHost(This->uri, aHost);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_SetHost(nsIWineURI *iface, const nsACString *aHost)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aHost);

    if(This->uri)
        return nsIURI_SetHost(This->uri, aHost);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetPort(nsIWineURI *iface, PRInt32 *aPort)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aPort);

    if(This->uri)
        return nsIURI_GetPort(This->uri, aPort);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_SetPort(nsIWineURI *iface, PRInt32 aPort)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%d)\n", This, aPort);

    if(This->uri)
        return nsIURI_SetPort(This->uri, aPort);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetPath(nsIWineURI *iface, nsACString *aPath)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aPath);

    if(This->uri)
        return nsIURI_GetPath(This->uri, aPath);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_SetPath(nsIWineURI *iface, const nsACString *aPath)
{
    nsURI *This = NSURI_THIS(iface);
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
            nsIWineURI_SetWineURL(NSWINEURI(This), new_url);
        else
            WARN("UrlCombine failed: %08x\n", hres);
    }

    if(!This->uri)
        return NS_OK;

    return nsIURI_SetPath(This->uri, aPath);
}

static nsresult NSAPI nsURI_Equals(nsIWineURI *iface, nsIURI *other, PRBool *_retval)
{
    nsURI *This = NSURI_THIS(iface);
    nsIWineURI *wine_uri;
    LPCWSTR other_url = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p %p)\n", This, other, _retval);

    if(This->uri)
        return nsIURI_Equals(This->uri, other, _retval);

    nsres = nsIURI_QueryInterface(other, &IID_nsIWineURI, (void**)&wine_uri);
    if(NS_FAILED(nsres)) {
        TRACE("Could not get nsIWineURI interface\n");
        *_retval = FALSE;
        return NS_OK;
    }

    nsIWineURI_GetWineURL(wine_uri, &other_url);
    *_retval = !UrlCompareW(This->wine_url, other_url, TRUE);
    nsIWineURI_Release(wine_uri);

    return NS_OK;
}

static nsresult NSAPI nsURI_SchemeIs(nsIWineURI *iface, const char *scheme, PRBool *_retval)
{
    nsURI *This = NSURI_THIS(iface);

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

static nsresult NSAPI nsURI_Clone(nsIWineURI *iface, nsIURI **_retval)
{
    nsURI *This = NSURI_THIS(iface);
    nsIURI *nsuri = NULL;
    nsIWineURI *wine_uri;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, _retval);

    if(This->uri) {
        nsres = nsIURI_Clone(This->uri, &nsuri);
        if(NS_FAILED(nsres)) {
            WARN("Clone failed: %08x\n", nsres);
            return nsres;
        }
    }

    nsres = create_uri(nsuri, This->container, &wine_uri);
    if(NS_FAILED(nsres)) {
        WARN("create_uri failed: %08x\n", nsres);
        return nsres;
    }

    *_retval = (nsIURI*)wine_uri;
    return nsIWineURI_SetWineURL(wine_uri, This->wine_url);
}

static nsresult NSAPI nsURI_Resolve(nsIWineURI *iface, const nsACString *arelativePath,
        nsACString *_retval)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p %p)\n", This, arelativePath, _retval);

    if(This->uri)
        return nsIURI_Resolve(This->uri, arelativePath, _retval);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetAsciiSpec(nsIWineURI *iface, nsACString *aAsciiSpec)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aAsciiSpec);

    if(This->use_wine_url)
        return nsIURI_GetSpec(NSURI(This), aAsciiSpec);

    if(This->uri)
        return nsIURI_GetAsciiSpec(This->uri, aAsciiSpec);

    TRACE("returning error\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetAsciiHost(nsIWineURI *iface, nsACString *aAsciiHost)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aAsciiHost);

    if(This->uri)
        return nsIURI_GetAsciiHost(This->uri, aAsciiHost);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetOriginCharset(nsIWineURI *iface, nsACString *aOriginCharset)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aOriginCharset);

    if(This->uri)
        return nsIURI_GetOriginCharset(This->uri, aOriginCharset);

    FIXME("default action not implemented\n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetNSContainer(nsIWineURI *iface, NSContainer **aContainer)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aContainer);

    if(This->container)
        nsIWebBrowserChrome_AddRef(NSWBCHROME(This->container));
    *aContainer = This->container;

    return NS_OK;
}

static nsresult NSAPI nsURI_SetNSContainer(nsIWineURI *iface, NSContainer *aContainer)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aContainer);

    if(This->container) {
        if(This->container == aContainer)
            return NS_OK;
        TRACE("Changing %p -> %p\n", This->container, aContainer);
        nsIWebBrowserChrome_Release(NSWBCHROME(This->container));
    }

    if(aContainer)
        nsIWebBrowserChrome_AddRef(NSWBCHROME(aContainer));
    This->container = aContainer;

    return NS_OK;
}

static nsresult NSAPI nsURI_GetIsDocumentURI(nsIWineURI *iface, PRBool *aIsDocumentURI)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aIsDocumentURI);

    *aIsDocumentURI = This->is_doc_uri;
    return NS_OK;
}

static nsresult NSAPI nsURI_SetIsDocumentURI(nsIWineURI *iface, PRBool aIsDocumentURI)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%x)\n", This, aIsDocumentURI);

    This->is_doc_uri = aIsDocumentURI;
    return NS_OK;
}

static nsresult NSAPI nsURI_GetWineURL(nsIWineURI *iface, LPCWSTR *aURL)
{
    nsURI *This = NSURI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, aURL);

    *aURL = This->wine_url;
    return NS_OK;
}

static nsresult NSAPI nsURI_SetWineURL(nsIWineURI *iface, LPCWSTR aURL)
{
    nsURI *This = NSURI_THIS(iface);

    static const WCHAR wszFtp[]   = {'f','t','p',':'};
    static const WCHAR wszHttp[]  = {'h','t','t','p',':'};
    static const WCHAR wszHttps[] = {'h','t','t','p','s',':'};

    TRACE("(%p)->(%s)\n", This, debugstr_w(aURL));

    heap_free(This->wine_url);

    if(aURL) {
        int len = strlenW(aURL)+1;
        This->wine_url = heap_alloc(len*sizeof(WCHAR));
        memcpy(This->wine_url, aURL, len*sizeof(WCHAR));

        /* FIXME: Always use wine url */
        This->use_wine_url =
               strncmpW(aURL, wszFtp,   sizeof(wszFtp)/sizeof(WCHAR))
            && strncmpW(aURL, wszHttp,  sizeof(wszHttp)/sizeof(WCHAR))
            && strncmpW(aURL, wszHttps, sizeof(wszHttps)/sizeof(WCHAR));
    }else {
        This->wine_url = NULL;
        This->use_wine_url = FALSE;
    }

    return NS_OK;
}

#undef NSURI_THIS

static const nsIWineURIVtbl nsWineURIVtbl = {
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
    nsURI_GetNSContainer,
    nsURI_SetNSContainer,
    nsURI_GetIsDocumentURI,
    nsURI_SetIsDocumentURI,
    nsURI_GetWineURL,
    nsURI_SetWineURL
};

static nsresult create_uri(nsIURI *uri, NSContainer *container, nsIWineURI **_retval)
{
    nsURI *ret = heap_alloc(sizeof(nsURI));

    ret->lpWineURIVtbl = &nsWineURIVtbl;
    ret->ref = 1;
    ret->uri = uri;
    ret->container = container;
    ret->wine_url = NULL;
    ret->is_doc_uri = FALSE;
    ret->use_wine_url = FALSE;

    if(container)
        nsIWebBrowserChrome_AddRef(NSWBCHROME(container));

    TRACE("retval=%p\n", ret);
    *_retval = NSWINEURI(ret);
    return NS_OK;
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
    static const char chromeW[] = "chrome:";
    static const char jarW[] = "jar:";
    static const char resourceW[] = "resource:";
    static const char javascriptW[] = "javascript:";

    return !strncasecmp(spec, chromeW,     sizeof(chromeW)-1)
        || !strncasecmp(spec, resourceW,   sizeof(resourceW)-1)
        || !strncasecmp(spec, jarW,        sizeof(jarW)-1)
        || !strncasecmp(spec, javascriptW, sizeof(javascriptW)-1);
}

static nsresult NSAPI nsIOService_NewURI(nsIIOService *iface, const nsACString *aSpec,
        const char *aOriginCharset, nsIURI *aBaseURI, nsIURI **_retval)
{
    const char *spec = NULL;
    NSContainer *nscontainer = NULL;
    nsIURI *uri = NULL;
    LPCWSTR base_wine_url = NULL;
    nsIWineURI *base_wine_uri = NULL, *wine_uri;
    BOOL is_wine_uri = FALSE;
    nsresult nsres;

    nsACString_GetData(aSpec, &spec);

    TRACE("(%p(%s) %s %p %p)\n", aSpec, debugstr_a(spec), debugstr_a(aOriginCharset),
          aBaseURI, _retval);

    if(is_gecko_special_uri(spec))
        return nsIIOService_NewURI(nsio, aSpec, aOriginCharset, aBaseURI, _retval);

    if(!strncmp(spec, "wine:", 5)) {
        spec += 5;
        is_wine_uri = TRUE;
    }

    if(aBaseURI) {
        nsACString base_uri_str;
        const char *base_uri = NULL;

        nsACString_Init(&base_uri_str, NULL);

        nsres = nsIURI_GetSpec(aBaseURI, &base_uri_str);
        if(NS_SUCCEEDED(nsres)) {
            nsACString_GetData(&base_uri_str, &base_uri);
            TRACE("base_uri=%s\n", debugstr_a(base_uri));
        }else {
            ERR("GetSpec failed: %08x\n", nsres);
        }

        nsACString_Finish(&base_uri_str);
    }

    nsres = nsIIOService_NewURI(nsio, aSpec, aOriginCharset, aBaseURI, &uri);
    if(NS_FAILED(nsres))
        TRACE("NewURI failed: %08x\n", nsres);

    if(aBaseURI) {
        nsres = nsIURI_QueryInterface(aBaseURI, &IID_nsIWineURI, (void**)&base_wine_uri);
        if(NS_SUCCEEDED(nsres)) {
            nsIWineURI_GetNSContainer(base_wine_uri, &nscontainer);
            nsIWineURI_GetWineURL(base_wine_uri, &base_wine_url);
        }else {
            TRACE("Could not get base nsIWineURI: %08x\n", nsres);
        }
    }

    TRACE("nscontainer = %p\n", nscontainer);

    nsres = create_uri(uri, nscontainer, &wine_uri);
    *_retval = (nsIURI*)wine_uri;

    if(nscontainer)
        nsIWebBrowserChrome_Release(NSWBCHROME(nscontainer));

    if(base_wine_url) {
        WCHAR url[INTERNET_MAX_URL_LENGTH], rel_url[INTERNET_MAX_URL_LENGTH];
        DWORD len;
        HRESULT hres;

        MultiByteToWideChar(CP_ACP, 0, spec, -1, rel_url, sizeof(rel_url)/sizeof(WCHAR));

        hres = CoInternetCombineUrl(base_wine_url, rel_url,
                                    URL_ESCAPE_SPACES_ONLY|URL_DONT_ESCAPE_EXTRA_INFO,
                                    url, sizeof(url)/sizeof(WCHAR), &len, 0);
        if(SUCCEEDED(hres))
            nsIWineURI_SetWineURL(wine_uri, url);
        else
             WARN("CoCombineUrl failed: %08x\n", hres);
    }else if(is_wine_uri) {
        WCHAR url[INTERNET_MAX_URL_LENGTH];

        MultiByteToWideChar(CP_ACP, 0, spec, -1, url, sizeof(url)/sizeof(WCHAR));
        nsIWineURI_SetWineURL(wine_uri, url);
    }

    if(base_wine_uri)
        nsIWineURI_Release(base_wine_uri);

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
    nsIChannel *channel = NULL;
    nsChannel *ret;
    nsIWineURI *wine_uri;
    nsresult nsres;

    TRACE("(%p %p)\n", aURI, _retval);

    nsres = nsIIOService_NewChannelFromURI(nsio, aURI, &channel);
    if(NS_FAILED(nsres) && nsres != NS_ERROR_UNKNOWN_PROTOCOL) {
        WARN("NewChannelFromURI failed: %08x\n", nsres);
        *_retval = channel;
        return nsres;
    }

    nsres = nsIURI_QueryInterface(aURI, &IID_nsIWineURI, (void**)&wine_uri);
    if(NS_FAILED(nsres)) {
        WARN("Could not get nsIWineURI: %08x\n", nsres);
        *_retval = channel;
        return channel ? NS_OK : NS_ERROR_UNEXPECTED;
    }

    ret = heap_alloc_zero(sizeof(nsChannel));

    ret->lpHttpChannelVtbl = &nsChannelVtbl;
    ret->lpUploadChannelVtbl = &nsUploadChannelVtbl;
    ret->ref = 1;
    ret->channel = channel;
    ret->uri = wine_uri;

    nsIURI_AddRef(aURI);
    ret->original_uri = aURI;

    if(channel)
        nsIChannel_QueryInterface(channel, &IID_nsIHttpChannel, (void**)&ret->http_channel);

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
