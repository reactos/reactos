/*
 * Copyright 2005-2006 Jacek Caban for CodeWeavers
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
#include "mshtml.h"
#include "mshtmhst.h"
#include "hlink.h"

#include "wine/list.h"
#include "wine/unicode.h"

#ifdef INIT_GUID
#include "initguid.h"
#endif

#include "nsiface.h"

#define GENERATE_MSHTML_NS_FAILURE(code) \
    ((nsresult) ((PRUint32)(1<<31) | ((PRUint32)(0x45+6)<<16) | (PRUint32)(code)))

#define NS_OK                     ((nsresult)0x00000000L)
#define NS_ERROR_FAILURE          ((nsresult)0x80004005L)
#define NS_NOINTERFACE            ((nsresult)0x80004002L)
#define NS_ERROR_NOT_IMPLEMENTED  ((nsresult)0x80004001L)
#define NS_ERROR_INVALID_ARG      ((nsresult)0x80070057L) 
#define NS_ERROR_UNEXPECTED       ((nsresult)0x8000ffffL)
#define NS_ERROR_UNKNOWN_PROTOCOL ((nsresult)0x804b0012L)

#define WINE_NS_LOAD_FROM_MONIKER GENERATE_MSHTML_NS_FAILURE(0)

#define NS_FAILED(res) ((res) & 0x80000000)
#define NS_SUCCEEDED(res) (!NS_FAILED(res))

#define NSAPI WINAPI

#define MSHTML_E_NODOC    0x800a025c

typedef struct HTMLDOMNode HTMLDOMNode;
typedef struct ConnectionPoint ConnectionPoint;
typedef struct BSCallback BSCallback;
typedef struct nsChannelBSC nsChannelBSC;

typedef struct {
    const IHTMLWindow2Vtbl *lpHTMLWindow2Vtbl;

    LONG ref;

    HTMLDocument *doc;
    nsIDOMWindow *nswindow;

    struct list entry;
} HTMLWindow;

typedef enum {
    UNKNOWN_USERMODE,
    BROWSEMODE,
    EDITMODE        
} USERMODE;

typedef struct {
    const IConnectionPointContainerVtbl  *lpConnectionPointContainerVtbl;

    ConnectionPoint *cp_list;
    IUnknown *outer;
} ConnectionPointContainer;

struct ConnectionPoint {
    const IConnectionPointVtbl *lpConnectionPointVtbl;

    IConnectionPointContainer *container;

    union {
        IUnknown *unk;
        IDispatch *disp;
        IPropertyNotifySink *propnotif;
    } *sinks;
    DWORD sinks_size;

    const IID *iid;

    ConnectionPoint *next;
};

typedef struct {
    const IHTMLLocationVtbl *lpHTMLLocationVtbl;

    LONG ref;

    HTMLDocument *doc;
} HTMLLocation;

typedef struct {
    const IHTMLOptionElementFactoryVtbl *lpHTMLOptionElementFactoryVtbl;

    LONG ref;

    HTMLDocument *doc;
} HTMLOptionElementFactory;

struct HTMLDocument {
    const IHTMLDocument2Vtbl              *lpHTMLDocument2Vtbl;
    const IHTMLDocument3Vtbl              *lpHTMLDocument3Vtbl;
    const IHTMLDocument4Vtbl              *lpHTMLDocument4Vtbl;
    const IHTMLDocument5Vtbl              *lpHTMLDocument5Vtbl;
    const IPersistMonikerVtbl             *lpPersistMonikerVtbl;
    const IPersistFileVtbl                *lpPersistFileVtbl;
    const IMonikerPropVtbl                *lpMonikerPropVtbl;
    const IOleObjectVtbl                  *lpOleObjectVtbl;
    const IOleDocumentVtbl                *lpOleDocumentVtbl;
    const IOleDocumentViewVtbl            *lpOleDocumentViewVtbl;
    const IOleInPlaceActiveObjectVtbl     *lpOleInPlaceActiveObjectVtbl;
    const IViewObject2Vtbl                *lpViewObject2Vtbl;
    const IOleInPlaceObjectWindowlessVtbl *lpOleInPlaceObjectWindowlessVtbl;
    const IServiceProviderVtbl            *lpServiceProviderVtbl;
    const IOleCommandTargetVtbl           *lpOleCommandTargetVtbl;
    const IOleControlVtbl                 *lpOleControlVtbl;
    const IHlinkTargetVtbl                *lpHlinkTargetVtbl;
    const IPersistStreamInitVtbl          *lpPersistStreamInitVtbl;
    const ICustomDocVtbl                  *lpCustomDocVtbl;

    LONG ref;

    NSContainer *nscontainer;
    HTMLWindow *window;

    IOleClientSite *client;
    IDocHostUIHandler *hostui;
    IOleInPlaceSite *ipsite;
    IOleInPlaceFrame *frame;
    IOleInPlaceUIWindow *ip_window;

    IOleUndoManager *undomgr;

    nsChannelBSC *bscallback;
    IMoniker *mon;
    LPOLESTR url;
    struct list bindings;

    struct list script_hosts;

    HWND hwnd;
    HWND tooltips_hwnd;

    DOCHOSTUIINFO hostinfo;

    USERMODE usermode;
    READYSTATE readystate;
    BOOL in_place_active;
    BOOL ui_active;
    BOOL window_active;
    BOOL has_key_path;
    BOOL container_locked;
    BOOL focus;
    LPWSTR mime;

    DWORD update;

    ConnectionPointContainer cp_container;
    ConnectionPoint cp_htmldocevents;
    ConnectionPoint cp_htmldocevents2;
    ConnectionPoint cp_propnotif;

    HTMLOptionElementFactory *option_factory;
    HTMLLocation *location;

    struct list selection_list;
    struct list range_list;

    HTMLDOMNode *nodes;
};

typedef struct {
    const nsIDOMEventListenerVtbl      *lpDOMEventListenerVtbl;
    NSContainer *This;
} nsEventListener;

struct NSContainer {
    const nsIWebBrowserChromeVtbl       *lpWebBrowserChromeVtbl;
    const nsIContextMenuListenerVtbl    *lpContextMenuListenerVtbl;
    const nsIURIContentListenerVtbl     *lpURIContentListenerVtbl;
    const nsIEmbeddingSiteWindowVtbl    *lpEmbeddingSiteWindowVtbl;
    const nsITooltipListenerVtbl        *lpTooltipListenerVtbl;
    const nsIInterfaceRequestorVtbl     *lpInterfaceRequestorVtbl;
    const nsIWeakReferenceVtbl          *lpWeakReferenceVtbl;
    const nsISupportsWeakReferenceVtbl  *lpSupportsWeakReferenceVtbl;

    nsEventListener blur_listener;
    nsEventListener focus_listener;
    nsEventListener keypress_listener;
    nsEventListener load_listener;
    nsEventListener node_insert_listener;

    nsIWebBrowser *webbrowser;
    nsIWebNavigation *navigation;
    nsIBaseWindow *window;
    nsIWebBrowserFocus *focus;

    nsIEditor *editor;
    nsIController *editor_controller;

    LONG ref;

    NSContainer *parent;
    HTMLDocument *doc;

    nsIURIContentListener *content_listener;

    HWND hwnd;

    nsChannelBSC *bscallback; /* hack */
    HWND reset_focus; /* hack */
};

