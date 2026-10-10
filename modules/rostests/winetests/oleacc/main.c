/*
 * oleacc tests
 *
 * Copyright 2008 Nikolay Sivov
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

#include "wine/test.h"
#include <stdio.h>

#include "initguid.h"
#include <ole2.h>
#include <commctrl.h>
#include <oleacc.h>
#include <servprov.h>

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

DEFINE_EXPECT(winproc_GETOBJECT);
DEFINE_EXPECT(Accessible_QI_IEnumVARIANT);
DEFINE_EXPECT(Accessible_get_accChildCount);
DEFINE_EXPECT(Accessible_get_accChild);
DEFINE_EXPECT(Accessible_get_accName);
DEFINE_EXPECT(Accessible_get_accParent);
DEFINE_EXPECT(Accessible_accNavigate);
DEFINE_EXPECT(Accessible_child_get_accName);
DEFINE_EXPECT(Accessible_child_get_accParent);
DEFINE_EXPECT(Accessible_child_accNavigate);

#define NAVDIR_INTERNAL_HWND 10
DEFINE_GUID(SID_AccFromDAWrapper, 0x33f139ee, 0xe509, 0x47f7, 0xbf,0x39, 0x83,0x76,0x44,0xf7,0x45,0x76);

static HANDLE (WINAPI *pGetProcessHandleFromHwnd)(HWND);

static BOOL init(void)
{
    HMODULE oleacc = GetModuleHandleA("oleacc.dll");

    pGetProcessHandleFromHwnd = (void*)GetProcAddress(oleacc, "GetProcessHandleFromHwnd");
    if(!pGetProcessHandleFromHwnd) {
        win_skip("GetProcessHandleFromHwnd not available\n");
        return FALSE;
    }

    return TRUE;
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

static struct Accessible
{
    IAccessible IAccessible_iface;
    IOleWindow IOleWindow_iface;
    LONG ref;

    IAccessible *parent;
    HWND acc_hwnd;
    HWND ow_hwnd;
} Accessible, Accessible_child;

static inline struct Accessible* impl_from_Accessible(IAccessible *iface)
{
    return CONTAINING_RECORD(iface, struct Accessible, IAccessible_iface);
}

static HRESULT WINAPI Accessible_QueryInterface(
        IAccessible *iface, REFIID riid, void **ppvObject)
{
    struct Accessible *This = impl_from_Accessible(iface);

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IDispatch) ||
            IsEqualIID(riid, &IID_IAccessible)) {
        IAccessible_AddRef(iface);
        *ppvObject = iface;
        return S_OK;
    }

    if(IsEqualIID(riid, &IID_IOleWindow)) {
        *ppvObject = &This->IOleWindow_iface;
        IOleWindow_AddRef(&This->IOleWindow_iface);
        return S_OK;
    }

    if(IsEqualIID(riid, &IID_IEnumVARIANT)) {
        CHECK_EXPECT(Accessible_QI_IEnumVARIANT);
        return E_NOINTERFACE;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI Accessible_AddRef(IAccessible *iface)
{
    struct Accessible *This = impl_from_Accessible(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI Accessible_Release(IAccessible *iface)
{
    struct Accessible *This = impl_from_Accessible(iface);
    return InterlockedDecrement(&This->ref);
}

static HRESULT WINAPI Accessible_GetTypeInfoCount(
        IAccessible *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_GetTypeInfo(IAccessible *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_GetIDsOfNames(IAccessible *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_Invoke(IAccessible *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accParent(
        IAccessible *iface, IDispatch **ppdispParent)
{
    struct Accessible *This = impl_from_Accessible(iface);

    if(This == &Accessible_child)
        CHECK_EXPECT(Accessible_child_get_accParent);
    else
        CHECK_EXPECT(Accessible_get_accParent);

    if(This->parent)
    {
        return IAccessible_QueryInterface(This->parent, &IID_IDispatch,
                (void **)ppdispParent);
    }
    *ppdispParent = NULL;
    return S_FALSE;
}

static HRESULT WINAPI Accessible_get_accChildCount(
        IAccessible *iface, LONG *pcountChildren)
{
    struct Accessible *This = impl_from_Accessible(iface);

    ok(This == &Accessible, "unexpected call\n");
    CHECK_EXPECT(Accessible_get_accChildCount);
    *pcountChildren = 1;
    return S_OK;
}

static HRESULT WINAPI Accessible_get_accChild(IAccessible *iface,
        VARIANT varChildID, IDispatch **ppdispChild)
{
    struct Accessible *This = impl_from_Accessible(iface);

    ok(This == &Accessible, "unexpected call\n");
    CHECK_EXPECT(Accessible_get_accChild);
    ok(V_VT(&varChildID) == VT_I4, "V_VT(&varChildID) = %d\n", V_VT(&varChildID));

    switch(V_I4(&varChildID))
    {
    case 1:
        *ppdispChild = NULL;
        return S_OK;
    case 2:
        *ppdispChild = NULL;
        return S_FALSE;
    case 3:
        *ppdispChild = (IDispatch*)&Accessible_child.IAccessible_iface;
        IDispatch_AddRef(*ppdispChild);
        return S_OK;
    case 4:
        *ppdispChild = (IDispatch*)&Accessible_child.IAccessible_iface;
        IDispatch_AddRef(*ppdispChild);
        return S_FALSE;
    default:
        ok(0, "unexpected call\n");
        return E_NOTIMPL;
    }
}

static HRESULT WINAPI Accessible_get_accName(IAccessible *iface,
        VARIANT varID, BSTR *pszName)
{
    struct Accessible *This = impl_from_Accessible(iface);

    if(This == &Accessible_child)
        CHECK_EXPECT(Accessible_child_get_accName);
    else
        CHECK_EXPECT(Accessible_get_accName);

    ok(!pszName, "pszName != NULL\n");
    return E_INVALIDARG;
}

static HRESULT WINAPI Accessible_get_accValue(IAccessible *iface,
        VARIANT varID, BSTR *pszValue)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accDescription(IAccessible *iface,
        VARIANT varID, BSTR *pszDescription)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accRole(IAccessible *iface,
        VARIANT varID, VARIANT *pvarRole)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accState(IAccessible *iface,
        VARIANT varID, VARIANT *pvarState)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accHelp(IAccessible *iface,
        VARIANT varID, BSTR *pszHelp)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accHelpTopic(IAccessible *iface,
        BSTR *pszHelpFile, VARIANT varID, LONG *pidTopic)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accKeyboardShortcut(IAccessible *iface,
        VARIANT varID, BSTR *pszKeyboardShortcut)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accFocus(IAccessible *iface, VARIANT *pvarID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accSelection(
        IAccessible *iface, VARIANT *pvarID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accDefaultAction(IAccessible *iface,
        VARIANT varID, BSTR *pszDefaultAction)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_accSelect(IAccessible *iface,
        LONG flagsSelect, VARIANT varID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_accLocation(IAccessible *iface, LONG *pxLeft,
        LONG *pyTop, LONG *pcxWidth, LONG *pcyHeight, VARIANT varID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_accNavigate(IAccessible *iface,
        LONG navDir, VARIANT varStart, VARIANT *pvarEnd)
{
    struct Accessible *This = impl_from_Accessible(iface);

    if(This == &Accessible_child)
        CHECK_EXPECT(Accessible_child_accNavigate);
    else
        CHECK_EXPECT(Accessible_accNavigate);

    /*
     * Magic number value for retrieving an HWND. Used by DynamicAnnotation
     * IAccessible wrapper.
     */
    if(navDir == NAVDIR_INTERNAL_HWND) {
        V_VT(pvarEnd) = VT_I4;
        V_I4(pvarEnd) = HandleToULong(This->acc_hwnd);
        return V_I4(pvarEnd) ? S_OK : S_FALSE;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_accHitTest(IAccessible *iface,
        LONG xLeft, LONG yTop, VARIANT *pvarID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_accDoDefaultAction(
        IAccessible *iface, VARIANT varID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_put_accName(IAccessible *iface,
        VARIANT varID, BSTR pszName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_put_accValue(IAccessible *iface,
        VARIANT varID, BSTR pszValue)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IAccessibleVtbl AccessibleVtbl = {
    Accessible_QueryInterface,
    Accessible_AddRef,
    Accessible_Release,
    Accessible_GetTypeInfoCount,
    Accessible_GetTypeInfo,
    Accessible_GetIDsOfNames,
    Accessible_Invoke,
    Accessible_get_accParent,
    Accessible_get_accChildCount,
    Accessible_get_accChild,
    Accessible_get_accName,
    Accessible_get_accValue,
    Accessible_get_accDescription,
    Accessible_get_accRole,
    Accessible_get_accState,
    Accessible_get_accHelp,
    Accessible_get_accHelpTopic,
    Accessible_get_accKeyboardShortcut,
    Accessible_get_accFocus,
    Accessible_get_accSelection,
    Accessible_get_accDefaultAction,
    Accessible_accSelect,
    Accessible_accLocation,
    Accessible_accNavigate,
    Accessible_accHitTest,
    Accessible_accDoDefaultAction,
    Accessible_put_accName,
    Accessible_put_accValue
};

static inline struct Accessible* impl_from_OleWindow(IOleWindow *iface)
{
    return CONTAINING_RECORD(iface, struct Accessible, IOleWindow_iface);
}

static HRESULT WINAPI OleWindow_QueryInterface(IOleWindow *iface, REFIID riid, void **obj)
{
    struct Accessible *This = impl_from_OleWindow(iface);
    return IAccessible_QueryInterface(&This->IAccessible_iface, riid, obj);
}

static ULONG WINAPI OleWindow_AddRef(IOleWindow *iface)
{
    struct Accessible *This = impl_from_OleWindow(iface);
    return IAccessible_AddRef(&This->IAccessible_iface);
}

static ULONG WINAPI OleWindow_Release(IOleWindow *iface)
{
    struct Accessible *This = impl_from_OleWindow(iface);
    return IAccessible_Release(&This->IAccessible_iface);
}

static HRESULT WINAPI OleWindow_GetWindow(IOleWindow *iface, HWND *hwnd)
{
    struct Accessible *This = impl_from_OleWindow(iface);

    *hwnd = This->ow_hwnd;
    return *hwnd ? S_OK : E_FAIL;
}

static HRESULT WINAPI OleWindow_ContextSensitiveHelp(IOleWindow *iface, BOOL f_enter_mode)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleWindowVtbl OleWindowVtbl = {
    OleWindow_QueryInterface,
    OleWindow_AddRef,
    OleWindow_Release,
    OleWindow_GetWindow,
    OleWindow_ContextSensitiveHelp
};

static struct Accessible Accessible =
{
    { &AccessibleVtbl },
    { &OleWindowVtbl },
    1,
    NULL,
    0, 0
};
static struct Accessible Accessible_child =
{
    { &AccessibleVtbl },
    { &OleWindowVtbl },
    1,
    &Accessible.IAccessible_iface,
    0, 0
};

static void test_getroletext(void)
{
    INT ret, role;
    CHAR buf[2], *buff;
    WCHAR bufW[2], *buffW;

    /* wrong role number */
    ret = GetRoleTextA(-1, NULL, 0);
    ok(ret == 0, "GetRoleTextA doesn't return zero on wrong role number, got %d\n", ret);
    buf[0] = '*';
    ret = GetRoleTextA(-1, buf, 2);
    ok(ret == 0, "GetRoleTextA doesn't return zero on wrong role number, got %d\n", ret);
    ok(buf[0] == 0, "GetRoleTextA doesn't return NULL char on wrong role number\n");
    buf[0] = '*';
    ret = GetRoleTextA(-1, buf, 0);
    ok(ret == 0, "GetRoleTextA doesn't return zero on wrong role number, got %d\n", ret);
    ok(buf[0] == '*', "GetRoleTextA modified buffer on wrong role number\n");

    ret = GetRoleTextW(-1, NULL, 0);
    ok(ret == 0, "GetRoleTextW doesn't return zero on wrong role number, got %d\n", ret);
    bufW[0] = '*';
    ret = GetRoleTextW(-1, bufW, 2);
    ok(ret == 0, "GetRoleTextW doesn't return zero on wrong role number, got %d\n", ret);
    ok(bufW[0] == '\0', "GetRoleTextW doesn't return NULL char on wrong role number\n");
    bufW[0] = '*';
    ret = GetRoleTextW(-1, bufW, 0);
    ok(ret == 0, "GetRoleTextW doesn't return zero on wrong role number, got %d\n", ret);

    /* zero role number - not documented */
    ret = GetRoleTextA(0, NULL, 0);
    ok(ret > 0, "GetRoleTextA doesn't return (>0) for zero role number, got %d\n", ret);
    ret = GetRoleTextW(0, NULL, 0);
    ok(ret > 0, "GetRoleTextW doesn't return (>0) for zero role number, got %d\n", ret);

    /* NULL buffer, return length */
    ret = GetRoleTextA(ROLE_SYSTEM_TITLEBAR, NULL, 0);
    ok(ret > 0, "GetRoleTextA doesn't return length on NULL buffer, got %d\n", ret);
    ret = GetRoleTextA(ROLE_SYSTEM_TITLEBAR, NULL, 1);
    ok(ret > 0, "GetRoleTextA doesn't return length on NULL buffer, got %d\n", ret);
    ret = GetRoleTextW(ROLE_SYSTEM_TITLEBAR, NULL, 0);
    ok(ret > 0, "GetRoleTextW doesn't return length on NULL buffer, got %d\n", ret);
    ret = GetRoleTextW(ROLE_SYSTEM_TITLEBAR, NULL, 1);
    ok(ret > 0, "GetRoleTextW doesn't return length on NULL buffer, got %d\n", ret);

    /* use a smaller buffer */
    bufW[0] = '*';
    ret = GetRoleTextA(ROLE_SYSTEM_TITLEBAR, buf, 0);
    ok(!ret, "GetRoleTextA doesn't return 0, got %d\n", ret);
    ok(buf[0] == '*', "GetRoleTextA modified buffer\n");
    buffW = NULL;
    ret = GetRoleTextW(ROLE_SYSTEM_TITLEBAR, (WCHAR*)&buffW, 0);
    ok(ret, "GetRoleTextW doesn't return length\n");
    ok(buffW != NULL, "GetRoleTextW doesn't modify buffer\n");
    buf[0] = '*';
    ret = GetRoleTextA(ROLE_SYSTEM_TITLEBAR, buf, 1);
    ok(ret == 0, "GetRoleTextA returned wrong length\n");
    ok(buf[0] == '\0', "GetRoleTextA returned not zero-length buffer\n");
    buf[0] = '*';
    ret = GetRoleTextA(ROLE_SYSTEM_TITLEBAR, buf, 2);
    ok(!ret, "GetRoleTextA returned wrong length, got %d, expected 0\n", ret);
    ok(!buf[0] || broken(buf[0]!='*') /* WinXP */,
            "GetRoleTextA returned not zero-length buffer : (%c)\n", buf[0]);

    bufW[0] = '*';
    ret = GetRoleTextW(ROLE_SYSTEM_TITLEBAR, bufW, 1);
    ok(ret == 0, "GetRoleTextW returned wrong length, got %d, expected 1\n", ret);
    ok(bufW[0] == '\0', "GetRoleTextW returned not zero-length buffer\n");
    bufW[1] = '*';
    ret = GetRoleTextW(ROLE_SYSTEM_TITLEBAR, bufW, 2);
    ok(ret == 1, "GetRoleTextW returned wrong length, got %d, expected 1\n", ret);
    ok(bufW[1] == '\0', "GetRoleTextW returned not zero-length buffer\n");

    /* use bigger buffer */
    ret = GetRoleTextA(ROLE_SYSTEM_TITLEBAR, NULL, 0);
    buff = HeapAlloc(GetProcessHeap(), 0, 2*ret);
    buff[2*ret-1] = '*';
    ret = GetRoleTextA(ROLE_SYSTEM_TITLEBAR, buff, 2*ret);
    ok(buff[2*ret-1] == '*', "GetRoleTextA shouldn't modify this part of buffer\n");
    HeapFree(GetProcessHeap(), 0, buff);

    ret = GetRoleTextW(ROLE_SYSTEM_TITLEBAR, NULL, 0);
    buffW = HeapAlloc(GetProcessHeap(), 0, 2*ret*sizeof(WCHAR));
    buffW[2*ret-1] = '*';
    ret = GetRoleTextW(ROLE_SYSTEM_TITLEBAR, buffW, 2*ret);
    ok(buffW[2*ret-1] == '*', "GetRoleTextW shouldn't modify this part of buffer\n");
    HeapFree(GetProcessHeap(), 0, buffW);

    /* check returned length for all roles */
    for(role = 0; role <= ROLE_SYSTEM_OUTLINEBUTTON; role++){
        CHAR buff2[100];
        WCHAR buff2W[100];

        /* NT4 and W2K don't clear the buffer on a nonexistent role in the A-call */
        memset(buff2, 0, sizeof(buff2));

        ret = GetRoleTextA(role, NULL, 0);
        ok(ret > 0, "Expected the role to be present\n");

        GetRoleTextA(role, buff2, sizeof(buff2));
        ok(ret == lstrlenA(buff2),
           "GetRoleTextA: returned length doesn't match returned buffer for role %d\n", role);

        /* Win98 and WinMe don't clear the buffer on a nonexistent role in the W-call */
        memset(buff2W, 0, sizeof(buff2W));

        ret = GetRoleTextW(role, NULL, 0);
        GetRoleTextW(role, buff2W, ARRAY_SIZE(buff2W));
        ok(ret == lstrlenW(buff2W),
           "GetRoleTextW: returned length doesn't match returned buffer for role %d\n", role);
    }
}

static void test_GetStateText(void)
{
    WCHAR buf[1024], buf2[1024];
    char bufa[1024];
    void *ptr;
    UINT ret, ret2;
    int i;

    ret2 = GetStateTextW(0, NULL, 1024);
    ok(ret2, "GetStateText failed\n");

    ptr = NULL;
    ret = GetStateTextW(0, (WCHAR*)&ptr, 0);
    ok(ret == ret2, "got %d, expected %d\n", ret, ret2);
    ok(ptr != NULL, "ptr was not changed\n");

    ret = GetStateTextW(0, buf, 1024);
    ok(ret == ret2, "got %d, expected %d\n", ret, ret2);
    ok(!memcmp(buf, ptr, ret*sizeof(WCHAR)), "got %s, expected %s\n",
            wine_dbgstr_wn(buf, ret), wine_dbgstr_wn(ptr, ret));

    ret = GetStateTextW(0, buf, 1);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(!buf[0], "buf[0] = '%c'\n", buf[0]);

    for(i=0; i<31; i++) {
        ret = GetStateTextW(1<<i, buf, 1024);
        ok(ret, "%d) GetStateText failed\n", i);
    }
    ret = GetStateTextW(1u<<31, buf, 1024);
    ok(!ret, "31) GetStateText succeeded: %d\n", ret);

    ret = GetStateTextW(2, buf, 1024);
    ok(ret, "GetStateText failed\n");
    ret2 = GetStateTextW(3, buf2, 1024);
    ok(ret2, "GetStateText failed\n");
    ok(ret == ret2, "got %d, expected %d\n", ret2, ret);
    ok(!memcmp(buf, buf2, ret*sizeof(WCHAR)),
            "GetStateText(2,...) returned different data than GetStateText(3,...)\n");

    ret2 = GetStateTextA(0, NULL, 1024);
    ok(ret2, "GetStateText failed\n");

    ptr = NULL;
    ret = GetStateTextA(0, (CHAR*)&ptr, 0);
    ok(!ret, "got %d\n", ret);
    ok(ptr == NULL, "ptr was changed\n");

    ret = GetStateTextA(0, NULL, 0);
    ok(ret == ret2, "got %d, expected %d\n", ret, ret2);

    ret = GetStateTextA(0, bufa, 1024);
    ok(ret == ret2, "got %d, expected %d\n", ret, ret2);

    ret = GetStateTextA(0, bufa, 1);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(!bufa[0], "bufa[0] = '%c'\n", bufa[0]);

    for(i=0; i<31; i++) {
        ret = GetStateTextA(1<<i, bufa, 1024);
        ok(ret, "%d) GetStateText failed\n", i);
    }
    ret = GetStateTextA(1u<<31, bufa, 1024);
    ok(!ret, "31) GetStateText succeeded: %d\n", ret);
}

static LONG Object_ref = 1;
static HRESULT WINAPI Object_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    if(IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }
    /* on Win7 AccessibleObjectFromEvent doesn't check return value */
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Object_AddRef(IUnknown *iface)
{
    return InterlockedIncrement(&Object_ref);
}

static ULONG WINAPI Object_Release(IUnknown *iface)
{
    return InterlockedDecrement(&Object_ref);
}

static IUnknownVtbl ObjectVtbl = {
    Object_QueryInterface,
    Object_AddRef,
    Object_Release
};

static IUnknown Object = {&ObjectVtbl};

static void test_LresultFromObject(const char *name)
{
    PROCESS_INFORMATION proc;
    STARTUPINFOA startup;
    char cmdline[MAX_PATH];
    IUnknown *unk;
    HRESULT hres;
    LRESULT lres;

    lres = LresultFromObject(NULL, 0, 0);
    ok(lres == E_INVALIDARG, "got %Ix\n", lres);

    hres = ObjectFromLresult(0, &IID_IUnknown, 0, (void**)&unk);
    ok(hres == E_FAIL, "got %lx\n", hres);
    hres = ObjectFromLresult(0x10000, &IID_IUnknown, 0, (void**)&unk);
    ok(hres == E_FAIL, "got %lx\n", hres);

    ok(Object_ref == 1, "Object_ref = %ld\n", Object_ref);
    lres = LresultFromObject(&IID_IUnknown, 0, &Object);
    ok(SUCCEEDED(lres), "got %Ix\n", lres);
    ok(Object_ref > 1, "Object_ref = %ld\n", Object_ref);

    hres = ObjectFromLresult(lres, &IID_IUnknown, 0, (void**)&unk);
    ok(hres == S_OK, "hres = %lx\n", hres);
    ok(unk == &Object, "unk != &Object\n");
    IUnknown_Release(unk);
    ok(Object_ref == 1, "Object_ref = %ld\n", Object_ref);

    lres = LresultFromObject(&IID_IUnknown, 0, &Object);
    ok(SUCCEEDED(lres), "got %Ix\n", lres);
    ok(Object_ref > 1, "Object_ref = %ld\n", Object_ref);

    sprintf(cmdline, "\"%s\" main ObjectFromLresult %s", name, wine_dbgstr_longlong(lres));
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &proc);
    wait_child_process(proc.hProcess);
    ok(Object_ref == 1, "Object_ref = %ld\n", Object_ref);
}

static LRESULT WINAPI test_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch(msg) {
    case WM_GETOBJECT:
        if(lparam == OBJID_QUERYCLASSNAMEIDX) {
            ok(!wparam, "wparam = %Ix\n", wparam);
            return 0;
        }

        ok(wparam==0xffffffff, "wparam = %Ix\n", wparam);
        if(lparam == (DWORD)OBJID_CURSOR)
            return E_UNEXPECTED;
        if(lparam == (DWORD)OBJID_CLIENT)
            return LresultFromObject(&IID_IUnknown, wparam, &Object);
        if(lparam == (DWORD)OBJID_WINDOW)
            return 0;
        if(lparam == 1)
            return LresultFromObject(&IID_IUnknown, wparam, (IUnknown*)&Accessible);

        ok(0, "unexpected (%Id)\n", lparam);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static BOOL register_window_class(void)
{
    WNDCLASSA cls;

    memset(&cls, 0, sizeof(cls));
    cls.lpfnWndProc = test_window_proc;
    cls.lpszClassName = "oleacc_test";
    cls.hInstance = GetModuleHandleA(NULL);

    return RegisterClassA(&cls);
}

static void unregister_window_class(void)
{
    UnregisterClassA("oleacc_test", NULL);
}

static void test_AccessibleObjectFromWindow(void)
{
    IUnknown *unk;
    HRESULT hr;
    HWND hwnd;

    hr = AccessibleObjectFromWindow(NULL, OBJID_CURSOR, &IID_IUnknown, NULL);
    ok(hr == E_INVALIDARG, "got %lx\n", hr);

    hr = AccessibleObjectFromWindow(NULL, OBJID_CURSOR, &IID_IUnknown, (void**)&unk);
    todo_wine ok(hr == S_OK, "got %lx\n", hr);
    if(hr == S_OK) IUnknown_Release(unk);

    hwnd = CreateWindowA("oleacc_test", "test", WS_OVERLAPPEDWINDOW,
            0, 0, 0, 0, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindow failed\n");

    hr = AccessibleObjectFromWindow(hwnd, OBJID_CURSOR, &IID_IUnknown, (void**)&unk);
    ok(hr == E_UNEXPECTED, "got %lx\n", hr);

    ok(Object_ref == 1, "Object_ref = %ld\n", Object_ref);
    hr = AccessibleObjectFromWindow(hwnd, OBJID_CLIENT, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(Object_ref == 2, "Object_ref = %ld\n", Object_ref);
    IUnknown_Release(unk);

    DestroyWindow(hwnd);
}

static void test_AccessibleObjectFromEvent(void)
{
    IAccessible *acc, *acc2;
    IServiceProvider *sp;
    VARIANT cid;
    HRESULT hr;
    HWND hwnd;

    hwnd = CreateWindowA("oleacc_test", "test", WS_OVERLAPPEDWINDOW,
            0, 0, 0, 0, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindow failed\n");

    hr = AccessibleObjectFromEvent(NULL, OBJID_CLIENT, CHILDID_SELF, &acc, &cid);
    ok(hr == E_FAIL, "got %#lx\n", hr);

    hr = AccessibleObjectFromEvent(hwnd, OBJID_CLIENT, CHILDID_SELF, NULL, &cid);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);

    acc = (IAccessible*)0xdeadbeef;
    V_VT(&cid) = VT_UNKNOWN;
    V_UNKNOWN(&cid) = (IUnknown*)0xdeadbeef;
    hr = AccessibleObjectFromEvent(hwnd, OBJID_CLIENT, CHILDID_SELF, &acc, &cid);
    ok(hr == E_NOINTERFACE || broken(hr == S_OK), "got %#lx\n", hr);
    if (hr == S_OK)
        IAccessible_Release(acc);
    else
    {
        ok(acc == NULL, "Unexpected acc %p\n", acc);
        ok(V_VT(&cid) == VT_EMPTY, "got %#x, expected %#x\n", V_VT(&cid), VT_I4);
    }

    hr = AccessibleObjectFromEvent(hwnd, OBJID_CURSOR, CHILDID_SELF, &acc, &cid);
    ok(hr == E_UNEXPECTED, "got %#lx\n", hr);

    SET_EXPECT(Accessible_get_accChild);
    hr = AccessibleObjectFromEvent(hwnd, 1, 1, &acc, &cid);
    CHECK_CALLED(Accessible_get_accChild);
    ok(hr == S_OK, "got %#lx\n", hr);
    todo_wine ok(!iface_cmp((IUnknown*)acc, (IUnknown*)&Accessible), "acc == &Accessible\n");
    /* Get &Accessible out of the Dynamic Annotation IAccessible wrapper. */
    if (!iface_cmp((IUnknown*)acc, (IUnknown*)&Accessible))
    {
        hr = IAccessible_QueryInterface(acc, &IID_IServiceProvider, (void **)&sp);
        ok(hr == S_OK, "got %#lx\n", hr);
        ok(!!sp, "sp == NULL\n");

        hr = IServiceProvider_QueryService(sp, &SID_AccFromDAWrapper, &IID_IAccessible,
                (void**)&acc2);
        ok(hr == S_OK, "got %#lx\n", hr);
        ok(iface_cmp((IUnknown*)acc2, (IUnknown*)&Accessible), "acc != &Accessible\n");
        IAccessible_Release(acc2);
        IServiceProvider_Release(sp);
    }

    ok(V_VT(&cid) == VT_I4, "got %#x, expected %#x\n", V_VT(&cid), VT_I4);
    ok(V_I4(&cid) == 1, "got %#lx, expected 1\n", V_I4(&cid));
    SET_EXPECT(Accessible_get_accParent);
    SET_EXPECT(Accessible_get_accName);
    V_I4(&cid) = 0;
    hr = IAccessible_get_accName(acc, cid, NULL);
    ok(hr == E_INVALIDARG, "get_accName returned %lx\n", hr);
    todo_wine CHECK_CALLED(Accessible_get_accParent);
    CHECK_CALLED(Accessible_get_accName);
    IAccessible_Release(acc);

    SET_EXPECT(Accessible_get_accChild);
    hr = AccessibleObjectFromEvent(hwnd, 1, 2, &acc, &cid);
    CHECK_CALLED(Accessible_get_accChild);
    ok(hr == S_OK, "got %#lx\n", hr);
    todo_wine ok(!iface_cmp((IUnknown*)acc, (IUnknown*)&Accessible), "acc == &Accessible\n");
    ok(V_VT(&cid) == VT_I4, "got %#x, expected %#x\n", V_VT(&cid), VT_I4);
    ok(V_I4(&cid) == 2, "got %#lx, expected 2\n", V_I4(&cid));
    SET_EXPECT(Accessible_get_accParent);
    SET_EXPECT(Accessible_get_accName);
    V_I4(&cid) = 0;
    hr = IAccessible_get_accName(acc, cid, NULL);
    ok(hr == E_INVALIDARG, "get_accName returned %lx\n", hr);
    todo_wine CHECK_CALLED(Accessible_get_accParent);
    CHECK_CALLED(Accessible_get_accName);
    IAccessible_Release(acc);

    SET_EXPECT(Accessible_get_accChild);
    hr = AccessibleObjectFromEvent(hwnd, 1, 3, &acc, &cid);
    CHECK_CALLED(Accessible_get_accChild);
    ok(hr == S_OK, "got %#lx\n", hr);
    todo_wine ok(!iface_cmp((IUnknown*)acc, (IUnknown*)&Accessible_child), "acc == &Accessible_child\n");
    ok(V_VT(&cid) == VT_I4, "got %#x, expected %#x\n", V_VT(&cid), VT_I4);
    ok(V_I4(&cid) == CHILDID_SELF, "got %#lx, expected %#x\n", V_I4(&cid), CHILDID_SELF);
    SET_EXPECT(Accessible_child_get_accParent);
    SET_EXPECT(Accessible_child_get_accName);
    hr = IAccessible_get_accName(acc, cid, NULL);
    ok(hr == E_INVALIDARG, "get_accName returned %lx\n", hr);
    todo_wine CHECK_CALLED(Accessible_child_get_accParent);
    CHECK_CALLED(Accessible_child_get_accName);
    IAccessible_Release(acc);

    SET_EXPECT(Accessible_get_accChild);
    hr = AccessibleObjectFromEvent(hwnd, 1, 4, &acc, &cid);
    CHECK_CALLED(Accessible_get_accChild);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(acc == &Accessible_child.IAccessible_iface, "acc != &Accessible_child\n");
    ok(V_VT(&cid) == VT_I4, "got %#x, expected %#x\n", V_VT(&cid), VT_I4);
    ok(V_I4(&cid) == CHILDID_SELF, "got %#lx, expected %#x\n", V_I4(&cid), CHILDID_SELF);
    SET_EXPECT(Accessible_child_get_accName);
    hr = IAccessible_get_accName(acc, cid, NULL);
    ok(hr == E_INVALIDARG, "get_accName returned %lx\n", hr);
    CHECK_CALLED(Accessible_child_get_accName);
    IAccessible_Release(acc);

    DestroyWindow(hwnd);
    ok(Accessible.ref == 1, "Accessible.ref = %ld\n", Accessible.ref);
    ok(Accessible_child.ref == 1, "Accessible.ref = %ld\n", Accessible_child.ref);
}

static void test_GetProcessHandleFromHwnd(void)
{
    HANDLE proc;
    HWND hwnd;

    proc = pGetProcessHandleFromHwnd(NULL);
    ok(!proc, "proc = %p\n", proc);

    hwnd = CreateWindowA("static", "", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindow failed\n");

    proc = pGetProcessHandleFromHwnd(hwnd);
    ok(proc != NULL, "proc == NULL\n");
    CloseHandle(proc);

    DestroyWindow(hwnd);
}

static void test_AccessibleChildren(IAccessible *acc)
{
    VARIANT children[3];
    LONG count;
    HRESULT hr;

    count = -1;
    hr = AccessibleChildren(NULL, 0, 0, children, &count);
    ok(hr == E_INVALIDARG, "AccessibleChildren returned %lx\n", hr);
    ok(count == -1, "count = %ld\n", count);
    hr = AccessibleChildren(acc, 0, 0, NULL, &count);
    ok(hr == E_INVALIDARG, "AccessibleChildren returned %lx\n", hr);
    ok(count == -1, "count = %ld\n", count);
    hr = AccessibleChildren(acc, 0, 0, children, NULL);
    ok(hr == E_INVALIDARG, "AccessibleChildren returned %lx\n", hr);

    if(acc == &Accessible.IAccessible_iface) {
        SET_EXPECT(Accessible_QI_IEnumVARIANT);
        SET_EXPECT(Accessible_get_accChildCount);
    }
    hr = AccessibleChildren(acc, 0, 0, children, &count);
    ok(hr == S_OK, "AccessibleChildren returned %lx\n", hr);
    if(acc == &Accessible.IAccessible_iface) {
        CHECK_CALLED(Accessible_QI_IEnumVARIANT);
        CHECK_CALLED(Accessible_get_accChildCount);
    }
    ok(!count, "count = %ld\n", count);
    count = -1;
    if(acc == &Accessible.IAccessible_iface) {
        SET_EXPECT(Accessible_QI_IEnumVARIANT);
        SET_EXPECT(Accessible_get_accChildCount);
    }
    hr = AccessibleChildren(acc, 5, 0, children, &count);
    ok(hr == S_OK, "AccessibleChildren returned %lx\n", hr);
    if(acc == &Accessible.IAccessible_iface) {
        CHECK_CALLED(Accessible_QI_IEnumVARIANT);
        CHECK_CALLED(Accessible_get_accChildCount);
    }
    ok(!count, "count = %ld\n", count);

    memset(children, 0xfe, sizeof(children));
    V_VT(children) = VT_DISPATCH;
    if(acc == &Accessible.IAccessible_iface) {
        SET_EXPECT(Accessible_QI_IEnumVARIANT);
        SET_EXPECT(Accessible_get_accChildCount);
        SET_EXPECT(Accessible_get_accChild);
    }
    hr = AccessibleChildren(acc, 0, 1, children, &count);
    ok(hr == S_OK, "AccessibleChildren returned %lx\n", hr);
    if(acc == &Accessible.IAccessible_iface) {
        CHECK_CALLED(Accessible_QI_IEnumVARIANT);
        CHECK_CALLED(Accessible_get_accChildCount);
        CHECK_CALLED(Accessible_get_accChild);

        ok(V_VT(children) == VT_I4, "V_VT(children) = %d\n", V_VT(children));
        ok(V_I4(children) == 1, "V_I4(children) = %ld\n", V_I4(children));
    }else {
        ok(V_VT(children) == VT_DISPATCH, "V_VT(children) = %d\n", V_VT(children));
        IDispatch_Release(V_DISPATCH(children));
    }
    ok(count == 1, "count = %ld\n", count);

    if(acc == &Accessible.IAccessible_iface) {
        SET_EXPECT(Accessible_QI_IEnumVARIANT);
        SET_EXPECT(Accessible_get_accChildCount);
        SET_EXPECT(Accessible_get_accChild);
    }
    hr = AccessibleChildren(acc, 0, 3, children, &count);
    ok(hr == S_FALSE, "AccessibleChildren returned %lx\n", hr);
    if(acc == &Accessible.IAccessible_iface) {
        CHECK_CALLED(Accessible_QI_IEnumVARIANT);
        CHECK_CALLED(Accessible_get_accChildCount);
        CHECK_CALLED(Accessible_get_accChild);

        ok(V_VT(children) == VT_I4, "V_VT(children) = %d\n", V_VT(children));
        ok(V_I4(children) == 1, "V_I4(children) = %ld\n", V_I4(children));

        ok(count == 1, "count = %ld\n", count);
        ok(V_VT(children+1) == VT_EMPTY, "V_VT(children+1) = %d\n", V_VT(children+1));
    }else {
        ok(V_VT(children) == VT_DISPATCH, "V_VT(children) = %d\n", V_VT(children));
        IDispatch_Release(V_DISPATCH(children));

        ok(count == 2, "count = %ld\n", count);
        ok(V_VT(children+1) == VT_DISPATCH, "V_VT(children+1) = %d\n", V_VT(children+1));
        IDispatch_Release(V_DISPATCH(children+1));
    }
    ok(V_VT(children+2) == VT_EMPTY, "V_VT(children+2) = %d\n", V_VT(children+2));

    ok(Accessible.ref == 1, "Accessible.ref = %ld\n", Accessible.ref);
    ok(Accessible_child.ref == 1, "Accessible.ref = %ld\n", Accessible_child.ref);
}

#define check_acc_state(acc, state) _check_acc_state(__LINE__, acc, state)
static void _check_acc_state(unsigned line, IAccessible *acc, INT state)
{
    VARIANT vid, v;
    HRESULT hr;

    V_VT(&vid) = VT_I4;
    V_I4(&vid) = CHILDID_SELF;
    hr = IAccessible_get_accState(acc, vid, &v);
    ok_(__FILE__, line)(hr == S_OK, "got %lx\n", hr);
    ok_(__FILE__, line)(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok_(__FILE__, line)(V_I4(&v) == state, "V_I4(&v) = %lx\n", V_I4(&v));
}

#define check_acc_hwnd(unk, hwnd) _check_acc_hwnd(__LINE__, unk, hwnd)
static void _check_acc_hwnd(unsigned line, IUnknown *unk, HWND exp)
{
    IOleWindow *ow;
    HRESULT hr;
    HWND hwnd;

    hr = IUnknown_QueryInterface(unk, &IID_IOleWindow, (void**)&ow);
    ok_(__FILE__, line)(hr == S_OK, "got %lx\n", hr);
    hr = IOleWindow_GetWindow(ow, &hwnd);
    ok_(__FILE__, line)(hr == S_OK, "got %lx\n", hr);
    ok_(__FILE__, line)(hwnd == exp, "hwnd = %p, expected %p\n", hwnd, exp);
    IOleWindow_Release(ow);
}

static DWORD WINAPI default_client_thread(LPVOID param)
{
    IAccessible *acc = param;
    IOleWindow *ow;
    HRESULT hr;
    HWND hwnd;

    hr = IAccessible_QueryInterface(acc, &IID_IOleWindow, (void**)&ow);
    ok(hr == S_OK, "got %lx\n", hr);
    hr = IOleWindow_GetWindow(ow, &hwnd);
    ok(hr == S_OK, "got %lx\n", hr);
    IOleWindow_Release(ow);

    ShowWindow(hwnd, SW_SHOW);
    check_acc_state(acc, STATE_SYSTEM_FOCUSABLE | STATE_SYSTEM_FOCUSED);
    return 0;
}

static void test_default_client_accessible_object(void)
{
    IAccessible *acc, *win;
    IDispatch *disp;
    IEnumVARIANT *ev;
    HWND chld, chld2, btn, hwnd, hwnd2;
    HRESULT hr;
    VARIANT vid, v;
    BSTR str;
    POINT pt;
    RECT rect;
    LONG l, left, top, width, height;
    ULONG fetched;
    HANDLE thread;

    hwnd = CreateWindowA("oleacc_test", "wnd &t &junk", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindow failed\n");
    chld = CreateWindowA("static", "static &t &junk", WS_CHILD | WS_VISIBLE,
            0, 0, 50, 50, hwnd, NULL, NULL, NULL);
    ok(chld != NULL, "CreateWindow failed\n");
    btn = CreateWindowA("BUTTON", "btn &t &junk", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            50, 0, 50, 50, hwnd, NULL, NULL, NULL);
    ok(btn != NULL, "CreateWindow failed\n");
    chld2 = CreateWindowA("static", "static &t &junk", WS_CHILD | WS_VISIBLE,
            0, 0, 50, 50, chld, NULL, NULL, NULL);
    ok(chld2 != NULL, "CreateWindow failed\n");

    hr = CreateStdAccessibleObject(NULL, OBJID_CLIENT, &IID_IAccessible, (void**)&acc);
    ok(hr == E_FAIL, "got %lx\n", hr);


    /* Test the static message */
    hr = CreateStdAccessibleObject(chld, OBJID_CLIENT, &IID_IAccessible, (void**)&acc);
    ok(hr == S_OK, "got %lx\n", hr);

    V_VT(&vid) = VT_I4;
    V_I4(&vid) = CHILDID_SELF;
    hr = IAccessible_get_accName(acc, vid, &str);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(!lstrcmpW(str, L"static t &junk"), "name = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hr = IAccessible_get_accKeyboardShortcut(acc, vid, &str);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(!lstrcmpW(str, L"Alt+t"), "str = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IAccessible_Release(acc);


    /* Test the button */
    hr = CreateStdAccessibleObject(btn, OBJID_CLIENT, &IID_IAccessible, (void**)&acc);
    ok(hr == S_OK, "got %lx\n", hr);

    V_VT(&vid) = VT_I4;
    V_I4(&vid) = CHILDID_SELF;
    hr = IAccessible_get_accName(acc, vid, &str);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(!lstrcmpW(str, L"btn t &junk"), "name = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hr = IAccessible_get_accKeyboardShortcut(acc, vid, &str);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(!lstrcmpW(str, L"Alt+t"), "str = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IAccessible_Release(acc);


    /* Now we can test and destroy the top-level window */
    hr = CreateStdAccessibleObject(hwnd, OBJID_CLIENT, &IID_IAccessible, (void**)&acc);
    ok(hr == S_OK, "got %lx\n", hr);

    check_acc_hwnd((IUnknown*)acc, hwnd);
    hr = WindowFromAccessibleObject(acc, &hwnd2);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(hwnd == hwnd2, "hwnd2 = %p, expected %p\n", hwnd2, hwnd);

    hr = IAccessible_get_accChildCount(acc, &l);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(l == 2, "l = %ld\n", l);

    V_VT(&vid) = VT_I4;
    V_I4(&vid) = CHILDID_SELF;
    disp = (void*)0xdeadbeef;
    hr = IAccessible_get_accChild(acc, vid, &disp);
    ok(hr == E_INVALIDARG, "get_accChild returned %lx\n", hr);
    ok(disp == NULL, "disp = %p\n", disp);

    V_I4(&vid) = 1;
    disp = (void*)0xdeadbeef;
    hr = IAccessible_get_accChild(acc, vid, &disp);
    ok(hr == E_INVALIDARG, "get_accChild returned %lx\n", hr);
    ok(disp == NULL, "disp = %p\n", disp);

    /* Neither the parent nor any child windows have focus, VT_EMPTY. */
    hr = IAccessible_get_accFocus(acc, &v);
    ok(hr == S_OK, "hr %#lx\n", hr);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(&v) = %d\n", V_VT(&v));

    /* Set the focus to the parent window. */
    ShowWindow(hwnd, SW_SHOW);
    if (!SetForegroundWindow(hwnd))
    {
        skip("SetForegroundWindow failed\n");
        IAccessible_Release(acc);
        DestroyWindow(hwnd);
        return;
    }
    hr = IAccessible_get_accFocus(acc, &v);
    ok(GetFocus() == hwnd, "test window has no focus\n");
    ok(hr == S_OK, "hr %#lx\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == CHILDID_SELF, "V_I4(&v) = %ld\n", V_I4(&v));

    /* Set focus to each child window. */
    SetFocus(btn);
    hr = IAccessible_get_accFocus(acc, &v);
    ok(GetFocus() == btn, "test window has no focus\n");
    ok(hr == S_OK, "hr %#lx\n", hr);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) != NULL, "V_DISPATCH(&v) = %p\n", V_DISPATCH(&v));
    check_acc_hwnd((IUnknown*)V_DISPATCH(&v), btn);

    hr = IDispatch_QueryInterface(V_DISPATCH(&v), &IID_IAccessible, (void**)&win);
    ok(hr == S_OK, "got %lx\n", hr);
    IDispatch_Release(V_DISPATCH(&v));

    V_VT(&vid) = VT_I4;
    V_I4(&vid) = CHILDID_SELF;
    hr = IAccessible_get_accRole(win, vid, &v);
    todo_wine ok(hr == S_OK, "got %lx\n", hr);
    todo_wine ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    todo_wine ok(V_I4(&v) == ROLE_SYSTEM_WINDOW, "V_I4(&v) = %ld\n", V_I4(&v));
    IAccessible_Release(win);

    SetFocus(chld);
    hr = IAccessible_get_accFocus(acc, &v);
    ok(hr == S_OK, "hr %#lx\n", hr);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) != NULL, "V_DISPATCH(&v) = %p\n", V_DISPATCH(&v));
    check_acc_hwnd((IUnknown*)V_DISPATCH(&v), chld);

    hr = IDispatch_QueryInterface(V_DISPATCH(&v), &IID_IAccessible, (void**)&win);
    ok(hr == S_OK, "got %lx\n", hr);
    IDispatch_Release(V_DISPATCH(&v));

    hr = IAccessible_get_accRole(win, vid, &v);
    todo_wine ok(hr == S_OK, "got %lx\n", hr);
    todo_wine ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    todo_wine ok(V_I4(&v) == ROLE_SYSTEM_WINDOW, "V_I4(&v) = %ld\n", V_I4(&v));
    IAccessible_Release(win);

    /* Child of a child, still works on parent HWND. */
    SetFocus(chld2);
    hr = IAccessible_get_accFocus(acc, &v);
    ok(hr == S_OK, "hr %#lx\n", hr);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) != NULL, "V_DISPATCH(&v) = %p\n", V_DISPATCH(&v));
    check_acc_hwnd((IUnknown*)V_DISPATCH(&v), chld2);

    hr = IDispatch_QueryInterface(V_DISPATCH(&v), &IID_IAccessible, (void**)&win);
    ok(hr == S_OK, "got %lx\n", hr);
    IDispatch_Release(V_DISPATCH(&v));

    hr = IAccessible_get_accRole(win, vid, &v);
    todo_wine ok(hr == S_OK, "got %lx\n", hr);
    todo_wine ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    todo_wine ok(V_I4(&v) == ROLE_SYSTEM_WINDOW, "V_I4(&v) = %ld\n", V_I4(&v));
    IAccessible_Release(win);

    ShowWindow(hwnd, SW_HIDE);

    hr = IAccessible_QueryInterface(acc, &IID_IEnumVARIANT, (void**)&ev);
    ok(hr == S_OK, "got %lx\n", hr);

    hr = IEnumVARIANT_Skip(ev, 100);
    ok(hr == S_FALSE, "Skip returned %lx\n", hr);

    V_VT(&v) = VT_I4;
    fetched = 1;
    hr = IEnumVARIANT_Next(ev, 1, &v, &fetched);
    ok(hr == S_FALSE, "got %lx\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(fetched == 0, "fetched = %ld\n", fetched);

    hr = IEnumVARIANT_Reset(ev);
    ok(hr == S_OK, "got %lx\n", hr);

    V_VT(&v) = VT_I4;
    fetched = 2;
    hr = IEnumVARIANT_Next(ev, 1, &v, &fetched);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(&v) = %d\n", V_VT(&v));
    IDispatch_Release(V_DISPATCH(&v));
    ok(fetched == 1, "fetched = %ld\n", fetched);
    IEnumVARIANT_Release(ev);

    test_AccessibleChildren(acc);

    V_VT(&vid) = VT_I4;
    V_I4(&vid) = CHILDID_SELF;
    hr = IAccessible_get_accName(acc, vid, &str);
    ok(hr == S_OK, "got %lx\n", hr);
    /* Window names don't have keyboard shortcuts */
    todo_wine ok(!lstrcmpW(str, L"wnd &t &junk") ||
       broken(!lstrcmpW(str, L"wnd t &junk")), /* Windows < 10 1607 */
       "name = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hr = IAccessible_get_accKeyboardShortcut(acc, vid, &str);
    todo_wine ok(hr == S_FALSE || broken(hr == S_OK), "got %lx\n", hr);
    todo_wine ok(str == NULL || broken(!lstrcmpW(str, L"Alt+t")), "str = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    V_I4(&vid) = 1;
    str = (void*)0xdeadbeef;
    hr = IAccessible_get_accName(acc, vid, &str);
    ok(hr == E_INVALIDARG, "got %lx\n", hr);
    ok(!str, "str != NULL\n");
    V_I4(&vid) = CHILDID_SELF;

    str = (void*)0xdeadbeef;
    hr = IAccessible_get_accValue(acc, vid, &str);
    ok(hr == S_FALSE, "got %lx\n", hr);
    ok(!str, "str != NULL\n");

    str = (void*)0xdeadbeef;
    hr = IAccessible_get_accDescription(acc, vid, &str);
    ok(hr == S_FALSE, "got %lx\n", hr);
    ok(!str, "str != NULL\n");

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (void*)0xdeadbeef;
    hr = IAccessible_get_accRole(acc, vid, &v);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == ROLE_SYSTEM_CLIENT, "V_I4(&v) = %ld\n", V_I4(&v));

    check_acc_state(acc, STATE_SYSTEM_FOCUSABLE | STATE_SYSTEM_INVISIBLE);
    SetFocus(hwnd);
    if (GetForegroundWindow() != hwnd)
    {
        todo_wine ok(0, "incorrect foreground window\n");
        SetForegroundWindow(hwnd);
    }
    check_acc_state(acc, STATE_SYSTEM_FOCUSABLE | STATE_SYSTEM_INVISIBLE |
            STATE_SYSTEM_FOCUSED);
    ShowWindow(hwnd, SW_SHOW);
    check_acc_state(acc, STATE_SYSTEM_FOCUSABLE | STATE_SYSTEM_FOCUSED);
    ShowWindow(hwnd, SW_HIDE);

    str = (void*)0xdeadbeef;
    hr = IAccessible_get_accHelp(acc, vid, &str);
    ok(hr == S_FALSE, "got %lx\n", hr);
    ok(!str, "str != NULL\n");

    str = (void*)0xdeadbeef;
    hr = IAccessible_get_accDefaultAction(acc, vid, &str);
    ok(hr == S_FALSE, "got %lx\n", hr);
    ok(!str, "str != NULL\n");

    pt.x = pt.y = 60;
    ok(ClientToScreen(hwnd, &pt), "ClientToScreen failed\n");
    hr = IAccessible_accHitTest(acc, pt.x, pt.y, &v);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 0, "V_I4(&v) = %ld\n", V_I4(&v));

    pt.x = pt.y = 25;
    ok(ClientToScreen(hwnd, &pt), "ClientToScreen failed\n");
    hr = IAccessible_accHitTest(acc, pt.x, pt.y, &v);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 0, "V_I4(&v) = %ld\n", V_I4(&v));

    ShowWindow(hwnd, SW_SHOW);
    pt.x = pt.y = 60;
    ok(ClientToScreen(hwnd, &pt), "ClientToScreen failed\n");
    hr = IAccessible_accHitTest(acc, pt.x, pt.y, &v);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 0, "V_I4(&v) = %ld\n", V_I4(&v));

    pt.x = pt.y = 25;
    ok(ClientToScreen(hwnd, &pt), "ClientToScreen failed\n");
    hr = IAccessible_accHitTest(acc, pt.x, pt.y, &v);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) != NULL, "V_DISPATCH(&v) = %p\n", V_DISPATCH(&v));
    VariantClear(&v);

    ShowWindow(chld, SW_HIDE);
    pt.x = pt.y = 25;
    ok(ClientToScreen(hwnd, &pt), "ClientToScreen failed\n");
    hr = IAccessible_accHitTest(acc, pt.x, pt.y, &v);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 0, "V_I4(&v) = %ld\n", V_I4(&v));

    hr = IAccessible_get_accParent(acc, &disp);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(disp != NULL, "disp == NULL\n");
    IDispatch_Release(disp);

    ok(GetClientRect(hwnd, &rect), "GetClientRect failed\n");
    pt.x = rect.left;
    pt.y = rect.top;
    MapWindowPoints(hwnd, NULL, &pt, 1);
    rect.left = pt.x;
    rect.top = pt.y;
    pt.x = rect.right;
    pt.y = rect.bottom;
    MapWindowPoints(hwnd, NULL, &pt, 1);
    hr = IAccessible_accLocation(acc, &left, &top, &width, &height, vid);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(left == rect.left, "left = %ld, expected %ld\n", left, rect.left);
    ok(top == rect.top, "top = %ld, expected %ld\n", top, rect.top);
    ok(width == pt.x-rect.left, "width = %ld, expected %ld\n", width, pt.x-rect.left);
    ok(height == pt.y-rect.top, "height = %ld, expected %ld\n", height, pt.y-rect.top);

    thread = CreateThread(NULL, 0, default_client_thread, (void *)acc, 0, NULL);
    while(MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        MSG msg;
        while(PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    CloseHandle(thread);

    DestroyWindow(hwnd);

    hr = IAccessible_get_accChildCount(acc, &l);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(l == 0, "l = %ld\n", l);

    hr = IAccessible_get_accName(acc, vid, &str);
    ok(hr == E_INVALIDARG, "got %lx\n", hr);

    hr = IAccessible_get_accValue(acc, vid, &str);
    ok(hr == S_FALSE, "got %lx\n", hr);

    hr = IAccessible_get_accRole(acc, vid, &v);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == ROLE_SYSTEM_CLIENT, "V_I4(&v) = %ld\n", V_I4(&v));

    hr = IAccessible_get_accState(acc, vid, &v);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == STATE_SYSTEM_INVISIBLE, "V_I4(&v) = %lx\n", V_I4(&v));

    hr = IAccessible_get_accFocus(acc, &v);
    ok(hr == S_OK, "hr %#lx\n", hr);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(&v) = %d\n", V_VT(&v));

    hr = IAccessible_accHitTest(acc, 200, 200, &v);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 0, "V_I4(&v) = %ld\n", V_I4(&v));

    disp = (void*)0xdeadbeef;
    hr = IAccessible_get_accParent(acc, &disp);
    ok(hr == E_FAIL, "got %lx\n", hr);
    ok(disp == NULL, "disp = %p\n", disp);

    hr = IAccessible_accLocation(acc, &left, &top, &width, &height, vid);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(left == 0, "left =  %ld\n", left);
    ok(top == 0, "top =  %ld\n", top);
    ok(width == 0, "width =  %ld\n", width);
    ok(height == 0, "height =  %ld\n", height);

    IAccessible_Release(acc);
}

static void test_AccessibleObjectFromPoint(void)
{
    HWND hwnd, child;
    IAccessible *acc;
    VARIANT cid, var;
    POINT point;
    HRESULT hr;

    hwnd = CreateWindowA("oleacc_test", "test", WS_POPUP | WS_VISIBLE,
            0, 0, 400, 300, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindow failed\n");
    ok(SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)&DefWindowProcA),
            "SetWindowLongPtr failed\n");

    point.x = point.y = 10;
    ok(ClientToScreen(hwnd, &point), "ClientToScreen failed\n");

    if (WindowFromPoint(point) != hwnd)
    {
        win_skip("test window not returned from WindowFromPoint\n");
        DestroyWindow(hwnd);
        return;
    }

    hr = AccessibleObjectFromPoint(point, NULL, NULL);
    ok(hr == E_INVALIDARG, "got %lx\n", hr);

    hr = AccessibleObjectFromPoint(point, &acc, NULL);
    ok(hr == E_INVALIDARG, "got %lx\n", hr);

    V_VT(&cid) = VT_DISPATCH;
    V_DISPATCH(&cid) = (IDispatch*)0xdeadbeef;
    hr = AccessibleObjectFromPoint(point, NULL, &cid);
    ok(hr == E_INVALIDARG, "got %lx\n", hr);
    ok(V_VT(&cid) == VT_DISPATCH, "got %#x, expected %#x\n", V_VT(&cid), VT_DISPATCH);

    hr = AccessibleObjectFromPoint(point, &acc, &cid);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(V_VT(&cid) == VT_I4, "got %#x, expected %#x\n", V_VT(&cid), VT_I4);
    ok(V_I4(&cid) == CHILDID_SELF, "got %#lx, expected %#x\n", V_I4(&cid), CHILDID_SELF);
    check_acc_hwnd((IUnknown*)acc, hwnd);
    hr = IAccessible_get_accRole(acc, cid, &var);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(V_VT(&var) == VT_I4, "got %#x, expected %#x\n", V_VT(&var), VT_I4);
    ok(V_I4(&var) == ROLE_SYSTEM_CLIENT, "got %#lx, expected %#x\n",
            V_I4(&var), ROLE_SYSTEM_CLIENT);
    IAccessible_Release(acc);

    child = CreateWindowA("edit", "edit", WS_CHILD | WS_VISIBLE,
            0, 0, 100, 100, hwnd, NULL, NULL, NULL);
    ok(child != NULL, "CreateWindow failed\n");

    if (WindowFromPoint(point) != child)
    {
        win_skip("test window not returned from WindowFromPoint\n");
        DestroyWindow(hwnd);
        return;
    }

    hr = AccessibleObjectFromPoint(point, &acc, &cid);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(V_VT(&cid) == VT_I4, "got %#x, expected %#x\n", V_VT(&cid), VT_I4);
    ok(V_I4(&cid) == CHILDID_SELF, "got %#lx, expected %#x\n", V_I4(&cid), CHILDID_SELF);
    check_acc_hwnd((IUnknown*)acc, child);
    hr = IAccessible_get_accRole(acc, cid, &var);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(V_VT(&var) == VT_I4, "got %#x, expected %#x\n", V_VT(&var), VT_I4);
    ok(V_I4(&var) == ROLE_SYSTEM_TEXT, "got %#lx, expected %#x\n",
            V_I4(&var), ROLE_SYSTEM_TEXT);
    IAccessible_Release(acc);

    DestroyWindow(hwnd);
}

