/*
 * Component Object Tests
 *
 * Copyright 2005 Robert Shearman
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

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#define USE_COM_CONTEXT_DEF
#include "objbase.h"
#include "shlguid.h"
#include "urlmon.h" /* for CLSID_FileProtocol */
#include "dde.h"
#include "cguid.h"

#include "ctxtcall.h"

#include "wine/test.h"
#include "winternl.h"
#include "initguid.h"

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE; static unsigned int called_ ## func = 0

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func++; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func, n) \
    do { \
        ok(called_ ## func == n, "expected " #func " called %u times, got %u\n", n, called_ ## func); \
        expect_ ## func = FALSE; \
        called_ ## func = 0; \
    }while(0)

DEFINE_EXPECT(CreateStub);
DEFINE_EXPECT(PreInitialize);
DEFINE_EXPECT(PostInitialize);
DEFINE_EXPECT(PreUninitialize);
DEFINE_EXPECT(PostUninitialize);

/* functions that are not present on all versions of Windows */
static HRESULT (WINAPI * pCoGetObjectContext)(REFIID riid, LPVOID *ppv);
static HRESULT (WINAPI * pCoSwitchCallContext)(IUnknown *pObject, IUnknown **ppOldObject);
static HRESULT (WINAPI * pCoGetContextToken)(ULONG_PTR *token);
static HRESULT (WINAPI * pCoGetApartmentType)(APTTYPE *type, APTTYPEQUALIFIER *qualifier);
static HRESULT (WINAPI * pCoIncrementMTAUsage)(CO_MTA_USAGE_COOKIE *cookie);
static HRESULT (WINAPI * pCoDecrementMTAUsage)(CO_MTA_USAGE_COOKIE cookie);
static LONG (WINAPI * pRegDeleteKeyExA)(HKEY, LPCSTR, REGSAM, DWORD);
static LONG (WINAPI * pRegOverridePredefKey)(HKEY key, HKEY override);
static HRESULT (WINAPI * pCoCreateInstanceFromApp)(REFCLSID clsid, IUnknown *outer, DWORD clscontext,
        void *reserved, DWORD count, MULTI_QI *results);

static BOOL   (WINAPI *pIsWow64Process)(HANDLE, LPBOOL);

#define ok_ole_success(hr, func) ok(hr == S_OK, func " failed with error %#08lx\n", hr)
#define ok_more_than_one_lock() ok(cLocks > 0, "Number of locks should be > 0, but actually is %ld\n", cLocks)
#define ok_no_locks() ok(cLocks == 0, "Number of locks should be 0, but actually is %ld\n", cLocks)

static const CLSID CLSID_non_existent =   { 0x12345678, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
static const CLSID CLSID_StdFont = { 0x0be35203, 0x8f91, 0x11ce, { 0x9d, 0xe3, 0x00, 0xaa, 0x00, 0x4b, 0xb8, 0x51 } };
static const GUID IID_Testiface = { 0x22222222, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
static const GUID IID_Testiface2 = { 0x32222222, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
static const GUID IID_Testiface3 = { 0x42222222, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
static const GUID IID_Testiface4 = { 0x52222222, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
static const GUID IID_Testiface5 = { 0x62222222, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
static const GUID IID_Testiface6 = { 0x72222222, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
static const GUID IID_Testiface7 = { 0x82222222, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
static const GUID IID_Testiface8 = { 0x92222222, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
static const GUID IID_TestPS = { 0x66666666, 0x8888, 0x7777, { 0x66, 0x66, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 } };

DEFINE_GUID(CLSID_testclsid, 0xacd014c7,0x9535,0x4fac,0x8b,0x53,0xa4,0x8c,0xa7,0xf4,0xd7,0x26);

static const WCHAR stdfont[] = {'S','t','d','F','o','n','t',0};
static const WCHAR wszNonExistent[] = {'N','o','n','E','x','i','s','t','e','n','t',0};
static const WCHAR wszCLSID_StdFont[] =
{
    '{','0','b','e','3','5','2','0','3','-','8','f','9','1','-','1','1','c','e','-',
    '9','d','e','3','-','0','0','a','a','0','0','4','b','b','8','5','1','}',0
};
static const WCHAR progidW[] = {'P','r','o','g','I','d','.','P','r','o','g','I','d',0};
static const WCHAR cf_brokenW[] = {'{','0','0','0','0','0','0','0','1','-','0','0','0','0','-','0','0','0','0','-',
                                    'c','0','0','0','-','0','0','0','0','0','0','0','0','0','0','4','6','}','a',0};

DEFINE_GUID(IID_IWineTest, 0x5201163f, 0x8164, 0x4fd0, 0xa1, 0xa2, 0x5d, 0x5a, 0x36, 0x54, 0xd3, 0xbd);
DEFINE_GUID(CLSID_WineOOPTest, 0x5201163f, 0x8164, 0x4fd0, 0xa1, 0xa2, 0x5d, 0x5a, 0x36, 0x54, 0xd3, 0xbd);

static LONG cLocks;

static void LockModule(void)
{
    InterlockedIncrement(&cLocks);
}

static void UnlockModule(void)
{
    InterlockedDecrement(&cLocks);
}

static HRESULT WINAPI Test_IClassFactory_QueryInterface(
    LPCLASSFACTORY iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IClassFactory))
    {
        *ppvObj = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Test_IClassFactory_AddRef(LPCLASSFACTORY iface)
{
    LockModule();
    return 2; /* non-heap-based object */
}

static ULONG WINAPI Test_IClassFactory_Release(LPCLASSFACTORY iface)
{
    UnlockModule();
    return 1; /* non-heap-based object */
}

static IID create_instance_iid;
static HRESULT WINAPI Test_IClassFactory_CreateInstance(
    LPCLASSFACTORY iface,
    IUnknown *pUnkOuter,
    REFIID riid,
    LPVOID *ppvObj)
{
    *ppvObj = NULL;
    create_instance_iid = *riid;
    if (pUnkOuter) return CLASS_E_NOAGGREGATION;
    return E_NOINTERFACE;
}

static HRESULT WINAPI Test_IClassFactory_LockServer(
    LPCLASSFACTORY iface,
    BOOL fLock)
{
    return S_OK;
}

static const IClassFactoryVtbl TestClassFactory_Vtbl =
{
    Test_IClassFactory_QueryInterface,
    Test_IClassFactory_AddRef,
    Test_IClassFactory_Release,
    Test_IClassFactory_CreateInstance,
    Test_IClassFactory_LockServer
};

static IClassFactory Test_ClassFactory = { &TestClassFactory_Vtbl };

static WCHAR manifest_path[MAX_PATH];

static BOOL create_manifest_file(const char *filename, const char *manifest)
{
    int manifest_len;
    DWORD size;
    HANDLE file;
    WCHAR path[MAX_PATH];

    MultiByteToWideChar( CP_ACP, 0, filename, -1, path, MAX_PATH );
    GetFullPathNameW(path, ARRAY_SIZE(manifest_path), manifest_path, NULL);

    manifest_len = strlen(manifest);
    file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed: %lu\n", GetLastError());
    if(file == INVALID_HANDLE_VALUE)
        return FALSE;
    WriteFile(file, manifest, manifest_len, &size, NULL);
    CloseHandle(file);

    return TRUE;
}

static void extract_resource(const char *name, const char *type, const char *path)
{
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    file = CreateFileA(path, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to create a file at %s, error %ld.\n", path, GetLastError());

    res = FindResourceA(NULL, name, type);
    ok(res != 0, "Failed to find resource.\n");
    ptr = LockResource(LoadResource(GetModuleHandleA(NULL), res));
    WriteFile(file, ptr, SizeofResource(GetModuleHandleA(NULL), res), &written, NULL);
    ok(written == SizeofResource(GetModuleHandleA(NULL), res), "Failed to write file.\n" );
    CloseHandle(file);
}

static char testlib[MAX_PATH];

static HANDLE activate_context(const char *manifest, ULONG_PTR *cookie)
{
    WCHAR path[MAX_PATH];
    ACTCTXW actctx;
    HANDLE handle;
    BOOL ret;

    create_manifest_file("file.manifest", manifest);

    MultiByteToWideChar( CP_ACP, 0, "file.manifest", -1, path, MAX_PATH );
    memset(&actctx, 0, sizeof(ACTCTXW));
    actctx.cbSize = sizeof(ACTCTXW);
    actctx.lpSource = path;

    handle = CreateActCtxW(&actctx);
    ok(handle != INVALID_HANDLE_VALUE || broken(handle == INVALID_HANDLE_VALUE) /* some old XP/2k3 versions */,
        "handle == INVALID_HANDLE_VALUE, error %lu\n", GetLastError());
    if (handle == INVALID_HANDLE_VALUE)
    {
        win_skip("activation context generation failed, some tests will be skipped. Error %ld\n", GetLastError());
        handle = NULL;
    }

    ok(actctx.cbSize == sizeof(ACTCTXW), "actctx.cbSize=%ld\n", actctx.cbSize);
    ok(actctx.dwFlags == 0, "actctx.dwFlags=%ld\n", actctx.dwFlags);
    ok(actctx.lpSource == path, "actctx.lpSource=%p\n", actctx.lpSource);
    ok(actctx.wProcessorArchitecture == 0, "actctx.wProcessorArchitecture=%d\n", actctx.wProcessorArchitecture);
    ok(actctx.wLangId == 0, "actctx.wLangId=%d\n", actctx.wLangId);
    ok(actctx.lpAssemblyDirectory == NULL, "actctx.lpAssemblyDirectory=%p\n", actctx.lpAssemblyDirectory);
    ok(actctx.lpResourceName == NULL, "actctx.lpResourceName=%p\n", actctx.lpResourceName);
    ok(actctx.lpApplicationName == NULL, "actctx.lpApplicationName=%p\n", actctx.lpApplicationName);
    ok(actctx.hModule == NULL, "actctx.hModule=%p\n", actctx.hModule);

    DeleteFileA("file.manifest");

    if (handle)
    {
        ret = ActivateActCtx(handle, cookie);
        ok(ret, "ActivateActCtx failed: %lu\n", GetLastError());
    }

    return handle;
}

static void deactivate_context(HANDLE handle, ULONG_PTR cookie)
{
    BOOL ret;

    ret = DeactivateActCtx(0, cookie);
    ok(ret, "Failed to deactivate context, error %ld.\n", GetLastError());
    ReleaseActCtx(handle);
}

static const char actctx_manifest[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.2.3.4\"  name=\"Wine.Test\" type=\"win32\""
" publicKeyToken=\"6595b6414666f1df\" />"
"<file name=\"testlib.dll\">"
"    <comClass"
"              clsid=\"{0000033a-0000-0000-c000-000000000046}\""
"              progid=\"FTMarshal\""
"    />"
"    <comClass"
"              clsid=\"{5201163f-8164-4fd0-a1a2-5d5a3654d3bd}\""
"              progid=\"WineOOPTest\""
"    />"
"    <comClass description=\"Test com class\""
"              clsid=\"{12345678-1234-1234-1234-56789abcdef0}\""
"              progid=\"ProgId.ProgId\""
"              miscStatusIcon=\"recomposeonresize\""
"    />"
"    <comClass description=\"CustomFont Description\" clsid=\"{0be35203-8f91-11ce-9de3-00aa004bb851}\""
"              progid=\"CustomFont\""
"              miscStatusIcon=\"recomposeonresize\""
"              miscStatusContent=\"insideout\""
"    />"
"    <comClass description=\"StdFont Description\" clsid=\"{0be35203-8f91-11ce-9de3-00aa004bb852}\""
"              progid=\"StdFont\""
"    />"
"    <comClass clsid=\"{62222222-1234-1234-1234-56789abcdef0}\" >"
"        <progid>ProgId.ProgId.1</progid>"
"    </comClass>"
"    <comInterfaceProxyStub "
"        name=\"Iifaceps\""
"        iid=\"{22222222-1234-1234-1234-56789abcdef0}\""
"        proxyStubClsid32=\"{66666666-8888-7777-6666-555555555555}\""
"    />"
"    <comInterfaceProxyStub "
"        name=\"Iifaceps5\""
"        iid=\"{82222222-1234-1234-1234-56789abcdef0}\""
"    />"
"</file>"
"    <comInterfaceExternalProxyStub "
"        name=\"Iifaceps2\""
"        iid=\"{32222222-1234-1234-1234-56789abcdef0}\""
"    />"
"    <comInterfaceExternalProxyStub "
"        name=\"Iifaceps3\""
"        iid=\"{42222222-1234-1234-1234-56789abcdef0}\""
"        proxyStubClsid32=\"{66666666-8888-7777-6666-555555555555}\""
"    />"
"    <comInterfaceExternalProxyStub "
"        name=\"Iifaceps4\""
"        iid=\"{52222222-1234-1234-1234-56789abcdef0}\""
"        proxyStubClsid32=\"{00000000-0000-0000-0000-000000000000}\""
"    />"
"    <clrClass "
"        clsid=\"{72222222-1234-1234-1234-56789abcdef0}\""
"        name=\"clrclass\""
"    >"
"        <progid>clrprogid.1</progid>"
"    </clrClass>"
"</assembly>";

static const char actctx_manifest2[] =
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"<assemblyIdentity version=\"1.2.3.4\"  name=\"Wine.Test\" type=\"win32\""
" publicKeyToken=\"6595b6414666f1df\" />"
"<file name=\"testlib.dll\">"
"    <comInterfaceProxyStub "
"        name=\"Testiface7\""
"        iid=\"{52222222-1234-1234-1234-56789abcdef0}\""
"        proxyStubClsid32=\"{82222222-1234-1234-1234-56789abcdef0}\""
"        threadingModel=\"Apartment\""
"    />"
"</file>"
"<file name=\"testlib4.dll\">"
"    <comInterfaceProxyStub "
"        name=\"Testiface8\""
"        iid=\"{92222222-1234-1234-1234-56789abcdef0}\""
"        threadingModel=\"Apartment\""
"    />"
"</file>"
"    <comInterfaceExternalProxyStub "
"        name=\"Iifaceps3\""
"        iid=\"{42222222-1234-1234-1234-56789abcdef0}\""
"        proxyStubClsid32=\"{66666666-8888-7777-6666-555555555555}\""
"    />"
"</assembly>";

DEFINE_GUID(CLSID_Testclass, 0x12345678, 0x1234, 0x1234, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0);

static void test_ProgIDFromCLSID(void)
{
    ULONG_PTR cookie = 0;
    LPWSTR progid;
    HANDLE handle;
    HRESULT hr;

    hr = ProgIDFromCLSID(&CLSID_StdFont, &progid);
    ok(hr == S_OK, "ProgIDFromCLSID failed with error 0x%08lx\n", hr);
    if (hr == S_OK)
    {
        ok(!lstrcmpiW(progid, stdfont), "Didn't get expected prog ID\n");
        CoTaskMemFree(progid);
    }

    progid = (LPWSTR)0xdeadbeef;
    hr = ProgIDFromCLSID(&CLSID_non_existent, &progid);
    ok(hr == REGDB_E_CLASSNOTREG, "ProgIDFromCLSID returned %08lx\n", hr);
    ok(progid == NULL, "ProgIDFromCLSID returns with progid %p\n", progid);

    hr = ProgIDFromCLSID(&CLSID_StdFont, NULL);
    ok(hr == E_INVALIDARG, "ProgIDFromCLSID should return E_INVALIDARG instead of 0x%08lx\n", hr);

    if ((handle = activate_context(actctx_manifest, &cookie)))
    {
        static const WCHAR customfontW[] = {'C','u','s','t','o','m','F','o','n','t',0};

        hr = ProgIDFromCLSID(&CLSID_non_existent, &progid);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(!lstrcmpiW(progid, progidW), "got %s\n", wine_dbgstr_w(progid));
        CoTaskMemFree(progid);

        /* try something registered and redirected */
        progid = NULL;
        hr = ProgIDFromCLSID(&CLSID_StdFont, &progid);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(!lstrcmpiW(progid, customfontW), "got wrong progid %s\n", wine_dbgstr_w(progid));
        CoTaskMemFree(progid);

        /* classes without default progid, progid list is not used */
        progid = (void *)0xdeadbeef;
        hr = ProgIDFromCLSID(&IID_Testiface5, &progid);
        ok(hr == REGDB_E_CLASSNOTREG && progid == NULL, "got 0x%08lx, progid %p\n", hr, progid);

        progid = (void *)0xdeadbeef;
        hr = ProgIDFromCLSID(&IID_Testiface6, &progid);
        ok(hr == REGDB_E_CLASSNOTREG && progid == NULL, "got 0x%08lx, progid %p\n", hr, progid);

        deactivate_context(handle, cookie);
    }
}

static void test_CLSIDFromProgID(void)
{
    ULONG_PTR cookie = 0;
    HANDLE handle;
    CLSID clsid;
    HRESULT hr = CLSIDFromProgID(stdfont, &clsid);
    ok(hr == S_OK, "CLSIDFromProgID failed with error 0x%08lx\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_StdFont), "clsid wasn't equal to CLSID_StdFont\n");

    hr = CLSIDFromString(stdfont, &clsid);
    ok_ole_success(hr, "CLSIDFromString");
    ok(IsEqualCLSID(&clsid, &CLSID_StdFont), "clsid wasn't equal to CLSID_StdFont\n");

    /* test some failure cases */

    hr = CLSIDFromProgID(wszNonExistent, NULL);
    ok(hr == E_INVALIDARG, "CLSIDFromProgID should have returned E_INVALIDARG instead of 0x%08lx\n", hr);

    hr = CLSIDFromProgID(NULL, &clsid);
    ok(hr == E_INVALIDARG, "CLSIDFromProgID should have returned E_INVALIDARG instead of 0x%08lx\n", hr);

    memset(&clsid, 0xcc, sizeof(clsid));
    hr = CLSIDFromProgID(wszNonExistent, &clsid);
    ok(hr == CO_E_CLASSSTRING, "CLSIDFromProgID on nonexistent ProgID should have returned CO_E_CLASSSTRING instead of 0x%08lx\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_NULL), "CLSIDFromProgID should have set clsid to all-zeros on failure\n");

    /* fails without proper context */
    memset(&clsid, 0xcc, sizeof(clsid));
    hr = CLSIDFromProgID(progidW, &clsid);
    ok(hr == CO_E_CLASSSTRING, "got 0x%08lx\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_NULL), "wrong clsid\n");

    if ((handle = activate_context(actctx_manifest, &cookie)))
    {
        GUID clsid1;

        memset(&clsid, 0xcc, sizeof(clsid));
        hr = CLSIDFromProgID(wszNonExistent, &clsid);
        ok(hr == CO_E_CLASSSTRING, "got 0x%08lx\n", hr);
        ok(IsEqualCLSID(&clsid, &CLSID_NULL), "should have zero CLSID on failure\n");

        /* CLSIDFromString() doesn't check activation context */
        hr = CLSIDFromString(progidW, &clsid);
        ok(hr == CO_E_CLASSSTRING, "got 0x%08lx\n", hr);

        clsid = CLSID_NULL;
        hr = CLSIDFromProgID(progidW, &clsid);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        /* it returns generated CLSID here */
        ok(!IsEqualCLSID(&clsid, &CLSID_non_existent) && !IsEqualCLSID(&clsid, &CLSID_NULL),
                 "got wrong clsid %s\n", wine_dbgstr_guid(&clsid));

        /* duplicate progid present in context - returns generated guid here too */
        clsid = CLSID_NULL;
        hr = CLSIDFromProgID(stdfont, &clsid);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        clsid1 = CLSID_StdFont;
        /* that's where it differs from StdFont */
        clsid1.Data4[7] = 0x52;
        ok(!IsEqualCLSID(&clsid, &CLSID_StdFont) && !IsEqualCLSID(&clsid, &CLSID_NULL) && !IsEqualCLSID(&clsid, &clsid1),
            "got %s\n", wine_dbgstr_guid(&clsid));

        deactivate_context(handle, cookie);
    }
}

static void test_CLSIDFromString(void)
{
    CLSID clsid;
    WCHAR wszCLSID_Broken[50];
    UINT i;

    HRESULT hr = CLSIDFromString(wszCLSID_StdFont, &clsid);
    ok_ole_success(hr, "CLSIDFromString");
    ok(IsEqualCLSID(&clsid, &CLSID_StdFont), "clsid wasn't equal to CLSID_StdFont\n");

    memset(&clsid, 0xab, sizeof(clsid));
    hr = CLSIDFromString(NULL, &clsid);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_NULL), "clsid wasn't equal to CLSID_NULL\n");

    /* string is longer, but starts with a valid CLSID */
    memset(&clsid, 0, sizeof(clsid));
    hr = CLSIDFromString(cf_brokenW, &clsid);
    ok(hr == CO_E_CLASSSTRING, "got 0x%08lx\n", hr);
    ok(IsEqualCLSID(&clsid, &IID_IClassFactory), "got %s\n", wine_dbgstr_guid(&clsid));

    lstrcpyW(wszCLSID_Broken, wszCLSID_StdFont);
    for(i = lstrlenW(wszCLSID_StdFont); i < 49; i++)
        wszCLSID_Broken[i] = 'A';
    wszCLSID_Broken[i] = '\0';

    memset(&clsid, 0, sizeof(CLSID));
    hr = CLSIDFromString(wszCLSID_Broken, &clsid);
    ok(hr == CO_E_CLASSSTRING, "Got %08lx\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_StdFont), "clsid wasn't equal to CLSID_StdFont\n");

    wszCLSID_Broken[lstrlenW(wszCLSID_StdFont)-1] = 'A';
    memset(&clsid, 0, sizeof(CLSID));
    hr = CLSIDFromString(wszCLSID_Broken, &clsid);
    ok(hr == CO_E_CLASSSTRING, "Got %08lx\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_StdFont), "clsid wasn't equal to CLSID_StdFont\n");

    wszCLSID_Broken[lstrlenW(wszCLSID_StdFont)] = '\0';
    memset(&clsid, 0, sizeof(CLSID));
    hr = CLSIDFromString(wszCLSID_Broken, &clsid);
    ok(hr == CO_E_CLASSSTRING, "Got %08lx\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_StdFont), "clsid wasn't equal to CLSID_StdFont\n");

    wszCLSID_Broken[lstrlenW(wszCLSID_StdFont)-1] = '\0';
    memset(&clsid, 0, sizeof(CLSID));
    hr = CLSIDFromString(wszCLSID_Broken, &clsid);
    ok(hr == CO_E_CLASSSTRING, "Got %08lx\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_StdFont), "clsid wasn't equal to CLSID_StdFont\n");

    memset(&clsid, 0xcc, sizeof(CLSID));
    hr = CLSIDFromString(wszCLSID_Broken+1, &clsid);
    ok(hr == CO_E_CLASSSTRING, "Got %08lx\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_NULL), "clsid wasn't equal to CLSID_NULL\n");

    wszCLSID_Broken[9] = '*';
    memset(&clsid, 0xcc, sizeof(CLSID));
    hr = CLSIDFromString(wszCLSID_Broken, &clsid);
    ok(hr == CO_E_CLASSSTRING, "Got %08lx\n", hr);
    ok(clsid.Data1 == CLSID_StdFont.Data1, "Got %s\n", debugstr_guid(&clsid));
    ok(clsid.Data2 == 0xcccc, "Got %04x\n", clsid.Data2);

    wszCLSID_Broken[3] = '*';
    memset(&clsid, 0xcc, sizeof(CLSID));
    hr = CLSIDFromString(wszCLSID_Broken, &clsid);
    ok(hr == CO_E_CLASSSTRING, "Got %08lx\n", hr);
    ok(clsid.Data1 == 0xb, "Got %s\n", debugstr_guid(&clsid));
    ok(clsid.Data2 == 0xcccc, "Got %04x\n", clsid.Data2);

    wszCLSID_Broken[3] = '\0';
    memset(&clsid, 0xcc, sizeof(CLSID));
    hr = CLSIDFromString(wszCLSID_Broken, &clsid);
    ok(hr == CO_E_CLASSSTRING, "Got %08lx\n", hr);
    ok(clsid.Data1 == 0xb, "Got %s\n", debugstr_guid(&clsid));
    ok(clsid.Data2 == 0xcccc, "Got %04x\n", clsid.Data2);
}

static void test_IIDFromString(void)
{
    static const WCHAR cfW[] = {'{','0','0','0','0','0','0','0','1','-','0','0','0','0','-','0','0','0','0','-',
                                    'c','0','0','0','-','0','0','0','0','0','0','0','0','0','0','4','6','}',0};
    static const WCHAR brokenW[] = {'{','0','0','0','0','0','0','0','1','-','0','0','0','0','-','0','0','0','0','-',
                                        'g','0','0','0','-','0','0','0','0','0','0','0','0','0','0','4','6','}',0};
    static const WCHAR broken2W[] = {'{','0','0','0','0','0','0','0','1','=','0','0','0','0','-','0','0','0','0','-',
                                        'g','0','0','0','-','0','0','0','0','0','0','0','0','0','0','4','6','}',0};
    static const WCHAR broken3W[] = {'b','r','o','k','e','n','0','0','1','=','0','0','0','0','-','0','0','0','0','-',
                                        'g','0','0','0','-','0','0','0','0','0','0','0','0','0','0','4','6','}',0};
    HRESULT hr;
    IID iid;

    hr = IIDFromString(wszCLSID_StdFont, &iid);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(IsEqualIID(&iid, &CLSID_StdFont), "got iid %s\n", wine_dbgstr_guid(&iid));

    memset(&iid, 0xab, sizeof(iid));
    hr = IIDFromString(NULL, &iid);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(IsEqualIID(&iid, &CLSID_NULL), "got iid %s\n", wine_dbgstr_guid(&iid));

    hr = IIDFromString(cfW, &iid);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(IsEqualIID(&iid, &IID_IClassFactory), "got iid %s\n", wine_dbgstr_guid(&iid));

    /* string starts with a valid IID but is longer */
    memset(&iid, 0xab, sizeof(iid));
    hr = IIDFromString(cf_brokenW, &iid);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
    ok(iid.Data1 == 0xabababab, "Got %s\n", debugstr_guid(&iid));

    /* invalid IID in a valid format */
    memset(&iid, 0xab, sizeof(iid));
    hr = IIDFromString(brokenW, &iid);
    ok(hr == CO_E_IIDSTRING, "got 0x%08lx\n", hr);
    ok(iid.Data1 == 0x00000001, "Got %s\n", debugstr_guid(&iid));

    memset(&iid, 0xab, sizeof(iid));
    hr = IIDFromString(broken2W, &iid);
    ok(hr == CO_E_IIDSTRING, "got 0x%08lx\n", hr);
    ok(iid.Data1 == 0x00000001, "Got %s\n", debugstr_guid(&iid));

    /* format is broken, but string length is okay */
    memset(&iid, 0xab, sizeof(iid));
    hr = IIDFromString(broken3W, &iid);
    ok(hr == CO_E_IIDSTRING, "got 0x%08lx\n", hr);
    ok(iid.Data1 == 0xabababab, "Got %s\n", debugstr_guid(&iid));

    /* invalid string */
    memset(&iid, 0xab, sizeof(iid));
    hr = IIDFromString(wszNonExistent, &iid);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
    ok(iid.Data1 == 0xabababab, "Got %s\n", debugstr_guid(&iid));

    /* valid ProgID */
    memset(&iid, 0xab, sizeof(iid));
    hr = IIDFromString(stdfont, &iid);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
    ok(iid.Data1 == 0xabababab, "Got %s\n", debugstr_guid(&iid));
}

static void test_StringFromGUID2(void)
{
  WCHAR str[50];
  int len;

  /* invalid pointer */
  SetLastError(0xdeadbeef);
  len = StringFromGUID2(NULL,str,50);
  ok(len == 0, "len: %d (expected 0)\n", len);
  ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %lx\n", GetLastError());

  /* Test corner cases for buffer size */
  len = StringFromGUID2(&CLSID_StdFont,str,50);
  ok(len == 39, "len: %d (expected 39)\n", len);
  ok(!lstrcmpiW(str, wszCLSID_StdFont),"string wasn't equal for CLSID_StdFont\n");

  memset(str,0,sizeof str);
  len = StringFromGUID2(&CLSID_StdFont,str,39);
  ok(len == 39, "len: %d (expected 39)\n", len);
  ok(!lstrcmpiW(str, wszCLSID_StdFont),"string wasn't equal for CLSID_StdFont\n");

  len = StringFromGUID2(&CLSID_StdFont,str,38);
  ok(len == 0, "len: %d (expected 0)\n", len);

  len = StringFromGUID2(&CLSID_StdFont,str,30);
  ok(len == 0, "len: %d (expected 0)\n", len);
}

#define test_apt_type(t, q) _test_apt_type(t, q, __LINE__)
static void _test_apt_type(APTTYPE expected_type, APTTYPEQUALIFIER expected_qualifier, int line)
{
    APTTYPEQUALIFIER qualifier = ~0u;
    APTTYPE type = ~0u;
    HRESULT hr;

    if (!pCoGetApartmentType)
        return;

    hr = pCoGetApartmentType(&type, &qualifier);
    ok_(__FILE__, line)(hr == S_OK || (type == APTTYPE_CURRENT && hr == CO_E_NOTINITIALIZED),
            "Unexpected hr %#lx.\n", hr);
    ok_(__FILE__, line)(type == expected_type, "Wrong apartment type %d, expected %d\n", type, expected_type);
    ok_(__FILE__, line)(qualifier == expected_qualifier, "Wrong apartment qualifier %d, expected %d\n", qualifier,
        expected_qualifier);
}

static void test_CoCreateInstance(void)
{
    HRESULT hr;
    IUnknown *pUnk;
    REFCLSID rclsid = &CLSID_InternetZoneManager;

    pUnk = (IUnknown *)0xdeadbeef;
    hr = CoCreateInstance(rclsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&pUnk);
    ok(hr == CO_E_NOTINITIALIZED, "CoCreateInstance should have returned CO_E_NOTINITIALIZED instead of 0x%08lx\n", hr);
    ok(pUnk == NULL, "CoCreateInstance should have changed the passed in pointer to NULL, instead of %p\n", pUnk);

    OleInitialize(NULL);

    /* test errors returned for non-registered clsids */
    hr = CoCreateInstance(&CLSID_non_existent, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&pUnk);
    ok(hr == REGDB_E_CLASSNOTREG, "CoCreateInstance for non-registered inproc server should have returned REGDB_E_CLASSNOTREG instead of 0x%08lx\n", hr);
    hr = CoCreateInstance(&CLSID_non_existent, NULL, CLSCTX_INPROC_HANDLER, &IID_IUnknown, (void **)&pUnk);
    ok(hr == REGDB_E_CLASSNOTREG, "CoCreateInstance for non-registered inproc handler should have returned REGDB_E_CLASSNOTREG instead of 0x%08lx\n", hr);
    hr = CoCreateInstance(&CLSID_non_existent, NULL, CLSCTX_LOCAL_SERVER, &IID_IUnknown, (void **)&pUnk);
    ok(hr == REGDB_E_CLASSNOTREG, "CoCreateInstance for non-registered local server should have returned REGDB_E_CLASSNOTREG instead of 0x%08lx\n", hr);
    hr = CoCreateInstance(&CLSID_non_existent, NULL, CLSCTX_REMOTE_SERVER, &IID_IUnknown, (void **)&pUnk);
    ok(hr == REGDB_E_CLASSNOTREG, "CoCreateInstance for non-registered remote server should have returned REGDB_E_CLASSNOTREG instead of 0x%08lx\n", hr);

    hr = CoCreateInstance(rclsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&pUnk);
    if(hr == REGDB_E_CLASSNOTREG)
    {
        skip("IE not installed so can't test CoCreateInstance\n");
        OleUninitialize();
        return;
    }

    ok_ole_success(hr, "CoCreateInstance");
    if(pUnk) IUnknown_Release(pUnk);
    OleUninitialize();

    hr = CoCreateInstance(rclsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&pUnk);
    ok(hr == CO_E_NOTINITIALIZED, "CoCreateInstance should have returned CO_E_NOTINITIALIZED instead of 0x%08lx\n", hr);

    test_apt_type(APTTYPE_CURRENT, APTTYPEQUALIFIER_NONE);
}

struct comclassredirect_data
{
    ULONG size;
    ULONG flags;
    DWORD model;
    GUID  clsid;
    GUID  alias;
    GUID  clsid2;
    GUID  tlid;
    ULONG name_len;
    ULONG name_offset;
    ULONG progid_len;
    ULONG progid_offset;
    ULONG clrdata_len;
    ULONG clrdata_offset;
    DWORD miscstatus;
    DWORD miscstatuscontent;
    DWORD miscstatusthumbnail;
    DWORD miscstatusicon;
    DWORD miscstatusdocprint;
};

static void test_CoGetClassObject(void)
{
    HRESULT hr;
    HANDLE handle;
    ULONG_PTR cookie;
    IUnknown *pUnk;
    REFCLSID rclsid = &CLSID_InternetZoneManager;
    HKEY hkey;
    LONG res;

    hr = CoGetClassObject(rclsid, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void **)&pUnk);
    ok(hr == CO_E_NOTINITIALIZED, "CoGetClassObject should have returned CO_E_NOTINITIALIZED instead of 0x%08lx\n", hr);
    ok(pUnk == NULL, "CoGetClassObject should have changed the passed in pointer to NULL, instead of %p\n", pUnk);

    hr = CoGetClassObject(rclsid, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, NULL);
    ok(hr == E_INVALIDARG ||
       broken(hr == CO_E_NOTINITIALIZED), /* win9x */
       "CoGetClassObject should have returned E_INVALIDARG instead of 0x%08lx\n", hr);

    test_apt_type(APTTYPE_CURRENT, APTTYPEQUALIFIER_NONE);

    if (!pRegOverridePredefKey)
    {
        win_skip("RegOverridePredefKey not available\n");
        return;
    }

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CoGetClassObject(rclsid, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void **)&pUnk);
    if (hr == S_OK)
    {
        IUnknown_Release(pUnk);

        res = RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Classes", 0, NULL, 0,
                             KEY_ALL_ACCESS, NULL, &hkey, NULL);
        ok(!res, "RegCreateKeyEx returned %ld\n", res);

        res = pRegOverridePredefKey(HKEY_CLASSES_ROOT, hkey);
        ok(!res, "RegOverridePredefKey returned %ld\n", res);

        hr = CoGetClassObject(rclsid, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void **)&pUnk);
        ok(hr == S_OK, "CoGetClassObject should have returned S_OK instead of 0x%08lx\n", hr);

        res = pRegOverridePredefKey(HKEY_CLASSES_ROOT, NULL);
        ok(!res, "RegOverridePredefKey returned %ld\n", res);

        if (hr == S_OK) IUnknown_Release(pUnk);
        RegCloseKey(hkey);
    }

    hr = CoGetClassObject(&CLSID_InProcFreeMarshaler, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void **)&pUnk);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    IUnknown_Release(pUnk);

    /* context redefines FreeMarshaler CLSID */
    if ((handle = activate_context(actctx_manifest, &cookie)))
    {
        hr = CoGetClassObject(&CLSID_InProcFreeMarshaler, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void **)&pUnk);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        IUnknown_Release(pUnk);

        hr = CoGetClassObject(&IID_Testiface7, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void **)&pUnk);
        ok(hr == 0x80001235 || broken(hr == HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND)) /* winxp */, "Unexpected hr %#lx.\n", hr);

        hr = CoGetClassObject(&IID_Testiface8, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void **)&pUnk);
        ok(hr == REGDB_E_CLASSNOTREG, "Unexpected hr %#lx.\n", hr);

        deactivate_context(handle, cookie);
    }

    if ((handle = activate_context(actctx_manifest2, &cookie)))
    {
        struct comclassredirect_data *comclass;
        ACTCTX_SECTION_KEYED_DATA data;
        BOOL ret;

        /* This one will load test dll and get back specific error code. */
        hr = CoGetClassObject(&IID_Testiface7, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void **)&pUnk);
        ok(hr == 0x80001235 || broken(hr == HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND)) /* winxp */, "Unexpected hr %#lx.\n", hr);

        hr = CoGetClassObject(&IID_Testiface8, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void **)&pUnk);
        ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

        memset(&data, 0, sizeof(data));
        data.cbSize = sizeof(data);
        ret = FindActCtxSectionGuid(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION, &IID_Testiface8, &data);
        ok(ret, "Section not found.\n");

        memset(&data, 0, sizeof(data));
        data.cbSize = sizeof(data);

        /* External proxy-stubs are not accessible. */
        ret = FindActCtxSectionGuid(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION, &IID_Testiface3, &data);
        ok(!ret, "Unexpected return value.\n");

        ret = FindActCtxSectionGuid(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION, &IID_TestPS, &data);
        ok(!ret, "Unexpected return value.\n");

        ret = FindActCtxSectionGuid(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION, &IID_Testiface7, &data);
        ok(ret, "Unexpected return value.\n");

        ret = FindActCtxSectionGuid(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION, &IID_Testiface4, &data);
        ok(!ret, "Unexpected return value.\n");

        ret = FindActCtxSectionGuid(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION, &IID_Testiface8, &data);
        ok(ret, "Unexpected return value.\n");

        comclass = data.lpData;
        if (comclass)
        {
            WCHAR *name = (WCHAR *)((char *)data.lpSectionBase + comclass->name_offset);
            ok(!lstrcmpW(name, L"testlib4.dll"), "Unexpected module name %s.\n", wine_dbgstr_w(name));
        }

        deactivate_context(handle, cookie);
    }

    CoUninitialize();
}

static void test_CoCreateInstanceEx(void)
{
    MULTI_QI qi_res = { &IID_IMoniker };
    DWORD cookie;
    HRESULT hr;

    CoInitialize(NULL);

    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");

    create_instance_iid = IID_NULL;
    hr = CoCreateInstanceEx(&CLSID_WineOOPTest, NULL, CLSCTX_INPROC_SERVER, NULL, 1, &qi_res);
    ok(hr == E_NOINTERFACE, "CoCreateInstanceEx failed: %08lx\n", hr);
    ok(IsEqualGUID(&create_instance_iid, qi_res.pIID), "Unexpected CreateInstance iid %s\n",
       wine_dbgstr_guid(&create_instance_iid));

    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    CoUninitialize();
}

static ATOM register_dummy_class(void)
{
    WNDCLASSA wc =
    {
        0,
        DefWindowProcA,
        0,
        0,
        GetModuleHandleA(NULL),
        NULL,
        LoadCursorA(NULL, (LPSTR)IDC_ARROW),
        (HBRUSH)(COLOR_BTNFACE+1),
        NULL,
        "WineOleTestClass",
    };

    return RegisterClassA(&wc);
}

static void test_ole_menu(void)
{
	HWND hwndFrame;
	HRESULT hr;

	hwndFrame = CreateWindowA((LPCSTR)MAKEINTATOM(register_dummy_class()), "Test", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);
	hr = OleSetMenuDescriptor(NULL, hwndFrame, NULL, NULL, NULL);
	todo_wine ok_ole_success(hr, "OleSetMenuDescriptor");

	DestroyWindow(hwndFrame);
}


static HRESULT WINAPI MessageFilter_QueryInterface(IMessageFilter *iface, REFIID riid, void ** ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IMessageFilter))
    {
        *ppvObj = iface;
        IMessageFilter_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI MessageFilter_AddRef(IMessageFilter *iface)
{
    return 2; /* non-heap object */
}

static ULONG WINAPI MessageFilter_Release(IMessageFilter *iface)
{
    return 1; /* non-heap object */
}

static DWORD WINAPI MessageFilter_HandleInComingCall(
  IMessageFilter *iface,
  DWORD dwCallType,
  HTASK threadIDCaller,
  DWORD dwTickCount,
  LPINTERFACEINFO lpInterfaceInfo)
{
    trace("HandleInComingCall\n");
    return SERVERCALL_ISHANDLED;
}

static DWORD WINAPI MessageFilter_RetryRejectedCall(
  IMessageFilter *iface,
  HTASK threadIDCallee,
  DWORD dwTickCount,
  DWORD dwRejectType)
{
    trace("RetryRejectedCall\n");
    return 0;
}

static DWORD WINAPI MessageFilter_MessagePending(
  IMessageFilter *iface,
  HTASK threadIDCallee,
  DWORD dwTickCount,
  DWORD dwPendingType)
{
    trace("MessagePending\n");
    todo_wine ok(0, "unexpected call\n");
    return PENDINGMSG_WAITNOPROCESS;
}

static const IMessageFilterVtbl MessageFilter_Vtbl =
{
    MessageFilter_QueryInterface,
    MessageFilter_AddRef,
    MessageFilter_Release,
    MessageFilter_HandleInComingCall,
    MessageFilter_RetryRejectedCall,
    MessageFilter_MessagePending
};

static IMessageFilter MessageFilter = { &MessageFilter_Vtbl };

static void test_CoRegisterMessageFilter(void)
{
    HRESULT hr;
    IMessageFilter *prev_filter;

    hr = CoRegisterMessageFilter(&MessageFilter, &prev_filter);
    ok(hr == CO_E_NOT_SUPPORTED,
        "CoRegisterMessageFilter should have failed with CO_E_NOT_SUPPORTED instead of 0x%08lx\n",
        hr);

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    prev_filter = (IMessageFilter *)0xdeadbeef;
    hr = CoRegisterMessageFilter(&MessageFilter, &prev_filter);
    ok(hr == CO_E_NOT_SUPPORTED,
        "CoRegisterMessageFilter should have failed with CO_E_NOT_SUPPORTED instead of 0x%08lx\n",
        hr);
    ok(prev_filter == (IMessageFilter *)0xdeadbeef,
        "prev_filter should have been set to %p\n", prev_filter);
    CoUninitialize();

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoRegisterMessageFilter(NULL, NULL);
    ok_ole_success(hr, "CoRegisterMessageFilter");

    prev_filter = (IMessageFilter *)0xdeadbeef;
    hr = CoRegisterMessageFilter(NULL, &prev_filter);
    ok_ole_success(hr, "CoRegisterMessageFilter");
    ok(prev_filter == NULL, "prev_filter should have been set to NULL instead of %p\n", prev_filter);

    hr = CoRegisterMessageFilter(&MessageFilter, &prev_filter);
    ok_ole_success(hr, "CoRegisterMessageFilter");
    ok(prev_filter == NULL, "prev_filter should have been set to NULL instead of %p\n", prev_filter);

    hr = CoRegisterMessageFilter(NULL, NULL);
    ok_ole_success(hr, "CoRegisterMessageFilter");

    CoUninitialize();
}

static IUnknown Test_Unknown;

static HRESULT WINAPI EnumOLEVERB_QueryInterface(IEnumOLEVERB *iface, REFIID riid, void **ppv)
{
    return IUnknown_QueryInterface(&Test_Unknown, riid, ppv);
}

static ULONG WINAPI EnumOLEVERB_AddRef(IEnumOLEVERB *iface)
{
    return 2;
}

static ULONG WINAPI EnumOLEVERB_Release(IEnumOLEVERB *iface)
{
    return 1;
}

static HRESULT WINAPI EnumOLEVERB_Next(IEnumOLEVERB *iface, ULONG celt, OLEVERB *rgelt, ULONG *fetched)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumOLEVERB_Skip(IEnumOLEVERB *iface, ULONG celt)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumOLEVERB_Reset(IEnumOLEVERB *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumOLEVERB_Clone(IEnumOLEVERB *iface, IEnumOLEVERB **ppenum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IEnumOLEVERBVtbl EnumOLEVERBVtbl = {
    EnumOLEVERB_QueryInterface,
    EnumOLEVERB_AddRef,
    EnumOLEVERB_Release,
    EnumOLEVERB_Next,
    EnumOLEVERB_Skip,
    EnumOLEVERB_Reset,
    EnumOLEVERB_Clone
};

static IEnumOLEVERB EnumOLEVERB = { &EnumOLEVERBVtbl };

static HRESULT WINAPI Test_IUnknown_QueryInterface(
    IUnknown *iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IWineTest)) {
        *ppvObj = iface;
    }else if(IsEqualIID(riid, &IID_IEnumOLEVERB)) {
        *ppvObj = &EnumOLEVERB;
    }else {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppvObj);
    return S_OK;
}

static ULONG WINAPI Test_IUnknown_AddRef(IUnknown *iface)
{
    return 2; /* non-heap-based object */
}

static ULONG WINAPI Test_IUnknown_Release(IUnknown *iface)
{
    return 1; /* non-heap-based object */
}

static const IUnknownVtbl TestUnknown_Vtbl =
{
    Test_IUnknown_QueryInterface,
    Test_IUnknown_AddRef,
    Test_IUnknown_Release,
};

static IUnknown Test_Unknown = { &TestUnknown_Vtbl };

static IPSFactoryBuffer *ps_factory_buffer;

static HRESULT WINAPI PSFactoryBuffer_QueryInterface(
    IPSFactoryBuffer * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppvObject)
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IPSFactoryBuffer))
    {
        *ppvObject = This;
        IPSFactoryBuffer_AddRef(This);
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI PSFactoryBuffer_AddRef(
    IPSFactoryBuffer * This)
{
    return 2;
}

static ULONG WINAPI PSFactoryBuffer_Release(
    IPSFactoryBuffer * This)
{
    return 1;
}

static HRESULT WINAPI PSFactoryBuffer_CreateProxy(
    IPSFactoryBuffer * This,
    /* [in] */ IUnknown *pUnkOuter,
    /* [in] */ REFIID riid,
    /* [out] */ IRpcProxyBuffer **ppProxy,
    /* [out] */ void **ppv)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI PSFactoryBuffer_CreateStub(
    IPSFactoryBuffer * This,
    /* [in] */ REFIID riid,
    /* [unique][in] */ IUnknown *pUnkServer,
    /* [out] */ IRpcStubBuffer **ppStub)
{
    CHECK_EXPECT(CreateStub);

    ok(pUnkServer == &Test_Unknown, "unexpected pUnkServer %p\n", pUnkServer);
    if(!ps_factory_buffer)
        return E_NOTIMPL;

    return IPSFactoryBuffer_CreateStub(ps_factory_buffer, &IID_IEnumOLEVERB, pUnkServer, ppStub);
}

static IPSFactoryBufferVtbl PSFactoryBufferVtbl =
{
    PSFactoryBuffer_QueryInterface,
    PSFactoryBuffer_AddRef,
    PSFactoryBuffer_Release,
    PSFactoryBuffer_CreateProxy,
    PSFactoryBuffer_CreateStub
};

static IPSFactoryBuffer PSFactoryBuffer = { &PSFactoryBufferVtbl };

static const CLSID CLSID_WineTestPSFactoryBuffer =
{
    0x52011640,
    0x8164,
    0x4fd0,
    {0xa1, 0xa2, 0x5d, 0x5a, 0x36, 0x54, 0xd3, 0xbd}
}; /* 52011640-8164-4fd0-a1a2-5d5a3654d3bd */

static DWORD CALLBACK register_ps_clsid_thread(void *context)
{
    HRESULT hr;
    CLSID clsid = {0};

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoGetPSClsid(&IID_IWineTest, &clsid);
    ok_ole_success(hr, "CoGetPSClsid");
    ok(IsEqualGUID(&clsid, &CLSID_WineTestPSFactoryBuffer), "expected %s, got %s\n",
                   wine_dbgstr_guid(&CLSID_WineTestPSFactoryBuffer), wine_dbgstr_guid(&clsid));

    /* test registering a PSClsid in an apartment which is then destroyed */
    hr = CoRegisterPSClsid(&IID_TestPS, &clsid);
    ok_ole_success(hr, "CoRegisterPSClsid");

    CoUninitialize();

    return hr;
}

static void test_CoRegisterPSClsid(void)
{
    HRESULT hr;
    DWORD dwRegistrationKey;
    IStream *stream;
    CLSID clsid;
    HANDLE thread;
    DWORD tid;

    hr = CoRegisterPSClsid(&IID_IWineTest, &CLSID_WineTestPSFactoryBuffer);
    ok(hr == CO_E_NOTINITIALIZED, "CoRegisterPSClsid should have returned CO_E_NOTINITIALIZED instead of 0x%08lx\n", hr);

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoRegisterClassObject(&CLSID_WineTestPSFactoryBuffer, (IUnknown *)&PSFactoryBuffer,
        CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &dwRegistrationKey);
    ok_ole_success(hr, "CoRegisterClassObject");

    hr = CoRegisterPSClsid(&IID_IWineTest, &CLSID_WineTestPSFactoryBuffer);
    ok_ole_success(hr, "CoRegisterPSClsid");

    hr = CoGetPSClsid(&IID_IWineTest, &clsid);
    ok_ole_success(hr, "CoGetPSClsid");
    ok(IsEqualGUID(&clsid, &CLSID_WineTestPSFactoryBuffer), "expected %s, got %s\n",
                   wine_dbgstr_guid(&CLSID_WineTestPSFactoryBuffer), wine_dbgstr_guid(&clsid));

    thread = CreateThread(NULL, 0, register_ps_clsid_thread, NULL, 0, &tid);
    ok(thread != NULL, "CreateThread failed with error %ld\n", GetLastError());
    ok(!WaitForSingleObject(thread, 10000), "wait timed out\n");
    CloseHandle(thread);

    hr = CoGetPSClsid(&IID_TestPS, &clsid);
    ok_ole_success(hr, "CoGetPSClsid");
    ok(IsEqualGUID(&clsid, &CLSID_WineTestPSFactoryBuffer), "expected %s, got %s\n",
                   wine_dbgstr_guid(&CLSID_WineTestPSFactoryBuffer), wine_dbgstr_guid(&clsid));

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok_ole_success(hr, "CreateStreamOnHGlobal");

    SET_EXPECT(CreateStub);
    hr = CoMarshalInterface(stream, &IID_IWineTest, &Test_Unknown, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == E_NOTIMPL, "CoMarshalInterface should have returned E_NOTIMPL instead of 0x%08lx\n", hr);
    CHECK_CALLED(CreateStub, 1);

    hr = CoGetPSClsid(&IID_IEnumOLEVERB, &clsid);
    ok_ole_success(hr, "CoGetPSClsid");

    hr = CoGetClassObject(&clsid, CLSCTX_INPROC_SERVER, NULL, &IID_IPSFactoryBuffer, (void **)&ps_factory_buffer);
    ok_ole_success(hr, "CoGetClassObject");

    hr = CoRegisterPSClsid(&IID_IEnumOLEVERB, &CLSID_WineTestPSFactoryBuffer);
    ok_ole_success(hr, "CoRegisterPSClsid");

    SET_EXPECT(CreateStub);
    hr = CoMarshalInterface(stream, &IID_IEnumOLEVERB, (IUnknown*)&EnumOLEVERB, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == S_OK, "CoMarshalInterface should have returned S_OK instead of 0x%08lx\n", hr);
    CHECK_CALLED(CreateStub, 1);

    hr = CoMarshalInterface(stream, &IID_IEnumOLEVERB, &Test_Unknown, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == S_OK, "CoMarshalInterface should have returned S_OK instead of 0x%08lx\n", hr);

    IStream_Release(stream);
    IPSFactoryBuffer_Release(ps_factory_buffer);
    ps_factory_buffer = NULL;

    hr = CoRevokeClassObject(dwRegistrationKey);
    ok_ole_success(hr, "CoRevokeClassObject");

    CoUninitialize();

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoGetPSClsid(&IID_IWineTest, &clsid);
    ok(hr == REGDB_E_IIDNOTREG, "CoGetPSClsid should have returned REGDB_E_IIDNOTREG instead of 0x%08lx\n", hr);

    hr = CoGetPSClsid(&IID_TestPS, &clsid);
    ok(hr == REGDB_E_IIDNOTREG, "CoGetPSClsid should have returned REGDB_E_IIDNOTREG instead of 0x%08lx\n", hr);

    CoUninitialize();

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CoRegisterPSClsid(&IID_IWineTest, &CLSID_WineTestPSFactoryBuffer);
    ok_ole_success(hr, "CoRegisterPSClsid");

    hr = CoGetPSClsid(&IID_IWineTest, &clsid);
    ok_ole_success(hr, "CoGetPSClsid");
    ok(IsEqualGUID(&clsid, &CLSID_WineTestPSFactoryBuffer), "expected %s, got %s\n",
                   wine_dbgstr_guid(&CLSID_WineTestPSFactoryBuffer), wine_dbgstr_guid(&clsid));

    thread = CreateThread(NULL, 0, register_ps_clsid_thread, NULL, 0, &tid);
    ok(thread != NULL, "CreateThread failed with error %ld\n", GetLastError());
    ok(!WaitForSingleObject(thread, 10000), "wait timed out\n");
    CloseHandle(thread);

    hr = CoGetPSClsid(&IID_TestPS, &clsid);
    ok_ole_success(hr, "CoGetPSClsid");
    ok(IsEqualGUID(&clsid, &CLSID_WineTestPSFactoryBuffer), "expected %s, got %s\n",
                   wine_dbgstr_guid(&CLSID_WineTestPSFactoryBuffer), wine_dbgstr_guid(&clsid));

    CoUninitialize();
}

static void test_CoGetPSClsid(void)
{
    ULONG_PTR cookie;
    HANDLE handle;
    HRESULT hr;
    CLSID clsid;
    HKEY hkey;
    LONG res;
    const BOOL is_win64 = (sizeof(void*) != sizeof(int));
    BOOL is_wow64 = FALSE;

    hr = CoGetPSClsid(&IID_IClassFactory, &clsid);
    ok(hr == CO_E_NOTINITIALIZED,
       "CoGetPSClsid should have returned CO_E_NOTINITIALIZED instead of 0x%08lx\n",
       hr);

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoGetPSClsid(&IID_IClassFactory, &clsid);
    ok_ole_success(hr, "CoGetPSClsid");

    hr = CoGetPSClsid(&IID_IWineTest, &clsid);
    ok(hr == REGDB_E_IIDNOTREG,
       "CoGetPSClsid for random IID returned 0x%08lx instead of REGDB_E_IIDNOTREG\n",
       hr);

    hr = CoGetPSClsid(&IID_IClassFactory, NULL);
    ok(hr == E_INVALIDARG,
       "CoGetPSClsid for null clsid returned 0x%08lx instead of E_INVALIDARG\n",
       hr);

    if (!pRegOverridePredefKey)
    {
        win_skip("RegOverridePredefKey not available\n");
        CoUninitialize();
        return;
    }
    hr = CoGetPSClsid(&IID_IClassFactory, &clsid);
    ok_ole_success(hr, "CoGetPSClsid");

    res = RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Classes", 0, NULL, 0,
                          KEY_ALL_ACCESS, NULL, &hkey, NULL);
    ok(!res, "RegCreateKeyEx returned %ld\n", res);

    res = pRegOverridePredefKey(HKEY_CLASSES_ROOT, hkey);
    ok(!res, "RegOverridePredefKey returned %ld\n", res);

    hr = CoGetPSClsid(&IID_IClassFactory, &clsid);
    ok_ole_success(hr, "CoGetPSClsid");

    res = pRegOverridePredefKey(HKEY_CLASSES_ROOT, NULL);
    ok(!res, "RegOverridePredefKey returned %ld\n", res);

    RegCloseKey(hkey);

    /* not registered CLSID */
    hr = CoGetPSClsid(&IID_Testiface, &clsid);
    ok(hr == REGDB_E_IIDNOTREG, "got 0x%08lx\n", hr);

    if ((handle = activate_context(actctx_manifest, &cookie)))
    {
        memset(&clsid, 0, sizeof(clsid));
        hr = CoGetPSClsid(&IID_Testiface, &clsid);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(IsEqualGUID(&clsid, &IID_Testiface), "got clsid %s\n", wine_dbgstr_guid(&clsid));

        memset(&clsid, 0, sizeof(clsid));
        hr = CoGetPSClsid(&IID_Testiface2, &clsid);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(IsEqualGUID(&clsid, &IID_Testiface2), "got clsid %s\n", wine_dbgstr_guid(&clsid));

        memset(&clsid, 0, sizeof(clsid));
        hr = CoGetPSClsid(&IID_Testiface3, &clsid);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(IsEqualGUID(&clsid, &IID_TestPS), "got clsid %s\n", wine_dbgstr_guid(&clsid));

        memset(&clsid, 0xaa, sizeof(clsid));
        hr = CoGetPSClsid(&IID_Testiface4, &clsid);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(IsEqualGUID(&clsid, &GUID_NULL), "got clsid %s\n", wine_dbgstr_guid(&clsid));

        memset(&clsid, 0xaa, sizeof(clsid));
        hr = CoGetPSClsid(&IID_Testiface7, &clsid);
        ok(hr == S_OK, "Failed to get PS CLSID, hr %#lx.\n", hr);
        ok(IsEqualGUID(&clsid, &IID_Testiface7), "Unexpected CLSID %s.\n", wine_dbgstr_guid(&clsid));

        /* register same interface and try to get CLSID back */
        hr = CoRegisterPSClsid(&IID_Testiface, &IID_Testiface4);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        memset(&clsid, 0, sizeof(clsid));
        hr = CoGetPSClsid(&IID_Testiface, &clsid);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(IsEqualGUID(&clsid, &IID_Testiface4), "got clsid %s\n", wine_dbgstr_guid(&clsid));

        deactivate_context(handle, cookie);
    }

    if (pRegDeleteKeyExA &&
        (is_win64 ||
         (pIsWow64Process && pIsWow64Process(GetCurrentProcess(), &is_wow64) && is_wow64)))
    {
        static GUID IID_DeadBeef = {0xdeadbeef,0xdead,0xbeef,{0xde,0xad,0xbe,0xef,0xde,0xad,0xbe,0xef}};
        static const char clsidDeadBeef[] = "{deadbeef-dead-beef-dead-beefdeadbeef}";
        static const char clsidA[] = "{66666666-8888-7777-6666-555555555555}";
        HKEY hkey_iface, hkey_psclsid;
        REGSAM opposite = is_win64 ? KEY_WOW64_32KEY : KEY_WOW64_64KEY;

        hr = CoGetPSClsid(&IID_DeadBeef, &clsid);
        ok(hr == REGDB_E_IIDNOTREG, "got 0x%08lx\n", hr);

        res = RegCreateKeyExA(HKEY_CLASSES_ROOT, "Interface",
                              0, NULL, 0, KEY_ALL_ACCESS | opposite, NULL, &hkey_iface, NULL);
        ok(!res, "RegCreateKeyEx returned %ld\n", res);
        res = RegCreateKeyExA(hkey_iface, clsidDeadBeef,
                              0, NULL, 0, KEY_ALL_ACCESS | opposite, NULL, &hkey, NULL);
        if (res == ERROR_ACCESS_DENIED)
        {
            win_skip("Failed to create a key, skipping some of CoGetPSClsid() tests\n");
            goto cleanup;
        }

        ok(!res, "RegCreateKeyEx returned %ld\n", res);
        res = RegCreateKeyExA(hkey, "ProxyStubClsid32",
                              0, NULL, 0, KEY_ALL_ACCESS | opposite, NULL, &hkey_psclsid, NULL);
        ok(!res, "RegCreateKeyEx returned %ld\n", res);
        res = RegSetValueExA(hkey_psclsid, NULL, 0, REG_SZ, (const BYTE *)clsidA, strlen(clsidA)+1);
        ok(!res, "RegSetValueEx returned %ld\n", res);
        RegCloseKey(hkey_psclsid);

        hr = CoGetPSClsid(&IID_DeadBeef, &clsid);
        ok_ole_success(hr, "CoGetPSClsid");
        ok(IsEqualGUID(&clsid, &IID_TestPS), "got clsid %s\n", wine_dbgstr_guid(&clsid));

        res = pRegDeleteKeyExA(hkey, "ProxyStubClsid32", opposite, 0);
        ok(!res, "RegDeleteKeyEx returned %ld\n", res);
        RegCloseKey(hkey);
        res = pRegDeleteKeyExA(hkey_iface, clsidDeadBeef, opposite, 0);
        ok(!res, "RegDeleteKeyEx returned %ld\n", res);

    cleanup:
        RegCloseKey(hkey_iface);
    }

    CoUninitialize();
}

/* basic test, mainly for invalid arguments. see marshal.c for more */
static void test_CoUnmarshalInterface(void)
{
    IUnknown *pProxy;
    IStream *pStream;
    HRESULT hr;

    hr = CoUnmarshalInterface(NULL, &IID_IUnknown, (void **)&pProxy);
    ok(hr == E_INVALIDARG, "CoUnmarshalInterface should have returned E_INVALIDARG instead of 0x%08lx\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, "CreateStreamOnHGlobal");

    hr = CoUnmarshalInterface(pStream, &IID_IUnknown, (void **)&pProxy);
    todo_wine
    ok(hr == CO_E_NOTINITIALIZED, "CoUnmarshalInterface should have returned CO_E_NOTINITIALIZED instead of 0x%08lx\n", hr);

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoUnmarshalInterface(pStream, &IID_IUnknown, (void **)&pProxy);
    ok(hr == STG_E_READFAULT, "CoUnmarshalInterface should have returned STG_E_READFAULT instead of 0x%08lx\n", hr);

    CoUninitialize();

    hr = CoUnmarshalInterface(pStream, &IID_IUnknown, NULL);
    ok(hr == E_INVALIDARG, "CoUnmarshalInterface should have returned E_INVALIDARG instead of 0x%08lx\n", hr);

    IStream_Release(pStream);
}

static void test_CoGetInterfaceAndReleaseStream(void)
{
    HRESULT hr;
    IUnknown *pUnk;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoGetInterfaceAndReleaseStream(NULL, &IID_IUnknown, (void**)&pUnk);
    ok(hr == E_INVALIDARG, "hr %08lx\n", hr);

    CoUninitialize();
}

/* basic test, mainly for invalid arguments. see marshal.c for more */
static void test_CoMarshalInterface(void)
{
    IStream *pStream;
    HRESULT hr;
    static const LARGE_INTEGER llZero;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, "CreateStreamOnHGlobal");

    hr = CoMarshalInterface(pStream, &IID_IUnknown, NULL, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == E_INVALIDARG, "CoMarshalInterface should have returned E_INVALIDARG instead of 0x%08lx\n", hr);

    hr = CoMarshalInterface(NULL, &IID_IUnknown, (IUnknown *)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == E_INVALIDARG, "CoMarshalInterface should have returned E_INVALIDARG instead of 0x%08lx\n", hr);

    hr = CoMarshalInterface(pStream, &IID_IUnknown, (IUnknown *)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, "CoMarshalInterface");

    /* stream not rewound */
    hr = CoReleaseMarshalData(pStream);
    ok(hr == STG_E_READFAULT, "CoReleaseMarshalData should have returned STG_E_READFAULT instead of 0x%08lx\n", hr);

    hr = IStream_Seek(pStream, llZero, STREAM_SEEK_SET, NULL);
    ok_ole_success(hr, "IStream_Seek");

    hr = CoReleaseMarshalData(pStream);
    ok_ole_success(hr, "CoReleaseMarshalData");

    IStream_Release(pStream);

    CoUninitialize();
}

static void test_CoMarshalInterThreadInterfaceInStream(void)
{
    IStream *pStream;
    HRESULT hr;
    IClassFactory *pProxy;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    cLocks = 0;

    hr = CoMarshalInterThreadInterfaceInStream(&IID_IUnknown, (IUnknown *)&Test_ClassFactory, NULL);
    ok(hr == E_INVALIDARG, "CoMarshalInterThreadInterfaceInStream should have returned E_INVALIDARG instead of 0x%08lx\n", hr);

    hr = CoMarshalInterThreadInterfaceInStream(&IID_IUnknown, NULL, &pStream);
    ok(hr == E_INVALIDARG, "CoMarshalInterThreadInterfaceInStream should have returned E_INVALIDARG instead of 0x%08lx\n", hr);

    ok_no_locks();

    hr = CoMarshalInterThreadInterfaceInStream(&IID_IUnknown, (IUnknown *)&Test_ClassFactory, &pStream);
    ok_ole_success(hr, "CoMarshalInterThreadInterfaceInStream");

    ok_more_than_one_lock();

    hr = CoUnmarshalInterface(pStream, &IID_IClassFactory, (void **)&pProxy);
    ok_ole_success(hr, "CoUnmarshalInterface");

    IClassFactory_Release(pProxy);
    IStream_Release(pStream);

    ok_no_locks();

    CoUninitialize();
}

static void test_CoRegisterClassObject(void)
{
    ULONG_PTR ctxcookie;
    HANDLE handle;
    DWORD cookie;
    HRESULT hr;
    IClassFactory *pcf;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    /* CLSCTX_INPROC_SERVER */
    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_INPROC_SERVER, REGCLS_SINGLEUSE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");
    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");
    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_INPROC_SERVER, REGCLS_MULTI_SEPARATE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");
    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    /* CLSCTX_LOCAL_SERVER */
    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_LOCAL_SERVER, REGCLS_SINGLEUSE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");
    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");
    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_LOCAL_SERVER, REGCLS_MULTI_SEPARATE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");
    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    /* CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER */
    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER, REGCLS_SINGLEUSE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");
    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    /* test whether an object that doesn't support IClassFactory can be
     * registered for CLSCTX_LOCAL_SERVER */
    hr = CoRegisterClassObject(&CLSID_WineOOPTest, &Test_Unknown,
                               CLSCTX_LOCAL_SERVER, REGCLS_SINGLEUSE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");
    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    /* test whether registered class becomes invalid when apartment is destroyed */
    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_INPROC_SERVER, REGCLS_SINGLEUSE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");

    CoUninitialize();
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoGetClassObject(&CLSID_WineOOPTest, CLSCTX_INPROC_SERVER, NULL,
                          &IID_IClassFactory, (void **)&pcf);
    ok(hr == REGDB_E_CLASSNOTREG, "object registered in an apartment shouldn't accessible after it is destroyed\n");

    /* crashes with at least win9x DCOM! */
    if (0)
        CoRevokeClassObject(cookie);

    /* test that object is accessible */
    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory, CLSCTX_INPROC_SERVER,
        REGCLS_MULTIPLEUSE, &cookie);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = CoGetClassObject(&CLSID_WineOOPTest, CLSCTX_INPROC_SERVER, NULL, &IID_IClassFactory, (void**)&pcf);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    IClassFactory_Release(pcf);

    /* context now contains CLSID_WineOOPTest, test if registered one could still be used */
    if ((handle = activate_context(actctx_manifest, &ctxcookie)))
    {
        hr = CoGetClassObject(&CLSID_WineOOPTest, CLSCTX_INPROC_SERVER, NULL, &IID_IClassFactory, (void**)&pcf);
        ok(hr == 0x80001234 || broken(hr == HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND)) /* winxp */, "Unexpected hr %#lx.\n", hr);

        deactivate_context(handle, ctxcookie);
    }

    hr = CoGetClassObject(&CLSID_WineOOPTest, CLSCTX_INPROC_SERVER, NULL, &IID_IClassFactory, (void**)&pcf);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    IClassFactory_Release(pcf);

    hr = CoRevokeClassObject(cookie);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    CoUninitialize();
}

static HRESULT get_class_object(CLSCTX clsctx)
{
    HRESULT hr;
    IClassFactory *pcf;

    hr = CoGetClassObject(&CLSID_WineOOPTest, clsctx, NULL, &IID_IClassFactory,
                          (void **)&pcf);

    if (SUCCEEDED(hr))
        IClassFactory_Release(pcf);

    return hr;
}

static DWORD CALLBACK get_class_object_thread(LPVOID pv)
{
    CLSCTX clsctx = (CLSCTX)(DWORD_PTR)pv;
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = get_class_object(clsctx);

    CoUninitialize();

    return hr;
}

static DWORD CALLBACK get_class_object_proxy_thread(LPVOID pv)
{
    CLSCTX clsctx = (CLSCTX)(DWORD_PTR)pv;
    HRESULT hr;
    IClassFactory *pcf;
    IMultiQI *pMQI;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoGetClassObject(&CLSID_WineOOPTest, clsctx, NULL, &IID_IClassFactory,
                          (void **)&pcf);

    if (SUCCEEDED(hr))
    {
        hr = IClassFactory_QueryInterface(pcf, &IID_IMultiQI, (void **)&pMQI);
        if (SUCCEEDED(hr))
            IMultiQI_Release(pMQI);
        IClassFactory_Release(pcf);
    }

    CoUninitialize();

    return hr;
}

static DWORD CALLBACK register_class_object_thread(LPVOID pv)
{
    HRESULT hr;
    DWORD cookie;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_INPROC_SERVER, REGCLS_SINGLEUSE, &cookie);

    CoUninitialize();

    return hr;
}

static DWORD CALLBACK revoke_class_object_thread(LPVOID pv)
{
    DWORD cookie = (DWORD_PTR)pv;
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoRevokeClassObject(cookie);

    CoUninitialize();

    return hr;
}

static void test_registered_object_thread_affinity(void)
{
    HRESULT hr;
    DWORD cookie;
    HANDLE thread;
    DWORD tid;
    DWORD exitcode;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    /* CLSCTX_INPROC_SERVER */

    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_INPROC_SERVER, REGCLS_SINGLEUSE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");

    thread = CreateThread(NULL, 0, get_class_object_thread, (LPVOID)CLSCTX_INPROC_SERVER, 0, &tid);
    ok(thread != NULL, "CreateThread failed with error %ld\n", GetLastError());
    ok( !WaitForSingleObject(thread, 10000), "wait timed out\n" );
    GetExitCodeThread(thread, &exitcode);
    hr = exitcode;
    ok(hr == REGDB_E_CLASSNOTREG, "CoGetClassObject on inproc object "
       "registered in different thread should return REGDB_E_CLASSNOTREG "
       "instead of 0x%08lx\n", hr);

    hr = get_class_object(CLSCTX_INPROC_SERVER);
    ok(hr == S_OK, "CoGetClassObject on inproc object registered in same "
       "thread should return S_OK instead of 0x%08lx\n", hr);

    thread = CreateThread(NULL, 0, register_class_object_thread, NULL, 0, &tid);
    ok(thread != NULL, "CreateThread failed with error %ld\n", GetLastError());
    ok ( !WaitForSingleObject(thread, 10000), "wait timed out\n" );
    GetExitCodeThread(thread, &exitcode);
    hr = exitcode;
    ok(hr == S_OK, "CoRegisterClassObject with same CLSID but in different thread should return S_OK instead of 0x%08lx\n", hr);

    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    /* CLSCTX_LOCAL_SERVER */

    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");

    thread = CreateThread(NULL, 0, get_class_object_proxy_thread, (LPVOID)CLSCTX_LOCAL_SERVER, 0, &tid);
    ok(thread != NULL, "CreateThread failed with error %ld\n", GetLastError());
    while (MsgWaitForMultipleObjects(1, &thread, FALSE, 10000, QS_ALLINPUT) == WAIT_OBJECT_0 + 1)
    {
        MSG msg;
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
    GetExitCodeThread(thread, &exitcode);
    hr = exitcode;
    ok(hr == S_OK, "CoGetClassObject on local server object "
       "registered in different thread should return S_OK "
       "instead of 0x%08lx\n", hr);

    hr = get_class_object(CLSCTX_LOCAL_SERVER);
    ok(hr == S_OK, "CoGetClassObject on local server object registered in same "
       "thread should return S_OK instead of 0x%08lx\n", hr);

    thread = CreateThread(NULL, 0, revoke_class_object_thread, (LPVOID)(DWORD_PTR)cookie, 0, &tid);
    ok(thread != NULL, "CreateThread failed with error %ld\n", GetLastError());
    ok( !WaitForSingleObject(thread, 10000), "wait timed out\n" );
    GetExitCodeThread(thread, &exitcode);
    hr = exitcode;
    ok(hr == RPC_E_WRONG_THREAD || broken(hr == S_OK) /* win8 */, "CoRevokeClassObject called from different "
       "thread to where registered should return RPC_E_WRONG_THREAD instead of 0x%08lx\n", hr);

    thread = CreateThread(NULL, 0, register_class_object_thread, NULL, 0, &tid);
    ok(thread != NULL, "CreateThread failed with error %ld\n", GetLastError());
    ok( !WaitForSingleObject(thread, 10000), "wait timed out\n" );
    GetExitCodeThread(thread, &exitcode);
    hr = exitcode;
    ok(hr == S_OK, "CoRegisterClassObject with same CLSID but in different "
        "thread should return S_OK instead of 0x%08lx\n", hr);

    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    CoUninitialize();
}

static DWORD CALLBACK free_libraries_thread(LPVOID p)
{
    CoFreeUnusedLibraries();
    return 0;
}

static inline BOOL is_module_loaded(const char *module)
{
    return GetModuleHandleA(module) != 0;
}

static void test_CoFreeUnusedLibraries(void)
{
    HRESULT hr;
    IUnknown *pUnk;
    DWORD tid;
    HANDLE thread;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    ok(!is_module_loaded("urlmon.dll"), "urlmon.dll shouldn't be loaded\n");

    hr = CoCreateInstance(&CLSID_FileProtocol, NULL, CLSCTX_INPROC_SERVER, &IID_IInternetProtocol, (void **)&pUnk);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        skip("IE not installed so can't run CoFreeUnusedLibraries test\n");
        CoUninitialize();
        return;
    }
    ok_ole_success(hr, "CoCreateInstance");

    ok(is_module_loaded("urlmon.dll"), "urlmon.dll should be loaded\n");

    ok(pUnk != NULL ||
       broken(pUnk == NULL), /* win9x */
       "Expected a valid pointer\n");
    if (pUnk)
        IUnknown_Release(pUnk);

    ok(is_module_loaded("urlmon.dll"), "urlmon.dll should be loaded\n");

    thread = CreateThread(NULL, 0, free_libraries_thread, NULL, 0, &tid);
    ok( !WaitForSingleObject(thread, 10000), "wait timed out\n" );
    CloseHandle(thread);

    ok(is_module_loaded("urlmon.dll"), "urlmon.dll should be loaded\n");

    CoFreeUnusedLibraries();

    ok(!is_module_loaded("urlmon.dll"), "urlmon.dll shouldn't be loaded\n");

    CoUninitialize();
}

static void test_CoGetObjectContext(void)
{
    HRESULT hr;
    ULONG refs;
    IComThreadingInfo *pComThreadingInfo, *threadinginfo2;
    IContextCallback *pContextCallback;
    IObjContext *pObjContext;
    APTTYPE apttype;
    THDTYPE thdtype;
    GUID id, id2;

    if (!pCoGetObjectContext)
    {
        win_skip("CoGetObjectContext not present\n");
        return;
    }

    hr = pCoGetObjectContext(&IID_IComThreadingInfo, (void **)&pComThreadingInfo);
    ok(hr == CO_E_NOTINITIALIZED, "CoGetObjectContext should have returned CO_E_NOTINITIALIZED instead of 0x%08lx\n", hr);
    ok(pComThreadingInfo == NULL, "pComThreadingInfo should have been set to NULL\n");

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_apt_type(APTTYPE_MAINSTA, APTTYPEQUALIFIER_NONE);

    hr = pCoGetObjectContext(&IID_IComThreadingInfo, (void **)&pComThreadingInfo);
    ok_ole_success(hr, "CoGetObjectContext");

    threadinginfo2 = NULL;
    hr = pCoGetObjectContext(&IID_IComThreadingInfo, (void **)&threadinginfo2);
    ok(hr == S_OK, "Expected S_OK, got 0x%08lx\n", hr);
    ok(pComThreadingInfo == threadinginfo2, "got different instance\n");
    IComThreadingInfo_Release(threadinginfo2);

    hr = IComThreadingInfo_GetCurrentLogicalThreadId(pComThreadingInfo, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    id = id2 = GUID_NULL;
    hr = IComThreadingInfo_GetCurrentLogicalThreadId(pComThreadingInfo, &id);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = CoGetCurrentLogicalThreadId(&id2);
    ok(IsEqualGUID(&id, &id2), "got %s, expected %s\n", wine_dbgstr_guid(&id), wine_dbgstr_guid(&id2));

    hr = IComThreadingInfo_GetCurrentApartmentType(pComThreadingInfo, &apttype);
    ok_ole_success(hr, "IComThreadingInfo_GetCurrentApartmentType");
    ok(apttype == APTTYPE_MAINSTA, "apartment type should be APTTYPE_MAINSTA instead of %d\n", apttype);

    hr = IComThreadingInfo_GetCurrentThreadType(pComThreadingInfo, &thdtype);
    ok_ole_success(hr, "IComThreadingInfo_GetCurrentThreadType");
    ok(thdtype == THDTYPE_PROCESSMESSAGES, "thread type should be THDTYPE_PROCESSMESSAGES instead of %d\n", thdtype);

    refs = IComThreadingInfo_Release(pComThreadingInfo);
    ok(refs == 0, "pComThreadingInfo should have 0 refs instead of %ld refs\n", refs);

    hr = pCoGetObjectContext(&IID_IContextCallback, (void **)&pContextCallback);
    ok_ole_success(hr, "CoGetObjectContext(ContextCallback)");

    refs = IContextCallback_Release(pContextCallback);
    ok(refs == 0, "pContextCallback should have 0 refs instead of %ld refs\n", refs);

    CoUninitialize();

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = pCoGetObjectContext(&IID_IComThreadingInfo, (void **)&pComThreadingInfo);
    ok_ole_success(hr, "CoGetObjectContext");

    hr = IComThreadingInfo_GetCurrentApartmentType(pComThreadingInfo, &apttype);
    ok_ole_success(hr, "IComThreadingInfo_GetCurrentApartmentType");
    ok(apttype == APTTYPE_MTA, "apartment type should be APTTYPE_MTA instead of %d\n", apttype);

    hr = IComThreadingInfo_GetCurrentThreadType(pComThreadingInfo, &thdtype);
    ok_ole_success(hr, "IComThreadingInfo_GetCurrentThreadType");
    ok(thdtype == THDTYPE_BLOCKMESSAGES, "thread type should be THDTYPE_BLOCKMESSAGES instead of %d\n", thdtype);

    refs = IComThreadingInfo_Release(pComThreadingInfo);
    ok(refs == 0, "pComThreadingInfo should have 0 refs instead of %ld refs\n", refs);

    hr = pCoGetObjectContext(&IID_IContextCallback, (void **)&pContextCallback);
    ok_ole_success(hr, "CoGetObjectContext(ContextCallback)");

    refs = IContextCallback_Release(pContextCallback);
    ok(refs == 0, "pContextCallback should have 0 refs instead of %ld refs\n", refs);

    hr = pCoGetObjectContext(&IID_IObjContext, (void **)&pObjContext);
    ok_ole_success(hr, "CoGetObjectContext");

    refs = IObjContext_Release(pObjContext);
    ok(refs == 0, "pObjContext should have 0 refs instead of %ld refs\n", refs);

    CoUninitialize();
}

typedef struct {
    IUnknown IUnknown_iface;
    LONG refs;
} Test_CallContext;

static inline Test_CallContext *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, Test_CallContext, IUnknown_iface);
}

static HRESULT WINAPI Test_CallContext_QueryInterface(
    IUnknown *iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown))
    {
        *ppvObj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Test_CallContext_AddRef(IUnknown *iface)
{
    Test_CallContext *This = impl_from_IUnknown(iface);
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI Test_CallContext_Release(IUnknown *iface)
{
    Test_CallContext *This = impl_from_IUnknown(iface);
    ULONG refs = InterlockedDecrement(&This->refs);
    if (!refs)
        free(This);
    return refs;
}

static const IUnknownVtbl TestCallContext_Vtbl =
{
    Test_CallContext_QueryInterface,
    Test_CallContext_AddRef,
    Test_CallContext_Release
};

static void test_CoGetCallContext(void)
{
    HRESULT hr;
    ULONG refs;
    IUnknown *pUnk;
    Test_CallContext *test_object;

    if (!pCoSwitchCallContext)
    {
        skip("CoSwitchCallContext not present\n");
        return;
    }

    CoInitialize(NULL);

    test_object = malloc(sizeof(Test_CallContext));
    test_object->IUnknown_iface.lpVtbl = &TestCallContext_Vtbl;
    test_object->refs = 1;

    hr = CoGetCallContext(&IID_IUnknown, (void**)&pUnk);
    ok(hr == RPC_E_CALL_COMPLETE, "Expected RPC_E_CALL_COMPLETE, got 0x%08lx\n", hr);

    pUnk = (IUnknown*)0xdeadbeef;
    hr = pCoSwitchCallContext(&test_object->IUnknown_iface, &pUnk);
    ok_ole_success(hr, "CoSwitchCallContext");
    ok(pUnk == NULL, "expected NULL, got %p\n", pUnk);
    refs = IUnknown_AddRef(&test_object->IUnknown_iface);
    ok(refs == 2, "Expected refcount 2, got %ld\n", refs);
    IUnknown_Release(&test_object->IUnknown_iface);

    pUnk = (IUnknown*)0xdeadbeef;
    hr = CoGetCallContext(&IID_IUnknown, (void**)&pUnk);
    ok_ole_success(hr, "CoGetCallContext");
    ok(pUnk == &test_object->IUnknown_iface, "expected %p, got %p\n",
       &test_object->IUnknown_iface, pUnk);
    refs = IUnknown_AddRef(&test_object->IUnknown_iface);
    ok(refs == 3, "Expected refcount 3, got %ld\n", refs);
    IUnknown_Release(&test_object->IUnknown_iface);
    IUnknown_Release(pUnk);

    pUnk = (IUnknown*)0xdeadbeef;
    hr = pCoSwitchCallContext(NULL, &pUnk);
    ok_ole_success(hr, "CoSwitchCallContext");
    ok(pUnk == &test_object->IUnknown_iface, "expected %p, got %p\n",
       &test_object->IUnknown_iface, pUnk);
    refs = IUnknown_AddRef(&test_object->IUnknown_iface);
    ok(refs == 2, "Expected refcount 2, got %ld\n", refs);
    IUnknown_Release(&test_object->IUnknown_iface);

    hr = CoGetCallContext(&IID_IUnknown, (void**)&pUnk);
    ok(hr == RPC_E_CALL_COMPLETE, "Expected RPC_E_CALL_COMPLETE, got 0x%08lx\n", hr);

    IUnknown_Release(&test_object->IUnknown_iface);

    CoUninitialize();
}

static void test_CoGetContextToken(void)
{
    HRESULT hr;
    ULONG refs;
    ULONG_PTR token, token2;
    IObjContext *ctx;

    if (!pCoGetContextToken)
    {
        win_skip("CoGetContextToken not present\n");
        return;
    }

    token = 0xdeadbeef;
    hr = pCoGetContextToken(&token);
    ok(hr == CO_E_NOTINITIALIZED, "Expected CO_E_NOTINITIALIZED, got 0x%08lx\n", hr);
    ok(token == 0xdeadbeef, "Expected 0, got 0x%Ix\n", token);

    test_apt_type(APTTYPE_CURRENT, APTTYPEQUALIFIER_NONE);

    CoInitialize(NULL);

    test_apt_type(APTTYPE_MAINSTA, APTTYPEQUALIFIER_NONE);

    hr = pCoGetContextToken(NULL);
    ok(hr == E_POINTER, "Expected E_POINTER, got 0x%08lx\n", hr);

    token = 0;
    hr = pCoGetContextToken(&token);
    ok(hr == S_OK, "Expected S_OK, got 0x%08lx\n", hr);
    ok(token, "Expected token != 0\n");

    token2 = 0;
    hr = pCoGetContextToken(&token2);
    ok(hr == S_OK, "Expected S_OK, got 0x%08lx\n", hr);
    ok(token2 == token, "got different token\n");

    refs = IUnknown_AddRef((IUnknown *)token);
    ok(refs == 1, "Expected 1, got %lu\n", refs);

    hr = pCoGetObjectContext(&IID_IObjContext, (void **)&ctx);
    ok(hr == S_OK, "Expected S_OK, got 0x%08lx\n", hr);
    ok(ctx == (IObjContext *)token, "Expected interface pointers to be the same\n");

    refs = IObjContext_AddRef(ctx);
    ok(refs == 3, "Expected 3, got %lu\n", refs);

    refs = IObjContext_Release(ctx);
    ok(refs == 2, "Expected 2, got %lu\n", refs);

    refs = IUnknown_Release((IUnknown *)token);
    ok(refs == 1, "Expected 1, got %lu\n", refs);

    /* CoGetContextToken does not add a reference */
    token = 0;
    hr = pCoGetContextToken(&token);
    ok(hr == S_OK, "Expected S_OK, got 0x%08lx\n", hr);
    ok(token, "Expected token != 0\n");
    ok(ctx == (IObjContext *)token, "Expected interface pointers to be the same\n");

    refs = IObjContext_AddRef(ctx);
    ok(refs == 2, "Expected 1, got %lu\n", refs);

    refs = IObjContext_Release(ctx);
    ok(refs == 1, "Expected 0, got %lu\n", refs);

    refs = IObjContext_Release(ctx);
    ok(refs == 0, "Expected 0, got %lu\n", refs);

    CoUninitialize();
}

static void test_TreatAsClass(void)
{
    HRESULT hr;
    CLSID out;
    static GUID deadbeef = {0xdeadbeef,0xdead,0xbeef,{0xde,0xad,0xbe,0xef,0xde,0xad,0xbe,0xef}};
    static const char deadbeefA[] = "{DEADBEEF-DEAD-BEEF-DEAD-BEEFDEADBEEF}";
    IInternetProtocol *pIP = NULL;
    HKEY clsidkey, deadbeefkey;
    LONG lr;

    hr = CoGetTreatAsClass(&deadbeef,&out);
    ok (hr == S_FALSE, "expected S_FALSE got %lx\n",hr);
    ok (IsEqualGUID(&out,&deadbeef), "expected to get same clsid back\n");

    hr = CoGetTreatAsClass(NULL, &out);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr);
    ok(IsEqualGUID(&out, &deadbeef), "expected no change to the clsid\n");

    hr = CoGetTreatAsClass(&deadbeef, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr);

    lr = RegOpenKeyExA(HKEY_CLASSES_ROOT, "CLSID", 0, KEY_READ, &clsidkey);
    ok(!lr, "Couldn't open CLSID key, error %ld\n", lr);

    lr = RegCreateKeyExA(clsidkey, deadbeefA, 0, NULL, 0, KEY_WRITE, NULL, &deadbeefkey, NULL);
    if (lr) {
        win_skip("CoGetTreatAsClass() tests will be skipped (failed to create a test key, error %ld)\n", lr);
        RegCloseKey(clsidkey);
        return;
    }

    hr = CoTreatAsClass(&deadbeef, &deadbeef);
    ok(hr == REGDB_E_WRITEREGDB, "CoTreatAsClass gave wrong error: %08lx\n", hr);

    hr = CoTreatAsClass(&deadbeef, &CLSID_FileProtocol);
    if(hr == REGDB_E_WRITEREGDB){
        win_skip("Insufficient privileges to use CoTreatAsClass\n");
        goto exit;
    }
    ok(hr == S_OK, "CoTreatAsClass failed: %08lx\n", hr);

    hr = CoGetTreatAsClass(&deadbeef, &out);
    ok(hr == S_OK, "CoGetTreatAsClass failed: %08lx\n",hr);
    ok(IsEqualGUID(&out, &CLSID_FileProtocol), "expected to get substituted clsid\n");

    OleInitialize(NULL);

    hr = CoCreateInstance(&deadbeef, NULL, CLSCTX_INPROC_SERVER, &IID_IInternetProtocol, (void **)&pIP);
    if(hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("IE not installed so can't test CoCreateInstance\n");
        goto exit;
    }

    ok(hr == S_OK, "CoCreateInstance failed: %08lx\n", hr);
    if(pIP){
        IInternetProtocol_Release(pIP);
        pIP = NULL;
    }

    if (pCoCreateInstanceFromApp)
    {
        MULTI_QI mqi = { 0 };

        mqi.pIID = &IID_IInternetProtocol;
        hr = pCoCreateInstanceFromApp(&deadbeef, NULL, CLSCTX_INPROC_SERVER, NULL, 1, &mqi);
        ok(hr == REGDB_E_CLASSNOTREG, "Unexpected hr %#lx.\n", hr);

        hr = CoCreateInstance(&deadbeef, NULL, CLSCTX_INPROC_SERVER | CLSCTX_APPCONTAINER, &IID_IInternetProtocol,
                (void **)&pIP);
        ok(hr == REGDB_E_CLASSNOTREG, "Unexpected hr %#lx.\n", hr);

        hr = CoCreateInstance(&deadbeef, NULL, CLSCTX_INPROC_SERVER, &IID_IInternetProtocol, (void **)&pIP);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IUnknown_Release(pIP);
    }

    hr = CoTreatAsClass(&deadbeef, &CLSID_NULL);
    ok(hr == S_OK, "CoTreatAsClass failed: %08lx\n", hr);

    hr = CoGetTreatAsClass(&deadbeef, &out);
    ok(hr == S_FALSE, "expected S_FALSE got %08lx\n", hr);
    ok(IsEqualGUID(&out, &deadbeef), "expected to get same clsid back\n");

    /* bizarrely, native's CoTreatAsClass takes some time to take effect in CoCreateInstance */
    Sleep(200);

    hr = CoCreateInstance(&deadbeef, NULL, CLSCTX_INPROC_SERVER, &IID_IInternetProtocol, (void **)&pIP);
    ok(hr == REGDB_E_CLASSNOTREG, "CoCreateInstance gave wrong error: %08lx\n", hr);

    if(pIP)
        IInternetProtocol_Release(pIP);

exit:
    OleUninitialize();
    RegCloseKey(deadbeefkey);
    RegDeleteKeyA(clsidkey, deadbeefA);
    RegCloseKey(clsidkey);
}

static void test_CoInitializeEx(void)
{
    HRESULT hr;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ok(hr == S_OK, "CoInitializeEx failed with error 0x%08lx\n", hr);

    /* Calling OleInitialize for the first time should yield S_OK even with
     * apartment already initialized by previous CoInitialize(Ex) calls. */
    hr = OleInitialize(NULL);
    ok(hr == S_OK, "OleInitialize failed with error 0x%08lx\n", hr);

    /* Subsequent calls to OleInitialize should return S_FALSE */
    hr = OleInitialize(NULL);
    ok(hr == S_FALSE, "Expected S_FALSE, hr = 0x%08lx\n", hr);

    /* Cleanup */
    CoUninitialize();
    OleUninitialize();
    OleUninitialize();
}

static void test_OleInitialize_InitCounting(void)
{
    HRESULT hr;
    IUnknown *pUnk;
    REFCLSID rclsid = &CLSID_InternetZoneManager;

    /* 1. OleInitialize fails but OleUninitialize is still called: apartment stays initialized */
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    ok(hr == S_OK, "CoInitializeEx(COINIT_MULTITHREADED) failed with error 0x%08lx\n", hr);

    hr = OleInitialize(NULL);
    ok(hr == RPC_E_CHANGED_MODE, "OleInitialize should have returned 0x%08lx instead of 0x%08lx\n", RPC_E_CHANGED_MODE, hr);
    OleUninitialize();

    pUnk = (IUnknown *)0xdeadbeef;
    hr = CoCreateInstance(rclsid, NULL, 0x17, &IID_IUnknown, (void **)&pUnk);
    ok(hr == S_OK, "CoCreateInstance should have returned 0x%08lx instead of 0x%08lx\n", S_OK, hr);
    if (pUnk) IUnknown_Release(pUnk);

    CoUninitialize();

    /* 2. Extra multiple OleUninitialize: apartment stays initialized until CoUninitialize */
    hr = CoInitialize(NULL);
    ok(hr == S_OK, "CoInitialize() failed with error 0x%08lx\n", hr);

    hr = OleInitialize(NULL);
    ok(hr == S_OK, "OleInitialize should have returned 0x%08lx instead of 0x%08lx\n", S_OK, hr);
    OleUninitialize();
    OleUninitialize();
    OleUninitialize();

    pUnk = (IUnknown *)0xdeadbeef;
    hr = CoCreateInstance(rclsid, NULL, 0x17, &IID_IUnknown, (void **)&pUnk);
    ok(hr == S_OK, "CoCreateInstance should have returned 0x%08lx instead of 0x%08lx\n", S_OK, hr);
    if (pUnk) IUnknown_Release(pUnk);

    CoUninitialize();

    pUnk = (IUnknown *)0xdeadbeef;
    hr = CoCreateInstance(rclsid, NULL, 0x17, &IID_IUnknown, (void **)&pUnk);
    ok(hr == CO_E_NOTINITIALIZED, "CoCreateInstance should have returned 0x%08lx instead of 0x%08lx\n", CO_E_NOTINITIALIZED, hr);
    if (pUnk) IUnknown_Release(pUnk);

    /* 3. CoUninitialize does not formally deinit Ole */
    hr = CoInitialize(NULL);
    ok(hr == S_OK, "CoInitialize() failed with error 0x%08lx\n", hr);

    hr = OleInitialize(NULL);
    ok(hr == S_OK, "OleInitialize should have returned 0x%08lx instead of 0x%08lx\n", S_OK, hr);

    CoUninitialize();
    CoUninitialize();

    pUnk = (IUnknown *)0xdeadbeef;
    hr = CoCreateInstance(rclsid, NULL, 0x17, &IID_IUnknown, (void **)&pUnk);
    ok(hr == CO_E_NOTINITIALIZED, "CoCreateInstance should have returned 0x%08lx instead of 0x%08lx\n", CO_E_NOTINITIALIZED, hr);
      /* COM is not initialized anymore */
    if (pUnk) IUnknown_Release(pUnk);

    hr = OleInitialize(NULL);
    ok(hr == S_FALSE, "OleInitialize should have returned 0x%08lx instead of 0x%08lx\n", S_FALSE, hr);
      /* ... but native OleInit returns S_FALSE as if Ole is considered initialized */

    OleUninitialize();

}

static void test_OleRegGetMiscStatus(void)
{
    ULONG_PTR cookie;
    HANDLE handle;
    DWORD status;
    HRESULT hr;

    hr = OleRegGetMiscStatus(&CLSID_Testclass, DVASPECT_ICON, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    status = 0xdeadbeef;
    hr = OleRegGetMiscStatus(&CLSID_Testclass, DVASPECT_ICON, &status);
    ok(hr == REGDB_E_CLASSNOTREG, "got 0x%08lx\n", hr);
    ok(status == 0, "got 0x%08lx\n", status);

    status = -1;
    hr = OleRegGetMiscStatus(&CLSID_StdFont, DVASPECT_ICON, &status);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(status == 0, "got 0x%08lx\n", status);

    if ((handle = activate_context(actctx_manifest, &cookie)))
    {
        status = 0;
        hr = OleRegGetMiscStatus(&CLSID_Testclass, DVASPECT_ICON, &status);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(status == OLEMISC_RECOMPOSEONRESIZE, "got 0x%08lx\n", status);

        /* context data takes precedence over registration info */
        status = 0;
        hr = OleRegGetMiscStatus(&CLSID_StdFont, DVASPECT_ICON, &status);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(status == OLEMISC_RECOMPOSEONRESIZE, "got 0x%08lx\n", status);

        /* there's no such attribute in context */
        status = -1;
        hr = OleRegGetMiscStatus(&CLSID_Testclass, DVASPECT_DOCPRINT, &status);
        ok(hr == S_OK, "got 0x%08lx\n", hr);
        ok(status == 0, "got 0x%08lx\n", status);

        deactivate_context(handle, cookie);
    }
}

static void test_OleRegGetUserType(void)
{
    static const WCHAR stdfont_usertypeW[] = {'S','t','a','n','d','a','r','d',' ','F','o','n','t',0};
    static const WCHAR stdfont2_usertypeW[] = {'C','L','S','I','D','_','S','t','d','F','o','n','t',0};
    static const WCHAR clsidkeyW[] = {'C','L','S','I','D',0};
    static const WCHAR defvalueW[] = {'D','e','f','a','u','l','t',' ','N','a','m','e',0};
    static const WCHAR auxvalue0W[] = {'A','u','x',' ','N','a','m','e',' ','0',0};
    static const WCHAR auxvalue2W[] = {'A','u','x',' ','N','a','m','e',' ','2',0};
    static const WCHAR auxvalue3W[] = {'A','u','x',' ','N','a','m','e',' ','3',0};
    static const WCHAR auxvalue4W[] = {'A','u','x',' ','N','a','m','e',' ','4',0};

    static const char auxvalues[][16] = {
        "Aux Name 0",
        "Aux Name 1",
        "Aux Name 2",
        "Aux Name 3",
        "Aux Name 4"
    };

    HKEY clsidhkey, hkey, auxhkey, classkey;
    DWORD form, ret, disposition;
    WCHAR clsidW[39];
    ULONG_PTR cookie;
    HANDLE handle;
    HRESULT hr;
    WCHAR *str;
    int i;

    for (form = 0; form <= USERCLASSTYPE_APPNAME+1; form++) {
        hr = OleRegGetUserType(&CLSID_Testclass, form, NULL);
        ok(hr == E_INVALIDARG, "form %lu: got 0x%08lx\n", form, hr);

        str = (void*)0xdeadbeef;
        hr = OleRegGetUserType(&CLSID_Testclass, form, &str);
        ok(hr == REGDB_E_CLASSNOTREG, "form %lu: got 0x%08lx\n", form, hr);
        ok(str == NULL, "form %lu: got %p\n", form, str);

        /* same string returned for StdFont for all form types */
        str = NULL;
        hr = OleRegGetUserType(&CLSID_StdFont, form, &str);
        ok(hr == S_OK, "form %lu: got 0x%08lx\n", form, hr);
        ok(!lstrcmpW(str, stdfont_usertypeW) || !lstrcmpW(str, stdfont2_usertypeW) /* winxp */,
            "form %lu, got %s\n", form, wine_dbgstr_w(str));
        CoTaskMemFree(str);
    }

    if ((handle = activate_context(actctx_manifest, &cookie)))
    {
        for (form = 0; form <= USERCLASSTYPE_APPNAME+1; form++) {
            str = (void*)0xdeadbeef;
            hr = OleRegGetUserType(&CLSID_Testclass, form, &str);
            ok(hr == REGDB_E_CLASSNOTREG, "form %lu: got 0x%08lx\n", form, hr);
            ok(str == NULL, "form %lu: got %s\n", form, wine_dbgstr_w(str));

            /* same string returned for StdFont for all form types */
            str = NULL;
            hr = OleRegGetUserType(&CLSID_StdFont, form, &str);
            ok(hr == S_OK, "form %lu: got 0x%08lx\n", form, hr);
            ok(!lstrcmpW(str, stdfont_usertypeW) || !lstrcmpW(str, stdfont2_usertypeW) /* winxp */,
                "form %lu, got %s\n", form, wine_dbgstr_w(str));
            CoTaskMemFree(str);
        }

        deactivate_context(handle, cookie);
    }

    /* test using registered CLSID */
    StringFromGUID2(&CLSID_non_existent, clsidW, ARRAY_SIZE(clsidW));

    ret = RegCreateKeyExW(HKEY_CLASSES_ROOT, clsidkeyW, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &clsidhkey, &disposition);
    if (!ret)
    {
        ret = RegCreateKeyExW(clsidhkey, clsidW, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &classkey, NULL);
        if (ret)
            RegCloseKey(clsidhkey);
    }

    if (ret == ERROR_ACCESS_DENIED)
    {
        win_skip("Failed to create test key, skipping some of OleRegGetUserType() tests.\n");
        return;
    }

    ok(!ret, "failed to create a key, error %ld\n", ret);

    ret = RegSetValueExW(classkey, NULL, 0, REG_SZ, (const BYTE*)defvalueW, sizeof(defvalueW));
    ok(!ret, "got error %ld\n", ret);

    ret = RegCreateKeyExA(classkey, "AuxUserType", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &auxhkey, NULL);
    ok(!ret, "got error %ld\n", ret);

    /* populate AuxUserType */
    for (i = 0; i <= 4; i++) {
        char name[16];

        sprintf(name, "AuxUserType\\%d", i);
        ret = RegCreateKeyExA(classkey, name, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, NULL);
        ok(!ret, "got error %ld\n", ret);

        ret = RegSetValueExA(hkey, NULL, 0, REG_SZ, (const BYTE*)auxvalues[i], strlen(auxvalues[i]));
        ok(!ret, "got error %ld\n", ret);
        RegCloseKey(hkey);
    }

    str = NULL;
    hr = OleRegGetUserType(&CLSID_non_existent, 0, &str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(!lstrcmpW(str, auxvalue0W), "got %s\n", wine_dbgstr_w(str));
    CoTaskMemFree(str);

    str = NULL;
    hr = OleRegGetUserType(&CLSID_non_existent, USERCLASSTYPE_FULL, &str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(!lstrcmpW(str, defvalueW), "got %s\n", wine_dbgstr_w(str));
    CoTaskMemFree(str);

    str = NULL;
    hr = OleRegGetUserType(&CLSID_non_existent, USERCLASSTYPE_SHORT, &str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(!lstrcmpW(str, auxvalue2W), "got %s\n", wine_dbgstr_w(str));
    CoTaskMemFree(str);

    str = NULL;
    hr = OleRegGetUserType(&CLSID_non_existent, USERCLASSTYPE_APPNAME, &str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(!lstrcmpW(str, auxvalue3W), "got %s\n", wine_dbgstr_w(str));
    CoTaskMemFree(str);

    str = NULL;
    hr = OleRegGetUserType(&CLSID_non_existent, USERCLASSTYPE_APPNAME+1, &str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(!lstrcmpW(str, auxvalue4W), "got %s\n", wine_dbgstr_w(str));
    CoTaskMemFree(str);

    str = NULL;
    hr = OleRegGetUserType(&CLSID_non_existent, USERCLASSTYPE_APPNAME+2, &str);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(!lstrcmpW(str, defvalueW), "got %s\n", wine_dbgstr_w(str));
    CoTaskMemFree(str);

    /* registry cleanup */
    for (i = 0; i <= 4; i++)
    {
        char name[2];
        sprintf(name, "%d", i);
        RegDeleteKeyA(auxhkey, name);
    }
    RegCloseKey(auxhkey);
    RegDeleteKeyA(classkey, "AuxUserType");
    RegCloseKey(classkey);
    RegDeleteKeyW(clsidhkey, clsidW);
    RegCloseKey(clsidhkey);
    if (disposition == REG_CREATED_NEW_KEY)
        RegDeleteKeyA(HKEY_CLASSES_ROOT, "CLSID");
}

static void test_CoCreateGuid(void)
{
    HRESULT hr;

    hr = CoCreateGuid(NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
}

static void CALLBACK apc_test_proc(ULONG_PTR param)
{
    /* nothing */
}

static DWORD CALLBACK release_semaphore_thread( LPVOID arg )
{
    HANDLE handle = arg;
    if (WaitForSingleObject(handle, 200) == WAIT_TIMEOUT)
        ReleaseSemaphore(handle, 1, NULL);
    return 0;
}

static DWORD CALLBACK send_message_thread(LPVOID arg)
{
    HWND hWnd = arg;
    SendMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    return 0;
}

static DWORD CALLBACK send_and_post_user_message_thread(void *arg)
{
    HWND hwnd = arg;
    Sleep(30);
    SendMessageA(hwnd, WM_USER, 0, 0);
    PostMessageA(hwnd, WM_USER, 0, 0);
    return 0;
}

static DWORD CALLBACK post_message_thread(LPVOID arg)
{
    HWND hWnd = arg;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    return 0;
}

static const char cls_name[] = "cowait_test_class";

static UINT cowait_msgs[100], cowait_msgs_first, cowait_msgs_last;

static void cowait_msgs_reset(void)
{
    cowait_msgs_first = cowait_msgs_last = 0;
}

#define cowait_msgs_expect_empty() _cowait_msgs_expect_empty(__LINE__)
static void _cowait_msgs_expect_empty(unsigned line)
{
    while(cowait_msgs_first < cowait_msgs_last) {
        ok_(__FILE__,line)(0, "unexpected message %u\n", cowait_msgs[cowait_msgs_first]);
        cowait_msgs_first++;
    }
    cowait_msgs_reset();
}

#define cowait_msgs_expect_notified(a) _cowait_msgs_expect_notified(__LINE__,a)
static void _cowait_msgs_expect_notified(unsigned line, UINT expected_msg)
{
    if(cowait_msgs_first == cowait_msgs_last) {
        ok_(__FILE__,line)(0, "expected message %u, received none\n", expected_msg);
    }else {
        ok_(__FILE__,line)(cowait_msgs[cowait_msgs_first] == expected_msg,
                           "expected message %u, received %u \n",
                           expected_msg, cowait_msgs[cowait_msgs_first]);
        cowait_msgs_first++;
    }
}

#define cowait_msgs_expect_queued(a,b) _cowait_msgs_expect_queued(__LINE__,a,b)
static void _cowait_msgs_expect_queued(unsigned line, HWND hwnd, UINT expected_msg)
{
    MSG msg;
    BOOL success;

    success = PeekMessageA(&msg, hwnd, expected_msg, expected_msg, PM_REMOVE);
    ok_(__FILE__,line)(success, "PeekMessageA failed: %lu\n", GetLastError());
    if(success)
        ok_(__FILE__,line)(msg.message == expected_msg, "unexpected message %u, expected %u\n",
                           msg.message, expected_msg);
}

static void flush_messages(void)
{
    MSG msg;
    while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE ));
}

static LRESULT CALLBACK cowait_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if(cowait_msgs_last < ARRAY_SIZE(cowait_msgs))
        cowait_msgs[cowait_msgs_last++] = msg;
    if(msg == WM_DDE_FIRST)
        return 6;
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static DWORD CALLBACK cowait_unmarshal_thread(void *arg)
{
    IStream *stream = arg;
    IEnumOLEVERB *enum_verb;
    LARGE_INTEGER zero;
    IUnknown *unk;
    HRESULT hr;

    CoInitialize(NULL);

    zero.QuadPart = 0;
    hr = IStream_Seek(stream, zero, STREAM_SEEK_SET, NULL);
    ok(hr == S_OK, "Seek failed: %08lx\n", hr);

    hr = CoUnmarshalInterface(stream, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "CoUnmarshalInterface failed: %08lx\n", hr);

    hr = IUnknown_QueryInterface(unk, &IID_IEnumOLEVERB, (void**)&enum_verb);
    ok(hr == S_OK, "QueryInterface failed: %08lx\n", hr);

    IEnumOLEVERB_Release(enum_verb);
    IUnknown_Release(unk);

    CoUninitialize();
    return 0;
}

static DWORD CALLBACK test_CoWaitForMultipleHandles_thread(LPVOID arg)
{
    HANDLE *handles = arg, event, thread;
    IStream *stream;
    BOOL success;
    DWORD index, tid;
    HRESULT hr;
    HWND hWnd;
    UINT uMSG = 0xc065;
    MSG msg;
    int ret;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ok(hr == S_OK, "CoInitializeEx failed with error 0x%08lx\n", hr);

    hWnd = CreateWindowExA(0, cls_name, "Test (thread)", WS_TILEDWINDOW, 0, 0, 640, 480, 0, 0, 0, 0);
    ok(hWnd != 0, "CreateWindowExA failed %lu\n", GetLastError());

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(0, 50, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08lx\n", hr);
    ok(index==0 || index==0xdeadbeef/* Win 8 */, "expected index 0, got %lu\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(!success, "CoWaitForMultipleHandles didn't pump any messages\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_USER, 0, 0);
    hr = CoWaitForMultipleHandles(0, 50, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08lx\n", hr);
    ok(index==0 || index==0xdeadbeef/* Win 8 */, "expected index 0, got %lu\n", index);
    success = PeekMessageA(&msg, hWnd, WM_USER, WM_USER, PM_REMOVE);
    ok(success, "CoWaitForMultipleHandles unexpectedly pumped messages\n");

    /* Even if CoWaitForMultipleHandles does not pump a message it peeks
     * at ALL of them */
    index = 0xdeadbeef;
    PostMessageA(NULL, uMSG, 0, 0);

    hr = CoWaitForMultipleHandles(COWAIT_ALERTABLE, 50, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08lx\n", hr);
    ok(index == 0 || broken(index == 0xdeadbeef) /* Win 8 */, "expected index 0, got %lu\n", index);

    /* Make sure message was peeked at */
    ret = MsgWaitForMultipleObjectsEx(0, NULL, 2, QS_ALLPOSTMESSAGE, MWMO_ALERTABLE);
    ok(ret == WAIT_TIMEOUT, "MsgWaitForMultipleObjects returned %x\n", ret);

    /* But not pumped */
    success = PeekMessageA(&msg, NULL, uMSG, uMSG, PM_REMOVE);
    ok(success, "CoWaitForMultipleHandles unexpectedly pumped messages\n");

    DestroyWindow(hWnd);
    CoUninitialize();

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ok(hr == S_OK, "CoInitializeEx failed with error 0x%08lx\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "CreateStreamOnHGlobal failed: %08lx\n", hr);

    hr = CoMarshalInterface(stream, &IID_IUnknown, &Test_Unknown, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == S_OK, "CoMarshalInterface should have returned S_OK instead of 0x%08lx\n", hr);

    event = CreateEventW(NULL, TRUE, FALSE, NULL);

    PostQuitMessage(66);
    PostThreadMessageW(GetCurrentThreadId(), WM_QUIT, 0, 0);

    hr = CoRegisterMessageFilter(&MessageFilter, NULL);
    ok(hr == S_OK, "CoRegisterMessageFilter failed: %08lx\n", hr);

    thread = CreateThread(NULL, 0, cowait_unmarshal_thread, stream, 0, &tid);
    ok(thread != NULL, "CreateThread failed, error %lu\n", GetLastError());
    hr = CoWaitForMultipleHandles(0, 50, 1, &event, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08lx\n", hr);
    hr = CoWaitForMultipleHandles(0, 200, 1, &thread, &index);
    ok(hr == S_OK, "expected S_OK, got 0x%08lx\n", hr);
    ok(index == WAIT_OBJECT_0, "cowait_unmarshal_thread didn't finish\n");
    CloseHandle(thread);

    hr = CoRegisterMessageFilter(NULL, NULL);
    ok(hr == S_OK, "CoRegisterMessageFilter failed: %08lx\n", hr);

    IStream_Release(stream);

    CloseHandle(event);
    CoUninitialize();
    return 0;
}

static void test_CoWaitForMultipleHandles(void)
{
    HANDLE handles[2], thread;
    DWORD index, tid;
    WNDCLASSEXA wc;
    BOOL success;
    HRESULT hr;
    HWND hWnd;
    MSG msg;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ok(hr == S_OK, "CoInitializeEx failed with error 0x%08lx\n", hr);

    memset(&wc, 0, sizeof(wc));
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_VREDRAW | CS_HREDRAW;
    wc.hInstance     = GetModuleHandleA(0);
    wc.hCursor       = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = cls_name;
    wc.lpfnWndProc   = cowait_window_proc;
    success = RegisterClassExA(&wc) != 0;
    ok(success, "RegisterClassExA failed %lu\n", GetLastError());

    hWnd = CreateWindowExA(0, cls_name, "Test", WS_TILEDWINDOW, 0, 0, 640, 480, 0, 0, 0, 0);
    ok(hWnd != 0, "CreateWindowExA failed %lu\n", GetLastError());
    handles[0] = CreateSemaphoreA(NULL, 1, 1, NULL);
    ok(handles[0] != 0, "CreateSemaphoreA failed %lu\n", GetLastError());
    handles[1] = CreateSemaphoreA(NULL, 1, 1, NULL);
    ok(handles[1] != 0, "CreateSemaphoreA failed %lu\n", GetLastError());

    /* test without flags */

    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(0, 50, 0, handles, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got 0x%08lx\n", hr);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(success, "CoWaitForMultipleHandles unexpectedly pumped messages\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(0, 50, 0, NULL, &index);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got 0x%08lx\n", hr);
    ok(index == 0, "expected index 0, got %lu\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(success, "CoWaitForMultipleHandles unexpectedly pumped messages\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(0, 50, 0, handles, &index);
    ok(hr == RPC_E_NO_SYNC, "expected RPC_E_NO_SYNC, got 0x%08lx\n", hr);
    ok(index == 0, "expected index 0, got %lu\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(success, "CoWaitForMultipleHandles unexpectedly pumped messages\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(0, 50, 1, handles, &index);
    ok(hr == S_OK, "expected S_OK, got 0x%08lx\n", hr);
    ok(index == 0, "expected index 0, got %lu\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(success, "CoWaitForMultipleHandles unexpectedly pumped messages\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(0, 50, 2, handles, &index);
    ok(hr == S_OK, "expected S_OK, got 0x%08lx\n", hr);
    ok(index == 1, "expected index 1, got %lu\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(success, "CoWaitForMultipleHandles unexpectedly pumped messages\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(0, 50, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08lx\n", hr);
    ok(index == 0 || broken(index == 0xdeadbeef) /* Win 8 */, "expected index 0, got %lu\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(!success, "CoWaitForMultipleHandles didn't pump any messages\n");

    /* test PostMessageA/SendMessageA from a different thread */

    index = 0xdeadbeef;
    thread = CreateThread(NULL, 0, post_message_thread, hWnd, 0, &tid);
    ok(thread != NULL, "CreateThread failed, error %lu\n", GetLastError());
    hr = CoWaitForMultipleHandles(0, 100, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08lx\n", hr);
    ok(index == 0 || broken(index == 0xdeadbeef) /* Win 8 */, "expected index 0, got %lu\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(!success, "CoWaitForMultipleHandles didn't pump any messages\n");
    index = WaitForSingleObject(thread, 200);
    ok(index == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(thread);

    index = 0xdeadbeef;
    thread = CreateThread(NULL, 0, send_message_thread, hWnd, 0, &tid);
    ok(thread != NULL, "CreateThread failed, error %lu\n", GetLastError());
    hr = CoWaitForMultipleHandles(0, 100, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08lx\n", hr);
    ok(index == 0 || broken(index == 0xdeadbeef) /* Win 8 */, "expected index 0, got %lu\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(!success, "CoWaitForMultipleHandles didn't pump any messages\n");
    index = WaitForSingleObject(thread, 200);
    ok(index == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(thread);

    ReleaseSemaphore(handles[0], 1, NULL);
    ReleaseSemaphore(handles[1], 1, NULL);

    /* test with COWAIT_WAITALL */

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(COWAIT_WAITALL, 50, 2, handles, &index);
    ok(hr == S_OK, "expected S_OK, got 0x%08lx\n", hr);
    ok(index == 0, "expected index 0, got %lu\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(success, "CoWaitForMultipleHandles unexpectedly pumped messages\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(0, 50, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08lx\n", hr);
    ok(index == 0 || broken(index == 0xdeadbeef) /* Win 8 */, "expected index 0, got %lu\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(!success, "CoWaitForMultipleHandles didn't pump any messages\n");

    ReleaseSemaphore(handles[0], 1, NULL);
    ReleaseSemaphore(handles[1], 1, NULL);

    /* test with COWAIT_ALERTABLE */

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(COWAIT_ALERTABLE, 50, 1, handles, &index);
    ok(hr == S_OK, "expected S_OK, got 0x%08lx\n", hr);
    ok(index == 0, "expected index 0, got %lu\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(success, "CoWaitForMultipleHandles unexpectedly pumped messages\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(COWAIT_ALERTABLE, 50, 2, handles, &index);
    ok(hr == S_OK, "expected S_OK, got 0x%08lx\n", hr);
    ok(index == 1, "expected index 1, got %lu\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(success, "CoWaitForMultipleHandles unexpectedly pumped messages\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(COWAIT_ALERTABLE, 50, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08lx\n", hr);
    ok(index == 0 || broken(index == 0xdeadbeef) /* Win 8 */, "expected index 0, got %lu\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(!success, "CoWaitForMultipleHandles didn't pump any messages\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    success = QueueUserAPC(apc_test_proc, GetCurrentThread(), 0);
    ok(success, "QueueUserAPC failed %lu\n", GetLastError());
    hr = CoWaitForMultipleHandles(COWAIT_ALERTABLE, 50, 2, handles, &index);
    ok(hr == S_OK, "expected S_OK, got 0x%08lx\n", hr);
    ok(index == WAIT_IO_COMPLETION, "expected index WAIT_IO_COMPLETION, got %lu\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(success, "CoWaitForMultipleHandles unexpectedly pumped messages\n");

    /* test with COWAIT_INPUTAVAILABLE (semaphores are still locked) */

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_NOREMOVE);
    ok(success, "PeekMessageA returned FALSE\n");
    hr = CoWaitForMultipleHandles(0, 50, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08lx\n", hr);
    ok(index == 0 || broken(index == 0xdeadbeef) /* Win 8 */, "expected index 0, got %lu\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(!success, "CoWaitForMultipleHandles didn't pump any messages\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_NOREMOVE);
    ok(success, "PeekMessageA returned FALSE\n");
    thread = CreateThread(NULL, 0, release_semaphore_thread, handles[1], 0, &tid);
    ok(thread != NULL, "CreateThread failed, error %lu\n", GetLastError());
    hr = CoWaitForMultipleHandles(COWAIT_INPUTAVAILABLE, 50, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING || broken(hr == E_INVALIDARG) || broken(hr == S_OK) /* Win 8 */,
       "expected RPC_S_CALLPENDING, got 0x%08lx\n", hr);
    if (hr != S_OK) ReleaseSemaphore(handles[1], 1, NULL);
    ok(index == 0 || broken(index == 1) /* Win 8 */, "expected index 0, got %lu\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(!success || broken(success && hr == E_INVALIDARG),
       "CoWaitForMultipleHandles didn't pump any messages\n");
    index = WaitForSingleObject(thread, 200);
    ok(index == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(thread);

    cowait_msgs_reset();
    PostMessageA(hWnd, 0, 0, 0);
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    PostMessageA(hWnd, WM_USER+1, 0, 0);
    PostMessageA(hWnd, WM_DDE_FIRST+1, 0, 0);
    thread = CreateThread(NULL, 0, send_and_post_user_message_thread, hWnd, 0, &tid);
    ok(thread != NULL, "CreateThread failed, error %lu\n", GetLastError());

    hr = CoWaitForMultipleHandles(0, 100, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08lx\n", hr);

    cowait_msgs_expect_notified(WM_DDE_FIRST);
    cowait_msgs_expect_notified(WM_DDE_FIRST+1);
    cowait_msgs_expect_notified(WM_USER);
    cowait_msgs_expect_empty();
    cowait_msgs_expect_queued(hWnd, WM_USER);
    cowait_msgs_expect_queued(hWnd, WM_USER+1);
    flush_messages();

    index = WaitForSingleObject(thread, 200);
    ok(index == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(thread);

    /* test behaviour of WM_QUIT (semaphores are still locked) */

    PostMessageA(hWnd, WM_QUIT, 40, 0);
    memset(&msg, 0, sizeof(msg));
    success = PeekMessageA(&msg, hWnd, WM_QUIT, WM_QUIT, PM_REMOVE);
    ok(success, "PeekMessageA failed, error %lu\n", GetLastError());
    ok(msg.message == WM_QUIT, "expected msg.message = WM_QUIT, got %u\n", msg.message);
    ok(msg.wParam == 40, "expected msg.wParam = 40, got %Iu\n", msg.wParam);
    success = PeekMessageA(&msg, hWnd, WM_QUIT, WM_QUIT, PM_REMOVE);
    ok(!success, "PeekMessageA succeeded\n");

    cowait_msgs_reset();
    PostMessageA(hWnd, WM_QUIT, 40, 0);
    PostMessageA(hWnd, 0, 0, 0);
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    PostMessageA(hWnd, WM_USER+1, 0, 0);
    PostMessageA(hWnd, WM_DDE_FIRST+1, 0, 0);
    thread = CreateThread(NULL, 0, send_and_post_user_message_thread, hWnd, 0, &tid);
    ok(thread != NULL, "CreateThread failed, error %lu\n", GetLastError());

    hr = CoWaitForMultipleHandles(0, 100, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08lx\n", hr);

    cowait_msgs_expect_notified(WM_DDE_FIRST);
    cowait_msgs_expect_notified(WM_DDE_FIRST+1);
    cowait_msgs_expect_notified(WM_USER);
    cowait_msgs_expect_empty();
    cowait_msgs_expect_queued(hWnd, WM_USER);
    flush_messages();

    index = WaitForSingleObject(thread, 200);
    ok(index == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(thread);

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    PostMessageA(hWnd, WM_QUIT, 41, 0);
    thread = CreateThread(NULL, 0, post_message_thread, hWnd, 0, &tid);
    ok(thread != NULL, "CreateThread failed, error %lu\n", GetLastError());
    hr = CoWaitForMultipleHandles(0, 100, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08lx\n", hr);
    ok(index == 0 || broken(index == 0xdeadbeef) /* Win 8 */, "expected index 0, got %lu\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    todo_wine
    ok(success || broken(!success) /* Win 2000/XP/8 */, "PeekMessageA failed, error %lu\n", GetLastError());
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(!success, "PeekMessageA succeeded\n");
    memset(&msg, 0, sizeof(msg));
    success = PeekMessageA(&msg, hWnd, WM_QUIT, WM_QUIT, PM_REMOVE);
    todo_wine
    ok(!success || broken(success) /* Win 2000/XP/8 */, "PeekMessageA succeeded\n");
    if (success)
    {
        ok(msg.message == WM_QUIT, "expected msg.message = WM_QUIT, got %u\n", msg.message);
        ok(msg.wParam == 41, "expected msg.wParam = 41, got %Iu\n", msg.wParam);
    }
    index = WaitForSingleObject(thread, 200);
    ok(index == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(thread);

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    PostMessageA(hWnd, WM_QUIT, 42, 0);
    thread = CreateThread(NULL, 0, send_message_thread, hWnd, 0, &tid);
    ok(thread != NULL, "CreateThread failed, error %lu\n", GetLastError());
    hr = CoWaitForMultipleHandles(0, 500, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08lx\n", hr);
    ok(index == 0 || broken(index == 0xdeadbeef) /* Win 8 */, "expected index 0, got %lu\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(!success, "CoWaitForMultipleHandles didn't pump all WM_DDE_FIRST messages\n");
    memset(&msg, 0, sizeof(msg));
    success = PeekMessageA(&msg, hWnd, WM_QUIT, WM_QUIT, PM_REMOVE);
    ok(success, "PeekMessageA failed, error %lu\n", GetLastError());
    ok(msg.message == WM_QUIT, "expected msg.message = WM_QUIT, got %u\n", msg.message);
    ok(msg.wParam == 42, "expected msg.wParam = 42, got %Iu\n", msg.wParam);
    index = WaitForSingleObject(thread, 200);
    ok(index == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(thread);

    PostQuitMessage(43);
    memset(&msg, 0, sizeof(msg));
    success = PeekMessageA(&msg, hWnd, WM_QUIT, WM_QUIT, PM_REMOVE);
    ok(success || broken(!success) /* Win 8 */, "PeekMessageA failed, error %lu\n", GetLastError());
    if (!success)
        win_skip("PostQuitMessage didn't queue a WM_QUIT message, skipping tests\n");
    else
    {
        ok(msg.message == WM_QUIT, "expected msg.message = WM_QUIT, got %u\n", msg.message);
        ok(msg.wParam == 43, "expected msg.wParam = 43, got %Iu\n", msg.wParam);
        success = PeekMessageA(&msg, hWnd, WM_QUIT, WM_QUIT, PM_REMOVE);
        ok(!success, "PeekMessageA succeeded\n");

        index = 0xdeadbeef;
        PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
        PostQuitMessage(44);
        thread = CreateThread(NULL, 0, post_message_thread, hWnd, 0, &tid);
        ok(thread != NULL, "CreateThread failed, error %lu\n", GetLastError());
        hr = CoWaitForMultipleHandles(0, 100, 2, handles, &index);
        ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08lx\n", hr);
        ok(index == 0 || broken(index == 0xdeadbeef) /* Win 8 */, "expected index 0, got %lu\n", index);
        success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
        ok(success, "PeekMessageA failed, error %lu\n", GetLastError());
        success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
        ok(!success, "PeekMessageA succeeded\n");
        success = PeekMessageA(&msg, hWnd, WM_QUIT, WM_QUIT, PM_REMOVE);
        ok(!success, "CoWaitForMultipleHandles didn't remove WM_QUIT messages\n");
        index = WaitForSingleObject(thread, 200);
        ok(index == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
        CloseHandle(thread);

        index = 0xdeadbeef;
        PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
        PostQuitMessage(45);
        thread = CreateThread(NULL, 0, send_message_thread, hWnd, 0, &tid);
        ok(thread != NULL, "CreateThread failed, error %lu\n", GetLastError());
        hr = CoWaitForMultipleHandles(0, 100, 2, handles, &index);
        ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08lx\n", hr);
        ok(index == 0 || broken(index == 0xdeadbeef) /* Win 8 */, "expected index 0, got %lu\n", index);
        success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
        ok(success, "PeekMessageA failed, error %lu\n", GetLastError());
        success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
        ok(!success, "PeekMessageA succeeded\n");
        success = PeekMessageA(&msg, hWnd, WM_QUIT, WM_QUIT, PM_REMOVE);
        ok(!success, "CoWaitForMultipleHandles didn't remove WM_QUIT messages\n");
        index = WaitForSingleObject(thread, 200);
        ok(index == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
        CloseHandle(thread);
    }

    /* test message pumping when CoWaitForMultipleHandles is called from non main apartment thread */
    thread = CreateThread(NULL, 0, test_CoWaitForMultipleHandles_thread, handles, 0, &tid);
    index = WaitForSingleObject(thread, 5000);
    ok(index == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(thread);

    CoUninitialize();

    /* If COM was not initialized, messages are neither pumped nor peeked at */
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(0, 100, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "got %#lx\n", hr);
    success = MsgWaitForMultipleObjectsEx(0, NULL, 2, QS_ALLPOSTMESSAGE, MWMO_ALERTABLE);
    ok(success == 0, "MsgWaitForMultipleObjects returned %x\n", success);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(success, "PeekMessage failed: %lu\n", GetLastError());

    /* same in an MTA */
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(0, 100, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "got %#lx\n", hr);
    success = MsgWaitForMultipleObjectsEx(0, NULL, 2, QS_ALLPOSTMESSAGE, MWMO_ALERTABLE);
    ok(success == 0, "MsgWaitForMultipleObjects returned %x\n", success);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(success, "PeekMessage failed: %lu\n", GetLastError());

    CoUninitialize();

    CloseHandle(handles[0]);
    CloseHandle(handles[1]);
    DestroyWindow(hWnd);

    success = UnregisterClassA(cls_name, GetModuleHandleA(0));
    ok(success, "UnregisterClass failed %lu\n", GetLastError());
}

static void test_CoGetMalloc(void)
{
    IMalloc *imalloc;
    SIZE_T size;
    HRESULT hr;
    char *ptr;
    int ret;

    if (0) /* crashes on native */
        hr = CoGetMalloc(0, NULL);

    imalloc = (void*)0xdeadbeef;
    hr = CoGetMalloc(0, &imalloc);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
    ok(imalloc == NULL, "got %p\n", imalloc);

    imalloc = (void*)0xdeadbeef;
    hr = CoGetMalloc(MEMCTX_SHARED, &imalloc);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
    ok(imalloc == NULL, "got %p\n", imalloc);

    imalloc = (void*)0xdeadbeef;
    hr = CoGetMalloc(MEMCTX_MACSYSTEM, &imalloc);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
    ok(imalloc == NULL, "got %p\n", imalloc);

    imalloc = (void*)0xdeadbeef;
    hr = CoGetMalloc(MEMCTX_UNKNOWN, &imalloc);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
    ok(imalloc == NULL, "got %p\n", imalloc);

    imalloc = (void*)0xdeadbeef;
    hr = CoGetMalloc(MEMCTX_SAME, &imalloc);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
    ok(imalloc == NULL, "got %p\n", imalloc);

    imalloc = NULL;
    hr = CoGetMalloc(MEMCTX_TASK, &imalloc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(imalloc != NULL, "got %p\n", imalloc);

    /* DidAlloc() */
    ptr = IMalloc_Alloc(imalloc, 16);
    ok(!!ptr, "Failed to allocate block.\n");

    ret = IMalloc_DidAlloc(imalloc, ptr);
    ok(ret == 1, "Unexpected return value %d.\n", ret);

    ret = IMalloc_DidAlloc(imalloc, NULL);
    ok(ret == -1, "Unexpected return value %d.\n", ret);

    ret = IMalloc_DidAlloc(imalloc, (void *)0x1);
    ok(ret == 0, "Unexpected return value %d.\n", ret);

    ret = IMalloc_DidAlloc(imalloc, ptr + 4);
    ok(ret == 0, "Unexpected return value %d.\n", ret);

    /* GetSize() */
    size = IMalloc_GetSize(imalloc, NULL);
    ok(size == (SIZE_T)-1, "Unexpected return value.\n");

    size = IMalloc_GetSize(imalloc, ptr);
    ok(size == 16, "Unexpected return value.\n");

    IMalloc_Free(imalloc, ptr);

    IMalloc_Release(imalloc);
}

static void test_CoGetApartmentType(void)
{
    APTTYPEQUALIFIER qualifier;
    APTTYPE type;
    HRESULT hr;

    if (!pCoGetApartmentType)
    {
        win_skip("CoGetApartmentType not present\n");
        return;
    }

    hr = pCoGetApartmentType(NULL, NULL);
    ok(hr == E_INVALIDARG, "CoGetApartmentType succeeded, error: 0x%08lx\n", hr);

    type = 0xdeadbeef;
    hr = pCoGetApartmentType(&type, NULL);
    ok(hr == E_INVALIDARG, "CoGetApartmentType succeeded, error: 0x%08lx\n", hr);
    ok(type == 0xdeadbeef, "Expected 0xdeadbeef, got %u\n", type);

    qualifier = 0xdeadbeef;
    hr = pCoGetApartmentType(NULL, &qualifier);
    ok(hr == E_INVALIDARG, "CoGetApartmentType succeeded, error: 0x%08lx\n", hr);
    ok(qualifier == 0xdeadbeef, "Expected 0xdeadbeef, got %u\n", qualifier);

    type = 0xdeadbeef;
    qualifier = 0xdeadbeef;
    hr = pCoGetApartmentType(&type, &qualifier);
    ok(hr == CO_E_NOTINITIALIZED, "CoGetApartmentType succeeded, error: 0x%08lx\n", hr);
    ok(type == APTTYPE_CURRENT, "Expected APTTYPE_CURRENT, got %u\n", type);
    ok(qualifier == APTTYPEQUALIFIER_NONE, "Expected APTTYPEQUALIFIER_NONE, got %u\n", qualifier);

    type = 0xdeadbeef;
    qualifier = 0xdeadbeef;
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ok(hr == S_OK, "CoInitializeEx failed, error: 0x%08lx\n", hr);
    hr = pCoGetApartmentType(&type, &qualifier);
    ok(hr == S_OK, "CoGetApartmentType failed, error: 0x%08lx\n", hr);
    ok(type == APTTYPE_MAINSTA, "Expected APTTYPE_MAINSTA, got %u\n", type);
    ok(qualifier == APTTYPEQUALIFIER_NONE, "Expected APTTYPEQUALIFIER_NONE, got %u\n", qualifier);
    CoUninitialize();

    type = 0xdeadbeef;
    qualifier = 0xdeadbeef;
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    ok(hr == S_OK, "CoInitializeEx failed, error: 0x%08lx\n", hr);
    hr = pCoGetApartmentType(&type, &qualifier);
    ok(hr == S_OK, "CoGetApartmentType failed, error: 0x%08lx\n", hr);
    ok(type == APTTYPE_MTA, "Expected APTTYPE_MTA, got %u\n", type);
    ok(qualifier == APTTYPEQUALIFIER_NONE, "Expected APTTYPEQUALIFIER_NONE, got %u\n", qualifier);
    CoUninitialize();
}

static HRESULT WINAPI testspy_QI(IMallocSpy *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IMallocSpy) || IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IMallocSpy_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI testspy_AddRef(IMallocSpy *iface)
{
    return 2;
}

static ULONG WINAPI testspy_Release(IMallocSpy *iface)
{
    return 1;
}

static SIZE_T WINAPI testspy_PreAlloc(IMallocSpy *iface, SIZE_T cb)
{
    ok(0, "unexpected call\n");
    return 0;
}

static void* WINAPI testspy_PostAlloc(IMallocSpy *iface, void *ptr)
{
    ok(0, "unexpected call\n");
    return NULL;
}

static void* WINAPI testspy_PreFree(IMallocSpy *iface, void *ptr, BOOL spyed)
{
    ok(0, "unexpected call\n");
    return NULL;
}

static void WINAPI testspy_PostFree(IMallocSpy *iface, BOOL spyed)
{
    ok(0, "unexpected call\n");
}

static SIZE_T WINAPI testspy_PreRealloc(IMallocSpy *iface, void *ptr, SIZE_T cb, void **newptr, BOOL spyed)
{
    ok(0, "unexpected call\n");
    return 0;
}

static void* WINAPI testspy_PostRealloc(IMallocSpy *iface, void *ptr, BOOL spyed)
{
    ok(0, "unexpected call\n");
    return NULL;
}

static void* WINAPI testspy_PreGetSize(IMallocSpy *iface, void *ptr, BOOL spyed)
{
    ok(0, "unexpected call\n");
    return NULL;
}

static SIZE_T WINAPI testspy_PostGetSize(IMallocSpy *iface, SIZE_T actual, BOOL spyed)
{
    ok(0, "unexpected call\n");
    return 0;
}

static void* WINAPI testspy_PreDidAlloc(IMallocSpy *iface, void *ptr, BOOL spyed)
{
    ok(0, "unexpected call\n");
    return NULL;
}

static int WINAPI testspy_PostDidAlloc(IMallocSpy *iface, void *ptr, BOOL spyed, int actual)
{
    ok(0, "unexpected call\n");
    return 0;
}

static void WINAPI testspy_PreHeapMinimize(IMallocSpy *iface)
{
    ok(0, "unexpected call\n");
}

static void WINAPI testspy_PostHeapMinimize(IMallocSpy *iface)
{
    ok(0, "unexpected call\n");
}

static const IMallocSpyVtbl testspyvtbl =
{
    testspy_QI,
    testspy_AddRef,
    testspy_Release,
    testspy_PreAlloc,
    testspy_PostAlloc,
    testspy_PreFree,
    testspy_PostFree,
    testspy_PreRealloc,
    testspy_PostRealloc,
    testspy_PreGetSize,
    testspy_PostGetSize,
    testspy_PreDidAlloc,
    testspy_PostDidAlloc,
    testspy_PreHeapMinimize,
    testspy_PostHeapMinimize
};

static IMallocSpy testspy = { &testspyvtbl };

static void test_IMallocSpy(void)
{
    IMalloc *imalloc;
    HRESULT hr;

    hr = CoRegisterMallocSpy(NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    hr = CoRevokeMallocSpy();
    ok(hr == CO_E_OBJNOTREG, "got 0x%08lx\n", hr);

    hr = CoRegisterMallocSpy(&testspy);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = CoRegisterMallocSpy(NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    hr = CoRegisterMallocSpy(&testspy);
    ok(hr == CO_E_OBJISREG, "got 0x%08lx\n", hr);

    imalloc = NULL;
    hr = CoGetMalloc(MEMCTX_TASK, &imalloc);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(imalloc != NULL, "got %p\n", imalloc);

    IMalloc_Free(imalloc, NULL);

    IMalloc_Release(imalloc);

    hr = CoRevokeMallocSpy();
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = CoRevokeMallocSpy();
    ok(hr == CO_E_OBJNOTREG, "got 0x%08lx\n", hr);
}

static void test_CoGetCurrentLogicalThreadId(void)
{
    HRESULT hr;
    GUID id;

    hr = CoGetCurrentLogicalThreadId(NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    id = GUID_NULL;
    hr = CoGetCurrentLogicalThreadId(&id);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(!IsEqualGUID(&id, &GUID_NULL), "got null id\n");
}

static HRESULT WINAPI testinitialize_QI(IInitializeSpy *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IInitializeSpy) || IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IInitializeSpy_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI testinitialize_AddRef(IInitializeSpy *iface)
{
    return 2;
}

static ULONG WINAPI testinitialize_Release(IInitializeSpy *iface)
{
    return 1;
}

static DWORD expected_coinit_flags;
static ULARGE_INTEGER init_cookies[3];
static BOOL revoke_spies_on_uninit;

static HRESULT WINAPI testinitialize_PreInitialize(IInitializeSpy *iface, DWORD coinit, DWORD aptrefs)
{
    CHECK_EXPECT2(PreInitialize);
    ok(coinit == expected_coinit_flags, "Unexpected init flags %#lx, expected %#lx.\n", coinit, expected_coinit_flags);
    return S_OK;
}

static HRESULT WINAPI testinitialize_PostInitialize(IInitializeSpy *iface, HRESULT hr, DWORD coinit, DWORD aptrefs)
{
    CHECK_EXPECT2(PostInitialize);
    ok(coinit == expected_coinit_flags, "Unexpected init flags %#lx, expected %#lx.\n", coinit, expected_coinit_flags);
    return hr;
}

static HRESULT WINAPI testinitialize_PreUninitialize(IInitializeSpy *iface, DWORD aptrefs)
{
    HRESULT hr;
    CHECK_EXPECT2(PreUninitialize);
    if (revoke_spies_on_uninit)
    {
        hr = CoRevokeInitializeSpy(init_cookies[0]);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = CoRevokeInitializeSpy(init_cookies[1]);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        hr = CoRevokeInitializeSpy(init_cookies[2]);
        ok(hr == S_OK, "got 0x%08lx\n", hr);

        revoke_spies_on_uninit = FALSE;
    }
    return S_OK;
}

static HRESULT WINAPI testinitialize_PostUninitialize(IInitializeSpy *iface, DWORD aptrefs)
{
    CHECK_EXPECT2(PostUninitialize);
    return E_NOTIMPL;
}

static const IInitializeSpyVtbl testinitializevtbl =
{
    testinitialize_QI,
    testinitialize_AddRef,
    testinitialize_Release,
    testinitialize_PreInitialize,
    testinitialize_PostInitialize,
    testinitialize_PreUninitialize,
    testinitialize_PostUninitialize
};

static IInitializeSpy testinitialize = { &testinitializevtbl };

static DWORD WINAPI test_init_spies_proc(void *arg)
{
    HRESULT hr;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
    ok(hr == S_OK, "Failed to initialize COM, hr %#lx.\n", hr);

    hr = CoRevokeInitializeSpy(init_cookies[2]);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    CoUninitialize();
    return 0;
}

static void test_IInitializeSpy(BOOL mt)
{
    HRESULT hr;

    if (mt)
    {
        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        ok(hr == S_OK, "CoInitializeEx failed: %#lx\n", hr);
    }

    hr = CoRegisterInitializeSpy(NULL, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    init_cookies[0].QuadPart = 1;
    hr = CoRegisterInitializeSpy(NULL, &init_cookies[0]);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
    ok(init_cookies[0].QuadPart == 1, "got wrong cookie\n");

    hr = CoRegisterInitializeSpy(&testinitialize, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    init_cookies[0].HighPart = 0;
    init_cookies[0].LowPart = 1;
    hr = CoRegisterInitializeSpy(&testinitialize, &init_cookies[0]);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(init_cookies[0].HighPart == GetCurrentThreadId(), "got high part 0x%08lx, expected 0x%08lx\n", init_cookies[0].HighPart,
        GetCurrentThreadId());
    if (!mt) ok(init_cookies[0].LowPart == 0, "got wrong low part 0x%lx\n", init_cookies[0].LowPart);

    /* register same instance one more time */
    init_cookies[1].HighPart = 0;
    init_cookies[1].LowPart = 0;
    hr = CoRegisterInitializeSpy(&testinitialize, &init_cookies[1]);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(init_cookies[1].HighPart == GetCurrentThreadId(), "got high part 0x%08lx, expected 0x%08lx\n", init_cookies[1].HighPart,
        GetCurrentThreadId());
    if (!mt) ok(init_cookies[1].LowPart == 1, "got wrong low part 0x%lx\n", init_cookies[1].LowPart);

    init_cookies[2].HighPart = 0;
    init_cookies[2].LowPart = 0;
    hr = CoRegisterInitializeSpy(&testinitialize, &init_cookies[2]);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(init_cookies[2].HighPart == GetCurrentThreadId(), "got high part 0x%08lx, expected 0x%08lx\n", init_cookies[2].HighPart,
        GetCurrentThreadId());
    if (!mt) ok(init_cookies[2].LowPart == 2, "got wrong low part 0x%lx\n", init_cookies[2].LowPart);

    hr = CoRevokeInitializeSpy(init_cookies[1]);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = CoRevokeInitializeSpy(init_cookies[1]);
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);

    init_cookies[1].HighPart = 0;
    init_cookies[1].LowPart = 0;
    hr = CoRegisterInitializeSpy(&testinitialize, &init_cookies[1]);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(init_cookies[1].HighPart == GetCurrentThreadId(), "got high part 0x%08lx, expected 0x%08lx\n", init_cookies[1].HighPart,
        GetCurrentThreadId());
    if (!mt) ok(init_cookies[1].LowPart == 1, "got wrong low part 0x%lx\n", init_cookies[1].LowPart);

    SET_EXPECT(PreInitialize);
    SET_EXPECT(PostInitialize);
    hr = CoInitializeEx(NULL, expected_coinit_flags = ((mt ? COINIT_MULTITHREADED : COINIT_APARTMENTTHREADED) | COINIT_DISABLE_OLE1DDE));
    ok(hr == (mt ? S_FALSE : S_OK), "Failed to initialize COM, hr %#lx.\n", hr);
    CHECK_CALLED(PreInitialize, 3);
    CHECK_CALLED(PostInitialize, 3);

    if (mt)
    {
        HANDLE thread;
        thread = CreateThread(NULL, 0, test_init_spies_proc, NULL, 0, NULL);
        ok(thread != NULL, "CreateThread failed: %lu\n", GetLastError());
        ok(!WaitForSingleObject(thread, 1000), "wait failed\n");
    }

    SET_EXPECT(PreInitialize);
    SET_EXPECT(PostInitialize);
    hr = CoInitializeEx(NULL, expected_coinit_flags = ((mt ? COINIT_MULTITHREADED : COINIT_APARTMENTTHREADED) | COINIT_DISABLE_OLE1DDE));
    ok(hr == S_FALSE, "Failed to initialize COM, hr %#lx.\n", hr);
    CHECK_CALLED(PreInitialize, 3);
    CHECK_CALLED(PostInitialize, 3);

    SET_EXPECT(PreUninitialize);
    SET_EXPECT(PostUninitialize);
    CoUninitialize();
    CHECK_CALLED(PreUninitialize, 3);
    CHECK_CALLED(PostUninitialize, 3);

    SET_EXPECT(PreUninitialize);
    SET_EXPECT(PostUninitialize);
    CoUninitialize();
    CHECK_CALLED(PreUninitialize, 3);
    CHECK_CALLED(PostUninitialize, 3);

    if (mt)
    {
        SET_EXPECT(PreUninitialize);
        SET_EXPECT(PostUninitialize);
        CoUninitialize();
        CHECK_CALLED(PreUninitialize, 3);
        CHECK_CALLED(PostUninitialize, 3);
    }

    SET_EXPECT(PreInitialize);
    SET_EXPECT(PostInitialize);
    hr = CoInitializeEx(NULL, expected_coinit_flags = ((mt ? COINIT_MULTITHREADED : COINIT_APARTMENTTHREADED) | COINIT_DISABLE_OLE1DDE));
    ok(hr == S_OK, "Failed to initialize COM, hr %#lx.\n", hr);
    CHECK_CALLED(PreInitialize, 3);
    CHECK_CALLED(PostInitialize, 3);

    SET_EXPECT(PreUninitialize);
    revoke_spies_on_uninit = TRUE;
    CoUninitialize();
    CHECK_CALLED(PreUninitialize, 1);
}

static HRESULT g_persistfile_qi_ret;
static HRESULT g_persistfile_load_ret;
static HRESULT WINAPI testinstance_QI(IPersistFile *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown)) {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    if (IsEqualIID(riid, &IID_IPersistFile)) {
        if (SUCCEEDED(g_persistfile_qi_ret)) {
            *obj = iface;
            IUnknown_AddRef(iface);
        }
        else
            *obj = NULL;
        return g_persistfile_qi_ret;
    }

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI testinstance_AddRef(IPersistFile *iface)
{
    return 2;
}

static ULONG WINAPI testinstance_Release(IPersistFile *iface)
{
    return 1;
}

static HRESULT WINAPI testinstance_GetClassID(IPersistFile *iface, CLSID *clsid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testinstance_IsDirty(IPersistFile *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testinstance_Load(IPersistFile *iface, LPCOLESTR filename, DWORD mode)
{
    return g_persistfile_load_ret;
}

static HRESULT WINAPI testinstance_Save(IPersistFile *iface, LPCOLESTR filename, BOOL remember)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI testinstance_SaveCompleted(IPersistFile *iface, LPCOLESTR filename)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI testinstance_GetCurFile(IPersistFile *iface, LPOLESTR *filename)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IPersistFileVtbl testpersistfilevtbl = {
    testinstance_QI,
    testinstance_AddRef,
    testinstance_Release,
    testinstance_GetClassID,
    testinstance_IsDirty,
    testinstance_Load,
    testinstance_Save,
    testinstance_SaveCompleted,
    testinstance_GetCurFile
};

static IPersistFile testpersistfile = { &testpersistfilevtbl };

static HRESULT WINAPI getinstance_cf_QI(IClassFactory *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IClassFactory)) {
        *obj = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI getinstance_cf_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI getinstance_cf_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI getinstance_cf_CreateInstance(IClassFactory *iface, IUnknown *outer,
    REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown)) {
        *obj = &testpersistfile;
        return S_OK;
    }

    ok(0, "unexpected call, riid %s\n", wine_dbgstr_guid(riid));
    *obj = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI getinstance_cf_LockServer(IClassFactory *iface, BOOL lock)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IClassFactoryVtbl getinstance_cf_vtbl = {
    getinstance_cf_QI,
    getinstance_cf_AddRef,
    getinstance_cf_Release,
    getinstance_cf_CreateInstance,
    getinstance_cf_LockServer
};

static IClassFactory getinstance_cf = { &getinstance_cf_vtbl  };

static void test_CoGetInstanceFromFile(void)
{
    static const WCHAR filenameW[] = {'d','u','m','m','y','p','a','t','h',0};
    CLSID *clsid = (CLSID*)&CLSID_testclsid;
    MULTI_QI mqi[2];
    DWORD cookie;
    HRESULT hr;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    /* CLSID is not specified, file does not exist */
    mqi[0].pIID = &IID_IUnknown;
    mqi[0].pItf = NULL;
    mqi[0].hr = E_NOTIMPL;
    hr = CoGetInstanceFromFile(NULL, NULL, NULL, CLSCTX_INPROC_SERVER, STGM_READ, (OLECHAR*)filenameW, 1, mqi);
    todo_wine
    ok(hr == MK_E_CANTOPENFILE, "got 0x%08lx\n", hr);
    ok(mqi[0].pItf == NULL, "got %p\n", mqi[0].pItf);
    ok(mqi[0].hr == E_NOINTERFACE, "got 0x%08lx\n", mqi[0].hr);

    /* class is not available */
    mqi[0].pIID = &IID_IUnknown;
    mqi[0].pItf = NULL;
    mqi[0].hr = E_NOTIMPL;
    hr = CoGetInstanceFromFile(NULL, clsid, NULL, CLSCTX_INPROC_SERVER, STGM_READ, (OLECHAR*)filenameW, 1, mqi);
    ok(hr == REGDB_E_CLASSNOTREG, "got 0x%08lx\n", hr);
    ok(mqi[0].pItf == NULL, "got %p\n", mqi[0].pItf);
    ok(mqi[0].hr == REGDB_E_CLASSNOTREG, "got 0x%08lx\n", mqi[0].hr);

    hr = CoRegisterClassObject(clsid, (IUnknown*)&getinstance_cf, CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE,
        &cookie);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    mqi[0].pIID = &IID_IUnknown;
    mqi[0].pItf = (void*)0xdeadbeef;
    mqi[0].hr = S_OK;
    hr = CoGetInstanceFromFile(NULL, clsid, NULL, CLSCTX_INPROC_SERVER, STGM_READ, (OLECHAR*)filenameW, 1, mqi);
todo_wine {
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
    ok(mqi[0].pItf == (void*)0xdeadbeef, "got %p\n", mqi[0].pItf);
}
    ok(mqi[0].hr == S_OK, "got 0x%08lx\n", mqi[0].hr);

    mqi[0].pIID = &IID_IUnknown;
    mqi[0].pItf = (void*)0xdeadbeef;
    mqi[0].hr = E_NOTIMPL;
    hr = CoGetInstanceFromFile(NULL, clsid, NULL, CLSCTX_INPROC_SERVER, STGM_READ, (OLECHAR*)filenameW, 1, mqi);
todo_wine {
    ok(hr == E_INVALIDARG, "got 0x%08lx\n", hr);
    ok(mqi[0].pItf == (void*)0xdeadbeef, "got %p\n", mqi[0].pItf);
    ok(mqi[0].hr == E_NOTIMPL, "got 0x%08lx\n", mqi[0].hr);
}
    mqi[0].pIID = &IID_IUnknown;
    mqi[0].pItf = NULL;
    mqi[0].hr = E_NOTIMPL;
    hr = CoGetInstanceFromFile(NULL, clsid, NULL, CLSCTX_INPROC_SERVER, STGM_READ, (OLECHAR*)filenameW, 1, mqi);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(mqi[0].pItf != NULL, "got %p\n", mqi[0].pItf);
    ok(mqi[0].hr == S_OK, "got 0x%08lx\n", mqi[0].hr);

    mqi[0].pIID = &IID_IUnknown;
    mqi[0].pItf = NULL;
    mqi[0].hr = S_OK;
    hr = CoGetInstanceFromFile(NULL, clsid, NULL, CLSCTX_INPROC_SERVER, STGM_READ, (OLECHAR*)filenameW, 1, mqi);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(mqi[0].pItf != NULL, "got %p\n", mqi[0].pItf);
    ok(mqi[0].hr == S_OK, "got 0x%08lx\n", mqi[0].hr);

    mqi[0].pIID = &IID_IUnknown;
    mqi[0].pItf = NULL;
    mqi[0].hr = S_OK;
    g_persistfile_qi_ret = S_FALSE;
    hr = CoGetInstanceFromFile(NULL, clsid, NULL, CLSCTX_INPROC_SERVER, STGM_READ, (OLECHAR*)filenameW, 1, mqi);
    ok(hr == S_OK, "got 0x%08lx\n", hr);
    ok(mqi[0].pItf != NULL, "got %p\n", mqi[0].pItf);
    ok(mqi[0].hr == S_OK, "got 0x%08lx\n", mqi[0].hr);
    g_persistfile_qi_ret = S_OK;

    mqi[0].pIID = &IID_IUnknown;
    mqi[0].pItf = NULL;
    mqi[0].hr = S_OK;
    mqi[1].pIID = &IID_IUnknown;
    mqi[1].pItf = NULL;
    mqi[1].hr = S_OK;
    g_persistfile_qi_ret = 0x8000efef;
    hr = CoGetInstanceFromFile(NULL, clsid, NULL, CLSCTX_INPROC_SERVER, STGM_READ, (OLECHAR*)filenameW, 2, mqi);
    ok(hr == 0x8000efef, "got 0x%08lx\n", hr);
    ok(mqi[0].pItf == NULL, "got %p\n", mqi[0].pItf);
    ok(mqi[0].hr == 0x8000efef, "got 0x%08lx\n", mqi[0].hr);
    ok(mqi[1].pItf == NULL, "got %p\n", mqi[1].pItf);
    ok(mqi[1].hr == 0x8000efef, "got 0x%08lx\n", mqi[1].hr);
    g_persistfile_qi_ret = S_OK;

    mqi[0].pIID = &IID_IUnknown;
    mqi[0].pItf = NULL;
    mqi[0].hr = S_OK;
    mqi[1].pIID = &IID_IUnknown;
    mqi[1].pItf = NULL;
    mqi[1].hr = S_OK;
    g_persistfile_load_ret = 0x8000fefe;
    hr = CoGetInstanceFromFile(NULL, clsid, NULL, CLSCTX_INPROC_SERVER, STGM_READ, (OLECHAR*)filenameW, 2, mqi);
    ok(hr == 0x8000fefe, "got 0x%08lx\n", hr);
    ok(mqi[0].pItf == NULL, "got %p\n", mqi[0].pItf);
    ok(mqi[0].hr == 0x8000fefe, "got 0x%08lx\n", mqi[0].hr);
    ok(mqi[1].pItf == NULL, "got %p\n", mqi[1].pItf);
    ok(mqi[1].hr == 0x8000fefe, "got 0x%08lx\n", mqi[1].hr);
    g_persistfile_load_ret = S_OK;

    hr = CoRevokeClassObject(cookie);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    CoUninitialize();
}

static void test_GlobalOptions(void)
{
    IGlobalOptions *global_options;
    ULONG_PTR value;
    HRESULT hres;

    CoInitialize(NULL);

    hres = CoCreateInstance(&CLSID_GlobalOptions, NULL, CLSCTX_INPROC_SERVER,
            &IID_IGlobalOptions, (void**)&global_options);
    ok(hres == S_OK || broken(hres == E_NOINTERFACE), "CoCreateInstance(CLSID_GlobalOptions) failed: %08lx\n", hres);
    if(FAILED(hres))
    {
        win_skip("CLSID_GlobalOptions not available\n");
        CoUninitialize();
        return;
    }

    hres = IGlobalOptions_Query(global_options, 0, &value);
    ok(FAILED(hres), "Unexpected hr %#lx.\n", hres);

    hres = IGlobalOptions_Query(global_options, COMGLB_PROPERTIES_RESERVED3 + 1, &value);
    ok(FAILED(hres), "Unexpected hr %#lx.\n", hres);

    value = ~0u;
    hres = IGlobalOptions_Query(global_options, COMGLB_EXCEPTION_HANDLING, &value);
    ok(hres == S_OK || broken(hres == E_FAIL) /* Vista */, "Unexpected hr %#lx.\n", hres);
    if (SUCCEEDED(hres))
        ok(value == COMGLB_EXCEPTION_HANDLE, "Unexpected value %Id.\n", value);

    IGlobalOptions_Release(global_options);

    hres = CoCreateInstance(&CLSID_GlobalOptions, (IUnknown*)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IGlobalOptions, (void**)&global_options);
    ok(hres == E_INVALIDARG, "CoCreateInstance(CLSID_GlobalOptions) failed: %08lx\n", hres);

    CoUninitialize();
}

static void init_funcs(void)
{
    HMODULE hOle32 = GetModuleHandleA("ole32");
    HMODULE hAdvapi32 = GetModuleHandleA("advapi32");
    HMODULE hkernel32 = GetModuleHandleA("kernel32");

    pCoGetObjectContext = (void*)GetProcAddress(hOle32, "CoGetObjectContext");
    pCoSwitchCallContext = (void*)GetProcAddress(hOle32, "CoSwitchCallContext");
    pCoGetContextToken = (void*)GetProcAddress(hOle32, "CoGetContextToken");
    pCoGetApartmentType = (void*)GetProcAddress(hOle32, "CoGetApartmentType");
    pCoIncrementMTAUsage = (void*)GetProcAddress(hOle32, "CoIncrementMTAUsage");
    pCoDecrementMTAUsage = (void*)GetProcAddress(hOle32, "CoDecrementMTAUsage");
    pCoCreateInstanceFromApp = (void*)GetProcAddress(hOle32, "CoCreateInstanceFromApp");
    pRegDeleteKeyExA = (void*)GetProcAddress(hAdvapi32, "RegDeleteKeyExA");
    pRegOverridePredefKey = (void*)GetProcAddress(hAdvapi32, "RegOverridePredefKey");

    pIsWow64Process = (void*)GetProcAddress(hkernel32, "IsWow64Process");
}

static DWORD CALLBACK implicit_mta_proc(void *param)
{
    IComThreadingInfo *threading_info;
    ULONG_PTR token;
    IUnknown *unk;
    DWORD cookie;
    CLSID clsid;
    HRESULT hr;

    test_apt_type(APTTYPE_MTA, APTTYPEQUALIFIER_IMPLICIT_MTA);

    hr = CoCreateInstance(&CLSID_InternetZoneManager, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
    ok_ole_success(hr, "CoCreateInstance");
    IUnknown_Release(unk);

    hr = CoGetClassObject(&CLSID_InternetZoneManager, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void **)&unk);
    ok_ole_success(hr, "CoGetClassObject");
    IUnknown_Release(unk);

    hr = CoGetObjectContext(&IID_IComThreadingInfo, (void **)&threading_info);
    ok_ole_success(hr, "CoGetObjectContext");
    IComThreadingInfo_Release(threading_info);

    hr = CoGetContextToken(&token);
    ok_ole_success(hr, "CoGetContextToken");

    hr = CoRegisterPSClsid(&IID_IWineTest, &CLSID_WineTestPSFactoryBuffer);
    ok_ole_success(hr, "CoRegisterPSClsid");

    hr = CoGetPSClsid(&IID_IClassFactory, &clsid);
    ok_ole_success(hr, "CoGetPSClsid");

    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_INPROC_SERVER, REGCLS_SINGLEUSE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");

    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    hr = CoRegisterMessageFilter(NULL, NULL);
    ok(hr == CO_E_NOT_SUPPORTED, "got %#lx\n", hr);

    hr = CoLockObjectExternal(&Test_Unknown, TRUE, TRUE);
    ok_ole_success(hr, "CoLockObjectExternal");

    hr = CoDisconnectObject(&Test_Unknown, 0);
    ok_ole_success(hr, "CoDisconnectObject");

    return 0;
}

/* Some COM functions (perhaps even all of them?) can make use of an "implicit"
 * multi-threaded apartment created by another thread in the same process. */
static void test_implicit_mta(void)
{
    HANDLE thread;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    thread = CreateThread(NULL, 0, implicit_mta_proc, NULL, 0, NULL);
    ok(!WaitForSingleObject(thread, 1000), "wait failed\n");

    CoUninitialize();
}

static DWORD WINAPI co_get_current_process_thread(void *param)
{
    DWORD *id = param;

    *id = CoGetCurrentProcess();
    return 0;
}

static void test_CoGetCurrentProcess(void)
{
    DWORD id, id2;
    HANDLE thread;

    id = CoGetCurrentProcess();
    ok(!!id && id != GetCurrentProcessId() && id != GetCurrentThreadId(), "Unexpected result %ld.\n", id);

    id2 = 0;
    thread = CreateThread(NULL, 0, co_get_current_process_thread, &id2, 0, NULL);
    ok(thread != NULL, "Failed to create test thread.\n");
    ok(!WaitForSingleObject(thread, 10000), "Wait timed out.\n");
    CloseHandle(thread);

    ok(id2 && id2 != id, "Unexpected id from another thread.\n");
}

static void test_mta_usage(void)
{
    CO_MTA_USAGE_COOKIE cookie, cookie2;
    HRESULT hr;

    if (!pCoIncrementMTAUsage)
    {
        win_skip("CoIncrementMTAUsage() is not available.\n");
        return;
    }

    test_apt_type(APTTYPE_CURRENT, APTTYPEQUALIFIER_NONE);

    cookie = 0;
    hr = pCoIncrementMTAUsage(&cookie);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(cookie != NULL, "Unexpected cookie %p.\n", cookie);

    cookie2 = 0;
    hr = pCoIncrementMTAUsage(&cookie2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(cookie2 != NULL && cookie2 != cookie, "Unexpected cookie %p.\n", cookie2);

    test_apt_type(APTTYPE_MTA, APTTYPEQUALIFIER_IMPLICIT_MTA);

    hr = pCoDecrementMTAUsage(cookie);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    test_apt_type(APTTYPE_MTA, APTTYPEQUALIFIER_IMPLICIT_MTA);

    hr = pCoDecrementMTAUsage(cookie2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    test_apt_type(APTTYPE_CURRENT, APTTYPEQUALIFIER_NONE);

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    test_apt_type(APTTYPE_MAINSTA, APTTYPEQUALIFIER_NONE);

    cookie = 0;
    hr = pCoIncrementMTAUsage(&cookie);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(cookie != NULL, "Unexpected cookie %p.\n", cookie);

    test_apt_type(APTTYPE_MAINSTA, APTTYPEQUALIFIER_NONE);

    hr = pCoDecrementMTAUsage(cookie);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CoUninitialize();

    test_apt_type(APTTYPE_CURRENT, APTTYPEQUALIFIER_NONE);
}

static void test_CoCreateInstanceFromApp(void)
{
    static const CLSID *supported_classes[] =
    {
        &CLSID_InProcFreeMarshaler,
        &CLSID_GlobalOptions,
        &CLSID_StdGlobalInterfaceTable,
    };
    static const CLSID *unsupported_classes[] =
    {
        &CLSID_ManualResetEvent,
    };
    unsigned int i;
    IUnknown *unk;
    DWORD cookie;
    MULTI_QI mqi;
    HRESULT hr;
    HANDLE handle;
    ULONG_PTR actctx_cookie;

    if (!pCoCreateInstanceFromApp)
    {
        win_skip("CoCreateInstanceFromApp() is not available.\n");
        return;
    }

    CoInitialize(NULL);

    for (i = 0; i < ARRAY_SIZE(supported_classes); ++i)
    {
        memset(&mqi, 0, sizeof(mqi));
        mqi.pIID = &IID_IUnknown;
        hr = pCoCreateInstanceFromApp(supported_classes[i], NULL, CLSCTX_INPROC_SERVER, NULL, 1, &mqi);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IUnknown_Release(mqi.pItf);

        hr = CoCreateInstance(supported_classes[i], NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IUnknown_Release(unk);
    }

    for (i = 0; i < ARRAY_SIZE(unsupported_classes); ++i)
    {
        memset(&mqi, 0, sizeof(mqi));
        mqi.pIID = &IID_IUnknown;
        hr = pCoCreateInstanceFromApp(unsupported_classes[i], NULL, CLSCTX_INPROC_SERVER, NULL, 1, &mqi);
        ok(hr == REGDB_E_CLASSNOTREG, "Unexpected hr %#lx.\n", hr);

        hr = CoCreateInstance(unsupported_classes[i], NULL, CLSCTX_INPROC_SERVER | CLSCTX_APPCONTAINER,
                &IID_IUnknown, (void **)&unk);
        ok(hr == REGDB_E_CLASSNOTREG, "Unexpected hr %#lx.\n", hr);

        hr = CoCreateInstance(unsupported_classes[i], NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        IUnknown_Release(unk);
    }

    /* Locally registered classes are filtered out. */
    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory, CLSCTX_INPROC_SERVER,
            REGCLS_MULTIPLEUSE, &cookie);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoGetClassObject(&CLSID_WineOOPTest, CLSCTX_INPROC_SERVER, NULL, &IID_IClassFactory, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoGetClassObject(&CLSID_WineOOPTest, CLSCTX_INPROC_SERVER | CLSCTX_APPCONTAINER, NULL,
            &IID_IClassFactory, (void **)&unk);
    todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_WineOOPTest, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
    ok(hr == E_NOINTERFACE, "Unexpected hr %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_WineOOPTest, NULL, CLSCTX_INPROC_SERVER | CLSCTX_APPCONTAINER,
            &IID_IUnknown, (void **)&unk);
    ok(hr == REGDB_E_CLASSNOTREG, "Unexpected hr %#lx.\n", hr);

    memset(&mqi, 0, sizeof(mqi));
    mqi.pIID = &IID_IUnknown;
    hr = pCoCreateInstanceFromApp(&CLSID_WineOOPTest, NULL, CLSCTX_INPROC_SERVER, NULL, 1, &mqi);
    ok(hr == REGDB_E_CLASSNOTREG, "Unexpected hr %#lx.\n", hr);

    hr = CoRevokeClassObject(cookie);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Activation context */
    if ((handle = activate_context(actctx_manifest, &actctx_cookie)))
    {
        hr = CoCreateInstance(&IID_Testiface7, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
        ok(hr == 0x80001235, "Unexpected hr %#lx.\n", hr);

        hr = CoCreateInstance(&IID_Testiface7, NULL, CLSCTX_INPROC_SERVER | CLSCTX_APPCONTAINER,
                &IID_IUnknown, (void **)&unk);
        ok(hr == 0x80001235, "Unexpected hr %#lx.\n", hr);

        deactivate_context(handle, actctx_cookie);
    }

    CoUninitialize();
}

static void test_call_cancellation(void)
{
    HRESULT hr;

    /* Cancellation is disabled initially. */
    hr = CoDisableCallCancellation(NULL);
    ok(hr == CO_E_CANCEL_DISABLED, "Unexpected hr %#lx.\n", hr);

    hr = CoEnableCallCancellation(NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoDisableCallCancellation(NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoDisableCallCancellation(NULL);
    ok(hr == CO_E_CANCEL_DISABLED, "Unexpected hr %#lx.\n", hr);

    hr = CoEnableCallCancellation(NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Counter is not affected by initialization. */
    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoDisableCallCancellation(NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoDisableCallCancellation(NULL);
    ok(hr == CO_E_CANCEL_DISABLED, "Unexpected hr %#lx.\n", hr);

    hr = CoEnableCallCancellation(NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CoUninitialize();

    hr = CoDisableCallCancellation(NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoDisableCallCancellation(NULL);
    ok(hr == CO_E_CANCEL_DISABLED, "Unexpected hr %#lx.\n", hr);

    /* It's cumulative. */
    hr = CoEnableCallCancellation(NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoEnableCallCancellation(NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoDisableCallCancellation(NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoDisableCallCancellation(NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoDisableCallCancellation(NULL);
    ok(hr == CO_E_CANCEL_DISABLED, "Unexpected hr %#lx.\n", hr);
}

enum oletlsflags
{
    OLETLS_UUIDINITIALIZED = 0x2,
    OLETLS_DISABLE_OLE1DDE = 0x40,
    OLETLS_APARTMENTTHREADED = 0x80,
    OLETLS_MULTITHREADED = 0x100,
};

struct oletlsdata
{
    void *threadbase;
    void *smallocator;
    DWORD id;
    DWORD flags;
};

static DWORD get_oletlsflags(void)
{
    struct oletlsdata *data = NtCurrentTeb()->ReservedForOle;
    return data ? data->flags : 0;
}

static DWORD CALLBACK oletlsdata_test_thread(void *arg)
{
    IUnknown *unk;
    DWORD flags;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_InternetZoneManager, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IUnknown_Release(unk);

    /* Flag is not set for implicit MTA. */
    flags = get_oletlsflags();
    ok(!(flags & OLETLS_MULTITHREADED), "Unexpected flags %#lx.\n", flags);

    return 0;
}

static void test_oletlsdata(void)
{
    HANDLE thread;
    DWORD flags;
    HRESULT hr;
    GUID guid;

    /* STA */
    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    flags = get_oletlsflags();
    ok(flags & OLETLS_APARTMENTTHREADED && !(flags & OLETLS_DISABLE_OLE1DDE), "Unexpected flags %#lx.\n", flags);
    CoUninitialize();
    flags = get_oletlsflags();
    ok(!(flags & (OLETLS_APARTMENTTHREADED | OLETLS_MULTITHREADED | OLETLS_DISABLE_OLE1DDE)), "Unexpected flags %#lx.\n", flags);

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    flags = get_oletlsflags();
    ok(flags & OLETLS_APARTMENTTHREADED && flags & OLETLS_DISABLE_OLE1DDE, "Unexpected flags %#lx.\n", flags);
    CoUninitialize();
    flags = get_oletlsflags();
    ok(!(flags & (OLETLS_APARTMENTTHREADED | OLETLS_MULTITHREADED | OLETLS_DISABLE_OLE1DDE)), "Unexpected flags %#lx.\n", flags);

    /* MTA */
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    flags = get_oletlsflags();
    ok(flags & OLETLS_MULTITHREADED && flags & OLETLS_DISABLE_OLE1DDE, "Unexpected flags %#lx.\n", flags);

    /* Implicit case. */
    thread = CreateThread(NULL, 0, oletlsdata_test_thread, NULL, 0, &flags);
    ok(thread != NULL, "Failed to create a test thread, error %ld.\n", GetLastError());
    ok(!WaitForSingleObject(thread, 5000), "Wait timed out.\n");
    CloseHandle(thread);

    CoUninitialize();
    flags = get_oletlsflags();
    ok(!(flags & (OLETLS_APARTMENTTHREADED | OLETLS_MULTITHREADED | OLETLS_DISABLE_OLE1DDE)), "Unexpected flags %#lx.\n", flags);

    /* Thread ID. */
    flags = get_oletlsflags();
    ok(!(flags & OLETLS_UUIDINITIALIZED), "Unexpected flags %#lx.\n", flags);

    hr = CoGetCurrentLogicalThreadId(&guid);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    flags = get_oletlsflags();
    ok(flags & OLETLS_UUIDINITIALIZED && !(flags & (OLETLS_APARTMENTTHREADED | OLETLS_MULTITHREADED)),
            "Unexpected flags %#lx.\n", flags);
}

START_TEST(compobj)
{
    init_funcs();

    GetTempPathA(ARRAY_SIZE(testlib), testlib);
    SetCurrentDirectoryA(testlib);
    lstrcatA(testlib, "\\testlib.dll");
    extract_resource("testlib.dll", "TESTDLL", testlib);

    test_oletlsdata();
    test_ProgIDFromCLSID();
    test_CLSIDFromProgID();
    test_CLSIDFromString();
    test_IIDFromString();
    test_StringFromGUID2();
    test_CoCreateInstance();
    test_ole_menu();
    test_CoGetClassObject();
    test_CoCreateInstanceEx();
    test_CoRegisterMessageFilter();
    test_CoRegisterPSClsid();
    test_CoGetPSClsid();
    test_CoUnmarshalInterface();
    test_CoGetInterfaceAndReleaseStream();
    test_CoMarshalInterface();
    test_CoMarshalInterThreadInterfaceInStream();
    test_CoRegisterClassObject();
    test_registered_object_thread_affinity();
    test_CoFreeUnusedLibraries();
    test_CoGetObjectContext();
    test_CoGetCallContext();
    test_CoGetContextToken();
    test_TreatAsClass();
    test_CoInitializeEx();
    test_OleInitialize_InitCounting();
    test_OleRegGetMiscStatus();
    test_CoCreateGuid();
    test_CoWaitForMultipleHandles();
    test_CoGetMalloc();
    test_OleRegGetUserType();
    test_CoGetApartmentType();
    test_IMallocSpy();
    test_CoGetCurrentLogicalThreadId();
    test_IInitializeSpy(FALSE);
    test_IInitializeSpy(TRUE);
    test_CoGetInstanceFromFile();
    test_GlobalOptions();
    test_implicit_mta();
    test_CoGetCurrentProcess();
    test_mta_usage();
    test_CoCreateInstanceFromApp();
    test_call_cancellation();

    DeleteFileA( testlib );
}
