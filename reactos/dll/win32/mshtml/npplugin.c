/*
 * Copyright 2010 Jacek Caban for CodeWeavers
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

/* Parts of npapi.h */

#define NP_VERSION_MAJOR 0
#define NP_VERSION_MINOR 25

typedef unsigned char NPBool;
typedef INT16 NPError;
typedef INT16 NPReason;
typedef char *NPMIMEType;

typedef struct _NPP {
    void *pdata;
    void *ndata;
} NPP_t, *NPP;

typedef struct _NPStream {
    void *pdata;
    void *ndata;
    const char *url;
    UINT32 end;
    UINT32 lastmodified;
    void *notifyData;
    const char *headers;
} NPStream;

typedef struct _NPSavedData {
    INT32 len;
    void *buf;
} NPSavedData;

typedef struct _NPRect {
    UINT16 top;
    UINT16 left;
    UINT16 bottom;
    UINT16 right;
} NPRect;

typedef enum {
    NPFocusNext = 0,
    NPFocusPrevious = 1
} NPFocusDirection;

#define NP_ABI_MASK 0

typedef enum {
    NPPVpluginNameString = 1,
    NPPVpluginDescriptionString,
    NPPVpluginWindowBool,
    NPPVpluginTransparentBool,
    NPPVjavaClass,
    NPPVpluginWindowSize,
    NPPVpluginTimerInterval,
    NPPVpluginScriptableInstance = (10 | NP_ABI_MASK),
    NPPVpluginScriptableIID = 11,
    NPPVjavascriptPushCallerBool = 12,
    NPPVpluginKeepLibraryInMemory = 13,
    NPPVpluginNeedsXEmbed = 14,
    NPPVpluginScriptableNPObject = 15,
    NPPVformValue = 16,
    NPPVpluginUrlRequestsDisplayedBool = 17,
    NPPVpluginWantsAllNetworkStreams = 18,
    NPPVpluginNativeAccessibleAtkPlugId = 19,
    NPPVpluginCancelSrcStream = 20,
    NPPVSupportsAdvancedKeyHandling = 21
} NPPVariable;

typedef enum {
    NPNVxDisplay = 1,
    NPNVxtAppContext,
    NPNVnetscapeWindow,
    NPNVjavascriptEnabledBool,
    NPNVasdEnabledBool,
    NPNVisOfflineBool,
    NPNVserviceManager = (10 | NP_ABI_MASK),
    NPNVDOMElement     = (11 | NP_ABI_MASK),
    NPNVDOMWindow      = (12 | NP_ABI_MASK),
    NPNVToolkit        = (13 | NP_ABI_MASK),
    NPNVSupportsXEmbedBool = 14,
    NPNVWindowNPObject = 15,
    NPNVPluginElementNPObject = 16,
    NPNVSupportsWindowless = 17,
    NPNVprivateModeBool = 18,
    NPNVsupportsAdvancedKeyHandling = 21
} NPNVariable;

typedef enum {
    NPWindowTypeWindow = 1,
    NPWindowTypeDrawable
} NPWindowType;

typedef struct _NPWindow {
    void *window;
    INT32 x;
    INT32 y;
    UINT32 width;
    UINT32 height;
    NPRect clipRect;
    NPWindowType type;
} NPWindow;

typedef struct _NPFullPrint {
    NPBool pluginPrinted;
    NPBool printOne;
    void *platformPrint;
} NPFullPrint;

typedef struct _NPEmbedPrint {
    NPWindow window;
    void *platformPrint;
} NPEmbedPrint;

typedef struct _NPPrint {
    UINT16 mode;
    union {
        NPFullPrint fullPrint;
        NPEmbedPrint embedPrint;
    } print;
} NPPrint;

typedef HRGN NPRegion;

#define NPERR_BASE                         0
#define NPERR_NO_ERROR                    (NPERR_BASE + 0)
#define NPERR_GENERIC_ERROR               (NPERR_BASE + 1)
#define NPERR_INVALID_INSTANCE_ERROR      (NPERR_BASE + 2)
#define NPERR_INVALID_FUNCTABLE_ERROR     (NPERR_BASE + 3)
#define NPERR_MODULE_LOAD_FAILED_ERROR    (NPERR_BASE + 4)
#define NPERR_OUT_OF_MEMORY_ERROR         (NPERR_BASE + 5)
#define NPERR_INVALID_PLUGIN_ERROR        (NPERR_BASE + 6)
#define NPERR_INVALID_PLUGIN_DIR_ERROR    (NPERR_BASE + 7)
#define NPERR_INCOMPATIBLE_VERSION_ERROR  (NPERR_BASE + 8)
#define NPERR_INVALID_PARAM               (NPERR_BASE + 9)
#define NPERR_INVALID_URL                 (NPERR_BASE + 10)
#define NPERR_FILE_NOT_FOUND              (NPERR_BASE + 11)
#define NPERR_NO_DATA                     (NPERR_BASE + 12)
#define NPERR_STREAM_NOT_SEEKABLE         (NPERR_BASE + 13)

