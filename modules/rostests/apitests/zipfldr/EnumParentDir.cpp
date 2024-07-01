/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test the result of enumerating over a folder with a zip in it
 * COPYRIGHT:   Copyright 2020-2023 Mark Jansen (mark.jansen@reactos.org)
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
        if (!_wcsicmp(Array[n].Name, DisplayName) && !Array[n].Found)
        {
            Array[n].Found = true;
            ExpectedName = Array[n].Name;
            ExpectedAttributes = Array[n].Attributes;
            return;
        }
    }
}

const SFGAOF BaseFileAttributes = SFGAO_FILESYSTEM;
const SFGAOF BaseFolderAttributes = SFGAO_FILESYSTEM | SFGAO_FOLDER;
FileInfo ExpectedFiles[] = {
    { L"test.txt", BaseFileAttributes, false },
    { L"TMP0.zip", BaseFileAttributes, false},  // 2k3 Shows this as a file, newer shows this as a folder
    { L"ASUBFLD", BaseFolderAttributes | SFGAO_FILESYSANCESTOR | SFGAO_STORAGEANCESTOR, false },
};
BOOL FoundZipfldr = FALSE;

static void
test_EnumDirFiles(const WCHAR *TestFolder, BOOL EnumFolders)
{
    CComPtr<IShellFolder> spFolder;
    if (!InitializeShellFolder(TestFolder, spFolder))
        return;

    CComPtr<IEnumIDList> spEnum;
    ok_int(zipfldr_loaded(), FoundZipfldr);
    HRESULT hr = spFolder->EnumObjects(NULL, EnumFolders ? SHCONTF_FOLDERS : SHCONTF_NONFOLDERS, &spEnum);
    ok_hr(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;
    ok_int(zipfldr_loaded(), FoundZipfldr);

    do
    {
        CComHeapPtr<ITEMID_CHILD> child;
        ULONG celtFetched = 0;
        ok_int(zipfldr_loaded(), FoundZipfldr);
        hr = spEnum->Next(1, &child, &celtFetched);
        ok_int(zipfldr_loaded(), FoundZipfldr);
        if (hr != S_OK)
            break;
        ok_int(celtFetched, 1);
        if (celtFetched != 1)
            break;

        LPCWSTR ExpectedName;
        SFGAOF ExpectedAttributes;
        FindExpectedFile(ExpectedFiles, RTL_NUMBER_OF(ExpectedFiles), spFolder, child, ExpectedName, ExpectedAttributes);

        ok_int(zipfldr_loaded(), FoundZipfldr);
        ok_displayname(spFolder, child, SHGDN_NORMAL, ExpectedName);
        ok_int(zipfldr_loaded(), FoundZipfldr);

        SFGAOF Attributes = SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_HASSUBFOLDER;
        hr = spFolder->GetAttributesOf(1, &child, &Attributes);

        if (!_wcsicmp(ExpectedName, L"TMP0.zip"))
        {
            // We allow both .zip files being a 'file' (2k3) or a 'folder' (win10)
            if (Attributes & SFGAO_FOLDER)
                ExpectedAttributes |= SFGAO_FOLDER;
            // Only at this point (after calling GetAttributesOf) it will load zipfldr
            FoundZipfldr = TRUE;
            trace("Found zip (%S)\n", ExpectedName);
        }
        ok_int(zipfldr_loaded(), FoundZipfldr);

        /* Just keep the ones we are interested in */
        Attributes &= (SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR | SFGAO_STORAGEANCESTOR);
        ok_hr(hr, S_OK);
        ok_hex(Attributes, ExpectedAttributes);
    } while (true);

    ok_hr(hr, S_FALSE);
    ok_int(zipfldr_loaded(), FoundZipfldr);
}


START_TEST(EnumParentDir)
{
    HRESULT hr = CoInitialize(NULL);

    ok_hr(hr, S_OK);
    if (!SUCCEEDED(hr))
        return;

    ok_int(zipfldr_loaded(), FALSE);

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

        winetest_push_context("Files");
        test_EnumDirFiles(TestFolder, FALSE);
        winetest_pop_context();
        winetest_push_context("Folders");
        test_EnumDirFiles(TestFolder, TRUE);
        winetest_pop_context();

        for (size_t n = 0; n < RTL_NUMBER_OF(ExpectedFiles); ++n)
        {
            ok(ExpectedFiles[n].Found, "Did not find %S\n", ExpectedFiles[n].Name);
        }

        RemoveDirectoryW(SubFolder);

        CloseHandle(hFile);
    }

    DeleteFileW(ZipTestFile);
    RemoveDirectoryW(TestFolder);
    CoUninitialize();
}
