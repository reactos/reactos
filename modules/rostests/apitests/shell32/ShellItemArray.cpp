/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for CShellItemArray::GetAttributes
 * COPYRIGHT:   Copyright 2026 Alex Mendoza <05alex.mendozaa@gmail.com>
 */

#include "shelltest.h"
#include <shellutils.h>

#define SENTINEL 0xDEADBEEF

typedef HRESULT (WINAPI *tSHCreateShellItemArrayFromDataObject)(IDataObject *pdo, REFIID riid, void **ppv);
static tSHCreateShellItemArrayFromDataObject pSHCreateShellItemArrayFromDataObject;

static WCHAR g_Root[MAX_PATH];
static WCHAR g_Paths[3][MAX_PATH];

struct ARRAY_SET
{
    const char *Name;
    UINT Count;
    UINT Index[3];
};

static const ARRAY_SET g_Sets[] =
{
    { "file",          1, { 0 } },
    { "dir",           1, { 1 } },
    { "file+dir",      2, { 0, 1 } },
    { "dir+dir2",      2, { 1, 2 } },
    { "dir+file+dir2", 3, { 1, 0, 2 } },
};

struct ATTR_CASE
{
    UINT Set;
    DWORD Flags;
    SFGAOF Mask;
    HRESULT hrExpected;
    SFGAOF Expected;
};

static const ATTR_CASE g_Cases[] =
{
    { 0, SIATTRIBFLAGS_AND, SFGAO_FOLDER, S_FALSE, 0 },
    { 0, SIATTRIBFLAGS_OR, SFGAO_FOLDER, S_FALSE, 0 },
    { 0, SIATTRIBFLAGS_AND, SFGAO_FOLDER | SFGAO_FILESYSTEM, S_FALSE, SFGAO_FILESYSTEM },
    { 1, SIATTRIBFLAGS_AND, SFGAO_FOLDER, S_OK, SFGAO_FOLDER },
    { 1, SIATTRIBFLAGS_OR, SFGAO_FOLDER, S_OK, SFGAO_FOLDER },
    { 2, SIATTRIBFLAGS_AND, SFGAO_FOLDER, S_FALSE, 0 },
    { 2, SIATTRIBFLAGS_OR, SFGAO_FOLDER, S_OK, SFGAO_FOLDER },
    { 2, SIATTRIBFLAGS_AND, SFGAO_FOLDER | SFGAO_FILESYSTEM, S_FALSE, SFGAO_FILESYSTEM },
    { 2, SIATTRIBFLAGS_OR, SFGAO_FOLDER | SFGAO_FILESYSTEM, S_OK, SFGAO_FOLDER | SFGAO_FILESYSTEM },
    { 2, (DWORD)(SIATTRIBFLAGS_AND | SIATTRIBFLAGS_ALLITEMS), SFGAO_FOLDER, S_FALSE, 0 },
    { 3, SIATTRIBFLAGS_AND, SFGAO_FOLDER, S_OK, SFGAO_FOLDER },
    { 3, SIATTRIBFLAGS_OR, SFGAO_FOLDER, S_OK, SFGAO_FOLDER },
    { 4, SIATTRIBFLAGS_AND, SFGAO_FOLDER, S_FALSE, 0 },
    { 4, SIATTRIBFLAGS_OR, SFGAO_FOLDER, S_OK, SFGAO_FOLDER },
    { 4, SIATTRIBFLAGS_AND, SFGAO_FOLDER | SFGAO_FILESYSTEM, S_FALSE, SFGAO_FILESYSTEM },
    { 4, SIATTRIBFLAGS_OR, SFGAO_FOLDER | SFGAO_FILESYSTEM, S_OK, SFGAO_FOLDER | SFGAO_FILESYSTEM },
};

struct SET_CTX
{
    CComHeapPtr<ITEMIDLIST> Pidls[3];
    PCUITEMID_CHILD Children[3];
    CComPtr<IShellFolder> Folder;
    UINT Count;

    HRESULT Init(const ARRAY_SET &Set)
    {
        Count = Set.Count;

        for (UINT i = 0; i < Count; ++i)
        {
            HRESULT hr = SHParseDisplayName(g_Paths[Set.Index[i]], NULL, &Pidls[i], 0, NULL);
            if (FAILED(hr))
                return hr;

            Children[i] = ILFindLastID(Pidls[i]);
        }

        return SHBindToParent(Pidls[0], IID_PPV_ARG(IShellFolder, &Folder), NULL);
    }

    HRESULT CreateArray(IShellItemArray **ppsia)
    {
        CComPtr<IDataObject> pdo;
        HRESULT hr = Folder->GetUIObjectOf(NULL, Count, Children, IID_IDataObject, NULL, (void **)&pdo);
        if (FAILED(hr))
            return hr;

        return pSHCreateShellItemArrayFromDataObject(pdo, IID_IShellItemArray, (void **)ppsia);
    }
};