/* Parts of npfunctions.h */

typedef NPError (CDECL *NPP_NewProcPtr)(NPMIMEType,NPP,UINT16,INT16,char**,char**,NPSavedData*);
typedef NPError (CDECL *NPP_DestroyProcPtr)(NPP,NPSavedData**);
typedef NPError (CDECL *NPP_SetWindowProcPtr)(NPP,NPWindow*);
typedef NPError (CDECL *NPP_NewStreamProcPtr)(NPP,NPMIMEType,NPStream*,NPBool,UINT16*);
typedef NPError (CDECL *NPP_DestroyStreamProcPtr)(NPP,NPStream*,NPReason);
typedef INT32 (CDECL *NPP_WriteReadyProcPtr)(NPP,NPStream*);
typedef INT32 (CDECL *NPP_WriteProcPtr)(NPP,NPStream*,INT32,INT32,void*);
typedef void (CDECL *NPP_StreamAsFileProcPtr)(NPP,NPStream*,const char*);
typedef void (CDECL *NPP_PrintProcPtr)(NPP,NPPrint*);
typedef INT16 (CDECL *NPP_HandleEventProcPtr)(NPP,void*);
typedef void (CDECL *NPP_URLNotifyProcPtr)(NPP,const char*,NPReason,void*);
typedef NPError (CDECL *NPP_GetValueProcPtr)(NPP,NPPVariable,void*);
typedef NPError (CDECL *NPP_SetValueProcPtr)(NPP,NPNVariable,void*);
typedef NPBool (CDECL *NPP_GotFocusPtr)(NPP,NPFocusDirection);
typedef void (CDECL *NPP_LostFocusPtr)(NPP);

typedef struct _NPPluginFuncs {
    UINT16 size;
    UINT16 version;
    NPP_NewProcPtr newp;
    NPP_DestroyProcPtr destroy;
    NPP_SetWindowProcPtr setwindow;
    NPP_NewStreamProcPtr newstream;
    NPP_DestroyStreamProcPtr destroystream;
    NPP_StreamAsFileProcPtr asfile;
    NPP_WriteReadyProcPtr writeready;
    NPP_WriteProcPtr write;
    NPP_PrintProcPtr print;
    NPP_HandleEventProcPtr event;
    NPP_URLNotifyProcPtr urlnotify;
    void *javaClass;
    NPP_GetValueProcPtr getvalue;
    NPP_SetValueProcPtr setvalue;
    NPP_GotFocusPtr gotfocus;
    NPP_LostFocusPtr lostfocus;
} NPPluginFuncs;

static nsIDOMHTMLElement *get_dom_element(NPP instance)
{
    nsISupports *instance_unk = (nsISupports*)instance->ndata;
    nsIPluginInstance *plugin_instance;
    nsIDOMHTMLElement *html_elem;
    nsIDOMElement *elem;
    nsresult nsres;

    nsres = nsISupports_QueryInterface(instance_unk, &IID_nsIPluginInstance, (void**)&plugin_instance);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIPluginInstance interface: %08x\n", nsres);
        return NULL;
    }

    nsres = nsIPluginInstance_GetDOMElement(plugin_instance, &elem);
    nsIPluginInstance_Release(plugin_instance);
    if(NS_FAILED(nsres)) {
        ERR("GetDOMElement failed: %08x\n", nsres);
        return NULL;
    }

    nsres = nsIDOMElement_QueryInterface(elem, &IID_nsIDOMHTMLElement, (void**)&html_elem);
    nsIDOMElement_Release(elem);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMHTMLElement iface: %08x\n", nsres);
        return NULL;
    }

    return html_elem;
}

