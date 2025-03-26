/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for SHGetAttributesFromDataObject
 * COPYRIGHT:   Copyright 2021 Mark Jansen <mark.jansen@reactos.org>
 */

#include "shelltest.h"
#include <ndk/rtlfuncs.h>
#include <stdio.h>
#include <shellutils.h>
#include <shlwapi.h>


static CLIPFORMAT g_DataObjectAttributes = 0;
static const DWORD dwDefaultAttributeMask = SFGAO_CANCOPY | SFGAO_CANMOVE | SFGAO_STORAGE | SFGAO_CANRENAME | SFGAO_CANDELETE |
                                     SFGAO_READONLY | SFGAO_STREAM | SFGAO_FOLDER;
static_assert(dwDefaultAttributeMask == 0x2044003B, "Unexpected default attribute mask");


struct TmpFile
{
    WCHAR Buffer[MAX_PATH] = {};

    void Create(LPCWSTR Folder)
    {
        GetTempFileNameW(Folder, L"SHG", 0, Buffer);
    }

    ~TmpFile()
    {
        if (Buffer[0])
        {
            SetFileAttributesW(Buffer, FILE_ATTRIBUTE_NORMAL);
            DeleteFileW(Buffer);
        }
    }
};


CComPtr<IShellFolder> _BindToObject(PCUIDLIST_ABSOLUTE pidl)
{
    CComPtr<IShellFolder> spDesktop, spResult;
    HRESULT hr = SHGetDesktopFolder(&spDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return spResult;

    if (FAILED_UNEXPECTEDLY(spDesktop->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &spResult))))
    {
        spResult.Release();
    }
    return spResult;
}