static void test_CAccPropServices(void)
{
    IAccPropServices *acc_prop_services;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_CAccPropServices, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IAccPropServices, (void**)&acc_prop_services);
    ok(hres == S_OK, "Could not create CAccPropServices instance: %08lx\n", hres);

    IAccPropServices_Release(acc_prop_services);
}

static LRESULT WINAPI test_query_class(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg != WM_GETOBJECT)
        return 0;

    CHECK_EXPECT(winproc_GETOBJECT);
    ok(!wparam, "wparam = %Ix\n", wparam);
    ok(lparam == OBJID_QUERYCLASSNAMEIDX, "lparam = %Ix\n", lparam);
    return 0;
}

#define check_acc_proxy_service( acc ) \
        check_acc_proxy_service_( (acc), __LINE__)
static void check_acc_proxy_service_(IAccessible *acc, int line)
{
    IServiceProvider *service = NULL;
    IUnknown *unk = NULL;
    HRESULT hr;

    hr = IAccessible_QueryInterface(acc, &IID_IServiceProvider, (void **)&service);
    ok(hr == S_OK, "got %#lx\n", hr);

    hr = IServiceProvider_QueryService(service, &IIS_IsOleaccProxy, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "got %#lx\n", hr);
    ok(!!unk, "unk == NULL\n");
    ok(iface_cmp(unk, (IUnknown*)acc), "unk != acc\n");
    IUnknown_Release(unk);

    unk = (IUnknown*)0xdeadbeef;
    hr = IServiceProvider_QueryService(service, &IID_IUnknown, &IID_IUnknown, (void **)&unk);
    ok(hr == E_INVALIDARG, "got %#lx\n", hr);
    ok(!unk, "unk != NULL\n");
    IServiceProvider_Release(service);
}

