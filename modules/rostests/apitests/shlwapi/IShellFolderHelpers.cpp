/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for SHLWAPI IShellFolder helpers
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <versionhelpers.h>

#include <shlwapi_undoc.h>
#include <ishellfolder_helpers.h>

static INT s_nStep = 0;

class CTestShellFolder : public IShellFolder
{
public:
    CTestShellFolder() { }
    virtual ~CTestShellFolder() { }

    static void *operator new(size_t size)
    {
        return LocalAlloc(LPTR, size);
    }
    static void operator delete(void *ptr)
    {
        LocalFree(ptr);
    }
    static void operator delete(void *ptr, size_t size)
    {
        LocalFree(ptr);
    }

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject) override
    {
        ok_int(s_nStep, 11);
        ok_int(IsEqualGUID(riid, IID_IShellFolder2), TRUE);
        ++s_nStep;
        return E_NOINTERFACE;
    }
    STDMETHOD_(ULONG, AddRef)() override
    {
        ok_int(TRUE, FALSE);
        return 1;
    }
    STDMETHOD_(ULONG, Release)() override
    {
        ok_int(TRUE, FALSE);
        return 1;
    }

    // IShellFolder methods
    STDMETHOD(ParseDisplayName)(
        HWND hwndOwner,
        LPBC pbc,
        LPOLESTR lpszDisplayName,
        ULONG *pchEaten,
        PIDLIST_RELATIVE *ppidl,
        ULONG *pdwAttributes) override
    {
        ok_ptr(*ppidl, NULL);
        ok_long(*pdwAttributes, 0);
        ++s_nStep;
        return 0xDEADFACE;
    }
    STDMETHOD(EnumObjects)(
        HWND hwndOwner,
        DWORD dwFlags,
        LPENUMIDLIST *ppEnumIDList) override
    {
        ok_int(TRUE, FALSE);
        return E_NOTIMPL;
    }
    STDMETHOD(BindToObject)(
        PCUIDLIST_RELATIVE pidl,
        LPBC pbcReserved,
        REFIID riid,
        LPVOID *ppvOut) override
    {
        ok_int(TRUE, FALSE);
        return E_NOTIMPL;
    }
    STDMETHOD(BindToStorage)(
        PCUIDLIST_RELATIVE pidl,
        LPBC pbcReserved,
        REFIID riid,
        LPVOID *ppvOut) override
    {
        ok_int(TRUE, FALSE);
        return E_NOTIMPL;
    }
    STDMETHOD(CompareIDs)(
        LPARAM lParam,
        PCUIDLIST_RELATIVE pidl1,
        PCUIDLIST_RELATIVE pidl2) override
    {
        switch (s_nStep)
        {
            case 11:
                // It shouldn't come here
                ok_int(TRUE, FALSE);
                break;
            case 12:
                ok_long((LONG)lParam, 0x00001234);
                break;
            case 13:
                ok_long((LONG)lParam, 0x00005678);
                break;
            default:
                skip("\n");
                break;
        }
        ++s_nStep;
        return 0xFEEDF00D;
    }
    STDMETHOD(CreateViewObject)(
        HWND hwndOwner,
        REFIID riid,
        LPVOID *ppvOut) override
    {
        ok_int(TRUE, FALSE);
        return E_NOTIMPL;
    }
    STDMETHOD(GetAttributesOf)(
        UINT cidl,
        PCUITEMID_CHILD_ARRAY apidl,
        DWORD *rgfInOut) override
    {
        ok_int(TRUE, FALSE);
        return E_NOTIMPL;
    }
    STDMETHOD(GetUIObjectOf)(
        HWND hwndOwner,
        UINT cidl,
        PCUITEMID_CHILD_ARRAY apidl,
        REFIID riid,
        UINT * prgfInOut,
        LPVOID * ppvOut) override
    {
        ok_int(TRUE, FALSE);
        return E_NOTIMPL;
    }
    STDMETHOD(GetDisplayNameOf)(
        PCUITEMID_CHILD pidl,
        DWORD dwFlags,
        LPSTRRET strRet) override
    {
        switch (s_nStep)
        {
            case 0:
                ok_long(dwFlags, SHGDN_FORPARSING | SHGDN_FORADDRESSBAR | SHGDN_FOREDITING |
                                 SHGDN_INFOLDER);
                break;
            case 1:
                ok_long(dwFlags, SHGDN_FORPARSING | SHGDN_FORADDRESSBAR | SHGDN_INFOLDER);
                break;
            case 2:
                ok_long(dwFlags, SHGDN_FORPARSING | SHGDN_INFOLDER);
                break;
            case 3:
                ok_long(dwFlags, SHGDN_FORPARSING);
                break;
            case 4:
                ok_long(dwFlags, SHGDN_FORADDRESSBAR | SHGDN_FOREDITING | SHGDN_INFOLDER);
                break;
            case 5:
                ok_long(dwFlags, SHGDN_FORADDRESSBAR | SHGDN_INFOLDER);
                break;
            case 6:
                ok_long(dwFlags, SHGDN_INFOLDER);
                break;
            case 7:
                ok_long(dwFlags, SHGDN_FORPARSING | SHGDN_INFOLDER);
                break;
            case 8:
                ok_long(dwFlags, SHGDN_INFOLDER);
                break;
            case 9:
                ok_long(dwFlags, SHGDN_NORMAL);
                break;
            default:
                skip("\n");
                break;
        }
        ++s_nStep;
        return E_FAIL;
    }
    STDMETHOD(SetNameOf)(
        HWND hwndOwner,
        PCUITEMID_CHILD pidl,
        LPCOLESTR lpName,
        DWORD dwFlags,
        PITEMID_CHILD *pPidlOut) override
    {
        ok_int(TRUE, FALSE);
        return E_NOTIMPL;
    }
};

