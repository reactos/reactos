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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winternl.h"
#include "ole2.h"
#include "shlguid.h"
#include "wininet.h"
#include "shlwapi.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "binding.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define NS_IOSERVICE_CLASSNAME "nsIOService"
#define NS_IOSERVICE_CONTRACTID "@mozilla.org/network/io-service;1"

static const IID NS_IOSERVICE_CID =
    {0x9ac9e770, 0x18bc, 0x11d3, {0x93, 0x37, 0x00, 0x10, 0x4b, 0xa0, 0xfd, 0x40}};
static const IID IID_nsWineURI =
    {0x5088272e, 0x900b, 0x11da, {0xc6,0x87, 0x00,0x0f,0xea,0x57,0xf2,0x1a}};

static nsIIOService *nsio = NULL;

static const char *request_method_strings[] = {"GET", "PUT", "POST"};

struct  nsWineURI {
    nsIFileURL nsIFileURL_iface; /* For non-file URL objects, it's just nsIURL */
    nsIStandardURL nsIStandardURL_iface;

    LONG ref;

    nsChannelBSC *channel_bsc;
    IUri *uri;
    IUriBuilder *uri_builder;
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
            WARN("CreateUriSimple failed: %08lx\n", hres);
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

BOOL compare_uri_ignoring_frag(IUri *uri1, IUri *uri2)
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
        WARN("CoInternetCombineUrlEx failed: %08lx\n", hres);
    return hres;
}

static nsresult create_nsuri(IUri*,nsWineURI**);

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

    if(!str) {
        nsACString_SetData(ret_str, NULL);
        return NS_OK;
    }

    if(!*str) {
        nsACString_SetData(ret_str, "");
        return NS_OK;
    }

    lena = WideCharToMultiByte(CP_UTF8, 0, str, len, NULL, 0, NULL, NULL);
    stra = malloc(lena + 1);
    if(!stra)
        return NS_ERROR_OUT_OF_MEMORY;

    WideCharToMultiByte(CP_UTF8, 0, str, len, stra, lena, NULL, NULL);
    stra[lena] = 0;

    nsACString_SetData(ret_str, stra);
    free(stra);
    return NS_OK;
}

HRESULT nsuri_to_url(LPCWSTR nsuri, BOOL ret_empty, BSTR *ret)
{
    const WCHAR *ptr = nsuri;

    static const WCHAR wine_prefixW[] = {'w','i','n','e',':'};

    if(!wcsncmp(nsuri, wine_prefixW, ARRAY_SIZE(wine_prefixW)))
        ptr += ARRAY_SIZE(wine_prefixW);

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

    if(!doc->client)
        return TRUE;

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

static nsresult before_async_open(nsChannel *channel, GeckoBrowser *container, BOOL *cancel)
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
    IUnknown_AddRef(doc->outer_unk);

    if(doc->hostui) {
        OLECHAR *new_url;
        hres = IDocHostUIHandler_TranslateUrl(doc->hostui, 0, display_uri, &new_url);
        if(hres == S_OK && new_url) {
            if(wcscmp(display_uri, new_url)) {
                FIXME("TranslateUrl returned new URL %s -> %s\n", debugstr_w(display_uri), debugstr_w(new_url));
                CoTaskMemFree(new_url);
                *cancel = TRUE;
                goto done;
            }
            CoTaskMemFree(new_url);
        }
    }

    if(!exec_shldocvw_67(doc, display_uri)) {
        SysFreeString(display_uri);
        *cancel = FALSE;
        goto done;
    }

    hres = hlink_frame_navigate(doc, display_uri, channel, 0, cancel);
    SysFreeString(display_uri);
    if(FAILED(hres))
        *cancel = TRUE;

done:
    IUnknown_Release(doc->outer_unk);
    return NS_OK;
}

