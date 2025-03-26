/*
 *    MSHTML Class Factory
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2003 Mike McCormack
 * Copyright 2005 Jacek Caban
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

#include <advpub.h>
#include <rpcproxy.h>
#include <mlang.h>
#include <initguid.h>

HINSTANCE hInst;
DWORD mshtml_tls = TLS_OUT_OF_INDEXES;

void (__cdecl *ccp_init)(ExternalCycleCollectionParticipant*,const CCObjCallback*);
nsrefcnt (__cdecl *ccref_incr)(nsCycleCollectingAutoRefCnt*,nsISupports*);
nsrefcnt (__cdecl *ccref_decr)(nsCycleCollectingAutoRefCnt*,nsISupports*,ExternalCycleCollectionParticipant*);
void (__cdecl *ccref_init)(nsCycleCollectingAutoRefCnt*,nsrefcnt);
void (__cdecl *describe_cc_node)(nsCycleCollectingAutoRefCnt*,const char*,nsCycleCollectionTraversalCallback*);
void (__cdecl *note_cc_edge)(nsISupports*,const char*,nsCycleCollectionTraversalCallback*);

static HINSTANCE shdoclc = NULL;
static HDC display_dc;
static WCHAR *status_strings[IDS_STATUS_LAST-IDS_STATUS_FIRST+1];
static IMultiLanguage2 *mlang;

UINT cp_from_charset_string(BSTR charset)
{
    MIMECSETINFO info;
    HRESULT hres;

    if(!mlang) {
        IMultiLanguage2 *new_mlang;

        hres = CoCreateInstance(&CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER,
                &IID_IMultiLanguage2, (void**)&new_mlang);
        if(FAILED(hres)) {
            ERR("Could not create CMultiLanguage instance\n");
            return CP_UTF8;
        }

        if(InterlockedCompareExchangePointer((void**)&mlang, new_mlang, NULL))
            IMultiLanguage2_Release(new_mlang);
    }

    hres = IMultiLanguage2_GetCharsetInfo(mlang, charset, &info);
    if(FAILED(hres)) {
        FIXME("GetCharsetInfo failed: %08x\n", hres);
        return CP_UTF8;
    }

    return info.uiInternetEncoding;
}

static void thread_detach(void)
{
    thread_data_t *thread_data;

    thread_data = get_thread_data(FALSE);
    if(!thread_data)
        return;

    if(thread_data->thread_hwnd)
        DestroyWindow(thread_data->thread_hwnd);

    heap_free(thread_data);
}

static void free_strings(void)
{
    unsigned int i;
    for(i = 0; i < sizeof(status_strings)/sizeof(*status_strings); i++)
        heap_free(status_strings[i]);
}

static void process_detach(void)
{
    close_gecko();
    release_typelib();

    if(shdoclc)
        FreeLibrary(shdoclc);
    if(mshtml_tls != TLS_OUT_OF_INDEXES)
        TlsFree(mshtml_tls);
    if(display_dc)
        DeleteObject(display_dc);
    if(mlang)
        IMultiLanguage2_Release(mlang);

    free_strings();
}

void set_statustext(HTMLDocumentObj* doc, INT id, LPCWSTR arg)
{
    int index = id - IDS_STATUS_FIRST;
    WCHAR *p = status_strings[index];
    DWORD len;

    if(!doc->frame)
        return;

    if(!p) {
        len = 255;
        p = heap_alloc(len * sizeof(WCHAR));
        len = LoadStringW(hInst, id, p, len) + 1;
        p = heap_realloc(p, len * sizeof(WCHAR));
        if(InterlockedCompareExchangePointer((void**)&status_strings[index], p, NULL)) {
            heap_free(p);
            p = status_strings[index];
        }
    }

    if(arg) {
        WCHAR *buf;

        len = lstrlenW(p) + lstrlenW(arg) - 1;
        buf = heap_alloc(len * sizeof(WCHAR));

        snprintfW(buf, len, p, arg);

        p = buf;
    }

    IOleInPlaceFrame_SetStatusText(doc->frame, p);

    if(arg)
        heap_free(p);
}

HRESULT do_query_service(IUnknown *unk, REFGUID guid_service, REFIID riid, void **ppv)
{
    IServiceProvider *sp;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IServiceProvider, (void**)&sp);
    if(FAILED(hres))
        return hres;

    hres = IServiceProvider_QueryService(sp, guid_service, riid, ppv);
    IServiceProvider_Release(sp);
    return hres;
}

HINSTANCE get_shdoclc(void)
{
    static const WCHAR wszShdoclc[] =
        {'s','h','d','o','c','l','c','.','d','l','l',0};

    if(shdoclc)
        return shdoclc;

    return shdoclc = LoadLibraryExW(wszShdoclc, NULL, LOAD_LIBRARY_AS_DATAFILE);
}

HDC get_display_dc(void)
{
    static const WCHAR displayW[] = {'D','I','S','P','L','A','Y',0};

    if(!display_dc) {
        HDC hdc;

        hdc = CreateICW(displayW, NULL, NULL, NULL);
        if(InterlockedCompareExchangePointer((void**)&display_dc, hdc, NULL))
            DeleteObject(hdc);
    }

    return display_dc;
}

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID reserved)
{
    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        hInst = hInstDLL;
        break;
    case DLL_PROCESS_DETACH:
        if (reserved) break;
        process_detach();
        break;
    case DLL_THREAD_DETACH:
        thread_detach();
        break;
    }
    return TRUE;
}

/***********************************************************
 *    ClassFactory implementation
 */
