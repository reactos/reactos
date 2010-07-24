/*
 * Copyright 2005-2009 Jacek Caban for CodeWeavers
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

#include "wingdi.h"
#include "docobj.h"
#include "comcat.h"
#include "mshtml.h"
#include "mshtmhst.h"
#include "hlink.h"
#include "perhist.h"
#include "dispex.h"

#include "wine/list.h"
#include "wine/unicode.h"

#ifdef INIT_GUID
#include "initguid.h"
#endif

#include "nsiface.h"

#define NS_OK                     ((nsresult)0x00000000L)
#define NS_ERROR_FAILURE          ((nsresult)0x80004005L)
#define NS_NOINTERFACE            ((nsresult)0x80004002L)
#define NS_ERROR_NOT_IMPLEMENTED  ((nsresult)0x80004001L)
#define NS_ERROR_NOT_AVAILABLE    ((nsresult)0x80040111L)
#define NS_ERROR_INVALID_ARG      ((nsresult)0x80070057L) 
#define NS_ERROR_UNEXPECTED       ((nsresult)0x8000ffffL)
#define NS_ERROR_UNKNOWN_PROTOCOL ((nsresult)0x804b0012L)

#define NS_FAILED(res) ((res) & 0x80000000)
#define NS_SUCCEEDED(res) (!NS_FAILED(res))

#define NSAPI WINAPI

#define MSHTML_E_NODOC    0x800a025c

typedef struct HTMLDOMNode HTMLDOMNode;
typedef struct ConnectionPoint ConnectionPoint;
typedef struct BSCallback BSCallback;
typedef struct event_target_t event_target_t;

/* NOTE: make sure to keep in sync with dispex.c */
typedef enum {
    NULL_tid,
    DispCEventObj_tid,
    DispDOMChildrenCollection_tid,
    DispHTMLAnchorElement_tid,
    DispHTMLBody_tid,
    DispHTMLCommentElement_tid,
    DispHTMLCurrentStyle_tid,
    DispHTMLDocument_tid,
    DispHTMLDOMTextNode_tid,
    DispHTMLElementCollection_tid,
    DispHTMLFormElement_tid,
    DispHTMLGenericElement_tid,
    DispHTMLFrameElement_tid,
    DispHTMLIFrame_tid,
    DispHTMLImg_tid,
    DispHTMLInputElement_tid,
    DispHTMLLocation_tid,
    DispHTMLNavigator_tid,
    DispHTMLOptionElement_tid,
    DispHTMLScreen_tid,
    DispHTMLScriptElement_tid,
    DispHTMLSelectElement_tid,
    DispHTMLStyle_tid,
    DispHTMLTable_tid,
    DispHTMLTableRow_tid,
    DispHTMLTextAreaElement_tid,
    DispHTMLUnknownElement_tid,
    DispHTMLWindow2_tid,
    HTMLDocumentEvents_tid,
    IHTMLAnchorElement_tid,
    IHTMLBodyElement_tid,
    IHTMLBodyElement2_tid,
    IHTMLCommentElement_tid,
    IHTMLCurrentStyle_tid,
    IHTMLCurrentStyle2_tid,
    IHTMLCurrentStyle3_tid,
    IHTMLCurrentStyle4_tid,
    IHTMLDocument2_tid,
    IHTMLDocument3_tid,
    IHTMLDocument4_tid,
    IHTMLDocument5_tid,
    IHTMLDOMChildrenCollection_tid,
    IHTMLDOMNode_tid,
    IHTMLDOMNode2_tid,
    IHTMLDOMTextNode_tid,
    IHTMLElement_tid,
    IHTMLElement2_tid,
    IHTMLElement3_tid,
    IHTMLElement4_tid,
    IHTMLElementCollection_tid,
    IHTMLEventObj_tid,
    IHTMLFiltersCollection_tid,
    IHTMLFormElement_tid,
    IHTMLFrameBase_tid,
    IHTMLFrameBase2_tid,
    IHTMLFrameElement3_tid,
    IHTMLGenericElement_tid,
    IHTMLIFrameElement_tid,
    IHTMLImageElementFactory_tid,
    IHTMLImgElement_tid,
    IHTMLInputElement_tid,
    IHTMLLocation_tid,
    IHTMLOptionElement_tid,
    IHTMLScreen_tid,
    IHTMLScriptElement_tid,
    IHTMLSelectElement_tid,
    IHTMLStyle_tid,
    IHTMLStyle2_tid,
    IHTMLStyle3_tid,
    IHTMLStyle4_tid,
    IHTMLTable_tid,
    IHTMLTableRow_tid,
    IHTMLTextAreaElement_tid,
    IHTMLTextContainer_tid,
    IHTMLUniqueName_tid,
    IHTMLWindow2_tid,
    IHTMLWindow3_tid,
    IHTMLWindow4_tid,
    IOmNavigator_tid,
    LAST_tid
} tid_t;

typedef struct dispex_data_t dispex_data_t;
typedef struct dispex_dynamic_data_t dispex_dynamic_data_t;

#define MSHTML_DISPID_CUSTOM_MIN 0x60000000
#define MSHTML_DISPID_CUSTOM_MAX 0x6fffffff
#define MSHTML_CUSTOM_DISPID_CNT (MSHTML_DISPID_CUSTOM_MAX-MSHTML_DISPID_CUSTOM_MIN)