HRESULT load_nsuri(HTMLOuterWindow *window, nsWineURI *uri, nsIInputStream *post_stream,
        nsChannelBSC *channelbsc, DWORD flags)
{
    nsIWebNavigation *web_navigation;
    nsDocShellInfoLoadType load_type;
    nsIDocShellLoadInfo *load_info;
    nsIDocShell *doc_shell;
    HTMLDocumentNode *doc;
    nsresult nsres;

    nsres = get_nsinterface((nsISupports*)window->nswindow, &IID_nsIWebNavigation, (void**)&web_navigation);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIWebNavigation interface: %08lx\n", nsres);
        return E_FAIL;
    }

    nsres = nsIWebNavigation_QueryInterface(web_navigation, &IID_nsIDocShell, (void**)&doc_shell);
    nsIWebNavigation_Release(web_navigation);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDocShell: %08lx\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDocShell_CreateLoadInfo(doc_shell, &load_info);
    if(NS_FAILED(nsres)) {
        nsIDocShell_Release(doc_shell);
        return E_FAIL;
    }

    if(flags & LOAD_FLAGS_IS_REFRESH)
        load_type = (flags & LOAD_FLAGS_BYPASS_CACHE) ? loadReloadBypassCache : loadReloadNormal;
    else
        load_type = (flags & LOAD_FLAGS_BYPASS_CACHE) ? loadNormalBypassCache : loadNormal;
    nsres = nsIDocShellLoadInfo_SetLoadType(load_info, load_type);
    assert(nsres == NS_OK);

    if(post_stream) {
        nsres = nsIDocShellLoadInfo_SetPostDataStream(load_info, post_stream);
        assert(nsres == NS_OK);
    }

    if(window->uri_nofrag) {
        nsWineURI *referrer_uri;
        nsres = create_nsuri(window->uri_nofrag, &referrer_uri);
        if(NS_SUCCEEDED(nsres)) {
            nsres = nsIDocShellLoadInfo_SetReferrer(load_info, (nsIURI*)&referrer_uri->nsIFileURL_iface);
            assert(nsres == NS_OK);
            nsIFileURL_Release(&referrer_uri->nsIFileURL_iface);
        }
    }

    uri->channel_bsc = channelbsc;
    doc = window->base.inner_window->doc;
    doc->skip_mutation_notif = TRUE;
    nsres = nsIDocShell_LoadURI(doc_shell, (nsIURI*)&uri->nsIFileURL_iface, load_info, flags, FALSE);
    if(doc == window->base.inner_window->doc)
        doc->skip_mutation_notif = FALSE;
    uri->channel_bsc = NULL;
    nsIDocShell_Release(doc_shell);
    nsIDocShellLoadInfo_Release(load_info);
    if(NS_FAILED(nsres)) {
        WARN("LoadURI failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static inline BOOL is_http_channel(nsChannel *This)
{
    return This->uri->scheme == URL_SCHEME_HTTP || This->uri->scheme == URL_SCHEME_HTTPS;
}

static http_header_t *find_http_header(struct list *headers, const WCHAR *name, int len)
{
    http_header_t *iter;

    LIST_FOR_EACH_ENTRY(iter, headers, http_header_t, entry) {
        if(!wcsnicmp(iter->header, name, len) && !iter->header[len])
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
    header_name = strdupAtoW(header_namea);
    if(!header_name)
        return NS_ERROR_UNEXPECTED;

    header = find_http_header(headers, header_name, lstrlenW(header_name));
    free(header_name);
    if(!header)
        return NS_ERROR_NOT_AVAILABLE;

    data = strdupWtoA(header->data);
    if(!data)
        return NS_ERROR_UNEXPECTED;

    TRACE("%s -> %s\n", debugstr_a(header_namea), debugstr_a(data));
    nsACString_SetData(_retval, data);
    free(data);
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

        new_data = strndupW(value, value_len);
        if(!new_data)
            return E_OUTOFMEMORY;

        free(header->data);
        header->data = new_data;
    }else {
        header = malloc(sizeof(http_header_t));
        if(!header)
            return E_OUTOFMEMORY;

        header->header = strndupW(name, name_len);
        header->data = strndupW(value, value_len);
        if(!header->header || !header->data) {
            free(header->header);
            free(header->data);
            free(header);
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
    name = strdupAtoW(namea);
    if(!name)
        return NS_ERROR_UNEXPECTED;

    nsACString_GetData(value_str, &valuea);
    value = strdupAtoW(valuea);
    if(!value) {
        free(name);
        return NS_ERROR_UNEXPECTED;
    }

    hres = set_http_header(headers, name, lstrlenW(name), value, lstrlenW(value));

    free(name);
    free(value);
    return SUCCEEDED(hres) ? NS_OK : NS_ERROR_UNEXPECTED;
}

static nsresult visit_http_headers(struct list *headers, nsIHttpHeaderVisitor *visitor)
{
    nsACString header_str, value_str;
    char *header, *value;
    http_header_t *iter;
    nsresult nsres;

    LIST_FOR_EACH_ENTRY(iter, headers, http_header_t, entry) {
        header = strdupWtoA(iter->header);
        if(!header)
            return NS_ERROR_OUT_OF_MEMORY;

        value = strdupWtoA(iter->data);
        if(!value) {
            free(header);
            return NS_ERROR_OUT_OF_MEMORY;
        }

        nsACString_InitDepend(&header_str, header);
        nsACString_InitDepend(&value_str, value);
        nsres = nsIHttpHeaderVisitor_VisitHeader(visitor, &header_str, &value_str);
        nsACString_Finish(&header_str);
        nsACString_Finish(&value_str);
        free(header);
        free(value);
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
        free(iter->header);
        free(iter->data);
        free(iter);
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
    }else if(IsEqualGUID(&IID_nsIFormPOSTActionChannel, riid)) {
        TRACE("(%p)->(IID_nsIFormPOSTActionChannel %p)\n", This, result);
        *result = &This->nsIUploadChannel_iface;
    }else if(IsEqualGUID(&IID_nsIHttpChannelInternal, riid)) {
        TRACE("(%p)->(IID_nsIHttpChannelInternal %p)\n", This, result);
        *result = is_http_channel(This) ? &This->nsIHttpChannelInternal_iface : NULL;
    }else if(IsEqualGUID(&IID_nsICacheInfoChannel, riid)) {
        TRACE("(%p)->(IID_nsICacheInfoChannel %p)\n", This, result);
        *result = is_http_channel(This) ? &This->nsICacheInfoChannel_iface : NULL;
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

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static nsrefcnt NSAPI nsChannel_Release(nsIHttpChannel *iface)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    nsrefcnt ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        if(This->owner)
            nsISupports_Release(This->owner);
        if(This->post_data_stream)
            nsIInputStream_Release(This->post_data_stream);
        if(This->load_info)
            nsISupports_Release(This->load_info);
        if(This->load_group)
            nsILoadGroup_Release(This->load_group);
        if(This->notif_callback)
            nsIInterfaceRequestor_Release(This->notif_callback);
        if(This->original_uri)
            nsIURI_Release(This->original_uri);
        if(This->referrer)
            nsIURI_Release(This->referrer);

        nsIFileURL_Release(&This->uri->nsIFileURL_iface);

        free_http_headers(&This->response_headers);
        free_http_headers(&This->request_headers);

        free(This->content_type);
        free(This->charset);
        free(This);
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

    TRACE("(%p)->(%p) returning %#lx\n", This, aStatus, This->status);

    return *aStatus = This->status;
}

static nsresult NSAPI nsChannel_Cancel(nsIHttpChannel *iface, nsresult aStatus)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    TRACE("(%p)->(%08lx)\n", This, aStatus);

    if(NS_FAILED(aStatus))
        This->status = aStatus;

    if(This->binding && This->binding->bsc.binding)
        IBinding_Abort(This->binding->bsc.binding);
    else
        WARN("No binding to cancel\n");
    return NS_OK;
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
    *aURI = (nsIURI*)&This->uri->nsIFileURL_iface;

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

    if(This->load_flags & LOAD_DOCUMENT_URI) {
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
    free(This->content_type);
    This->content_type = strdup(content_type);

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
    charset = strdup(data);
    if(!charset)
        return NS_ERROR_OUT_OF_MEMORY;

    free(This->charset);
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

static nsresult NSAPI nsChannel_Open2(nsIHttpChannel *iface, nsIInputStream **_retval)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)->(%p)\n", This, _retval);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static HTMLOuterWindow *get_channel_window(nsChannel *This, UINT32 *load_type)
{
    nsIWebProgress *web_progress = NULL;
    mozIDOMWindowProxy *mozwindow;
    HTMLOuterWindow *window;
    nsresult nsres;

    if(This->load_group) {
        nsIRequestObserver *req_observer;

        nsres = nsILoadGroup_GetGroupObserver(This->load_group, &req_observer);
        if(NS_FAILED(nsres)) {
            ERR("GetGroupObserver failed: %08lx\n", nsres);
            return NULL;
        }

        if(req_observer) {
            nsres = nsIRequestObserver_QueryInterface(req_observer, &IID_nsIWebProgress, (void**)&web_progress);
            nsIRequestObserver_Release(req_observer);
            if(NS_FAILED(nsres)) {
                ERR("Could not get nsIWebProgress iface: %08lx\n", nsres);
                return NULL;
            }
        }
    }

    if(!web_progress && This->notif_callback) {
        nsres = nsIInterfaceRequestor_GetInterface(This->notif_callback, &IID_nsIWebProgress, (void**)&web_progress);
        if(NS_FAILED(nsres)) {
            ERR("GetInterface(IID_nsIWebProgress failed: %08lx\n", nsres);
            return NULL;
        }
    }

    if(!web_progress) {
        ERR("Could not find nsIWebProgress\n");
        return NULL;
    }

    nsIWebProgress_GetLoadType(web_progress, load_type);

    nsres = nsIWebProgress_GetDOMWindow(web_progress, &mozwindow);
    nsIWebProgress_Release(web_progress);
    if(NS_FAILED(nsres) || !mozwindow) {
        ERR("GetDOMWindow failed: %08lx\n", nsres);
        return NULL;
    }

    window = mozwindow_to_window(mozwindow);
    mozIDOMWindowProxy_Release(mozwindow);

    if(window)
        IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);
    else
        FIXME("NULL window for %p\n", mozwindow);
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
}

static nsresult async_open(nsChannel *This, HTMLOuterWindow *window, BOOL is_doc_channel, UINT32 load_type,
        nsIStreamListener *listener, nsISupports *context)
{
    nsChannelBSC *bscallback;
    IMoniker *mon = NULL;
    HRESULT hres;

    hres = CreateURLMonikerEx2(NULL, This->uri->uri, &mon, 0);
    if(FAILED(hres)) {
        WARN("CreateURLMoniker failed: %08lx\n", hres);
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
            async_start_doc_binding(window, window->pending_window, (load_type & LOAD_CMD_RELOAD) ? BINDING_REFRESH : BINDING_NAVIGATED);
        IBindStatusCallback_Release(&bscallback->bsc.IBindStatusCallback_iface);
        if(FAILED(hres))
            return NS_ERROR_UNEXPECTED;
    }else {
        start_binding_task_t *task;

        task = malloc(sizeof(start_binding_task_t));
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
    UINT32 load_type = LOAD_CMD_NORMAL;
    HTMLOuterWindow *window = NULL;
    BOOL is_document_channel;
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
            WARN("GetDisplayUri failed: %08lx\n", hres);
        }
    }

    window = get_channel_window(This, &load_type);
    if(!window) {
        ERR("window = NULL\n");
        return NS_ERROR_UNEXPECTED;
    }

    is_document_channel = !!(This->load_flags & LOAD_DOCUMENT_URI);

    if(is_document_channel) {
        if(This->uri->channel_bsc) {
            channelbsc_set_channel(This->uri->channel_bsc, This, aListener, aContext);
            cancel = TRUE;
        }

        if(is_main_content_window(window)) {
            if(!This->uri->channel_bsc) {
                /* top window navigation initiated by Gecko */
                nsres = before_async_open(This, window->browser, &cancel);
                if(NS_SUCCEEDED(nsres)  && cancel) {
                    TRACE("canceled\n");
                    nsres = NS_BINDING_ABORTED;
                }
            }else if(window->browser->doc && window->browser->doc->mime) {
                free(This->content_type);
                This->content_type = strdupWtoA(window->browser->doc->mime);
            }
        }
    }

    if(!cancel)
        nsres = async_open(This, window, is_document_channel, load_type, aListener, aContext);

    if(NS_SUCCEEDED(nsres) && This->load_group) {
        nsres = nsILoadGroup_AddRequest(This->load_group, (nsIRequest*)&This->nsIHttpChannel_iface,
                aContext);
        if(NS_FAILED(nsres))
            ERR("AddRequest failed: %08lx\n", nsres);
    }

    IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
    return nsres;
}

static nsresult NSAPI nsChannel_AsyncOpen2(nsIHttpChannel *iface, nsIStreamListener *aListener)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    FIXME("(%p)->(%p)\n", This, aListener);
    return nsIHttpChannel_AsyncOpen(&This->nsIHttpChannel_iface, aListener, NULL);
}

static nsresult NSAPI nsChannel_GetContentDisposition(nsIHttpChannel *iface, UINT32 *aContentDisposition)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    TRACE("(%p)->(%p) unimplemented\n", This, aContentDisposition);
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
    TRACE("(%p)->(%p) unimplemented\n", This, aContentDispositionHeader);
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
    for(i=0; i < ARRAY_SIZE(request_method_strings); i++) {
        if(!stricmp(method, request_method_strings[i])) {
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

    return nsIHttpChannel_SetReferrerWithPolicy(&This->nsIHttpChannel_iface, aReferrer, 0);
}

static nsresult NSAPI nsChannel_GetReferrerPolicy(nsIHttpChannel *iface, UINT32 *aReferrerPolicy)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    TRACE("(%p)->(%p) unimplemented\n", This, aReferrerPolicy);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetReferrerWithPolicy(nsIHttpChannel *iface, nsIURI *aReferrer, UINT32 aReferrerPolicy)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    DWORD channel_scheme, referrer_scheme;
    nsWineURI *referrer;
    BSTR referrer_uri;
    nsresult nsres;
    HRESULT hres;

    static const WCHAR refererW[] = {'R','e','f','e','r','e','r'};

    TRACE("(%p)->(%p %d)\n", This, aReferrer, aReferrerPolicy);

    if(aReferrerPolicy)
        FIXME("refferer policy %d not implemented\n", aReferrerPolicy);

    unlink_ref(&This->referrer);
    if(!aReferrer)
        return NS_OK;

    nsres = nsIURI_QueryInterface(aReferrer, &IID_nsWineURI, (void**)&referrer);
    if(NS_FAILED(nsres))
        return NS_OK;

    if(!ensure_uri(referrer)) {
        nsIFileURL_Release(&referrer->nsIFileURL_iface);
        return NS_ERROR_UNEXPECTED;
    }

    if(!ensure_uri(This->uri) || IUri_GetScheme(This->uri->uri, &channel_scheme) != S_OK)
        channel_scheme = INTERNET_SCHEME_UNKNOWN;

    if(IUri_GetScheme(referrer->uri, &referrer_scheme) != S_OK)
        referrer_scheme = INTERNET_SCHEME_UNKNOWN;

    if(referrer_scheme == INTERNET_SCHEME_HTTPS && channel_scheme != INTERNET_SCHEME_HTTPS) {
        TRACE("Ignoring https referrer on non-https channel\n");
        nsIFileURL_Release(&referrer->nsIFileURL_iface);
        return NS_OK;
    }

    hres = IUri_GetDisplayUri(referrer->uri, &referrer_uri);
    if(SUCCEEDED(hres)) {
        set_http_header(&This->request_headers, refererW, ARRAY_SIZE(refererW), referrer_uri, SysStringLen(referrer_uri));
        SysFreeString(referrer_uri);
    }

    This->referrer = (nsIURI*)&referrer->nsIFileURL_iface;
    return NS_OK;
}

static nsresult NSAPI nsHttpChannel_GetProtocolVersion(nsIHttpChannel *iface, nsACString *aProtocolVersion)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    FIXME("(%p)->(%p)\n", This, aProtocolVersion);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannel_GetTransferSize(nsIHttpChannel *iface, UINT64 *aTransferSize)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    FIXME("(%p)->(%p)\n", This, aTransferSize);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannel_GetDecodedBodySize(nsIHttpChannel *iface, UINT64 *aDecodedBodySize)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    FIXME("(%p)->(%p)\n", This, aDecodedBodySize);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannel_GetEncodedBodySize(nsIHttpChannel *iface, UINT64 *aEncodedBodySize)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    FIXME("(%p)->(%p)\n", This, aEncodedBodySize);
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

static nsresult NSAPI nsChannel_SetEmptyRequestHeader(nsIHttpChannel *iface, const nsACString *aHeader)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_nsacstr(aHeader));
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_VisitRequestHeaders(nsIHttpChannel *iface,
                                                    nsIHttpHeaderVisitor *aVisitor)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)->(%p)\n", This, aVisitor);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_VisitNonDefaultRequestHeaders(nsIHttpChannel *iface, nsIHttpHeaderVisitor *aVisitor)
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

