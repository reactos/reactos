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

#ifndef _MSHTML_PRIVATE_H_
#define _MSHTML_PRIVATE_H_

#include <wine/config.h>

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <ole2.h>
#include <docobjectservice.h>
#include <mshtmhst.h>
#include <mshtmcid.h>
#include <mshtmdid.h>
#include <perhist.h>
#include <objsafe.h>
#include <htiframe.h>
#include <tlogstg.h>
#include <shdeprecated.h>
#include <shlguid.h>
#define NO_SHLWAPI_REG
#include <shlwapi.h>
#include <optary.h>
#include <idispids.h>
#include <wininet.h>
#include <nsiface.h>

#include <wine/debug.h>
#include <wine/list.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define NS_ERROR_GENERATE_FAILURE(module,code) \
    ((nsresult) (((UINT32)(1u<<31)) | ((UINT32)(module+0x45)<<16) | ((UINT32)(code))))

#define NS_OK                     ((nsresult)0x00000000L)
#define NS_ERROR_FAILURE          ((nsresult)0x80004005L)
#define NS_ERROR_OUT_OF_MEMORY    ((nsresult)0x8007000EL)
#define NS_ERROR_NOT_IMPLEMENTED  ((nsresult)0x80004001L)
#define NS_NOINTERFACE            ((nsresult)0x80004002L)
#define NS_ERROR_INVALID_POINTER  ((nsresult)0x80004003L)
#define NS_ERROR_NULL_POINTER     NS_ERROR_INVALID_POINTER
#define NS_ERROR_NOT_AVAILABLE    ((nsresult)0x80040111L)
#define NS_ERROR_INVALID_ARG      ((nsresult)0x80070057L) 
#define NS_ERROR_UNEXPECTED       ((nsresult)0x8000ffffL)

#define NS_ERROR_MODULE_NETWORK    6

#define NS_BINDING_ABORTED         NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_NETWORK, 2)
#define NS_ERROR_UNKNOWN_PROTOCOL  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_NETWORK, 18)

#define NS_FAILED(res) ((res) & 0x80000000)
#define NS_SUCCEEDED(res) (!NS_FAILED(res))

#define NSAPI WINAPI

#define MSHTML_E_NODOC    0x800a025c

typedef struct HTMLDOMNode HTMLDOMNode;
typedef struct ConnectionPoint ConnectionPoint;
typedef struct BSCallback BSCallback;
typedef struct event_target_t event_target_t;

#define TID_LIST \
    XIID(NULL) \
    XDIID(DispCEventObj) \
    XDIID(DispCPlugins) \
    XDIID(DispDOMChildrenCollection) \
    XDIID(DispHTMLAnchorElement) \
    XDIID(DispHTMLAreaElement) \
    XDIID(DispHTMLAttributeCollection) \
    XDIID(DispHTMLBody) \
    XDIID(DispHTMLButtonElement) \
    XDIID(DispHTMLCommentElement) \
    XDIID(DispHTMLCurrentStyle) \
    XDIID(DispHTMLDocument) \
    XDIID(DispHTMLDOMAttribute) \
    XDIID(DispHTMLDOMTextNode) \
    XDIID(DispHTMLElementCollection) \
    XDIID(DispHTMLEmbed) \
    XDIID(DispHTMLFormElement) \
    XDIID(DispHTMLGenericElement) \
    XDIID(DispHTMLFrameElement) \
    XDIID(DispHTMLHeadElement) \
    XDIID(DispHTMLHistory) \
    XDIID(DispHTMLIFrame) \
    XDIID(DispHTMLImg) \
    XDIID(DispHTMLInputElement) \
    XDIID(DispHTMLLabelElement) \
    XDIID(DispHTMLLinkElement) \
    XDIID(DispHTMLLocation) \
    XDIID(DispHTMLMetaElement) \
    XDIID(DispHTMLNavigator) \
    XDIID(DispHTMLObjectElement) \
    XDIID(DispHTMLOptionElement) \
    XDIID(DispHTMLScreen) \
    XDIID(DispHTMLScriptElement) \
    XDIID(DispHTMLSelectElement) \
    XDIID(DispHTMLStyle) \
    XDIID(DispHTMLStyleElement) \
    XDIID(DispHTMLStyleSheet) \
    XDIID(DispHTMLStyleSheetRulesCollection) \
    XDIID(DispHTMLStyleSheetsCollection) \
    XDIID(DispHTMLTable) \
    XDIID(DispHTMLTableCell) \
    XDIID(DispHTMLTableRow) \
    XDIID(DispHTMLTextAreaElement) \
    XDIID(DispHTMLTitleElement) \
    XDIID(DispHTMLUnknownElement) \
    XDIID(DispHTMLWindow2) \
    XDIID(DispHTMLXMLHttpRequest) \
    XDIID(HTMLDocumentEvents) \
    XDIID(HTMLElementEvents2) \
    XIID(IHTMLAnchorElement) \
    XIID(IHTMLAreaElement) \
    XIID(IHTMLAttributeCollection) \
    XIID(IHTMLAttributeCollection2) \
    XIID(IHTMLAttributeCollection3) \
    XIID(IHTMLBodyElement) \
    XIID(IHTMLBodyElement2) \
    XIID(IHTMLButtonElement) \
    XIID(IHTMLCommentElement) \
    XIID(IHTMLCurrentStyle) \
    XIID(IHTMLCurrentStyle2) \
    XIID(IHTMLCurrentStyle3) \
    XIID(IHTMLCurrentStyle4) \
    XIID(IHTMLDocument2) \
    XIID(IHTMLDocument3) \
    XIID(IHTMLDocument4) \
    XIID(IHTMLDocument5) \
    XIID(IHTMLDOMAttribute) \
    XIID(IHTMLDOMAttribute2) \
    XIID(IHTMLDOMChildrenCollection) \
    XIID(IHTMLDOMImplementation) \
    XIID(IHTMLDOMNode) \
    XIID(IHTMLDOMNode2) \
    XIID(IHTMLDOMTextNode) \
    XIID(IHTMLDOMTextNode2) \
    XIID(IHTMLElement) \
    XIID(IHTMLElement2) \
    XIID(IHTMLElement3) \
    XIID(IHTMLElement4) \
    XIID(IHTMLElementCollection) \
    XIID(IHTMLEmbedElement) \
    XIID(IHTMLEventObj) \
    XIID(IHTMLFiltersCollection) \
    XIID(IHTMLFormElement) \
    XIID(IHTMLFrameBase) \
    XIID(IHTMLFrameBase2) \
    XIID(IHTMLFrameElement3) \
    XIID(IHTMLGenericElement) \
    XIID(IHTMLHeadElement) \
    XIID(IHTMLIFrameElement) \
    XIID(IHTMLIFrameElement2) \
    XIID(IHTMLIFrameElement3) \
    XIID(IHTMLImageElementFactory) \
    XIID(IHTMLImgElement) \
    XIID(IHTMLInputElement) \
    XIID(IHTMLLabelElement) \
    XIID(IHTMLLinkElement) \
    XIID(IHTMLLocation) \
    XIID(IHTMLMetaElement) \
    XIID(IHTMLMimeTypesCollection) \
    XIID(IHTMLObjectElement) \
    XIID(IHTMLObjectElement2) \
    XIID(IHTMLOptionElement) \
    XIID(IHTMLOptionElementFactory) \
    XIID(IHTMLPluginsCollection) \
    XIID(IHTMLRect) \
    XIID(IHTMLScreen) \
    XIID(IHTMLScriptElement) \
    XIID(IHTMLSelectElement) \
    XIID(IHTMLSelectionObject) \
    XIID(IHTMLSelectionObject2) \
    XIID(IHTMLStorage) \
    XIID(IHTMLStyle) \
    XIID(IHTMLStyle2) \
    XIID(IHTMLStyle3) \
    XIID(IHTMLStyle4) \
    XIID(IHTMLStyle5) \
    XIID(IHTMLStyle6) \
    XIID(IHTMLStyleElement) \
    XIID(IHTMLStyleSheet) \
    XIID(IHTMLStyleSheetRulesCollection) \
    XIID(IHTMLStyleSheetsCollection) \
    XIID(IHTMLTable) \
    XIID(IHTMLTable2) \
    XIID(IHTMLTable3) \
    XIID(IHTMLTableCell) \
    XIID(IHTMLTableRow) \
    XIID(IHTMLTextAreaElement) \
    XIID(IHTMLTextContainer) \
    XIID(IHTMLTitleElement) \
    XIID(IHTMLTxtRange) \
    XIID(IHTMLUniqueName) \
    XIID(IHTMLWindow2) \
    XIID(IHTMLWindow3) \
    XIID(IHTMLWindow4) \
    XIID(IHTMLWindow5) \
    XIID(IHTMLWindow6) \
    XIID(IHTMLXMLHttpRequest) \
    XIID(IHTMLXMLHttpRequestFactory) \
    XIID(IOmHistory) \
    XIID(IOmNavigator)