typedef struct {
    HRESULT (*value)(IUnknown*,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,IServiceProvider*);
    HRESULT (*get_dispid)(IUnknown*,BSTR,DWORD,DISPID*);
    HRESULT (*invoke)(IUnknown*,DISPID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,IServiceProvider*);
} dispex_static_data_vtbl_t;

typedef struct {
    const dispex_static_data_vtbl_t *vtbl;
    const tid_t disp_tid;
    dispex_data_t *data;
    const tid_t* const iface_tids;
} dispex_static_data_t;

typedef struct {
    const IDispatchExVtbl  *lpIDispatchExVtbl;

    IUnknown *outer;

    dispex_static_data_t *data;
    dispex_dynamic_data_t *dynamic_data;
} DispatchEx;

void init_dispex(DispatchEx*,IUnknown*,dispex_static_data_t*);
void release_dispex(DispatchEx*);
BOOL dispex_query_interface(DispatchEx*,REFIID,void**);
HRESULT dispex_get_dprop_ref(DispatchEx*,const WCHAR*,BOOL,VARIANT**);
HRESULT get_dispids(tid_t,DWORD*,DISPID**);
HRESULT remove_prop(DispatchEx*,BSTR,VARIANT_BOOL*);

typedef struct HTMLWindow HTMLWindow;
typedef struct HTMLDocumentNode HTMLDocumentNode;
typedef struct HTMLDocumentObj HTMLDocumentObj;
typedef struct HTMLFrameBase HTMLFrameBase;
typedef struct NSContainer NSContainer;

typedef enum {
    SCRIPTMODE_GECKO,
    SCRIPTMODE_ACTIVESCRIPT
} SCRIPTMODE;

typedef struct ScriptHost ScriptHost;

typedef enum {
    GLOBAL_SCRIPTVAR,
    GLOBAL_ELEMENTVAR
} global_prop_type_t;

typedef struct {
    global_prop_type_t type;
    WCHAR *name;
    ScriptHost *script_host;
    DISPID id;
} global_prop_t;

typedef struct {
    const IHTMLOptionElementFactoryVtbl *lpHTMLOptionElementFactoryVtbl;

    LONG ref;

    HTMLWindow *window;
} HTMLOptionElementFactory;

typedef struct {
    DispatchEx dispex;
    const IHTMLImageElementFactoryVtbl *lpHTMLImageElementFactoryVtbl;

    LONG ref;

    HTMLWindow *window;
} HTMLImageElementFactory;

struct HTMLLocation {
    DispatchEx dispex;
    const IHTMLLocationVtbl *lpHTMLLocationVtbl;

    LONG ref;

    HTMLWindow *window;
};

typedef struct {
    HTMLWindow *window;
    LONG ref;
}  windowref_t;

typedef struct nsChannelBSC nsChannelBSC;

struct HTMLWindow {
    DispatchEx dispex;
    const IHTMLWindow2Vtbl *lpHTMLWindow2Vtbl;
    const IHTMLWindow3Vtbl *lpHTMLWindow3Vtbl;
    const IHTMLWindow4Vtbl *lpHTMLWindow4Vtbl;
    const IHTMLPrivateWindowVtbl *lpIHTMLPrivateWindowVtbl;
    const IDispatchExVtbl  *lpIDispatchExVtbl;

    LONG ref;

    windowref_t *window_ref;
    LONG task_magic;

    HTMLDocumentNode *doc;
    HTMLDocumentObj *doc_obj;
    nsIDOMWindow *nswindow;
    HTMLWindow *parent;
    HTMLFrameBase *frame_element;
    READYSTATE readystate;

    nsChannelBSC *bscallback;
    IMoniker *mon;
    LPOLESTR url;

    IHTMLEventObj *event;

    SCRIPTMODE scriptmode;
    struct list script_hosts;

    HTMLOptionElementFactory *option_factory;
    HTMLImageElementFactory *image_factory;
    HTMLLocation *location;
    IHTMLScreen *screen;

    global_prop_t *global_props;
    DWORD global_prop_cnt;
    DWORD global_prop_size;

    struct list children;
    struct list sibling_entry;
    struct list entry;
};

typedef enum {
    UNKNOWN_USERMODE,
    BROWSEMODE,
    EDITMODE        
} USERMODE;

typedef struct _cp_static_data_t {
    tid_t tid;
    void (*on_advise)(IUnknown*,struct _cp_static_data_t*);
    DWORD id_cnt;
    DISPID *ids;
} cp_static_data_t;

typedef struct ConnectionPointContainer {
    const IConnectionPointContainerVtbl  *lpConnectionPointContainerVtbl;

    ConnectionPoint *cp_list;
    IUnknown *outer;
    struct ConnectionPointContainer *forward_container;
} ConnectionPointContainer;

struct ConnectionPoint {
    const IConnectionPointVtbl *lpConnectionPointVtbl;

    ConnectionPointContainer *container;

    union {
        IUnknown *unk;
        IDispatch *disp;
        IPropertyNotifySink *propnotif;
    } *sinks;
    DWORD sinks_size;

    const IID *iid;
    cp_static_data_t *data;

    ConnectionPoint *next;
};