static HTMLInnerWindow *get_elem_window(nsIDOMHTMLElement *elem)
{
    nsIDOMWindow *nswindow;
    nsIDOMDocument *nsdoc;
    HTMLOuterWindow *window;
    nsresult nsres;

    nsres = nsIDOMHTMLElement_GetOwnerDocument(elem, &nsdoc);
    if(NS_FAILED(nsres))
        return NULL;

    nsres = nsIDOMDocument_GetDefaultView(nsdoc, &nswindow);
    nsIDOMDocument_Release(nsdoc);
    if(NS_FAILED(nsres) || !nswindow)
        return NULL;

    window = nswindow_to_window(nswindow);
    nsIDOMWindow_Release(nswindow);

    return window->base.inner_window;
}

static BOOL parse_classid(const PRUnichar *classid, CLSID *clsid)
{
    const WCHAR *ptr;
    unsigned len;
    HRESULT hres;

    static const PRUnichar clsidW[] = {'c','l','s','i','d',':'};

    if(strncmpiW(classid, clsidW, sizeof(clsidW)/sizeof(WCHAR)))
        return FALSE;

    ptr = classid + sizeof(clsidW)/sizeof(WCHAR);
    len = strlenW(ptr);

    if(len == 38) {
        hres = CLSIDFromString(ptr, clsid);
    }else if(len == 36) {
        WCHAR buf[39];

        buf[0] = '{';
        memcpy(buf+1, ptr, len*sizeof(WCHAR));
        buf[37] = '}';
        buf[38] = 0;
        hres = CLSIDFromString(buf, clsid);
    }else {
        return FALSE;
    }

    return SUCCEEDED(hres);
}

static BOOL get_elem_clsid(nsIDOMHTMLElement *elem, CLSID *clsid)
{
    const PRUnichar *val;
    nsAString val_str;
    nsresult nsres;
    BOOL ret = FALSE;

    static const PRUnichar classidW[] = {'c','l','a','s','s','i','d',0};

    nsres = get_elem_attr_value(elem, classidW, &val_str, &val);
    if(NS_SUCCEEDED(nsres)) {
        if(*val)
            ret = parse_classid(val, clsid);
        nsAString_Finish(&val_str);
    }

    return ret;
}

typedef struct {
    IBindStatusCallback IBindStatusCallback_iface;
    IWindowForBindingUI IWindowForBindingUI_iface;
    LONG ref;
} InstallCallback;

static inline InstallCallback *impl_from_IBindStatusCallback(IBindStatusCallback *iface)
{
    return CONTAINING_RECORD(iface, InstallCallback, IBindStatusCallback_iface);
}

static HRESULT WINAPI InstallCallback_QueryInterface(IBindStatusCallback *iface,
        REFIID riid, void **ppv)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IBindStatusCallback_iface;
    }else if(IsEqualGUID(&IID_IBindStatusCallback, riid)) {
        TRACE("(%p)->(IID_IBindStatusCallback %p)\n", This, ppv);
        *ppv = &This->IBindStatusCallback_iface;
    }else if(IsEqualGUID(&IID_IWindowForBindingUI, riid)) {
        TRACE("(%p)->(IID_IWindowForBindingUI %p)\n", This, ppv);
        *ppv = &This->IWindowForBindingUI_iface;
    }else {
        TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI InstallCallback_AddRef(IBindStatusCallback *iface)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI InstallCallback_Release(IBindStatusCallback *iface)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI InstallCallback_OnStartBinding(IBindStatusCallback *iface,
        DWORD dwReserved, IBinding *pib)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);
    TRACE("(%p)->(%x %p)\n", This, dwReserved, pib);
    return S_OK;
}

static HRESULT WINAPI InstallCallback_GetPriority(IBindStatusCallback *iface, LONG *pnPriority)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);
    TRACE("(%p)->(%p)\n", This, pnPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallCallback_OnLowResource(IBindStatusCallback *iface, DWORD dwReserved)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);
    TRACE("(%p)->(%x)\n", This, dwReserved);
    return S_OK;
}

static HRESULT WINAPI InstallCallback_OnProgress(IBindStatusCallback *iface, ULONG ulProgress,
        ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);
    TRACE("(%p)->(%u %u %u %s)\n", This, ulProgress, ulProgressMax, ulStatusCode, debugstr_w(szStatusText));
    return S_OK;
}

static HRESULT WINAPI InstallCallback_OnStopBinding(IBindStatusCallback *iface,
        HRESULT hresult, LPCWSTR szError)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);
    TRACE("(%p)->(%08x %s)\n", This, hresult, debugstr_w(szError));
    return S_OK;
}