typedef HRESULT (*CreateInstanceFunc)(IUnknown*,REFIID,void**);
typedef struct {
    IClassFactory IClassFactory_iface;
    LONG ref;
    CreateInstanceFunc fnCreateInstance;
} ClassFactory;

static inline ClassFactory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, ClassFactory, IClassFactory_iface);
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFGUID riid, void **ppvObject)
{
    if(IsEqualGUID(&IID_IClassFactory, riid) || IsEqualGUID(&IID_IUnknown, riid)) {
        IClassFactory_AddRef(iface);
        *ppvObject = iface;
        return S_OK;
    }

    WARN("not supported iid %s\n", debugstr_mshtml_guid(riid));
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    ClassFactory *This = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref = %u\n", This, ref);
    return ref;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    ClassFactory *This = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref = %u\n", This, ref);

    if(!ref) {
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter,
        REFIID riid, void **ppvObject)
{
    ClassFactory *This = impl_from_IClassFactory(iface);
    return This->fnCreateInstance(pUnkOuter, riid, ppvObject);
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    TRACE("(%p)->(%x)\n", iface, dolock);

    /* We never unload the DLL. See DllCanUnloadNow(). */
    return S_OK;
}

static const IClassFactoryVtbl HTMLClassFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static HRESULT ClassFactory_Create(REFIID riid, void **ppv, CreateInstanceFunc fnCreateInstance)
{
    ClassFactory *ret = heap_alloc(sizeof(ClassFactory));
    HRESULT hres;

    ret->IClassFactory_iface.lpVtbl = &HTMLClassFactoryVtbl;
    ret->ref = 0;
    ret->fnCreateInstance = fnCreateInstance;

    hres = IClassFactory_QueryInterface(&ret->IClassFactory_iface, riid, ppv);
    if(FAILED(hres)) {
        heap_free(ret);
        *ppv = NULL;
    }
    return hres;
}

/******************************************************************
 *		DllGetClassObject (MSHTML.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    if(IsEqualGUID(&CLSID_HTMLDocument, rclsid)) {
        TRACE("(CLSID_HTMLDocument %s %p)\n", debugstr_mshtml_guid(riid), ppv);
        return ClassFactory_Create(riid, ppv, HTMLDocument_Create);
    }else if(IsEqualGUID(&CLSID_AboutProtocol, rclsid)) {
        TRACE("(CLSID_AboutProtocol %s %p)\n", debugstr_mshtml_guid(riid), ppv);
        return ProtocolFactory_Create(rclsid, riid, ppv);
    }else if(IsEqualGUID(&CLSID_JSProtocol, rclsid)) {
        TRACE("(CLSID_JSProtocol %s %p)\n", debugstr_mshtml_guid(riid), ppv);
        return ProtocolFactory_Create(rclsid, riid, ppv);
    }else if(IsEqualGUID(&CLSID_MailtoProtocol, rclsid)) {
        TRACE("(CLSID_MailtoProtocol %s %p)\n", debugstr_mshtml_guid(riid), ppv);
        return ProtocolFactory_Create(rclsid, riid, ppv);
    }else if(IsEqualGUID(&CLSID_ResProtocol, rclsid)) {
        TRACE("(CLSID_ResProtocol %s %p)\n", debugstr_mshtml_guid(riid), ppv);
        return ProtocolFactory_Create(rclsid, riid, ppv);
    }else if(IsEqualGUID(&CLSID_SysimageProtocol, rclsid)) {
        TRACE("(CLSID_SysimageProtocol %s %p)\n", debugstr_mshtml_guid(riid), ppv);
        return ProtocolFactory_Create(rclsid, riid, ppv);
    }else if(IsEqualGUID(&CLSID_HTMLLoadOptions, rclsid)) {
        TRACE("(CLSID_HTMLLoadOptions %s %p)\n", debugstr_mshtml_guid(riid), ppv);
        return ClassFactory_Create(riid, ppv, HTMLLoadOptions_Create);
    }

    FIXME("Unknown class %s\n", debugstr_guid(rclsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/******************************************************************
 *              DllCanUnloadNow (MSHTML.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    TRACE("()\n");
    /* The cost of keeping this DLL in memory is small. */
    return S_FALSE;
}