struct HTMLDocument {
    const IHTMLDocument2Vtbl              *lpHTMLDocument2Vtbl;
    const IHTMLDocument3Vtbl              *lpHTMLDocument3Vtbl;
    const IHTMLDocument4Vtbl              *lpHTMLDocument4Vtbl;
    const IHTMLDocument5Vtbl              *lpHTMLDocument5Vtbl;
    const IHTMLDocument6Vtbl              *lpHTMLDocument6Vtbl;
    const IPersistMonikerVtbl             *lpPersistMonikerVtbl;
    const IPersistFileVtbl                *lpPersistFileVtbl;
    const IPersistHistoryVtbl             *lpPersistHistoryVtbl;
    const IMonikerPropVtbl                *lpMonikerPropVtbl;
    const IOleObjectVtbl                  *lpOleObjectVtbl;
    const IOleDocumentVtbl                *lpOleDocumentVtbl;
    const IOleDocumentViewVtbl            *lpOleDocumentViewVtbl;
    const IOleInPlaceActiveObjectVtbl     *lpOleInPlaceActiveObjectVtbl;
    const IViewObjectExVtbl               *lpViewObjectExVtbl;
    const IOleInPlaceObjectWindowlessVtbl *lpOleInPlaceObjectWindowlessVtbl;
    const IServiceProviderVtbl            *lpServiceProviderVtbl;
    const IOleCommandTargetVtbl           *lpOleCommandTargetVtbl;
    const IOleControlVtbl                 *lpOleControlVtbl;
    const IHlinkTargetVtbl                *lpHlinkTargetVtbl;
    const IPersistStreamInitVtbl          *lpPersistStreamInitVtbl;
    const IDispatchExVtbl                 *lpIDispatchExVtbl;
    const ISupportErrorInfoVtbl           *lpSupportErrorInfoVtbl;
    const IObjectWithSiteVtbl             *lpObjectWithSiteVtbl;

    IUnknown *unk_impl;
    IDispatchEx *dispex;

    HTMLDocumentObj *doc_obj;
    HTMLDocumentNode *doc_node;

    HTMLWindow *window;

    LONG task_magic;

    ConnectionPointContainer cp_container;
    ConnectionPoint cp_htmldocevents;
    ConnectionPoint cp_htmldocevents2;
    ConnectionPoint cp_propnotif;
    ConnectionPoint cp_dispatch;

    IOleAdviseHolder *advise_holder;
};

static inline HRESULT htmldoc_query_interface(HTMLDocument *This, REFIID riid, void **ppv)
{
    return IUnknown_QueryInterface(This->unk_impl, riid, ppv);
}

static inline ULONG htmldoc_addref(HTMLDocument *This)
{
    return IUnknown_AddRef(This->unk_impl);
}

static inline ULONG htmldoc_release(HTMLDocument *This)
{
    return IUnknown_Release(This->unk_impl);
}

struct HTMLDocumentObj {
    HTMLDocument basedoc;
    DispatchEx dispex;
    const ICustomDocVtbl  *lpCustomDocVtbl;

    LONG ref;

    NSContainer *nscontainer;

    IOleClientSite *client;
    IDocHostUIHandler *hostui;
    IOleInPlaceSite *ipsite;
    IOleInPlaceFrame *frame;
    IOleInPlaceUIWindow *ip_window;
    IAdviseSink *view_sink;

    DOCHOSTUIINFO hostinfo;

    IOleUndoManager *undomgr;

    HWND hwnd;
    HWND tooltips_hwnd;

    BOOL request_uiactivate;
    BOOL in_place_active;
    BOOL ui_active;
    BOOL window_active;
    BOOL hostui_setup;
    BOOL container_locked;
    BOOL focus;
    INT download_state;

    USERMODE usermode;
    LPWSTR mime;

    DWORD update;
};

struct NSContainer {
    const nsIWebBrowserChromeVtbl       *lpWebBrowserChromeVtbl;
    const nsIContextMenuListenerVtbl    *lpContextMenuListenerVtbl;
    const nsIURIContentListenerVtbl     *lpURIContentListenerVtbl;
    const nsIEmbeddingSiteWindowVtbl    *lpEmbeddingSiteWindowVtbl;
    const nsITooltipListenerVtbl        *lpTooltipListenerVtbl;
    const nsIInterfaceRequestorVtbl     *lpInterfaceRequestorVtbl;
    const nsIWeakReferenceVtbl          *lpWeakReferenceVtbl;
    const nsISupportsWeakReferenceVtbl  *lpSupportsWeakReferenceVtbl;

    nsIWebBrowser *webbrowser;
    nsIWebNavigation *navigation;
    nsIBaseWindow *window;
    nsIWebBrowserFocus *focus;

    nsIEditor *editor;
    nsIController *editor_controller;

    LONG ref;

    NSContainer *parent;
    HTMLDocumentObj *doc;

    nsIURIContentListener *content_listener;

    HWND hwnd;
};

typedef struct nsWineURI nsWineURI;

HRESULT set_wine_url(nsWineURI*,LPCWSTR);
nsresult on_start_uri_open(NSContainer*,nsIURI*,PRBool*);

typedef struct {
    const nsIHttpChannelVtbl *lpHttpChannelVtbl;
    const nsIUploadChannelVtbl *lpUploadChannelVtbl;
    const nsIHttpChannelInternalVtbl *lpIHttpChannelInternalVtbl;

    LONG ref;

    nsWineURI *uri;
    nsIInputStream *post_data_stream;
    nsILoadGroup *load_group;
    nsIInterfaceRequestor *notif_callback;
    nsISupports *owner;
    nsLoadFlags load_flags;
    nsIURI *original_uri;
    char *content_type;
    char *charset;
    PRUint32 response_status;
    struct list response_headers;
    UINT url_scheme;
} nsChannel;

