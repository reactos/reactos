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

#include <wine/test.h>
#include <stdio.h>

//#include "initguid.h"
#include <oleacc.h>

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

DEFINE_EXPECT(Accessible_QI_IEnumVARIANT);
DEFINE_EXPECT(Accessible_get_accChildCount);
DEFINE_EXPECT(Accessible_get_accChild);

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

static HRESULT WINAPI Accessible_QueryInterface(
        IAccessible *iface, REFIID riid, void **ppvObject)
{
    if(IsEqualIID(riid, &IID_IEnumVARIANT)) {
        CHECK_EXPECT(Accessible_QI_IEnumVARIANT);
        return E_NOINTERFACE;
    }

    ok(0, "unexpected QI call: %s\n", wine_dbgstr_guid(riid));
    return E_NOTIMPL;
}

static ULONG WINAPI Accessible_AddRef(IAccessible *iface)
{
    return 2;
}

static ULONG WINAPI Accessible_Release(IAccessible *iface)
{
    return 1;
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
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Accessible_get_accChildCount(
        IAccessible *iface, LONG *pcountChildren)
{
    CHECK_EXPECT(Accessible_get_accChildCount);
    *pcountChildren = 1;
    return S_OK;
}

static HRESULT WINAPI Accessible_get_accChild(IAccessible *iface,
        VARIANT varChildID, IDispatch **ppdispChild)
{
    CHECK_EXPECT(Accessible_get_accChild);
    ok(V_VT(&varChildID) == VT_I4, "V_VT(&varChildID) = %d\n", V_VT(&varChildID));
    ok(V_I4(&varChildID) == 1, "V_I4(&varChildID) = %d\n", V_I4(&varChildID));

    *ppdispChild = NULL;
    return S_OK;
}

static HRESULT WINAPI Accessible_get_accName(IAccessible *iface,
        VARIANT varID, BSTR *pszName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
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
    ok(0, "unexpected call\n");
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

static IAccessible Accessible = {&AccessibleVtbl};

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
        GetRoleTextW(role, buff2W, sizeof(buff2W)/sizeof(WCHAR));
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

static int Object_ref = 1;
static HRESULT WINAPI Object_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    if(IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }
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
    ok(lres == E_INVALIDARG, "got %lx\n", lres);

    hres = ObjectFromLresult(0, &IID_IUnknown, 0, (void**)&unk);
    ok(hres == E_FAIL, "got %x\n", hres);
    hres = ObjectFromLresult(0x10000, &IID_IUnknown, 0, (void**)&unk);
    ok(hres == E_FAIL, "got %x\n", hres);

    ok(Object_ref == 1, "Object_ref = %d\n", Object_ref);
    lres = LresultFromObject(&IID_IUnknown, 0, &Object);
    ok(SUCCEEDED(lres), "got %lx\n", lres);
    ok(Object_ref > 1, "Object_ref = %d\n", Object_ref);

    hres = ObjectFromLresult(lres, &IID_IUnknown, 0, (void**)&unk);
    ok(hres == S_OK, "hres = %x\n", hres);
    ok(unk == &Object, "unk != &Object\n");
    IUnknown_Release(unk);
    ok(Object_ref == 1, "Object_ref = %d\n", Object_ref);

    lres = LresultFromObject(&IID_IUnknown, 0, &Object);
    ok(SUCCEEDED(lres), "got %lx\n", lres);
    ok(Object_ref > 1, "Object_ref = %d\n", Object_ref);

    sprintf(cmdline, "\"%s\" main ObjectFromLresult %lx", name, lres);
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &proc);
    winetest_wait_child_process(proc.hProcess);
    ok(Object_ref == 1, "Object_ref = %d\n", Object_ref);
}

