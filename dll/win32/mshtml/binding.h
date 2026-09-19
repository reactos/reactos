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

typedef struct nsWineURI nsWineURI;

/* Keep sync with request_method_strings in nsio.c */
typedef enum {
    METHOD_GET,
    METHOD_PUT,
    METHOD_POST
} REQUEST_METHOD;

typedef enum {
    BOM_NONE,
    BOM_UTF8,
    BOM_UTF16
} binding_bom_t;

typedef struct {
    nsIHttpChannel         nsIHttpChannel_iface;
    nsIUploadChannel       nsIUploadChannel_iface;
    nsIHttpChannelInternal nsIHttpChannelInternal_iface;
    nsICacheInfoChannel    nsICacheInfoChannel_iface;

    LONG ref;

    nsWineURI *uri;
    nsIInputStream *post_data_stream;
    BOOL post_data_contains_headers;
    nsILoadGroup *load_group;
    nsIInterfaceRequestor *notif_callback;
    nsISupports *owner;
    nsILoadInfo *load_info;
    nsLoadFlags load_flags;
    nsIURI *original_uri;
    nsIURI *referrer;
    char *content_type;
    char *charset;
    nsresult status;
    UINT32 response_status;
    char *response_status_text;
    REQUEST_METHOD request_method;
    struct list response_headers;
    struct list request_headers;

    nsChannelBSC *binding;
} nsChannel;

typedef struct {
    nsIInputStream *post_stream;
    WCHAR *headers;
    HGLOBAL post_data;
    ULONG post_data_len;
} request_data_t;

typedef struct BSCallbackVtbl BSCallbackVtbl;

struct BSCallback {
    IBindStatusCallback   IBindStatusCallback_iface;
    IServiceProvider      IServiceProvider_iface;
    IHttpNegotiate2       IHttpNegotiate2_iface;
    IInternetBindInfo     IInternetBindInfo_iface;
    IBindCallbackRedirect IBindCallbackRedirect_iface;

    const BSCallbackVtbl          *vtbl;

    LONG ref;

    request_data_t request_data;
    ULONG read;
    DWORD bindf;
    DWORD bindinfo_options;
    BOOL bindinfo_ready;
    binding_bom_t bom;

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
    ULONG progress;
    ULONG total;
    BOOL is_js;
    BOOL is_doc_channel;
    BOOL response_processed;

    nsProtocolStream *nsstream;
};

struct BSCallbackVtbl {
    void (*destroy)(BSCallback*);
    HRESULT (*init_bindinfo)(BSCallback*);
    HRESULT (*start_binding)(BSCallback*);
    HRESULT (*stop_binding)(BSCallback*,HRESULT);
    HRESULT (*read_data)(BSCallback*,IStream*);
    HRESULT (*on_progress)(BSCallback*,ULONG,ULONG,ULONG,LPCWSTR);
    HRESULT (*on_response)(BSCallback*,DWORD,LPCWSTR);
    HRESULT (*beginning_transaction)(BSCallback*,WCHAR**);
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

HRESULT set_http_header(struct list*,const WCHAR*,int,const WCHAR*,int);
HRESULT create_redirect_nschannel(const WCHAR*,nsChannel*,nsChannel**);

HRESULT hlink_frame_navigate(HTMLDocumentObj*,LPCWSTR,nsChannel*,DWORD,BOOL*);
HRESULT create_doc_uri(IUri*,nsWineURI**);
HRESULT load_nsuri(HTMLOuterWindow*,nsWineURI*,nsIInputStream*,nsChannelBSC*,DWORD);
HRESULT set_moniker(HTMLOuterWindow*,IMoniker*,IUri*,IBindCtx*,nsChannelBSC*,BOOL);
void prepare_for_binding(HTMLDocumentObj*,IMoniker*,DWORD);
HRESULT super_navigate(HTMLOuterWindow*,IUri*,DWORD,const WCHAR*,BYTE*,DWORD);
HRESULT load_uri(HTMLOuterWindow*,IUri*,DWORD);
HRESULT navigate_new_window(HTMLOuterWindow*,IUri*,const WCHAR*,request_data_t*,IHTMLWindow2**);
HRESULT navigate_url(HTMLOuterWindow*,const WCHAR*,IUri*,DWORD);
HRESULT submit_form(HTMLOuterWindow*,const WCHAR*,IUri*,nsIInputStream*);
void process_document_response_headers(HTMLDocumentNode*,IBinding*);

void init_bscallback(BSCallback*,const BSCallbackVtbl*,IMoniker*,DWORD);
HRESULT create_channelbsc(IMoniker*,const WCHAR*,BYTE*,DWORD,BOOL,nsChannelBSC**);
HRESULT channelbsc_load_stream(HTMLInnerWindow*,IMoniker*,IStream*);
void channelbsc_set_channel(nsChannelBSC*,nsChannel*,nsIStreamListener*,nsISupports*);
IUri *nsuri_get_uri(nsWineURI*);
nsresult create_onload_blocker_request(nsIRequest**);

HRESULT read_stream(BSCallback*,IStream*,void*,DWORD,DWORD*);

HRESULT create_relative_uri(HTMLOuterWindow*,const WCHAR*,IUri**);
HRESULT create_uri(const WCHAR*,DWORD,IUri**);
IUri *get_uri_nofrag(IUri*);
BOOL compare_uri_ignoring_frag(IUri *uri1, IUri *uri2);

void set_current_mon(HTMLOuterWindow*,IMoniker*,DWORD);
void set_current_uri(HTMLOuterWindow*,IUri*);
