/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for zipfldr
 * COPYRIGHT:   Copyright 2020 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

static bool g_bOldZipfldr = false;

void ok_displayname_(const char* file, int line, IShellFolder* pFolder, PCUITEMID_CHILD pidl, SHGDNF Flags, LPCWSTR Name)
{
    STRRET NameRet;
    HRESULT hr;

    hr = pFolder->GetDisplayNameOf(pidl, Flags, &NameRet);
    ok_hr_(file, line, hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    WCHAR DisplayName[MAX_PATH];
    hr = StrRetToBufW(&NameRet, pidl, DisplayName, RTL_NUMBER_OF(DisplayName));
    ok_hr_(file, line, hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    ok_wstr_(file, line, DisplayName, Name);
}

struct ZipFiles
{
    LPCWSTR Name;
    SFGAOF Attributes;
};

static CLIPFORMAT cfHIDA = RegisterClipboardFormatA(CFSTR_SHELLIDLISTA);
static CLIPFORMAT cfFileDescriptor = RegisterClipboardFormatW(CFSTR_FILEDESCRIPTORW);

static void test_EnumObjects_Files(const WCHAR* Filename, IShellFolder* pFolder)
{
    CComPtr<IEnumIDList> spEnum;
    HRESULT hr = pFolder->EnumObjects(NULL, SHCONTF_NONFOLDERS, &spEnum);
    ok_hr(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    SFGAOF BaseAttributes = SFGAO_STREAM | SFGAO_HASPROPSHEET | SFGAO_CANDELETE | SFGAO_CANCOPY | SFGAO_CANMOVE;
    ZipFiles ExpectedFiles[] = {
        { L"cccc.txt", BaseAttributes },
        { L"bbbb.txt", BaseAttributes },
    };

    ULONG totalFetched = 0;
    do
    {
        CComHeapPtr<ITEMID_CHILD> child;
        ULONG celtFetched = 0;
        hr = spEnum->Next(1, &child, &celtFetched);
        if (hr != S_OK)
            break;
        ok_int(celtFetched, 1);
        if (celtFetched != 1)
            break;

        LPCWSTR ExpectedName = totalFetched < RTL_NUMBER_OF(ExpectedFiles) ? ExpectedFiles[totalFetched].Name : L"<TOO MANY FILES>";
        SFGAOF ExpectedAttributes = totalFetched < RTL_NUMBER_OF(ExpectedFiles) ? ExpectedFiles[totalFetched].Attributes : 0xdeaddead;

        totalFetched++;


        CComPtr<IDataObject> spDataObj;
        hr = pFolder->GetUIObjectOf(NULL, 1, &child, IID_IDataObject, NULL, reinterpret_cast<LPVOID*>(&spDataObj));
        ok_hr(hr, S_OK);

        if (!g_bOldZipfldr && !IsFormatAdvertised(spDataObj, cfHIDA, TYMED_HGLOBAL))
        {
            trace("Pre-Vista zipfldr\n");
            // No seconds in filetimes, less functional IStream* implementation
            g_bOldZipfldr = true;
        }

        ok_displayname(pFolder, child, SHGDN_NORMAL, ExpectedName);
        if (g_bOldZipfldr)
        {
            ok_displayname(pFolder, child, SHGDN_FORPARSING, ExpectedName);
        }
        else
        {
            WCHAR Buffer[MAX_PATH];
            StringCchPrintfW(Buffer, _countof(Buffer), L"%s\\%s", Filename, ExpectedName);
            ok_displayname(pFolder, child, SHGDN_FORPARSING, Buffer);
        }
        ok_displayname(pFolder, child, SHGDN_FORPARSING | SHGDN_INFOLDER, ExpectedName);
        ok_displayname(pFolder, child, SHGDN_FOREDITING , ExpectedName);
        ok_displayname(pFolder, child, SHGDN_FOREDITING | SHGDN_INFOLDER, ExpectedName);
        ok_displayname(pFolder, child, SHGDN_FORADDRESSBAR , ExpectedName);
        ok_displayname(pFolder, child, SHGDN_FORADDRESSBAR | SHGDN_INFOLDER, ExpectedName);

        SFGAOF Attributes = 0;
        hr = pFolder->GetAttributesOf(1, &child, &Attributes);
        ok_hr(hr, S_OK);
        ok_hex(Attributes, ExpectedAttributes);

        STGMEDIUM medium = {0};
        FORMATETC etc = { cfFileDescriptor, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        HRESULT hr = spDataObj->GetData(&etc, &medium);
        ok_hex(hr, S_OK);
        if (!SUCCEEDED(hr))
            continue;

        ok_hex(medium.tymed, TYMED_HGLOBAL);
        PVOID pData = GlobalLock(medium.hGlobal);
        FILEGROUPDESCRIPTORW* Descriptor = static_cast<FILEGROUPDESCRIPTORW*>(pData);
        ok_hex(Descriptor->cItems, 1);
        if (Descriptor->cItems == 1)
            ok_wstr(Descriptor->fgd[0].cFileName, ExpectedName);
        GlobalUnlock(medium.hGlobal);
        ReleaseStgMedium(&medium);
    } while (true);

    ok_int(totalFetched, RTL_NUMBER_OF(ExpectedFiles));
    ok_hr(hr, S_FALSE);
}

static void test_EnumObjects_Folders(const WCHAR* Filename, IShellFolder* pFolder)
{
    CComPtr<IEnumIDList> spEnum;
    HRESULT hr = pFolder->EnumObjects(NULL, SHCONTF_FOLDERS, &spEnum);
    ok_hr(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    SFGAOF BaseAttributes = SFGAO_FOLDER | SFGAO_DROPTARGET | SFGAO_HASPROPSHEET | SFGAO_CANDELETE | SFGAO_CANCOPY | SFGAO_CANMOVE;
    ZipFiles ExpectedFolders[] = {
        { L"aaaa", BaseAttributes | (g_bOldZipfldr ? 0 : SFGAO_STORAGE) },
        { L"cccc", BaseAttributes | (g_bOldZipfldr ? 0 : SFGAO_STORAGE) },
    };

    LPCWSTR ExpectedExtraFiles[2][4] =
    {
        {
            L"aaaa\\a_aaaa",
            L"aaaa\\a_cccc",
            L"aaaa\\a_bbbb.txt",
            L"aaaa\\a_cccc.txt",
        },
        {
            L"cccc\\c_cccc",
            L"cccc\\c_bbbb.txt",
            L"cccc\\c_cccc.txt",
            L"cccc\\c_aaaa",
        },
    };

    ULONG totalFetched = 0;
    do
    {
        CComHeapPtr<ITEMID_CHILD> child;
        ULONG celtFetched = 0;
        hr = spEnum->Next(1, &child, &celtFetched);
        if (hr != S_OK)
            break;
        ok_int(celtFetched, 1);
        if (celtFetched != 1)
            break;

        LPCWSTR ExpectedName = totalFetched < RTL_NUMBER_OF(ExpectedFolders) ? ExpectedFolders[totalFetched].Name : L"<TOO MANY FILES>";
        SFGAOF ExpectedAttributes = totalFetched < RTL_NUMBER_OF(ExpectedFolders) ? ExpectedFolders[totalFetched].Attributes : 0xdeaddead;
        LPCWSTR* ExtraFiles = totalFetched < RTL_NUMBER_OF(ExpectedExtraFiles) ? ExpectedExtraFiles[totalFetched] : ExpectedExtraFiles[0];

        totalFetched++;

        ok_displayname(pFolder, child, SHGDN_NORMAL, ExpectedName);
        if (g_bOldZipfldr)
        {
            ok_displayname(pFolder, child, SHGDN_FORPARSING, ExpectedName);
        }
        else
        {
            WCHAR Buffer[MAX_PATH];
            StringCchPrintfW(Buffer, _countof(Buffer), L"%s\\%s", Filename, ExpectedName);
            ok_displayname(pFolder, child, SHGDN_FORPARSING, Buffer);
        }
        ok_displayname(pFolder, child, SHGDN_FORPARSING | SHGDN_INFOLDER, ExpectedName);
        ok_displayname(pFolder, child, SHGDN_FOREDITING , ExpectedName);
        ok_displayname(pFolder, child, SHGDN_FOREDITING | SHGDN_INFOLDER, ExpectedName);

        SFGAOF Attributes = 0;
        hr = pFolder->GetAttributesOf(1, &child, &Attributes);
        ok_hr(hr, S_OK);
        ok_hex(Attributes, ExpectedAttributes);

        CComPtr<IDataObject> spData;
        hr = pFolder->GetUIObjectOf(NULL, 1, &child, IID_IDataObject, NULL, reinterpret_cast<LPVOID*>(&spData));
        ok_hr(hr, S_OK);

        STGMEDIUM medium = {0};
        FORMATETC etc = { cfFileDescriptor, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        HRESULT hr = spData->GetData(&etc, &medium);
        ok_hex(hr, S_OK);
        if (!SUCCEEDED(hr))
            continue;

        ok_hex(medium.tymed, TYMED_HGLOBAL);
        PVOID pData = GlobalLock(medium.hGlobal);
        FILEGROUPDESCRIPTORW* Descriptor = static_cast<FILEGROUPDESCRIPTORW*>(pData);
        ok_hex(Descriptor->cItems, 5);
        if (Descriptor->cItems == 5)
        {
            ok_wstr(Descriptor->fgd[0].cFileName, ExpectedName);
            ok_wstr(Descriptor->fgd[1].cFileName, ExtraFiles[0]);
            ok_wstr(Descriptor->fgd[2].cFileName, ExtraFiles[1]);
            ok_wstr(Descriptor->fgd[3].cFileName, ExtraFiles[2]);
            ok_wstr(Descriptor->fgd[4].cFileName, ExtraFiles[3]);
        }
        GlobalUnlock(medium.hGlobal);
        ReleaseStgMedium(&medium);
    } while (true);

    ok_int(totalFetched, RTL_NUMBER_OF(ExpectedFolders));
    ok_hr(hr, S_FALSE);
}

static void test_EnumObjects(const WCHAR* Filename)
{
    CComPtr<IShellFolder> spFolder;
    if (!InitializeShellFolder(Filename, spFolder))
        return;

    // Let's ask the shell how it wants to display the full path to our zip file
    WCHAR ZipFilename[MAX_PATH] = L"<Failed to query zip path>";
    {
        CComPtr<IPersistFolder2> spPersistFolder;
        HRESULT hr = spFolder->QueryInterface(IID_PPV_ARG(IPersistFolder2, &spPersistFolder));
        ok_hr(hr, S_OK);
        if (SUCCEEDED(hr))
        {
            CComHeapPtr<ITEMIDLIST_ABSOLUTE> zipPidl;
            hr = spPersistFolder->GetCurFolder(&zipPidl);
            ok_hr(hr, S_OK);
            if (SUCCEEDED(hr))
            {
                BOOL bHasPath = SHGetPathFromIDListW(zipPidl, ZipFilename);
                ok_int(bHasPath, TRUE);
            }
        }
    }


    test_EnumObjects_Files(ZipFilename, spFolder);
    test_EnumObjects_Folders(ZipFilename, spFolder);
}

START_TEST(EnumObjects)
{
    skip("Code in zipfldr not implemented yet\n");
    return;

    HRESULT hr = CoInitialize(NULL);

    ok_hr(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    WCHAR ZipTestFile[MAX_PATH];
    if (!extract_resource(ZipTestFile, MAKEINTRESOURCEW(IDR_ZIP_TEST_ENUM), NULL))
        return;
    test_EnumObjects(ZipTestFile);
    DeleteFileW(ZipTestFile);
    CoUninitialize();
}