struct ResponseHeader {
    struct list entry;
    WCHAR *header;
    WCHAR *data;
};

typedef struct {
    HRESULT (*qi)(HTMLDOMNode*,REFIID,void**);
    void (*destructor)(HTMLDOMNode*);
    event_target_t **(*get_event_target)(HTMLDOMNode*);
    HRESULT (*call_event)(HTMLDOMNode*,DWORD,BOOL*);
    HRESULT (*put_disabled)(HTMLDOMNode*,VARIANT_BOOL);
    HRESULT (*get_disabled)(HTMLDOMNode*,VARIANT_BOOL*);
    HRESULT (*get_document)(HTMLDOMNode*,IDispatch**);
    HRESULT (*get_readystate)(HTMLDOMNode*,BSTR*);
    HRESULT (*get_dispid)(HTMLDOMNode*,BSTR,DWORD,DISPID*);
    HRESULT (*invoke)(HTMLDOMNode*,DISPID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,IServiceProvider*);
    HRESULT (*bind_to_tree)(HTMLDOMNode*);
} NodeImplVtbl;

struct HTMLDOMNode {
    DispatchEx dispex;
    const IHTMLDOMNodeVtbl   *lpHTMLDOMNodeVtbl;
    const IHTMLDOMNode2Vtbl  *lpHTMLDOMNode2Vtbl;
    const NodeImplVtbl *vtbl;

    LONG ref;

    nsIDOMNode *nsnode;
    HTMLDocumentNode *doc;
    event_target_t *event_target;
    ConnectionPointContainer *cp_container;

    HTMLDOMNode *next;
};

typedef struct {
    HTMLDOMNode node;
    ConnectionPointContainer cp_container;

    const IHTMLElementVtbl   *lpHTMLElementVtbl;
    const IHTMLElement2Vtbl  *lpHTMLElement2Vtbl;
    const IHTMLElement3Vtbl  *lpHTMLElement3Vtbl;

    nsIDOMHTMLElement *nselem;
} HTMLElement;

#define HTMLELEMENT_TIDS    \
    IHTMLDOMNode_tid,       \
    IHTMLDOMNode2_tid,      \
    IHTMLElement_tid,       \
    IHTMLElement2_tid,      \
    IHTMLElement3_tid,      \
    IHTMLElement4_tid

typedef struct {
    HTMLElement element;

    const IHTMLTextContainerVtbl *lpHTMLTextContainerVtbl;

    ConnectionPoint cp;
} HTMLTextContainer;

struct HTMLFrameBase {
    HTMLElement element;

    const IHTMLFrameBaseVtbl   *lpIHTMLFrameBaseVtbl;
    const IHTMLFrameBase2Vtbl  *lpIHTMLFrameBase2Vtbl;

    HTMLWindow *content_window;

    nsIDOMHTMLFrameElement *nsframe;
    nsIDOMHTMLIFrameElement *nsiframe;
};

typedef struct _mutation_queue_t {
    DWORD type;
    nsISupports *nsiface;

    struct _mutation_queue_t *next;
} mutation_queue_t;

typedef struct nsDocumentEventListener nsDocumentEventListener;

struct HTMLDocumentNode {
    HTMLDOMNode node;
    HTMLDocument basedoc;

    const IInternetHostSecurityManagerVtbl *lpIInternetHostSecurityManagerVtbl;

    const nsIDocumentObserverVtbl  *lpIDocumentObserverVtbl;
    const nsIRunnableVtbl  *lpIRunnableVtbl;

    LONG ref;

    nsIDOMHTMLDocument *nsdoc;
    HTMLDOMNode *nodes;
    BOOL content_ready;
    event_target_t *body_event_target;

    IInternetSecurityManager *secmgr;
    ICatInformation *catmgr;
    nsDocumentEventListener *nsevent_listener;
    BOOL *event_vector;

    mutation_queue_t *mutation_queue;
    mutation_queue_t *mutation_queue_tail;

    struct list bindings;
    struct list selection_list;
    struct list range_list;
};

#define HTMLWINDOW2(x)   ((IHTMLWindow2*)                 &(x)->lpHTMLWindow2Vtbl)
#define HTMLWINDOW3(x)   ((IHTMLWindow3*)                 &(x)->lpHTMLWindow3Vtbl)
#define HTMLWINDOW4(x)   ((IHTMLWindow4*)                 &(x)->lpHTMLWindow4Vtbl)