typedef enum {
#define XIID(iface) iface ## _tid,
#define XDIID(iface) iface ## _tid,
TID_LIST
#undef XIID
#undef XDIID
    LAST_tid
} tid_t;

typedef struct dispex_data_t dispex_data_t;
typedef struct dispex_dynamic_data_t dispex_dynamic_data_t;

#define MSHTML_DISPID_CUSTOM_MIN 0x60000000
#define MSHTML_DISPID_CUSTOM_MAX 0x6fffffff
#define MSHTML_CUSTOM_DISPID_CNT (MSHTML_DISPID_CUSTOM_MAX-MSHTML_DISPID_CUSTOM_MIN)

typedef struct DispatchEx DispatchEx;

typedef struct {
    HRESULT (*value)(DispatchEx*,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,IServiceProvider*);
    HRESULT (*get_dispid)(DispatchEx*,BSTR,DWORD,DISPID*);
    HRESULT (*invoke)(DispatchEx*,DISPID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,IServiceProvider*);
    HRESULT (*populate_props)(DispatchEx*);
    /* We abuse this vtbl for EventTarget functions to avoid separated vtbl. */
    event_target_t **(*get_event_target_ptr)(DispatchEx*);
    void (*bind_event)(DispatchEx*,int);
} dispex_static_data_vtbl_t;

typedef struct {
    const dispex_static_data_vtbl_t *vtbl;
    const tid_t disp_tid;
    dispex_data_t *data;
    const tid_t* const iface_tids;
} dispex_static_data_t;

struct DispatchEx {
    IDispatchEx IDispatchEx_iface;

    IUnknown *outer;

    dispex_static_data_t *data;
    dispex_dynamic_data_t *dynamic_data;
};

typedef struct {
    UINT_PTR x;
} nsCycleCollectingAutoRefCnt;

typedef struct {
    void *vtbl;
    int ref_flags;
    void *callbacks;
} ExternalCycleCollectionParticipant;

typedef struct nsCycleCollectionTraversalCallback nsCycleCollectionTraversalCallback;

typedef struct {
    nsresult (NSAPI *traverse)(void*,void*,nsCycleCollectionTraversalCallback*);
    nsresult (NSAPI *unlink)(void*);
    void (NSAPI *delete_cycle_collectable)(void*);
} CCObjCallback;

DEFINE_GUID(IID_nsXPCOMCycleCollectionParticipant, 0x9674489b,0x1f6f,0x4550,0xa7,0x30, 0xcc,0xae,0xdd,0x10,0x4c,0xf9);

extern nsrefcnt (__cdecl *ccref_incr)(nsCycleCollectingAutoRefCnt*,nsISupports*) DECLSPEC_HIDDEN;
extern nsrefcnt (__cdecl *ccref_decr)(nsCycleCollectingAutoRefCnt*,nsISupports*,ExternalCycleCollectionParticipant*) DECLSPEC_HIDDEN;
extern void (__cdecl *ccref_init)(nsCycleCollectingAutoRefCnt*,nsrefcnt) DECLSPEC_HIDDEN;
extern void (__cdecl *ccp_init)(ExternalCycleCollectionParticipant*,const CCObjCallback*) DECLSPEC_HIDDEN;
extern void (__cdecl *describe_cc_node)(nsCycleCollectingAutoRefCnt*,const char*,nsCycleCollectionTraversalCallback*) DECLSPEC_HIDDEN;
extern void (__cdecl *note_cc_edge)(nsISupports*,const char*,nsCycleCollectionTraversalCallback*) DECLSPEC_HIDDEN;

void init_dispex(DispatchEx*,IUnknown*,dispex_static_data_t*) DECLSPEC_HIDDEN;
void release_dispex(DispatchEx*) DECLSPEC_HIDDEN;
BOOL dispex_query_interface(DispatchEx*,REFIID,void**) DECLSPEC_HIDDEN;
HRESULT dispex_get_dprop_ref(DispatchEx*,const WCHAR*,BOOL,VARIANT**) DECLSPEC_HIDDEN;
HRESULT get_dispids(tid_t,DWORD*,DISPID**) DECLSPEC_HIDDEN;
HRESULT remove_attribute(DispatchEx*,DISPID,VARIANT_BOOL*) DECLSPEC_HIDDEN;
HRESULT dispex_get_dynid(DispatchEx*,const WCHAR*,DISPID*) DECLSPEC_HIDDEN;
void dispex_traverse(DispatchEx*,nsCycleCollectionTraversalCallback*) DECLSPEC_HIDDEN;
void dispex_unlink(DispatchEx*) DECLSPEC_HIDDEN;
void release_typelib(void) DECLSPEC_HIDDEN;
HRESULT get_htmldoc_classinfo(ITypeInfo **typeinfo) DECLSPEC_HIDDEN;

