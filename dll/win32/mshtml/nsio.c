/*
 * Copyright 2006-2010 Jacek Caban for CodeWeavers
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

#define NS_IOSERVICE_CLASSNAME "nsIOService"
#define NS_IOSERVICE_CONTRACTID "@mozilla.org/network/io-service;1"

static const IID NS_IOSERVICE_CID =
    {0x9ac9e770, 0x18bc, 0x11d3, {0x93, 0x37, 0x00, 0x10, 0x4b, 0xa0, 0xfd, 0x40}};
static const IID IID_nsWineURI =
    {0x5088272e, 0x900b, 0x11da, {0xc6,0x87, 0x00,0x0f,0xea,0x57,0xf2,0x1a}};

static nsIIOService *nsio = NULL;
static nsINetUtil *net_util;

static const char *request_method_strings[] = {"GET", "PUT", "POST"};

struct  nsWineURI {
    nsIFileURL nsIFileURL_iface; /* For non-file URL objects, it's just nsIURL */
    nsIStandardURL nsIStandardURL_iface;

    LONG ref;

    NSContainer *container;
    windowref_t *window_ref;
    nsChannelBSC *channel_bsc;
    IUri *uri;
    IUriBuilder *uri_builder;
    char *origin_charset;
    BOOL is_doc_uri;
    BOOL is_mutable;
    DWORD scheme;
};

static BOOL ensure_uri(nsWineURI *This)
{
    HRESULT hres;

    assert(This->uri || This->uri_builder);

    if(!This->uri) {
        hres = IUriBuilder_CreateUriSimple(This->uri_builder, 0, 0, &This->uri);
        if(FAILED(hres)) {
            WARN("CreateUriSimple failed: %08x\n", hres);
            return FALSE;
        }
    }

    return TRUE;
}

IUri *nsuri_get_uri(nsWineURI *nsuri)
{
    if(!ensure_uri(nsuri))
        return NULL;

    IUri_AddRef(nsuri->uri);
    return nsuri->uri;
}

IUri *get_uri_nofrag(IUri *uri)
{
    IUriBuilder *uri_builder;
    IUri *ret;
    BOOL b;
    HRESULT hres;

    hres = IUri_HasProperty(uri, Uri_PROPERTY_FRAGMENT, &b);
    if(SUCCEEDED(hres) && !b) {
        IUri_AddRef(uri);
        return uri;
    }

    hres = CreateIUriBuilder(uri, 0, 0, &uri_builder);
    if(FAILED(hres))
        return NULL;

    hres = IUriBuilder_RemoveProperties(uri_builder, Uri_HAS_FRAGMENT);
    if(SUCCEEDED(hres))
        hres = IUriBuilder_CreateUriSimple(uri_builder, 0, 0, &ret);
    IUriBuilder_Release(uri_builder);
    if(FAILED(hres))
        return NULL;

    return ret;
}

static BOOL compare_ignoring_frag(IUri *uri1, IUri *uri2)
{
    IUri *uri_nofrag1, *uri_nofrag2;
    BOOL ret = FALSE;

    uri_nofrag1 = get_uri_nofrag(uri1);
    if(!uri_nofrag1)
        return FALSE;

    uri_nofrag2 = get_uri_nofrag(uri2);
    if(uri_nofrag2) {
        IUri_IsEqual(uri_nofrag1, uri_nofrag2, &ret);
        IUri_Release(uri_nofrag2);
    }

    IUri_Release(uri_nofrag1);
    return ret;
}

static HRESULT combine_url(IUri *base_uri, const WCHAR *rel_url, IUri **ret)
{
    IUri *uri_nofrag;
    HRESULT hres;

    uri_nofrag = get_uri_nofrag(base_uri);
    if(!uri_nofrag)
        return E_FAIL;

    hres = CoInternetCombineUrlEx(uri_nofrag, rel_url, URL_ESCAPE_SPACES_ONLY|URL_DONT_ESCAPE_EXTRA_INFO,
                ret, 0);
    IUri_Release(uri_nofrag);
    if(FAILED(hres))
        WARN("CoInternetCombineUrlEx failed: %08x\n", hres);
    return hres;
}

static nsresult create_nsuri(IUri*,HTMLOuterWindow*,NSContainer*,const char*,nsWineURI**);

static const char *debugstr_nsacstr(const nsACString *nsstr)
{
    const char *data;

    nsACString_GetData(nsstr, &data);
    return debugstr_a(data);
}

static nsresult return_wstr_nsacstr(nsACString *ret_str, const WCHAR *str, int len)
{
    char *stra;
    int lena;

    TRACE("returning %s\n", debugstr_wn(str, len));

    if(!*str) {
        nsACString_SetData(ret_str, "");
        return NS_OK;
    }

    lena = WideCharToMultiByte(CP_UTF8, 0, str, len, NULL, 0, NULL, NULL);
    stra = heap_alloc(lena+1);
    if(!stra)
        return NS_ERROR_OUT_OF_MEMORY;

    WideCharToMultiByte(CP_UTF8, 0, str, len, stra, lena, NULL, NULL);
    stra[lena] = 0;

    nsACString_SetData(ret_str, stra);
    heap_free(stra);
    return NS_OK;
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

static BOOL exec_shldocvw_67(HTMLDocumentObj *doc, BSTR url)
{
    IOleCommandTarget *cmdtrg = NULL;
    HRESULT hres;

    hres = IOleClientSite_QueryInterface(doc->client, &IID_IOleCommandTarget, (void**)&cmdtrg);
    if(SUCCEEDED(hres)) {
        VARIANT varUrl, varRes;

        V_VT(&varUrl) = VT_BSTR;
        V_BSTR(&varUrl) = url;
        V_VT(&varRes) = VT_BOOL;

        hres = IOleCommandTarget_Exec(cmdtrg, &CGID_ShellDocView, 67, 0, &varUrl, &varRes);

        IOleCommandTarget_Release(cmdtrg);

        if(SUCCEEDED(hres) && !V_BOOL(&varRes)) {
            TRACE("got VARIANT_FALSE, do not load\n");
            return FALSE;
        }
    }

    return TRUE;
}

static nsresult before_async_open(nsChannel *channel, NSContainer *container, BOOL *cancel)
{
    HTMLDocumentObj *doc = container->doc;
    BSTR display_uri;
    HRESULT hres;

    if(!doc->client) {
        *cancel = TRUE;
        return NS_OK;
    }

    hres = IUri_GetDisplayUri(channel->uri->uri, &display_uri);
    if(FAILED(hres))
        return NS_ERROR_FAILURE;

    if(!exec_shldocvw_67(doc, display_uri)) {
        SysFreeString(display_uri);
        *cancel = FALSE;
        return NS_OK;
    }

    hres = hlink_frame_navigate(&doc->basedoc, display_uri, channel, 0, cancel);
    SysFreeString(display_uri);
    if(FAILED(hres))
        *cancel = TRUE;
    return NS_OK;
}

HRESULT load_nsuri(HTMLOuterWindow *window, nsWineURI *uri, nsIInputStream *post_stream,
        nsChannelBSC *channelbsc, DWORD flags)
{
    nsIDocShellLoadInfo *load_info = NULL;
    nsIWebNavigation *web_navigation;
    nsIDocShell *doc_shell;
    HTMLDocumentNode *doc;
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

    if(post_stream) {
        nsres = nsIDocShell_CreateLoadInfo(doc_shell, &load_info);
        if(NS_FAILED(nsres)) {
            nsIDocShell_Release(doc_shell);
            return E_FAIL;
        }

        nsres = nsIDocShellLoadInfo_SetPostDataStream(load_info, post_stream);
        assert(nsres == NS_OK);
    }

    uri->channel_bsc = channelbsc;
    doc = window->base.inner_window->doc;
    doc->skip_mutation_notif = TRUE;
    nsres = nsIDocShell_LoadURI(doc_shell, (nsIURI*)&uri->nsIFileURL_iface, load_info, flags, FALSE);
    if(doc == window->base.inner_window->doc)
        doc->skip_mutation_notif = FALSE;
    uri->channel_bsc = NULL;
    nsIDocShell_Release(doc_shell);
    if(load_info)
        nsIDocShellLoadInfo_Release(load_info);
    if(NS_FAILED(nsres)) {
        WARN("LoadURI failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static void set_uri_nscontainer(nsWineURI *This, NSContainer *nscontainer)
{
    if(This->container) {
        if(This->container == nscontainer)
            return;
        TRACE("Changing %p -> %p\n", This->container, nscontainer);
        nsIWebBrowserChrome_Release(&This->container->nsIWebBrowserChrome_iface);
    }

    if(nscontainer)
        nsIWebBrowserChrome_AddRef(&nscontainer->nsIWebBrowserChrome_iface);
    This->container = nscontainer;
}

static void set_uri_window(nsWineURI *This, HTMLOuterWindow *window)
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
    return This->uri->scheme == URL_SCHEME_HTTP || This->uri->scheme == URL_SCHEME_HTTPS;
}

static http_header_t *find_http_header(struct list *headers, const WCHAR *name, int len)
{
    http_header_t *iter;

    LIST_FOR_EACH_ENTRY(iter, headers, http_header_t, entry) {
        if(!strncmpiW(iter->header, name, len) && !iter->header[len])
            return iter;
    }

    return NULL;
}

static nsresult get_channel_http_header(struct list *headers, const nsACString *header_name_str,
        nsACString *_retval)
{
    const char *header_namea;
    http_header_t *header;
    WCHAR *header_name;
    char *data;

    nsACString_GetData(header_name_str, &header_namea);
    header_name = heap_strdupAtoW(header_namea);
    if(!header_name)
        return NS_ERROR_UNEXPECTED;

    header = find_http_header(headers, header_name, strlenW(header_name));
    heap_free(header_name);
    if(!header)
        return NS_ERROR_NOT_AVAILABLE;

    data = heap_strdupWtoA(header->data);
    if(!data)
        return NS_ERROR_UNEXPECTED;

    TRACE("%s -> %s\n", debugstr_a(header_namea), debugstr_a(data));
    nsACString_SetData(_retval, data);
    heap_free(data);
    return NS_OK;
}

HRESULT set_http_header(struct list *headers, const WCHAR *name, int name_len,
        const WCHAR *value, int value_len)
{
    http_header_t *header;

    TRACE("%s: %s\n", debugstr_wn(name, name_len), debugstr_wn(value, value_len));

    header = find_http_header(headers, name, name_len);
    if(header) {
        WCHAR *new_data;

        new_data = heap_strndupW(value, value_len);
        if(!new_data)
            return E_OUTOFMEMORY;

        heap_free(header->data);
        header->data = new_data;
    }else {
        header = heap_alloc(sizeof(http_header_t));
        if(!header)
            return E_OUTOFMEMORY;

        header->header = heap_strndupW(name, name_len);
        header->data = heap_strndupW(value, value_len);
        if(!header->header || !header->data) {
            heap_free(header->header);
            heap_free(header->data);
            heap_free(header);
            return E_OUTOFMEMORY;
        }

        list_add_tail(headers, &header->entry);
    }

    return S_OK;
}

static nsresult set_channel_http_header(struct list *headers, const nsACString *name_str,
        const nsACString *value_str)
{
    const char *namea, *valuea;
    WCHAR *name, *value;
    HRESULT hres;

    nsACString_GetData(name_str, &namea);
    name = heap_strdupAtoW(namea);
    if(!name)
        return NS_ERROR_UNEXPECTED;

    nsACString_GetData(value_str, &valuea);
    value = heap_strdupAtoW(valuea);
    if(!value) {
        heap_free(name);
        return NS_ERROR_UNEXPECTED;
    }

    hres = set_http_header(headers, name, strlenW(name), value, strlenW(value));

    heap_free(name);
    heap_free(value);
    return SUCCEEDED(hres) ? NS_OK : NS_ERROR_UNEXPECTED;
}

static nsresult visit_http_headers(struct list *headers, nsIHttpHeaderVisitor *visitor)
{
    nsACString header_str, value_str;
    char *header, *value;
    http_header_t *iter;
    nsresult nsres;

    LIST_FOR_EACH_ENTRY(iter, headers, http_header_t, entry) {
        header = heap_strdupWtoA(iter->header);
        if(!header)
            return NS_ERROR_OUT_OF_MEMORY;

        value = heap_strdupWtoA(iter->data);
        if(!value) {
            heap_free(header);
            return NS_ERROR_OUT_OF_MEMORY;
        }

        nsACString_InitDepend(&header_str, header);
        nsACString_InitDepend(&value_str, value);
        nsres = nsIHttpHeaderVisitor_VisitHeader(visitor, &header_str, &value_str);
        nsACString_Finish(&header_str);
        nsACString_Finish(&value_str);
        heap_free(header);
        heap_free(value);
        if(NS_FAILED(nsres))
            break;
    }

    return NS_OK;
}

static void free_http_headers(struct list *list)
{
    http_header_t *iter, *iter_next;

    LIST_FOR_EACH_ENTRY_SAFE(iter, iter_next, list, http_header_t, entry) {
        list_remove(&iter->entry);
        heap_free(iter->header);
        heap_free(iter->data);
        heap_free(iter);
    }
}

static inline nsChannel *impl_from_nsIHttpChannel(nsIHttpChannel *iface)
{
    return CONTAINING_RECORD(iface, nsChannel, nsIHttpChannel_iface);
}

static nsresult NSAPI nsChannel_QueryInterface(nsIHttpChannel *iface, nsIIDRef riid, void **result)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports %p)\n", This, result);
        *result = &This->nsIHttpChannel_iface;
    }else if(IsEqualGUID(&IID_nsIRequest, riid)) {
        TRACE("(%p)->(IID_nsIRequest %p)\n", This, result);
        *result = &This->nsIHttpChannel_iface;
    }else if(IsEqualGUID(&IID_nsIChannel, riid)) {
        TRACE("(%p)->(IID_nsIChannel %p)\n", This, result);
        *result = &This->nsIHttpChannel_iface;
    }else if(IsEqualGUID(&IID_nsIHttpChannel, riid)) {
        TRACE("(%p)->(IID_nsIHttpChannel %p)\n", This, result);
        *result = is_http_channel(This) ? &This->nsIHttpChannel_iface : NULL;
    }else if(IsEqualGUID(&IID_nsIUploadChannel, riid)) {
        TRACE("(%p)->(IID_nsIUploadChannel %p)\n", This, result);
        *result = &This->nsIUploadChannel_iface;
    }else if(IsEqualGUID(&IID_nsIHttpChannelInternal, riid)) {
        TRACE("(%p)->(IID_nsIHttpChannelInternal %p)\n", This, result);
        *result = is_http_channel(This) ? &This->nsIHttpChannelInternal_iface : NULL;
    }else {
        TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), result);
        *result = NULL;
    }

    if(*result) {
        nsIHttpChannel_AddRef(&This->nsIHttpChannel_iface);
        return NS_OK;
    }

    return NS_NOINTERFACE;
}