static HRESULT WINAPI InstallCallback_GetBindInfo(IBindStatusCallback *iface,
        DWORD* grfBINDF, BINDINFO* pbindinfo)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);

    *grfBINDF = BINDF_ASYNCHRONOUS;
    return S_OK;
}

static HRESULT WINAPI InstallCallback_OnDataAvailable(IBindStatusCallback *iface, DWORD grfBSCF,
        DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);
    ERR("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallCallback_OnObjectAvailable(IBindStatusCallback *iface,
        REFIID riid, IUnknown* punk)
{
    InstallCallback *This = impl_from_IBindStatusCallback(iface);
    ERR("(%p)\n", This);
    return E_NOTIMPL;
}

static IBindStatusCallbackVtbl InstallCallbackVtbl = {
    InstallCallback_QueryInterface,
    InstallCallback_AddRef,
    InstallCallback_Release,
    InstallCallback_OnStartBinding,
    InstallCallback_GetPriority,
    InstallCallback_OnLowResource,
    InstallCallback_OnProgress,
    InstallCallback_OnStopBinding,
    InstallCallback_GetBindInfo,
    InstallCallback_OnDataAvailable,
    InstallCallback_OnObjectAvailable
};

static inline InstallCallback *impl_from_IWindowForBindingUI(IWindowForBindingUI *iface)
{
    return CONTAINING_RECORD(iface, InstallCallback, IWindowForBindingUI_iface);
}

static HRESULT WINAPI WindowForBindingUI_QueryInterface(IWindowForBindingUI *iface, REFIID riid, void **ppv)
{
    InstallCallback *This = impl_from_IWindowForBindingUI(iface);
    return IBindStatusCallback_QueryInterface(&This->IBindStatusCallback_iface, riid, ppv);
}

static ULONG WINAPI WindowForBindingUI_AddRef(IWindowForBindingUI *iface)
{
    InstallCallback *This = impl_from_IWindowForBindingUI(iface);
    return IBindStatusCallback_AddRef(&This->IBindStatusCallback_iface);
}

static ULONG WINAPI WindowForBindingUI_Release(IWindowForBindingUI *iface)
{
    InstallCallback *This = impl_from_IWindowForBindingUI(iface);
    return IBindStatusCallback_Release(&This->IBindStatusCallback_iface);
}

static HRESULT WINAPI WindowForBindingUI_GetWindow(IWindowForBindingUI *iface, REFGUID rguidReason, HWND *phwnd)
{
    InstallCallback *This = impl_from_IWindowForBindingUI(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(rguidReason), phwnd);
    *phwnd = NULL;
    return S_OK;
}

static const IWindowForBindingUIVtbl WindowForBindingUIVtbl = {
    WindowForBindingUI_QueryInterface,
    WindowForBindingUI_AddRef,
    WindowForBindingUI_Release,
    WindowForBindingUI_GetWindow
};

typedef struct {
    struct list entry;
    IUri *uri;
} install_entry_t;

static struct list install_list = LIST_INIT(install_list);

static CRITICAL_SECTION cs_install_list;
static CRITICAL_SECTION_DEBUG cs_install_list_dbg =
{
    0, 0, &cs_install_list,
    { &cs_install_list_dbg.ProcessLocksList, &cs_install_list_dbg.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": install_list") }
};
static CRITICAL_SECTION cs_install_list = { &cs_install_list_dbg, -1, 0, 0, 0, 0 };

static void install_codebase(const WCHAR *url)
{
    InstallCallback *callback;
    IBindCtx *bctx;
    HRESULT hres;

    callback = heap_alloc(sizeof(*callback));
    if(!callback)
        return;

    callback->IBindStatusCallback_iface.lpVtbl = &InstallCallbackVtbl;
    callback->IWindowForBindingUI_iface.lpVtbl = &WindowForBindingUIVtbl;
    callback->ref = 1;

    hres = CreateAsyncBindCtx(0, &callback->IBindStatusCallback_iface, NULL, &bctx);
    IBindStatusCallback_Release(&callback->IBindStatusCallback_iface);
    if(FAILED(hres))
        return;

    hres = AsyncInstallDistributionUnit(NULL, NULL, NULL, 0, 0, url, bctx, NULL, 0);
    IBindCtx_Release(bctx);
    if(FAILED(hres))
        WARN("FAILED: %08x\n", hres);
}