static BOOL SetupFiles()
{
    WCHAR Temp[MAX_PATH];
    HANDLE hFile;

    GetTempPathW(_countof(Temp), Temp);
    wsprintfW(g_Root, L"%lsSIA_%lu", Temp, GetCurrentProcessId());
    wsprintfW(g_Paths[0], L"%ls\\file.txt", g_Root);
    wsprintfW(g_Paths[1], L"%ls\\dir1", g_Root);
    wsprintfW(g_Paths[2], L"%ls\\dir2", g_Root);

    if (!CreateDirectoryW(g_Root, NULL))
        return FALSE;

    if (!CreateDirectoryW(g_Paths[1], NULL) || !CreateDirectoryW(g_Paths[2], NULL))
        return FALSE;

    hFile = CreateFileW(g_Paths[0], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    CloseHandle(hFile);
    return TRUE;
}

static void CleanupFiles()
{
    DeleteFileW(g_Paths[0]);
    RemoveDirectoryW(g_Paths[1]);
    RemoveDirectoryW(g_Paths[2]);
    RemoveDirectoryW(g_Root);
}

static void test_AndOr()
{
    for (UINT i = 0; i < _countof(g_Cases); ++i)
    {
        const ATTR_CASE &Case = g_Cases[i];
        const ARRAY_SET &Set = g_Sets[Case.Set];
        SET_CTX Ctx;
        CComPtr<IShellItemArray> psia;
        DWORD Count = 0;
        SFGAOF Out = SENTINEL;

        HRESULT hr = Ctx.Init(Set);
        ok(hr == S_OK, "%s: Init failed, hr=0x%lx\n", Set.Name, hr);
        if (FAILED(hr))
            continue;

        hr = Ctx.CreateArray(&psia);
        ok(hr == S_OK, "%s: CreateArray failed, hr=0x%lx\n", Set.Name, hr);
        if (FAILED(hr))
            continue;

        hr = psia->GetCount(&Count);
        ok(hr == S_OK && Count == Set.Count, "%s: GetCount hr=0x%lx count=%lu\n", Set.Name, hr, Count);

        hr = psia->GetAttributes((SIATTRIBFLAGS)Case.Flags, Case.Mask, &Out);
        ok(hr == Case.hrExpected, "%s flags=0x%lx mask=0x%lx: hr=0x%lx, expected 0x%lx\n",
           Set.Name, Case.Flags, Case.Mask, hr, Case.hrExpected);
        ok(Out == Case.Expected, "%s flags=0x%lx mask=0x%lx: out=0x%lx, expected 0x%lx\n",
           Set.Name, Case.Flags, Case.Mask, Out, Case.Expected);
    }
}

static void test_AppCompat()
{
    static const SFGAOF Masks[] = { SFGAO_FOLDER, SFGAO_FOLDER | SFGAO_FILESYSTEM };

    for (UINT s = 0; s < _countof(g_Sets); ++s)
    {
        for (UINT m = 0; m < _countof(Masks); ++m)
        {
            const ARRAY_SET &Set = g_Sets[s];
            SET_CTX Ctx;
            CComPtr<IShellItemArray> psia;
            SFGAOF Expected = Masks[m];
            SFGAOF Out = SENTINEL;
            HRESULT hrExpected;

            HRESULT hr = Ctx.Init(Set);
            ok(hr == S_OK, "%s: Init failed, hr=0x%lx\n", Set.Name, hr);
            if (FAILED(hr))
                continue;

            hr = Ctx.CreateArray(&psia);
            ok(hr == S_OK, "%s: CreateArray failed, hr=0x%lx\n", Set.Name, hr);
            if (FAILED(hr))
                continue;

            hr = Ctx.Folder->GetAttributesOf(Ctx.Count, Ctx.Children, &Expected);
            ok(SUCCEEDED(hr), "%s: GetAttributesOf failed, hr=0x%lx\n", Set.Name, hr);
            if (FAILED(hr))
                continue;

            Expected &= Masks[m];
            hrExpected = (Expected == Masks[m]) ? S_OK : S_FALSE;

            hr = psia->GetAttributes(SIATTRIBFLAGS_APPCOMPAT, Masks[m], &Out);
            ok(hr == hrExpected, "%s APPCOMPAT mask=0x%lx: hr=0x%lx, expected 0x%lx\n",
               Set.Name, Masks[m], hr, hrExpected);
            ok(Out == Expected, "%s APPCOMPAT mask=0x%lx: out=0x%lx, expected 0x%lx\n",
               Set.Name, Masks[m], Out, Expected);
        }
    }
}

START_TEST(ShellItemArray)
{
    CCoInit init;
    if (FAILED(init.hr))
    {
        skip("CoInitialize failed, hr=0x%lx\n", init.hr);
        return;
    }

    pSHCreateShellItemArrayFromDataObject = (tSHCreateShellItemArrayFromDataObject)
        GetProcAddress(GetModuleHandleW(L"shell32.dll"), "SHCreateShellItemArrayFromDataObject");
    if (!pSHCreateShellItemArrayFromDataObject)
    {
        skip("SHCreateShellItemArrayFromDataObject is not available\n");
        return;
    }

    if (SetupFiles())
    {
        test_AndOr();
        test_AppCompat();
    }
    else
    {
        skip("Unable to create the test files\n");
    }

    CleanupFiles();
}