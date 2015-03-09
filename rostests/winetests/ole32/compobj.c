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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define CONST_VTABLE

#include <stdarg.h>
#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winreg.h>
#define USE_COM_CONTEXT_DEF
#include <initguid.h>
//#include "objbase.h"
//#include "shlguid.h"
#include <ole2.h>
#include <urlmon.h> /* for CLSID_FileProtocol */
#include <dde.h>

#include <ctxtcall.h>

#include <wine/test.h>

extern const IID GUID_NULL;

/* functions that are not present on all versions of Windows */
static HRESULT (WINAPI * pCoInitializeEx)(LPVOID lpReserved, DWORD dwCoInit);
static HRESULT (WINAPI * pCoGetObjectContext)(REFIID riid, LPVOID *ppv);
static HRESULT (WINAPI * pCoSwitchCallContext)(IUnknown *pObject, IUnknown **ppOldObject);
static HRESULT (WINAPI * pCoGetTreatAsClass)(REFCLSID clsidOld, LPCLSID pClsidNew);
static HRESULT (WINAPI * pCoTreatAsClass)(REFCLSID clsidOld, REFCLSID pClsidNew);
static HRESULT (WINAPI * pCoGetContextToken)(ULONG_PTR *token);
static LONG (WINAPI * pRegDeleteKeyExA)(HKEY, LPCSTR, REGSAM, DWORD);
static LONG (WINAPI * pRegOverridePredefKey)(HKEY key, HKEY override);

static BOOL   (WINAPI *pActivateActCtx)(HANDLE,ULONG_PTR*);
static HANDLE (WINAPI *pCreateActCtxW)(PCACTCTXW);
static BOOL   (WINAPI *pDeactivateActCtx)(DWORD,ULONG_PTR);
static BOOL   (WINAPI *pIsWow64Process)(HANDLE, LPBOOL);
static void   (WINAPI *pReleaseActCtx)(HANDLE);

#define ok_ole_success(hr, func) ok(hr == S_OK, func " failed with error 0x%08x\n", hr)
#define ok_more_than_one_lock() ok(cLocks > 0, "Number of locks should be > 0, but actually is %d\n", cLocks)
#define ok_no_locks() ok(cLocks == 0, "Number of locks should be 0, but actually is %d\n", cLocks)