static nsresult NSAPI nsChannel_GetIsMainDocumentChannel(nsIHttpChannel *iface, cpp_bool *aIsMainDocumentChannel)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    FIXME("(%p)->(%p)\n", This, aIsMainDocumentChannel);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_SetIsMainDocumentChannel(nsIHttpChannel *iface, cpp_bool aIsMainDocumentChannel)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    FIXME("(%p)->(%x)\n", This, aIsMainDocumentChannel);
    return NS_ERROR_NOT_IMPLEMENTED;
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

    TRACE("(%p)->(%p)\n", This, _retval);

    header = find_http_header(&This->response_headers, cache_controlW, ARRAY_SIZE(cache_controlW));
    *_retval = header && !wcsicmp(header->data, L"no-store");
    return NS_OK;
}

static nsresult NSAPI nsChannel_IsNoCacheResponse(nsIHttpChannel *iface, cpp_bool *_retval)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);
    http_header_t *header;

    static const WCHAR cache_controlW[] = {'C','a','c','h','e','-','C','o','n','t','r','o','l'};

    TRACE("(%p)->(%p)\n", This, _retval);

    header = find_http_header(&This->response_headers, cache_controlW, ARRAY_SIZE(cache_controlW));
    *_retval = header && !wcsicmp(header->data, L"no-cache");
    /* FIXME: Gecko also checks if max-age is in the past */
    return NS_OK;
}