static LRESULT WINAPI test_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch(msg) {
    case WM_GETOBJECT:
        if(lparam == OBJID_QUERYCLASSNAMEIDX) {
            ok(!wparam, "wparam = %lx\n", wparam);
            return 0;
        }

        ok(wparam==0xffffffff, "wparam = %lx\n", wparam);
        if(lparam == (DWORD)OBJID_CURSOR)
            return E_UNEXPECTED;
        if(lparam == (DWORD)OBJID_CLIENT)
            return LresultFromObject(&IID_IUnknown, wparam, &Object);
        if(lparam == (DWORD)OBJID_WINDOW)
            return 0;

        ok(0, "unexpected (%ld)\n", lparam);
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
    ok(hr == E_INVALIDARG, "got %x\n", hr);

    hr = AccessibleObjectFromWindow(NULL, OBJID_CURSOR, &IID_IUnknown, (void**)&unk);
    todo_wine ok(hr == S_OK, "got %x\n", hr);
    if(hr == S_OK) IUnknown_Release(unk);

    hwnd = CreateWindowA("oleacc_test", "test", WS_OVERLAPPEDWINDOW,
            0, 0, 0, 0, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindow failed\n");

    hr = AccessibleObjectFromWindow(hwnd, OBJID_CURSOR, &IID_IUnknown, (void**)&unk);
    ok(hr == E_UNEXPECTED, "got %x\n", hr);

    ok(Object_ref == 1, "Object_ref = %d\n", Object_ref);
    hr = AccessibleObjectFromWindow(hwnd, OBJID_CLIENT, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "got %x\n", hr);
    ok(Object_ref == 2, "Object_ref = %d\n", Object_ref);
    IUnknown_Release(unk);

    DestroyWindow(hwnd);
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
    ok(hr == E_INVALIDARG, "AccessibleChildren returned %x\n", hr);
    ok(count == -1, "count = %d\n", count);
    hr = AccessibleChildren(acc, 0, 0, NULL, &count);
    ok(hr == E_INVALIDARG, "AccessibleChildren returned %x\n", hr);
    ok(count == -1, "count = %d\n", count);
    hr = AccessibleChildren(acc, 0, 0, children, NULL);
    ok(hr == E_INVALIDARG, "AccessibleChildren returned %x\n", hr);

    if(acc == &Accessible) {
        SET_EXPECT(Accessible_QI_IEnumVARIANT);
        SET_EXPECT(Accessible_get_accChildCount);
    }
    hr = AccessibleChildren(acc, 0, 0, children, &count);
    ok(hr == S_OK, "AccessibleChildren returned %x\n", hr);
    if(acc == &Accessible) {
        CHECK_CALLED(Accessible_QI_IEnumVARIANT);
        CHECK_CALLED(Accessible_get_accChildCount);
    }
    ok(!count, "count = %d\n", count);
    count = -1;
    if(acc == &Accessible) {
        SET_EXPECT(Accessible_QI_IEnumVARIANT);
        SET_EXPECT(Accessible_get_accChildCount);
    }
    hr = AccessibleChildren(acc, 5, 0, children, &count);
    ok(hr == S_OK, "AccessibleChildren returned %x\n", hr);
    if(acc == &Accessible) {
        CHECK_CALLED(Accessible_QI_IEnumVARIANT);
        CHECK_CALLED(Accessible_get_accChildCount);
    }
    ok(!count, "count = %d\n", count);

    memset(children, 0xfe, sizeof(children));
    V_VT(children) = VT_DISPATCH;
    if(acc == &Accessible) {
        SET_EXPECT(Accessible_QI_IEnumVARIANT);
        SET_EXPECT(Accessible_get_accChildCount);
        SET_EXPECT(Accessible_get_accChild);
    }
    hr = AccessibleChildren(acc, 0, 1, children, &count);
    ok(hr == S_OK, "AccessibleChildren returned %x\n", hr);
    if(acc == &Accessible) {
        CHECK_CALLED(Accessible_QI_IEnumVARIANT);
        CHECK_CALLED(Accessible_get_accChildCount);
        CHECK_CALLED(Accessible_get_accChild);

        ok(V_VT(children) == VT_I4, "V_VT(children) = %d\n", V_VT(children));
        ok(V_I4(children) == 1, "V_I4(children) = %d\n", V_I4(children));
    }else {
        ok(V_VT(children) == VT_DISPATCH, "V_VT(children) = %d\n", V_VT(children));
        IDispatch_Release(V_DISPATCH(children));
    }
    ok(count == 1, "count = %d\n", count);

    if(acc == &Accessible) {
        SET_EXPECT(Accessible_QI_IEnumVARIANT);
        SET_EXPECT(Accessible_get_accChildCount);
        SET_EXPECT(Accessible_get_accChild);
    }
    hr = AccessibleChildren(acc, 0, 3, children, &count);
    ok(hr == S_FALSE, "AccessibleChildren returned %x\n", hr);
    if(acc == &Accessible) {
        CHECK_CALLED(Accessible_QI_IEnumVARIANT);
        CHECK_CALLED(Accessible_get_accChildCount);
        CHECK_CALLED(Accessible_get_accChild);

        ok(V_VT(children) == VT_I4, "V_VT(children) = %d\n", V_VT(children));
        ok(V_I4(children) == 1, "V_I4(children) = %d\n", V_I4(children));
    }else {
        ok(V_VT(children) == VT_DISPATCH, "V_VT(children) = %d\n", V_VT(children));
        IDispatch_Release(V_DISPATCH(children));
    }
    ok(count == 1, "count = %d\n", count);
    ok(V_VT(children+1) == VT_EMPTY, "V_VT(children+1) = %d\n", V_VT(children+1));
    ok(V_VT(children+2) == VT_EMPTY, "V_VT(children+2) = %d\n", V_VT(children+2));
}

static void test_default_client_accessible_object(void)
{
    static const WCHAR testW[] = {'t','e','s','t',' ','t',' ','&','j','u','n','k',0};
    static const WCHAR shortcutW[] = {'A','l','t','+','t',0};

    IAccessible *acc;
    IDispatch *disp;
    IOleWindow *ow;
    IEnumVARIANT *ev;
    HWND chld, hwnd, hwnd2;
    HRESULT hr;
    VARIANT vid, v;
    BSTR str;
    POINT pt;
    RECT rect;
    LONG l, left, top, width, height;
    ULONG fetched;

    hwnd = CreateWindowA("oleacc_test", "test &t &junk", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "CreateWindow failed\n");
    chld = CreateWindowA("static", "message", WS_CHILD | WS_VISIBLE,
            0, 0, 50, 50, hwnd, NULL, NULL, NULL);
    ok(chld != NULL, "CreateWindow failed\n");

    hr = CreateStdAccessibleObject(NULL, OBJID_CLIENT, &IID_IAccessible, (void**)&acc);
    ok(hr == E_FAIL, "got %x\n", hr);

    hr = CreateStdAccessibleObject(hwnd, OBJID_CLIENT, &IID_IAccessible, (void**)&acc);
    ok(hr == S_OK, "got %x\n", hr);

    hr = IAccessible_QueryInterface(acc, &IID_IOleWindow, (void**)&ow);
    ok(hr == S_OK, "got %x\n", hr);
    hr = IOleWindow_GetWindow(ow, &hwnd2);
    ok(hr == S_OK, "got %x\n", hr);
    ok(hwnd == hwnd2, "hwnd2 = %p, expected %p\n", hwnd2, hwnd);
    hr = WindowFromAccessibleObject(acc, &hwnd2);
    ok(hr == S_OK, "got %x\n", hr);
    ok(hwnd == hwnd2, "hwnd2 = %p, expected %p\n", hwnd2, hwnd);
    IOleWindow_Release(ow);

    hr = IAccessible_get_accChildCount(acc, &l);
    ok(hr == S_OK, "got %x\n", hr);
    ok(l == 1, "l = %d\n", l);

    V_VT(&vid) = VT_I4;
    V_I4(&vid) = CHILDID_SELF;
    disp = (void*)0xdeadbeef;
    hr = IAccessible_get_accChild(acc, vid, &disp);
    ok(hr == E_INVALIDARG, "get_accChild returned %x\n", hr);
    ok(disp == NULL, "disp = %p\n", disp);

    V_I4(&vid) = 1;
    disp = (void*)0xdeadbeef;
    hr = IAccessible_get_accChild(acc, vid, &disp);
    ok(hr == E_INVALIDARG, "get_accChild returned %x\n", hr);
    ok(disp == NULL, "disp = %p\n", disp);

    hr = IAccessible_QueryInterface(acc, &IID_IEnumVARIANT, (void**)&ev);
    ok(hr == S_OK, "got %x\n", hr);

    hr = IEnumVARIANT_Skip(ev, 100);
    ok(hr == S_FALSE, "Skip returned %x\n", hr);

    V_VT(&v) = VT_I4;
    fetched = 1;
    hr = IEnumVARIANT_Next(ev, 1, &v, &fetched);
    ok(hr == S_FALSE, "got %x\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(fetched == 0, "fetched = %d\n", fetched);

    hr = IEnumVARIANT_Reset(ev);
    ok(hr == S_OK, "got %x\n", hr);

    V_VT(&v) = VT_I4;
    fetched = 2;
    hr = IEnumVARIANT_Next(ev, 1, &v, &fetched);
    ok(hr == S_OK, "got %x\n", hr);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(&v) = %d\n", V_VT(&v));
    IDispatch_Release(V_DISPATCH(&v));
    ok(fetched == 1, "fetched = %d\n", fetched);
    IEnumVARIANT_Release(ev);

    test_AccessibleChildren(acc);

    V_VT(&vid) = VT_I4;
    V_I4(&vid) = CHILDID_SELF;
    hr = IAccessible_get_accName(acc, vid, &str);
    ok(hr == S_OK, "got %x\n", hr);
    ok(!lstrcmpW(str, testW), "name = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    V_I4(&vid) = 1;
    str = (void*)0xdeadbeef;
    hr = IAccessible_get_accName(acc, vid, &str);
    ok(hr == E_INVALIDARG, "got %x\n", hr);
    ok(!str, "str != NULL\n");
    V_I4(&vid) = CHILDID_SELF;

    str = (void*)0xdeadbeef;
    hr = IAccessible_get_accValue(acc, vid, &str);
    ok(hr == S_FALSE, "got %x\n", hr);
    ok(!str, "str != NULL\n");

    str = (void*)0xdeadbeef;
    hr = IAccessible_get_accDescription(acc, vid, &str);
    ok(hr == S_FALSE, "got %x\n", hr);
    ok(!str, "str != NULL\n");

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (void*)0xdeadbeef;
    hr = IAccessible_get_accRole(acc, vid, &v);
    ok(hr == S_OK, "got %x\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == ROLE_SYSTEM_CLIENT, "V_I4(&v) = %d\n", V_I4(&v));

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (void*)0xdeadbeef;
    hr = IAccessible_get_accState(acc, vid, &v);
    ok(hr == S_OK, "got %x\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == (STATE_SYSTEM_FOCUSABLE|STATE_SYSTEM_INVISIBLE),
            "V_I4(&v) = %x\n", V_I4(&v));

    str = (void*)0xdeadbeef;
    hr = IAccessible_get_accHelp(acc, vid, &str);
    ok(hr == S_FALSE, "got %x\n", hr);
    ok(!str, "str != NULL\n");

    hr = IAccessible_get_accKeyboardShortcut(acc, vid, &str);
    ok(hr == S_OK, "got %x\n", hr);
    ok(!lstrcmpW(str, shortcutW), "str = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    str = (void*)0xdeadbeef;
    hr = IAccessible_get_accDefaultAction(acc, vid, &str);
    ok(hr == S_FALSE, "got %x\n", hr);
    ok(!str, "str != NULL\n");

    pt.x = pt.y = 60;
    ok(ClientToScreen(hwnd, &pt), "ClientToScreen failed\n");
    hr = IAccessible_accHitTest(acc, pt.x, pt.y, &v);
    ok(hr == S_OK, "got %x\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 0, "V_I4(&v) = %d\n", V_I4(&v));

    pt.x = pt.y = 25;
    ok(ClientToScreen(hwnd, &pt), "ClientToScreen failed\n");
    hr = IAccessible_accHitTest(acc, pt.x, pt.y, &v);
    ok(hr == S_OK, "got %x\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 0, "V_I4(&v) = %d\n", V_I4(&v));

    ShowWindow(hwnd, TRUE);
    pt.x = pt.y = 60;
    ok(ClientToScreen(hwnd, &pt), "ClientToScreen failed\n");
    hr = IAccessible_accHitTest(acc, pt.x, pt.y, &v);
    ok(hr == S_OK, "got %x\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 0, "V_I4(&v) = %d\n", V_I4(&v));

    pt.x = pt.y = 25;
    ok(ClientToScreen(hwnd, &pt), "ClientToScreen failed\n");
    hr = IAccessible_accHitTest(acc, pt.x, pt.y, &v);
    ok(hr == S_OK, "got %x\n", hr);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) != NULL, "V_DISPATCH(&v) = %p\n", V_DISPATCH(&v));
    VariantClear(&v);

    ShowWindow(chld, FALSE);
    pt.x = pt.y = 25;
    ok(ClientToScreen(hwnd, &pt), "ClientToScreen failed\n");
    hr = IAccessible_accHitTest(acc, pt.x, pt.y, &v);
    ok(hr == S_OK, "got %x\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 0, "V_I4(&v) = %d\n", V_I4(&v));

    hr = IAccessible_get_accParent(acc, &disp);
    ok(hr == S_OK, "got %x\n", hr);
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
    ok(hr == S_OK, "got %x\n", hr);
    ok(left == rect.left, "left = %d, expected %d\n", left, rect.left);
    ok(top == rect.top, "top = %d, expected %d\n", top, rect.top);
    ok(width == pt.x-rect.left, "width = %d, expected %d\n", width, pt.x-rect.left);
    ok(height == pt.y-rect.top, "height = %d, expected %d\n", height, pt.y-rect.top);

    DestroyWindow(hwnd);

    hr = IAccessible_get_accChildCount(acc, &l);
    ok(hr == S_OK, "got %x\n", hr);
    ok(l == 0, "l = %d\n", l);

    hr = IAccessible_get_accName(acc, vid, &str);
    ok(hr == E_INVALIDARG, "got %x\n", hr);

    hr = IAccessible_get_accValue(acc, vid, &str);
    ok(hr == S_FALSE, "got %x\n", hr);

    hr = IAccessible_get_accRole(acc, vid, &v);
    ok(hr == S_OK, "got %x\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == ROLE_SYSTEM_CLIENT, "V_I4(&v) = %d\n", V_I4(&v));

    hr = IAccessible_get_accState(acc, vid, &v);
    ok(hr == S_OK, "got %x\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == STATE_SYSTEM_INVISIBLE, "V_I4(&v) = %x\n", V_I4(&v));

    hr = IAccessible_accHitTest(acc, 200, 200, &v);
    ok(hr == S_OK, "got %x\n", hr);
    ok(V_VT(&v) == VT_I4, "V_VT(&v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 0, "V_I4(&v) = %d\n", V_I4(&v));

    disp = (void*)0xdeadbeef;
    hr = IAccessible_get_accParent(acc, &disp);
    ok(hr == E_FAIL, "got %x\n", hr);
    ok(disp == NULL, "disp = %p\n", disp);

    hr = IAccessible_accLocation(acc, &left, &top, &width, &height, vid);
    ok(hr == S_OK, "got %x\n", hr);
    ok(left == 0, "left =  %d\n", left);
    ok(top == 0, "top =  %d\n", top);
    ok(width == 0, "width =  %d\n", width);
    ok(height == 0, "height =  %d\n", height);

    IAccessible_Release(acc);
}

static void test_CAccPropServices(void)
{
    IAccPropServices *acc_prop_services;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_CAccPropServices, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IAccPropServices, (void**)&acc_prop_services);
    ok(hres == S_OK, "Could not create CAccPropServices instance: %08x\n", hres);

    IAccPropServices_Release(acc_prop_services);
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

        sscanf(argv[3], "%lx", &lres);
        hres = ObjectFromLresult(lres, &IID_IUnknown, 0, (void**)&unk);
        ok(hres == S_OK, "hres = %x\n", hres);
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
    test_AccessibleChildren(&Accessible);

    unregister_window_class();
    CoUninitialize();

    CoInitialize(NULL);
    test_CAccPropServices();
    CoUninitialize();
}