static void test_CreateStdAccessibleObject_classes(void)
{
    static const struct {
        const WCHAR *class;
        BOOL window; /* uses default window accessibility object */
        BOOL client; /* uses default client accessibility object */
    } tests[] =
    {
        { WC_LISTBOXW },
        { L"#32768" },
        { WC_BUTTONW, TRUE },
        { WC_STATICW, TRUE },
        { WC_EDITW, TRUE },
        { WC_COMBOBOXW, TRUE },
        { L"#32770", TRUE },
        { L"#32769", TRUE },
        { WC_SCROLLBARW, TRUE },
        { STATUSCLASSNAMEW, TRUE },
        { TOOLBARCLASSNAMEW, TRUE },
        { PROGRESS_CLASSW, TRUE },
        { ANIMATE_CLASSW, TRUE },
        { WC_TABCONTROLW, TRUE },
        { HOTKEY_CLASSW, TRUE },
        { WC_HEADERW, TRUE },
        { TRACKBAR_CLASSW, TRUE },
        { WC_LISTVIEWW, TRUE },
        { UPDOWN_CLASSW, TRUE },
        { TOOLTIPS_CLASSW, TRUE },
        { WC_TREEVIEWW, TRUE },
        { MONTHCAL_CLASSW, TRUE, TRUE },
        { DATETIMEPICK_CLASSW, TRUE },
        { WC_IPADDRESSW, TRUE }
    };

    LRESULT (WINAPI *win_proc)(HWND, UINT, WPARAM, LPARAM);
    IAccessible *acc;
    HRESULT hr;
    HWND hwnd;
    int i;

    for(i=0; i<ARRAY_SIZE(tests); i++)
    {
        winetest_push_context("class = %s", wine_dbgstr_w(tests[i].class));
        hwnd = CreateWindowW(tests[i].class, L"name", WS_OVERLAPPEDWINDOW,
                0, 0, 0, 0, NULL, NULL, NULL, NULL);
        ok(hwnd != NULL, "CreateWindow failed\n");
        win_proc = (void*)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)test_query_class);

        if (tests[i].client)
            SET_EXPECT(winproc_GETOBJECT);
        hr = CreateStdAccessibleObject(hwnd, OBJID_CLIENT, &IID_IAccessible, (void**)&acc);
        ok(hr == S_OK, "CreateStdAccessibleObject failed %lx\n", hr);
        if (tests[i].client)
            CHECK_CALLED(winproc_GETOBJECT);
        check_acc_proxy_service(acc);
        IAccessible_Release(acc);

        if (tests[i].window)
            SET_EXPECT(winproc_GETOBJECT);
        hr = CreateStdAccessibleObject(hwnd, OBJID_WINDOW, &IID_IAccessible, (void**)&acc);
        ok(hr == S_OK, "CreateStdAccessibleObject failed %lx\n", hr);
        if (tests[i].window)
            CHECK_CALLED(winproc_GETOBJECT);
        check_acc_proxy_service(acc);
        IAccessible_Release(acc);

        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)win_proc);
        DestroyWindow(hwnd);
        winetest_pop_context();
    }
}