typedef struct {
    const nsIHttpChannelVtbl *lpHttpChannelVtbl;
    const nsIUploadChannelVtbl *lpUploadChannelVtbl;

    LONG ref;

    nsIChannel *channel;
    nsIHttpChannel *http_channel;
    nsIWineURI *uri;
    nsIInputStream *post_data_stream;
    nsILoadGroup *load_group;
    nsIInterfaceRequestor *notif_callback;
    nsLoadFlags load_flags;
    nsIURI *original_uri;
    char *content_type;
    char *charset;
} nsChannel;

typedef struct {
    HRESULT (*qi)(HTMLDOMNode*,REFIID,void**);
    void (*destructor)(HTMLDOMNode*);
} NodeImplVtbl;

struct HTMLDOMNode {
    const IHTMLDOMNodeVtbl *lpHTMLDOMNodeVtbl;
    const NodeImplVtbl *vtbl;

    LONG ref;

    nsIDOMNode *nsnode;
    HTMLDocument *doc;

    HTMLDOMNode *next;
};

typedef struct {
    HTMLDOMNode node;
    ConnectionPointContainer cp_container;

    const IHTMLElementVtbl   *lpHTMLElementVtbl;
    const IHTMLElement2Vtbl  *lpHTMLElement2Vtbl;

    nsIDOMHTMLElement *nselem;
} HTMLElement;