static void check_codebase(HTMLInnerWindow *window, nsIDOMHTMLElement *nselem)
{
    BOOL is_on_list = FALSE;
    install_entry_t *iter;
    const PRUnichar *val;
    nsAString val_str;
    IUri *uri = NULL;
    nsresult nsres;
    HRESULT hres;

    static const PRUnichar codebaseW[] = {'c','o','d','e','b','a','s','e',0};

    nsres = get_elem_attr_value(nselem, codebaseW, &val_str, &val);
    if(NS_SUCCEEDED(nsres)) {
        if(*val) {
            hres = CoInternetCombineUrlEx(window->base.outer_window->uri, val, 0, &uri, 0);
            if(FAILED(hres))
                uri = NULL;
        }
        nsAString_Finish(&val_str);
    }

    if(!uri)
        return;

    EnterCriticalSection(&cs_install_list);

    LIST_FOR_EACH_ENTRY(iter, &install_list, install_entry_t, entry) {
        BOOL eq;

        hres = IUri_IsEqual(uri, iter->uri, &eq);
        if(SUCCEEDED(hres) && eq) {
            TRACE("already proceeded\n");
            is_on_list = TRUE;
            break;
        }
    }

    if(!is_on_list) {
        iter = heap_alloc(sizeof(*iter));
        if(iter) {
            IUri_AddRef(uri);
            iter->uri = uri;

            list_add_tail(&install_list, &iter->entry);
        }
    }

    LeaveCriticalSection(&cs_install_list);

    if(!is_on_list) {
        BSTR display_uri;

        hres = IUri_GetDisplayUri(uri, &display_uri);
        if(SUCCEEDED(hres)) {
            install_codebase(display_uri);
            SysFreeString(display_uri);
        }
    }

    IUri_Release(uri);
}

static IUnknown *create_activex_object(HTMLInnerWindow *window, nsIDOMHTMLElement *nselem, CLSID *clsid)
{
    IClassFactoryEx *cfex;
    IClassFactory *cf;
    IUnknown *obj;
    DWORD policy;
    HRESULT hres;

    if(!get_elem_clsid(nselem, clsid)) {
        WARN("Could not determine element CLSID\n");
        return NULL;
    }

    TRACE("clsid %s\n", debugstr_guid(clsid));

    policy = 0;
    hres = IInternetHostSecurityManager_ProcessUrlAction(&window->doc->IInternetHostSecurityManager_iface,
            URLACTION_ACTIVEX_RUN, (BYTE*)&policy, sizeof(policy), (BYTE*)clsid, sizeof(GUID), 0, 0);
    if(FAILED(hres) || policy != URLPOLICY_ALLOW) {
        WARN("ProcessUrlAction returned %08x %x\n", hres, policy);
        return NULL;
    }

    hres = CoGetClassObject(clsid, CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER, NULL, &IID_IClassFactory, (void**)&cf);
    if(hres == REGDB_E_CLASSNOTREG)
        check_codebase(window, nselem);
    if(FAILED(hres))
        return NULL;

    hres = IClassFactory_QueryInterface(cf, &IID_IClassFactoryEx, (void**)&cfex);
    if(SUCCEEDED(hres)) {
        FIXME("Use IClassFactoryEx\n");
        IClassFactoryEx_Release(cfex);
    }

    hres = IClassFactory_CreateInstance(cf, NULL, &IID_IUnknown, (void**)&obj);
    IClassFactory_Release(cf);
    if(FAILED(hres))
        return NULL;

    return obj;
}

static NPError CDECL NPP_New(NPMIMEType pluginType, NPP instance, UINT16 mode, INT16 argc, char **argn,
        char **argv, NPSavedData *saved)
{
    nsIDOMHTMLElement *nselem;
    HTMLInnerWindow *window;
    IUnknown *obj;
    CLSID clsid;
    NPError err = NPERR_NO_ERROR;

    TRACE("(%s %p %x %d %p %p %p)\n", debugstr_a(pluginType), instance, mode, argc, argn, argv, saved);

    nselem = get_dom_element(instance);
    if(!nselem) {
        ERR("Could not get DOM element\n");
        return NPERR_GENERIC_ERROR;
    }

    window = get_elem_window(nselem);
    if(!window) {
        ERR("Could not get element's window object\n");
        nsIDOMHTMLElement_Release(nselem);
        return NPERR_GENERIC_ERROR;
    }

    obj = create_activex_object(window, nselem, &clsid);
    if(obj) {
        PluginHost *host;
        HRESULT hres;

        hres = create_plugin_host(window->doc, (nsIDOMElement*)nselem, obj, &clsid, &host);
        IUnknown_Release(obj);
        if(SUCCEEDED(hres))
            instance->pdata = host;
        else
            err = NPERR_GENERIC_ERROR;
    }else {
        err = NPERR_GENERIC_ERROR;
    }

    nsIDOMHTMLElement_Release(nselem);
    return err;
}