typedef struct
{
    const WCHAR *name;
    const WCHAR *value;
    HRESULT value_hr;

    const WCHAR *description;
    const WCHAR *help;
    const WCHAR *kbd_shortcut;
    const WCHAR *default_action;

    INT  role;
    INT  state;
    LONG child_count;
    BOOL valid_child;
    BOOL valid_parent;
    INT  focus_vt;
    INT  focus_cid;
} acc_expected_vals;

#define check_acc_vals( acc, vals) \
        check_acc_vals_( (acc), (vals), __LINE__)
static void check_acc_vals_(IAccessible *acc, const acc_expected_vals *vals,
        int line)
{
    LONG left, top, width, height;
    LONG child_count;
    VARIANT vid, var;
    IDispatch *disp;
    HRESULT hr;
    RECT rect;
    HWND hwnd;
    POINT pt;
    BSTR str;

    V_VT(&vid) = VT_I4;
    V_I4(&vid) = CHILDID_SELF;
    hr = IAccessible_get_accName(acc, vid, &str);
    ok_(__FILE__, line) (hr == (vals->name ? S_OK : S_FALSE), "get_accName returned %#lx, expected %#lx\n",
            hr, (vals->name ? S_OK : S_FALSE));
    ok_(__FILE__, line) (!lstrcmpW(str, vals->name), "get_accName returned %s, expected %s\n",
            wine_dbgstr_w(str), wine_dbgstr_w(vals->name));
    SysFreeString(str);

    hr = IAccessible_get_accValue(acc, vid, &str);
    ok_(__FILE__, line) (hr == vals->value_hr, "get_accValue returned %#lx, expected %#lx\n",
            hr, vals->value_hr);
    ok_(__FILE__, line) (!lstrcmpW(str, vals->value), "get_accValue returned %s, expected %s\n",
            wine_dbgstr_w(str), wine_dbgstr_w(vals->value));
    SysFreeString(str);

    hr = IAccessible_get_accDescription(acc, vid, &str);
    ok_(__FILE__, line) (hr == (vals->description ? S_OK : S_FALSE), "get_accDescription returned %#lx, expected %#lx\n",
            hr, vals->help ? S_OK : S_FALSE);
    ok_(__FILE__, line) (!lstrcmpW(str, vals->description), "get_accDescription returned %s, expected %s\n",
            wine_dbgstr_w(str), wine_dbgstr_w(vals->description));
    SysFreeString(str);

    hr = IAccessible_get_accHelp(acc, vid, &str);
    ok_(__FILE__, line) (hr == (vals->help ? S_OK : S_FALSE), "get_accHelp returned %#lx, expected %#lx\n",
            hr, vals->help ? S_OK : S_FALSE);
    ok_(__FILE__, line) (!lstrcmpW(str, vals->help), "get_accHelp returned %s, expected %s\n",
            wine_dbgstr_w(str), wine_dbgstr_w(vals->help));
    SysFreeString(str);

    hr = IAccessible_get_accKeyboardShortcut(acc, vid, &str);
    ok_(__FILE__, line) (hr == (vals->kbd_shortcut ? S_OK : S_FALSE), "get_accKeyboardShortcut returned %#lx, expected %#lx\n",
            hr, vals->help ? S_OK : S_FALSE);
    ok_(__FILE__, line) (!lstrcmpW(str, vals->kbd_shortcut), "get_accKeyboardShortcut returned %s, expected %s\n",
            wine_dbgstr_w(str), wine_dbgstr_w(vals->kbd_shortcut));
    SysFreeString(str);

    hr = IAccessible_get_accDefaultAction(acc, vid, &str);
    ok_(__FILE__, line) (hr == (vals->default_action ? S_OK : S_FALSE), "get_accDefaultAction returned %#lx, expected %#lx\n",
            hr, vals->help ? S_OK : S_FALSE);
    ok_(__FILE__, line) (!lstrcmpW(str, vals->default_action), "get_accDefaultAction returned %s, expected %s\n",
            wine_dbgstr_w(str), wine_dbgstr_w(vals->default_action));
    SysFreeString(str);

    V_VT(&var) = VT_DISPATCH;
    V_DISPATCH(&var) = (void*)0xdeadbeef;
    hr = IAccessible_get_accRole(acc, vid, &var);
    ok_(__FILE__, line) (hr == S_OK, "get_accRole returned %#lx\n", hr);
    ok_(__FILE__, line) (V_VT(&var) == VT_I4, "V_VT(&var) returned %d, expected %d\n", V_VT(&var), VT_I4);
    ok_(__FILE__, line) (V_I4(&var) == vals->role, "get_accRole returned %ld, expected %d\n",
            V_I4(&var), vals->role);

    V_VT(&var) = VT_DISPATCH;
    V_DISPATCH(&var) = (void*)0xdeadbeef;
    hr = IAccessible_get_accState(acc, vid, &var);
    ok_(__FILE__, line) (hr == S_OK, "get_accState returned %#lx\n", hr);
    ok_(__FILE__, line) (V_VT(&var) == VT_I4, "V_VT(&var) returned %d, expected %d\n", V_VT(&var), VT_I4);
    ok_(__FILE__, line) (V_I4(&var) == vals->state, "get_accState returned %#lx, expected %#x\n",
            V_I4(&var), vals->state);

    hr = WindowFromAccessibleObject(acc, &hwnd);
    ok_(__FILE__, line) (hr == S_OK, "got %lx\n", hr);
    ok_(__FILE__, line) (GetClientRect(hwnd, &rect), "GetClientRect failed\n");
    pt.x = rect.left;
    pt.y = rect.top;
    MapWindowPoints(hwnd, NULL, &pt, 1);
    rect.left = pt.x;
    rect.top = pt.y;
    pt.x = rect.right;
    pt.y = rect.bottom;
    MapWindowPoints(hwnd, NULL, &pt, 1);
    hr = IAccessible_accLocation(acc, &left, &top, &width, &height, vid);
    ok_(__FILE__, line) (hr == S_OK, "got %lx\n", hr);
    ok_(__FILE__, line) (left == rect.left, "left = %ld, expected %ld\n", left, rect.left);
    ok_(__FILE__, line) (top == rect.top, "top = %ld, expected %ld\n", top, rect.top);
    ok_(__FILE__, line) (width == pt.x-rect.left, "width = %ld, expected %ld\n", width, pt.x-rect.left);
    ok_(__FILE__, line) (height == pt.y-rect.top, "height = %ld, expected %ld\n", height, pt.y-rect.top);

    child_count = -1;
    hr = IAccessible_get_accChildCount(acc, &child_count);
    ok_(__FILE__, line) (hr == S_OK, "get_accChildCount returned %#lx\n", hr);
    ok_(__FILE__, line) (child_count == vals->child_count, "get_accChildCount returned %ld, expected %#lx\n",
            child_count, vals->child_count);

    disp = (void *)0xdeadbeef;
    V_VT(&var) = VT_I4;
    V_I4(&var) = 1;
    hr = IAccessible_get_accChild(acc, var, &disp);
    ok_(__FILE__, line) (hr == (vals->valid_child ? S_OK : E_INVALIDARG), "get_accChild returned %#lx, expected %#lx\n",
            hr, (vals->valid_child ? S_OK : E_INVALIDARG));
    if (disp)
    {
        ok_(__FILE__, line) (vals->valid_child, "get_accChild unexpectedly returned a child\n");
        IDispatch_Release(disp);
    }
    else
        ok_(__FILE__, line) (!vals->valid_child, "get_accChild expected a valid child, none was returned\n");

    disp = (void *)0xdeadbeef;
    hr = IAccessible_get_accParent(acc, &disp);
    ok_(__FILE__, line) (hr == S_OK, "get_accParent returned %#lx\n", hr);
    ok_(__FILE__, line) (disp != NULL, "get_accParent returned a NULL pareent\n");
    IDispatch_Release(disp);

    V_VT(&var) = VT_EMPTY;
    hr = IAccessible_get_accFocus(acc, &var);
    ok_(__FILE__, line) (hr == S_OK, "get_accFocus returned %#lx\n", hr);
    ok_(__FILE__, line) (V_VT(&var) == vals->focus_vt, "get_accFocus returned V_VT(&var) %d, expected %d\n",
            V_VT(&var), vals->focus_vt);
    switch(vals->focus_vt)
    {
    case VT_I4:
        ok_(__FILE__, line) (V_I4(&var) == vals->focus_cid, "get_accFocus returned childID %ld, expected %d\n",
                V_I4(&var), vals->focus_cid);
        break;

    case VT_DISPATCH:
        ok_(__FILE__, line) (V_DISPATCH(&var) != NULL, "get_accFocus returned NULL IDispatch\n");
        IDispatch_Release(V_DISPATCH(&var));
        break;

    default:
        break;
    }
}