#define HTMLDOC(x)       ((IHTMLDocument2*)               &(x)->lpHTMLDocument2Vtbl)
#define HTMLDOC3(x)      ((IHTMLDocument3*)               &(x)->lpHTMLDocument3Vtbl)
#define HTMLDOC4(x)      ((IHTMLDocument4*)               &(x)->lpHTMLDocument4Vtbl)
#define HTMLDOC5(x)      ((IHTMLDocument5*)               &(x)->lpHTMLDocument5Vtbl)
#define HTMLDOC6(x)      ((IHTMLDocument6*)               &(x)->lpHTMLDocument6Vtbl)
#define PERSIST(x)       ((IPersist*)                     &(x)->lpPersistFileVtbl)
#define PERSISTMON(x)    ((IPersistMoniker*)              &(x)->lpPersistMonikerVtbl)
#define PERSISTFILE(x)   ((IPersistFile*)                 &(x)->lpPersistFileVtbl)
#define MONPROP(x)       ((IMonikerProp*)                 &(x)->lpMonikerPropVtbl)
#define OLEOBJ(x)        ((IOleObject*)                   &(x)->lpOleObjectVtbl)
#define OLEDOC(x)        ((IOleDocument*)                 &(x)->lpOleDocumentVtbl)
#define DOCVIEW(x)       ((IOleDocumentView*)             &(x)->lpOleDocumentViewVtbl)
#define OLEWIN(x)        ((IOleWindow*)                   &(x)->lpOleInPlaceActiveObjectVtbl)
#define ACTOBJ(x)        ((IOleInPlaceActiveObject*)      &(x)->lpOleInPlaceActiveObjectVtbl)
#define VIEWOBJ(x)       ((IViewObject*)                  &(x)->lpViewObjectExVtbl)
#define VIEWOBJ2(x)      ((IViewObject2*)                 &(x)->lpViewObjectExVtbl)
#define VIEWOBJEX(x)      ((IViewObjectEx*)               &(x)->lpViewObjectExVtbl)
#define INPLACEOBJ(x)    ((IOleInPlaceObject*)            &(x)->lpOleInPlaceObjectWindowlessVtbl)
#define INPLACEWIN(x)    ((IOleInPlaceObjectWindowless*)  &(x)->lpOleInPlaceObjectWindowlessVtbl)
#define SERVPROV(x)      ((IServiceProvider*)             &(x)->lpServiceProviderVtbl)
#define CMDTARGET(x)     ((IOleCommandTarget*)            &(x)->lpOleCommandTargetVtbl)
#define CONTROL(x)       ((IOleControl*)                  &(x)->lpOleControlVtbl)
#define HLNKTARGET(x)    ((IHlinkTarget*)                 &(x)->lpHlinkTargetVtbl)
#define CONPTCONT(x)     ((IConnectionPointContainer*)    &(x)->lpConnectionPointContainerVtbl)
#define PERSTRINIT(x)    ((IPersistStreamInit*)           &(x)->lpPersistStreamInitVtbl)
#define PERSISTHIST(x)   ((IPersistHistory*)              &(x)->lpPersistHistoryVtbl)
#define CUSTOMDOC(x)     ((ICustomDoc*)                   &(x)->lpCustomDocVtbl)
#define OBJSITE(x)       ((IObjectWithSite*)              &(x)->lpObjectWithSiteVtbl)

#define NSWBCHROME(x)    ((nsIWebBrowserChrome*)          &(x)->lpWebBrowserChromeVtbl)
#define NSCML(x)         ((nsIContextMenuListener*)       &(x)->lpContextMenuListenerVtbl)
#define NSURICL(x)       ((nsIURIContentListener*)        &(x)->lpURIContentListenerVtbl)
#define NSEMBWNDS(x)     ((nsIEmbeddingSiteWindow*)       &(x)->lpEmbeddingSiteWindowVtbl)
#define NSIFACEREQ(x)    ((nsIInterfaceRequestor*)        &(x)->lpInterfaceRequestorVtbl)
#define NSTOOLTIP(x)     ((nsITooltipListener*)           &(x)->lpTooltipListenerVtbl)
#define NSEVENTLIST(x)   ((nsIDOMEventListener*)          &(x)->lpDOMEventListenerVtbl)
#define NSWEAKREF(x)     ((nsIWeakReference*)             &(x)->lpWeakReferenceVtbl)
#define NSSUPWEAKREF(x)  ((nsISupportsWeakReference*)     &(x)->lpSupportsWeakReferenceVtbl)

#define NSDOCOBS(x)      ((nsIDocumentObserver*)          &(x)->lpIDocumentObserverVtbl)

#define NSRUNNABLE(x)    ((nsIRunnable*)  &(x)->lpIRunnableVtbl)

#define NSCHANNEL(x)     ((nsIChannel*)        &(x)->lpHttpChannelVtbl)
#define NSHTTPCHANNEL(x) ((nsIHttpChannel*)    &(x)->lpHttpChannelVtbl)
#define NSUPCHANNEL(x)   ((nsIUploadChannel*)  &(x)->lpUploadChannelVtbl)
#define NSHTTPINTERNAL(x) ((nsIHttpChannelInternal*)  &(x)->lpIHttpChannelInternalVtbl)

#define HTTPNEG(x)       ((IHttpNegotiate2*)              &(x)->lpHttpNegotiate2Vtbl)
#define STATUSCLB(x)     ((IBindStatusCallback*)          &(x)->lpBindStatusCallbackVtbl)
#define BINDINFO(x)      ((IInternetBindInfo*)            &(x)->lpInternetBindInfoVtbl);

#define HTMLELEM(x)      ((IHTMLElement*)                 &(x)->lpHTMLElementVtbl)
#define HTMLELEM2(x)     ((IHTMLElement2*)                &(x)->lpHTMLElement2Vtbl)
#define HTMLELEM3(x)     ((IHTMLElement3*)                &(x)->lpHTMLElement3Vtbl)
#define HTMLDOMNODE(x)   ((IHTMLDOMNode*)                 &(x)->lpHTMLDOMNodeVtbl)
#define HTMLDOMNODE2(x)  ((IHTMLDOMNode2*)                &(x)->lpHTMLDOMNode2Vtbl)

#define HTMLTEXTCONT(x)  ((IHTMLTextContainer*)           &(x)->lpHTMLTextContainerVtbl)
#define HTMLFRAMEBASE(x) ((IHTMLFrameBase*)               &(x)->lpIHTMLFrameBaseVtbl)
#define HTMLFRAMEBASE2(x) ((IHTMLFrameBase2*)             &(x)->lpIHTMLFrameBase2Vtbl)

