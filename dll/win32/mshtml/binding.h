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

#pragma once

typedef struct nsWineURI nsWineURI;

/* Keep sync with request_method_strings in nsio.c */
typedef enum {
    METHOD_GET,
    METHOD_PUT,
    METHOD_POST
} REQUEST_METHOD;

typedef struct {
    nsIHttpChannel         nsIHttpChannel_iface;
    nsIUploadChannel       nsIUploadChannel_iface;
    nsIHttpChannelInternal nsIHttpChannelInternal_iface;

    LONG ref;

    nsWineURI *uri;
    nsIInputStream *post_data_stream;
    BOOL post_data_contains_headers;
    nsILoadGroup *load_group;
    nsIInterfaceRequestor *notif_callback;
    nsISupports *owner;
    nsLoadFlags load_flags;
    nsIURI *original_uri;
    nsIURI *referrer;
    char *content_type;
    char *charset;
    UINT32 response_status;
    REQUEST_METHOD request_method;
    struct list response_headers;
    struct list request_headers;
} nsChannel;

typedef struct {
    WCHAR *headers;
    HGLOBAL post_data;
    ULONG post_data_len;
} request_data_t;

typedef struct BSCallbackVtbl BSCallbackVtbl;

struct BSCallback {
    IBindStatusCallback IBindStatusCallback_iface;
    IServiceProvider    IServiceProvider_iface;
    IHttpNegotiate2     IHttpNegotiate2_iface;
    IInternetBindInfo   IInternetBindInfo_iface;

    const BSCallbackVtbl          *vtbl;

    LONG ref;

    request_data_t request_data;
    ULONG readed;
    DWORD bindf;
    BOOL bindinfo_ready;
    int bom;

    IMoniker *mon;
    IBinding *binding;

    HTMLInnerWindow *window;

    struct list entry;
};

typedef struct nsProtocolStream nsProtocolStream;

struct nsChannelBSC {
    BSCallback bsc;

    nsChannel *nschannel;
    nsIStreamListener *nslistener;
    nsISupports *nscontext;
    BOOL is_js;
    BOOL is_doc_channel;
    BOOL response_processed;

    nsProtocolStream *nsstream;
};

typedef struct {
    struct list entry;
    WCHAR *header;
    WCHAR *data;
} http_header_t;

#define BINDING_NAVIGATED    0x0001
#define BINDING_REPLACE      0x0002
#define BINDING_FROMHIST     0x0004
#define BINDING_REFRESH      0x0008
#define BINDING_SUBMIT       0x0010
#define BINDING_NOFRAG       0x0020

HRESULT set_http_header(struct list*,const WCHAR*,int,const WCHAR*,int) DECLSPEC_HIDDEN;
HRESULT create_redirect_nschannel(const WCHAR*,nsChannel*,nsChannel**) DECLSPEC_HIDDEN;

nsresult on_start_uri_open(NSContainer*,nsIURI*,cpp_bool*) DECLSPEC_HIDDEN;
HRESULT hlink_frame_navigate(HTMLDocument*,LPCWSTR,nsChannel*,DWORD,BOOL*) DECLSPEC_HIDDEN;
HRESULT create_doc_uri(HTMLOuterWindow*,IUri*,nsWineURI**) DECLSPEC_HIDDEN;
HRESULT load_nsuri(HTMLOuterWindow*,nsWineURI*,nsChannelBSC*,DWORD) DECLSPEC_HIDDEN;
HRESULT set_moniker(HTMLOuterWindow*,IMoniker*,IUri*,IBindCtx*,nsChannelBSC*,BOOL) DECLSPEC_HIDDEN;
void prepare_for_binding(HTMLDocument*,IMoniker*,DWORD) DECLSPEC_HIDDEN;
HRESULT super_navigate(HTMLOuterWindow*,IUri*,DWORD,const WCHAR*,BYTE*,DWORD) DECLSPEC_HIDDEN;
HRESULT load_uri(HTMLOuterWindow*,IUri*,DWORD) DECLSPEC_HIDDEN;
HRESULT navigate_new_window(HTMLOuterWindow*,IUri*,const WCHAR*,IHTMLWindow2**) DECLSPEC_HIDDEN;
HRESULT navigate_url(HTMLOuterWindow*,const WCHAR*,IUri*,DWORD) DECLSPEC_HIDDEN;
HRESULT submit_form(HTMLOuterWindow*,IUri*,nsIInputStream*) DECLSPEC_HIDDEN;

HRESULT create_channelbsc(IMoniker*,const WCHAR*,BYTE*,DWORD,BOOL,nsChannelBSC**) DECLSPEC_HIDDEN;
HRESULT channelbsc_load_stream(HTMLInnerWindow*,IMoniker*,IStream*) DECLSPEC_HIDDEN;
void channelbsc_set_channel(nsChannelBSC*,nsChannel*,nsIStreamListener*,nsISupports*) DECLSPEC_HIDDEN;
IUri *nsuri_get_uri(nsWineURI*) DECLSPEC_HIDDEN;

HRESULT create_relative_uri(HTMLOuterWindow*,const WCHAR*,IUri**) DECLSPEC_HIDDEN;
HRESULT create_uri(const WCHAR*,DWORD,IUri**) DECLSPEC_HIDDEN;
IUri *get_uri_nofrag(IUri*) DECLSPEC_HIDDEN;

void set_current_mon(HTMLOuterWindow*,IMoniker*,DWORD) DECLSPEC_HIDDEN;
void set_current_uri(HTMLOuterWindow*,IUri*) DECLSPEC_HIDDEN;

HRESULT bind_mon_to_wstr(HTMLInnerWindow*,IMoniker*,WCHAR**) DECLSPEC_HIDDEN;