static nsrefcnt NSAPI nsChannel_AddRef(nsIHttpChannel *iface)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    nsrefcnt ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static nsrefcnt NSAPI nsChannel_Release(nsIHttpChannel *iface)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    if(!ref) {
        nsIFileURL_Release(&This->uri->nsIFileURL_iface);
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
        if(This->referrer)
            nsIURI_Release(This->referrer);

        free_http_headers(&This->response_headers);
        free_http_headers(&This->request_headers);

        heap_free(This->content_type);
        heap_free(This->charset);
        heap_free(This);
    }

    return ref;
}

static nsresult NSAPI nsChannel_GetName(nsIHttpChannel *iface, nsACString *aName)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%p)\n", This, aName);

    return nsIFileURL_GetSpec(&This->uri->nsIFileURL_iface, aName);
}

static nsresult NSAPI nsChannel_IsPending(nsIHttpChannel *iface, cpp_bool *_retval)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)->(%p)\n", This, _retval);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetStatus(nsIHttpChannel *iface, nsresult *aStatus)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    WARN("(%p)->(%p) returning NS_OK\n", This, aStatus);

    return *aStatus = NS_OK;
}

static nsresult NSAPI nsChannel_Cancel(nsIHttpChannel *iface, nsresult aStatus)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)->(%08x)\n", This, aStatus);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_Suspend(nsIHttpChannel *iface)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)\n", This);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_Resume(nsIHttpChannel *iface)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)\n", This);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetLoadGroup(nsIHttpChannel *iface, nsILoadGroup **aLoadGroup)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%p)\n", This, aLoadGroup);

    if(This->load_group)
        nsILoadGroup_AddRef(This->load_group);

    *aLoadGroup = This->load_group;
    return NS_OK;
}

static nsresult NSAPI nsChannel_SetLoadGroup(nsIHttpChannel *iface, nsILoadGroup *aLoadGroup)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

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
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%p)\n", This, aLoadFlags);

    *aLoadFlags = This->load_flags;
    return NS_OK;
}

static nsresult NSAPI nsChannel_SetLoadFlags(nsIHttpChannel *iface, nsLoadFlags aLoadFlags)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%08x)\n", This, aLoadFlags);

    This->load_flags = aLoadFlags;
    return NS_OK;
}

static nsresult NSAPI nsChannel_GetOriginalURI(nsIHttpChannel *iface, nsIURI **aOriginalURI)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%p)\n", This, aOriginalURI);

    if(This->original_uri)
        nsIURI_AddRef(This->original_uri);

    *aOriginalURI = This->original_uri;
    return NS_OK;
}

static nsresult NSAPI nsChannel_SetOriginalURI(nsIHttpChannel *iface, nsIURI *aOriginalURI)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%p)\n", This, aOriginalURI);

    if(This->original_uri)
        nsIURI_Release(This->original_uri);

    nsIURI_AddRef(aOriginalURI);
    This->original_uri = aOriginalURI;
    return NS_OK;
}

static nsresult NSAPI nsChannel_GetURI(nsIHttpChannel *iface, nsIURI **aURI)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%p)\n", This, aURI);

    nsIFileURL_AddRef(&This->uri->nsIFileURL_iface);
    *aURI = (nsIURI*)This->uri;

    return NS_OK;
}

static nsresult NSAPI nsChannel_GetOwner(nsIHttpChannel *iface, nsISupports **aOwner)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%p)\n", This, aOwner);

    if(This->owner)
        nsISupports_AddRef(This->owner);
    *aOwner = This->owner;

    return NS_OK;
}

static nsresult NSAPI nsChannel_SetOwner(nsIHttpChannel *iface, nsISupports *aOwner)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

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
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%p)\n", This, aNotificationCallbacks);

    if(This->notif_callback)
        nsIInterfaceRequestor_AddRef(This->notif_callback);
    *aNotificationCallbacks = This->notif_callback;

    return NS_OK;
}

static nsresult NSAPI nsChannel_SetNotificationCallbacks(nsIHttpChannel *iface,
        nsIInterfaceRequestor *aNotificationCallbacks)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

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
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%p)\n", This, aSecurityInfo);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetContentType(nsIHttpChannel *iface, nsACString *aContentType)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%p)\n", This, aContentType);

    if(This->content_type) {
        nsACString_SetData(aContentType, This->content_type);
        return S_OK;
    }

    if(This->uri->is_doc_uri) {
        WARN("Document channel with no MIME set. Assuming text/html\n");
        nsACString_SetData(aContentType, "text/html");
        return S_OK;
    }

    WARN("unknown type\n");
    return NS_ERROR_FAILURE;
}

static nsresult NSAPI nsChannel_SetContentType(nsIHttpChannel *iface,
                                               const nsACString *aContentType)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    const char *content_type;

    TRACE("(%p)->(%s)\n", This, debugstr_nsacstr(aContentType));

    nsACString_GetData(aContentType, &content_type);
    heap_free(This->content_type);
    This->content_type = heap_strdupA(content_type);

    return NS_OK;
}

static nsresult NSAPI nsChannel_GetContentCharset(nsIHttpChannel *iface,
                                                  nsACString *aContentCharset)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

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
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    const char *data;
    char *charset;

    TRACE("(%p)->(%s)\n", This, debugstr_nsacstr(aContentCharset));

    nsACString_GetData(aContentCharset, &data);
    charset = heap_strdupA(data);
    if(!charset)
        return NS_ERROR_OUT_OF_MEMORY;

    heap_free(This->charset);
    This->charset = charset;
    return NS_OK;
}

static nsresult NSAPI nsChannel_GetContentLength(nsIHttpChannel *iface, INT64 *aContentLength)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)->(%p)\n", This, aContentLength);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetContentLength(nsIHttpChannel *iface, INT64 aContentLength)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)->(%s)\n", This, wine_dbgstr_longlong(aContentLength));

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_Open(nsIHttpChannel *iface, nsIInputStream **_retval)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)->(%p)\n", This, _retval);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static HTMLOuterWindow *get_window_from_load_group(nsChannel *This)
{
    HTMLOuterWindow *window;
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
        IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);
    nsIFileURL_Release(&wine_uri->nsIFileURL_iface);

    return window;
}

static HTMLOuterWindow *get_channel_window(nsChannel *This)
{
    nsIWebProgress *web_progress;
    nsIDOMWindow *nswindow;
    HTMLOuterWindow *window;
    nsresult nsres;

    if(This->load_group) {
        nsIRequestObserver *req_observer;

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
    }else if(This->notif_callback) {
        nsres = nsIInterfaceRequestor_GetInterface(This->notif_callback, &IID_nsIWebProgress, (void**)&web_progress);
        if(NS_FAILED(nsres)) {
            ERR("GetInterface(IID_nsIWebProgress failed: %08x\n", nsres);
            return NULL;
        }
    }else {
        ERR("no load group nor notif callback\n");
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
        IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);
    else
        FIXME("NULL window for %p\n", nswindow);
    return window;
}

typedef struct {
    task_t header;
    HTMLInnerWindow *window;
    nsChannelBSC *bscallback;
} start_binding_task_t;

static void start_binding_proc(task_t *_task)
{
    start_binding_task_t *task = (start_binding_task_t*)_task;

    start_binding(task->window, (BSCallback*)task->bscallback, NULL);
}

static void start_binding_task_destr(task_t *_task)
{
    start_binding_task_t *task = (start_binding_task_t*)_task;

    IBindStatusCallback_Release(&task->bscallback->bsc.IBindStatusCallback_iface);
    heap_free(task);
}

static nsresult async_open(nsChannel *This, HTMLOuterWindow *window, BOOL is_doc_channel, nsIStreamListener *listener,
        nsISupports *context)
{
    nsChannelBSC *bscallback;
    IMoniker *mon = NULL;
    HRESULT hres;

    hres = CreateURLMonikerEx2(NULL, This->uri->uri, &mon, 0);
    if(FAILED(hres)) {
        WARN("CreateURLMoniker failed: %08x\n", hres);
        return NS_ERROR_UNEXPECTED;
    }

    if(is_doc_channel)
        set_current_mon(window, mon, BINDING_NAVIGATED);

    hres = create_channelbsc(mon, NULL, NULL, 0, is_doc_channel, &bscallback);
    IMoniker_Release(mon);
    if(FAILED(hres))
        return NS_ERROR_UNEXPECTED;

    channelbsc_set_channel(bscallback, This, listener, context);

    if(is_doc_channel) {
        hres = create_pending_window(window, bscallback);
        if(SUCCEEDED(hres))
            async_start_doc_binding(window, window->pending_window);
        IBindStatusCallback_Release(&bscallback->bsc.IBindStatusCallback_iface);
        if(FAILED(hres))
            return NS_ERROR_UNEXPECTED;
    }else {
        start_binding_task_t *task;

        task = heap_alloc(sizeof(start_binding_task_t));
        if(!task) {
            IBindStatusCallback_Release(&bscallback->bsc.IBindStatusCallback_iface);
            return NS_ERROR_OUT_OF_MEMORY;
        }

        task->window = window->base.inner_window;
        task->bscallback = bscallback;
        hres = push_task(&task->header, start_binding_proc, start_binding_task_destr, window->base.inner_window->task_magic);
        if(FAILED(hres))
            return NS_ERROR_OUT_OF_MEMORY;
    }

    return NS_OK;
}

static nsresult NSAPI nsChannel_AsyncOpen(nsIHttpChannel *iface, nsIStreamListener *aListener,
                                          nsISupports *aContext)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    HTMLOuterWindow *window = NULL;
    BOOL cancel = FALSE;
    nsresult nsres = NS_OK;

    TRACE("(%p)->(%p %p)\n", This, aListener, aContext);

    if(!ensure_uri(This->uri))
        return NS_ERROR_FAILURE;

    if(TRACE_ON(mshtml)) {
        HRESULT hres;
        BSTR uri_str;

        hres = IUri_GetDisplayUri(This->uri->uri, &uri_str);
        if(SUCCEEDED(hres)) {
            TRACE("opening %s\n", debugstr_w(uri_str));
            SysFreeString(uri_str);
        }else {
            WARN("GetDisplayUri failed: %08x\n", hres);
        }
    }

    if(This->uri->is_doc_uri) {
        window = get_channel_window(This);
        if(window) {
            set_uri_window(This->uri, window);
        }else if(This->uri->container) {
            BOOL b;

            /* nscontainer->doc should be NULL which means navigation to a new window */
            if(This->uri->container->doc)
                FIXME("nscontainer->doc = %p\n", This->uri->container->doc);

            nsres = before_async_open(This, This->uri->container, &b);
            if(NS_FAILED(nsres))
                return nsres;
            if(b)
                FIXME("Navigation not cancelled\n");
            return NS_ERROR_UNEXPECTED;
        }
    }

    if(!window) {
        if(This->uri->window_ref && This->uri->window_ref->window) {
            window = This->uri->window_ref->window;
            IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);
        }else {
            /* FIXME: Analyze removing get_window_from_load_group call */
            if(This->load_group)
                window = get_window_from_load_group(This);
            if(!window)
                window = get_channel_window(This);
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

            cancel = TRUE;
        }else {
            nsres = before_async_open(This, window->doc_obj->nscontainer, &cancel);
            if(NS_SUCCEEDED(nsres)  && cancel) {
                TRACE("canceled\n");
                nsres = NS_BINDING_ABORTED;
            }
        }
    }

    if(!cancel)
        nsres = async_open(This, window, This->uri->is_doc_uri, aListener, aContext);

    if(NS_SUCCEEDED(nsres) && This->load_group) {
        nsres = nsILoadGroup_AddRequest(This->load_group, (nsIRequest*)&This->nsIHttpChannel_iface,
                aContext);
        if(NS_FAILED(nsres))
            ERR("AddRequest failed: %08x\n", nsres);
    }

    IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
    return nsres;
}

