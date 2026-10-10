/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for IShellFolder
 * COPYRIGHT:   Copyright 2026 Whindmar Saksit <whindsaks@proton.me>
 */

#include "shelltest.h"
#include <shellutils.h>
#include <shlguid_undoc.h>

static IShellFolder2* CreateFolder2Instance(WORD csidl)
{
    PIDLIST_ABSOLUTE pidl;
    HRESULT hr = SHGetSpecialFolderLocation(NULL, csidl, &pidl);
    if (FAILED(hr))
        return NULL;
    IShellFolder2 *pFolder;
    hr = SHELL_BindToObject(NULL, pidl, NULL, IID_PPV_ARG(IShellFolder2, &pFolder));
    ILFree(pidl);
    return SUCCEEDED(hr) ? pFolder : NULL;
}

static void TestFolderSearch()
{
    const bool nt5 = LOBYTE(GetVersion()) < 6;
    static const struct {
        BYTE csidl;
        HRESULT hr;
        const GUID *pExpected;
    } defs[] = 
    {
        { CSIDL_DESKTOP, E_NOTIMPL, NULL },
        { CSIDL_DRIVES, nt5 ? S_OK : E_NOTIMPL, nt5 ? &CLSID_ShellSearchExt : NULL },
        { CSIDL_CONTROLS, nt5 ? S_OK : E_NOTIMPL, nt5 ? &CLSID_ShellSearchExt : NULL },
        { CSIDL_BITBUCKET, nt5 ? S_OK : E_NOTIMPL, nt5 ? &CLSID_ShellSearchExt : NULL },
        { CSIDL_SYSTEM, E_NOTIMPL, NULL },
        { CSIDL_FONTS, E_NOTIMPL, NULL },
        { CSIDL_COOKIES, E_NOTIMPL, NULL },
        { CSIDL_CONNECTIONS, E_NOTIMPL, NULL },
    };
    for (SIZE_T i = 0; i < _countof(defs); ++i)
    {
        CComPtr<IShellFolder2> pFolder(CreateFolder2Instance(defs[i].csidl));
        if (!pFolder)
        {
            skip("Can't create special folder %d\n", defs[i].csidl);
            continue;
        }
        GUID invalid = IID_IPersistFolder3;
        GUID guid = invalid;
        HRESULT hr = pFolder->GetDefaultSearchGUID(&guid);
        ok(hr == defs[i].hr, "Unexpected result %#x for %d\n", hr, defs[i].csidl);
        ok(IsEqualGUID(guid, defs[i].pExpected ? *defs[i].pExpected : invalid), "GUID mismatch for %d!\n", defs[i].csidl);
    }

    static const struct {
        BYTE csidl;
        HRESULT hr;
        bool NullOutput;
    } enums[] = 
    {
        { CSIDL_DESKTOP, E_NOTIMPL, true },
        { CSIDL_DRIVES, E_NOTIMPL, true },
        { CSIDL_CONTROLS, E_NOTIMPL, true },
        { CSIDL_BITBUCKET, E_NOTIMPL, true },
        { CSIDL_SYSTEM, E_NOTIMPL, true },
        { CSIDL_FONTS, E_NOTIMPL, true },
        { CSIDL_COOKIES, E_NOTIMPL, true },
    };
    for (SIZE_T i = 0; i < _countof(enums); ++i)
    {
        CComPtr<IShellFolder2> pFolder(CreateFolder2Instance(enums[i].csidl));
        if (!pFolder)
        {
            skip("Can't create special folder %d\n", enums[i].csidl);
            continue;
        }
        IEnumExtraSearch *invalid = (IEnumExtraSearch*)(SIZE_T)0xBAADF00Dul;
        IEnumExtraSearch *pEnum = invalid;
        HRESULT hr = pFolder->EnumSearches(&pEnum);
        ok(hr == enums[i].hr, "Unexpected result %#x for %d\n", hr, enums[i].csidl);
        ok(pEnum == (enums[i].NullOutput ? NULL : invalid), "Result mismatch for %d!\n", enums[i].csidl);
        if (pEnum && pEnum != invalid)
            pEnum->Release();
    }
}

