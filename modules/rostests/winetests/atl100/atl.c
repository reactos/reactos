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

#define COBJMACROS
#define CONST_VTABLE

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <exdisp.h>

#include <atlbase.h>
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
static const WCHAR progid1W[] = L"Shell.Explorer.2";
static const WCHAR clsid1W[] = L"{8856f961-340a-11d0-a96b-00c04fd705a2}";
static const WCHAR url1W[] = L"http://test.winehq.org/tests/winehq_snapshot/";
static const WCHAR mshtml1W[] = L"mshtml:<html><body>test</body></html>";
static const WCHAR mshtml2W[] = L"MSHTML:<html><body>test</body></html>";
static const WCHAR mshtml3W[] = L"<html><body>test</body></html>";
static const char html_str[] = "<html><body>test</body><html>";

static BOOL is_token_admin(HANDLE token)
{
    PSID administrators = NULL;
    SID_IDENTIFIER_AUTHORITY nt_authority = { SECURITY_NT_AUTHORITY };
    DWORD groups_size;
    PTOKEN_GROUPS groups;
    DWORD group_index;

    /* Create a well-known SID for the Administrators group. */
    if (! AllocateAndInitializeSid(&nt_authority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                   DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                   &administrators))
        return FALSE;

    /* Get the group info from the token */
    groups_size = 0;
    GetTokenInformation(token, TokenGroups, NULL, 0, &groups_size);
    groups = HeapAlloc(GetProcessHeap(), 0, groups_size);
    if (groups == NULL)
    {
        FreeSid(administrators);
        return FALSE;
    }
    if (! GetTokenInformation(token, TokenGroups, groups, groups_size, &groups_size))
    {
        HeapFree(GetProcessHeap(), 0, groups);
        FreeSid(administrators);
        return FALSE;
    }

    /* Now check if the token groups include the Administrators group */
    for (group_index = 0; group_index < groups->GroupCount; group_index++)
    {
        if (EqualSid(groups->Groups[group_index].Sid, administrators))
        {
            HeapFree(GetProcessHeap(), 0, groups);
            FreeSid(administrators);
            return TRUE;
        }
    }

    /* If we end up here we didn't find the Administrators group */
    HeapFree(GetProcessHeap(), 0, groups);
    FreeSid(administrators);
    return FALSE;
}