static nsresult NSAPI nsChannel_GetContentDisposition(nsIHttpChannel *iface, UINT32 *aContentDisposition)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    FIXME("(%p)->(%p)\n", This, aContentDisposition);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetContentDisposition(nsIHttpChannel *iface, UINT32 aContentDisposition)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    FIXME("(%p)->(%u)\n", This, aContentDisposition);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetContentDispositionFilename(nsIHttpChannel *iface, nsAString *aContentDispositionFilename)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    FIXME("(%p)->(%p)\n", This, aContentDispositionFilename);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetContentDispositionFilename(nsIHttpChannel *iface, const nsAString *aContentDispositionFilename)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    FIXME("(%p)->(%p)\n", This, aContentDispositionFilename);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetContentDispositionHeader(nsIHttpChannel *iface, nsACString *aContentDispositionHeader)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    FIXME("(%p)->(%p)\n", This, aContentDispositionHeader);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetLoadInfo(nsIHttpChannel *iface, nsILoadInfo **aLoadInfo)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%p)\n", This, aLoadInfo);

    if(This->load_info)
        nsISupports_AddRef(This->load_info);
    *aLoadInfo = This->load_info;
    return NS_OK;
}

static nsresult NSAPI nsChannel_SetLoadInfo(nsIHttpChannel *iface, nsILoadInfo *aLoadInfo)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%p)\n", This, aLoadInfo);

    if(This->load_info)
        nsISupports_Release(This->load_info);
    This->load_info = aLoadInfo;
    if(This->load_info)
        nsISupports_AddRef(This->load_info);
    return NS_OK;
}

static nsresult NSAPI nsChannel_GetRequestMethod(nsIHttpChannel *iface, nsACString *aRequestMethod)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%p)\n", This, aRequestMethod);

    nsACString_SetData(aRequestMethod, request_method_strings[This->request_method]);
    return NS_OK;
}

static nsresult NSAPI nsChannel_SetRequestMethod(nsIHttpChannel *iface,
                                                 const nsACString *aRequestMethod)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    const char *method;
    unsigned i;

    TRACE("(%p)->(%s)\n", This, debugstr_nsacstr(aRequestMethod));

    nsACString_GetData(aRequestMethod, &method);
    for(i=0; i < sizeof(request_method_strings)/sizeof(*request_method_strings); i++) {
        if(!strcasecmp(method, request_method_strings[i])) {
            This->request_method = i;
            return NS_OK;
        }
    }

    ERR("Invalid method %s\n", debugstr_a(method));
    return NS_ERROR_UNEXPECTED;
}

static nsresult NSAPI nsChannel_GetReferrer(nsIHttpChannel *iface, nsIURI **aReferrer)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%p)\n", This, aReferrer);

    if(This->referrer)
        nsIURI_AddRef(This->referrer);
    *aReferrer = This->referrer;
    return NS_OK;
}

static nsresult NSAPI nsChannel_SetReferrer(nsIHttpChannel *iface, nsIURI *aReferrer)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%p)\n", This, aReferrer);

    if(aReferrer)
        nsIURI_AddRef(aReferrer);
    if(This->referrer)
        nsIURI_Release(This->referrer);
    This->referrer = aReferrer;
    return NS_OK;
}

static nsresult NSAPI nsChannel_GetReferrerPolicy(nsIHttpChannel *iface, UINT32 *aReferrerPolicy)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    FIXME("(%p)->(%p)\n", This, aReferrerPolicy);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetReferrerWithPolicy(nsIHttpChannel *iface, nsIURI *aReferrer, UINT32 aReferrerPolicy)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    FIXME("(%p)->(%p %x)\n", This, aReferrer, aReferrerPolicy);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetRequestHeader(nsIHttpChannel *iface,
         const nsACString *aHeader, nsACString *_retval)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_nsacstr(aHeader), _retval);

    return get_channel_http_header(&This->request_headers, aHeader, _retval);
}

static nsresult NSAPI nsChannel_SetRequestHeader(nsIHttpChannel *iface,
         const nsACString *aHeader, const nsACString *aValue, cpp_bool aMerge)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%s %s %x)\n", This, debugstr_nsacstr(aHeader), debugstr_nsacstr(aValue), aMerge);

    if(aMerge)
        FIXME("aMerge not supported\n");

    return set_channel_http_header(&This->request_headers, aHeader, aValue);
}

static nsresult NSAPI nsChannel_VisitRequestHeaders(nsIHttpChannel *iface,
                                                    nsIHttpHeaderVisitor *aVisitor)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)->(%p)\n", This, aVisitor);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetAllowPipelining(nsIHttpChannel *iface, cpp_bool *aAllowPipelining)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)->(%p)\n", This, aAllowPipelining);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetAllowPipelining(nsIHttpChannel *iface, cpp_bool aAllowPipelining)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)->(%x)\n", This, aAllowPipelining);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetAllowTLS(nsIHttpChannel *iface, cpp_bool *aAllowTLS)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    FIXME("(%p)->(%p)\n", This, aAllowTLS);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetAllowTLS(nsIHttpChannel *iface, cpp_bool aAllowTLS)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    FIXME("(%p)->(%x)\n", This, aAllowTLS);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetRedirectionLimit(nsIHttpChannel *iface, UINT32 *aRedirectionLimit)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)->(%p)\n", This, aRedirectionLimit);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetRedirectionLimit(nsIHttpChannel *iface, UINT32 aRedirectionLimit)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)->(%u)\n", This, aRedirectionLimit);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_GetResponseStatus(nsIHttpChannel *iface, UINT32 *aResponseStatus)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

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
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%p)\n", This, aResponseStatusText);

    nsACString_SetData(aResponseStatusText, This->response_status_text);
    return NS_OK;
}

static nsresult NSAPI nsChannel_GetRequestSucceeded(nsIHttpChannel *iface,
                                                    cpp_bool *aRequestSucceeded)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%p)\n", This, aRequestSucceeded);

    if(!This->response_status)
        return NS_ERROR_NOT_AVAILABLE;

    *aRequestSucceeded = This->response_status/100 == 2;

    return NS_OK;
}

static nsresult NSAPI nsChannel_GetResponseHeader(nsIHttpChannel *iface,
         const nsACString *header, nsACString *_retval)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_nsacstr(header), _retval);

    return get_channel_http_header(&This->response_headers, header, _retval);
}

static nsresult NSAPI nsChannel_SetResponseHeader(nsIHttpChannel *iface,
        const nsACString *header, const nsACString *value, cpp_bool merge)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)->(%s %s %x)\n", This, debugstr_nsacstr(header), debugstr_nsacstr(value), merge);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_VisitResponseHeaders(nsIHttpChannel *iface,
        nsIHttpHeaderVisitor *aVisitor)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%p)\n", This, aVisitor);

    return visit_http_headers(&This->response_headers, aVisitor);
}

static nsresult NSAPI nsChannel_IsNoStoreResponse(nsIHttpChannel *iface, cpp_bool *_retval)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    http_header_t *header;

    static const WCHAR cache_controlW[] = {'C','a','c','h','e','-','C','o','n','t','r','o','l'};
    static const WCHAR no_storeW[] = {'n','o','-','s','t','o','r','e',0};

    TRACE("(%p)->(%p)\n", This, _retval);

    header = find_http_header(&This->response_headers, cache_controlW, sizeof(cache_controlW)/sizeof(WCHAR));
    *_retval = header && !strcmpiW(header->data, no_storeW);
    return NS_OK;
}

static nsresult NSAPI nsChannel_IsNoCacheResponse(nsIHttpChannel *iface, cpp_bool *_retval)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)->(%p)\n", This, _retval);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_IsPrivateResponse(nsIHttpChannel *iface, cpp_bool *_retval)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)->(%p)\n", This, _retval);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_RedirectTo(nsIHttpChannel *iface, nsIURI *aNewURI)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)->(%p)\n", This, aNewURI);

    return NS_ERROR_NOT_IMPLEMENTED;
}

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
    nsChannel_GetContentDisposition,
    nsChannel_SetContentDisposition,
    nsChannel_GetContentDispositionFilename,
    nsChannel_SetContentDispositionFilename,
    nsChannel_GetContentDispositionHeader,
    nsChannel_GetLoadInfo,
    nsChannel_SetLoadInfo,
    nsChannel_GetRequestMethod,
    nsChannel_SetRequestMethod,
    nsChannel_GetReferrer,
    nsChannel_SetReferrer,
    nsChannel_GetReferrerPolicy,
    nsChannel_SetReferrerWithPolicy,
    nsChannel_GetRequestHeader,
    nsChannel_SetRequestHeader,
    nsChannel_VisitRequestHeaders,
    nsChannel_GetAllowPipelining,
    nsChannel_SetAllowPipelining,
    nsChannel_GetAllowTLS,
    nsChannel_SetAllowTLS,
    nsChannel_GetRedirectionLimit,
    nsChannel_SetRedirectionLimit,
    nsChannel_GetResponseStatus,
    nsChannel_GetResponseStatusText,
    nsChannel_GetRequestSucceeded,
    nsChannel_GetResponseHeader,
    nsChannel_SetResponseHeader,
    nsChannel_VisitResponseHeaders,
    nsChannel_IsNoStoreResponse,
    nsChannel_IsNoCacheResponse,
    nsChannel_IsPrivateResponse,
    nsChannel_RedirectTo
};

static inline nsChannel *impl_from_nsIUploadChannel(nsIUploadChannel *iface)
{
    return CONTAINING_RECORD(iface, nsChannel, nsIUploadChannel_iface);
}

static nsresult NSAPI nsUploadChannel_QueryInterface(nsIUploadChannel *iface, nsIIDRef riid,
        void **result)
{
    nsChannel *This = impl_from_nsIUploadChannel(iface);
    return nsIHttpChannel_QueryInterface(&This->nsIHttpChannel_iface, riid, result);
}

static nsrefcnt NSAPI nsUploadChannel_AddRef(nsIUploadChannel *iface)
{
    nsChannel *This = impl_from_nsIUploadChannel(iface);
    return nsIHttpChannel_AddRef(&This->nsIHttpChannel_iface);
}

static nsrefcnt NSAPI nsUploadChannel_Release(nsIUploadChannel *iface)
{
    nsChannel *This = impl_from_nsIUploadChannel(iface);
    return nsIHttpChannel_Release(&This->nsIHttpChannel_iface);
}

static nsresult NSAPI nsUploadChannel_SetUploadStream(nsIUploadChannel *iface,
        nsIInputStream *aStream, const nsACString *aContentType, INT64 aContentLength)
{
    nsChannel *This = impl_from_nsIUploadChannel(iface);
    const char *content_type;

    static const WCHAR content_typeW[] =
        {'C','o','n','t','e','n','t','-','T','y','p','e',0};

    TRACE("(%p)->(%p %s %s)\n", This, aStream, debugstr_nsacstr(aContentType), wine_dbgstr_longlong(aContentLength));

    This->post_data_contains_headers = TRUE;

    if(aContentType) {
        nsACString_GetData(aContentType, &content_type);
        if(*content_type) {
            WCHAR *ct;

            ct = heap_strdupAtoW(content_type);
            if(!ct)
                return NS_ERROR_UNEXPECTED;

            set_http_header(&This->request_headers, content_typeW,
                    sizeof(content_typeW)/sizeof(WCHAR), ct, strlenW(ct));
            heap_free(ct);
            This->post_data_contains_headers = FALSE;
        }
    }

    if(aContentLength != -1)
        FIXME("Unsupported acontentLength = %s\n", wine_dbgstr_longlong(aContentLength));

    if(This->post_data_stream)
        nsIInputStream_Release(This->post_data_stream);
    This->post_data_stream = aStream;
    if(aStream)
        nsIInputStream_AddRef(aStream);

    This->request_method = METHOD_POST;
    return NS_OK;
}

static nsresult NSAPI nsUploadChannel_GetUploadStream(nsIUploadChannel *iface,
        nsIInputStream **aUploadStream)
{
    nsChannel *This = impl_from_nsIUploadChannel(iface);

    TRACE("(%p)->(%p)\n", This, aUploadStream);

    if(This->post_data_stream)
        nsIInputStream_AddRef(This->post_data_stream);

    *aUploadStream = This->post_data_stream;
    return NS_OK;
}

static const nsIUploadChannelVtbl nsUploadChannelVtbl = {
    nsUploadChannel_QueryInterface,
    nsUploadChannel_AddRef,
    nsUploadChannel_Release,
    nsUploadChannel_SetUploadStream,
    nsUploadChannel_GetUploadStream
};

static inline nsChannel *impl_from_nsIHttpChannelInternal(nsIHttpChannelInternal *iface)
{
    return CONTAINING_RECORD(iface, nsChannel, nsIHttpChannelInternal_iface);
}

static nsresult NSAPI nsHttpChannelInternal_QueryInterface(nsIHttpChannelInternal *iface, nsIIDRef riid,
        void **result)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    return nsIHttpChannel_QueryInterface(&This->nsIHttpChannel_iface, riid, result);
}

static nsrefcnt NSAPI nsHttpChannelInternal_AddRef(nsIHttpChannelInternal *iface)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    return nsIHttpChannel_AddRef(&This->nsIHttpChannel_iface);
}

static nsrefcnt NSAPI nsHttpChannelInternal_Release(nsIHttpChannelInternal *iface)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    return nsIHttpChannel_Release(&This->nsIHttpChannel_iface);
}