typedef enum {
    DISPEXPROP_CUSTOM,
    DISPEXPROP_DYNAMIC,
    DISPEXPROP_BUILTIN
} dispex_prop_type_t;

dispex_prop_type_t get_dispid_type(DISPID) DECLSPEC_HIDDEN;

typedef struct HTMLWindow HTMLWindow;
typedef struct HTMLInnerWindow HTMLInnerWindow;
typedef struct HTMLOuterWindow HTMLOuterWindow;
typedef struct HTMLDocumentNode HTMLDocumentNode;
typedef struct HTMLDocumentObj HTMLDocumentObj;
typedef struct HTMLFrameBase HTMLFrameBase;
typedef struct NSContainer NSContainer;
typedef struct HTMLAttributeCollection HTMLAttributeCollection;

typedef enum {
    SCRIPTMODE_GECKO,
    SCRIPTMODE_ACTIVESCRIPT
} SCRIPTMODE;

typedef struct ScriptHost ScriptHost;

typedef enum {
    GLOBAL_SCRIPTVAR,
    GLOBAL_ELEMENTVAR,
    GLOBAL_DISPEXVAR,
    GLOBAL_FRAMEVAR
} global_prop_type_t;

typedef struct {
    global_prop_type_t type;
    WCHAR *name;
    ScriptHost *script_host;
    DISPID id;
} global_prop_t;

typedef struct {
    DispatchEx dispex;
    event_target_t *ptr;
} EventTarget;

typedef struct {
    DispatchEx dispex;
    IHTMLOptionElementFactory IHTMLOptionElementFactory_iface;

    LONG ref;

    HTMLInnerWindow *window;
} HTMLOptionElementFactory;

typedef struct {
    DispatchEx dispex;
    IHTMLImageElementFactory IHTMLImageElementFactory_iface;

    LONG ref;

    HTMLInnerWindow *window;
} HTMLImageElementFactory;

typedef struct {
    DispatchEx dispex;
    IHTMLXMLHttpRequestFactory IHTMLXMLHttpRequestFactory_iface;

    LONG ref;

    HTMLInnerWindow *window;
} HTMLXMLHttpRequestFactory;

struct HTMLLocation {
    DispatchEx dispex;
    IHTMLLocation IHTMLLocation_iface;

    LONG ref;

    HTMLInnerWindow *window;
};

typedef struct {
    DispatchEx dispex;
    IOmHistory IOmHistory_iface;

    LONG ref;

    HTMLInnerWindow *window;
} OmHistory;

typedef struct {
    HTMLOuterWindow *window;
    LONG ref;
}  windowref_t;

typedef struct nsChannelBSC nsChannelBSC;

struct HTMLWindow {
    IHTMLWindow2       IHTMLWindow2_iface;
    IHTMLWindow3       IHTMLWindow3_iface;
    IHTMLWindow4       IHTMLWindow4_iface;
    IHTMLWindow5       IHTMLWindow5_iface;
    IHTMLWindow6       IHTMLWindow6_iface;
    IHTMLPrivateWindow IHTMLPrivateWindow_iface;
    IDispatchEx        IDispatchEx_iface;
    IServiceProvider   IServiceProvider_iface;
    ITravelLogClient   ITravelLogClient_iface;
    IObjectIdentity    IObjectIdentity_iface;

    LONG ref;

    HTMLInnerWindow *inner_window;
    HTMLOuterWindow *outer_window;
};

struct HTMLOuterWindow {
    HTMLWindow base;

    windowref_t *window_ref;
    LONG task_magic;

    HTMLDocumentObj *doc_obj;
    nsIDOMWindow *nswindow;
    HTMLOuterWindow *parent;
    HTMLFrameBase *frame_element;

    READYSTATE readystate;
    BOOL readystate_locked;
    unsigned readystate_pending;

    HTMLInnerWindow *pending_window;
    IMoniker *mon;
    IUri *uri;
    IUri *uri_nofrag;
    BSTR url;
    DWORD load_flags;

    SCRIPTMODE scriptmode;

    IInternetSecurityManager *secmgr;

    struct list children;
    struct list sibling_entry;
    struct list entry;
};

struct HTMLInnerWindow {
    HTMLWindow base;
    EventTarget event_target;

    HTMLDocumentNode *doc;

    struct list script_hosts;

    IHTMLEventObj *event;

    HTMLImageElementFactory *image_factory;
    HTMLOptionElementFactory *option_factory;
    HTMLXMLHttpRequestFactory *xhr_factory;
    IHTMLScreen *screen;
    OmHistory *history;
    IHTMLStorage *session_storage;

    unsigned parser_callback_cnt;
    struct list script_queue;

    global_prop_t *global_props;
    DWORD global_prop_cnt;
    DWORD global_prop_size;

    LONG task_magic;

    HTMLLocation *location;

    IMoniker *mon;
    nsChannelBSC *bscallback;
    struct list bindings;
};

typedef enum {
    UNKNOWN_USERMODE,
    BROWSEMODE,
    EDITMODE        
} USERMODE;

typedef struct _cp_static_data_t {
    tid_t tid;
    void (*on_advise)(IUnknown*,struct _cp_static_data_t*);
    BOOL pass_event_arg;
    DWORD id_cnt;
    DISPID *ids;
} cp_static_data_t;

typedef struct {
    const IID *riid;
    cp_static_data_t *desc;
} cpc_entry_t;

typedef struct ConnectionPointContainer {
    IConnectionPointContainer IConnectionPointContainer_iface;

    ConnectionPoint *cps;
    const cpc_entry_t *cp_entries;
    IUnknown *outer;
    struct ConnectionPointContainer *forward_container;
} ConnectionPointContainer;

struct  ConnectionPoint {
    IConnectionPoint IConnectionPoint_iface;

    ConnectionPointContainer *container;

    union {
        IUnknown *unk;
        IDispatch *disp;
        IPropertyNotifySink *propnotif;
    } *sinks;
    DWORD sinks_size;

    const IID *iid;
    cp_static_data_t *data;
};