static nsresult NSAPI nsChannel_IsPrivateResponse(nsIHttpChannel *iface, cpp_bool *_retval)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)->(%p)\n", This, _retval);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsChannel_RedirectTo(nsIHttpChannel *iface, nsIURI *aTargetURI)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)->(%p)\n", This, aTargetURI);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannel_GetSchedulingContextID(nsIHttpChannel *iface, nsIID *aSchedulingContextID)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)->(%p)\n", This, aSchedulingContextID);

    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannel_SetSchedulingContextID(nsIHttpChannel *iface, const nsIID aSchedulingContextID)
{
    nsChannel *This = impl_from_nsIHttpChannel(iface);

    FIXME("(%p)->(%s)\n", This, debugstr_guid(&aSchedulingContextID));

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
    nsChannel_Open2,
    nsChannel_AsyncOpen,
    nsChannel_AsyncOpen2,
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
    nsHttpChannel_GetProtocolVersion,
    nsHttpChannel_GetTransferSize,
    nsHttpChannel_GetDecodedBodySize,
    nsHttpChannel_GetEncodedBodySize,
    nsChannel_GetRequestHeader,
    nsChannel_SetRequestHeader,
    nsChannel_SetEmptyRequestHeader,
    nsChannel_VisitRequestHeaders,
    nsChannel_VisitNonDefaultRequestHeaders,
    nsChannel_GetAllowPipelining,
    nsChannel_SetAllowPipelining,
    nsChannel_GetAllowTLS,
    nsChannel_SetAllowTLS,
    nsChannel_GetRedirectionLimit,
    nsChannel_SetRedirectionLimit,
    nsChannel_GetResponseStatus,
    nsChannel_GetResponseStatusText,
    nsChannel_GetRequestSucceeded,
    nsChannel_GetIsMainDocumentChannel,
    nsChannel_SetIsMainDocumentChannel,
    nsChannel_GetResponseHeader,
    nsChannel_SetResponseHeader,
    nsChannel_VisitResponseHeaders,
    nsChannel_IsNoStoreResponse,
    nsChannel_IsNoCacheResponse,
    nsChannel_IsPrivateResponse,
    nsChannel_RedirectTo,
    nsHttpChannel_GetSchedulingContextID,
    nsHttpChannel_SetSchedulingContextID
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
        {'C','o','n','t','e','n','t','-','T','y','p','e'};

    TRACE("(%p)->(%p %s %s)\n", This, aStream, debugstr_nsacstr(aContentType), wine_dbgstr_longlong(aContentLength));

    This->post_data_contains_headers = TRUE;

    if(aContentType) {
        nsACString_GetData(aContentType, &content_type);
        if(*content_type) {
            WCHAR *ct;

            ct = strdupAtoW(content_type);
            if(!ct)
                return NS_ERROR_UNEXPECTED;

            set_http_header(&This->request_headers, content_typeW, ARRAY_SIZE(content_typeW), ct, lstrlenW(ct));
            free(ct);
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

    TRACE("(%p)->() unimplemented\n", This);

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

    TRACE("(%p)->(%p) unimplemented\n", This, aMessages);

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

static nsresult NSAPI nsHttpChannelInternal_GetThirdPartyFlags(nsIHttpChannelInternal *iface, UINT32 *aThirdPartyFlags)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%p)\n", This, aThirdPartyFlags);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_SetThirdPartyFlags(nsIHttpChannelInternal *iface, UINT32 aThirdPartyFlags)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%x)\n", This, aThirdPartyFlags);
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

static nsresult NSAPI nsHttpChannelInternal_GetInitialRwin(nsIHttpChannelInternal *iface,
        UINT32 *aInitialRwin)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%p)\n", This, aInitialRwin);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_SetInitialRwin(nsIHttpChannelInternal *iface,
        UINT32 aInitialRwin)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%x)\n", This, aInitialRwin);
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

static nsresult NSAPI nsHttpChannelInternal_GetLastModifiedTime(nsIHttpChannelInternal *iface, PRTime *aLastModifiedTime)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%p)\n", This, aLastModifiedTime);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_ForceIntercepted(nsIHttpChannelInternal *iface, UINT64 aInterceptionID)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%s)\n", This, wine_dbgstr_longlong(aInterceptionID));
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetResponseSynthesized(nsIHttpChannelInternal *iface, cpp_bool *ResponseSynthesized)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p, %p)\n", This, ResponseSynthesized);
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
    TRACE("(%p)->(%x)\n", This, aCorsIncludeCredentials);
    return NS_OK;
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
    TRACE("(%p)->(%d)\n", This, aCorsMode);
    return NS_OK;
}