static nsresult NSAPI nsHttpChannelInternal_GetDocumentURI(nsIHttpChannelInternal *iface, nsIURI **aDocumentURI)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);

    FIXME("(%p)->()\n", This);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_SetDocumentURI(nsIHttpChannelInternal *iface, nsIURI *aDocumentURI)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);

    FIXME("(%p)->()\n", This);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetRequestVersion(nsIHttpChannelInternal *iface, UINT32 *major, UINT32 *minor)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);

    FIXME("(%p)->()\n", This);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetResponseVersion(nsIHttpChannelInternal *iface, UINT32 *major, UINT32 *minor)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);

    FIXME("(%p)->()\n", This);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_TakeAllSecurityMessages(nsIHttpChannelInternal *iface, void *aMessages)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);

    FIXME("(%p)->()\n", This);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_SetCookie(nsIHttpChannelInternal *iface, const char *aCookieHeader)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);

    FIXME("(%p)->()\n", This);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_SetupFallbackChannel(nsIHttpChannelInternal *iface, const char *aFallbackKey)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);

    FIXME("(%p)->()\n", This);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetForceAllowThirdPartyCookie(nsIHttpChannelInternal *iface, cpp_bool *aForceThirdPartyCookie)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);

    FIXME("(%p)->()\n", This);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_SetForceAllowThirdPartyCookie(nsIHttpChannelInternal *iface, cpp_bool aForceThirdPartyCookie)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);

    FIXME("(%p)->()\n", This);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetCanceled(nsIHttpChannelInternal *iface, cpp_bool *aCanceled)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);

    FIXME("(%p)->(%p)\n", This, aCanceled);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetChannelIsForDownload(nsIHttpChannelInternal *iface, cpp_bool *aCanceled)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);

    FIXME("(%p)->(%p)\n", This, aCanceled);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_SetChannelIsForDownload(nsIHttpChannelInternal *iface, cpp_bool aCanceled)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);

    FIXME("(%p)->(%x)\n", This, aCanceled);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetLocalAddress(nsIHttpChannelInternal *iface, nsACString *aLocalAddress)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);

    FIXME("(%p)->(%p)\n", This, aLocalAddress);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetLocalPort(nsIHttpChannelInternal *iface, LONG *aLocalPort)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);

    FIXME("(%p)->(%p)\n", This, aLocalPort);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetRemoteAddress(nsIHttpChannelInternal *iface, nsACString *aRemoteAddress)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);

    FIXME("(%p)->(%p)\n", This, aRemoteAddress);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetRemotePort(nsIHttpChannelInternal *iface, LONG *aRemotePort)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);

    FIXME("(%p)->(%p)\n", This, aRemotePort);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_SetCacheKeysRedirectChain(nsIHttpChannelInternal *iface, void *cacheKeys)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);

    FIXME("(%p)->(%p)\n", This, cacheKeys);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_HTTPUpgrade(nsIHttpChannelInternal *iface,
        const nsACString *aProtocolName, nsIHttpUpgradeListener *aListener)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_nsacstr(aProtocolName), aListener);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetAllowSpdy(nsIHttpChannelInternal *iface, cpp_bool *aAllowSpdy)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%p)\n", This, aAllowSpdy);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_SetAllowSpdy(nsIHttpChannelInternal *iface, cpp_bool aAllowSpdy)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%x)\n", This, aAllowSpdy);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetResponseTimeoutEnabled(nsIHttpChannelInternal *iface,
        cpp_bool *aResponseTimeoutEnabled)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%p)\n", This, aResponseTimeoutEnabled);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_SetResponseTimeoutEnabled(nsIHttpChannelInternal *iface,
        cpp_bool aResponseTimeoutEnabled)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%x)\n", This, aResponseTimeoutEnabled);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetApiRedirectToURI(nsIHttpChannelInternal *iface, nsIURI **aApiRedirectToURI)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%p)\n", This, aApiRedirectToURI);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetAllowAltSvc(nsIHttpChannelInternal *iface, cpp_bool *aAllowAltSvc)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%p)\n", This, aAllowAltSvc);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_SetAllowAltSvc(nsIHttpChannelInternal *iface, cpp_bool aAllowAltSvc)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%x)\n", This, aAllowAltSvc);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_AddRedirect(nsIHttpChannelInternal *iface, nsIPrincipal *aPrincipal)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%p)\n", This, aPrincipal);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetLastModifiedTime(nsIHttpChannelInternal *iface, PRTime *aLastModifiedTime)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%p)\n", This, aLastModifiedTime);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_ForceNoIntercept(nsIHttpChannelInternal *iface)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)\n", This);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetCorsIncludeCredentials(nsIHttpChannelInternal *iface,
        cpp_bool *aCorsIncludeCredentials)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%p)\n", This, aCorsIncludeCredentials);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_SetCorsIncludeCredentials(nsIHttpChannelInternal *iface,
        cpp_bool aCorsIncludeCredentials)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%x)\n", This, aCorsIncludeCredentials);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetCorsMode(nsIHttpChannelInternal *iface, UINT32 *aCorsMode)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%p)\n", This, aCorsMode);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_SetCorsMode(nsIHttpChannelInternal *iface, UINT32 aCorsMode)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%d)\n", This, aCorsMode);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetTopWindowURI(nsIHttpChannelInternal *iface, nsIURI **aTopWindowURI)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%p)\n", This, aTopWindowURI);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetNetworkInterfaceId(nsIHttpChannelInternal *iface,
        nsACString *aNetworkInterfaceId)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%p)\n", This, aNetworkInterfaceId);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_SetNetworkInterfaceId(nsIHttpChannelInternal *iface,
        const nsACString *aNetworkInterfaceId)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_nsacstr(aNetworkInterfaceId));
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_ContinueBeginConnect(nsIHttpChannelInternal *iface)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)\n", This);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetProxyURI(nsIHttpChannelInternal *iface, nsIURI **aProxyURI)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%p)\n", This, aProxyURI);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static const nsIHttpChannelInternalVtbl nsHttpChannelInternalVtbl = {
    nsHttpChannelInternal_QueryInterface,
    nsHttpChannelInternal_AddRef,
    nsHttpChannelInternal_Release,
    nsHttpChannelInternal_GetDocumentURI,
    nsHttpChannelInternal_SetDocumentURI,
    nsHttpChannelInternal_GetRequestVersion,
    nsHttpChannelInternal_GetResponseVersion,
    nsHttpChannelInternal_TakeAllSecurityMessages,
    nsHttpChannelInternal_SetCookie,
    nsHttpChannelInternal_SetupFallbackChannel,
    nsHttpChannelInternal_GetForceAllowThirdPartyCookie,
    nsHttpChannelInternal_SetForceAllowThirdPartyCookie,
    nsHttpChannelInternal_GetCanceled,
    nsHttpChannelInternal_GetChannelIsForDownload,
    nsHttpChannelInternal_SetChannelIsForDownload,
    nsHttpChannelInternal_GetLocalAddress,
    nsHttpChannelInternal_GetLocalPort,
    nsHttpChannelInternal_GetRemoteAddress,
    nsHttpChannelInternal_GetRemotePort,
    nsHttpChannelInternal_SetCacheKeysRedirectChain,
    nsHttpChannelInternal_HTTPUpgrade,
    nsHttpChannelInternal_GetAllowSpdy,
    nsHttpChannelInternal_SetAllowSpdy,
    nsHttpChannelInternal_GetResponseTimeoutEnabled,
    nsHttpChannelInternal_SetResponseTimeoutEnabled,
    nsHttpChannelInternal_GetApiRedirectToURI,
    nsHttpChannelInternal_GetAllowAltSvc,
    nsHttpChannelInternal_SetAllowAltSvc,
    nsHttpChannelInternal_AddRedirect,
    nsHttpChannelInternal_GetLastModifiedTime,
    nsHttpChannelInternal_ForceNoIntercept,
    nsHttpChannelInternal_GetCorsIncludeCredentials,
    nsHttpChannelInternal_SetCorsIncludeCredentials,
    nsHttpChannelInternal_GetCorsMode,
    nsHttpChannelInternal_SetCorsMode,
    nsHttpChannelInternal_GetTopWindowURI,
    nsHttpChannelInternal_GetNetworkInterfaceId,
    nsHttpChannelInternal_SetNetworkInterfaceId,
    nsHttpChannelInternal_ContinueBeginConnect,
    nsHttpChannelInternal_GetProxyURI
};


static void invalidate_uri(nsWineURI *This)
{
    if(This->uri) {
        IUri_Release(This->uri);
        This->uri = NULL;
    }
}

static BOOL ensure_uri_builder(nsWineURI *This)
{
    if(!This->is_mutable) {
        WARN("Not mutable URI\n");
        return FALSE;
    }

    if(!This->uri_builder) {
        HRESULT hres;

        if(!ensure_uri(This))
            return FALSE;

        hres = CreateIUriBuilder(This->uri, 0, 0, &This->uri_builder);
        if(FAILED(hres)) {
            WARN("CreateIUriBuilder failed: %08x\n", hres);
            return FALSE;
        }
    }

    invalidate_uri(This);
    return TRUE;
}

static nsresult get_uri_string(nsWineURI *This, Uri_PROPERTY prop, nsACString *ret)
{
    char *vala;
    BSTR val;
    HRESULT hres;

    if(!ensure_uri(This))
        return NS_ERROR_UNEXPECTED;

    hres = IUri_GetPropertyBSTR(This->uri, prop, &val, 0);
    if(FAILED(hres)) {
        WARN("GetPropertyBSTR failed: %08x\n", hres);
        return NS_ERROR_UNEXPECTED;
    }

    vala = heap_strdupWtoU(val);
    SysFreeString(val);
    if(!vala)
        return NS_ERROR_OUT_OF_MEMORY;

    TRACE("ret %s\n", debugstr_a(vala));
    nsACString_SetData(ret, vala);
    heap_free(vala);
    return NS_OK;
}

static inline nsWineURI *impl_from_nsIFileURL(nsIFileURL *iface)
{
    return CONTAINING_RECORD(iface, nsWineURI, nsIFileURL_iface);
}

static nsresult NSAPI nsURI_QueryInterface(nsIFileURL *iface, nsIIDRef riid, void **result)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);

    *result = NULL;

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports %p)\n", This, result);
        *result = &This->nsIFileURL_iface;
    }else if(IsEqualGUID(&IID_nsIURI, riid)) {
        TRACE("(%p)->(IID_nsIURI %p)\n", This, result);
        *result = &This->nsIFileURL_iface;
    }else if(IsEqualGUID(&IID_nsIURL, riid)) {
        TRACE("(%p)->(IID_nsIURL %p)\n", This, result);
        *result = &This->nsIFileURL_iface;
    }else if(IsEqualGUID(&IID_nsIFileURL, riid)) {
        TRACE("(%p)->(IID_nsIFileURL %p)\n", This, result);
        *result = This->scheme == URL_SCHEME_FILE ? &This->nsIFileURL_iface : NULL;
    }else if(IsEqualGUID(&IID_nsIMutable, riid)) {
        TRACE("(%p)->(IID_nsIMutable %p)\n", This, result);
        *result = &This->nsIStandardURL_iface;
    }else if(IsEqualGUID(&IID_nsIStandardURL, riid)) {
        TRACE("(%p)->(IID_nsIStandardURL %p)\n", This, result);
        *result = &This->nsIStandardURL_iface;
    }else if(IsEqualGUID(&IID_nsWineURI, riid)) {
        TRACE("(%p)->(IID_nsWineURI %p)\n", This, result);
        *result = This;
    }

    if(*result) {
        nsIFileURL_AddRef(&This->nsIFileURL_iface);
        return NS_OK;
    }

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), result);
    return NS_NOINTERFACE;
}

static nsrefcnt NSAPI nsURI_AddRef(nsIFileURL *iface)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static nsrefcnt NSAPI nsURI_Release(nsIFileURL *iface)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->window_ref)
            windowref_release(This->window_ref);
        if(This->container)
            nsIWebBrowserChrome_Release(&This->container->nsIWebBrowserChrome_iface);
        if(This->uri)
            IUri_Release(This->uri);
        heap_free(This->origin_charset);
        heap_free(This);
    }

    return ref;
}

static nsresult NSAPI nsURI_GetSpec(nsIFileURL *iface, nsACString *aSpec)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);

    TRACE("(%p)->(%p)\n", This, aSpec);

    return get_uri_string(This, Uri_PROPERTY_DISPLAY_URI, aSpec);
}

static nsresult NSAPI nsURI_SetSpec(nsIFileURL *iface, const nsACString *aSpec)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    const char *speca;
    WCHAR *spec;
    IUri *uri;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_nsacstr(aSpec));

    if(!This->is_mutable)
        return NS_ERROR_UNEXPECTED;

    nsACString_GetData(aSpec, &speca);
    spec = heap_strdupUtoW(speca);
    if(!spec)
        return NS_ERROR_OUT_OF_MEMORY;

    hres = create_uri(spec, 0, &uri);
    heap_free(spec);
    if(FAILED(hres)) {
        WARN("create_uri failed: %08x\n", hres);
        return NS_ERROR_FAILURE;
    }

    invalidate_uri(This);
    if(This->uri_builder) {
        IUriBuilder_Release(This->uri_builder);
        This->uri_builder = NULL;
    }

    This->uri = uri;
    return NS_OK;
}

static nsresult NSAPI nsURI_GetPrePath(nsIFileURL *iface, nsACString *aPrePath)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    IUriBuilder *uri_builder;
    BSTR display_uri;
    IUri *uri;
    int len;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, aPrePath);

    if(!ensure_uri(This))
        return NS_ERROR_UNEXPECTED;

    hres = CreateIUriBuilder(This->uri, 0, 0, &uri_builder);
    if(FAILED(hres))
        return NS_ERROR_FAILURE;

    hres = IUriBuilder_RemoveProperties(uri_builder, Uri_HAS_PATH|Uri_HAS_QUERY|Uri_HAS_FRAGMENT);
    if(SUCCEEDED(hres))
        hres = IUriBuilder_CreateUriSimple(uri_builder, 0, 0, &uri);
    IUriBuilder_Release(uri_builder);
    if(FAILED(hres))
        return NS_ERROR_FAILURE;

    hres = IUri_GetDisplayUri(uri, &display_uri);
    IUri_Release(uri);
    if(FAILED(hres))
        return NS_ERROR_FAILURE;

    /* Remove trailing slash that may be appended as default path. */
    len = SysStringLen(display_uri);
    if(len && display_uri[len-1] == '/')
        display_uri[len-1] = 0;

    nsres = return_wstr_nsacstr(aPrePath, display_uri, -1);
    SysFreeString(display_uri);
    return nsres;
}