typedef struct {
    HTMLElement element;

    const IHTMLTextContainerVtbl *lpHTMLTextContainerVtbl;

    ConnectionPoint cp;
} HTMLTextContainer;

#define HTMLWINDOW2(x)   ((IHTMLWindow2*)                 &(x)->lpHTMLWindow2Vtbl)

#define HTMLDOC(x)       ((IHTMLDocument2*)               &(x)->lpHTMLDocument2Vtbl)
#define HTMLDOC3(x)      ((IHTMLDocument3*)               &(x)->lpHTMLDocument3Vtbl)
#define HTMLDOC4(x)      ((IHTMLDocument4*)               &(x)->lpHTMLDocument4Vtbl)
#define HTMLDOC5(x)      ((IHTMLDocument5*)               &(x)->lpHTMLDocument5Vtbl)
#define PERSIST(x)       ((IPersist*)                     &(x)->lpPersistFileVtbl)
#define PERSISTMON(x)    ((IPersistMoniker*)              &(x)->lpPersistMonikerVtbl)
#define PERSISTFILE(x)   ((IPersistFile*)                 &(x)->lpPersistFileVtbl)
#define MONPROP(x)       ((IMonikerProp*)                 &(x)->lpMonikerPropVtbl)
#define OLEOBJ(x)        ((IOleObject*)                   &(x)->lpOleObjectVtbl)
#define OLEDOC(x)        ((IOleDocument*)                 &(x)->lpOleDocumentVtbl)
#define DOCVIEW(x)       ((IOleDocumentView*)             &(x)->lpOleDocumentViewVtbl)
#define OLEWIN(x)        ((IOleWindow*)                   &(x)->lpOleInPlaceActiveObjectVtbl)
#define ACTOBJ(x)        ((IOleInPlaceActiveObject*)      &(x)->lpOleInPlaceActiveObjectVtbl)
#define VIEWOBJ(x)       ((IViewObject*)                  &(x)->lpViewObject2Vtbl)
#define VIEWOBJ2(x)      ((IViewObject2*)                 &(x)->lpViewObject2Vtbl)
#define INPLACEOBJ(x)    ((IOleInPlaceObject*)            &(x)->lpOleInPlaceObjectWindowlessVtbl)
#define INPLACEWIN(x)    ((IOleInPlaceObjectWindowless*)  &(x)->lpOleInPlaceObjectWindowlessVtbl)
#define SERVPROV(x)      ((IServiceProvider*)             &(x)->lpServiceProviderVtbl)
#define CMDTARGET(x)     ((IOleCommandTarget*)            &(x)->lpOleCommandTargetVtbl)
#define CONTROL(x)       ((IOleControl*)                  &(x)->lpOleControlVtbl)
#define HLNKTARGET(x)    ((IHlinkTarget*)                 &(x)->lpHlinkTargetVtbl)
#define CONPTCONT(x)     ((IConnectionPointContainer*)    &(x)->lpConnectionPointContainerVtbl)
#define PERSTRINIT(x)    ((IPersistStreamInit*)           &(x)->lpPersistStreamInitVtbl)
#define CUSTOMDOC(x)     ((ICustomDoc*)                   &(x)->lpCustomDocVtbl)