static void TestGetDefaultColumn()
{
    const bool nt5 = LOBYTE(GetVersion()) < 6;
    static const struct {
        BYTE csidl;
        HRESULT hr;
        BYTE col;
    } defs[] = 
    {
        { CSIDL_DESKTOP, E_NOTIMPL },
        { CSIDL_DRIVES, E_NOTIMPL },
        { CSIDL_CONTROLS, E_NOTIMPL },
        { CSIDL_BITBUCKET, E_NOTIMPL },
        { CSIDL_SYSTEM, E_NOTIMPL },
        { CSIDL_PRINTERS, E_NOTIMPL },
        { CSIDL_NETWORK, nt5 ? E_NOTIMPL : S_OK, 0 },
        { CSIDL_FONTS, E_NOTIMPL },
        { CSIDL_COOKIES, E_NOTIMPL },
        { CSIDL_CONNECTIONS, E_NOTIMPL },
    };
    for (SIZE_T i = 0; i < _countof(defs); ++i)
    {
        CComPtr<IShellFolder2> pFolder(CreateFolder2Instance(defs[i].csidl));
        if (!pFolder)
        {
            skip("Can't create special folder %d\n", defs[i].csidl);
            continue;
        }
        ULONG invalid = (ULONG)-42;
        ULONG sort = invalid, disp = invalid;
        HRESULT hr = pFolder->GetDefaultColumn(0, &sort, &disp);
        ok(hr == defs[i].hr, "Unexpected result %#x for %d\n", hr, defs[i].csidl);
        ULONG expected = SUCCEEDED(hr) ? defs[i].col : invalid;
        ok(sort == expected, "Unexpected output %#x for %d\n", sort, defs[i].csidl);
        ok(disp == expected, "Unexpected output %#x for %d\n", disp, defs[i].csidl);
    }
}

static void TestGetDefaultColumnState()
{
    // Note: We are just testing the first column here, specific tests should be performed in the tests for each folder.
    const bool nt5 = LOBYTE(GetVersion()) < 6;
    static const struct {
        BYTE csidl;
        HRESULT hr;
        BYTE type;
    } states[] = 
    {
        { CSIDL_DESKTOP, S_OK, SHCOLSTATE_TYPE_STR },
        { CSIDL_DRIVES, S_OK, SHCOLSTATE_TYPE_STR },
        { CSIDL_CONTROLS, nt5 ? E_NOTIMPL : S_OK, SHCOLSTATE_TYPE_STR },
        { CSIDL_BITBUCKET, S_OK, SHCOLSTATE_TYPE_STR },
        { CSIDL_SYSTEM, S_OK, SHCOLSTATE_TYPE_STR },
        { CSIDL_PRINTERS, S_OK, SHCOLSTATE_TYPE_STR },
        { CSIDL_NETWORK, S_OK, SHCOLSTATE_TYPE_STR },
        { CSIDL_FONTS, S_OK, SHCOLSTATE_TYPE_STR },
        { CSIDL_COOKIES, S_OK, SHCOLSTATE_TYPE_STR },
        { CSIDL_CONNECTIONS, S_OK, SHCOLSTATE_TYPE_STR },
    };
    for (SIZE_T i = 0; i < _countof(states); ++i)
    {
        CComPtr<IShellFolder2> pFolder(CreateFolder2Instance(states[i].csidl));
        if (!pFolder)
        {
            skip("Can't create special folder %d\n", states[i].csidl);
            continue;
        }
        UINT invalid = SHCOLSTATE_TYPEMASK;
        SHCOLSTATEF flags = invalid;
        HRESULT hr = pFolder->GetDefaultColumnState(0, &flags);
        ok(hr == states[i].hr, "Unexpected result %#x for %d\n", hr, states[i].csidl);
        UINT expected = SUCCEEDED(states[i].hr) ? states[i].type : invalid;
        ok((flags & SHCOLSTATE_TYPEMASK) == expected, "Unexpected output %#x for %d\n", flags, states[i].csidl);
    }
}

START_TEST(ShellFolder)
{
    CCoInit ComInit;

    TestFolderSearch();
    TestGetDefaultColumn();
    TestGetDefaultColumnState();
}