static nsresult NSAPI nsURI_GetScheme(nsIFileURL *iface, nsACString *aScheme)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    DWORD scheme;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, aScheme);

    if(!ensure_uri(This))
        return NS_ERROR_UNEXPECTED;

    hres = IUri_GetScheme(This->uri, &scheme);
    if(FAILED(hres)) {
        WARN("GetScheme failed: %08x\n", hres);
        return NS_ERROR_UNEXPECTED;
    }

    if(scheme == URL_SCHEME_ABOUT) {
        nsACString_SetData(aScheme, "wine");
        return NS_OK;
    }

    return get_uri_string(This, Uri_PROPERTY_SCHEME_NAME, aScheme);
}

static nsresult NSAPI nsURI_SetScheme(nsIFileURL *iface, const nsACString *aScheme)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    const char *schemea;
    WCHAR *scheme;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_nsacstr(aScheme));

    if(!ensure_uri_builder(This))
        return NS_ERROR_UNEXPECTED;

    nsACString_GetData(aScheme, &schemea);
    scheme = heap_strdupUtoW(schemea);
    if(!scheme)
        return NS_ERROR_OUT_OF_MEMORY;

    hres = IUriBuilder_SetSchemeName(This->uri_builder, scheme);
    heap_free(scheme);
    if(FAILED(hres))
        return NS_ERROR_UNEXPECTED;

    return NS_OK;
}

static nsresult NSAPI nsURI_GetUserPass(nsIFileURL *iface, nsACString *aUserPass)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    BSTR user, pass;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, aUserPass);

    if(!ensure_uri(This))
        return NS_ERROR_UNEXPECTED;

    hres = IUri_GetUserName(This->uri, &user);
    if(FAILED(hres))
        return NS_ERROR_FAILURE;

    hres = IUri_GetPassword(This->uri, &pass);
    if(FAILED(hres)) {
        SysFreeString(user);
        return NS_ERROR_FAILURE;
    }

    if(*user || *pass) {
        FIXME("Construct user:pass string\n");
    }else {
        nsACString_SetData(aUserPass, "");
    }

    SysFreeString(user);
    SysFreeString(pass);
    return NS_OK;
}

static nsresult NSAPI nsURI_SetUserPass(nsIFileURL *iface, const nsACString *aUserPass)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    WCHAR *user = NULL, *pass = NULL, *buf = NULL;
    const char *user_pass;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_nsacstr(aUserPass));

    if(!ensure_uri_builder(This))
        return NS_ERROR_UNEXPECTED;

    nsACString_GetData(aUserPass, &user_pass);
    if(*user_pass) {
        WCHAR *ptr;

        buf = heap_strdupUtoW(user_pass);
        if(!buf)
            return NS_ERROR_OUT_OF_MEMORY;

        ptr = strchrW(buf, ':');
        if(!ptr) {
            user = buf;
        }else if(ptr != buf) {
            *ptr++ = 0;
            user = buf;
            if(*ptr)
                pass = ptr;
        }else {
            pass = buf+1;
        }
    }

    hres = IUriBuilder_SetUserName(This->uri_builder, user);
    if(SUCCEEDED(hres))
        hres = IUriBuilder_SetPassword(This->uri_builder, pass);

    heap_free(buf);
    return SUCCEEDED(hres) ? NS_OK : NS_ERROR_FAILURE;
}

static nsresult NSAPI nsURI_GetUsername(nsIFileURL *iface, nsACString *aUsername)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);

    TRACE("(%p)->(%p)\n", This, aUsername);

    return get_uri_string(This, Uri_PROPERTY_USER_NAME, aUsername);
}

static nsresult NSAPI nsURI_SetUsername(nsIFileURL *iface, const nsACString *aUsername)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    const char *usera;
    WCHAR *user;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_nsacstr(aUsername));

    if(!ensure_uri_builder(This))
        return NS_ERROR_UNEXPECTED;

    nsACString_GetData(aUsername, &usera);
    user = heap_strdupUtoW(usera);
    if(!user)
        return NS_ERROR_OUT_OF_MEMORY;

    hres = IUriBuilder_SetUserName(This->uri_builder, user);
    heap_free(user);
    if(FAILED(hres))
        return NS_ERROR_UNEXPECTED;

    return NS_OK;
}

static nsresult NSAPI nsURI_GetPassword(nsIFileURL *iface, nsACString *aPassword)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);

    TRACE("(%p)->(%p)\n", This, aPassword);

    return get_uri_string(This, Uri_PROPERTY_PASSWORD, aPassword);
}

static nsresult NSAPI nsURI_SetPassword(nsIFileURL *iface, const nsACString *aPassword)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    const char *passa;
    WCHAR *pass;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_nsacstr(aPassword));

    if(!ensure_uri_builder(This))
        return NS_ERROR_UNEXPECTED;

    nsACString_GetData(aPassword, &passa);
    pass = heap_strdupUtoW(passa);
    if(!pass)
        return NS_ERROR_OUT_OF_MEMORY;

    hres = IUriBuilder_SetPassword(This->uri_builder, pass);
    heap_free(pass);
    if(FAILED(hres))
        return NS_ERROR_UNEXPECTED;

    return NS_OK;
}

static nsresult NSAPI nsURI_GetHostPort(nsIFileURL *iface, nsACString *aHostPort)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    const WCHAR *ptr;
    char *vala;
    BSTR val;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, aHostPort);

    if(!ensure_uri(This))
        return NS_ERROR_UNEXPECTED;

    hres = IUri_GetAuthority(This->uri, &val);
    if(FAILED(hres)) {
        WARN("GetAuthority failed: %08x\n", hres);
        return NS_ERROR_UNEXPECTED;
    }

    ptr = strchrW(val, '@');
    if(!ptr)
        ptr = val;

    vala = heap_strdupWtoU(ptr);
    SysFreeString(val);
    if(!vala)
        return NS_ERROR_OUT_OF_MEMORY;

    TRACE("ret %s\n", debugstr_a(vala));
    nsACString_SetData(aHostPort, vala);
    heap_free(vala);
    return NS_OK;
}

static nsresult NSAPI nsURI_SetHostPort(nsIFileURL *iface, const nsACString *aHostPort)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);

    WARN("(%p)->(%s)\n", This, debugstr_nsacstr(aHostPort));

    /* Not implemented by Gecko */
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURI_GetHost(nsIFileURL *iface, nsACString *aHost)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);

    TRACE("(%p)->(%p)\n", This, aHost);

    return get_uri_string(This, Uri_PROPERTY_HOST, aHost);
}

static nsresult NSAPI nsURI_SetHost(nsIFileURL *iface, const nsACString *aHost)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    const char *hosta;
    WCHAR *host;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_nsacstr(aHost));

    if(!ensure_uri_builder(This))
        return NS_ERROR_UNEXPECTED;

    nsACString_GetData(aHost, &hosta);
    host = heap_strdupUtoW(hosta);
    if(!host)
        return NS_ERROR_OUT_OF_MEMORY;

    hres = IUriBuilder_SetHost(This->uri_builder, host);
    heap_free(host);
    if(FAILED(hres))
        return NS_ERROR_UNEXPECTED;

    return NS_OK;
}

static nsresult NSAPI nsURI_GetPort(nsIFileURL *iface, LONG *aPort)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    DWORD port;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, aPort);

    if(!ensure_uri(This))
        return NS_ERROR_UNEXPECTED;

    hres = IUri_GetPort(This->uri, &port);
    if(FAILED(hres)) {
        WARN("GetPort failed: %08x\n", hres);
        return NS_ERROR_UNEXPECTED;
    }

    *aPort = port ? port : -1;
    return NS_OK;
}

static nsresult NSAPI nsURI_SetPort(nsIFileURL *iface, LONG aPort)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    HRESULT hres;

    TRACE("(%p)->(%d)\n", This, aPort);

    if(!ensure_uri_builder(This))
        return NS_ERROR_UNEXPECTED;

    hres = IUriBuilder_SetPort(This->uri_builder, aPort != -1, aPort);
    return SUCCEEDED(hres) ? NS_OK : NS_ERROR_FAILURE;
}

static nsresult NSAPI nsURI_GetPath(nsIFileURL *iface, nsACString *aPath)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);

    TRACE("(%p)->(%p)\n", This, aPath);

    return get_uri_string(This, Uri_PROPERTY_PATH, aPath);
}

static nsresult NSAPI nsURI_SetPath(nsIFileURL *iface, const nsACString *aPath)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    const char *patha;
    WCHAR *path;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_nsacstr(aPath));

    if(!ensure_uri_builder(This))
        return NS_ERROR_UNEXPECTED;

    nsACString_GetData(aPath, &patha);
    path = heap_strdupUtoW(patha);
    if(!path)
        return NS_ERROR_OUT_OF_MEMORY;

    hres = IUriBuilder_SetPath(This->uri_builder, path);
    heap_free(path);
    if(FAILED(hres))
        return NS_ERROR_UNEXPECTED;

    return NS_OK;
}

static nsresult NSAPI nsURI_Equals(nsIFileURL *iface, nsIURI *other, cpp_bool *_retval)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    nsWineURI *other_obj;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p %p)\n", This, other, _retval);

    nsres = nsIURI_QueryInterface(other, &IID_nsWineURI, (void**)&other_obj);
    if(NS_FAILED(nsres)) {
        TRACE("Could not get nsWineURI interface\n");
        *_retval = FALSE;
        return NS_OK;
    }

    if(ensure_uri(This) && ensure_uri(other_obj)) {
        BOOL b;

        hres = IUri_IsEqual(This->uri, other_obj->uri, &b);
        if(SUCCEEDED(hres)) {
            *_retval = b;
            nsres = NS_OK;
        }else {
            nsres = NS_ERROR_FAILURE;
        }
    }else {
        nsres = NS_ERROR_UNEXPECTED;
    }

    nsIFileURL_Release(&other_obj->nsIFileURL_iface);
    return nsres;
}

static nsresult NSAPI nsURI_SchemeIs(nsIFileURL *iface, const char *scheme, cpp_bool *_retval)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    WCHAR buf[INTERNET_MAX_SCHEME_LENGTH];
    BSTR scheme_name;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_a(scheme), _retval);

    if(!ensure_uri(This))
        return NS_ERROR_UNEXPECTED;

    hres = IUri_GetSchemeName(This->uri, &scheme_name);
    if(FAILED(hres))
        return NS_ERROR_UNEXPECTED;

    MultiByteToWideChar(CP_UTF8, 0, scheme, -1, buf, sizeof(buf)/sizeof(WCHAR));
    *_retval = !strcmpW(scheme_name, buf);
    SysFreeString(scheme_name);
    return NS_OK;
}

static nsresult NSAPI nsURI_Clone(nsIFileURL *iface, nsIURI **_retval)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    nsWineURI *wine_uri;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, _retval);

    if(!ensure_uri(This))
        return NS_ERROR_UNEXPECTED;

    nsres = create_nsuri(This->uri, This->window_ref ? This->window_ref->window : NULL,
            This->container, This->origin_charset, &wine_uri);
    if(NS_FAILED(nsres)) {
        WARN("create_nsuri failed: %08x\n", nsres);
        return nsres;
    }

    *_retval = (nsIURI*)&wine_uri->nsIFileURL_iface;
    return NS_OK;
}

static nsresult NSAPI nsURI_Resolve(nsIFileURL *iface, const nsACString *aRelativePath,
        nsACString *_retval)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    const char *patha;
    IUri *new_uri;
    WCHAR *path;
    char *reta;
    BSTR ret;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_nsacstr(aRelativePath), _retval);

    if(!ensure_uri(This))
        return NS_ERROR_UNEXPECTED;

    nsACString_GetData(aRelativePath, &patha);
    path = heap_strdupUtoW(patha);
    if(!path)
        return NS_ERROR_OUT_OF_MEMORY;

    hres = combine_url(This->uri, path, &new_uri);
    heap_free(path);
    if(FAILED(hres))
        return NS_ERROR_FAILURE;

    hres = IUri_GetDisplayUri(new_uri, &ret);
    IUri_Release(new_uri);
    if(FAILED(hres))
        return NS_ERROR_FAILURE;

    reta = heap_strdupWtoU(ret);
    SysFreeString(ret);
    if(!reta)
        return NS_ERROR_OUT_OF_MEMORY;

    TRACE("returning %s\n", debugstr_a(reta));
    nsACString_SetData(_retval, reta);
    heap_free(reta);
    return NS_OK;
}

static nsresult NSAPI nsURI_GetAsciiSpec(nsIFileURL *iface, nsACString *aAsciiSpec)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);

    TRACE("(%p)->(%p)\n", This, aAsciiSpec);

    return nsIFileURL_GetSpec(&This->nsIFileURL_iface, aAsciiSpec);
}

static nsresult NSAPI nsURI_GetAsciiHost(nsIFileURL *iface, nsACString *aAsciiHost)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);

    WARN("(%p)->(%p) FIXME: Use Uri_PUNYCODE_IDN_HOST flag\n", This, aAsciiHost);

    return get_uri_string(This, Uri_PROPERTY_HOST, aAsciiHost);
}

