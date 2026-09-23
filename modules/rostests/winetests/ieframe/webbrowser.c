/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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
#include "exdisp.h"
#include "htiframe.h"
#include "htiface.h"
#include "mshtmhst.h"
#include "mshtmcid.h"
#include "mshtml.h"
#include "idispids.h"
#include "olectl.h"
#include "mshtmdid.h"
#include "shobjidl.h"
#include "shdeprecated.h"
#include "shlguid.h"
#include "exdispid.h"
#include "mimeinfo.h"
#include "hlink.h"
#include "docobjectservice.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
DEFINE_GUID(outer_test_iid,0x06d4cd6c,0x18dd,0x11ea,0x8e,0x76,0xfc,0xaa,0x14,0x72,0x2d,0xac);
DEFINE_OLEGUID(CGID_DocHostCmdPriv, 0x000214D4L, 0, 0);
DEFINE_GUID(IID_IProxyManager,0x00000008,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    do { called_ ## func = FALSE; expect_ ## func = TRUE; } while(0)

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

#define CLEAR_CALLED(func) \
    expect_ ## func = called_ ## func = FALSE

DEFINE_EXPECT(GetContainer);
DEFINE_EXPECT(Site_GetWindow);
DEFINE_EXPECT(ShowObject);
DEFINE_EXPECT(CanInPlaceActivate);
DEFINE_EXPECT(OnInPlaceActivate);
DEFINE_EXPECT(OnUIActivate);
DEFINE_EXPECT(GetWindowContext);
DEFINE_EXPECT(Frame_GetWindow);
DEFINE_EXPECT(Frame_SetActiveObject);
DEFINE_EXPECT(UIWindow_SetActiveObject);
DEFINE_EXPECT(SetMenu);
DEFINE_EXPECT(Invoke_AMBIENT_USERMODE);
DEFINE_EXPECT(Invoke_AMBIENT_DLCONTROL);
DEFINE_EXPECT(Invoke_AMBIENT_USERAGENT);
DEFINE_EXPECT(Invoke_AMBIENT_PALETTE);
DEFINE_EXPECT(Invoke_AMBIENT_SILENT);
DEFINE_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
DEFINE_EXPECT(Invoke_STATUSTEXTCHANGE);
DEFINE_EXPECT(Invoke_PROPERTYCHANGE);
DEFINE_EXPECT(Invoke_DOWNLOADBEGIN);
DEFINE_EXPECT(Invoke_BEFORENAVIGATE2);
DEFINE_EXPECT(Invoke_SETSECURELOCKICON);
DEFINE_EXPECT(Invoke_FILEDOWNLOAD);
DEFINE_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_FALSE);
DEFINE_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_TRUE);
DEFINE_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_FALSE);
DEFINE_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_TRUE);
DEFINE_EXPECT(Invoke_COMMANDSTATECHANGE_UPDATECOMMANDS);
DEFINE_EXPECT(Invoke_DOWNLOADCOMPLETE);
DEFINE_EXPECT(Invoke_ONMENUBAR);
DEFINE_EXPECT(Invoke_ONADDRESSBAR);
DEFINE_EXPECT(Invoke_ONSTATUSBAR);
DEFINE_EXPECT(Invoke_ONTOOLBAR);
DEFINE_EXPECT(Invoke_ONFULLSCREEN);
DEFINE_EXPECT(Invoke_ONTHEATERMODE);
DEFINE_EXPECT(Invoke_WINDOWSETRESIZABLE);
DEFINE_EXPECT(Invoke_TITLECHANGE);
DEFINE_EXPECT(Invoke_NAVIGATECOMPLETE2);
DEFINE_EXPECT(Invoke_PROGRESSCHANGE);
DEFINE_EXPECT(Invoke_DOCUMENTCOMPLETE);
DEFINE_EXPECT(Invoke_WINDOWCLOSING);
DEFINE_EXPECT(Invoke_SETPHISHINGFILTERSTATUS);
DEFINE_EXPECT(EnableModeless_TRUE);
DEFINE_EXPECT(EnableModeless_FALSE);
DEFINE_EXPECT(GetHostInfo);
DEFINE_EXPECT(GetOptionKeyPath);
DEFINE_EXPECT(GetOverridesKeyPath);
DEFINE_EXPECT(SetStatusText);
DEFINE_EXPECT(UpdateUI);
DEFINE_EXPECT(Exec_SETDOWNLOADSTATE_0);
DEFINE_EXPECT(Exec_SETDOWNLOADSTATE_1);
DEFINE_EXPECT(Exec_SETPROGRESSMAX);
DEFINE_EXPECT(Exec_SETPROGRESSPOS);
DEFINE_EXPECT(Exec_UPDATECOMMANDS);
DEFINE_EXPECT(Exec_SETTITLE);
DEFINE_EXPECT(QueryStatus_SETPROGRESSTEXT);
DEFINE_EXPECT(Exec_STOP);
DEFINE_EXPECT(Exec_IDM_STOP);
DEFINE_EXPECT(Exec_DocHostCommandHandler_2300);
DEFINE_EXPECT(QueryStatus_STOP);
DEFINE_EXPECT(QueryStatus_IDM_STOP);
DEFINE_EXPECT(DocHost_EnableModeless_TRUE);
DEFINE_EXPECT(DocHost_EnableModeless_FALSE);
DEFINE_EXPECT(DocHost_TranslateAccelerator);
DEFINE_EXPECT(GetDropTarget);
DEFINE_EXPECT(TranslateUrl);
DEFINE_EXPECT(ShowUI);
DEFINE_EXPECT(HideUI);
DEFINE_EXPECT(OnUIDeactivate);
DEFINE_EXPECT(OnInPlaceDeactivate);
DEFINE_EXPECT(RequestUIActivate);
DEFINE_EXPECT(ControlSite_TranslateAccelerator);
DEFINE_EXPECT(OnFocus_TRUE);
DEFINE_EXPECT(OnFocus_FALSE);
DEFINE_EXPECT(GetExternal);
DEFINE_EXPECT(outer_QI_test);
DEFINE_EXPECT(Advise_OnClose);

static VARIANT_BOOL exvb;

static IWebBrowser2 *wb;

static HWND container_hwnd, shell_embedding_hwnd;
static BOOL is_downloading, do_download, is_first_load, use_container_olecmd;
static BOOL test_close, test_hide, is_http, use_container_dochostui;
static HRESULT hr_dochost_TranslateAccelerator = E_NOTIMPL;
static HRESULT hr_site_TranslateAccelerator = E_NOTIMPL;
static const WCHAR *current_url;
static int wb_version, expect_update_commands_enable, set_update_commands_enable;
static BOOL nav_back_todo, nav_forward_todo; /* FIXME */

enum SessionOp
{
    SESSION_QUERY,
    SESSION_INCREMENT,
    SESSION_DECREMENT
};

static LONG (WINAPI *pSetQueryNetSessionCount)(DWORD);

#define BUSY_FAIL 2

#define DWL_EXPECT_BEFORE_NAVIGATE  0x01
#define DWL_FROM_PUT_HREF           0x02
#define DWL_FROM_GOBACK             0x04
#define DWL_FROM_GOFORWARD          0x08
#define DWL_HTTP                    0x10
#define DWL_REFRESH                 0x20
#define DWL_BACK_ENABLE             0x40

static DWORD dwl_flags;

static IAdviseSink test_sink;

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

#define create_webbrowser() _create_webbrowser(__LINE__)
static IWebBrowser2 *_create_webbrowser(unsigned line)
{
    IWebBrowser2 *ret;
    HRESULT hres;

    wb_version = 2;

    hres = CoCreateInstance(&CLSID_WebBrowser, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IWebBrowser2, (void**)&ret);
    ok_(__FILE__,line)(hres == S_OK, "Creating WebBrowser object failed: %08lx\n", hres);
    return ret;
}

#define test_LocationURL(a,b) _test_LocationURL(__LINE__,a,b)
static void _test_LocationURL(unsigned line, IWebBrowser2 *wb, const WCHAR *exurl)
{
    BSTR url = (void*)0xdeadbeef;
    HRESULT hres;

    hres = IWebBrowser2_get_LocationURL(wb, &url);
    ok_(__FILE__,line) (hres == (*exurl ? S_OK : S_FALSE), "get_LocationURL failed: %08lx\n", hres);
    if (SUCCEEDED(hres))
    {
        ok_(__FILE__,line) (!lstrcmpW(url, exurl), "unexpected URL: %s\n", wine_dbgstr_w(url));
        SysFreeString(url);
    }
}

#define test_LocationName(a,b) _test_LocationName(__LINE__,a,b)
static void _test_LocationName(unsigned line, IWebBrowser2 *wb, const WCHAR *exname)
{
    BSTR name = (void*)0xdeadbeef;
    HRESULT hres;

    hres = IWebBrowser2_get_LocationName(wb, &name);
    ok_(__FILE__,line) (hres == (*exname ? S_OK : S_FALSE), "get_LocationName failed: %08lx\n", hres);
    ok_(__FILE__,line) (!lstrcmpW(name, exname) || broken(is_http && !lstrcmpW(name, current_url)) /* Win10 2004 */,
            "expected %s, got %s\n", wine_dbgstr_w(exname), wine_dbgstr_w(name));
    SysFreeString(name);
}

#define test_ready_state(a,b) _test_ready_state(__LINE__,a,b)
static void _test_ready_state(unsigned line, READYSTATE exstate, VARIANT_BOOL expect_busy)
{
    READYSTATE state;
    VARIANT_BOOL busy;
    HRESULT hres;

    hres = IWebBrowser2_get_ReadyState(wb, &state);
    ok_(__FILE__,line)(hres == S_OK, "get_ReadyState failed: %08lx\n", hres);
    ok_(__FILE__,line)(state == exstate, "ReadyState = %d, expected %d\n", state, exstate);

    hres = IWebBrowser2_get_Busy(wb, &busy);
    if(expect_busy != BUSY_FAIL) {
        ok_(__FILE__,line)(hres == S_OK, "get_ReadyState failed: %08lx\n", hres);
        ok_(__FILE__,line)(busy == expect_busy, "Busy = %x, expected %x for ready state %d\n",
                           busy, expect_busy, state);
    }else {
        todo_wine
        ok_(__FILE__,line)(hres == E_FAIL, "get_ReadyState returned: %08lx\n", hres);
    }
}

#define get_document(u) _get_document(__LINE__,u)
static IHTMLDocument2 *_get_document(unsigned line, IWebBrowser2 *wb)
{
    IHTMLDocument2 *html_doc;
    IDispatch *disp;
    HRESULT hres;

    disp = NULL;
    hres = IWebBrowser2_get_Document(wb, &disp);
    ok_(__FILE__,line)(hres == S_OK, "get_Document failed: %08lx\n", hres);
    ok_(__FILE__,line)(disp != NULL, "doc_disp == NULL\n");

    hres = IDispatch_QueryInterface(disp, &IID_IHTMLDocument2, (void**)&html_doc);
    ok_(__FILE__,line)(hres == S_OK, "Could not get IHTMLDocument iface: %08lx\n", hres);
    ok(disp == (IDispatch*)html_doc, "disp != html_doc\n");
    IDispatch_Release(disp);

    return html_doc;
}

#define get_dochost(u) _get_dochost(__LINE__,u)
static IOleClientSite *_get_dochost(unsigned line, IWebBrowser2 *unk)
{
    IOleClientSite *client_site;
    IHTMLDocument2 *doc;
    IOleObject *oleobj;
    HRESULT hres;

    doc = _get_document(line, unk);
    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleObject, (void**)&oleobj);
    IHTMLDocument2_Release(doc);
    ok_(__FILE__,line)(hres == S_OK, "Got 0x%08lx\n", hres);

    hres = IOleObject_GetClientSite(oleobj, &client_site);
    IOleObject_Release(oleobj);
    ok_(__FILE__,line)(hres == S_OK, "Got 0x%08lx\n", hres);

    return client_site;
}

static HRESULT QueryInterface(REFIID,void**);

static HRESULT WINAPI OleCommandTarget_QueryInterface(IOleCommandTarget *iface,
        REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
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
    ok(cCmds == 1, "cCmds=%ld, expected 1\n", cCmds);
    ok(!pCmdText, "pCmdText != NULL\n");

    switch(prgCmds[0].cmdID) {
    case OLECMDID_STOP:
        CHECK_EXPECT2(QueryStatus_STOP);
        prgCmds[0].cmdf = OLECMDF_SUPPORTED;
        return S_OK;
    case OLECMDID_SETPROGRESSTEXT:
        CHECK_EXPECT(QueryStatus_SETPROGRESSTEXT);
        prgCmds[0].cmdf = OLECMDF_ENABLED;
        return S_OK;
    case IDM_STOP:
        /* Note:
         *  IDM_STOP is a command specific to CGID_MSHTML (not an OLECMDID command).
         *  This command appears here for the ExecWB and QueryStatusWB tests in order
         *  to help demonstrate that these routines use a NULL pguidCmdGroup.
         */
        CHECK_EXPECT(QueryStatus_IDM_STOP);
        prgCmds[0].cmdf = 0;
        return S_OK;
    default:
        ok(0, "unexpected command %ld\n", prgCmds[0].cmdID);
    }

    return E_FAIL;
}

static HRESULT WINAPI OleCommandTarget_Exec(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    if(!pguidCmdGroup) {
        switch(nCmdID) {
        case OLECMDID_SETPROGRESSMAX:
            CHECK_EXPECT2(Exec_SETPROGRESSMAX);
            ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER, "nCmdexecopts=%08lx\n", nCmdexecopt);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            if(pvaIn)
                ok(V_VT(pvaIn) == VT_I4, "V_VT(pvaIn)=%d, expected VT_I4\n", V_VT(pvaIn));
            ok(pvaOut == NULL, "pvaOut=%p, expected NULL\n", pvaOut);
            return S_OK;
        case OLECMDID_SETPROGRESSPOS:
            CHECK_EXPECT2(Exec_SETPROGRESSPOS);
            ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER, "nCmdexecopts=%08lx\n", nCmdexecopt);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            if(pvaIn)
                ok(V_VT(pvaIn) == VT_I4, "V_VT(pvaIn)=%d, expected VT_I4\n", V_VT(pvaIn));
            ok(pvaOut == NULL, "pvaOut=%p, expected NULL\n", pvaOut);
            return S_OK;
        case OLECMDID_SETDOWNLOADSTATE:
            if(is_downloading)
                ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER || !nCmdexecopt,
                   "nCmdexecopts=%08lx\n", nCmdexecopt);
            else
                ok(!nCmdexecopt, "nCmdexecopts=%08lx\n", nCmdexecopt);
            ok(pvaOut == NULL, "pvaOut=%p\n", pvaOut);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            ok(V_VT(pvaIn) == VT_I4, "V_VT(pvaIn)=%d\n", V_VT(pvaIn));
            switch(V_I4(pvaIn)) {
            case 0:
                CHECK_EXPECT2(Exec_SETDOWNLOADSTATE_0);
                break;
            case 1:
                CHECK_EXPECT2(Exec_SETDOWNLOADSTATE_1);
                break;
            default:
                ok(0, "unexpected V_I4(pvaIn)=%ld\n", V_I4(pvaIn));
            }
            return S_OK;
        case OLECMDID_UPDATECOMMANDS:
            CHECK_EXPECT2(Exec_UPDATECOMMANDS);
            ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER, "nCmdexecopts=%08lx\n", nCmdexecopt);
            ok(!pvaIn, "pvaIn != NULL\n");
            ok(!pvaOut, "pvaOut=%p, expected NULL\n", pvaOut);
            return S_OK;
        case OLECMDID_SETTITLE:
            CHECK_EXPECT(Exec_SETTITLE);
            /* TODO: test args */
            return S_OK;
        case OLECMDID_STOP:
            CHECK_EXPECT(Exec_STOP);
            return S_OK;
        case IDM_STOP:
            /* Note:
             *  IDM_STOP is a command specific to CGID_MSHTML (not an OLECMDID command).
             *  This command appears here for the ExecWB and QueryStatusWB tests in order
             *  to help demonstrate that these routines use a NULL pguidCmdGroup.
             */
            CHECK_EXPECT(Exec_IDM_STOP);
            return OLECMDERR_E_NOTSUPPORTED;
        case OLECMDID_UPDATETRAVELENTRY_DATARECOVERY:
        case OLECMDID_PAGEAVAILABLE: /* TODO (IE11) */
            return E_NOTIMPL;
        case 6058: /* TODO */
            return E_NOTIMPL;
        default:
            ok(0, "unexpected nCmdID %ld\n", nCmdID);
        }
    }else if(IsEqualGUID(&CGID_Explorer, pguidCmdGroup)) {
        switch(nCmdID) {
        case 20: /* TODO */
        case 24: /* TODO */
        case 25: /* IE5 */
        case 37: /* TODO */
        case 39: /* TODO */
        case 66: /* TODO */
        case 67: /* TODO */
        case 69: /* TODO */
        case 101: /* TODO (IE8) */
        case 109: /* TODO (IE9) */
        case 113: /* TODO (IE10) */
        case 119: /* IE11 */
            return E_FAIL;
        default:
            ok(0, "unexpected nCmdID %ld\n", nCmdID);
        }
    }else if(IsEqualGUID(&CGID_ShellDocView, pguidCmdGroup)) {
        switch(nCmdID) {
        case 63: /* win10 */
        case 105: /* TODO */
        case 132: /* win10 */
        case 133: /* IE11 */
        case 134: /* TODO (IE10) */
        case 135: /* IE11 */
        case 136: /* TODO (IE10) */
        case 137: /* win10 */
        case 138: /* TODO */
        case 140: /* TODO (Win7) */
        case 144: /* TODO */
        case 178: /* IE11 */
        case 179: /* IE11 */
        case 180: /* IE11 */
        case 181: /* IE11 */
        case 182: /* win10 */
            return E_FAIL;
        default:
            ok(0, "unexpected nCmdID %ld\n", nCmdID);
        }
    }else if(IsEqualGUID(&CGID_DocHostCmdPriv, pguidCmdGroup)) {
        switch(nCmdID) {
        case 11: /* TODO */
            break;
        default:
            ok(0, "unexpected nCmdID %ld of CGID_DocHostCmdPriv\n", nCmdID);
        }
    }else if(IsEqualGUID(&CGID_DocHostCommandHandler, pguidCmdGroup)) {
        switch(nCmdID) {
        case 6041: /* TODO */
            break;
        case 2300:
            CHECK_EXPECT(Exec_DocHostCommandHandler_2300);
            return E_NOTIMPL;
        default:
            ok(0, "unexpected nCmdID %ld of CGID_DocHostCommandHandler\n", nCmdID);
        }
    }else {
        ok(0, "unexpected pguidCmdGroup %s\n", wine_dbgstr_guid(pguidCmdGroup));
    }

    return E_FAIL;
}

static IOleCommandTargetVtbl OleCommandTargetVtbl = {
    OleCommandTarget_QueryInterface,
    OleCommandTarget_AddRef,
    OleCommandTarget_Release,
    OleCommandTarget_QueryStatus,
    OleCommandTarget_Exec
};

static IOleCommandTarget OleCommandTarget = { &OleCommandTargetVtbl };

static HRESULT WINAPI OleContainer_QueryInterface(IOleContainer *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_ITargetContainer, riid))
        return E_NOINTERFACE; /* TODO */

    if(IsEqualGUID(&IID_IOleCommandTarget, riid)) {
        if(use_container_olecmd)
        {
            *ppv = &OleCommandTarget;
            return S_OK;
        }
        else
            return E_NOINTERFACE;
    }

    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
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
        LPOLESTR pszDisplayName, ULONG *pchEaten, IMoniker **ppmkOut)
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
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
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

static HRESULT WINAPI Dispatch_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
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

static HRESULT WINAPI Dispatch_Invoke(IDispatch *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(IsEqualGUID(&IID_NULL, riid), "riid != IID_NULL\n");
    ok(pDispParams != NULL, "pDispParams == NULL\n");
    ok(pExcepInfo == NULL, "pExcepInfo=%p, expected NULL\n", pExcepInfo);
    ok(V_VT(pVarResult) == VT_EMPTY, "V_VT(pVarResult)=%d\n", V_VT(pVarResult));
    ok(wFlags == DISPATCH_PROPERTYGET, "wFlags=%08x, expected DISPATCH_PROPERTYGET\n", wFlags);
    ok(pDispParams->rgvarg == NULL, "pDispParams->rgvarg = %p\n", pDispParams->rgvarg);
    ok(pDispParams->rgdispidNamedArgs == NULL,
       "pDispParams->rgdispidNamedArgs = %p\n", pDispParams->rgdispidNamedArgs);
    ok(pDispParams->cArgs == 0, "pDispParams->cArgs = %d\n", pDispParams->cArgs);
    ok(pDispParams->cNamedArgs == 0, "pDispParams->cNamedArgs = %d\n", pDispParams->cNamedArgs);

    switch(dispIdMember) {
    case DISPID_AMBIENT_USERMODE:
        CHECK_EXPECT2(Invoke_AMBIENT_USERMODE);
        return E_FAIL;
    case DISPID_AMBIENT_DLCONTROL:
        CHECK_EXPECT(Invoke_AMBIENT_DLCONTROL);
        ok(puArgErr != NULL, "puArgErr=%p\n", puArgErr);
        return E_FAIL;
    case DISPID_AMBIENT_USERAGENT:
       CHECK_EXPECT(Invoke_AMBIENT_USERAGENT);
        ok(puArgErr != NULL, "puArgErr=%p\n", puArgErr);
        return E_FAIL;
    case DISPID_AMBIENT_PALETTE:
        CHECK_EXPECT(Invoke_AMBIENT_PALETTE);
        ok(puArgErr != NULL, "puArgErr=%p\n", puArgErr);
        return E_FAIL;
    case DISPID_AMBIENT_OFFLINEIFNOTCONNECTED:
        CHECK_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        ok(puArgErr == NULL, "puArgErr=%p\n", puArgErr);
        V_VT(pVarResult) = VT_BOOL;
        V_BOOL(pVarResult) = VARIANT_FALSE;
        return S_OK;
    case DISPID_AMBIENT_SILENT:
        CHECK_EXPECT(Invoke_AMBIENT_SILENT);
        ok(puArgErr == NULL, "puArgErr=%p\n", puArgErr);
        V_VT(pVarResult) = VT_BOOL;
        V_BOOL(pVarResult) = VARIANT_FALSE;
        return S_OK;
    }

    ok(0, "unexpected dispIdMember %ld\n", dispIdMember);
    return E_NOTIMPL;
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

static HRESULT WINAPI WebBrowserEvents2_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&DIID_DWebBrowserEvents2, riid) || IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