static const CLSID CLSID_non_existent =   { 0x12345678, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
static const CLSID CLSID_StdFont = { 0x0be35203, 0x8f91, 0x11ce, { 0x9d, 0xe3, 0x00, 0xaa, 0x00, 0x4b, 0xb8, 0x51 } };
static const GUID IID_Testiface = { 0x22222222, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
static const GUID IID_Testiface2 = { 0x32222222, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
static const GUID IID_Testiface3 = { 0x42222222, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
static const GUID IID_Testiface4 = { 0x52222222, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
static const GUID IID_Testiface5 = { 0x62222222, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
static const GUID IID_Testiface6 = { 0x72222222, 0x1234, 0x1234, { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 } };
static const GUID IID_TestPS = { 0x66666666, 0x8888, 0x7777, { 0x66, 0x66, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 } };

DEFINE_GUID(CLSID_InProcFreeMarshaler, 0x0000033a,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);

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

static HRESULT WINAPI Test_IClassFactory_CreateInstance(
    LPCLASSFACTORY iface,
    IUnknown *pUnkOuter,
    REFIID riid,
    LPVOID *ppvObj)
{
    *ppvObj = NULL;
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
    GetFullPathNameW(path, sizeof(manifest_path)/sizeof(WCHAR), manifest_path, NULL);

    manifest_len = strlen(manifest);
    file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed: %u\n", GetLastError());
    if(file == INVALID_HANDLE_VALUE)
        return FALSE;
    WriteFile(file, manifest, manifest_len, &size, NULL);
    CloseHandle(file);

    return TRUE;
}

static HANDLE activate_context(const char *manifest, ULONG_PTR *cookie)
{
    WCHAR path[MAX_PATH];
    ACTCTXW actctx;
    HANDLE handle;
    BOOL ret;

    if (!pCreateActCtxW) return NULL;

    create_manifest_file("file.manifest", manifest);

    MultiByteToWideChar( CP_ACP, 0, "file.manifest", -1, path, MAX_PATH );
    memset(&actctx, 0, sizeof(ACTCTXW));
    actctx.cbSize = sizeof(ACTCTXW);
    actctx.lpSource = path;

    handle = pCreateActCtxW(&actctx);
    ok(handle != INVALID_HANDLE_VALUE || broken(handle == INVALID_HANDLE_VALUE) /* some old XP/2k3 versions */,
        "handle == INVALID_HANDLE_VALUE, error %u\n", GetLastError());
    if (handle == INVALID_HANDLE_VALUE)
    {
        win_skip("activation context generation failed, some tests will be skipped\n");
        handle = NULL;
    }

    ok(actctx.cbSize == sizeof(ACTCTXW), "actctx.cbSize=%d\n", actctx.cbSize);
    ok(actctx.dwFlags == 0, "actctx.dwFlags=%d\n", actctx.dwFlags);
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
        ret = pActivateActCtx(handle, cookie);
        ok(ret, "ActivateActCtx failed: %u\n", GetLastError());
    }

    return handle;
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
"    <comClass clsid=\"{0be35203-8f91-11ce-9de3-00aa004bb851}\""
"              progid=\"CustomFont\""
"              miscStatusIcon=\"recomposeonresize\""
"              miscStatusContent=\"insideout\""
"    />"
"    <comClass clsid=\"{0be35203-8f91-11ce-9de3-00aa004bb852}\""
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

DEFINE_GUID(CLSID_Testclass, 0x12345678, 0x1234, 0x1234, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0);

static void test_ProgIDFromCLSID(void)
{
    ULONG_PTR cookie = 0;
    LPWSTR progid;
    HANDLE handle;
    HRESULT hr;

    hr = ProgIDFromCLSID(&CLSID_StdFont, &progid);
    ok(hr == S_OK, "ProgIDFromCLSID failed with error 0x%08x\n", hr);
    if (hr == S_OK)
    {
        ok(!lstrcmpiW(progid, stdfont), "Didn't get expected prog ID\n");
        CoTaskMemFree(progid);
    }

    progid = (LPWSTR)0xdeadbeef;
    hr = ProgIDFromCLSID(&CLSID_non_existent, &progid);
    ok(hr == REGDB_E_CLASSNOTREG, "ProgIDFromCLSID returned %08x\n", hr);
    ok(progid == NULL, "ProgIDFromCLSID returns with progid %p\n", progid);

    hr = ProgIDFromCLSID(&CLSID_StdFont, NULL);
    ok(hr == E_INVALIDARG, "ProgIDFromCLSID should return E_INVALIDARG instead of 0x%08x\n", hr);

    if ((handle = activate_context(actctx_manifest, &cookie)))
    {
        static const WCHAR customfontW[] = {'C','u','s','t','o','m','F','o','n','t',0};

        hr = ProgIDFromCLSID(&CLSID_non_existent, &progid);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(!lstrcmpiW(progid, progidW), "got %s\n", wine_dbgstr_w(progid));
        CoTaskMemFree(progid);

        /* try something registered and redirected */
        progid = NULL;
        hr = ProgIDFromCLSID(&CLSID_StdFont, &progid);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(!lstrcmpiW(progid, customfontW), "got wrong progid %s\n", wine_dbgstr_w(progid));
        CoTaskMemFree(progid);

        /* classes without default progid, progid list is not used */
        hr = ProgIDFromCLSID(&IID_Testiface5, &progid);
        ok(hr == REGDB_E_CLASSNOTREG, "got 0x%08x\n", hr);

        hr = ProgIDFromCLSID(&IID_Testiface6, &progid);
        ok(hr == REGDB_E_CLASSNOTREG, "got 0x%08x\n", hr);

        pDeactivateActCtx(0, cookie);
        pReleaseActCtx(handle);
    }
}

static void test_CLSIDFromProgID(void)
{
    ULONG_PTR cookie = 0;
    HANDLE handle;
    CLSID clsid;
    HRESULT hr = CLSIDFromProgID(stdfont, &clsid);
    ok(hr == S_OK, "CLSIDFromProgID failed with error 0x%08x\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_StdFont), "clsid wasn't equal to CLSID_StdFont\n");

    hr = CLSIDFromString(stdfont, &clsid);
    ok_ole_success(hr, "CLSIDFromString");
    ok(IsEqualCLSID(&clsid, &CLSID_StdFont), "clsid wasn't equal to CLSID_StdFont\n");

    /* test some failure cases */

    hr = CLSIDFromProgID(wszNonExistent, NULL);
    ok(hr == E_INVALIDARG, "CLSIDFromProgID should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    hr = CLSIDFromProgID(NULL, &clsid);
    ok(hr == E_INVALIDARG, "CLSIDFromProgID should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    memset(&clsid, 0xcc, sizeof(clsid));
    hr = CLSIDFromProgID(wszNonExistent, &clsid);
    ok(hr == CO_E_CLASSSTRING, "CLSIDFromProgID on nonexistent ProgID should have returned CO_E_CLASSSTRING instead of 0x%08x\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_NULL), "CLSIDFromProgID should have set clsid to all-zeros on failure\n");

    /* fails without proper context */
    memset(&clsid, 0xcc, sizeof(clsid));
    hr = CLSIDFromProgID(progidW, &clsid);
    ok(hr == CO_E_CLASSSTRING, "got 0x%08x\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_NULL), "wrong clsid\n");

    if ((handle = activate_context(actctx_manifest, &cookie)))
    {
        GUID clsid1;

        memset(&clsid, 0xcc, sizeof(clsid));
        hr = CLSIDFromProgID(wszNonExistent, &clsid);
        ok(hr == CO_E_CLASSSTRING, "got 0x%08x\n", hr);
        ok(IsEqualCLSID(&clsid, &CLSID_NULL), "should have zero CLSID on failure\n");

        /* CLSIDFromString() doesn't check activation context */
        hr = CLSIDFromString(progidW, &clsid);
        ok(hr == CO_E_CLASSSTRING, "got 0x%08x\n", hr);

        clsid = CLSID_NULL;
        hr = CLSIDFromProgID(progidW, &clsid);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        /* it returns generated CLSID here */
        ok(!IsEqualCLSID(&clsid, &CLSID_non_existent) && !IsEqualCLSID(&clsid, &CLSID_NULL),
                 "got wrong clsid %s\n", wine_dbgstr_guid(&clsid));

        /* duplicate progid present in context - returns generated guid here too */
        clsid = CLSID_NULL;
        hr = CLSIDFromProgID(stdfont, &clsid);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        clsid1 = CLSID_StdFont;
        /* that's where it differs from StdFont */
        clsid1.Data4[7] = 0x52;
        ok(!IsEqualCLSID(&clsid, &CLSID_StdFont) && !IsEqualCLSID(&clsid, &CLSID_NULL) && !IsEqualCLSID(&clsid, &clsid1),
            "got %s\n", wine_dbgstr_guid(&clsid));

        pDeactivateActCtx(0, cookie);
        pReleaseActCtx(handle);
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
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_NULL), "clsid wasn't equal to CLSID_NULL\n");

    /* string is longer, but starts with a valid CLSID */
    memset(&clsid, 0, sizeof(clsid));
    hr = CLSIDFromString(cf_brokenW, &clsid);
    ok(hr == CO_E_CLASSSTRING, "got 0x%08x\n", hr);
    ok(IsEqualCLSID(&clsid, &IID_IClassFactory), "got %s\n", wine_dbgstr_guid(&clsid));

    lstrcpyW(wszCLSID_Broken, wszCLSID_StdFont);
    for(i = lstrlenW(wszCLSID_StdFont); i < 49; i++)
        wszCLSID_Broken[i] = 'A';
    wszCLSID_Broken[i] = '\0';

    memset(&clsid, 0, sizeof(CLSID));
    hr = CLSIDFromString(wszCLSID_Broken, &clsid);
    ok(hr == CO_E_CLASSSTRING, "Got %08x\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_StdFont), "clsid wasn't equal to CLSID_StdFont\n");

    wszCLSID_Broken[lstrlenW(wszCLSID_StdFont)-1] = 'A';
    memset(&clsid, 0, sizeof(CLSID));
    hr = CLSIDFromString(wszCLSID_Broken, &clsid);
    ok(hr == CO_E_CLASSSTRING, "Got %08x\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_StdFont), "clsid wasn't equal to CLSID_StdFont\n");

    wszCLSID_Broken[lstrlenW(wszCLSID_StdFont)] = '\0';
    memset(&clsid, 0, sizeof(CLSID));
    hr = CLSIDFromString(wszCLSID_Broken, &clsid);
    ok(hr == CO_E_CLASSSTRING, "Got %08x\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_StdFont), "clsid wasn't equal to CLSID_StdFont\n");

    wszCLSID_Broken[lstrlenW(wszCLSID_StdFont)-1] = '\0';
    memset(&clsid, 0, sizeof(CLSID));
    hr = CLSIDFromString(wszCLSID_Broken, &clsid);
    ok(hr == CO_E_CLASSSTRING, "Got %08x\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_StdFont), "clsid wasn't equal to CLSID_StdFont\n");

    memset(&clsid, 0xcc, sizeof(CLSID));
    hr = CLSIDFromString(wszCLSID_Broken+1, &clsid);
    ok(hr == CO_E_CLASSSTRING, "Got %08x\n", hr);
    ok(IsEqualCLSID(&clsid, &CLSID_NULL), "clsid wasn't equal to CLSID_NULL\n");

    wszCLSID_Broken[9] = '*';
    memset(&clsid, 0xcc, sizeof(CLSID));
    hr = CLSIDFromString(wszCLSID_Broken, &clsid);
    ok(hr == CO_E_CLASSSTRING, "Got %08x\n", hr);
    ok(clsid.Data1 == CLSID_StdFont.Data1, "Got %08x\n", clsid.Data1);
    ok(clsid.Data2 == 0xcccc, "Got %04x\n", clsid.Data2);

    wszCLSID_Broken[3] = '*';
    memset(&clsid, 0xcc, sizeof(CLSID));
    hr = CLSIDFromString(wszCLSID_Broken, &clsid);
    ok(hr == CO_E_CLASSSTRING, "Got %08x\n", hr);
    ok(clsid.Data1 == 0xb, "Got %08x\n", clsid.Data1);
    ok(clsid.Data2 == 0xcccc, "Got %04x\n", clsid.Data2);

    wszCLSID_Broken[3] = '\0';
    memset(&clsid, 0xcc, sizeof(CLSID));
    hr = CLSIDFromString(wszCLSID_Broken, &clsid);
    ok(hr == CO_E_CLASSSTRING, "Got %08x\n", hr);
    ok(clsid.Data1 == 0xb, "Got %08x\n", clsid.Data1);
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
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(IsEqualIID(&iid, &CLSID_StdFont), "got iid %s\n", wine_dbgstr_guid(&iid));

    memset(&iid, 0xab, sizeof(iid));
    hr = IIDFromString(NULL, &iid);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(IsEqualIID(&iid, &CLSID_NULL), "got iid %s\n", wine_dbgstr_guid(&iid));

    hr = IIDFromString(cfW, &iid);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(IsEqualIID(&iid, &IID_IClassFactory), "got iid %s\n", wine_dbgstr_guid(&iid));

    /* string starts with a valid IID but is longer */
    memset(&iid, 0xab, sizeof(iid));
    hr = IIDFromString(cf_brokenW, &iid);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    ok(iid.Data1 == 0xabababab, "Got %08x\n", iid.Data1);

    /* invalid IID in a valid format */
    memset(&iid, 0xab, sizeof(iid));
    hr = IIDFromString(brokenW, &iid);
    ok(hr == CO_E_IIDSTRING, "got 0x%08x\n", hr);
    ok(iid.Data1 == 0x00000001, "Got %08x\n", iid.Data1);

    memset(&iid, 0xab, sizeof(iid));
    hr = IIDFromString(broken2W, &iid);
    ok(hr == CO_E_IIDSTRING, "got 0x%08x\n", hr);
    ok(iid.Data1 == 0x00000001, "Got %08x\n", iid.Data1);

    /* format is broken, but string length is okay */
    memset(&iid, 0xab, sizeof(iid));
    hr = IIDFromString(broken3W, &iid);
    ok(hr == CO_E_IIDSTRING, "got 0x%08x\n", hr);
    ok(iid.Data1 == 0xabababab, "Got %08x\n", iid.Data1);

    /* invalid string */
    memset(&iid, 0xab, sizeof(iid));
    hr = IIDFromString(wszNonExistent, &iid);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    ok(iid.Data1 == 0xabababab, "Got %08x\n", iid.Data1);

    /* valid ProgID */
    memset(&iid, 0xab, sizeof(iid));
    hr = IIDFromString(stdfont, &iid);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    ok(iid.Data1 == 0xabababab, "Got %08x\n", iid.Data1);
}

static void test_StringFromGUID2(void)
{
  WCHAR str[50];
  int len;

  /* invalid pointer */
  SetLastError(0xdeadbeef);
  len = StringFromGUID2(NULL,str,50);
  ok(len == 0, "len: %d (expected 0)\n", len);
  ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %x\n", GetLastError());

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

struct info
{
    HANDLE wait, stop;
};

static DWORD CALLBACK ole_initialize_thread(LPVOID pv)
{
    HRESULT hr;
    struct info *info = pv;

    hr = pCoInitializeEx(NULL, COINIT_MULTITHREADED);

    SetEvent(info->wait);
    WaitForSingleObject(info->stop, 10000);

    CoUninitialize();
    return hr;
}

static void test_CoCreateInstance(void)
{
    HRESULT hr;
    HANDLE thread;
    DWORD tid, exitcode;
    IUnknown *pUnk;
    struct info info;
    REFCLSID rclsid = &CLSID_InternetZoneManager;

    pUnk = (IUnknown *)0xdeadbeef;
    hr = CoCreateInstance(rclsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&pUnk);
    ok(hr == CO_E_NOTINITIALIZED, "CoCreateInstance should have returned CO_E_NOTINITIALIZED instead of 0x%08x\n", hr);
    ok(pUnk == NULL, "CoCreateInstance should have changed the passed in pointer to NULL, instead of %p\n", pUnk);

    OleInitialize(NULL);

    /* test errors returned for non-registered clsids */
    hr = CoCreateInstance(&CLSID_non_existent, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&pUnk);
    ok(hr == REGDB_E_CLASSNOTREG, "CoCreateInstance for non-registered inproc server should have returned REGDB_E_CLASSNOTREG instead of 0x%08x\n", hr);
    hr = CoCreateInstance(&CLSID_non_existent, NULL, CLSCTX_INPROC_HANDLER, &IID_IUnknown, (void **)&pUnk);
    ok(hr == REGDB_E_CLASSNOTREG, "CoCreateInstance for non-registered inproc handler should have returned REGDB_E_CLASSNOTREG instead of 0x%08x\n", hr);
    hr = CoCreateInstance(&CLSID_non_existent, NULL, CLSCTX_LOCAL_SERVER, &IID_IUnknown, (void **)&pUnk);
    ok(hr == REGDB_E_CLASSNOTREG, "CoCreateInstance for non-registered local server should have returned REGDB_E_CLASSNOTREG instead of 0x%08x\n", hr);
    hr = CoCreateInstance(&CLSID_non_existent, NULL, CLSCTX_REMOTE_SERVER, &IID_IUnknown, (void **)&pUnk);
    ok(hr == REGDB_E_CLASSNOTREG, "CoCreateInstance for non-registered remote server should have returned REGDB_E_CLASSNOTREG instead of 0x%08x\n", hr);

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
    ok(hr == CO_E_NOTINITIALIZED, "CoCreateInstance should have returned CO_E_NOTINITIALIZED instead of 0x%08x\n", hr);

    /* show that COM doesn't have to be initialized for multi-threaded apartments if another
       thread has already done so */

    info.wait = CreateEventA(NULL, TRUE, FALSE, NULL);
    ok(info.wait != NULL, "CreateEvent failed with error %d\n", GetLastError());

    info.stop = CreateEventA(NULL, TRUE, FALSE, NULL);
    ok(info.stop != NULL, "CreateEvent failed with error %d\n", GetLastError());

    thread = CreateThread(NULL, 0, ole_initialize_thread, &info, 0, &tid);
    ok(thread != NULL, "CreateThread failed with error %d\n", GetLastError());

    ok( !WaitForSingleObject(info.wait, 10000 ), "wait timed out\n" );

    pUnk = (IUnknown *)0xdeadbeef;
    hr = CoCreateInstance(rclsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&pUnk);
    ok(hr == S_OK, "CoCreateInstance should have returned S_OK instead of 0x%08x\n", hr);
    if (pUnk) IUnknown_Release(pUnk);

    SetEvent(info.stop);
    ok( !WaitForSingleObject(thread, 10000), "wait timed out\n" );

    GetExitCodeThread(thread, &exitcode);
    hr = exitcode;
    ok(hr == S_OK, "thread should have returned S_OK instead of 0x%08x\n", hr);

    CloseHandle(thread);
    CloseHandle(info.wait);
    CloseHandle(info.stop);
}

static void test_CoGetClassObject(void)
{
    HRESULT hr;
    HANDLE thread, handle;
    DWORD tid, exitcode;
    ULONG_PTR cookie;
    IUnknown *pUnk;
    struct info info;
    REFCLSID rclsid = &CLSID_InternetZoneManager;
    HKEY hkey;
    LONG res;

    hr = CoGetClassObject(rclsid, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void **)&pUnk);
    ok(hr == CO_E_NOTINITIALIZED, "CoGetClassObject should have returned CO_E_NOTINITIALIZED instead of 0x%08x\n", hr);
    ok(pUnk == NULL, "CoGetClassObject should have changed the passed in pointer to NULL, instead of %p\n", pUnk);

    hr = CoGetClassObject(rclsid, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, NULL);
    ok(hr == E_INVALIDARG ||
       broken(hr == CO_E_NOTINITIALIZED), /* win9x */
       "CoGetClassObject should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    /* show that COM doesn't have to be initialized for multi-threaded apartments if another
       thread has already done so */

    info.wait = CreateEventA(NULL, TRUE, FALSE, NULL);
    ok(info.wait != NULL, "CreateEvent failed with error %d\n", GetLastError());

    info.stop = CreateEventA(NULL, TRUE, FALSE, NULL);
    ok(info.stop != NULL, "CreateEvent failed with error %d\n", GetLastError());

    thread = CreateThread(NULL, 0, ole_initialize_thread, &info, 0, &tid);
    ok(thread != NULL, "CreateThread failed with error %d\n", GetLastError());

    ok( !WaitForSingleObject(info.wait, 10000), "wait timed out\n" );

    pUnk = (IUnknown *)0xdeadbeef;
    hr = CoGetClassObject(rclsid, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void **)&pUnk);
    if(hr == REGDB_E_CLASSNOTREG)
        skip("IE not installed so can't test CoGetClassObject\n");
    else
    {
        ok(hr == S_OK, "CoGetClassObject should have returned S_OK instead of 0x%08x\n", hr);
        if (pUnk) IUnknown_Release(pUnk);
    }

    SetEvent(info.stop);
    ok( !WaitForSingleObject(thread, 10000), "wait timed out\n" );

    GetExitCodeThread(thread, &exitcode);
    hr = exitcode;
    ok(hr == S_OK, "thread should have returned S_OK instead of 0x%08x\n", hr);

    CloseHandle(thread);
    CloseHandle(info.wait);
    CloseHandle(info.stop);

    if (!pRegOverridePredefKey)
    {
        win_skip("RegOverridePredefKey not available\n");
        return;
    }

    pCoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CoGetClassObject(rclsid, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void **)&pUnk);
    if (hr == S_OK)
    {
        IUnknown_Release(pUnk);

        res = RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Classes", 0, NULL, 0,
                             KEY_ALL_ACCESS, NULL, &hkey, NULL);
        ok(!res, "RegCreateKeyEx returned %d\n", res);

        res = pRegOverridePredefKey(HKEY_CLASSES_ROOT, hkey);
        ok(!res, "RegOverridePredefKey returned %d\n", res);

        hr = CoGetClassObject(rclsid, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void **)&pUnk);
        ok(hr == S_OK, "CoGetClassObject should have returned S_OK instead of 0x%08x\n", hr);

        res = pRegOverridePredefKey(HKEY_CLASSES_ROOT, NULL);
        ok(!res, "RegOverridePredefKey returned %d\n", res);

        if (hr == S_OK) IUnknown_Release(pUnk);
        RegCloseKey(hkey);
    }

    hr = CoGetClassObject(&CLSID_InProcFreeMarshaler, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void **)&pUnk);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IUnknown_Release(pUnk);

    /* context redefines FreeMarshaler CLSID */
    if ((handle = activate_context(actctx_manifest, &cookie)))
    {
        hr = CoGetClassObject(&CLSID_InProcFreeMarshaler, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void **)&pUnk);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        IUnknown_Release(pUnk);

        pDeactivateActCtx(0, cookie);
        pReleaseActCtx(handle);
    }

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
        IsEqualGUID(riid, &IID_IClassFactory))
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
        "CoRegisterMessageFilter should have failed with CO_E_NOT_SUPPORTED instead of 0x%08x\n",
        hr);

    pCoInitializeEx(NULL, COINIT_MULTITHREADED);
    prev_filter = (IMessageFilter *)0xdeadbeef;
    hr = CoRegisterMessageFilter(&MessageFilter, &prev_filter);
    ok(hr == CO_E_NOT_SUPPORTED,
        "CoRegisterMessageFilter should have failed with CO_E_NOT_SUPPORTED instead of 0x%08x\n",
        hr);
    ok(prev_filter == (IMessageFilter *)0xdeadbeef,
        "prev_filter should have been set to %p\n", prev_filter);
    CoUninitialize();

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

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

static HRESULT WINAPI Test_IUnknown_QueryInterface(
    IUnknown *iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IWineTest))
    {
        *ppvObj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
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
    return E_NOTIMPL;
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

static void test_CoRegisterPSClsid(void)
{
    HRESULT hr;
    DWORD dwRegistrationKey;
    IStream *stream;
    CLSID clsid;

    hr = CoRegisterPSClsid(&IID_IWineTest, &CLSID_WineTestPSFactoryBuffer);
    ok(hr == CO_E_NOTINITIALIZED, "CoRegisterPSClsid should have returned CO_E_NOTINITIALIZED instead of 0x%08x\n", hr);

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoRegisterClassObject(&CLSID_WineTestPSFactoryBuffer, (IUnknown *)&PSFactoryBuffer,
        CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &dwRegistrationKey);
    ok_ole_success(hr, "CoRegisterClassObject");

    hr = CoRegisterPSClsid(&IID_IWineTest, &CLSID_WineTestPSFactoryBuffer);
    ok_ole_success(hr, "CoRegisterPSClsid");

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok_ole_success(hr, "CreateStreamOnHGlobal");

    hr = CoMarshalInterface(stream, &IID_IWineTest, &Test_Unknown, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == E_NOTIMPL, "CoMarshalInterface should have returned E_NOTIMPL instead of 0x%08x\n", hr);
    IStream_Release(stream);

    hr = CoRevokeClassObject(dwRegistrationKey);
    ok_ole_success(hr, "CoRevokeClassObject");

    CoUninitialize();

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoGetPSClsid(&IID_IWineTest, &clsid);
    ok(hr == REGDB_E_IIDNOTREG, "CoGetPSClsid should have returned REGDB_E_IIDNOTREG instead of 0x%08x\n", hr);

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
       "CoGetPSClsid should have returned CO_E_NOTINITIALIZED instead of 0x%08x\n",
       hr);

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoGetPSClsid(&IID_IClassFactory, &clsid);
    ok_ole_success(hr, "CoGetPSClsid");

    hr = CoGetPSClsid(&IID_IWineTest, &clsid);
    ok(hr == REGDB_E_IIDNOTREG,
       "CoGetPSClsid for random IID returned 0x%08x instead of REGDB_E_IIDNOTREG\n",
       hr);

    hr = CoGetPSClsid(&IID_IClassFactory, NULL);
    ok(hr == E_INVALIDARG,
       "CoGetPSClsid for null clsid returned 0x%08x instead of E_INVALIDARG\n",
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
    ok(!res, "RegCreateKeyEx returned %d\n", res);

    res = pRegOverridePredefKey(HKEY_CLASSES_ROOT, hkey);
    ok(!res, "RegOverridePredefKey returned %d\n", res);

    hr = CoGetPSClsid(&IID_IClassFactory, &clsid);
    ok_ole_success(hr, "CoGetPSClsid");

    res = pRegOverridePredefKey(HKEY_CLASSES_ROOT, NULL);
    ok(!res, "RegOverridePredefKey returned %d\n", res);

    RegCloseKey(hkey);

    /* not registered CLSID */
    hr = CoGetPSClsid(&IID_Testiface, &clsid);
    ok(hr == REGDB_E_IIDNOTREG, "got 0x%08x\n", hr);

    if ((handle = activate_context(actctx_manifest, &cookie)))
    {
        memset(&clsid, 0, sizeof(clsid));
        hr = CoGetPSClsid(&IID_Testiface, &clsid);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(IsEqualGUID(&clsid, &IID_Testiface), "got clsid %s\n", wine_dbgstr_guid(&clsid));

        memset(&clsid, 0, sizeof(clsid));
        hr = CoGetPSClsid(&IID_Testiface2, &clsid);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(IsEqualGUID(&clsid, &IID_Testiface2), "got clsid %s\n", wine_dbgstr_guid(&clsid));

        memset(&clsid, 0, sizeof(clsid));
        hr = CoGetPSClsid(&IID_Testiface3, &clsid);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(IsEqualGUID(&clsid, &IID_TestPS), "got clsid %s\n", wine_dbgstr_guid(&clsid));

        memset(&clsid, 0xaa, sizeof(clsid));
        hr = CoGetPSClsid(&IID_Testiface4, &clsid);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(IsEqualGUID(&clsid, &GUID_NULL), "got clsid %s\n", wine_dbgstr_guid(&clsid));

        /* register same interface and try to get CLSID back */
        hr = CoRegisterPSClsid(&IID_Testiface, &IID_Testiface4);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        memset(&clsid, 0, sizeof(clsid));
        hr = CoGetPSClsid(&IID_Testiface, &clsid);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(IsEqualGUID(&clsid, &IID_Testiface4), "got clsid %s\n", wine_dbgstr_guid(&clsid));

        pDeactivateActCtx(0, cookie);
        pReleaseActCtx(handle);
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
        ok(hr == REGDB_E_IIDNOTREG, "got 0x%08x\n", hr);

        res = RegCreateKeyExA(HKEY_CLASSES_ROOT, "Interface",
                              0, NULL, 0, KEY_ALL_ACCESS | opposite, NULL, &hkey_iface, NULL);
        ok(!res, "RegCreateKeyEx returned %d\n", res);
        res = RegCreateKeyExA(hkey_iface, clsidDeadBeef,
                              0, NULL, 0, KEY_ALL_ACCESS | opposite, NULL, &hkey, NULL);
        ok(!res, "RegCreateKeyEx returned %d\n", res);
        res = RegCreateKeyExA(hkey, "ProxyStubClsid32",
                              0, NULL, 0, KEY_ALL_ACCESS | opposite, NULL, &hkey_psclsid, NULL);
        ok(!res, "RegCreateKeyEx returned %d\n", res);
        res = RegSetValueExA(hkey_psclsid, NULL, 0, REG_SZ, (const BYTE *)clsidA, strlen(clsidA)+1);
        ok(!res, "RegSetValueEx returned %d\n", res);
        RegCloseKey(hkey_psclsid);

        hr = CoGetPSClsid(&IID_DeadBeef, &clsid);
        ok_ole_success(hr, "CoGetPSClsid");
        ok(IsEqualGUID(&clsid, &IID_TestPS), "got clsid %s\n", wine_dbgstr_guid(&clsid));

        res = pRegDeleteKeyExA(hkey, "ProxyStubClsid32", opposite, 0);
        ok(!res, "RegDeleteKeyEx returned %d\n", res);
        RegCloseKey(hkey);
        res = pRegDeleteKeyExA(hkey_iface, clsidDeadBeef, opposite, 0);
        ok(!res, "RegDeleteKeyEx returned %d\n", res);
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
    ok(hr == E_INVALIDARG, "CoUnmarshalInterface should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, "CreateStreamOnHGlobal");

    hr = CoUnmarshalInterface(pStream, &IID_IUnknown, (void **)&pProxy);
    todo_wine
    ok(hr == CO_E_NOTINITIALIZED, "CoUnmarshalInterface should have returned CO_E_NOTINITIALIZED instead of 0x%08x\n", hr);

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoUnmarshalInterface(pStream, &IID_IUnknown, (void **)&pProxy);
    ok(hr == STG_E_READFAULT, "CoUnmarshalInterface should have returned STG_E_READFAULT instead of 0x%08x\n", hr);

    CoUninitialize();

    hr = CoUnmarshalInterface(pStream, &IID_IUnknown, NULL);
    ok(hr == E_INVALIDARG, "CoUnmarshalInterface should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    IStream_Release(pStream);
}

static void test_CoGetInterfaceAndReleaseStream(void)
{
    HRESULT hr;
    IUnknown *pUnk;

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoGetInterfaceAndReleaseStream(NULL, &IID_IUnknown, (void**)&pUnk);
    ok(hr == E_INVALIDARG, "hr %08x\n", hr);

    CoUninitialize();
}

/* basic test, mainly for invalid arguments. see marshal.c for more */
static void test_CoMarshalInterface(void)
{
    IStream *pStream;
    HRESULT hr;
    static const LARGE_INTEGER llZero;

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, "CreateStreamOnHGlobal");

    hr = CoMarshalInterface(pStream, &IID_IUnknown, NULL, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == E_INVALIDARG, "CoMarshalInterface should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    hr = CoMarshalInterface(NULL, &IID_IUnknown, (IUnknown *)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok(hr == E_INVALIDARG, "CoMarshalInterface should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    hr = CoMarshalInterface(pStream, &IID_IUnknown, (IUnknown *)&Test_ClassFactory, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    ok_ole_success(hr, "CoMarshalInterface");

    /* stream not rewound */
    hr = CoReleaseMarshalData(pStream);
    ok(hr == STG_E_READFAULT, "CoReleaseMarshalData should have returned STG_E_READFAULT instead of 0x%08x\n", hr);

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

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    cLocks = 0;

    hr = CoMarshalInterThreadInterfaceInStream(&IID_IUnknown, (IUnknown *)&Test_ClassFactory, NULL);
    ok(hr == E_INVALIDARG, "CoMarshalInterThreadInterfaceInStream should have returned E_INVALIDARG instead of 0x%08x\n", hr);

    hr = CoMarshalInterThreadInterfaceInStream(&IID_IUnknown, NULL, &pStream);
    ok(hr == E_INVALIDARG, "CoMarshalInterThreadInterfaceInStream should have returned E_INVALIDARG instead of 0x%08x\n", hr);

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

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

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
    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoGetClassObject(&CLSID_WineOOPTest, CLSCTX_INPROC_SERVER, NULL,
                          &IID_IClassFactory, (void **)&pcf);
    ok(hr == REGDB_E_CLASSNOTREG, "object registered in an apartment shouldn't accessible after it is destroyed\n");

    /* crashes with at least win9x DCOM! */
    if (0)
        CoRevokeClassObject(cookie);

    /* test that object is accessible */
    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory, CLSCTX_INPROC_SERVER,
        REGCLS_MULTIPLEUSE, &cookie);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = CoGetClassObject(&CLSID_WineOOPTest, CLSCTX_INPROC_SERVER, NULL, &IID_IClassFactory, (void**)&pcf);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IClassFactory_Release(pcf);

    /* context now contains CLSID_WineOOPTest, test if registered one could still be used */
    if ((handle = activate_context(actctx_manifest, &ctxcookie)))
    {
        hr = CoGetClassObject(&CLSID_WineOOPTest, CLSCTX_INPROC_SERVER, NULL, &IID_IClassFactory, (void**)&pcf);
todo_wine
        ok(hr == HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND), "got 0x%08x\n", hr);

        pDeactivateActCtx(0, ctxcookie);
        pReleaseActCtx(handle);
    }

    hr = CoGetClassObject(&CLSID_WineOOPTest, CLSCTX_INPROC_SERVER, NULL, &IID_IClassFactory, (void**)&pcf);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IClassFactory_Release(pcf);

    hr = CoRevokeClassObject(cookie);
    ok(hr == S_OK, "got 0x%08x\n", hr);

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

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

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

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

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

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_INPROC_SERVER, REGCLS_SINGLEUSE, &cookie);

    CoUninitialize();

    return hr;
}

static DWORD CALLBACK revoke_class_object_thread(LPVOID pv)
{
    DWORD cookie = (DWORD_PTR)pv;
    HRESULT hr;

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

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

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    /* CLSCTX_INPROC_SERVER */

    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_INPROC_SERVER, REGCLS_SINGLEUSE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");

    thread = CreateThread(NULL, 0, get_class_object_thread, (LPVOID)CLSCTX_INPROC_SERVER, 0, &tid);
    ok(thread != NULL, "CreateThread failed with error %d\n", GetLastError());
    ok( !WaitForSingleObject(thread, 10000), "wait timed out\n" );
    GetExitCodeThread(thread, &exitcode);
    hr = exitcode;
    ok(hr == REGDB_E_CLASSNOTREG, "CoGetClassObject on inproc object "
       "registered in different thread should return REGDB_E_CLASSNOTREG "
       "instead of 0x%08x\n", hr);

    hr = get_class_object(CLSCTX_INPROC_SERVER);
    ok(hr == S_OK, "CoGetClassObject on inproc object registered in same "
       "thread should return S_OK instead of 0x%08x\n", hr);

    thread = CreateThread(NULL, 0, register_class_object_thread, NULL, 0, &tid);
    ok(thread != NULL, "CreateThread failed with error %d\n", GetLastError());
    ok ( !WaitForSingleObject(thread, 10000), "wait timed out\n" );
    GetExitCodeThread(thread, &exitcode);
    hr = exitcode;
    ok(hr == S_OK, "CoRegisterClassObject with same CLSID but in different thread should return S_OK instead of 0x%08x\n", hr);

    hr = CoRevokeClassObject(cookie);
    ok_ole_success(hr, "CoRevokeClassObject");

    /* CLSCTX_LOCAL_SERVER */

    hr = CoRegisterClassObject(&CLSID_WineOOPTest, (IUnknown *)&Test_ClassFactory,
                               CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &cookie);
    ok_ole_success(hr, "CoRegisterClassObject");

    thread = CreateThread(NULL, 0, get_class_object_proxy_thread, (LPVOID)CLSCTX_LOCAL_SERVER, 0, &tid);
    ok(thread != NULL, "CreateThread failed with error %d\n", GetLastError());
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
       "instead of 0x%08x\n", hr);

    hr = get_class_object(CLSCTX_LOCAL_SERVER);
    ok(hr == S_OK, "CoGetClassObject on local server object registered in same "
       "thread should return S_OK instead of 0x%08x\n", hr);

    thread = CreateThread(NULL, 0, revoke_class_object_thread, (LPVOID)(DWORD_PTR)cookie, 0, &tid);
    ok(thread != NULL, "CreateThread failed with error %d\n", GetLastError());
    ok( !WaitForSingleObject(thread, 10000), "wait timed out\n" );
    GetExitCodeThread(thread, &exitcode);
    hr = exitcode;
    ok(hr == RPC_E_WRONG_THREAD, "CoRevokeClassObject called from different "
       "thread to where registered should return RPC_E_WRONG_THREAD instead of 0x%08x\n", hr);

    thread = CreateThread(NULL, 0, register_class_object_thread, NULL, 0, &tid);
    ok(thread != NULL, "CreateThread failed with error %d\n", GetLastError());
    ok( !WaitForSingleObject(thread, 10000), "wait timed out\n" );
    GetExitCodeThread(thread, &exitcode);
    hr = exitcode;
    ok(hr == S_OK, "CoRegisterClassObject with same CLSID but in different "
        "thread should return S_OK instead of 0x%08x\n", hr);

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

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

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
    IComThreadingInfo *pComThreadingInfo;
    IContextCallback *pContextCallback;
    IObjContext *pObjContext;
    APTTYPE apttype;
    THDTYPE thdtype;
    struct info info;
    HANDLE thread;
    DWORD tid, exitcode;

    if (!pCoGetObjectContext)
    {
        skip("CoGetObjectContext not present\n");
        return;
    }

    hr = pCoGetObjectContext(&IID_IComThreadingInfo, (void **)&pComThreadingInfo);
    ok(hr == CO_E_NOTINITIALIZED, "CoGetObjectContext should have returned CO_E_NOTINITIALIZED instead of 0x%08x\n", hr);
    ok(pComThreadingInfo == NULL, "pComThreadingInfo should have been set to NULL\n");

    /* show that COM doesn't have to be initialized for multi-threaded apartments if another
       thread has already done so */

    info.wait = CreateEventA(NULL, TRUE, FALSE, NULL);
    ok(info.wait != NULL, "CreateEvent failed with error %d\n", GetLastError());

    info.stop = CreateEventA(NULL, TRUE, FALSE, NULL);
    ok(info.stop != NULL, "CreateEvent failed with error %d\n", GetLastError());

    thread = CreateThread(NULL, 0, ole_initialize_thread, &info, 0, &tid);
    ok(thread != NULL, "CreateThread failed with error %d\n", GetLastError());

    ok( !WaitForSingleObject(info.wait, 10000), "wait timed out\n" );

    pComThreadingInfo = NULL;
    hr = pCoGetObjectContext(&IID_IComThreadingInfo, (void **)&pComThreadingInfo);
    ok(hr == S_OK, "Expected S_OK, got 0x%08x\n", hr);
    IComThreadingInfo_Release(pComThreadingInfo);

    SetEvent(info.stop);
    ok( !WaitForSingleObject(thread, 10000), "wait timed out\n" );

    GetExitCodeThread(thread, &exitcode);
    hr = exitcode;
    ok(hr == S_OK, "thread should have returned S_OK instead of 0x%08x\n", hr);

    CloseHandle(thread);
    CloseHandle(info.wait);
    CloseHandle(info.stop);

    pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = pCoGetObjectContext(&IID_IComThreadingInfo, (void **)&pComThreadingInfo);
    ok_ole_success(hr, "CoGetObjectContext");

    hr = IComThreadingInfo_GetCurrentApartmentType(pComThreadingInfo, &apttype);
    ok_ole_success(hr, "IComThreadingInfo_GetCurrentApartmentType");
    ok(apttype == APTTYPE_MAINSTA, "apartment type should be APTTYPE_MAINSTA instead of %d\n", apttype);

    hr = IComThreadingInfo_GetCurrentThreadType(pComThreadingInfo, &thdtype);
    ok_ole_success(hr, "IComThreadingInfo_GetCurrentThreadType");
    ok(thdtype == THDTYPE_PROCESSMESSAGES, "thread type should be THDTYPE_PROCESSMESSAGES instead of %d\n", thdtype);

    refs = IComThreadingInfo_Release(pComThreadingInfo);
    ok(refs == 0, "pComThreadingInfo should have 0 refs instead of %d refs\n", refs);

    hr = pCoGetObjectContext(&IID_IContextCallback, (void **)&pContextCallback);
    ok_ole_success(hr, "CoGetObjectContext(ContextCallback)");

    if (hr == S_OK)
    {
        refs = IContextCallback_Release(pContextCallback);
        ok(refs == 0, "pContextCallback should have 0 refs instead of %d refs\n", refs);
    }

    CoUninitialize();

    pCoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = pCoGetObjectContext(&IID_IComThreadingInfo, (void **)&pComThreadingInfo);
    ok_ole_success(hr, "CoGetObjectContext");

    hr = IComThreadingInfo_GetCurrentApartmentType(pComThreadingInfo, &apttype);
    ok_ole_success(hr, "IComThreadingInfo_GetCurrentApartmentType");
    ok(apttype == APTTYPE_MTA, "apartment type should be APTTYPE_MTA instead of %d\n", apttype);

    hr = IComThreadingInfo_GetCurrentThreadType(pComThreadingInfo, &thdtype);
    ok_ole_success(hr, "IComThreadingInfo_GetCurrentThreadType");
    ok(thdtype == THDTYPE_BLOCKMESSAGES, "thread type should be THDTYPE_BLOCKMESSAGES instead of %d\n", thdtype);

    refs = IComThreadingInfo_Release(pComThreadingInfo);
    ok(refs == 0, "pComThreadingInfo should have 0 refs instead of %d refs\n", refs);

    hr = pCoGetObjectContext(&IID_IContextCallback, (void **)&pContextCallback);
    ok_ole_success(hr, "CoGetObjectContext(ContextCallback)");

    if (hr == S_OK)
    {
        refs = IContextCallback_Release(pContextCallback);
        ok(refs == 0, "pContextCallback should have 0 refs instead of %d refs\n", refs);
    }

    hr = pCoGetObjectContext(&IID_IObjContext, (void **)&pObjContext);
    ok_ole_success(hr, "CoGetObjectContext");

    refs = IObjContext_Release(pObjContext);
    ok(refs == 0, "pObjContext should have 0 refs instead of %d refs\n", refs);

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
        HeapFree(GetProcessHeap(), 0, This);
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

    test_object = HeapAlloc(GetProcessHeap(), 0, sizeof(Test_CallContext));
    test_object->IUnknown_iface.lpVtbl = &TestCallContext_Vtbl;
    test_object->refs = 1;

    hr = CoGetCallContext(&IID_IUnknown, (void**)&pUnk);
    ok(hr == RPC_E_CALL_COMPLETE, "Expected RPC_E_CALL_COMPLETE, got 0x%08x\n", hr);

    pUnk = (IUnknown*)0xdeadbeef;
    hr = pCoSwitchCallContext(&test_object->IUnknown_iface, &pUnk);
    ok_ole_success(hr, "CoSwitchCallContext");
    ok(pUnk == NULL, "expected NULL, got %p\n", pUnk);
    refs = IUnknown_AddRef(&test_object->IUnknown_iface);
    ok(refs == 2, "Expected refcount 2, got %d\n", refs);
    IUnknown_Release(&test_object->IUnknown_iface);

    pUnk = (IUnknown*)0xdeadbeef;
    hr = CoGetCallContext(&IID_IUnknown, (void**)&pUnk);
    ok_ole_success(hr, "CoGetCallContext");
    ok(pUnk == &test_object->IUnknown_iface, "expected %p, got %p\n",
       &test_object->IUnknown_iface, pUnk);
    refs = IUnknown_AddRef(&test_object->IUnknown_iface);
    ok(refs == 3, "Expected refcount 3, got %d\n", refs);
    IUnknown_Release(&test_object->IUnknown_iface);
    IUnknown_Release(pUnk);

    pUnk = (IUnknown*)0xdeadbeef;
    hr = pCoSwitchCallContext(NULL, &pUnk);
    ok_ole_success(hr, "CoSwitchCallContext");
    ok(pUnk == &test_object->IUnknown_iface, "expected %p, got %p\n",
       &test_object->IUnknown_iface, pUnk);
    refs = IUnknown_AddRef(&test_object->IUnknown_iface);
    ok(refs == 2, "Expected refcount 2, got %d\n", refs);
    IUnknown_Release(&test_object->IUnknown_iface);

    hr = CoGetCallContext(&IID_IUnknown, (void**)&pUnk);
    ok(hr == RPC_E_CALL_COMPLETE, "Expected RPC_E_CALL_COMPLETE, got 0x%08x\n", hr);

    IUnknown_Release(&test_object->IUnknown_iface);

    CoUninitialize();
}

static void test_CoGetContextToken(void)
{
    HRESULT hr;
    ULONG refs;
    ULONG_PTR token;
    IObjContext *ctx;
    struct info info;
    HANDLE thread;
    DWORD tid, exitcode;

    if (!pCoGetContextToken)
    {
        win_skip("CoGetContextToken not present\n");
        return;
    }

    token = 0xdeadbeef;
    hr = pCoGetContextToken(&token);
    ok(hr == CO_E_NOTINITIALIZED, "Expected CO_E_NOTINITIALIZED, got 0x%08x\n", hr);
    ok(token == 0xdeadbeef, "Expected 0, got 0x%lx\n", token);

    /* show that COM doesn't have to be initialized for multi-threaded apartments if another
       thread has already done so */

    info.wait = CreateEventA(NULL, TRUE, FALSE, NULL);
    ok(info.wait != NULL, "CreateEvent failed with error %d\n", GetLastError());

    info.stop = CreateEventA(NULL, TRUE, FALSE, NULL);
    ok(info.stop != NULL, "CreateEvent failed with error %d\n", GetLastError());

    thread = CreateThread(NULL, 0, ole_initialize_thread, &info, 0, &tid);
    ok(thread != NULL, "CreateThread failed with error %d\n", GetLastError());

    ok( !WaitForSingleObject(info.wait, 10000), "wait timed out\n" );

    token = 0;
    hr = pCoGetContextToken(&token);
    ok(hr == S_OK, "Expected S_OK, got 0x%08x\n", hr);

    SetEvent(info.stop);
    ok( !WaitForSingleObject(thread, 10000), "wait timed out\n" );

    GetExitCodeThread(thread, &exitcode);
    hr = exitcode;
    ok(hr == S_OK, "thread should have returned S_OK instead of 0x%08x\n", hr);

    CloseHandle(thread);
    CloseHandle(info.wait);
    CloseHandle(info.stop);

    CoInitialize(NULL);

    hr = pCoGetContextToken(NULL);
    ok(hr == E_POINTER, "Expected E_POINTER, got 0x%08x\n", hr);

    token = 0;
    hr = pCoGetContextToken(&token);
    ok(hr == S_OK, "Expected S_OK, got 0x%08x\n", hr);
    ok(token, "Expected token != 0\n");

    refs = IUnknown_AddRef((IUnknown *)token);
    todo_wine ok(refs == 1, "Expected 1, got %u\n", refs);

    hr = pCoGetObjectContext(&IID_IObjContext, (void **)&ctx);
    ok(hr == S_OK, "Expected S_OK, got 0x%08x\n", hr);
    todo_wine ok(ctx == (IObjContext *)token, "Expected interface pointers to be the same\n");

    refs = IObjContext_AddRef(ctx);
    todo_wine ok(refs == 3, "Expected 3, got %u\n", refs);

    refs = IObjContext_Release(ctx);
    todo_wine ok(refs == 2, "Expected 2, got %u\n", refs);

    refs = IUnknown_Release((IUnknown *)token);
    ok(refs == 1, "Expected 1, got %u\n", refs);

    /* CoGetContextToken does not add a reference */
    token = 0;
    hr = pCoGetContextToken(&token);
    ok(hr == S_OK, "Expected S_OK, got 0x%08x\n", hr);
    ok(token, "Expected token != 0\n");
    todo_wine ok(ctx == (IObjContext *)token, "Expected interface pointers to be the same\n");

    refs = IObjContext_AddRef(ctx);
    ok(refs == 2, "Expected 1, got %u\n", refs);

    refs = IObjContext_Release(ctx);
    ok(refs == 1, "Expected 0, got %u\n", refs);

    refs = IObjContext_Release(ctx);
    ok(refs == 0, "Expected 0, got %u\n", refs);

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

    if (!pCoGetTreatAsClass)
    {
        win_skip("CoGetTreatAsClass not present\n");
        return;
    }
    hr = pCoGetTreatAsClass(&deadbeef,&out);
    ok (hr == S_FALSE, "expected S_FALSE got %x\n",hr);
    ok (IsEqualGUID(&out,&deadbeef), "expected to get same clsid back\n");

    lr = RegOpenKeyExA(HKEY_CLASSES_ROOT, "CLSID", 0, KEY_READ, &clsidkey);
    ok(lr == ERROR_SUCCESS, "Couldn't open CLSID key\n");

    lr = RegCreateKeyExA(clsidkey, deadbeefA, 0, NULL, 0, KEY_WRITE, NULL, &deadbeefkey, NULL);
    ok(lr == ERROR_SUCCESS, "Couldn't create class key\n");

    hr = pCoTreatAsClass(&deadbeef, &deadbeef);
    ok(hr == REGDB_E_WRITEREGDB, "CoTreatAsClass gave wrong error: %08x\n", hr);

    hr = pCoTreatAsClass(&deadbeef, &CLSID_FileProtocol);
    if(hr == REGDB_E_WRITEREGDB){
        win_skip("Insufficient privileges to use CoTreatAsClass\n");
        goto exit;
    }
    ok(hr == S_OK, "CoTreatAsClass failed: %08x\n", hr);

    hr = pCoGetTreatAsClass(&deadbeef, &out);
    ok(hr == S_OK, "CoGetTreatAsClass failed: %08x\n",hr);
    ok(IsEqualGUID(&out, &CLSID_FileProtocol), "expected to get substituted clsid\n");

    OleInitialize(NULL);

    hr = CoCreateInstance(&deadbeef, NULL, CLSCTX_INPROC_SERVER, &IID_IInternetProtocol, (void **)&pIP);
    if(hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("IE not installed so can't test CoCreateInstance\n");
        goto exit;
    }

    ok(hr == S_OK, "CoCreateInstance failed: %08x\n", hr);
    if(pIP){
        IInternetProtocol_Release(pIP);
        pIP = NULL;
    }

    hr = pCoTreatAsClass(&deadbeef, &CLSID_NULL);
    ok(hr == S_OK, "CoTreatAsClass failed: %08x\n", hr);

    hr = pCoGetTreatAsClass(&deadbeef, &out);
    ok(hr == S_FALSE, "expected S_FALSE got %08x\n", hr);
    ok(IsEqualGUID(&out, &deadbeef), "expected to get same clsid back\n");

    /* bizarrely, native's CoTreatAsClass takes some time to take effect in CoCreateInstance */
    Sleep(200);

    hr = CoCreateInstance(&deadbeef, NULL, CLSCTX_INPROC_SERVER, &IID_IInternetProtocol, (void **)&pIP);
    ok(hr == REGDB_E_CLASSNOTREG, "CoCreateInstance gave wrong error: %08x\n", hr);

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

    hr = pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ok(hr == S_OK, "CoInitializeEx failed with error 0x%08x\n", hr);

    /* Calling OleInitialize for the first time should yield S_OK even with
     * apartment already initialized by previous CoInitialize(Ex) calls. */
    hr = OleInitialize(NULL);
    ok(hr == S_OK, "OleInitialize failed with error 0x%08x\n", hr);

    /* Subsequent calls to OleInitialize should return S_FALSE */
    hr = OleInitialize(NULL);
    ok(hr == S_FALSE, "Expected S_FALSE, hr = 0x%08x\n", hr);

    /* Cleanup */
    CoUninitialize();
    OleUninitialize();
    OleUninitialize();
}

static void test_OleRegGetMiscStatus(void)
{
    ULONG_PTR cookie;
    HANDLE handle;
    DWORD status;
    HRESULT hr;

    hr = OleRegGetMiscStatus(&CLSID_Testclass, DVASPECT_ICON, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    status = 0xdeadbeef;
    hr = OleRegGetMiscStatus(&CLSID_Testclass, DVASPECT_ICON, &status);
    ok(hr == REGDB_E_CLASSNOTREG, "got 0x%08x\n", hr);
    ok(status == 0, "got 0x%08x\n", status);

    status = -1;
    hr = OleRegGetMiscStatus(&CLSID_StdFont, DVASPECT_ICON, &status);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(status == 0, "got 0x%08x\n", status);

    if ((handle = activate_context(actctx_manifest, &cookie)))
    {
        status = 0;
        hr = OleRegGetMiscStatus(&CLSID_Testclass, DVASPECT_ICON, &status);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(status == OLEMISC_RECOMPOSEONRESIZE, "got 0x%08x\n", status);

        /* context data takes precedence over registration info */
        status = 0;
        hr = OleRegGetMiscStatus(&CLSID_StdFont, DVASPECT_ICON, &status);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(status == OLEMISC_RECOMPOSEONRESIZE, "got 0x%08x\n", status);

        /* there's no such attribute in context */
        status = -1;
        hr = OleRegGetMiscStatus(&CLSID_Testclass, DVASPECT_DOCPRINT, &status);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(status == 0, "got 0x%08x\n", status);

        pDeactivateActCtx(0, cookie);
        pReleaseActCtx(handle);
    }
}

static void test_CoCreateGuid(void)
{
    HRESULT hr;

    hr = CoCreateGuid(NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
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
    Sleep(50);
    SendMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    return 0;
}

static DWORD CALLBACK post_message_thread(LPVOID arg)
{
    HWND hWnd = arg;
    Sleep(50);
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    return 0;
}

static void test_CoWaitForMultipleHandles(void)
{
    static const char cls_name[] = "cowait_test_class";
    HANDLE handles[2], thread;
    DWORD index, tid;
    WNDCLASSEXA wc;
    BOOL success;
    HRESULT hr;
    HWND hWnd;
    MSG msg;

    hr = pCoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ok(hr == S_OK, "CoInitializeEx failed with error 0x%08x\n", hr);

    memset(&wc, 0, sizeof(wc));
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_VREDRAW | CS_HREDRAW;
    wc.hInstance     = GetModuleHandleA(0);
    wc.hCursor       = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = cls_name;
    wc.lpfnWndProc   = DefWindowProcA;
    success = RegisterClassExA(&wc) != 0;
    ok(success, "RegisterClassExA failed %u\n", GetLastError());

    hWnd = CreateWindowExA(0, cls_name, "Test", WS_TILEDWINDOW, 0, 0, 640, 480, 0, 0, 0, 0);
    ok(hWnd != 0, "CreateWindowExA failed %u\n", GetLastError());
    handles[0] = CreateSemaphoreA(NULL, 1, 1, NULL);
    ok(handles[0] != 0, "CreateSemaphoreA failed %u\n", GetLastError());
    handles[1] = CreateSemaphoreA(NULL, 1, 1, NULL);
    ok(handles[1] != 0, "CreateSemaphoreA failed %u\n", GetLastError());

    /* test without flags */

    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(0, 50, 0, handles, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got 0x%08x\n", hr);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(success, "CoWaitForMultipleHandles unexpectedly pumped messages\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(0, 50, 0, NULL, &index);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got 0x%08x\n", hr);
    ok(index == 0, "expected index 0, got %u\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(success, "CoWaitForMultipleHandles unexpectedly pumped messages\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(0, 50, 0, handles, &index);
    ok(hr == RPC_E_NO_SYNC, "expected RPC_E_NO_SYNC, got 0x%08x\n", hr);
    ok(index == 0, "expected index 0, got %u\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(success, "CoWaitForMultipleHandles unexpectedly pumped messages\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(0, 50, 1, handles, &index);
    ok(hr == S_OK, "expected S_OK, got 0x%08x\n", hr);
    ok(index == 0, "expected index 0, got %u\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(success, "CoWaitForMultipleHandles unexpectedly pumped messages\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(0, 50, 2, handles, &index);
    ok(hr == S_OK, "expected S_OK, got 0x%08x\n", hr);
    ok(index == 1, "expected index 1, got %u\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(success, "CoWaitForMultipleHandles unexpectedly pumped messages\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(0, 50, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08x\n", hr);
    ok(index == 0 || broken(index == 0xdeadbeef) /* Win 8 */, "expected index 0, got %u\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(!success, "CoWaitForMultipleHandles didn't pump any messages\n");

    /* test PostMessageA/SendMessageA from a different thread */

    index = 0xdeadbeef;
    thread = CreateThread(NULL, 0, post_message_thread, hWnd, 0, &tid);
    ok(thread != NULL, "CreateThread failed, error %u\n", GetLastError());
    hr = CoWaitForMultipleHandles(0, 100, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08x\n", hr);
    ok(index == 0 || broken(index == 0xdeadbeef) /* Win 8 */, "expected index 0, got %u\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(!success, "CoWaitForMultipleHandles didn't pump any messages\n");
    index = WaitForSingleObject(thread, 200);
    ok(index == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(thread);

    index = 0xdeadbeef;
    thread = CreateThread(NULL, 0, send_message_thread, hWnd, 0, &tid);
    ok(thread != NULL, "CreateThread failed, error %u\n", GetLastError());
    hr = CoWaitForMultipleHandles(0, 100, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08x\n", hr);
    ok(index == 0 || broken(index == 0xdeadbeef) /* Win 8 */, "expected index 0, got %u\n", index);
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
    ok(hr == S_OK, "expected S_OK, got 0x%08x\n", hr);
    ok(index == 0, "expected index 0, got %u\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(success, "CoWaitForMultipleHandles unexpectedly pumped messages\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(0, 50, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08x\n", hr);
    ok(index == 0 || broken(index == 0xdeadbeef) /* Win 8 */, "expected index 0, got %u\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(!success, "CoWaitForMultipleHandles didn't pump any messages\n");

    ReleaseSemaphore(handles[0], 1, NULL);
    ReleaseSemaphore(handles[1], 1, NULL);

    /* test with COWAIT_ALERTABLE */

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(COWAIT_ALERTABLE, 50, 1, handles, &index);
    ok(hr == S_OK, "expected S_OK, got 0x%08x\n", hr);
    ok(index == 0, "expected index 0, got %u\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(success, "CoWaitForMultipleHandles unexpectedly pumped messages\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(COWAIT_ALERTABLE, 50, 2, handles, &index);
    ok(hr == S_OK, "expected S_OK, got 0x%08x\n", hr);
    ok(index == 1, "expected index 1, got %u\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(success, "CoWaitForMultipleHandles unexpectedly pumped messages\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    hr = CoWaitForMultipleHandles(COWAIT_ALERTABLE, 50, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08x\n", hr);
    ok(index == 0 || broken(index == 0xdeadbeef) /* Win 8 */, "expected index 0, got %u\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(!success, "CoWaitForMultipleHandles didn't pump any messages\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    success = QueueUserAPC(apc_test_proc, GetCurrentThread(), 0);
    ok(success, "QueueUserAPC failed %u\n", GetLastError());
    hr = CoWaitForMultipleHandles(COWAIT_ALERTABLE, 50, 2, handles, &index);
    ok(hr == S_OK, "expected S_OK, got 0x%08x\n", hr);
    ok(index == WAIT_IO_COMPLETION, "expected index WAIT_IO_COMPLETION, got %u\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(success, "CoWaitForMultipleHandles unexpectedly pumped messages\n");

    /* test with COWAIT_INPUTAVAILABLE (semaphores are still locked) */

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_NOREMOVE);
    ok(success, "PeekMessageA returned FALSE\n");
    hr = CoWaitForMultipleHandles(0, 50, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08x\n", hr);
    ok(index == 0 || broken(index == 0xdeadbeef) /* Win 8 */, "expected index 0, got %u\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(!success, "CoWaitForMultipleHandles didn't pump any messages\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_NOREMOVE);
    ok(success, "PeekMessageA returned FALSE\n");
    thread = CreateThread(NULL, 0, release_semaphore_thread, handles[1], 0, &tid);
    ok(thread != NULL, "CreateThread failed, error %u\n", GetLastError());
    hr = CoWaitForMultipleHandles(COWAIT_INPUTAVAILABLE, 50, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING || broken(hr == E_INVALIDARG) || broken(hr == S_OK) /* Win 8 */,
       "expected RPC_S_CALLPENDING, got 0x%08x\n", hr);
    if (hr != S_OK) ReleaseSemaphore(handles[1], 1, NULL);
    ok(index == 0 || broken(index == 1) /* Win 8 */, "expected index 0, got %u\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(!success || broken(success && hr == E_INVALIDARG),
       "CoWaitForMultipleHandles didn't pump any messages\n");
    index = WaitForSingleObject(thread, 200);
    ok(index == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(thread);

    /* test behaviour of WM_QUIT (semaphores are still locked) */

    PostMessageA(hWnd, WM_QUIT, 40, 0);
    memset(&msg, 0, sizeof(msg));
    success = PeekMessageA(&msg, hWnd, WM_QUIT, WM_QUIT, PM_REMOVE);
    ok(success, "PeekMessageA failed, error %u\n", GetLastError());
    ok(msg.message == WM_QUIT, "expected msg.message = WM_QUIT, got %u\n", msg.message);
    ok(msg.wParam == 40, "expected msg.wParam = 40, got %lu\n", msg.wParam);
    success = PeekMessageA(&msg, hWnd, WM_QUIT, WM_QUIT, PM_REMOVE);
    ok(!success, "PeekMessageA succeeded\n");

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    PostMessageA(hWnd, WM_QUIT, 41, 0);
    thread = CreateThread(NULL, 0, post_message_thread, hWnd, 0, &tid);
    ok(thread != NULL, "CreateThread failed, error %u\n", GetLastError());
    hr = CoWaitForMultipleHandles(0, 100, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08x\n", hr);
    ok(index == 0 || broken(index == 0xdeadbeef) /* Win 8 */, "expected index 0, got %u\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    todo_wine
    ok(success || broken(!success) /* Win 2000/XP/8 */, "PeekMessageA failed, error %u\n", GetLastError());
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(!success, "PeekMessageA succeeded\n");
    memset(&msg, 0, sizeof(msg));
    success = PeekMessageA(&msg, hWnd, WM_QUIT, WM_QUIT, PM_REMOVE);
    todo_wine
    ok(!success || broken(success) /* Win 2000/XP/8 */, "PeekMessageA succeeded\n");
    if (success)
    {
        ok(msg.message == WM_QUIT, "expected msg.message = WM_QUIT, got %u\n", msg.message);
        ok(msg.wParam == 41, "expected msg.wParam = 41, got %lu\n", msg.wParam);
    }
    index = WaitForSingleObject(thread, 200);
    ok(index == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(thread);

    index = 0xdeadbeef;
    PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
    PostMessageA(hWnd, WM_QUIT, 42, 0);
    thread = CreateThread(NULL, 0, send_message_thread, hWnd, 0, &tid);
    ok(thread != NULL, "CreateThread failed, error %u\n", GetLastError());
    hr = CoWaitForMultipleHandles(0, 100, 2, handles, &index);
    ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08x\n", hr);
    ok(index == 0 || broken(index == 0xdeadbeef) /* Win 8 */, "expected index 0, got %u\n", index);
    success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
    ok(!success, "CoWaitForMultipleHandles didn't pump all WM_DDE_FIRST messages\n");
    memset(&msg, 0, sizeof(msg));
    success = PeekMessageA(&msg, hWnd, WM_QUIT, WM_QUIT, PM_REMOVE);
    ok(success, "PeekMessageA failed, error %u\n", GetLastError());
    ok(msg.message == WM_QUIT, "expected msg.message = WM_QUIT, got %u\n", msg.message);
    ok(msg.wParam == 42, "expected msg.wParam = 42, got %lu\n", msg.wParam);
    index = WaitForSingleObject(thread, 200);
    ok(index == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
    CloseHandle(thread);

    PostQuitMessage(43);
    memset(&msg, 0, sizeof(msg));
    success = PeekMessageA(&msg, hWnd, WM_QUIT, WM_QUIT, PM_REMOVE);
    ok(success || broken(!success) /* Win 8 */, "PeekMessageA failed, error %u\n", GetLastError());
    if (!success)
        win_skip("PostQuitMessage didn't queue a WM_QUIT message, skipping tests\n");
    else
    {
        ok(msg.message == WM_QUIT, "expected msg.message = WM_QUIT, got %u\n", msg.message);
        ok(msg.wParam == 43, "expected msg.wParam = 43, got %lu\n", msg.wParam);
        success = PeekMessageA(&msg, hWnd, WM_QUIT, WM_QUIT, PM_REMOVE);
        ok(!success, "PeekMessageA succeeded\n");

        index = 0xdeadbeef;
        PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
        PostQuitMessage(44);
        thread = CreateThread(NULL, 0, post_message_thread, hWnd, 0, &tid);
        ok(thread != NULL, "CreateThread failed, error %u\n", GetLastError());
        hr = CoWaitForMultipleHandles(0, 100, 2, handles, &index);
        ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08x\n", hr);
        ok(index == 0 || broken(index == 0xdeadbeef) /* Win 8 */, "expected index 0, got %u\n", index);
        success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
        ok(success, "PeekMessageA failed, error %u\n", GetLastError());
        success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
        todo_wine
        ok(!success, "PeekMessageA succeeded\n");
        success = PeekMessageA(&msg, hWnd, WM_QUIT, WM_QUIT, PM_REMOVE);
        todo_wine
        ok(!success, "CoWaitForMultipleHandles didn't remove WM_QUIT messages\n");
        index = WaitForSingleObject(thread, 200);
        ok(index == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
        CloseHandle(thread);

        index = 0xdeadbeef;
        PostMessageA(hWnd, WM_DDE_FIRST, 0, 0);
        PostQuitMessage(45);
        thread = CreateThread(NULL, 0, send_message_thread, hWnd, 0, &tid);
        ok(thread != NULL, "CreateThread failed, error %u\n", GetLastError());
        hr = CoWaitForMultipleHandles(0, 100, 2, handles, &index);
        ok(hr == RPC_S_CALLPENDING, "expected RPC_S_CALLPENDING, got 0x%08x\n", hr);
        ok(index == 0 || broken(index == 0xdeadbeef) /* Win 8 */, "expected index 0, got %u\n", index);
        success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
        ok(success, "PeekMessageA failed, error %u\n", GetLastError());
        success = PeekMessageA(&msg, hWnd, WM_DDE_FIRST, WM_DDE_FIRST, PM_REMOVE);
        todo_wine
        ok(!success, "PeekMessageA succeeded\n");
        success = PeekMessageA(&msg, hWnd, WM_QUIT, WM_QUIT, PM_REMOVE);
        ok(!success, "CoWaitForMultipleHandles didn't remove WM_QUIT messages\n");
        index = WaitForSingleObject(thread, 200);
        ok(index == WAIT_OBJECT_0, "WaitForSingleObject failed\n");
        CloseHandle(thread);
    }

    CloseHandle(handles[0]);
    CloseHandle(handles[1]);
    DestroyWindow(hWnd);

    success = UnregisterClassA(cls_name, GetModuleHandleA(0));
    ok(success, "UnregisterClass failed %u\n", GetLastError());

    CoUninitialize();
}

static void init_funcs(void)
{
    HMODULE hOle32 = GetModuleHandleA("ole32");
    HMODULE hAdvapi32 = GetModuleHandleA("advapi32");
    HMODULE hkernel32 = GetModuleHandleA("kernel32");

    pCoGetObjectContext = (void*)GetProcAddress(hOle32, "CoGetObjectContext");
    pCoSwitchCallContext = (void*)GetProcAddress(hOle32, "CoSwitchCallContext");
    pCoGetTreatAsClass = (void*)GetProcAddress(hOle32,"CoGetTreatAsClass");
    pCoTreatAsClass = (void*)GetProcAddress(hOle32,"CoTreatAsClass");
    pCoGetContextToken = (void*)GetProcAddress(hOle32, "CoGetContextToken");
    pRegDeleteKeyExA = (void*)GetProcAddress(hAdvapi32, "RegDeleteKeyExA");
    pRegOverridePredefKey = (void*)GetProcAddress(hAdvapi32, "RegOverridePredefKey");
    pCoInitializeEx = (void*)GetProcAddress(hOle32, "CoInitializeEx");

    pActivateActCtx = (void*)GetProcAddress(hkernel32, "ActivateActCtx");
    pCreateActCtxW = (void*)GetProcAddress(hkernel32, "CreateActCtxW");
    pDeactivateActCtx = (void*)GetProcAddress(hkernel32, "DeactivateActCtx");
    pIsWow64Process = (void*)GetProcAddress(hkernel32, "IsWow64Process");
    pReleaseActCtx = (void*)GetProcAddress(hkernel32, "ReleaseActCtx");
}

START_TEST(compobj)
{
    init_funcs();

    if (!pCoInitializeEx)
    {
        trace("You need DCOM95 installed to run this test\n");
        return;
    }

    if (!pCreateActCtxW)
        win_skip("Activation contexts are not supported, some tests will be skipped.\n");

    test_ProgIDFromCLSID();
    test_CLSIDFromProgID();
    test_CLSIDFromString();
    test_IIDFromString();
    test_StringFromGUID2();
    test_CoCreateInstance();
    test_ole_menu();
    test_CoGetClassObject();
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
    test_OleRegGetMiscStatus();
    test_CoCreateGuid();
    test_CoWaitForMultipleHandles();
}