static const acc_expected_vals edit_acc_vals[] = {
    /* edit0, readonly edit, no label. */
    { .name           = NULL,
      .value          = L"edit0-test",
      .value_hr       = S_OK,
      .description    = NULL,
      .help           = NULL,
      .kbd_shortcut   = NULL,
      .default_action = NULL,
      .role           = ROLE_SYSTEM_TEXT,
      .state          = STATE_SYSTEM_FOCUSABLE | STATE_SYSTEM_READONLY,
      .child_count    = 0,
      .valid_child    = FALSE,
      .valid_parent   = TRUE,
      .focus_vt       = VT_EMPTY,
      .focus_cid      = 0, },
    /* edit1, ES_PASSWORD edit style. */
    { .name           = L"label0:",
      .value          = NULL,
      .value_hr       = E_ACCESSDENIED,
      .description    = NULL,
      .help           = NULL,
      .kbd_shortcut   = L"Alt+l",
      .default_action = NULL,
      .role           = ROLE_SYSTEM_TEXT,
      .state          = STATE_SYSTEM_PROTECTED | STATE_SYSTEM_FOCUSABLE,
      .child_count    = 0,
      .valid_child    = FALSE,
      .valid_parent   = TRUE,
      .focus_vt       = VT_EMPTY,
      .focus_cid      = 0, },
    /* edit2, multi-line edit. */
    { .name           = L"label1:",
      .value          = L"edit2-test\r\ntest-edit2\n",
      .value_hr       = S_OK,
      .description    = NULL,
      .help           = NULL,
      .kbd_shortcut   = L"Alt+e",
      .default_action = NULL,
      .role           = ROLE_SYSTEM_TEXT,
      .state          = STATE_SYSTEM_FOCUSABLE | STATE_SYSTEM_FOCUSED,
      .child_count    = 0,
      .valid_child    = FALSE,
      .valid_parent   = TRUE,
      .focus_vt       = VT_I4,
      .focus_cid      = CHILDID_SELF, },
    /* edit3, edit with child HWND. */
    { .name           = L"label1:",
      .value          = L"",
      .value_hr       = S_OK,
      .description    = NULL,
      .help           = NULL,
      .kbd_shortcut   = L"Alt+l",
      .default_action = NULL,
      .role           = ROLE_SYSTEM_TEXT,
      .state          = STATE_SYSTEM_FOCUSABLE,
      .child_count    = 1,
      .valid_child    = FALSE,
      .valid_parent   = TRUE,
      .focus_vt       = VT_DISPATCH,
      .focus_cid      = 0, },
};