static void ok_attributes_(IDataObject* pDataObject, HRESULT expect_hr, DWORD expect_mask, DWORD expect_attr, UINT expect_items)
{
    FORMATETC fmt = { g_DataObjectAttributes, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM medium = {};

    HRESULT hr = pDataObject->GetData(&fmt, &medium);
    winetest_ok(hr == expect_hr, "Unexpected result from GetData, got 0x%lx, expected 0x%lx\n", hr, expect_hr);

    if (hr == expect_hr && expect_hr == S_OK)
    {
        LPVOID blob = GlobalLock(medium.hGlobal);
        winetest_ok(blob != nullptr, "Failed to lock hGlobal\n");
        if (blob)
        {
            SIZE_T size = GlobalSize(medium.hGlobal);
            winetest_ok(size == 0xc, "Unexpected size, got %lu, expected 12\n", size);
            if (size == 0xc)
            {
                PDWORD data = (PDWORD)blob;
                winetest_ok(data[0] == expect_mask, "Unexpected mask, got 0x%lx, expected 0x%lx\n", data[0], expect_mask);
                winetest_ok(data[1] == expect_attr, "Unexpected attr, got 0x%lx, expected 0x%lx\n", data[1], expect_attr);
                winetest_ok(data[2] == expect_items, "Unexpected item count, got %lu, expected %u\n", data[2], expect_items);
            }
            GlobalUnlock(medium.hGlobal);
        }
    }

    if (SUCCEEDED(hr))
        ReleaseStgMedium(&medium);
}


#define ok_attributes           (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : ok_attributes_
#define ok_hr_ret(x, y)         ok_hr(x, y); if (x != y) return

static void test_SpecialCases()
{
    DWORD dwAttributeMask = 0, dwAttributes = 123;
    UINT cItems = 123;

    HRESULT hr = SHGetAttributesFromDataObject(nullptr, dwAttributeMask, &dwAttributes, &cItems);
    ok_hr(hr, S_OK);
    ok_int(dwAttributes, 0);
    ok_int(cItems, 0);

    cItems = 123;
    hr = SHGetAttributesFromDataObject(nullptr, dwAttributeMask, nullptr, &cItems);
    ok_hr(hr, S_OK);
    ok_int(cItems, 0);

    dwAttributes = 123;
    hr = SHGetAttributesFromDataObject(nullptr, dwAttributeMask, &dwAttributes, nullptr);
    ok_hr(hr, S_OK);
    ok_int(dwAttributes, 0);
}


static void test_AttributesRegistration()
{
    WCHAR Buffer[MAX_PATH] = {};

    GetModuleFileNameW(NULL, Buffer, _countof(Buffer));
    CComHeapPtr<ITEMIDLIST_ABSOLUTE> spPath(ILCreateFromPathW(Buffer));

    ok(spPath != nullptr, "Unable to create pidl from %S\n", Buffer);
    if (spPath == nullptr)
        return;

    SFGAOF attributes = dwDefaultAttributeMask;
    HRESULT hr;
    {
        CComPtr<IShellFolder> spFolder;
        PCUITEMID_CHILD child;
        hr = SHBindToParent(spPath, IID_PPV_ARG(IShellFolder, &spFolder), &child);
        ok_hr_ret(hr, S_OK);

        hr = spFolder->GetAttributesOf(1, &child, &attributes);
        ok_hr_ret(hr, S_OK);

        attributes &= dwDefaultAttributeMask;
    }

    CComHeapPtr<ITEMIDLIST> parent(ILClone(spPath));
    PCIDLIST_RELATIVE child = ILFindLastID(spPath);
    ILRemoveLastID(parent);

    CComPtr<IDataObject> spDataObject;
    hr = CIDLData_CreateFromIDArray(parent, 1, &child, &spDataObject);
    ok_hr_ret(hr, S_OK);

    /* Not registered yet */
    ok_attributes(spDataObject, DV_E_FORMATETC, 0, 0, 0);

    /* Ask for attributes, without specifying any */
    DWORD dwAttributeMask = 0, dwAttributes = 0;
    UINT cItems = 0;
    hr = SHGetAttributesFromDataObject(spDataObject, dwAttributeMask, &dwAttributes, &cItems);
    ok_hr(hr, S_OK);

    /* Now there are attributes registered */
    ok_attributes(spDataObject, S_OK, dwDefaultAttributeMask, attributes, 1);

    // Now add an additional mask value (our exe should have a propsheet!)
    dwAttributeMask = SFGAO_HASPROPSHEET;
    dwAttributes = 0;
    cItems = 0;
    hr = SHGetAttributesFromDataObject(spDataObject, dwAttributeMask, &dwAttributes, &cItems);
    ok_hr(hr, S_OK);

    // Observe that this is now also cached
    ok_attributes(spDataObject, S_OK, dwDefaultAttributeMask | SFGAO_HASPROPSHEET, attributes | SFGAO_HASPROPSHEET, 1);
}

static void test_MultipleFiles()
{
    TmpFile TmpFile1, TmpFile2, TmpFile3;

    CComHeapPtr<ITEMIDLIST> pidl_tmpfolder;
    CComHeapPtr<ITEMIDLIST> pidl1, pidl2, pidl3;

    ITEMIDLIST* items[3] = {};
    SFGAOF attributes_first = dwDefaultAttributeMask;
    SFGAOF attributes2 = dwDefaultAttributeMask;
    SFGAOF attributes3 = dwDefaultAttributeMask;
    SFGAOF attributes_last = dwDefaultAttributeMask;

    HRESULT hr;
    {
        WCHAR TempFolder[MAX_PATH] = {};
        GetTempPathW(_countof(TempFolder), TempFolder);

        // Create temp files
        TmpFile1.Create(TempFolder);
        TmpFile2.Create(TempFolder);
        TmpFile3.Create(TempFolder);

        // Last file is read-only
        SetFileAttributesW(TmpFile3.Buffer, FILE_ATTRIBUTE_READONLY);

        hr = SHParseDisplayName(TempFolder, NULL, &pidl_tmpfolder, NULL, NULL);
        ok_hr_ret(hr, S_OK);

        CComPtr<IShellFolder> spFolder = _BindToObject(pidl_tmpfolder);
        ok(!!spFolder, "Unable to bind to tmp folder\n");
        if (!spFolder)
            return;

        hr = spFolder->ParseDisplayName(NULL, 0, PathFindFileNameW(TmpFile1.Buffer), NULL, &pidl1, NULL);
        ok_hr_ret(hr, S_OK);

        hr = spFolder->ParseDisplayName(NULL, 0, PathFindFileNameW(TmpFile2.Buffer), NULL, &pidl2, NULL);
        ok_hr_ret(hr, S_OK);

        hr = spFolder->ParseDisplayName(NULL, 0, PathFindFileNameW(TmpFile3.Buffer), NULL, &pidl3, NULL);
        ok_hr_ret(hr, S_OK);

        items[0] = pidl1;
        items[1] = pidl2;
        items[2] = pidl3;

        // Query file attributes
        hr = spFolder->GetAttributesOf(1, items, &attributes_first);
        ok_hr(hr, S_OK);

        hr = spFolder->GetAttributesOf(2, items, &attributes2);
        ok_hr(hr, S_OK);

        hr = spFolder->GetAttributesOf(3, items, &attributes3);
        ok_hr(hr, S_OK);

        hr = spFolder->GetAttributesOf(1, items + 2, &attributes_last);
        ok_hr(hr, S_OK);

        // Ignore any non-default attributes
        attributes_first &= dwDefaultAttributeMask;
        attributes2 &= dwDefaultAttributeMask;
        attributes3 &= dwDefaultAttributeMask;
        attributes_last &= dwDefaultAttributeMask;
    }

    // Only 'single' files have the stream attribute set
    ok(attributes_first & SFGAO_STREAM, "Expected SFGAO_STREAM on attributes_first (0x%lx)\n", attributes_first);
    ok(!(attributes2 & SFGAO_STREAM), "Expected no SFGAO_STREAM on attributes2 (0x%lx)\n", attributes2);
    ok(!(attributes3 & SFGAO_STREAM), "Expected no SFGAO_STREAM on attributes3 (0x%lx)\n", attributes3);
    ok(attributes_last & SFGAO_STREAM, "Expected SFGAO_STREAM on attributes_last (0x%lx)\n", attributes_last);

    // Only attributes common on all are returned, so only the last has the readonly bit set!
    ok(!(attributes_first & SFGAO_READONLY), "Expected no SFGAO_READONLY on attributes_first (0x%lx)\n", attributes_first);
    ok(!(attributes2 & SFGAO_READONLY), "Expected no SFGAO_READONLY on attributes2 (0x%lx)\n", attributes2);
    ok(!(attributes3 & SFGAO_READONLY), "Expected no SFGAO_READONLY on attributes3 (0x%lx)\n", attributes3);
    ok(attributes_last & SFGAO_READONLY, "Expected SFGAO_READONLY on attributes_last (0x%lx)\n", attributes_last);

    // The actual tests
    {
        // Just the first file
        CComPtr<IDataObject> spDataObject;
        hr = CIDLData_CreateFromIDArray(pidl_tmpfolder, 1, items, &spDataObject);
        ok_hr_ret(hr, S_OK);

        DWORD dwAttributeMask = 0, dwAttributes = 123;
        UINT cItems = 123;
        hr = SHGetAttributesFromDataObject(spDataObject, dwAttributeMask, &dwAttributes, &cItems);
        ok_hr(hr, S_OK);
        ok_attributes(spDataObject, S_OK, dwDefaultAttributeMask, attributes_first, 1);
    }

    {
        // First 2 files
        CComPtr<IDataObject> spDataObject;
        hr = CIDLData_CreateFromIDArray(pidl_tmpfolder, 2, items, &spDataObject);
        ok_hr_ret(hr, S_OK);

        DWORD dwAttributeMask = 0, dwAttributes = 123;
        UINT cItems = 123;
        hr = SHGetAttributesFromDataObject(spDataObject, dwAttributeMask, &dwAttributes, &cItems);
        ok_hr(hr, S_OK);
        ok_attributes(spDataObject, S_OK, dwDefaultAttributeMask, attributes2, 2);
    }

    {
        // All 3 files
        CComPtr<IDataObject> spDataObject;
        hr = CIDLData_CreateFromIDArray(pidl_tmpfolder, 3, items, &spDataObject);
        ok_hr_ret(hr, S_OK);

        DWORD dwAttributeMask = 0, dwAttributes = 123;
        UINT cItems = 123;
        hr = SHGetAttributesFromDataObject(spDataObject, dwAttributeMask, &dwAttributes, &cItems);
        ok_hr(hr, S_OK);
        ok_attributes(spDataObject, S_OK, dwDefaultAttributeMask, attributes3, 3);
    }

    {
        // Only the last file
        CComPtr<IDataObject> spDataObject;
        hr = CIDLData_CreateFromIDArray(pidl_tmpfolder, 1, items + 2, &spDataObject);
        ok_hr_ret(hr, S_OK);

        DWORD dwAttributeMask = 0, dwAttributes = 123;
        UINT cItems = 123;
        hr = SHGetAttributesFromDataObject(spDataObject, dwAttributeMask, &dwAttributes, &cItems);
        ok_hr(hr, S_OK);
        ok_attributes(spDataObject, S_OK, dwDefaultAttributeMask, attributes_last, 1);
    }
}

START_TEST(SHGetAttributesFromDataObject)
{
    HRESULT hr;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ok_hr(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    g_DataObjectAttributes = (CLIPFORMAT)RegisterClipboardFormatW(L"DataObjectAttributes");
    ok(g_DataObjectAttributes != 0, "Unable to register DataObjectAttributes\n");

    test_SpecialCases();
    test_AttributesRegistration();
    test_MultipleFiles();

    CoUninitialize();
}