/***********************************************************************
 *          RunHTMLApplication (MSHTML.@)
 *
 * Appears to have the same prototype as WinMain.
 */
HRESULT WINAPI RunHTMLApplication( HINSTANCE hinst, HINSTANCE hPrevInst,
                               LPSTR szCmdLine, INT nCmdShow )
{
    FIXME("%p %p %s %d\n", hinst, hPrevInst, debugstr_a(szCmdLine), nCmdShow );
    return 0;
}

/***********************************************************************
 *          RNIGetCompatibleVersion (MSHTML.@)
 */
DWORD WINAPI RNIGetCompatibleVersion(void)
{
    TRACE("()\n");
    return 0x20000;
}

/***********************************************************************
 *          DllInstall (MSHTML.@)
 */
HRESULT WINAPI DllInstall(BOOL bInstall, LPCWSTR cmdline)
{
    FIXME("stub %d %s: returning S_OK\n", bInstall, debugstr_w(cmdline));
    return S_OK;
}

/***********************************************************************
 *          ShowHTMLDialog (MSHTML.@)
 */
HRESULT WINAPI ShowHTMLDialog(HWND hwndParent, IMoniker *pMk, VARIANT *pvarArgIn,
        WCHAR *pchOptions, VARIANT *pvarArgOut)
{
    FIXME("(%p %p %p %s %p)\n", hwndParent, pMk, pvarArgIn, debugstr_w(pchOptions), pvarArgOut);
    return E_NOTIMPL;
}

/***********************************************************************
 *          PrintHTML (MSHTML.@)
 */
void WINAPI PrintHTML(HWND hwnd, HINSTANCE handle, LPCSTR cmdline, INT show)
{
    FIXME("(%p %p %s %x)\n", hwnd, handle, debugstr_a(cmdline), show);
}

DEFINE_GUID(CLSID_CBackgroundPropertyPage, 0x3050F232, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_CCDAnchorPropertyPage, 0x3050F1FC, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_CCDGenericPropertyPage, 0x3050F17F, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_CDwnBindInfo, 0x3050F3C2, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_CHiFiUses, 0x5AAF51B3, 0xB1F0, 0x11D1, 0xB6,0xAB, 0x00,0xA0,0xC9,0x08,0x33,0xE9);
DEFINE_GUID(CLSID_CHtmlComponentConstructor, 0x3050F4F8, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_CInlineStylePropertyPage, 0x3050F296, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_CPeerHandler, 0x5AAF51B2, 0xB1F0, 0x11D1, 0xB6,0xAB, 0x00,0xA0,0xC9,0x08,0x33,0xE9);
DEFINE_GUID(CLSID_CRecalcEngine, 0x3050F499, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_CSvrOMUses, 0x3050F4F0, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_CrSource, 0x65014010, 0x9F62, 0x11D1, 0xA6,0x51, 0x00,0x60,0x08,0x11,0xD5,0xCE);
DEFINE_GUID(CLSID_ExternalFrameworkSite, 0x3050F163, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_HTADocument, 0x3050F5C8, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_HTMLPluginDocument, 0x25336921, 0x03F9, 0x11CF, 0x8F,0xD0, 0x00,0xAA,0x00,0x68,0x6F,0x13);
DEFINE_GUID(CLSID_HTMLPopup, 0x3050F667, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_HTMLPopupDoc, 0x3050F67D, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_HTMLServerDoc, 0x3050F4E7, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_IImageDecodeFilter, 0x607FD4E8, 0x0A03, 0x11D1, 0xAB,0x1D, 0x00,0xC0,0x4F,0xC9,0xB3,0x04);
DEFINE_GUID(CLSID_IImgCtx, 0x3050F3D6, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_IntDitherer, 0x05F6FE1A, 0xECEF, 0x11D0, 0xAA,0xE7, 0x00,0xC0,0x4F,0xC9,0xB3,0x04);
DEFINE_GUID(CLSID_MHTMLDocument, 0x3050F3D9, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_TridentAPI, 0x429AF92C, 0xA51F, 0x11D2, 0x86,0x1E, 0x00,0xC0,0x4F,0xA3,0x5C,0x89);