static void test_default_edit_accessible_object(void)
{
    HWND hwnd, label0, label1, btn0, btn1;
    IAccessible *acc;
    HWND edit[4];
    HRESULT hr;
    VARIANT v;
    BSTR str;

    hwnd = CreateWindowW(L"oleacc_test", L"edit_acc_test_win", WS_OVERLAPPEDWINDOW,
            0, 0, 220, 160, NULL, NULL, NULL, NULL);
    ok(!!hwnd, "CreateWindow failed\n");

    edit[0] = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | ES_READONLY, 5, 5, 190, 20,
            hwnd, NULL, NULL, NULL);
    ok(!!edit[0], "Failed to create edit[0] hwnd\n");

    label0 = CreateWindowW(L"STATIC", L"&label0:", WS_VISIBLE | WS_CHILD, 5, 30, 55, 20,
            hwnd, NULL, NULL, NULL);
    ok(!!label0, "Failed to create label0 hwnd\n");

    edit[1] = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | ES_PASSWORD,
            65, 30, 130, 20, hwnd, NULL, NULL, NULL);
    ok(!!edit[1], "Failed to create edit[1] hwnd\n");

    label1 = CreateWindowW(L"STATIC", L"lab&el1:", WS_VISIBLE | WS_CHILD, 5, 55, 45, 20,
            hwnd, NULL, NULL, NULL);
    ok(!!label1, "Failed to create label1 hwnd\n");

    btn0 = CreateWindowW(L"BUTTON", L"but&ton0", WS_VISIBLE | WS_CHILD, 55, 55, 45, 20,
            hwnd, NULL, NULL, NULL);
    ok(!!btn0, "Failed to create btn0 hwnd\n");

    edit[2] = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | ES_MULTILINE,
            105, 55, 90, 40, hwnd, NULL, NULL, NULL);
    ok(!!edit[2], "Failed to create edit[2] hwnd\n");

    edit[3] = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD, 5, 100, 190, 20,
            hwnd, NULL, NULL, NULL);
    ok(!!edit[3], "Failed to create edit[3] hwnd\n");

    /* Button embedded within an edit control window. */
    btn1 = CreateWindowW(L"BUTTON", L"button1", WS_VISIBLE | WS_CHILD, 100, 5, 85, 10,
            edit[3], NULL, NULL, NULL);
    ok(!!btn1, "Failed to create btn1 hwnd\n");

    hr = CreateStdAccessibleObject(edit[0], OBJID_CLIENT, &IID_IAccessible, (void**)&acc);
    ok(hr == S_OK, "got %lx\n", hr);
    V_VT(&v) = VT_I4;
    V_I4(&v) = CHILDID_SELF;
    str = SysAllocString(L"edit0-test");
    hr = IAccessible_put_accValue(acc, v, str);
    ok(hr == S_OK, "got %lx\n", hr);
    SysFreeString(str);
    check_acc_vals(acc, &edit_acc_vals[0]);
    IAccessible_Release(acc);

    hr = CreateStdAccessibleObject(edit[1], OBJID_CLIENT, &IID_IAccessible, (void**)&acc);
    ok(hr == S_OK, "got %lx\n", hr);
    str = SysAllocString(L"password");
    hr = IAccessible_put_accValue(acc, v, str);
    ok(hr == S_OK, "got %lx\n", hr);
    SysFreeString(str);
    check_acc_vals(acc, &edit_acc_vals[1]);
    IAccessible_Release(acc);

    if (!SetForegroundWindow(hwnd))
    {
        skip("SetForegroundWindow failed\n");
        DestroyWindow(hwnd);
        return;
    }
    SetFocus(edit[2]);
    hr = CreateStdAccessibleObject(edit[2], OBJID_CLIENT, &IID_IAccessible, (void**)&acc);
    str = SysAllocString(L"edit2-test\r\ntest-edit2\n");
    hr = IAccessible_put_accValue(acc, v, str);
    ok(hr == S_OK, "got %lx\n", hr);
    SysFreeString(str);
    check_acc_vals(acc, &edit_acc_vals[2]);
    IAccessible_Release(acc);

    /*
     * Hiding a label with a keyboard shortcut makes get_accKeyboardShortcut
     * on the edit no longer return the labels shortcut, however get_accName
     * still returns the labels string.
     */
    ShowWindow(label1, SW_HIDE);
    SetFocus(btn1);
    hr = CreateStdAccessibleObject(edit[3], OBJID_CLIENT, &IID_IAccessible, (void**)&acc);
    ok(hr == S_OK, "got %lx\n", hr);
    check_acc_vals(acc, &edit_acc_vals[3]);
    IAccessible_Release(acc);

    DestroyWindow(hwnd);
}