#define HTMLOPTFACTORY(x)  ((IHTMLOptionElementFactory*)  &(x)->lpHTMLOptionElementFactoryVtbl)
#define HTMLIMGFACTORY(x)  ((IHTMLImageElementFactory*)   &(x)->lpHTMLImageElementFactoryVtbl)
#define HTMLLOCATION(x)    ((IHTMLLocation*)              &(x)->lpHTMLLocationVtbl)

#define DISPATCHEX(x)    ((IDispatchEx*) &(x)->lpIDispatchExVtbl)

#define SUPPERRINFO(x)   ((ISupportErrorInfo*) &(x)->lpSupportErrorInfoVtbl)

#define HOSTSECMGR(x)    ((IInternetHostSecurityManager*)  &(x)->lpIInternetHostSecurityManagerVtbl)

#define DEFINE_THIS2(cls,ifc,iface) ((cls*)((BYTE*)(iface)-offsetof(cls,ifc)))
#define DEFINE_THIS(cls,ifc,iface) DEFINE_THIS2(cls,lp ## ifc ## Vtbl,iface)

HRESULT HTMLDocument_Create(IUnknown*,REFIID,void**);
HRESULT HTMLLoadOptions_Create(IUnknown*,REFIID,void**);
HRESULT create_doc_from_nsdoc(nsIDOMHTMLDocument*,HTMLDocumentObj*,HTMLWindow*,HTMLDocumentNode**);

HRESULT HTMLWindow_Create(HTMLDocumentObj*,nsIDOMWindow*,HTMLWindow*,HTMLWindow**);
void update_window_doc(HTMLWindow*);
HTMLWindow *nswindow_to_window(const nsIDOMWindow*);
nsIDOMWindow *get_nsdoc_window(nsIDOMDocument*);
HTMLOptionElementFactory *HTMLOptionElementFactory_Create(HTMLWindow*);
HTMLImageElementFactory *HTMLImageElementFactory_Create(HTMLWindow*);
HRESULT HTMLLocation_Create(HTMLWindow*,HTMLLocation**);
IOmNavigator *OmNavigator_Create(void);
HRESULT HTMLScreen_Create(IHTMLScreen**);

void HTMLDocument_HTMLDocument3_Init(HTMLDocument*);
void HTMLDocument_HTMLDocument5_Init(HTMLDocument*);
void HTMLDocument_Persist_Init(HTMLDocument*);
void HTMLDocument_OleCmd_Init(HTMLDocument*);
void HTMLDocument_OleObj_Init(HTMLDocument*);
void HTMLDocument_View_Init(HTMLDocument*);
void HTMLDocument_Window_Init(HTMLDocument*);
void HTMLDocument_Service_Init(HTMLDocument*);
void HTMLDocument_Hlink_Init(HTMLDocument*);

void HTMLDocumentNode_SecMgr_Init(HTMLDocumentNode*);

HRESULT HTMLCurrentStyle_Create(HTMLElement*,IHTMLCurrentStyle**);

void ConnectionPoint_Init(ConnectionPoint*,ConnectionPointContainer*,REFIID,cp_static_data_t*);
void ConnectionPointContainer_Init(ConnectionPointContainer*,IUnknown*);
void ConnectionPointContainer_Destroy(ConnectionPointContainer*);

NSContainer *NSContainer_Create(HTMLDocumentObj*,NSContainer*);
void NSContainer_Release(NSContainer*);

void init_mutation(HTMLDocumentNode*);
void release_mutation(HTMLDocumentNode*);

void HTMLDocument_LockContainer(HTMLDocumentObj*,BOOL);
void show_context_menu(HTMLDocumentObj*,DWORD,POINT*,IDispatch*);
void notif_focus(HTMLDocumentObj*);

void show_tooltip(HTMLDocumentObj*,DWORD,DWORD,LPCWSTR);
void hide_tooltip(HTMLDocumentObj*);
HRESULT get_client_disp_property(IOleClientSite*,DISPID,VARIANT*);

HRESULT ProtocolFactory_Create(REFCLSID,REFIID,void**);

BOOL load_gecko(BOOL);
void close_gecko(void);
void register_nsservice(nsIComponentRegistrar*,nsIServiceManager*);
void init_nsio(nsIComponentManager*,nsIComponentRegistrar*);
void release_nsio(void);
BOOL install_wine_gecko(BOOL);

HRESULT nsuri_to_url(LPCWSTR,BOOL,BSTR*);
HRESULT create_doc_uri(HTMLWindow*,WCHAR*,nsWineURI**);
HRESULT load_nsuri(HTMLWindow*,nsWineURI*,nsChannelBSC*,DWORD);

HRESULT hlink_frame_navigate(HTMLDocument*,LPCWSTR,nsIInputStream*,DWORD,BOOL*);
HRESULT navigate_url(HTMLWindow*,const WCHAR*,const WCHAR*);
HRESULT set_frame_doc(HTMLFrameBase*,nsIDOMDocument*);
HRESULT set_moniker(HTMLDocument*,IMoniker*,IBindCtx*,nsChannelBSC*,BOOL);

void call_property_onchanged(ConnectionPoint*,DISPID);
HRESULT call_set_active_object(IOleInPlaceUIWindow*,IOleInPlaceActiveObject*);