static void Test_GetDisplayNameOf(void)
{
    CTestShellFolder *psf = new CTestShellFolder();
    HRESULT hr;

    hr = IShellFolder_GetDisplayNameOf(
        psf,
        NULL,
        SHGDN_FOREDITING | SHGDN_FORADDRESSBAR | SHGDN_FORPARSING | SHGDN_INFOLDER,
        NULL,
        0);
    ok_long(hr, E_FAIL);
    ok_int(s_nStep, 4);

    hr = IShellFolder_GetDisplayNameOf(
        psf,
        NULL,
        SHGDN_FOREDITING | SHGDN_FORADDRESSBAR | SHGDN_INFOLDER,
        NULL,
        0);
    ok_long(hr, E_FAIL);
    ok_int(s_nStep, 10);

    if (s_nStep != 10)
        skip("s_nStep value is wrong\n");

    delete psf;
}

static void Test_ParseDisplayName(void)
{
    CTestShellFolder *psf = new CTestShellFolder();
    HRESULT hr;

    s_nStep = 10;
    LPITEMIDLIST pidl = (LPITEMIDLIST)UlongToPtr(0xDEADDEAD);
    hr = IShellFolder_ParseDisplayName(
        psf,
        NULL,
        NULL,
        NULL,
        NULL,
        &pidl,
        NULL);
    ok_long(hr, 0xDEADFACE);
    ok_int(s_nStep, 11);

    delete psf;
}

typedef HRESULT (WINAPI *FN_IShellFolder_CompareIDs)(
    _In_ IShellFolder *psf,
    _In_ LPARAM lParam,
    _In_ PCUIDLIST_RELATIVE pidl1,
    _In_ PCUIDLIST_RELATIVE pidl2);

static void Test_CompareIDs(void)
{
    FN_IShellFolder_CompareIDs fnIShellFolder_CompareIDs;
    fnIShellFolder_CompareIDs =
        (FN_IShellFolder_CompareIDs)
            GetProcAddress(GetModuleHandleA("shlwapi"), MAKEINTRESOURCEA(551));

    if (IsWindowsVistaOrGreater())
    {
        skip("Vista+\n");
        ok(fnIShellFolder_CompareIDs == NULL, "Vista+ has no IShellFolder_CompareIDs\n");
        return;
    }

    CTestShellFolder *psf = new CTestShellFolder();
    HRESULT hr;

    s_nStep = 11;
    hr = fnIShellFolder_CompareIDs(
        psf,
        0xFFFF1234,
        NULL,
        NULL);
    ok_long(hr, 0xFEEDF00D);
    ok_int(s_nStep, 13);

    s_nStep = 13;
    hr = fnIShellFolder_CompareIDs(
        psf,
        0x00005678,
        NULL,
        NULL);
    ok_long(hr, 0xFEEDF00D);
    ok_int(s_nStep, 14);

    delete psf;
}

START_TEST(IShellFolderHelpers)
{
    HRESULT hrCoInit = ::CoInitialize(NULL);

    Test_GetDisplayNameOf();
    Test_ParseDisplayName();
    Test_CompareIDs();

    if (SUCCEEDED(hrCoInit))
        ::CoUninitialize();
}