struct HTMLDocument {
    IHTMLDocument2              IHTMLDocument2_iface;
    IHTMLDocument3              IHTMLDocument3_iface;
    IHTMLDocument4              IHTMLDocument4_iface;
    IHTMLDocument5              IHTMLDocument5_iface;
    IHTMLDocument6              IHTMLDocument6_iface;
    IHTMLDocument7              IHTMLDocument7_iface;
    IPersistMoniker             IPersistMoniker_iface;
    IPersistFile                IPersistFile_iface;
    IPersistHistory             IPersistHistory_iface;
    IMonikerProp                IMonikerProp_iface;
    IOleObject                  IOleObject_iface;
    IOleDocument                IOleDocument_iface;
    IOleDocumentView            IOleDocumentView_iface;
    IOleInPlaceActiveObject     IOleInPlaceActiveObject_iface;
    IViewObjectEx               IViewObjectEx_iface;
    IOleInPlaceObjectWindowless IOleInPlaceObjectWindowless_iface;
    IServiceProvider            IServiceProvider_iface;
    IOleCommandTarget           IOleCommandTarget_iface;
    IOleControl                 IOleControl_iface;
    IHlinkTarget                IHlinkTarget_iface;
    IPersistStreamInit          IPersistStreamInit_iface;
    IDispatchEx                 IDispatchEx_iface;
    ISupportErrorInfo           ISupportErrorInfo_iface;
    IObjectWithSite             IObjectWithSite_iface;
    IOleContainer               IOleContainer_iface;
    IObjectSafety               IObjectSafety_iface;
    IProvideClassInfo           IProvideClassInfo_iface;

    IUnknown *unk_impl;
    IDispatchEx *dispex;

    HTMLDocumentObj *doc_obj;
    HTMLDocumentNode *doc_node;

    HTMLOuterWindow *window;

    LONG task_magic;

    ConnectionPointContainer cp_container;
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
    ICustomDoc ICustomDoc_iface;
    ITargetContainer ITargetContainer_iface;

    IWindowForBindingUI IWindowForBindingUI_iface;

    LONG ref;

    NSContainer *nscontainer;

    IOleClientSite *client;
    IDocHostUIHandler *hostui;
    IOleCommandTarget *client_cmdtrg;
    BOOL custom_hostui;
    IOleInPlaceSite *ipsite;
    IOleInPlaceFrame *frame;
    IOleInPlaceUIWindow *ip_window;
    IAdviseSink *view_sink;
    IDocObjectService *doc_object_service;
    IUnknown *webbrowser;
    ITravelLog *travel_log;
    IUnknown *browser_service;

    DOCHOSTUIINFO hostinfo;

    IOleUndoManager *undomgr;
    IHTMLEditServices *editsvcs;

    HWND hwnd;
    HWND tooltips_hwnd;

    BOOL request_uiactivate;
    BOOL in_place_active;
    BOOL ui_active;
    BOOL window_active;
    BOOL hostui_setup;
    BOOL container_locked;
    BOOL focus;
    BOOL has_popup;
    INT download_state;

    USERMODE usermode;
    LPWSTR mime;

    DWORD update;
};

typedef struct nsWeakReference nsWeakReference;

struct NSContainer {
    nsIWebBrowserChrome      nsIWebBrowserChrome_iface;
    nsIContextMenuListener   nsIContextMenuListener_iface;
    nsIURIContentListener    nsIURIContentListener_iface;
    nsIEmbeddingSiteWindow   nsIEmbeddingSiteWindow_iface;
    nsITooltipListener       nsITooltipListener_iface;
    nsIInterfaceRequestor    nsIInterfaceRequestor_iface;
    nsISupportsWeakReference nsISupportsWeakReference_iface;

    nsIWebBrowser *webbrowser;
    nsIWebNavigation *navigation;
    nsIBaseWindow *window;
    nsIWebBrowserFocus *focus;

    nsIEditor *editor;
    nsIController *editor_controller;

    LONG ref;

    nsWeakReference *weak_reference;

    NSContainer *parent;
    HTMLDocumentObj *doc;

    nsIURIContentListener *content_listener;

    HWND hwnd;
};

typedef struct {
    HRESULT (*qi)(HTMLDOMNode*,REFIID,void**);
    void (*destructor)(HTMLDOMNode*);
    const cpc_entry_t *cpc_entries;
    HRESULT (*clone)(HTMLDOMNode*,nsIDOMNode*,HTMLDOMNode**);
    HRESULT (*handle_event)(HTMLDOMNode*,DWORD,nsIDOMEvent*,BOOL*);
    HRESULT (*get_attr_col)(HTMLDOMNode*,HTMLAttributeCollection**);
    event_target_t **(*get_event_target_ptr)(HTMLDOMNode*);
    HRESULT (*fire_event)(HTMLDOMNode*,DWORD,BOOL*);
    HRESULT (*put_disabled)(HTMLDOMNode*,VARIANT_BOOL);
    HRESULT (*get_disabled)(HTMLDOMNode*,VARIANT_BOOL*);
    HRESULT (*get_document)(HTMLDOMNode*,IDispatch**);
    HRESULT (*get_readystate)(HTMLDOMNode*,BSTR*);
    HRESULT (*get_dispid)(HTMLDOMNode*,BSTR,DWORD,DISPID*);
    HRESULT (*invoke)(HTMLDOMNode*,DISPID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,IServiceProvider*);
    HRESULT (*bind_to_tree)(HTMLDOMNode*);
    void (*traverse)(HTMLDOMNode*,nsCycleCollectionTraversalCallback*);
    void (*unlink)(HTMLDOMNode*);
    BOOL (*is_text_edit)(HTMLDOMNode*);
} NodeImplVtbl;

struct HTMLDOMNode {
    EventTarget   event_target;
    IHTMLDOMNode  IHTMLDOMNode_iface;
    IHTMLDOMNode2 IHTMLDOMNode2_iface;
    const NodeImplVtbl *vtbl;

    nsCycleCollectingAutoRefCnt ccref;

    nsIDOMNode *nsnode;
    HTMLDocumentNode *doc;
    ConnectionPointContainer *cp_container;
};

static inline void node_addref(HTMLDOMNode *node)
{
    IHTMLDOMNode_AddRef(&node->IHTMLDOMNode_iface);
}

static inline void node_release(HTMLDOMNode *node)
{
    IHTMLDOMNode_Release(&node->IHTMLDOMNode_iface);
}

typedef struct {
    HTMLDOMNode node;
    ConnectionPointContainer cp_container;

    IHTMLElement  IHTMLElement_iface;
    IHTMLElement2 IHTMLElement2_iface;
    IHTMLElement3 IHTMLElement3_iface;
    IHTMLElement4 IHTMLElement4_iface;

    nsIDOMHTMLElement *nselem;
    HTMLStyle *style;
    HTMLStyle *runtime_style;
    HTMLAttributeCollection *attrs;
    WCHAR *filter;
} HTMLElement;