#define INF_SET_ID(id)            \
    do                            \
    {                             \
        static CHAR name[] = #id; \
                                  \
        pse[i].pszName = name;    \
        clsids[i++] = &id;        \
    } while (0)

#define INF_SET_CLSID(clsid) INF_SET_ID(CLSID_ ## clsid)

static HRESULT register_server(BOOL do_register)
{
    HRESULT hres;
    HMODULE hAdvpack;
    HRESULT (WINAPI *pRegInstall)(HMODULE hm, LPCSTR pszSection, const STRTABLEA* pstTable);
    STRTABLEA strtable;
    STRENTRYA pse[35];
    static CLSID const *clsids[35];
    unsigned int i = 0;

    static const WCHAR wszAdvpack[] = {'a','d','v','p','a','c','k','.','d','l','l',0};

    TRACE("(%x)\n", do_register);

    INF_SET_CLSID(AboutProtocol);
    INF_SET_CLSID(CAnchorBrowsePropertyPage);
    INF_SET_CLSID(CBackgroundPropertyPage);
    INF_SET_CLSID(CCDAnchorPropertyPage);
    INF_SET_CLSID(CCDGenericPropertyPage);
    INF_SET_CLSID(CDocBrowsePropertyPage);
    INF_SET_CLSID(CDwnBindInfo);
    INF_SET_CLSID(CHiFiUses);
    INF_SET_CLSID(CHtmlComponentConstructor);
    INF_SET_CLSID(CImageBrowsePropertyPage);
    INF_SET_CLSID(CInlineStylePropertyPage);
    INF_SET_CLSID(CPeerHandler);
    INF_SET_CLSID(CRecalcEngine);
    INF_SET_CLSID(CSvrOMUses);
    INF_SET_CLSID(CrSource);
    INF_SET_CLSID(ExternalFrameworkSite);
    INF_SET_CLSID(HTADocument);
    INF_SET_CLSID(HTMLDocument);
    INF_SET_CLSID(HTMLLoadOptions);
    INF_SET_CLSID(HTMLPluginDocument);
    INF_SET_CLSID(HTMLPopup);
    INF_SET_CLSID(HTMLPopupDoc);
    INF_SET_CLSID(HTMLServerDoc);
    INF_SET_CLSID(HTMLWindowProxy);
    INF_SET_CLSID(IImageDecodeFilter);
    INF_SET_CLSID(IImgCtx);
    INF_SET_CLSID(IntDitherer);
    INF_SET_CLSID(JSProtocol);
    INF_SET_CLSID(MHTMLDocument);
    INF_SET_CLSID(MailtoProtocol);
    INF_SET_CLSID(ResProtocol);
    INF_SET_CLSID(Scriptlet);
    INF_SET_CLSID(SysimageProtocol);
    INF_SET_CLSID(TridentAPI);
    INF_SET_ID(LIBID_MSHTML);

    for(i=0; i < sizeof(pse)/sizeof(pse[0]); i++) {
        pse[i].pszValue = heap_alloc(39);
        sprintf(pse[i].pszValue, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                clsids[i]->Data1, clsids[i]->Data2, clsids[i]->Data3, clsids[i]->Data4[0],
                clsids[i]->Data4[1], clsids[i]->Data4[2], clsids[i]->Data4[3], clsids[i]->Data4[4],
                clsids[i]->Data4[5], clsids[i]->Data4[6], clsids[i]->Data4[7]);
    }

    strtable.cEntries = sizeof(pse)/sizeof(pse[0]);
    strtable.pse = pse;

    hAdvpack = LoadLibraryW(wszAdvpack);
    pRegInstall = (void *)GetProcAddress(hAdvpack, "RegInstall");

    hres = pRegInstall(hInst, do_register ? "RegisterDll" : "UnregisterDll", &strtable);

    FreeLibrary(hAdvpack);

    for(i=0; i < sizeof(pse)/sizeof(pse[0]); i++)
        heap_free(pse[i].pszValue);

    if(FAILED(hres)) {
        ERR("RegInstall failed: %08x\n", hres);
        return hres;
    }

    if(do_register) {
        ITypeLib *typelib;

        static const WCHAR wszMSHTML[] = {'m','s','h','t','m','l','.','t','l','b',0};

        hres = LoadTypeLibEx(wszMSHTML, REGKIND_REGISTER, &typelib);
        if(SUCCEEDED(hres))
            ITypeLib_Release(typelib);
    }else {
        hres = UnRegisterTypeLib(&LIBID_MSHTML, 4, 0, LOCALE_SYSTEM_DEFAULT, SYS_WIN32);
    }

    if(FAILED(hres))
        ERR("typelib registration failed: %08x\n", hres);

    return hres;
}

