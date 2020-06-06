/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test the result of enumerating over a folder with a zip in it
 * COPYRIGHT:   Copyright 2020 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

struct FileInfo
{
    LPCWSTR Name;
    SFGAOF Attributes;
    bool Found;
};

int zipfldr_loaded()
{
    return GetModuleHandleA("zipfldr.dll") ? TRUE : FALSE;
}

void FindExpectedFile(FileInfo* Array, size_t len, IShellFolder* pFolder, PCUITEMID_CHILD pidl, LPCWSTR& ExpectedName, SFGAOF& ExpectedAttributes)
{
    ExpectedName = L"<WRONG FILE>";
    ExpectedAttributes = (SFGAOF)~0;

    STRRET NameRet;
    HRESULT hr;

    hr = pFolder->GetDisplayNameOf(pidl, SHGDN_NORMAL, &NameRet);
    ok_hr(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    WCHAR DisplayName[MAX_PATH];
    hr = StrRetToBufW(&NameRet, pidl, DisplayName, RTL_NUMBER_OF(DisplayName));
    ok_hr(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    for (size_t n = 0; n < len; ++n)
    {
        if (!wcsicmp(Array[n].Name, DisplayName) && !Array[n].Found)
        {
            Array[n].Found = true;
            ExpectedName = Array[n].Name;
            ExpectedAttributes = Array[n].Attributes;
            return;
        }
    }
}

static void test_EnumDirFiles(const WCHAR* TestFolder)
{
    CComPtr<IShellFolder> spFolder;
    if (!InitializeShellFolder(TestFolder, spFolder))
        return;

    CComPtr<IEnumIDList> spEnum;
    ok_int(zipfldr_loaded(), FALSE);
    HRESULT hr = spFolder->EnumObjects(NULL, SHCONTF_NONFOLDERS, &spEnum);
    ok_hr(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;
    ok_int(zipfldr_loaded(), FALSE);

    SFGAOF BaseAttributes = SFGAO_FILESYSTEM;
    FileInfo ExpectedFiles[] = {
        { L"test.txt", BaseAttributes, false },
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

        LPCWSTR ExpectedName;
        SFGAOF ExpectedAttributes;
        FindExpectedFile(ExpectedFiles, RTL_NUMBER_OF(ExpectedFiles), spFolder, child, ExpectedName, ExpectedAttributes);

        totalFetched++;

        ok_displayname(spFolder, child, SHGDN_NORMAL, ExpectedName);

        SFGAOF Attributes = SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_HASSUBFOLDER;
        hr = spFolder->GetAttributesOf(1, &child, &Attributes);
        /* Just keep the ones we are interested in */
        Attributes &= (SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR | SFGAO_STORAGEANCESTOR);
        ok_hr(hr, S_OK);
        ok_hex(Attributes, ExpectedAttributes);
    } while (true);

    ok_int(totalFetched, RTL_NUMBER_OF(ExpectedFiles));
    ok_hr(hr, S_FALSE);
    ok_int(zipfldr_loaded(), FALSE);
}

static void test_EnumDirDirs(const WCHAR* TestFolder)
{
    CComPtr<IShellFolder> spFolder;
    if (!InitializeShellFolder(TestFolder, spFolder))
        return;

    CComPtr<IEnumIDList> spEnum;
    ok_int(zipfldr_loaded(), FALSE);
    HRESULT hr = spFolder->EnumObjects(NULL, SHCONTF_FOLDERS, &spEnum);
    ok_hr(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    ok_int(zipfldr_loaded(), FALSE);

    SFGAOF BaseAttributes = SFGAO_FILESYSTEM | SFGAO_FOLDER;
    FileInfo ExpectedFiles[] = {
        { L"TMP0.zip", BaseAttributes, false},
        { L"ASUBFLD", BaseAttributes | SFGAO_FILESYSANCESTOR | SFGAO_STORAGEANCESTOR, false },
    };

    bool bFoundZipfldr = false;

    ULONG totalFetched = 0;
    do
    {
        CComHeapPtr<ITEMID_CHILD> child;
        ULONG celtFetched = 0;
        ok_int(zipfldr_loaded(), bFoundZipfldr ? TRUE : FALSE);
        hr = spEnum->Next(1, &child, &celtFetched);
        if (hr != S_OK)
            break;
        ok_int(celtFetched, 1);
        if (celtFetched != 1)
            break;

        ok_int(zipfldr_loaded(), bFoundZipfldr ? TRUE : FALSE);

        LPCWSTR ExpectedName;
        SFGAOF ExpectedAttributes;
        FindExpectedFile(ExpectedFiles, RTL_NUMBER_OF(ExpectedFiles), spFolder, child, ExpectedName, ExpectedAttributes);

        totalFetched++;

        ok_displayname(spFolder, child, SHGDN_NORMAL, ExpectedName);
        trace("Current: %S\n", ExpectedName);

        SFGAOF Attributes = SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_HASSUBFOLDER;
        hr = spFolder->GetAttributesOf(1, &child, &Attributes);
        if ((ExpectedAttributes & SFGAO_FILESYSANCESTOR))
        {
            ok_int(zipfldr_loaded(), bFoundZipfldr ? TRUE : FALSE);
        }
        else
        {
            ok_int(zipfldr_loaded(), TRUE);
            bFoundZipfldr = true;
        }
        /* Just keep the ones we are interested in */
        Attributes &= (SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR | SFGAO_STORAGEANCESTOR);
        ok_hr(hr, S_OK);
        ok_hex(Attributes, ExpectedAttributes);
    } while (true);

    ok_int(totalFetched, RTL_NUMBER_OF(ExpectedFiles));
    ok_hr(hr, S_FALSE);
}


START_TEST(EnumParentDir)
{
    HRESULT hr = CoInitialize(NULL);

    ok_hr(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    WCHAR TestFolder[MAX_PATH], TestFile[MAX_PATH], SubFolder[MAX_PATH];
    GetTempPathW(_countof(TestFolder), TestFolder);
    PathAppendW(TestFolder, L"ZipDir");
    PathAddBackslashW(TestFolder);

    CreateDirectoryW(TestFolder, NULL);

    WCHAR ZipTestFile[MAX_PATH];
    if (!extract_resource(ZipTestFile, MAKEINTRESOURCEW(IDR_ZIP_TEST_FILE), TestFolder))
    {
        RemoveDirectoryW(TestFolder);
        return;
    }

    StringCchPrintfW(TestFile, _countof(TestFile), L"%stest.txt", TestFolder);

    HANDLE hFile = CreateFileW(TestFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE, NULL);

    ok(hFile != INVALID_HANDLE_VALUE, "Error creating %S\n", TestFile);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        StringCchPrintfW(SubFolder, _countof(SubFolder), L"%sASUBFLD", TestFolder);

        CreateDirectoryW(SubFolder, NULL);

        test_EnumDirFiles(TestFolder);
        test_EnumDirDirs(TestFolder);

        RemoveDirectoryW(SubFolder);

        CloseHandle(hFile);
    }

    DeleteFileW(ZipTestFile);
    RemoveDirectoryW(TestFolder);
    CoUninitialize();
}