#define test_invoke_bool(p,s) _test_invoke_bool(__LINE__,p,s)
static void _test_invoke_bool(unsigned line, const DISPPARAMS *params, BOOL strict)
{
    ok_(__FILE__,line) (params->rgvarg != NULL, "rgvarg == NULL\n");
    ok_(__FILE__,line) (params->cArgs == 1, "cArgs=%d, expected 1\n", params->cArgs);
    ok_(__FILE__,line) (V_VT(params->rgvarg) == VT_BOOL, "V_VT(arg)=%d\n", V_VT(params->rgvarg));
    if(strict)
        ok_(__FILE__,line) (V_BOOL(params->rgvarg) == exvb, "V_VT(arg)=%x, expected %x\n",
                            V_BOOL(params->rgvarg), exvb);
    else
        ok_(__FILE__,line) (!V_BOOL(params->rgvarg) == !exvb, "V_VT(arg)=%x, expected %x\n",
                            V_BOOL(params->rgvarg), exvb);
}

static void test_OnBeforeNavigate(const VARIANT *disp, const VARIANT *url, const VARIANT *flags,
        const VARIANT *frame, const VARIANT *post_data, const VARIANT *headers, const VARIANT *cancel)
{
    BSTR str;

    ok(V_VT(disp) == VT_DISPATCH, "V_VT(disp)=%d, expected VT_DISPATCH\n", V_VT(disp));
    ok(V_DISPATCH(disp) != NULL, "V_DISPATCH(disp) == NULL\n");
    ok(V_DISPATCH(disp) == (IDispatch*)wb, "V_DISPATCH(disp)=%p, wb=%p\n", V_DISPATCH(disp), wb);

    ok(V_VT(url) == (VT_BYREF|VT_VARIANT), "V_VT(url)=%x, expected VT_BYREF|VT_VARIANT\n", V_VT(url));
    ok(V_VARIANTREF(url) != NULL, "V_VARIANTREF(url) == NULL)\n");
    if(V_VARIANTREF(url)) {
        ok(V_VT(V_VARIANTREF(url)) == VT_BSTR, "V_VT(V_VARIANTREF(url))=%d, expected VT_BSTR\n",
           V_VT(V_VARIANTREF(url)));
        ok(V_BSTR(V_VARIANTREF(url)) != NULL, "V_BSTR(V_VARIANTREF(url)) == NULL\n");
        ok(!lstrcmpW(V_BSTR(V_VARIANTREF(url)), current_url), "unexpected url %s, expected %s\n",
           wine_dbgstr_w(V_BSTR(V_VARIANTREF(url))), wine_dbgstr_w(current_url));
    }

    ok(V_VT(flags) == (VT_BYREF|VT_VARIANT), "V_VT(flags)=%x, expected VT_BYREF|VT_VARIANT\n",
       V_VT(flags));
    ok(V_VT(flags) == (VT_BYREF|VT_VARIANT), "V_VT(flags)=%x, expected VT_BYREF|VT_VARIANT\n",
       V_VT(flags));
    ok(V_VARIANTREF(flags) != NULL, "V_VARIANTREF(flags) == NULL)\n");
    if(V_VARIANTREF(flags)) {
        int f;

        ok(V_VT(V_VARIANTREF(flags)) == VT_I4, "V_VT(V_VARIANTREF(flags))=%d, expected VT_I4\n",
           V_VT(V_VARIANTREF(flags)));
        f = V_I4(V_VARIANTREF(flags));
        f &= ~0x100; /* IE11 sets this flag */
        if(is_first_load)
            ok(!f, "flags = %lx, expected 0\n", V_I4(V_VARIANTREF(flags)));
        else
            ok(!(f & ~0x40), "flags = %lx, expected 0x40 or 0\n", V_I4(V_VARIANTREF(flags)));
    }

    ok(V_VT(frame) == (VT_BYREF|VT_VARIANT), "V_VT(frame)=%x, expected VT_BYREF|VT_VARIANT\n",
       V_VT(frame));
    ok(V_VT(frame) == (VT_BYREF|VT_VARIANT), "V_VT(frame)=%x, expected VT_BYREF|VT_VARIANT\n",
       V_VT(frame));
    ok(V_VARIANTREF(frame) != NULL, "V_VARIANTREF(frame) == NULL)\n");
    if(V_VARIANTREF(frame)) {
        ok(V_VT(V_VARIANTREF(frame)) == VT_BSTR, "V_VT(V_VARIANTREF(frame))=%d, expected VT_BSTR\n",
           V_VT(V_VARIANTREF(frame)));
        ok(V_BSTR(V_VARIANTREF(frame)) == NULL, "V_BSTR(V_VARIANTREF(frame)) = %p, expected NULL\n",
           V_BSTR(V_VARIANTREF(frame)));
    }

    ok(V_VT(post_data) == (VT_BYREF|VT_VARIANT), "V_VT(post_data)=%x, expected VT_BYREF|VT_VARIANT\n",
       V_VT(post_data));
    ok(V_VT(post_data) == (VT_BYREF|VT_VARIANT), "V_VT(post_data)=%x, expected VT_BYREF|VT_VARIANT\n",
       V_VT(post_data));
    ok(V_VARIANTREF(post_data) != NULL, "V_VARIANTREF(post_data) == NULL)\n");
    if(V_VARIANTREF(post_data)) {
        ok(V_VT(V_VARIANTREF(post_data)) == (VT_VARIANT|VT_BYREF),
           "V_VT(V_VARIANTREF(post_data))=%d, expected VT_VARIANT|VT_BYREF\n",
           V_VT(V_VARIANTREF(post_data)));
        ok(V_VARIANTREF(V_VARIANTREF(post_data)) != NULL,
           "V_VARIANTREF(V_VARIANTREF(post_data)) == NULL\n");
        if(V_VARIANTREF(V_VARIANTREF(post_data))) {
            ok(V_VT(V_VARIANTREF(V_VARIANTREF(post_data))) == VT_EMPTY,
               "V_VT(V_VARIANTREF(V_VARIANTREF(post_data))) = %d, expected VT_EMPTY\n",
               V_VT(V_VARIANTREF(V_VARIANTREF(post_data))));

            if(V_VT(V_VARIANTREF(V_VARIANTREF(post_data))) == (VT_UI1|VT_ARRAY)) {
                const SAFEARRAY *sa = V_ARRAY(V_VARIANTREF(V_VARIANTREF(post_data)));

                ok(sa->cDims == 1, "sa->cDims = %d, expected 1\n", sa->cDims);
                ok(sa->fFeatures == 0, "sa->fFeatures = %d, expected 0\n", sa->fFeatures);
            }
        }
    }

    ok(V_VT(headers) == (VT_BYREF|VT_VARIANT), "V_VT(headers)=%x, expected VT_BYREF|VT_VARIANT\n",
       V_VT(headers));
    ok(V_VARIANTREF(headers) != NULL, "V_VARIANTREF(headers) == NULL)\n");
    if(V_VARIANTREF(headers)) {
        ok(V_VT(V_VARIANTREF(headers)) == VT_BSTR, "V_VT(V_VARIANTREF(headers))=%d, expected VT_BSTR\n",
           V_VT(V_VARIANTREF(headers)));
        str = V_BSTR(V_VARIANTREF(headers));
        ok(!str || !lstrcmpW(str, L"Referer: http://test.winehq.org/tests/hello.html\r\n"),
           "V_BSTR(V_VARIANTREF(headers)) = %s, expected NULL\n", wine_dbgstr_w(str));
    }

    ok(V_VT(cancel) == (VT_BYREF|VT_BOOL), "V_VT(cancel)=%x, expected VT_BYREF|VT_BOOL\n",
       V_VT(cancel));
    ok(V_BOOLREF(cancel) != NULL, "V_BOOLREF(pDispParams->rgvarg[0] == NULL)\n");
    if(V_BOOLREF(cancel))
        ok(*V_BOOLREF(cancel) == VARIANT_FALSE, "*V_BOOLREF(cancel) = %x, expected VARIANT_FALSE\n",
           *V_BOOLREF(cancel));
}

static void test_navigatecomplete2(DISPPARAMS *dp)
{
    VARIANT *v;

    CHECK_EXPECT(Invoke_NAVIGATECOMPLETE2);

    ok(dp->rgvarg != NULL, "rgvarg == NULL\n");
    ok(dp->cArgs == 2, "cArgs=%d, expected 2\n", dp->cArgs);

    ok(V_VT(dp->rgvarg) == (VT_BYREF|VT_VARIANT), "V_VT(dp->rgvarg) = %d\n", V_VT(dp->rgvarg));
    v = V_VARIANTREF(dp->rgvarg);
    ok(V_VT(v) == VT_BSTR, "V_VT(url) = %d\n", V_VT(v));
    ok(!lstrcmpW(V_BSTR(v), current_url), "url=%s, expected %s\n", wine_dbgstr_w(V_BSTR(v)),
       wine_dbgstr_w(current_url));

    ok(V_VT(dp->rgvarg+1) == VT_DISPATCH, "V_VT(dp->rgvarg+1) = %d\n", V_VT(dp->rgvarg+1));
    ok(V_DISPATCH(dp->rgvarg+1) == (IDispatch*)wb, "V_DISPATCH=%p, wb=%p\n", V_DISPATCH(dp->rgvarg+1), wb);

    if(dwl_flags & (DWL_FROM_PUT_HREF|DWL_FROM_GOBACK|DWL_FROM_GOFORWARD))
        test_ready_state(READYSTATE_COMPLETE, VARIANT_TRUE);
    else
        test_ready_state(READYSTATE_LOADING, VARIANT_TRUE);
}

static void test_documentcomplete(DISPPARAMS *dp)
{
    VARIANT *v;

    CHECK_EXPECT(Invoke_DOCUMENTCOMPLETE);

    ok(dp->rgvarg != NULL, "rgvarg == NULL\n");
    ok(dp->cArgs == 2, "cArgs=%d, expected 2\n", dp->cArgs);

    ok(V_VT(dp->rgvarg) == (VT_BYREF|VT_VARIANT), "V_VT(dp->rgvarg) = %d\n", V_VT(dp->rgvarg));
    v = V_VARIANTREF(dp->rgvarg);
    ok(V_VT(v) == VT_BSTR, "V_VT(url) = %d\n", V_VT(v));
    ok(!lstrcmpW(V_BSTR(v), current_url), "url=%s, expected %s\n", wine_dbgstr_w(V_BSTR(v)),
       wine_dbgstr_w(current_url));

    ok(V_VT(dp->rgvarg+1) == VT_DISPATCH, "V_VT(dp->rgvarg+1) = %d\n", V_VT(dp->rgvarg+1));
    ok(V_DISPATCH(dp->rgvarg+1) == (IDispatch*)wb, "V_DISPATCH=%p, wb=%p\n", V_DISPATCH(dp->rgvarg+1), wb);

    test_ready_state(READYSTATE_COMPLETE, VARIANT_FALSE);
}

static HRESULT WINAPI WebBrowserEvents2_Invoke(IDispatch *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(IsEqualGUID(&IID_NULL, riid), "riid != IID_NULL\n");
    ok(pDispParams != NULL, "pDispParams == NULL\n");
    ok(pExcepInfo == NULL, "pExcepInfo=%p, expected NULL\n", pExcepInfo);
    ok(pVarResult == NULL, "pVarResult=%p\n", pVarResult);
    ok(wFlags == DISPATCH_METHOD, "wFlags=%08x, expected DISPATCH_METHOD\n", wFlags);
    ok(pDispParams->rgdispidNamedArgs == NULL,
       "pDispParams->rgdispidNamedArgs = %p\n", pDispParams->rgdispidNamedArgs);
    ok(pDispParams->cNamedArgs == 0, "pDispParams->cNamedArgs = %d\n", pDispParams->cNamedArgs);

    switch(dispIdMember) {
    case DISPID_STATUSTEXTCHANGE:
        CHECK_EXPECT2(Invoke_STATUSTEXTCHANGE);

        ok(pDispParams->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pDispParams->cArgs == 1, "cArgs=%d, expected 1\n", pDispParams->cArgs);
        ok(V_VT(pDispParams->rgvarg) == VT_BSTR, "V_VT(pDispParams->rgvarg)=%d, expected VT_BSTR\n",
           V_VT(pDispParams->rgvarg));
        ok(V_BSTR(pDispParams->rgvarg) != NULL, "V_BSTR(pDispParams->rgvarg) is NULL\n");
        /* TODO: Check text */
        break;

    case DISPID_PROPERTYCHANGE:
        CHECK_EXPECT2(Invoke_PROPERTYCHANGE);

        ok(pDispParams->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pDispParams->cArgs == 1, "cArgs=%d, expected 1\n", pDispParams->cArgs);
        /* TODO: Check args */
        break;

    case DISPID_DOWNLOADBEGIN:
        CHECK_EXPECT2(Invoke_DOWNLOADBEGIN);

        ok(pDispParams->rgvarg == NULL, "rgvarg=%p, expected NULL\n", pDispParams->rgvarg);
        ok(pDispParams->cArgs == 0, "cArgs=%d, expected 0\n", pDispParams->cArgs);
        if(dwl_flags & (DWL_FROM_PUT_HREF|DWL_FROM_GOFORWARD|DWL_FROM_GOBACK))
            test_ready_state(READYSTATE_COMPLETE, VARIANT_TRUE);
        else if(!(dwl_flags & DWL_REFRESH)) /* todo_wine */
            test_ready_state(READYSTATE_LOADING, VARIANT_TRUE);
        break;

    case DISPID_BEFORENAVIGATE2:
        CHECK_EXPECT(Invoke_BEFORENAVIGATE2);

        ok(pDispParams->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pDispParams->cArgs == 7, "cArgs=%d, expected 7\n", pDispParams->cArgs);
        test_OnBeforeNavigate(pDispParams->rgvarg+6, pDispParams->rgvarg+5, pDispParams->rgvarg+4,
                              pDispParams->rgvarg+3, pDispParams->rgvarg+2, pDispParams->rgvarg+1,
                              pDispParams->rgvarg);
        if(dwl_flags & (DWL_FROM_PUT_HREF|DWL_FROM_GOFORWARD))
            test_ready_state(READYSTATE_COMPLETE, VARIANT_FALSE);
        else
            test_ready_state(READYSTATE_LOADING, VARIANT_FALSE);
        break;

    case DISPID_SETSECURELOCKICON:
        CHECK_EXPECT2(Invoke_SETSECURELOCKICON);

        ok(pDispParams->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pDispParams->cArgs == 1, "cArgs=%d, expected 1\n", pDispParams->cArgs);
        /* TODO: Check args */
        break;

    case DISPID_FILEDOWNLOAD:
        CHECK_EXPECT(Invoke_FILEDOWNLOAD);

        ok(pDispParams->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pDispParams->cArgs == 2, "cArgs=%d, expected 2\n", pDispParams->cArgs);
        /* TODO: Check args */
        break;

    case DISPID_COMMANDSTATECHANGE:
        ok(pDispParams->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pDispParams->cArgs == 2, "cArgs=%d, expected 2\n", pDispParams->cArgs);

        ok(V_VT(pDispParams->rgvarg) == VT_BOOL, "V_VT(pDispParams->rgvarg) = %d\n", V_VT(pDispParams->rgvarg));
        ok(V_VT(pDispParams->rgvarg+1) == VT_I4, "V_VT(pDispParams->rgvarg+1) = %d\n", V_VT(pDispParams->rgvarg+1));
        ok(V_I4(pDispParams->rgvarg+1) == CSC_NAVIGATEFORWARD ||
           V_I4(pDispParams->rgvarg+1) == CSC_NAVIGATEBACK ||
           V_I4(pDispParams->rgvarg+1) == CSC_UPDATECOMMANDS,
           "V_I4(pDispParams->rgvarg+1) = %lx\n", V_I4(pDispParams->rgvarg+1));

        if (V_I4(pDispParams->rgvarg+1) == CSC_NAVIGATEFORWARD)
        {
            todo_wine_if(nav_forward_todo) {
                if(V_BOOL(pDispParams->rgvarg))
                    CHECK_EXPECT2(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_TRUE);
                else
                    CHECK_EXPECT2(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_FALSE);
            }
        }
        else if (V_I4(pDispParams->rgvarg+1) == CSC_NAVIGATEBACK)
        {
            todo_wine_if(nav_back_todo) {
                if(V_BOOL(pDispParams->rgvarg))
                    CHECK_EXPECT2(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_TRUE);
                else
                    CHECK_EXPECT2(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_FALSE);
            }
        }
        else if (V_I4(pDispParams->rgvarg+1) == CSC_UPDATECOMMANDS)
        {
            todo_wine CHECK_EXPECT2(Invoke_COMMANDSTATECHANGE_UPDATECOMMANDS);
            set_update_commands_enable = V_BOOL(pDispParams->rgvarg);
        }
        break;

    case DISPID_DOWNLOADCOMPLETE:
        CHECK_EXPECT2(Invoke_DOWNLOADCOMPLETE);

        ok(pDispParams->rgvarg == NULL, "rgvarg=%p, expected NULL\n", pDispParams->rgvarg);
        ok(pDispParams->cArgs == 0, "cArgs=%d, expected 0\n", pDispParams->cArgs);
        if(use_container_olecmd)
            test_ready_state(READYSTATE_LOADING, VARIANT_FALSE);
        break;

    case DISPID_ONMENUBAR:
        CHECK_EXPECT(Invoke_ONMENUBAR);
        test_invoke_bool(pDispParams, TRUE);
        break;

    case DISPID_ONADDRESSBAR:
        CHECK_EXPECT(Invoke_ONADDRESSBAR);
        test_invoke_bool(pDispParams, TRUE);
        break;

    case DISPID_ONSTATUSBAR:
        CHECK_EXPECT(Invoke_ONSTATUSBAR);
        test_invoke_bool(pDispParams, TRUE);
        break;

    case DISPID_ONTOOLBAR:
        CHECK_EXPECT(Invoke_ONTOOLBAR);
        test_invoke_bool(pDispParams, FALSE);
        break;

    case DISPID_ONFULLSCREEN:
        CHECK_EXPECT(Invoke_ONFULLSCREEN);
        test_invoke_bool(pDispParams, TRUE);
        break;

    case DISPID_ONTHEATERMODE:
        CHECK_EXPECT(Invoke_ONTHEATERMODE);
        test_invoke_bool(pDispParams, TRUE);
        break;

    case DISPID_WINDOWSETRESIZABLE:
        CHECK_EXPECT(Invoke_WINDOWSETRESIZABLE);
        test_invoke_bool(pDispParams, TRUE);
        break;

    case DISPID_TITLECHANGE:
        CHECK_EXPECT2(Invoke_TITLECHANGE);
        /* FIXME */
        break;

    case DISPID_NAVIGATECOMPLETE2:
        test_navigatecomplete2(pDispParams);
        break;

    case DISPID_PROGRESSCHANGE:
        CHECK_EXPECT2(Invoke_PROGRESSCHANGE);
        /* FIXME */
        break;

    case DISPID_DOCUMENTCOMPLETE:
        test_documentcomplete(pDispParams);
        break;

    case DISPID_WINDOWCLOSING: {
        VARIANT *is_child = pDispParams->rgvarg+1, *cancel = pDispParams->rgvarg;

        CHECK_EXPECT(Invoke_WINDOWCLOSING);

        ok(pDispParams->cArgs == 2, "pdp->cArgs = %d\n", pDispParams->cArgs);
        ok(V_VT(is_child) == VT_BOOL, "V_VT(is_child) = %d\n", V_VT(is_child));
        ok(!V_BOOL(is_child), "V_BOOL(is_child) = %x\n", V_BOOL(is_child));
        ok(V_VT(cancel) == (VT_BYREF|VT_BOOL), "V_VT(cancel) = %d\n", V_VT(cancel));
        ok(!*V_BOOLREF(cancel), "*V_BOOLREF(cancel) = %x\n", *V_BOOLREF(cancel));

        *V_BOOLREF(cancel) = VARIANT_TRUE;
        return S_OK;
    }

    case DISPID_SETPHISHINGFILTERSTATUS: /* FIXME */
        CHECK_EXPECT2(Invoke_SETPHISHINGFILTERSTATUS);
        break;

    case DISPID_BEFORESCRIPTEXECUTE: /* FIXME: IE10 */
        break;

    case DISPID_PRIVACYIMPACTEDSTATECHANGE:
        break;

    default:
        ok(0, "unexpected dispIdMember %ld\n", dispIdMember);
    }

    return S_OK;
}

static IDispatchVtbl WebBrowserEvents2Vtbl = {
    WebBrowserEvents2_QueryInterface,
    Dispatch_AddRef,
    Dispatch_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    Dispatch_GetIDsOfNames,
    WebBrowserEvents2_Invoke
};

static IDispatch WebBrowserEvents2 = { &WebBrowserEvents2Vtbl };

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

    ok(ppContainer != NULL, "ppContainer == NULL\n");
    if(ppContainer)
        *ppContainer = &OleContainer;

    return S_OK;
}

