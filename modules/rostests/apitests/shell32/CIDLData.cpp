/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for CIDLDataObj
 * COPYRIGHT:   Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
 */

#include "shelltest.h"
#include <ndk/rtlfuncs.h>
#include <stdio.h>
#include <shellutils.h>
#include <shlwapi.h>

static DWORD g_WinVersion;


static void TestAdviseAndCanonical(PCIDLIST_ABSOLUTE pidlFolder, UINT cidl, PCUIDLIST_RELATIVE_ARRAY apidl)
{
    CComPtr<IDataObject> spDataObj;
    HRESULT hr = CIDLData_CreateFromIDArray(pidlFolder, cidl, apidl, &spDataObj);

    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    hr = spDataObj->DAdvise(NULL, 0, NULL, NULL);
    ok_hex(hr, OLE_E_ADVISENOTSUPPORTED);

    hr = spDataObj->DUnadvise(0);
    ok_hex(hr, OLE_E_ADVISENOTSUPPORTED);

    hr = spDataObj->EnumDAdvise(NULL);
    ok_hex(hr, OLE_E_ADVISENOTSUPPORTED);


    FORMATETC in = {1, (DVTARGETDEVICE*)2, 3, 4, 5};
    FORMATETC out = {6, (DVTARGETDEVICE*)7, 8, 9, 10};

    hr = spDataObj->GetCanonicalFormatEtc(&in, &out);
    ok_hex(hr, DATA_S_SAMEFORMATETC);

    if (g_WinVersion < _WIN32_WINNT_VISTA)
    {
        ok_int(out.cfFormat, 6);
        ok_ptr(out.ptd, (void*)7);
        ok_int(out.dwAspect, 8);
        ok_int(out.lindex, 9);
        ok_int(out.tymed, 10);
        trace("out unmodified\n");
    }
    else
    {
        ok_int(out.cfFormat, in.cfFormat);
        ok_ptr(out.ptd, NULL);
        ok_int(out.dwAspect, (int)in.dwAspect);
        ok_int(out.lindex, in.lindex);
        ok_int(out.tymed, (int)in.tymed);
        trace("in copied to out\n");
    }
}


#define ok_wstri(x, y) \
    ok(_wcsicmp(x, y) == 0, "Wrong string. Expected '%S', got '%S'\n", y, x)

static void TestHIDA(PVOID pData, SIZE_T Size, LPCWSTR ExpectRoot, LPCWSTR ExpectPath1, LPCWSTR ExpectPath2)
{
    LPIDA pida = (LPIDA)pData;

    ok_int(pida->cidl, 2);
    if (pida->cidl != 2)
        return;

    WCHAR FolderPath[MAX_PATH], Item1[MAX_PATH], Item2[MAX_PATH];
    BOOL bRet = SHGetPathFromIDListW(HIDA_GetPIDLFolder(pida), FolderPath);
    ok_int(bRet, TRUE);
    if (!bRet)
        return;
    ok_wstri(FolderPath, ExpectRoot);

    CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidl1(ILCombine(HIDA_GetPIDLFolder(pida), HIDA_GetPIDLItem(pida, 0)));
    CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidl2(ILCombine(HIDA_GetPIDLFolder(pida), HIDA_GetPIDLItem(pida, 1)));

    bRet = SHGetPathFromIDListW(pidl1, Item1);
    ok_int(bRet, TRUE);
    if (!bRet)
        return;
    ok_wstri(Item1, ExpectPath1);

    bRet = SHGetPathFromIDListW(pidl2, Item2);
    ok_int(bRet, TRUE);
    if (!bRet)
        return;
    ok_wstri(Item2, ExpectPath2);
}

static void TestHDROP(PVOID pData, SIZE_T Size, LPCWSTR ExpectRoot, LPCWSTR ExpectPath1, LPCWSTR ExpectPath2)
{
    DROPFILES* pDropFiles = (DROPFILES*)pData;
    ok_int(pDropFiles->fWide, TRUE);

    LPCWSTR Expected[2] = { ExpectPath1, ExpectPath2 };

    SIZE_T offset = pDropFiles->pFiles;
    UINT Count = 0;
    for (;;Count++)
    {
        LPCWSTR ptr = (LPCWSTR)(((BYTE*)pDropFiles) + offset);
        if (!*ptr)
            break;

        if (Count < _countof(Expected))
            ok_wstri(Expected[Count], ptr);

        offset += (wcslen(ptr) + 1) * sizeof(WCHAR);
    }
    ok_int(Count, 2);
}