#define HTMLELEMENT_TIDS    \
    IHTMLDOMNode_tid,       \
    IHTMLDOMNode2_tid,      \
    IHTMLElement_tid,       \
    IHTMLElement2_tid,      \
    IHTMLElement3_tid,      \
    IHTMLElement4_tid

extern cp_static_data_t HTMLElementEvents2_data DECLSPEC_HIDDEN;
#define HTMLELEMENT_CPC {&DIID_HTMLElementEvents2, &HTMLElementEvents2_data}
extern const cpc_entry_t HTMLElement_cpc[] DECLSPEC_HIDDEN;

typedef struct {
    HTMLElement element;

    IHTMLTextContainer IHTMLTextContainer_iface;
} HTMLTextContainer;

struct HTMLFrameBase {
    HTMLElement element;

    IHTMLFrameBase  IHTMLFrameBase_iface;
    IHTMLFrameBase2 IHTMLFrameBase2_iface;

    HTMLOuterWindow *content_window;

    nsIDOMHTMLFrameElement *nsframe;
    nsIDOMHTMLIFrameElement *nsiframe;
};

typedef struct nsDocumentEventListener nsDocumentEventListener;

struct HTMLDocumentNode {
    HTMLDOMNode node;
    HTMLDocument basedoc;

    IInternetHostSecurityManager IInternetHostSecurityManager_iface;

    nsIDocumentObserver          nsIDocumentObserver_iface;

    LONG ref;

    HTMLInnerWindow *window;

    nsIDOMHTMLDocument *nsdoc;
    BOOL content_ready;
    event_target_t *body_event_target;

    IHTMLDOMImplementation *dom_implementation;

    ICatInformation *catmgr;
    nsDocumentEventListener *nsevent_listener;
    BOOL *event_vector;

    WCHAR **elem_vars;
    unsigned elem_vars_size;
    unsigned elem_vars_cnt;

    BOOL skip_mutation_notif;

    UINT charset;

    struct list selection_list;
    struct list range_list;
    struct list plugin_hosts;
};

HRESULT HTMLDocument_Create(IUnknown*,REFIID,void**) DECLSPEC_HIDDEN;
HRESULT HTMLLoadOptions_Create(IUnknown*,REFIID,void**) DECLSPEC_HIDDEN;
HRESULT create_doc_from_nsdoc(nsIDOMHTMLDocument*,HTMLDocumentObj*,HTMLInnerWindow*,HTMLDocumentNode**) DECLSPEC_HIDDEN;

HRESULT HTMLOuterWindow_Create(HTMLDocumentObj*,nsIDOMWindow*,HTMLOuterWindow*,HTMLOuterWindow**) DECLSPEC_HIDDEN;
HRESULT update_window_doc(HTMLInnerWindow*) DECLSPEC_HIDDEN;
HTMLOuterWindow *nswindow_to_window(const nsIDOMWindow*) DECLSPEC_HIDDEN;
void get_top_window(HTMLOuterWindow*,HTMLOuterWindow**) DECLSPEC_HIDDEN;
HRESULT HTMLOptionElementFactory_Create(HTMLInnerWindow*,HTMLOptionElementFactory**) DECLSPEC_HIDDEN;
HRESULT HTMLImageElementFactory_Create(HTMLInnerWindow*,HTMLImageElementFactory**) DECLSPEC_HIDDEN;
HRESULT HTMLXMLHttpRequestFactory_Create(HTMLInnerWindow*,HTMLXMLHttpRequestFactory**) DECLSPEC_HIDDEN;
HRESULT HTMLLocation_Create(HTMLInnerWindow*,HTMLLocation**) DECLSPEC_HIDDEN;
IOmNavigator *OmNavigator_Create(void) DECLSPEC_HIDDEN;
HRESULT HTMLScreen_Create(IHTMLScreen**) DECLSPEC_HIDDEN;
HRESULT create_history(HTMLInnerWindow*,OmHistory**) DECLSPEC_HIDDEN;
HRESULT create_dom_implementation(IHTMLDOMImplementation**) DECLSPEC_HIDDEN;

HRESULT create_storage(IHTMLStorage**) DECLSPEC_HIDDEN;

void HTMLDocument_Persist_Init(HTMLDocument*) DECLSPEC_HIDDEN;
void HTMLDocument_OleCmd_Init(HTMLDocument*) DECLSPEC_HIDDEN;
void HTMLDocument_OleObj_Init(HTMLDocument*) DECLSPEC_HIDDEN;
void HTMLDocument_View_Init(HTMLDocument*) DECLSPEC_HIDDEN;
void HTMLDocument_Window_Init(HTMLDocument*) DECLSPEC_HIDDEN;
void HTMLDocument_Service_Init(HTMLDocument*) DECLSPEC_HIDDEN;
void HTMLDocument_Hlink_Init(HTMLDocument*) DECLSPEC_HIDDEN;

void TargetContainer_Init(HTMLDocumentObj*) DECLSPEC_HIDDEN;
void init_binding_ui(HTMLDocumentObj*) DECLSPEC_HIDDEN;

void HTMLDocumentNode_SecMgr_Init(HTMLDocumentNode*) DECLSPEC_HIDDEN;

HRESULT HTMLCurrentStyle_Create(HTMLElement*,IHTMLCurrentStyle**) DECLSPEC_HIDDEN;

void ConnectionPointContainer_Init(ConnectionPointContainer*,IUnknown*,const cpc_entry_t*) DECLSPEC_HIDDEN;
void ConnectionPointContainer_Destroy(ConnectionPointContainer*) DECLSPEC_HIDDEN;

HRESULT create_nscontainer(HTMLDocumentObj*,NSContainer**) DECLSPEC_HIDDEN;
void NSContainer_Release(NSContainer*) DECLSPEC_HIDDEN;

void init_mutation(nsIComponentManager*) DECLSPEC_HIDDEN;
void init_document_mutation(HTMLDocumentNode*) DECLSPEC_HIDDEN;
void release_document_mutation(HTMLDocumentNode*) DECLSPEC_HIDDEN;
JSContext *get_context_from_document(nsIDOMHTMLDocument*) DECLSPEC_HIDDEN;

void HTMLDocument_LockContainer(HTMLDocumentObj*,BOOL) DECLSPEC_HIDDEN;
void show_context_menu(HTMLDocumentObj*,DWORD,POINT*,IDispatch*) DECLSPEC_HIDDEN;
void notif_focus(HTMLDocumentObj*) DECLSPEC_HIDDEN;