static HRESULT WINAPI ClientSite_ShowObject(IOleClientSite *iface)
{
    CHECK_EXPECT(ShowObject);
    return S_OK;
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
static IOleClientSite ClientSite2 = { &ClientSiteVtbl };

static HRESULT WINAPI IOleControlSite_fnQueryInterface(IOleControlSite *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IOleControlSite_fnAddRef(IOleControlSite *iface)
{
    return 2;
}

static ULONG WINAPI IOleControlSite_fnRelease(IOleControlSite *iface)
{
    return 1;
}

static HRESULT WINAPI IOleControlSite_fnOnControlInfoChanged(IOleControlSite* This)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IOleControlSite_fnLockInPlaceActive(IOleControlSite* This,
                                                          BOOL fLock)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IOleControlSite_fnGetExtendedControl(IOleControlSite* This,
                                                           IDispatch **ppDisp)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IOleControlSite_fnTransformCoords(IOleControlSite* This,
                                                        POINTL *pPtlHimetric,
                                                        POINTF *pPtfContainer,
                                                        DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI IOleControlSite_fnTranslateAccelerator(IOleControlSite* This, MSG *pMsg,
                                                             DWORD grfModifiers)
{
    CHECK_EXPECT(ControlSite_TranslateAccelerator);
    return hr_site_TranslateAccelerator;
}

static HRESULT WINAPI IOleControlSite_fnOnFocus(IOleControlSite* This, BOOL fGotFocus)
{
    if(fGotFocus)
        CHECK_EXPECT2(OnFocus_TRUE);
    else
        CHECK_EXPECT2(OnFocus_FALSE);
    return E_NOTIMPL;
}

static HRESULT WINAPI IOleControlSite_fnShowPropertyFrame(IOleControlSite* This)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IOleControlSiteVtbl ControlSiteVtbl = {
    IOleControlSite_fnQueryInterface,
    IOleControlSite_fnAddRef,
    IOleControlSite_fnRelease,
    IOleControlSite_fnOnControlInfoChanged,
    IOleControlSite_fnLockInPlaceActive,
    IOleControlSite_fnGetExtendedControl,
    IOleControlSite_fnTransformCoords,
    IOleControlSite_fnTranslateAccelerator,
    IOleControlSite_fnOnFocus,
    IOleControlSite_fnShowPropertyFrame
};

static IOleControlSite ControlSite = { &ControlSiteVtbl };

static HRESULT WINAPI InPlaceUIWindow_QueryInterface(IOleInPlaceFrame *iface,
                                                     REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI InPlaceUIWindow_AddRef(IOleInPlaceFrame *iface)
{
    return 2;
}

static ULONG WINAPI InPlaceUIWindow_Release(IOleInPlaceFrame *iface)
{
    return 1;
}

static HRESULT WINAPI InPlaceUIWindow_GetWindow(IOleInPlaceFrame *iface, HWND *phwnd)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_GetWindow(IOleInPlaceFrame *iface, HWND *phwnd)
{
    CHECK_EXPECT(Frame_GetWindow);
    ok(phwnd != NULL, "phwnd == NULL\n");
    if(phwnd)
        *phwnd = container_hwnd;
    return S_OK;
}

static HRESULT WINAPI InPlaceUIWindow_ContextSensitiveHelp(IOleInPlaceFrame *iface, BOOL fEnterMode)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceUIWindow_GetBorder(IOleInPlaceFrame *iface, LPRECT lprectBorder)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceUIWindow_RequestBorderSpace(IOleInPlaceFrame *iface,
        LPCBORDERWIDTHS pborderwidths)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceUIWindow_SetBorderSpace(IOleInPlaceFrame *iface,
        LPCBORDERWIDTHS pborderwidths)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceUIWindow_SetActiveObject(IOleInPlaceFrame *iface,
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    CHECK_EXPECT(UIWindow_SetActiveObject);
    if(!test_close && !test_hide) {
        ok(pActiveObject != NULL, "pActiveObject = NULL\n");
        ok(!lstrcmpW(pszObjName, L"item"), "unexpected pszObjName\n");
    } else {
        ok(!pActiveObject, "pActiveObject != NULL\n");
        ok(!pszObjName, "pszObjName != NULL\n");
    }
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_SetActiveObject(IOleInPlaceFrame *iface,
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    CHECK_EXPECT(Frame_SetActiveObject);
    if(!test_close && !test_hide) {
        ok(pActiveObject != NULL, "pActiveObject = NULL\n");
        ok(!lstrcmpW(pszObjName, L"item"), "unexpected pszObjName\n");
    } else {
        ok(!pActiveObject, "pActiveObject != NULL\n");
        ok(!pszObjName, "pszObjName != NULL\n");
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
    CHECK_EXPECT(SetMenu);
    ok(hmenuShared == NULL, "hmenuShared=%p\n", hmenuShared);
    ok(holemenu == NULL, "holemenu=%p\n", holemenu);
    ok(hwndActiveObject == shell_embedding_hwnd, "hwndActiveObject=%p, expected %p\n",
       hwndActiveObject, shell_embedding_hwnd);
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_RemoveMenus(IOleInPlaceFrame *iface, HMENU hmenuShared)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetStatusText(IOleInPlaceFrame *iface, LPCOLESTR pszStatusText)
{
    CHECK_EXPECT2(SetStatusText);
    /* FIXME: Check pszStatusText */
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_EnableModeless(IOleInPlaceFrame *iface, BOOL fEnable)
{
    if(fEnable)
        CHECK_EXPECT2(EnableModeless_TRUE);
    else
        CHECK_EXPECT2(EnableModeless_FALSE);
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_TranslateAccelerator(IOleInPlaceFrame *iface, LPMSG lpmsg, WORD wID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleInPlaceFrameVtbl InPlaceUIWindowVtbl = {
    InPlaceUIWindow_QueryInterface,
    InPlaceUIWindow_AddRef,
    InPlaceUIWindow_Release,
    InPlaceUIWindow_GetWindow,
    InPlaceUIWindow_ContextSensitiveHelp,
    InPlaceUIWindow_GetBorder,
    InPlaceUIWindow_RequestBorderSpace,
    InPlaceUIWindow_SetBorderSpace,
    InPlaceUIWindow_SetActiveObject,
};

static IOleInPlaceUIWindow InPlaceUIWindow = { (IOleInPlaceUIWindowVtbl*)&InPlaceUIWindowVtbl };

static const IOleInPlaceFrameVtbl InPlaceFrameVtbl = {
    InPlaceUIWindow_QueryInterface,
    InPlaceUIWindow_AddRef,
    InPlaceUIWindow_Release,
    InPlaceFrame_GetWindow,
    InPlaceUIWindow_ContextSensitiveHelp,
    InPlaceUIWindow_GetBorder,
    InPlaceUIWindow_RequestBorderSpace,
    InPlaceUIWindow_SetBorderSpace,
    InPlaceFrame_SetActiveObject,
    InPlaceFrame_InsertMenus,
    InPlaceFrame_SetMenu,
    InPlaceFrame_RemoveMenus,
    InPlaceFrame_SetStatusText,
    InPlaceFrame_EnableModeless,
    InPlaceFrame_TranslateAccelerator
};

static IOleInPlaceFrame InPlaceFrame = { &InPlaceFrameVtbl };

static HRESULT WINAPI InPlaceSite_QueryInterface(IOleInPlaceSiteEx *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI InPlaceSite_AddRef(IOleInPlaceSiteEx *iface)
{
    return 2;
}

static ULONG WINAPI InPlaceSite_Release(IOleInPlaceSiteEx *iface)
{
    return 1;
}

static HRESULT WINAPI InPlaceSite_GetWindow(IOleInPlaceSiteEx *iface, HWND *phwnd)
{
    CHECK_EXPECT(Site_GetWindow);
    ok(phwnd != NULL, "phwnd == NULL\n");
    if(phwnd)
        *phwnd = container_hwnd;
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_ContextSensitiveHelp(IOleInPlaceSiteEx *iface, BOOL fEnterMode)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_CanInPlaceActivate(IOleInPlaceSiteEx *iface)
{
    CHECK_EXPECT(CanInPlaceActivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceActivate(IOleInPlaceSiteEx *iface)
{
    CHECK_EXPECT(OnInPlaceActivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnUIActivate(IOleInPlaceSiteEx *iface)
{
    CHECK_EXPECT(OnUIActivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_GetWindowContext(IOleInPlaceSiteEx *iface,
        IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc, LPRECT lprcPosRect,
        LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    static const RECT pos_rect = {2,1,1002,901};
    static const RECT clip_rect = {10,10,990,890};

    CHECK_EXPECT(GetWindowContext);

    ok(ppFrame != NULL, "ppFrame = NULL\n");
    if(ppFrame)
        *ppFrame = &InPlaceFrame;

    ok(ppDoc != NULL, "ppDoc = NULL\n");
    if(ppDoc)
        *ppDoc = &InPlaceUIWindow;

    ok(lprcPosRect != NULL, "lprcPosRect = NULL\n");
    if(lprcPosRect)
        memcpy(lprcPosRect, &pos_rect, sizeof(RECT));

    ok(lprcClipRect != NULL, "lprcClipRect = NULL\n");
    if(lprcClipRect)
        memcpy(lprcClipRect, &clip_rect, sizeof(RECT));

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

static HRESULT WINAPI InPlaceSite_Scroll(IOleInPlaceSiteEx *iface, SIZE scrollExtant)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnUIDeactivate(IOleInPlaceSiteEx *iface, BOOL fUndoable)
{
    CHECK_EXPECT(OnUIDeactivate);
    ok(!fUndoable, "fUndoable should be FALSE\n");
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceDeactivate(IOleInPlaceSiteEx *iface)
{
    CHECK_EXPECT(OnInPlaceDeactivate);
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_DiscardUndoState(IOleInPlaceSiteEx *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_DeactivateAndUndo(IOleInPlaceSiteEx *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnPosRectChange(IOleInPlaceSiteEx *iface, LPCRECT lprcPosRect)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceActivateEx(IOleInPlaceSiteEx *iface,
                                                      BOOL *pNoRedraw, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceDeactivateEx(IOleInPlaceSiteEx *iface,
                                                        BOOL fNoRedraw)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_RequestUIActivate(IOleInPlaceSiteEx *iface)
{
    CHECK_EXPECT2(RequestUIActivate);
    return S_OK;
}

static const IOleInPlaceSiteExVtbl InPlaceSiteExVtbl = {
    InPlaceSite_QueryInterface,
    InPlaceSite_AddRef,
    InPlaceSite_Release,
    InPlaceSite_GetWindow,
    InPlaceSite_ContextSensitiveHelp,
    InPlaceSite_CanInPlaceActivate,
    InPlaceSite_OnInPlaceActivate,
    InPlaceSite_OnUIActivate,
    InPlaceSite_GetWindowContext,
    InPlaceSite_Scroll,
    InPlaceSite_OnUIDeactivate,
    InPlaceSite_OnInPlaceDeactivate,
    InPlaceSite_DiscardUndoState,
    InPlaceSite_DeactivateAndUndo,
    InPlaceSite_OnPosRectChange,
    InPlaceSite_OnInPlaceActivateEx,
    InPlaceSite_OnInPlaceDeactivateEx,
    InPlaceSite_RequestUIActivate
};

static IOleInPlaceSiteEx InPlaceSite = { &InPlaceSiteExVtbl };

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
    ok(0, "unexpected call %ld %p %p %p\n", dwID, ppt, pcmdtReserved, pdicpReserved);
    return S_FALSE;
}

static HRESULT WINAPI DocHostUIHandler_GetHostInfo(IDocHostUIHandler2 *iface, DOCHOSTUIINFO *pInfo)
{
    CHECK_EXPECT2(GetHostInfo);
    ok(pInfo != NULL, "pInfo=NULL\n");
    if(pInfo) {
        ok(pInfo->cbSize == sizeof(DOCHOSTUIINFO) || broken(!pInfo->cbSize), "pInfo->cbSize=%lu\n", pInfo->cbSize);
        ok(!pInfo->dwFlags, "pInfo->dwFlags=%08lx, expected 0\n", pInfo->dwFlags);
        ok(!pInfo->dwDoubleClick, "pInfo->dwDoubleClick=%08lx, expected 0\n", pInfo->dwDoubleClick);
        ok(!pInfo->pchHostCss, "pInfo->pchHostCss=%p, expected NULL\n", pInfo->pchHostCss);
        ok(!pInfo->pchHostNS, "pInfo->pchhostNS=%p, expected NULL\n", pInfo->pchHostNS);
    }
    return E_FAIL;
}

static HRESULT WINAPI DocHostUIHandler_ShowUI(IDocHostUIHandler2 *iface, DWORD dwID,
        IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget,
        IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc)
{
    CHECK_EXPECT(ShowUI);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_HideUI(IDocHostUIHandler2 *iface)
{
    CHECK_EXPECT(HideUI);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_UpdateUI(IDocHostUIHandler2 *iface)
{
    CHECK_EXPECT2(UpdateUI);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_EnableModeless(IDocHostUIHandler2 *iface, BOOL fEnable)
{
    if(fEnable)
        CHECK_EXPECT(DocHost_EnableModeless_TRUE);
    else
        CHECK_EXPECT(DocHost_EnableModeless_FALSE);

    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_OnDocWindowActivate(IDocHostUIHandler2 *iface, BOOL fActivate)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_OnFrameWindowActivate(IDocHostUIHandler2 *iface, BOOL fActivate)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
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
    CHECK_EXPECT(DocHost_TranslateAccelerator);
    ok(pguidCmdGroup != NULL, "Got NULL pguidCmdGroup.\n");
    if(pguidCmdGroup)
        ok(IsEqualGUID(pguidCmdGroup, &CGID_MSHTML), "Unexpected pguidCmdGroup\n");
    ok(lpMsg != NULL, "Got NULL lpMsg.\n");
    return hr_dochost_TranslateAccelerator;
}

static HRESULT WINAPI DocHostUIHandler_GetOptionKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    CHECK_EXPECT(GetOptionKeyPath);
    ok(pchKey != NULL, "pchKey==NULL\n");
    if(pchKey)
        ok(*pchKey == NULL, "*pchKey=%p\n", *pchKey);
    ok(!dw, "dw=%lx\n", dw);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetDropTarget(IDocHostUIHandler2 *iface,
        IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
    CHECK_EXPECT(GetDropTarget);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetExternal(IDocHostUIHandler2 *iface, IDispatch **ppDispatch)
{
    CHECK_EXPECT(GetExternal);
    *ppDispatch = NULL;
    return S_FALSE;
}

static HRESULT WINAPI DocHostUIHandler_TranslateUrl(IDocHostUIHandler2 *iface, DWORD dwTranslate,
        OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
    todo_wine_if(is_downloading && !(dwl_flags & DWL_EXPECT_BEFORE_NAVIGATE))
        CHECK_EXPECT(TranslateUrl);
    return E_NOTIMPL;
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
    CHECK_EXPECT(GetOverridesKeyPath);
    ok(pchKey != NULL, "pchKey==NULL\n");
    if(pchKey)
        ok(*pchKey == NULL, "*pchKey=%p\n", *pchKey);
    ok(!dw, "dw=%lx\n", dw);
    return E_NOTIMPL;
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


static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
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

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface,
                                    REFGUID guidService, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(!winetest_interactive)
        return E_NOINTERFACE;

    if (IsEqualGUID(&SID_STopLevelBrowser, guidService))
        trace("Service SID_STopLevelBrowser\n");
    else if (IsEqualGUID(&SID_SEditCommandTarget, guidService))
        trace("Service SID_SEditCommandTarget\n");
    else if (IsEqualGUID(&IID_ITargetFrame2, guidService))
        trace("Service IID_ITargetFrame2\n");
    else if (IsEqualGUID(&SID_SInternetSecurityManager, guidService))
        trace("Service SID_SInternetSecurityManager\n");
    else if (IsEqualGUID(&SID_SOleUndoManager, guidService))
        trace("Service SID_SOleUndoManager\n");
    else if (IsEqualGUID(&SID_IMimeInfo, guidService))
        trace("Service SID_IMimeInfo\n");
    else if (IsEqualGUID(&SID_STopWindow, guidService))
        trace("Service SID_STopWindow\n");

    /* 30D02401-6A81-11D0-8274-00C04FD5AE38 Explorer Bar: Search */
    /* D1E7AFEC-6A2E-11D0-8C78-00C04FD918B4 no info */
    /* A9227C3C-7F8E-11D0-8CB0-00A0C92DBFE8 no info */
    /* 371EA634-DC5C-11D1-BA57-00C04FC2040E one reference to IVersionHost */
    /* 3050F429-98B5-11CF-BB82-00AA00BDCE0B IID_IElementBehaviorFactory */
    /* 6D12FE80-7911-11CF-9534-0000C05BAE0B SID_DefView */
    /* AD7F6C62-F6BD-11D2-959B-006097C553C8 no info */
    /* 53A2D5B1-D2FC-11D0-84E0-006097C9987D no info */
    /* 3050F312-98B5-11CF-BB82-00AA00BDCE0B HTMLFrameBaseClass */
    /* 639447BD-B2D3-44B9-9FB0-510F23CB45E4 no info */
    /* 20C46561-8491-11CF-960C-0080C7F4EE85 no info */

    else
        trace("Service %s not supported\n", wine_dbgstr_guid(guidService));

    return E_NOINTERFACE;
}


static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static IServiceProvider ServiceProvider = { &ServiceProviderVtbl };


static HRESULT QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)
            || IsEqualGUID(&IID_IOleClientSite, riid))
        *ppv = &ClientSite;
    else if(IsEqualGUID(&IID_IOleWindow, riid)
            || IsEqualGUID(&IID_IOleInPlaceSite, riid)
            || IsEqualGUID(&IID_IOleInPlaceSiteEx, riid))
        *ppv = &InPlaceSite;
    else if(IsEqualGUID(&IID_IDocHostUIHandler, riid)
            || IsEqualGUID(&IID_IDocHostUIHandler2, riid))
        *ppv = use_container_dochostui ? &DocHostUIHandler : NULL;
    else if(IsEqualGUID(&IID_IDispatch, riid))
        *ppv = &Dispatch;
    else if(IsEqualGUID(&IID_IServiceProvider, riid))
        *ppv = &ServiceProvider;
    else if(IsEqualGUID(&IID_IDocHostShowUI, riid))
        trace("interface IID_IDocHostShowUI\n");
    else if(IsEqualGUID(&IID_IOleControlSite, riid))
        *ppv = &ControlSite;
    else if(IsEqualGUID(&IID_IOleCommandTarget, riid))
        trace("interface IID_IOleCommandTarget\n");

    /* B6EA2050-048A-11D1-82B9-00C04FB9942E IAxWinHostWindow */

    else
    {
        /* are there more interfaces, that a host can support? */
        trace("%s: interface not supported\n", wine_dbgstr_guid(riid));
    }

    return (*ppv) ? S_OK : E_NOINTERFACE;
}

static LRESULT WINAPI wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static HWND create_container_window(void)
{
    static WNDCLASSEXW wndclass = {
        sizeof(WNDCLASSEXW),
        0,
        wnd_proc,
        0, 0, NULL, NULL, NULL, NULL, NULL,
        L"WebBrowserContainer",
        NULL
    };

    RegisterClassExW(&wndclass);
    return CreateWindowW(L"WebBrowserContainer", L"WebBrowserContainer",
            WS_OVERLAPPEDWINDOW, 10, 10, 600, 600, NULL, NULL, NULL, NULL);
}

static void test_DoVerb(IWebBrowser2 *unk)
{
    IOleObject *oleobj;
    RECT rect = {0,0,1000,1000};
    HRESULT hres;
    DWORD connection;

    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_OleObject) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IOleObject_Advise(oleobj, &test_sink, &connection);
    ok(hres == S_OK, "Advise failed: %08lx\n", hres);

    SET_EXPECT(CanInPlaceActivate);
    SET_EXPECT(Site_GetWindow);
    SET_EXPECT(OnInPlaceActivate);
    SET_EXPECT(GetWindowContext);
    SET_EXPECT(ShowObject);
    SET_EXPECT(GetContainer);
    SET_EXPECT(Frame_GetWindow);
    SET_EXPECT(OnUIActivate);
    SET_EXPECT(Frame_SetActiveObject);
    SET_EXPECT(UIWindow_SetActiveObject);
    SET_EXPECT(SetMenu);
    SET_EXPECT(OnFocus_TRUE);

    hres = IOleObject_DoVerb(oleobj, OLEIVERB_SHOW, NULL, &ClientSite,
                             0, (HWND)0xdeadbeef, &rect);
    ok(hres == S_OK, "DoVerb failed: %08lx\n", hres);

    CHECK_CALLED(CanInPlaceActivate);
    CHECK_CALLED(Site_GetWindow);
    CHECK_CALLED(OnInPlaceActivate);
    CHECK_CALLED(GetWindowContext);
    CHECK_CALLED(ShowObject);
    CHECK_CALLED(GetContainer);
    CHECK_CALLED(Frame_GetWindow);
    CHECK_CALLED(OnUIActivate);
    CHECK_CALLED(Frame_SetActiveObject);
    CHECK_CALLED(UIWindow_SetActiveObject);
    CHECK_CALLED(SetMenu);
    CHECK_CALLED(OnFocus_TRUE);

    hres = IOleObject_DoVerb(oleobj, OLEIVERB_SHOW, NULL, &ClientSite,
                           0, (HWND)0xdeadbeef, &rect);
    ok(hres == S_OK, "DoVerb failed: %08lx\n", hres);

    hres = IOleObject_Unadvise(oleobj, connection);
    ok(hres == S_OK, "Unadvise failed: %08lx\n", hres);

    IOleObject_Release(oleobj);
}

static void call_DoVerb(IWebBrowser2 *unk, LONG verb)
{
    IOleObject *oleobj;
    RECT rect = {60,60,600,600};
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_OleObject) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IOleObject_DoVerb(oleobj, verb, NULL, &ClientSite,
                             -1, container_hwnd, &rect);
    ok(hres == S_OK, "DoVerb failed: %08lx\n", hres);

    IOleObject_Release(oleobj);
}

static HWND get_hwnd(IWebBrowser2 *unk)
{
    IOleInPlaceObject *inplace;
    HWND hwnd;
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleInPlaceObject, (void**)&inplace);
    ok(hres == S_OK, "QueryInterface(IID_OleInPlaceObject) failed: %08lx\n", hres);

    hres = IOleInPlaceObject_GetWindow(inplace, &hwnd);
    ok(hres == S_OK, "GetWindow failed: %08lx\n", hres);

    IOleInPlaceObject_Release(inplace);
    return hwnd;
}

static void test_GetMiscStatus(IOleObject *oleobj)
{
    DWORD st, i;
    HRESULT hres;

    for(i=0; i<10; i++) {
        st = 0xdeadbeef;
        hres = IOleObject_GetMiscStatus(oleobj, i, &st);
        ok(hres == S_OK, "GetMiscStatus failed: %08lx\n", hres);
        ok(st == (OLEMISC_SETCLIENTSITEFIRST|OLEMISC_ACTIVATEWHENVISIBLE|OLEMISC_INSIDEOUT
                  |OLEMISC_CANTLINKINSIDE|OLEMISC_RECOMPOSEONRESIZE),
           "st=%08lx, expected OLEMISC_SETCLIENTSITEFIRST|OLEMISC_ACTIVATEWHENVISIBLE|"
           "OLEMISC_INSIDEOUT|OLEMISC_CANTLINKINSIDE|OLEMISC_RECOMPOSEONRESIZE)\n", st);
    }
}

static void test_SetHostNames(IOleObject *oleobj)
{
    HRESULT hres;

    hres = IOleObject_SetHostNames(oleobj, L"test app", (void*)0xdeadbeef);
    ok(hres == S_OK, "SetHostNames failed: %08lx\n", hres);
}

static void test_ClientSite(IWebBrowser2 *unk, IOleClientSite *client, BOOL stop_download)
{
    IOleObject *oleobj;
    IOleInPlaceObject *inplace;
    DWORD session_count;
    HWND hwnd;
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_OleObject) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    test_GetMiscStatus(oleobj);
    test_SetHostNames(oleobj);

    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleInPlaceObject, (void**)&inplace);
    ok(hres == S_OK, "QueryInterface(IID_OleInPlaceObject) failed: %08lx\n", hres);
    if(FAILED(hres)) {
        IOleObject_Release(oleobj);
        return;
    }

    hres = IOleInPlaceObject_GetWindow(inplace, &hwnd);
    ok(hres == S_OK, "GetWindow failed: %08lx\n", hres);
    ok((hwnd == NULL) ^ (client == NULL), "unexpected hwnd %p\n", hwnd);

    if(client) {
        session_count = pSetQueryNetSessionCount(SESSION_QUERY);

        SET_EXPECT(GetContainer);
        SET_EXPECT(Site_GetWindow);
        SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        SET_EXPECT(Invoke_AMBIENT_SILENT);
    }else if(stop_download) {
        if (use_container_olecmd) {
            SET_EXPECT(Exec_SETDOWNLOADSTATE_0);
            SET_EXPECT(Invoke_DOWNLOADCOMPLETE);
        }

        SET_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_FALSE);
        SET_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_FALSE);

        expect_update_commands_enable = 0;
        set_update_commands_enable = 0xdead;
        SET_EXPECT(Invoke_COMMANDSTATECHANGE_UPDATECOMMANDS);
    }else {
        SET_EXPECT(Invoke_STATUSTEXTCHANGE);
        SET_EXPECT(Invoke_PROGRESSCHANGE);
        nav_back_todo = nav_forward_todo = TRUE;
    }

    hres = IOleObject_SetClientSite(oleobj, client);
    ok(hres == S_OK, "SetClientSite failed: %08lx\n", hres);

    if(client) {
        DWORD count = pSetQueryNetSessionCount(SESSION_QUERY);
        ok(count >= session_count + 1, "count = %lu expected %lu\n", count, session_count + 1);

        CHECK_CALLED(GetContainer);
        CHECK_CALLED(Site_GetWindow);
        CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        CHECK_CALLED(Invoke_AMBIENT_SILENT);
    }else if(stop_download) {
        if (use_container_olecmd) {
            todo_wine CHECK_CALLED(Exec_SETDOWNLOADSTATE_0);
            todo_wine CHECK_CALLED(Invoke_DOWNLOADCOMPLETE);
        }

        CHECK_CALLED(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_FALSE);
        CHECK_CALLED(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_FALSE);

        todo_wine CHECK_CALLED(Invoke_COMMANDSTATECHANGE_UPDATECOMMANDS);
        todo_wine ok(expect_update_commands_enable == set_update_commands_enable,
           "got %d\n", set_update_commands_enable);
    }else {
        CLEAR_CALLED(Invoke_STATUSTEXTCHANGE); /* Called by IE9 */
        CLEAR_CALLED(Invoke_PROGRESSCHANGE);
        nav_back_todo = nav_forward_todo = FALSE;
    }

    hres = IOleInPlaceObject_GetWindow(inplace, &hwnd);
    ok(hres == S_OK, "GetWindow failed: %08lx\n", hres);
    ok((hwnd == NULL) == (client == NULL), "unexpected hwnd %p\n", hwnd);

    shell_embedding_hwnd = hwnd;

    test_SetHostNames(oleobj);

    IOleInPlaceObject_Release(inplace);
    IOleObject_Release(oleobj);
}

static void test_change_ClientSite(IWebBrowser2 *unk)
{
    BOOL old_use_container_olecmd = use_container_olecmd;
    IOleClientSite *doc_clientsite;
    IOleInPlaceObject *inplace;
    IOleCommandTarget *olecmd;
    IOleObject *oleobj;
    HRESULT hres;
    HWND hwnd;

    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_OleObject) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    use_container_olecmd = FALSE;

    SET_EXPECT(Site_GetWindow);
    SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    SET_EXPECT(Invoke_AMBIENT_SILENT);

    hres = IOleObject_SetClientSite(oleobj, &ClientSite2);
    ok(hres == S_OK, "SetClientSite failed: %08lx\n", hres);
    IOleObject_Release(oleobj);

    CHECK_CALLED(Site_GetWindow);
    CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    CHECK_CALLED(Invoke_AMBIENT_SILENT);

    doc_clientsite = get_dochost(unk);
    hres = IOleClientSite_QueryInterface(doc_clientsite, &IID_IOleCommandTarget, (void**)&olecmd);
    ok(hres == S_OK, "QueryInterface(IOleCommandTarget) failed: %08lx\n", hres);
    IOleClientSite_Release(doc_clientsite);

    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleInPlaceObject, (void**)&inplace);
    ok(hres == S_OK, "QueryInterface(OleInPlaceObject) failed: %08lx\n", hres);
    hres = IOleInPlaceObject_GetWindow(inplace, &hwnd);
    ok(hres == S_OK, "GetWindow failed: %08lx\n", hres);
    ok(hwnd == shell_embedding_hwnd, "unexpected hwnd %p\n", hwnd);
    IOleInPlaceObject_Release(inplace);

    if(old_use_container_olecmd) {
        SET_EXPECT(Exec_UPDATECOMMANDS);

        hres = IOleCommandTarget_Exec(olecmd, NULL, OLECMDID_UPDATECOMMANDS,
                OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
        ok(hres == S_OK, "Exec(OLECMDID_UPDATECOMMAND) failed: %08lx\n", hres);

        CHECK_CALLED(Exec_UPDATECOMMANDS);
        use_container_olecmd = TRUE;
    }

    IOleCommandTarget_Release(olecmd);
}

static void test_ClassInfo(IWebBrowser2 *unk)
{
    IProvideClassInfo2 *class_info;
    TYPEATTR *type_attr;
    ITypeInfo *typeinfo;
    GUID guid;
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(unk, &IID_IProvideClassInfo2, (void**)&class_info);
    ok(hres == S_OK, "QueryInterface(IID_IProvideClassInfo)  failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IProvideClassInfo2_GetGUID(class_info, GUIDKIND_DEFAULT_SOURCE_DISP_IID, &guid);
    ok(hres == S_OK, "GetGUID failed: %08lx\n", hres);
    ok(IsEqualGUID(wb_version > 1 ? &DIID_DWebBrowserEvents2 : &DIID_DWebBrowserEvents, &guid), "wrong guid\n");

    hres = IProvideClassInfo2_GetGUID(class_info, 0, &guid);
    ok(hres == E_FAIL, "GetGUID failed: %08lx, expected E_FAIL\n", hres);
    ok(IsEqualGUID(&IID_NULL, &guid), "wrong guid\n");

    hres = IProvideClassInfo2_GetGUID(class_info, 2, &guid);
    ok(hres == E_FAIL, "GetGUID failed: %08lx, expected E_FAIL\n", hres);
    ok(IsEqualGUID(&IID_NULL, &guid), "wrong guid\n");

    hres = IProvideClassInfo2_GetGUID(class_info, GUIDKIND_DEFAULT_SOURCE_DISP_IID, NULL);
    ok(hres == E_POINTER, "GetGUID failed: %08lx, expected E_POINTER\n", hres);

    hres = IProvideClassInfo2_GetGUID(class_info, 0, NULL);
    ok(hres == E_POINTER, "GetGUID failed: %08lx, expected E_POINTER\n", hres);

    typeinfo = NULL;
    hres = IProvideClassInfo2_GetClassInfo(class_info, &typeinfo);
    ok(hres == S_OK, "GetClassInfo failed: %08lx\n", hres);
    ok(typeinfo != NULL, "typeinfo == NULL\n");

    hres = ITypeInfo_GetTypeAttr(typeinfo, &type_attr);
    ok(hres == S_OK, "GetTypeAtr failed: %08lx\n", hres);

    ok(IsEqualGUID(&type_attr->guid, wb_version > 1 ? &CLSID_WebBrowser : &CLSID_WebBrowser_V1),
       "guid = %s\n", wine_dbgstr_guid(&type_attr->guid));

    ITypeInfo_ReleaseTypeAttr(typeinfo, type_attr);
    ITypeInfo_Release(typeinfo);

    IProvideClassInfo2_Release(class_info);
}

#define expect_oleverb(a,b) _expect_oleverb(__LINE__,a,b)
static void _expect_oleverb(unsigned line, const OLEVERB *verb, LONG exverb)
{
    ok_(__FILE__,line)(verb->lVerb == exverb, "verb->lVerb = %ld, expected %ld\n", verb->lVerb, exverb);
    ok_(__FILE__,line)(!verb->lpszVerbName, "verb->lpszVerbName = %s\n", wine_dbgstr_w(verb->lpszVerbName));
    ok_(__FILE__,line)(!verb->fuFlags, "verb->fuFlags = %lx\n", verb->fuFlags);
    ok_(__FILE__,line)(!verb->grfAttribs, "verb->grfAttribs = %lx\n", verb->grfAttribs);
}

#define test_next_oleverb(a,b) _test_next_oleverb(__LINE__,a,b)
static void _test_next_oleverb(unsigned line, IEnumOLEVERB *enum_verbs, LONG exverb)
{
    ULONG fetched = 0xdeadbeef;
    OLEVERB verb;
    HRESULT hres;

    fetched = 0xdeadbeef;
    memset(&verb, 0xa, sizeof(verb));
    hres = IEnumOLEVERB_Next(enum_verbs, 1, &verb, &fetched);
    ok_(__FILE__,line)(hres == S_OK, "Next failed: %08lx\n", hres);
    ok_(__FILE__,line)(!fetched, "fetched = %ld\n", fetched);
    _expect_oleverb(line, &verb, exverb);
}

static void test_EnumVerbs(IWebBrowser2 *wb)
{
    IEnumOLEVERB *enum_verbs;
    IOleObject *oleobj;
    OLEVERB verbs[20];
    ULONG fetched;
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(wb, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_IOleObject) failed: %08lx\n", hres);

    hres = IOleObject_EnumVerbs(oleobj, &enum_verbs);
    IOleObject_Release(oleobj);
    ok(hres == S_OK, "EnumVerbs failed: %08lx\n", hres);
    ok(enum_verbs != NULL, "enum_verbs == NULL\n");

    fetched = 0xdeadbeef;
    memset(verbs, 0xa, sizeof(verbs));
    verbs[1].lVerb = 0xdeadbeef;
    hres = IEnumOLEVERB_Next(enum_verbs, ARRAY_SIZE(verbs), verbs, &fetched);
    ok(hres == S_OK, "Next failed: %08lx\n", hres);
    ok(!fetched, "fetched = %ld\n", fetched);
    /* Although fetched==0, an element is returned. */
    expect_oleverb(verbs, OLEIVERB_PRIMARY);
    /* The first argument is ignored and always one element is returned. */
    ok(verbs[1].lVerb == 0xdeadbeef, "verbs[1].lVerb = %lx\n", verbs[1].lVerb);

    test_next_oleverb(enum_verbs, OLEIVERB_INPLACEACTIVATE);
    test_next_oleverb(enum_verbs, OLEIVERB_UIACTIVATE);
    test_next_oleverb(enum_verbs, OLEIVERB_SHOW);
    test_next_oleverb(enum_verbs, OLEIVERB_HIDE);

    /* There is anouther verb, returned correctly. */
    fetched = 0xdeadbeef;
    memset(verbs, 0xa, sizeof(verbs));
    verbs[0].lVerb = 0xdeadbeef;
    hres = IEnumOLEVERB_Next(enum_verbs, ARRAY_SIZE(verbs), verbs, &fetched);
    todo_wine ok(hres == S_OK, "Next failed: %08lx\n", hres);
    todo_wine ok(fetched == 1, "fetched = %ld\n", fetched);
    todo_wine ok(verbs[0].lVerb != 0xdeadbeef, "verbs[0].lVerb = %lx\n", verbs[0].lVerb);

    hres = IEnumOLEVERB_Next(enum_verbs, ARRAY_SIZE(verbs), verbs, &fetched);
    ok(hres == S_FALSE, "Next failed: %08lx\n", hres);
    ok(!fetched, "fetched = %ld\n", fetched);

    hres = IEnumOLEVERB_Reset(enum_verbs);
    ok(hres == S_OK, "Reset failed: %08lx\n", hres);

    fetched = 0xdeadbeef;
    hres = IEnumOLEVERB_Next(enum_verbs, 1, verbs, &fetched);
    ok(hres == S_OK, "Next failed: %08lx\n", hres);
    ok(!fetched, "fetched = %ld\n", fetched);

    hres = IEnumOLEVERB_Next(enum_verbs, 1, verbs, NULL);
    ok(hres == S_OK, "Next failed: %08lx\n", hres);

    hres = IEnumOLEVERB_Skip(enum_verbs, 20);
    ok(hres == S_OK, "Reset failed: %08lx\n", hres);

    fetched = 0xdeadbeef;
    hres = IEnumOLEVERB_Next(enum_verbs, 1, verbs, &fetched);
    ok(hres == S_OK, "Next failed: %08lx\n", hres);
    ok(!fetched, "fetched = %ld\n", fetched);

    test_next_oleverb(enum_verbs, OLEIVERB_SHOW);

    IEnumOLEVERB_Release(enum_verbs);
}

static void test_Persist(IWebBrowser2 *wb, int version)
{
    IPersist *persist;
    GUID guid;
    HRESULT hr;

    hr = IWebBrowser2_QueryInterface(wb, &IID_IPersist, (void **)&persist);
    ok(hr == S_OK, "QueryInterface(IID_IPersist) failed: %08lx\n", hr);

    hr = IPersist_GetClassID(persist, &guid);
    ok(hr == S_OK, "GetClassID failed: %08lx\n", hr);
    if (version == 1)
        ok(IsEqualGUID(&guid, &CLSID_WebBrowser_V1), "got wrong CLSID: %s\n", wine_dbgstr_guid(&guid));
    else
        ok(IsEqualGUID(&guid, &CLSID_WebBrowser), "got wrong CLSID: %s\n", wine_dbgstr_guid(&guid));

    IPersist_Release(persist);
}

static void test_OleObject(IWebBrowser2 *wb, int version)
{
    IOleObject *oleobj;
    CLSID clsid;
    HRESULT hr;

    hr = IWebBrowser2_QueryInterface(wb, &IID_IOleObject, (void **)&oleobj);
    ok(hr == S_OK, "QueryInterface(IID_IOleObject) failed: %08lx\n", hr);

    hr = IOleObject_GetUserClassID(oleobj, &clsid);
    ok(hr == S_OK, "GetUserClassID failed: %08lx\n", hr);
    if (version == 1)
        ok(IsEqualGUID(&clsid, &CLSID_WebBrowser_V1), "got wrong CLSID: %s\n", wine_dbgstr_guid(&clsid));
    else
        ok(IsEqualGUID(&clsid, &CLSID_WebBrowser), "got wrong CLSID: %s\n", wine_dbgstr_guid(&clsid));

    IOleObject_Release(oleobj);
}

static void test_ie_funcs(IWebBrowser2 *wb)
{
    IDispatch *disp;
    VARIANT_BOOL b;
    int i;
    SHANDLE_PTR hwnd;
    HRESULT hres;
    BSTR sName;

    /* HWND */

    hwnd = 0xdeadbeef;
    hres = IWebBrowser2_get_HWND(wb, &hwnd);
    ok(hres == E_FAIL, "get_HWND failed: %08lx, expected E_FAIL\n", hres);
    ok(hwnd == 0, "unexpected hwnd %p\n", (PVOID)hwnd);

    /* MenuBar */

    hres = IWebBrowser2_get_MenuBar(wb, &b);
    ok(hres == S_OK, "get_MenuBar failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONMENUBAR);
    hres = IWebBrowser2_put_MenuBar(wb, (exvb = VARIANT_FALSE));
    ok(hres == S_OK, "put_MenuBar failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_ONMENUBAR);

    hres = IWebBrowser2_get_MenuBar(wb, &b);
    ok(hres == S_OK, "get_MenuBar failed: %08lx\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONMENUBAR);
    hres = IWebBrowser2_put_MenuBar(wb, (exvb = 100));
    ok(hres == S_OK, "put_MenuBar failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_ONMENUBAR);

    hres = IWebBrowser2_get_MenuBar(wb, &b);
    ok(hres == S_OK, "get_MenuBar failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    /* AddressBar */

    hres = IWebBrowser2_get_AddressBar(wb, &b);
    ok(hres == S_OK, "get_AddressBar failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONADDRESSBAR);
    hres = IWebBrowser2_put_AddressBar(wb, (exvb = VARIANT_FALSE));
    ok(hres == S_OK, "put_AddressBar failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_ONADDRESSBAR);

    hres = IWebBrowser2_get_AddressBar(wb, &b);
    ok(hres == S_OK, "get_MenuBar failed: %08lx\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONADDRESSBAR);
    hres = IWebBrowser2_put_AddressBar(wb, (exvb = 100));
    ok(hres == S_OK, "put_AddressBar failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_ONADDRESSBAR);

    hres = IWebBrowser2_get_AddressBar(wb, &b);
    ok(hres == S_OK, "get_AddressBar failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONADDRESSBAR);
    hres = IWebBrowser2_put_AddressBar(wb, (exvb = VARIANT_TRUE));
    ok(hres == S_OK, "put_MenuBar failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_ONADDRESSBAR);

    /* StatusBar */

    hres = IWebBrowser2_get_StatusBar(wb, &b);
    ok(hres == S_OK, "get_StatusBar failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONSTATUSBAR);
    hres = IWebBrowser2_put_StatusBar(wb, (exvb = VARIANT_TRUE));
    ok(hres == S_OK, "put_StatusBar failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_ONSTATUSBAR);

    hres = IWebBrowser2_get_StatusBar(wb, &b);
    ok(hres == S_OK, "get_StatusBar failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONSTATUSBAR);
    hres = IWebBrowser2_put_StatusBar(wb, (exvb = VARIANT_FALSE));
    ok(hres == S_OK, "put_StatusBar failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_ONSTATUSBAR);

    hres = IWebBrowser2_get_StatusBar(wb, &b);
    ok(hres == S_OK, "get_StatusBar failed: %08lx\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONSTATUSBAR);
    hres = IWebBrowser2_put_StatusBar(wb, (exvb = 100));
    ok(hres == S_OK, "put_StatusBar failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_ONSTATUSBAR);

    hres = IWebBrowser2_get_StatusBar(wb, &b);
    ok(hres == S_OK, "get_StatusBar failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    /* ToolBar */

    hres = IWebBrowser2_get_ToolBar(wb, &i);
    ok(hres == S_OK, "get_ToolBar failed: %08lx\n", hres);
    ok(i, "i=%x\n", i);

    SET_EXPECT(Invoke_ONTOOLBAR);
    hres = IWebBrowser2_put_ToolBar(wb, (exvb = VARIANT_FALSE));
    ok(hres == S_OK, "put_ToolBar failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_ONTOOLBAR);

    hres = IWebBrowser2_get_ToolBar(wb, &i);
    ok(hres == S_OK, "get_ToolBar failed: %08lx\n", hres);
    ok(i == VARIANT_FALSE, "b=%x\n", i);

    SET_EXPECT(Invoke_ONTOOLBAR);
    hres = IWebBrowser2_put_ToolBar(wb, (exvb = 100));
    ok(hres == S_OK, "put_ToolBar failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_ONTOOLBAR);

    hres = IWebBrowser2_get_ToolBar(wb, &i);
    ok(hres == S_OK, "get_ToolBar failed: %08lx\n", hres);
    ok(i, "i=%x\n", i);

    SET_EXPECT(Invoke_ONTOOLBAR);
    hres = IWebBrowser2_put_ToolBar(wb, (exvb = VARIANT_TRUE));
    ok(hres == S_OK, "put_ToolBar failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_ONTOOLBAR);

    /* FullScreen */

    hres = IWebBrowser2_get_FullScreen(wb, &b);
    ok(hres == S_OK, "get_FullScreen failed: %08lx\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONFULLSCREEN);
    hres = IWebBrowser2_put_FullScreen(wb, (exvb = VARIANT_TRUE));
    ok(hres == S_OK, "put_FullScreen failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_ONFULLSCREEN);

    hres = IWebBrowser2_get_FullScreen(wb, &b);
    ok(hres == S_OK, "get_FullScreen failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONFULLSCREEN);
    hres = IWebBrowser2_put_FullScreen(wb, (exvb = 100));
    ok(hres == S_OK, "put_FullScreen failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_ONFULLSCREEN);

    hres = IWebBrowser2_get_FullScreen(wb, &b);
    ok(hres == S_OK, "get_FullScreen failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONFULLSCREEN);
    hres = IWebBrowser2_put_FullScreen(wb, (exvb = VARIANT_FALSE));
    ok(hres == S_OK, "put_FullScreen failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_ONFULLSCREEN);

    /* TheaterMode */

    hres = IWebBrowser2_get_TheaterMode(wb, &b);
    ok(hres == S_OK, "get_TheaterMode failed: %08lx\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONTHEATERMODE);
    hres = IWebBrowser2_put_TheaterMode(wb, (exvb = VARIANT_TRUE));
    ok(hres == S_OK, "put_TheaterMode failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_ONTHEATERMODE);

    hres = IWebBrowser2_get_TheaterMode(wb, &b);
    ok(hres == S_OK, "get_TheaterMode failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONTHEATERMODE);
    hres = IWebBrowser2_put_TheaterMode(wb, (exvb = 100));
    ok(hres == S_OK, "put_TheaterMode failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_ONTHEATERMODE);

    hres = IWebBrowser2_get_TheaterMode(wb, &b);
    ok(hres == S_OK, "get_TheaterMode failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONTHEATERMODE);
    hres = IWebBrowser2_put_TheaterMode(wb, (exvb = VARIANT_FALSE));
    ok(hres == S_OK, "put_TheaterMode failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_ONTHEATERMODE);

    /* Resizable */

    b = 0x100;
    hres = IWebBrowser2_get_Resizable(wb, &b);
    ok(hres == E_NOTIMPL, "get_Resizable failed: %08lx\n", hres);
    ok(b == 0x100, "b=%x\n", b);

    SET_EXPECT(Invoke_WINDOWSETRESIZABLE);
    hres = IWebBrowser2_put_Resizable(wb, (exvb = VARIANT_TRUE));
    ok(hres == S_OK, "put_Resizable failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_WINDOWSETRESIZABLE);

    SET_EXPECT(Invoke_WINDOWSETRESIZABLE);
    hres = IWebBrowser2_put_Resizable(wb, (exvb = VARIANT_FALSE));
    ok(hres == S_OK, "put_Resizable failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_WINDOWSETRESIZABLE);

    hres = IWebBrowser2_get_Resizable(wb, &b);
    ok(hres == E_NOTIMPL, "get_Resizable failed: %08lx\n", hres);
    ok(b == 0x100, "b=%x\n", b);

    /* Application */

    disp = NULL;
    hres = IWebBrowser2_get_Application(wb, &disp);
    ok(hres == S_OK, "get_Application failed: %08lx\n", hres);
    ok(disp == (void*)wb, "disp=%p, expected %p\n", disp, wb);
    if(disp)
        IDispatch_Release(disp);

    hres = IWebBrowser2_get_Application(wb, NULL);
    ok(hres == E_POINTER, "get_Application failed: %08lx, expected E_POINTER\n", hres);

    /* Name */
    hres = IWebBrowser2_get_Name(wb, &sName);
    ok(hres == S_OK, "getName failed: %08lx, expected S_OK\n", hres);
    if (is_lang_english())
        ok(!lstrcmpW(sName, L"Microsoft Web Browser Control"), "got '%s', expected 'Microsoft Web Browser Control'\n", wine_dbgstr_w(sName));
    else /* Non-English cannot be blank. */
        ok(sName!=NULL, "get_Name return a NULL string.\n");
    SysFreeString(sName);

    /* RegisterAsDropTarget */
    hres = IWebBrowser2_get_RegisterAsDropTarget(wb, NULL);
    ok(hres == E_INVALIDARG, "get_RegisterAsDropTarget returned: %08lx\n", hres);

    /* Quit */

    hres = IWebBrowser2_Quit(wb);
    ok(hres == E_FAIL, "Quit failed: %08lx, expected E_FAIL\n", hres);
}

static void test_Silent(IWebBrowser2 *wb, IOleControl *control, BOOL is_clientsite)
{
    VARIANT_BOOL b;
    HRESULT hres;

    b = 100;
    hres = IWebBrowser2_get_Silent(wb, &b);
    ok(hres == S_OK, "get_Silent failed: %08lx\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    hres = IWebBrowser2_put_Silent(wb, VARIANT_TRUE);
    ok(hres == S_OK, "set_Silent failed: %08lx\n", hres);

    b = 100;
    hres = IWebBrowser2_get_Silent(wb, &b);
    ok(hres == S_OK, "get_Silent failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    hres = IWebBrowser2_put_Silent(wb, 100);
    ok(hres == S_OK, "set_Silent failed: %08lx\n", hres);

    b = 100;
    hres = IWebBrowser2_get_Silent(wb, &b);
    ok(hres == S_OK, "get_Silent failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    hres = IWebBrowser2_put_Silent(wb, VARIANT_FALSE);
    ok(hres == S_OK, "set_Silent failed: %08lx\n", hres);

    b = 100;
    hres = IWebBrowser2_get_Silent(wb, &b);
    ok(hres == S_OK, "get_Silent failed: %08lx\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    if(is_clientsite) {
        hres = IWebBrowser2_put_Silent(wb, VARIANT_TRUE);
        ok(hres == S_OK, "set_Silent failed: %08lx\n", hres);

        SET_EXPECT(Invoke_AMBIENT_SILENT);
    }

    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_AMBIENT_SILENT);
    ok(hres == S_OK, "OnAmbientPropertyChange failed %08lx\n", hres);

    if(is_clientsite)
        CHECK_CALLED(Invoke_AMBIENT_SILENT);

    b = 100;
    hres = IWebBrowser2_get_Silent(wb, &b);
    ok(hres == S_OK, "get_Silent failed: %08lx\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);
}

static void test_Offline(IWebBrowser2 *wb, IOleControl *control, BOOL is_clientsite)
{
    VARIANT_BOOL b;
    HRESULT hres;

    b = 100;
    hres = IWebBrowser2_get_Offline(wb, &b);
    ok(hres == S_OK, "get_Offline failed: %08lx\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    hres = IWebBrowser2_put_Offline(wb, VARIANT_TRUE);
    ok(hres == S_OK, "set_Offline failed: %08lx\n", hres);

    b = 100;
    hres = IWebBrowser2_get_Offline(wb, &b);
    ok(hres == S_OK, "get_Offline failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    hres = IWebBrowser2_put_Offline(wb, 100);
    ok(hres == S_OK, "set_Offline failed: %08lx\n", hres);

    b = 100;
    hres = IWebBrowser2_get_Offline(wb, &b);
    ok(hres == S_OK, "get_Offline failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    hres = IWebBrowser2_put_Offline(wb, VARIANT_FALSE);
    ok(hres == S_OK, "set_Offline failed: %08lx\n", hres);

    b = 100;
    hres = IWebBrowser2_get_Offline(wb, &b);
    ok(hres == S_OK, "get_Offline failed: %08lx\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    if(is_clientsite) {
        hres = IWebBrowser2_put_Offline(wb, VARIANT_TRUE);
        ok(hres == S_OK, "set_Offline failed: %08lx\n", hres);

        SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    }

    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_AMBIENT_OFFLINEIFNOTCONNECTED);
    ok(hres == S_OK, "OnAmbientPropertyChange failed %08lx\n", hres);

    if(is_clientsite)
        CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);

    b = 100;
    hres = IWebBrowser2_get_Offline(wb, &b);
    ok(hres == S_OK, "get_Offline failed: %08lx\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);
}

static void test_ambient_unknown(IWebBrowser2 *wb, IOleControl *control, BOOL is_clientsite)
{
    HRESULT hres;

    SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    SET_EXPECT(Invoke_AMBIENT_SILENT);
    SET_EXPECT(Invoke_AMBIENT_USERMODE);
    SET_EXPECT(Invoke_AMBIENT_DLCONTROL);
    SET_EXPECT(Invoke_AMBIENT_USERAGENT);
    SET_EXPECT(Invoke_AMBIENT_PALETTE);

    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_UNKNOWN);
    ok(hres == S_OK, "OnAmbientPropertyChange failed %08lx\n", hres);

    CHECK_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    CHECK_EXPECT(Invoke_AMBIENT_SILENT);
    CHECK_EXPECT(Invoke_AMBIENT_USERMODE);
    CHECK_EXPECT(Invoke_AMBIENT_DLCONTROL);
    CHECK_EXPECT(Invoke_AMBIENT_USERAGENT);
    CHECK_EXPECT(Invoke_AMBIENT_PALETTE);

    CLEAR_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    CLEAR_CALLED(Invoke_AMBIENT_SILENT);
    CLEAR_CALLED(Invoke_AMBIENT_USERMODE);
    CLEAR_CALLED(Invoke_AMBIENT_DLCONTROL);
    CLEAR_CALLED(Invoke_AMBIENT_USERAGENT);
    CLEAR_CALLED(Invoke_AMBIENT_PALETTE);
}

static void test_wb_funcs(IWebBrowser2 *wb, BOOL is_clientsite)
{
    IOleControl *control;
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(wb, &IID_IOleControl, (void**)&control);
    ok(hres == S_OK, "Could not get IOleControl interface: %08lx\n", hres);

    test_Silent(wb, control, is_clientsite);
    test_Offline(wb, control, is_clientsite);
    test_ambient_unknown(wb, control, is_clientsite);

    IOleControl_Release(control);
}

static void test_GetControlInfo(IWebBrowser2 *unk)
{
    IOleControl *control;
    CONTROLINFO info;
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleControl, (void**)&control);
    ok(hres == S_OK, "Could not get IOleControl: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IOleControl_GetControlInfo(control, &info);
    ok(hres == E_NOTIMPL, "GetControlInfo failed: %08lx, expected E_NOTIMPL\n", hres);
    hres = IOleControl_GetControlInfo(control, NULL);
    ok(hres == E_NOTIMPL, "GetControlInfo failed: %08lx, expected E_NOTIMPL\n", hres);

    IOleControl_Release(control);
}

static void test_Extent(IWebBrowser2 *unk)
{
    IOleObject *oleobj;
    SIZE size, expected;
    HRESULT hres;
    DWORD dpi_x;
    DWORD dpi_y;
    HDC hdc;

    /* default aspect ratio is 96dpi / 96dpi */
    hdc = GetDC(0);
    dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
    dpi_y = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(0, hdc);
    if (dpi_x != 96 || dpi_y != 96)
        trace("dpi: %ld / %ld\n", dpi_y, dpi_y);

    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "Could not get IOleObject: %08lx\n", hres);
    if(FAILED(hres))
        return;

    size.cx = size.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, DVASPECT_CONTENT, &size);
    ok(hres == S_OK, "GetExtent failed: %08lx\n", hres);
    /* Default size is 50x20 pixels, in himetric units */
    expected.cx = MulDiv( 50, 2540, dpi_x );
    expected.cy = MulDiv( 20, 2540, dpi_y );
    ok(size.cx == expected.cx && size.cy == expected.cy, "size = {%ld %ld} (expected %ld %ld)\n",
       size.cx, size.cy, expected.cx, expected.cy );

    size.cx = 800;
    size.cy = 700;
    hres = IOleObject_SetExtent(oleobj, DVASPECT_CONTENT, &size);
    ok(hres == S_OK, "SetExtent failed: %08lx\n", hres);

    size.cx = size.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, DVASPECT_CONTENT, &size);
    ok(hres == S_OK, "GetExtent failed: %08lx\n", hres);
    ok(size.cx == 800 && size.cy == 700, "size = {%ld %ld}\n", size.cx, size.cy);

    size.cx = size.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, 0, &size);
    ok(hres == S_OK, "GetExtent failed: %08lx\n", hres);
    ok(size.cx == 800 && size.cy == 700, "size = {%ld %ld}\n", size.cx, size.cy);

    size.cx = 900;
    size.cy = 800;
    hres = IOleObject_SetExtent(oleobj, 0, &size);
    ok(hres == S_OK, "SetExtent failed: %08lx\n", hres);

    size.cx = size.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, 0, &size);
    ok(hres == S_OK, "GetExtent failed: %08lx\n", hres);
    ok(size.cx == 900 && size.cy == 800, "size = {%ld %ld}\n", size.cx, size.cy);

    size.cx = size.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, 0xdeadbeef, &size);
    ok(hres == S_OK, "GetExtent failed: %08lx\n", hres);
    ok(size.cx == 900 && size.cy == 800, "size = {%ld %ld}\n", size.cx, size.cy);

    size.cx = 1000;
    size.cy = 900;
    hres = IOleObject_SetExtent(oleobj, 0xdeadbeef, &size);
    ok(hres == S_OK, "SetExtent failed: %08lx\n", hres);

    size.cx = size.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, 0xdeadbeef, &size);
    ok(hres == S_OK, "GetExtent failed: %08lx\n", hres);
    ok(size.cx == 1000 && size.cy == 900, "size = {%ld %ld}\n", size.cx, size.cy);

    size.cx = size.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, DVASPECT_CONTENT, &size);
    ok(hres == S_OK, "GetExtent failed: %08lx\n", hres);
    ok(size.cx == 1000 && size.cy == 900, "size = {%ld %ld}\n", size.cx, size.cy);

    IOleObject_Release(oleobj);
}

static void test_ConnectionPoint(IWebBrowser2 *unk, BOOL init)
{
    IConnectionPointContainer *container;
    IConnectionPoint *point;
    HRESULT hres;

    static DWORD dw = 100;

    hres = IWebBrowser2_QueryInterface(unk, &IID_IConnectionPointContainer, (void**)&container);
    ok(hres == S_OK, "QueryInterface(IID_IConnectionPointContainer) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IConnectionPointContainer_FindConnectionPoint(container, &DIID_DWebBrowserEvents2, &point);
    IConnectionPointContainer_Release(container);
    ok(hres == S_OK, "FindConnectionPoint failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    if(init) {
        hres = IConnectionPoint_Advise(point, (IUnknown*)&WebBrowserEvents2, &dw);
        ok(hres == S_OK, "Advise failed: %08lx\n", hres);
        ok(dw == 1, "dw=%ld, expected 1\n", dw);
    }else {
        hres = IConnectionPoint_Unadvise(point, dw);
        ok(hres == S_OK, "Unadvise failed: %08lx\n", hres);
    }

    IConnectionPoint_Release(point);
}

static void test_Navigate2(IWebBrowser2 *webbrowser, const WCHAR *nav_url)
{
    const WCHAR *title = L"WineHQ - Run Windows applications on Linux, BSD, Solaris and Mac OS X";
    VARIANT url;
    BOOL is_file;
    HRESULT hres;

    test_LocationURL(webbrowser, is_first_load ? L"" : current_url);
    test_LocationName(webbrowser, is_first_load ? L"" : (is_http ? title : current_url));
    test_ready_state(is_first_load ? READYSTATE_UNINITIALIZED : READYSTATE_COMPLETE, VARIANT_FALSE);

    is_http = !memcmp(nav_url, "http:", 5);
    V_VT(&url) = VT_BSTR;
    V_BSTR(&url) = SysAllocString(current_url = nav_url);

    if((is_file = !wcsnicmp(nav_url, L"file://", 7)))
        current_url = nav_url + 7;

    if(is_first_load) {
        SET_EXPECT(Invoke_AMBIENT_USERMODE);
        SET_EXPECT(Invoke_PROPERTYCHANGE);
        SET_EXPECT(Invoke_BEFORENAVIGATE2);
        SET_EXPECT(Invoke_DOWNLOADBEGIN);
        if (use_container_olecmd) SET_EXPECT(Exec_SETDOWNLOADSTATE_1);
        SET_EXPECT(EnableModeless_FALSE);
        SET_EXPECT(Invoke_STATUSTEXTCHANGE);
        SET_EXPECT(SetStatusText);
        if(use_container_dochostui)
            SET_EXPECT(GetHostInfo);
        SET_EXPECT(Invoke_AMBIENT_DLCONTROL);
        SET_EXPECT(Invoke_AMBIENT_USERAGENT);
        SET_EXPECT(Invoke_AMBIENT_PALETTE);
        if(use_container_dochostui) {
            SET_EXPECT(GetOptionKeyPath);
            SET_EXPECT(GetOverridesKeyPath);
        }
        if (use_container_olecmd) {
            SET_EXPECT(QueryStatus_SETPROGRESSTEXT);
            SET_EXPECT(Exec_SETPROGRESSMAX);
            SET_EXPECT(Exec_SETPROGRESSPOS);
        }
        SET_EXPECT(Invoke_SETSECURELOCKICON);
        SET_EXPECT(Invoke_FILEDOWNLOAD);
        if (use_container_olecmd) SET_EXPECT(Exec_SETDOWNLOADSTATE_0);
        SET_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_FALSE);
        SET_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_FALSE);
        SET_EXPECT(EnableModeless_TRUE);
        if (!use_container_olecmd) SET_EXPECT(Invoke_DOWNLOADCOMPLETE);
        if (is_file) SET_EXPECT(Invoke_PROGRESSCHANGE);
    }

    hres = IWebBrowser2_Navigate2(webbrowser, &url, NULL, NULL, NULL, NULL);
    ok(hres == S_OK, "Navigate2 failed: %08lx\n", hres);

    if(is_first_load) {
        CHECK_CALLED(Invoke_AMBIENT_USERMODE);
        todo_wine CHECK_CALLED(Invoke_PROPERTYCHANGE);
        CHECK_CALLED(Invoke_BEFORENAVIGATE2);
        CHECK_CALLED(Invoke_DOWNLOADBEGIN);
        if (use_container_olecmd) todo_wine CHECK_CALLED(Exec_SETDOWNLOADSTATE_1);
        CHECK_CALLED(EnableModeless_FALSE);
        CHECK_CALLED(Invoke_STATUSTEXTCHANGE);
        CHECK_CALLED(SetStatusText);
        if(use_container_dochostui)
            CHECK_CALLED(GetHostInfo);
        CHECK_CALLED(Invoke_AMBIENT_DLCONTROL);
        CHECK_CALLED(Invoke_AMBIENT_USERAGENT);
        CLEAR_CALLED(Invoke_AMBIENT_PALETTE); /* Not called by IE9 */
        if(use_container_dochostui) {
            CLEAR_CALLED(GetOptionKeyPath);
            CHECK_CALLED(GetOverridesKeyPath);
        }
        if (use_container_olecmd) {
            todo_wine CHECK_CALLED(QueryStatus_SETPROGRESSTEXT);
            todo_wine CHECK_CALLED(Exec_SETPROGRESSMAX);
            todo_wine CHECK_CALLED(Exec_SETPROGRESSPOS);
        }
        todo_wine CHECK_CALLED(Invoke_SETSECURELOCKICON);
        todo_wine CHECK_CALLED(Invoke_FILEDOWNLOAD);
        CHECK_CALLED(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_FALSE);
        CHECK_CALLED(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_FALSE);
        if (use_container_olecmd) todo_wine CHECK_CALLED(Exec_SETDOWNLOADSTATE_0);
        CHECK_CALLED(EnableModeless_TRUE);
        if (!use_container_olecmd) CHECK_CALLED(Invoke_DOWNLOADCOMPLETE);
        if (is_file) todo_wine CHECK_CALLED(Invoke_PROGRESSCHANGE);
    }

    VariantClear(&url);

    test_ready_state(READYSTATE_LOADING, VARIANT_FALSE);
}

static void test_QueryStatusWB(IWebBrowser2 *webbrowser, BOOL has_document)
{
    HRESULT hres, success_state;
    OLECMDF success_flags;
    enum OLECMDF status;

    /*
     * We can tell the difference between the custom container target and the built-in target
     * since the custom target returns OLECMDF_SUPPORTED instead of OLECMDF_ENABLED.
     */
    if (use_container_olecmd)
        success_flags = OLECMDF_SUPPORTED;
    else
        success_flags = OLECMDF_ENABLED;

    /* When no target is available (no document or custom target) an error is returned */
    if (has_document)
        success_state = S_OK;
    else
        success_state = E_UNEXPECTED;

    /*
     * Test a safe operation that exists as both a high-numbered MSHTML id and an OLECMDID.
     * These tests show that QueryStatusWB uses a NULL pguidCmdGroup, since OLECMDID_STOP
     * is enabled and IDM_STOP is not.
     */
    status = 0xdeadbeef;
    if (use_container_olecmd) SET_EXPECT(QueryStatus_STOP);
    hres = IWebBrowser2_QueryStatusWB(webbrowser, OLECMDID_STOP, &status);
    ok(hres == success_state, "QueryStatusWB failed: %08lx %08lx\n", hres, success_state);
    todo_wine_if (!use_container_olecmd && has_document)
        ok((has_document && status == success_flags) || (!has_document && status == 0xdeadbeef),
           "OLECMDID_STOP not enabled/supported: %08x %08x\n", status, success_flags);
    status = 0xdeadbeef;
    if (use_container_olecmd) SET_EXPECT(QueryStatus_IDM_STOP);
    hres = IWebBrowser2_QueryStatusWB(webbrowser, IDM_STOP, &status);
    ok(hres == success_state, "QueryStatusWB failed: %08lx %08lx\n", hres, success_state);
    ok((has_document && status == 0) || (!has_document && status == 0xdeadbeef),
       "IDM_STOP is enabled/supported: %08x %d\n", status, has_document);
}

static void test_ExecWB(IWebBrowser2 *webbrowser, BOOL has_document)
{
    HRESULT hres, olecmdid_state, idm_state;

    /* When no target is available (no document or custom target) an error is returned */
    if (has_document)
    {
        olecmdid_state = S_OK;
        idm_state = OLECMDERR_E_NOTSUPPORTED;
    }
    else
    {
        olecmdid_state = E_UNEXPECTED;
        idm_state = E_UNEXPECTED;
    }

    /*
     * Test a safe operation that exists as both a high-numbered MSHTML id and an OLECMDID.
     * These tests show that QueryStatusWB uses a NULL pguidCmdGroup, since OLECMDID_STOP
     * succeeds (S_OK) and IDM_STOP does not (OLECMDERR_E_NOTSUPPORTED).
     */
    if(use_container_olecmd) {
        SET_EXPECT(Exec_STOP);
    }else if(has_document) {
        SET_EXPECT(Invoke_STATUSTEXTCHANGE);
        SET_EXPECT(SetStatusText);
    }
    hres = IWebBrowser2_ExecWB(webbrowser, OLECMDID_STOP, OLECMDEXECOPT_DONTPROMPTUSER, 0, 0);
    if(!use_container_olecmd && has_document) {
        todo_wine ok(hres == olecmdid_state, "ExecWB failed: %08lx %08lx\n", hres, olecmdid_state);
        CLEAR_CALLED(Invoke_STATUSTEXTCHANGE); /* Called by IE9 */
        CLEAR_CALLED(SetStatusText); /* Called by IE9 */
    }else {
        ok(hres == olecmdid_state, "ExecWB failed: %08lx %08lx\n", hres, olecmdid_state);
    }
    if (use_container_olecmd)
        SET_EXPECT(Exec_IDM_STOP);
    hres = IWebBrowser2_ExecWB(webbrowser, IDM_STOP, OLECMDEXECOPT_DONTPROMPTUSER, 0, 0);
    ok(hres == idm_state, "ExecWB failed: %08lx %08lx\n", hres, idm_state);
}

static void test_download(DWORD flags)
{
    BOOL *b = &called_Invoke_DOCUMENTCOMPLETE;
    MSG msg;

    if(flags & DWL_REFRESH)
        b = use_container_olecmd ? &called_Exec_SETDOWNLOADSTATE_0 : &called_Invoke_DOWNLOADCOMPLETE;
    else if((flags & DWL_FROM_PUT_HREF) && !use_container_olecmd && 0)
        b = &called_Invoke_DOWNLOADCOMPLETE;

    is_downloading = TRUE;
    dwl_flags = flags;

    if(flags & (DWL_FROM_PUT_HREF|DWL_FROM_GOBACK|DWL_FROM_GOFORWARD|DWL_REFRESH))
        test_ready_state(READYSTATE_COMPLETE, VARIANT_FALSE);
    else
        test_ready_state(READYSTATE_LOADING, VARIANT_FALSE);

    if(flags & (DWL_EXPECT_BEFORE_NAVIGATE|DWL_FROM_GOFORWARD|DWL_REFRESH))
        SET_EXPECT(Invoke_PROPERTYCHANGE);

    if(flags & DWL_EXPECT_BEFORE_NAVIGATE) {
        SET_EXPECT(Invoke_BEFORENAVIGATE2);
        SET_EXPECT(TranslateUrl);
    }
    if(use_container_olecmd) {
        SET_EXPECT(Exec_SETPROGRESSMAX);
        SET_EXPECT(Exec_SETPROGRESSPOS);
        SET_EXPECT(Exec_SETDOWNLOADSTATE_1);
    }else {
        SET_EXPECT(Invoke_DOWNLOADBEGIN);
    }
    SET_EXPECT(DocHost_EnableModeless_FALSE);
    SET_EXPECT(DocHost_EnableModeless_TRUE);
    SET_EXPECT(Invoke_SETSECURELOCKICON);
    SET_EXPECT(Invoke_SETPHISHINGFILTERSTATUS);
    SET_EXPECT(EnableModeless_FALSE);

    if(!(flags & DWL_REFRESH)) {
        if(flags & (DWL_FROM_GOFORWARD|DWL_BACK_ENABLE))
            SET_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_TRUE);
        else
            SET_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_FALSE);
        if(flags & DWL_FROM_GOBACK)
            SET_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_TRUE);
        else
            SET_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_FALSE);
    }

    expect_update_commands_enable = 0;
    set_update_commands_enable = 0xdead;
    SET_EXPECT(Invoke_COMMANDSTATECHANGE_UPDATECOMMANDS);

    SET_EXPECT(Invoke_STATUSTEXTCHANGE);
    SET_EXPECT(Invoke_PROPERTYCHANGE);
    SET_EXPECT(SetStatusText);
    SET_EXPECT(EnableModeless_TRUE);
    if(!is_first_load)
        SET_EXPECT(GetHostInfo);
    if(use_container_olecmd)
        SET_EXPECT(Exec_SETDOWNLOADSTATE_0);
    else
        SET_EXPECT(Invoke_DOWNLOADCOMPLETE);
    SET_EXPECT(Invoke_TITLECHANGE);
    if(!(flags & DWL_REFRESH))
        SET_EXPECT(Invoke_NAVIGATECOMPLETE2);
    if(is_first_load)
        SET_EXPECT(GetDropTarget);
    SET_EXPECT(Invoke_PROGRESSCHANGE);
    if(!(flags & DWL_REFRESH))
        SET_EXPECT(Invoke_DOCUMENTCOMPLETE);

    if(flags & DWL_HTTP)
        SET_EXPECT(Exec_SETTITLE);
    SET_EXPECT(UpdateUI);
    if(use_container_olecmd)
        SET_EXPECT(Exec_UPDATECOMMANDS);
    SET_EXPECT(QueryStatus_STOP);
    SET_EXPECT(GetOverridesKeyPath); /* Called randomly on some VMs. */

    trace("Downloading...\n");
    while(!*b && GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if(flags & DWL_EXPECT_BEFORE_NAVIGATE)
        todo_wine CHECK_CALLED(Invoke_PROPERTYCHANGE);
    else if(flags & (DWL_FROM_GOFORWARD|DWL_REFRESH))
        CLEAR_CALLED(Invoke_PROPERTYCHANGE); /* called by IE11 */

    if(flags & DWL_EXPECT_BEFORE_NAVIGATE) {
        CHECK_CALLED(Invoke_BEFORENAVIGATE2);
        CHECK_CALLED(TranslateUrl);
    }
    if(use_container_olecmd) {
        todo_wine CHECK_CALLED(Exec_SETPROGRESSMAX);
        todo_wine CHECK_CALLED(Exec_SETPROGRESSPOS);
        CHECK_CALLED(Exec_SETDOWNLOADSTATE_1);
    }else {
        SET_EXPECT(Invoke_DOWNLOADBEGIN);
    }
    CLEAR_CALLED(DocHost_EnableModeless_FALSE); /* IE 7 */
    CLEAR_CALLED(DocHost_EnableModeless_TRUE); /* IE 7 */
    todo_wine CHECK_CALLED(Invoke_SETSECURELOCKICON);
    CLEAR_CALLED(Invoke_SETPHISHINGFILTERSTATUS); /* IE 7 */
    if(is_first_load)
        todo_wine CHECK_CALLED(EnableModeless_FALSE);
    else
        CLEAR_CALLED(EnableModeless_FALSE); /* IE 8 */

    if(!(flags & DWL_REFRESH)) {
        todo_wine_if(nav_back_todo) {
            if(flags & (DWL_FROM_GOFORWARD|DWL_BACK_ENABLE))
                CHECK_CALLED(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_TRUE);
            else
                CHECK_CALLED(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_FALSE);
        }
        if(flags & DWL_FROM_GOBACK)
            CHECK_CALLED(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_TRUE);
        else
            CHECK_CALLED(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_FALSE);
    }
    CLEAR_CALLED(Invoke_COMMANDSTATECHANGE_UPDATECOMMANDS);
    todo_wine CHECK_CALLED(Invoke_STATUSTEXTCHANGE);
    todo_wine CHECK_CALLED(SetStatusText);
    if(is_first_load)
        todo_wine CHECK_CALLED(EnableModeless_TRUE);
    else
        CLEAR_CALLED(EnableModeless_FALSE); /* IE 8 */
    if(!is_first_load)
        todo_wine CHECK_CALLED(GetHostInfo);
    if(use_container_olecmd)
        CHECK_CALLED(Exec_SETDOWNLOADSTATE_0);
    else
        CHECK_CALLED(Invoke_DOWNLOADCOMPLETE);
    todo_wine CHECK_CALLED(Invoke_TITLECHANGE);
    if(!(flags & DWL_REFRESH))
        CHECK_CALLED(Invoke_NAVIGATECOMPLETE2);
    if(is_first_load)
        todo_wine CHECK_CALLED(GetDropTarget);
    if(!(flags & DWL_REFRESH))
        CHECK_CALLED(Invoke_DOCUMENTCOMPLETE);

    is_downloading = FALSE;

    test_ready_state(READYSTATE_COMPLETE, VARIANT_FALSE);

    while(use_container_olecmd && !called_Exec_UPDATECOMMANDS && GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    todo_wine CHECK_CALLED(Invoke_PROGRESSCHANGE);
    if(flags & DWL_HTTP)
        CLEAR_CALLED(Exec_SETTITLE); /* FIXME: make it more strict */
    if(use_container_olecmd)
        CHECK_CALLED(UpdateUI);
    else
        CLEAR_CALLED(UpdateUI);
    if(use_container_olecmd)
        CHECK_CALLED(Exec_UPDATECOMMANDS);
    CLEAR_CALLED(QueryStatus_STOP);
    CLEAR_CALLED(GetOverridesKeyPath);
}

static void test_Refresh(IWebBrowser2 *webbrowser, BOOL use_refresh2)
{
    HRESULT hres;

    trace("Refresh...\n");

    if(use_container_olecmd)
        SET_EXPECT(Exec_DocHostCommandHandler_2300);

    if(use_refresh2) {
        VARIANT v;

        V_VT(&v) = VT_I4;
        V_I4(&v) = REFRESH_NORMAL;

        hres = IWebBrowser2_Refresh2(webbrowser, &v);
        ok(hres == S_OK, "Refresh failed: %08lx\n", hres);
    }else {
        hres = IWebBrowser2_Refresh(webbrowser);
        ok(hres == S_OK, "Refresh failed: %08lx\n", hres);
    }

    if(use_container_olecmd)
        CHECK_CALLED(Exec_DocHostCommandHandler_2300);

    test_download(DWL_REFRESH);
}

static void test_olecmd(IWebBrowser2 *unk, BOOL loaded)
{
    IOleCommandTarget *cmdtrg;
    OLECMD cmds[3];
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleCommandTarget, (void**)&cmdtrg);
    ok(hres == S_OK, "Could not get IOleCommandTarget iface: %08lx\n", hres);
    if(FAILED(hres))
        return;

    cmds[0].cmdID = OLECMDID_SPELL;
    cmds[0].cmdf = 0xdeadbeef;
    cmds[1].cmdID = OLECMDID_REFRESH;
    cmds[1].cmdf = 0xdeadbeef;
    hres = IOleCommandTarget_QueryStatus(cmdtrg, NULL, 2, cmds, NULL);
    if(loaded) {
        ok(hres == S_OK, "QueryStatus failed: %08lx\n", hres);
        ok(cmds[0].cmdf == OLECMDF_SUPPORTED, "OLECMDID_SPELL cmdf = %lx\n", cmds[0].cmdf);
        ok(cmds[1].cmdf == (OLECMDF_ENABLED|OLECMDF_SUPPORTED),
           "OLECMDID_REFRESH cmdf = %lx\n", cmds[1].cmdf);
    }else {
        ok(hres == 0x80040104, "QueryStatus failed: %08lx\n", hres);
        ok(cmds[0].cmdf == 0xdeadbeef, "OLECMDID_SPELL cmdf = %lx\n", cmds[0].cmdf);
        ok(cmds[1].cmdf == 0xdeadbeef, "OLECMDID_REFRESH cmdf = %lx\n", cmds[0].cmdf);
    }

    IOleCommandTarget_Release(cmdtrg);
}

static void test_IServiceProvider(IWebBrowser2 *unk)
{
    IServiceProvider *servprov = (void*)0xdeadbeef;
    IUnknown *iface;
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(unk, &IID_IServiceProvider, (void**)&servprov);
    ok(hres == S_OK, "QueryInterface returned %08lx, expected S_OK\n", hres);
    if(FAILED(hres))
        return;

    hres = IServiceProvider_QueryService(servprov, &SID_STopLevelBrowser, &IID_IBrowserService2, (LPVOID*)&iface);
    ok(hres == E_FAIL, "QueryService returned %08lx, expected E_FAIL\n", hres);
    ok(!iface, "QueryService returned %p, expected NULL\n", iface);
    if(hres == S_OK)
        IUnknown_Release(iface);

    hres = IServiceProvider_QueryService(servprov, &SID_SHTMLWindow, &IID_IHTMLWindow2, (LPVOID*)&iface);
    ok(hres == S_OK, "QueryService returned %08lx, expected S_OK\n", hres);
    ok(iface != NULL, "QueryService returned NULL\n");
    if(hres == S_OK)
        IUnknown_Release(iface);

    IServiceProvider_Release(servprov);
}

static void test_put_href(IWebBrowser2 *unk, const WCHAR *url)
{
    IHTMLLocation *location;
    IHTMLDocument2 *doc;
    BSTR str;
    HRESULT hres;

    trace("put_href(%s)...\n", wine_dbgstr_w(url));

    doc = get_document(unk);

    location = NULL;
    hres = IHTMLDocument2_get_location(doc, &location);
    IHTMLDocument2_Release(doc);
    ok(hres == S_OK, "get_location failed: %08lx\n", hres);
    ok(location != NULL, "location == NULL\n");

    is_http = !wcsncmp(url, L"http:", 5);

    SET_EXPECT(TranslateUrl);
    SET_EXPECT(Invoke_BEFORENAVIGATE2);
    SET_EXPECT(Invoke_PROPERTYCHANGE);

    dwl_flags = DWL_FROM_PUT_HREF;

    str = SysAllocString(current_url = url);
    is_first_load = FALSE;
    hres = IHTMLLocation_put_href(location, str);

    CHECK_CALLED(TranslateUrl);
    CHECK_CALLED(Invoke_BEFORENAVIGATE2);
    if(!is_http)
        todo_wine CHECK_CALLED(Invoke_PROPERTYCHANGE);

    IHTMLLocation_Release(location);
    SysFreeString(str);
    ok(hres == S_OK, "put_href failed: %08lx\n", hres);

    test_ready_state(READYSTATE_COMPLETE, VARIANT_FALSE);
}

static void test_go_back(IWebBrowser2 *wb, const WCHAR *back_url, int back_enable, int forward_enable, int forward_todo)
{
    HRESULT hres;

    current_url = back_url;

    SET_EXPECT(Invoke_BEFORENAVIGATE2);

    if(back_enable)
        SET_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_TRUE);
    else
        SET_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_FALSE);

    if(forward_enable)
        SET_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_TRUE);
    else
        SET_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_FALSE);

    nav_forward_todo = forward_todo;
    SET_EXPECT(Invoke_PROPERTYCHANGE);
    hres = IWebBrowser2_GoBack(wb);
    ok(hres == S_OK, "GoBack failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_BEFORENAVIGATE2);
    nav_forward_todo = FALSE;

    todo_wine_if(nav_back_todo) {
        if(back_enable)
            CHECK_CALLED(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_TRUE);
        else
            CHECK_CALLED(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_FALSE);
    }

    todo_wine_if(forward_todo) {
        if(forward_enable)
            CHECK_CALLED(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_TRUE);
        else
            CHECK_CALLED(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_FALSE);
    }

    CLEAR_CALLED(Invoke_PROPERTYCHANGE); /* called by IE11 */
}

static void test_go_forward(IWebBrowser2 *wb, const WCHAR *forward_url, int back_enable, int forward_enable)
{
    HRESULT hres;

    current_url = forward_url;
    dwl_flags |= DWL_FROM_GOFORWARD;

    SET_EXPECT(Invoke_BEFORENAVIGATE2);

    if(back_enable)
        SET_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_TRUE);
    else
        SET_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_FALSE);

    if(forward_enable)
        SET_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_TRUE);
    else
        SET_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_FALSE);

    hres = IWebBrowser2_GoForward(wb);
    ok(hres == S_OK, "GoForward failed: %08lx\n", hres);
    CHECK_CALLED(Invoke_BEFORENAVIGATE2);

    if(back_enable)
        CHECK_CALLED(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_TRUE);
    else
        CHECK_CALLED(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_FALSE);

    if(forward_enable)
        CHECK_CALLED(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_TRUE);
    else
        CHECK_CALLED(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_FALSE);
}

static void test_QueryInterface(IWebBrowser2 *wb)
{
    IQuickActivate *qa = (IQuickActivate*)0xdeadbeef;
    IRunnableObject *runnable = (IRunnableObject*)0xdeadbeef;
    IPerPropertyBrowsing *propbrowse = (void*)0xdeadbeef;
    IOleInPlaceSite *inplace = (void*)0xdeadbeef;
    IOleCache *cache = (void*)0xdeadbeef;
    IObjectWithSite *site = (void*)0xdeadbeef;
    IViewObjectEx *viewex = (void*)0xdeadbeef;
    IOleLink *link = (void*)0xdeadbeef;
    IMarshal *marshal = (void*)0xdeadbeef;
    IUnknown *unk = (IUnknown*)wb;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IQuickActivate, (void**)&qa);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08lx, expected E_NOINTERFACE\n", hres);
    ok(qa == NULL, "qa=%p, expected NULL\n", qa);

    hres = IUnknown_QueryInterface(unk, &IID_IRunnableObject, (void**)&runnable);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08lx, expected E_NOINTERFACE\n", hres);
    ok(runnable == NULL, "runnable=%p, expected NULL\n", runnable);

    hres = IUnknown_QueryInterface(unk, &IID_IPerPropertyBrowsing, (void**)&propbrowse);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08lx, expected E_NOINTERFACE\n", hres);
    ok(propbrowse == NULL, "propbrowse=%p, expected NULL\n", propbrowse);

    hres = IUnknown_QueryInterface(unk, &IID_IOleCache, (void**)&cache);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08lx, expected E_NOINTERFACE\n", hres);
    ok(cache == NULL, "cache=%p, expected NULL\n", cache);

    hres = IUnknown_QueryInterface(unk, &IID_IOleInPlaceSite, (void**)&inplace);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08lx, expected E_NOINTERFACE\n", hres);
    ok(inplace == NULL, "inplace=%p, expected NULL\n", inplace);

    hres = IUnknown_QueryInterface(unk, &IID_IObjectWithSite, (void**)&site);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08lx, expected E_NOINTERFACE\n", hres);
    ok(site == NULL, "site=%p, expected NULL\n", site);

    hres = IUnknown_QueryInterface(unk, &IID_IViewObjectEx, (void**)&viewex);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08lx, expected E_NOINTERFACE\n", hres);
    ok(viewex == NULL, "viewex=%p, expected NULL\n", viewex);

    hres = IUnknown_QueryInterface(unk, &IID_IOleLink, (void**)&link);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08lx, expected E_NOINTERFACE\n", hres);
    ok(link == NULL, "link=%p, expected NULL\n", link);

    hres = IUnknown_QueryInterface(unk, &IID_IMarshal, (void**)&marshal);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08lx, expected E_NOINTERFACE\n", hres);
    ok(marshal == NULL, "marshal=%p, expected NULL\n", marshal);

}

static void test_UIActivate(IWebBrowser2 *unk, BOOL activate)
{
    IOleDocumentView *docview;
    IHTMLDocument2 *doc;
    HRESULT hres;

    doc = get_document(unk);

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleDocumentView, (void**)&docview);
    ok(hres == S_OK, "Got 0x%08lx\n", hres);
    if(SUCCEEDED(hres)) {
        if(activate) {
            SET_EXPECT(RequestUIActivate);
            SET_EXPECT(ShowUI);
            SET_EXPECT(HideUI);
            SET_EXPECT(OnFocus_FALSE);
        }

        hres = IOleDocumentView_UIActivate(docview, activate);
        todo_wine_if(activate)
            ok(hres == S_OK, "Got 0x%08lx\n", hres);

        if(activate) {
            todo_wine {
                CHECK_CALLED(RequestUIActivate);
                CHECK_CALLED(ShowUI);
                CHECK_CALLED(HideUI);
                CHECK_CALLED(OnFocus_FALSE);
            }
        }

        IOleDocumentView_Release(docview);
    }

    IHTMLDocument2_Release(doc);
}

static void test_external(IWebBrowser2 *unk)
{
    IDocHostUIHandler2 *dochost;
    IOleClientSite *client;
    IDispatch *disp;
    HRESULT hres;

    client = get_dochost(unk);

    hres = IOleClientSite_QueryInterface(client, &IID_IDocHostUIHandler2, (void**)&dochost);
    ok(hres == S_OK, "Could not get IDocHostUIHandler2 iface: %08lx\n", hres);
    IOleClientSite_Release(client);

    if(use_container_dochostui)
        SET_EXPECT(GetExternal);
    disp = (void*)0xdeadbeef;
    hres = IDocHostUIHandler2_GetExternal(dochost, &disp);
    if(use_container_dochostui) {
        CHECK_CALLED(GetExternal);
        ok(hres == S_FALSE, "GetExternal failed: %08lx\n", hres);
        ok(!disp, "disp = %p\n", disp);
    }else {
        IShellUIHelper *uihelper;

        ok(hres == S_OK, "GetExternal failed: %08lx\n", hres);
        ok(disp != NULL, "disp == NULL\n");

        hres = IDispatch_QueryInterface(disp, &IID_IShellUIHelper, (void**)&uihelper);
        ok(hres == S_OK, "Could not get IShellUIHelper iface: %08lx\n", hres);
        IShellUIHelper_Release(uihelper);
        IDispatch_Release(disp);
    }

    IDocHostUIHandler2_Release(dochost);
}

static void test_htmlwindow_close(IWebBrowser2 *wb)
{
    IHTMLWindow2 *window;
    IHTMLDocument2 *doc;
    HRESULT hres;

    doc = get_document(wb);

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok(hres == S_OK, "get_parentWindow failed: %08lx\n", hres);
    IHTMLDocument2_Release(doc);

    SET_EXPECT(Invoke_WINDOWCLOSING);

    hres = IHTMLWindow2_close(window);
    ok(hres == S_OK, "close failed: %08lx\n", hres);

    CHECK_CALLED(Invoke_WINDOWCLOSING);

    IHTMLWindow2_Release(window);
}

static void test_TranslateAccelerator(IWebBrowser2 *unk)
{
    IOleClientSite *doc_clientsite;
    IOleInPlaceActiveObject *pao;
    HRESULT hres;
    DWORD keycode;
    MSG msg_a = {
        container_hwnd,
        0, 0, 0,
        GetTickCount(),
        {5, 5}
    };

    test_Navigate2(unk, L"about:blank");

    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleInPlaceActiveObject, (void**)&pao);
    ok(hres == S_OK, "Got 0x%08lx\n", hres);
    if(SUCCEEDED(hres)) {
        /* One accelerator that should be handled by mshtml */
        msg_a.message = WM_KEYDOWN;
        msg_a.wParam = VK_F1;
        hres = IOleInPlaceActiveObject_TranslateAccelerator(pao, &msg_a);
        ok(hres == S_FALSE, "Got 0x%08lx (%04x::%02Ix)\n", hres, msg_a.message, msg_a.wParam);

        /* And one that should not */
        msg_a.message = WM_KEYDOWN;
        msg_a.wParam = VK_F5;
        hres = IOleInPlaceActiveObject_TranslateAccelerator(pao, &msg_a);
        ok(hres == S_FALSE, "Got 0x%08lx (%04x::%02Ix)\n", hres, msg_a.message, msg_a.wParam);

        IOleInPlaceActiveObject_Release(pao);
    }

    test_UIActivate(unk, TRUE);

    /* Test again after UIActivate */
    hres = IWebBrowser2_QueryInterface(unk, &IID_IOleInPlaceActiveObject, (void**)&pao);
    ok(hres == S_OK, "Got 0x%08lx\n", hres);
    if(SUCCEEDED(hres)) {
        /* One accelerator that should be handled by mshtml */
        msg_a.message = WM_KEYDOWN;
        msg_a.wParam = VK_F1;
        SET_EXPECT(DocHost_TranslateAccelerator);
        SET_EXPECT(ControlSite_TranslateAccelerator);
        hres = IOleInPlaceActiveObject_TranslateAccelerator(pao, &msg_a);
        ok(hres == S_FALSE, "Got 0x%08lx (%04x::%02Ix)\n", hres, msg_a.message, msg_a.wParam);
        todo_wine CHECK_CALLED(DocHost_TranslateAccelerator);
        todo_wine CHECK_CALLED(ControlSite_TranslateAccelerator);

        /* And one that should not */
        msg_a.message = WM_KEYDOWN;
        msg_a.wParam = VK_F5;
        SET_EXPECT(DocHost_TranslateAccelerator);
        hres = IOleInPlaceActiveObject_TranslateAccelerator(pao, &msg_a);
        todo_wine ok(hres == S_OK, "Got 0x%08lx (%04x::%02Ix)\n", hres, msg_a.message, msg_a.wParam);
        todo_wine CHECK_CALLED(DocHost_TranslateAccelerator);

        IOleInPlaceActiveObject_Release(pao);
    }

    doc_clientsite = get_dochost(unk);
    if(doc_clientsite) {
        IDocHostUIHandler2 *dochost;
        IOleControlSite *doc_controlsite;
        IUnknown *unk_test;

        hres = IOleClientSite_QueryInterface(doc_clientsite, &IID_IOleInPlaceFrame, (void**)&unk_test);
        ok(hres == E_NOINTERFACE, "Got 0x%08lx\n", hres);
        if(SUCCEEDED(hres)) IUnknown_Release(unk_test);

        hres = IOleClientSite_QueryInterface(doc_clientsite, &IID_IDocHostShowUI, (void**)&unk_test);
        todo_wine ok(hres == S_OK, "Got 0x%08lx\n", hres);
        if(SUCCEEDED(hres)) IUnknown_Release(unk_test);

        hres = IOleClientSite_QueryInterface(doc_clientsite, &IID_IDocHostUIHandler, (void**)&unk_test);
        ok(hres == S_OK, "Got 0x%08lx\n", hres);
        if(SUCCEEDED(hres)) IUnknown_Release(unk_test);

        hres = IOleClientSite_QueryInterface(doc_clientsite, &IID_IDocHostUIHandler2, (void**)&dochost);
        ok(hres == S_OK, "Got 0x%08lx\n", hres);
        if(SUCCEEDED(hres)) {
            msg_a.message = WM_KEYDOWN;
            hr_dochost_TranslateAccelerator = 0xdeadbeef;
            for(keycode = 0; keycode <= 0x100; keycode++) {
                msg_a.wParam = keycode;
                SET_EXPECT(DocHost_TranslateAccelerator);
                hres = IDocHostUIHandler2_TranslateAccelerator(dochost, &msg_a, &CGID_MSHTML, 1234);
                ok(hres == 0xdeadbeef, "Got 0x%08lx\n", hres);
                CHECK_CALLED(DocHost_TranslateAccelerator);
            }
            hr_dochost_TranslateAccelerator = E_NOTIMPL;

            SET_EXPECT(DocHost_TranslateAccelerator);
            hres = IDocHostUIHandler2_TranslateAccelerator(dochost, &msg_a, &CGID_MSHTML, 1234);
            ok(hres == E_NOTIMPL, "Got 0x%08lx\n", hres);
            CHECK_CALLED(DocHost_TranslateAccelerator);

            IDocHostUIHandler2_Release(dochost);
        }

        hres = IOleClientSite_QueryInterface(doc_clientsite, &IID_IOleControlSite, (void**)&doc_controlsite);
        ok(hres == S_OK, "Got 0x%08lx\n", hres);
        if(SUCCEEDED(hres)) {
            msg_a.message = WM_KEYDOWN;
            hr_site_TranslateAccelerator = 0xdeadbeef;
            for(keycode = 0; keycode < 0x100; keycode++) {
                msg_a.wParam = keycode;
                SET_EXPECT(ControlSite_TranslateAccelerator);
                hres = IOleControlSite_TranslateAccelerator(doc_controlsite, &msg_a, 0);
                if(keycode == 0x9 || keycode == 0x75)
                    todo_wine ok(hres == S_OK, "Got 0x%08lx (keycode: %04lx)\n", hres, keycode);
                else
                    ok(hres == S_FALSE, "Got 0x%08lx (keycode: %04lx)\n", hres, keycode);

                CHECK_CALLED(ControlSite_TranslateAccelerator);
            }
            msg_a.wParam = VK_LEFT;
            SET_EXPECT(ControlSite_TranslateAccelerator);
            hres = IOleControlSite_TranslateAccelerator(doc_controlsite, &msg_a, 0);
            ok(hres == S_FALSE, "Got 0x%08lx (keycode: %04lx)\n", hres, keycode);
            CHECK_CALLED(ControlSite_TranslateAccelerator);

            hr_site_TranslateAccelerator = S_OK;
            SET_EXPECT(ControlSite_TranslateAccelerator);
            hres = IOleControlSite_TranslateAccelerator(doc_controlsite, &msg_a, 0);
            ok(hres == S_OK, "Got 0x%08lx (keycode: %04lx)\n", hres, keycode);
            CHECK_CALLED(ControlSite_TranslateAccelerator);

            hr_site_TranslateAccelerator = E_NOTIMPL;
            SET_EXPECT(ControlSite_TranslateAccelerator);
            hres = IOleControlSite_TranslateAccelerator(doc_controlsite, &msg_a, 0);
            ok(hres == S_FALSE, "Got 0x%08lx (keycode: %04lx)\n", hres, keycode);
            CHECK_CALLED(ControlSite_TranslateAccelerator);

            IOleControlSite_Release(doc_controlsite);
        }

        IOleClientSite_Release(doc_clientsite);
    }

    test_UIActivate(unk, FALSE);
}

static void test_dochost_qs(IWebBrowser2 *webbrowser)
{
    IOleClientSite *client_site;
    IServiceProvider *serv_prov;
    IUnknown *service;
    HRESULT hres;

    client_site = get_dochost(webbrowser);
    hres = IOleClientSite_QueryInterface(client_site, &IID_IServiceProvider, (void**)&serv_prov);
    IOleClientSite_Release(client_site);
    ok(hres == S_OK, "Could not get IServiceProvider iface: %08lx\n", hres);

    hres = IServiceProvider_QueryService(serv_prov, &IID_IHlinkFrame, &IID_IHlinkFrame, (void**)&service);
    ok(hres == S_OK, "QueryService failed: %08lx\n", hres);
    ok(iface_cmp(service, (IUnknown*)webbrowser), "service != unk\n");
    IUnknown_Release(service);

    hres = IServiceProvider_QueryService(serv_prov, &IID_IWebBrowserApp, &IID_IHlinkFrame, (void**)&service);
    ok(hres == S_OK, "QueryService failed: %08lx\n", hres);
    ok(iface_cmp(service, (IUnknown*)webbrowser), "service != unk\n");
    IUnknown_Release(service);

    hres = IServiceProvider_QueryService(serv_prov, &IID_ITargetFrame, &IID_ITargetFrame, (void**)&service);
    ok(hres == S_OK, "QueryService failed: %08lx\n", hres);
    ok(iface_cmp(service, (IUnknown*)webbrowser), "service != unk\n");
    IUnknown_Release(service);

    hres = IServiceProvider_QueryService(serv_prov, &IID_IShellBrowser, &IID_IShellBrowser, (void**)&service);
    ok(hres == S_OK, "QueryService failed: %08lx\n", hres);
    IUnknown_Release(service);

    hres = IServiceProvider_QueryService(serv_prov, &IID_IShellBrowser, &IID_IBrowserService, (void**)&service);
    ok(hres == S_OK, "QueryService failed: %08lx\n", hres);
    IUnknown_Release(service);

    hres = IServiceProvider_QueryService(serv_prov, &IID_IShellBrowser, &IID_IDocObjectService, (void**)&service);
    ok(hres == S_OK, "QueryService failed: %08lx\n", hres);
    IUnknown_Release(service);

    IServiceProvider_Release(serv_prov);
}

static void test_Close(IWebBrowser2 *wb, BOOL do_download)
{
    IOleObject *oo;
    IOleClientSite *ocs;
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(wb, &IID_IOleObject, (void**)&oo);
    ok(hres == S_OK, "QueryInterface failed: %08lx\n", hres);
    if(hres != S_OK)
        return;

    test_close = TRUE;

    SET_EXPECT(Frame_SetActiveObject);
    SET_EXPECT(UIWindow_SetActiveObject);
    SET_EXPECT(OnUIDeactivate);
    SET_EXPECT(OnFocus_FALSE);
    SET_EXPECT(OnInPlaceDeactivate);
    SET_EXPECT(Invoke_STATUSTEXTCHANGE);
    if(!do_download) {
        SET_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_FALSE);
        SET_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_FALSE);

        expect_update_commands_enable = 0;
        set_update_commands_enable = 0xdead;
        SET_EXPECT(Invoke_COMMANDSTATECHANGE_UPDATECOMMANDS);

        if(use_container_olecmd)
            SET_EXPECT(Invoke_DOWNLOADCOMPLETE);
    }else {
        nav_back_todo = nav_forward_todo = TRUE;
    }
    SET_EXPECT(Advise_OnClose);
    hres = IOleObject_Close(oo, OLECLOSE_NOSAVE);
    ok(hres == S_OK, "OleObject_Close failed: %lx\n", hres);
    CHECK_CALLED(Frame_SetActiveObject);
    CHECK_CALLED(UIWindow_SetActiveObject);
    CHECK_CALLED(OnUIDeactivate);
    CHECK_CALLED(OnFocus_FALSE);
    CHECK_CALLED(OnInPlaceDeactivate);
    CLEAR_CALLED(Invoke_STATUSTEXTCHANGE); /* Called by IE9 */
    if(!do_download) {
        CHECK_CALLED(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_FALSE);
        CHECK_CALLED(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_FALSE);

        todo_wine CHECK_CALLED(Invoke_COMMANDSTATECHANGE_UPDATECOMMANDS);
        todo_wine ok(expect_update_commands_enable == set_update_commands_enable,
           "got %d\n", set_update_commands_enable);

        if(use_container_olecmd)
            todo_wine CHECK_CALLED(Invoke_DOWNLOADCOMPLETE);
    }else {
        nav_back_todo = nav_forward_todo = FALSE;
    }
    CHECK_CALLED(Advise_OnClose);

    hres = IOleObject_GetClientSite(oo, &ocs);
    ok(hres == S_OK, "hres = %lx\n", hres);
    ok(!ocs, "ocs != NULL\n");

    SET_EXPECT(GetContainer);
    SET_EXPECT(Site_GetWindow);
    SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    SET_EXPECT(Invoke_AMBIENT_SILENT);
    hres = IOleObject_DoVerb(oo, OLEIVERB_HIDE, NULL, (IOleClientSite*)0xdeadbeef,
            0, (HWND)0xdeadbeef, NULL);
    ok(hres == S_OK, "DoVerb failed: %08lx\n", hres);
    CHECK_CALLED(GetContainer);
    CHECK_CALLED(Site_GetWindow);
    CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    CHECK_CALLED(Invoke_AMBIENT_SILENT);

    hres = IOleObject_GetClientSite(oo, &ocs);
    ok(hres == S_OK, "hres = %lx\n", hres);
    ok(ocs == &ClientSite, "ocs != &ClientSite\n");
    if(ocs)
        IOleClientSite_Release(ocs);

    SET_EXPECT(OnFocus_FALSE);
    SET_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_FALSE);
    SET_EXPECT(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_FALSE);
    SET_EXPECT(Advise_OnClose);
    hres = IOleObject_Close(oo, OLECLOSE_NOSAVE);
    ok(hres == S_OK, "OleObject_Close failed: %lx\n", hres);
    CHECK_NOT_CALLED(OnFocus_FALSE);
    todo_wine CHECK_NOT_CALLED(Invoke_COMMANDSTATECHANGE_NAVIGATEBACK_FALSE);
    todo_wine CHECK_NOT_CALLED(Invoke_COMMANDSTATECHANGE_NAVIGATEFORWARD_FALSE);
    CHECK_CALLED(Advise_OnClose);

    test_close = FALSE;
    IOleObject_Release(oo);
}

static void test_Advise(IWebBrowser2 *wb)
{
    IOleObject *oleobj;
    IEnumSTATDATA *data;
    DWORD connection[2];
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(wb, &IID_IOleObject, (void **)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_IOleObject) failed: %08lx\n", hres);

    hres = IOleObject_Unadvise(oleobj, 0);
    ok(hres == OLE_E_NOCONNECTION, "Unadvise returned: %08lx\n", hres);

    data = (void *)0xdeadbeef;
    hres = IOleObject_EnumAdvise(oleobj, &data);
    ok(hres == E_NOTIMPL, "EnumAdvise returned: %08lx\n", hres);
    ok(data == NULL, "got data %p\n", data);

    connection[0] = 0xdeadbeef;
    hres = IOleObject_Advise(oleobj, NULL, &connection[0]);
    ok(hres == E_INVALIDARG, "Advise returned: %08lx\n", hres);
    ok(connection[0] == 0, "got connection %lu\n", connection[0]);

    hres = IOleObject_Advise(oleobj, &test_sink, NULL);
    ok(hres == E_INVALIDARG, "Advise returned: %08lx\n", hres);

    connection[0] = 0xdeadbeef;
    hres = IOleObject_Advise(oleobj, &test_sink, &connection[0]);
    ok(hres == S_OK, "Advise returned: %08lx\n", hres);
    ok(connection[0] != 0xdeadbeef, "got connection %lu\n", connection[0]);

    connection[1] = 0xdeadbeef;
    hres = IOleObject_Advise(oleobj, &test_sink, &connection[1]);
    ok(hres == S_OK, "Advise returned: %08lx\n", hres);
    ok(connection[1] == connection[0] + 1, "got connection %lu\n", connection[1]);

    hres = IOleObject_Unadvise(oleobj, connection[1]);
    ok(hres == S_OK, "Unadvise returned: %08lx\n", hres);

    hres = IOleObject_Unadvise(oleobj, connection[1]);
    ok(hres == OLE_E_NOCONNECTION, "Unadvise returned: %08lx\n", hres);

    hres = IOleObject_Unadvise(oleobj, connection[0]);
    ok(hres == S_OK, "Unadvise returned: %08lx\n", hres);

    IOleObject_Release(oleobj);
}

#define TEST_DOWNLOAD    0x0001
#define TEST_NOOLECMD    0x0002
#define TEST_NODOCHOST   0x0004

static void init_test(IWebBrowser2 *webbrowser, DWORD flags)
{
    wb = webbrowser;

    do_download = is_downloading = (flags & TEST_DOWNLOAD) != 0;
    is_first_load = TRUE;
    dwl_flags = 0;
    use_container_olecmd = !(flags & TEST_NOOLECMD);
    use_container_dochostui = !(flags & TEST_NODOCHOST);
}

static void test_WebBrowser(DWORD flags, BOOL do_close)
{
    IWebBrowser2 *webbrowser;
    IOleObject *oleobj;
    HRESULT hres;
    ULONG ref, connection;

    webbrowser = create_webbrowser();
    if(!webbrowser)
        return;

    init_test(webbrowser, flags);

    hres = IWebBrowser2_QueryInterface(webbrowser, &IID_IOleObject, (void **)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_OleObject) failed: %08lx\n", hres);

    hres = IOleObject_Advise(oleobj, &test_sink, &connection);
    ok(hres == S_OK, "Advise failed: %08lx\n", hres);

    test_QueryStatusWB(webbrowser, FALSE);
    test_ExecWB(webbrowser, FALSE);
    test_QueryInterface(webbrowser);
    test_ready_state(READYSTATE_UNINITIALIZED, BUSY_FAIL);
    test_ClassInfo(webbrowser);
    test_EnumVerbs(webbrowser);
    test_LocationURL(webbrowser, L"");
    test_ConnectionPoint(webbrowser, TRUE);

    test_ClientSite(webbrowser, &ClientSite, !do_download);
    test_Extent(webbrowser);
    test_wb_funcs(webbrowser, TRUE);
    test_DoVerb(webbrowser);
    test_olecmd(webbrowser, FALSE);
    test_Navigate2(webbrowser, L"about:blank");
    test_QueryStatusWB(webbrowser, TRUE);
    test_Persist(webbrowser, 2);
    test_OleObject(webbrowser, 2);
    test_Advise(webbrowser);

    if(do_download) {
        IHTMLDocument2 *doc, *doc2;

        test_download(0);
        test_olecmd(webbrowser, TRUE);
        doc = get_document(webbrowser);

        test_put_href(webbrowser, L"about:test");
        test_download(DWL_FROM_PUT_HREF);
        doc2 = get_document(webbrowser);
        ok(doc == doc2, "doc != doc2\n");
        IHTMLDocument2_Release(doc2);

        trace("Navigate2 repeated...\n");
        test_Navigate2(webbrowser, L"about:blank");
        test_download(DWL_EXPECT_BEFORE_NAVIGATE);
        doc2 = get_document(webbrowser);
        ok(doc == doc2, "doc != doc2\n");
        IHTMLDocument2_Release(doc2);
        IHTMLDocument2_Release(doc);

        if(!do_close) {
            trace("Navigate2 http URL...\n");
            test_ready_state(READYSTATE_COMPLETE, VARIANT_FALSE);
            test_Navigate2(webbrowser, L"http://test.winehq.org/tests/hello.html");
            nav_back_todo = TRUE;
            test_download(DWL_EXPECT_BEFORE_NAVIGATE|DWL_HTTP);
            nav_back_todo = FALSE;

            test_Refresh(webbrowser, FALSE);
            test_Refresh(webbrowser, TRUE);

            trace("put_href http URL...\n");
            test_put_href(webbrowser, L"http://test.winehq.org/tests/winehq_snapshot/");
            test_download(DWL_FROM_PUT_HREF|DWL_HTTP|DWL_BACK_ENABLE);

            trace("GoBack...\n");
            nav_back_todo = TRUE;
            test_go_back(webbrowser, L"http://test.winehq.org/tests/hello.html", 0, 0, 1);
            test_download(DWL_FROM_GOBACK|DWL_HTTP);
            nav_back_todo = FALSE;

            trace("GoForward...\n");
            test_go_forward(webbrowser, L"http://test.winehq.org/tests/winehq_snapshot/", -1, 0);
            test_download(DWL_FROM_GOFORWARD|DWL_HTTP);

            trace("GoBack...\n");
            nav_back_todo = TRUE;
            test_go_back(webbrowser, L"http://test.winehq.org/tests/hello.html", 0, -1, 0);
            test_download(DWL_FROM_GOBACK|DWL_HTTP);
            nav_back_todo = FALSE;

            trace("GoForward...\n");
            test_go_forward(webbrowser, L"http://test.winehq.org/tests/winehq_snapshot/", -1, 0);
            test_download(DWL_FROM_GOFORWARD|DWL_HTTP);
        }else {
            trace("Navigate2 repeated with the same URL...\n");
            test_Navigate2(webbrowser, L"about:blank");
            test_download(DWL_EXPECT_BEFORE_NAVIGATE);
        }

        test_EnumVerbs(webbrowser);
        test_TranslateAccelerator(webbrowser);

        test_dochost_qs(webbrowser);
    }else {
        test_ExecWB(webbrowser, TRUE);
    }

    test_external(webbrowser);
    test_htmlwindow_close(webbrowser);

    if(do_close) {
        test_Close(webbrowser, do_download);
    }else {
        test_change_ClientSite(webbrowser);
        test_ClientSite(webbrowser, NULL, !do_download);
    }
    test_ie_funcs(webbrowser);
    test_GetControlInfo(webbrowser);
    test_wb_funcs(webbrowser, FALSE);
    test_ConnectionPoint(webbrowser, FALSE);
    test_IServiceProvider(webbrowser);

    hres = IOleObject_Unadvise(oleobj, connection);
    ok(hres == S_OK, "Unadvise failed: %08lx\n", hres);

    IOleObject_Release(oleobj);

    ref = IWebBrowser2_Release(webbrowser);
    ok(ref == 0 || broken(do_download && !do_close), "ref=%ld, expected 0\n", ref);
}

static void test_WebBrowserV1(void)
{
    IWebBrowser2 *wb;
    IOleObject *oleobj;
    ULONG ref, connection;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_WebBrowser_V1, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IWebBrowser2, (void**)&wb);
    ok(hres == S_OK, "Could not get WebBrowserV1 instance: %08lx\n", hres);

    init_test(wb, 0);
    wb_version = 1;

    hres = IWebBrowser2_QueryInterface(wb, &IID_IOleObject, (void **)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_OleObject) failed: %08lx\n", hres);

    hres = IOleObject_Advise(oleobj, &test_sink, &connection);
    ok(hres == S_OK, "Advise failed: %08lx\n", hres);

    test_QueryStatusWB(wb, FALSE);
    test_ExecWB(wb, FALSE);
    test_QueryInterface(wb);
    test_ready_state(READYSTATE_UNINITIALIZED, BUSY_FAIL);
    test_ClassInfo(wb);
    test_EnumVerbs(wb);
    test_Persist(wb, 1);
    test_OleObject(wb, 1);
    test_Advise(wb);

    hres = IOleObject_Unadvise(oleobj, connection);
    ok(hres == S_OK, "Unadvise failed: %08lx\n", hres);

    IOleObject_Release(oleobj);

    ref = IWebBrowser2_Release(wb);
    ok(ref == 0, "ref=%ld, expected 0\n", ref);
}

static void test_WebBrowser_slim_container(void)
{
    IWebBrowser2 *webbrowser;
    ULONG ref;

    webbrowser = create_webbrowser();
    init_test(webbrowser, TEST_NOOLECMD|TEST_NODOCHOST);

    test_ConnectionPoint(webbrowser, TRUE);
    test_ClientSite(webbrowser, &ClientSite, TRUE);
    test_DoVerb(webbrowser);
    test_Navigate2(webbrowser, L"about:blank");

    /* Tests of interest */
    test_QueryStatusWB(webbrowser, TRUE);
    test_ExecWB(webbrowser, TRUE);
    test_external(webbrowser);

    /* Cleanup stage */
    test_ClientSite(webbrowser, NULL, TRUE);
    test_ConnectionPoint(webbrowser, FALSE);

    ref = IWebBrowser2_Release(webbrowser);
    ok(ref == 0, "ref=%ld, expected 0\n", ref);
}

static void test_WebBrowser_DoVerb(void)
{
    IWebBrowser2 *webbrowser;
    RECT rect;
    HWND hwnd;
    ULONG ref;
    BOOL res;
    HRESULT hres;
    VARIANT_BOOL b;

    webbrowser = create_webbrowser();
    init_test(webbrowser, 0);

    test_ClientSite(webbrowser, &ClientSite, FALSE);

    SET_EXPECT(CanInPlaceActivate);
    SET_EXPECT(Site_GetWindow);
    SET_EXPECT(OnInPlaceActivate);
    SET_EXPECT(GetWindowContext);
    SET_EXPECT(ShowObject);
    SET_EXPECT(GetContainer);
    SET_EXPECT(Frame_GetWindow);
    call_DoVerb(webbrowser, OLEIVERB_INPLACEACTIVATE);
    CHECK_CALLED(CanInPlaceActivate);
    CHECK_CALLED(Site_GetWindow);
    CHECK_CALLED(OnInPlaceActivate);
    CHECK_CALLED(GetWindowContext);
    CHECK_CALLED(ShowObject);
    CHECK_CALLED(GetContainer);
    CHECK_CALLED(Frame_GetWindow);

    hwnd = get_hwnd(webbrowser);

    memset(&rect, 0xa, sizeof(rect));
    res = GetWindowRect(hwnd, &rect);
    ok(res, "GetWindowRect failed: %lu\n", GetLastError());

    SET_EXPECT(OnInPlaceDeactivate);
    call_DoVerb(webbrowser, OLEIVERB_HIDE);
    CHECK_CALLED(OnInPlaceDeactivate);

    b = 0x100;
    hres = IWebBrowser2_get_Visible(webbrowser, &b);
    ok(hres == S_OK, "get_Visible failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "Visible = %x\n", b);

    SET_EXPECT(CanInPlaceActivate);
    SET_EXPECT(Site_GetWindow);
    SET_EXPECT(OnInPlaceActivate);
    SET_EXPECT(GetWindowContext);
    SET_EXPECT(ShowObject);
    SET_EXPECT(GetContainer);
    SET_EXPECT(Frame_GetWindow);
    SET_EXPECT(OnUIActivate);
    SET_EXPECT(Frame_SetActiveObject);
    SET_EXPECT(UIWindow_SetActiveObject);
    SET_EXPECT(SetMenu);
    SET_EXPECT(OnFocus_TRUE);
    call_DoVerb(webbrowser, OLEIVERB_SHOW);
    CHECK_CALLED(CanInPlaceActivate);
    CHECK_CALLED(Site_GetWindow);
    CHECK_CALLED(OnInPlaceActivate);
    CHECK_CALLED(GetWindowContext);
    CHECK_CALLED(ShowObject);
    CHECK_CALLED(GetContainer);
    CHECK_CALLED(Frame_GetWindow);
    CHECK_CALLED(OnUIActivate);
    CHECK_CALLED(Frame_SetActiveObject);
    CHECK_CALLED(UIWindow_SetActiveObject);
    CHECK_CALLED(SetMenu);
    CHECK_CALLED(OnFocus_TRUE);

    b = 0x100;
    hres = IWebBrowser2_get_Visible(webbrowser, &b);
    ok(hres == S_OK, "get_Visible failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "Visible = %x\n", b);

    call_DoVerb(webbrowser, OLEIVERB_SHOW);
    call_DoVerb(webbrowser, OLEIVERB_UIACTIVATE);

    SET_EXPECT(Frame_SetActiveObject);
    SET_EXPECT(UIWindow_SetActiveObject);
    SET_EXPECT(OnUIDeactivate);
    SET_EXPECT(OnFocus_FALSE);
    SET_EXPECT(OnInPlaceDeactivate);
    test_hide = TRUE;
    call_DoVerb(webbrowser, OLEIVERB_HIDE);
    test_hide = FALSE;
    CHECK_CALLED(Frame_SetActiveObject);
    CHECK_CALLED(UIWindow_SetActiveObject);
    CHECK_CALLED(OnUIDeactivate);
    CHECK_CALLED(OnFocus_FALSE);
    CHECK_CALLED(OnInPlaceDeactivate);

    b = 0x100;
    hres = IWebBrowser2_get_Visible(webbrowser, &b);
    ok(hres == S_OK, "get_Visible failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "Visible = %x\n", b);

    SET_EXPECT(CanInPlaceActivate);
    SET_EXPECT(Site_GetWindow);
    SET_EXPECT(OnInPlaceActivate);
    SET_EXPECT(GetWindowContext);
    SET_EXPECT(ShowObject);
    SET_EXPECT(GetContainer);
    SET_EXPECT(Frame_GetWindow);
    SET_EXPECT(OnUIActivate);
    SET_EXPECT(Frame_SetActiveObject);
    SET_EXPECT(UIWindow_SetActiveObject);
    SET_EXPECT(SetMenu);
    SET_EXPECT(OnFocus_TRUE);
    call_DoVerb(webbrowser, OLEIVERB_SHOW);
    CHECK_CALLED(CanInPlaceActivate);
    CHECK_CALLED(Site_GetWindow);
    CHECK_CALLED(OnInPlaceActivate);
    CHECK_CALLED(GetWindowContext);
    CHECK_CALLED(ShowObject);
    CHECK_CALLED(GetContainer);
    CHECK_CALLED(Frame_GetWindow);
    CHECK_CALLED(OnUIActivate);
    CHECK_CALLED(Frame_SetActiveObject);
    CHECK_CALLED(UIWindow_SetActiveObject);
    CHECK_CALLED(SetMenu);
    CHECK_CALLED(OnFocus_TRUE);

    b = 0x100;
    hres = IWebBrowser2_get_Visible(webbrowser, &b);
    ok(hres == S_OK, "get_Visible failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "Visible = %x\n", b);

    SET_EXPECT(Frame_SetActiveObject);
    SET_EXPECT(UIWindow_SetActiveObject);
    SET_EXPECT(OnUIDeactivate);
    SET_EXPECT(OnFocus_FALSE);
    SET_EXPECT(OnInPlaceDeactivate);
    test_hide = TRUE;
    call_DoVerb(webbrowser, OLEIVERB_HIDE);
    CHECK_CALLED(Frame_SetActiveObject);
    CHECK_CALLED(UIWindow_SetActiveObject);
    CHECK_CALLED(OnUIDeactivate);
    CHECK_CALLED(OnFocus_FALSE);
    CHECK_CALLED(OnInPlaceDeactivate);

    call_DoVerb(webbrowser, OLEIVERB_HIDE);
    test_hide = FALSE;

    b = 0x100;
    hres = IWebBrowser2_get_Visible(webbrowser, &b);
    ok(hres == S_OK, "get_Visible failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "Visible = %x\n", b);

    SET_EXPECT(CanInPlaceActivate);
    SET_EXPECT(Site_GetWindow);
    SET_EXPECT(OnInPlaceActivate);
    SET_EXPECT(GetWindowContext);
    SET_EXPECT(ShowObject);
    SET_EXPECT(GetContainer);
    SET_EXPECT(Frame_GetWindow);
    SET_EXPECT(OnUIActivate);
    SET_EXPECT(Frame_SetActiveObject);
    SET_EXPECT(UIWindow_SetActiveObject);
    SET_EXPECT(SetMenu);
    SET_EXPECT(OnFocus_TRUE);
    call_DoVerb(webbrowser, OLEIVERB_UIACTIVATE);
    CHECK_CALLED(CanInPlaceActivate);
    CHECK_CALLED(Site_GetWindow);
    CHECK_CALLED(OnInPlaceActivate);
    CHECK_CALLED(GetWindowContext);
    CHECK_CALLED(ShowObject);
    CHECK_CALLED(GetContainer);
    CHECK_CALLED(Frame_GetWindow);
    CHECK_CALLED(OnUIActivate);
    CHECK_CALLED(Frame_SetActiveObject);
    CHECK_CALLED(UIWindow_SetActiveObject);
    CHECK_CALLED(SetMenu);
    CHECK_CALLED(OnFocus_TRUE);

    b = 0x100;
    hres = IWebBrowser2_get_Visible(webbrowser, &b);
    ok(hres == S_OK, "get_Visible failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "Visible = %x\n", b);

    call_DoVerb(webbrowser, OLEIVERB_SHOW);

    test_ClientSite(webbrowser, NULL, FALSE);

    ref = IWebBrowser2_Release(webbrowser);
    ok(ref == 0, "ref=%ld, expected 0\n", ref);
}


/* Check if Internet Explorer is configured to run in "Enhanced Security Configuration" (aka hardened mode) */
/* Note: this code is duplicated in dlls/mshtml/tests/mshtml_test.h and dlls/urlmon/tests/sec_mgr.c */
static BOOL is_ie_hardened(void)
{
    HKEY zone_map;
    DWORD ie_harden, type, size;

    ie_harden = 0;
    if(RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap",
                    0, KEY_QUERY_VALUE, &zone_map) == ERROR_SUCCESS) {
        size = sizeof(DWORD);
        if (RegQueryValueExA(zone_map, "IEHarden", NULL, &type, (LPBYTE) &ie_harden, &size) != ERROR_SUCCESS ||
            type != REG_DWORD) {
            ie_harden = 0;
        }
        RegCloseKey(zone_map);
    }

    return ie_harden != 0;
}

static void test_FileProtocol(void)
{
    IWebBrowser2 *webbrowser;
    HANDLE file;
    ULONG ref;
    WCHAR file_path[MAX_PATH];
    WCHAR file_url[MAX_PATH] = L"File://";

    static const WCHAR test_file[] = L"wine_test.html";

    GetTempPathW(MAX_PATH, file_path);
    lstrcatW(file_path, test_file);

    webbrowser = create_webbrowser();
    if(!webbrowser)
        return;

    init_test(webbrowser, 0);

    file = CreateFileW(file_path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if(file == INVALID_HANDLE_VALUE && GetLastError() != ERROR_FILE_EXISTS){
        ok(0, "CreateFile failed\n");
        return;
    }
    CloseHandle(file);

    GetLongPathNameW(file_path, file_path, ARRAY_SIZE(file_path));
    lstrcatW(file_url, file_path);

    test_ConnectionPoint(webbrowser, TRUE);
    test_ClientSite(webbrowser, &ClientSite, TRUE);
    test_DoVerb(webbrowser);
    test_Navigate2(webbrowser, file_url);
    test_ClientSite(webbrowser, NULL, TRUE);

    ref = IWebBrowser2_Release(webbrowser);
    ok(ref == 0, "ref=%lu, expected 0\n", ref);

    if(file != INVALID_HANDLE_VALUE)
        DeleteFileW(file_path);
}

static HRESULT WINAPI sink_QueryInterface( IAdviseSink *iface, REFIID riid, void **obj)
{
    if (IsEqualGUID(riid, &IID_IAdviseSink) || IsEqualGUID(riid, &IID_IUnknown)) {
        *obj = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_IProxyManager))
        return E_NOINTERFACE;

    ok(0, "unexpected call QI(%s)\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI sink_AddRef(IAdviseSink *iface)
{
    return 2;
}

static ULONG WINAPI sink_Release(IAdviseSink *iface)
{
    return 1;
}

static void WINAPI sink_OnDataChange(IAdviseSink *iface, FORMATETC *format, STGMEDIUM *medium)
{
    trace("OnDataChange(%p, %p, %p)\n", iface, format, medium);
}

static void WINAPI sink_OnViewChange(IAdviseSink *iface, DWORD aspect, LONG index)
{
    trace("OnViewChange(%p, %08lx, %ld)\n", iface, aspect, index);
}

static void WINAPI sink_OnRename(IAdviseSink *iface, IMoniker *moniker)
{
    trace("OnRename(%p, %p) \n", iface, moniker);
}

static void WINAPI sink_OnSave(IAdviseSink *iface)
{
    trace("OnSave(%p)\n", iface);
}

static void WINAPI sink_OnClose(IAdviseSink *iface)
{
    trace("OnClose(%p)\n", iface);
    CHECK_EXPECT(Advise_OnClose);
}

static const IAdviseSinkVtbl sink_vtbl =
{
    sink_QueryInterface,
    sink_AddRef,
    sink_Release,
    sink_OnDataChange,
    sink_OnViewChange,
    sink_OnRename,
    sink_OnSave,
    sink_OnClose
};

static IAdviseSink test_sink = { &sink_vtbl };

static void test_SetAdvise(void)
{
    HRESULT hr;
    IWebBrowser2 *browser;
    IViewObject2 *view;
    IAdviseSink *sink;
    IOleObject *oleobj;
    IDispatch *doc;
    DWORD aspects, flags;

    if (!(browser = create_webbrowser())) return;
    init_test(browser, 0);

    hr = IWebBrowser2_QueryInterface(browser, &IID_IViewObject2, (void **)&view);
    ok(hr == S_OK, "got %08lx\n", hr);
    if (FAILED(hr)) return;

    aspects = flags = 0xdeadbeef;
    sink = (IAdviseSink *)0xdeadbeef;
    hr = IViewObject2_GetAdvise(view, &aspects, &flags, &sink);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(!aspects, "got %08lx\n", aspects);
    ok(!flags, "got %08lx\n", aspects);
    ok(sink == NULL, "got %p\n", sink);

    hr = IViewObject2_SetAdvise(view, DVASPECT_CONTENT, 0, &test_sink);
    ok(hr == S_OK, "got %08lx\n", hr);

    aspects = flags = 0xdeadbeef;
    sink = (IAdviseSink *)0xdeadbeef;
    hr = IViewObject2_GetAdvise(view, &aspects, &flags, &sink);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(aspects == DVASPECT_CONTENT, "got %08lx\n", aspects);
    ok(!flags, "got %08lx\n", aspects);
    ok(sink == &test_sink, "got %p\n", sink);

    hr = IWebBrowser2_QueryInterface(browser, &IID_IOleObject, (void **)&oleobj);
    ok(hr == S_OK, "got %08lx\n", hr);

    SET_EXPECT(GetContainer);
    SET_EXPECT(Site_GetWindow);
    SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    SET_EXPECT(Invoke_AMBIENT_SILENT);
    hr = IOleObject_SetClientSite(oleobj, &ClientSite);
    ok(hr == S_OK, "got %08lx\n", hr);
    CHECK_CALLED(GetContainer);
    CHECK_CALLED(Site_GetWindow);
    CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    CHECK_CALLED(Invoke_AMBIENT_SILENT);

    sink = (IAdviseSink *)0xdeadbeef;
    hr = IViewObject2_GetAdvise(view, &aspects, &flags, &sink);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(sink == &test_sink, "got %p\n", sink);

    hr = IOleObject_SetClientSite(oleobj, NULL);
    ok(hr == S_OK, "got %08lx\n", hr);

    aspects = flags = 0xdeadbeef;
    sink = (IAdviseSink *)0xdeadbeef;
    hr = IViewObject2_GetAdvise(view, &aspects, &flags, &sink);
    ok(hr == S_OK, "got %08lx\n", hr);
    ok(sink == &test_sink, "got %p\n", sink);

    hr = IViewObject2_SetAdvise(view, 0, 0, NULL);
    ok(hr == S_OK, "got %08lx\n", hr);

    doc = (void*)0xdeadbeef;
    hr = IWebBrowser2_get_Document(browser, &doc);
    ok(hr == S_FALSE, "get_Document failed: %08lx\n", hr);
    ok(!doc, "doc = %p\n", doc);

    IOleObject_Release(oleobj);
    IViewObject2_Release(view);
    IWebBrowser2_Release(browser);
}

static void test_SetQueryNetSessionCount(void)
{
    LONG count, init_count, inc_count, dec_count;

    init_count = pSetQueryNetSessionCount(SESSION_QUERY);
    trace("init_count %ld\n", init_count);

    inc_count = pSetQueryNetSessionCount(SESSION_INCREMENT);
    ok(inc_count > init_count, "count = %ld\n", inc_count);

    count = pSetQueryNetSessionCount(SESSION_QUERY);
    ok(count == inc_count, "count = %ld\n", count);

    dec_count = pSetQueryNetSessionCount(SESSION_DECREMENT);
    ok(dec_count < inc_count, "count = %ld\n", dec_count);

    count = pSetQueryNetSessionCount(SESSION_QUERY);
    ok(count == dec_count, "count = %ld\n", count);
}

static HRESULT WINAPI outer_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &outer_test_iid)) {
        CHECK_EXPECT(outer_QI_test);
        *ppv = (IUnknown*)0xdeadbeef;
        return S_OK;
    }
    ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI outer_AddRef(IUnknown *iface)
{
    return 2;
}

static ULONG WINAPI outer_Release(IUnknown *iface)
{
    return 1;
}

static IUnknownVtbl outer_vtbl = {
    outer_QueryInterface,
    outer_AddRef,
    outer_Release
};

static void test_com_aggregation(void)
{
    HRESULT hr;
    IClassFactory *class_factory;
    IUnknown outer = { &outer_vtbl };
    IUnknown *unk = NULL;
    IWebBrowser *web_browser = NULL;
    IUnknown *unk2 = NULL;

    hr = CoGetClassObject(&CLSID_WebBrowser, CLSCTX_INPROC_SERVER, NULL, &IID_IClassFactory, (void**)&class_factory);
    ok(hr == S_OK, "CoGetClassObject failed: %08lx\n", hr);

    hr = IClassFactory_CreateInstance(class_factory, &outer, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "CreateInstance returned hr = %08lx\n", hr);
    ok(unk != NULL, "result NULL, hr = %08lx\n", hr);

    hr = IUnknown_QueryInterface(unk, &IID_IWebBrowser, (void**)&web_browser);
    ok(hr == S_OK, "QI to IWebBrowser failed, hr=%08lx\n", hr);

    SET_EXPECT(outer_QI_test);
    hr = IWebBrowser_QueryInterface(web_browser, &outer_test_iid, (void**)&unk2);
    CHECK_CALLED(outer_QI_test);
    ok(hr == S_OK, "Could not get test iface: %08lx\n", hr);
    ok(unk2 == (IUnknown*)0xdeadbeef, "unexpected unk2\n");

    IWebBrowser_Release(web_browser);
    IUnknown_Release(unk);
    IClassFactory_Release(class_factory);
}

START_TEST(webbrowser)
{
    HMODULE ieframe = LoadLibraryW(L"ieframe.dll");

    pSetQueryNetSessionCount = (void*)GetProcAddress(ieframe, "SetQueryNetSessionCount");

    OleInitialize(NULL);

    test_SetQueryNetSessionCount();

    container_hwnd = create_container_window();

    trace("Testing WebBrowser (no download)...\n");
    test_WebBrowser(0, FALSE);
    test_WebBrowser(0, TRUE);

    if(!is_ie_hardened()) {
        trace("Testing WebBrowser...\n");
        test_WebBrowser(TEST_DOWNLOAD, FALSE);
        trace("Testing WebBrowser(no olecmd)...\n");
        test_WebBrowser(TEST_DOWNLOAD|TEST_NOOLECMD, FALSE);
    }else {
        win_skip("Skipping http tests in hardened mode\n");
    }

    trace("Testing WebBrowser DoVerb\n");
    test_WebBrowser_DoVerb();
    trace("Testing WebBrowser (with close)...\n");
    test_WebBrowser(TEST_DOWNLOAD, TRUE);
    trace("Testing WebBrowser w/o container-based olecmd...\n");
    test_WebBrowser_slim_container();
    trace("Testing WebBrowserV1...\n");
    test_WebBrowserV1();
    trace("Testing file protocol...\n");
    test_FileProtocol();
    trace("Testing SetAdvise...\n");
    test_SetAdvise();
    test_com_aggregation();

    OleUninitialize();
}