void *nsalloc(size_t) __WINE_ALLOC_SIZE(1);
void nsfree(void*);

void nsACString_InitDepend(nsACString*,const char*);
void nsACString_SetData(nsACString*,const char*);
PRUint32 nsACString_GetData(const nsACString*,const char**);
void nsACString_Finish(nsACString*);

BOOL nsAString_Init(nsAString*,const PRUnichar*);
void nsAString_InitDepend(nsAString*,const PRUnichar*);
void nsAString_SetData(nsAString*,const PRUnichar*);
PRUint32 nsAString_GetData(const nsAString*,const PRUnichar**);
void nsAString_Finish(nsAString*);

nsICommandParams *create_nscommand_params(void);
HRESULT nsnode_to_nsstring(nsIDOMNode*,nsAString*);
void get_editor_controller(NSContainer*);
nsresult get_nsinterface(nsISupports*,REFIID,void**);

void init_nsevents(HTMLDocumentNode*);
void release_nsevents(HTMLDocumentNode*);
void add_nsevent_listener(HTMLDocumentNode*,nsIDOMNode*,LPCWSTR);

void set_window_bscallback(HTMLWindow*,nsChannelBSC*);
void set_current_mon(HTMLWindow*,IMoniker*);
HRESULT start_binding(HTMLWindow*,HTMLDocumentNode*,BSCallback*,IBindCtx*);
HRESULT async_start_doc_binding(HTMLWindow*,nsChannelBSC*);
void abort_document_bindings(HTMLDocumentNode*);

HRESULT bind_mon_to_buffer(HTMLDocumentNode*,IMoniker*,void**,DWORD*);

HRESULT create_channelbsc(IMoniker*,WCHAR*,BYTE*,DWORD,nsChannelBSC**);
HRESULT channelbsc_load_stream(nsChannelBSC*,IStream*);
void channelbsc_set_channel(nsChannelBSC*,nsChannel*,nsIStreamListener*,nsISupports*);
IMoniker *get_channelbsc_mon(nsChannelBSC*);

void set_ready_state(HTMLWindow*,READYSTATE);

HRESULT HTMLSelectionObject_Create(HTMLDocumentNode*,nsISelection*,IHTMLSelectionObject**);
HRESULT HTMLTxtRange_Create(HTMLDocumentNode*,nsIDOMRange*,IHTMLTxtRange**);
IHTMLStyle *HTMLStyle_Create(nsIDOMCSSStyleDeclaration*);
IHTMLStyleSheet *HTMLStyleSheet_Create(nsIDOMStyleSheet*);
IHTMLStyleSheetsCollection *HTMLStyleSheetsCollection_Create(nsIDOMStyleSheetList*);

void detach_selection(HTMLDocumentNode*);
void detach_ranges(HTMLDocumentNode*);
HRESULT get_node_text(HTMLDOMNode*,BSTR*);

HRESULT create_nselem(HTMLDocumentNode*,const WCHAR*,nsIDOMHTMLElement**);

HTMLDOMNode *HTMLDOMTextNode_Create(HTMLDocumentNode*,nsIDOMNode*);

HTMLElement *HTMLElement_Create(HTMLDocumentNode*,nsIDOMNode*,BOOL);
HTMLElement *HTMLCommentElement_Create(HTMLDocumentNode*,nsIDOMNode*);
HTMLElement *HTMLAnchorElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*);
HTMLElement *HTMLBodyElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*);
HTMLElement *HTMLFormElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*);
HTMLElement *HTMLFrameElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*);
HTMLElement *HTMLIFrame_Create(HTMLDocumentNode*,nsIDOMHTMLElement*);
HTMLElement *HTMLImgElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*);
HTMLElement *HTMLInputElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*);
HTMLElement *HTMLOptionElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*);
HTMLElement *HTMLScriptElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*);
HTMLElement *HTMLSelectElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*);
HTMLElement *HTMLTable_Create(HTMLDocumentNode*,nsIDOMHTMLElement*);
HTMLElement *HTMLTableRow_Create(HTMLDocumentNode*,nsIDOMHTMLElement*);
HTMLElement *HTMLTextAreaElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*);
HTMLElement *HTMLGenericElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*);

void HTMLDOMNode_Init(HTMLDocumentNode*,HTMLDOMNode*,nsIDOMNode*);
void HTMLElement_Init(HTMLElement*,HTMLDocumentNode*,nsIDOMHTMLElement*,dispex_static_data_t*);
void HTMLElement2_Init(HTMLElement*);
void HTMLElement3_Init(HTMLElement*);
void HTMLTextContainer_Init(HTMLTextContainer*,HTMLDocumentNode*,nsIDOMHTMLElement*,dispex_static_data_t*);
void HTMLFrameBase_Init(HTMLFrameBase*,HTMLDocumentNode*,nsIDOMHTMLElement*,dispex_static_data_t*);

HRESULT HTMLDOMNode_QI(HTMLDOMNode*,REFIID,void**);
void HTMLDOMNode_destructor(HTMLDOMNode*);

HRESULT HTMLElement_QI(HTMLDOMNode*,REFIID,void**);
void HTMLElement_destructor(HTMLDOMNode*);

HRESULT HTMLFrameBase_QI(HTMLFrameBase*,REFIID,void**);
void HTMLFrameBase_destructor(HTMLFrameBase*);

HTMLDOMNode *get_node(HTMLDocumentNode*,nsIDOMNode*,BOOL);
void release_nodes(HTMLDocumentNode*);