static NPError CDECL NPP_Destroy(NPP instance, NPSavedData **save)
{
    PluginHost *host = instance->pdata;

    TRACE("(%p %p)\n", instance, save);

    if(!host)
        return NPERR_GENERIC_ERROR;

    detach_plugin_host(host);
    IOleClientSite_Release(&host->IOleClientSite_iface);
    instance->pdata = NULL;
    return NPERR_NO_ERROR;
}

static NPError CDECL NPP_SetWindow(NPP instance, NPWindow *window)
{
    PluginHost *host = instance->pdata;
    RECT pos_rect = {0, 0, window->width, window->height};

    TRACE("(%p %p)\n", instance, window);

    if(!host)
        return NPERR_GENERIC_ERROR;

    update_plugin_window(host, window->window, &pos_rect);
    return NPERR_NO_ERROR;
}

static NPError CDECL NPP_NewStream(NPP instance, NPMIMEType type, NPStream *stream, NPBool seekable, UINT16 *stype)
{
    TRACE("\n");
    return NPERR_GENERIC_ERROR;
}

static NPError CDECL NPP_DestroyStream(NPP instance, NPStream *stream, NPReason reason)
{
    TRACE("\n");
    return NPERR_GENERIC_ERROR;
}

static INT32 CDECL NPP_WriteReady(NPP instance, NPStream *stream)
{
    TRACE("\n");
    return NPERR_GENERIC_ERROR;
}

static INT32 CDECL NPP_Write(NPP instance, NPStream *stream, INT32 offset, INT32 len, void *buffer)
{
    TRACE("\n");
    return NPERR_GENERIC_ERROR;
}

static void CDECL NPP_StreamAsFile(NPP instance, NPStream *stream, const char *fname)
{
    TRACE("\n");
}

static void CDECL NPP_Print(NPP instance, NPPrint *platformPrint)
{
    FIXME("\n");
}

static INT16 CDECL NPP_HandleEvent(NPP instance, void *event)
{
    TRACE("\n");
    return NPERR_GENERIC_ERROR;
}

static void CDECL NPP_URLNotify(NPP instance, const char *url, NPReason reason, void *notifyData)
{
    TRACE("\n");
}

static NPError CDECL NPP_GetValue(NPP instance, NPPVariable variable, void *ret_value)
{
    TRACE("\n");
    return NPERR_GENERIC_ERROR;
}

static NPError CDECL NPP_SetValue(NPP instance, NPNVariable variable, void *value)
{
    TRACE("\n");
    return NPERR_GENERIC_ERROR;
}

static NPBool CDECL NPP_GotFocus(NPP instance, NPFocusDirection direction)
{
    FIXME("\n");
    return NPERR_GENERIC_ERROR;
}

static void CDECL NPP_LostFocus(NPP instance)
{
    FIXME("\n");
}

/***********************************************************************
 *          NP_GetEntryPoints (mshtml.@)
 */
NPError WINAPI NP_GetEntryPoints(NPPluginFuncs* funcs)
{
    TRACE("(%p)\n", funcs);

    funcs->version = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
    funcs->newp = NPP_New;
    funcs->destroy = NPP_Destroy;
    funcs->setwindow = NPP_SetWindow;
    funcs->newstream = NPP_NewStream;
    funcs->destroystream = NPP_DestroyStream;
    funcs->asfile = NPP_StreamAsFile;
    funcs->writeready = NPP_WriteReady;
    funcs->write = NPP_Write;
    funcs->print = NPP_Print;
    funcs->event = NPP_HandleEvent;
    funcs->urlnotify = NPP_URLNotify;
    funcs->javaClass = NULL;
    funcs->getvalue = NPP_GetValue;
    funcs->setvalue = NPP_SetValue;
    funcs->gotfocus = NPP_GotFocus;
    funcs->lostfocus = NPP_LostFocus;

    return NPERR_NO_ERROR;
}