#define NSWBCHROME(x)    ((nsIWebBrowserChrome*)          &(x)->lpWebBrowserChromeVtbl)
#define NSCML(x)         ((nsIContextMenuListener*)       &(x)->lpContextMenuListenerVtbl)
#define NSURICL(x)       ((nsIURIContentListener*)        &(x)->lpURIContentListenerVtbl)
#define NSEMBWNDS(x)     ((nsIEmbeddingSiteWindow*)       &(x)->lpEmbeddingSiteWindowVtbl)
#define NSIFACEREQ(x)    ((nsIInterfaceRequestor*)        &(x)->lpInterfaceRequestorVtbl)
#define NSTOOLTIP(x)     ((nsITooltipListener*)           &(x)->lpTooltipListenerVtbl)
#define NSEVENTLIST(x)   ((nsIDOMEventListener*)          &(x)->lpDOMEventListenerVtbl)
#define NSWEAKREF(x)     ((nsIWeakReference*)             &(x)->lpWeakReferenceVtbl)
#define NSSUPWEAKREF(x)  ((nsISupportsWeakReference*)     &(x)->lpSupportsWeakReferenceVtbl)

#define NSCHANNEL(x)     ((nsIChannel*)        &(x)->lpHttpChannelVtbl)
#define NSHTTPCHANNEL(x) ((nsIHttpChannel*)    &(x)->lpHttpChannelVtbl)
#define NSUPCHANNEL(x)   ((nsIUploadChannel*)  &(x)->lpUploadChannelVtbl)

#define HTTPNEG(x)       ((IHttpNegotiate2*)              &(x)->lpHttpNegotiate2Vtbl)
#define STATUSCLB(x)     ((IBindStatusCallback*)          &(x)->lpBindStatusCallbackVtbl)
#define BINDINFO(x)      ((IInternetBindInfo*)            &(x)->lpInternetBindInfoVtbl);

#define HTMLELEM(x)      ((IHTMLElement*)                 &(x)->lpHTMLElementVtbl)
#define HTMLELEM2(x)     ((IHTMLElement2*)                &(x)->lpHTMLElement2Vtbl)
#define HTMLDOMNODE(x)   ((IHTMLDOMNode*)                 &(x)->lpHTMLDOMNodeVtbl)

#define HTMLTEXTCONT(x)  ((IHTMLTextContainer*)           &(x)->lpHTMLTextContainerVtbl)

#define HTMLOPTFACTORY(x)  ((IHTMLOptionElementFactory*)  &(x)->lpHTMLOptionElementFactoryVtbl)
#define HTMLLOCATION(x)  ((IHTMLLocation*) &(x)->lpHTMLLocationVtbl)

#define DEFINE_THIS2(cls,ifc,iface) ((cls*)((BYTE*)(iface)-offsetof(cls,ifc)))
#define DEFINE_THIS(cls,ifc,iface) DEFINE_THIS2(cls,lp ## ifc ## Vtbl,iface)

HRESULT HTMLDocument_Create(IUnknown*,REFIID,void**);
HRESULT HTMLLoadOptions_Create(IUnknown*,REFIID,void**);

HTMLWindow *HTMLWindow_Create(HTMLDocument*);
HTMLWindow *nswindow_to_window(const nsIDOMWindow*);
HTMLOptionElementFactory *HTMLOptionElementFactory_Create(HTMLDocument*);
HTMLLocation *HTMLLocation_Create(HTMLDocument*);
IOmNavigator *OmNavigator_Create(void);
void setup_nswindow(HTMLWindow*);

void HTMLDocument_HTMLDocument3_Init(HTMLDocument*);
void HTMLDocument_HTMLDocument5_Init(HTMLDocument*);
void HTMLDocument_Persist_Init(HTMLDocument*);
void HTMLDocument_OleCmd_Init(HTMLDocument*);
void HTMLDocument_OleObj_Init(HTMLDocument*);
void HTMLDocument_View_Init(HTMLDocument*);
void HTMLDocument_Window_Init(HTMLDocument*);
void HTMLDocument_Service_Init(HTMLDocument*);
void HTMLDocument_Hlink_Init(HTMLDocument*);

void ConnectionPoint_Init(ConnectionPoint*,ConnectionPointContainer*,REFIID);
void ConnectionPointContainer_Init(ConnectionPointContainer*,IUnknown*);
void ConnectionPointContainer_Destroy(ConnectionPointContainer*);

