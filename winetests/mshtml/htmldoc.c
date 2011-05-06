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

#define COBJMACROS
#define CONST_VTABLE

#include <wine/test.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "initguid.h"
#include "ole2.h"
#include "mshtml.h"
#include "docobj.h"
#include "wininet.h"
#include "mshtmhst.h"
#include "mshtmdid.h"
#include "mshtmcid.h"
#include "hlink.h"
#include "dispex.h"
#include "idispids.h"
#include "shlguid.h"
#include "perhist.h"
#include "shobjidl.h"
#include "mshtml_test.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
DEFINE_GUID(IID_IProxyManager,0x00000008,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_OLEGUID(CGID_DocHostCmdPriv, 0x000214D4L, 0, 0);

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define SET_CALLED(func) \
    called_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

static IOleDocumentView *view = NULL;
static HWND container_hwnd = NULL, hwnd = NULL, last_hwnd = NULL;

DEFINE_EXPECT(LockContainer);
DEFINE_EXPECT(SetActiveObject);
DEFINE_EXPECT(SetActiveObject_null);
DEFINE_EXPECT(GetWindow);
DEFINE_EXPECT(CanInPlaceActivate);
DEFINE_EXPECT(OnInPlaceActivate);
DEFINE_EXPECT(OnInPlaceActivateEx);
DEFINE_EXPECT(OnUIActivate);
DEFINE_EXPECT(GetWindowContext);
DEFINE_EXPECT(OnUIDeactivate);
DEFINE_EXPECT(OnInPlaceDeactivate);
DEFINE_EXPECT(OnInPlaceDeactivateEx);
DEFINE_EXPECT(GetContainer);
DEFINE_EXPECT(ShowUI);
DEFINE_EXPECT(ActivateMe);
DEFINE_EXPECT(GetHostInfo);
DEFINE_EXPECT(HideUI);
DEFINE_EXPECT(GetOptionKeyPath);
DEFINE_EXPECT(GetOverrideKeyPath);
DEFINE_EXPECT(SetStatusText);
DEFINE_EXPECT(QueryStatus_SETPROGRESSTEXT);
DEFINE_EXPECT(QueryStatus_OPEN);
DEFINE_EXPECT(QueryStatus_NEW);
DEFINE_EXPECT(Exec_SETPROGRESSMAX);
DEFINE_EXPECT(Exec_SETPROGRESSPOS);
DEFINE_EXPECT(Exec_HTTPEQUIV_DONE); 
DEFINE_EXPECT(Exec_SETDOWNLOADSTATE_0);
DEFINE_EXPECT(Exec_SETDOWNLOADSTATE_1);
DEFINE_EXPECT(Exec_ShellDocView_37);
DEFINE_EXPECT(Exec_ShellDocView_63);
DEFINE_EXPECT(Exec_ShellDocView_67);
DEFINE_EXPECT(Exec_ShellDocView_84);
DEFINE_EXPECT(Exec_ShellDocView_103);
DEFINE_EXPECT(Exec_ShellDocView_105);
DEFINE_EXPECT(Exec_ShellDocView_140);
DEFINE_EXPECT(Exec_UPDATECOMMANDS);
DEFINE_EXPECT(Exec_SETTITLE);
DEFINE_EXPECT(Exec_HTTPEQUIV);
DEFINE_EXPECT(Exec_MSHTML_PARSECOMPLETE);
DEFINE_EXPECT(Exec_Explorer_69);
DEFINE_EXPECT(Exec_DOCCANNAVIGATE);
DEFINE_EXPECT(Invoke_AMBIENT_USERMODE);
DEFINE_EXPECT(Invoke_AMBIENT_DLCONTROL);
DEFINE_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
DEFINE_EXPECT(Invoke_AMBIENT_SILENT);
DEFINE_EXPECT(Invoke_AMBIENT_USERAGENT);
DEFINE_EXPECT(Invoke_AMBIENT_PALETTE);
DEFINE_EXPECT(GetDropTarget);
DEFINE_EXPECT(UpdateUI);
DEFINE_EXPECT(Navigate);
DEFINE_EXPECT(OnFrameWindowActivate);
DEFINE_EXPECT(OnChanged_READYSTATE);
DEFINE_EXPECT(OnChanged_1005);
DEFINE_EXPECT(OnChanged_1012);
DEFINE_EXPECT(GetDisplayName);
DEFINE_EXPECT(BindToStorage);
DEFINE_EXPECT(IsSystemMoniker);
DEFINE_EXPECT(GetBindResult);
DEFINE_EXPECT(GetClassID);
DEFINE_EXPECT(Abort);
DEFINE_EXPECT(Read);
DEFINE_EXPECT(CreateInstance);
DEFINE_EXPECT(Start);
DEFINE_EXPECT(Terminate);
DEFINE_EXPECT(Protocol_Read);
DEFINE_EXPECT(LockRequest);
DEFINE_EXPECT(UnlockRequest);
DEFINE_EXPECT(OnFocus_TRUE);
DEFINE_EXPECT(OnFocus_FALSE);
DEFINE_EXPECT(RequestUIActivate);
DEFINE_EXPECT(InPlaceFrame_SetBorderSpace);
DEFINE_EXPECT(InPlaceUIWindow_SetActiveObject);
DEFINE_EXPECT(GetExternal);
DEFINE_EXPECT(EnableModeless_TRUE);
DEFINE_EXPECT(EnableModeless_FALSE);
DEFINE_EXPECT(Frame_EnableModeless_TRUE);
DEFINE_EXPECT(Frame_EnableModeless_FALSE);
DEFINE_EXPECT(Frame_GetWindow);
DEFINE_EXPECT(TranslateUrl);
DEFINE_EXPECT(Advise_Close);
DEFINE_EXPECT(OnViewChange);
DEFINE_EXPECT(EvaluateNewWindow);

static IUnknown *doc_unk;
static IMoniker *doc_mon;
static BOOL expect_LockContainer_fLock;
static BOOL expect_InPlaceUIWindow_SetActiveObject_active = TRUE;
static BOOL ipsex, ipsw;
static BOOL set_clientsite, container_locked, navigated_load;
static BOOL readystate_set_loading = FALSE, readystate_set_interactive = FALSE, load_from_stream;
static BOOL editmode = FALSE, show_failed;
static BOOL inplace_deactivated, open_call;
static int stream_read, protocol_read;
static enum load_state_t {
    LD_DOLOAD,
    LD_LOADING,
    LD_LOADED,
    LD_INTERACTIVE,
    LD_COMPLETE,
    LD_NO
} load_state;

static LPCOLESTR expect_status_text = NULL;

static const char html_page[] =
"<html>"
"<head><link rel=\"stylesheet\" type=\"text/css\" href=\"test.css\"></head>"
"<body>test</body>"
"</html>";

static const char css_data[] = "body {color: red}";

static const WCHAR http_urlW[] =
    {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g',0};

static const WCHAR doc_url[] = {'w','i','n','e','t','e','s','t',':','d','o','c',0};
static const WCHAR about_blank_url[] = {'a','b','o','u','t',':','b','l','a','n','k',0};

#define DOCHOST_DOCCANNAVIGATE 0

static HRESULT QueryInterface(REFIID riid, void **ppv);
static void test_MSHTML_QueryStatus(IHTMLDocument2*,DWORD);

#define test_readyState(u) _test_readyState(__LINE__,u)
static void _test_readyState(unsigned,IUnknown*);

static const WCHAR wszTimesNewRoman[] =
    {'T','i','m','e','s',' ','N','e','w',' ','R','o','m','a','n',0};
static const WCHAR wszArial[] =
    {'A','r','i','a','l',0};

static const char *debugstr_guid(REFIID riid)
{
    static char buf[50];

    if(!riid)
        return "(null)";

    sprintf(buf, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            riid->Data1, riid->Data2, riid->Data3, riid->Data4[0],
            riid->Data4[1], riid->Data4[2], riid->Data4[3], riid->Data4[4],
            riid->Data4[5], riid->Data4[6], riid->Data4[7]);

    return buf;
}

static int strcmp_wa(LPCWSTR strw, const char *stra)
{
    CHAR buf[512];
    WideCharToMultiByte(CP_ACP, 0, strw, -1, buf, sizeof(buf), NULL, NULL);
    return lstrcmpA(stra, buf);
}

static const WCHAR *strstrW( const WCHAR *str, const WCHAR *sub )
{
    while (*str)
    {
        const WCHAR *p1 = str, *p2 = sub;
        while (*p1 && *p2 && *p1 == *p2) { p1++; p2++; }
        if (!*p2) return str;
        str++;
    }
    return NULL;
}

static BSTR a2bstr(const char *str)
{
    BSTR ret;
    int len;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    ret = SysAllocStringLen(NULL, len);
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);

    return ret;
}

static BOOL is_english(void)
{
    return PRIMARYLANGID(GetSystemDefaultLangID()) == LANG_ENGLISH
        && PRIMARYLANGID(GetUserDefaultLangID()) == LANG_ENGLISH;
}

#define EXPECT_UPDATEUI  1
#define EXPECT_SETTITLE  2

static void test_timer(DWORD flags)
{
    BOOL *b = &called_Exec_SETTITLE;
    MSG msg;

    if(flags & EXPECT_UPDATEUI) {
        SET_EXPECT(UpdateUI);
        SET_EXPECT(Exec_UPDATECOMMANDS);
        b = &called_UpdateUI;
    }
    if(flags & EXPECT_SETTITLE)
        SET_EXPECT(Exec_SETTITLE);

    while(!*b && GetMessage(&msg, hwnd, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(flags & EXPECT_UPDATEUI) {
        CHECK_CALLED(UpdateUI);
        CHECK_CALLED(Exec_UPDATECOMMANDS);
    }
    if(flags & EXPECT_SETTITLE)
        CHECK_CALLED(Exec_SETTITLE);
}

static IMoniker Moniker;

#define test_GetCurMoniker(u,m,v) _test_GetCurMoniker(__LINE__,u,m,v)
static void _test_GetCurMoniker(unsigned line, IUnknown *unk, IMoniker *exmon, LPCWSTR exurl)
{
    IHTMLDocument2 *doc;
    IPersistMoniker *permon;
    IMoniker *mon = (void*)0xdeadbeef;
    BSTR doc_url = (void*)0xdeadbeef;
    HRESULT hres;

    if(open_call)
        return; /* FIXME */

    hres = IUnknown_QueryInterface(unk, &IID_IPersistMoniker, (void**)&permon);
    ok(hres == S_OK, "QueryInterface(IID_IPersistMoniker) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLDocument2, (void**)&doc);
    ok(hres == S_OK, "QueryInterface(IID_IHTMLDocument2) failed: %08x\n", hres);

    hres = IHTMLDocument2_get_URL(doc, &doc_url);
    ok(hres == S_OK, "get_URL failed: %08x\n", hres);

    hres = IPersistMoniker_GetCurMoniker(permon, &mon);
    IPersistMoniker_Release(permon);

    if(exmon) {
        LPOLESTR url;
        BOOL exb = expect_GetDisplayName;
        BOOL clb = called_GetDisplayName;

        ok_(__FILE__,line)(hres == S_OK, "GetCurrentMoniker failed: %08x\n", hres);
        ok_(__FILE__,line)(mon == exmon, "mon(%p) != exmon(%p)\n", mon, exmon);

        if(mon == &Moniker)
            SET_EXPECT(GetDisplayName);
        hres = IMoniker_GetDisplayName(mon, NULL, NULL, &url);
        ok(hres == S_OK, "GetDisplayName failed: %08x\n", hres);
        if(mon == &Moniker)
            CHECK_CALLED(GetDisplayName);
        expect_GetDisplayName = exb;
        called_GetDisplayName = clb;

        ok(!lstrcmpW(url, doc_url), "url != doc_url\n");
        CoTaskMemFree(url);
    }else if(exurl) {
        LPOLESTR url;

        ok_(__FILE__,line)(hres == S_OK, "GetCurrentMoniker failed: %08x\n", hres);

        hres = IMoniker_GetDisplayName(mon, NULL, NULL, &url);
        ok(hres == S_OK, "GetDisplayName failed: %08x\n", hres);

        ok(!lstrcmpW(url, exurl), "unexpected url %s\n", wine_dbgstr_w(url));
        ok(!lstrcmpW(url, doc_url), "url != doc_url\n");

        CoTaskMemFree(url);
    }else {
        ok(hres == E_UNEXPECTED,
           "GetCurrentMoniker failed: %08x, expected E_UNEXPECTED\n", hres);
        ok(mon == (IMoniker*)0xdeadbeef, "mon=%p\n", mon);
        ok(!lstrcmpW(doc_url, about_blank_url), "doc_url is not about:blank\n");
    }

    SysFreeString(doc_url);
    IHTMLDocument_Release(doc);
    if(mon && mon != (void*)0xdeadbeef)
        IMoniker_Release(mon);
}

DEFINE_GUID(IID_External_unk,0x30510406,0x98B5,0x11CF,0xBB,0x82,0x00,0xAA,0x00,0xBD,0xCE,0x0B);

static HRESULT WINAPI External_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IProxyManager, riid))
        return E_NOINTERFACE; /* TODO */
    if(IsEqualGUID(&IID_IDispatchEx, riid))
        return E_NOINTERFACE; /* TODO */
    if(IsEqualGUID(&IID_External_unk, riid))
        return E_NOINTERFACE; /* TODO */

    ok(0, "unexpected riid: %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI Dispatch_AddRef(IDispatch *iface)
{
    return 2;
}

static ULONG WINAPI Dispatch_Release(IDispatch *iface)
{
    return 1;
}

static HRESULT WINAPI Dispatch_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_GetTypeInfo(IDispatch *iface, UINT iTInfo, LCID lcid,
        ITypeInfo **ppTInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI External_Invoke(IDispatch *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IDispatchVtbl ExternalVtbl = {
    External_QueryInterface,
    Dispatch_AddRef,
    Dispatch_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    Dispatch_GetIDsOfNames,
    External_Invoke
};

static IDispatch External = { &ExternalVtbl };

static HRESULT WINAPI Protocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IInternetProtocol, riid)) {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Protocol_AddRef(IInternetProtocol *iface)
{
    return 2;
}

static ULONG WINAPI Protocol_Release(IInternetProtocol *iface)
{
    return 1;
}

static HRESULT WINAPI Protocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, HANDLE_PTR dwReserved)
{
    BINDINFO bindinfo;
    DWORD bindf = 0;
    HRESULT hres;

    static const WCHAR wszTextCss[] = {'t','e','x','t','/','c','s','s',0};
    static const WCHAR empty_str = {0};

    CHECK_EXPECT(Start);

    ok(pOIProtSink != NULL, "pOIProtSink == NULL\n");
    ok(pOIBindInfo != NULL, "pOIBindInfo == NULL\n");
    ok(!grfPI, "grfPI = %x\n", grfPI);
    ok(!dwReserved, "dwReserved = %lx\n", dwReserved);

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(bindinfo);
    hres = IInternetBindInfo_GetBindInfo(pOIBindInfo, &bindf, &bindinfo);
    ok(hres == S_OK, "GetBindInfo failed: %08x\n", hres);
    ok(bindf == (BINDF_FROMURLMON|BINDF_PULLDATA|BINDF_NEEDFILE|BINDF_ASYNCSTORAGE|BINDF_ASYNCHRONOUS),
       "bindf = %x\n", bindf);

    ok(bindinfo.cbSize == sizeof(bindinfo), "bindinfo.cbSize=%d\n", bindinfo.cbSize);
    ok(bindinfo.szExtraInfo == NULL, "bindinfo.szExtraInfo=%p\n", bindinfo.szExtraInfo);
    /* TODO: test stgmedData */
    ok(bindinfo.grfBindInfoF == 0, "bindinfo.grfBinfInfoF=%08x\n", bindinfo.grfBindInfoF);
    ok(bindinfo.dwBindVerb == 0, "bindinfo.dwBindVerb=%d\n", bindinfo.dwBindVerb);
    ok(bindinfo.szCustomVerb == 0, "bindinfo.szCustomVerb=%p\n", bindinfo.szCustomVerb);
    ok(bindinfo.cbstgmedData == 0, "bindinfo.cbstgmedData=%d\n", bindinfo.cbstgmedData);
    ok(bindinfo.dwOptions == 0x80000 ||
       bindinfo.dwOptions == 0x4080000, /* win2k3 */
       "bindinfo.dwOptions=%x\n", bindinfo.dwOptions);
    ok(bindinfo.dwOptionsFlags == 0, "bindinfo.dwOptionsFlags=%d\n", bindinfo.dwOptionsFlags);
    /* TODO: test dwCodePage */
    /* TODO: test securityAttributes */
    ok(IsEqualGUID(&IID_NULL, &bindinfo.iid), "unexpected bindinfo.iid\n");
    ok(bindinfo.pUnk == NULL, "bindinfo.pUnk=%p\n", bindinfo.pUnk);
    ok(bindinfo.dwReserved == 0, "bindinfo.dwReserved=%d\n", bindinfo.dwReserved);

    hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
            BINDSTATUS_CACHEFILENAMEAVAILABLE, &empty_str);
    ok(hres == S_OK, "ReportProgress(BINDSTATUS_CACHEFILENAMEAVAILABLE) failed: %08x\n", hres);

    hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
            BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE, wszTextCss);
    ok(hres == S_OK,
       "ReportProgress(BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE) failed: %08x\n", hres);

    hres = IInternetProtocolSink_ReportData(pOIProtSink,
            BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION, 13, 13);
    ok(hres == S_OK, "ReportData failed: %08x\n", hres);

    hres = IInternetProtocolSink_ReportResult(pOIProtSink, S_OK, 0, NULL);
    ok(hres == S_OK, "ReportResult failed: %08x\n", hres);

    return S_OK;
}

static HRESULT WINAPI Protocol_Continue(IInternetProtocol *iface,
        PROTOCOLDATA *pProtocolData)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    CHECK_EXPECT(Terminate);
    return S_OK;
}

static HRESULT WINAPI Protocol_Suspend(IInternetProtocol *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_Resume(IInternetProtocol *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_Read(IInternetProtocol *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    CHECK_EXPECT2(Protocol_Read);

    ok(pv != NULL, "pv == NULL\n");
    ok(cb > sizeof(css_data), "cb < sizeof(css_data)\n");
    ok(pcbRead != NULL, "pcbRead == NULL\n");
    ok(!*pcbRead, "*pcbRead=%d\n", *pcbRead);

    if(protocol_read)
        return S_FALSE;

    protocol_read += *pcbRead = sizeof(css_data)-1;
    memcpy(pv, css_data, sizeof(css_data)-1);
    return S_OK;
}

static HRESULT WINAPI Protocol_Seek(IInternetProtocol *iface,
        LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    CHECK_EXPECT(LockRequest);
    return S_OK;
}

static HRESULT WINAPI Protocol_UnlockRequest(IInternetProtocol *iface)
{
    CHECK_EXPECT(UnlockRequest);
    return S_OK;
}

static const IInternetProtocolVtbl ProtocolVtbl = {
    Protocol_QueryInterface,
    Protocol_AddRef,
    Protocol_Release,
    Protocol_Start,
    Protocol_Continue,
    Protocol_Abort,
    Protocol_Terminate,
    Protocol_Suspend,
    Protocol_Resume,
    Protocol_Read,
    Protocol_Seek,
    Protocol_LockRequest,
    Protocol_UnlockRequest
};

static IInternetProtocol Protocol = { &ProtocolVtbl };

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    if(!IsEqualGUID(&IID_IInternetProtocolInfo, riid))
        ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *pOuter,
                                        REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IInternetProtocolInfo, riid))
        return E_NOINTERFACE;

    CHECK_EXPECT(CreateInstance);
    ok(ppv != NULL, "ppv == NULL\n");

    *ppv = &Protocol;
    return S_OK;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

static const IClassFactoryVtbl ClassFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory ClassFactory = { &ClassFactoryVtbl };