void release_script_hosts(HTMLWindow*);
void connect_scripts(HTMLWindow*);
void doc_insert_script(HTMLWindow*,nsIDOMHTMLScriptElement*);
IDispatch *script_parse_event(HTMLWindow*,LPCWSTR);
void set_script_mode(HTMLWindow*,SCRIPTMODE);
BOOL find_global_prop(HTMLWindow*,BSTR,DWORD,ScriptHost**,DISPID*);
IDispatch *get_script_disp(ScriptHost*);
HRESULT search_window_props(HTMLWindow*,BSTR,DWORD,DISPID*);

IHTMLElementCollection *create_all_collection(HTMLDOMNode*,BOOL);
IHTMLElementCollection *create_collection_from_nodelist(HTMLDocumentNode*,IUnknown*,nsIDOMNodeList*);
IHTMLElementCollection *create_collection_from_htmlcol(HTMLDocumentNode*,IUnknown*,nsIDOMHTMLCollection*);

/* commands */
typedef struct {
    DWORD id;
    HRESULT (*query)(HTMLDocument*,OLECMD*);
    HRESULT (*exec)(HTMLDocument*,DWORD,VARIANT*,VARIANT*);
} cmdtable_t;

extern const cmdtable_t editmode_cmds[];

void do_ns_command(HTMLDocument*,const char*,nsICommandParams*);

/* timer */
#define UPDATE_UI       0x0001
#define UPDATE_TITLE    0x0002

void update_doc(HTMLDocument*,DWORD);
void update_title(HTMLDocumentObj*);

/* editor */
void init_editor(HTMLDocument*);
void handle_edit_event(HTMLDocument*,nsIDOMEvent*);
HRESULT editor_exec_copy(HTMLDocument*,DWORD,VARIANT*,VARIANT*);
HRESULT editor_exec_cut(HTMLDocument*,DWORD,VARIANT*,VARIANT*);
HRESULT editor_exec_paste(HTMLDocument*,DWORD,VARIANT*,VARIANT*);
void handle_edit_load(HTMLDocument*);
HRESULT editor_is_dirty(HTMLDocument*);
void set_dirty(HTMLDocument*,VARIANT_BOOL);

extern DWORD mshtml_tls;

typedef struct task_t task_t;
typedef void (*task_proc_t)(task_t*);

struct task_t {
    LONG target_magic;
    task_proc_t proc;
    struct task_t *next;
};

typedef struct {
    task_t header;
    HTMLDocumentObj *doc;
} docobj_task_t;

typedef struct {
    HWND thread_hwnd;
    task_t *task_queue_head;
    task_t *task_queue_tail;
    struct list timer_list;
} thread_data_t;

thread_data_t *get_thread_data(BOOL);
HWND get_thread_hwnd(void);

LONG get_task_target_magic(void);
void push_task(task_t*,task_proc_t,LONG);
void remove_target_tasks(LONG);

DWORD set_task_timer(HTMLDocument*,DWORD,BOOL,IDispatch*);
HRESULT clear_task_timer(HTMLDocument*,BOOL,DWORD);

void release_typelib(void);
HRESULT call_disp_func(IDispatch*,DISPPARAMS*);

const char *debugstr_variant(const VARIANT*);

DEFINE_GUID(CLSID_AboutProtocol, 0x3050F406, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_JSProtocol, 0x3050F3B2, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_MailtoProtocol, 0x3050F3DA, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_ResProtocol, 0x3050F3BC, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_SysimageProtocol, 0x76E67A63, 0x06E9, 0x11D2, 0xA8,0x40, 0x00,0x60,0x08,0x05,0x93,0x82);

DEFINE_GUID(CLSID_CMarkup,0x3050f4fb,0x98b5,0x11cf,0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b);

/* memory allocation functions */

static inline void * __WINE_ALLOC_SIZE(1) heap_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline void * __WINE_ALLOC_SIZE(1) heap_alloc_zero(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

static inline void * __WINE_ALLOC_SIZE(2) heap_realloc(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), 0, mem, len);
}

static inline void * __WINE_ALLOC_SIZE(2) heap_realloc_zero(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mem, len);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

static inline LPWSTR heap_strdupW(LPCWSTR str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD size;

        size = (strlenW(str)+1)*sizeof(WCHAR);
        ret = heap_alloc(size);
        memcpy(ret, str, size);
    }

    return ret;
}

static inline char *heap_strdupA(const char *str)
{
    char *ret = NULL;

    if(str) {
        DWORD size;

        size = strlen(str)+1;
        ret = heap_alloc(size);
        memcpy(ret, str, size);
    }

    return ret;
}

static inline WCHAR *heap_strdupAtoW(const char *str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD len;

        len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
        ret = heap_alloc(len*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    }

    return ret;
}

static inline char *heap_strdupWtoA(LPCWSTR str)
{
    char *ret = NULL;

    if(str) {
        DWORD size = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
        ret = heap_alloc(size);
        WideCharToMultiByte(CP_ACP, 0, str, -1, ret, size, NULL, NULL);
    }

    return ret;
}

static inline void windowref_addref(windowref_t *ref)
{
    InterlockedIncrement(&ref->ref);
}

static inline void windowref_release(windowref_t *ref)
{
    if(!InterlockedDecrement(&ref->ref))
        heap_free(ref);
}

HDC get_display_dc(void);
HINSTANCE get_shdoclc(void);

extern HINSTANCE hInst;
