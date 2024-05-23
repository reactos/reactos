/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for IShellFolder helpers
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <shlobj.h>
#include <shlwapi.h>

#define SHLWAPI_ISHELLFOLDER_HELPERS
#include <shlwapi_undoc.h>

static INT s_nStage = 0;

void *operator new(size_t size)
{
    return LocalAlloc(LPTR, size);
}

void operator delete(void *ptr)
{
    LocalFree(ptr);
}

class CTestShellFolder : public IShellFolder
{
public:
    CTestShellFolder() { }
    virtual ~CTestShellFolder() { }

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject) override
    {
        return E_NOTIMPL;
    }
    STDMETHOD_(ULONG, AddRef)() override
    {
        return 1;
    }
    STDMETHOD_(ULONG, Release)() override
    {
        return 1;
    }

    // IShellFolder methods
    STDMETHOD(ParseDisplayName)(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName, ULONG *pchEaten, PIDLIST_RELATIVE *ppidl, ULONG *pdwAttributes) override
    {
        return E_NOTIMPL;
    }
    STDMETHOD(EnumObjects)(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList) override
    {
        return E_NOTIMPL;
    }
    STDMETHOD(BindToObject)(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut) override
    {
        return E_NOTIMPL;
    }
    STDMETHOD(BindToStorage)(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut) override
    {
        return E_NOTIMPL;
    }
    STDMETHOD(CompareIDs)(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2) override
    {
        return E_NOTIMPL;
    }
    STDMETHOD(CreateViewObject)(HWND hwndOwner, REFIID riid, LPVOID *ppvOut) override
    {
        return E_NOTIMPL;
    }
    STDMETHOD(GetAttributesOf)(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut) override
    {
        return E_NOTIMPL;
    }
    STDMETHOD(GetUIObjectOf)(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT * prgfInOut, LPVOID * ppvOut) override
    {
        return E_NOTIMPL;
    }
    STDMETHOD(GetDisplayNameOf)(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet) override
    {
        switch (s_nStage)
        {
            case 0:
                ok_long(dwFlags, SHGDN_FORPARSING | SHGDN_FORADDRESSBAR | SHGDN_FOREDITING | SHGDN_INFOLDER);
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
        ++s_nStage;
        return E_FAIL;
    }
    STDMETHOD(SetNameOf)(HWND hwndOwner, PCUITEMID_CHILD pidl, LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut) override
    {
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
    ok_int(s_nStage, 4);

    hr = IShellFolder_GetDisplayNameOf(
        psf,
        NULL,
        SHGDN_FOREDITING | SHGDN_FORADDRESSBAR | SHGDN_INFOLDER,
        NULL,
        0);
    ok_long(hr, E_FAIL);
    ok_int(s_nStage, 10);

    if (s_nStage != 10)
        skip("s_nStage value is wrong\n");

    delete psf;
}

static void Test_ParseDisplayName(void)
{
    // FIXME
}

static void Test_CompareIDs(void)
{
    // FIXME
}

START_TEST(IShellFolderHelpers)
{
    Test_GetDisplayNameOf();
    Test_ParseDisplayName();
    Test_CompareIDs();
}