#undef INF_SET_CLSID
#undef INF_SET_ID

/***********************************************************************
 *          DllRegisterServer (MSHTML.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    HRESULT hres;

    hres = __wine_register_resources( hInst );
    if(SUCCEEDED(hres))
        hres = register_server(TRUE);
    if(SUCCEEDED(hres))
        load_gecko();

    return hres;
}

/***********************************************************************
 *          DllUnregisterServer (MSHTML.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    HRESULT hres = __wine_unregister_resources( hInst );
    if(SUCCEEDED(hres)) hres = register_server(FALSE);
    return hres;
}

const char *debugstr_mshtml_guid(const GUID *iid)
{
#define X(x) if(IsEqualGUID(iid, &x)) return #x
    X(DIID_HTMLDocumentEvents);
    X(DIID_HTMLDocumentEvents2);
    X(DIID_HTMLTableEvents);
    X(DIID_HTMLTextContainerEvents);
    X(IID_HTMLPluginContainer);
    X(IID_IConnectionPoint);
    X(IID_IConnectionPointContainer);
    X(IID_ICustomDoc);
    X(IID_IDispatch);
    X(IID_IDispatchEx);
    X(IID_IDispatchJS);
    X(IID_UndocumentedScriptIface);
    X(IID_IEnumConnections);
    X(IID_IEnumVARIANT);
    X(IID_IHlinkTarget);
    X(IID_IHTMLDocument6);
    X(IID_IHTMLDocument7);
    X(IID_IHTMLEditServices);
    X(IID_IHTMLFramesCollection2);
    X(IID_IHTMLPrivateWindow);
    X(IID_IHtmlLoadOptions);
    X(IID_IInternetHostSecurityManager);
    X(IID_IMonikerProp);
    X(IID_IObjectIdentity);
    X(IID_IObjectSafety);
    X(IID_IObjectWithSite);
    X(IID_IOleContainer);
    X(IID_IOleCommandTarget);
    X(IID_IOleControl);
    X(IID_IOleDocument);
    X(IID_IOleDocumentView);
    X(IID_IOleInPlaceActiveObject);
    X(IID_IOleInPlaceFrame);
    X(IID_IOleInPlaceObject);
    X(IID_IOleInPlaceObjectWindowless);
    X(IID_IOleInPlaceUIWindow);
    X(IID_IOleObject);
    X(IID_IOleWindow);
    X(IID_IOptionArray);
    X(IID_IPersist);
    X(IID_IPersistFile);
    X(IID_IPersistHistory);
    X(IID_IPersistMoniker);
    X(IID_IPersistStreamInit);
    X(IID_IPropertyNotifySink);
    X(IID_IProvideClassInfo);
    X(IID_IServiceProvider);
    X(IID_ISupportErrorInfo);
    X(IID_ITargetContainer);
    X(IID_ITravelLogClient);
    X(IID_IUnknown);
    X(IID_IViewObject);
    X(IID_IViewObject2);
    X(IID_IViewObjectEx);
    X(IID_nsCycleCollectionISupports);
    X(IID_nsXPCOMCycleCollectionParticipant);
#define XIID(x) X(IID_##x);
#define XDIID(x) X(DIID_##x);
    TID_LIST
#undef XIID
#undef XDIID
#undef X

    return debugstr_guid(iid);
}