NSContainer *NSContainer_Create(HTMLDocument*,NSContainer*);
void NSContainer_Release(NSContainer*);

void HTMLDocument_LockContainer(HTMLDocument*,BOOL);
void show_context_menu(HTMLDocument*,DWORD,POINT*,IDispatch*);
void notif_focus(HTMLDocument*);

void show_tooltip(HTMLDocument*,DWORD,DWORD,LPCWSTR);
void hide_tooltip(HTMLDocument*);
HRESULT get_client_disp_property(IOleClientSite*,DISPID,VARIANT*);

HRESULT ProtocolFactory_Create(REFCLSID,REFIID,void**);

BOOL load_gecko(BOOL);
void close_gecko(void);
void register_nsservice(nsIComponentRegistrar*,nsIServiceManager*);
void init_nsio(nsIComponentManager*,nsIComponentRegistrar*);
BOOL install_wine_gecko(BOOL);

void hlink_frame_navigate(HTMLDocument*,IHlinkFrame*,LPCWSTR,nsIInputStream*,DWORD);

void call_property_onchanged(ConnectionPoint*,DISPID);
HRESULT call_set_active_object(IOleInPlaceUIWindow*,IOleInPlaceActiveObject*);

void *nsalloc(size_t);
void nsfree(void*);

void nsACString_Init(nsACString*,const char*);
void nsACString_SetData(nsACString*,const char*);
PRUint32 nsACString_GetData(const nsACString*,const char**);
void nsACString_Finish(nsACString*);

void nsAString_Init(nsAString*,const PRUnichar*);
void nsAString_SetData(nsAString*,const PRUnichar*);
PRUint32 nsAString_GetData(const nsAString*,const PRUnichar**);
void nsAString_Finish(nsAString*);

nsIInputStream *create_nsstream(const char*,PRInt32);
nsICommandParams *create_nscommand_params(void);
nsIMutableArray *create_nsarray(void);
nsIWritableVariant *create_nsvariant(void);
void nsnode_to_nsstring(nsIDOMNode*,nsAString*);
void get_editor_controller(NSContainer*);
void init_nsevents(NSContainer*);
nsresult get_nsinterface(nsISupports*,REFIID,void**);

void set_document_bscallback(HTMLDocument*,nsChannelBSC*);
void set_current_mon(HTMLDocument*,IMoniker*);
HRESULT start_binding(HTMLDocument*,BSCallback*,IBindCtx*);

HRESULT bind_mon_to_buffer(HTMLDocument*,IMoniker*,void**);

nsChannelBSC *create_channelbsc(IMoniker*);
HRESULT channelbsc_load_stream(nsChannelBSC*,IStream*);
void channelbsc_set_channel(nsChannelBSC*,nsChannel*,nsIStreamListener*,nsISupports*);
IMoniker *get_channelbsc_mon(nsChannelBSC*);

IHTMLSelectionObject *HTMLSelectionObject_Create(HTMLDocument*,nsISelection*);
IHTMLTxtRange *HTMLTxtRange_Create(HTMLDocument*,nsIDOMRange*);
IHTMLStyle *HTMLStyle_Create(nsIDOMCSSStyleDeclaration*);
IHTMLStyleSheet *HTMLStyleSheet_Create(nsIDOMStyleSheet*);
IHTMLStyleSheetsCollection *HTMLStyleSheetsCollection_Create(nsIDOMStyleSheetList*);

void detach_selection(HTMLDocument*);
void detach_ranges(HTMLDocument*);

HTMLElement *HTMLElement_Create(nsIDOMNode*);
HTMLElement *HTMLAnchorElement_Create(nsIDOMHTMLElement*);
HTMLElement *HTMLBodyElement_Create(nsIDOMHTMLElement*);
HTMLElement *HTMLInputElement_Create(nsIDOMHTMLElement*);
HTMLElement *HTMLOptionElement_Create(nsIDOMHTMLElement*);
HTMLElement *HTMLScriptElement_Create(nsIDOMHTMLElement*);
HTMLElement *HTMLSelectElement_Create(nsIDOMHTMLElement*);
HTMLElement *HTMLTable_Create(nsIDOMHTMLElement*);
HTMLElement *HTMLTextAreaElement_Create(nsIDOMHTMLElement*);