static void test_WindowFromAccessibleObject(void)
{
    HRESULT hr;
    HWND hwnd;

    /* Successfully retrieve an HWND from the IOleWindow interface. */
    Accessible.ow_hwnd = (HWND)0xdeadf00d;
    hwnd = (HWND)0xdeadbeef;
    hr = WindowFromAccessibleObject(&Accessible.IAccessible_iface, &hwnd);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(hwnd == (HWND)0xdeadf00d, "hwnd != 0xdeadf00d!\n");

    /* Successfully retrieve an HWND from IAccessible::accNavigate. */
    Accessible.acc_hwnd = (HWND)0xdeadf00d;
    Accessible.ow_hwnd = NULL;
    hwnd = (HWND)0xdeadbeef;
    SET_EXPECT(Accessible_accNavigate);
    hr = WindowFromAccessibleObject(&Accessible.IAccessible_iface, &hwnd);
    ok(hr == S_OK, "got %lx\n", hr);
    /* This value gets sign-extended on 64-bit. */
    ok(hwnd == IntToPtr(0xdeadf00d), "hwnd != 0xdeadf00d!\n");
    CHECK_CALLED(Accessible_accNavigate);

    /* Don't return an HWND from either method. */
    Accessible.acc_hwnd = NULL;
    hwnd = (HWND)0xdeadbeef;
    SET_EXPECT(Accessible_accNavigate);
    SET_EXPECT(Accessible_get_accParent);
    hr = WindowFromAccessibleObject(&Accessible.IAccessible_iface, &hwnd);
    /* Return value from IAccessible::get_accParent. */
    ok(hr == S_FALSE, "got %lx\n", hr);
    ok(!hwnd, "hwnd %p\n", hwnd);
    CHECK_CALLED(Accessible_accNavigate);
    CHECK_CALLED(Accessible_get_accParent);

    /* Successfully retrieve an HWND from a parent IAccessible's IOleWindow interface. */
    Accessible.ow_hwnd = (HWND)0xdeadf00d;
    hwnd = (HWND)0xdeadbeef;
    SET_EXPECT(Accessible_child_accNavigate);
    SET_EXPECT(Accessible_child_get_accParent);
    hr = WindowFromAccessibleObject(&Accessible_child.IAccessible_iface, &hwnd);
    ok(hr == S_OK, "got %lx\n", hr);
    ok(hwnd == (HWND)0xdeadf00d, "hwnd != 0xdeadf00d!\n");
    CHECK_CALLED(Accessible_child_accNavigate);
    CHECK_CALLED(Accessible_child_get_accParent);

    Accessible.ow_hwnd = NULL;
    ok(Accessible.ref == 1, "Accessible.ref = %ld\n", Accessible.ref);
    ok(Accessible_child.ref == 1, "Accessible.ref = %ld\n", Accessible_child.ref);
}