static BOOL is_process_limited(void)
{
    static BOOL (WINAPI *pOpenProcessToken)(HANDLE, DWORD, PHANDLE) = NULL;
    HANDLE token;
    BOOL result=FALSE;

    if (!pOpenProcessToken)
    {
        HMODULE hadvapi32 = GetModuleHandleA("advapi32.dll");
        pOpenProcessToken = (void*)GetProcAddress(hadvapi32, "OpenProcessToken");
        if (!pOpenProcessToken)
            return FALSE;
    }

    if (pOpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    {
        BOOL ret;
        TOKEN_ELEVATION_TYPE type = TokenElevationTypeDefault;
        DWORD size;

        ret = GetTokenInformation(token, TokenElevationType, &type, sizeof(type), &size);
        if (ret)
        {
            if (type == TokenElevationTypeDefault)
                /* UAC is disabled, check for administrators group */
                result = !is_token_admin(token);
            else if (type == TokenElevationTypeFull)
                result = FALSE;
            else if (type == TokenElevationTypeLimited)
                result = TRUE;
        }
        CloseHandle(token);
    }
    return result;
}

static void test_winmodule(void)
{
    _AtlCreateWndData create_data[3];
    _ATL_WIN_MODULE winmod;
    void *p;
    HRESULT hres;

    winmod.cbSize = 0xdeadbeef;
    hres = AtlWinModuleInit(&winmod);
    ok(hres == E_INVALIDARG, "AtlWinModuleInit failed: %08lx\n", hres);

    winmod.cbSize = sizeof(winmod);
    winmod.m_pCreateWndList = (void*)0xdeadbeef;
    winmod.m_csWindowCreate.LockCount = 0xdeadbeef;
    winmod.m_rgWindowClassAtoms.m_aT = (void*)0xdeadbeef;
    winmod.m_rgWindowClassAtoms.m_nSize = 0xdeadbeef;
    winmod.m_rgWindowClassAtoms.m_nAllocSize = 0xdeadbeef;
    hres = AtlWinModuleInit(&winmod);
    ok(hres == S_OK, "AtlWinModuleInit failed: %08lx\n", hres);
    ok(!winmod.m_pCreateWndList, "winmod.m_pCreateWndList = %p\n", winmod.m_pCreateWndList);
    ok(winmod.m_csWindowCreate.LockCount == -1, "winmod.m_csWindowCreate.LockCount = %ld\n",
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
    ok(create_data[0].m_dwThreadID == GetCurrentThreadId(), "unexpected create_data[0].m_dwThreadID %lx\n",
       create_data[0].m_dwThreadID);
    ok(!create_data[0].m_pNext, "unexpected create_data[0].m_pNext %p\n", create_data[0].m_pNext);

    AtlWinModuleAddCreateWndData(&winmod, create_data+1, (void*)0xdead0002);
    ok(winmod.m_pCreateWndList == create_data+1, "winmod.m_pCreateWndList != create_data\n");
    ok(create_data[1].m_pThis == (void*)0xdead0002, "unexpected create_data[1].m_pThis %p\n", create_data[1].m_pThis);
    ok(create_data[1].m_dwThreadID == GetCurrentThreadId(), "unexpected create_data[1].m_dwThreadID %lx\n",
       create_data[1].m_dwThreadID);
    ok(create_data[1].m_pNext == create_data, "unexpected create_data[1].m_pNext %p\n", create_data[1].m_pNext);

    AtlWinModuleAddCreateWndData(&winmod, create_data+2, (void*)0xdead0003);
    ok(winmod.m_pCreateWndList == create_data+2, "winmod.m_pCreateWndList != create_data\n");
    ok(create_data[2].m_pThis == (void*)0xdead0003, "unexpected create_data[2].m_pThis %p\n", create_data[2].m_pThis);
    ok(create_data[2].m_dwThreadID == GetCurrentThreadId(), "unexpected create_data[2].m_dwThreadID %lx\n",
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
    ok_(__FILE__,line)(res == ERROR_FILE_NOT_FOUND, "Attempting to open %s returned %lu\n", key_name, res);
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

    if (is_process_limited())
    {
        skip("process is limited\n");
        return;
    }

    hres = AtlRegisterClassCategoriesHelper(&CLSID_Test, catmap, TRUE);
    ok(hres == S_OK, "AtlRegisterClassCategoriesHelper failed: %08lx\n", hres);

    test_key_exists(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}");
    test_key_exists(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}\\Implemented Categories\\{" CATID_CATTEST1_STR "}");
    test_key_exists(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}\\Required Categories\\{" CATID_CATTEST2_STR "}");

    hres = AtlRegisterClassCategoriesHelper(&CLSID_Test, catmap, FALSE);
    ok(hres == S_OK, "AtlRegisterClassCategoriesHelper failed: %08lx\n", hres);

    test_key_not_exists(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}\\Implemented Categories");
    test_key_not_exists(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}\\Required Categories");
    test_key_exists(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}");

    ok(RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}") == ERROR_SUCCESS, "Could not delete key\n");

    hres = AtlRegisterClassCategoriesHelper(&CLSID_Test, NULL, TRUE);
    ok(hres == S_OK, "AtlRegisterClassCategoriesHelper failed: %08lx\n", hres);

    test_key_not_exists(HKEY_CLASSES_ROOT, "CLSID\\{" CLSID_TEST_STR "}");

    b = 10;
    hres = AtlGetPerUserRegistration(&b);
    ok(hres == S_OK, "AtlGetPerUserRegistration failed: %08lx\n", hres);
    ok(!b, "AtlGetPerUserRegistration returned %x\n", b);
}

static void test_typelib(void)
{
    ITypeLib *typelib;
    HINSTANCE inst;
    size_t len;
    BSTR path;
    HRESULT hres;

    inst = LoadLibraryA("scrrun.dll");
    ok(inst != NULL, "Could not load scrrun.dll\n");

    typelib = NULL;
    hres = AtlLoadTypeLib(inst, NULL, &path, &typelib);
    ok(hres == S_OK, "AtlLoadTypeLib failed: %08lx\n", hres);
    FreeLibrary(inst);

    len = SysStringLen(path);
    ok(len > ARRAY_SIZE(L"\\scrrun.dll")
       && lstrcmpiW(path+len-ARRAY_SIZE(L"\\scrrun.dll"), L"\\scrrun.dll"),
       "unexpected path %s\n", wine_dbgstr_w(path));
    SysFreeString(path);
    ok(typelib != NULL, "typelib == NULL\n");
    ITypeLib_Release(typelib);

    inst = LoadLibraryA("mshtml.dll");
    ok(inst != NULL, "Could not load mshtml.dll\n");

    typelib = NULL;
    hres = AtlLoadTypeLib(inst, NULL, &path, &typelib);
    ok(hres == S_OK, "AtlLoadTypeLib failed: %08lx\n", hres);
    FreeLibrary(inst);

    len = SysStringLen(path);
    ok(len > ARRAY_SIZE(L"\\mshtml.tlb")
       && lstrcmpiW(path+len-ARRAY_SIZE(L"\\mshtml.tlb"), L"\\mshtml.tlb"),
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
    ok(dwCookie == 0xdeadbeef, "dwCookie = %lx\n", dwCookie);
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
    ok(hres == E_INVALIDARG, "expect E_INVALIDARG, returned %08lx\n", hres);

    hres = AtlUnadvise(NULL, &CLSID_Test, 0xdeadbeef);
    ok(hres == E_INVALIDARG, "expect E_INVALIDARG, returned %08lx\n", hres);

    hres = AtlAdvise((IUnknown*)&ConnectionPointContainer, (IUnknown*)0xdead0000, &CLSID_Test, &cookie);
    ok(hres == S_OK, "AtlAdvise failed: %08lx\n", hres);
    ok(cookie == 0xdeadbeef, "cookie = %lx\n", cookie);
    ok(advise_cnt == 1, "advise_cnt = %d\n", advise_cnt);

    hres = AtlUnadvise((IUnknown*)&ConnectionPointContainer, &CLSID_Test, 0xdeadbeef);
    ok(hres == S_OK, "AtlUnadvise failed: %08lx\n", hres);
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
    ok(dwGuidKind == GUIDKIND_DEFAULT_SOURCE_DISP_IID, "unexpected dwGuidKind %lx\n", dwGuidKind);
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
    ITypeLib *typelib;
    HRESULT hres;

    ok(!iTInfo, "iTInfo = %d\n", iTInfo);
    ok(!lcid, "lcid = %lx\n", lcid);

    hres = LoadTypeLib(L"mshtml.tlb", &typelib);
    ok(hres == S_OK, "LoadTypeLib failed: %08lx\n", hres);

    hres = ITypeLib_GetTypeInfoOfGuid(typelib, &IID_IHTMLElement, ppTInfo);
    ok(hres == S_OK, "GetTypeInfoOfGuid failed: %08lx\n", hres);

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
    ok(hres == S_OK, "AtlGetObjectSourceInterface failed: %08lx\n", hres);
    ok(IsEqualGUID(&libid, &LIBID_MSHTML), "libid = %s\n", wine_dbgstr_guid(&libid));
    ok(IsEqualGUID(&iid, &DIID_DispHTMLBody), "iid = %s\n", wine_dbgstr_guid(&iid));
    ok(maj_ver == 4 && min_ver == 0, "ver = %d.%d\n", maj_ver, min_ver);

    support_classinfo2 = FALSE;
    persist_clsid = CLSID_HTMLDocument;

    maj_ver = min_ver = 0xdead;
    hres = AtlGetObjectSourceInterface((IUnknown*)&Dispatch, &libid, &iid, &maj_ver, &min_ver);
    ok(hres == S_OK, "AtlGetObjectSourceInterface failed: %08lx\n", hres);
    ok(IsEqualGUID(&libid, &LIBID_MSHTML), "libid = %s\n", wine_dbgstr_guid(&libid));
    ok(IsEqualGUID(&iid, &DIID_HTMLDocumentEvents), "iid = %s\n", wine_dbgstr_guid(&iid));
    ok(maj_ver == 4 && min_ver == 0, "ver = %d.%d\n", maj_ver, min_ver);

    persist_clsid = CLSID_HTMLStyle;

    maj_ver = min_ver = 0xdead;
    hres = AtlGetObjectSourceInterface((IUnknown*)&Dispatch, &libid, &iid, &maj_ver, &min_ver);
    ok(hres == S_OK, "AtlGetObjectSourceInterface failed: %08lx\n", hres);
    ok(IsEqualGUID(&libid, &LIBID_MSHTML), "libid = %s\n", wine_dbgstr_guid(&libid));
    ok(IsEqualGUID(&iid, &IID_NULL), "iid = %s\n", wine_dbgstr_guid(&iid));
    ok(maj_ver == 4 && min_ver == 0, "ver = %d.%d\n", maj_ver, min_ver);
}

static void test_ax_win(void)
{
    DWORD ret, ret_size, i;
    HRESULT res;
    HWND hwnd;
    HANDLE hfile;
    IUnknown *control;
    WNDPROC wndproc[2] = {NULL, NULL};
    WCHAR file_uri1W[MAX_PATH], pathW[MAX_PATH];
    WNDCLASSEXW wcex;
    static HMODULE hinstance = 0;
    static const WCHAR cls_names[][16] =
    {
        L"AtlAxWin100",
        L"AtlAxWinLic100"
    };

    ret = AtlAxWinInit();
    ok(ret, "AtlAxWinInit failed\n");

    hinstance = GetModuleHandleA(NULL);

    for (i = 0; i < 2; i++)
    {
        memset(&wcex, 0, sizeof(wcex));
        wcex.cbSize = sizeof(wcex);
        ret = GetClassInfoExW(hinstance, cls_names[i], &wcex);
        ok(ret, "%s has not registered\n", wine_dbgstr_w(cls_names[i]));
        ok(wcex.style == (CS_GLOBALCLASS | CS_DBLCLKS), "wcex.style %08x\n", wcex.style);
        wndproc[i] = wcex.lpfnWndProc;

        hwnd = CreateWindowW(cls_names[i], NULL, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        ok(hwnd != NULL, "CreateWindow failed!\n");
        control = (IUnknown *)0xdeadbeef;
        res = AtlAxGetControl(hwnd, &control);
        ok(res == E_FAIL, "Expected E_FAIL, returned %08lx\n", res);
        ok(!control, "returned %p\n", control);
        if (control) IUnknown_Release(control);
        DestroyWindow(hwnd);

        hwnd = CreateWindowW(cls_names[i], L"", 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        ok(hwnd != NULL, "CreateWindow failed!\n");
        control = (IUnknown *)0xdeadbeef;
        res = AtlAxGetControl(hwnd, &control);
        ok(res == E_FAIL, "Expected E_FAIL, returned %08lx\n", res);
        ok(!control, "returned %p\n", control);
        if (control) IUnknown_Release(control);
        DestroyWindow(hwnd);

        hwnd = CreateWindowW(cls_names[i], L"random", 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        todo_wine ok(!hwnd, "returned %p\n", hwnd);
        if(hwnd) DestroyWindow(hwnd);

        hwnd = CreateWindowW(cls_names[i], progid1W, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        ok(hwnd != NULL, "CreateWindow failed!\n");
        control = NULL;
        res = AtlAxGetControl(hwnd, &control);
        ok(res == S_OK, "AtlAxGetControl failed with res %08lx\n", res);
        ok(control != NULL, "AtlAxGetControl failed!\n");
        IUnknown_Release(control);
        DestroyWindow(hwnd);

        hwnd = CreateWindowW(cls_names[i], clsid1W, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        ok(hwnd != NULL, "CreateWindow failed!\n");
        control = NULL;
        res = AtlAxGetControl(hwnd, &control);
        ok(res == S_OK, "AtlAxGetControl failed with res %08lx\n", res);
        ok(control != NULL, "AtlAxGetControl failed!\n");
        IUnknown_Release(control);
        DestroyWindow(hwnd);

        hwnd = CreateWindowW(cls_names[i], url1W, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        ok(hwnd != NULL, "CreateWindow failed!\n");
        control = NULL;
        res = AtlAxGetControl(hwnd, &control);
        ok(res == S_OK, "AtlAxGetControl failed with res %08lx\n", res);
        ok(control != NULL, "AtlAxGetControl failed!\n");
        IUnknown_Release(control);
        DestroyWindow(hwnd);

        /* test html stream with "MSHTML:" prefix */
        hwnd = CreateWindowW(cls_names[i], mshtml1W, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        ok(hwnd != NULL, "CreateWindow failed!\n");
        control = NULL;
        res = AtlAxGetControl(hwnd, &control);
        ok(res == S_OK, "AtlAxGetControl failed with res %08lx\n", res);
        ok(control != NULL, "AtlAxGetControl failed!\n");
        IUnknown_Release(control);
        DestroyWindow(hwnd);

        hwnd = CreateWindowW(cls_names[i], mshtml2W, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        ok(hwnd != NULL, "CreateWindow failed!\n");
        control = NULL;
        res = AtlAxGetControl(hwnd, &control);
        ok(res == S_OK, "AtlAxGetControl failed with res %08lx\n", res);
        ok(control != NULL, "AtlAxGetControl failed!\n");
        IUnknown_Release(control);
        DestroyWindow(hwnd);

        /* test html stream without "MSHTML:" prefix */
        hwnd = CreateWindowW(cls_names[i], mshtml3W, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        todo_wine ok(!hwnd, "returned %p\n", hwnd);
        if(hwnd) DestroyWindow(hwnd);

        ret = GetTempPathW(MAX_PATH, pathW);
        ok(ret, "GetTempPath failed!\n");
        lstrcatW(pathW, L"test.html");
        hfile = CreateFileW(pathW, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, 0);
        ok(hfile != INVALID_HANDLE_VALUE, "failed to create file\n");
        ret = WriteFile(hfile, html_str, sizeof(html_str), &ret_size, NULL);
        ok(ret, "WriteFile failed\n");
        CloseHandle(hfile);

        /* test C:// scheme */
        hwnd = CreateWindowW(cls_names[i], pathW, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        ok(hwnd != NULL, "CreateWindow failed!\n");
        control = NULL;
        res = AtlAxGetControl(hwnd, &control);
        ok(res == S_OK, "AtlAxGetControl failed with res %08lx\n", res);
        ok(control != NULL, "AtlAxGetControl failed!\n");
        IUnknown_Release(control);
        DestroyWindow(hwnd);

        /* test file:// scheme */
        lstrcpyW(file_uri1W, L"file:///");
        lstrcatW(file_uri1W, pathW);
        hwnd = CreateWindowW(cls_names[i], file_uri1W, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        ok(hwnd != NULL, "CreateWindow failed!\n");
        control = NULL;
        res = AtlAxGetControl(hwnd, &control);
        ok(res == S_OK, "AtlAxGetControl failed with res %08lx\n", res);
        ok(control != NULL, "AtlAxGetControl failed!\n");
        IUnknown_Release(control);
        DestroyWindow(hwnd);

        /* test file:// scheme on non-existent file */
        ret = DeleteFileW(pathW);
        ok(ret, "DeleteFile failed (gle=%lu)\n", GetLastError());
        hwnd = CreateWindowW(cls_names[i], file_uri1W, 0, 100, 100, 100, 100, NULL, NULL, NULL, NULL);
        ok(hwnd != NULL, "CreateWindow failed!\n");
        control = NULL;
        res = AtlAxGetControl(hwnd, &control);
        ok(res == S_OK, "AtlAxGetControl failed with res %08lx\n", res);
        ok(control != NULL, "AtlAxGetControl failed!\n");
        IUnknown_Release(control);
        DestroyWindow(hwnd);
    }
    todo_wine ok(wndproc[0] != wndproc[1], "expected different proc!\n");
}

static ATOM register_class(void)
{
    WNDCLASSA wndclassA;

    wndclassA.style = 0;
    wndclassA.lpfnWndProc = DefWindowProcA;
    wndclassA.cbClsExtra = 0;
    wndclassA.cbWndExtra = 0;
    wndclassA.hInstance = GetModuleHandleA(NULL);
    wndclassA.hIcon = NULL;
    wndclassA.hCursor = LoadCursorA(NULL, (LPSTR)IDC_ARROW);
    wndclassA.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    wndclassA.lpszMenuName = NULL;
    wndclassA.lpszClassName = "WineAtlTestClass";

    return RegisterClassA(&wndclassA);
}

static HWND create_container_window(void)
{
    return CreateWindowA("WineAtlTestClass", "Wine ATL Test Window", 0,
                              CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                              CW_USEDEFAULT, NULL, NULL, NULL, NULL);
}

static void test_AtlAxAttachControl(void)
{
    HWND hwnd;
    HRESULT hr;
    IUnknown *control, *container;
    LONG val;

    hr = AtlAxAttachControl(NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Expected AtlAxAttachControl to return E_INVALIDARG, got 0x%08lx\n", hr);

    container = (IUnknown *)0xdeadbeef;
    hr = AtlAxAttachControl(NULL, NULL, &container);
    ok(hr == E_INVALIDARG, "Expected AtlAxAttachControl to return E_INVALIDARG, got 0x%08lx\n", hr);
    ok(container == (IUnknown *)0xdeadbeef,
       "Expected the output container pointer to be untouched, got %p\n", container);

    hwnd = create_container_window();
    hr = AtlAxAttachControl(NULL, hwnd, NULL);
    ok(hr == E_INVALIDARG, "Expected AtlAxAttachControl to return E_INVALIDARG, got 0x%08lx\n", hr);
    DestroyWindow(hwnd);

    hwnd = create_container_window();
    container = (IUnknown *)0xdeadbeef;
    hr = AtlAxAttachControl(NULL, hwnd, &container);
    ok(hr == E_INVALIDARG, "Expected AtlAxAttachControl to return E_INVALIDARG, got 0x%08lx\n", hr);
    ok(container == (IUnknown *)0xdeadbeef, "returned %p\n", container);
    DestroyWindow(hwnd);

    hr = CoCreateInstance(&CLSID_WebBrowser, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                          &IID_IOleObject, (void **)&control);
    ok(hr == S_OK, "Expected CoCreateInstance to return S_OK, got 0x%08lx\n", hr);

    if (FAILED(hr))
    {
        skip("Couldn't obtain a test IOleObject instance\n");
        return;
    }

    hr = AtlAxAttachControl(control, NULL, NULL);
    ok(hr == S_FALSE, "Expected AtlAxAttachControl to return S_FALSE, got 0x%08lx\n", hr);

    container = NULL;
    hr = AtlAxAttachControl(control, NULL, &container);
    ok(hr == S_FALSE, "Expected AtlAxAttachControl to return S_FALSE, got 0x%08lx\n", hr);
    ok(container != NULL, "got %p\n", container);
    IUnknown_Release(container);

    hwnd = create_container_window();
    SetWindowLongW(hwnd, GWLP_USERDATA, 0xdeadbeef);
    hr = AtlAxAttachControl(control, hwnd, NULL);
    ok(hr == S_OK, "Expected AtlAxAttachControl to return S_OK, got 0x%08lx\n", hr);
    val = GetWindowLongW(hwnd, GWLP_USERDATA);
    ok(val == 0xdeadbeef, "returned %08lx\n", val);
    DestroyWindow(hwnd);

    hwnd = create_container_window();
    SetWindowLongW(hwnd, GWLP_USERDATA, 0xdeadbeef);
    container = NULL;
    hr = AtlAxAttachControl(control, hwnd, &container);
    ok(hr == S_OK, "Expected AtlAxAttachControl to return S_OK, got 0x%08lx\n", hr);
    ok(container != NULL, "Expected not NULL!\n");
    IUnknown_Release(container);
    val = GetWindowLongW(hwnd, GWLP_USERDATA);
    ok(val == 0xdeadbeef, "Expected unchanged, returned %08lx\n", val);
    DestroyWindow(hwnd);

    IUnknown_Release(control);
}

static void test_AtlAxCreateControl(void)
{
    HWND hwnd;
    IUnknown *control, *container;
    HRESULT hr;
    DWORD ret, ret_size;
    HANDLE hfile;
    WCHAR file_uri1W[MAX_PATH], pathW[MAX_PATH];

    container = NULL;
    control = (IUnknown *)0xdeadbeef;
    hr = AtlAxCreateControlEx(NULL, NULL, NULL, &container, &control, NULL, NULL);
    todo_wine ok(hr == S_FALSE, "got 0x%08lx\n", hr);
    todo_wine ok(container != NULL, "returned %p\n", container);
    ok(!control, "returned %p\n", control);

    container = NULL;
    control = (IUnknown *)0xdeadbeef;
    hwnd = create_container_window();
    ok(hwnd != NULL, "create window failed!\n");
    hr = AtlAxCreateControlEx(NULL, hwnd, NULL, &container, &control, &IID_NULL, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    todo_wine ok(container != NULL, "returned %p!\n", container);
    ok(!control, "returned %p\n", control);
    DestroyWindow(hwnd);

    container = NULL;
    control = (IUnknown *)0xdeadbeef;
    hwnd = create_container_window();
    ok(hwnd != NULL, "create window failed!\n");
    hr = AtlAxCreateControlEx(L"", hwnd, NULL, &container, &control, &IID_NULL, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    todo_wine ok(container != NULL, "returned %p!\n", container);
    ok(!control, "returned %p\n", control);
    DestroyWindow(hwnd);

    container = (IUnknown *)0xdeadbeef;
    control = (IUnknown *)0xdeadbeef;
    hwnd = create_container_window();
    ok(hwnd != NULL, "create window failed!\n");
    hr = AtlAxCreateControlEx(L"random", hwnd, NULL, &container, &control, &IID_NULL, NULL);
    ok(hr == CO_E_CLASSSTRING, "got 0x%08lx\n", hr);
    ok(!container, "returned %p!\n", container);
    ok(!control, "returned %p\n", control);
    DestroyWindow(hwnd);

    container = NULL;
    control = NULL;
    hwnd = create_container_window();
    ok(hwnd != NULL, "create window failed!\n");
    hr = AtlAxCreateControlEx(progid1W, hwnd, NULL, &container, &control, &IID_NULL, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(container != NULL, "returned %p!\n", container);
    ok(control != NULL, "returned %p\n", control);
    IUnknown_Release(container);
    IUnknown_Release(control);
    DestroyWindow(hwnd);

    container = NULL;
    control = NULL;
    hwnd = create_container_window();
    ok(hwnd != NULL, "create window failed!\n");
    hr = AtlAxCreateControlEx(clsid1W, hwnd, NULL, &container, &control, &IID_NULL, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(container != NULL, "returned %p!\n", container);
    ok(control != NULL, "returned %p\n", control);
    IUnknown_Release(container);
    IUnknown_Release(control);
    DestroyWindow(hwnd);

    container = NULL;
    control = NULL;
    hwnd = create_container_window();
    ok(hwnd != NULL, "create window failed!\n");
    hr = AtlAxCreateControlEx(url1W, hwnd, NULL, &container, &control, &IID_NULL, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(container != NULL, "returned %p!\n", container);
    ok(control != NULL, "returned %p\n", control);
    IUnknown_Release(container);
    IUnknown_Release(control);
    DestroyWindow(hwnd);

    container = NULL;
    control = NULL;
    hwnd = create_container_window();
    ok(hwnd != NULL, "create window failed!\n");
    hr = AtlAxCreateControlEx(mshtml1W, hwnd, NULL, &container, &control, &IID_NULL, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(container != NULL, "returned %p!\n", container);
    ok(control != NULL, "returned %p\n", control);
    IUnknown_Release(container);
    IUnknown_Release(control);
    DestroyWindow(hwnd);

    container = NULL;
    control = NULL;
    hwnd = create_container_window();
    ok(hwnd != NULL, "create window failed!\n");
    hr = AtlAxCreateControlEx(mshtml2W, hwnd, NULL, &container, &control, &IID_NULL, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(container != NULL, "returned %p!\n", container);
    ok(control != NULL, "returned %p\n", control);
    IUnknown_Release(container);
    IUnknown_Release(control);
    DestroyWindow(hwnd);

    container = (IUnknown *)0xdeadbeef;
    control = (IUnknown *)0xdeadbeef;
    hwnd = create_container_window();
    ok(hwnd != NULL, "create window failed!\n");
    hr = AtlAxCreateControlEx(mshtml3W, hwnd, NULL, &container, &control, &IID_NULL, NULL);
    ok(hr == CO_E_CLASSSTRING, "got 0x%08lx\n", hr);
    ok(!container, "returned %p!\n", container);
    ok(!control, "returned %p\n", control);
    DestroyWindow(hwnd);

    ret = GetTempPathW(MAX_PATH, pathW);
    ok(ret, "GetTempPath failed!\n");
    lstrcatW(pathW, L"test.html");
    hfile = CreateFileW(pathW, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to create file\n");
    ret = WriteFile(hfile, html_str, sizeof(html_str), &ret_size, NULL);
    ok(ret, "WriteFile failed\n");
    CloseHandle(hfile);

    /* test C:// scheme */
    container = NULL;
    control = NULL;
    hwnd = create_container_window();
    ok(hwnd != NULL, "create window failed!\n");
    hr = AtlAxCreateControlEx(pathW, hwnd, NULL, &container, &control, &IID_NULL, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(container != NULL, "returned %p!\n", container);
    ok(control != NULL, "returned %p\n", control);
    IUnknown_Release(container);
    IUnknown_Release(control);
    DestroyWindow(hwnd);

    /* test file:// scheme */
    lstrcpyW(file_uri1W, L"file:///");
    lstrcatW(file_uri1W, pathW);
    container = NULL;
    control = NULL;
    hwnd = create_container_window();
    ok(hwnd != NULL, "create window failed!\n");
    hr = AtlAxCreateControlEx(file_uri1W, hwnd, NULL, &container, &control, &IID_NULL, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(container != NULL, "returned %p!\n", container);
    ok(control != NULL, "returned %p\n", control);
    IUnknown_Release(container);
    IUnknown_Release(control);
    DestroyWindow(hwnd);

    /* test file:// scheme on non-existent file. */
    ret = DeleteFileW(pathW);
    ok(ret, "DeleteFile failed (gle=%lu)\n", GetLastError());
    container = NULL;
    control = NULL;
    hwnd = create_container_window();
    ok(hwnd != NULL, "create window failed!\n");
    hr = AtlAxCreateControlEx(file_uri1W, hwnd, NULL, &container, &control, &IID_NULL, NULL);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(container != NULL, "returned %p!\n", container);
    ok(control != NULL, "returned %p\n", control);
    IUnknown_Release(container);
    IUnknown_Release(control);
    DestroyWindow(hwnd);
}

static void test_AtlComModuleGetClassObject(void)
{
    _ATL_OBJMAP_ENTRY *null_entry = NULL;
    _ATL_COM_MODULE module;
    HRESULT hr;
    void *ret;

    /* Test NULL module */
    hr = AtlComModuleGetClassObject(NULL, &GUID_NULL, &IID_NULL, &ret);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Test NULL m_ppAutoObjMapFirst and m_ppAutoObjMapLast */
    module.cbSize = sizeof(module);
    module.m_ppAutoObjMapFirst = NULL;
    module.m_ppAutoObjMapLast = NULL;
    hr = AtlComModuleGetClassObject(&module, &GUID_NULL, &IID_NULL, &ret);
    ok(hr == CLASS_E_CLASSNOTAVAILABLE, "Unexpected hr %#lx.\n", hr);

    /* Test m_ppAutoObjMapFirst and m_ppAutoObjMapLast both pointing to a NULL entry */
    module.cbSize = sizeof(module);
    module.m_ppAutoObjMapFirst = &null_entry;
    module.m_ppAutoObjMapLast = &null_entry;
    hr = AtlComModuleGetClassObject(&module, &GUID_NULL, &IID_NULL, &ret);
    ok(hr == CLASS_E_CLASSNOTAVAILABLE, "Unexpected hr %#lx.\n", hr);
}

static void test_AtlComModuleRegisterClassObjects(void)
{
    _ATL_OBJMAP_ENTRY *null_entry = NULL;
    _ATL_COM_MODULE module;
    HRESULT hr;

    /* Test NULL module */
    hr = AtlComModuleRegisterClassObjects(NULL, CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Test NULL m_ppAutoObjMapFirst and m_ppAutoObjMapLast */
    module.cbSize = sizeof(module);
    module.m_ppAutoObjMapFirst = NULL;
    module.m_ppAutoObjMapLast = NULL;
    hr = AtlComModuleRegisterClassObjects(&module, CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE);
    todo_wine_if(hr == S_OK)
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    /* Test m_ppAutoObjMapFirst and m_ppAutoObjMapLast both pointing to a NULL entry */
    module.cbSize = sizeof(module);
    module.m_ppAutoObjMapFirst = &null_entry;
    module.m_ppAutoObjMapLast = &null_entry;
    hr = AtlComModuleRegisterClassObjects(&module, CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE);
    todo_wine_if(hr == S_OK)
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
}

static void test_AtlComModuleRevokeClassObjects(void)
{
    _ATL_OBJMAP_ENTRY *null_entry = NULL;
    _ATL_COM_MODULE module;
    HRESULT hr;

    /* Test NULL module */
    hr = AtlComModuleRevokeClassObjects(NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Test NULL m_ppAutoObjMapFirst and m_ppAutoObjMapLast */
    module.cbSize = sizeof(module);
    module.m_ppAutoObjMapFirst = NULL;
    module.m_ppAutoObjMapLast = NULL;
    hr = AtlComModuleRevokeClassObjects(&module);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Test m_ppAutoObjMapFirst and m_ppAutoObjMapLast both pointing to a NULL entry */
    module.cbSize = sizeof(module);
    module.m_ppAutoObjMapFirst = &null_entry;
    module.m_ppAutoObjMapLast = &null_entry;
    hr = AtlComModuleRevokeClassObjects(&module);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
}

START_TEST(atl)
{
    if (!register_class())
        return;

    CoInitialize(NULL);

    test_winmodule();
    test_regcat();
    test_typelib();
    test_cp();
    test_source_iface();
    test_ax_win();
    test_AtlAxAttachControl();
    test_AtlAxCreateControl();
    test_AtlComModuleGetClassObject();
    test_AtlComModuleRegisterClassObjects();
    test_AtlComModuleRevokeClassObjects();

    CoUninitialize();
}