void HTMLElement_Init(HTMLElement*);
void HTMLElement2_Init(HTMLElement*);
void HTMLTextContainer_Init(HTMLTextContainer*);

HRESULT HTMLDOMNode_QI(HTMLDOMNode*,REFIID,void**);
void HTMLDOMNode_destructor(HTMLDOMNode*);

HRESULT HTMLElement_QI(HTMLDOMNode*,REFIID,void**);
void HTMLElement_destructor(HTMLDOMNode*);

HTMLDOMNode *get_node(HTMLDocument*,nsIDOMNode*);
void release_nodes(HTMLDocument*);

void release_script_hosts(HTMLDocument*);
void connect_scripts(HTMLDocument*);
void doc_insert_script(HTMLDocument*,nsIDOMHTMLScriptElement*);

IHTMLElementCollection *create_all_collection(HTMLDOMNode*);

/* commands */
typedef struct {
    DWORD id;
    HRESULT (*query)(HTMLDocument*,OLECMD*);
    HRESULT (*exec)(HTMLDocument*,DWORD,VARIANT*,VARIANT*);
} cmdtable_t;

extern const cmdtable_t editmode_cmds[];

void do_ns_command(NSContainer*,const char*,nsICommandParams*);

/* timer */
#define UPDATE_UI       0x0001
#define UPDATE_TITLE    0x0002

void update_doc(HTMLDocument *This, DWORD flags);
void update_title(HTMLDocument*);

/* editor */
void init_editor(HTMLDocument*);
void set_ns_editmode(NSContainer*);
void handle_edit_event(HTMLDocument*,nsIDOMEvent*);
HRESULT editor_exec_copy(HTMLDocument*,DWORD,VARIANT*,VARIANT*);
HRESULT editor_exec_cut(HTMLDocument*,DWORD,VARIANT*,VARIANT*);
HRESULT editor_exec_paste(HTMLDocument*,DWORD,VARIANT*,VARIANT*);
void handle_edit_load(HTMLDocument*);
HRESULT editor_is_dirty(HTMLDocument*);
void set_dirty(HTMLDocument*,VARIANT_BOOL);

extern DWORD mshtml_tls;

typedef struct task_t {
    HTMLDocument *doc;

    enum {
        TASK_SETDOWNLOADSTATE,
        TASK_PARSECOMPLETE,
        TASK_SETPROGRESS,
        TASK_START_BINDING
    } task_id;

    nsChannelBSC *bscallback;

    struct task_t *next;
} task_t;

typedef struct {
    HWND thread_hwnd;
    task_t *task_queue_head;
    task_t *task_queue_tail;
} thread_data_t;

thread_data_t *get_thread_data(BOOL);
HWND get_thread_hwnd(void);
void push_task(task_t*);
void remove_doc_tasks(const HTMLDocument*);

/* typelibs */
enum tid_t {
    IHTMLWindow2_tid,
    LAST_tid
};

HRESULT get_typeinfo(enum tid_t, ITypeInfo**);

DEFINE_GUID(CLSID_AboutProtocol, 0x3050F406, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_JSProtocol, 0x3050F3B2, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_MailtoProtocol, 0x3050F3DA, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_ResProtocol, 0x3050F3BC, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_SysimageProtocol, 0x76E67A63, 0x06E9, 0x11D2, 0xA8,0x40, 0x00,0x60,0x08,0x05,0x93,0x82);

DEFINE_GUID(CLSID_CMarkup,0x3050f4fb,0x98b5,0x11cf,0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b);

extern LONG module_ref;
#define LOCK_MODULE()   InterlockedIncrement(&module_ref)
#define UNLOCK_MODULE() InterlockedDecrement(&module_ref)

/* memory allocation functions */

static inline void *heap_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline void *heap_alloc_zero(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

static inline void *heap_realloc(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), 0, mem, len);
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

HINSTANCE get_shdoclc(void);

extern HINSTANCE hInst;
