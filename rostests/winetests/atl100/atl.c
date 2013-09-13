/*
 * Copyright 2012 Jacek Caban for CodeWeavers
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

#include <stdarg.h>
#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define CONST_VTABLE

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winreg.h>
#include <wingdi.h>
#include <objbase.h>
#include <oleauto.h>

#include <wine/atlbase.h>
#include <mshtml.h>

#include <wine/test.h>

static const GUID CLSID_Test =
    {0x178fc163,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
#define CLSID_TEST_STR "178fc163-0000-0000-0000-000000000046"

static const GUID CATID_CatTest1 =
    {0x178fc163,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x46}};
#define CATID_CATTEST1_STR "178fc163-0000-0000-0000-000000000146"

static const GUID CATID_CatTest2 =
    {0x178fc163,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x46}};
#define CATID_CATTEST2_STR "178fc163-0000-0000-0000-000000000246"

static const char *debugstr_guid(REFIID riid)
{
    static char buf[50];

    if(!riid)
        return "(null)";

    sprintf(buf, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            riid->Data1, riid->Data2, riid->Data3, riid->Data4[0],
            riid->Data4[1], riid->Data4[2], riid->Data4[3], riid->Data4[4],
            riid->Data4[5], riid->Data4[6], riid->Data4[7]);

    return buf;
}

static void test_winmodule(void)
{
    _AtlCreateWndData create_data[3];
    _ATL_WIN_MODULE winmod;
    void *p;
    HRESULT hres;

    winmod.cbSize = 0xdeadbeef;
    hres = AtlWinModuleInit(&winmod);
    ok(hres == E_INVALIDARG, "AtlWinModuleInit failed: %08x\n", hres);

    winmod.cbSize = sizeof(winmod);
    winmod.m_pCreateWndList = (void*)0xdeadbeef;
    winmod.m_csWindowCreate.LockCount = 0xdeadbeef;
    winmod.m_rgWindowClassAtoms.m_aT = (void*)0xdeadbeef;
    winmod.m_rgWindowClassAtoms.m_nSize = 0xdeadbeef;
    winmod.m_rgWindowClassAtoms.m_nAllocSize = 0xdeadbeef;
    hres = AtlWinModuleInit(&winmod);
    ok(hres == S_OK, "AtlWinModuleInit failed: %08x\n", hres);
    ok(!winmod.m_pCreateWndList, "winmod.m_pCreateWndList = %p\n", winmod.m_pCreateWndList);
    ok(winmod.m_csWindowCreate.LockCount == -1, "winmod.m_csWindowCreate.LockCount = %d\n",
       winmod.m_csWindowCreate.LockCount);
    ok(winmod.m_rgWindowClassAtoms.m_aT == (void*)0xdeadbeef, "winmod.m_rgWindowClassAtoms.m_aT = %p\n",
       winmod.m_rgWindowClassAtoms.m_aT);
    ok(winmod.m_rgWindowClassAtoms.m_nSize == 0xdeadbeef, "winmod.m_rgWindowClassAtoms.m_nSize = %d\n",
       winmod.m_rgWindowClassAtoms.m_nSize);
    ok(winmod.m_rgWindowClassAtoms.m_nAllocSize == 0xdeadbeef, "winmod.m_rgWindowClassAtoms.m_nAllocSize = %d\n",
       winmod.m_rgWindowClassAtoms.m_nAllocSize);

    InitializeCriticalSection(&winmod.m_csWindowCreate);

    AtlWinModuleAddCreateWndData(&winmod, create_data, (void*)0xdead0001);
    ok(winmod.m_pCreateWndList == create_data, "winmod.m_pCreateWndList != create_data\n");
    ok(create_data[0].m_pThis == (void*)0xdead0001, "unexpected create_data[0].m_pThis %p\n", create_data[0].m_pThis);
    ok(create_data[0].m_dwThreadID == GetCurrentThreadId(), "unexpected create_data[0].m_dwThreadID %x\n",
       create_data[0].m_dwThreadID);
    ok(!create_data[0].m_pNext, "unexpected create_data[0].m_pNext %p\n", create_data[0].m_pNext);

    AtlWinModuleAddCreateWndData(&winmod, create_data+1, (void*)0xdead0002);
    ok(winmod.m_pCreateWndList == create_data+1, "winmod.m_pCreateWndList != create_data\n");
    ok(create_data[1].m_pThis == (void*)0xdead0002, "unexpected create_data[1].m_pThis %p\n", create_data[1].m_pThis);
    ok(create_data[1].m_dwThreadID == GetCurrentThreadId(), "unexpected create_data[1].m_dwThreadID %x\n",
       create_data[1].m_dwThreadID);
    ok(create_data[1].m_pNext == create_data, "unexpected create_data[1].m_pNext %p\n", create_data[1].m_pNext);

    AtlWinModuleAddCreateWndData(&winmod, create_data+2, (void*)0xdead0003);
    ok(winmod.m_pCreateWndList == create_data+2, "winmod.m_pCreateWndList != create_data\n");
    ok(create_data[2].m_pThis == (void*)0xdead0003, "unexpected create_data[2].m_pThis %p\n", create_data[2].m_pThis);
    ok(create_data[2].m_dwThreadID == GetCurrentThreadId(), "unexpected create_data[2].m_dwThreadID %x\n",
       create_data[2].m_dwThreadID);
    ok(create_data[2].m_pNext == create_data+1, "unexpected create_data[2].m_pNext %p\n", create_data[2].m_pNext);

    p = AtlWinModuleExtractCreateWndData(&winmod);
    ok(p == (void*)0xdead0003, "unexpected AtlWinModuleExtractCreateWndData result %p\n", p);
    ok(winmod.m_pCreateWndList == create_data+1, "winmod.m_pCreateWndList != create_data\n");
    ok(create_data[2].m_pNext == create_data+1, "unexpected create_data[2].m_pNext %p\n", create_data[2].m_pNext);

    create_data[1].m_dwThreadID = 0xdeadbeef;

    p = AtlWinModuleExtractCreateWndData(&winmod);
    ok(p == (void*)0xdead0001, "unexpected AtlWinModuleExtractCreateWndData result %p\n", p);
    ok(winmod.m_pCreateWndList == create_data+1, "winmod.m_pCreateWndList != create_data\n");
    ok(!create_data[0].m_pNext, "unexpected create_data[0].m_pNext %p\n", create_data[0].m_pNext);
    ok(!create_data[1].m_pNext, "unexpected create_data[1].m_pNext %p\n", create_data[1].m_pNext);

    p = AtlWinModuleExtractCreateWndData(&winmod);
    ok(!p, "unexpected AtlWinModuleExtractCreateWndData result %p\n", p);
    ok(winmod.m_pCreateWndList == create_data+1, "winmod.m_pCreateWndList != create_data\n");
}

#define test_key_exists(a,b) _test_key_exists(__LINE__,a,b)
static void _test_key_exists(unsigned line, HKEY root, const char *key_name)
{
    HKEY key;
    DWORD res;

    res = RegOpenKeyA(root, key_name, &key);
    ok_(__FILE__,line)(res == ERROR_SUCCESS, "Could not open key %s\n", key_name);
    if(res == ERROR_SUCCESS)
        RegCloseKey(key);
}

#define test_key_not_exists(a,b) _test_key_not_exists(__LINE__,a,b)
static void _test_key_not_exists(unsigned line, HKEY root, const char *key_name)
{
    HKEY key;
    DWORD res;

    res = RegOpenKeyA(root, key_name, &key);
    ok_(__FILE__,line)(res == ERROR_FILE_NOT_FOUND, "Attempting to open %s returned %u\n", key_name, res);
    if(res == ERROR_SUCCESS)
        RegCloseKey(key);
}

static void test_regcat(void)
{
    unsigned char b;
    HRESULT hres;

    const struct _ATL_CATMAP_ENTRY catmap[] = {
        {_ATL_CATMAP_ENTRY_IMPLEMENTED, &CATID_CatTest1},
        {_ATL_CATMAP_ENTRY_REQUIRED, &CATID_CatTest2},
        {_ATL_CATMAP_ENTRY_END}
    };

    hres = AtlRegisterClassCategoriesHelper(&CLSID_Test, catmap, TRUE);
    ok(hres == S_OK, "AtlRegisterClassCategoriesHelper failed: %08x\n", hres);

    test_key_exists(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}");
    test_key_exists(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}\\Implemented Categories\\{" CATID_CATTEST1_STR "}");
    test_key_exists(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}\\Required Categories\\{" CATID_CATTEST2_STR "}");

    hres = AtlRegisterClassCategoriesHelper(&CLSID_Test, catmap, FALSE);
    ok(hres == S_OK, "AtlRegisterClassCategoriesHelper failed: %08x\n", hres);

    test_key_not_exists(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}\\Implemented Categories");
    test_key_not_exists(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}\\Required Categories");
    test_key_exists(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}");

    ok(RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}") == ERROR_SUCCESS, "Could not delete key\n");

    hres = AtlRegisterClassCategoriesHelper(&CLSID_Test, NULL, TRUE);
    ok(hres == S_OK, "AtlRegisterClassCategoriesHelper failed: %08x\n", hres);

    test_key_not_exists(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}");

    b = 10;
    hres = AtlGetPerUserRegistration(&b);
    ok(hres == S_OK, "AtlGetPerUserRegistration failed: %08x\n", hres);
    ok(!b, "AtlGetPerUserRegistration returned %x\n", b);
}

static void test_typelib(void)
{
    ITypeLib *typelib;
    HINSTANCE inst;
    size_t len;
    BSTR path;
    HRESULT hres;

    static const WCHAR scrrun_dll_suffixW[] = {'\\','s','c','r','r','u','n','.','d','l','l',0};
    static const WCHAR mshtml_tlb_suffixW[] = {'\\','m','s','h','t','m','l','.','t','l','b',0};

    inst = LoadLibraryA("scrrun.dll");
    ok(inst != NULL, "Could not load scrrun.dll\n");

    typelib = NULL;
    hres = AtlLoadTypeLib(inst, NULL, &path, &typelib);
    ok(hres == S_OK, "AtlLoadTypeLib failed: %08x\n", hres);
    FreeLibrary(inst);

    len = SysStringLen(path);
    ok(len > sizeof(scrrun_dll_suffixW)/sizeof(WCHAR)
       && lstrcmpiW(path+len-sizeof(scrrun_dll_suffixW)/sizeof(WCHAR), scrrun_dll_suffixW),
       "unexpected path %s\n", wine_dbgstr_w(path));
    SysFreeString(path);
    ok(typelib != NULL, "typelib == NULL\n");
    ITypeLib_Release(typelib);

    inst = LoadLibraryA("mshtml.dll");
    ok(inst != NULL, "Could not load mshtml.dll\n");

    typelib = NULL;
    hres = AtlLoadTypeLib(inst, NULL, &path, &typelib);
    ok(hres == S_OK, "AtlLoadTypeLib failed: %08x\n", hres);
    FreeLibrary(inst);

    len = SysStringLen(path);
    ok(len > sizeof(mshtml_tlb_suffixW)/sizeof(WCHAR)
       && lstrcmpiW(path+len-sizeof(mshtml_tlb_suffixW)/sizeof(WCHAR), mshtml_tlb_suffixW),
       "unexpected path %s\n", wine_dbgstr_w(path));
    SysFreeString(path);
    ok(typelib != NULL, "typelib == NULL\n");
    ITypeLib_Release(typelib);
}

static HRESULT WINAPI ConnectionPoint_QueryInterface(IConnectionPoint *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IConnectionPoint, riid)) {
        *ppv = iface;
        return S_OK;
    }

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

static int advise_cnt;

static HRESULT WINAPI ConnectionPoint_Advise(IConnectionPoint *iface, IUnknown *pUnkSink,
                                             DWORD *pdwCookie)
{
    ok(pUnkSink == (IUnknown*)0xdead0000, "pUnkSink = %p\n", pUnkSink);
    *pdwCookie = 0xdeadbeef;
    advise_cnt++;
    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_Unadvise(IConnectionPoint *iface, DWORD dwCookie)
{
    ok(dwCookie == 0xdeadbeef, "dwCookie = %x\n", dwCookie);
    advise_cnt--;
    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_EnumConnections(IConnectionPoint *iface,
                                                      IEnumConnections **ppEnum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
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

static IConnectionPoint ConnectionPoint = { &ConnectionPointVtbl };

static HRESULT WINAPI ConnectionPointContainer_QueryInterface(IConnectionPointContainer *iface,
        REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IConnectionPointContainer, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected call\n");
    return E_NOTIMPL;
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
    ok(IsEqualGUID(riid, &CLSID_Test), "unexpected riid\n");
    *ppCP = &ConnectionPoint;
    return S_OK;
}

static const IConnectionPointContainerVtbl ConnectionPointContainerVtbl = {
    ConnectionPointContainer_QueryInterface,
    ConnectionPointContainer_AddRef,
    ConnectionPointContainer_Release,
    ConnectionPointContainer_EnumConnectionPoints,
    ConnectionPointContainer_FindConnectionPoint
};

static IConnectionPointContainer ConnectionPointContainer = { &ConnectionPointContainerVtbl };

static void test_cp(void)
{
    DWORD cookie = 0;
    HRESULT hres;

    hres = AtlAdvise(NULL, (IUnknown*)0xdeed0000, &CLSID_Test, &cookie);
    ok(hres == E_INVALIDARG, "expect E_INVALIDARG, returned %08x\n", hres);

    hres = AtlUnadvise(NULL, &CLSID_Test, 0xdeadbeef);
    ok(hres == E_INVALIDARG, "expect E_INVALIDARG, returned %08x\n", hres);

    hres = AtlAdvise((IUnknown*)&ConnectionPointContainer, (IUnknown*)0xdead0000, &CLSID_Test, &cookie);
    ok(hres == S_OK, "AtlAdvise failed: %08x\n", hres);
    ok(cookie == 0xdeadbeef, "cookie = %x\n", cookie);
    ok(advise_cnt == 1, "advise_cnt = %d\n", advise_cnt);

    hres = AtlUnadvise((IUnknown*)&ConnectionPointContainer, &CLSID_Test, 0xdeadbeef);
    ok(hres == S_OK, "AtlUnadvise failed: %08x\n", hres);
    ok(!advise_cnt, "advise_cnt = %d\n", advise_cnt);
}

static CLSID persist_clsid;

static HRESULT WINAPI Persist_QueryInterface(IPersist *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI Persist_AddRef(IPersist *iface)
{
    return 2;
}

static ULONG WINAPI Persist_Release(IPersist *iface)
{
    return 1;
}

static HRESULT WINAPI Persist_GetClassID(IPersist *iface, CLSID *pClassID)
{
    *pClassID = persist_clsid;
    return S_OK;
}

static const IPersistVtbl PersistVtbl = {
    Persist_QueryInterface,
    Persist_AddRef,
    Persist_Release,
    Persist_GetClassID
};

static IPersist Persist = { &PersistVtbl };

static HRESULT WINAPI ProvideClassInfo2_QueryInterface(IProvideClassInfo2 *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ProvideClassInfo2_AddRef(IProvideClassInfo2 *iface)
{
    return 2;
}

static ULONG WINAPI ProvideClassInfo2_Release(IProvideClassInfo2 *iface)
{
    return 1;
}

static HRESULT WINAPI ProvideClassInfo2_GetClassInfo(IProvideClassInfo2 *iface, ITypeInfo **ppTI)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProvideClassInfo2_GetGUID(IProvideClassInfo2 *iface, DWORD dwGuidKind, GUID *pGUID)
{
    ok(dwGuidKind == GUIDKIND_DEFAULT_SOURCE_DISP_IID, "unexpected dwGuidKind %x\n", dwGuidKind);
    *pGUID = DIID_DispHTMLBody;
    return S_OK;
}

static const IProvideClassInfo2Vtbl ProvideClassInfo2Vtbl = {
    ProvideClassInfo2_QueryInterface,
    ProvideClassInfo2_AddRef,
    ProvideClassInfo2_Release,
    ProvideClassInfo2_GetClassInfo,
    ProvideClassInfo2_GetGUID
};

static IProvideClassInfo2 ProvideClassInfo2 = { &ProvideClassInfo2Vtbl };
static BOOL support_classinfo2;

static HRESULT WINAPI Dispatch_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IProvideClassInfo2, riid)) {
        if(!support_classinfo2)
            return E_NOINTERFACE;
        *ppv = &ProvideClassInfo2;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IPersist, riid)) {
        *ppv = &Persist;
        return S_OK;
    }

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
    ITypeLib *typelib;
    HRESULT hres;

    static const WCHAR mshtml_tlbW[] = {'m','s','h','t','m','l','.','t','l','b',0};

    ok(!iTInfo, "iTInfo = %d\n", iTInfo);
    ok(!lcid, "lcid = %x\n", lcid);

    hres = LoadTypeLib(mshtml_tlbW, &typelib);
    ok(hres == S_OK, "LoadTypeLib failed: %08x\n", hres);

    hres = ITypeLib_GetTypeInfoOfGuid(typelib, &IID_IHTMLElement, ppTInfo);
    ok(hres == S_OK, "GetTypeInfoOfGuid failed: %08x\n", hres);

    ITypeLib_Release(typelib);
    return S_OK;
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
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
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

static void test_source_iface(void)
{
    unsigned short maj_ver, min_ver;
    IID libid, iid;
    HRESULT hres;

    support_classinfo2 = TRUE;

    maj_ver = min_ver = 0xdead;
    hres = AtlGetObjectSourceInterface((IUnknown*)&Dispatch, &libid, &iid, &maj_ver, &min_ver);
    ok(hres == S_OK, "AtlGetObjectSourceInterface failed: %08x\n", hres);
    ok(IsEqualGUID(&libid, &LIBID_MSHTML), "libid = %s\n", debugstr_guid(&libid));
    ok(IsEqualGUID(&iid, &DIID_DispHTMLBody), "iid = %s\n", debugstr_guid(&iid));
    ok(maj_ver == 4 && min_ver == 0, "ver = %d.%d\n", maj_ver, min_ver);

    support_classinfo2 = FALSE;
    persist_clsid = CLSID_HTMLDocument;

    maj_ver = min_ver = 0xdead;
    hres = AtlGetObjectSourceInterface((IUnknown*)&Dispatch, &libid, &iid, &maj_ver, &min_ver);
    ok(hres == S_OK, "AtlGetObjectSourceInterface failed: %08x\n", hres);
    ok(IsEqualGUID(&libid, &LIBID_MSHTML), "libid = %s\n", debugstr_guid(&libid));
    ok(IsEqualGUID(&iid, &DIID_HTMLDocumentEvents), "iid = %s\n", debugstr_guid(&iid));
    ok(maj_ver == 4 && min_ver == 0, "ver = %d.%d\n", maj_ver, min_ver);

    persist_clsid = CLSID_HTMLStyle;

    maj_ver = min_ver = 0xdead;
    hres = AtlGetObjectSourceInterface((IUnknown*)&Dispatch, &libid, &iid, &maj_ver, &min_ver);
    ok(hres == S_OK, "AtlGetObjectSourceInterface failed: %08x\n", hres);
    ok(IsEqualGUID(&libid, &LIBID_MSHTML), "libid = %s\n", debugstr_guid(&libid));
    ok(IsEqualGUID(&iid, &IID_NULL), "iid = %s\n", debugstr_guid(&iid));
    ok(maj_ver == 4 && min_ver == 0, "ver = %d.%d\n", maj_ver, min_ver);
}

static void test_ax_win(void)
{
    BOOL ret;
    WNDCLASSEXW wcex;
    static const WCHAR AtlAxWin100[] = {'A','t','l','A','x','W','i','n','1','0','0',0};
    static const WCHAR AtlAxWinLic100[] = {'A','t','l','A','x','W','i','n','L','i','c','1','0','0',0};
    static HMODULE hinstance = 0;

    ret = AtlAxWinInit();
    ok(ret, "AtlAxWinInit failed\n");

    hinstance = GetModuleHandleA(NULL);

    memset(&wcex, 0, sizeof(wcex));
    wcex.cbSize = sizeof(wcex);
    ret = GetClassInfoExW(hinstance, AtlAxWin100, &wcex);
    ok(ret, "AtlAxWin100 has not registered\n");
    ok(wcex.style == (CS_GLOBALCLASS | CS_DBLCLKS), "wcex.style %08x\n", wcex.style);

    memset(&wcex, 0, sizeof(wcex));
    wcex.cbSize = sizeof(wcex);
    ret = GetClassInfoExW(hinstance, AtlAxWinLic100, &wcex);
    ok(ret, "AtlAxWinLic100 has not registered\n");
    ok(wcex.style == (CS_GLOBALCLASS | CS_DBLCLKS), "wcex.style %08x\n", wcex.style);
}

START_TEST(atl)
{
    CoInitialize(NULL);

    test_winmodule();
    test_regcat();
    test_typelib();
    test_cp();
    test_source_iface();
    test_ax_win();

    CoUninitialize();
}