static nsresult NSAPI nsHttpChannelInternal_GetRedirectMode(nsIHttpChannelInternal *iface, UINT32 *aRedirectMode)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%p)\n", This, aRedirectMode);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_SetRedirectMode(nsIHttpChannelInternal *iface, UINT32 aRedirectMode)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    TRACE("(%p)->(%d) unimplemented\n", This, aRedirectMode);
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

static nsresult NSAPI nsHttpChannelInternal_GetProxyURI(nsIHttpChannelInternal *iface, nsIURI **aProxyURI)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%p)\n", This, aProxyURI);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_SetCorsPreflightParameters(nsIHttpChannelInternal *iface,
        const void /*nsTArray<nsCString>*/ *unsafeHeaders)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%p)\n", This, unsafeHeaders);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_GetBlockAuthPrompt(nsIHttpChannelInternal *iface, cpp_bool *aBlockAuthPrompt)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%p)\n", This, aBlockAuthPrompt);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsHttpChannelInternal_SetBlockAuthPrompt(nsIHttpChannelInternal *iface, cpp_bool aBlockAuthPrompt)
{
    nsChannel *This = impl_from_nsIHttpChannelInternal(iface);
    FIXME("(%p)->(%x)\n", This, aBlockAuthPrompt);
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
    nsHttpChannelInternal_GetThirdPartyFlags,
    nsHttpChannelInternal_SetThirdPartyFlags,
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
    nsHttpChannelInternal_GetInitialRwin,
    nsHttpChannelInternal_SetInitialRwin,
    nsHttpChannelInternal_GetApiRedirectToURI,
    nsHttpChannelInternal_GetAllowAltSvc,
    nsHttpChannelInternal_SetAllowAltSvc,
    nsHttpChannelInternal_GetLastModifiedTime,
    nsHttpChannelInternal_ForceIntercepted,
    nsHttpChannelInternal_GetResponseSynthesized,
    nsHttpChannelInternal_GetCorsIncludeCredentials,
    nsHttpChannelInternal_SetCorsIncludeCredentials,
    nsHttpChannelInternal_GetCorsMode,
    nsHttpChannelInternal_SetCorsMode,
    nsHttpChannelInternal_GetRedirectMode,
    nsHttpChannelInternal_SetRedirectMode,
    nsHttpChannelInternal_GetTopWindowURI,
    nsHttpChannelInternal_GetNetworkInterfaceId,
    nsHttpChannelInternal_SetNetworkInterfaceId,
    nsHttpChannelInternal_GetProxyURI,
    nsHttpChannelInternal_SetCorsPreflightParameters,
    nsHttpChannelInternal_GetBlockAuthPrompt,
    nsHttpChannelInternal_SetBlockAuthPrompt
};

static inline nsChannel *impl_from_nsICacheInfoChannel(nsICacheInfoChannel *iface)
{
    return CONTAINING_RECORD(iface, nsChannel, nsICacheInfoChannel_iface);
}

static nsresult NSAPI nsCacheInfoChannel_QueryInterface(nsICacheInfoChannel *iface, nsIIDRef riid,
        void **result)
{
    nsChannel *This = impl_from_nsICacheInfoChannel(iface);
    return nsIHttpChannel_QueryInterface(&This->nsIHttpChannel_iface, riid, result);
}

static nsrefcnt NSAPI nsCacheInfoChannel_AddRef(nsICacheInfoChannel *iface)
{
    nsChannel *This = impl_from_nsICacheInfoChannel(iface);
    return nsIHttpChannel_AddRef(&This->nsIHttpChannel_iface);
}

static nsrefcnt NSAPI nsCacheInfoChannel_Release(nsICacheInfoChannel *iface)
{
    nsChannel *This = impl_from_nsICacheInfoChannel(iface);
    return nsIHttpChannel_Release(&This->nsIHttpChannel_iface);
}