static nsresult NSAPI nsURI_GetOriginCharset(nsIFileURL *iface, nsACString *aOriginCharset)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);

    TRACE("(%p)->(%p)\n", This, aOriginCharset);

    nsACString_SetData(aOriginCharset, This->origin_charset);
    return NS_OK;
}

static nsresult NSAPI nsURL_GetRef(nsIFileURL *iface, nsACString *aRef)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    char *refa = NULL;
    BSTR ref;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, aRef);

    if(!ensure_uri(This))
        return NS_ERROR_UNEXPECTED;

    hres = IUri_GetFragment(This->uri, &ref);
    if(FAILED(hres))
        return NS_ERROR_UNEXPECTED;

    refa = heap_strdupWtoU(ref);
    SysFreeString(ref);
    if(ref && !refa)
        return NS_ERROR_OUT_OF_MEMORY;

    nsACString_SetData(aRef, refa && *refa == '#' ? refa+1 : refa);
    heap_free(refa);
    return NS_OK;
}

static nsresult NSAPI nsURL_SetRef(nsIFileURL *iface, const nsACString *aRef)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    const char *refa;
    WCHAR *ref;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_nsacstr(aRef));

    if(!ensure_uri_builder(This))
        return NS_ERROR_UNEXPECTED;

    nsACString_GetData(aRef, &refa);
    ref = heap_strdupUtoW(refa);
    if(!ref)
        return NS_ERROR_OUT_OF_MEMORY;

    hres = IUriBuilder_SetFragment(This->uri_builder, ref);
    heap_free(ref);
    if(FAILED(hres))
        return NS_ERROR_UNEXPECTED;

    return NS_OK;
}

static nsresult NSAPI nsURI_EqualsExceptRef(nsIFileURL *iface, nsIURI *other, cpp_bool *_retval)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    nsWineURI *other_obj;
    nsresult nsres;

    TRACE("(%p)->(%p %p)\n", This, other, _retval);

    nsres = nsIURI_QueryInterface(other, &IID_nsWineURI, (void**)&other_obj);
    if(NS_FAILED(nsres)) {
        TRACE("Could not get nsWineURI interface\n");
        *_retval = FALSE;
        return NS_OK;
    }

    if(ensure_uri(This) && ensure_uri(other_obj)) {
        *_retval = compare_ignoring_frag(This->uri, other_obj->uri);
        nsres = NS_OK;
    }else {
        nsres = NS_ERROR_UNEXPECTED;
    }

    nsIFileURL_Release(&other_obj->nsIFileURL_iface);
    return nsres;
}

static nsresult NSAPI nsURI_CloneIgnoreRef(nsIFileURL *iface, nsIURI **_retval)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    nsWineURI *wine_uri;
    IUri *uri;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, _retval);

    if(!ensure_uri(This))
        return NS_ERROR_UNEXPECTED;

    uri = get_uri_nofrag(This->uri);
    if(!uri)
        return NS_ERROR_FAILURE;

    nsres = create_nsuri(uri, This->window_ref ? This->window_ref->window : NULL, This->container,
            This->origin_charset, &wine_uri);
    IUri_Release(uri);
    if(NS_FAILED(nsres)) {
        WARN("create_nsuri failed: %08x\n", nsres);
        return nsres;
    }

    *_retval = (nsIURI*)&wine_uri->nsIFileURL_iface;
    return NS_OK;
}

static nsresult NSAPI nsURI_GetSpecIgnoringRef(nsIFileURL *iface, nsACString *aSpecIgnoringRef)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);

    FIXME("(%p)->(%p)\n", This, aSpecIgnoringRef);

    return nsIFileURL_GetSpec(&This->nsIFileURL_iface, aSpecIgnoringRef);
}

static nsresult NSAPI nsURI_GetHasRef(nsIFileURL *iface, cpp_bool *aHasRef)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    BOOL b;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, aHasRef);

    if(!ensure_uri(This))
        return NS_ERROR_UNEXPECTED;

    hres = IUri_HasProperty(This->uri, Uri_PROPERTY_FRAGMENT, &b);
    if(FAILED(hres))
        return NS_ERROR_FAILURE;

    *aHasRef = b;
    return NS_OK;
}

static nsresult NSAPI nsURL_GetFilePath(nsIFileURL *iface, nsACString *aFilePath)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);

    TRACE("(%p)->(%p)\n", This, aFilePath);

    return nsIFileURL_GetPath(&This->nsIFileURL_iface, aFilePath);
}

static nsresult NSAPI nsURL_SetFilePath(nsIFileURL *iface, const nsACString *aFilePath)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_nsacstr(aFilePath));

    if(!This->is_mutable)
        return NS_ERROR_UNEXPECTED;

    return nsIFileURL_SetPath(&This->nsIFileURL_iface, aFilePath);
}

static nsresult NSAPI nsURL_GetQuery(nsIFileURL *iface, nsACString *aQuery)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    WCHAR *ptr;
    BSTR query;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, aQuery);

    if(!ensure_uri(This))
        return NS_ERROR_UNEXPECTED;

    hres = IUri_GetQuery(This->uri, &query);
    if(FAILED(hres))
        return NS_ERROR_FAILURE;

    ptr = query;
    if(ptr && *ptr == '?')
        ptr++;

    nsres = return_wstr_nsacstr(aQuery, ptr, -1);
    SysFreeString(query);
    return nsres;
}

static nsresult NSAPI nsURL_SetQuery(nsIFileURL *iface, const nsACString *aQuery)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    const char *querya;
    WCHAR *query;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_nsacstr(aQuery));

    if(!ensure_uri_builder(This))
        return NS_ERROR_UNEXPECTED;

    nsACString_GetData(aQuery, &querya);
    query = heap_strdupUtoW(querya);
    if(!query)
        return NS_ERROR_OUT_OF_MEMORY;

    hres = IUriBuilder_SetQuery(This->uri_builder, query);
    heap_free(query);
    if(FAILED(hres))
        return NS_ERROR_UNEXPECTED;

    return NS_OK;
}

static nsresult get_uri_path(nsWineURI *This, BSTR *path, const WCHAR **file, const WCHAR **ext)
{
    const WCHAR *ptr;
    HRESULT hres;

    if(!ensure_uri(This))
        return NS_ERROR_UNEXPECTED;

    hres = IUri_GetPath(This->uri, path);
    if(FAILED(hres))
        return NS_ERROR_FAILURE;

    for(ptr = *path + SysStringLen(*path)-1; ptr > *path && *ptr != '/' && *ptr != '\\'; ptr--);
    if(*ptr == '/' || *ptr == '\\')
        ptr++;
    *file = ptr;

    if(ext) {
        ptr = strrchrW(ptr, '.');
        if(!ptr)
            ptr = *path + SysStringLen(*path);
        *ext = ptr;
    }

    return NS_OK;
}

static nsresult NSAPI nsURL_GetDirectory(nsIFileURL *iface, nsACString *aDirectory)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    const WCHAR *file;
    BSTR path;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, aDirectory);

    nsres = get_uri_path(This, &path, &file, NULL);
    if(NS_FAILED(nsres))
        return nsres;

    nsres = return_wstr_nsacstr(aDirectory, path, file-path);
    SysFreeString(path);
    return nsres;
}

static nsresult NSAPI nsURL_SetDirectory(nsIFileURL *iface, const nsACString *aDirectory)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);

    WARN("(%p)->(%s)\n", This, debugstr_nsacstr(aDirectory));

    /* Not implemented by Gecko */
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_GetFileName(nsIFileURL *iface, nsACString *aFileName)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    const WCHAR *file;
    BSTR path;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, aFileName);

    nsres = get_uri_path(This, &path, &file, NULL);
    if(NS_FAILED(nsres))
        return nsres;

    nsres = return_wstr_nsacstr(aFileName, file, -1);
    SysFreeString(path);
    return nsres;
}

static nsresult NSAPI nsURL_SetFileName(nsIFileURL *iface, const nsACString *aFileName)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_nsacstr(aFileName));
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_GetFileBaseName(nsIFileURL *iface, nsACString *aFileBaseName)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    const WCHAR *file, *ext;
    BSTR path;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, aFileBaseName);

    nsres = get_uri_path(This, &path, &file, &ext);
    if(NS_FAILED(nsres))
        return nsres;

    nsres = return_wstr_nsacstr(aFileBaseName, file, ext-file);
    SysFreeString(path);
    return nsres;
}

static nsresult NSAPI nsURL_SetFileBaseName(nsIFileURL *iface, const nsACString *aFileBaseName)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_nsacstr(aFileBaseName));
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_GetFileExtension(nsIFileURL *iface, nsACString *aFileExtension)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);

    TRACE("(%p)->(%p)\n", This, aFileExtension);

    return get_uri_string(This, Uri_PROPERTY_EXTENSION, aFileExtension);
}

static nsresult NSAPI nsURL_SetFileExtension(nsIFileURL *iface, const nsACString *aFileExtension)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_nsacstr(aFileExtension));
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_GetCommonBaseSpec(nsIFileURL *iface, nsIURI *aURIToCompare, nsACString *_retval)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    FIXME("(%p)->(%p %p)\n", This, aURIToCompare, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsURL_GetRelativeSpec(nsIFileURL *iface, nsIURI *aURIToCompare, nsACString *_retval)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    FIXME("(%p)->(%p %p)\n", This, aURIToCompare, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsFileURL_GetFile(nsIFileURL *iface, nsIFile **aFile)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    WCHAR path[MAX_PATH];
    DWORD size;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, aFile);

    hres = CoInternetParseIUri(This->uri, PARSE_PATH_FROM_URL, 0, path, sizeof(path)/sizeof(WCHAR), &size, 0);
    if(FAILED(hres)) {
        WARN("CoInternetParseIUri failed: %08x\n", hres);
        return NS_ERROR_FAILURE;
    }

    return create_nsfile(path, aFile);
}

static nsresult NSAPI nsFileURL_SetFile(nsIFileURL *iface, nsIFile *aFile)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    FIXME("(%p)->(%p)\n", This, aFile);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static const nsIFileURLVtbl nsFileURLVtbl = {
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
    nsURL_GetRef,
    nsURL_SetRef,
    nsURI_EqualsExceptRef,
    nsURI_CloneIgnoreRef,
    nsURI_GetSpecIgnoringRef,
    nsURI_GetHasRef,
    nsURL_GetFilePath,
    nsURL_SetFilePath,
    nsURL_GetQuery,
    nsURL_SetQuery,
    nsURL_GetDirectory,
    nsURL_SetDirectory,
    nsURL_GetFileName,
    nsURL_SetFileName,
    nsURL_GetFileBaseName,
    nsURL_SetFileBaseName,
    nsURL_GetFileExtension,
    nsURL_SetFileExtension,
    nsURL_GetCommonBaseSpec,
    nsURL_GetRelativeSpec,
    nsFileURL_GetFile,
    nsFileURL_SetFile
};

static inline nsWineURI *impl_from_nsIStandardURL(nsIStandardURL *iface)
{
    return CONTAINING_RECORD(iface, nsWineURI, nsIStandardURL_iface);
}

static nsresult NSAPI nsStandardURL_QueryInterface(nsIStandardURL *iface, nsIIDRef riid,
        void **result)
{
    nsWineURI *This = impl_from_nsIStandardURL(iface);
    return nsIFileURL_QueryInterface(&This->nsIFileURL_iface, riid, result);
}

static nsrefcnt NSAPI nsStandardURL_AddRef(nsIStandardURL *iface)
{
    nsWineURI *This = impl_from_nsIStandardURL(iface);
    return nsIFileURL_AddRef(&This->nsIFileURL_iface);
}

static nsrefcnt NSAPI nsStandardURL_Release(nsIStandardURL *iface)
{
    nsWineURI *This = impl_from_nsIStandardURL(iface);
    return nsIFileURL_Release(&This->nsIFileURL_iface);
}

static nsresult NSAPI nsStandardURL_GetMutable(nsIStandardURL *iface, cpp_bool *aMutable)
{
    nsWineURI *This = impl_from_nsIStandardURL(iface);

    TRACE("(%p)->(%p)\n", This, aMutable);

    *aMutable = This->is_mutable;
    return NS_OK;
}

static nsresult NSAPI nsStandardURL_SetMutable(nsIStandardURL *iface, cpp_bool aMutable)
{
    nsWineURI *This = impl_from_nsIStandardURL(iface);

    TRACE("(%p)->(%x)\n", This, aMutable);

    This->is_mutable = aMutable;
    return NS_OK;
}