static void TestFilenameA(PVOID pData, SIZE_T Size, LPCWSTR ExpectRoot, LPCWSTR ExpectPath1, LPCWSTR ExpectPath2)
{
    LPCSTR FirstFile = (LPCSTR)pData;
    LPWSTR FirstFileW;

    HRESULT hr = SHStrDupA(FirstFile, &FirstFileW);
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    ok_wstri(ExpectPath1, FirstFileW);
    CoTaskMemFree(FirstFileW);
}

static void TestFilenameW(PVOID pData, SIZE_T Size, LPCWSTR ExpectRoot, LPCWSTR ExpectPath1, LPCWSTR ExpectPath2)
{
    LPCWSTR FirstFile = (LPCWSTR)pData;
    ok_wstri(ExpectPath1, FirstFile);
}


static void TestDefaultFormat(PCIDLIST_ABSOLUTE pidlFolder, UINT cidl, PCUIDLIST_RELATIVE_ARRAY apidl)
{
    CComPtr<IDataObject> spDataObj;
    HRESULT hr = CIDLData_CreateFromIDArray(pidlFolder, cidl, apidl, &spDataObj);

    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    CComPtr<IEnumFORMATETC> pEnumFmt;
    hr = spDataObj->EnumFormatEtc(DATADIR_GET, &pEnumFmt);

    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    UINT Expected[4] = {
        RegisterClipboardFormatA(CFSTR_SHELLIDLISTA),
        CF_HDROP,
        RegisterClipboardFormatA(CFSTR_FILENAMEA),
        RegisterClipboardFormatA("FileNameW"),
    };

    UINT Count = 0;
    FORMATETC fmt;
    while (S_OK == (hr=pEnumFmt->Next(1, &fmt, NULL)))
    {
        char szGot[512], szExpected[512];
        GetClipboardFormatNameA(fmt.cfFormat, szGot, sizeof(szGot));
        ok(Count < _countof(Expected), "%u\n", Count);
        if (Count < _countof(Expected))
        {
            GetClipboardFormatNameA(Expected[Count], szExpected, sizeof(szExpected));
            ok(fmt.cfFormat == Expected[Count], "Got 0x%x(%s), expected 0x%x(%s) for %u\n",
               fmt.cfFormat, szGot, Expected[Count], szExpected, Count);
        }

        ok(fmt.ptd == NULL, "Got 0x%p, expected 0x%p for [%u].ptd\n", fmt.ptd, (void*)NULL, Count);
        ok(fmt.dwAspect == DVASPECT_CONTENT, "Got 0x%lu, expected 0x%d for [%u].dwAspect\n", fmt.dwAspect, DVASPECT_CONTENT, Count);
        ok(fmt.lindex == -1, "Got 0x%lx, expected 0x%x for [%u].lindex\n", fmt.lindex, -1, Count);
        ok(fmt.tymed == TYMED_HGLOBAL, "Got 0x%lu, expected 0x%d for [%u].tymed\n", fmt.tymed, TYMED_HGLOBAL, Count);

        Count++;
    }
    trace("Got %u formats\n", Count);
    ULONG ExpectedCount = (g_WinVersion < _WIN32_WINNT_WIN8) ? 1 : 4;
    ok_int(Count, (int)ExpectedCount);
    ok_hex(hr, S_FALSE);

    typedef void (*TestFunction)(PVOID pData, SIZE_T Size, LPCWSTR ExpectRoot, LPCWSTR ExpectPath1, LPCWSTR ExpectPath2);
    TestFunction TestFormats[] = {
        TestHIDA,
        TestHDROP,
        TestFilenameA,
        TestFilenameW,
    };

    WCHAR ExpectRoot[MAX_PATH], ExpectItem1[MAX_PATH], ExpectItem2[MAX_PATH];

    hr = SHGetFolderPathW(NULL, CSIDL_WINDOWS, NULL, 0, ExpectRoot);
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    hr = SHGetFolderPathW(NULL, CSIDL_SYSTEM, NULL, 0, ExpectItem1);
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    hr = SHGetFolderPathW(NULL, CSIDL_RESOURCES, NULL, 0, ExpectItem2);
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;


    /* The formats are not synthesized on request */
    for (Count = 0; Count < _countof(Expected); ++Count)
    {
        STGMEDIUM medium = {0};
        FORMATETC etc = { (CLIPFORMAT)Expected[Count], NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        char szExpected[512];

        GetClipboardFormatNameA(etc.cfFormat, szExpected, sizeof(szExpected));
        hr = spDataObj->GetData(&etc, &medium);
        HRESULT hr2 = spDataObj->QueryGetData(&etc);
        ok_hex(hr2, SUCCEEDED(hr) ? S_OK : S_FALSE);

        if (Count < ExpectedCount)
        {
            ok(hr == S_OK, "0x%x (0x%x(%s))\n", (unsigned int)hr, Expected[Count], szExpected);
            ok(medium.tymed == TYMED_HGLOBAL, "0x%lx (0x%x(%s))\n", medium.tymed, Expected[Count], szExpected);
            if (hr == S_OK && medium.tymed == TYMED_HGLOBAL)
            {
                PVOID pData = GlobalLock(medium.hGlobal);
                SIZE_T Size = GlobalSize(medium.hGlobal);
                TestFormats[Count](pData, Size, ExpectRoot, ExpectItem1, ExpectItem2);
                GlobalUnlock(medium.hGlobal);
            }
        }
        else
        {
            if (g_WinVersion < _WIN32_WINNT_VISTA)
                ok(hr == E_INVALIDARG, "0x%x (0x%x(%s))\n", (unsigned int)hr, Expected[Count], szExpected);
            else
                ok(hr == DV_E_FORMATETC, "0x%x (0x%x(%s))\n", (unsigned int)hr, Expected[Count], szExpected);
        }

        if (SUCCEEDED(hr))
            ReleaseStgMedium(&medium);
    }

    // Not registered
    CLIPFORMAT Format = RegisterClipboardFormatW(CFSTR_PREFERREDDROPEFFECTW);
    FORMATETC formatetc = { Format, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM medium;

    hr = spDataObj->GetData(&formatetc, &medium);
    if (g_WinVersion < _WIN32_WINNT_VISTA)
        ok_hex(hr, E_INVALIDARG);
    else
        ok_hex(hr, DV_E_FORMATETC);
}


static void TestSetAndGetExtraFormat(PCIDLIST_ABSOLUTE pidlFolder, UINT cidl, PCUIDLIST_RELATIVE_ARRAY apidl)
{
    CComPtr<IDataObject> spDataObj;
    HRESULT hr = CIDLData_CreateFromIDArray(pidlFolder, cidl, apidl, &spDataObj);

    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    STGMEDIUM medium = {0};
    medium.tymed = TYMED_HGLOBAL;
    medium.hGlobal = GlobalAlloc(GHND, sizeof(DWORD));
    ok(medium.hGlobal != NULL, "Download more ram\n");
    PDWORD data = (PDWORD)GlobalLock(medium.hGlobal);
    *data = 12345;
    GlobalUnlock(medium.hGlobal);

    UINT flags = GlobalFlags(medium.hGlobal);
    SIZE_T size = GlobalSize(medium.hGlobal);
    ok_hex(flags, 0);
    ok_size_t(size, sizeof(DWORD));

    FORMATETC etc = { (CLIPFORMAT)RegisterClipboardFormatA(CFSTR_INDRAGLOOPA), NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    FORMATETC etc2 = etc;

    /* Not supported! */
    hr = spDataObj->SetData(&etc, &medium, FALSE);
    if (g_WinVersion < _WIN32_WINNT_WIN8)
        ok_hex(hr, E_INVALIDARG);
    else
        ok_hex(hr, E_NOTIMPL);

    /* Object takes ownership! */
    hr = spDataObj->SetData(&etc, &medium, TRUE);
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    /* Does not touch the hGlobal! */
    flags = GlobalFlags(medium.hGlobal);
    size = GlobalSize(medium.hGlobal);
    ok_hex(flags, 0);
    ok_size_t(size, sizeof(DWORD));

    STGMEDIUM medium2 = {0};

    /* No conversion */
    etc2.dwAspect = DVASPECT_DOCPRINT;
    hr = spDataObj->GetData(&etc2, &medium2);
    HRESULT hr2 = spDataObj->QueryGetData(&etc2);
    ok_hex(hr2, SUCCEEDED(hr) ? S_OK : S_FALSE);
    if (g_WinVersion < _WIN32_WINNT_VISTA)
        ok_hex(hr, E_INVALIDARG);
    else
        ok_hex(hr, DV_E_FORMATETC);

    etc2.dwAspect = DVASPECT_CONTENT;
    etc2.tymed = TYMED_NULL;
    hr = spDataObj->GetData(&etc2, &medium2);
    hr2 = spDataObj->QueryGetData(&etc2);
    ok_hex(hr2, SUCCEEDED(hr) ? S_OK : S_FALSE);
    if (g_WinVersion < _WIN32_WINNT_VISTA)
        ok_hex(hr, E_INVALIDARG);
    else
        ok_hex(hr, DV_E_FORMATETC);
    etc2.tymed = TYMED_HGLOBAL;

    ok_ptr(medium2.pUnkForRelease, NULL);
    hr = spDataObj->GetData(&etc2, &medium2);
    hr2 = spDataObj->QueryGetData(&etc2);
    ok_hex(hr2, SUCCEEDED(hr) ? S_OK : S_FALSE);
    ok_hex(hr, S_OK);
    if (hr == S_OK)
    {
        ok_hex(medium2.tymed, TYMED_HGLOBAL);
        if (g_WinVersion < _WIN32_WINNT_VISTA)
        {
            /* The IDataObject is set as pUnkForRelease */
            ok(medium2.pUnkForRelease == (IUnknown*)spDataObj, "Expected the data object (0x%p), got 0x%p\n",
                (IUnknown*)spDataObj, medium2.pUnkForRelease);
            ok(medium.hGlobal == medium2.hGlobal, "Pointers are not the same!, got 0x%p and 0x%p\n", medium.hGlobal, medium2.hGlobal);
        }
        else
        {
            ok_ptr(medium2.pUnkForRelease, NULL);
            ok(medium.hGlobal != medium2.hGlobal, "Pointers are the same!\n");
        }

        flags = GlobalFlags(medium2.hGlobal);
        size = GlobalSize(medium2.hGlobal);
        ok_hex(flags, 0);
        ok_size_t(size, sizeof(DWORD));

        data = (PDWORD)GlobalLock(medium2.hGlobal);
        if (data)
            ok_int(*data, 12345);
        else
            ok(0, "GlobalLock: %lu\n", GetLastError());
        GlobalUnlock(medium2.hGlobal);

        HGLOBAL backup = medium2.hGlobal;
        ReleaseStgMedium(&medium2);

        flags = GlobalFlags(backup);
        size = GlobalSize(backup);
        if (g_WinVersion < _WIN32_WINNT_VISTA)
        {
            /* Same object! just the pUnkForRelease was set, so original hGlobal is still valid */
            ok_hex(flags, 0);
            ok_size_t(size, sizeof(DWORD));
        }
        else
        {
            ok_hex(flags, GMEM_INVALID_HANDLE);
            ok_size_t(size, 0);
        }

        /* Original is still intact (but no longer ours!) */
        flags = GlobalFlags(medium.hGlobal);
        size = GlobalSize(medium.hGlobal);
        ok_hex(flags, 0);
        ok_size_t(size, sizeof(DWORD));
    }

    HGLOBAL backup = medium.hGlobal;
    spDataObj.Release();

    /* Now our hGlobal is deleted */
    flags = GlobalFlags(backup);
    size = GlobalSize(backup);
    ok_hex(flags, GMEM_INVALID_HANDLE);
    ok_size_t(size, 0);
}

START_TEST(CIDLData)
{
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    RTL_OSVERSIONINFOEXW rtlinfo = {0};

    rtlinfo.dwOSVersionInfoSize = sizeof(rtlinfo);
    RtlGetVersion((PRTL_OSVERSIONINFOW)&rtlinfo);
    g_WinVersion = (rtlinfo.dwMajorVersion << 8) | rtlinfo.dwMinorVersion;

    CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidlWindows;
    CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidlSystem32;
    CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidlResources;

    hr = SHGetFolderLocation(NULL, CSIDL_WINDOWS, NULL, 0, &pidlWindows);
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    hr = SHGetFolderLocation(NULL, CSIDL_SYSTEM, NULL, 0, &pidlSystem32);
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    hr = SHGetFolderLocation(NULL, CSIDL_RESOURCES, NULL, 0, &pidlResources);
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    CComPtr<IShellFolder> shellFolder;
    PCUITEMID_CHILD child1;
    hr = SHBindToParent(pidlSystem32, IID_PPV_ARG(IShellFolder, &shellFolder), &child1);
    ok_hex(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    PCUITEMID_CHILD child2 = ILFindLastID(pidlResources);

    UINT cidl = 2;
    PCUIDLIST_RELATIVE apidl[2] = {
        child1, child2
    };

    TestAdviseAndCanonical(pidlWindows, cidl, apidl);
    TestDefaultFormat(pidlWindows, cidl, apidl);
    TestSetAndGetExtraFormat(pidlWindows, cidl, apidl);
}