void show_tooltip(HTMLDocumentObj*,DWORD,DWORD,LPCWSTR) DECLSPEC_HIDDEN;
void hide_tooltip(HTMLDocumentObj*) DECLSPEC_HIDDEN;
HRESULT get_client_disp_property(IOleClientSite*,DISPID,VARIANT*) DECLSPEC_HIDDEN;

UINT get_document_charset(HTMLDocumentNode*) DECLSPEC_HIDDEN;

HRESULT ProtocolFactory_Create(REFCLSID,REFIID,void**) DECLSPEC_HIDDEN;

BOOL load_gecko(void) DECLSPEC_HIDDEN;
void close_gecko(void) DECLSPEC_HIDDEN;
void register_nsservice(nsIComponentRegistrar*,nsIServiceManager*) DECLSPEC_HIDDEN;
void init_nsio(nsIComponentManager*,nsIComponentRegistrar*) DECLSPEC_HIDDEN;
void release_nsio(void) DECLSPEC_HIDDEN;
BOOL is_gecko_path(const char*) DECLSPEC_HIDDEN;
void set_viewer_zoom(NSContainer*,float) DECLSPEC_HIDDEN;

void init_node_cc(void) DECLSPEC_HIDDEN;

HRESULT nsuri_to_url(LPCWSTR,BOOL,BSTR*) DECLSPEC_HIDDEN;

HRESULT set_frame_doc(HTMLFrameBase*,nsIDOMDocument*) DECLSPEC_HIDDEN;

void call_property_onchanged(ConnectionPointContainer*,DISPID) DECLSPEC_HIDDEN;
HRESULT call_set_active_object(IOleInPlaceUIWindow*,IOleInPlaceActiveObject*) DECLSPEC_HIDDEN;

void *nsalloc(size_t) __WINE_ALLOC_SIZE(1) DECLSPEC_HIDDEN;
void nsfree(void*) DECLSPEC_HIDDEN;

BOOL nsACString_Init(nsACString *str, const char *data) DECLSPEC_HIDDEN;
void nsACString_InitDepend(nsACString*,const char*) DECLSPEC_HIDDEN;
void nsACString_SetData(nsACString*,const char*) DECLSPEC_HIDDEN;
UINT32 nsACString_GetData(const nsACString*,const char**) DECLSPEC_HIDDEN;
void nsACString_Finish(nsACString*) DECLSPEC_HIDDEN;

BOOL nsAString_Init(nsAString*,const PRUnichar*) DECLSPEC_HIDDEN;
void nsAString_InitDepend(nsAString*,const PRUnichar*) DECLSPEC_HIDDEN;
UINT32 nsAString_GetData(const nsAString*,const PRUnichar**) DECLSPEC_HIDDEN;
void nsAString_Finish(nsAString*) DECLSPEC_HIDDEN;

HRESULT return_nsstr(nsresult,nsAString*,BSTR*) DECLSPEC_HIDDEN;

static inline HRESULT return_nsstr_variant(nsresult nsres, nsAString *nsstr, VARIANT *p)
{
    V_VT(p) = VT_BSTR;
    return return_nsstr(nsres, nsstr, &V_BSTR(p));
}

nsICommandParams *create_nscommand_params(void) DECLSPEC_HIDDEN;
HRESULT nsnode_to_nsstring(nsIDOMNode*,nsAString*) DECLSPEC_HIDDEN;
void get_editor_controller(NSContainer*) DECLSPEC_HIDDEN;
nsresult get_nsinterface(nsISupports*,REFIID,void**) DECLSPEC_HIDDEN;
nsIWritableVariant *create_nsvariant(void) DECLSPEC_HIDDEN;
nsIXMLHttpRequest *create_nsxhr(nsIDOMWindow *nswindow) DECLSPEC_HIDDEN;
nsresult create_nsfile(const PRUnichar*,nsIFile**) DECLSPEC_HIDDEN;
char *get_nscategory_entry(const char*,const char*) DECLSPEC_HIDDEN;

HRESULT create_pending_window(HTMLOuterWindow*,nsChannelBSC*) DECLSPEC_HIDDEN;
HRESULT start_binding(HTMLInnerWindow*,BSCallback*,IBindCtx*) DECLSPEC_HIDDEN;
HRESULT async_start_doc_binding(HTMLOuterWindow*,HTMLInnerWindow*) DECLSPEC_HIDDEN;
void abort_window_bindings(HTMLInnerWindow*) DECLSPEC_HIDDEN;
void set_download_state(HTMLDocumentObj*,int) DECLSPEC_HIDDEN;
void call_docview_84(HTMLDocumentObj*) DECLSPEC_HIDDEN;

void set_ready_state(HTMLOuterWindow*,READYSTATE) DECLSPEC_HIDDEN;
HRESULT get_readystate_string(READYSTATE,BSTR*) DECLSPEC_HIDDEN;

HRESULT HTMLSelectionObject_Create(HTMLDocumentNode*,nsISelection*,IHTMLSelectionObject**) DECLSPEC_HIDDEN;
HRESULT HTMLTxtRange_Create(HTMLDocumentNode*,nsIDOMRange*,IHTMLTxtRange**) DECLSPEC_HIDDEN;
IHTMLStyleSheet *HTMLStyleSheet_Create(nsIDOMStyleSheet*) DECLSPEC_HIDDEN;
IHTMLStyleSheetsCollection *HTMLStyleSheetsCollection_Create(nsIDOMStyleSheetList*) DECLSPEC_HIDDEN;

void detach_selection(HTMLDocumentNode*) DECLSPEC_HIDDEN;
void detach_ranges(HTMLDocumentNode*) DECLSPEC_HIDDEN;
HRESULT get_node_text(HTMLDOMNode*,BSTR*) DECLSPEC_HIDDEN;
HRESULT replace_node_by_html(nsIDOMHTMLDocument*,nsIDOMNode*,const WCHAR*) DECLSPEC_HIDDEN;

HRESULT create_nselem(HTMLDocumentNode*,const WCHAR*,nsIDOMHTMLElement**) DECLSPEC_HIDDEN;
HRESULT create_element(HTMLDocumentNode*,const WCHAR*,HTMLElement**) DECLSPEC_HIDDEN;

HRESULT HTMLDOMTextNode_Create(HTMLDocumentNode*,nsIDOMNode*,HTMLDOMNode**) DECLSPEC_HIDDEN;

BOOL variant_to_nscolor(const VARIANT *v, nsAString *nsstr) DECLSPEC_HIDDEN;
HRESULT nscolor_to_str(LPCWSTR color, BSTR *ret) DECLSPEC_HIDDEN;


