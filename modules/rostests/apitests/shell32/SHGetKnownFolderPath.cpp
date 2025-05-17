/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for SHGetKnownFolderPath
 * COPYRIGHT:   Copyright 2025 Petru Răzvan (petrurazvan@proton.me)
 */
#include "shelltest.h"

typedef HRESULT (WINAPI *PSHGETKNOWNFOLDERPATH)(REFKNOWNFOLDERID, DWORD, HANDLE, PWSTR *);

static PSHGETKNOWNFOLDERPATH pSHGetKnownFolderPath = NULL;

START_TEST(SHGetKnownFolderPath)
{
    HRESULT hr;
    HINSTANCE hShell32;
    PWSTR path;
    HRESULT result;

    hShell32 = LoadLibraryW(L"shell32.dll");
    if (!hShell32)
    {
        skip("Failed to load shell32.dll\n");
        return;
    }

    pSHGetKnownFolderPath = (PSHGETKNOWNFOLDERPATH)GetProcAddress(hShell32, "SHGetKnownFolderPath");

    if (!pSHGetKnownFolderPath)
    {
        skip("SHGetKnownFolderPath not exported. Likely running on an OS older than Vista\n");
        FreeLibrary(hShell32);
        return;
    }

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        skip("Failed to initialize COM library. Error Code: 0x%08lX\n", hr);
    }
    else
    {
        path = NULL;
        result = pSHGetKnownFolderPath(FOLDERID_Desktop, 0, NULL, &path);
        if (result == S_OK)
        {
            ok(path != NULL, "Desktop: Expected success (hr=0x%lx), got path %ls\n", result, path);
        }
        else if (result == S_FALSE)
        {
             ok(path == NULL, "Desktop: Expected S_FALSE, got hr=0x%lx, path %p\n", result, path);
        }
        else
        {
            ok(0, "Desktop: Expected success, got Error Code: 0x%08lX\n", result);
            ok(path == NULL, "Desktop: Expected path to be NULL on failure\n");
        }
        if (path)
            SHFree(path);

        path = NULL;
        result = pSHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path);
        if (SUCCEEDED(result))
        {
            ok(path != NULL, "Documents: Expected success (hr=0x%lx), got path %ls\n", result, path);
        }
        else if (result == S_FALSE)
        {
             ok(path == NULL, "Documents: Expected S_FALSE, got hr=0x%lx, path %p\n", result, path);
        }
         else
        {
            ok(0, "Documents: Expected success, got Error Code: 0x0%08lX\n", result);
            ok(path == NULL, "Documents: Expected path to be NULL on failure\n");
        }
        if (path)
            SHFree(path);

        CoUninitialize();
    }

    FreeLibrary(hShell32);
}
