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
#include "mshtmhst.h"
#include "mshtmcid.h"
#include "idispids.h"
#include "olectl.h"
#include "mshtmdid.h"
#include "shobjidl.h"
#include "shlguid.h"
#include "exdispid.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

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

#define CHECK_CALLED_BROKEN(func) \
    do { \
        ok(called_ ## func || broken(!called_ ## func), "expected " #func "\n"); \
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
DEFINE_EXPECT(Invoke_COMMANDSTATECHANGE);
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
DEFINE_EXPECT(Invoke_282);
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
DEFINE_EXPECT(QueryStatus_SETPROGRESSTEXT);
DEFINE_EXPECT(QueryStatus_STOP);
DEFINE_EXPECT(DocHost_EnableModeless_TRUE);
DEFINE_EXPECT(DocHost_EnableModeless_FALSE);
DEFINE_EXPECT(GetDropTarget);

static const WCHAR wszItem[] = {'i','t','e','m',0};
static const WCHAR about_blankW[] = {'a','b','o','u','t',':','b','l','a','n','k',0};
static const WCHAR emptyW[] = {0};

static VARIANT_BOOL exvb;
static IWebBrowser2 *wb;

static HWND container_hwnd, shell_embedding_hwnd;
static BOOL is_downloading = FALSE;

static const char *debugstr_w(LPCWSTR str)
{
    static char buf[1024];
    if(!str)
        return "(null)";
    WideCharToMultiByte(CP_ACP, 0, str, -1, buf, sizeof(buf), NULL, NULL);
    return buf;
}

static const char *debugstr_guid(REFIID riid)
{
    static char buf[50];

    sprintf(buf, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            riid->Data1, riid->Data2, riid->Data3, riid->Data4[0],
            riid->Data4[1], riid->Data4[2], riid->Data4[3], riid->Data4[4],
            riid->Data4[5], riid->Data4[6], riid->Data4[7]);

    return buf;
}

#define test_LocationURL(a,b) _test_LocationURL(__LINE__,a,b)
static void _test_LocationURL(unsigned line, IUnknown *unk, LPCWSTR exurl)
{
    IWebBrowser2 *wb;
    BSTR url = (void*)0xdeadbeef;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IWebBrowser2, (void**)&wb);
    ok(hres == S_OK, "Could not get IWebBrowser2 interface: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IWebBrowser2_get_LocationURL(wb, &url);
    ok_(__FILE__,line) (hres == (*exurl ? S_OK : S_FALSE), "get_LocationURL failed: %08x\n", hres);
    ok_(__FILE__,line) (!lstrcmpW(url, exurl), "unexpected URL: %s\n", debugstr_w(url));

    SysFreeString(url);
    IWebBrowser2_Release(wb);
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
    ok(cCmds == 1, "cCmds=%d, expected 1\n", cCmds);
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
    default:
        ok(0, "unexpected command %d\n", prgCmds[0].cmdID);
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
            ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER, "nCmdexecopts=%08x\n", nCmdexecopt);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            if(pvaIn)
                ok(V_VT(pvaIn) == VT_I4, "V_VT(pvaIn)=%d, expected VT_I4\n", V_VT(pvaIn));
            ok(pvaOut == NULL, "pvaOut=%p, expected NULL\n", pvaOut);
            return S_OK;
        case OLECMDID_SETPROGRESSPOS:
            CHECK_EXPECT2(Exec_SETPROGRESSPOS);
            ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER, "nCmdexecopts=%08x\n", nCmdexecopt);
            ok(pvaIn != NULL, "pvaIn == NULL\n");
            if(pvaIn)
                ok(V_VT(pvaIn) == VT_I4, "V_VT(pvaIn)=%d, expected VT_I4\n", V_VT(pvaIn));
            ok(pvaOut == NULL, "pvaOut=%p, expected NULL\n", pvaOut);
            return S_OK;
        case OLECMDID_SETDOWNLOADSTATE:
            if(is_downloading)
                ok(nCmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER || !nCmdexecopt,
                   "nCmdexecopts=%08x\n", nCmdexecopt);
            else
                ok(!nCmdexecopt, "nCmdexecopts=%08x\n", nCmdexecopt);
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
                ok(0, "unexpevted V_I4(pvaIn)=%d\n", V_I4(pvaIn));
            }
            return S_OK;
        default:
            ok(0, "unexpected nsCmdID %d\n", nCmdID);
        }
    }else if(IsEqualGUID(&CGID_Explorer, pguidCmdGroup)) {
        switch(nCmdID) {
        case 24: /* TODO */
        case 25: /* IE5 */
        case 37: /* TODO */
        case 39: /* TODO */
        case 66: /* TODO */
        case 67: /* TODO */
        case 69: /* TODO */
            return E_FAIL;
        default:
            ok(0, "unexpected nCmdID %d\n", nCmdID);
        }
    }else {
        ok(0, "unepected pguidCmdGroup %s\n", debugstr_guid(pguidCmdGroup));
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
        *ppv = &OleCommandTarget;
        return S_OK;
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

    ok(0, "unexpected dispIdMember %d\n", dispIdMember);
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

    if(IsEqualGUID(&DIID_DWebBrowserEvents2, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected riid %s\n", debugstr_guid(riid));
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
    ok(V_VT(disp) == VT_DISPATCH, "V_VT(disp)=%d, expected VT_DISPATCH\n", V_VT(disp));
    ok(V_DISPATCH(disp) != NULL, "V_DISPATCH(disp) == NULL\n");
    ok(V_DISPATCH(disp) == (IDispatch*)wb, "V_DISPATCH(disp)=%p, wb=%p\n", V_DISPATCH(disp), wb);

    ok(V_VT(url) == (VT_BYREF|VT_VARIANT), "V_VT(url)=%x, expected VT_BYREF|VT_VARIANT\n", V_VT(url));
    ok(V_VARIANTREF(url) != NULL, "V_VARIANTREF(url) == NULL)\n");
    if(V_VARIANTREF(url)) {
        ok(V_VT(V_VARIANTREF(url)) == VT_BSTR, "V_VT(V_VARIANTREF(url))=%d, expected VT_BSTR\n",
           V_VT(V_VARIANTREF(url)));
        ok(V_BSTR(V_VARIANTREF(url)) != NULL, "V_BSTR(V_VARIANTREF(url)) == NULL\n");
        ok(!lstrcmpW(V_BSTR(V_VARIANTREF(url)), about_blankW), "unexpected url %s\n",
           debugstr_w(V_BSTR(V_VARIANTREF(url))));
    }

    ok(V_VT(flags) == (VT_BYREF|VT_VARIANT), "V_VT(flags)=%x, expected VT_BYREF|VT_VARIANT\n",
       V_VT(flags));
    ok(V_VT(flags) == (VT_BYREF|VT_VARIANT), "V_VT(flags)=%x, expected VT_BYREF|VT_VARIANT\n",
       V_VT(flags));
    ok(V_VARIANTREF(flags) != NULL, "V_VARIANTREF(flags) == NULL)\n");
    if(V_VARIANTREF(flags)) {
        ok(V_VT(V_VARIANTREF(flags)) == VT_I4, "V_VT(V_VARIANTREF(flags))=%d, expected VT_I4\n",
           V_VT(V_VARIANTREF(flags)));
        ok(V_I4(V_VARIANTREF(flags)) == 0, "V_I4(V_VARIANTREF(flags)) = %d, expected 0\n",
           V_I4(V_VARIANTREF(flags)));
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
        ok(V_BSTR(V_VARIANTREF(headers)) == NULL, "V_BSTR(V_VARIANTREF(heders)) = %p, expected NULL\n",
           V_BSTR(V_VARIANTREF(headers)));
    }

    ok(V_VT(cancel) == (VT_BYREF|VT_BOOL), "V_VT(cancel)=%x, expected VT_BYREF|VT_BOOL\n",
       V_VT(cancel));
    ok(V_BOOLREF(cancel) != NULL, "V_BOOLREF(pDispParams->rgvarg[0] == NULL)\n");
    if(V_BOOLREF(cancel))
        ok(*V_BOOLREF(cancel) == VARIANT_FALSE, "*V_BOOLREF(calcel) = %x, expected VARIANT_FALSE\n",
           *V_BOOLREF(cancel));
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
        /* TODO: Check text */
        break;

    case DISPID_PROPERTYCHANGE:
        CHECK_EXPECT2(Invoke_PROPERTYCHANGE);

        ok(pDispParams->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pDispParams->cArgs == 1, "cArgs=%d, expected 1\n", pDispParams->cArgs);
        /* TODO: Check args */
        break;

    case DISPID_DOWNLOADBEGIN:
        CHECK_EXPECT(Invoke_DOWNLOADBEGIN);

        ok(pDispParams->rgvarg == NULL, "rgvarg=%p, expected NULL\n", pDispParams->rgvarg);
        ok(pDispParams->cArgs == 0, "cArgs=%d, expected 0\n", pDispParams->cArgs);
        break;

    case DISPID_BEFORENAVIGATE2:
        CHECK_EXPECT(Invoke_BEFORENAVIGATE2);

        ok(pDispParams->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pDispParams->cArgs == 7, "cArgs=%d, expected 7\n", pDispParams->cArgs);
        test_OnBeforeNavigate(pDispParams->rgvarg+6, pDispParams->rgvarg+5, pDispParams->rgvarg+4,
                              pDispParams->rgvarg+3, pDispParams->rgvarg+2, pDispParams->rgvarg+1,
                              pDispParams->rgvarg);
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
        CHECK_EXPECT2(Invoke_COMMANDSTATECHANGE);

        ok(pDispParams->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pDispParams->cArgs == 2, "cArgs=%d, expected 2\n", pDispParams->cArgs);
        /* TODO: Check args */
        break;

    case DISPID_DOWNLOADCOMPLETE:
        CHECK_EXPECT(Invoke_DOWNLOADCOMPLETE);

        ok(pDispParams->rgvarg == NULL, "rgvarg=%p, expected NULL\n", pDispParams->rgvarg);
        ok(pDispParams->cArgs == 0, "cArgs=%d, expected 0\n", pDispParams->cArgs);
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
        CHECK_EXPECT(Invoke_NAVIGATECOMPLETE2);
        /* FIXME */
        break;

    case DISPID_PROGRESSCHANGE:
        CHECK_EXPECT(Invoke_PROGRESSCHANGE);
        /* FIXME */
        break;

    case DISPID_DOCUMENTCOMPLETE:
        CHECK_EXPECT(Invoke_DOCUMENTCOMPLETE);
        /* FIXME */
        break;

    case 282: /* FIXME */
        CHECK_EXPECT2(Invoke_282);
        break;

    default:
        ok(0, "unexpected dispIdMember %d\n", dispIdMember);
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
    ok(pActiveObject != NULL, "pActiveObject = NULL\n");
    ok(!lstrcmpW(pszObjName, wszItem), "unexpected pszObjName\n");
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_SetActiveObject(IOleInPlaceFrame *iface,
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    CHECK_EXPECT(Frame_SetActiveObject);
    ok(pActiveObject != NULL, "pActiveObject = NULL\n");
    ok(!lstrcmpW(pszObjName, wszItem), "unexpected pszObjName\n");
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
        lpFrameInfo->cb = sizeof(*lpFrameInfo);
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
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceDeactivate(IOleInPlaceSiteEx *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
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
    ok(0, "unexpected call\n");
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
    ok(0, "unexpected call %d %p %p %p\n", dwID, ppt, pcmdtReserved, pdicpReserved);
    return S_FALSE;
}

static HRESULT WINAPI DocHostUIHandler_GetHostInfo(IDocHostUIHandler2 *iface, DOCHOSTUIINFO *pInfo)
{
    CHECK_EXPECT2(GetHostInfo);
    ok(pInfo != NULL, "pInfo=NULL\n");
    if(pInfo) {
        ok(pInfo->cbSize == sizeof(DOCHOSTUIINFO) || broken(!pInfo->cbSize), "pInfo->cbSize=%u\n", pInfo->cbSize);
        ok(!pInfo->dwFlags, "pInfo->dwFlags=%08x, expected 0\n", pInfo->dwFlags);
        ok(!pInfo->dwDoubleClick, "pInfo->dwDoubleClick=%08x, expected 0\n", pInfo->dwDoubleClick);
        ok(!pInfo->pchHostCss, "pInfo->pchHostCss=%p, expected NULL\n", pInfo->pchHostCss);
        ok(!pInfo->pchHostNS, "pInfo->pchhostNS=%p, expected NULL\n", pInfo->pchHostNS);
    }
    return E_FAIL;
}

static HRESULT WINAPI DocHostUIHandler_ShowUI(IDocHostUIHandler2 *iface, DWORD dwID,
        IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget,
        IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_HideUI(IDocHostUIHandler2 *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_UpdateUI(IDocHostUIHandler2 *iface)
{
    CHECK_EXPECT(UpdateUI);
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
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetOptionKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    CHECK_EXPECT(GetOptionKeyPath);
    ok(pchKey != NULL, "pchKey==NULL\n");
    if(pchKey)
        ok(*pchKey == NULL, "*pchKey=%p\n", *pchKey);
    ok(!dw, "dw=%x\n", dw);
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
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_TranslateUrl(IDocHostUIHandler2 *iface, DWORD dwTranslate,
        OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
    ok(0, "unexpected call\n");
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
    ok(!dw, "dw=%x\n", dw);
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
        *ppv = &DocHostUIHandler;
    else if(IsEqualGUID(&IID_IDispatch, riid))
        *ppv = &Dispatch;

    if(*ppv)
        return S_OK;

    return E_NOINTERFACE;
}

static LRESULT WINAPI wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static HWND create_container_window(void)
{
    static const WCHAR wszWebBrowserContainer[] =
        {'W','e','b','B','r','o','w','s','e','r','C','o','n','t','a','i','n','e','r',0};
    static WNDCLASSEXW wndclass = {
        sizeof(WNDCLASSEXW),
        0,
        wnd_proc,
        0, 0, NULL, NULL, NULL, NULL, NULL,
        wszWebBrowserContainer,
        NULL
    };

    RegisterClassExW(&wndclass);
    return CreateWindowW(wszWebBrowserContainer, wszWebBrowserContainer,
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, NULL, NULL, NULL, NULL);
}

static void test_DoVerb(IUnknown *unk)
{
    IOleObject *oleobj;
    RECT rect = {0,0,1000,1000};
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_OleObject) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

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

    hres = IOleObject_DoVerb(oleobj, OLEIVERB_SHOW, NULL, &ClientSite,
                             0, (HWND)0xdeadbeef, &rect);
    ok(hres == S_OK, "DoVerb failed: %08x\n", hres);

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

    hres = IOleObject_DoVerb(oleobj, OLEIVERB_SHOW, NULL, &ClientSite,
                           0, (HWND)0xdeadbeef, &rect);
    ok(hres == S_OK, "DoVerb failed: %08x\n", hres);

    IOleObject_Release(oleobj);
}

static void test_GetMiscStatus(IOleObject *oleobj)
{
    DWORD st, i;
    HRESULT hres;

    for(i=0; i<10; i++) {
        st = 0xdeadbeef;
        hres = IOleObject_GetMiscStatus(oleobj, i, &st);
        ok(hres == S_OK, "GetMiscStatus failed: %08x\n", hres);
        ok(st == (OLEMISC_SETCLIENTSITEFIRST|OLEMISC_ACTIVATEWHENVISIBLE|OLEMISC_INSIDEOUT
                  |OLEMISC_CANTLINKINSIDE|OLEMISC_RECOMPOSEONRESIZE),
           "st=%08x, expected OLEMISC_SETCLIENTSITEFIRST|OLEMISC_ACTIVATEWHENVISIBLE|"
           "OLEMISC_INSIDEOUT|OLEMISC_CANTLINKINSIDE|OLEMISC_RECOMPOSEONRESIZE)\n", st);
    }
}

static void test_SetHostNames(IOleObject *oleobj)
{
    HRESULT hres;

    static const WCHAR test_appW[] =  {'t','e','s','t',' ','a','p','p',0};

    hres = IOleObject_SetHostNames(oleobj, test_appW, (void*)0xdeadbeef);
    ok(hres == S_OK, "SetHostNames failed: %08x\n", hres);
}

static void test_ClientSite(IUnknown *unk, IOleClientSite *client)
{
    IOleObject *oleobj;
    IOleInPlaceObject *inplace;
    HWND hwnd;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "QueryInterface(IID_OleObject) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    test_GetMiscStatus(oleobj);
    test_SetHostNames(oleobj);

    hres = IUnknown_QueryInterface(unk, &IID_IOleInPlaceObject, (void**)&inplace);
    ok(hres == S_OK, "QueryInterface(IID_OleInPlaceObject) failed: %08x\n", hres);
    if(FAILED(hres)) {
        IOleObject_Release(oleobj);
        return;
    }

    hres = IOleInPlaceObject_GetWindow(inplace, &hwnd);
    ok(hres == S_OK, "GetWindow failed: %08x\n", hres);
    ok((hwnd == NULL) ^ (client == NULL), "unexpected hwnd %p\n", hwnd);

    if(client) {
        SET_EXPECT(GetContainer);
        SET_EXPECT(Site_GetWindow);
        SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        SET_EXPECT(Invoke_AMBIENT_SILENT);
    }else if(!is_downloading) {
        SET_EXPECT(Invoke_DOWNLOADCOMPLETE);
        SET_EXPECT(Exec_SETDOWNLOADSTATE_0);
        SET_EXPECT(Invoke_COMMANDSTATECHANGE);
    }

    hres = IOleObject_SetClientSite(oleobj, client);
    ok(hres == S_OK, "SetClientSite failed: %08x\n", hres);

    if(client) {
        CHECK_CALLED(GetContainer);
        CHECK_CALLED(Site_GetWindow);
        CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
        CHECK_CALLED(Invoke_AMBIENT_SILENT);
    }else if(!is_downloading) {
        todo_wine CHECK_CALLED(Invoke_DOWNLOADCOMPLETE);
        todo_wine CHECK_CALLED(Exec_SETDOWNLOADSTATE_0);
        todo_wine CHECK_CALLED(Invoke_COMMANDSTATECHANGE);
   }

    hres = IOleInPlaceObject_GetWindow(inplace, &hwnd);
    ok(hres == S_OK, "GetWindow failed: %08x\n", hres);
    ok((hwnd == NULL) == (client == NULL), "unexpected hwnd %p\n", hwnd);

    shell_embedding_hwnd = hwnd;

    test_SetHostNames(oleobj);

    IOleInPlaceObject_Release(inplace);
    IOleObject_Release(oleobj);
}

static void test_ClassInfo(IUnknown *unk)
{
    IProvideClassInfo2 *class_info;
    GUID guid;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IProvideClassInfo2, (void**)&class_info);
    ok(hres == S_OK, "QueryInterface(IID_IProvideClassInfo)  failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IProvideClassInfo2_GetGUID(class_info, GUIDKIND_DEFAULT_SOURCE_DISP_IID, &guid);
    ok(hres == S_OK, "GetGUID failed: %08x\n", hres);
    ok(IsEqualGUID(&DIID_DWebBrowserEvents2, &guid), "wrong guid\n");

    hres = IProvideClassInfo2_GetGUID(class_info, 0, &guid);
    ok(hres == E_FAIL, "GetGUID failed: %08x, expected E_FAIL\n", hres);
    ok(IsEqualGUID(&IID_NULL, &guid), "wrong guid\n");

    hres = IProvideClassInfo2_GetGUID(class_info, 2, &guid);
    ok(hres == E_FAIL, "GetGUID failed: %08x, expected E_FAIL\n", hres);
    ok(IsEqualGUID(&IID_NULL, &guid), "wrong guid\n");

    hres = IProvideClassInfo2_GetGUID(class_info, GUIDKIND_DEFAULT_SOURCE_DISP_IID, NULL);
    ok(hres == E_POINTER, "GetGUID failed: %08x, expected E_POINTER\n", hres);

    hres = IProvideClassInfo2_GetGUID(class_info, 0, NULL);
    ok(hres == E_POINTER, "GetGUID failed: %08x, expected E_POINTER\n", hres);

    IProvideClassInfo2_Release(class_info);
}

static void test_ie_funcs(IUnknown *unk)
{
    IWebBrowser2 *wb;
    IDispatch *disp;
    VARIANT_BOOL b;
    int i;
    long hwnd;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IWebBrowser2, (void**)&wb);
    ok(hres == S_OK, "Could not get IWebBrowser2 interface: %08x\n", hres);
    if(FAILED(hres))
        return;

    /* HWND */

    hwnd = 0xdeadbeef;
    hres = IWebBrowser2_get_HWND(wb, &hwnd);
    ok(hres == E_FAIL, "get_HWND failed: %08x, expected E_FAIL\n", hres);
    ok(hwnd == 0, "unexpected hwnd %lx\n", hwnd);

    /* MenuBar */

    hres = IWebBrowser2_get_MenuBar(wb, &b);
    ok(hres == S_OK, "get_MenuBar failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONMENUBAR);
    hres = IWebBrowser2_put_MenuBar(wb, (exvb = VARIANT_FALSE));
    ok(hres == S_OK, "put_MenuBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONMENUBAR);

    hres = IWebBrowser2_get_MenuBar(wb, &b);
    ok(hres == S_OK, "get_MenuBar failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONMENUBAR);
    hres = IWebBrowser2_put_MenuBar(wb, (exvb = 100));
    ok(hres == S_OK, "put_MenuBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONMENUBAR);

    hres = IWebBrowser2_get_MenuBar(wb, &b);
    ok(hres == S_OK, "get_MenuBar failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    /* AddressBar */

    hres = IWebBrowser2_get_AddressBar(wb, &b);
    ok(hres == S_OK, "get_AddressBar failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONADDRESSBAR);
    hres = IWebBrowser2_put_AddressBar(wb, (exvb = VARIANT_FALSE));
    ok(hres == S_OK, "put_AddressBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONADDRESSBAR);

    hres = IWebBrowser2_get_AddressBar(wb, &b);
    ok(hres == S_OK, "get_MenuBar failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONADDRESSBAR);
    hres = IWebBrowser2_put_AddressBar(wb, (exvb = 100));
    ok(hres == S_OK, "put_AddressBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONADDRESSBAR);

    hres = IWebBrowser2_get_AddressBar(wb, &b);
    ok(hres == S_OK, "get_AddressBar failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONADDRESSBAR);
    hres = IWebBrowser2_put_AddressBar(wb, (exvb = VARIANT_TRUE));
    ok(hres == S_OK, "put_MenuBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONADDRESSBAR);

    /* StatusBar */

    hres = IWebBrowser2_get_StatusBar(wb, &b);
    ok(hres == S_OK, "get_StatusBar failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONSTATUSBAR);
    hres = IWebBrowser2_put_StatusBar(wb, (exvb = VARIANT_TRUE));
    ok(hres == S_OK, "put_StatusBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONSTATUSBAR);

    hres = IWebBrowser2_get_StatusBar(wb, &b);
    ok(hres == S_OK, "get_StatusBar failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONSTATUSBAR);
    hres = IWebBrowser2_put_StatusBar(wb, (exvb = VARIANT_FALSE));
    ok(hres == S_OK, "put_StatusBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONSTATUSBAR);

    hres = IWebBrowser2_get_StatusBar(wb, &b);
    ok(hres == S_OK, "get_StatusBar failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONSTATUSBAR);
    hres = IWebBrowser2_put_StatusBar(wb, (exvb = 100));
    ok(hres == S_OK, "put_StatusBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONSTATUSBAR);

    hres = IWebBrowser2_get_StatusBar(wb, &b);
    ok(hres == S_OK, "get_StatusBar failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    /* ToolBar */

    hres = IWebBrowser2_get_ToolBar(wb, &i);
    ok(hres == S_OK, "get_ToolBar failed: %08x\n", hres);
    ok(i, "i=%x\n", i);

    SET_EXPECT(Invoke_ONTOOLBAR);
    hres = IWebBrowser2_put_ToolBar(wb, (exvb = VARIANT_FALSE));
    ok(hres == S_OK, "put_ToolBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONTOOLBAR);

    hres = IWebBrowser2_get_ToolBar(wb, &i);
    ok(hres == S_OK, "get_ToolBar failed: %08x\n", hres);
    ok(i == VARIANT_FALSE, "b=%x\n", i);

    SET_EXPECT(Invoke_ONTOOLBAR);
    hres = IWebBrowser2_put_ToolBar(wb, (exvb = 100));
    ok(hres == S_OK, "put_ToolBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONTOOLBAR);

    hres = IWebBrowser2_get_ToolBar(wb, &i);
    ok(hres == S_OK, "get_ToolBar failed: %08x\n", hres);
    ok(i, "i=%x\n", i);

    SET_EXPECT(Invoke_ONTOOLBAR);
    hres = IWebBrowser2_put_ToolBar(wb, (exvb = VARIANT_TRUE));
    ok(hres == S_OK, "put_ToolBar failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONTOOLBAR);

    /* FullScreen */

    hres = IWebBrowser2_get_FullScreen(wb, &b);
    ok(hres == S_OK, "get_FullScreen failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONFULLSCREEN);
    hres = IWebBrowser2_put_FullScreen(wb, (exvb = VARIANT_TRUE));
    ok(hres == S_OK, "put_FullScreen failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONFULLSCREEN);

    hres = IWebBrowser2_get_FullScreen(wb, &b);
    ok(hres == S_OK, "get_FullScreen failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONFULLSCREEN);
    hres = IWebBrowser2_put_FullScreen(wb, (exvb = 100));
    ok(hres == S_OK, "put_FullScreen failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONFULLSCREEN);

    hres = IWebBrowser2_get_FullScreen(wb, &b);
    ok(hres == S_OK, "get_FullScreen failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONFULLSCREEN);
    hres = IWebBrowser2_put_FullScreen(wb, (exvb = VARIANT_FALSE));
    ok(hres == S_OK, "put_FullScreen failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONFULLSCREEN);

    /* TheaterMode */

    hres = IWebBrowser2_get_TheaterMode(wb, &b);
    ok(hres == S_OK, "get_TheaterMode failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONTHEATERMODE);
    hres = IWebBrowser2_put_TheaterMode(wb, (exvb = VARIANT_TRUE));
    ok(hres == S_OK, "put_TheaterMode failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONTHEATERMODE);

    hres = IWebBrowser2_get_TheaterMode(wb, &b);
    ok(hres == S_OK, "get_TheaterMode failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONTHEATERMODE);
    hres = IWebBrowser2_put_TheaterMode(wb, (exvb = 100));
    ok(hres == S_OK, "put_TheaterMode failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONTHEATERMODE);

    hres = IWebBrowser2_get_TheaterMode(wb, &b);
    ok(hres == S_OK, "get_TheaterMode failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    SET_EXPECT(Invoke_ONTHEATERMODE);
    hres = IWebBrowser2_put_TheaterMode(wb, (exvb = VARIANT_FALSE));
    ok(hres == S_OK, "put_TheaterMode failed: %08x\n", hres);
    CHECK_CALLED(Invoke_ONTHEATERMODE);

    /* Resizable */

    b = 0x100;
    hres = IWebBrowser2_get_Resizable(wb, &b);
    ok(hres == E_NOTIMPL, "get_Resizable failed: %08x\n", hres);
    ok(b == 0x100, "b=%x\n", b);

    SET_EXPECT(Invoke_WINDOWSETRESIZABLE);
    hres = IWebBrowser2_put_Resizable(wb, (exvb = VARIANT_TRUE));
    ok(hres == S_OK || broken(hres == E_NOTIMPL), "put_Resizable failed: %08x\n", hres);
    CHECK_CALLED_BROKEN(Invoke_WINDOWSETRESIZABLE);

    SET_EXPECT(Invoke_WINDOWSETRESIZABLE);
    hres = IWebBrowser2_put_Resizable(wb, (exvb = VARIANT_FALSE));
    ok(hres == S_OK || broken(hres == E_NOTIMPL), "put_Resizable failed: %08x\n", hres);
    CHECK_CALLED_BROKEN(Invoke_WINDOWSETRESIZABLE);

    hres = IWebBrowser2_get_Resizable(wb, &b);
    ok(hres == E_NOTIMPL, "get_Resizable failed: %08x\n", hres);
    ok(b == 0x100, "b=%x\n", b);

    /* Application */

    disp = NULL;
    hres = IWebBrowser2_get_Application(wb, &disp);
    ok(hres == S_OK, "get_Application failed: %08x\n", hres);
    ok(disp == (void*)wb, "disp=%p, expected %p\n", disp, wb);
    if(disp)
        IDispatch_Release(disp);

    hres = IWebBrowser2_get_Application(wb, NULL);
    ok(hres == E_POINTER, "get_Application failed: %08x, expected E_POINTER\n", hres);

    /* Quit */

    hres = IWebBrowser2_Quit(wb);
    ok(hres == E_FAIL, "Quit failed: %08x, expected E_FAIL\n", hres);

    IWebBrowser2_Release(wb);
}

static void test_Silent(IWebBrowser2 *wb, IOleControl *control, BOOL is_clientsite)
{
    VARIANT_BOOL b;
    HRESULT hres;

    b = 100;
    hres = IWebBrowser2_get_Silent(wb, &b);
    ok(hres == S_OK, "get_Silent failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    hres = IWebBrowser2_put_Silent(wb, VARIANT_TRUE);
    ok(hres == S_OK, "set_Silent failed: %08x\n", hres);

    b = 100;
    hres = IWebBrowser2_get_Silent(wb, &b);
    ok(hres == S_OK, "get_Silent failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    hres = IWebBrowser2_put_Silent(wb, 100);
    ok(hres == S_OK, "set_Silent failed: %08x\n", hres);

    b = 100;
    hres = IWebBrowser2_get_Silent(wb, &b);
    ok(hres == S_OK, "get_Silent failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    hres = IWebBrowser2_put_Silent(wb, VARIANT_FALSE);
    ok(hres == S_OK, "set_Silent failed: %08x\n", hres);

    b = 100;
    hres = IWebBrowser2_get_Silent(wb, &b);
    ok(hres == S_OK, "get_Silent failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    if(is_clientsite) {
        hres = IWebBrowser2_put_Silent(wb, VARIANT_TRUE);
        ok(hres == S_OK, "set_Silent failed: %08x\n", hres);

        SET_EXPECT(Invoke_AMBIENT_SILENT);
    }

    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_AMBIENT_SILENT);
    ok(hres == S_OK, "OnAmbientPropertyChange failed %08x\n", hres);

    if(is_clientsite)
        CHECK_CALLED(Invoke_AMBIENT_SILENT);

    b = 100;
    hres = IWebBrowser2_get_Silent(wb, &b);
    ok(hres == S_OK, "get_Silent failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);
}

static void test_Offline(IWebBrowser2 *wb, IOleControl *control, BOOL is_clientsite)
{
    VARIANT_BOOL b;
    HRESULT hres;

    b = 100;
    hres = IWebBrowser2_get_Offline(wb, &b);
    ok(hres == S_OK, "get_Offline failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    hres = IWebBrowser2_put_Offline(wb, VARIANT_TRUE);
    ok(hres == S_OK, "set_Offline failed: %08x\n", hres);

    b = 100;
    hres = IWebBrowser2_get_Offline(wb, &b);
    ok(hres == S_OK, "get_Offline failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    hres = IWebBrowser2_put_Offline(wb, 100);
    ok(hres == S_OK, "set_Offline failed: %08x\n", hres);

    b = 100;
    hres = IWebBrowser2_get_Offline(wb, &b);
    ok(hres == S_OK, "get_Offline failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "b=%x\n", b);

    hres = IWebBrowser2_put_Offline(wb, VARIANT_FALSE);
    ok(hres == S_OK, "set_Offline failed: %08x\n", hres);

    b = 100;
    hres = IWebBrowser2_get_Offline(wb, &b);
    ok(hres == S_OK, "get_Offline failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "b=%x\n", b);

    if(is_clientsite) {
        hres = IWebBrowser2_put_Offline(wb, VARIANT_TRUE);
        ok(hres == S_OK, "set_Offline failed: %08x\n", hres);

        SET_EXPECT(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);
    }

    hres = IOleControl_OnAmbientPropertyChange(control, DISPID_AMBIENT_OFFLINEIFNOTCONNECTED);
    ok(hres == S_OK, "OnAmbientPropertyChange failed %08x\n", hres);

    if(is_clientsite)
        CHECK_CALLED(Invoke_AMBIENT_OFFLINEIFNOTCONNECTED);

    b = 100;
    hres = IWebBrowser2_get_Offline(wb, &b);
    ok(hres == S_OK, "get_Offline failed: %08x\n", hres);
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
    ok(hres == S_OK, "OnAmbientPropertyChange failed %08x\n", hres);

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

static void test_wb_funcs(IUnknown *unk, BOOL is_clientsite)
{
    IWebBrowser2 *wb;
    IOleControl *control;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IWebBrowser2, (void**)&wb);
    ok(hres == S_OK, "Could not get IWebBrowser2 interface: %08x\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IOleControl, (void**)&control);
    ok(hres == S_OK, "Could not get IOleControl interface: %08x\n", hres);

    test_Silent(wb, control, is_clientsite);
    test_Offline(wb, control, is_clientsite);
    test_ambient_unknown(wb, control, is_clientsite);

    IWebBrowser_Release(wb);
    IOleControl_Release(control);
}

static void test_GetControlInfo(IUnknown *unk)
{
    IOleControl *control;
    CONTROLINFO info;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleControl, (void**)&control);
    ok(hres == S_OK, "Could not get IOleControl: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IOleControl_GetControlInfo(control, &info);
    ok(hres == E_NOTIMPL, "GetControlInfo failed: %08x, exxpected E_NOTIMPL\n", hres);
    hres = IOleControl_GetControlInfo(control, NULL);
    ok(hres == E_NOTIMPL, "GetControlInfo failed: %08x, exxpected E_NOTIMPL\n", hres);

    IOleControl_Release(control);
}

static void test_Extent(IUnknown *unk)
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
        trace("dpi: %d / %d\n", dpi_y, dpi_y);

    hres = IUnknown_QueryInterface(unk, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "Could not get IOleObkect: %08x\n", hres);
    if(FAILED(hres))
        return;

    size.cx = size.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, DVASPECT_CONTENT, &size);
    ok(hres == S_OK, "GetExtent failed: %08x\n", hres);
    /* Default size is 50x20 pixels, in himetric units */
    expected.cx = MulDiv( 50, 2540, dpi_x );
    expected.cy = MulDiv( 20, 2540, dpi_y );
    ok(size.cx == expected.cx && size.cy == expected.cy, "size = {%d %d} (expected %d %d)\n",
       size.cx, size.cy, expected.cx, expected.cy );

    size.cx = 800;
    size.cy = 700;
    hres = IOleObject_SetExtent(oleobj, DVASPECT_CONTENT, &size);
    ok(hres == S_OK, "SetExtent failed: %08x\n", hres);

    size.cx = size.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, DVASPECT_CONTENT, &size);
    ok(hres == S_OK, "GetExtent failed: %08x\n", hres);
    ok(size.cx == 800 && size.cy == 700, "size = {%d %d}\n", size.cx, size.cy);

    size.cx = size.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, 0, &size);
    ok(hres == S_OK, "GetExtent failed: %08x\n", hres);
    ok(size.cx == 800 && size.cy == 700, "size = {%d %d}\n", size.cx, size.cy);

    size.cx = 900;
    size.cy = 800;
    hres = IOleObject_SetExtent(oleobj, 0, &size);
    ok(hres == S_OK, "SetExtent failed: %08x\n", hres);

    size.cx = size.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, 0, &size);
    ok(hres == S_OK, "GetExtent failed: %08x\n", hres);
    ok(size.cx == 900 && size.cy == 800, "size = {%d %d}\n", size.cx, size.cy);

    size.cx = size.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, 0xdeadbeef, &size);
    ok(hres == S_OK, "GetExtent failed: %08x\n", hres);
    ok(size.cx == 900 && size.cy == 800, "size = {%d %d}\n", size.cx, size.cy);

    size.cx = 1000;
    size.cy = 900;
    hres = IOleObject_SetExtent(oleobj, 0xdeadbeef, &size);
    ok(hres == S_OK, "SetExtent failed: %08x\n", hres);

    size.cx = size.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, 0xdeadbeef, &size);
    ok(hres == S_OK, "GetExtent failed: %08x\n", hres);
    ok(size.cx == 1000 && size.cy == 900, "size = {%d %d}\n", size.cx, size.cy);

    size.cx = size.cy = 0xdeadbeef;
    hres = IOleObject_GetExtent(oleobj, DVASPECT_CONTENT, &size);
    ok(hres == S_OK, "GetExtent failed: %08x\n", hres);
    ok(size.cx == 1000 && size.cy == 900, "size = {%d %d}\n", size.cx, size.cy);

    IOleObject_Release(oleobj);
}

static void test_ConnectionPoint(IUnknown *unk, BOOL init)
{
    IConnectionPointContainer *container;
    IConnectionPoint *point;
    HRESULT hres;

    static DWORD dw = 100;

    hres = IUnknown_QueryInterface(unk, &IID_IConnectionPointContainer, (void**)&container);
    ok(hres == S_OK, "QueryInterface(IID_IConnectionPointContainer) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IConnectionPointContainer_FindConnectionPoint(container, &DIID_DWebBrowserEvents2, &point);
    IConnectionPointContainer_Release(container);
    ok(hres == S_OK, "FindConnectionPoint failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    if(init) {
        hres = IConnectionPoint_Advise(point, (IUnknown*)&WebBrowserEvents2, &dw);
        ok(hres == S_OK, "Advise failed: %08x\n", hres);
        ok(dw == 1, "dw=%d, expected 1\n", dw);
    }else {
        hres = IConnectionPoint_Unadvise(point, dw);
        ok(hres == S_OK, "Unadvise failed: %08x\n", hres);
    }

    IConnectionPoint_Release(point);
}

static void test_Navigate2(IUnknown *unk)
{
    IWebBrowser2 *webbrowser;
    VARIANT url;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IWebBrowser2, (void**)&webbrowser);
    ok(hres == S_OK, "QueryInterface(IID_IWebBrowser) failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    test_LocationURL(unk, emptyW);

    V_VT(&url) = VT_BSTR;
    V_BSTR(&url) = SysAllocString(about_blankW);

    SET_EXPECT(Invoke_AMBIENT_USERMODE);
    SET_EXPECT(Invoke_PROPERTYCHANGE);
    SET_EXPECT(Invoke_BEFORENAVIGATE2);
    SET_EXPECT(Invoke_DOWNLOADBEGIN);
    SET_EXPECT(Exec_SETDOWNLOADSTATE_1);
    SET_EXPECT(EnableModeless_FALSE);
    SET_EXPECT(Invoke_STATUSTEXTCHANGE);
    SET_EXPECT(SetStatusText);
    SET_EXPECT(GetHostInfo);
    SET_EXPECT(Invoke_AMBIENT_DLCONTROL);
    SET_EXPECT(Invoke_AMBIENT_USERAGENT);
    SET_EXPECT(Invoke_AMBIENT_PALETTE);
    SET_EXPECT(GetOptionKeyPath);
    SET_EXPECT(GetOverridesKeyPath);
    SET_EXPECT(QueryStatus_SETPROGRESSTEXT);
    SET_EXPECT(Exec_SETPROGRESSMAX);
    SET_EXPECT(Exec_SETPROGRESSPOS);
    SET_EXPECT(Invoke_SETSECURELOCKICON);
    SET_EXPECT(Invoke_FILEDOWNLOAD);
    SET_EXPECT(Exec_SETDOWNLOADSTATE_0);
    SET_EXPECT(Invoke_COMMANDSTATECHANGE);
    SET_EXPECT(EnableModeless_TRUE);

    hres = IWebBrowser2_Navigate2(webbrowser, &url, NULL, NULL, NULL, NULL);
    ok(hres == S_OK, "Navigate2 failed: %08x\n", hres);

    CHECK_CALLED(Invoke_AMBIENT_USERMODE);
    todo_wine CHECK_CALLED(Invoke_PROPERTYCHANGE);
    CHECK_CALLED(Invoke_BEFORENAVIGATE2);
    todo_wine CHECK_CALLED(Invoke_DOWNLOADBEGIN);
    todo_wine CHECK_CALLED(Exec_SETDOWNLOADSTATE_1);
    CHECK_CALLED(EnableModeless_FALSE);
    CHECK_CALLED(Invoke_STATUSTEXTCHANGE);
    CHECK_CALLED(SetStatusText);
    CHECK_CALLED(GetHostInfo);
    CHECK_CALLED(Invoke_AMBIENT_DLCONTROL);
    CHECK_CALLED(Invoke_AMBIENT_USERAGENT);
    CHECK_CALLED(Invoke_AMBIENT_PALETTE);
    CHECK_CALLED(GetOptionKeyPath);
    CHECK_CALLED_BROKEN(GetOverridesKeyPath);
    todo_wine CHECK_CALLED(QueryStatus_SETPROGRESSTEXT);
    todo_wine CHECK_CALLED(Exec_SETPROGRESSMAX);
    todo_wine CHECK_CALLED(Exec_SETPROGRESSPOS);
    todo_wine CHECK_CALLED_BROKEN(Invoke_SETSECURELOCKICON);
    todo_wine CHECK_CALLED_BROKEN(Invoke_FILEDOWNLOAD);
    todo_wine CHECK_CALLED(Invoke_COMMANDSTATECHANGE);
    todo_wine CHECK_CALLED(Exec_SETDOWNLOADSTATE_0);
    CHECK_CALLED(EnableModeless_TRUE);

    VariantClear(&url);
    IWebBrowser2_Release(webbrowser);
}

static void test_download(void)
{
    MSG msg;

    is_downloading = TRUE;

    SET_EXPECT(Exec_SETPROGRESSMAX);
    SET_EXPECT(Exec_SETPROGRESSPOS);
    SET_EXPECT(Exec_SETDOWNLOADSTATE_1);
    SET_EXPECT(DocHost_EnableModeless_FALSE);
    SET_EXPECT(DocHost_EnableModeless_TRUE);
    SET_EXPECT(Invoke_SETSECURELOCKICON);
    SET_EXPECT(Invoke_282);
    SET_EXPECT(EnableModeless_FALSE);
    SET_EXPECT(Invoke_COMMANDSTATECHANGE);
    SET_EXPECT(Invoke_STATUSTEXTCHANGE);
    SET_EXPECT(SetStatusText);
    SET_EXPECT(EnableModeless_TRUE);
    SET_EXPECT(QueryStatus_STOP);
    SET_EXPECT(Exec_SETDOWNLOADSTATE_0);
    SET_EXPECT(Invoke_TITLECHANGE);
    SET_EXPECT(Invoke_NAVIGATECOMPLETE2);
    SET_EXPECT(GetDropTarget);
    SET_EXPECT(Invoke_PROGRESSCHANGE);
    SET_EXPECT(Invoke_DOCUMENTCOMPLETE);

    while(!called_Invoke_DOCUMENTCOMPLETE && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    todo_wine CHECK_CALLED(Exec_SETPROGRESSMAX);
    todo_wine CHECK_CALLED(Exec_SETPROGRESSPOS);
    todo_wine CHECK_CALLED(Exec_SETDOWNLOADSTATE_1);
    CLEAR_CALLED(DocHost_EnableModeless_FALSE); /* IE 7 */
    CLEAR_CALLED(DocHost_EnableModeless_TRUE); /* IE 7 */
    todo_wine CHECK_CALLED(Invoke_SETSECURELOCKICON);
    CLEAR_CALLED(Invoke_282); /* IE 7 */
    todo_wine CHECK_CALLED(EnableModeless_FALSE);
    todo_wine CHECK_CALLED(Invoke_COMMANDSTATECHANGE);
    todo_wine CHECK_CALLED(Invoke_STATUSTEXTCHANGE);
    todo_wine CHECK_CALLED(SetStatusText);
    todo_wine CHECK_CALLED(EnableModeless_TRUE);
    todo_wine CHECK_CALLED(QueryStatus_STOP);
    todo_wine CHECK_CALLED(Exec_SETDOWNLOADSTATE_0);
    todo_wine CHECK_CALLED(Invoke_TITLECHANGE);
    CHECK_CALLED(Invoke_NAVIGATECOMPLETE2);
    todo_wine CHECK_CALLED(GetDropTarget);
    todo_wine CHECK_CALLED(Invoke_PROGRESSCHANGE);
    CHECK_CALLED(Invoke_DOCUMENTCOMPLETE);
}

static void test_olecmd(IUnknown *unk, BOOL loaded)
{
    IOleCommandTarget *cmdtrg;
    OLECMD cmds[3];
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleCommandTarget, (void**)&cmdtrg);
    ok(hres == S_OK, "Could not get IOleCommandTarget iface: %08x\n", hres);
    if(FAILED(hres))
        return;

    cmds[0].cmdID = OLECMDID_SPELL;
    cmds[0].cmdf = 0xdeadbeef;
    cmds[1].cmdID = OLECMDID_REFRESH;
    cmds[1].cmdf = 0xdeadbeef;
    hres = IOleCommandTarget_QueryStatus(cmdtrg, NULL, 2, cmds, NULL);
    if(loaded) {
        ok(hres == S_OK, "QueryStatus failed: %08x\n", hres);
        ok(cmds[0].cmdf == OLECMDF_SUPPORTED, "OLECMDID_SPELL cmdf = %x\n", cmds[0].cmdf);
        ok(cmds[1].cmdf == (OLECMDF_ENABLED|OLECMDF_SUPPORTED),
           "OLECMDID_REFRESH cmdf = %x\n", cmds[1].cmdf);
    }else {
        ok(hres == 0x80040104, "QueryStatus failed: %08x\n", hres);
        ok(cmds[0].cmdf == 0xdeadbeef, "OLECMDID_SPELL cmdf = %x\n", cmds[0].cmdf);
        ok(cmds[1].cmdf == 0xdeadbeef, "OLECMDID_REFRESH cmdf = %x\n", cmds[0].cmdf);
    }

    IOleCommandTarget_Release(cmdtrg);
}

static void test_IServiceProvider(IUnknown *unk)
{
    IServiceProvider *servprov = (void*)0xdeadbeef;
    HRESULT hres;
    IUnknown *ret = NULL;
    static const IID IID_IBrowserService2 =
        {0x68BD21CC,0x438B,0x11d2,{0xA5,0x60,0x00,0xA0,0xC,0x2D,0xBF,0xE8}};

    hres = IUnknown_QueryInterface(unk, &IID_IServiceProvider, (void**)&servprov);
    ok(hres == S_OK, "QueryInterface returned %08x, expected S_OK\n", hres);
    if(FAILED(hres))
        return;

    hres = IServiceProvider_QueryService(servprov, &SID_STopLevelBrowser, &IID_IBrowserService2, (LPVOID*)&ret);
    ok(hres == E_FAIL, "QueryService returned %08x, expected E_FAIL\n", hres);
    ok(ret == NULL, "ret returned %p, expected NULL\n", ret);
    if(hres == S_OK)
    {
        IUnknown_Release(ret);
    }

    IServiceProvider_Release(servprov);
}

static void test_QueryInterface(IUnknown *unk)
{
    IQuickActivate *qa = (IQuickActivate*)0xdeadbeef;
    IRunnableObject *runnable = (IRunnableObject*)0xdeadbeef;
    IPerPropertyBrowsing *propbrowse = (void*)0xdeadbeef;
    IOleInPlaceSite *inplace = (void*)0xdeadbeef;
    IOleCache *cache = (void*)0xdeadbeef;
    IObjectWithSite *site = (void*)0xdeadbeef;
    IViewObjectEx *viewex = (void*)0xdeadbeef;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IQuickActivate, (void**)&qa);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(qa == NULL, "qa=%p, expected NULL\n", qa);

    hres = IUnknown_QueryInterface(unk, &IID_IRunnableObject, (void**)&runnable);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(runnable == NULL, "runnable=%p, expected NULL\n", runnable);

    hres = IUnknown_QueryInterface(unk, &IID_IPerPropertyBrowsing, (void**)&propbrowse);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(propbrowse == NULL, "propbrowse=%p, expected NULL\n", propbrowse);

    hres = IUnknown_QueryInterface(unk, &IID_IOleCache, (void**)&cache);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(cache == NULL, "cache=%p, expected NULL\n", cache);

    hres = IUnknown_QueryInterface(unk, &IID_IOleInPlaceSite, (void**)&inplace);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(inplace == NULL, "inplace=%p, expected NULL\n", inplace);

    hres = IUnknown_QueryInterface(unk, &IID_IObjectWithSite, (void**)&site);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(site == NULL, "site=%p, expected NULL\n", site);

    hres = IUnknown_QueryInterface(unk, &IID_IViewObjectEx, (void**)&viewex);
    ok(hres == E_NOINTERFACE, "QueryInterface returned %08x, expected E_NOINTERFACE\n", hres);
    ok(viewex == NULL, "viewex=%p, expected NULL\n", viewex);

}

static void test_WebBrowser(BOOL do_download)
{
    IUnknown *unk = NULL;
    ULONG ref;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_WebBrowser, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoCreateInterface failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    is_downloading = FALSE;

    hres = IUnknown_QueryInterface(unk, &IID_IWebBrowser2, (void**)&wb);
    ok(hres == S_OK, "Could not get IWebBrowser2 iface: %08x\n", hres);

    test_QueryInterface(unk);
    test_ClassInfo(unk);
    test_LocationURL(unk, emptyW);
    test_ConnectionPoint(unk, TRUE);
    test_ClientSite(unk, &ClientSite);
    test_Extent(unk);
    test_wb_funcs(unk, TRUE);
    test_DoVerb(unk);
    test_olecmd(unk, FALSE);
    test_Navigate2(unk);

    if(do_download) {
        test_download();
        test_olecmd(unk, TRUE);
    }

    test_ClientSite(unk, NULL);
    test_ie_funcs(unk);
    test_GetControlInfo(unk);
    test_wb_funcs(unk, FALSE);
    test_ConnectionPoint(unk, FALSE);
    test_IServiceProvider(unk);

    IWebBrowser2_Release(wb);
    ref = IUnknown_Release(unk);
    ok(ref == 0, "ref=%d, expected 0\n", ref);
}

static void gecko_installer_workaround(BOOL disable)
{
    HKEY hkey;
    DWORD res;

    static BOOL has_url = FALSE;
    static char url[2048];

    if(!disable && !has_url)
        return;

    res = RegOpenKey(HKEY_CURRENT_USER, "Software\\Wine\\MSHTML", &hkey);
    if(res != ERROR_SUCCESS)
        return;

    if(disable) {
        DWORD type, size = sizeof(url);

        res = RegQueryValueEx(hkey, "GeckoUrl", NULL, &type, (PVOID)url, &size);
        if(res == ERROR_SUCCESS && type == REG_SZ)
            has_url = TRUE;

        RegDeleteValue(hkey, "GeckoUrl");
    }else {
        RegSetValueEx(hkey, "GeckoUrl", 0, REG_SZ, (PVOID)url, lstrlenA(url)+1);
    }

    RegCloseKey(hkey);
}

START_TEST(webbrowser)
{
    gecko_installer_workaround(TRUE);

    container_hwnd = create_container_window();

    OleInitialize(NULL);

    trace("Testing WebBrowser (no download)...\n");
    test_WebBrowser(FALSE);
    trace("Testing WebBrowser...\n");
    test_WebBrowser(TRUE);

    OleUninitialize();

    gecko_installer_workaround(FALSE);
}