struct HTMLAttributeCollection {
    DispatchEx dispex;
    IHTMLAttributeCollection IHTMLAttributeCollection_iface;
    IHTMLAttributeCollection2 IHTMLAttributeCollection2_iface;
    IHTMLAttributeCollection3 IHTMLAttributeCollection3_iface;

    LONG ref;

    HTMLElement *elem;
    struct list attrs;
};

typedef struct {
    DispatchEx dispex;
    IHTMLDOMAttribute IHTMLDOMAttribute_iface;
    IHTMLDOMAttribute2 IHTMLDOMAttribute2_iface;

    LONG ref;

    WCHAR *name;

    HTMLElement *elem;
    DISPID dispid;
    struct list entry;
} HTMLDOMAttribute;

HRESULT HTMLDOMAttribute_Create(const WCHAR*,HTMLElement*,DISPID,HTMLDOMAttribute**) DECLSPEC_HIDDEN;

HRESULT HTMLElement_Create(HTMLDocumentNode*,nsIDOMNode*,BOOL,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLCommentElement_Create(HTMLDocumentNode*,nsIDOMNode*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLAnchorElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLAreaElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLBodyElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLButtonElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLEmbedElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLFormElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLFrameElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLHeadElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLIFrame_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLStyleElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLImgElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLInputElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLLabelElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLLinkElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLMetaElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLObjectElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLOptionElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLScriptElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLSelectElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLTable_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLTableCell_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLTableRow_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLTextAreaElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLTitleElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;
HRESULT HTMLGenericElement_Create(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**) DECLSPEC_HIDDEN;

void HTMLDOMNode_Init(HTMLDocumentNode*,HTMLDOMNode*,nsIDOMNode*) DECLSPEC_HIDDEN;
void HTMLElement_Init(HTMLElement*,HTMLDocumentNode*,nsIDOMHTMLElement*,dispex_static_data_t*) DECLSPEC_HIDDEN;
void HTMLTextContainer_Init(HTMLTextContainer*,HTMLDocumentNode*,nsIDOMHTMLElement*,dispex_static_data_t*) DECLSPEC_HIDDEN;
void HTMLFrameBase_Init(HTMLFrameBase*,HTMLDocumentNode*,nsIDOMHTMLElement*,dispex_static_data_t*) DECLSPEC_HIDDEN;

HRESULT HTMLDOMNode_QI(HTMLDOMNode*,REFIID,void**) DECLSPEC_HIDDEN;
void HTMLDOMNode_destructor(HTMLDOMNode*) DECLSPEC_HIDDEN;

HRESULT HTMLElement_QI(HTMLDOMNode*,REFIID,void**) DECLSPEC_HIDDEN;
void HTMLElement_destructor(HTMLDOMNode*) DECLSPEC_HIDDEN;
HRESULT HTMLElement_clone(HTMLDOMNode*,nsIDOMNode*,HTMLDOMNode**) DECLSPEC_HIDDEN;
HRESULT HTMLElement_get_attr_col(HTMLDOMNode*,HTMLAttributeCollection**) DECLSPEC_HIDDEN;
HRESULT HTMLElement_handle_event(HTMLDOMNode*,DWORD,nsIDOMEvent*,BOOL*) DECLSPEC_HIDDEN;

HRESULT HTMLFrameBase_QI(HTMLFrameBase*,REFIID,void**) DECLSPEC_HIDDEN;
void HTMLFrameBase_destructor(HTMLFrameBase*) DECLSPEC_HIDDEN;

HRESULT get_node(HTMLDocumentNode*,nsIDOMNode*,BOOL,HTMLDOMNode**) DECLSPEC_HIDDEN;
HRESULT get_elem(HTMLDocumentNode*,nsIDOMElement*,HTMLElement**) DECLSPEC_HIDDEN;

HTMLElement *unsafe_impl_from_IHTMLElement(IHTMLElement*) DECLSPEC_HIDDEN;

HRESULT search_window_props(HTMLInnerWindow*,BSTR,DWORD,DISPID*) DECLSPEC_HIDDEN;
HRESULT get_frame_by_name(HTMLOuterWindow*,const WCHAR*,BOOL,HTMLOuterWindow**) DECLSPEC_HIDDEN;
HRESULT get_doc_elem_by_id(HTMLDocumentNode*,const WCHAR*,HTMLElement**) DECLSPEC_HIDDEN;
HTMLOuterWindow *get_target_window(HTMLOuterWindow*,nsAString*,BOOL*) DECLSPEC_HIDDEN;
HRESULT handle_link_click_event(HTMLElement*,nsAString*,nsAString*,nsIDOMEvent*,BOOL*) DECLSPEC_HIDDEN;

HRESULT wrap_iface(IUnknown*,IUnknown*,IUnknown**) DECLSPEC_HIDDEN;

IHTMLElementCollection *create_all_collection(HTMLDOMNode*,BOOL) DECLSPEC_HIDDEN;
IHTMLElementCollection *create_collection_from_nodelist(HTMLDocumentNode*,nsIDOMNodeList*) DECLSPEC_HIDDEN;
IHTMLElementCollection *create_collection_from_htmlcol(HTMLDocumentNode*,nsIDOMHTMLCollection*) DECLSPEC_HIDDEN;

#define ATTRFLAG_CASESENSITIVE  0x0001
#define ATTRFLAG_ASSTRING       0x0002
#define ATTRFLAG_EXPANDURL      0x0004

HRESULT get_elem_attr_value_by_dispid(HTMLElement*,DISPID,DWORD,VARIANT*) DECLSPEC_HIDDEN;
HRESULT get_elem_source_index(HTMLElement*,LONG*) DECLSPEC_HIDDEN;

nsresult get_elem_attr_value(nsIDOMHTMLElement*,const WCHAR*,nsAString*,const PRUnichar**) DECLSPEC_HIDDEN;
HRESULT elem_string_attr_getter(HTMLElement*,const WCHAR*,BOOL,BSTR*) DECLSPEC_HIDDEN;
HRESULT elem_string_attr_setter(HTMLElement*,const WCHAR*,const WCHAR*) DECLSPEC_HIDDEN;

/* commands */
typedef struct {
    DWORD id;
    HRESULT (*query)(HTMLDocument*,OLECMD*);
    HRESULT (*exec)(HTMLDocument*,DWORD,VARIANT*,VARIANT*);
} cmdtable_t;

extern const cmdtable_t editmode_cmds[] DECLSPEC_HIDDEN;

void do_ns_command(HTMLDocument*,const char*,nsICommandParams*) DECLSPEC_HIDDEN;

/* timer */
#define UPDATE_UI       0x0001
#define UPDATE_TITLE    0x0002

void update_doc(HTMLDocument*,DWORD) DECLSPEC_HIDDEN;
void update_title(HTMLDocumentObj*) DECLSPEC_HIDDEN;
void set_document_navigation(HTMLDocumentObj*,BOOL) DECLSPEC_HIDDEN;

HRESULT do_query_service(IUnknown*,REFGUID,REFIID,void**) DECLSPEC_HIDDEN;

/* editor */
HRESULT setup_edit_mode(HTMLDocumentObj*) DECLSPEC_HIDDEN;
void init_editor(HTMLDocument*) DECLSPEC_HIDDEN;
void handle_edit_event(HTMLDocument*,nsIDOMEvent*) DECLSPEC_HIDDEN;
HRESULT editor_exec_copy(HTMLDocument*,DWORD,VARIANT*,VARIANT*) DECLSPEC_HIDDEN;
HRESULT editor_exec_cut(HTMLDocument*,DWORD,VARIANT*,VARIANT*) DECLSPEC_HIDDEN;
HRESULT editor_exec_paste(HTMLDocument*,DWORD,VARIANT*,VARIANT*) DECLSPEC_HIDDEN;
void handle_edit_load(HTMLDocument*) DECLSPEC_HIDDEN;
HRESULT editor_is_dirty(HTMLDocument*) DECLSPEC_HIDDEN;
void set_dirty(HTMLDocument*,VARIANT_BOOL) DECLSPEC_HIDDEN;

extern DWORD mshtml_tls DECLSPEC_HIDDEN;

typedef struct task_t task_t;
typedef void (*task_proc_t)(task_t*);

struct task_t {
    LONG target_magic;
    task_proc_t proc;
    task_proc_t destr;
    struct list entry;
};

typedef struct {
    task_t header;
    HTMLDocumentObj *doc;
} docobj_task_t;

typedef struct {
    HWND thread_hwnd;
    struct list task_list;
    struct list timer_list;
} thread_data_t;

thread_data_t *get_thread_data(BOOL) DECLSPEC_HIDDEN;
HWND get_thread_hwnd(void) DECLSPEC_HIDDEN;

LONG get_task_target_magic(void) DECLSPEC_HIDDEN;
HRESULT push_task(task_t*,task_proc_t,task_proc_t,LONG) DECLSPEC_HIDDEN;
void remove_target_tasks(LONG) DECLSPEC_HIDDEN;
void flush_pending_tasks(LONG) DECLSPEC_HIDDEN;

HRESULT set_task_timer(HTMLInnerWindow*,DWORD,BOOL,IDispatch*,LONG*) DECLSPEC_HIDDEN;
HRESULT clear_task_timer(HTMLInnerWindow*,BOOL,DWORD) DECLSPEC_HIDDEN;

const char *debugstr_mshtml_guid(const GUID*) DECLSPEC_HIDDEN;

DEFINE_GUID(CLSID_AboutProtocol, 0x3050F406, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_JSProtocol, 0x3050F3B2, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_MailtoProtocol, 0x3050F3DA, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_ResProtocol, 0x3050F3BC, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_SysimageProtocol, 0x76E67A63, 0x06E9, 0x11D2, 0xA8,0x40, 0x00,0x60,0x08,0x05,0x93,0x82);

DEFINE_GUID(CLSID_CMarkup,0x3050f4fb,0x98b5,0x11cf,0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b);

DEFINE_OLEGUID(CGID_DocHostCmdPriv, 0x000214D4L, 0, 0);

DEFINE_GUID(CLSID_JScript, 0xf414c260,0x6ac0,0x11cf, 0xb6,0xd1,0x00,0xaa,0x00,0xbb,0xbb,0x58);
DEFINE_GUID(CLSID_VBScript, 0xb54f3741,0x5b07,0x11cf, 0xa4,0xb0,0x00,0xaa,0x00,0x4a,0x55,0xe8);

DEFINE_GUID(IID_UndocumentedScriptIface,0x719c3050,0xf9d3,0x11cf,0xa4,0x93,0x00,0x40,0x05,0x23,0xa8,0xa0);
DEFINE_GUID(IID_IDispatchJS,0x719c3050,0xf9d3,0x11cf,0xa4,0x93,0x00,0x40,0x05,0x23,0xa8,0xa6);

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
        if(ret)
            memcpy(ret, str, size);
    }

    return ret;
}