static HRESULT WINAPI HlinkFrame_QueryInterface(IHlinkFrame *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI HlinkFrame_AddRef(IHlinkFrame *iface)
{
    return 2;
}

static ULONG WINAPI HlinkFrame_Release(IHlinkFrame *iface)
{
    return 1;
}

static HRESULT WINAPI HlinkFrame_SetBrowseContext(IHlinkFrame *iface,
                                                  IHlinkBrowseContext *pihlbc)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkFrame_GetBrowseContext(IHlinkFrame *iface,
                                                  IHlinkBrowseContext **ppihlbc)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkFrame_Navigate(IHlinkFrame *iface, DWORD grfHLNF, LPBC pbc,
                                          IBindStatusCallback *pibsc, IHlink *pihlNavigate)
{
    HRESULT hres;

    CHECK_EXPECT(Navigate);

    ok(grfHLNF == 0, "grfHLNF=%d, expected 0\n", grfHLNF);
    ok(pbc != NULL, "pbc == NULL\n");
    ok(pibsc != NULL, "pubsc == NULL\n");
    ok(pihlNavigate != NULL, "puhlNavigate == NULL\n");

    if(pihlNavigate) {
        LPWSTR frame_name = (LPWSTR)0xdeadbeef;
        LPWSTR location = (LPWSTR)0xdeadbeef;
        IHlinkSite *site;
        IMoniker *mon = NULL;
        DWORD site_data = 0xdeadbeef;

        hres = IHlink_GetTargetFrameName(pihlNavigate, &frame_name);
        todo_wine
        ok(hres == S_FALSE, "GetTargetFrameName failed: %08x\n", hres);
        ok(frame_name == NULL, "frame_name = %p\n", frame_name);

        hres = IHlink_GetMonikerReference(pihlNavigate, 1, &mon, &location);
        ok(hres == S_OK, "GetMonikerReference failed: %08x\n", hres);
        ok(location == NULL, "location = %p\n", location);
        ok(mon != NULL, "mon == NULL\n");

        hres = IHlink_GetHlinkSite(pihlNavigate, &site, &site_data);
        ok(hres == S_OK, "GetHlinkSite failed: %08x\n", hres);
        ok(site == NULL, "site = %p\n, expected NULL\n", site);
        todo_wine
        ok(site_data == 0xdeadbeef, "site_data = %x\n", site_data);
    }

    return S_OK;
}

static HRESULT WINAPI HlinkFrame_OnNavigate(IHlinkFrame *iface, DWORD grfHLNF,
        IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName, DWORD dwreserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkFrame_UpdateHlink(IHlinkFrame *iface, ULONG uHLID,
        IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IHlinkFrameVtbl HlinkFrameVtbl = {
    HlinkFrame_QueryInterface,
    HlinkFrame_AddRef,
    HlinkFrame_Release,
    HlinkFrame_SetBrowseContext,
    HlinkFrame_GetBrowseContext,
    HlinkFrame_Navigate,
    HlinkFrame_OnNavigate,
    HlinkFrame_UpdateHlink
};

static IHlinkFrame HlinkFrame = { &HlinkFrameVtbl };

static HRESULT WINAPI NewWindowManager_QueryInterface(INewWindowManager *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI NewWindowManager_AddRef(INewWindowManager *iface)
{
    return 2;
}

static ULONG WINAPI NewWindowManager_Release(INewWindowManager *iface)
{
    return 1;
}

static HRESULT WINAPI NewWindowManager_EvaluateNewWindow(INewWindowManager *iface, LPCWSTR pszUrl,
        LPCWSTR pszName, LPCWSTR pszUrlContext, LPCWSTR pszFeatures, BOOL fReplace, DWORD dwFlags,
        DWORD dwUserActionTime)
{
    CHECK_EXPECT(EvaluateNewWindow);

    ok(!strcmp_wa(pszUrl, "about:blank"), "pszUrl = %s\n", wine_dbgstr_w(pszUrl));
    ok(!strcmp_wa(pszName, "test"), "pszName = %s\n", wine_dbgstr_w(pszName));
    ok(!strcmp_wa(pszUrlContext, "about:blank"), "pszUrlContext = %s\n", wine_dbgstr_w(pszUrlContext));
    ok(!pszFeatures, "pszFeatures = %s\n", wine_dbgstr_w(pszFeatures));
    ok(!fReplace, "fReplace = %x\n", fReplace);
    ok(dwFlags == NWMF_FIRST, "dwFlags = %x\n", dwFlags);
    ok(!dwUserActionTime, "dwUserActionime = %d\n", dwUserActionTime);

    return E_FAIL;
}

static const INewWindowManagerVtbl NewWindowManagerVtbl = {
    NewWindowManager_QueryInterface,
    NewWindowManager_AddRef,
    NewWindowManager_Release,
    NewWindowManager_EvaluateNewWindow
};

static INewWindowManager NewWindowManager = { &NewWindowManagerVtbl };

static HRESULT WINAPI PropertyNotifySink_QueryInterface(IPropertyNotifySink *iface,
        REFIID riid, void**ppv)
{
    if(IsEqualGUID(&IID_IPropertyNotifySink, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI PropertyNotifySink_AddRef(IPropertyNotifySink *iface)
{
    return 2;
}

static ULONG WINAPI PropertyNotifySink_Release(IPropertyNotifySink *iface)
{
    return 1;
}

static HRESULT WINAPI PropertyNotifySink_OnChanged(IPropertyNotifySink *iface, DISPID dispID)
{
    switch(dispID) {
    case DISPID_READYSTATE:
        CHECK_EXPECT2(OnChanged_READYSTATE);

        if(!readystate_set_interactive)
            test_MSHTML_QueryStatus(NULL, OLECMDF_SUPPORTED
                | (editmode && (load_state == LD_INTERACTIVE || load_state == LD_COMPLETE)
                   ? OLECMDF_ENABLED : 0));

        if(readystate_set_loading) {
            readystate_set_loading = FALSE;
            load_state = LD_LOADING;
        }
        if(!editmode || load_state != LD_LOADING || !called_Exec_Explorer_69)
            test_readyState(NULL);
        return S_OK;
    case 1005:
        CHECK_EXPECT(OnChanged_1005);
        if(!editmode)
            test_readyState(NULL);
        readystate_set_interactive = (load_state != LD_INTERACTIVE);
        return S_OK;
    case 1012:
        CHECK_EXPECT(OnChanged_1012);
        return S_OK;
    case 1030:
    case 3000029:
    case 3000030:
        /* TODO */
        return S_OK;
    }

    ok(0, "unexpected id %d\n", dispID);
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyNotifySink_OnRequestEdit(IPropertyNotifySink *iface, DISPID dispID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IPropertyNotifySinkVtbl PropertyNotifySinkVtbl = {
    PropertyNotifySink_QueryInterface,
    PropertyNotifySink_AddRef,
    PropertyNotifySink_Release,
    PropertyNotifySink_OnChanged,
    PropertyNotifySink_OnRequestEdit
};

static IPropertyNotifySink PropertyNotifySink = { &PropertyNotifySinkVtbl };

static HRESULT WINAPI Stream_QueryInterface(IStream *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI Stream_AddRef(IStream *iface)
{
    return 2;
}

static ULONG WINAPI Stream_Release(IStream *iface)
{
    return 1;
}

static HRESULT WINAPI Stream_Read(IStream *iface, void *pv,
                                  ULONG cb, ULONG *pcbRead)
{
    CHECK_EXPECT2(Read);
    ok(pv != NULL, "pv == NULL\n");
    ok(cb > sizeof(html_page), "cb = %d\n", cb);
    ok(pcbRead != NULL, "pcbRead == NULL\n");
    ok(!*pcbRead, "*pcbRead = %d\n", *pcbRead);

    if(stream_read)
        return S_FALSE;

    memcpy(pv, html_page, sizeof(html_page)-1);
    stream_read += *pcbRead = sizeof(html_page)-1;
    return S_OK;
}

static HRESULT WINAPI Stream_Write(IStream *iface, const void *pv,
                                          ULONG cb, ULONG *pcbWritten)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_Seek(IStream *iface, LARGE_INTEGER dlibMove,
                                         DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_SetSize(IStream *iface, ULARGE_INTEGER libNewSize)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_CopyTo(IStream *iface, IStream *pstm,
        ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_Commit(IStream *iface, DWORD grfCommitFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_Revert(IStream *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_LockRegion(IStream *iface, ULARGE_INTEGER libOffset,
                                               ULARGE_INTEGER cb, DWORD dwLockType)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_UnlockRegion(IStream *iface,
        ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_Stat(IStream *iface, STATSTG *pstatstg,
                                         DWORD dwStatFlag)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Stream_Clone(IStream *iface, IStream **ppstm)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IStreamVtbl StreamVtbl = {
    Stream_QueryInterface,
    Stream_AddRef,
    Stream_Release,
    Stream_Read,
    Stream_Write,
    Stream_Seek,
    Stream_SetSize,
    Stream_CopyTo,
    Stream_Commit,
    Stream_Revert,
    Stream_LockRegion,
    Stream_UnlockRegion,
    Stream_Stat,
    Stream_Clone
};

static IStream Stream = { &StreamVtbl };

static HRESULT WINAPI Binding_QueryInterface(IBinding *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IWinInetHttpInfo, riid))
        return E_NOINTERFACE; /* TODO */

    if(IsEqualGUID(&IID_IWinInetInfo, riid))
        return E_NOINTERFACE; /* TODO */

    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI Binding_AddRef(IBinding *iface)
{
    return 2;
}

static ULONG WINAPI Binding_Release(IBinding *iface)
{
    return 1;
}

static HRESULT WINAPI Binding_Abort(IBinding *iface)
{
    CHECK_EXPECT(Abort);
    return S_OK;
}

static HRESULT WINAPI Binding_Suspend(IBinding *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_Resume(IBinding *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_SetPriority(IBinding *iface, LONG nPriority)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_GetPriority(IBinding *iface, LONG *pnPriority)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Binding_GetBindResult(IBinding *iface, CLSID *pclsidProtocol,
        DWORD *pdwResult, LPOLESTR *pszResult, DWORD *pdwReserved)
{
    CHECK_EXPECT(GetBindResult);
    return E_NOTIMPL;
}

static const IBindingVtbl BindingVtbl = {
    Binding_QueryInterface,
    Binding_AddRef,
    Binding_Release,
    Binding_Abort,
    Binding_Suspend,
    Binding_Resume,
    Binding_SetPriority,
    Binding_GetPriority,
    Binding_GetBindResult
};

static IBinding Binding = { &BindingVtbl };

DEFINE_GUID(IID_IMoniker_unk,0xA158A630,0xED6F,0x45FB,0xB9,0x87,0xF6,0x86,0x76,0xF5,0x77,0x52);
DEFINE_GUID(IID_IMoniker_unk2, 0x79EAC9D3,0xBAF9,0x11CE,0x8C,0x82,0x00,0xAA,0x00,0x4B,0xA9,0x0B);

static HRESULT WINAPI Moniker_QueryInterface(IMoniker *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IMoniker_unk, riid))
        return E_NOINTERFACE; /* TODO */
    if(IsEqualGUID(&IID_IMoniker_unk2, riid))
        return E_NOINTERFACE; /* TODO */

    ok(0, "unexpected riid: %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI Moniker_AddRef(IMoniker *iface)
{
    return 2;
}

static ULONG WINAPI Moniker_Release(IMoniker *iface)
{
    return 1;
}

static HRESULT WINAPI Moniker_GetClassID(IMoniker *iface, CLSID *pClassID)
{
    CHECK_EXPECT(GetClassID);
    ok(IsEqualGUID(pClassID, &IID_NULL), "pClassID = %s\n", debugstr_guid(pClassID));
    return E_FAIL;
}

static HRESULT WINAPI Moniker_IsDirty(IMoniker *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_Load(IMoniker *iface, IStream *pStm)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_Save(IMoniker *iface, IStream *pStm, BOOL fClearDirty)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_GetSizeMax(IMoniker *iface, ULARGE_INTEGER *pcbSize)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_BindToObject(IMoniker *iface, IBindCtx *pcb, IMoniker *pmkToLeft,
        REFIID riidResult, void **ppvResult)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_BindToStorage(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft,
        REFIID riid, void **ppv)
{
    IBindStatusCallback *callback = NULL;
    FORMATETC formatetc = {0xc02d, NULL, 1, -1, TYMED_ISTREAM};
    STGMEDIUM stgmedium;
    BINDINFO bindinfo;
    DWORD bindf;
    HRESULT hres;

    static OLECHAR BSCBHolder[] = { '_','B','S','C','B','_','H','o','l','d','e','r','_',0 };
    static const WCHAR wszTextHtml[] = {'t','e','x','t','/','h','t','m','l',0};

    CHECK_EXPECT(BindToStorage);

    load_state = LD_LOADING;

    ok(pbc != NULL, "pbc == NULL\n");
    ok(pmkToLeft == NULL, "pmkToLeft=%p\n", pmkToLeft);
    ok(IsEqualGUID(&IID_IStream, riid), "unexpected riid\n");
    ok(ppv != NULL, "ppv == NULL\n");
    ok(*ppv == NULL, "*ppv=%p\n", *ppv);

    hres = IBindCtx_GetObjectParam(pbc, BSCBHolder, (IUnknown**)&callback);
    ok(hres == S_OK, "GetObjectParam failed: %08x\n", hres);
    ok(callback != NULL, "callback == NULL\n");

    memset(&bindinfo, 0xf0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(bindinfo);

    hres = IBindStatusCallback_GetBindInfo(callback, &bindf, &bindinfo);
    ok(hres == S_OK, "GetBindInfo failed: %08x\n", hres);
    ok(bindf == (BINDF_PULLDATA|BINDF_ASYNCSTORAGE|BINDF_ASYNCHRONOUS), "bindf = %08x\n", bindf);
    ok(bindinfo.cbSize == sizeof(bindinfo), "bindinfo.cbSize=%d\n", bindinfo.cbSize);
    ok(bindinfo.szExtraInfo == NULL, "bindinfo.szExtraInfo=%p\n", bindinfo.szExtraInfo);
    /* TODO: test stgmedData */
    ok(bindinfo.grfBindInfoF == 0, "bindinfo.grfBinfInfoF=%08x\n", bindinfo.grfBindInfoF);
    ok(bindinfo.dwBindVerb == 0, "bindinfo.dwBindVerb=%d\n", bindinfo.dwBindVerb);
    ok(bindinfo.szCustomVerb == 0, "bindinfo.szCustomVerb=%p\n", bindinfo.szCustomVerb);
    ok(bindinfo.cbstgmedData == 0, "bindinfo.cbstgmedData=%d\n", bindinfo.cbstgmedData);
    ok(bindinfo.dwOptions == 0x80000 || bindinfo.dwOptions == 0x4080000,
       "bindinfo.dwOptions=%x\n", bindinfo.dwOptions);
    ok(bindinfo.dwOptionsFlags == 0, "bindinfo.dwOptionsFlags=%d\n", bindinfo.dwOptionsFlags);
    /* TODO: test dwCodePage */
    /* TODO: test securityAttributes */
    ok(IsEqualGUID(&IID_NULL, &bindinfo.iid), "unexpected bindinfo.iid\n");
    ok(bindinfo.pUnk == NULL, "bindinfo.pUnk=%p\n", bindinfo.pUnk);
    ok(bindinfo.dwReserved == 0, "bindinfo.dwReserved=%d\n", bindinfo.dwReserved);

    hres = IBindStatusCallback_OnStartBinding(callback, 0, &Binding);
    ok(hres == S_OK, "OnStartBinding failed: %08x\n", hres);

    hres = IBindStatusCallback_OnProgress(callback, 0, 0, BINDSTATUS_MIMETYPEAVAILABLE,
                                          wszTextHtml);
    ok(hres == S_OK, "OnProgress(BINDSTATUS_MIMETYPEAVAILABLE) failed: %08x\n", hres);

    hres = IBindStatusCallback_OnProgress(callback, sizeof(html_page)-1, sizeof(html_page)-1,
                                          BINDSTATUS_BEGINDOWNLOADDATA, doc_url);
    ok(hres == S_OK, "OnProgress(BINDSTATUS_BEGINDOWNLOADDATA) failed: %08x\n", hres);

    SET_EXPECT(Read);
    stgmedium.tymed = TYMED_ISTREAM;
    U(stgmedium).pstm = &Stream;
    stgmedium.pUnkForRelease = (IUnknown*)iface;
    hres = IBindStatusCallback_OnDataAvailable(callback,
            BSCF_FIRSTDATANOTIFICATION|BSCF_LASTDATANOTIFICATION,
            sizeof(html_page)-1, &formatetc, &stgmedium);
    ok(hres == S_OK, "OnDataAvailable failed: %08x\n", hres);
    CHECK_CALLED(Read);

    hres = IBindStatusCallback_OnProgress(callback, sizeof(html_page)-1, sizeof(html_page)-1,
            BINDSTATUS_ENDDOWNLOADDATA, NULL);
    ok(hres == S_OK, "OnProgress(BINDSTATUS_ENDDOWNLOADDATA) failed: %08x\n", hres);

    SET_EXPECT(GetBindResult);
    hres = IBindStatusCallback_OnStopBinding(callback, S_OK, NULL);
    ok(hres == S_OK, "OnStopBinding failed: %08x\n", hres);
    SET_CALLED(GetBindResult); /* IE7 */

    IBindStatusCallback_Release(callback);

    return S_OK;
}

static HRESULT WINAPI Moniker_Reduce(IMoniker *iface, IBindCtx *pbc, DWORD dwReduceHowFar,
        IMoniker **ppmkToLeft, IMoniker **ppmkReduced)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_ComposeWith(IMoniker *iface, IMoniker *pmkRight,
        BOOL fOnlyIfNotGeneric, IMoniker **ppnkComposite)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_Enum(IMoniker *iface, BOOL fForwrd, IEnumMoniker **ppenumMoniker)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_IsEqual(IMoniker *iface, IMoniker *pmkOtherMoniker)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_Hash(IMoniker *iface, DWORD *pdwHash)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_IsRunning(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft,
        IMoniker *pmkNewlyRunning)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_GetTimeOfLastChange(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, FILETIME *pFileTime)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_Inverse(IMoniker *iface, IMoniker **ppmk)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_CommonPrefixWith(IMoniker *iface, IMoniker *pmkOther,
        IMoniker **ppmkPrefix)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_RelativePathTo(IMoniker *iface, IMoniker *pmkOther,
        IMoniker **pmkRelPath)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_GetDisplayName(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, LPOLESTR *ppszDisplayName)
{
    CHECK_EXPECT2(GetDisplayName);

    /* ok(pbc != NULL, "pbc == NULL\n"); */
    ok(pmkToLeft == NULL, "pmkToLeft=%p\n", pmkToLeft);
    ok(ppszDisplayName != NULL, "ppszDisplayName == NULL\n");

    *ppszDisplayName = CoTaskMemAlloc(sizeof(doc_url));
    memcpy(*ppszDisplayName, doc_url, sizeof(doc_url));

    return S_OK;
}

static HRESULT WINAPI Moniker_ParseDisplayName(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, LPOLESTR pszDisplayName, ULONG *pchEaten, IMoniker **ppmkOut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Moniker_IsSystemMoniker(IMoniker *iface, DWORD *pdwMksys)
{
    CHECK_EXPECT(IsSystemMoniker);
    return E_NOTIMPL;
}

static const IMonikerVtbl MonikerVtbl = {
    Moniker_QueryInterface,
    Moniker_AddRef,
    Moniker_Release,
    Moniker_GetClassID,
    Moniker_IsDirty,
    Moniker_Load,
    Moniker_Save,
    Moniker_GetSizeMax,
    Moniker_BindToObject,
    Moniker_BindToStorage,
    Moniker_Reduce,
    Moniker_ComposeWith,
    Moniker_Enum,
    Moniker_IsEqual,
    Moniker_Hash,
    Moniker_IsRunning,
    Moniker_GetTimeOfLastChange,
    Moniker_Inverse,
    Moniker_CommonPrefixWith,
    Moniker_RelativePathTo,
    Moniker_GetDisplayName,
    Moniker_ParseDisplayName,
    Moniker_IsSystemMoniker
};

static IMoniker Moniker = { &MonikerVtbl };

static HRESULT WINAPI OleContainer_QueryInterface(IOleContainer *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI OleContainer_AddRef(IOleContainer *iface)
{
    return 2;
}

static ULONG WINAPI OleContainer_Release(IOleContainer *iface)
{
    return 1;
}

static HRESULT WINAPI OleContainer_ParseDisplayName(IOleContainer *iface, IBindCtx *pbc,
        LPOLESTR pszDiaplayName, ULONG *pchEaten, IMoniker **ppmkOut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleContainer_EnumObjects(IOleContainer *iface, DWORD grfFlags,
        IEnumUnknown **ppenum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleContainer_LockContainer(IOleContainer *iface, BOOL fLock)
{
    CHECK_EXPECT(LockContainer);
    ok(expect_LockContainer_fLock == fLock, "fLock=%x, expected %x\n", fLock, expect_LockContainer_fLock);
    return S_OK;
}

static const IOleContainerVtbl OleContainerVtbl = {
    OleContainer_QueryInterface,
    OleContainer_AddRef,
    OleContainer_Release,
    OleContainer_ParseDisplayName,
    OleContainer_EnumObjects,
    OleContainer_LockContainer
};

static IOleContainer OleContainer = { &OleContainerVtbl };

static HRESULT WINAPI ClientSite_QueryInterface(IOleClientSite *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI ClientSite_AddRef(IOleClientSite *iface)
{
    return 2;
}

static ULONG WINAPI ClientSite_Release(IOleClientSite *iface)
{
    return 1;
}

static HRESULT WINAPI ClientSite_SaveObject(IOleClientSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_GetMoniker(IOleClientSite *iface, DWORD dwAssign, DWORD dwWhichMoniker,
        IMoniker **ppmon)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_GetContainer(IOleClientSite *iface, IOleContainer **ppContainer)
{
    CHECK_EXPECT(GetContainer);
    ok(ppContainer != NULL, "ppContainer = NULL\n");
    *ppContainer = &OleContainer;
    return S_OK;
}

static HRESULT WINAPI ClientSite_ShowObject(IOleClientSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_OnShowWindow(IOleClientSite *iface, BOOL fShow)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_RequestNewObjectLayout(IOleClientSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleClientSiteVtbl ClientSiteVtbl = {
    ClientSite_QueryInterface,
    ClientSite_AddRef,
    ClientSite_Release,
    ClientSite_SaveObject,
    ClientSite_GetMoniker,
    ClientSite_GetContainer,
    ClientSite_ShowObject,
    ClientSite_OnShowWindow,
    ClientSite_RequestNewObjectLayout
};

static IOleClientSite ClientSite = { &ClientSiteVtbl };

static HRESULT WINAPI InPlaceFrame_QueryInterface(IOleInPlaceFrame *iface, REFIID riid, void **ppv)
{
    static const GUID undocumented_frame_iid = {0xfbece6c9,0x48d7,0x4a37,{0x8f,0xe3,0x6a,0xd4,0x27,0x2f,0xdd,0xac}};

    if(!IsEqualGUID(&undocumented_frame_iid, riid))
        ok(0, "unexpected riid %s\n", debugstr_guid(riid));

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI InPlaceFrame_AddRef(IOleInPlaceFrame *iface)
{
    return 2;
}

static ULONG WINAPI InPlaceFrame_Release(IOleInPlaceFrame *iface)
{
    return 1;
}

static HRESULT WINAPI InPlaceFrame_GetWindow(IOleInPlaceFrame *iface, HWND *phwnd)
{
    CHECK_EXPECT(Frame_GetWindow);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_ContextSensitiveHelp(IOleInPlaceFrame *iface, BOOL fEnterMode)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_GetBorder(IOleInPlaceFrame *iface, LPRECT lprectBorder)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_RequestBorderSpace(IOleInPlaceFrame *iface,
        LPCBORDERWIDTHS pborderwidths)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetBorderSpace(IOleInPlaceFrame *iface,
        LPCBORDERWIDTHS pborderwidths)
{
    CHECK_EXPECT(InPlaceFrame_SetBorderSpace);
    return S_OK;
}

static HRESULT WINAPI InPlaceUIWindow_SetActiveObject(IOleInPlaceFrame *iface,
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    static const WCHAR wszHTML_Document[] =
        {'H','T','M','L',' ','D','o','c','u','m','e','n','t',0};

    CHECK_EXPECT2(InPlaceUIWindow_SetActiveObject);

    if(expect_InPlaceUIWindow_SetActiveObject_active) {
        ok(pActiveObject != NULL, "pActiveObject = NULL\n");
        if(pActiveObject && is_english())
            ok(!lstrcmpW(wszHTML_Document, pszObjName), "pszObjName != \"HTML Document\"\n");
    }
    else {
        ok(pActiveObject == NULL, "pActiveObject=%p, expected NULL\n", pActiveObject);
        ok(pszObjName == NULL, "pszObjName=%p, expected NULL\n", pszObjName);
    }
    expect_InPlaceUIWindow_SetActiveObject_active = !expect_InPlaceUIWindow_SetActiveObject_active;
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_SetActiveObject(IOleInPlaceFrame *iface,
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    static const WCHAR wszHTML_Document[] =
        {'H','T','M','L',' ','D','o','c','u','m','e','n','t',0};

    if(pActiveObject) {
        CHECK_EXPECT2(SetActiveObject);

        if(pActiveObject && is_english())
            ok(!lstrcmpW(wszHTML_Document, pszObjName), "pszObjName != \"HTML Document\"\n");
    }else {
        CHECK_EXPECT(SetActiveObject_null);

        ok(pActiveObject == NULL, "pActiveObject=%p, expected NULL\n", pActiveObject);
        ok(pszObjName == NULL, "pszObjName=%p, expected NULL\n", pszObjName);
    }

    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_InsertMenus(IOleInPlaceFrame *iface, HMENU hmenuShared,
        LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetMenu(IOleInPlaceFrame *iface, HMENU hmenuShared,
        HOLEMENU holemenu, HWND hwndActiveObject)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_RemoveMenus(IOleInPlaceFrame *iface, HMENU hmenuShared)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetStatusText(IOleInPlaceFrame *iface, LPCOLESTR pszStatusText)
{
    CHECK_EXPECT2(SetStatusText);
    if(!expect_status_text)
        ok(pszStatusText == NULL, "pszStatusText=%p, expected NULL\n", pszStatusText);
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_EnableModeless(IOleInPlaceFrame *iface, BOOL fEnable)
{
    if(fEnable)
        CHECK_EXPECT2(Frame_EnableModeless_TRUE);
    else
        CHECK_EXPECT2(Frame_EnableModeless_FALSE);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_TranslateAccelerator(IOleInPlaceFrame *iface, LPMSG lpmsg, WORD wID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleInPlaceFrameVtbl InPlaceFrameVtbl = {
    InPlaceFrame_QueryInterface,
    InPlaceFrame_AddRef,
    InPlaceFrame_Release,
    InPlaceFrame_GetWindow,
    InPlaceFrame_ContextSensitiveHelp,
    InPlaceFrame_GetBorder,
    InPlaceFrame_RequestBorderSpace,
    InPlaceFrame_SetBorderSpace,
    InPlaceFrame_SetActiveObject,
    InPlaceFrame_InsertMenus,
    InPlaceFrame_SetMenu,
    InPlaceFrame_RemoveMenus,
    InPlaceFrame_SetStatusText,
    InPlaceFrame_EnableModeless,
    InPlaceFrame_TranslateAccelerator
};

static IOleInPlaceFrame InPlaceFrame = { &InPlaceFrameVtbl };

static const IOleInPlaceFrameVtbl InPlaceUIWindowVtbl = {
    InPlaceFrame_QueryInterface,
    InPlaceFrame_AddRef,
    InPlaceFrame_Release,
    InPlaceFrame_GetWindow,
    InPlaceFrame_ContextSensitiveHelp,
    InPlaceFrame_GetBorder,
    InPlaceFrame_RequestBorderSpace,
    InPlaceFrame_SetBorderSpace,
    InPlaceUIWindow_SetActiveObject,
};

static IOleInPlaceFrame InPlaceUIWindow = { &InPlaceUIWindowVtbl };

static HRESULT WINAPI InPlaceSiteWindowless_QueryInterface(IOleInPlaceSiteWindowless *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI InPlaceSiteWindowless_AddRef(IOleInPlaceSiteWindowless *iface)
{
    return 2;
}

static ULONG WINAPI InPlaceSiteWindowless_Release(IOleInPlaceSiteWindowless *iface)
{
    return 1;
}

static HRESULT WINAPI InPlaceSiteWindowless_GetWindow(
        IOleInPlaceSiteWindowless *iface, HWND *phwnd)
{
    IOleClientSite *client_site;
    IOleObject *ole_obj;
    HRESULT hres;

    CHECK_EXPECT2(GetWindow);
    ok(phwnd != NULL, "phwnd = NULL\n");
    *phwnd = container_hwnd;

    hres = IUnknown_QueryInterface(doc_unk, &IID_IOleObject, (void**)&ole_obj);
    ok(hres == S_OK, "Could not get IOleObject: %08x\n", hres);

    hres = IOleObject_GetClientSite(ole_obj, &client_site);
    IOleObject_Release(ole_obj);
    ok(hres == S_OK, "GetClientSite failed: %08x\n", hres);
    ok(client_site == &ClientSite, "client_site != ClientSite\n");
    IOleClientSite_Release(client_site);

    return S_OK;
}

static HRESULT WINAPI InPlaceSiteWindowless_ContextSensitiveHelp(
        IOleInPlaceSiteWindowless *iface, BOOL fEnterMode)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_CanInPlaceActivate(
        IOleInPlaceSiteWindowless *iface)
{
    CHECK_EXPECT(CanInPlaceActivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSiteWindowless_OnInPlaceActivate(
        IOleInPlaceSiteWindowless *iface)
{
    CHECK_EXPECT(OnInPlaceActivate);
    inplace_deactivated = FALSE;
    return S_OK;
}

static HRESULT WINAPI InPlaceSiteWindowless_OnUIActivate(
        IOleInPlaceSiteWindowless *iface)
{
    CHECK_EXPECT(OnUIActivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSiteWindowless_GetWindowContext(
        IOleInPlaceSiteWindowless *iface, IOleInPlaceFrame **ppFrame,
        IOleInPlaceUIWindow **ppDoc, LPRECT lprcPosRect,
        LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    static const RECT rect = {0,0,500,500};

    CHECK_EXPECT(GetWindowContext);

    ok(ppFrame != NULL, "ppFrame = NULL\n");
    if(ppFrame)
        *ppFrame = &InPlaceFrame;
    ok(ppDoc != NULL, "ppDoc = NULL\n");
    if(ppDoc)
        *ppDoc = (IOleInPlaceUIWindow*)&InPlaceUIWindow;
    ok(lprcPosRect != NULL, "lprcPosRect = NULL\n");
    if(lprcPosRect)
        memcpy(lprcPosRect, &rect, sizeof(RECT));
    ok(lprcClipRect != NULL, "lprcClipRect = NULL\n");
    if(lprcClipRect)
        memcpy(lprcClipRect, &rect, sizeof(RECT));
    ok(lpFrameInfo != NULL, "lpFrameInfo = NULL\n");
    if(lpFrameInfo) {
        lpFrameInfo->cb = sizeof(*lpFrameInfo);
        lpFrameInfo->fMDIApp = FALSE;
        lpFrameInfo->hwndFrame = container_hwnd;
        lpFrameInfo->haccel = NULL;
        lpFrameInfo->cAccelEntries = 0;
    }

    return S_OK;
}

static HRESULT WINAPI InPlaceSiteWindowless_Scroll(
        IOleInPlaceSiteWindowless *iface, SIZE scrollExtent)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_OnUIDeactivate(
        IOleInPlaceSiteWindowless *iface, BOOL fUndoable)
{
    CHECK_EXPECT(OnUIDeactivate);
    ok(!fUndoable, "fUndoable = TRUE\n");
    return S_OK;
}

static HRESULT WINAPI InPlaceSiteWindowless_OnInPlaceDeactivate(
        IOleInPlaceSiteWindowless *iface)
{
    CHECK_EXPECT(OnInPlaceDeactivate);
    inplace_deactivated = TRUE;
    return S_OK;
}

static HRESULT WINAPI InPlaceSiteWindowless_DiscardUndoState(
        IOleInPlaceSiteWindowless *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_DeactivateAndUndo(
        IOleInPlaceSiteWindowless *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_OnPosRectChange(
        IOleInPlaceSiteWindowless *iface, LPCRECT lprcPosRect)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_OnInPlaceActivateEx(
        IOleInPlaceSiteWindowless *iface, BOOL *pfNoRedraw, DWORD dwFlags)
{
    CHECK_EXPECT(OnInPlaceActivateEx);

    ok(pfNoRedraw != NULL, "pfNoRedraw == NULL\n");
    ok(!*pfNoRedraw, "*pfNoRedraw == TRUE\n");
    ok(dwFlags == 0, "dwFlags = %08x\n", dwFlags);

    return S_OK;
}

static HRESULT WINAPI InPlaceSiteWindowless_OnInPlaceDeactivateEx(
        IOleInPlaceSiteWindowless *iface, BOOL fNoRedraw)
{
    CHECK_EXPECT(OnInPlaceDeactivateEx);

    ok(fNoRedraw, "fNoRedraw == FALSE\n");

    return S_OK;
}

static HRESULT WINAPI InPlaceSiteWindowless_RequestUIActivate(
        IOleInPlaceSiteWindowless *iface)
{
    CHECK_EXPECT2(RequestUIActivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSiteWindowless_CanWindowlessActivate(
        IOleInPlaceSiteWindowless *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_GetCapture(
        IOleInPlaceSiteWindowless *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_SetCapture(
        IOleInPlaceSiteWindowless *iface, BOOL fCapture)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_GetFocus(
        IOleInPlaceSiteWindowless *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_SetFocus(
        IOleInPlaceSiteWindowless *iface, BOOL fFocus)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_GetDC(
        IOleInPlaceSiteWindowless *iface, LPCRECT pRect,
        DWORD grfFlags, HDC *phDC)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_ReleaseDC(
        IOleInPlaceSiteWindowless *iface, HDC hDC)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_InvalidateRect(
        IOleInPlaceSiteWindowless *iface, LPCRECT pRect, BOOL fErase)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_InvalidateRgn(
        IOleInPlaceSiteWindowless *iface, HRGN hRGN, BOOL fErase)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_ScrollRect(
        IOleInPlaceSiteWindowless *iface, INT dx, INT dy,
        LPCRECT pRectScroll, LPCRECT pRectClip)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_AdjustRect(
        IOleInPlaceSiteWindowless *iface, LPRECT prc)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSiteWindowless_OnDefWindowMessage(
        IOleInPlaceSiteWindowless *iface, UINT msg,
        WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleInPlaceSiteWindowlessVtbl InPlaceSiteWindowlessVtbl = {
    InPlaceSiteWindowless_QueryInterface,
    InPlaceSiteWindowless_AddRef,
    InPlaceSiteWindowless_Release,
    InPlaceSiteWindowless_GetWindow,
    InPlaceSiteWindowless_ContextSensitiveHelp,
    InPlaceSiteWindowless_CanInPlaceActivate,
    InPlaceSiteWindowless_OnInPlaceActivate,
    InPlaceSiteWindowless_OnUIActivate,
    InPlaceSiteWindowless_GetWindowContext,
    InPlaceSiteWindowless_Scroll,
    InPlaceSiteWindowless_OnUIDeactivate,
    InPlaceSiteWindowless_OnInPlaceDeactivate,
    InPlaceSiteWindowless_DiscardUndoState,
    InPlaceSiteWindowless_DeactivateAndUndo,
    InPlaceSiteWindowless_OnPosRectChange,
    InPlaceSiteWindowless_OnInPlaceActivateEx,
    InPlaceSiteWindowless_OnInPlaceDeactivateEx,
    InPlaceSiteWindowless_RequestUIActivate,
    InPlaceSiteWindowless_CanWindowlessActivate,
    InPlaceSiteWindowless_GetCapture,
    InPlaceSiteWindowless_SetCapture,
    InPlaceSiteWindowless_GetFocus,
    InPlaceSiteWindowless_SetFocus,
    InPlaceSiteWindowless_GetDC,
    InPlaceSiteWindowless_ReleaseDC,
    InPlaceSiteWindowless_InvalidateRect,
    InPlaceSiteWindowless_InvalidateRgn,
    InPlaceSiteWindowless_ScrollRect,
    InPlaceSiteWindowless_AdjustRect,
    InPlaceSiteWindowless_OnDefWindowMessage
};

static IOleInPlaceSiteWindowless InPlaceSiteWindowless = { &InPlaceSiteWindowlessVtbl };

static HRESULT WINAPI DocumentSite_QueryInterface(IOleDocumentSite *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI DocumentSite_AddRef(IOleDocumentSite *iface)
{
    return 2;
}

static ULONG WINAPI DocumentSite_Release(IOleDocumentSite *iface)
{
    return 1;
}

typedef enum
{
    CallUIActivate_None,
    CallUIActivate_ActivateMe,
    CallUIActivate_AfterShow,
} CallUIActivate;

static BOOL call_UIActivate = CallUIActivate_ActivateMe;
static HRESULT WINAPI DocumentSite_ActivateMe(IOleDocumentSite *iface, IOleDocumentView *pViewToActivate)
{
    IOleDocument *document;
    HRESULT hres;

    CHECK_EXPECT(ActivateMe);
    ok(pViewToActivate != NULL, "pViewToActivate = NULL\n");

    hres = IOleDocumentView_QueryInterface(pViewToActivate, &IID_IOleDocument, (void**)&document);
    ok(hres == S_OK, "could not get IOleDocument: %08x\n", hres);

    if(SUCCEEDED(hres)) {
        hres = IOleDocument_CreateView(document, (IOleInPlaceSite*)&InPlaceSiteWindowless, NULL, 0, &view);
        ok(hres == S_OK, "CreateView failed: %08x\n", hres);

        if(SUCCEEDED(hres)) {
            IOleInPlaceActiveObject *activeobj = NULL;
            IOleInPlaceSite *inplacesite = NULL;
            HWND tmp_hwnd = NULL;
            static RECT rect = {0,0,400,500};

            hres = IOleDocumentView_GetInPlaceSite(view, &inplacesite);
            ok(hres == S_OK, "GetInPlaceSite failed: %08x\n", hres);
            ok(inplacesite == (IOleInPlaceSite*)&InPlaceSiteWindowless, "inplacesite=%p, expected %p\n",
                    inplacesite, &InPlaceSiteWindowless);

            hres = IOleDocumentView_SetInPlaceSite(view, (IOleInPlaceSite*)&InPlaceSiteWindowless);
            ok(hres == S_OK, "SetInPlaceSite failed: %08x\n", hres);

            hres = IOleDocumentView_GetInPlaceSite(view, &inplacesite);
            ok(hres == S_OK, "GetInPlaceSite failed: %08x\n", hres);
            ok(inplacesite == (IOleInPlaceSite*)&InPlaceSiteWindowless, "inplacesite=%p, expected %p\n",
                    inplacesite, &InPlaceSiteWindowless);

            hres = IOleDocumentView_QueryInterface(view, &IID_IOleInPlaceActiveObject, (void**)&activeobj);
            ok(hres == S_OK, "Could not get IOleInPlaceActiveObject: %08x\n", hres);

            if(activeobj) {
                IOleInPlaceActiveObject_GetWindow(activeobj, &hwnd);
                ok(hres == S_OK, "GetWindow failed: %08x\n", hres);
                ok(hwnd == NULL, "hwnd=%p, expeted NULL\n", hwnd);
            }
            
            if(call_UIActivate == CallUIActivate_ActivateMe) {
                SET_EXPECT(CanInPlaceActivate);
                SET_EXPECT(GetWindowContext);
                SET_EXPECT(GetWindow);
                if(ipsex)
                    SET_EXPECT(OnInPlaceActivateEx);
                else
                    SET_EXPECT(OnInPlaceActivate);
                SET_EXPECT(SetStatusText);
                SET_EXPECT(Exec_SETPROGRESSMAX);
                SET_EXPECT(Exec_SETPROGRESSPOS);
                SET_EXPECT(OnUIActivate);
                SET_EXPECT(SetActiveObject);
                SET_EXPECT(ShowUI);
                expect_status_text = NULL;

                hres = IOleDocumentView_UIActivate(view, TRUE);
                ok(hres == S_OK, "UIActivate failed: %08x\n", hres);

                CHECK_CALLED(CanInPlaceActivate);
                CHECK_CALLED(GetWindowContext);
                CHECK_CALLED(GetWindow);
                if(ipsex)
                    CHECK_CALLED(OnInPlaceActivateEx);
                else
                    CHECK_CALLED(OnInPlaceActivate);
                CHECK_CALLED(SetStatusText);
                CHECK_CALLED(Exec_SETPROGRESSMAX);
                CHECK_CALLED(Exec_SETPROGRESSPOS);
                CHECK_CALLED(OnUIActivate);
                CHECK_CALLED(SetActiveObject);
                CHECK_CALLED(ShowUI);

                if(activeobj) {
                    hres = IOleInPlaceActiveObject_GetWindow(activeobj, &hwnd);
                    ok(hres == S_OK, "GetWindow failed: %08x\n", hres);
                    ok(hwnd != NULL, "hwnd == NULL\n");
                    if(last_hwnd)
                        ok(hwnd == last_hwnd, "hwnd != last_hwnd\n");
                }

                hres = IOleDocumentView_UIActivate(view, TRUE);
                ok(hres == S_OK, "UIActivate failed: %08x\n", hres);

                if(activeobj) {
                    hres = IOleInPlaceActiveObject_GetWindow(activeobj, &tmp_hwnd);
                    ok(hres == S_OK, "GetWindow failed: %08x\n", hres);
                    ok(tmp_hwnd == hwnd, "tmp_hwnd=%p, expected %p\n", tmp_hwnd, hwnd);
                }
            }

            hres = IOleDocumentView_SetRect(view, &rect);
            ok(hres == S_OK, "SetRect failed: %08x\n", hres);

            if(call_UIActivate == CallUIActivate_ActivateMe) {
                hres = IOleDocumentView_Show(view, TRUE);
                ok(hres == S_OK, "Show failed: %08x\n", hres);
            }else {
                SET_EXPECT(CanInPlaceActivate);
                SET_EXPECT(GetWindowContext);
                SET_EXPECT(GetWindow);
                if(ipsex)
                    SET_EXPECT(OnInPlaceActivateEx);
                else
                    SET_EXPECT(OnInPlaceActivate);
                SET_EXPECT(SetStatusText);
                SET_EXPECT(Exec_SETPROGRESSMAX);
                SET_EXPECT(Exec_SETPROGRESSPOS);
                SET_EXPECT(OnUIActivate);
                expect_status_text = (load_state == LD_COMPLETE ? (LPCOLESTR)0xdeadbeef : NULL);

                hres = IOleDocumentView_Show(view, TRUE);
                if(FAILED(hres)) {
                    win_skip("Show failed\n");
                    if(activeobj)
                        IOleInPlaceActiveObject_Release(activeobj);
                    IOleDocument_Release(document);
                    show_failed = TRUE;
                    return S_OK;
                }
                ok(hres == S_OK, "Show failed: %08x\n", hres);

                CHECK_CALLED(CanInPlaceActivate);
                CHECK_CALLED(GetWindowContext);
                CHECK_CALLED(GetWindow);
                if(ipsex)
                    CHECK_CALLED(OnInPlaceActivateEx);
                else
                    CHECK_CALLED(OnInPlaceActivate);
                CHECK_CALLED(SetStatusText);
                CHECK_CALLED(Exec_SETPROGRESSMAX);
                CHECK_CALLED(Exec_SETPROGRESSPOS);

                if(activeobj) {
                    hres = IOleInPlaceActiveObject_GetWindow(activeobj, &hwnd);
                    ok(hres == S_OK, "GetWindow failed: %08x\n", hres);
                    ok(hwnd != NULL, "hwnd == NULL\n");
                    if(last_hwnd)
                        ok(hwnd == last_hwnd, "hwnd != last_hwnd\n");
                }
            }

            test_timer(EXPECT_UPDATEUI | ((load_state == LD_LOADING) ? EXPECT_SETTITLE : 0));

            if(activeobj)
                IOleInPlaceActiveObject_Release(activeobj);
        }

        IOleDocument_Release(document);
    }

    return S_OK;
}

static const IOleDocumentSiteVtbl DocumentSiteVtbl = {
    DocumentSite_QueryInterface,
    DocumentSite_AddRef,
    DocumentSite_Release,
    DocumentSite_ActivateMe
};

static IOleDocumentSite DocumentSite = { &DocumentSiteVtbl };

static HRESULT WINAPI OleControlSite_QueryInterface(IOleControlSite *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI OleControlSite_AddRef(IOleControlSite *iface)
{
    return 2;
}

static ULONG WINAPI OleControlSite_Release(IOleControlSite *iface)
{
    return 1;
}

static HRESULT WINAPI OleControlSite_OnControlInfoChanged(IOleControlSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControlSite_LockInPlaceActive(IOleControlSite *iface, BOOL fLock)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControlSite_GetExtendedControl(IOleControlSite *iface, IDispatch **ppDisp)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControlSite_TransformCoords(IOleControlSite *iface, POINTL *pPtHimetric,
        POINTF *pPtfContainer, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControlSite_TranslateAccelerator(IOleControlSite *iface,
        MSG *pMsg, DWORD grfModifiers)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControlSite_OnFocus(IOleControlSite *iface, BOOL fGotFocus)
{
    if(fGotFocus)
        CHECK_EXPECT(OnFocus_TRUE);
    else
        CHECK_EXPECT2(OnFocus_FALSE);
    return S_OK;
}

static HRESULT WINAPI OleControlSite_ShowPropertyFrame(IOleControlSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleControlSiteVtbl OleControlSiteVtbl = {
    OleControlSite_QueryInterface,
    OleControlSite_AddRef,
    OleControlSite_Release,
    OleControlSite_OnControlInfoChanged,
    OleControlSite_LockInPlaceActive,
    OleControlSite_GetExtendedControl,
    OleControlSite_TransformCoords,
    OleControlSite_TranslateAccelerator,
    OleControlSite_OnFocus,
    OleControlSite_ShowPropertyFrame
};

static IOleControlSite OleControlSite = { &OleControlSiteVtbl };

static HRESULT WINAPI DocHostUIHandler_QueryInterface(IDocHostUIHandler2 *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI DocHostUIHandler_AddRef(IDocHostUIHandler2 *iface)
{
    return 2;
}

static ULONG WINAPI DocHostUIHandler_Release(IDocHostUIHandler2 *iface)
{
    return 1;
}

static HRESULT WINAPI DocHostUIHandler_ShowContextMenu(IDocHostUIHandler2 *iface, DWORD dwID, POINT *ppt,
        IUnknown *pcmdtReserved, IDispatch *pdicpReserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetHostInfo(IDocHostUIHandler2 *iface, DOCHOSTUIINFO *pInfo)
{
    CHECK_EXPECT(GetHostInfo);
    ok(pInfo != NULL, "pInfo=NULL\n");
    if(pInfo) {
        ok(pInfo->cbSize == sizeof(DOCHOSTUIINFO), "pInfo->cbSize=%u\n", pInfo->cbSize);
        ok(!pInfo->dwFlags, "pInfo->dwFlags=%08x, expected 0\n", pInfo->dwFlags);
        pInfo->dwFlags = DOCHOSTUIFLAG_DISABLE_HELP_MENU | DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE
            | DOCHOSTUIFLAG_ACTIVATE_CLIENTHIT_ONLY | DOCHOSTUIFLAG_ENABLE_INPLACE_NAVIGATION
            | DOCHOSTUIFLAG_IME_ENABLE_RECONVERSION;
        ok(!pInfo->dwDoubleClick, "pInfo->dwDoubleClick=%08x, expected 0\n", pInfo->dwDoubleClick);
        ok(!pInfo->pchHostCss, "pInfo->pchHostCss=%p, expected NULL\n", pInfo->pchHostCss);
        ok(!pInfo->pchHostNS, "pInfo->pchhostNS=%p, expected NULL\n", pInfo->pchHostNS);
    }
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_ShowUI(IDocHostUIHandler2 *iface, DWORD dwID,
        IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget,
        IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc)
{
    CHECK_EXPECT(ShowUI);

    if (editmode)
        ok(dwID == DOCHOSTUITYPE_AUTHOR, "dwID=%d, expected DOCHOSTUITYPE_AUTHOR\n", dwID);
    else
        ok(dwID == DOCHOSTUITYPE_BROWSE, "dwID=%d, expected DOCHOSTUITYPE_BROWSE\n", dwID);
    ok(pActiveObject != NULL, "pActiveObject = NULL\n");
    ok(pCommandTarget != NULL, "pCommandTarget = NULL\n");
    ok(pFrame == &InPlaceFrame, "pFrame=%p, expected %p\n", pFrame, &InPlaceFrame);
    if (expect_InPlaceUIWindow_SetActiveObject_active)
        ok(pDoc == (IOleInPlaceUIWindow *)&InPlaceUIWindow, "pDoc=%p, expected %p\n", pDoc, &InPlaceUIWindow);
    else
        ok(pDoc == NULL, "pDoc=%p, expected NULL\n", pDoc);

    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_HideUI(IDocHostUIHandler2 *iface)
{
    CHECK_EXPECT(HideUI);
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_UpdateUI(IDocHostUIHandler2 *iface)
{
    CHECK_EXPECT(UpdateUI);
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_EnableModeless(IDocHostUIHandler2 *iface, BOOL fEnable)
{
    if(fEnable)
        CHECK_EXPECT2(EnableModeless_TRUE);
    else
        CHECK_EXPECT2(EnableModeless_FALSE);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_OnDocWindowActivate(IDocHostUIHandler2 *iface, BOOL fActivate)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static BOOL expect_OnFrameWindowActivate_fActivate;
static HRESULT WINAPI DocHostUIHandler_OnFrameWindowActivate(IDocHostUIHandler2 *iface, BOOL fActivate)
{
    CHECK_EXPECT2(OnFrameWindowActivate);
    ok(fActivate == expect_OnFrameWindowActivate_fActivate, "fActivate=%x\n", fActivate);
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_ResizeBorder(IDocHostUIHandler2 *iface, LPCRECT prcBorder,
        IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_TranslateAccelerator(IDocHostUIHandler2 *iface, LPMSG lpMsg,
        const GUID *pguidCmdGroup, DWORD nCmdID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetOptionKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    CHECK_EXPECT(GetOptionKeyPath);
    ok(pchKey != NULL, "pchKey = NULL\n");
    ok(!dw, "dw=%d, expected 0\n", dw);
    if(pchKey)
        ok(!*pchKey, "*pchKey=%p, expected NULL\n", *pchKey);
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_GetDropTarget(IDocHostUIHandler2 *iface,
        IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
    CHECK_EXPECT(GetDropTarget);
    /* TODO */
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetExternal(IDocHostUIHandler2 *iface, IDispatch **ppDispatch)
{
    CHECK_EXPECT(GetExternal);
    *ppDispatch = &External;
    return S_FALSE;
}

static HRESULT WINAPI DocHostUIHandler_TranslateUrl(IDocHostUIHandler2 *iface, DWORD dwTranslate,
        OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
    CHECK_EXPECT(TranslateUrl);
    ok(!dwTranslate, "dwTranslate = %x\n", dwTranslate);
    ok(!strcmp_wa(pchURLIn, "about:blank"), "pchURLIn = %s\n", wine_dbgstr_w(pchURLIn));
    ok(ppchURLOut != NULL, "ppchURLOut == NULL\n");
    ok(!*ppchURLOut, "*ppchURLOut = %p\n", *ppchURLOut);

    return S_FALSE;
}

static HRESULT WINAPI DocHostUIHandler_FilterDataObject(IDocHostUIHandler2 *iface, IDataObject *pDO,
        IDataObject **ppPORet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetOverrideKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    CHECK_EXPECT(GetOverrideKeyPath);
    ok(pchKey != NULL, "pchKey = NULL\n");
    if(pchKey)
        ok(!*pchKey, "*pchKey=%p, expected NULL\n", *pchKey);
    ok(!dw, "dw=%d, xepected 0\n", dw);
    return S_OK;
}

static const IDocHostUIHandler2Vtbl DocHostUIHandlerVtbl = {
    DocHostUIHandler_QueryInterface,
    DocHostUIHandler_AddRef,
    DocHostUIHandler_Release,
    DocHostUIHandler_ShowContextMenu,
    DocHostUIHandler_GetHostInfo,
    DocHostUIHandler_ShowUI,
    DocHostUIHandler_HideUI,
    DocHostUIHandler_UpdateUI,
    DocHostUIHandler_EnableModeless,
    DocHostUIHandler_OnDocWindowActivate,
    DocHostUIHandler_OnFrameWindowActivate,
    DocHostUIHandler_ResizeBorder,
    DocHostUIHandler_TranslateAccelerator,
    DocHostUIHandler_GetOptionKeyPath,
    DocHostUIHandler_GetDropTarget,
    DocHostUIHandler_GetExternal,
    DocHostUIHandler_TranslateUrl,
    DocHostUIHandler_FilterDataObject,
    DocHostUIHandler_GetOverrideKeyPath
};

static IDocHostUIHandler2 DocHostUIHandler = { &DocHostUIHandlerVtbl };

static HRESULT WINAPI OleCommandTarget_QueryInterface(IOleCommandTarget *iface,
        REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI OleCommandTarget_AddRef(IOleCommandTarget *iface)
{
    return 2;
}

static ULONG WINAPI OleCommandTarget_Release(IOleCommandTarget *iface)
{
    return 1;
}

static HRESULT WINAPI OleCommandTarget_QueryStatus(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    ok(!pguidCmdGroup, "pguidCmdGroup != MULL\n");
    ok(cCmds == 1, "cCmds=%d, expected 1\n", cCmds);
    ok(!pCmdText, "pCmdText != NULL\n");

    switch(prgCmds[0].cmdID) {
    case OLECMDID_SETPROGRESSTEXT:
        CHECK_EXPECT(QueryStatus_SETPROGRESSTEXT);
        prgCmds[0].cmdf = OLECMDF_ENABLED;
        return S_OK;
    case OLECMDID_OPEN:
        CHECK_EXPECT(QueryStatus_OPEN);
        prgCmds[0].cmdf = 0;
        return S_OK;
    case OLECMDID_NEW:
        CHECK_EXPECT(QueryStatus_NEW);
        prgCmds[0].cmdf = 0;
        return S_OK;
    default:
        ok(0, "unexpected command %d\n", prgCmds[0].cmdID);
    };

    return E_FAIL;
}

static HRESULT WINAPI OleCommandTarget_Exec(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    if((!pguidCmdGroup || !IsEqualGUID(pguidCmdGroup, &CGID_Explorer))
        && (!pguidCmdGroup || !IsEqualGUID(&CGID_ShellDocView, pguidCmdGroup) || nCmdID != 63))
        test_readyState(NULL);

    if(!pguidCmdGroup) {
        switch(nCmdID) {
        case OLECMDID_SETPROGRESSMAX:
            CHECK_EXPECT2(Exec_SETPROGRESSMAX);
            ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER, "nCmdexecopts=%08x\n", nCmdexecopt);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            if(pvaIn) {
                ok(V_VT(pvaIn) == VT_I4, "V_VT(pvaIn)=%d, expected VT_I4\n", V_VT(pvaIn));
                if(load_state == LD_NO)
                    ok(V_I4(pvaIn) == 0, "V_I4(pvaIn)=%d, expected 0\n", V_I4(pvaIn));
            }
            ok(pvaOut == NULL, "pvaOut=%p, expected NULL\n", pvaOut);
            return S_OK;
        case OLECMDID_SETPROGRESSPOS:
            CHECK_EXPECT2(Exec_SETPROGRESSPOS);
            ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER, "nCmdexecopts=%08x\n", nCmdexecopt);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            if(pvaIn) {
                ok(V_VT(pvaIn) == VT_I4, "V_VT(pvaIn)=%d, expected VT_I4\n", V_VT(pvaIn));
                if(load_state == LD_NO)
                    ok(V_I4(pvaIn) == 0, "V_I4(pvaIn)=%d, expected 0\n", V_I4(pvaIn));
            }
            ok(pvaOut == NULL, "pvaOut=%p, expected NULL\n", pvaOut);
            return S_OK;
        case OLECMDID_HTTPEQUIV_DONE:
            CHECK_EXPECT(Exec_HTTPEQUIV_DONE);
            ok(nCmdexecopt == 0, "nCmdexecopts=%08x\n", nCmdexecopt);
            ok(pvaOut == NULL, "pvaOut=%p\n", pvaOut);
            ok(pvaIn == NULL, "pvaIn=%p\n", pvaIn);
            readystate_set_loading = FALSE;
            readystate_set_interactive = FALSE;
            load_state = LD_COMPLETE;
            return S_OK;
        case OLECMDID_SETDOWNLOADSTATE:
            ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER, "nCmdexecopts=%08x\n", nCmdexecopt);
            ok(pvaOut == NULL, "pvaOut=%p\n", pvaOut);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            ok(V_VT(pvaIn) == VT_I4, "V_VT(pvaIn)=%d\n", V_VT(pvaIn));

            switch(V_I4(pvaIn)) {
            case 0:
                CHECK_EXPECT(Exec_SETDOWNLOADSTATE_0);
                load_state = LD_INTERACTIVE;
                break;
            case 1:
                CHECK_EXPECT(Exec_SETDOWNLOADSTATE_1);
                readystate_set_interactive = (load_state != LD_INTERACTIVE);
                break;
            default:
                ok(0, "unexpevted V_I4(pvaIn)=%d\n", V_I4(pvaIn));
            }

            return S_OK;
        case OLECMDID_UPDATECOMMANDS:
            CHECK_EXPECT(Exec_UPDATECOMMANDS);
            ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER, "nCmdexecopts=%08x\n", nCmdexecopt);
            ok(pvaIn == NULL, "pvaIn=%p\n", pvaIn);
            ok(pvaOut == NULL, "pvaOut=%p\n", pvaOut);
            return S_OK;
        case OLECMDID_SETTITLE:
            CHECK_EXPECT2(Exec_SETTITLE);
            ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER, "nCmdexecopts=%08x\n", nCmdexecopt);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            ok(pvaOut == NULL, "pvaOut=%p\n", pvaOut);
            ok(V_VT(pvaIn) == VT_BSTR, "V_VT(pvaIn)=%d\n", V_VT(pvaIn));
            ok(V_BSTR(pvaIn) != NULL, "V_BSTR(pvaIn) == NULL\n"); /* TODO */
            return S_OK;
        case OLECMDID_HTTPEQUIV:
            CHECK_EXPECT2(Exec_HTTPEQUIV);
            ok(!nCmdexecopt, "nCmdexecopts=%08x\n", nCmdexecopt);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            ok(pvaOut == NULL, "pvaOut=%p\n", pvaOut);
            ok(V_VT(pvaIn) == VT_BSTR, "V_VT(pvaIn)=%d\n", V_VT(pvaIn));
            ok(V_BSTR(pvaIn) != NULL, "V_BSTR(pvaIn) = NULL\n");
            test_readyState(NULL);
            return S_OK;
        case OLECMDID_UPDATETRAVELENTRY_DATARECOVERY:
            return E_FAIL; /* FIXME */
        default:
            ok(0, "unexpected command %d\n", nCmdID);
            return E_FAIL;
        };
    }

    if(IsEqualGUID(&CGID_ShellDocView, pguidCmdGroup)) {
        ok(nCmdexecopt == 0, "nCmdexecopts=%08x\n", nCmdexecopt);

        switch(nCmdID) {
        case 37:
            CHECK_EXPECT2(Exec_ShellDocView_37);

            if(load_from_stream || navigated_load)
                test_GetCurMoniker(doc_unk, NULL, about_blank_url);
            else if(!editmode)
                test_GetCurMoniker(doc_unk, doc_mon, NULL);

            ok(pvaOut == NULL, "pvaOut=%p, expected NULL\n", pvaOut);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            if(pvaIn) {
                ok(V_VT(pvaIn) == VT_I4, "V_VT(pvaIn)=%d, expected VT_I4\n", V_VT(pvaIn));
                ok(V_I4(pvaIn) == 0, "V_I4(pvaIn)=%d, expected 0\n", V_I4(pvaIn));
            }
            return S_OK;

        case 63: {
            IHTMLPrivateWindow *priv_window;
            HRESULT hres;

            CHECK_EXPECT(Exec_ShellDocView_63);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            ok(V_VT(pvaIn) == VT_UNKNOWN, "V_VT(pvaIn) = %d\n", V_VT(pvaIn));
            ok(V_UNKNOWN(pvaIn) != NULL, "VPUNKNOWN(pvaIn) = NULL\n");
            ok(pvaOut != NULL, "pvaOut == NULL\n");
            ok(V_VT(pvaOut) == VT_EMPTY, "V_VT(pvaOut) = %d\n", V_VT(pvaOut));

            hres = IUnknown_QueryInterface(V_UNKNOWN(pvaIn), &IID_IHTMLPrivateWindow, (void**)&priv_window);
            ok(hres == S_OK, "Could not get IHTMLPrivateWindow: %08x\n", hres);
            if(SUCCEEDED(hres))
                IHTMLPrivateWindow_Release(priv_window);

            load_state = LD_LOADING;
            return S_OK; /* TODO */
        }

        case 67:
            CHECK_EXPECT(Exec_ShellDocView_67);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            ok(V_VT(pvaIn) == VT_BSTR, "V_VT(pvaIn) = %d\n", V_VT(pvaIn));
            ok(!strcmp_wa(V_BSTR(pvaIn), "about:blank"), "V_BSTR(pvaIn) = %s\n", wine_dbgstr_w(V_BSTR(pvaIn)));
            ok(pvaOut != NULL, "pvaOut == NULL\n");
            ok(V_VT(pvaOut) == VT_BOOL, "V_VT(pvaOut) = %d\n", V_VT(pvaOut));
            ok(V_BOOL(pvaOut) == VARIANT_TRUE, "V_BOOL(pvaOut) = %x\n", V_BOOL(pvaOut));
            load_state = LD_DOLOAD;
            return S_OK;

        case 84:
            CHECK_EXPECT2(Exec_ShellDocView_84);

            ok(pvaIn == NULL, "pvaIn != NULL\n");
            ok(pvaOut != NULL, "pvaOut == NULL\n");
            if(pvaIn)
                ok(V_VT(pvaOut) == VT_EMPTY, "V_VT(pvaOut)=%d\n", V_VT(pvaOut));

            return E_NOTIMPL;

        case 103:
            CHECK_EXPECT2(Exec_ShellDocView_103);

            ok(pvaIn == NULL, "pvaIn != NULL\n");
            ok(pvaOut == NULL, "pvaOut != NULL\n");

            return E_NOTIMPL;

        case 105:
            CHECK_EXPECT2(Exec_ShellDocView_105);

            ok(pvaIn != NULL, "pvaIn == NULL\n");
            ok(pvaOut == NULL, "pvaOut != NULL\n");

            return E_NOTIMPL;

        case 140:
            CHECK_EXPECT2(Exec_ShellDocView_140);

            ok(pvaIn == NULL, "pvaIn != NULL\n");
            ok(pvaOut == NULL, "pvaOut != NULL\n");

            return E_NOTIMPL;

        default:
            ok(0, "unexpected command %d\n", nCmdID);
            return E_FAIL;
        };
    }

    if(IsEqualGUID(&CGID_MSHTML, pguidCmdGroup)) {
        ok(nCmdexecopt == 0, "nCmdexecopts=%08x\n", nCmdexecopt);

        switch(nCmdID) {
        case IDM_PARSECOMPLETE:
            CHECK_EXPECT(Exec_MSHTML_PARSECOMPLETE);
            ok(pvaIn == NULL, "pvaIn != NULL\n");
            ok(pvaOut == NULL, "pvaOut != NULL\n");
            return S_OK;
        default:
            ok(0, "unexpected command %d\n", nCmdID);
        };
    }

    if(IsEqualGUID(&CGID_DocHostCmdPriv, pguidCmdGroup)) {
        switch(nCmdID) {
        case DOCHOST_DOCCANNAVIGATE:
            CHECK_EXPECT(Exec_DOCCANNAVIGATE);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            ok(pvaOut == NULL, "pvaOut != NULL\n");
            ok(V_VT(pvaIn) == VT_UNKNOWN, "V_VT(pvaIn) != VT_UNKNOWN\n");
            /* FIXME: test V_UNKNOWN(pvaIn) == window */
            return S_OK;
        default:
            return E_FAIL; /* TODO */
        }
    }

    if(IsEqualGUID(&CGID_Explorer, pguidCmdGroup)) {
        ok(nCmdexecopt == 0, "nCmdexecopts=%08x\n", nCmdexecopt);

        switch(nCmdID) {
        case 69:
            CHECK_EXPECT2(Exec_Explorer_69);
            ok(pvaIn == NULL, "pvaIn != NULL\n");
            ok(pvaOut != NULL, "pvaOut == NULL\n");
            return E_NOTIMPL;
        default:
            ok(0, "unexpected cmd %d of CGID_Explorer\n", nCmdID);
        }
        return E_NOTIMPL;
    }

    if(IsEqualGUID(&CGID_DocHostCommandHandler, pguidCmdGroup)) {
        switch (nCmdID) {
        case OLECMDID_PAGEACTIONBLOCKED: /* win2k3 */
            SET_EXPECT(SetStatusText);
            ok(pvaIn == NULL, "pvaIn != NULL\n");
            ok(pvaOut == NULL, "pvaOut != NULL\n");
            return S_OK;
        case OLECMDID_SHOWSCRIPTERROR:
            /* TODO */
            return S_OK;
        default:
            ok(0, "unexpected command %d\n", nCmdID);
            return E_FAIL;
        }
    }

    ok(0, "unexpected pguidCmdGroup: %s\n", debugstr_guid(pguidCmdGroup));
    return E_NOTIMPL;
}

static IOleCommandTargetVtbl OleCommandTargetVtbl = {
    OleCommandTarget_QueryInterface,
    OleCommandTarget_AddRef,
    OleCommandTarget_Release,
    OleCommandTarget_QueryStatus,
    OleCommandTarget_Exec
};

static IOleCommandTarget OleCommandTarget = { &OleCommandTargetVtbl };

static HRESULT WINAPI Dispatch_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static HRESULT WINAPI Dispatch_Invoke(IDispatch *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(IsEqualGUID(&IID_NULL, riid), "riid != IID_NULL\n");
    ok(pDispParams != NULL, "pDispParams == NULL\n");
    ok(pExcepInfo == NULL, "pExcepInfo=%p, expected NULL\n", pExcepInfo);
    ok(puArgErr != NULL, "puArgErr == NULL\n");
    ok(V_VT(pVarResult) == 0, "V_VT(pVarResult)=%d, expected 0\n", V_VT(pVarResult));
    ok(wFlags == DISPATCH_PROPERTYGET, "wFlags=%08x, expected DISPATCH_PROPERTYGET\n", wFlags);
    test_readyState(NULL);

    switch(dispIdMember) {
    case DISPID_AMBIENT_USERMODE:
        CHECK_EXPECT2(Invoke_AMBIENT_USERMODE);
        V_VT(pVarResult) = VT_BOOL;
        V_BOOL(pVarResult) = VARIANT_TRUE;
        return S_OK;
    case DISPID_AMBIENT_DLCONTROL:
        CHECK_EXPECT2(Invoke_AMBIENT_DLCONTROL);
        return E_FAIL;
    case DISPID_AMBIENT_OFFLINEIFNOTCONNECTED:
        CHECK_EXPECT2(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        return E_FAIL;
    case DISPID_AMBIENT_SILENT:
        CHECK_EXPECT2(Invoke_AMBIENT_SILENT);
        V_VT(pVarResult) = VT_BOOL;
        V_BOOL(pVarResult) = VARIANT_FALSE;
        return S_OK;
    case DISPID_AMBIENT_USERAGENT:
        CHECK_EXPECT(Invoke_AMBIENT_USERAGENT);
        return E_FAIL;
    case DISPID_AMBIENT_PALETTE:
        CHECK_EXPECT(Invoke_AMBIENT_PALETTE);
        return E_FAIL;
    };

    ok(0, "unexpected dispid %d\n", dispIdMember);
    return E_FAIL;
}

static IDispatchVtbl DispatchVtbl = {
    Dispatch_QueryInterface,
    Dispatch_AddRef,
    Dispatch_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    Dispatch_GetIDsOfNames,
    Dispatch_Invoke
};

static IDispatch Dispatch = { &DispatchVtbl };

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface,
                                                     REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    return 2;
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    return 1;
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface, REFGUID guidService,
                                    REFIID riid, void **ppv)
{
    /*
     * Services used by HTMLDocument:
     *
     * IOleUndoManager
     * IInternetSecurityManager
     * ITargetFrame
     * {D5F78C80-5252-11CF-90FA-00AA0042106E}
     * HTMLFrameBase
     * IShellObject
     * {3050F312-98B5-11CF-BB82-00AA00BDCE0B}
     * {53A2D5B1-D2FC-11D0-84E0-006097C9987D}
     * {AD7F6C62-F6BD-11D2-959B-006097C553C8}
     * DefView (?)
     * {6D12FE80-7911-11CF-9534-0000C05BAE0B}
     * IElementBehaviorFactory
     * {3050F429-98B5-11CF-BB82-00AA00BDCE0B}
     * STopLevelBrowser
     * IHTMLWindow2
     * IInternetProtocol
     * IWebBrowserApp
     * UrlHostory
     * IHTMLEditHost
     * IHlinkFrame
     */

    if(IsEqualGUID(&IID_IHlinkFrame, guidService)) {
        ok(IsEqualGUID(&IID_IHlinkFrame, riid), "unexpected riid\n");
        *ppv = &HlinkFrame;
        return S_OK;
    }

    if(IsEqualGUID(&SID_SNewWindowManager, guidService)) {
        ok(IsEqualGUID(&IID_INewWindowManager, riid), "unexpected riid\n");
        *ppv = &NewWindowManager;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static IServiceProvider ServiceProvider = { &ServiceProviderVtbl };

static HRESULT WINAPI AdviseSink_QueryInterface(IAdviseSinkEx *iface,
        REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI AdviseSink_AddRef(IAdviseSinkEx *iface)
{
    return 2;
}

static ULONG WINAPI AdviseSink_Release(IAdviseSinkEx *iface)
{
    return 1;
}

static void WINAPI AdviseSink_OnDataChange(IAdviseSinkEx *iface,
        FORMATETC *pFormatetc, STGMEDIUM *pStgmed)
{
    ok(0, "unexpected call\n");
}

static void WINAPI AdviseSink_OnViewChange(IAdviseSinkEx *iface,
        DWORD dwAspect, LONG lindex)
{
    ok(0, "unexpected call\n");
}

static void WINAPI AdviseSink_OnRename(IAdviseSinkEx *iface, IMoniker *pmk)
{
    ok(0, "unexpected call\n");
}

static void WINAPI AdviseSink_OnSave(IAdviseSinkEx *iface)
{
    ok(0, "unexpected call\n");
}

static void WINAPI AdviseSink_OnClose(IAdviseSinkEx *iface)
{
    ok(0, "unexpected call\n");
}

static void WINAPI AdviseSinkEx_OnViewStatusChange(IAdviseSinkEx *iface, DWORD dwViewStatus)
{
    ok(0, "unexpected call\n");
}

static void WINAPI ObjectAdviseSink_OnClose(IAdviseSinkEx *iface)
{
    CHECK_EXPECT(Advise_Close);
}

static const IAdviseSinkExVtbl AdviseSinkVtbl = {
    AdviseSink_QueryInterface,
    AdviseSink_AddRef,
    AdviseSink_Release,
    AdviseSink_OnDataChange,
    AdviseSink_OnViewChange,
    AdviseSink_OnRename,
    AdviseSink_OnSave,
    ObjectAdviseSink_OnClose,
    AdviseSinkEx_OnViewStatusChange
};

static IAdviseSinkEx AdviseSink = { &AdviseSinkVtbl };

static HRESULT WINAPI ViewAdviseSink_QueryInterface(IAdviseSinkEx *iface,
        REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IAdviseSinkEx, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected riid %s\n", debugstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static void WINAPI ViewAdviseSink_OnViewChange(IAdviseSinkEx *iface,
        DWORD dwAspect, LONG lindex)
{
    CHECK_EXPECT2(OnViewChange);

    ok(dwAspect == DVASPECT_CONTENT, "dwAspect = %d\n", dwAspect);
    ok(lindex == -1, "lindex = %d\n", lindex);
}

static const IAdviseSinkExVtbl ViewAdviseSinkVtbl = {
    ViewAdviseSink_QueryInterface,
    AdviseSink_AddRef,
    AdviseSink_Release,
    AdviseSink_OnDataChange,
    ViewAdviseSink_OnViewChange,
    AdviseSink_OnRename,
    AdviseSink_OnSave,
    AdviseSink_OnClose,
    AdviseSinkEx_OnViewStatusChange
};

static IAdviseSinkEx ViewAdviseSink = { &ViewAdviseSinkVtbl };

DEFINE_GUID(IID_unk1, 0xD48A6EC6,0x6A4A,0x11CF,0x94,0xA7,0x44,0x45,0x53,0x54,0x00,0x00); /* HTMLWindow2 ? */
DEFINE_GUID(IID_IThumbnailView, 0x7BB0B520,0xB1A7,0x11D2,0xBB,0x23,0x00,0xC0,0x4F,0x79,0xAB,0xCD);
DEFINE_GUID(IID_IRenMailEditor, 0x000670BA,0x0000,0x0000,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_GUID(IID_unk4, 0x305104a6,0x98b5,0x11cf,0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b);
DEFINE_GUID(IID_IDocHostUIHandlerPriv, 0xf0d241d1,0x5d0e,0x4e85,0xbc,0xb4,0xfa,0xd7,0xf7,0xc5,0x52,0x8c);

static HRESULT QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IOleClientSite, riid))
        *ppv = &ClientSite;
    else if(IsEqualGUID(&IID_IOleDocumentSite, riid))
        *ppv = &DocumentSite;
    else if(IsEqualGUID(&IID_IDocHostUIHandler, riid) || IsEqualGUID(&IID_IDocHostUIHandler2, riid))
        *ppv = &DocHostUIHandler;
    else if(IsEqualGUID(&IID_IOleContainer, riid))
        *ppv = &OleContainer;
    else if(IsEqualGUID(&IID_IOleWindow, riid) || IsEqualGUID(&IID_IOleInPlaceSite, riid))
        *ppv = &InPlaceSiteWindowless;
    else if(IsEqualGUID(&IID_IOleCommandTarget , riid))
        *ppv = &OleCommandTarget;
    else if(IsEqualGUID(&IID_IDispatch, riid))
        *ppv = &Dispatch;
    else if(IsEqualGUID(&IID_IServiceProvider, riid))
        *ppv = &ServiceProvider;
    else if(IsEqualGUID(&IID_IOleInPlaceSiteEx, riid))
        *ppv = ipsex ? &InPlaceSiteWindowless : NULL;
    else if(IsEqualGUID(&IID_IOleInPlaceSiteWindowless, riid))
        *ppv = ipsw ? &InPlaceSiteWindowless : NULL;
    else if(IsEqualGUID(&IID_IOleControlSite, riid))
        *ppv = &OleControlSite;
    else if(IsEqualGUID(&IID_IDocHostShowUI, riid))
        return E_NOINTERFACE; /* TODO */
    else if(IsEqualGUID(&IID_IProxyManager, riid))
        return E_NOINTERFACE; /* ? */
    else if(IsEqualGUID(&IID_unk1, riid))
        return E_NOINTERFACE; /* HTMLWindow2 ? */
    else if(IsEqualGUID(&IID_IThumbnailView, riid))
        return E_NOINTERFACE; /* ? */
    else if(IsEqualGUID(&IID_IRenMailEditor, riid))
        return E_NOINTERFACE; /* ? */
    else if(IsEqualGUID(&IID_unk4, riid))
        return E_NOINTERFACE; /* ? */
    else if(IsEqualGUID(&IID_IDocHostUIHandlerPriv, riid))
        return E_NOINTERFACE; /* ? */
    else
        ok(0, "unexpected riid %s\n", debugstr_guid(riid));

    if(*ppv)
        return S_OK;
    return E_NOINTERFACE;
}

static LRESULT WINAPI wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void test_doscroll(IUnknown *unk)
{
    IHTMLDocument3 *doc;
    IHTMLElement2 *elem2;
    IHTMLElement *elem;
    VARIANT v;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLDocument3, (void**)&doc);
    ok(hres == S_OK, "Could not get IHTMLDocument3 iface: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IHTMLDocument3_get_documentElement(doc, &elem);
    IHTMLDocument3_Release(doc);
    ok(hres == S_OK, "get_documentElement failed: %08x\n", hres);
    switch(load_state) {
    case LD_DOLOAD:
    case LD_NO:
        if(!navigated_load)
            ok(!elem, "elem != NULL\n");
    default:
        break;
    case LD_INTERACTIVE:
    case LD_COMPLETE:
        ok(elem != NULL, "elem == NULL\n");
    }
    if(!elem)
        return;

    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLElement2, (void**)&elem2);
    IHTMLElement_Release(elem);
    ok(hres == S_OK, "Could not get IHTMLElement2 iface: %08x\n", hres);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("left");
    hres = IHTMLElement2_doScroll(elem2, v);
    SysFreeString(V_BSTR(&v));
    IHTMLElement2_Release(elem2);

    if(inplace_deactivated)
        ok(hres == E_PENDING, "doScroll failed: %08x\n", hres);
    else if(load_state == LD_COMPLETE)
        ok(hres == S_OK, "doScroll failed: %08x\n", hres);
    else
        ok(hres == E_PENDING || hres == S_OK, "doScroll failed: %08x\n", hres);
}

static void _test_readyState(unsigned line, IUnknown *unk)
{
    IHTMLDocument2 *htmldoc;
    DISPPARAMS dispparams;
    IHTMLElement *elem;
    BSTR state;
    VARIANT out;
    HRESULT hres;

    static const LPCSTR expected_state[] = {
        "uninitialized",
        "loading",
        NULL,
        "interactive",
        "complete",
        "uninitialized"
    };

    if(open_call)
        return; /* FIXME */

    if(!unk)
        unk = doc_unk;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLDocument2, (void**)&htmldoc);
    ok(hres == S_OK, "QueryInterface(IID_IHTMLDocument2) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IHTMLDocument2_get_readyState(htmldoc, NULL);
    ok(hres == E_POINTER, "get_readyState failed: %08x, expected\n", hres);

    hres = IHTMLDocument2_get_readyState(htmldoc, &state);
    ok(hres == S_OK, "get_ReadyState failed: %08x\n", hres);

    if(!strcmp_wa(state, "interactive") && load_state == LD_LOADING)
        load_state = LD_INTERACTIVE;

    ok_(__FILE__, line)
        (!strcmp_wa(state, expected_state[load_state]), "unexpected state %s, expected %d\n",
         wine_dbgstr_w(state), load_state);
    SysFreeString(state);

    hres = IHTMLDocument2_get_body(htmldoc, &elem);
    ok_(__FILE__,line)(hres == S_OK, "get_body failed: %08x\n", hres);
    if(elem) {
        IHTMLElement2 *elem2;
        VARIANT var;

        hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLElement2, (void**)&elem2);
        IHTMLElement_Release(elem);
        ok(hres == S_OK, "Could not get IHTMLElement2 iface: %08x\n", hres);

        hres = IHTMLElement2_get_readyState(elem2, &var);
        IHTMLElement2_Release(elem2);
        ok(hres == S_OK, "get_readyState failed: %08x\n", hres);
        ok(V_VT(&var) == VT_BSTR, "V_VT(state) = %d\n", V_VT(&var));
        ok(!strcmp_wa(V_BSTR(&var), "complete"), "unexpected body state %s\n", wine_dbgstr_w(V_BSTR(&var)));
        VariantClear(&var);
    }else {
        ok_(__FILE__,line)(load_state != LD_COMPLETE, "body is NULL in complete state\n");
    }

    dispparams.cArgs = 0;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = NULL;

    VariantInit(&out);

    hres = IHTMLDocument2_Invoke(htmldoc, DISPID_READYSTATE, &IID_NULL, 0, DISPATCH_PROPERTYGET,
                                 &dispparams, &out, NULL, NULL);
    ok(hres == S_OK, "Invoke(DISPID_READYSTATE) failed: %08x\n", hres);

    ok_(__FILE__,line) (V_VT(&out) == VT_I4, "V_VT(out)=%d\n", V_VT(&out));
    ok_(__FILE__,line) (V_I4(&out) == load_state%5, "VT_I4(out)=%d, expected %d\n", V_I4(&out), load_state%5);

    test_doscroll((IUnknown*)htmldoc);

    IHTMLDocument2_Release(htmldoc);
}

static void test_ViewAdviseSink(IHTMLDocument2 *doc)
{
    IViewObject *view;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IViewObject, (void**)&view);
    ok(hres == S_OK, "QueryInterface(IID_IViewObject) failed: %08x\n", hres);

    hres = IViewObject_SetAdvise(view, DVASPECT_CONTENT, ADVF_PRIMEFIRST, (IAdviseSink*)&ViewAdviseSink);
    ok(hres == S_OK, "SetAdvise failed: %08x\n", hres);

    IViewObject_Release(view);
}

static void test_ConnectionPoint(IConnectionPointContainer *container, REFIID riid)
{
    IConnectionPointContainer *tmp_container = NULL;
    IConnectionPoint *cp;
    IID iid;
    HRESULT hres;

    hres = IConnectionPointContainer_FindConnectionPoint(container, riid, &cp);
    ok(hres == S_OK, "FindConnectionPoint failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IConnectionPoint_GetConnectionInterface(cp, &iid);
    ok(hres == S_OK, "GetConnectionInterface failed: %08x\n", hres);
    ok(IsEqualGUID(riid, &iid), "wrong iid\n");

    hres = IConnectionPoint_GetConnectionInterface(cp, NULL);
    ok(hres == E_POINTER, "GetConnectionInterface failed: %08x, expected E_POINTER\n", hres);

    hres = IConnectionPoint_GetConnectionPointContainer(cp, &tmp_container);
    ok(hres == S_OK, "GetConnectionPointContainer failed: %08x\n", hres);
    ok(tmp_container == container, "container != tmp_container\n");
    if(SUCCEEDED(hres))
        IConnectionPointContainer_Release(tmp_container);

    hres = IConnectionPoint_GetConnectionPointContainer(cp, NULL);
    ok(hres == E_POINTER, "GetConnectionPointContainer failed: %08x, expected E_POINTER\n", hres);

    if(IsEqualGUID(&IID_IPropertyNotifySink, riid)) {
        DWORD cookie;

        hres = IConnectionPoint_Advise(cp, (IUnknown*)&PropertyNotifySink, &cookie);
        ok(hres == S_OK, "Advise failed: %08x\n", hres);
    }

    IConnectionPoint_Release(cp);
}

static void test_ConnectionPointContainer(IHTMLDocument2 *doc)
{
    IConnectionPointContainer *container;
    HRESULT hres;

    hres = IUnknown_QueryInterface(doc, &IID_IConnectionPointContainer, (void**)&container);
    ok(hres == S_OK, "QueryInterface(IID_IConnectionPointContainer) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    test_ConnectionPoint(container, &IID_IDispatch);
    test_ConnectionPoint(container, &IID_IPropertyNotifySink);
    test_ConnectionPoint(container, &DIID_HTMLDocumentEvents);
    test_ConnectionPoint(container, &DIID_HTMLDocumentEvents2);

    IConnectionPointContainer_Release(container);
}

static void test_Load(IPersistMoniker *persist, IMoniker *mon)
{
    IBindCtx *bind;
    HRESULT hres;
    WCHAR sz_html_clientsite_objectparam[MAX_PATH];

    lstrcpyW(sz_html_clientsite_objectparam, SZ_HTML_CLIENTSITE_OBJECTPARAM);

    test_readyState((IUnknown*)persist);

    doc_mon = mon;

    CreateBindCtx(0, &bind);
    IBindCtx_RegisterObjectParam(bind, sz_html_clientsite_objectparam,
                                 (IUnknown*)&ClientSite);

    if(mon == &Moniker)
        SET_EXPECT(GetDisplayName);
    if(!set_clientsite) {
        SET_EXPECT(Invoke_AMBIENT_USERMODE);
        SET_EXPECT(GetHostInfo);
        SET_EXPECT(Invoke_AMBIENT_DLCONTROL);
        SET_EXPECT(Invoke_AMBIENT_SILENT);
        SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        SET_EXPECT(Invoke_AMBIENT_USERAGENT);
        SET_EXPECT(Invoke_AMBIENT_PALETTE);
        SET_EXPECT(GetOptionKeyPath);
        SET_EXPECT(GetOverrideKeyPath);
        SET_EXPECT(GetWindow);
        SET_EXPECT(Exec_DOCCANNAVIGATE);
        SET_EXPECT(QueryStatus_SETPROGRESSTEXT);
        SET_EXPECT(Exec_SETPROGRESSMAX);
        SET_EXPECT(Exec_SETPROGRESSPOS);
        SET_EXPECT(Exec_ShellDocView_37);
    }
    if(!container_locked) {
        SET_EXPECT(GetContainer);
        SET_EXPECT(LockContainer);
    }
    SET_EXPECT(OnChanged_READYSTATE);
    SET_EXPECT(Exec_ShellDocView_84);
    SET_EXPECT(IsSystemMoniker);
    if(mon == &Moniker)
        SET_EXPECT(BindToStorage);
    SET_EXPECT(SetActiveObject);
    if(set_clientsite) {
        SET_EXPECT(Invoke_AMBIENT_SILENT);
        SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        SET_EXPECT(Exec_ShellDocView_37);
    }
    load_state = LD_DOLOAD;
    expect_LockContainer_fLock = TRUE;
    readystate_set_loading = TRUE;

    hres = IPersistMoniker_Load(persist, FALSE, mon, bind, 0x12);
    ok(hres == S_OK, "Load failed: %08x\n", hres);

    if(mon == &Moniker)
        CHECK_CALLED(GetDisplayName);
    if(!set_clientsite) {
        CHECK_CALLED(Invoke_AMBIENT_USERMODE);
        CHECK_CALLED(GetHostInfo);
        CHECK_CALLED(Invoke_AMBIENT_DLCONTROL);
        CHECK_CALLED(Invoke_AMBIENT_SILENT);
        CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        CHECK_CALLED(Invoke_AMBIENT_USERAGENT);
        CHECK_CALLED(Invoke_AMBIENT_PALETTE);
        CHECK_CALLED(GetOptionKeyPath);
        CHECK_CALLED(GetOverrideKeyPath);
        CHECK_CALLED(GetWindow);
        CHECK_CALLED(Exec_DOCCANNAVIGATE);
        CHECK_CALLED(QueryStatus_SETPROGRESSTEXT);
        CHECK_CALLED(Exec_SETPROGRESSMAX);
        CHECK_CALLED(Exec_SETPROGRESSPOS);
        CHECK_CALLED(Exec_ShellDocView_37);
    }
    if(!container_locked) {
        CHECK_CALLED(GetContainer);
        CHECK_CALLED(LockContainer);
        container_locked = TRUE;
    }
    CHECK_CALLED(OnChanged_READYSTATE);
    SET_CALLED(IsSystemMoniker); /* IE7 */
    SET_CALLED(Exec_ShellDocView_84);
    if(mon == &Moniker)
        CHECK_CALLED(BindToStorage);
    SET_CALLED(SetActiveObject); /* FIXME */
    if(set_clientsite) {
        CHECK_CALLED(Invoke_AMBIENT_SILENT);
        CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        CHECK_CALLED(Exec_ShellDocView_37);
    }

    set_clientsite = container_locked = TRUE;

    test_GetCurMoniker((IUnknown*)persist, mon, NULL);

    IBindCtx_Release(bind);

    test_readyState((IUnknown*)persist);
}

#define DWL_VERBDONE  0x0001
#define DWL_CSS       0x0002
#define DWL_TRYCSS    0x0004
#define DWL_HTTP      0x0008
#define DWL_EMPTY     0x0010

static void test_download(DWORD flags)
{
    HWND hwnd;
    MSG msg;

    hwnd = FindWindowA("Internet Explorer_Hidden", NULL);
    ok(hwnd != NULL, "Could not find hidden window\n");

    test_readyState(NULL);

    if(flags & (DWL_VERBDONE|DWL_HTTP))
        SET_EXPECT(Exec_SETPROGRESSMAX);
    if((flags & DWL_VERBDONE) && !load_from_stream)
        SET_EXPECT(GetHostInfo);
    SET_EXPECT(SetStatusText);
    if(!(flags & DWL_EMPTY))
        SET_EXPECT(Exec_SETDOWNLOADSTATE_1);
    SET_EXPECT(OnViewChange);
    SET_EXPECT(GetDropTarget);
    if(flags & DWL_TRYCSS)
        SET_EXPECT(Exec_ShellDocView_84);
    if(flags & DWL_CSS) {
        SET_EXPECT(CreateInstance);
        SET_EXPECT(Start);
        SET_EXPECT(LockRequest);
        SET_EXPECT(Terminate);
        SET_EXPECT(Protocol_Read);
        SET_EXPECT(UnlockRequest);
    }
    SET_EXPECT(Exec_Explorer_69);
    SET_EXPECT(EnableModeless_TRUE); /* IE7 */
    SET_EXPECT(Frame_EnableModeless_TRUE); /* IE7 */
    SET_EXPECT(EnableModeless_FALSE); /* IE7 */
    SET_EXPECT(Frame_EnableModeless_FALSE); /* IE7 */
    if(navigated_load)
        SET_EXPECT(Exec_ShellDocView_37);
    if(flags & DWL_HTTP) {
        SET_EXPECT(OnChanged_1012);
        SET_EXPECT(Exec_HTTPEQUIV);
        SET_EXPECT(Exec_SETTITLE);
    }
    SET_EXPECT(OnChanged_1005);
    SET_EXPECT(OnChanged_READYSTATE);
    SET_EXPECT(Exec_SETPROGRESSPOS);
    if(!(flags & DWL_EMPTY))
        SET_EXPECT(Exec_SETDOWNLOADSTATE_0);
    SET_EXPECT(Exec_ShellDocView_103);
    SET_EXPECT(Exec_ShellDocView_105);
    SET_EXPECT(Exec_ShellDocView_140);
    SET_EXPECT(Exec_MSHTML_PARSECOMPLETE);
    SET_EXPECT(Exec_HTTPEQUIV_DONE);
    SET_EXPECT(SetStatusText);
    if(navigated_load) {
        SET_EXPECT(UpdateUI);
        SET_EXPECT(Exec_UPDATECOMMANDS);
        SET_EXPECT(Exec_SETTITLE);
    }
    expect_status_text = (LPWSTR)0xdeadbeef; /* TODO */

    while(!called_Exec_HTTPEQUIV_DONE && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(flags & DWL_VERBDONE)
        CHECK_CALLED(Exec_SETPROGRESSMAX);
    if(flags & DWL_HTTP)
        SET_CALLED(Exec_SETPROGRESSMAX);
    if((flags & DWL_VERBDONE) && !load_from_stream) {
        if(navigated_load)
            todo_wine CHECK_CALLED(GetHostInfo);
        else
            CHECK_CALLED(GetHostInfo);
    }
    CHECK_CALLED(SetStatusText);
    if(!(flags & DWL_EMPTY))
        CHECK_CALLED(Exec_SETDOWNLOADSTATE_1);
    CHECK_CALLED(OnViewChange);
    if(navigated_load)
        CHECK_CALLED(GetDropTarget);
    else
        SET_CALLED(GetDropTarget);
    if(flags & DWL_TRYCSS)
        SET_CALLED(Exec_ShellDocView_84);
    if(flags & DWL_CSS) {
        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(Start);
        CHECK_CALLED(LockRequest);
        CHECK_CALLED(Terminate);
        CHECK_CALLED(Protocol_Read);
        CHECK_CALLED(UnlockRequest);
    }
    SET_CALLED(Exec_Explorer_69);
    SET_CALLED(EnableModeless_TRUE); /* IE7 */
    SET_CALLED(Frame_EnableModeless_TRUE); /* IE7 */
    SET_CALLED(EnableModeless_FALSE); /* IE7 */
    SET_CALLED(Frame_EnableModeless_FALSE); /* IE7 */
    if(navigated_load)
        todo_wine CHECK_CALLED(Exec_ShellDocView_37);
    if(flags & DWL_HTTP) todo_wine {
        CHECK_CALLED(OnChanged_1012);
        CHECK_CALLED(Exec_HTTPEQUIV);
        CHECK_CALLED(Exec_SETTITLE);
    }
    CHECK_CALLED(OnChanged_1005);
    CHECK_CALLED(OnChanged_READYSTATE);
    CHECK_CALLED(Exec_SETPROGRESSPOS);
    if(!(flags & DWL_EMPTY))
        CHECK_CALLED(Exec_SETDOWNLOADSTATE_0);
    SET_CALLED(Exec_ShellDocView_103);
    SET_CALLED(Exec_ShellDocView_105);
    SET_CALLED(Exec_ShellDocView_140);
    CHECK_CALLED(Exec_MSHTML_PARSECOMPLETE);
    CHECK_CALLED(Exec_HTTPEQUIV_DONE);
    SET_CALLED(SetStatusText);
    if(navigated_load) { /* avoiding race, FIXME: fund better way */
        SET_CALLED(UpdateUI);
        SET_CALLED(Exec_UPDATECOMMANDS);
        SET_CALLED(Exec_SETTITLE);
    }

    load_state = LD_COMPLETE;

    test_readyState(NULL);
}

static void test_Persist(IHTMLDocument2 *doc, IMoniker *mon)
{
    IPersistMoniker *persist_mon;
    IPersistFile *persist_file;
    GUID guid;
    HRESULT hres;

    hres = IUnknown_QueryInterface(doc, &IID_IPersistFile, (void**)&persist_file);
    ok(hres == S_OK, "QueryInterface(IID_IPersist) failed: %08x\n", hres);
    if(SUCCEEDED(hres)) {
        hres = IPersist_GetClassID(persist_file, NULL);
        ok(hres == E_INVALIDARG, "GetClassID returned: %08x, expected E_INVALIDARG\n", hres);

        hres = IPersist_GetClassID(persist_file, &guid);
        ok(hres == S_OK, "GetClassID failed: %08x\n", hres);
        ok(IsEqualGUID(&CLSID_HTMLDocument, &guid), "guid != CLSID_HTMLDocument\n");

        IPersist_Release(persist_file);
    }

    hres = IUnknown_QueryInterface(doc, &IID_IPersistMoniker, (void**)&persist_mon);
    ok(hres == S_OK, "QueryInterface(IID_IPersistMoniker) failed: %08x\n", hres);
    if(SUCCEEDED(hres)) {
        hres = IPersistMoniker_GetClassID(persist_mon, NULL);
        ok(hres == E_INVALIDARG, "GetClassID returned: %08x, expected E_INVALIDARG\n", hres);

        hres = IPersistMoniker_GetClassID(persist_mon, &guid);
        ok(hres == S_OK, "GetClassID failed: %08x\n", hres);
        ok(IsEqualGUID(&CLSID_HTMLDocument, &guid), "guid != CLSID_HTMLDocument\n");

        if(load_state == LD_DOLOAD)
            test_Load(persist_mon, mon);

        test_readyState((IUnknown*)doc);

        IPersistMoniker_Release(persist_mon);
    }
}

static void test_put_href(IHTMLDocument2 *doc)
{
    IHTMLPrivateWindow *priv_window;
    IHTMLWindow2 *window;
    IHTMLLocation *location;
    BSTR str, str2;
    VARIANT vempty;
    HRESULT hres;

    location = NULL;
    hres = IHTMLDocument2_get_location(doc, &location);
    ok(hres == S_OK, "get_location failed: %08x\n", hres);
    ok(location != NULL, "location == NULL\n");

    SET_EXPECT(TranslateUrl);
    SET_EXPECT(Navigate);
    str = a2bstr("about:blank");
    hres = IHTMLLocation_put_href(location, str);
    ok(hres == S_OK, "put_href failed: %08x\n", hres);
    CHECK_CALLED(TranslateUrl);
    CHECK_CALLED(Navigate);

    IHTMLLocation_Release(location);

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok(hres == S_OK, "get_parentWindow failed: %08x\n", hres);

    hres = IHTMLWindow2_QueryInterface(window, &IID_IHTMLPrivateWindow, (void**)&priv_window);
    IHTMLWindow2_Release(window);
    ok(hres == S_OK, "QueryInterface(IID_IHTMLPrivateWindow) failed: %08x\n", hres);

    readystate_set_loading = TRUE;
    navigated_load = TRUE;
    SET_EXPECT(TranslateUrl);
    SET_EXPECT(Exec_ShellDocView_67);
    SET_EXPECT(Invoke_AMBIENT_SILENT);
    SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    SET_EXPECT(OnChanged_READYSTATE);
    SET_EXPECT(Exec_ShellDocView_63);

    str2 = a2bstr("");
    V_VT(&vempty) = VT_EMPTY;
    hres = IHTMLPrivateWindow_SuperNavigate(priv_window, str, str2, NULL, NULL, &vempty, &vempty, 0);
    SysFreeString(str);
    SysFreeString(str2);
    ok(hres == S_OK, "SuperNavigate failed: %08x\n", hres);

    CHECK_CALLED(TranslateUrl);
    CHECK_CALLED(Exec_ShellDocView_67);
    CHECK_CALLED(Invoke_AMBIENT_SILENT);
    CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    SET_CALLED(OnChanged_READYSTATE); /* not always called */
    CHECK_CALLED(Exec_ShellDocView_63);

    test_GetCurMoniker(doc_unk, doc_mon, NULL);
    IHTMLPrivateWindow_Release(priv_window);

    test_download(DWL_VERBDONE);
}

static void test_open_window(IHTMLDocument2 *doc)
{
    IHTMLWindow2 *window, *new_window;
    BSTR name, url;
    HRESULT hres;

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok(hres == S_OK, "get_parentWindow failed: %08x\n", hres);

    url = a2bstr("about:blank");
    name = a2bstr("test");
    new_window = (void*)0xdeadbeef;

    open_call = TRUE;

    SET_EXPECT(TranslateUrl);
    SET_EXPECT(EvaluateNewWindow);

    hres = IHTMLWindow2_open(window, url, name, NULL, VARIANT_FALSE, &new_window);
    todo_wine
    ok(hres == S_OK, "open failed: %08x\n", hres);
    todo_wine
    ok(new_window == NULL, "new_window != NULL\n");

    todo_wine
    CHECK_CALLED(TranslateUrl);
    todo_wine
    CHECK_CALLED(EvaluateNewWindow);

    open_call = FALSE;
    SysFreeString(url);
    SysFreeString(name);

    IHTMLWindow2_Release(window);
    SysFreeString(name);
}

static void test_clear(IHTMLDocument2 *doc)
{
    HRESULT hres;

    hres = IHTMLDocument2_clear(doc);
    ok(hres == S_OK, "clear failed: %08x\n", hres);
}

static const OLECMDF expect_cmds[OLECMDID_GETPRINTTEMPLATE+1] = {
    0,
    OLECMDF_SUPPORTED,                  /* OLECMDID_OPEN */
    OLECMDF_SUPPORTED,                  /* OLECMDID_NEW */
    OLECMDF_SUPPORTED,                  /* OLECMDID_SAVE */
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_SAVEAS */
    OLECMDF_SUPPORTED,                  /* OLECMDID_SAVECOPYAS */
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_PRINT */
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_PRINTPREVIEW */
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_PAGESETUP */
    OLECMDF_SUPPORTED,                  /* OLECMDID_SPELL */
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_PROPERTIES */
    OLECMDF_SUPPORTED,                  /* OLECMDID_CUT */
    OLECMDF_SUPPORTED,                  /* OLECMDID_COPY */
    OLECMDF_SUPPORTED,                  /* OLECMDID_PASTE */
    OLECMDF_SUPPORTED,                  /* OLECMDID_PASTESPECIAL */
    OLECMDF_SUPPORTED,                  /* OLECMDID_UNDO */
    OLECMDF_SUPPORTED,                  /* OLECMDID_REDO */
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_SELECTALL */
    OLECMDF_SUPPORTED,                  /* OLECMDID_CLEARSELECTION */
    OLECMDF_SUPPORTED,                  /* OLECMDID_ZOOM */
    OLECMDF_SUPPORTED,                  /* OLECMDID_GETZOOMRANGE */
    0,
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_REFRESH */
    OLECMDF_SUPPORTED|OLECMDF_ENABLED,  /* OLECMDID_STOP */
    0,0,0,0,0,0,
    OLECMDF_SUPPORTED,                  /* OLECMDID_STOPDOWNLOAD */
    0,0,
    OLECMDF_SUPPORTED,                  /* OLECMDID_DELETE */
    0,0,
    OLECMDF_SUPPORTED,                  /* OLECMDID_ENABLE_INTERACTION */
    OLECMDF_SUPPORTED,                  /* OLECMDID_ONUNLOAD */
    0,0,0,0,0,
    OLECMDF_SUPPORTED,                  /* OLECMDID_SHOWPAGESETUP */
    OLECMDF_SUPPORTED,                  /* OLECMDID_SHOWPRINT */
    0,0,
    OLECMDF_SUPPORTED,                  /* OLECMDID_CLOSE */
    0,0,0,
    OLECMDF_SUPPORTED,                  /* OLECMDID_SETPRINTTEMPLATE */
    OLECMDF_SUPPORTED                   /* OLECMDID_GETPRINTTEMPLATE */
};

#define test_QueryStatus(u,cgid,cmdid,cmdf) _test_QueryStatus(__LINE__,u,cgid,cmdid,cmdf)
static void _test_QueryStatus(unsigned line, IUnknown *unk, REFIID cgid, ULONG cmdid, DWORD cmdf)
{
    IOleCommandTarget *cmdtrg;
    OLECMD olecmd = {cmdid, 0};
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleCommandTarget, (void**)&cmdtrg);
    ok_(__FILE__,line) (hres == S_OK, "QueryInterface(IID_IOleCommandTarget failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IOleCommandTarget_QueryStatus(cmdtrg, cgid, 1, &olecmd, NULL);
    ok(hres == S_OK, "QueryStatus(%u) failed: %08x\n", cmdid, hres);

    IOleCommandTarget_Release(cmdtrg);

    ok_(__FILE__,line) (olecmd.cmdID == cmdid, "cmdID changed\n");
    ok_(__FILE__,line) (olecmd.cmdf == cmdf, "(%u) cmdf=%08x, expected %08x\n", cmdid, olecmd.cmdf, cmdf);
}

static void test_MSHTML_QueryStatus(IHTMLDocument2 *doc, DWORD cmdf)
{
    IUnknown *unk = doc ? (IUnknown*)doc : doc_unk;

    test_QueryStatus(unk, &CGID_MSHTML, IDM_FONTNAME, cmdf);
    test_QueryStatus(unk, &CGID_MSHTML, IDM_FONTSIZE, cmdf);
    test_QueryStatus(unk, &CGID_MSHTML, IDM_SELECTALL, cmdf|OLECMDF_ENABLED);
    test_QueryStatus(unk, &CGID_MSHTML, IDM_BOLD, cmdf);
    test_QueryStatus(unk, &CGID_MSHTML, IDM_FORECOLOR, cmdf);
    test_QueryStatus(unk, &CGID_MSHTML, IDM_JUSTIFYCENTER, cmdf);
    test_QueryStatus(unk, &CGID_MSHTML, IDM_JUSTIFYLEFT, cmdf);
    test_QueryStatus(unk, &CGID_MSHTML, IDM_JUSTIFYRIGHT, cmdf);
    test_QueryStatus(unk, &CGID_MSHTML, IDM_ITALIC, cmdf);
    test_QueryStatus(unk, &CGID_MSHTML, IDM_UNDERLINE, cmdf);
    test_QueryStatus(unk, &CGID_MSHTML, IDM_HORIZONTALLINE, cmdf);
    test_QueryStatus(unk, &CGID_MSHTML, IDM_ORDERLIST, cmdf);
    test_QueryStatus(unk, &CGID_MSHTML, IDM_UNORDERLIST, cmdf);
    test_QueryStatus(unk, &CGID_MSHTML, IDM_INDENT, cmdf);
    test_QueryStatus(unk, &CGID_MSHTML, IDM_OUTDENT, cmdf);
    test_QueryStatus(unk, &CGID_MSHTML, IDM_DELETE, cmdf);
}

static void test_OleCommandTarget(IHTMLDocument2 *doc)
{
    IOleCommandTarget *cmdtrg;
    OLECMD cmds[OLECMDID_GETPRINTTEMPLATE];
    int i;
    HRESULT hres;

    hres = IUnknown_QueryInterface(doc, &IID_IOleCommandTarget, (void**)&cmdtrg);
    ok(hres == S_OK, "QueryInterface(IID_IOleCommandTarget failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    for(i=0; i<OLECMDID_GETPRINTTEMPLATE; i++) {
        cmds[i].cmdID = i+1;
        cmds[i].cmdf = 0xf0f0;
    }

    SET_EXPECT(QueryStatus_OPEN);
    SET_EXPECT(QueryStatus_NEW);
    hres = IOleCommandTarget_QueryStatus(cmdtrg, NULL, sizeof(cmds)/sizeof(cmds[0]), cmds, NULL);
    ok(hres == S_OK, "QueryStatus failed: %08x\n", hres);
    CHECK_CALLED(QueryStatus_OPEN);
    CHECK_CALLED(QueryStatus_NEW);

    for(i=0; i<OLECMDID_GETPRINTTEMPLATE; i++) {
        ok(cmds[i].cmdID == i+1, "cmds[%d].cmdID canged to %x\n", i, cmds[i].cmdID);
        if(i+1 == OLECMDID_FIND)
            continue;
        ok(cmds[i].cmdf == expect_cmds[i+1], "cmds[%d].cmdf=%x, expected %x\n",
                i+1, cmds[i].cmdf, expect_cmds[i+1]);
    }

    ok(!cmds[OLECMDID_FIND-1].cmdf || cmds[OLECMDID_FIND-1].cmdf == (OLECMDF_SUPPORTED|OLECMDF_ENABLED),
       "cmds[OLECMDID_FIND].cmdf=%x\n", cmds[OLECMDID_FIND-1].cmdf);

    IOleCommandTarget_Release(cmdtrg);
}

static void test_OleCommandTarget_fail(IHTMLDocument2 *doc)
{
    IOleCommandTarget *cmdtrg;
    int i;
    HRESULT hres;

    OLECMD cmd[2] = {
        {OLECMDID_OPEN, 0xf0f0},
        {OLECMDID_GETPRINTTEMPLATE+1, 0xf0f0}
    };

    hres = IUnknown_QueryInterface(doc, &IID_IOleCommandTarget, (void**)&cmdtrg);
    ok(hres == S_OK, "QueryInterface(IIDIOleCommandTarget failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IOleCommandTarget_QueryStatus(cmdtrg, NULL, 0, NULL, NULL);
    ok(hres == S_OK, "QueryStatus failed: %08x\n", hres);

    SET_EXPECT(QueryStatus_OPEN);
    hres = IOleCommandTarget_QueryStatus(cmdtrg, NULL, 2, cmd, NULL);
    CHECK_CALLED(QueryStatus_OPEN);

    ok(hres == OLECMDERR_E_NOTSUPPORTED,
            "QueryStatus failed: %08x, expected OLECMDERR_E_NOTSUPPORTED\n", hres);
    ok(cmd[1].cmdID == OLECMDID_GETPRINTTEMPLATE+1,
            "cmd[0].cmdID=%d, expected OLECMDID_GETPRINTTEMPLATE+1\n", cmd[0].cmdID);
    ok(cmd[1].cmdf == 0, "cmd[0].cmdf=%x, expected 0\n", cmd[0].cmdf);
    ok(cmd[0].cmdf == OLECMDF_SUPPORTED,
            "cmd[1].cmdf=%x, expected OLECMDF_SUPPORTED\n", cmd[1].cmdf);

    hres = IOleCommandTarget_QueryStatus(cmdtrg, &IID_IHTMLDocument2, 2, cmd, NULL);
    ok(hres == OLECMDERR_E_UNKNOWNGROUP,
            "QueryStatus failed: %08x, expected OLECMDERR_E_UNKNOWNGROUP\n", hres);

    for(i=0; i<OLECMDID_GETPRINTTEMPLATE; i++) {
        if(!expect_cmds[i]) {
            hres = IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_UPDATECOMMANDS,
                    OLECMDEXECOPT_DODEFAULT, NULL, NULL);
            ok(hres == OLECMDERR_E_NOTSUPPORTED,
                    "Exec failed: %08x, expected OLECMDERR_E_NOTSUPPORTED\n", hres);
        }
    }

    hres = IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_GETPRINTTEMPLATE+1,
            OLECMDEXECOPT_DODEFAULT, NULL, NULL);
    ok(hres == OLECMDERR_E_NOTSUPPORTED,
            "Exec failed: %08x, expected OLECMDERR_E_NOTSUPPORTED\n", hres);

    IOleCommandTarget_Release(cmdtrg);
}

static void test_exec_onunload(IHTMLDocument2 *doc)
{
    IOleCommandTarget *cmdtrg;
    VARIANT var;
    HRESULT hres;

    hres = IUnknown_QueryInterface(doc, &IID_IOleCommandTarget, (void**)&cmdtrg);
    ok(hres == S_OK, "QueryInterface(IID_IOleCommandTarget) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    memset(&var, 0x0a, sizeof(var));
    hres = IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_ONUNLOAD,
            OLECMDEXECOPT_DODEFAULT, NULL, &var);
    ok(hres == S_OK, "Exec(..., OLECMDID_ONUNLOAD, ...) failed: %08x\n", hres);
    ok(V_VT(&var) == VT_BOOL, "V_VT(var)=%d, expected VT_BOOL\n", V_VT(&var));
    ok(V_BOOL(&var) == VARIANT_TRUE, "V_BOOL(var)=%x, expected VARIANT_TRUE\n", V_BOOL(&var));

    hres = IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_ONUNLOAD,
            OLECMDEXECOPT_DODEFAULT, NULL, NULL);
    ok(hres == S_OK, "Exec(..., OLECMDID_ONUNLOAD, ...) failed: %08x\n", hres);

    IOleCommandTarget_Release(cmdtrg);
}

static void test_exec_editmode(IUnknown *unk, BOOL loaded)
{
    IOleCommandTarget *cmdtrg;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleCommandTarget, (void**)&cmdtrg);
    ok(hres == S_OK, "QueryInterface(IID_IOleCommandTarget) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    editmode = TRUE;

    if(loaded)
        load_state = LD_DOLOAD;

    if(loaded)
        SET_EXPECT(GetClassID);
    SET_EXPECT(SetStatusText);
    SET_EXPECT(Exec_ShellDocView_37);
    SET_EXPECT(GetHostInfo);
    if(loaded)
        SET_EXPECT(GetDisplayName);
    SET_EXPECT(Invoke_AMBIENT_SILENT);
    SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    SET_EXPECT(OnChanged_READYSTATE);
    SET_EXPECT(IsSystemMoniker);
    SET_EXPECT(Exec_ShellDocView_84);
    if(loaded)
        SET_EXPECT(BindToStorage);
    SET_EXPECT(InPlaceUIWindow_SetActiveObject);
    SET_EXPECT(HideUI);
    SET_EXPECT(ShowUI);
    SET_EXPECT(InPlaceFrame_SetBorderSpace);

    expect_status_text = NULL;
    readystate_set_loading = TRUE;

    hres = IOleCommandTarget_Exec(cmdtrg, &CGID_MSHTML, IDM_EDITMODE,
            OLECMDEXECOPT_DODEFAULT, NULL, NULL);
    ok(hres == S_OK, "Exec failed: %08x\n", hres);

    if(loaded)
        CHECK_CALLED(GetClassID);
    CHECK_CALLED(SetStatusText);
    CHECK_CALLED(Exec_ShellDocView_37);
    CHECK_CALLED(GetHostInfo);
    if(loaded)
        CHECK_CALLED(GetDisplayName);
    CHECK_CALLED(Invoke_AMBIENT_SILENT);
    CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    CHECK_CALLED(OnChanged_READYSTATE);
    SET_CALLED(IsSystemMoniker); /* IE7 */
    SET_CALLED(Exec_ShellDocView_84);
    if(loaded)
        CHECK_CALLED(BindToStorage);
    CHECK_CALLED(InPlaceUIWindow_SetActiveObject);
    CHECK_CALLED(HideUI);
    CHECK_CALLED(ShowUI);
    CHECK_CALLED(InPlaceFrame_SetBorderSpace);

    test_timer(EXPECT_UPDATEUI|EXPECT_SETTITLE);

    IOleCommandTarget_Release(cmdtrg);

    hres = IOleCommandTarget_Exec(cmdtrg, &CGID_MSHTML, IDM_EDITMODE,
            OLECMDEXECOPT_DODEFAULT, NULL, NULL);
    ok(hres == S_OK, "Exec failed: %08x\n", hres);
}

static void test_exec_fontname(IUnknown *unk, LPCWSTR name, LPCWSTR exname)
{
   IOleCommandTarget *cmdtrg;
   VARIANT *in = NULL, _in, *out = NULL, _out;
   HRESULT hres;

   hres = IUnknown_QueryInterface(unk, &IID_IOleCommandTarget, (void**)&cmdtrg);
   ok(hres == S_OK, "QueryInterface(IIDIOleM=CommandTarget failed: %08x\n", hres);
   if(FAILED(hres))
       return;

   if(name) {
       in = &_in;
       V_VT(in) = VT_BSTR;
       V_BSTR(in) = SysAllocString(name);
   }

   if(exname) {
       out = &_out;
       V_VT(out) = VT_I4;
       V_I4(out) = 0xdeadbeef;
   }

   hres = IOleCommandTarget_Exec(cmdtrg, &CGID_MSHTML, IDM_FONTNAME, 0, in, out);
   ok(hres == S_OK, "Exec(IDM_FONTNAME) failed: %08x\n", hres);

   if(in)
       VariantClear(in);

   if(out) {
       ok(V_VT(out) == VT_BSTR, "V_VT(out) = %x\n", V_VT(out));
       if(V_VT(out) == VT_BSTR) {
           if(exname)
               ok(!lstrcmpW(V_BSTR(out), name ? name : exname),
                  "unexpected fontname %s\n", wine_dbgstr_w(name));
           else
               ok(V_BSTR(out) == NULL, "V_BSTR(out) != NULL\n");
       }
       VariantClear(out);
   }

   IOleCommandTarget_Release(cmdtrg);
}

static void test_exec_noargs(IUnknown *unk, DWORD cmdid)
{
    IOleCommandTarget *cmdtrg;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleCommandTarget, (void**)&cmdtrg);
    ok(hres == S_OK, "QueryInterface(IID_IOleCommandTarget) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IOleCommandTarget_Exec(cmdtrg, &CGID_MSHTML, cmdid,
            OLECMDEXECOPT_DODEFAULT, NULL, NULL);
    ok(hres == S_OK, "Exec failed: %08x\n", hres);

    IOleCommandTarget_Release(cmdtrg);
}

static void test_IsDirty(IHTMLDocument2 *doc, HRESULT exhres)
{
    IPersistStreamInit *perinit;
    IPersistMoniker *permon;
    IPersistFile *perfile;
    HRESULT hres;

    hres = IUnknown_QueryInterface(doc, &IID_IPersistStreamInit, (void**)&perinit);
    ok(hres == S_OK, "QueryInterface(IID_IPersistStreamInit failed: %08x\n", hres);
    if(SUCCEEDED(hres)) {
        hres = IPersistStreamInit_IsDirty(perinit);
        ok(hres == exhres, "IsDirty() = %08x, expected %08x\n", hres, exhres);
        IPersistStreamInit_Release(perinit);
    }

    hres = IUnknown_QueryInterface(doc, &IID_IPersistMoniker, (void**)&permon);
    ok(hres == S_OK, "QueryInterface(IID_IPersistMoniker failed: %08x\n", hres);
    if(SUCCEEDED(hres)) {
        hres = IPersistMoniker_IsDirty(permon);
        ok(hres == exhres, "IsDirty() = %08x, expected %08x\n", hres, exhres);
        IPersistMoniker_Release(permon);
    }

    hres = IUnknown_QueryInterface(doc, &IID_IPersistFile, (void**)&perfile);
    ok(hres == S_OK, "QueryInterface(IID_IPersistFile failed: %08x\n", hres);
    if(SUCCEEDED(hres)) {
        hres = IPersistFile_IsDirty(perfile);
        ok(hres == exhres, "IsDirty() = %08x, expected %08x\n", hres, exhres);
        IPersistFile_Release(perfile);
    }
}

static HWND create_container_window(void)
{
    static const WCHAR wszHTMLDocumentTest[] =
        {'H','T','M','L','D','o','c','u','m','e','n','t','T','e','s','t',0};
    static WNDCLASSEXW wndclass = {
        sizeof(WNDCLASSEXW),
        0,
        wnd_proc,
        0, 0, NULL, NULL, NULL, NULL, NULL,
        wszHTMLDocumentTest,
        NULL
    };

    RegisterClassExW(&wndclass);
    return CreateWindowW(wszHTMLDocumentTest, wszHTMLDocumentTest,
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            515, 530, NULL, NULL, NULL, NULL);
}

static void test_DoVerb(IOleObject *oleobj)
{
    RECT rect = {0,0,500,500};
    HRESULT hres;

    if(!container_locked) {
        SET_EXPECT(GetContainer);
        SET_EXPECT(LockContainer);
    }
    SET_EXPECT(ActivateMe);
    expect_LockContainer_fLock = TRUE;

    hres = IOleObject_DoVerb(oleobj, OLEIVERB_SHOW, NULL, &ClientSite, -1, container_hwnd, &rect);
    ok(hres == S_OK, "DoVerb failed: %08x\n", hres);

    if(!container_locked) {
        CHECK_CALLED(GetContainer);
        CHECK_CALLED(LockContainer);
        container_locked = TRUE;
    }
    CHECK_CALLED(ActivateMe);
}

#define CLIENTSITE_EXPECTPATH 0x00000001
#define CLIENTSITE_SETNULL    0x00000002
#define CLIENTSITE_DONTSET    0x00000004

static void test_ClientSite(IOleObject *oleobj, DWORD flags)
{
    IOleClientSite *clientsite;
    HRESULT hres;

    if(flags & CLIENTSITE_SETNULL) {
        hres = IOleObject_GetClientSite(oleobj, &clientsite);
        ok(clientsite == &ClientSite, "clientsite=%p, expected %p\n", clientsite, &ClientSite);

        hres = IOleObject_SetClientSite(oleobj, NULL);
        ok(hres == S_OK, "SetClientSite failed: %08x\n", hres);

        set_clientsite = FALSE;
    }

    if(flags & CLIENTSITE_DONTSET)
        return;

    hres = IOleObject_GetClientSite(oleobj, &clientsite);
    ok(hres == S_OK, "GetClientSite failed: %08x\n", hres);
    ok(clientsite == (set_clientsite ? &ClientSite : NULL), "GetClientSite() = %p, expected %p\n",
            clientsite, set_clientsite ? &ClientSite : NULL);

    if(!set_clientsite) {
        SET_EXPECT(GetHostInfo);
        if(flags & CLIENTSITE_EXPECTPATH) {
            SET_EXPECT(GetOptionKeyPath);
            SET_EXPECT(GetOverrideKeyPath);
        }
        SET_EXPECT(GetWindow);
        if(flags & CLIENTSITE_EXPECTPATH)
            SET_EXPECT(Exec_DOCCANNAVIGATE);
        SET_EXPECT(QueryStatus_SETPROGRESSTEXT);
        SET_EXPECT(Exec_SETPROGRESSMAX);
        SET_EXPECT(Exec_SETPROGRESSPOS);
        SET_EXPECT(Invoke_AMBIENT_USERMODE);
        SET_EXPECT(Invoke_AMBIENT_DLCONTROL);
        SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        SET_EXPECT(Invoke_AMBIENT_SILENT);
        SET_EXPECT(Invoke_AMBIENT_USERAGENT);
        SET_EXPECT(Invoke_AMBIENT_PALETTE);

        hres = IOleObject_SetClientSite(oleobj, &ClientSite);
        ok(hres == S_OK, "SetClientSite failed: %08x\n", hres);

        CHECK_CALLED(GetHostInfo);
        if(flags & CLIENTSITE_EXPECTPATH) {
            CHECK_CALLED(GetOptionKeyPath);
            CHECK_CALLED(GetOverrideKeyPath);
        }
        CHECK_CALLED(GetWindow);
        if(flags & CLIENTSITE_EXPECTPATH)
            CHECK_CALLED(Exec_DOCCANNAVIGATE);
        CHECK_CALLED(QueryStatus_SETPROGRESSTEXT);
        CHECK_CALLED(Exec_SETPROGRESSMAX);
        CHECK_CALLED(Exec_SETPROGRESSPOS);
        CHECK_CALLED(Invoke_AMBIENT_USERMODE);
        CHECK_CALLED(Invoke_AMBIENT_DLCONTROL);
        CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED); 
        CHECK_CALLED(Invoke_AMBIENT_SILENT);
        CHECK_CALLED(Invoke_AMBIENT_USERAGENT);
        CHECK_CALLED(Invoke_AMBIENT_PALETTE);

        set_clientsite = TRUE;
    }

    hres = IOleObject_SetClientSite(oleobj, &ClientSite);
    ok(hres == S_OK, "SetClientSite failed: %08x\n", hres);

    hres = IOleObject_GetClientSite(oleobj, &clientsite);
    ok(hres == S_OK, "GetClientSite failed: %08x\n", hres);
    ok(clientsite == &ClientSite, "GetClientSite() = %p, expected %p\n", clientsite, &ClientSite);
}

static void test_OnAmbientPropertyChange(IHTMLDocument2 *doc)
{
    IOleControl *control = NULL;
    HRESULT hres;

    hres = IUnknown_QueryInterface(doc, &IID_IOleControl, (void**)&control);
    ok(hres == S_OK, "QueryInterface(IID_IOleControl failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    SET_EXPECT(Invoke_AMBIENT_USERMODE);
    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_AMBIENT_USERMODE);
    ok(hres == S_OK, "OnAmbientChange failed: %08x\n", hres);
    CHECK_CALLED(Invoke_AMBIENT_USERMODE);

    SET_EXPECT(Invoke_AMBIENT_DLCONTROL);
    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_AMBIENT_DLCONTROL);
    ok(hres == S_OK, "OnAmbientChange failed: %08x\n", hres);
    CHECK_CALLED(Invoke_AMBIENT_DLCONTROL);

    SET_EXPECT(Invoke_AMBIENT_DLCONTROL);
    SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_AMBIENT_OFFLINEIFNOTCONNECTED);
    ok(hres == S_OK, "OnAmbientChange failed: %08x\n", hres);
    CHECK_CALLED(Invoke_AMBIENT_DLCONTROL);
    CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);

    SET_EXPECT(Invoke_AMBIENT_DLCONTROL);
    SET_EXPECT(Invoke_AMBIENT_SILENT);
    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_AMBIENT_SILENT);
    ok(hres == S_OK, "OnAmbientChange failed: %08x\n", hres);
    CHECK_CALLED(Invoke_AMBIENT_DLCONTROL);
    CHECK_CALLED(Invoke_AMBIENT_SILENT);

    SET_EXPECT(Invoke_AMBIENT_USERAGENT);
    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_AMBIENT_USERAGENT);
    ok(hres == S_OK, "OnAmbientChange failed: %08x\n", hres);
    CHECK_CALLED(Invoke_AMBIENT_USERAGENT);

    SET_EXPECT(Invoke_AMBIENT_PALETTE);
    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_AMBIENT_PALETTE);
    ok(hres == S_OK, "OnAmbientChange failed: %08x\n", hres);
    CHECK_CALLED(Invoke_AMBIENT_PALETTE);

    IOleControl_Release(control);
}



static void test_OnAmbientPropertyChange2(IHTMLDocument2 *doc)
{
    IOleControl *control = NULL;
    HRESULT hres;

    hres = IUnknown_QueryInterface(doc, &IID_IOleControl, (void**)&control);
    ok(hres == S_OK, "QueryInterface(IID_IOleControl failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_AMBIENT_PALETTE);
    ok(hres == S_OK, "OnAmbientPropertyChange failed: %08x\n", hres);

    IOleControl_Release(control);
}

static void test_Close(IHTMLDocument2 *doc, BOOL set_client)
{
    IOleObject *oleobj = NULL;
    HRESULT hres;

    hres = IUnknown_QueryInterface(doc, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_IOleObject) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    SET_EXPECT(GetContainer);
    SET_EXPECT(LockContainer);
    expect_LockContainer_fLock = FALSE;
    hres = IOleObject_Close(oleobj, OLECLOSE_NOSAVE);
    ok(hres == S_OK, "Close failed: %08x\n", hres);
    CHECK_CALLED(GetContainer);
    CHECK_CALLED(LockContainer);
    container_locked = FALSE;

    if(set_client)
        test_ClientSite(oleobj, CLIENTSITE_SETNULL|CLIENTSITE_DONTSET);

    IOleObject_Release(oleobj);
}

static void test_Advise(IHTMLDocument2 *doc)
{
    IOleObject *oleobj = NULL;
    IEnumSTATDATA *enum_advise = (void*)0xdeadbeef;
    DWORD conn;
    HRESULT hres;

    hres = IUnknown_QueryInterface(doc, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_IOleObject) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IOleObject_Unadvise(oleobj, 0);
    ok(hres == OLE_E_NOCONNECTION, "Unadvise returned: %08x\n", hres);

    hres = IOleObject_EnumAdvise(oleobj, &enum_advise);
    ok(hres == S_OK, "EnumAdvise returned: %08x\n", hres);
    ok(enum_advise == NULL, "enum_advise != NULL\n");

    conn = -1;
    hres = IOleObject_Advise(oleobj, NULL, &conn);
    /* Old IE returns S_OK and sets conn to 1 */
    ok(hres == E_INVALIDARG || hres == S_OK, "Advise returned: %08x\n", hres);
    ok(conn == 0 || conn == 1, "conn = %d\n", conn);

    hres = IOleObject_Advise(oleobj, (IAdviseSink*)&AdviseSink, NULL);
    ok(hres == E_INVALIDARG, "Advise returned: %08x\n", hres);

    hres = IOleObject_Advise(oleobj, (IAdviseSink*)&AdviseSink, &conn);
    ok(hres == S_OK, "Advise returned: %08x\n", hres);
    ok(conn == 1, "conn = %d\n", conn);

    hres = IOleObject_Advise(oleobj, (IAdviseSink*)&AdviseSink, &conn);
    ok(hres == S_OK, "Advise returned: %08x\n", hres);
    ok(conn == 2, "conn = %d\n", conn);

    hres = IOleObject_Unadvise(oleobj, 1);
    ok(hres == S_OK, "Unadvise returned: %08x\n", hres);

    hres = IOleObject_Unadvise(oleobj, 1);
    ok(hres == OLE_E_NOCONNECTION, "Unadvise returned: %08x\n", hres);

    hres = IOleObject_Unadvise(oleobj, 2);
    ok(hres == S_OK, "Unadvise returned: %08x\n", hres);

    IOleObject_Release(oleobj);
}

static void test_OnFrameWindowActivate(IUnknown *unk)
{
    IOleInPlaceActiveObject *inplaceact;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleInPlaceActiveObject, (void**)&inplaceact);
    ok(hres == S_OK, "QueryInterface(IID_IOleInPlaceActiveObject) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    if(set_clientsite) {
        expect_OnFrameWindowActivate_fActivate = TRUE;
        SET_EXPECT(OnFrameWindowActivate);
        hres = IOleInPlaceActiveObject_OnFrameWindowActivate(inplaceact, TRUE);
        ok(hres == S_OK, "OnFrameWindowActivate failed: %08x\n", hres);
        CHECK_CALLED(OnFrameWindowActivate);

        SET_EXPECT(OnFrameWindowActivate);
        hres = IOleInPlaceActiveObject_OnFrameWindowActivate(inplaceact, TRUE);
        ok(hres == S_OK, "OnFrameWindowActivate failed: %08x\n", hres);
        CHECK_CALLED(OnFrameWindowActivate);

        expect_OnFrameWindowActivate_fActivate = FALSE;
        SET_EXPECT(OnFrameWindowActivate);
        hres = IOleInPlaceActiveObject_OnFrameWindowActivate(inplaceact, FALSE);
        ok(hres == S_OK, "OnFrameWindowActivate failed: %08x\n", hres);
        CHECK_CALLED(OnFrameWindowActivate);

        expect_OnFrameWindowActivate_fActivate = TRUE;
        SET_EXPECT(OnFrameWindowActivate);
        hres = IOleInPlaceActiveObject_OnFrameWindowActivate(inplaceact, TRUE);
        ok(hres == S_OK, "OnFrameWindowActivate failed: %08x\n", hres);
        CHECK_CALLED(OnFrameWindowActivate);
    }else {
        hres = IOleInPlaceActiveObject_OnFrameWindowActivate(inplaceact, FALSE);
        ok(hres == S_OK, "OnFrameWindowActivate failed: %08x\n", hres);

        hres = IOleInPlaceActiveObject_OnFrameWindowActivate(inplaceact, TRUE);
        ok(hres == S_OK, "OnFrameWindowActivate failed: %08x\n", hres);
    }

    IOleInPlaceActiveObject_Release(inplaceact);
}

static void test_InPlaceDeactivate(IHTMLDocument2 *doc, BOOL expect_call)
{
    IOleInPlaceObjectWindowless *windowlessobj = NULL;
    HRESULT hres;

    hres = IUnknown_QueryInterface(doc, &IID_IOleInPlaceObjectWindowless,
            (void**)&windowlessobj);
    ok(hres == S_OK, "QueryInterface(IID_IOleInPlaceObjectWindowless) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    if(expect_call) {
        SET_EXPECT(OnFocus_FALSE);
        if(ipsex)
            SET_EXPECT(OnInPlaceDeactivateEx);
        else
            SET_EXPECT(OnInPlaceDeactivate);
    }
    hres = IOleInPlaceObjectWindowless_InPlaceDeactivate(windowlessobj);
    ok(hres == S_OK, "InPlaceDeactivate failed: %08x\n", hres);
    if(expect_call) {
        CHECK_CALLED(OnFocus_FALSE);
        if(ipsex)
            CHECK_CALLED(OnInPlaceDeactivateEx);
        else
            CHECK_CALLED(OnInPlaceDeactivate);
    }

    IOleInPlaceObjectWindowless_Release(windowlessobj);
}

static void test_Activate(IHTMLDocument2 *doc, DWORD flags)
{
    IOleObject *oleobj = NULL;
    IOleDocumentView *docview;
    GUID guid;
    HRESULT hres;

    last_hwnd = hwnd;

    if(view)
        IOleDocumentView_Release(view);
    view = NULL;

    hres = IUnknown_QueryInterface(doc, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_IOleObject) failed: %08x\n", hres);

    hres = IOleObject_GetUserClassID(oleobj, NULL);
    ok(hres == E_INVALIDARG, "GetUserClassID returned: %08x, expected E_INVALIDARG\n", hres);

    hres = IOleObject_GetUserClassID(oleobj, &guid);
    ok(hres == S_OK, "GetUserClassID failed: %08x\n", hres);
    ok(IsEqualGUID(&guid, &CLSID_HTMLDocument), "guid != CLSID_HTMLDocument\n");

    test_OnFrameWindowActivate((IUnknown*)doc);

    test_ClientSite(oleobj, flags);
    test_InPlaceDeactivate(doc, FALSE);
    test_DoVerb(oleobj);

    if(call_UIActivate == CallUIActivate_AfterShow) {
        hres = IOleObject_QueryInterface(oleobj, &IID_IOleDocumentView, (void **)&docview);
        ok(hres == S_OK, "IOleObject_QueryInterface failed with error 0x%08x\n", hres);

        SET_EXPECT(OnFocus_TRUE);
        SET_EXPECT(SetActiveObject);
        SET_EXPECT(ShowUI);
        SET_EXPECT(InPlaceUIWindow_SetActiveObject);
        SET_EXPECT(InPlaceFrame_SetBorderSpace);
        expect_status_text = NULL;

        hres = IOleDocumentView_UIActivate(docview, TRUE);
        ok(hres == S_OK, "IOleDocumentView_UIActivate failed with error 0x%08x\n", hres);

        CHECK_CALLED(OnFocus_TRUE);
        CHECK_CALLED(SetActiveObject);
        CHECK_CALLED(ShowUI);
        CHECK_CALLED(InPlaceUIWindow_SetActiveObject);
        CHECK_CALLED(InPlaceFrame_SetBorderSpace);

        IOleDocumentView_Release(docview);
    }

    IOleObject_Release(oleobj);

    test_OnFrameWindowActivate((IUnknown*)doc);
}

static void test_Window(IHTMLDocument2 *doc, BOOL expect_success)
{
    IOleInPlaceActiveObject *activeobject = NULL;
    HWND tmp_hwnd;
    HRESULT hres;

    hres = IOleDocumentView_QueryInterface(view, &IID_IOleInPlaceActiveObject, (void**)&activeobject);
    ok(hres == S_OK, "Could not get IOleInPlaceActiveObject interface: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IOleInPlaceActiveObject_GetWindow(activeobject, &tmp_hwnd);

    if(expect_success) {
        ok(hres == S_OK, "GetWindow failed: %08x\n", hres);
        ok(tmp_hwnd == hwnd, "tmp_hwnd=%p, expected %p\n", tmp_hwnd, hwnd);
    }else {
        ok(hres == E_FAIL, "GetWindow returned %08x, expected E_FAIL\n", hres);
        ok(IsWindow(hwnd), "hwnd is destroyed\n");
    }

    IOleInPlaceActiveObject_Release(activeobject);
}

static void test_CloseView(void)
{
    IOleInPlaceSite *inplacesite = (IOleInPlaceSite*)0xff00ff00;
    HRESULT hres;

    if(!view)
        return;

    hres = IOleDocumentView_Show(view, FALSE);
    ok(hres == S_OK, "Show failed: %08x\n", hres);

    hres = IOleDocumentView_CloseView(view, 0);
    ok(hres == S_OK, "CloseView failed: %08x\n", hres);

    hres = IOleDocumentView_SetInPlaceSite(view, NULL);
    ok(hres == S_OK, "SetInPlaceSite failed: %08x\n", hres);

    hres = IOleDocumentView_GetInPlaceSite(view, &inplacesite);
    ok(hres == S_OK, "SetInPlaceSite failed: %08x\n", hres);
    ok(inplacesite == NULL, "inplacesite=%p, expected NULL\n", inplacesite);
}

static void test_UIDeactivate(void)
{
    HRESULT hres;

    if(call_UIActivate == CallUIActivate_AfterShow) {
        SET_EXPECT(InPlaceUIWindow_SetActiveObject);
    }
    if(call_UIActivate != CallUIActivate_None) {
        SET_EXPECT(SetActiveObject_null);
        SET_EXPECT(HideUI);
        SET_EXPECT(OnUIDeactivate);
    }

    hres = IOleDocumentView_UIActivate(view, FALSE);
    ok(hres == S_OK, "UIActivate failed: %08x\n", hres);

    if(call_UIActivate != CallUIActivate_None) {
        CHECK_CALLED(SetActiveObject_null);
        CHECK_CALLED(HideUI);
        CHECK_CALLED(OnUIDeactivate);
    }
    if(call_UIActivate == CallUIActivate_AfterShow) {
        CHECK_CALLED(InPlaceUIWindow_SetActiveObject);
    }
}

static void test_Hide(void)
{
    HRESULT hres;

    if(!view)
        return;

    hres = IOleDocumentView_Show(view, FALSE);
    ok(hres == S_OK, "Show failed: %08x\n", hres);
}

static HRESULT create_document(IHTMLDocument2 **doc)
{
    IHTMLDocument5 *doc5;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_HTMLDocument, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IHTMLDocument2, (void**)doc);
    ok(hres == S_OK, "CoCreateInstance failed: %08x\n", hres);
    if(FAILED(hres))
        return hres;

    hres = IHTMLDocument2_QueryInterface(*doc, &IID_IHTMLDocument5, (void**)&doc5);
    if(SUCCEEDED(hres)) {
        IHTMLDocument5_Release(doc5);
    }else {
        win_skip("Could not get IHTMLDocument5, probably too old IE\n");
        IHTMLDocument2_Release(*doc);
    }

    return hres;
}

static void test_Navigate(IHTMLDocument2 *doc)
{
    IHlinkTarget *hlink;
    HRESULT hres;

    hres = IUnknown_QueryInterface(doc, &IID_IHlinkTarget, (void**)&hlink);
    ok(hres == S_OK, "QueryInterface(IID_IHlinkTarget) failed: %08x\n", hres);

    SET_EXPECT(ActivateMe);
    hres = IHlinkTarget_Navigate(hlink, 0, NULL);
    ok(hres == S_OK, "Navigate failed: %08x\n", hres);
    CHECK_CALLED(ActivateMe);

    IHlinkTarget_Release(hlink);
}

static void test_external(IHTMLDocument2 *doc, BOOL initialized)
{
    IDispatch *external;
    IHTMLWindow2 *htmlwin;
    HRESULT hres;

    hres = IHTMLDocument2_get_parentWindow(doc, &htmlwin);
    ok(hres == S_OK, "get_parentWindow failed: %08x\n", hres);

    if(initialized)
        SET_EXPECT(GetExternal);
    external = (void*)0xdeadbeef;
    hres = IHTMLWindow2_get_external(htmlwin, &external);
    if(initialized) {
        ok(hres == S_FALSE || hres == S_OK, "get_external failed: %08x\n", hres);
        CHECK_CALLED(GetExternal);
        ok(external != NULL, "external == NULL\n");
    }else {
        ok(hres == S_OK, "get_external failed: %08x\n", hres);
        ok(external == NULL, "external != NULL\n");
    }

    IHTMLWindow2_Release(htmlwin);
}

static void test_StreamLoad(IHTMLDocument2 *doc)
{
    IPersistStreamInit *init;
    HRESULT hres;

    hres = IUnknown_QueryInterface(doc, &IID_IPersistStreamInit, (void**)&init);
    ok(hres == S_OK, "QueryInterface(IID_IPersistStreamInit) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    SET_EXPECT(Invoke_AMBIENT_SILENT);
    SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    SET_EXPECT(Exec_ShellDocView_37);
    SET_EXPECT(OnChanged_READYSTATE);
    SET_EXPECT(Read);
    readystate_set_loading = TRUE;

    hres = IPersistStreamInit_Load(init, &Stream);
    ok(hres == S_OK, "Load failed: %08x\n", hres);

    CHECK_CALLED(Invoke_AMBIENT_SILENT);
    CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    CHECK_CALLED(Exec_ShellDocView_37);
    CHECK_CALLED(OnChanged_READYSTATE);
    CHECK_CALLED(Read);

    test_timer(EXPECT_SETTITLE);
    test_GetCurMoniker((IUnknown*)doc, NULL, about_blank_url);

    IPersistStreamInit_Release(init);
}

static void test_StreamInitNew(IHTMLDocument2 *doc)
{
    IPersistStreamInit *init;
    HRESULT hres;

    hres = IUnknown_QueryInterface(doc, &IID_IPersistStreamInit, (void**)&init);
    ok(hres == S_OK, "QueryInterface(IID_IPersistStreamInit) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    SET_EXPECT(Invoke_AMBIENT_SILENT);
    SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    SET_EXPECT(Exec_ShellDocView_37);
    SET_EXPECT(OnChanged_READYSTATE);
    readystate_set_loading = TRUE;

    hres = IPersistStreamInit_InitNew(init);
    ok(hres == S_OK, "Load failed: %08x\n", hres);

    CHECK_CALLED(Invoke_AMBIENT_SILENT);
    CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    CHECK_CALLED(Exec_ShellDocView_37);
    CHECK_CALLED(OnChanged_READYSTATE);

    test_timer(EXPECT_SETTITLE);
    test_GetCurMoniker((IUnknown*)doc, NULL, about_blank_url);

    IPersistStreamInit_Release(init);
}

static void test_QueryInterface(IHTMLDocument2 *doc)
{
    IUnknown *qi;
    HRESULT hres;

    static const IID IID_UndocumentedScriptIface =
        {0x719c3050,0xf9d3,0x11cf,{0xa4,0x93,0x00,0x40,0x05,0x23,0xa8,0xa0}};

    qi = (void*)0xdeadbeef;
    hres = IUnknown_QueryInterface(doc, &IID_IRunnableObject, (void**)&qi);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(qi == NULL, "qirunnable=%p, expected NULL\n", qi);

    qi = (void*)0xdeadbeef;
    hres = IUnknown_QueryInterface(doc, &IID_IHTMLDOMNode, (void**)&qi);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(qi == NULL, "qi=%p, expected NULL\n", qi);

    qi = (void*)0xdeadbeef;
    hres = IUnknown_QueryInterface(doc, &IID_IHTMLDOMNode2, (void**)&qi);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(qi == NULL, "qi=%p, expected NULL\n", qi);

    qi = (void*)0xdeadbeef;
    hres = IUnknown_QueryInterface(doc, &IID_IPersistPropertyBag, (void**)&qi);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(qi == NULL, "qi=%p, expected NULL\n", qi);

    qi = (void*)0xdeadbeef;
    hres = IUnknown_QueryInterface(doc, &IID_UndocumentedScriptIface, (void**)&qi);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(qi == NULL, "qi=%p, expected NULL\n", qi);

    qi = (void*)0xdeadbeef;
    hres = IUnknown_QueryInterface(doc, &IID_IMarshal, (void**)&qi);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(qi == NULL, "qi=%p, expected NULL\n", qi);

    qi = (void*)0xdeadbeef;
    hres = IUnknown_QueryInterface(doc, &IID_IExternalConnection, (void**)&qi);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(qi == NULL, "qi=%p, expected NULL\n", qi);

    qi = (void*)0xdeadbeef;
    hres = IUnknown_QueryInterface(doc, &IID_IStdMarshalInfo, (void**)&qi);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(qi == NULL, "qi=%p, expected NULL\n", qi);

    hres = IUnknown_QueryInterface(doc, &IID_IDispatch, (void**)&qi);
    ok(hres == S_OK, "Could not get IDispatch interface: %08x\n", hres);
    ok(qi != (IUnknown*)doc, "disp == doc\n");
    IUnknown_Release(qi);
}

static void init_test(enum load_state_t ls) {
    doc_unk = NULL;
    hwnd = last_hwnd = NULL;
    set_clientsite = FALSE;
    load_from_stream = FALSE;
    call_UIActivate = CallUIActivate_None;
    load_state = ls;
    editmode = FALSE;
    stream_read = 0;
    protocol_read = 0;
    ipsex = FALSE;
    inplace_deactivated = FALSE;
    navigated_load = FALSE;
}

static void test_HTMLDocument(BOOL do_load)
{
    IHTMLDocument2 *doc;
    HRESULT hres;
    ULONG ref;

    trace("Testing HTMLDocument (%s)...\n", (do_load ? "load" : "no load"));

    init_test(do_load ? LD_DOLOAD : LD_NO);

    hres = create_document(&doc);
    if(FAILED(hres))
        return;
    doc_unk = (IUnknown*)doc;

    test_QueryInterface(doc);
    test_Advise(doc);
    test_IsDirty(doc, S_FALSE);
    test_MSHTML_QueryStatus(doc, OLECMDF_SUPPORTED);
    test_external(doc, FALSE);
    test_ViewAdviseSink(doc);
    test_ConnectionPointContainer(doc);
    test_GetCurMoniker((IUnknown*)doc, NULL, NULL);
    test_Persist(doc, &Moniker);
    if(!do_load)
        test_OnAmbientPropertyChange2(doc);

    test_Activate(doc, CLIENTSITE_EXPECTPATH);

    if(do_load) {
        test_download(DWL_CSS|DWL_TRYCSS);
        test_GetCurMoniker((IUnknown*)doc, &Moniker, NULL);
    }

    test_MSHTML_QueryStatus(doc, OLECMDF_SUPPORTED);
    test_OleCommandTarget_fail(doc);
    test_OleCommandTarget(doc);
    test_OnAmbientPropertyChange(doc);
    test_Window(doc, TRUE);
    test_external(doc, TRUE);

    test_UIDeactivate();
    test_OleCommandTarget(doc);
    test_Window(doc, TRUE);
    test_InPlaceDeactivate(doc, TRUE);

    /* Calling test_OleCommandTarget here causes Segmentation Fault with native
     * MSHTML. It doesn't with Wine. */

    test_Window(doc, FALSE);
    test_Hide();
    test_InPlaceDeactivate(doc, FALSE);
    test_CloseView();
    test_Close(doc, FALSE);

    /* Activate HTMLDocument again */
    test_Activate(doc, CLIENTSITE_SETNULL);
    test_Window(doc, TRUE);
    test_OleCommandTarget(doc);
    test_UIDeactivate();
    test_InPlaceDeactivate(doc, TRUE);
    test_Close(doc, FALSE);

    /* Activate HTMLDocument again, this time without UIActivate */
    call_UIActivate = CallUIActivate_None;
    test_Activate(doc, CLIENTSITE_SETNULL);
    test_Window(doc, TRUE);

    test_UIDeactivate();
    test_InPlaceDeactivate(doc, TRUE);
    test_CloseView();
    test_CloseView();
    test_Close(doc, TRUE);
    test_OnAmbientPropertyChange2(doc);
    test_GetCurMoniker((IUnknown*)doc, do_load ? &Moniker : NULL, NULL);

    if(!do_load) {
        /* Activate HTMLDocument again, calling UIActivate after showing the window */
        call_UIActivate = CallUIActivate_AfterShow;
        test_Activate(doc, 0);
        test_Window(doc, TRUE);
        test_OleCommandTarget(doc);
        test_UIDeactivate();
        test_InPlaceDeactivate(doc, TRUE);
        test_Close(doc, FALSE);
        call_UIActivate = CallUIActivate_None;
    }

    if(view)
        IOleDocumentView_Release(view);
    view = NULL;

    ok(IsWindow(hwnd), "hwnd is destroyed\n");

    ref = IHTMLDocument2_Release(doc);
    ok(ref == 0, "ref=%d, expected 0\n", ref);

    ok(!IsWindow(hwnd), "hwnd is not destroyed\n");
}

static void test_HTMLDocument_hlink(void)
{
    IHTMLDocument2 *doc;
    HRESULT hres;
    ULONG ref;

    trace("Testing HTMLDocument (hlink)...\n");

    init_test(LD_DOLOAD);
    ipsex = TRUE;

    hres = create_document(&doc);
    if(FAILED(hres))
        return;
    doc_unk = (IUnknown*)doc;

    test_ViewAdviseSink(doc);
    test_ConnectionPointContainer(doc);
    test_GetCurMoniker((IUnknown*)doc, NULL, NULL);
    test_Persist(doc, &Moniker);
    test_Navigate(doc);
    if(show_failed) {
        IUnknown_Release(doc);
        return;
    }

    test_download(DWL_CSS|DWL_TRYCSS);

    test_IsDirty(doc, S_FALSE);
    test_MSHTML_QueryStatus(doc, OLECMDF_SUPPORTED);

    test_exec_onunload(doc);
    test_Window(doc, TRUE);
    test_InPlaceDeactivate(doc, TRUE);
    test_Close(doc, FALSE);
    test_IsDirty(doc, S_FALSE);
    test_GetCurMoniker((IUnknown*)doc, &Moniker, NULL);
    test_clear(doc);
    test_GetCurMoniker((IUnknown*)doc, &Moniker, NULL);

    if(view)
        IOleDocumentView_Release(view);
    view = NULL;

    ref = IHTMLDocument2_Release(doc);
    ok(ref == 0, "ref=%d, expected 0\n", ref);
}

static void test_cookies(IHTMLDocument2 *doc)
{
    WCHAR buf[1024];
    DWORD size;
    BSTR str, str2;
    BOOL b;
    HRESULT hres;

    hres = IHTMLDocument2_get_cookie(doc, &str);
    ok(hres == S_OK, "get_cookie failed: %08x\n", hres);
    if(str) {
        size = sizeof(buf)/sizeof(WCHAR);
        b = InternetGetCookieW(http_urlW, NULL, buf, &size);
        ok(b, "InternetGetCookieW failed: %08x\n", GetLastError());
        ok(!lstrcmpW(buf, str), "cookie = %s, expected %s\n", wine_dbgstr_w(str), wine_dbgstr_w(buf));
        SysFreeString(str);
    }

    str = a2bstr("test=testval");
    hres = IHTMLDocument2_put_cookie(doc, str);
    ok(hres == S_OK, "put_cookie failed: %08x\n", hres);

    str2 = NULL;
    hres = IHTMLDocument2_get_cookie(doc, &str2);
    ok(hres == S_OK, "get_cookie failed: %08x\n", hres);
    ok(str2 != NULL, "cookie = NULL\n");
    size = sizeof(buf)/sizeof(WCHAR);
    b = InternetGetCookieW(http_urlW, NULL, buf, &size);
    ok(b, "InternetGetCookieW failed: %08x\n", GetLastError());
    ok(!lstrcmpW(buf, str2), "cookie = %s, expected %s\n", wine_dbgstr_w(str2), wine_dbgstr_w(buf));
    ok(strstrW(str2, str) != NULL, "could not find %s in %s\n", wine_dbgstr_w(str), wine_dbgstr_w(str2));
    SysFreeString(str);
    SysFreeString(str2);

    str = a2bstr("test=testval2");
    hres = IHTMLDocument2_put_cookie(doc, str);
    ok(hres == S_OK, "put_cookie failed: %08x\n", hres);

    str2 = NULL;
    hres = IHTMLDocument2_get_cookie(doc, &str2);
    ok(hres == S_OK, "get_cookie failed: %08x\n", hres);
    ok(str2 != NULL, "cookie = NULL\n");
    size = sizeof(buf)/sizeof(WCHAR);
    b = InternetGetCookieW(http_urlW, NULL, buf, &size);
    ok(b, "InternetGetCookieW failed: %08x\n", GetLastError());
    ok(!lstrcmpW(buf, str2), "cookie = %s, expected %s\n", wine_dbgstr_w(str2), wine_dbgstr_w(buf));
    ok(strstrW(str2, str) != NULL, "could not find %s in %s\n", wine_dbgstr_w(str), wine_dbgstr_w(str2));
    SysFreeString(str);
    SysFreeString(str2);
}

static void test_HTMLDocument_http(void)
{
    IMoniker *http_mon;
    IHTMLDocument2 *doc;
    ULONG ref;
    HRESULT hres;

    trace("Testing HTMLDocument (http)...\n");

    if(!winetest_interactive && is_ie_hardened()) {
        win_skip("IE running in Enhanced Security Configuration\n");
        return;
    }

    init_test(LD_DOLOAD);
    ipsex = TRUE;

    hres = create_document(&doc);
    if(FAILED(hres))
        return;
    doc_unk = (IUnknown*)doc;

    hres = CreateURLMoniker(NULL, http_urlW, &http_mon);
    ok(hres == S_OK, "CreateURLMoniker failed: %08x\n", hres);

    test_ViewAdviseSink(doc);
    test_ConnectionPointContainer(doc);
    test_GetCurMoniker((IUnknown*)doc, NULL, NULL);
    test_Persist(doc, http_mon);
    test_Navigate(doc);
    if(show_failed) {
        IUnknown_Release(doc);
        return;
    }

    test_download(DWL_HTTP);
    test_cookies(doc);
    test_IsDirty(doc, S_FALSE);
    test_MSHTML_QueryStatus(doc, OLECMDF_SUPPORTED);
    test_GetCurMoniker((IUnknown*)doc, http_mon, NULL);

    test_put_href(doc);
    test_open_window(doc);

    test_InPlaceDeactivate(doc, TRUE);
    test_Close(doc, FALSE);
    test_IsDirty(doc, S_FALSE);
    test_GetCurMoniker((IUnknown*)doc, NULL, about_blank_url);

    if(view)
        IOleDocumentView_Release(view);
    view = NULL;

    ref = IHTMLDocument2_Release(doc);
    ok(!ref, "ref=%d, expected 0\n", ref);

    ref = IMoniker_Release(http_mon);
    ok(!ref, "ref=%d, expected 0\n", ref);
}

static void test_QueryService(IHTMLDocument2 *doc, BOOL success)
{
    IServiceProvider *sp;
    IHlinkFrame *hf;
    HRESULT hres;

    hres = IUnknown_QueryInterface(doc, &IID_IServiceProvider, (void**)&sp);
    ok(hres == S_OK, "QueryService returned %08x\n", hres);

    hres = IServiceProvider_QueryService(sp, &IID_IHlinkFrame, &IID_IHlinkFrame, (void**)&hf);
    if(SUCCEEDED(hres))
        IHlinkFrame_Release(hf);

    ok(hres == (success?S_OK:E_NOINTERFACE), "QueryService returned %08x, expected %08x\n", hres, success?S_OK:E_NOINTERFACE);

    IServiceProvider_Release(sp);
}

static void test_HTMLDocument_StreamLoad(void)
{
    IHTMLDocument2 *doc;
    IOleObject *oleobj;
    DWORD conn;
    HRESULT hres;
    ULONG ref;

    trace("Testing HTMLDocument (IPersistStreamInit)...\n");

    init_test(LD_DOLOAD);
    load_from_stream = TRUE;

    hres = create_document(&doc);
    if(FAILED(hres))
        return;
    doc_unk = (IUnknown*)doc;

    hres = IUnknown_QueryInterface(doc, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "Could not get IOleObject: %08x\n", hres);

    hres = IOleObject_Advise(oleobj, (IAdviseSink*)&AdviseSink, &conn);
    ok(hres == S_OK, "Advise failed: %08x\n", hres);

    test_readyState((IUnknown*)doc);
    test_IsDirty(doc, S_FALSE);
    test_ViewAdviseSink(doc);
    test_ConnectionPointContainer(doc);
    test_QueryService(doc, FALSE);
    test_ClientSite(oleobj, CLIENTSITE_EXPECTPATH);
    test_QueryService(doc, TRUE);
    test_DoVerb(oleobj);
    test_MSHTML_QueryStatus(doc, OLECMDF_SUPPORTED);

    IOleObject_Release(oleobj);

    test_GetCurMoniker((IUnknown*)doc, NULL, NULL);
    test_StreamLoad(doc);
    test_download(DWL_VERBDONE|DWL_TRYCSS);
    test_MSHTML_QueryStatus(doc, OLECMDF_SUPPORTED);

    test_UIDeactivate();
    test_InPlaceDeactivate(doc, TRUE);
    SET_EXPECT(Advise_Close);
    test_Close(doc, FALSE);
    CHECK_CALLED(Advise_Close);
    test_IsDirty(doc, S_FALSE);

    if(view) {
        IOleDocumentView_Release(view);
        view = NULL;
    }


    ref = IHTMLDocument2_Release(doc);
    ok(ref == 0, "ref=%d, expected 0\n", ref);
}

static void test_HTMLDocument_StreamInitNew(void)
{
    IHTMLDocument2 *doc;
    IOleObject *oleobj;
    DWORD conn;
    HRESULT hres;
    ULONG ref;

    trace("Testing HTMLDocument (IPersistStreamInit)...\n");

    init_test(LD_DOLOAD);
    load_from_stream = TRUE;

    hres = create_document(&doc);
    if(FAILED(hres))
        return;
    doc_unk = (IUnknown*)doc;

    hres = IUnknown_QueryInterface(doc, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "Could not get IOleObject: %08x\n", hres);

    hres = IOleObject_Advise(oleobj, (IAdviseSink*)&AdviseSink, &conn);
    ok(hres == S_OK, "Advise failed: %08x\n", hres);

    test_readyState((IUnknown*)doc);
    test_IsDirty(doc, S_FALSE);
    test_ViewAdviseSink(doc);
    test_ConnectionPointContainer(doc);
    test_ClientSite(oleobj, CLIENTSITE_EXPECTPATH);
    test_DoVerb(oleobj);
    test_MSHTML_QueryStatus(doc, OLECMDF_SUPPORTED);

    IOleObject_Release(oleobj);

    test_GetCurMoniker((IUnknown*)doc, NULL, NULL);
    test_StreamInitNew(doc);
    test_download(DWL_VERBDONE|DWL_TRYCSS|DWL_EMPTY);
    test_MSHTML_QueryStatus(doc, OLECMDF_SUPPORTED);

    test_UIDeactivate();
    test_InPlaceDeactivate(doc, TRUE);
    SET_EXPECT(Advise_Close);
    test_Close(doc, FALSE);
    CHECK_CALLED(Advise_Close);
    test_IsDirty(doc, S_FALSE);

    if(view) {
        IOleDocumentView_Release(view);
        view = NULL;
    }


    ref = IHTMLDocument2_Release(doc);
    ok(ref == 0, "ref=%d, expected 0\n", ref);
}

static void test_edit_uiactivate(IOleObject *oleobj)
{
    IOleDocumentView *docview;
    HRESULT hres;

    hres = IOleObject_QueryInterface(oleobj, &IID_IOleDocumentView, (void **)&docview);
    ok(hres == S_OK, "IOleObject_QueryInterface failed with error 0x%08x\n", hres);

    SET_EXPECT(OnFocus_TRUE);
    SET_EXPECT(SetActiveObject);
    SET_EXPECT(ShowUI);
    SET_EXPECT(InPlaceUIWindow_SetActiveObject);
    SET_EXPECT(InPlaceFrame_SetBorderSpace);
    expect_status_text = NULL;

    hres = IOleDocumentView_UIActivate(docview, TRUE);
    ok(hres == S_OK, "IOleDocumentView_UIActivate failed with error 0x%08x\n", hres);

    CHECK_CALLED(OnFocus_TRUE);
    CHECK_CALLED(SetActiveObject);
    CHECK_CALLED(ShowUI);
    CHECK_CALLED(InPlaceUIWindow_SetActiveObject);
    CHECK_CALLED(InPlaceFrame_SetBorderSpace);

    IOleDocumentView_Release(docview);
}

static void test_editing_mode(BOOL do_load)
{
    IHTMLDocument2 *doc;
    IUnknown *unk;
    IOleObject *oleobj;
    DWORD conn;
    HRESULT hres;
    ULONG ref;

    trace("Testing HTMLDocument (edit%s)...\n", do_load ? " load" : "");

    init_test(do_load ? LD_DOLOAD : LD_NO);
    call_UIActivate = CallUIActivate_AfterShow;

    hres = create_document(&doc);
    if(FAILED(hres))
        return;
    unk = doc_unk = (IUnknown*)doc;

    hres = IUnknown_QueryInterface(doc, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "Could not get IOleObject: %08x\n", hres);

    hres = IOleObject_Advise(oleobj, (IAdviseSink*)&AdviseSink, &conn);
    ok(hres == S_OK, "Advise failed: %08x\n", hres);

    test_readyState((IUnknown*)doc);
    test_ViewAdviseSink(doc);
    test_ConnectionPointContainer(doc);
    test_ClientSite(oleobj, CLIENTSITE_EXPECTPATH);
    test_DoVerb(oleobj);
    test_edit_uiactivate(oleobj);

    test_MSHTML_QueryStatus(doc, OLECMDF_SUPPORTED);
    if(do_load)
        test_Persist(doc, &Moniker);
    stream_read = protocol_read = 0;
    test_exec_editmode(unk, do_load);
    test_UIDeactivate();
    call_UIActivate = CallUIActivate_None;
    IOleObject_Release(oleobj);

    test_MSHTML_QueryStatus(doc, OLECMDF_SUPPORTED);
    test_download(DWL_VERBDONE | (do_load ? DWL_CSS|DWL_TRYCSS : 0));

    SET_EXPECT(SetStatusText); /* ignore race in native mshtml */
    test_timer(EXPECT_UPDATEUI);
    SET_CALLED(SetStatusText);

    test_MSHTML_QueryStatus(doc, OLECMDF_SUPPORTED|OLECMDF_ENABLED);

    if(!do_load) {
        test_exec_fontname(unk, NULL, wszTimesNewRoman);
        test_exec_fontname(unk, wszArial, wszTimesNewRoman);
        test_timer(EXPECT_UPDATEUI);
        test_exec_fontname(unk, NULL, wszArial);

        test_exec_noargs(unk, IDM_JUSTIFYRIGHT);
        test_timer(EXPECT_UPDATEUI);
        test_QueryStatus(unk, &CGID_MSHTML, IDM_JUSTIFYRIGHT,
                         OLECMDF_SUPPORTED|OLECMDF_ENABLED|OLECMDF_LATCHED);

        test_exec_noargs(unk, IDM_JUSTIFYCENTER);
        test_timer(EXPECT_UPDATEUI);
        test_QueryStatus(unk, &CGID_MSHTML, IDM_JUSTIFYRIGHT,
                         OLECMDF_SUPPORTED|OLECMDF_ENABLED);
        test_QueryStatus(unk, &CGID_MSHTML, IDM_JUSTIFYCENTER,
                         OLECMDF_SUPPORTED|OLECMDF_ENABLED|OLECMDF_LATCHED);

        test_exec_noargs(unk, IDM_HORIZONTALLINE);
        test_timer(EXPECT_UPDATEUI);
        test_QueryStatus(unk, &CGID_MSHTML, IDM_HORIZONTALLINE,
                         OLECMDF_SUPPORTED|OLECMDF_ENABLED);
    }

    test_UIDeactivate();
    test_InPlaceDeactivate(doc, TRUE);
    SET_EXPECT(Advise_Close);
    test_Close(doc, FALSE);
    CHECK_CALLED(Advise_Close);

    if(view) {
        IOleDocumentView_Release(view);
        view = NULL;
    }

    ref = IUnknown_Release(unk);
    ok(ref == 0, "ref=%d, expected 0\n", ref);
}

static void test_UIActivate(BOOL do_load, BOOL use_ipsex, BOOL use_ipsw)
{
    IHTMLDocument2 *doc;
    IOleObject *oleobj;
    IOleInPlaceSite *inplacesite;
    HRESULT hres;
    ULONG ref;

    trace("Running OleDocumentView_UIActivate tests (%d %d %d)\n", do_load, use_ipsex, use_ipsw);

    init_test(do_load ? LD_DOLOAD : LD_NO);

    hres = create_document(&doc);
    if(FAILED(hres))
        return;
    doc_unk = (IUnknown*)doc;

    ipsex = use_ipsex;
    ipsw = use_ipsw;

    hres = IUnknown_QueryInterface(doc, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_IOleObject) failed: %08x\n", hres);

    hres = IUnknown_QueryInterface(doc, &IID_IOleDocumentView, (void**)&view);
    ok(hres == S_OK, "QueryInterface(IID_IOleDocumentView) failed: %08x\n", hres);

    SET_EXPECT(Invoke_AMBIENT_USERMODE);
    SET_EXPECT(GetHostInfo);
    SET_EXPECT(Invoke_AMBIENT_DLCONTROL);
    SET_EXPECT(Invoke_AMBIENT_SILENT);
    SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    SET_EXPECT(Invoke_AMBIENT_USERAGENT);
    SET_EXPECT(Invoke_AMBIENT_PALETTE);
    SET_EXPECT(GetOptionKeyPath);
    SET_EXPECT(GetOverrideKeyPath);
    SET_EXPECT(GetWindow);
    SET_EXPECT(Exec_DOCCANNAVIGATE);
    SET_EXPECT(QueryStatus_SETPROGRESSTEXT);
    SET_EXPECT(Exec_SETPROGRESSMAX);
    SET_EXPECT(Exec_SETPROGRESSPOS);

    hres = IOleObject_SetClientSite(oleobj, &ClientSite);
    ok(hres == S_OK, "SetClientSite failed: %08x\n", hres);

    CHECK_CALLED(Invoke_AMBIENT_USERMODE);
    CHECK_CALLED(GetHostInfo);
    CHECK_CALLED(Invoke_AMBIENT_DLCONTROL);
    CHECK_CALLED(Invoke_AMBIENT_SILENT);
    CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    CHECK_CALLED(Invoke_AMBIENT_USERAGENT);
    CHECK_CALLED(Invoke_AMBIENT_PALETTE);
    CHECK_CALLED(GetOptionKeyPath);
    CHECK_CALLED(GetOverrideKeyPath);
    CHECK_CALLED(GetWindow);
    CHECK_CALLED(Exec_DOCCANNAVIGATE);
    CHECK_CALLED(QueryStatus_SETPROGRESSTEXT);
    CHECK_CALLED(Exec_SETPROGRESSMAX);
    CHECK_CALLED(Exec_SETPROGRESSPOS);

    hres = IOleDocumentView_GetInPlaceSite(view, &inplacesite);
    ok(hres == S_OK, "GetInPlaceSite failed: %08x\n", hres);
    ok(inplacesite == NULL, "inplacesite = %p, expected NULL\n", inplacesite);

    SET_EXPECT(GetContainer);
    SET_EXPECT(LockContainer);
    SET_EXPECT(CanInPlaceActivate);
    SET_EXPECT(GetWindowContext);
    SET_EXPECT(GetWindow);
    if(use_ipsex) {
        SET_EXPECT(OnInPlaceActivateEx);
        SET_EXPECT(RequestUIActivate);
    }
    else
        SET_EXPECT(OnInPlaceActivate);
    SET_EXPECT(OnUIActivate);
    SET_EXPECT(SetStatusText);
    SET_EXPECT(Exec_SETPROGRESSMAX);
    SET_EXPECT(Exec_SETPROGRESSPOS);
    SET_EXPECT(ShowUI);
    SET_EXPECT(InPlaceUIWindow_SetActiveObject);
    SET_EXPECT(InPlaceFrame_SetBorderSpace);
    SET_EXPECT(OnFocus_TRUE);
    SET_EXPECT(SetActiveObject);
    expect_LockContainer_fLock = TRUE;

    hres = IOleDocumentView_UIActivate(view, TRUE);
    ok(hres == S_OK, "UIActivate failed: %08x\n", hres);

    CHECK_CALLED(GetContainer);
    CHECK_CALLED(LockContainer);
    CHECK_CALLED(CanInPlaceActivate);
    CHECK_CALLED(GetWindowContext);
    CHECK_CALLED(GetWindow);
    if(use_ipsex) {
        CHECK_CALLED(OnInPlaceActivateEx);
        SET_EXPECT(RequestUIActivate);
    }
    else
        CHECK_CALLED(OnInPlaceActivate);
    CHECK_CALLED(OnUIActivate);
    CHECK_CALLED(SetStatusText);
    CHECK_CALLED(Exec_SETPROGRESSMAX);
    CHECK_CALLED(Exec_SETPROGRESSPOS);
    CHECK_CALLED(ShowUI);
    CHECK_CALLED(InPlaceUIWindow_SetActiveObject);
    CHECK_CALLED(InPlaceFrame_SetBorderSpace);
    CHECK_CALLED(OnFocus_TRUE);
    CHECK_CALLED(SetActiveObject);
    container_locked = TRUE;

    SET_EXPECT(SetActiveObject_null);
    SET_EXPECT(InPlaceUIWindow_SetActiveObject);
    SET_EXPECT(HideUI);
    SET_EXPECT(OnUIDeactivate);

    hres = IOleDocumentView_UIActivate(view, FALSE);
    ok(hres == S_OK, "UIActivate failed: %08x\n", hres);

    CHECK_CALLED(SetActiveObject_null);
    CHECK_CALLED(InPlaceUIWindow_SetActiveObject);
    CHECK_CALLED(HideUI);
    CHECK_CALLED(OnUIDeactivate);

    hres = IOleDocumentView_GetInPlaceSite(view, &inplacesite);
    ok(hres == S_OK, "GetInPlaceSite failed: %08x\n", hres);
    ok(inplacesite != NULL, "inplacesite = NULL\n");
    IOleInPlaceSite_Release(inplacesite);

    SET_EXPECT(OnFocus_FALSE);
    if(use_ipsex)
        SET_EXPECT(OnInPlaceDeactivateEx);
    else
        SET_EXPECT(OnInPlaceDeactivate);

    test_CloseView();

    CHECK_CALLED(OnFocus_FALSE);
    if(use_ipsex)
        CHECK_CALLED(OnInPlaceDeactivateEx);
    else
        CHECK_CALLED(OnInPlaceDeactivate);

    test_Close(doc, TRUE);

    IOleObject_Release(oleobj);
    IOleDocumentView_Release(view);
    view = NULL;

    ref = IHTMLDocument2_Release(doc);
    ok(ref == 0, "ref=%d, expected 0\n", ref);
}

static void register_protocol(void)
{
    IInternetSession *session;
    HRESULT hres;

    static const WCHAR wsz_winetest[] = {'w','i','n','e','t','e','s','t',0};

    hres = CoInternetGetSession(0, &session, 0);
    ok(hres == S_OK, "CoInternetGetSession failed: %08x\n", hres);

    hres = IInternetSession_RegisterNameSpace(session, &ClassFactory, &IID_NULL,
            wsz_winetest, 0, NULL, 0);
    ok(hres == S_OK, "RegisterNameSpace failed: %08x\n", hres);

    IInternetSession_Release(session);
}

static void test_HTMLDoc_ISupportErrorInfo(void)
{
    IHTMLDocument2 *doc;
    HRESULT hres;
    ISupportErrorInfo *sinfo;
    LONG ref;

    hres = create_document(&doc);
    if(FAILED(hres))
        return;

    hres = IUnknown_QueryInterface(doc, &IID_ISupportErrorInfo, (void**)&sinfo);
    ok(hres == S_OK, "got %x\n", hres);
    ok(sinfo != NULL, "got %p\n", sinfo);
    if(sinfo)
    {
        hres = ISupportErrorInfo_InterfaceSupportsErrorInfo(sinfo, &IID_IErrorInfo);
        ok(hres == S_FALSE, "Expected S_OK, got %x\n", hres);
        IUnknown_Release(sinfo);
    }

    ref = IHTMLDocument2_Release(doc);
    ok(ref == 0, "ref=%d, expected 0\n", ref);
}

static void test_IPersistHistory(void)
{
    IHTMLDocument2 *doc;
    HRESULT hres;
    LONG ref;
    IPersistHistory *phist;

    hres = create_document(&doc);
    if(FAILED(hres))
        return;

    hres = IUnknown_QueryInterface(doc, &IID_IPersistHistory, (void**)&phist);
    ok(hres == S_OK, "QueryInterface returned %08x, expected S_OK\n", hres);
    if(hres == S_OK)
        IPersistHistory_Release(phist);

    ref = IHTMLDocument2_Release(doc);
    ok(ref == 0, "ref=%d, expected 0\n", ref);
}

START_TEST(htmldoc)
{
    CoInitialize(NULL);
    container_hwnd = create_container_window();
    register_protocol();

    test_HTMLDocument_hlink();
    if(!show_failed) {
        test_HTMLDocument(FALSE);
        test_HTMLDocument(TRUE);
        test_HTMLDocument_StreamLoad();
        test_HTMLDocument_StreamInitNew();
        test_editing_mode(FALSE);
        test_editing_mode(TRUE);
        test_HTMLDocument_http();
        test_UIActivate(FALSE, FALSE, FALSE);
        test_UIActivate(FALSE, TRUE, FALSE);
        test_UIActivate(FALSE, TRUE, TRUE);
        test_UIActivate(TRUE, FALSE, FALSE);
        test_UIActivate(TRUE, TRUE, FALSE);
        test_UIActivate(TRUE, TRUE, TRUE);
    }
    test_HTMLDoc_ISupportErrorInfo();
    test_IPersistHistory();

    DestroyWindow(container_hwnd);
    CoUninitialize();
}
