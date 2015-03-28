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
//#include <stdarg.h>
#include <stdio.h>

//#include "windef.h"
//#include "winbase.h"
#include <initguid.h>
//#include "ole2.h"
#include <mshtml.h>
//#include "docobj.h"
#include <docobjectservice.h>
#include <wininet.h>
#include <mshtmhst.h>
#include <mshtmdid.h>
#include <mshtmcid.h>
//#include "hlink.h"
//#include "dispex.h"
#include <idispids.h>
#include <shlguid.h>
#include <shdeprecated.h>
#include <perhist.h>
//#include "shobjidl.h"
#include <htiface.h>
#include <tlogstg.h>
#include <exdispid.h>
#include "mshtml_test.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
DEFINE_GUID(IID_IProxyManager,0x00000008,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
DEFINE_OLEGUID(CGID_DocHostCmdPriv, 0x000214D4L, 0, 0);
DEFINE_GUID(SID_SContainerDispatch,0xb722be00,0x4e68,0x101b,0xa2,0xbc,0x00,0xaa,0x00,0x40,0x47,0x70);

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

#define CHECK_NOT_CALLED(func) \
    do { \
        ok(!called_ ## func, "unexpected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED_BROKEN(func) \
    do { \
        ok(called_ ## func || broken(!called_ ## func), "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CLEAR_CALLED(func) \
    expect_ ## func = called_ ## func = FALSE


static IOleDocumentView *view = NULL;
static HWND container_hwnd = NULL, doc_hwnd = NULL, last_hwnd = NULL;

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
DEFINE_EXPECT(Exec_ShellDocView_62);
DEFINE_EXPECT(Exec_ShellDocView_63);
DEFINE_EXPECT(Exec_ShellDocView_67);
DEFINE_EXPECT(Exec_ShellDocView_84);
DEFINE_EXPECT(Exec_ShellDocView_103);
DEFINE_EXPECT(Exec_ShellDocView_105);
DEFINE_EXPECT(Exec_ShellDocView_138);
DEFINE_EXPECT(Exec_ShellDocView_140);
DEFINE_EXPECT(Exec_DocHostCommandHandler_2300);
DEFINE_EXPECT(Exec_UPDATECOMMANDS);
DEFINE_EXPECT(Exec_SETTITLE);
DEFINE_EXPECT(Exec_HTTPEQUIV);
DEFINE_EXPECT(Exec_MSHTML_PARSECOMPLETE);
DEFINE_EXPECT(Exec_Explorer_38);
DEFINE_EXPECT(Exec_Explorer_69);
DEFINE_EXPECT(Exec_DOCCANNAVIGATE);
DEFINE_EXPECT(Exec_DOCCANNAVIGATE_NULL);
DEFINE_EXPECT(Invoke_AMBIENT_USERMODE);
DEFINE_EXPECT(Invoke_AMBIENT_DLCONTROL);
DEFINE_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
DEFINE_EXPECT(Invoke_AMBIENT_SILENT);
DEFINE_EXPECT(Invoke_AMBIENT_USERAGENT);
DEFINE_EXPECT(Invoke_AMBIENT_PALETTE);
DEFINE_EXPECT(Invoke_OnReadyStateChange_Interactive);
DEFINE_EXPECT(Invoke_OnReadyStateChange_Loading);
DEFINE_EXPECT(Invoke_OnReadyStateChange_Complete);
DEFINE_EXPECT(GetDropTarget);
DEFINE_EXPECT(UpdateUI);
DEFINE_EXPECT(Navigate);
DEFINE_EXPECT(OnFrameWindowActivate);
DEFINE_EXPECT(OnChanged_READYSTATE);
DEFINE_EXPECT(OnChanged_1005);
DEFINE_EXPECT(OnChanged_1012);
DEFINE_EXPECT(OnChanged_1014);
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
DEFINE_EXPECT(GetTravelLog);
DEFINE_EXPECT(UpdateBackForwardState);
DEFINE_EXPECT(FireBeforeNavigate2);
DEFINE_EXPECT(FireNavigateComplete2);
DEFINE_EXPECT(FireDocumentComplete);
DEFINE_EXPECT(GetPendingUrl);
DEFINE_EXPECT(ActiveElementChanged);
DEFINE_EXPECT(IsErrorUrl);
DEFINE_EXPECT(get_LocationURL);
DEFINE_EXPECT(CountEntries);
DEFINE_EXPECT(FindConnectionPoint);
DEFINE_EXPECT(EnumConnections);
DEFINE_EXPECT(EnumConnections_Next);
DEFINE_EXPECT(WindowClosing);
DEFINE_EXPECT(NavigateWithBindCtx);

static BOOL is_ie9plus;
static IUnknown *doc_unk;
static IMoniker *doc_mon;
static BOOL expect_LockContainer_fLock;
static BOOL expect_InPlaceUIWindow_SetActiveObject_active = TRUE;
static BOOL ipsex, ipsw;
static BOOL set_clientsite, container_locked;
static BOOL readystate_set_loading = FALSE, readystate_set_interactive = FALSE, load_from_stream;
static BOOL editmode = FALSE, ignore_external_qi;
static BOOL inplace_deactivated, open_call;
static BOOL complete, loading_js, loading_hash, is_refresh;
static DWORD status_code = HTTP_STATUS_OK;
static BOOL asynchronous_binding = FALSE;
static BOOL support_wbapp, allow_new_window, no_travellog;
static BOOL report_mime;
static BOOL testing_submit;
static BOOL resetting_document;
static int stream_read, protocol_read;
static IStream *history_stream;
static enum load_state_t {
    LD_DOLOAD,
    LD_LOADING,
    LD_LOADED,
    LD_INTERACTIVE,
    LD_COMPLETE,
    LD_NO
} load_state;

static LPCOLESTR expect_status_text = NULL;
static const char *nav_url, *nav_serv_url, *prev_url;

static const char html_page[] =
"<html>"
"<head><link rel=\"stylesheet\" type=\"text/css\" href=\"test.css\"></head>"
"<body><div>test</div></body>"
"</html>";

static const char css_data[] = "body {color: red; margin: 0}";

static const WCHAR http_urlW[] =
    {'h','t','t','p',':','/','/','t','e','s','t','.','w','i','n','e','h','q','.','o','r','g','/','t','e','s','t','s','/','w','i','n','e','h','q','_','s','n','a','p','s','h','o','t','/',0};

static const WCHAR doc_url[] = {'w','i','n','e','t','e','s','t',':','d','o','c',0};

#define DOCHOST_DOCCANNAVIGATE 0
#define WM_CONTINUE_BINDING (WM_APP+1)

static HRESULT QueryInterface(REFIID riid, void **ppv);
static void test_MSHTML_QueryStatus(IHTMLDocument2*,DWORD);

#define test_readyState(u) _test_readyState(__LINE__,u)
static void _test_readyState(unsigned,IUnknown*);

static const WCHAR wszTimesNewRoman[] =
    {'T','i','m','e','s',' ','N','e','w',' ','R','o','m','a','n',0};
static const WCHAR wszArial[] =
    {'A','r','i','a','l',0};

static int strcmp_wa(LPCWSTR strw, const char *stra)
{
    CHAR buf[512];
    WideCharToMultiByte(CP_ACP, 0, strw, -1, buf, sizeof(buf), NULL, NULL);
    return lstrcmpA(stra, buf);
}

static BOOL wstr_contains(const WCHAR *strw, const char *stra)
{
    CHAR buf[512];
    WideCharToMultiByte(CP_ACP, 0, strw, -1, buf, sizeof(buf), NULL, NULL);
    return strstr(buf, stra) != NULL;
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

/* Returns true if the user interface is in English. Note that this does not
 * presume of the formatting of dates, numbers, etc.
 */
static BOOL is_lang_english(void)
{
    static HMODULE hkernel32 = NULL;
    static LANGID (WINAPI *pGetThreadUILanguage)(void) = NULL;
    static LANGID (WINAPI *pGetUserDefaultUILanguage)(void) = NULL;

    if (!hkernel32)
    {
        hkernel32 = GetModuleHandleA("kernel32.dll");
        pGetThreadUILanguage = (void*)GetProcAddress(hkernel32, "GetThreadUILanguage");
        pGetUserDefaultUILanguage = (void*)GetProcAddress(hkernel32, "GetUserDefaultUILanguage");
    }
    if (pGetThreadUILanguage)
        return PRIMARYLANGID(pGetThreadUILanguage()) == LANG_ENGLISH;
    if (pGetUserDefaultUILanguage)
        return PRIMARYLANGID(pGetUserDefaultUILanguage()) == LANG_ENGLISH;

    return PRIMARYLANGID(GetUserDefaultLangID()) == LANG_ENGLISH;
}

static BOOL iface_cmp(IUnknown *iface1, IUnknown *iface2)
{
    IUnknown *unk1, *unk2;

    if(iface1 == iface2)
        return TRUE;

    IUnknown_QueryInterface(iface1, &IID_IUnknown, (void**)&unk1);
    IUnknown_Release(unk1);
    IUnknown_QueryInterface(iface2, &IID_IUnknown, (void**)&unk2);
    IUnknown_Release(unk2);

    return unk1 == unk2;
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

    while(!*b && GetMessageA(&msg, doc_hwnd, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    if(flags & EXPECT_UPDATEUI) {
        CHECK_CALLED(UpdateUI);
        CHECK_CALLED(Exec_UPDATECOMMANDS);
    }
    if(flags & EXPECT_SETTITLE)
        CHECK_CALLED(Exec_SETTITLE);
}

static IMoniker Moniker;

#define test_GetCurMoniker(u,m,v,t) _test_GetCurMoniker(__LINE__,u,m,v,t)
static void _test_GetCurMoniker(unsigned line, IUnknown *unk, IMoniker *exmon, const char *exurl, BOOL is_todo)
{
    IHTMLDocument2 *doc;
    IPersistMoniker *permon;
    IMoniker *mon = (void*)0xdeadbeef;
    BSTR doc_url = (void*)0xdeadbeef;
    const WCHAR *ptr;
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
    for(ptr = doc_url; *ptr && *ptr != '#'; ptr++);

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

        if(!*ptr)
            ok(!lstrcmpW(url, doc_url), "url %s != doc_url %s\n", wine_dbgstr_w(url), wine_dbgstr_w(doc_url));
        else
            ok(!strcmp_wa(url, nav_serv_url), "url = %s, expected %s\n", wine_dbgstr_w(url), nav_serv_url);
        CoTaskMemFree(url);
    }else if(exurl) {
        LPOLESTR url;

        ok_(__FILE__,line)(hres == S_OK, "GetCurrentMoniker failed: %08x\n", hres);

        hres = IMoniker_GetDisplayName(mon, NULL, NULL, &url);
        ok(hres == S_OK, "GetDisplayName failed: %08x\n", hres);

        if(is_todo)
            todo_wine ok_(__FILE__,line)(!strcmp_wa(url, exurl), "unexpected url %s\n", wine_dbgstr_w(url));
        else
            ok_(__FILE__,line)(!strcmp_wa(url, exurl), "unexpected url %s\n", wine_dbgstr_w(url));
        if(!*ptr)
            ok_(__FILE__,line)(!lstrcmpW(url, doc_url), "url %s != doc_url %s\n", wine_dbgstr_w(url), wine_dbgstr_w(doc_url));

        CoTaskMemFree(url);
    }else {
        ok_(__FILE__,line)(hres == E_UNEXPECTED,
           "GetCurrentMoniker failed: %08x, expected E_UNEXPECTED\n", hres);
        ok_(__FILE__,line)(mon == (IMoniker*)0xdeadbeef, "mon=%p\n", mon);
        ok_(__FILE__,line)(!strcmp_wa(doc_url, "about:blank"), "doc_url is not about:blank\n");
    }

    SysFreeString(doc_url);
    IHTMLDocument2_Release(doc);
    if(mon && mon != (void*)0xdeadbeef)
        IMoniker_Release(mon);
}

#define test_current_url(a,b) _test_current_url(__LINE__,a,b)
static void _test_current_url(unsigned line, IUnknown *unk, const char *exurl)
{
    IHTMLDocument2 *doc;
    BSTR url;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLDocument2, (void**)&doc);
    ok_(__FILE__,line)(hres == S_OK, "QueryInterface(IID_IHTMLDocument2) failed: %08x\n", hres);

    hres = IHTMLDocument2_get_URL(doc, &url);
    ok_(__FILE__,line)(hres == S_OK, "get_URL failed: %08x\n", hres);
    ok_(__FILE__,line)(!strcmp_wa(url, exurl), "Unexpected URL %s, expected %s\n", wine_dbgstr_w(url), exurl);
    SysFreeString(url);

    IHTMLDocument2_Release(doc);
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

    if(!ignore_external_qi)
        ok(0, "unexpected riid: %s\n", wine_dbgstr_guid(riid));
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
    if(!testing_submit)
        ok(bindf == (BINDF_FROMURLMON|BINDF_PULLDATA|BINDF_NEEDFILE|BINDF_ASYNCSTORAGE|BINDF_ASYNCHRONOUS),
           "bindf = %x\n", bindf);
    else
        ok(bindf == (BINDF_FROMURLMON|BINDF_FORMS_SUBMIT|BINDF_PRAGMA_NO_CACHE|BINDF_HYPERLINK
                     |BINDF_PULLDATA|BINDF_NEEDFILE|BINDF_GETNEWESTVERSION|BINDF_ASYNCSTORAGE|BINDF_ASYNCHRONOUS),
           "bindf = %x\n", bindf);

    ok(bindinfo.cbSize == sizeof(bindinfo), "bindinfo.cbSize=%d\n", bindinfo.cbSize);
    ok(bindinfo.szExtraInfo == NULL, "bindinfo.szExtraInfo=%p\n", bindinfo.szExtraInfo);
    /* TODO: test stgmedData */
    ok(bindinfo.grfBindInfoF == 0, "bindinfo.grfBinfInfoF=%08x\n", bindinfo.grfBindInfoF);
    if(!testing_submit) {
        ok(bindinfo.dwBindVerb == BINDVERB_GET, "bindinfo.dwBindVerb=%d\n", bindinfo.dwBindVerb);
        ok(bindinfo.cbstgmedData == 0, "bindinfo.cbstgmedData=%d\n", bindinfo.cbstgmedData);
        ok(bindinfo.stgmedData.tymed == TYMED_NULL, "bindinfo.stgmedData.tymed=%d\n", bindinfo.stgmedData.tymed);
    }else {
        ok(bindinfo.dwBindVerb == BINDVERB_POST, "bindinfo.dwBindVerb=%d\n", bindinfo.dwBindVerb);
        ok(bindinfo.cbstgmedData == 8, "bindinfo.cbstgmedData=%d\n", bindinfo.cbstgmedData);
        ok(bindinfo.stgmedData.tymed == TYMED_HGLOBAL, "bindinfo.stgmedData.tymed=%d\n", bindinfo.stgmedData.tymed);
        ok(!memcmp(U(bindinfo.stgmedData).hGlobal, "cmd=TEST", 8), "unexpected hGlobal\n");
    }
    ok(bindinfo.szCustomVerb == 0, "bindinfo.szCustomVerb=%p\n", bindinfo.szCustomVerb);
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

    if(report_mime) {
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
                BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE, wszTextCss);
        ok(hres == S_OK,
                "ReportProgress(BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE) failed: %08x\n", hres);
    }

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
    ok(!*pcbRead || *pcbRead==sizeof(css_data)-1, "*pcbRead=%d\n", *pcbRead);

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
        ok(hres == S_FALSE, "GetTargetFrameName failed: %08x\n", hres);
        ok(frame_name == NULL, "frame_name = %p\n", frame_name);

        hres = IHlink_GetMonikerReference(pihlNavigate, 1, &mon, &location);
        ok(hres == S_OK, "GetMonikerReference failed: %08x\n", hres);
        ok(location == NULL, "location = %p\n", location);
        ok(mon != NULL, "mon == NULL\n");

        hres = IMoniker_GetDisplayName(mon, NULL, NULL, &location);
        ok(hres == S_OK, "GetDisplayName failed: %08x\n", hres);
        ok(!strcmp_wa(location, nav_url), "unexpected display name %s, expected %s\n", wine_dbgstr_w(location), nav_url);
        CoTaskMemFree(location);
        IMoniker_Release(mon);

        hres = IHlink_GetHlinkSite(pihlNavigate, &site, &site_data);
        ok(hres == S_OK, "GetHlinkSite failed: %08x\n", hres);
        ok(site == NULL, "site = %p\n, expected NULL\n", site);
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
    ok(!strcmp_wa(pszUrlContext, prev_url), "pszUrlContext = %s\n", wine_dbgstr_w(pszUrlContext));
    ok(!pszFeatures, "pszFeatures = %s\n", wine_dbgstr_w(pszFeatures));
    ok(!fReplace, "fReplace = %x\n", fReplace);
    ok(dwFlags == (allow_new_window ? 0 : NWMF_FIRST), "dwFlags = %x\n", dwFlags);
    ok(!dwUserActionTime, "dwUserActionime = %d\n", dwUserActionTime);

    return allow_new_window ? S_OK : E_FAIL;
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
    if(resetting_document)
        return S_OK;

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
        CHECK_EXPECT2(OnChanged_1005);
        if(!editmode)
            test_readyState(NULL);
        readystate_set_interactive = (load_state != LD_INTERACTIVE);
        return S_OK;
    case 1012:
        CHECK_EXPECT2(OnChanged_1012);
        return S_OK;
    case 1014:
        CHECK_EXPECT2(OnChanged_1014);
        return S_OK;
    case 1030:
    case 3000022:
    case 3000023:
    case 3000024:
    case 3000025:
    case 3000026:
    case 3000027:
    case 3000028:
    case 3000029:
    case 3000030:
    case 3000031:
    case 3000032:
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
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_ISequentialStream, riid) || IsEqualGUID(&IID_IStream, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));
    *ppv = NULL;
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

static HRESULT WINAPI WinInetHttpInfo_QueryInterface(
        IWinInetHttpInfo* This,
        REFIID riid,
        void **ppvObject)
{
    *ppvObject = NULL;

    if(IsEqualGUID(&IID_IGetBindHandle, riid))
        return E_NOINTERFACE;

    ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI WinInetHttpInfo_AddRef(
        IWinInetHttpInfo* This)
{
    return 2;
}

static ULONG WINAPI WinInetHttpInfo_Release(
        IWinInetHttpInfo* This)
{
    return 1;
}

static HRESULT WINAPI WinInetHttpInfo_QueryOption(
        IWinInetHttpInfo* This,
        DWORD dwOption,
        LPVOID pBuffer,
        DWORD *pcbBuf)
{
    return E_NOTIMPL; /* TODO */
}

static HRESULT WINAPI WinInetHttpInfo_QueryInfo(
        IWinInetHttpInfo* This,
        DWORD dwOption,
        LPVOID pBuffer,
        DWORD *pcbBuf,
        DWORD *pdwFlags,
        DWORD *pdwReserved)
{
    ok(pdwReserved == NULL, "pdwReserved != NULL\n");

    if(dwOption == (HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER)) {
        ok(pBuffer != NULL, "pBuffer == NULL\n");
        ok(*pcbBuf == sizeof(DWORD), "*pcbBuf = %d\n", *pcbBuf);
        ok(pdwFlags == NULL, "*pdwFlags != NULL\n");
        *((DWORD*)pBuffer) = status_code;
        return S_OK;
    }

    return E_NOTIMPL; /* TODO */
}

static const IWinInetHttpInfoVtbl WinInetHttpInfoVtbl = {
    WinInetHttpInfo_QueryInterface,
    WinInetHttpInfo_AddRef,
    WinInetHttpInfo_Release,
    WinInetHttpInfo_QueryOption,
    WinInetHttpInfo_QueryInfo
};

static IWinInetHttpInfo WinInetHttpInfo = { &WinInetHttpInfoVtbl };

DEFINE_GUID(IID_unk_binding, 0xf3d8f080,0xa5eb,0x476f,0x9d,0x19,0xa5,0xef,0x24,0xe5,0xc2,0xe6);

static HRESULT WINAPI Binding_QueryInterface(IBinding *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IWinInetInfo, riid) || IsEqualGUID(&IID_IWinInetHttpInfo, riid)) {
        *ppv = &WinInetHttpInfo;
        return S_OK;
    }

    if(IsEqualGUID(&IID_unk_binding, riid)) {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    trace("Binding::QI(%s)\n", wine_dbgstr_guid(riid));
    *ppv = NULL;
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
    if(asynchronous_binding)
        PeekMessageA(NULL, container_hwnd, WM_CONTINUE_BINDING, WM_CONTINUE_BINDING, PM_REMOVE);
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

    ok(0, "unexpected riid: %s\n", wine_dbgstr_guid(riid));
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
    ok(IsEqualGUID(pClassID, &IID_NULL), "pClassID = %s\n", wine_dbgstr_guid(pClassID));
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

static void test_binding_ui(IUnknown *unk)
{
    IWindowForBindingUI *binding_ui;
    IServiceProvider *serv_prov;
    HWND binding_hwnd;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IServiceProvider, (void**)&serv_prov);
    ok(hres == S_OK, "Could not get IServiceProvider: %08x\n", hres);

    hres = IServiceProvider_QueryService(serv_prov, &IID_IWindowForBindingUI, &IID_IWindowForBindingUI,
            (void**)&binding_ui);
    ok(hres == S_OK, "Could not get IWindowForBindingUI: %08x\n", hres);

    hres = IWindowForBindingUI_GetWindow(binding_ui, &IID_IHttpSecurity, &binding_hwnd);
    ok(hres == S_OK, "GetWindow(IID_IHttpSecurity) failed: %08x\n", hres);
    if(doc_hwnd)
        ok(binding_hwnd == doc_hwnd, "binding_hwnd != doc_hwnd\n");
    else
        todo_wine ok(binding_hwnd != NULL, "binding_hwnd == NULL\n");

    hres = IWindowForBindingUI_GetWindow(binding_ui, &IID_IAuthenticate, &binding_hwnd);
    ok(hres == S_OK, "GetWindow(IID_IHttpSecurity) failed: %08x\n", hres);
    if(doc_hwnd)
        ok(binding_hwnd == doc_hwnd, "binding_hwnd != doc_hwnd\n");
    else
        todo_wine ok(binding_hwnd != NULL, "binding_hwnd == NULL\n");

    hres = IWindowForBindingUI_GetWindow(binding_ui, &IID_IWindowForBindingUI, &binding_hwnd);
    ok(hres == S_OK, "GetWindow(IID_IHttpSecurity) failed: %08x\n", hres);
    if(doc_hwnd)
        ok(binding_hwnd == doc_hwnd, "binding_hwnd != doc_hwnd\n");
    else
        todo_wine ok(binding_hwnd != NULL, "binding_hwnd == NULL\n");

    IWindowForBindingUI_Release(binding_ui);
    IServiceProvider_Release(serv_prov);
}

static void continue_binding(IBindStatusCallback *callback)
{
    FORMATETC formatetc = {0xc02d, NULL, 1, -1, TYMED_ISTREAM};
    STGMEDIUM stgmedium;
    HRESULT hres;

    static const WCHAR wszTextHtml[] = {'t','e','x','t','/','h','t','m','l',0};

    test_binding_ui((IUnknown*)callback);

    if(report_mime) {
        hres = IBindStatusCallback_OnProgress(callback, 0, 0, BINDSTATUS_MIMETYPEAVAILABLE,
                wszTextHtml);
        ok(hres == S_OK, "OnProgress(BINDSTATUS_MIMETYPEAVAILABLE) failed: %08x\n", hres);
    }

    hres = IBindStatusCallback_OnProgress(callback, sizeof(html_page)-1, sizeof(html_page)-1,
            BINDSTATUS_BEGINDOWNLOADDATA, doc_url);
    ok(hres == S_OK, "OnProgress(BINDSTATUS_BEGINDOWNLOADDATA) failed: %08x\n", hres);
    if(status_code != HTTP_STATUS_OK) {
        CHECK_CALLED_BROKEN(IsErrorUrl);
        SET_EXPECT(IsErrorUrl);
    }

    SET_EXPECT(Read);
    stgmedium.tymed = TYMED_ISTREAM;
    U(stgmedium).pstm = &Stream;
    stgmedium.pUnkForRelease = (IUnknown*)&Moniker;
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
    CLEAR_CALLED(GetBindResult); /* IE7 */

    IBindStatusCallback_Release(callback);
}

static HRESULT WINAPI Moniker_BindToStorage(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft,
        REFIID riid, void **ppv)
{
    IBindStatusCallback *callback = NULL;
    BINDINFO bindinfo;
    DWORD bindf;
    HRESULT hres;

    static OLECHAR BSCBHolder[] = { '_','B','S','C','B','_','H','o','l','d','e','r','_',0 };

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
    ok((bindf & ~BINDF_HYPERLINK /* IE9 */) == (BINDF_PULLDATA|BINDF_ASYNCSTORAGE|BINDF_ASYNCHRONOUS), "bindf = %08x\n", bindf);
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

    if(asynchronous_binding) {
        PostMessageW(container_hwnd, WM_CONTINUE_BINDING, (WPARAM)callback, 0);
        return MK_S_ASYNCHRONOUS;
    }

    continue_binding(callback);
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
        ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));

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
        if(pActiveObject && is_lang_english())
            ok(!lstrcmpW(wszHTML_Document, pszObjName), "%s != \"HTML Document\"\n", wine_dbgstr_w(pszObjName));
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

        if(pActiveObject && is_lang_english())
            ok(!lstrcmpW(wszHTML_Document, pszObjName), "%s != \"HTML Document\"\n", wine_dbgstr_w(pszObjName));
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
    if(!resetting_document)
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
        ok(lpFrameInfo->cb == sizeof(*lpFrameInfo), "lpFrameInfo->cb = %u, expected %u\n", lpFrameInfo->cb, (unsigned)sizeof(*lpFrameInfo));
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
                HWND hwnd = (void*)0xdeadbeef;
                hres = IOleInPlaceActiveObject_GetWindow(activeobj, &hwnd);
                ok(hres == E_FAIL, "GetWindow returned %08x, expected E_FAIL\n", hres);
                ok(hwnd == NULL, "hwnd=%p, expected NULL\n", hwnd);
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
                    hres = IOleInPlaceActiveObject_GetWindow(activeobj, &doc_hwnd);
                    ok(hres == S_OK, "GetWindow failed: %08x\n", hres);
                    ok(doc_hwnd != NULL, "hwnd == NULL\n");
                    if(last_hwnd)
                        ok(doc_hwnd == last_hwnd, "hwnd != last_hwnd\n");
                }

                hres = IOleDocumentView_UIActivate(view, TRUE);
                ok(hres == S_OK, "UIActivate failed: %08x\n", hres);

                if(activeobj) {
                    hres = IOleInPlaceActiveObject_GetWindow(activeobj, &tmp_hwnd);
                    ok(hres == S_OK, "GetWindow failed: %08x\n", hres);
                    ok(tmp_hwnd == doc_hwnd, "tmp_hwnd=%p, expected %p\n", tmp_hwnd, doc_hwnd);
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
                    hres = IOleInPlaceActiveObject_GetWindow(activeobj, &doc_hwnd);
                    ok(hres == S_OK, "GetWindow failed: %08x\n", hres);
                    ok(doc_hwnd != NULL, "doc_hwnd == NULL\n");
                    if(last_hwnd)
                        ok(doc_hwnd == last_hwnd, "doc_hwnd != last_hwnd\n");
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

static IDocHostUIHandler2 *expect_uihandler_iface;

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
    ok(iface == expect_uihandler_iface, "called on unexpected iface\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetHostInfo(IDocHostUIHandler2 *iface, DOCHOSTUIINFO *pInfo)
{
    if(!resetting_document)
        CHECK_EXPECT(GetHostInfo);
    ok(iface == expect_uihandler_iface, "called on unexpected iface\n");
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
    ok(iface == expect_uihandler_iface, "called on unexpected iface\n");

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
    ok(iface == expect_uihandler_iface, "called on unexpected iface\n");
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_UpdateUI(IDocHostUIHandler2 *iface)
{
    CHECK_EXPECT(UpdateUI);
    ok(iface == expect_uihandler_iface, "called on unexpected iface\n");
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_EnableModeless(IDocHostUIHandler2 *iface, BOOL fEnable)
{
    ok(iface == expect_uihandler_iface, "called on unexpected iface\n");
    if(fEnable)
        CHECK_EXPECT2(EnableModeless_TRUE);
    else
        CHECK_EXPECT2(EnableModeless_FALSE);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_OnDocWindowActivate(IDocHostUIHandler2 *iface, BOOL fActivate)
{
    ok(0, "unexpected call\n");
    ok(iface == expect_uihandler_iface, "called on unexpected iface\n");
    return E_NOTIMPL;
}

static BOOL expect_OnFrameWindowActivate_fActivate;
static HRESULT WINAPI DocHostUIHandler_OnFrameWindowActivate(IDocHostUIHandler2 *iface, BOOL fActivate)
{
    CHECK_EXPECT2(OnFrameWindowActivate);
    ok(iface == expect_uihandler_iface, "called on unexpected iface\n");
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
    ok(iface == expect_uihandler_iface, "called on unexpected iface\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetOptionKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    CHECK_EXPECT(GetOptionKeyPath);
    ok(iface == expect_uihandler_iface, "called on unexpected iface\n");
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
    ok(iface == expect_uihandler_iface, "called on unexpected iface\n");
    /* TODO */
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetExternal(IDocHostUIHandler2 *iface, IDispatch **ppDispatch)
{
    CHECK_EXPECT(GetExternal);
    ok(iface == expect_uihandler_iface, "called on unexpected iface\n");
    *ppDispatch = &External;
    return S_FALSE;
}

static HRESULT WINAPI DocHostUIHandler_TranslateUrl(IDocHostUIHandler2 *iface, DWORD dwTranslate,
        OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
    CHECK_EXPECT(TranslateUrl);
    ok(iface == expect_uihandler_iface, "called on unexpected iface\n");
    ok(!dwTranslate, "dwTranslate = %x\n", dwTranslate);
    if(!loading_hash)
        ok(!strcmp_wa(pchURLIn, nav_serv_url), "pchURLIn = %s, expected %s\n", wine_dbgstr_w(pchURLIn), nav_serv_url);
    else
        todo_wine ok(!strcmp_wa(pchURLIn, nav_serv_url), "pchURLIn = %s, expected %s\n", wine_dbgstr_w(pchURLIn), nav_serv_url);
    ok(ppchURLOut != NULL, "ppchURLOut == NULL\n");
    ok(!*ppchURLOut, "*ppchURLOut = %p\n", *ppchURLOut);

    return S_FALSE;
}

static HRESULT WINAPI DocHostUIHandler_FilterDataObject(IDocHostUIHandler2 *iface, IDataObject *pDO,
        IDataObject **ppPORet)
{
    ok(0, "unexpected call\n");
    ok(iface == expect_uihandler_iface, "called on unexpected iface\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetOverrideKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    CHECK_EXPECT(GetOverrideKeyPath);
    ok(iface == expect_uihandler_iface, "called on unexpected iface\n");
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

static HRESULT WINAPI CustomDocHostUIHandler_QueryInterface(IDocHostUIHandler2 *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IDocHostUIHandler2, riid)) {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;

    if(IsEqualGUID(&IID_IOleCommandTarget, riid))
        return E_NOINTERFACE;

    if(IsEqualGUID(&IID_IDocHostShowUI, riid))
        return E_NOINTERFACE; /* TODO */

    trace("CustomDocHostUIHandler::QI(%s)\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static const IDocHostUIHandler2Vtbl CustomDocHostUIHandlerVtbl = {
    CustomDocHostUIHandler_QueryInterface,
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

static IDocHostUIHandler2 CustomDocHostUIHandler = { &CustomDocHostUIHandlerVtbl };

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

static void test_save_history(IUnknown *unk)
{
    IPersistHistory *per_hist;
    LARGE_INTEGER li;
    IStream *stream;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IPersistHistory, (void**)&per_hist);
    ok(hres == S_OK, "Could not get IPersistHistory iface: %08x\n", hres);

    hres = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hres == S_OK, "CreateStreamOnHGlobal failed: %08x\n", hres);

    hres = IPersistHistory_SaveHistory(per_hist, stream);
    ok(hres == S_OK, "SaveHistory failed: %08x\n", hres);
    IPersistHistory_Release(per_hist);

    li.QuadPart = 0;
    hres = IStream_Seek(stream, li, STREAM_SEEK_SET, NULL);
    ok(hres == S_OK, "Stat failed: %08x\n", hres);
    history_stream = stream;
}

static HRESULT WINAPI OleCommandTarget_Exec(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    if(resetting_document)
        return E_FAIL;

    if(!pguidCmdGroup) {
        test_readyState(NULL);

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
                if(!loading_js)
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
        case OLECMDID_PAGEAVAILABLE:
        case 6058:
            return E_FAIL; /* FIXME */
        default:
            ok(0, "unexpected command %d\n", nCmdID);
            return E_FAIL;
        };
    }

    if(IsEqualGUID(&CGID_ShellDocView, pguidCmdGroup)) {
        if(nCmdID != 63 && nCmdID != 178 && (!is_refresh || nCmdID != 37))
            test_readyState(NULL);
        ok(nCmdexecopt == 0, "nCmdexecopts=%08x\n", nCmdexecopt);

        switch(nCmdID) {
        case 37:
            CHECK_EXPECT2(Exec_ShellDocView_37);

            if(is_refresh && load_state == LD_COMPLETE) {
                load_state = LD_DOLOAD;
                test_readyState(NULL);
            }else if(is_refresh && load_state == LD_DOLOAD) {
                test_readyState(NULL);
                load_state = LD_LOADING;
            }else {
                if(nav_url)
                    test_GetCurMoniker(doc_unk, NULL, nav_serv_url, FALSE);
                else if(load_from_stream)
                    test_GetCurMoniker(doc_unk, NULL, "about:blank", FALSE);
                else if(!editmode)
                    test_GetCurMoniker(doc_unk, doc_mon, NULL, FALSE);
            }

            ok(pvaOut == NULL, "pvaOut=%p, expected NULL\n", pvaOut);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            if(pvaIn) {
                ok(V_VT(pvaIn) == VT_I4, "V_VT(pvaIn)=%d, expected VT_I4\n", V_VT(pvaIn));
                ok(V_I4(pvaIn) == 0, "V_I4(pvaIn)=%d, expected 0\n", V_I4(pvaIn));
            }
            return S_OK;

        case 62:
            CHECK_EXPECT(Exec_ShellDocView_62);
            ok(!pvaIn, "pvaIn != NULL\n");
            ok(!pvaOut, "pvaOut != NULL\n");
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

            load_state = loading_js ? LD_COMPLETE : LD_LOADING;
            return S_OK; /* TODO */
        }

        case 67:
            CHECK_EXPECT(Exec_ShellDocView_67);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            ok(V_VT(pvaIn) == VT_BSTR, "V_VT(pvaIn) = %d\n", V_VT(pvaIn));
            ok(!strcmp_wa(V_BSTR(pvaIn), nav_serv_url), "V_BSTR(pvaIn) = %s, expected \"%s\"\n",
               wine_dbgstr_w(V_BSTR(pvaIn)), nav_serv_url);
            ok(pvaOut != NULL, "pvaOut == NULL\n");
            ok(V_VT(pvaOut) == VT_BOOL, "V_VT(pvaOut) = %d\n", V_VT(pvaOut));
            ok(V_BOOL(pvaOut) == VARIANT_TRUE, "V_BOOL(pvaOut) = %x\n", V_BOOL(pvaOut));
            if(!loading_hash)
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

        case 138:
            CHECK_EXPECT2(Exec_ShellDocView_138);
            ok(!pvaIn, "pvaIn != NULL\n");
            ok(!pvaOut, "pvaOut != NULL\n");
            return S_OK;

        case 140:
            CHECK_EXPECT2(Exec_ShellDocView_140);

            ok(pvaIn == NULL, "pvaIn != NULL\n");
            ok(pvaOut == NULL, "pvaOut != NULL\n");

            return E_NOTIMPL;

        case 83:
        case 102:
        case 133:
        case 134: /* TODO */
        case 135:
        case 136: /* TODO */
        case 137:
        case 139: /* TODO */
        case 143: /* TODO */
        case 144: /* TODO */
        case 178:
        case 179:
        case 180:
        case 181:
        case 182:
            return E_NOTIMPL;

        default:
            ok(0, "unexpected command %d\n", nCmdID);
            return E_FAIL;
        };
    }

    if(IsEqualGUID(&CGID_MSHTML, pguidCmdGroup)) {
        test_readyState(NULL);
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
            if(pvaIn) {
                CHECK_EXPECT(Exec_DOCCANNAVIGATE);
                ok(V_VT(pvaIn) == VT_UNKNOWN, "V_VT(pvaIn) != VT_UNKNOWN\n");
                /* FIXME: test V_UNKNOWN(pvaIn) == window */
            }else {
                CHECK_EXPECT(Exec_DOCCANNAVIGATE_NULL);
            }

            test_readyState(NULL);
            ok(pvaOut == NULL, "pvaOut != NULL\n");
            return S_OK;
        case 1: {
            SAFEARRAY *sa;
            UINT dim;
            LONG ind=0;
            VARIANT var;
            HRESULT hres;

            test_readyState(NULL);

            ok(pvaIn != NULL, "pvaIn == NULL\n");
            ok(pvaOut != NULL || broken(!pvaOut), "pvaOut != NULL\n");
            ok(V_VT(pvaIn) == VT_ARRAY, "V_VT(pvaIn) = %d\n", V_VT(pvaIn));
            if(pvaOut)
                ok(V_VT(pvaOut) == VT_BOOL, "V_VT(pvaOut) = %d\n", V_VT(pvaOut));
            sa = V_ARRAY(pvaIn);

            dim = SafeArrayGetDim(sa);
            ok(dim == 1, "dim = %d\n", dim);
            hres = SafeArrayGetLBound(sa, 1, &ind);
            ok(hres == S_OK, "SafeArrayGetLBound failed: %x\n", hres);
            ok(ind == 0, "Lower bound = %d\n", ind);
            hres = SafeArrayGetUBound(sa, 1, &ind);
            ok(hres == S_OK, "SafeArrayGetUBound failed: %x\n", hres);
            ok(ind == 7 || ind == 8 /* IE11 */ ||broken(ind == 5), "Upper bound = %d\n", ind);

            ind = 0;
            SafeArrayGetElement(sa, &ind, &var);
            ok(V_VT(&var) == VT_I4, "Incorrect data type: %d\n", V_VT(&var));
            ok(V_I4(&var) == status_code, "Incorrect error code: %d\n", V_I4(&var));
            VariantClear(&var);
            ind = 1;
            SafeArrayGetElement(sa, &ind, &var);
            ok(V_VT(&var) == VT_BSTR, "Incorrect data type: %d\n", V_VT(&var));
            ok(!strcmp_wa(V_BSTR(&var), "winetest:doc"), "Page address: %s\n", wine_dbgstr_w(V_BSTR(&var)));
            VariantClear(&var);
            ind = 2;
            SafeArrayGetElement(sa, &ind, &var);
            ok(V_VT(&var) == VT_UNKNOWN, "Incorrect data type: %d\n", V_VT(&var));
            VariantClear(&var);
            ind = 3;
            SafeArrayGetElement(sa, &ind, &var);
            ok(V_VT(&var) == VT_UNKNOWN, "Incorrect data type: %d\n", V_VT(&var));
            VariantClear(&var);
            ind = 4;
            SafeArrayGetElement(sa, &ind, &var);
            ok(V_VT(&var) == VT_BOOL, "Incorrect data type: %d\n", V_VT(&var));
            ok(!V_BOOL(&var), "Unknown value is incorrect\n");
            VariantClear(&var);
            ind = 5;
            SafeArrayGetElement(sa, &ind, &var);
            ok(V_VT(&var) == VT_BOOL, "Incorrect data type: %d\n", V_VT(&var));
            ok(!V_BOOL(&var), "Unknown value is incorrect\n");
            VariantClear(&var);
        }
        default:
            return E_FAIL; /* TODO */
        }
    }

    if(IsEqualGUID(&CGID_Explorer, pguidCmdGroup)) {
        test_readyState(NULL);
        ok(nCmdexecopt == 0, "nCmdexecopts=%08x\n", nCmdexecopt);

        switch(nCmdID) {
        case 38:
            CHECK_EXPECT2(Exec_Explorer_38);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            ok(V_VT(pvaIn) == VT_I4 , "V_VT(pvaIn) = %d\n", V_VT(pvaIn));
            ok(!V_I4(pvaIn), "V_I4(pvaIn) = %d\n", V_I4(pvaIn));
            ok(!pvaOut, "pvaOut != NULL\n");

            test_current_url(doc_unk, prev_url);
            if(!history_stream)
                test_save_history(doc_unk);

            return S_OK;
        case 69:
            CHECK_EXPECT2(Exec_Explorer_69);
            ok(pvaIn == NULL, "pvaIn != NULL\n");
            ok(pvaOut != NULL, "pvaOut == NULL\n");
            return E_NOTIMPL;
        case 109: /* TODO */
            return E_NOTIMPL;
        default:
            ok(0, "unexpected cmd %d of CGID_Explorer\n", nCmdID);
        }
        return E_NOTIMPL;
    }

    if(IsEqualGUID(&CGID_DocHostCommandHandler, pguidCmdGroup)) {
        test_readyState(NULL);

        switch (nCmdID) {
        case OLECMDID_PAGEACTIONBLOCKED: /* win2k3 */
            SET_EXPECT(SetStatusText);
            ok(pvaIn == NULL, "pvaIn != NULL\n");
            ok(pvaOut == NULL, "pvaOut != NULL\n");
            return S_OK;
        case OLECMDID_SHOWSCRIPTERROR:
            /* TODO */
            return S_OK;
        case 2300:
            CHECK_EXPECT(Exec_DocHostCommandHandler_2300);
            return E_NOTIMPL;
        default:
            ok(0, "unexpected command %d\n", nCmdID);
            return E_FAIL;
        }
    }

    ok(0, "unexpected pguidCmdGroup: %s\n", wine_dbgstr_guid(pguidCmdGroup));
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
    if(resetting_document)
        return E_FAIL;

    ok(IsEqualGUID(&IID_NULL, riid), "riid != IID_NULL\n");
    ok(pDispParams != NULL, "pDispParams == NULL\n");
    ok(pExcepInfo == NULL, "pExcepInfo=%p, expected NULL\n", pExcepInfo);
    ok(puArgErr != NULL, "puArgErr == NULL\n");
    ok(V_VT(pVarResult) == 0, "V_VT(pVarResult)=%d, expected 0\n", V_VT(pVarResult));
    ok(wFlags == DISPATCH_PROPERTYGET, "wFlags=%08x, expected DISPATCH_PROPERTYGET\n", wFlags);

    if(dispIdMember != DISPID_AMBIENT_SILENT && dispIdMember != DISPID_AMBIENT_OFFLINEIFNOTCONNECTED)
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

static const IDispatchVtbl DispatchVtbl = {
    Dispatch_QueryInterface,
    Dispatch_AddRef,
    Dispatch_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    Dispatch_GetIDsOfNames,
    Dispatch_Invoke
};

static IDispatch Dispatch = { &DispatchVtbl };

static HRESULT WINAPI EventDispatch_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "Unexpected call\n");
    return E_NOINTERFACE;
}

static HRESULT WINAPI EventDispatch_Invoke(IDispatch *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HRESULT hres;
    IHTMLDocument2 *doc;
    BSTR state;

    if(resetting_document)
        return E_FAIL;

    ok(IsEqualGUID(&IID_NULL, riid), "riid = %s\n", wine_dbgstr_guid(riid));
    ok(pDispParams != NULL, "pDispParams == NULL\n");
    ok(pExcepInfo != NULL, "pExcepInfo == NULL\n");
    ok(puArgErr != NULL, "puArgErr == NULL\n");
    ok(V_VT(pVarResult) == 0, "V_VT(pVarResult) = %d\n", V_VT(pVarResult));
    ok(wFlags == DISPATCH_METHOD, "wFlags = %d, expected DISPATCH_METHOD\n", wFlags);

    hres = IUnknown_QueryInterface(doc_unk, &IID_IHTMLDocument2, (void**)&doc);
    ok(hres == S_OK, "QueryInterface(IID_IHTMLDocument2) failed: %08x\n", hres);

    switch(dispIdMember) {
    case DISPID_HTMLDOCUMENTEVENTS2_ONREADYSTATECHANGE:
        hres = IHTMLDocument2_get_readyState(doc, &state);
        ok(hres == S_OK, "get_readyState failed: %08x\n", hres);

        if(!strcmp_wa(state, "interactive"))
            CHECK_EXPECT(Invoke_OnReadyStateChange_Interactive);
        else if(!strcmp_wa(state, "loading"))
            CHECK_EXPECT(Invoke_OnReadyStateChange_Loading);
        else if(!strcmp_wa(state, "complete")) {
            CHECK_EXPECT(Invoke_OnReadyStateChange_Complete);
            complete = TRUE;
        } else
            ok(0, "Unexpected readyState: %s\n", wine_dbgstr_w(state));

        SysFreeString(state);
        break;
    case DISPID_HTMLDOCUMENTEVENTS2_ONPROPERTYCHANGE:
    case 1026:
    case 1027:
    case 1034:
    case 1035:
    case 1037:
    case 1047:
    case 1045:
    case 1044:
    case 1048:
    case 1049:
        break; /* FIXME: Handle these events. */
    default:
        ok(0, "Unexpected DISPID: %d\n", dispIdMember);
    }

    IHTMLDocument2_Release(doc);
    return S_OK;
}

static const IDispatchVtbl EventDispatchVtbl = {
    EventDispatch_QueryInterface,
    Dispatch_AddRef,
    Dispatch_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    Dispatch_GetIDsOfNames,
    EventDispatch_Invoke
};

static IDispatch EventDispatch = { &EventDispatchVtbl };

static HRESULT WINAPI TravelLog_QueryInterface(ITravelLog *iface, REFIID riid, void **ppv)
{
    static const IID IID_IIETravelLog2 = {0xb67cefd2,0xe3f1,0x478a,{0x9b,0xfa,0xd8,0x93,0x70,0x37,0x5e,0x94}};
    static const IID IID_unk_travellog = {0x6afc8b7f,0xbc17,0x4a95,{0x90,0x2f,0x6f,0x5c,0xb5,0x54,0xc3,0xd8}};
    static const IID IID_unk_travellog2 = {0xf6d02767,0x9c80,0x428d,{0xb9,0x74,0x3f,0x17,0x29,0x45,0x3f,0xdb}};

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_ITravelLog, riid)) {
        *ppv = iface;
        return S_OK;
    }

    if(!IsEqualGUID(&IID_IIETravelLog2, riid) && !IsEqualGUID(&IID_unk_travellog, riid)
       && !IsEqualGUID(&IID_unk_travellog2, riid))
        ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI TravelLog_AddRef(ITravelLog *iface)
{
    return 2;
}

static ULONG WINAPI TravelLog_Release(ITravelLog *iface)
{
    return 1;
}

static HRESULT WINAPI TravelLog_AddEntry(ITravelLog *iface, IUnknown *punk, BOOL fIsLocalAnchor)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TravelLog_UpdateEntry(ITravelLog *iface, IUnknown *punk, BOOL fIsLocalAnchor)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TravelLog_UpdateExternal(ITravelLog *iface, IUnknown *punk, IUnknown *punkHLBrowseContext)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TravelLog_Travel(ITravelLog *iface, IUnknown *punk, int iOffset)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TravelLog_GetTravelEntry(ITravelLog *iface, IUnknown *punk, int iOffset, ITravelEntry **ppte)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TravelLog_FindTravelEntry(ITravelLog *iface, IUnknown *punk, LPCITEMIDLIST pidl, ITravelEntry **ppte)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TravelLog_GetTooltipText(ITravelLog *iface, IUnknown *punk, int iOffset, int idsTemplate,
        LPWSTR pwzText, DWORD cchText)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TravelLog_InsertMenuEntries(ITravelLog *iface, IUnknown *punk, HMENU hmenu, int nPos,
        int idFirst, int idLast, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TravelLog_Clone(ITravelLog *iface, ITravelLog **pptl)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IBrowserService BrowserService;
static DWORD WINAPI TravelLog_CountEntries(ITravelLog *iface, IUnknown *punk)
{
    CHECK_EXPECT(CountEntries);

    ok(punk == (IUnknown*)&BrowserService, "punk != &BrowserService (%p)\n", punk);
    return 0;
}

static HRESULT WINAPI TravelLog_Revert(ITravelLog *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const ITravelLogVtbl TravelLogVtbl = {
    TravelLog_QueryInterface,
    TravelLog_AddRef,
    TravelLog_Release,
    TravelLog_AddEntry,
    TravelLog_UpdateEntry,
    TravelLog_UpdateExternal,
    TravelLog_Travel,
    TravelLog_GetTravelEntry,
    TravelLog_FindTravelEntry,
    TravelLog_GetTooltipText,
    TravelLog_InsertMenuEntries,
    TravelLog_Clone,
    TravelLog_CountEntries,
    TravelLog_Revert
};

static ITravelLog TravelLog = { &TravelLogVtbl };

static HRESULT browserservice_qi(REFIID,void**);

static HRESULT  WINAPI DocObjectService_QueryInterface(IDocObjectService* This, REFIID riid, void **ppv)
{
    return browserservice_qi(riid, ppv);
}

static ULONG  WINAPI DocObjectService_AddRef(
        IDocObjectService* This)
{
    return 2;
}

static ULONG  WINAPI DocObjectService_Release(
        IDocObjectService* This)
{
    return 1;
}

static HRESULT  WINAPI DocObjectService_FireBeforeNavigate2(IDocObjectService *iface, IDispatch *pDispatch,
        LPCWSTR lpszUrl, DWORD dwFlags, LPCWSTR lpszFrameName, BYTE *pPostData, DWORD cbPostData,
        LPCWSTR lpszHeaders, BOOL fPlayNavSound, BOOL *pfCancel)
{
    CHECK_EXPECT(FireBeforeNavigate2);

    ok(!pDispatch, "pDispatch = %p\n", pDispatch);
    ok(!strcmp_wa(lpszUrl, nav_url), "lpszUrl = %s, expected %s\n", wine_dbgstr_w(lpszUrl), nav_url);
    ok(dwFlags == 0x140 /* IE11*/ || dwFlags == 0x40 || !dwFlags || dwFlags == 0x50, "dwFlags = %x\n", dwFlags);
    ok(!lpszFrameName, "lpszFrameName = %s\n", wine_dbgstr_w(lpszFrameName));
    if(!testing_submit) {
        ok(!pPostData, "pPostData = %p\n", pPostData);
        ok(!cbPostData, "cbPostData = %d\n", cbPostData);
        ok(!lpszHeaders, "lpszHeaders = %s\n", wine_dbgstr_w(lpszHeaders));
    }else {
        ok(cbPostData == 9, "cbPostData = %d\n", cbPostData);
        ok(!memcmp(pPostData, "cmd=TEST", cbPostData), "pPostData = %p\n", pPostData);
        ok(wstr_contains(lpszHeaders, "Content-Type: application/x-www-form-urlencoded\r\n"),
           "lpszHeaders = %s\n", wine_dbgstr_w(lpszHeaders));

    }
    ok(fPlayNavSound, "fPlayNavSound = %x\n", fPlayNavSound);
    ok(pfCancel != NULL, "pfCancel = NULL\n");
    ok(!*pfCancel, "*pfCancel = %x\n", *pfCancel);

    return S_OK;
}

static HRESULT  WINAPI DocObjectService_FireNavigateComplete2(
        IDocObjectService* This,
        IHTMLWindow2 *pHTMLWindow2,
        DWORD dwFlags)
{
    CHECK_EXPECT(FireNavigateComplete2);
    test_readyState(NULL);

    if(loading_hash)
        ok(dwFlags == 0x10 || broken(!dwFlags), "dwFlags = %x, expected 0x10\n", dwFlags);
    else
        ok(!(dwFlags &~1), "dwFlags = %x\n", dwFlags);

    ok(pHTMLWindow2 != NULL, "pHTMLWindow2 = NULL\n");

    return S_OK;
}

static HRESULT  WINAPI DocObjectService_FireDownloadBegin(
        IDocObjectService* This)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI DocObjectService_FireDownloadComplete(
        IDocObjectService* This)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI DocObjectService_FireDocumentComplete(
        IDocObjectService* This,
        IHTMLWindow2 *pHTMLWindow,
        DWORD dwFlags)
{
    CHECK_EXPECT(FireDocumentComplete);

    ok(pHTMLWindow != NULL, "pHTMLWindow == NULL\n");
    ok(!dwFlags, "dwFlags = %x\n", dwFlags);

    return S_OK;
}

static HRESULT  WINAPI DocObjectService_UpdateDesktopComponent(
        IDocObjectService* This,
        IHTMLWindow2 *pHTMLWindow)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI DocObjectService_GetPendingUrl(
        IDocObjectService* This,
        BSTR *pbstrPendingUrl)
{
    if(!resetting_document)
        CHECK_EXPECT(GetPendingUrl);
    return E_NOTIMPL;
}

static HRESULT  WINAPI DocObjectService_ActiveElementChanged(
        IDocObjectService* This,
        IHTMLElement *pHTMLElement)
{
    CHECK_EXPECT2(ActiveElementChanged);
    return E_NOTIMPL;
}

static HRESULT  WINAPI DocObjectService_GetUrlSearchComponent(
        IDocObjectService* This,
        BSTR *pbstrSearch)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI DocObjectService_IsErrorUrl(
        IDocObjectService* This,
        LPCWSTR lpszUrl,
        BOOL *pfIsError)
{
    CHECK_EXPECT(IsErrorUrl);
    *pfIsError = FALSE;
    return S_OK;
}

static IDocObjectServiceVtbl DocObjectServiceVtbl = {
    DocObjectService_QueryInterface,
    DocObjectService_AddRef,
    DocObjectService_Release,
    DocObjectService_FireBeforeNavigate2,
    DocObjectService_FireNavigateComplete2,
    DocObjectService_FireDownloadBegin,
    DocObjectService_FireDownloadComplete,
    DocObjectService_FireDocumentComplete,
    DocObjectService_UpdateDesktopComponent,
    DocObjectService_GetPendingUrl,
    DocObjectService_ActiveElementChanged,
    DocObjectService_GetUrlSearchComponent,
    DocObjectService_IsErrorUrl
};

static IDocObjectService DocObjectService = { &DocObjectServiceVtbl };

static HRESULT WINAPI ShellBrowser_QueryInterface(IShellBrowser *iface, REFIID riid, void **ppv)
{
    return browserservice_qi(riid, ppv);
}

static ULONG WINAPI ShellBrowser_AddRef(IShellBrowser *iface)
{
    return 2;
}

static ULONG WINAPI ShellBrowser_Release(IShellBrowser *iface)
{
    return 1;
}

static HRESULT WINAPI ShellBrowser_GetWindow(IShellBrowser *iface, HWND *phwnd)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_ContextSensitiveHelp(IShellBrowser *iface, BOOL fEnterMode)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_InsertMenusSB(IShellBrowser *iface, HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_SetMenuSB(IShellBrowser *iface, HMENU hmenuShared, HOLEMENU holemenuReserved,
        HWND hwndActiveObject)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_RemoveMenusSB(IShellBrowser *iface, HMENU hmenuShared)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_SetStatusTextSB(IShellBrowser *iface, LPCOLESTR pszStatusText)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_EnableModelessSB(IShellBrowser *iface, BOOL fEnable)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_TranslateAcceleratorSB(IShellBrowser *iface, MSG *pmsg, WORD wID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_BrowseObject(IShellBrowser *iface, LPCITEMIDLIST pidl, UINT wFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_GetViewStateStream(IShellBrowser *iface, DWORD grfMode, IStream **ppStrm)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_GetControlWindow(IShellBrowser *iface, UINT id, HWND *phwnd)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_SendControlMsg(IShellBrowser *iface, UINT id, UINT uMsg, WPARAM wParam,
        LPARAM lParam, LRESULT *pret)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_QueryActiveShellView(IShellBrowser *iface, IShellView **ppshv)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_OnViewWindowActive(IShellBrowser* iface, IShellView *pshv)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_SetToolbarItems(IShellBrowser *iface, LPTBBUTTONSB lpButtons,
        UINT nButtons, UINT uFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IShellBrowserVtbl ShellBrowserVtbl = {
    ShellBrowser_QueryInterface,
    ShellBrowser_AddRef,
    ShellBrowser_Release,
    ShellBrowser_GetWindow,
    ShellBrowser_ContextSensitiveHelp,
    ShellBrowser_InsertMenusSB,
    ShellBrowser_SetMenuSB,
    ShellBrowser_RemoveMenusSB,
    ShellBrowser_SetStatusTextSB,
    ShellBrowser_EnableModelessSB,
    ShellBrowser_TranslateAcceleratorSB,
    ShellBrowser_BrowseObject,
    ShellBrowser_GetViewStateStream,
    ShellBrowser_GetControlWindow,
    ShellBrowser_SendControlMsg,
    ShellBrowser_QueryActiveShellView,
    ShellBrowser_OnViewWindowActive,
    ShellBrowser_SetToolbarItems
};

static IShellBrowser ShellBrowser = { &ShellBrowserVtbl };

static HRESULT  WINAPI BrowserService_QueryInterface(IBrowserService *iface, REFIID riid, void **ppv)
{
    return browserservice_qi(riid, ppv);
}

static ULONG  WINAPI BrowserService_AddRef(
        IBrowserService* This)
{
    return 2;
}

static ULONG  WINAPI BrowserService_Release(
        IBrowserService* This)
{
    return 1;
}

static HRESULT  WINAPI BrowserService_GetParentSite(
        IBrowserService* This,
        IOleInPlaceSite **ppipsite)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_SetTitle(
        IBrowserService* This,
        IShellView *psv,
        LPCWSTR pszName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_GetTitle(
        IBrowserService* This,
        IShellView *psv,
        LPWSTR pszName,
        DWORD cchName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_GetOleObject(
        IBrowserService* This,
        IOleObject **ppobjv)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_GetTravelLog(IBrowserService* This, ITravelLog **pptl)
{
    CHECK_EXPECT(GetTravelLog);

    ok(pptl != NULL, "pptl = NULL\n");

    if(!support_wbapp)
        return E_NOTIMPL;

    *pptl = &TravelLog;
    return S_OK;
}

static HRESULT  WINAPI BrowserService_ShowControlWindow(
        IBrowserService* This,
        UINT id,
        BOOL fShow)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_IsControlWindowShown(
        IBrowserService* This,
        UINT id,
        BOOL *pfShown)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_IEGetDisplayName(
        IBrowserService* This,
        PCIDLIST_ABSOLUTE pidl,
        LPWSTR pwszName,
        UINT uFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_IEParseDisplayName(
        IBrowserService* This,
        UINT uiCP,
        LPCWSTR pwszPath,
        PIDLIST_ABSOLUTE *ppidlOut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_DisplayParseError(
        IBrowserService* This,
        HRESULT hres,
        LPCWSTR pwszPath)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_NavigateToPidl(
        IBrowserService* This,
        PCIDLIST_ABSOLUTE pidl,
        DWORD grfHLNF)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_SetNavigateState(
        IBrowserService* This,
        BNSTATE bnstate)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_GetNavigateState(
        IBrowserService* This,
        BNSTATE *pbnstate)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_NotifyRedirect(
        IBrowserService* This,
        IShellView *psv,
        PCIDLIST_ABSOLUTE pidl,
        BOOL *pfDidBrowse)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_UpdateWindowList(
        IBrowserService* This)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_UpdateBackForwardState(
        IBrowserService* This)
{
    CHECK_EXPECT(UpdateBackForwardState);
    return S_OK;
}

static HRESULT  WINAPI BrowserService_SetFlags(
        IBrowserService* This,
        DWORD dwFlags,
        DWORD dwFlagMask)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_GetFlags(
        IBrowserService* This,
        DWORD *pdwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_CanNavigateNow(
        IBrowserService* This)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_GetPidl(
        IBrowserService* This,
        PIDLIST_ABSOLUTE *ppidl)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_SetReferrer(
        IBrowserService* This,
        PCIDLIST_ABSOLUTE pidl)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static DWORD  WINAPI BrowserService_GetBrowserIndex(
        IBrowserService* This)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_GetBrowserByIndex(
        IBrowserService* This,
        DWORD dwID,
        IUnknown **ppunk)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_GetHistoryObject(
        IBrowserService* This,
        IOleObject **ppole,
        IStream **pstm,
        IBindCtx **ppbc)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_SetHistoryObject(
        IBrowserService* This,
        IOleObject *pole,
        BOOL fIsLocalAnchor)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_CacheOLEServer(
        IBrowserService* This,
        IOleObject *pole)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_GetSetCodePage(
        IBrowserService* This,
        VARIANT *pvarIn,
        VARIANT *pvarOut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_OnHttpEquiv(
        IBrowserService* This,
        IShellView *psv,
        BOOL fDone,
        VARIANT *pvarargIn,
        VARIANT *pvarargOut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_GetPalette(
        IBrowserService* This,
        HPALETTE *hpal)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT  WINAPI BrowserService_RegisterWindow(
        IBrowserService* This,
        BOOL fForceRegister,
        int swc)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IBrowserServiceVtbl BrowserServiceVtbl = {
    BrowserService_QueryInterface,
    BrowserService_AddRef,
    BrowserService_Release,
    BrowserService_GetParentSite,
    BrowserService_SetTitle,
    BrowserService_GetTitle,
    BrowserService_GetOleObject,
    BrowserService_GetTravelLog,
    BrowserService_ShowControlWindow,
    BrowserService_IsControlWindowShown,
    BrowserService_IEGetDisplayName,
    BrowserService_IEParseDisplayName,
    BrowserService_DisplayParseError,
    BrowserService_NavigateToPidl,
    BrowserService_SetNavigateState,
    BrowserService_GetNavigateState,
    BrowserService_NotifyRedirect,
    BrowserService_UpdateWindowList,
    BrowserService_UpdateBackForwardState,
    BrowserService_SetFlags,
    BrowserService_GetFlags,
    BrowserService_CanNavigateNow,
    BrowserService_GetPidl,
    BrowserService_SetReferrer,
    BrowserService_GetBrowserIndex,
    BrowserService_GetBrowserByIndex,
    BrowserService_GetHistoryObject,
    BrowserService_SetHistoryObject,
    BrowserService_CacheOLEServer,
    BrowserService_GetSetCodePage,
    BrowserService_OnHttpEquiv,
    BrowserService_GetPalette,
    BrowserService_RegisterWindow
};

static IBrowserService BrowserService = { &BrowserServiceVtbl };

DEFINE_GUID(IID_ITabBrowserService, 0x5E8FA523,0x83D4,0x4DBE,0x81,0x99,0x4C,0x18,0xE4,0x85,0x87,0x25);

static HRESULT browserservice_qi(REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IShellBrowser, riid)) {
        *ppv = &ShellBrowser;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IDocObjectService, riid)) {
        *ppv = &DocObjectService;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IBrowserService, riid)) {
        *ppv = &BrowserService;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static HRESULT WINAPI WBE2Sink_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    ok(0, "unexpected riid: %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static HRESULT WINAPI WBE2Sink_Invoke(IDispatch *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pdp, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(IsEqualGUID(&IID_NULL, riid), "riid != IID_NULL\n");
    ok(pdp != NULL, "pDispParams == NULL\n");
    ok(pExcepInfo == NULL, "pExcepInfo=%p, expected NULL\n", pExcepInfo);
    ok(puArgErr == NULL, "puArgErr != NULL\n");
    ok(pVarResult == NULL, "pVarResult != NULL\n");
    ok(wFlags == DISPATCH_METHOD, "wFlags=%08x, expected DISPATCH_METHOD\n", wFlags);
    ok(!pdp->cNamedArgs, "pdp->cNamedArgs = %d\n", pdp->cNamedArgs);
    ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs = %p\n", pdp->rgdispidNamedArgs);

    switch(dispIdMember) {
    case DISPID_WINDOWCLOSING: {
        VARIANT *is_child = pdp->rgvarg+1, *cancel = pdp->rgvarg;

        CHECK_EXPECT(WindowClosing);

        ok(pdp->cArgs == 2, "pdp->cArgs = %d\n", pdp->cArgs);
        ok(V_VT(is_child) == VT_BOOL, "V_VT(is_child) = %d\n", V_VT(is_child));
        ok(!V_BOOL(is_child), "V_BOOL(is_child) = %x\n", V_BOOL(is_child));
        ok(V_VT(cancel) == (VT_BYREF|VT_BOOL), "V_VT(cancel) = %d\n", V_VT(cancel));
        ok(!*V_BOOLREF(cancel), "*V_BOOLREF(cancel) = %x\n", *V_BOOLREF(cancel));

        *V_BOOLREF(cancel) = VARIANT_TRUE;
        return S_OK;
    }
    default:
        ok(0, "unexpected id %d\n", dispIdMember);
    }

    return E_NOTIMPL;
}

static const IDispatchVtbl WBE2SinkVtbl = {
    WBE2Sink_QueryInterface,
    Dispatch_AddRef,
    Dispatch_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    Dispatch_GetIDsOfNames,
    WBE2Sink_Invoke
};

static IDispatch WBE2Sink = { &WBE2SinkVtbl };

static HRESULT WINAPI EnumConnections_QueryInterface(IEnumConnections *iface, REFIID riid, LPVOID *ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI EnumConnections_AddRef(IEnumConnections *iface)
{
    return 2;
}

static ULONG WINAPI EnumConnections_Release(IEnumConnections *iface)
{
    return 1;
}

static BOOL next_called;

static HRESULT WINAPI EnumConnections_Next(IEnumConnections *iface, ULONG cConnections, CONNECTDATA *rgcd, ULONG *pcFetched)
{
    CHECK_EXPECT2(EnumConnections_Next);

    ok(cConnections == 1, "cConnections = %d\n", cConnections);
    ok(pcFetched != NULL, "pcFetched == NULL\n");

    if(next_called) {
        *pcFetched = 0;
        return S_FALSE;
    }

    next_called = TRUE;
    rgcd->pUnk = (IUnknown*)&WBE2Sink;
    rgcd->dwCookie = 0xdeadbeef;
    *pcFetched = 1;
    return S_OK;
}

static HRESULT WINAPI EnumConnections_Skip(IEnumConnections *iface, ULONG ulConnections)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static HRESULT WINAPI EnumConnections_Reset(IEnumConnections *iface)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static HRESULT WINAPI EnumConnections_Clone(IEnumConnections *iface, IEnumConnections **ppEnum)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static const IEnumConnectionsVtbl EnumConnectionsVtbl = {
    EnumConnections_QueryInterface,
    EnumConnections_AddRef,
    EnumConnections_Release,
    EnumConnections_Next,
    EnumConnections_Skip,
    EnumConnections_Reset,
    EnumConnections_Clone
};

static IEnumConnections EnumConnections = { &EnumConnectionsVtbl };

static HRESULT WINAPI ConnectionPoint_QueryInterface(IConnectionPoint *iface, REFIID riid, LPVOID *ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ConnectionPoint_AddRef(IConnectionPoint *iface)
{
    return 2;
}

static ULONG WINAPI ConnectionPoint_Release(IConnectionPoint *iface)
{
    return 1;
}

static HRESULT WINAPI ConnectionPoint_GetConnectionInterface(IConnectionPoint *iface, IID *pIID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPoint_GetConnectionPointContainer(IConnectionPoint *iface,
        IConnectionPointContainer **ppCPC)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPoint_Advise(IConnectionPoint *iface, IUnknown *pUnkSink, DWORD *pdwCookie)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPoint_Unadvise(IConnectionPoint *iface, DWORD dwCookie)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPoint_EnumConnections(IConnectionPoint *iface, IEnumConnections **ppEnum)
{
    CHECK_EXPECT(EnumConnections);

    *ppEnum = &EnumConnections;
    next_called = FALSE;
    return S_OK;
}

static const IConnectionPointVtbl ConnectionPointVtbl =
{
    ConnectionPoint_QueryInterface,
    ConnectionPoint_AddRef,
    ConnectionPoint_Release,
    ConnectionPoint_GetConnectionInterface,
    ConnectionPoint_GetConnectionPointContainer,
    ConnectionPoint_Advise,
    ConnectionPoint_Unadvise,
    ConnectionPoint_EnumConnections
};

static IConnectionPoint ConnectionPointWBE2 = { &ConnectionPointVtbl };

static HRESULT WINAPI ConnectionPointContainer_QueryInterface(IConnectionPointContainer *iface,
                                                              REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ConnectionPointContainer_AddRef(IConnectionPointContainer *iface)
{
    return 2;
}

static ULONG WINAPI ConnectionPointContainer_Release(IConnectionPointContainer *iface)
{
    return 1;
}

static HRESULT WINAPI ConnectionPointContainer_EnumConnectionPoints(IConnectionPointContainer *iface,
        IEnumConnectionPoints **ppEnum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPointContainer_FindConnectionPoint(IConnectionPointContainer *iface,
        REFIID riid, IConnectionPoint **ppCP)
{
    CHECK_EXPECT(FindConnectionPoint);

    if(IsEqualGUID(riid, &DIID_DWebBrowserEvents2)) {
        *ppCP = &ConnectionPointWBE2;
        return S_OK;
    }

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOTIMPL;
}

static const IConnectionPointContainerVtbl ConnectionPointContainerVtbl = {
    ConnectionPointContainer_QueryInterface,
    ConnectionPointContainer_AddRef,
    ConnectionPointContainer_Release,
    ConnectionPointContainer_EnumConnectionPoints,
    ConnectionPointContainer_FindConnectionPoint
};

static IConnectionPointContainer ConnectionPointContainer = { &ConnectionPointContainerVtbl };

static void test_NavigateWithBindCtx(BSTR uri, VARIANT *flags, VARIANT *target_frame, VARIANT *post_data,
        VARIANT *headers, IBindCtx *bind_ctx, LPOLESTR url_fragment)
{
    ok(!strcmp_wa(uri, nav_url), "uri = %s\n", wine_dbgstr_w(uri));
    ok(V_VT(flags) == VT_I4, "V_VT(flags) = %d\n", V_VT(flags));
    ok(V_I4(flags) == navHyperlink, "V_I4(flags) = %x\n", V_I4(flags));
    ok(!target_frame, "target_frame != NULL\n");
    ok(!post_data, "post_data != NULL\n");
    ok(!headers, "headers != NULL\n");
    ok(bind_ctx != NULL, "bind_ctx == NULL\n");
    ok(!url_fragment, "url_dragment = %s\n", wine_dbgstr_w(url_fragment));
}

static HRESULT wb_qi(REFIID riid, void **ppv);

static HRESULT WINAPI WebBrowserPriv_QueryInterface(IWebBrowserPriv *iface, REFIID riid, void **ppv)
{
    return wb_qi(riid, ppv);
}

static ULONG WINAPI WebBrowserPriv_AddRef(IWebBrowserPriv *iface)
{
    return 2;
}

static ULONG WINAPI WebBrowserPriv_Release(IWebBrowserPriv *iface)
{
    return 1;
}

static HRESULT WINAPI WebBrowserPriv_NavigateWithBindCtx(IWebBrowserPriv *iface, VARIANT *uri, VARIANT *flags,
        VARIANT *target_frame, VARIANT *post_data, VARIANT *headers, IBindCtx *bind_ctx, LPOLESTR url_fragment)
{
    trace("NavigateWithBindCtx\n");

    CHECK_EXPECT(NavigateWithBindCtx);

    ok(V_VT(uri) == VT_BSTR, "V_VT(uri) = %d\n", V_VT(uri));
    test_NavigateWithBindCtx(V_BSTR(uri), flags, target_frame, post_data, headers, bind_ctx, url_fragment);
    return S_OK;
}

static HRESULT WINAPI WebBrowserPriv_OnClose(IWebBrowserPriv *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IWebBrowserPrivVtbl WebBrowserPrivVtbl = {
    WebBrowserPriv_QueryInterface,
    WebBrowserPriv_AddRef,
    WebBrowserPriv_Release,
    WebBrowserPriv_NavigateWithBindCtx,
    WebBrowserPriv_OnClose
};

static IWebBrowserPriv WebBrowserPriv = { &WebBrowserPrivVtbl };

static HRESULT WINAPI WebBrowserPriv2IE8_QueryInterface(IWebBrowserPriv2IE8 *iface, REFIID riid, void **ppv)
{
    return wb_qi(riid, ppv);
}

static ULONG WINAPI WebBrowserPriv2IE8_AddRef(IWebBrowserPriv2IE8 *iface)
{
    return 2;
}

static ULONG WINAPI WebBrowserPriv2IE8_Release(IWebBrowserPriv2IE8 *iface)
{
    return 1;
}

static HRESULT WINAPI WebBrowserPriv2IE8_NavigateWithBindCtx2(IWebBrowserPriv2IE8 *iface, IUri *uri, VARIANT *flags,
        VARIANT *target_frame, VARIANT *post_data, VARIANT *headers, IBindCtx *bind_ctx, LPOLESTR url_fragment)
{
    BSTR str;
    HRESULT hres;

    trace("IE8: NavigateWithBindCtx2\n");

    CHECK_EXPECT(NavigateWithBindCtx);

    hres = IUri_GetDisplayUri(uri, &str);
    ok(hres == S_OK, "GetDisplayUri failed: %08x\n", hres);
    test_NavigateWithBindCtx(str, flags, target_frame, post_data, headers, bind_ctx, url_fragment);
    SysFreeString(str);
    return S_OK;
}

static HRESULT WINAPI WebBrowserPriv2IE8_SetBrowserFrameOptions(IWebBrowserPriv2IE8 *iface, DWORD opt1, DWORD opt2)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowserPriv2IE8_DetachConnectionPoints(IWebBrowserPriv2IE8 *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowserPriv2IE8_GetProcessId(IWebBrowserPriv2IE8 *iface, DWORD *pid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowserPriv2IE8_CompatAttachEditEvents(IWebBrowserPriv2IE8 *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowserPriv2IE8_HandleOpenOptions(IWebBrowserPriv2IE8 *iface, IUnknown *obj, BSTR bstr, int options)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowserPriv2IE8_SetSearchTerm(IWebBrowserPriv2IE8 *iface, BSTR term)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowserPriv2IE8_GetSearchTerm(IWebBrowserPriv2IE8 *iface, BSTR *term)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowserPriv2IE8_GetCurrentDocument(IWebBrowserPriv2IE8 *iface, IDispatch **doc)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IWebBrowserPriv2IE8Vtbl WebBrowserPriv2IE8Vtbl = {
    WebBrowserPriv2IE8_QueryInterface,
    WebBrowserPriv2IE8_AddRef,
    WebBrowserPriv2IE8_Release,
    WebBrowserPriv2IE8_NavigateWithBindCtx2,
    WebBrowserPriv2IE8_SetBrowserFrameOptions,
    WebBrowserPriv2IE8_DetachConnectionPoints,
    WebBrowserPriv2IE8_GetProcessId,
    WebBrowserPriv2IE8_CompatAttachEditEvents,
    WebBrowserPriv2IE8_HandleOpenOptions,
    WebBrowserPriv2IE8_SetSearchTerm,
    WebBrowserPriv2IE8_GetSearchTerm,
    WebBrowserPriv2IE8_GetCurrentDocument
};

static IWebBrowserPriv2IE8 WebBrowserPriv2IE8 = { &WebBrowserPriv2IE8Vtbl };

static HRESULT WINAPI WebBrowserPriv2IE9_QueryInterface(IWebBrowserPriv2IE9 *iface, REFIID riid, void **ppv)
{
    return wb_qi(riid, ppv);
}

static ULONG WINAPI WebBrowserPriv2IE9_AddRef(IWebBrowserPriv2IE9 *iface)
{
    return 2;
}

static ULONG WINAPI WebBrowserPriv2IE9_Release(IWebBrowserPriv2IE9 *iface)
{
    return 1;
}

static HRESULT WINAPI WebBrowserPriv2IE9_NavigateWithBindCtx2(IWebBrowserPriv2IE9 *iface, IUri *uri, VARIANT *flags,
        VARIANT *target_frame, VARIANT *post_data, VARIANT *headers, IBindCtx *bind_ctx, LPOLESTR url_fragment, DWORD unknown)
{
    BSTR str;
    HRESULT hres;

    trace("IE9: NavigateWithBindCtx2\n");

    CHECK_EXPECT(NavigateWithBindCtx);

    hres = IUri_GetDisplayUri(uri, &str);
    ok(hres == S_OK, "GetDisplayUri failed: %08x\n", hres);
    test_NavigateWithBindCtx(str, flags, target_frame, post_data, headers, bind_ctx, url_fragment);
    SysFreeString(str);
    return S_OK;
}

static const IWebBrowserPriv2IE9Vtbl WebBrowserPriv2IE9Vtbl = {
    WebBrowserPriv2IE9_QueryInterface,
    WebBrowserPriv2IE9_AddRef,
    WebBrowserPriv2IE9_Release,
    WebBrowserPriv2IE9_NavigateWithBindCtx2
};

static IWebBrowserPriv2IE9 WebBrowserPriv2IE9 = { &WebBrowserPriv2IE9Vtbl };

static HRESULT WINAPI WebBrowser_QueryInterface(IWebBrowser2 *iface, REFIID riid, void **ppv)
{
    return wb_qi(riid, ppv);
}

static ULONG WINAPI WebBrowser_AddRef(IWebBrowser2 *iface)
{
    return 2;
}

static ULONG WINAPI WebBrowser_Release(IWebBrowser2 *iface)
{
    return 1;
}

static HRESULT WINAPI WebBrowser_GetTypeInfoCount(IWebBrowser2 *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_GetTypeInfo(IWebBrowser2 *iface, UINT iTInfo, LCID lcid,
        LPTYPEINFO *ppTInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_GetIDsOfNames(IWebBrowser2 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_Invoke(IWebBrowser2 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExepInfo, UINT *puArgErr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_GoBack(IWebBrowser2 *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_GoForward(IWebBrowser2 *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_GoHome(IWebBrowser2 *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_GoSearch(IWebBrowser2 *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_Navigate(IWebBrowser2 *iface, BSTR szUrl,
        VARIANT *Flags, VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_Refresh(IWebBrowser2 *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_Refresh2(IWebBrowser2 *iface, VARIANT *Level)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_Stop(IWebBrowser2 *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Application(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Parent(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Container(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Document(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_TopLevelContainer(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Type(IWebBrowser2 *iface, BSTR *Type)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Left(IWebBrowser2 *iface, LONG *pl)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_Left(IWebBrowser2 *iface, LONG Left)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Top(IWebBrowser2 *iface, LONG *pl)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_Top(IWebBrowser2 *iface, LONG Top)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Width(IWebBrowser2 *iface, LONG *pl)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_Width(IWebBrowser2 *iface, LONG Width)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Height(IWebBrowser2 *iface, LONG *pl)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_Height(IWebBrowser2 *iface, LONG Height)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_LocationName(IWebBrowser2 *iface, BSTR *LocationName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_LocationURL(IWebBrowser2 *iface, BSTR *LocationURL)
{
    CHECK_EXPECT(get_LocationURL);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Busy(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_Quit(IWebBrowser2 *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_ClientToWindow(IWebBrowser2 *iface, int *pcx, int *pcy)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_PutProperty(IWebBrowser2 *iface, BSTR szProperty, VARIANT vtValue)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_GetProperty(IWebBrowser2 *iface, BSTR szProperty, VARIANT *pvtValue)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Name(IWebBrowser2 *iface, BSTR *Name)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_HWND(IWebBrowser2 *iface, SHANDLE_PTR *pHWND)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_FullName(IWebBrowser2 *iface, BSTR *FullName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Path(IWebBrowser2 *iface, BSTR *Path)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Visible(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_Visible(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_StatusBar(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_StatusBar(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_StatusText(IWebBrowser2 *iface, BSTR *StatusText)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_StatusText(IWebBrowser2 *iface, BSTR StatusText)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_ToolBar(IWebBrowser2 *iface, int *Value)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_ToolBar(IWebBrowser2 *iface, int Value)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_MenuBar(IWebBrowser2 *iface, VARIANT_BOOL *Value)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_MenuBar(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_FullScreen(IWebBrowser2 *iface, VARIANT_BOOL *pbFullScreen)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_FullScreen(IWebBrowser2 *iface, VARIANT_BOOL bFullScreen)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_Navigate2(IWebBrowser2 *iface, VARIANT *URL, VARIANT *Flags,
        VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_QueryStatusWB(IWebBrowser2 *iface, OLECMDID cmdID, OLECMDF *pcmdf)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_ExecWB(IWebBrowser2 *iface, OLECMDID cmdID,
        OLECMDEXECOPT cmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_ShowBrowserBar(IWebBrowser2 *iface, VARIANT *pvaClsid,
        VARIANT *pvarShow, VARIANT *pvarSize)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_ReadyState(IWebBrowser2 *iface, READYSTATE *lpReadyState)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Offline(IWebBrowser2 *iface, VARIANT_BOOL *pbOffline)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_Offline(IWebBrowser2 *iface, VARIANT_BOOL bOffline)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Silent(IWebBrowser2 *iface, VARIANT_BOOL *pbSilent)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_Silent(IWebBrowser2 *iface, VARIANT_BOOL bSilent)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_RegisterAsBrowser(IWebBrowser2 *iface,
        VARIANT_BOOL *pbRegister)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_RegisterAsBrowser(IWebBrowser2 *iface,
        VARIANT_BOOL bRegister)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_RegisterAsDropTarget(IWebBrowser2 *iface,
        VARIANT_BOOL *pbRegister)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_RegisterAsDropTarget(IWebBrowser2 *iface,
        VARIANT_BOOL bRegister)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_TheaterMode(IWebBrowser2 *iface, VARIANT_BOOL *pbRegister)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_TheaterMode(IWebBrowser2 *iface, VARIANT_BOOL bRegister)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_AddressBar(IWebBrowser2 *iface, VARIANT_BOOL *Value)
{
    trace("get_AddressBar: ignoring\n"); /* Some old IEs call it */
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_AddressBar(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Resizable(IWebBrowser2 *iface, VARIANT_BOOL *Value)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_Resizable(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IWebBrowser2Vtbl WebBrowser2Vtbl =
{
    WebBrowser_QueryInterface,
    WebBrowser_AddRef,
    WebBrowser_Release,
    WebBrowser_GetTypeInfoCount,
    WebBrowser_GetTypeInfo,
    WebBrowser_GetIDsOfNames,
    WebBrowser_Invoke,
    WebBrowser_GoBack,
    WebBrowser_GoForward,
    WebBrowser_GoHome,
    WebBrowser_GoSearch,
    WebBrowser_Navigate,
    WebBrowser_Refresh,
    WebBrowser_Refresh2,
    WebBrowser_Stop,
    WebBrowser_get_Application,
    WebBrowser_get_Parent,
    WebBrowser_get_Container,
    WebBrowser_get_Document,
    WebBrowser_get_TopLevelContainer,
    WebBrowser_get_Type,
    WebBrowser_get_Left,
    WebBrowser_put_Left,
    WebBrowser_get_Top,
    WebBrowser_put_Top,
    WebBrowser_get_Width,
    WebBrowser_put_Width,
    WebBrowser_get_Height,
    WebBrowser_put_Height,
    WebBrowser_get_LocationName,
    WebBrowser_get_LocationURL,
    WebBrowser_get_Busy,
    WebBrowser_Quit,
    WebBrowser_ClientToWindow,
    WebBrowser_PutProperty,
    WebBrowser_GetProperty,
    WebBrowser_get_Name,
    WebBrowser_get_HWND,
    WebBrowser_get_FullName,
    WebBrowser_get_Path,
    WebBrowser_get_Visible,
    WebBrowser_put_Visible,
    WebBrowser_get_StatusBar,
    WebBrowser_put_StatusBar,
    WebBrowser_get_StatusText,
    WebBrowser_put_StatusText,
    WebBrowser_get_ToolBar,
    WebBrowser_put_ToolBar,
    WebBrowser_get_MenuBar,
    WebBrowser_put_MenuBar,
    WebBrowser_get_FullScreen,
    WebBrowser_put_FullScreen,
    WebBrowser_Navigate2,
    WebBrowser_QueryStatusWB,
    WebBrowser_ExecWB,
    WebBrowser_ShowBrowserBar,
    WebBrowser_get_ReadyState,
    WebBrowser_get_Offline,
    WebBrowser_put_Offline,
    WebBrowser_get_Silent,
    WebBrowser_put_Silent,
    WebBrowser_get_RegisterAsBrowser,
    WebBrowser_put_RegisterAsBrowser,
    WebBrowser_get_RegisterAsDropTarget,
    WebBrowser_put_RegisterAsDropTarget,
    WebBrowser_get_TheaterMode,
    WebBrowser_put_TheaterMode,
    WebBrowser_get_AddressBar,
    WebBrowser_put_AddressBar,
    WebBrowser_get_Resizable,
    WebBrowser_put_Resizable
};

static IWebBrowser2 WebBrowser2 = { &WebBrowser2Vtbl };

static HRESULT wb_qi(REFIID riid, void **ppv)
{
    static const IID IID_IWebBrowserPriv2IE7 = {0x1af32b6c, 0xa3ba,0x48b9,{0xb2,0x4e,0x8a,0xa9,0xc4,0x1f,0x6e,0xcd}};
    static const IID IID_IWebBrowserPriv2IE8XP = {0x486f6159,0x9f3f,0x4827,{0x82,0xd4,0x28,0x3c,0xef,0x39,0x77,0x33}};

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IWebBrowser, riid)
            || IsEqualGUID(&IID_IWebBrowserApp, riid) || IsEqualGUID(&IID_IWebBrowser2, riid)) {
        *ppv = &WebBrowser2;
        return S_OK;
    }

    if(IsEqualGUID(riid, &IID_IOleObject))
        return E_NOINTERFACE; /* TODO */

    if(IsEqualGUID(riid, &IID_IConnectionPointContainer)) {
        *ppv = &ConnectionPointContainer;
        return S_OK;
    }

    if(IsEqualGUID(riid, &IID_IWebBrowserPriv)) {
        *ppv = &WebBrowserPriv;
        return S_OK;
    }

    if(IsEqualGUID(riid, &IID_IWebBrowserPriv2IE8)) {
        /* IE8 and IE9 versions use the same IID, but have different declarations. */
        *ppv = is_ie9plus ? (void*)&WebBrowserPriv2IE9 : (void*)&WebBrowserPriv2IE8;
        return S_OK;
    }

    if(IsEqualGUID(riid, &IID_IWebBrowserPriv2IE7)) {
        trace("QI(IID_IWebBrowserPriv2IE7)\n");
        return E_NOINTERFACE;
    }

    if(IsEqualGUID(riid, &IID_IWebBrowserPriv2IE8XP)) {
        trace("QI(IID_IWebBrowserPriv2IE8XP)\n");
        return E_NOINTERFACE;
    }

    ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

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

    if(IsEqualGUID(&IID_IShellBrowser, guidService)) {
        ok(IsEqualGUID(&IID_IBrowserService, riid), "unexpected riid\n");
        *ppv = &BrowserService;
        return S_OK;
    }

    if(support_wbapp && IsEqualGUID(&IID_IWebBrowserApp, guidService)) {
        if(IsEqualGUID(riid, &IID_IWebBrowser2)) {
            *ppv = &WebBrowser2;
            return S_OK;
        }
        ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
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

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
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
DEFINE_GUID(IID_unk5, 0x5f95accc,0xd7a1,0x4574,0xbc,0xcb,0x69,0x71,0x35,0xbc,0x41,0xde);

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
    else if(IsEqualGUID(&IID_unk5, riid))
        return E_NOINTERFACE; /* IE10 */
    else if(IsEqualGUID(&IID_IDocHostUIHandlerPriv, riid))
        return E_NOINTERFACE; /* ? */
    else
        trace("QI(%s)\n", wine_dbgstr_guid(riid));

    if(*ppv)
        return S_OK;
    return E_NOINTERFACE;
}

static LRESULT WINAPI wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if(msg == WM_CONTINUE_BINDING) {
        IBindStatusCallback *callback = (IBindStatusCallback*)wParam;
        continue_binding(callback);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
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
        if(!nav_url && !editmode)
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

    if(open_call || resetting_document)
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
    DWORD cookie;

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
        hres = IConnectionPoint_Advise(cp, (IUnknown*)&PropertyNotifySink, &cookie);
        ok(hres == S_OK, "Advise failed: %08x\n", hres);
        hres = IConnectionPoint_Unadvise(cp, cookie);
        ok(hres == S_OK, "Unadvise failed: %08x\n", hres);
        hres = IConnectionPoint_Advise(cp, (IUnknown*)&PropertyNotifySink, NULL);
        ok(hres == S_OK, "Advise failed: %08x\n", hres);
    } else if(IsEqualGUID(&IID_IDispatch, riid)) {
        IEnumConnections *enum_conn;
        CONNECTDATA conn_data;
        ULONG fetched;

        hres = IConnectionPoint_Advise(cp, (IUnknown*)&EventDispatch, &cookie);
        ok(hres == S_OK, "Advise failed: %08x\n", hres);

        hres = IConnectionPoint_EnumConnections(cp, &enum_conn);
        ok(hres == S_OK, "EnumConnections failed: %08x\n", hres);

        fetched = 0;
        hres = IEnumConnections_Next(enum_conn, 1, &conn_data, &fetched);
        ok(hres == S_OK, "Next failed: %08x\n", hres);
        ok(conn_data.pUnk == (IUnknown*)&EventDispatch, "conn_data.pUnk == EventDispatch\n");
        ok(conn_data.dwCookie == cookie, "conn_data.dwCookie != cookie\n");
        IUnknown_Release(conn_data.pUnk);

        fetched = 0xdeadbeef;
        hres = IEnumConnections_Next(enum_conn, 1, &conn_data, &fetched);
        ok(hres == S_FALSE, "Next failed: %08x\n", hres);
        ok(!fetched, "fetched = %d\n", fetched);

        IEnumConnections_Release(enum_conn);
    }

    IConnectionPoint_Release(cp);
}

static void test_ConnectionPointContainer(IHTMLDocument2 *doc)
{
    IConnectionPointContainer *container;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IConnectionPointContainer, (void**)&container);
    ok(hres == S_OK, "QueryInterface(IID_IConnectionPointContainer) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    test_ConnectionPoint(container, &IID_IDispatch);
    test_ConnectionPoint(container, &IID_IPropertyNotifySink);
    test_ConnectionPoint(container, &DIID_HTMLDocumentEvents);
    test_ConnectionPoint(container, &DIID_HTMLDocumentEvents2);

    IConnectionPointContainer_Release(container);
}

static void set_custom_uihandler(IHTMLDocument2 *doc, IDocHostUIHandler2 *uihandler)
{
    ICustomDoc *custom_doc;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_ICustomDoc, (void**)&custom_doc);
    ok(hres == S_OK, "Could not get ICustomDoc iface: %08x\n", hres);

    expect_uihandler_iface = uihandler;

    hres = ICustomDoc_SetUIHandler(custom_doc, (IDocHostUIHandler*)uihandler);
    ICustomDoc_Release(custom_doc);
    ok(hres == S_OK, "SetUIHandler failed: %08x\n", hres);
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
    SET_EXPECT(Invoke_OnReadyStateChange_Loading);
    SET_EXPECT(IsSystemMoniker);
    if(mon == &Moniker)
        SET_EXPECT(BindToStorage);
    SET_EXPECT(SetActiveObject);
    if(set_clientsite) {
        SET_EXPECT(Invoke_AMBIENT_SILENT);
        SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        SET_EXPECT(Exec_ShellDocView_37);
        SET_EXPECT(IsErrorUrl);
    }else {
        SET_EXPECT(GetTravelLog);
    }
    SET_EXPECT(Exec_ShellDocView_84);
    SET_EXPECT(GetPendingUrl);
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
        CLEAR_CALLED(Invoke_AMBIENT_PALETTE); /* not called on IE9 */
        CLEAR_CALLED(GetOptionKeyPath); /* not called on some IE9 */
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
    CHECK_CALLED(Invoke_OnReadyStateChange_Loading);
    CLEAR_CALLED(IsSystemMoniker); /* IE7 */
    if(mon == &Moniker)
        CHECK_CALLED(BindToStorage);
    CLEAR_CALLED(SetActiveObject); /* FIXME */
    if(set_clientsite) {
        CHECK_CALLED(Invoke_AMBIENT_SILENT);
        CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        CHECK_CALLED(Exec_ShellDocView_37);
        todo_wine CHECK_CALLED_BROKEN(IsErrorUrl);
    }else {
        CHECK_CALLED(GetTravelLog);
    }
    CHECK_CALLED_BROKEN(Exec_ShellDocView_84);
    todo_wine CHECK_CALLED(GetPendingUrl);

    set_clientsite = container_locked = TRUE;

    test_GetCurMoniker((IUnknown*)persist, mon, NULL, FALSE);

    IBindCtx_Release(bind);

    test_readyState((IUnknown*)persist);
}

#define DWL_VERBDONE           0x0001
#define DWL_CSS                0x0002
#define DWL_TRYCSS             0x0004
#define DWL_HTTP               0x0008
#define DWL_EMPTY              0x0010
#define DWL_JAVASCRIPT         0x0020
#define DWL_ONREADY_LOADING    0x0040
#define DWL_EXPECT_HISTUPDATE  0x0080
#define DWL_FROM_HISTORY       0x0100
#define DWL_REFRESH            0x0200
#define DWL_EX_GETHOSTINFO     0x0400
#define DWL_EXTERNAL           0x0800

static void test_download(DWORD flags)
{
    const BOOL is_extern = (flags & DWL_EXTERNAL) != 0;
    const BOOL is_js = (flags & DWL_JAVASCRIPT) != 0;
    HWND hwnd;
    BOOL *b;
    MSG msg;

    if(is_js)
        b = &called_Exec_SETDOWNLOADSTATE_0;
    else if(is_extern)
        b = &called_NavigateWithBindCtx;
    else
        b = &called_Exec_HTTPEQUIV_DONE;
    is_refresh = (flags & DWL_REFRESH) != 0;

    hwnd = FindWindowA("Internet Explorer_Hidden", NULL);
    ok(hwnd != NULL, "Could not find hidden window\n");

    test_readyState(NULL);

    if(flags & DWL_REFRESH) {
        SET_EXPECT(Invoke_AMBIENT_SILENT);
        SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    }
    if(flags & (DWL_VERBDONE|DWL_HTTP))
        SET_EXPECT(Exec_SETPROGRESSMAX);
    if(flags & DWL_EX_GETHOSTINFO)
        SET_EXPECT(GetHostInfo);
    SET_EXPECT(SetStatusText);
    if(!(flags & DWL_EMPTY))
        SET_EXPECT(Exec_SETDOWNLOADSTATE_1);
    if(is_js)
        SET_EXPECT(GetExternal);
    SET_EXPECT(OnViewChange);
    SET_EXPECT(GetDropTarget);
    if((flags & DWL_TRYCSS) && !(flags & DWL_EMPTY))
        SET_EXPECT(Exec_ShellDocView_84);
    if(flags & DWL_CSS) {
        SET_EXPECT(CreateInstance);
        SET_EXPECT(Start);
        SET_EXPECT(LockRequest);
        SET_EXPECT(Terminate);
        SET_EXPECT(Protocol_Read);
        SET_EXPECT(UnlockRequest);
    }
    if(flags & DWL_ONREADY_LOADING)
        SET_EXPECT(Invoke_OnReadyStateChange_Loading);
    if(!(flags & (DWL_EMPTY|DWL_JAVASCRIPT)))
        SET_EXPECT(Invoke_OnReadyStateChange_Interactive);
    if(!is_js && !is_extern)
        SET_EXPECT(Invoke_OnReadyStateChange_Complete);
    SET_EXPECT(Exec_Explorer_69);
    SET_EXPECT(EnableModeless_TRUE); /* IE7 */
    SET_EXPECT(Frame_EnableModeless_TRUE); /* IE7 */
    SET_EXPECT(EnableModeless_FALSE); /* IE7 */
    SET_EXPECT(Frame_EnableModeless_FALSE); /* IE7 */
    if((nav_url && !is_js && !is_extern) || (flags & (DWL_CSS|DWL_HTTP)))
        SET_EXPECT(Exec_ShellDocView_37);
    if(flags & DWL_HTTP) {
        if(!(flags & DWL_FROM_HISTORY))
            SET_EXPECT(OnChanged_1012);
        SET_EXPECT(Exec_HTTPEQUIV);
        SET_EXPECT(Exec_SETTITLE);
    }
    if(!is_js && !is_extern)
        SET_EXPECT(OnChanged_1005);
    SET_EXPECT(OnChanged_READYSTATE);
    SET_EXPECT(Exec_SETPROGRESSPOS);
    if(!(flags & DWL_EMPTY))
        SET_EXPECT(Exec_SETDOWNLOADSTATE_0);
    SET_EXPECT(Exec_ShellDocView_103);
    SET_EXPECT(Exec_ShellDocView_105);
    SET_EXPECT(Exec_ShellDocView_140);
    if(!is_js && !is_extern) {
        SET_EXPECT(Exec_MSHTML_PARSECOMPLETE);
        if(support_wbapp) /* Called on some Vista installations */
            SET_EXPECT(CountEntries);
        SET_EXPECT(Exec_HTTPEQUIV_DONE);
    }
    SET_EXPECT(SetStatusText);
    if(nav_url || support_wbapp) {
        SET_EXPECT(UpdateUI);
        SET_EXPECT(Exec_UPDATECOMMANDS);
        SET_EXPECT(Exec_SETTITLE);
        if(flags & DWL_EXPECT_HISTUPDATE)
            SET_EXPECT(Exec_Explorer_38);
        SET_EXPECT(UpdateBackForwardState);
    }
    if(!is_js && !is_extern) {
        if(!editmode && !(flags & DWL_REFRESH)) {
            if(!(flags & DWL_EMPTY))
                SET_EXPECT(FireNavigateComplete2);
            SET_EXPECT(FireDocumentComplete);
        }
        SET_EXPECT(ActiveElementChanged);
    }
    SET_EXPECT(IsErrorUrl);
    if(is_extern) {
        SET_EXPECT(Exec_ShellDocView_62);
        SET_EXPECT(Exec_DOCCANNAVIGATE_NULL);
        SET_EXPECT(NavigateWithBindCtx);
        SET_EXPECT(Exec_Explorer_38); /* todo_wine */
    }
    if(editmode || is_refresh)
        SET_EXPECT(Exec_ShellDocView_138);
    expect_status_text = (LPWSTR)0xdeadbeef; /* TODO */

    while(!*b && GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    if(flags & DWL_REFRESH) {
        CHECK_CALLED(Invoke_AMBIENT_SILENT);
        CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    }
    if(flags & DWL_VERBDONE)
        CHECK_CALLED(Exec_SETPROGRESSMAX);
    if(flags & DWL_HTTP)
        SET_CALLED(Exec_SETPROGRESSMAX);
    if(flags &  DWL_EX_GETHOSTINFO) {
        if(nav_url)
            todo_wine CHECK_CALLED(GetHostInfo);
        else
            CHECK_CALLED(GetHostInfo);
    }
    CHECK_CALLED(SetStatusText);
    if(!(flags & DWL_EMPTY))
        CHECK_CALLED(Exec_SETDOWNLOADSTATE_1);
    if(is_js)
        CHECK_CALLED(GetExternal);
    CHECK_CALLED(OnViewChange);
    CLEAR_CALLED(GetDropTarget);
    if((flags & DWL_TRYCSS) && !(flags & DWL_EMPTY))
        todo_wine CHECK_CALLED_BROKEN(Exec_ShellDocView_84);
    if(flags & DWL_CSS) {
        CHECK_CALLED(CreateInstance);
        CHECK_CALLED(Start);
        CHECK_CALLED(LockRequest);
        CHECK_CALLED(Terminate);
        CHECK_CALLED(Protocol_Read);
        CHECK_CALLED(UnlockRequest);
    }
    if(flags & DWL_ONREADY_LOADING)
        CHECK_CALLED(Invoke_OnReadyStateChange_Loading);
    if(!(flags & (DWL_EMPTY|DWL_JAVASCRIPT))) {
        if(!is_extern)
            CHECK_CALLED(Invoke_OnReadyStateChange_Interactive);
        else
            todo_wine CHECK_CALLED(Invoke_OnReadyStateChange_Interactive);
    }
    if(!is_js && !is_extern)
        CHECK_CALLED(Invoke_OnReadyStateChange_Complete);
    SET_CALLED(Exec_Explorer_69);
    SET_CALLED(EnableModeless_TRUE); /* IE7 */
    SET_CALLED(Frame_EnableModeless_TRUE); /* IE7 */
    SET_CALLED(EnableModeless_FALSE); /* IE7 */
    SET_CALLED(Frame_EnableModeless_FALSE); /* IE7 */
    if(nav_url && !is_js && !is_extern && !(flags & DWL_REFRESH))
        todo_wine CHECK_CALLED(Exec_ShellDocView_37);
    else if(flags & (DWL_CSS|DWL_HTTP))
        CLEAR_CALLED(Exec_ShellDocView_37); /* Called by IE9 */
    if(flags & DWL_HTTP)  {
        if(!(flags & DWL_FROM_HISTORY))
            todo_wine CHECK_CALLED(OnChanged_1012);
        todo_wine CHECK_CALLED(Exec_HTTPEQUIV);
        if(!(flags & DWL_REFRESH))
            todo_wine CHECK_CALLED(Exec_SETTITLE);
        else
            CHECK_CALLED(Exec_SETTITLE);
    }
    if(!is_js) {
        if(!is_extern)
            CHECK_CALLED(OnChanged_1005);
        CHECK_CALLED(OnChanged_READYSTATE);
        CHECK_CALLED(Exec_SETPROGRESSPOS);
    }else {
        CLEAR_CALLED(OnChanged_READYSTATE); /* sometimes called */
        todo_wine CHECK_CALLED(Exec_SETPROGRESSPOS);
    }
    if(!(flags & DWL_EMPTY)) {
        if(!is_extern)
            CHECK_CALLED(Exec_SETDOWNLOADSTATE_0);
        else
            todo_wine CHECK_CALLED(Exec_SETDOWNLOADSTATE_0);
    }
    CLEAR_CALLED(Exec_ShellDocView_103);
    CLEAR_CALLED(Exec_ShellDocView_105);
    CLEAR_CALLED(Exec_ShellDocView_140);
    if(!is_js && !is_extern) {
        CHECK_CALLED(Exec_MSHTML_PARSECOMPLETE);
        if(support_wbapp) /* Called on some Vista installations */
            CLEAR_CALLED(CountEntries);
        CHECK_CALLED(Exec_HTTPEQUIV_DONE);
    }
    SET_CALLED(SetStatusText);
    if(nav_url || support_wbapp) { /* avoiding race, FIXME: find better way */
        CLEAR_CALLED(UpdateUI);
        CLEAR_CALLED(Exec_UPDATECOMMANDS);
        CLEAR_CALLED(Exec_SETTITLE);
        if(flags & DWL_EXPECT_HISTUPDATE) {
            if(flags & DWL_FROM_HISTORY)
                CHECK_CALLED_BROKEN(Exec_Explorer_38); /* Some old IEs don't call it. */
            else
                CHECK_CALLED(Exec_Explorer_38);
        }
        todo_wine CHECK_CALLED_BROKEN(UpdateBackForwardState);
    }
    if(!is_js && !is_extern) {
        if(!editmode && !(flags & DWL_REFRESH)) {
            if(!(flags & DWL_EMPTY)) {
                if(support_wbapp)
                    CHECK_CALLED(FireNavigateComplete2);
                else
                    todo_wine CHECK_CALLED(FireNavigateComplete2);
            }
            CHECK_CALLED(FireDocumentComplete);
        }
        todo_wine CHECK_CALLED(ActiveElementChanged);
    }
    todo_wine CHECK_CALLED_BROKEN(IsErrorUrl);
    if(is_extern) {
        CHECK_CALLED(Exec_ShellDocView_62);
        CHECK_CALLED(Exec_DOCCANNAVIGATE_NULL);
        CHECK_CALLED(NavigateWithBindCtx);
        todo_wine CHECK_NOT_CALLED(Exec_Explorer_38);
    }
    if(editmode || is_refresh)
        CLEAR_CALLED(Exec_ShellDocView_138); /* IE11 */

    if(!is_extern)
        load_state = LD_COMPLETE;

    test_readyState(NULL);
}

static void test_Persist(IHTMLDocument2 *doc, IMoniker *mon)
{
    IPersistMoniker *persist_mon;
    IPersistFile *persist_file;
    GUID guid;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IPersistFile, (void**)&persist_file);
    ok(hres == S_OK, "QueryInterface(IID_IPersist) failed: %08x\n", hres);
    if(SUCCEEDED(hres)) {
        hres = IPersistFile_GetClassID(persist_file, NULL);
        ok(hres == E_INVALIDARG, "GetClassID returned: %08x, expected E_INVALIDARG\n", hres);

        hres = IPersistFile_GetClassID(persist_file, &guid);
        ok(hres == S_OK, "GetClassID failed: %08x\n", hres);
        ok(IsEqualGUID(&CLSID_HTMLDocument, &guid), "guid != CLSID_HTMLDocument\n");

        IPersistFile_Release(persist_file);
    }

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IPersistMoniker, (void**)&persist_mon);
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

static void test_put_href(IHTMLDocument2 *doc, BOOL use_replace, const char *href, const char *new_nav_url, BOOL is_js,
        BOOL is_hash, DWORD dwl_flags)
{
    const char *prev_nav_url = NULL;
    IHTMLPrivateWindow *priv_window;
    IHTMLLocation *location;
    IHTMLWindow2 *window;
    BSTR str, str2;
    HRESULT hres;

    trace("put_href %s...\n", new_nav_url);

    loading_js = is_js;
    loading_hash = is_hash;

    location = NULL;
    hres = IHTMLDocument2_get_location(doc, &location);
    ok(hres == S_OK, "get_location failed: %08x\n", hres);
    ok(location != NULL, "location == NULL\n");

    prev_url = prev_nav_url = nav_url;
    nav_url = new_nav_url;
    if(!loading_hash)
        nav_serv_url = new_nav_url;
    if(!href)
        href = new_nav_url;

    str = a2bstr(href);
    SET_EXPECT(TranslateUrl);
    if(support_wbapp) {
        SET_EXPECT(FireBeforeNavigate2);
        SET_EXPECT(Exec_ShellDocView_67);
        if(!is_hash) {
            SET_EXPECT(Invoke_AMBIENT_SILENT);
            SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
            SET_EXPECT(Exec_ShellDocView_63);
            SET_EXPECT(Exec_ShellDocView_84);
        }else {
            SET_EXPECT(FireNavigateComplete2);
            SET_EXPECT(FireDocumentComplete);
        }
    }else {
        SET_EXPECT(Navigate);
    }
    if(use_replace) {
        hres = IHTMLLocation_replace(location, str);
        ok(hres == S_OK, "put_href failed: %08x\n", hres);
    }else {
        hres = IHTMLLocation_put_href(location, str);
        if(is_js && hres == E_ACCESSDENIED)
            win_skip("put_href: got E_ACCESSDENIED\n");
        else
            ok(hres == S_OK, "put_href failed: %08x\n", hres);
    }
    SysFreeString(str);
    if(hres == S_OK) {
        CHECK_CALLED(TranslateUrl);
        if(support_wbapp) {
            CHECK_CALLED(FireBeforeNavigate2);
            CLEAR_CALLED(Exec_ShellDocView_67); /* Not called by IE11 */
            if(!is_hash) {
                CHECK_CALLED(Invoke_AMBIENT_SILENT);
                CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
                CHECK_CALLED(Exec_ShellDocView_63);
                CHECK_CALLED_BROKEN(Exec_ShellDocView_84);
            }else {
                CHECK_CALLED(FireNavigateComplete2);
                CHECK_CALLED(FireDocumentComplete);
            }
        }else {
            CHECK_CALLED(Navigate);
        }
    }else {
        CLEAR_CALLED(TranslateUrl);
        if(support_wbapp) {
            CLEAR_CALLED(FireBeforeNavigate2);
            SET_CALLED(Exec_ShellDocView_67);
            if(!is_hash) {
                CLEAR_CALLED(Invoke_AMBIENT_SILENT);
                CLEAR_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
                CLEAR_CALLED(Exec_ShellDocView_63);
                todo_wine CLEAR_CALLED(Exec_ShellDocView_84);
            }else {
                CLEAR_CALLED(FireNavigateComplete2);
                CLEAR_CALLED(FireDocumentComplete);
            }
        }else {
            CLEAR_CALLED(Navigate);
        }
    }

    IHTMLLocation_Release(location);

    if(is_js && hres == E_ACCESSDENIED) {
        nav_url = prev_nav_url;
        return;
    }

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok(hres == S_OK, "get_parentWindow failed: %08x\n", hres);

    hres = IHTMLWindow2_QueryInterface(window, &IID_IHTMLPrivateWindow, (void**)&priv_window);
    IHTMLWindow2_Release(window);
    ok(hres == S_OK, "QueryInterface(IID_IHTMLPrivateWindow) failed: %08x\n", hres);

    if(!support_wbapp) {
        VARIANT vempty;

        readystate_set_loading = TRUE;
        SET_EXPECT(TranslateUrl);
        SET_EXPECT(Exec_ShellDocView_67);
        SET_EXPECT(Invoke_AMBIENT_SILENT);
        SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        SET_EXPECT(Exec_ShellDocView_63);
        SET_EXPECT(Exec_ShellDocView_84);

        str = a2bstr(nav_url);
        str2 = a2bstr("");
        V_VT(&vempty) = VT_EMPTY;
        hres = IHTMLPrivateWindow_SuperNavigate(priv_window, str, str2, NULL, NULL, &vempty, &vempty, 0);
        SysFreeString(str);
        SysFreeString(str2);
        ok(hres == S_OK, "SuperNavigate failed: %08x\n", hres);

        CHECK_CALLED(TranslateUrl);
        CLEAR_CALLED(Exec_ShellDocView_67); /* Not called by IE11 */
        CHECK_CALLED(Invoke_AMBIENT_SILENT);
        CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        CHECK_CALLED(Exec_ShellDocView_63);
        CHECK_CALLED_BROKEN(Exec_ShellDocView_84);
    }

    if(doc_mon) {
        test_GetCurMoniker(doc_unk, doc_mon, NULL, FALSE);
        doc_mon = NULL;
    }

    if(!is_hash) {
        hres = IHTMLPrivateWindow_GetAddressBarUrl(priv_window, &str2);
        ok(hres == S_OK, "GetAddressBarUrl failed: %08x\n", hres);
        ok(!strcmp_wa(str2, prev_nav_url), "unexpected address bar url:  %s, expected %s\n", wine_dbgstr_w(str2), prev_nav_url);
        SysFreeString(str2);

        if(is_js) {
            ignore_external_qi = TRUE;
            dwl_flags |= DWL_JAVASCRIPT;
        }else {
            if(!(dwl_flags & DWL_EXTERNAL))
                dwl_flags |= DWL_EX_GETHOSTINFO;
            dwl_flags |= DWL_ONREADY_LOADING;
        }
        test_download(DWL_VERBDONE | dwl_flags);
        if(is_js)
            ignore_external_qi = FALSE;

    }

    hres = IHTMLPrivateWindow_GetAddressBarUrl(priv_window, &str2);
    ok(hres == S_OK, "GetAddressBarUrl failed: %08x\n", hres);
    if(is_js)
        ok(!strcmp_wa(str2, prev_nav_url), "unexpected address bar url:  %s\n", wine_dbgstr_w(str2));
    else if (dwl_flags & DWL_EXTERNAL)
        todo_wine ok(!strcmp_wa(str2, prev_nav_url), "unexpected address bar url:  %s\n", wine_dbgstr_w(str2));
    else
        ok(!strcmp_wa(str2, nav_url), "unexpected address bar url:  %s\n", wine_dbgstr_w(str2));
    SysFreeString(str2);
    IHTMLPrivateWindow_Release(priv_window);

    loading_js = FALSE;
    if(is_js)
        nav_url = prev_nav_url;
}

static void test_load_history(IHTMLDocument2 *doc)
{
    IPersistHistory *per_hist;
    HRESULT hres;

    trace("LoadHistory...\n");

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IPersistHistory, (void**)&per_hist);
    ok(hres == S_OK, "Could not get IPersistHistory iface: %08x\n", hres);

    prev_url = nav_url;
    nav_url = "http://test.winehq.org/tests/winehq_snapshot/#test";
    nav_serv_url = "http://test.winehq.org/tests/winehq_snapshot/";

    SET_EXPECT(Exec_ShellDocView_138);
    SET_EXPECT(Exec_ShellDocView_67);
    SET_EXPECT(FireBeforeNavigate2);
    SET_EXPECT(Invoke_AMBIENT_SILENT);
    SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);

    hres = IPersistHistory_LoadHistory(per_hist, history_stream, NULL);
    ok(hres == S_OK, "LoadHistory failed: %08x\n", hres);

    CLEAR_CALLED(Exec_ShellDocView_138); /* Not called by IE11 */
    CLEAR_CALLED(Exec_ShellDocView_67); /* Not called by IE11 */
    CHECK_CALLED(FireBeforeNavigate2);
    CHECK_CALLED(Invoke_AMBIENT_SILENT);
    CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);

    load_state = LD_LOADING;
    test_timer(EXPECT_UPDATEUI|EXPECT_SETTITLE);

    test_download(DWL_VERBDONE|DWL_HTTP|DWL_EXPECT_HISTUPDATE|DWL_ONREADY_LOADING|DWL_FROM_HISTORY|DWL_EX_GETHOSTINFO);

    IPersistHistory_Release(per_hist);
    IStream_Release(history_stream);
    history_stream = NULL;
}

static void test_OmHistory(IHTMLDocument2 *doc)
{
    IHTMLWindow2 *win;
    IOmHistory *hist;
    short len;
    HRESULT hres;

    hres = IHTMLDocument2_get_parentWindow(doc, &win);
    ok(hres == S_OK, "get_parentWindow failed: %08x\n", hres);

    hres = IHTMLWindow2_get_history(win, &hist);
    ok(hres == S_OK, "get_history failed: %08x\n", hres);
    IHTMLWindow2_Release(win);

    SET_EXPECT(CountEntries);
    hres = IOmHistory_get_length(hist, &len);
    CHECK_CALLED(CountEntries);
    ok(hres == S_OK, "get_length failed: %08x\n", hres);
    ok(len == 0, "len = %d\n", len);

    IOmHistory_Release(hist);
}

static void test_refresh(IHTMLDocument2 *doc)
{
    IOleCommandTarget *cmdtrg;
    VARIANT vin, vout;
    HRESULT hres;

    trace("Refresh...\n");

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleCommandTarget, (void**)&cmdtrg);
    ok(hres == S_OK, "Could not get IOleCommandTarget iface: %08x\n", hres);

    V_VT(&vin) = VT_EMPTY;
    V_VT(&vout) = VT_EMPTY;
    SET_EXPECT(Exec_DocHostCommandHandler_2300);
    hres = IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_REFRESH, OLECMDEXECOPT_PROMPTUSER, &vin, &vout);
    ok(hres == S_OK, "Exec failed: %08x\n", hres);
    ok(V_VT(&vout) == VT_EMPTY, "V_VT(vout) = %d\n", V_VT(&vout));
    CHECK_CALLED(Exec_DocHostCommandHandler_2300);

    IOleCommandTarget_Release(cmdtrg);

    test_download(DWL_VERBDONE|DWL_HTTP|DWL_ONREADY_LOADING|DWL_REFRESH|DWL_EX_GETHOSTINFO);
}

static void test_open_window(IHTMLDocument2 *doc, BOOL do_block)
{
    IHTMLWindow2 *window, *new_window;
    BSTR name, url;
    HRESULT hres;

    allow_new_window = !do_block;

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok(hres == S_OK, "get_parentWindow failed: %08x\n", hres);

    url = a2bstr(nav_serv_url = nav_url = "about:blank");
    name = a2bstr("test");
    new_window = (void*)0xdeadbeef;

    trace("open...\n");
    open_call = TRUE;

    if(support_wbapp)
        SET_EXPECT(get_LocationURL);
    SET_EXPECT(TranslateUrl);
    SET_EXPECT(EvaluateNewWindow);

    hres = IHTMLWindow2_open(window, url, name, NULL, VARIANT_FALSE, &new_window);
    open_call = FALSE;
    SysFreeString(url);
    SysFreeString(name);

    if(support_wbapp)
        todo_wine CHECK_CALLED_BROKEN(get_LocationURL);
    todo_wine
    CHECK_CALLED(TranslateUrl);

    if(!called_EvaluateNewWindow) {
        win_skip("INewWindowManager not supported\n");
        if(SUCCEEDED(hres) && new_window)
            IHTMLWindow2_Release(new_window);
        IHTMLWindow2_Release(window);
        return;
    }
    CHECK_CALLED(EvaluateNewWindow);

    ok(hres == S_OK, "open failed: %08x\n", hres);

    if(do_block) {
        ok(!new_window, "new_window != NULL\n");
    }else {
        ok(new_window != NULL, "new_window == NULL\n");

        hres = IHTMLWindow2_close(new_window);
        ok(hres == S_OK, "close failed: %08x\n", hres);
        IHTMLWindow2_Release(new_window);
    }

    IHTMLWindow2_Release(window);
}

static void test_window_close(IHTMLDocument2 *doc)
{
    IHTMLWindow2 *window;
    HRESULT hres;

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok(hres == S_OK, "get_parentWindow failed: %08x\n", hres);

    SET_EXPECT(FindConnectionPoint);
    SET_EXPECT(EnumConnections);
    SET_EXPECT(EnumConnections_Next);
    SET_EXPECT(WindowClosing);

    hres = IHTMLWindow2_close(window);
    ok(hres == S_OK, "close failed: %08x\n", hres);

    CHECK_CALLED(FindConnectionPoint);
    CHECK_CALLED(EnumConnections);
    CHECK_CALLED(EnumConnections_Next);
    CHECK_CALLED(WindowClosing);

    IHTMLWindow2_Release(window);
}

static void test_elem_from_point(IHTMLDocument2 *doc)
{
    IHTMLElement *elem;
    BSTR tag;
    HRESULT hres;

    elem = NULL;
    hres = IHTMLDocument2_elementFromPoint(doc, 3, 3, &elem);
    ok(hres == S_OK, "elementFromPoint failed: %08x\n", hres);
    ok(elem != NULL, "elem == NULL\n");

    hres = IHTMLElement_get_tagName(elem, &tag);
    IHTMLElement_Release(elem);
    ok(hres == S_OK, "get_tagName failed: %08x\n", hres);
    ok(!strcmp_wa(tag, "DIV"), "tag = %s\n", wine_dbgstr_w(tag));
}

static void test_clear(IHTMLDocument2 *doc)
{
    HRESULT hres;

    hres = IHTMLDocument2_clear(doc);
    ok(hres == S_OK, "clear failed: %08x\n", hres);
}

static const OLECMDF expect_cmds[] = {
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
    ok(hres == cmdf ? S_OK : OLECMDERR_E_NOTSUPPORTED, "QueryStatus(%u) failed: %08x\n", cmdid, hres);

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
    OLECMD cmds[sizeof(expect_cmds)/sizeof(*expect_cmds)-1];
    int i;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleCommandTarget, (void**)&cmdtrg);
    ok(hres == S_OK, "QueryInterface(IID_IOleCommandTarget failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    for(i=0; i < sizeof(cmds)/sizeof(*cmds); i++) {
        cmds[i].cmdID = i+1;
        cmds[i].cmdf = 0xf0f0;
    }

    SET_EXPECT(QueryStatus_OPEN);
    SET_EXPECT(QueryStatus_NEW);
    hres = IOleCommandTarget_QueryStatus(cmdtrg, NULL, sizeof(cmds)/sizeof(cmds[0]), cmds, NULL);
    ok(hres == S_OK, "QueryStatus failed: %08x\n", hres);
    CHECK_CALLED(QueryStatus_OPEN);
    CHECK_CALLED(QueryStatus_NEW);

    for(i=0; i < sizeof(cmds)/sizeof(*cmds); i++) {
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

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleCommandTarget, (void**)&cmdtrg);
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

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleCommandTarget, (void**)&cmdtrg);
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
    SET_EXPECT(Invoke_OnReadyStateChange_Loading);
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
    CHECK_CALLED(Invoke_OnReadyStateChange_Loading);
    CLEAR_CALLED(IsSystemMoniker); /* IE7 */
    CHECK_CALLED_BROKEN(Exec_ShellDocView_84);
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

static void test_exec_optical_zoom(IHTMLDocument2 *doc, int factor)
{
    IOleCommandTarget *cmdtrg;
    VARIANT v;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleCommandTarget, (void**)&cmdtrg);
    ok(hres == S_OK, "QueryInterface(IID_IOleCommandTarget) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    V_VT(&v) = VT_I4;
    V_I4(&v) = factor;

    SET_EXPECT(GetOverrideKeyPath);
    hres = IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_OPTICAL_ZOOM,
            OLECMDEXECOPT_DODEFAULT, &v, NULL);
    ok(hres == S_OK || broken(hres == OLECMDERR_E_NOTSUPPORTED) /* IE6 */, "Exec failed: %08x\n", hres);
    CLEAR_CALLED(GetOverrideKeyPath);

    IOleCommandTarget_Release(cmdtrg);

    test_QueryStatus((IUnknown*)doc, NULL, OLECMDID_OPTICAL_ZOOM, 0);
}

static void test_IsDirty(IHTMLDocument2 *doc, HRESULT exhres)
{
    IPersistStreamInit *perinit;
    IPersistMoniker *permon;
    IPersistFile *perfile;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IPersistStreamInit, (void**)&perinit);
    ok(hres == S_OK, "QueryInterface(IID_IPersistStreamInit failed: %08x\n", hres);
    if(SUCCEEDED(hres)) {
        hres = IPersistStreamInit_IsDirty(perinit);
        ok(hres == exhres, "IsDirty() = %08x, expected %08x\n", hres, exhres);
        IPersistStreamInit_Release(perinit);
    }

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IPersistMoniker, (void**)&permon);
    ok(hres == S_OK, "QueryInterface(IID_IPersistMoniker failed: %08x\n", hres);
    if(SUCCEEDED(hres)) {
        hres = IPersistMoniker_IsDirty(permon);
        ok(hres == exhres, "IsDirty() = %08x, expected %08x\n", hres, exhres);
        IPersistMoniker_Release(permon);
    }

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IPersistFile, (void**)&perfile);
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
        ok(hres == S_OK, "SetClientSite failed: %08x\n", hres);
        if(set_clientsite)
            ok(clientsite == &ClientSite, "clientsite=%p, expected %p\n", clientsite, &ClientSite);
        else
            ok(!clientsite, "clientsite != NULL\n");

        SET_EXPECT(GetOverrideKeyPath);
        hres = IOleObject_SetClientSite(oleobj, NULL);
        ok(hres == S_OK, "SetClientSite failed: %08x\n", hres);
        CLEAR_CALLED(GetOverrideKeyPath); /* IE9 sometimes calls it */

        set_clientsite = FALSE;
    }

    if(flags & CLIENTSITE_DONTSET)
        return;

    if(!expect_uihandler_iface)
        expect_uihandler_iface = &DocHostUIHandler;

    hres = IOleObject_GetClientSite(oleobj, &clientsite);
    ok(hres == S_OK, "GetClientSite failed: %08x\n", hres);
    ok(clientsite == (set_clientsite ? &ClientSite : NULL), "GetClientSite() = %p, expected %p\n",
            clientsite, set_clientsite ? &ClientSite : NULL);

    if(!set_clientsite) {
        if(expect_uihandler_iface)
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
        SET_EXPECT(GetOverrideKeyPath);
        SET_EXPECT(GetTravelLog);
        SET_EXPECT(Exec_ShellDocView_84);

        hres = IOleObject_SetClientSite(oleobj, &ClientSite);
        ok(hres == S_OK, "SetClientSite failed: %08x\n", hres);

        if(expect_uihandler_iface)
            CHECK_CALLED(GetHostInfo);
        if(flags & CLIENTSITE_EXPECTPATH) {
            CLEAR_CALLED(GetOptionKeyPath); /* not called on some IE9 */
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
        CLEAR_CALLED(Invoke_AMBIENT_PALETTE); /* not called on IE9 */
        CLEAR_CALLED(GetOverrideKeyPath); /* Called by IE9 */
        CHECK_CALLED(GetTravelLog);
        CHECK_CALLED_BROKEN(Exec_ShellDocView_84);

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

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleControl, (void**)&control);
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
    CLEAR_CALLED(Invoke_AMBIENT_PALETTE); /* not called on IE9 */

    IOleControl_Release(control);
}



static void test_OnAmbientPropertyChange2(IHTMLDocument2 *doc)
{
    IOleControl *control = NULL;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleControl, (void**)&control);
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

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleObject, (void**)&oleobj);
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

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleObject, (void**)&oleobj);
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

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleInPlaceObjectWindowless,
            (void**)&windowlessobj);
    ok(hres == S_OK, "QueryInterface(IID_IOleInPlaceObjectWindowless) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    if(expect_call) {
        SET_EXPECT(SetStatusText);
        SET_EXPECT(OnFocus_FALSE);
        if(ipsex)
            SET_EXPECT(OnInPlaceDeactivateEx);
        else
            SET_EXPECT(OnInPlaceDeactivate);
    }
    hres = IOleInPlaceObjectWindowless_InPlaceDeactivate(windowlessobj);
    ok(hres == S_OK, "InPlaceDeactivate failed: %08x\n", hres);
    if(expect_call) {
        CLEAR_CALLED(SetStatusText); /* Called by IE9 */
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

    last_hwnd = doc_hwnd;

    if(view)
        IOleDocumentView_Release(view);
    view = NULL;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleObject, (void**)&oleobj);
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
        ok(tmp_hwnd == doc_hwnd, "tmp_hwnd=%p, expected %p\n", tmp_hwnd, doc_hwnd);
    }else {
        ok(hres == E_FAIL, "GetWindow returned %08x, expected E_FAIL\n", hres);
        ok(IsWindow(doc_hwnd), "hwnd is destroyed\n");
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

static IHTMLDocument2 *create_document(void)
{
    IHTMLDocument2 *doc;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_HTMLDocument, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IHTMLDocument2, (void**)&doc);
    ok(hres == S_OK, "CoCreateInstance failed: %08x\n", hres);
    if(FAILED(hres))
        return NULL;

    return doc;
}

static void release_document(IHTMLDocument2 *doc)
{
    IUnknown *unk;
    ULONG ref;
    HRESULT hres;

    /* Some broken IEs don't like if the last released reference is IHTMLDocument2 iface.
     * To workaround it, we release it via IUnknown iface */
    hres = IHTMLDocument2_QueryInterface(doc, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "Could not get IUnknown iface: %08x\n", hres);

    IHTMLDocument2_Release(doc);
    ref = IUnknown_Release(unk);
    ok(!ref, "ref = %d\n", ref);
}

static void test_Navigate(IHTMLDocument2 *doc)
{
    IHlinkTarget *hlink;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHlinkTarget, (void**)&hlink);
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

static void test_enum_objects(IOleContainer *container)
{
    IEnumUnknown *enum_unknown;
    IUnknown *buf[100] = {(void*)0xdeadbeef};
    ULONG fetched;
    HRESULT hres;

    enum_unknown = NULL;
    hres = IOleContainer_EnumObjects(container, OLECONTF_EMBEDDINGS, &enum_unknown);
    ok(hres == S_OK, "EnumObjects failed: %08x\n", hres);
    ok(enum_unknown != NULL, "enum_unknown == NULL\n");

    fetched = 0xdeadbeef;
    hres = IEnumUnknown_Next(enum_unknown, sizeof(buf)/sizeof(*buf), buf, &fetched);
    ok(hres == S_FALSE, "Next returned %08x\n", hres);
    ok(!fetched, "fetched = %d\n", fetched);
    ok(buf[0] == (void*)0xdeadbeef, "buf[0] = %p\n", buf[0]);

    fetched = 0xdeadbeef;
    hres = IEnumUnknown_Next(enum_unknown, 1, buf, &fetched);
    ok(hres == S_FALSE, "Next returned %08x\n", hres);
    ok(!fetched, "fetched = %d\n", fetched);

    hres = IEnumUnknown_Next(enum_unknown, 1, buf, NULL);
    ok(hres == S_FALSE, "Next returned %08x\n", hres);

    IEnumUnknown_Release(enum_unknown);
}

static void test_target_container(IHTMLDocument2 *doc)
{
    IOleContainer *ole_container, *doc_ole_container;
    ITargetContainer *target_container;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_ITargetContainer, (void**)&target_container);
    ok(hres == S_OK, "Could not get ITargetContainer iface: %08x\n", hres);

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleContainer, (void**)&doc_ole_container);
    ok(hres == S_OK, "Could not get ITargetContainer iface: %08x\n", hres);

    ole_container = (void*)0xdeadbeef;
    hres = ITargetContainer_GetFramesContainer(target_container, &ole_container);
    ok(hres == S_OK, "GetFramesContainer failed: %08x\n", hres);
    ok(ole_container != NULL, "ole_container == NULL\n");
    ok(iface_cmp((IUnknown*)ole_container, (IUnknown*)doc_ole_container), "ole_container != doc_ole_container\n");
    test_enum_objects(ole_container);
    IOleContainer_Release(ole_container);

    ITargetContainer_Release(target_container);
    IOleContainer_Release(doc_ole_container);
}

static void test_travellog(IHTMLDocument2 *doc)
{
    ITravelLogClient *travellog_client;
    IHTMLWindow2 *window, *top_window;
    IUnknown *unk;
    HRESULT hres;

    window = NULL;
    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok(hres == S_OK, "get_parentWindow failed: %08x\n", hres);
    ok(window != NULL, "window = NULL\n");

    hres = IHTMLWindow2_get_top(window, &top_window);
    IHTMLWindow2_Release(window);
    ok(hres == S_OK, "get_top failed: %08x\n", hres);

    hres = IHTMLWindow2_QueryInterface(top_window, &IID_ITravelLogClient, (void**)&travellog_client);
    IHTMLWindow2_Release(top_window);
    if(hres == E_NOINTERFACE) {
        win_skip("ITravelLogClient not supported\n");
        no_travellog = TRUE;
        return;
    }
    ok(hres == S_OK, "Could not get ITraveLogClient iface: %08x\n", hres);

    unk = (void*)0xdeadbeef;
    hres = ITravelLogClient_FindWindowByIndex(travellog_client, 0, &unk);
    ok(hres == E_FAIL, "FindWindowByIndex failed: %08x\n", hres);
    ok(!unk, "unk != NULL\n");

    ITravelLogClient_Release(travellog_client);
}

static void test_StreamLoad(IHTMLDocument2 *doc)
{
    IPersistStreamInit *init;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IPersistStreamInit, (void**)&init);
    ok(hres == S_OK, "QueryInterface(IID_IPersistStreamInit) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    SET_EXPECT(Invoke_AMBIENT_SILENT);
    SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    SET_EXPECT(Exec_ShellDocView_37);
    SET_EXPECT(OnChanged_READYSTATE);
    SET_EXPECT(Invoke_OnReadyStateChange_Loading);
    SET_EXPECT(Read);
    SET_EXPECT(GetPendingUrl);
    readystate_set_loading = TRUE;

    hres = IPersistStreamInit_Load(init, &Stream);
    ok(hres == S_OK, "Load failed: %08x\n", hres);

    CHECK_CALLED(Invoke_AMBIENT_SILENT);
    CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    CHECK_CALLED(Exec_ShellDocView_37);
    CHECK_CALLED(OnChanged_READYSTATE);
    CHECK_CALLED(Invoke_OnReadyStateChange_Loading);
    CHECK_CALLED(Read);
    todo_wine CHECK_CALLED(GetPendingUrl);

    test_timer(EXPECT_SETTITLE);
    test_GetCurMoniker((IUnknown*)doc, NULL, "about:blank", FALSE);

    IPersistStreamInit_Release(init);
}

static void test_StreamInitNew(IHTMLDocument2 *doc)
{
    IPersistStreamInit *init;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IPersistStreamInit, (void**)&init);
    ok(hres == S_OK, "QueryInterface(IID_IPersistStreamInit) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    SET_EXPECT(Invoke_AMBIENT_SILENT);
    SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    SET_EXPECT(Exec_ShellDocView_37);
    SET_EXPECT(OnChanged_READYSTATE);
    SET_EXPECT(Invoke_OnReadyStateChange_Loading);
    SET_EXPECT(GetPendingUrl);
    readystate_set_loading = TRUE;

    hres = IPersistStreamInit_InitNew(init);
    ok(hres == S_OK, "Load failed: %08x\n", hres);

    CHECK_CALLED(Invoke_AMBIENT_SILENT);
    CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    CHECK_CALLED(Exec_ShellDocView_37);
    CHECK_CALLED(OnChanged_READYSTATE);
    CHECK_CALLED(Invoke_OnReadyStateChange_Loading);
    todo_wine CHECK_CALLED(GetPendingUrl);

    test_timer(EXPECT_SETTITLE);
    test_GetCurMoniker((IUnknown*)doc, NULL, "about:blank", FALSE);

    IPersistStreamInit_Release(init);
}

static void test_QueryInterface(IHTMLDocument2 *htmldoc)
{
    IUnknown *qi, *doc = (IUnknown*)htmldoc;
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

    qi = (void*)0xdeadbeef;
    hres = IUnknown_QueryInterface(doc, &IID_ITargetFrame, (void**)&qi);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(qi == NULL, "qi=%p, expected NULL\n", qi);

    hres = IUnknown_QueryInterface(doc, &IID_IDispatch, (void**)&qi);
    ok(hres == S_OK, "Could not get IDispatch interface: %08x\n", hres);
    ok(qi != (IUnknown*)doc, "disp == doc\n");
    IUnknown_Release(qi);
}

static void init_test(enum load_state_t ls) {
    doc_unk = NULL;
    doc_hwnd = last_hwnd = NULL;
    set_clientsite = FALSE;
    load_from_stream = FALSE;
    call_UIActivate = CallUIActivate_None;
    load_state = ls;
    editmode = FALSE;
    stream_read = 0;
    protocol_read = 0;
    nav_url = NULL;
    ipsex = FALSE;
    inplace_deactivated = FALSE;
    complete = FALSE;
    testing_submit = FALSE;
    expect_uihandler_iface = &DocHostUIHandler;
}

static void test_HTMLDocument(BOOL do_load, BOOL mime)
{
    IHTMLDocument2 *doc;

    trace("Testing HTMLDocument (%s, %s)...\n", (do_load ? "load" : "no load"),
            (report_mime ? "mime" : "no mime"));

    init_test(do_load ? LD_DOLOAD : LD_NO);
    report_mime = mime;

    doc = create_document();
    doc_unk = (IUnknown*)doc;

    test_QueryInterface(doc);
    test_Advise(doc);
    test_IsDirty(doc, S_FALSE);
    test_MSHTML_QueryStatus(doc, OLECMDF_SUPPORTED);
    test_external(doc, FALSE);
    test_ViewAdviseSink(doc);
    test_ConnectionPointContainer(doc);
    test_GetCurMoniker((IUnknown*)doc, NULL, NULL, FALSE);
    test_Persist(doc, &Moniker);
    if(!do_load)
        test_OnAmbientPropertyChange2(doc);

    test_Activate(doc, CLIENTSITE_EXPECTPATH);

    if(do_load) {
        set_custom_uihandler(doc, &CustomDocHostUIHandler);
        test_download(DWL_CSS|DWL_TRYCSS);
        test_GetCurMoniker((IUnknown*)doc, &Moniker, NULL, FALSE);
        test_elem_from_point(doc);
    }

    test_MSHTML_QueryStatus(doc, OLECMDF_SUPPORTED);
    test_OleCommandTarget_fail(doc);
    test_OleCommandTarget(doc);
    test_exec_optical_zoom(doc, 200);
    test_exec_optical_zoom(doc, 100);
    test_OnAmbientPropertyChange(doc);
    test_Window(doc, TRUE);
    test_external(doc, TRUE);
    test_target_container(doc);

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

    test_external(doc, TRUE);
    set_custom_uihandler(doc, NULL);
    test_external(doc, FALSE);

    test_UIDeactivate();
    test_InPlaceDeactivate(doc, TRUE);
    test_CloseView();
    test_CloseView();
    test_Close(doc, TRUE);
    test_OnAmbientPropertyChange2(doc);
    test_GetCurMoniker((IUnknown*)doc, do_load ? &Moniker : NULL, NULL, FALSE);

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

    ok(IsWindow(doc_hwnd), "hwnd is destroyed\n");
    release_document(doc);
    ok(!IsWindow(doc_hwnd), "hwnd is not destroyed\n");
}

static void test_HTMLDocument_hlink(DWORD status)
{
    IHTMLDocument2 *doc;

    trace("Testing HTMLDocument (hlink)...\n");

    init_test(LD_DOLOAD);
    ipsex = TRUE;

    doc = create_document();
    doc_unk = (IUnknown*)doc;

    set_custom_uihandler(doc, &CustomDocHostUIHandler);
    test_ViewAdviseSink(doc);
    test_ConnectionPointContainer(doc);
    test_GetCurMoniker((IUnknown*)doc, NULL, NULL, FALSE);
    test_Persist(doc, &Moniker);
    test_Navigate(doc);

    status_code = status;
    test_download(DWL_CSS|DWL_TRYCSS);
    status_code = HTTP_STATUS_OK;

    test_IsDirty(doc, S_FALSE);
    test_MSHTML_QueryStatus(doc, OLECMDF_SUPPORTED);

    test_exec_onunload(doc);
    test_Window(doc, TRUE);
    test_InPlaceDeactivate(doc, TRUE);
    test_Close(doc, FALSE);
    test_IsDirty(doc, S_FALSE);
    test_GetCurMoniker((IUnknown*)doc, &Moniker, NULL, FALSE);
    test_clear(doc);
    test_GetCurMoniker((IUnknown*)doc, &Moniker, NULL, FALSE);

    if(view)
        IOleDocumentView_Release(view);
    view = NULL;

    release_document(doc);
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
    if(str2)
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
    if(str2)
        ok(strstrW(str2, str) != NULL, "could not find %s in %s\n", wine_dbgstr_w(str), wine_dbgstr_w(str2));
    SysFreeString(str);
    SysFreeString(str2);
}

static void test_HTMLDocument_http(BOOL with_wbapp)
{
    IMoniker *http_mon;
    IHTMLDocument2 *doc;
    ULONG ref;
    HRESULT hres;

    trace("Testing HTMLDocument (http%s)...\n", with_wbapp ? " with IWebBrowserApp" : "");

    support_wbapp = with_wbapp;

    if(!winetest_interactive && is_ie_hardened()) {
        win_skip("IE running in Enhanced Security Configuration\n");
        return;
    }

    init_test(LD_DOLOAD);
    ipsex = TRUE;

    doc = create_document();
    doc_unk = (IUnknown*)doc;

    hres = CreateURLMoniker(NULL, http_urlW, &http_mon);
    ok(hres == S_OK, "CreateURLMoniker failed: %08x\n", hres);

    test_ViewAdviseSink(doc);
    test_ConnectionPointContainer(doc);
    test_GetCurMoniker((IUnknown*)doc, NULL, NULL, FALSE);
    test_Persist(doc, http_mon);
    test_Navigate(doc);
    test_download(DWL_HTTP);
    test_cookies(doc);
    test_IsDirty(doc, S_FALSE);
    test_MSHTML_QueryStatus(doc, OLECMDF_SUPPORTED);
    test_GetCurMoniker((IUnknown*)doc, http_mon, NULL, FALSE);
    test_travellog(doc);
    test_binding_ui((IUnknown*)doc);

    nav_url = nav_serv_url = "http://test.winehq.org/tests/winehq_snapshot/"; /* for valid prev nav_url */
    if(support_wbapp) {
        test_put_href(doc, FALSE, "#test", "http://test.winehq.org/tests/winehq_snapshot/#test", FALSE, TRUE, 0);
        test_travellog(doc);
        test_refresh(doc);
    }
    test_put_href(doc, FALSE, NULL, "javascript:external%20&&undefined", TRUE, FALSE, 0);
    test_put_href(doc, FALSE, NULL, "about:blank", FALSE, FALSE, support_wbapp ? DWL_EXPECT_HISTUPDATE : 0);
    test_put_href(doc, TRUE, NULL, "about:replace", FALSE, FALSE, 0);
    if(support_wbapp) {
        test_load_history(doc);
        test_OmHistory(doc);
        test_put_href(doc, FALSE, NULL, "about:blank", FALSE, FALSE, support_wbapp ? DWL_EXPECT_HISTUPDATE : 0);
    }

    prev_url = nav_serv_url;
    test_open_window(doc, TRUE);
    if(!support_wbapp) /* FIXME */
        test_open_window(doc, FALSE);
    if(support_wbapp) {
        test_put_href(doc, FALSE, NULL, "http://test.winehq.org/tests/file.winetest", FALSE, FALSE, DWL_EXTERNAL);
        test_window_close(doc);
    }

    test_InPlaceDeactivate(doc, TRUE);
    test_Close(doc, FALSE);
    test_IsDirty(doc, S_FALSE);
    test_GetCurMoniker((IUnknown*)doc, NULL, prev_url, support_wbapp);

    if(view)
        IOleDocumentView_Release(view);
    view = NULL;

    release_document(doc);

    ref = IMoniker_Release(http_mon);
    ok(!ref, "ref=%d, expected 0\n", ref);
}

static void put_inner_html(IHTMLElement *elem, const char *html)
{
    BSTR str = a2bstr(html);
    HRESULT hres;

    hres = IHTMLElement_put_innerHTML(elem, str);
    ok(hres == S_OK, "put_innerHTML failed: %08x\n", hres);

    SysFreeString(str);
}

static IHTMLElement *get_elem_by_id(IHTMLDocument2 *doc, const char *id)
{
    IHTMLDocument3 *doc3;
    BSTR str = a2bstr(id);
    IHTMLElement *ret;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument3, (void**)&doc3);
    ok(hres == S_OK, "Could not get IHTMLDocument3 iface: %08x\n", hres);

    hres = IHTMLDocument3_getElementById(doc3, str, &ret);
    ok(hres == S_OK, "getElementById failed: %08x\n", hres);

    IHTMLDocument3_Release(doc3);
    return ret;
}

static void reset_document(IHTMLDocument2 *doc)
{
    IPersistStreamInit *init;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IPersistStreamInit, (void**)&init);
    ok(hres == S_OK, "QueryInterface(IID_IPersistStreamInit) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    resetting_document = TRUE;

    hres = IPersistStreamInit_InitNew(init);
    ok(hres == S_OK, "Load failed: %08x\n", hres);

    resetting_document = FALSE;

    test_GetCurMoniker((IUnknown*)doc, NULL, "about:blank", FALSE);

    IPersistStreamInit_Release(init);
}

static void test_submit(void)
{
    IHTMLElement *body, *form_elem;
    IHTMLFormElement *form;
    IHTMLDocument2 *doc;
    HRESULT hres;

    if(no_travellog)
        return;

    trace("Testing submit...\n");

    support_wbapp = TRUE;

    if(!winetest_interactive && is_ie_hardened()) {
        win_skip("IE running in Enhanced Security Configuration\n");
        return;
    }

    init_test(LD_DOLOAD);
    ipsex = TRUE;

    doc = create_document();
    doc_unk = (IUnknown*)doc;

    test_ConnectionPointContainer(doc);
    test_ViewAdviseSink(doc);
    test_Persist(doc, &Moniker);
    test_Navigate(doc);
    test_download(DWL_CSS|DWL_TRYCSS);

    hres = IHTMLDocument2_get_body(doc, &body);
    ok(hres == S_OK, "get_body failed: %08x\n", hres);
    ok(body != NULL, "body = NULL\n");

    put_inner_html(body, "<form action='test_submit' method='post' id='fid'><input type='hidden' name='cmd' value='TEST'></form>");
    IHTMLElement_Release(body);

    form_elem = get_elem_by_id(doc, "fid");
    ok(form_elem != NULL, "form = NULL\n");

    hres = IHTMLElement_QueryInterface(form_elem, &IID_IHTMLFormElement, (void**)&form);
    ok(hres == S_OK, "Could not get IHTMLFormElement: %08x\n", hres);
    IHTMLElement_Release(form_elem);

    nav_url = nav_serv_url = "winetest:test_submit";
    testing_submit = TRUE;

    SET_EXPECT(TranslateUrl);
    SET_EXPECT(FireBeforeNavigate2);
    SET_EXPECT(Exec_ShellDocView_67);
    SET_EXPECT(Invoke_AMBIENT_SILENT);
    SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    SET_EXPECT(Exec_ShellDocView_63);
    SET_EXPECT(Exec_ShellDocView_84);
    SET_EXPECT(CreateInstance);
    SET_EXPECT(Start);
    SET_EXPECT(Protocol_Read);
    SET_EXPECT(LockRequest);
    SET_EXPECT(GetClassID);
    SET_EXPECT(Exec_ShellDocView_138);
    SET_EXPECT(OnViewChange);

    SET_EXPECT(UnlockRequest);
    SET_EXPECT(Terminate);

    hres = IHTMLFormElement_submit(form);
    ok(hres == S_OK, "submit failed: %08x\n", hres);

    CHECK_CALLED(TranslateUrl);
    CHECK_CALLED(FireBeforeNavigate2);
    CLEAR_CALLED(Exec_ShellDocView_67); /* Not called by IE11 */
    CHECK_CALLED(Invoke_AMBIENT_SILENT);
    CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    CHECK_CALLED(Exec_ShellDocView_63);
    CLEAR_CALLED(Exec_ShellDocView_84); /* Not called by IE11 */
    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(Start);
    CHECK_CALLED(Protocol_Read);
    CHECK_CALLED(LockRequest);
    todo_wine CHECK_CALLED(GetClassID);
    CLEAR_CALLED(Exec_ShellDocView_138); /* called only by some versions */
    CLEAR_CALLED(OnViewChange); /* called only by some versions */

    todo_wine CHECK_NOT_CALLED(UnlockRequest);
    todo_wine CHECK_NOT_CALLED(Terminate);

    IHTMLFormElement_Release(form);

    test_GetCurMoniker((IUnknown*)doc, &Moniker, NULL, FALSE);

    SET_EXPECT(UnlockRequest);
    reset_document(doc);
    todo_wine CHECK_CALLED(UnlockRequest);

    test_InPlaceDeactivate(doc, TRUE);
    test_Close(doc, FALSE);

    if(view)
        IOleDocumentView_Release(view);
    view = NULL;

    release_document(doc);
}

static void test_QueryService(IHTMLDocument2 *doc, BOOL success)
{
    IHTMLWindow2 *window, *sp_window;
    IServiceProvider *sp;
    IHlinkFrame *hf;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IServiceProvider, (void**)&sp);
    ok(hres == S_OK, "QueryService returned %08x\n", hres);

    hres = IServiceProvider_QueryService(sp, &IID_IHlinkFrame, &IID_IHlinkFrame, (void**)&hf);
    if(!success) {
        ok(hres == E_NOINTERFACE, "QueryService returned %08x, expected E_NOINTERFACE\n", hres);
        IServiceProvider_Release(sp);
        return;
    }

    ok(hres == S_OK, "QueryService(IID_IHlinkFrame) failed: %08x\n", hres);
    ok(hf == &HlinkFrame, "hf != HlinkFrame\n");
    IHlinkFrame_Release(hf);

    IServiceProvider_Release(sp);

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok(hres == S_OK, "get_parentWindow failed: %08x\n", hres);

    hres = IHTMLWindow2_QueryInterface(window, &IID_IServiceProvider, (void**)&sp);
    ok(hres == S_OK, "Could not get IServiceProvider iface: %08x\n", hres);

    hres = IServiceProvider_QueryService(sp, &IID_IHTMLWindow2, &IID_IHTMLWindow2, (void**)&sp_window);
    ok(hres == S_OK, "QueryService(IID_IHTMLWindow2) failed: %08x\n", hres);
    /* FIXME: test returned window */
    IHTMLWindow2_Release(sp_window);

    hres = IServiceProvider_QueryService(sp, &IID_IHlinkFrame, &IID_IHlinkFrame, (void**)&hf);
    ok(hres == S_OK, "QueryService(IID_IHlinkFrame) failed: %08x\n", hres);
    ok(hf == &HlinkFrame, "hf != HlinkFrame\n");
    IHlinkFrame_Release(hf);

    IServiceProvider_Release(sp);
    IHTMLWindow2_Release(window);
}

static void test_HTMLDocument_StreamLoad(void)
{
    IHTMLDocument2 *doc;
    IOleObject *oleobj;
    DWORD conn;
    HRESULT hres;

    trace("Testing HTMLDocument (IPersistStreamInit)...\n");

    init_test(LD_DOLOAD);
    load_from_stream = TRUE;

    doc = create_document();
    doc_unk = (IUnknown*)doc;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleObject, (void**)&oleobj);
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

    test_GetCurMoniker((IUnknown*)doc, NULL, NULL, FALSE);
    test_StreamLoad(doc);
    test_download(DWL_VERBDONE|DWL_TRYCSS);
    test_MSHTML_QueryStatus(doc, OLECMDF_SUPPORTED);

    test_UIDeactivate();
    test_InPlaceDeactivate(doc, TRUE);
    SET_EXPECT(Advise_Close);
    test_Close(doc, TRUE);
    CHECK_CALLED(Advise_Close);
    test_external(doc, FALSE);
    test_IsDirty(doc, S_FALSE);

    set_custom_uihandler(doc, &CustomDocHostUIHandler);
    test_ClientSite(oleobj, CLIENTSITE_SETNULL);
    test_external(doc, TRUE);
    test_ClientSite(oleobj, CLIENTSITE_SETNULL|CLIENTSITE_DONTSET);
    test_external(doc, TRUE);
    set_custom_uihandler(doc, NULL);
    test_ClientSite(oleobj, CLIENTSITE_SETNULL|CLIENTSITE_DONTSET);

    IOleObject_Release(oleobj);
    if(view) {
        IOleDocumentView_Release(view);
        view = NULL;
    }

    release_document(doc);
}

static void test_HTMLDocument_StreamInitNew(void)
{
    IHTMLDocument2 *doc;
    IOleObject *oleobj;
    DWORD conn;
    HRESULT hres;

    trace("Testing HTMLDocument (IPersistStreamInit::InitNew)...\n");

    init_test(LD_DOLOAD);
    load_from_stream = TRUE;

    doc = create_document();
    doc_unk = (IUnknown*)doc;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleObject, (void**)&oleobj);
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

    test_GetCurMoniker((IUnknown*)doc, NULL, NULL, FALSE);
    test_StreamInitNew(doc);

    SET_EXPECT(Invoke_OnReadyStateChange_Interactive);
    test_download(DWL_VERBDONE|DWL_TRYCSS|DWL_EMPTY);
    todo_wine CHECK_NOT_CALLED(Invoke_OnReadyStateChange_Interactive);

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

    release_document(doc);
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

static void test_editing_mode(BOOL do_load, BOOL use_design_mode)
{
    IHTMLDocument2 *doc;
    IUnknown *unk;
    IOleObject *oleobj;
    DWORD conn;
    HRESULT hres;

    trace("Testing HTMLDocument (edit%s%s)...\n", do_load ? " load" : "", use_design_mode ? " using designMode" : "");

    init_test(do_load ? LD_DOLOAD : LD_NO);
    call_UIActivate = CallUIActivate_AfterShow;

    doc = create_document();
    unk = doc_unk = (IUnknown*)doc;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleObject, (void**)&oleobj);
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

    if(!use_design_mode) {
        test_exec_editmode(unk, do_load);
        test_UIDeactivate();
        call_UIActivate = CallUIActivate_None;
    }else {
        BSTR on;

        SET_EXPECT(Exec_SETTITLE);
        test_download(DWL_VERBDONE|DWL_CSS|DWL_TRYCSS);
        CLEAR_CALLED(Exec_SETTITLE);

        editmode = TRUE;
        load_state = LD_DOLOAD;
        readystate_set_loading = TRUE;

        SET_EXPECT(OnChanged_1005);
        SET_EXPECT(ActiveElementChanged);
        SET_EXPECT(GetClassID);
        SET_EXPECT(SetStatusText);
        SET_EXPECT(Exec_ShellDocView_37);
        SET_EXPECT(GetHostInfo);
        SET_EXPECT(GetDisplayName);
        SET_EXPECT(Invoke_AMBIENT_SILENT);
        SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        SET_EXPECT(OnChanged_READYSTATE);
        SET_EXPECT(Invoke_OnReadyStateChange_Loading);
        SET_EXPECT(IsSystemMoniker);
        SET_EXPECT(Exec_ShellDocView_84);
        SET_EXPECT(BindToStorage);
        SET_EXPECT(InPlaceUIWindow_SetActiveObject);
        SET_EXPECT(HideUI);
        SET_EXPECT(ShowUI);
        SET_EXPECT(InPlaceFrame_SetBorderSpace);
        SET_EXPECT(OnChanged_1014);

        on = a2bstr("On");
        hres = IHTMLDocument2_put_designMode(doc, on);
        SysFreeString(on);
        ok(hres == S_OK, "put_designMode failed: %08x\n", hres);

        todo_wine CHECK_CALLED(OnChanged_1005);
        todo_wine CHECK_CALLED(ActiveElementChanged);
        CHECK_CALLED(GetClassID);
        CHECK_CALLED(SetStatusText);
        CHECK_CALLED(Exec_ShellDocView_37);
        CHECK_CALLED(GetHostInfo);
        CHECK_CALLED(GetDisplayName);
        CHECK_CALLED(Invoke_AMBIENT_SILENT);
        CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        CHECK_CALLED(OnChanged_READYSTATE);
        CHECK_CALLED(Invoke_OnReadyStateChange_Loading);
        CLEAR_CALLED(IsSystemMoniker); /* IE7 */
        CHECK_CALLED_BROKEN(Exec_ShellDocView_84);
        CHECK_CALLED(BindToStorage);
        CHECK_CALLED(InPlaceUIWindow_SetActiveObject);
        CHECK_CALLED(HideUI);
        CHECK_CALLED(ShowUI);
        CHECK_CALLED(InPlaceFrame_SetBorderSpace);
        CHECK_CALLED(OnChanged_1014);

        test_timer(EXPECT_UPDATEUI|EXPECT_SETTITLE);
    }

    IOleObject_Release(oleobj);

    test_MSHTML_QueryStatus(doc, OLECMDF_SUPPORTED);
    test_download(DWL_VERBDONE | DWL_EX_GETHOSTINFO | (do_load ? DWL_CSS|DWL_TRYCSS : 0));

    SET_EXPECT(SetStatusText); /* ignore race in native mshtml */
    test_timer(EXPECT_UPDATEUI);
    CLEAR_CALLED(SetStatusText);

    test_MSHTML_QueryStatus(doc, OLECMDF_SUPPORTED|OLECMDF_ENABLED);

    if(!do_load) {
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

    release_document(doc);
}

static void test_UIActivate(BOOL do_load, BOOL use_ipsex, BOOL use_ipsw)
{
    IHTMLDocument2 *doc;
    IOleObject *oleobj;
    IOleInPlaceSite *inplacesite;
    HRESULT hres;

    trace("Running OleDocumentView_UIActivate tests (%d %d %d)\n", do_load, use_ipsex, use_ipsw);

    init_test(do_load ? LD_DOLOAD : LD_NO);

    doc = create_document();
    doc_unk = (IUnknown*)doc;

    ipsex = use_ipsex;
    ipsw = use_ipsw;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_IOleObject) failed: %08x\n", hres);

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleDocumentView, (void**)&view);
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
    SET_EXPECT(GetTravelLog);
    SET_EXPECT(Exec_ShellDocView_84);

    hres = IOleObject_SetClientSite(oleobj, &ClientSite);
    ok(hres == S_OK, "SetClientSite failed: %08x\n", hres);
    set_clientsite = TRUE;

    CHECK_CALLED(Invoke_AMBIENT_USERMODE);
    CHECK_CALLED(GetHostInfo);
    CHECK_CALLED(Invoke_AMBIENT_DLCONTROL);
    CHECK_CALLED(Invoke_AMBIENT_SILENT);
    CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    CHECK_CALLED(Invoke_AMBIENT_USERAGENT);
    CLEAR_CALLED(Invoke_AMBIENT_PALETTE); /* not called on IE9 */
    CLEAR_CALLED(GetOptionKeyPath); /* not called on some IE9 */
    CHECK_CALLED(GetOverrideKeyPath);
    CHECK_CALLED(GetWindow);
    CHECK_CALLED(Exec_DOCCANNAVIGATE);
    CHECK_CALLED(QueryStatus_SETPROGRESSTEXT);
    CHECK_CALLED(Exec_SETPROGRESSMAX);
    CHECK_CALLED(Exec_SETPROGRESSPOS);
    CHECK_CALLED(GetTravelLog);
    CHECK_CALLED_BROKEN(Exec_ShellDocView_84);

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

    SET_EXPECT(SetStatusText);
    SET_EXPECT(OnFocus_FALSE);
    if(use_ipsex)
        SET_EXPECT(OnInPlaceDeactivateEx);
    else
        SET_EXPECT(OnInPlaceDeactivate);

    test_CloseView();

    CLEAR_CALLED(SetStatusText); /* Called by IE9 */
    CHECK_CALLED(OnFocus_FALSE);
    if(use_ipsex)
        CHECK_CALLED(OnInPlaceDeactivateEx);
    else
        CHECK_CALLED(OnInPlaceDeactivate);

    test_Close(doc, TRUE);

    IOleObject_Release(oleobj);
    IOleDocumentView_Release(view);
    view = NULL;

    release_document(doc);
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

    doc = create_document();

    hres = IHTMLDocument2_QueryInterface(doc, &IID_ISupportErrorInfo, (void**)&sinfo);
    ok(hres == S_OK, "got %x\n", hres);
    ok(sinfo != NULL, "got %p\n", sinfo);
    if(sinfo)
    {
        hres = ISupportErrorInfo_InterfaceSupportsErrorInfo(sinfo, &IID_IErrorInfo);
        ok(hres == S_FALSE, "Expected S_OK, got %x\n", hres);
        ISupportErrorInfo_Release(sinfo);
    }

    release_document(doc);
}

static BOOL check_ie(void)
{
    IHTMLDocument2 *doc;
    IHTMLDocument5 *doc5;
    IHTMLDocument7 *doc7;
    HRESULT hres;

    doc = create_document();
    if(!doc)
        return FALSE;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument7, (void**)&doc7);
    if(SUCCEEDED(hres)) {
        is_ie9plus = TRUE;
        IHTMLDocument7_Release(doc7);
    }

    trace("is_ie9plus %x\n", is_ie9plus);

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument5, (void**)&doc5);
    if(SUCCEEDED(hres))
        IHTMLDocument5_Release(doc5);

    release_document(doc);
    return SUCCEEDED(hres);
}

static void test_ServiceProvider(void)
{
    IHTMLDocument3 *doc3, *doc3_2;
    IServiceProvider *provider;
    IHTMLDocument2 *doc, *doc2;
    IUnknown *unk;
    HRESULT hres;

    doc = create_document();
    if(!doc)
        return;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IServiceProvider, (void**)&provider);
    ok(hres == S_OK, "got 0x%08x\n", hres);

    hres = IServiceProvider_QueryService(provider, &SID_SContainerDispatch, &IID_IHTMLDocument2, (void**)&doc2);
    ok(hres == S_OK, "got 0x%08x\n", hres);
    ok(iface_cmp((IUnknown*)doc2, (IUnknown*)doc), "got wrong pointer\n");
    IHTMLDocument2_Release(doc2);

    hres = IServiceProvider_QueryService(provider, &SID_SContainerDispatch, &IID_IHTMLDocument3, (void**)&doc3);
    ok(hres == S_OK, "got 0x%08x\n", hres);
    ok(iface_cmp((IUnknown*)doc3, (IUnknown*)doc), "got wrong pointer\n");

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument3, (void**)&doc3_2);
    ok(hres == S_OK, "got 0x%08x\n", hres);
    ok(iface_cmp((IUnknown*)doc3_2, (IUnknown*)doc), "got wrong pointer\n");
    ok(iface_cmp((IUnknown*)doc3_2, (IUnknown*)doc3), "got wrong pointer\n");
    IHTMLDocument3_Release(doc3);
    IHTMLDocument3_Release(doc3_2);

    hres = IServiceProvider_QueryService(provider, &SID_SContainerDispatch, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "got 0x%08x\n", hres);
    ok(iface_cmp((IUnknown*)doc, unk), "got wrong pointer\n");

    IUnknown_Release(unk);
    IServiceProvider_Release(provider);
    release_document(doc);
}

START_TEST(htmldoc)
{
    CoInitialize(NULL);

    if(!check_ie()) {
        CoUninitialize();
        win_skip("Too old IE\n");
        return;
    }

    container_hwnd = create_container_window();
    register_protocol();

    asynchronous_binding = TRUE;
    test_HTMLDocument_hlink(HTTP_STATUS_NOT_FOUND);

    asynchronous_binding = FALSE;
    test_HTMLDocument_hlink(HTTP_STATUS_OK);
    test_HTMLDocument(FALSE, TRUE);
    test_HTMLDocument(TRUE, FALSE);
    test_HTMLDocument(TRUE, TRUE);
    test_HTMLDocument_StreamLoad();
    test_HTMLDocument_StreamInitNew();
    if (winetest_interactive)
        test_editing_mode(FALSE, FALSE);
    else
        skip("Skipping test_editing_mode(FALSE, FALSE). ROSTESTS-113.\n");
    test_editing_mode(TRUE, FALSE);
    test_editing_mode(TRUE, TRUE);
    if (winetest_interactive)
    {
        test_HTMLDocument_http(FALSE);
        test_HTMLDocument_http(TRUE);
    }
    else
    {
        skip("Skipping test_HTMLDocument_http(FALSE). ROSTESTS-113.\n");
        skip("Skipping test_HTMLDocument_http(TRUE). ROSTESTS-113.\n");
    }

    test_submit();
    test_UIActivate(FALSE, FALSE, FALSE);
    test_UIActivate(FALSE, TRUE, FALSE);
    test_UIActivate(FALSE, TRUE, TRUE);
    test_UIActivate(TRUE, FALSE, FALSE);
    test_UIActivate(TRUE, TRUE, FALSE);
    test_UIActivate(TRUE, TRUE, TRUE);
    test_HTMLDoc_ISupportErrorInfo();
    test_ServiceProvider();

    DestroyWindow(container_hwnd);
    CoUninitialize();
}