static inline LPWSTR heap_strndupW(LPCWSTR str, unsigned len)
{
    LPWSTR ret = NULL;

    if(str) {
        ret = heap_alloc((len+1)*sizeof(WCHAR));
        if(ret)
        {
            memcpy(ret, str, len*sizeof(WCHAR));
            ret[len] = 0;
        }
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
        if(ret)
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
        if(ret)
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
        if(ret)
            WideCharToMultiByte(CP_ACP, 0, str, -1, ret, size, NULL, NULL);
    }

    return ret;
}

static inline WCHAR *heap_strdupUtoW(const char *str)
{
    WCHAR *ret = NULL;

    if(str) {
        size_t len;

        len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
        ret = heap_alloc(len*sizeof(WCHAR));
        if(ret)
            MultiByteToWideChar(CP_UTF8, 0, str, -1, ret, len);
    }

    return ret;
}

static inline char *heap_strdupWtoU(const WCHAR *str)
{
    char *ret = NULL;

    if(str) {
        size_t size = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
        ret = heap_alloc(size);
        if(ret)
            WideCharToMultiByte(CP_UTF8, 0, str, -1, ret, size, NULL, NULL);
    }

    return ret;
}

static inline char *heap_strndupWtoU(LPCWSTR str, unsigned len)
{
    char *ret = NULL;
    DWORD size;

    if(str && len) {
        size = WideCharToMultiByte(CP_UTF8, 0, str, len, NULL, 0, NULL, NULL);
        ret = heap_alloc(size + 1);
        if(ret) {
            WideCharToMultiByte(CP_UTF8, 0, str, len, ret, size, NULL, NULL);
            ret[size] = '\0';
        }
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

UINT cp_from_charset_string(BSTR) DECLSPEC_HIDDEN;
HDC get_display_dc(void) DECLSPEC_HIDDEN;
HINSTANCE get_shdoclc(void) DECLSPEC_HIDDEN;
void set_statustext(HTMLDocumentObj*,INT,LPCWSTR) DECLSPEC_HIDDEN;

extern HINSTANCE hInst DECLSPEC_HIDDEN;

#include "binding.h"
#include "htmlevent.h"
#include "htmlscript.h"
#include "htmlstyle.h"
#include "pluginhost.h"
#include "resource.h"

#endif /* _MSHTML_PRIVATE_H_ */