static nsresult NSAPI nsCacheInfoChannel_GetCacheTokenExpirationTime(nsICacheInfoChannel *iface, UINT32 *p)
{
    nsChannel *This = impl_from_nsICacheInfoChannel(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static nsresult NSAPI nsCacheInfoChannel_GetCacheTokenCachedCharset(nsICacheInfoChannel *iface, nsACString *p)
{
    nsChannel *This = impl_from_nsICacheInfoChannel(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static nsresult NSAPI nsCacheInfoChannel_SetCacheTokenCachedCharset(nsICacheInfoChannel *iface, const nsACString *p)
{
    nsChannel *This = impl_from_nsICacheInfoChannel(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_nsacstr(p));
    return E_NOTIMPL;
}

static nsresult NSAPI nsCacheInfoChannel_IsFromCache(nsICacheInfoChannel *iface, cpp_bool *p)
{
    nsChannel *This = impl_from_nsICacheInfoChannel(iface);
    FIXME("(%p)->(%p)\n", This, p);
    *p = FALSE;
    return NS_OK;
}

static nsresult NSAPI nsCacheInfoChannel_GetCacheKey(nsICacheInfoChannel *iface, nsISupports **p)
{
    nsChannel *This = impl_from_nsICacheInfoChannel(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static nsresult NSAPI nsCacheInfoChannel_SetCacheKey(nsICacheInfoChannel *iface, nsISupports *key)
{
    nsChannel *This = impl_from_nsICacheInfoChannel(iface);
    FIXME("(%p)->(%p)\n", This, key);
    return E_NOTIMPL;
}

static nsresult NSAPI nsCacheInfoChannel_GetAllowStaleCacheContent(nsICacheInfoChannel *iface, cpp_bool *p)
{
    nsChannel *This = impl_from_nsICacheInfoChannel(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static nsresult NSAPI nsCacheInfoChannel_SetAllowStaleCacheContent(nsICacheInfoChannel *iface, cpp_bool allow)
{
    nsChannel *This = impl_from_nsICacheInfoChannel(iface);
    FIXME("(%p)->(%x)\n", This, allow);
    return E_NOTIMPL;
}

static const nsICacheInfoChannelVtbl nsCacheInfoChannelVtbl = {
    nsCacheInfoChannel_QueryInterface,
    nsCacheInfoChannel_AddRef,
    nsCacheInfoChannel_Release,
    nsCacheInfoChannel_GetCacheTokenExpirationTime,
    nsCacheInfoChannel_GetCacheTokenCachedCharset,
    nsCacheInfoChannel_SetCacheTokenCachedCharset,
    nsCacheInfoChannel_IsFromCache,
    nsCacheInfoChannel_GetCacheKey,
    nsCacheInfoChannel_SetCacheKey,
    nsCacheInfoChannel_GetAllowStaleCacheContent,
    nsCacheInfoChannel_SetAllowStaleCacheContent
};

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
            WARN("CreateIUriBuilder failed: %08lx\n", hres);
            return FALSE;
        }
    }

    unlink_ref(&This->uri);
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
        WARN("GetPropertyBSTR failed: %08lx\n", hres);
        return NS_ERROR_UNEXPECTED;
    }

    vala = strdupWtoU(hres == S_OK ? val : NULL);
    SysFreeString(val);
    if(hres == S_OK && !vala)
        return NS_ERROR_OUT_OF_MEMORY;

    TRACE("ret %s\n", debugstr_a(vala));
    nsACString_SetData(ret, vala);
    free(vala);
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

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static nsrefcnt NSAPI nsURI_Release(nsIFileURL *iface)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        if(This->uri)
            IUri_Release(This->uri);
        if(This->uri_builder)
            IUriBuilder_Release(This->uri_builder);
        free(This);
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
    spec = strdupUtoW(speca);
    if(!spec)
        return NS_ERROR_OUT_OF_MEMORY;

    hres = create_uri(spec, 0, &uri);
    free(spec);
    if(FAILED(hres)) {
        WARN("create_uri failed: %08lx\n", hres);
        return NS_ERROR_FAILURE;
    }

    unlink_ref(&This->uri);
    unlink_ref(&This->uri_builder);

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
        WARN("GetScheme failed: %08lx\n", hres);
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
    scheme = strdupUtoW(schemea);
    if(!scheme)
        return NS_ERROR_OUT_OF_MEMORY;

    hres = IUriBuilder_SetSchemeName(This->uri_builder, scheme);
    free(scheme);
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

        buf = strdupUtoW(user_pass);
        if(!buf)
            return NS_ERROR_OUT_OF_MEMORY;

        ptr = wcschr(buf, ':');
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

    free(buf);
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
    user = strdupUtoW(usera);
    if(!user)
        return NS_ERROR_OUT_OF_MEMORY;

    hres = IUriBuilder_SetUserName(This->uri_builder, user);
    free(user);
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
    pass = strdupUtoW(passa);
    if(!pass)
        return NS_ERROR_OUT_OF_MEMORY;

    hres = IUriBuilder_SetPassword(This->uri_builder, pass);
    free(pass);
    if(FAILED(hres))
        return NS_ERROR_UNEXPECTED;

    return NS_OK;
}

static nsresult NSAPI nsURI_GetHostPort(nsIFileURL *iface, nsACString *aHostPort)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    const WCHAR *ptr = NULL;
    char *vala;
    BSTR val;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, aHostPort);

    if(!ensure_uri(This))
        return NS_ERROR_UNEXPECTED;

    hres = IUri_GetAuthority(This->uri, &val);
    if(FAILED(hres)) {
        WARN("GetAuthority failed: %08lx\n", hres);
        return NS_ERROR_UNEXPECTED;
    }

    if(hres == S_OK) {
        ptr = wcschr(val, '@');
        if(!ptr)
            ptr = val;
    }
    vala = strdupWtoU(ptr);
    SysFreeString(val);
    if(hres == S_OK && !vala)
        return NS_ERROR_OUT_OF_MEMORY;

    TRACE("ret %s\n", debugstr_a(vala));
    nsACString_SetData(aHostPort, vala);
    free(vala);
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
    host = strdupUtoW(hosta);
    if(!host)
        return NS_ERROR_OUT_OF_MEMORY;

    hres = IUriBuilder_SetHost(This->uri_builder, host);
    free(host);
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
        WARN("GetPort failed: %08lx\n", hres);
        return NS_ERROR_UNEXPECTED;
    }

    *aPort = port ? port : -1;
    return NS_OK;
}

static nsresult NSAPI nsURI_SetPort(nsIFileURL *iface, LONG aPort)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);
    HRESULT hres;

    TRACE("(%p)->(%ld)\n", This, aPort);

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
    path = strdupUtoW(patha);
    if(!path)
        return NS_ERROR_OUT_OF_MEMORY;

    hres = IUriBuilder_SetPath(This->uri_builder, path);
    free(path);
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

    if(hres != S_OK)
        *_retval = FALSE;
    else {
        MultiByteToWideChar(CP_UTF8, 0, scheme, -1, buf, ARRAY_SIZE(buf));
        *_retval = !wcscmp(scheme_name, buf);
    }
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

    nsres = create_nsuri(This->uri, &wine_uri);
    if(NS_FAILED(nsres)) {
        WARN("create_nsuri failed: %08lx\n", nsres);
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
    path = strdupUtoW(patha);
    if(!path)
        return NS_ERROR_OUT_OF_MEMORY;

    hres = combine_url(This->uri, path, &new_uri);
    free(path);
    if(FAILED(hres))
        return NS_ERROR_FAILURE;

    hres = IUri_GetDisplayUri(new_uri, &ret);
    IUri_Release(new_uri);
    if(FAILED(hres))
        return NS_ERROR_FAILURE;

    reta = strdupWtoU(ret);
    SysFreeString(ret);
    if(!reta)
        return NS_ERROR_OUT_OF_MEMORY;

    TRACE("returning %s\n", debugstr_a(reta));
    nsACString_SetData(_retval, reta);
    free(reta);
    return NS_OK;
}

static nsresult NSAPI nsURI_GetAsciiSpec(nsIFileURL *iface, nsACString *aAsciiSpec)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);

    TRACE("(%p)->(%p)\n", This, aAsciiSpec);

    return nsIFileURL_GetSpec(&This->nsIFileURL_iface, aAsciiSpec);
}