START_TEST(main)
{
    int argc;
    char **argv;

    if(!init())
        return;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    argc = winetest_get_mainargs(&argv);
    if(argc == 4 && !strcmp(argv[2], "ObjectFromLresult")) {
        IUnknown *unk;
        HRESULT hres;
        LRESULT lres;

        lres = strtoll( argv[3], NULL, 16 );
        hres = ObjectFromLresult(lres, &IID_IUnknown, 0, (void**)&unk);
        ok(hres == S_OK, "hres = %lx\n", hres);
        IUnknown_Release(unk);

        CoUninitialize();
        return;
    }

    if(!register_window_class()) {
        skip("can't register test window class\n");
        return;
    }

    test_getroletext();
    test_GetStateText();
    test_LresultFromObject(argv[0]);
    test_AccessibleObjectFromWindow();
    test_GetProcessHandleFromHwnd();
    test_default_client_accessible_object();
    test_AccessibleChildren(&Accessible.IAccessible_iface);
    test_AccessibleObjectFromEvent();
    test_AccessibleObjectFromPoint();
    test_CreateStdAccessibleObject_classes();
    test_default_edit_accessible_object();
    test_WindowFromAccessibleObject();

    unregister_window_class();
    CoUninitialize();

    CoInitialize(NULL);
    test_CAccPropServices();
    CoUninitialize();
}