static nsresult NSAPI nsStandardURL_Init(nsIStandardURL *iface, UINT32 aUrlType, LONG aDefaultPort,
        const nsACString *aSpec, const char *aOriginCharset, nsIURI *aBaseURI)
{
    nsWineURI *This = impl_from_nsIStandardURL(iface);
    FIXME("(%p)->(%d %d %s %s %p)\n", This, aUrlType, aDefaultPort, debugstr_nsacstr(aSpec), debugstr_a(aOriginCharset), aBaseURI);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static const nsIStandardURLVtbl nsStandardURLVtbl = {
    nsStandardURL_QueryInterface,
    nsStandardURL_AddRef,
    nsStandardURL_Release,
    nsStandardURL_GetMutable,
    nsStandardURL_SetMutable,
    nsStandardURL_Init
};

static nsresult create_nsuri(IUri *iuri, HTMLOuterWindow *window, NSContainer *container,
        const char *origin_charset, nsWineURI **_retval)
{
    nsWineURI *ret;
    HRESULT hres;

    ret = heap_alloc_zero(sizeof(nsWineURI));
    if(!ret)
        return NS_ERROR_OUT_OF_MEMORY;

    ret->nsIFileURL_iface.lpVtbl = &nsFileURLVtbl;
    ret->nsIStandardURL_iface.lpVtbl = &nsStandardURLVtbl;
    ret->ref = 1;
    ret->is_mutable = TRUE;

    set_uri_nscontainer(ret, container);
    set_uri_window(ret, window);

    IUri_AddRef(iuri);
    ret->uri = iuri;

    hres = IUri_GetScheme(iuri, &ret->scheme);
    if(FAILED(hres))
        ret->scheme = URL_SCHEME_UNKNOWN;

    if(origin_charset && *origin_charset && strcmp(origin_charset, "UTF-8")) {
        ret->origin_charset = heap_strdupA(origin_charset);
        if(!ret->origin_charset) {
            nsIFileURL_Release(&ret->nsIFileURL_iface);
            return NS_ERROR_OUT_OF_MEMORY;
        }
    }

    TRACE("retval=%p\n", ret);
    *_retval = ret;
    return NS_OK;
}

HRESULT create_doc_uri(HTMLOuterWindow *window, IUri *iuri, nsWineURI **ret)
{
    nsWineURI *uri;
    nsresult nsres;

    nsres = create_nsuri(iuri, window, window->doc_obj->nscontainer, NULL, &uri);
    if(NS_FAILED(nsres))
        return E_FAIL;

    uri->is_doc_uri = TRUE;

    *ret = uri;
    return S_OK;
}

static nsresult create_nschannel(nsWineURI *uri, nsChannel **ret)
{
    nsChannel *channel;

    if(!ensure_uri(uri))
        return NS_ERROR_UNEXPECTED;

    channel = heap_alloc_zero(sizeof(nsChannel));
    if(!channel)
        return NS_ERROR_OUT_OF_MEMORY;

    channel->nsIHttpChannel_iface.lpVtbl = &nsChannelVtbl;
    channel->nsIUploadChannel_iface.lpVtbl = &nsUploadChannelVtbl;
    channel->nsIHttpChannelInternal_iface.lpVtbl = &nsHttpChannelInternalVtbl;
    channel->ref = 1;
    channel->request_method = METHOD_GET;
    list_init(&channel->response_headers);
    list_init(&channel->request_headers);

    nsIFileURL_AddRef(&uri->nsIFileURL_iface);
    channel->uri = uri;

    *ret = channel;
    return NS_OK;
}

HRESULT create_redirect_nschannel(const WCHAR *url, nsChannel *orig_channel, nsChannel **ret)
{
    HTMLOuterWindow *window = NULL;
    nsChannel *channel;
    nsWineURI *uri;
    IUri *iuri;
    nsresult nsres;
    HRESULT hres;

    hres = create_uri(url, 0, &iuri);
    if(FAILED(hres))
        return hres;

    if(orig_channel->uri->window_ref)
        window = orig_channel->uri->window_ref->window;
    nsres = create_nsuri(iuri, window, NULL, NULL, &uri);
    IUri_Release(iuri);
    if(NS_FAILED(nsres))
        return E_FAIL;

    nsres = create_nschannel(uri, &channel);
    nsIFileURL_Release(&uri->nsIFileURL_iface);
    if(NS_FAILED(nsres))
        return E_FAIL;

    if(orig_channel->load_group) {
        nsILoadGroup_AddRef(orig_channel->load_group);
        channel->load_group = orig_channel->load_group;
    }

    if(orig_channel->notif_callback) {
        nsIInterfaceRequestor_AddRef(orig_channel->notif_callback);
        channel->notif_callback = orig_channel->notif_callback;
    }

    channel->load_flags = orig_channel->load_flags | LOAD_REPLACE;

    if(orig_channel->request_method == METHOD_POST)
        FIXME("unsupported POST method\n");

    if(orig_channel->original_uri) {
        nsIURI_AddRef(orig_channel->original_uri);
        channel->original_uri = orig_channel->original_uri;
    }

    if(orig_channel->referrer) {
        nsIURI_AddRef(orig_channel->referrer);
        channel->referrer = orig_channel->referrer;
    }

    *ret = channel;
    return S_OK;
}

typedef struct {
    nsIProtocolHandler nsIProtocolHandler_iface;

    LONG ref;

    nsIProtocolHandler *nshandler;
} nsProtocolHandler;

static inline nsProtocolHandler *impl_from_nsIProtocolHandler(nsIProtocolHandler *iface)
{
    return CONTAINING_RECORD(iface, nsProtocolHandler, nsIProtocolHandler_iface);
}

static nsresult NSAPI nsProtocolHandler_QueryInterface(nsIProtocolHandler *iface, nsIIDRef riid,
        void **result)
{
    nsProtocolHandler *This = impl_from_nsIProtocolHandler(iface);

    *result = NULL;

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports %p)\n", This, result);
        *result = &This->nsIProtocolHandler_iface;
    }else if(IsEqualGUID(&IID_nsIProtocolHandler, riid)) {
        TRACE("(%p)->(IID_nsIProtocolHandler %p)\n", This, result);
        *result = &This->nsIProtocolHandler_iface;
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
    nsProtocolHandler *This = impl_from_nsIProtocolHandler(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static nsrefcnt NSAPI nsProtocolHandler_Release(nsIProtocolHandler *iface)
{
    nsProtocolHandler *This = impl_from_nsIProtocolHandler(iface);
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
    nsProtocolHandler *This = impl_from_nsIProtocolHandler(iface);

    TRACE("(%p)->(%p)\n", This, aScheme);

    if(This->nshandler)
        return nsIProtocolHandler_GetScheme(This->nshandler, aScheme);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsProtocolHandler_GetDefaultPort(nsIProtocolHandler *iface,
        LONG *aDefaultPort)
{
    nsProtocolHandler *This = impl_from_nsIProtocolHandler(iface);

    TRACE("(%p)->(%p)\n", This, aDefaultPort);

    if(This->nshandler)
        return nsIProtocolHandler_GetDefaultPort(This->nshandler, aDefaultPort);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsProtocolHandler_GetProtocolFlags(nsIProtocolHandler *iface,
                                                         UINT32 *aProtocolFlags)
{
    nsProtocolHandler *This = impl_from_nsIProtocolHandler(iface);

    TRACE("(%p)->(%p)\n", This, aProtocolFlags);

    if(This->nshandler)
        return nsIProtocolHandler_GetProtocolFlags(This->nshandler, aProtocolFlags);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsProtocolHandler_NewURI(nsIProtocolHandler *iface,
        const nsACString *aSpec, const char *aOriginCharset, nsIURI *aBaseURI, nsIURI **_retval)
{
    nsProtocolHandler *This = impl_from_nsIProtocolHandler(iface);

    TRACE("((%p)->%s %s %p %p)\n", This, debugstr_nsacstr(aSpec), debugstr_a(aOriginCharset),
          aBaseURI, _retval);

    if(This->nshandler)
        return nsIProtocolHandler_NewURI(This->nshandler, aSpec, aOriginCharset, aBaseURI, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsProtocolHandler_NewChannel2(nsIProtocolHandler *iface,
        nsIURI *aURI, nsILoadInfo *aLoadInfo, nsIChannel **_retval)
{
    nsProtocolHandler *This = impl_from_nsIProtocolHandler(iface);

    TRACE("(%p)->(%p %p %p)\n", This, aURI, aLoadInfo, _retval);

    if(This->nshandler)
        return nsIProtocolHandler_NewChannel2(This->nshandler, aURI, aLoadInfo, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsProtocolHandler_NewChannel(nsIProtocolHandler *iface,
        nsIURI *aURI, nsIChannel **_retval)
{
    nsProtocolHandler *This = impl_from_nsIProtocolHandler(iface);

    TRACE("(%p)->(%p %p)\n", This, aURI, _retval);

    if(This->nshandler)
        return nsIProtocolHandler_NewChannel(This->nshandler, aURI, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsProtocolHandler_AllowPort(nsIProtocolHandler *iface,
        LONG port, const char *scheme, cpp_bool *_retval)
{
    nsProtocolHandler *This = impl_from_nsIProtocolHandler(iface);

    TRACE("(%p)->(%d %s %p)\n", This, port, debugstr_a(scheme), _retval);

    if(This->nshandler)
        return nsIProtocolHandler_AllowPort(This->nshandler, port, scheme, _retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static const nsIProtocolHandlerVtbl nsProtocolHandlerVtbl = {
    nsProtocolHandler_QueryInterface,
    nsProtocolHandler_AddRef,
    nsProtocolHandler_Release,
    nsProtocolHandler_GetScheme,
    nsProtocolHandler_GetDefaultPort,
    nsProtocolHandler_GetProtocolFlags,
    nsProtocolHandler_NewURI,
    nsProtocolHandler_NewChannel2,
    nsProtocolHandler_NewChannel,
    nsProtocolHandler_AllowPort
};

static nsresult NSAPI nsIOService_QueryInterface(nsIIOService*,nsIIDRef,void**);

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
    nsProtocolHandler *ret;
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

    ret = heap_alloc(sizeof(nsProtocolHandler));
    if(!ret)
        return NS_ERROR_OUT_OF_MEMORY;

    ret->nsIProtocolHandler_iface.lpVtbl = &nsProtocolHandlerVtbl;
    ret->ref = 1;
    ret->nshandler = nshandler;
    *_retval = &ret->nsIProtocolHandler_iface;

    TRACE("return %p\n", *_retval);
    return NS_OK;
}

static nsresult NSAPI nsIOService_GetProtocolFlags(nsIIOService *iface, const char *aScheme,
                                                    UINT32 *_retval)
{
    TRACE("(%s %p)\n", debugstr_a(aScheme), _retval);
    return nsIIOService_GetProtocolFlags(nsio, aScheme, _retval);
}

static BOOL is_gecko_special_uri(const char *spec)
{
    static const char *special_schemes[] = {"chrome:", "data:", "jar:", "moz-safe-about", "resource:", "javascript:", "wyciwyg:"};
    unsigned int i;

    for(i=0; i < sizeof(special_schemes)/sizeof(*special_schemes); i++) {
        if(!strncasecmp(spec, special_schemes[i], strlen(special_schemes[i])))
            return TRUE;
    }

    if(!strncasecmp(spec, "file:", 5)) {
        const char *ptr = spec+5;
        while(*ptr == '/')
            ptr++;
        return is_gecko_path(ptr);
    }

    return FALSE;
}

static nsresult NSAPI nsIOService_NewURI(nsIIOService *iface, const nsACString *aSpec,
        const char *aOriginCharset, nsIURI *aBaseURI, nsIURI **_retval)
{
    nsWineURI *wine_uri, *base_wine_uri = NULL;
    WCHAR new_spec[INTERNET_MAX_URL_LENGTH];
    HTMLOuterWindow *window = NULL;
    const char *spec = NULL;
    UINT cp = CP_UTF8;
    IUri *urlmon_uri;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%s %s %p %p)\n", debugstr_nsacstr(aSpec), debugstr_a(aOriginCharset),
          aBaseURI, _retval);

    nsACString_GetData(aSpec, &spec);
    if(is_gecko_special_uri(spec))
        return nsIIOService_NewURI(nsio, aSpec, aOriginCharset, aBaseURI, _retval);

    if(!strncmp(spec, "wine:", 5))
        spec += 5;

    if(aBaseURI) {
        nsres = nsIURI_QueryInterface(aBaseURI, &IID_nsWineURI, (void**)&base_wine_uri);
        if(NS_SUCCEEDED(nsres)) {
            if(!ensure_uri(base_wine_uri))
                return NS_ERROR_UNEXPECTED;
            if(base_wine_uri->window_ref)
                window = base_wine_uri->window_ref->window;
        }else {
            WARN("Could not get base nsWineURI: %08x\n", nsres);
        }
    }

    if(aOriginCharset && *aOriginCharset && strncasecmp(aOriginCharset, "utf", 3)) {
        BSTR charset;
        int len;

        len = MultiByteToWideChar(CP_UTF8, 0, aOriginCharset, -1, NULL, 0);
        charset = SysAllocStringLen(NULL, len-1);
        if(!charset)
            return NS_ERROR_OUT_OF_MEMORY;
        MultiByteToWideChar(CP_UTF8, 0, aOriginCharset, -1, charset, len);

        cp = cp_from_charset_string(charset);

        SysFreeString(charset);
    }

    MultiByteToWideChar(cp, 0, spec, -1, new_spec, sizeof(new_spec)/sizeof(WCHAR));

    if(base_wine_uri) {
        hres = combine_url(base_wine_uri->uri, new_spec, &urlmon_uri);
    }else {
        hres = create_uri(new_spec, 0, &urlmon_uri);
        if(FAILED(hres))
            WARN("create_uri failed: %08x\n", hres);
    }

    if(FAILED(hres))
        return nsIIOService_NewURI(nsio, aSpec, aOriginCharset, aBaseURI, _retval);

    nsres = create_nsuri(urlmon_uri, window, NULL, NULL, &wine_uri);
    IUri_Release(urlmon_uri);
    if(base_wine_uri)
        nsIFileURL_Release(&base_wine_uri->nsIFileURL_iface);
    if(NS_FAILED(nsres))
        return nsres;

    *_retval = (nsIURI*)&wine_uri->nsIFileURL_iface;
    return nsres;
}

static nsresult NSAPI nsIOService_NewFileURI(nsIIOService *iface, nsIFile *aFile,
                                             nsIURI **_retval)
{
    TRACE("(%p %p)\n", aFile, _retval);
    return nsIIOService_NewFileURI(nsio, aFile, _retval);
}

static nsresult new_channel_from_uri(nsIURI *uri, nsILoadInfo *load_info, nsIChannel **_retval)
{
    nsWineURI *wine_uri;
    nsChannel *ret;
    nsresult nsres;

    nsres = nsIURI_QueryInterface(uri, &IID_nsWineURI, (void**)&wine_uri);
    if(NS_FAILED(nsres)) {
        TRACE("Could not get nsWineURI: %08x\n", nsres);
        return nsIIOService_NewChannelFromURI(nsio, uri, _retval);
    }

    nsres = create_nschannel(wine_uri, &ret);
    nsIFileURL_Release(&wine_uri->nsIFileURL_iface);
    if(NS_FAILED(nsres))
        return nsres;

    nsIURI_AddRef(uri);
    ret->original_uri = uri;

    if(load_info)
        nsIHttpChannel_SetLoadInfo(&ret->nsIHttpChannel_iface, load_info);

    *_retval = (nsIChannel*)&ret->nsIHttpChannel_iface;
    return NS_OK;
}

static nsresult NSAPI nsIOService_NewChannelFromURI2(nsIIOService *iface, nsIURI *aURI,
        nsIDOMNode *aLoadingNode, nsIPrincipal *aLoadingPrincipal, nsIPrincipal *aTriggeringPrincipal,
        UINT32 aSecurityFlags, UINT32 aContentPolicyType, nsIChannel **_retval)
{
    nsILoadInfo *load_info = NULL;
    nsresult nsres;

    TRACE("(%p %p %p %p %x %d %p)\n", aURI, aLoadingNode, aLoadingPrincipal, aTriggeringPrincipal,
          aSecurityFlags, aContentPolicyType, _retval);

    if(aLoadingNode || aLoadingPrincipal) {
        nsres = nsIIOService_NewLoadInfo(nsio, aLoadingPrincipal, aTriggeringPrincipal, aLoadingNode,
                aSecurityFlags, aContentPolicyType, &load_info);
        assert(nsres == NS_OK);
    }

    nsres = new_channel_from_uri(aURI, load_info, _retval);
    if(load_info)
        nsISupports_Release(load_info);
    return nsres;
}

static nsresult NSAPI nsIOService_NewChannelFromURIWithLoadInfo(nsIIOService *iface, nsIURI *aURI,
        nsILoadInfo *aLoadInfo, nsIChannel **_retval)
{
    TRACE("(%p %p %p)\n", aURI, aLoadInfo, _retval);
    return new_channel_from_uri(aURI, aLoadInfo, _retval);
}

static nsresult NSAPI nsIOService_NewChannelFromURI(nsIIOService *iface, nsIURI *aURI,
        nsIChannel **_retval)
{
    TRACE("(%p %p)\n", aURI, _retval);
    return new_channel_from_uri(aURI, NULL, _retval);
}

static nsresult NSAPI nsIOService_NewChannel2(nsIIOService *iface, const nsACString *aSpec,
        const char *aOriginCharset, nsIURI *aBaseURI, nsIDOMNode *aLoadingNode, nsIPrincipal *aLoadingPrincipal,
         nsIPrincipal *aTriggeringPrincipal, UINT32 aSecurityFlags, UINT32 aContentPolicyType, nsIChannel **_retval)
{
    TRACE("(%s %s %p %p %p %p %x %d %p)\n", debugstr_nsacstr(aSpec), debugstr_a(aOriginCharset), aBaseURI,
          aLoadingNode, aLoadingPrincipal, aTriggeringPrincipal, aSecurityFlags, aContentPolicyType, _retval);
    return nsIIOService_NewChannel2(nsio, aSpec, aOriginCharset, aBaseURI, aLoadingNode, aLoadingPrincipal,
            aTriggeringPrincipal, aSecurityFlags, aContentPolicyType, _retval);
}

static nsresult NSAPI nsIOService_NewChannel(nsIIOService *iface, const nsACString *aSpec,
        const char *aOriginCharset, nsIURI *aBaseURI, nsIChannel **_retval)
{
    TRACE("(%s %s %p %p)\n", debugstr_nsacstr(aSpec), debugstr_a(aOriginCharset), aBaseURI, _retval);
    return nsIIOService_NewChannel(nsio, aSpec, aOriginCharset, aBaseURI, _retval);
}

static nsresult NSAPI nsIOService_GetOffline(nsIIOService *iface, cpp_bool *aOffline)
{
    TRACE("(%p)\n", aOffline);
    return nsIIOService_GetOffline(nsio, aOffline);
}

static nsresult NSAPI nsIOService_SetOffline(nsIIOService *iface, cpp_bool aOffline)
{
    TRACE("(%x)\n", aOffline);
    return nsIIOService_SetOffline(nsio, aOffline);
}

static nsresult NSAPI nsIOService_GetConnectivity(nsIIOService *iface, cpp_bool *aConnectivity)
{
    TRACE("(%p)\n", aConnectivity);
    return nsIIOService_GetConnectivity(nsio, aConnectivity);
}

static nsresult NSAPI nsIOService_SetAppOffline(nsIIOService *iface, UINT32 appId, INT32 state)
{
    TRACE("(%d %x)\n", appId, state);
    return nsIIOService_SetAppOffline(nsio, appId, state);
}

static nsresult NSAPI nsIOService_IsAppOffline(nsIIOService *iface, UINT32 appId, cpp_bool *_retval)
{
    TRACE("(%u %p)\n", appId, _retval);
    return nsIIOService_IsAppOffline(nsio, appId, _retval);
}

static nsresult NSAPI nsIOService_GetAppOfflineState(nsIIOService *iface, UINT32 appId, INT32 *_retval)
{
    TRACE("(%d %p)\n", appId, _retval);
    return nsIIOService_GetAppOfflineState(nsio, appId, _retval);
}

static nsresult NSAPI nsIOService_AllowPort(nsIIOService *iface, LONG aPort,
                                             const char *aScheme, cpp_bool *_retval)
{
    TRACE("(%d %s %p)\n", aPort, debugstr_a(aScheme), _retval);
    return nsIIOService_AllowPort(nsio, aPort, debugstr_a(aScheme), _retval);
}

static nsresult NSAPI nsIOService_ExtractScheme(nsIIOService *iface, const nsACString *urlString,
                                                 nsACString * _retval)
{
    TRACE("(%s %p)\n", debugstr_nsacstr(urlString), _retval);
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
    nsIOService_NewChannelFromURI2,
    nsIOService_NewChannelFromURIWithLoadInfo,
    nsIOService_NewChannelFromURI,
    nsIOService_NewChannel2,
    nsIOService_NewChannel,
    nsIOService_GetOffline,
    nsIOService_SetOffline,
    nsIOService_GetConnectivity,
    nsIOService_SetAppOffline,
    nsIOService_IsAppOffline,
    nsIOService_GetAppOfflineState,
    nsIOService_AllowPort,
    nsIOService_ExtractScheme
};

static nsIIOService nsIOService = { &nsIOServiceVtbl };

static nsresult NSAPI nsNetUtil_QueryInterface(nsINetUtil *iface, nsIIDRef riid,
        void **result)
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
        nsACString *aCharset, cpp_bool *aHadCharset, nsACString *aContentType)
{
    TRACE("(%s %p %p %p)\n", debugstr_nsacstr(aTypeHeader), aCharset, aHadCharset, aContentType);

    return nsINetUtil_ParseContentType(net_util, aTypeHeader, aCharset, aHadCharset, aContentType);
}

static const char *debugstr_protocol_flags(UINT32 flags)
{
    switch(flags) {
#define X(f) case f: return #f
    X(URI_STD);
    X(URI_NORELATIVE);
    X(URI_NOAUTH);
    X(ALLOWS_PROXY);
    X(ALLOWS_PROXY_HTTP);
    X(URI_INHERITS_SECURITY_CONTEXT);
    X(URI_FORBIDS_AUTOMATIC_DOCUMENT_REPLACEMENT);
    X(URI_LOADABLE_BY_ANYONE);
    X(URI_DANGEROUS_TO_LOAD);
    X(URI_IS_UI_RESOURCE);
    X(URI_IS_LOCAL_FILE);
    X(URI_LOADABLE_BY_SUBSUMERS);
    X(URI_DOES_NOT_RETURN_DATA);
    X(URI_IS_LOCAL_RESOURCE);
    X(URI_OPENING_EXECUTES_SCRIPT);
    X(URI_NON_PERSISTABLE);
    X(URI_FORBIDS_COOKIE_ACCESS);
    X(URI_CROSS_ORIGIN_NEEDS_WEBAPPS_PERM);
    X(URI_SYNC_LOAD_IS_OK);
    X(URI_SAFE_TO_LOAD_IN_SECURE_CONTEXT);
#undef X
    default:
        return wine_dbg_sprintf("%08x", flags);
    }
}

static nsresult NSAPI nsNetUtil_ProtocolHasFlags(nsINetUtil *iface, nsIURI *aURI, UINT32 aFlags, cpp_bool *_retval)
{
    TRACE("(%p %s %p)\n", aURI, debugstr_protocol_flags(aFlags), _retval);

    return nsINetUtil_ProtocolHasFlags(net_util, aURI, aFlags, _retval);
}

static nsresult NSAPI nsNetUtil_URIChainHasFlags(nsINetUtil *iface, nsIURI *aURI, UINT32 aFlags, cpp_bool *_retval)
{
    TRACE("(%p %s %p)\n", aURI, debugstr_protocol_flags(aFlags), _retval);

    if(aFlags == URI_DOES_NOT_RETURN_DATA) {
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

static nsresult NSAPI nsNetUtil_NewSimpleNestedURI(nsINetUtil *iface, nsIURI *aURI, nsIURI **_retval)
{
    TRACE("(%p %p)\n", aURI, _retval);

    return nsINetUtil_NewSimpleNestedURI(net_util, aURI, _retval);
}

static nsresult NSAPI nsNetUtil_EscapeString(nsINetUtil *iface, const nsACString *aString,
                                             UINT32 aEscapeType, nsACString *_retval)
{
    TRACE("(%s %x %p)\n", debugstr_nsacstr(aString), aEscapeType, _retval);

    return nsINetUtil_EscapeString(net_util, aString, aEscapeType, _retval);
}

static nsresult NSAPI nsNetUtil_EscapeURL(nsINetUtil *iface, const nsACString *aStr, UINT32 aFlags,
                                          nsACString *_retval)
{
    TRACE("(%s %08x %p)\n", debugstr_nsacstr(aStr), aFlags, _retval);

    return nsINetUtil_EscapeURL(net_util, aStr, aFlags, _retval);
}

static nsresult NSAPI nsNetUtil_UnescapeString(nsINetUtil *iface, const nsACString *aStr,
                                               UINT32 aFlags, nsACString *_retval)
{
    TRACE("(%s %08x %p)\n", debugstr_nsacstr(aStr), aFlags, _retval);

    return nsINetUtil_UnescapeString(net_util, aStr, aFlags, _retval);
}

static nsresult NSAPI nsNetUtil_ExtractCharsetFromContentType(nsINetUtil *iface, const nsACString *aTypeHeader,
        nsACString *aCharset, LONG *aCharsetStart, LONG *aCharsetEnd, cpp_bool *_retval)
{
    TRACE("(%s %p %p %p %p)\n", debugstr_nsacstr(aTypeHeader), aCharset, aCharsetStart,
          aCharsetEnd, _retval);

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
    nsNetUtil_NewSimpleNestedURI,
    nsNetUtil_EscapeString,
    nsNetUtil_EscapeURL,
    nsNetUtil_UnescapeString,
    nsNetUtil_ExtractCharsetFromContentType
};

static nsINetUtil nsNetUtil = { &nsNetUtilVtbl };

static nsresult NSAPI nsIOService_QueryInterface(nsIIOService *iface, nsIIDRef riid,
        void **result)
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
        void **result)
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

static nsresult NSAPI nsIOServiceFactory_LockFactory(nsIFactory *iface, cpp_bool lock)
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

static BOOL translate_url(HTMLDocumentObj *doc, nsWineURI *uri)
{
    OLECHAR *new_url = NULL;
    WCHAR *url;
    BOOL ret = FALSE;
    HRESULT hres;

    if(!doc->hostui || !ensure_uri(uri))
        return FALSE;

    hres = IUri_GetDisplayUri(uri->uri, &url);
    if(FAILED(hres))
        return FALSE;

    hres = IDocHostUIHandler_TranslateUrl(doc->hostui, 0, url, &new_url);
    if(hres == S_OK && new_url) {
        if(strcmpW(url, new_url)) {
            FIXME("TranslateUrl returned new URL %s -> %s\n", debugstr_w(url), debugstr_w(new_url));
            ret = TRUE;
        }
        CoTaskMemFree(new_url);
    }

    SysFreeString(url);
    return ret;
}

nsresult on_start_uri_open(NSContainer *nscontainer, nsIURI *uri, cpp_bool *_retval)
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
        wine_uri->is_doc_uri = TRUE;

        if(!wine_uri->container) {
            nsIWebBrowserChrome_AddRef(&nscontainer->nsIWebBrowserChrome_iface);
            wine_uri->container = nscontainer;
        }

        if(nscontainer->doc)
            *_retval = translate_url(nscontainer->doc, wine_uri);
    }

    nsIFileURL_Release(&wine_uri->nsIFileURL_iface);
    return NS_OK;
}

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