static nsresult NSAPI nsURI_GetAsciiHostPort(nsIFileURL *iface, nsACString *aAsciiHostPort)
{
    nsWineURI *This = impl_from_nsIFileURL(iface);

    WARN("(%p)->(%p) FIXME: Use Uri_PUNYCODE_IDN_HOST flag\n", This, aAsciiHostPort);

    return nsIFileURL_GetHostPort(&This->nsIFileURL_iface, aAsciiHostPort);
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

    nsACString_SetData(aOriginCharset, NULL); /* assume utf-8, we store URI as utf-16 anyway */
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

    refa = strdupWtoU(ref);
    SysFreeString(ref);
    if(ref && !refa)
        return NS_ERROR_OUT_OF_MEMORY;

    nsACString_SetData(aRef, refa && *refa == '#' ? refa+1 : refa);
    free(refa);
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
    ref = strdupUtoW(refa);
    if(!ref)
        return NS_ERROR_OUT_OF_MEMORY;

    hres = IUriBuilder_SetFragment(This->uri_builder, ref);
    free(ref);
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
        *_retval = compare_uri_ignoring_frag(This->uri, other_obj->uri);
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

    nsres = create_nsuri(uri, &wine_uri);
    IUri_Release(uri);
    if(NS_FAILED(nsres)) {
        WARN("create_nsuri failed: %08lx\n", nsres);
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
    query = strdupUtoW(querya);
    if(!query)
        return NS_ERROR_OUT_OF_MEMORY;

    hres = IUriBuilder_SetQuery(This->uri_builder, query);
    free(query);
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
    if(hres != S_OK) {
        SysFreeString(*path);
        *ext = *file = *path = NULL;
        return NS_OK;
    }

    for(ptr = *path + SysStringLen(*path)-1; ptr > *path && *ptr != '/' && *ptr != '\\'; ptr--);
    if(*ptr == '/' || *ptr == '\\')
        ptr++;
    *file = ptr;

    if(ext) {
        ptr = wcsrchr(ptr, '.');
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

    hres = CoInternetParseIUri(This->uri, PARSE_PATH_FROM_URL, 0, path, ARRAY_SIZE(path), &size, 0);
    if(FAILED(hres)) {
        WARN("CoInternetParseIUri failed: %08lx\n", hres);
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
    nsURI_GetAsciiHostPort,
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
    FIXME("(%p)->(%d %ld %s %s %p)\n", This, aUrlType, aDefaultPort, debugstr_nsacstr(aSpec), debugstr_a(aOriginCharset), aBaseURI);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static nsresult NSAPI nsStandardURL_SetDefaultPort(nsIStandardURL *iface, LONG aNewDefaultPort)
{
    nsWineURI *This = impl_from_nsIStandardURL(iface);
    FIXME("(%p)->(%ld)\n", This, aNewDefaultPort);
    return NS_ERROR_NOT_IMPLEMENTED;
}

static const nsIStandardURLVtbl nsStandardURLVtbl = {
    nsStandardURL_QueryInterface,
    nsStandardURL_AddRef,
    nsStandardURL_Release,
    nsStandardURL_GetMutable,
    nsStandardURL_SetMutable,
    nsStandardURL_Init,
    nsStandardURL_SetDefaultPort
};

static nsresult create_nsuri(IUri *iuri, nsWineURI **_retval)
{
    nsWineURI *ret;
    HRESULT hres;

    ret = calloc(1, sizeof(nsWineURI));
    if(!ret)
        return NS_ERROR_OUT_OF_MEMORY;

    ret->nsIFileURL_iface.lpVtbl = &nsFileURLVtbl;
    ret->nsIStandardURL_iface.lpVtbl = &nsStandardURLVtbl;
    ret->ref = 1;
    ret->is_mutable = TRUE;

    IUri_AddRef(iuri);
    ret->uri = iuri;

    hres = IUri_GetScheme(iuri, &ret->scheme);
    if(hres != S_OK)
        ret->scheme = URL_SCHEME_UNKNOWN;

    TRACE("retval=%p\n", ret);
    *_retval = ret;
    return NS_OK;
}

HRESULT create_doc_uri(IUri *iuri, nsWineURI **ret)
{
    nsresult nsres;
    nsres = create_nsuri(iuri, ret);
    return NS_SUCCEEDED(nsres) ? S_OK : E_OUTOFMEMORY;
}

static nsresult create_nschannel(nsWineURI *uri, nsChannel **ret)
{
    nsChannel *channel;

    if(!ensure_uri(uri))
        return NS_ERROR_UNEXPECTED;

    channel = calloc(1, sizeof(nsChannel));
    if(!channel)
        return NS_ERROR_OUT_OF_MEMORY;

    channel->nsIHttpChannel_iface.lpVtbl = &nsChannelVtbl;
    channel->nsIUploadChannel_iface.lpVtbl = &nsUploadChannelVtbl;
    channel->nsIHttpChannelInternal_iface.lpVtbl = &nsHttpChannelInternalVtbl;
    channel->nsICacheInfoChannel_iface.lpVtbl = &nsCacheInfoChannelVtbl;
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
    nsChannel *channel;
    nsWineURI *uri;
    IUri *iuri;
    nsresult nsres;
    HRESULT hres;

    hres = create_uri(url, 0, &iuri);
    if(FAILED(hres))
        return hres;

    nsres = create_nsuri(iuri, &uri);
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

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static nsrefcnt NSAPI nsProtocolHandler_Release(nsIProtocolHandler *iface)
{
    nsProtocolHandler *This = impl_from_nsIProtocolHandler(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        if(This->nshandler)
            nsIProtocolHandler_Release(This->nshandler);
        free(This);
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

    TRACE("(%p)->(%ld %s %p)\n", This, port, debugstr_a(scheme), _retval);

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

static nsresult NSAPI nsIOServiceHook_QueryInterface(nsIIOServiceHook *iface, nsIIDRef riid,
        void **result)
{
    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(IID_nsISupports %p)\n", result);
        *result = iface;
    }else if(IsEqualGUID(&IID_nsIIOServiceHook, riid)) {
        TRACE("(IID_nsIIOServiceHook %p)\n", result);
        *result = iface;
    }else {
        ERR("(%s %p)\n", debugstr_guid(riid), result);
        *result = NULL;
        return NS_NOINTERFACE;
    }

    nsISupports_AddRef((nsISupports*)*result);
    return NS_OK;
}

static nsrefcnt NSAPI nsIOServiceHook_AddRef(nsIIOServiceHook *iface)
{
    return 2;
}

static nsrefcnt NSAPI nsIOServiceHook_Release(nsIIOServiceHook *iface)
{
    return 1;
}

static nsresult NSAPI nsIOServiceHook_NewChannel(nsIIOServiceHook *iface, nsIURI *aURI,
        nsILoadInfo *aLoadInfo, nsIChannel **_retval)
{
    nsWineURI *wine_uri;
    nsChannel *ret;
    nsresult nsres;

    TRACE("(%p %p %p)\n", aURI, aLoadInfo, _retval);

    nsres = nsIURI_QueryInterface(aURI, &IID_nsWineURI, (void**)&wine_uri);
    if(NS_FAILED(nsres)) {
        TRACE("Could not get nsWineURI: %08lx\n", nsres);
        return NS_SUCCESS_DEFAULT_ACTION;
    }

    nsres = create_nschannel(wine_uri, &ret);
    nsIFileURL_Release(&wine_uri->nsIFileURL_iface);
    if(NS_FAILED(nsres))
        return nsres;

    nsIURI_AddRef(aURI);
    ret->original_uri = aURI;

    if(aLoadInfo)
        nsIHttpChannel_SetLoadInfo(&ret->nsIHttpChannel_iface, aLoadInfo);

    *_retval = (nsIChannel*)&ret->nsIHttpChannel_iface;
    return NS_OK;
}

static nsresult NSAPI nsIOServiceHook_GetProtocolHandler(nsIIOServiceHook *iface, nsIProtocolHandler *aHandler,
        nsIProtocolHandler **_retval)
{
    nsIExternalProtocolHandler *nsexthandler;
    nsProtocolHandler *ret;
    nsresult nsres;

    TRACE("(%p %p)\n", aHandler, _retval);

    nsres = nsIProtocolHandler_QueryInterface(aHandler, &IID_nsIExternalProtocolHandler, (void**)&nsexthandler);
    if(NS_FAILED(nsres)) {
        nsIProtocolHandler_AddRef(aHandler);
        *_retval = aHandler;
        return NS_OK;
    }

    nsIExternalProtocolHandler_Release(nsexthandler);

    ret = malloc(sizeof(nsProtocolHandler));
    if(!ret)
        return NS_ERROR_OUT_OF_MEMORY;

    ret->nsIProtocolHandler_iface.lpVtbl = &nsProtocolHandlerVtbl;
    ret->ref = 1;
    nsIProtocolHandler_AddRef(aHandler);
    ret->nshandler = aHandler;


    *_retval = &ret->nsIProtocolHandler_iface;
    TRACE("return %p\n", *_retval);
    return NS_OK;
}

static BOOL is_gecko_special_uri(const char *spec)
{
    static const char *special_schemes[] = {"chrome:", "data:", "jar:", "moz-safe-about", "resource://gre/", "resource://gre-resources/", "javascript:", "wyciwyg:"};
    unsigned int i;

    for(i=0; i < ARRAY_SIZE(special_schemes); i++) {
        if(!_strnicmp(spec, special_schemes[i], strlen(special_schemes[i])))
            return TRUE;
    }

    if(!_strnicmp(spec, "file:", 5)) {
        const char *ptr = spec+5;
        while(*ptr == '/')
            ptr++;
        return is_gecko_path(ptr);
    }

    return FALSE;
}

static nsresult NSAPI nsIOServiceHook_NewURI(nsIIOServiceHook *iface, const nsACString *aSpec,
        const char *aOriginCharset, nsIURI *aBaseURI, nsIURI **_retval)
{
    nsWineURI *wine_uri, *base_wine_uri = NULL;
    WCHAR new_spec[INTERNET_MAX_URL_LENGTH];
    const char *spec = NULL;
    UINT cp = CP_UTF8;
    IUri *urlmon_uri;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%s %s %p %p)\n", debugstr_nsacstr(aSpec), debugstr_a(aOriginCharset),
          aBaseURI, _retval);

    nsACString_GetData(aSpec, &spec);
    if(is_gecko_special_uri(spec))
        return NS_SUCCESS_DEFAULT_ACTION;

    if(!strncmp(spec, "wine:", 5))
        spec += 5;

    if(aOriginCharset && *aOriginCharset && _strnicmp(aOriginCharset, "utf", 3)) {
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

    MultiByteToWideChar(cp, 0, spec, -1, new_spec, ARRAY_SIZE(new_spec));

    if(aBaseURI) {
        nsres = nsIURI_QueryInterface(aBaseURI, &IID_nsWineURI, (void**)&base_wine_uri);
        if(NS_SUCCEEDED(nsres)) {
            if(!ensure_uri(base_wine_uri)) {
                nsIFileURL_Release(&base_wine_uri->nsIFileURL_iface);
                return NS_ERROR_UNEXPECTED;
            }
        }else {
            WARN("Could not get base nsWineURI: %08lx\n", nsres);
        }
    }

    if(base_wine_uri) {
        hres = combine_url(base_wine_uri->uri, new_spec, &urlmon_uri);
        nsIFileURL_Release(&base_wine_uri->nsIFileURL_iface);
    }else {
        hres = create_uri(new_spec, 0, &urlmon_uri);
        if(FAILED(hres))
            WARN("create_uri failed: %08lx\n", hres);
    }

    if(FAILED(hres))
        return NS_SUCCESS_DEFAULT_ACTION;

    nsres = create_nsuri(urlmon_uri, &wine_uri);
    IUri_Release(urlmon_uri);
    if(NS_FAILED(nsres))
        return nsres;

    *_retval = (nsIURI*)&wine_uri->nsIFileURL_iface;
    return nsres;
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
    X(URI_FETCHABLE_BY_ANYONE);
#undef X
    default:
        return wine_dbg_sprintf("%08x", flags);
    }
}

static nsresult NSAPI nsIOServiceHook_ProtocolHasFlags(nsIIOServiceHook *iface, nsIURI *aURI, UINT32 aFlags, cpp_bool *_retval)
{
    TRACE("(%p %s %p)\n", aURI, debugstr_protocol_flags(aFlags), _retval);
    return NS_SUCCESS_DEFAULT_ACTION;
}

static nsresult NSAPI nsIOServiceHook_URIChainHasFlags(nsIIOServiceHook *iface, nsIURI *aURI, UINT32 aFlags, cpp_bool *_retval)
{
    TRACE("(%p %s %p)\n", aURI, debugstr_protocol_flags(aFlags), _retval);

    if(aFlags == URI_DOES_NOT_RETURN_DATA) {
        *_retval = FALSE;
        return S_OK;
    }

    return NS_SUCCESS_DEFAULT_ACTION;
}

static const nsIIOServiceHookVtbl nsIOServiceHookVtbl = {
    nsIOServiceHook_QueryInterface,
    nsIOServiceHook_AddRef,
    nsIOServiceHook_Release,
    nsIOServiceHook_NewChannel,
    nsIOServiceHook_GetProtocolHandler,
    nsIOServiceHook_NewURI,
    nsIOServiceHook_ProtocolHasFlags,
    nsIOServiceHook_URIChainHasFlags
};

static nsIIOServiceHook nsIOServiceHook = { &nsIOServiceHookVtbl };

void init_nsio(nsIComponentManager *component_manager)
{
    nsIFactory *old_factory = NULL;
    nsresult nsres;

    nsres = nsIComponentManager_GetClassObject(component_manager, &NS_IOSERVICE_CID,
                                               &IID_nsIFactory, (void**)&old_factory);
    if(NS_FAILED(nsres)) {
        ERR("Could not get factory: %08lx\n", nsres);
        return;
    }

    nsres = nsIFactory_CreateInstance(old_factory, NULL, &IID_nsIIOService, (void**)&nsio);
    nsIFactory_Release(old_factory);
    if(NS_FAILED(nsres)) {
        ERR("Couldn not create nsIOService instance %08lx\n", nsres);
        return;
    }

    nsres = nsIIOService_SetHook(nsio, &nsIOServiceHook);
    assert(nsres == NS_OK);
}

void release_nsio(void)
{
    if(nsio) {
        nsIIOService_Release(nsio);
        nsio = NULL;
    }
}

nsresult create_onload_blocker_request(nsIRequest **ret)
{
    nsIChannel *channel;
    nsACString spec;
    nsresult nsres;

    nsACString_InitDepend(&spec, "about:wine-script-onload-blocker");
    nsres = nsIIOService_NewChannel(nsio, &spec, NULL, NULL, &channel);
    nsACString_Finish(&spec);
    if(NS_FAILED(nsres)) {
        ERR("Failed to create channel: %08lx\n", nsres);
        return nsres;
    }

    *ret = (nsIRequest *)channel;
    return NS_OK;
}
