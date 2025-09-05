/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for SHGetKnownFolderPath
 * COPYRIGHT:   Copyright 2025 Petru Răzvan (petrurazvan@proton.me)
 */
#include "shelltest.h"
#include <shlobj.h>
#include <windows.h>
#include <wchar.h>
#include <stdlib.h>
#include <string.h>

typedef HRESULT (WINAPI *PSHGETKNOWNFOLDERPATH)(REFKNOWNFOLDERID, DWORD, HANDLE, PWSTR *);
static PSHGETKNOWNFOLDERPATH pSHGetKnownFolderPath = NULL;

static BOOL PathStartsWith(PWSTR path, PWSTR base)
{
    int len = lstrlenW(base);
    if (len == 0)
        return FALSE;
    return _wcsnicmp(path, base, len) == 0;
}

static PWSTR GetUserProfilePath()
{
    DWORD size = GetEnvironmentVariableW(L"USERPROFILE", NULL, 0);
    if (size == 0)
        return NULL;
    PWSTR userProfile = (PWSTR)malloc(size * sizeof(wchar_t));
    if (!userProfile)
        return NULL;
    if (GetEnvironmentVariableW(L"USERPROFILE", userProfile, size) == 0)
    {
        free(userProfile);
        return NULL;
    }
    return userProfile;
}

START_TEST(SHGetKnownFolderPath)
{
    HRESULT hr;
    HMODULE hShell32;
    PWSTR path = NULL;
    PWSTR userProfile = NULL;
    PWSTR expectedDesktop = NULL;
    PWSTR expectedDocuments1 = NULL;
    PWSTR expectedDocuments2 = NULL;
    size_t len;

    hShell32 = GetModuleHandleW(L"shell32.dll");
    if (!hShell32)
    {
        skip("Failed to find shell32.dll\n");
        return;
    }

    pSHGetKnownFolderPath = (PSHGETKNOWNFOLDERPATH)GetProcAddress(hShell32, "SHGetKnownFolderPath");
    if (!pSHGetKnownFolderPath)
    {
        skip("SHGetKnownFolderPath not exported. Likely running on an OS older than Vista\n");
        return;
    }

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        skip("Failed to initialize COM library. Error Code: 0x%08lX\n", hr);
        return;
    }

    userProfile = GetUserProfilePath();
    if (!userProfile)
    {
        skip("Failed to get USERPROFILE environment variable\n");
        CoUninitialize();
        return;
    }

    len = wcslen(userProfile) + wcslen(L"\\Desktop") + 1;
    expectedDesktop = (PWSTR)malloc(len * sizeof(wchar_t));
    if (!expectedDesktop)
    {
        free(userProfile);
        CoUninitialize();
        skip("Failed to allocate memory\n");
        return;
    }
    wcscpy(expectedDesktop, userProfile);
    wcscat(expectedDesktop, L"\\Desktop");

    hr = pSHGetKnownFolderPath(FOLDERID_Desktop, 0, NULL, &path);
    ok(hr == S_OK, "SHGetKnownFolderPath(FOLDERID_Desktop) returned 0x%08lX, expected S_OK\n", hr);
    if (hr == S_OK)
    {
        ok(path != NULL, "Expected non-NULL path pointer for Desktop folder\n");
        ok(PathStartsWith(path, expectedDesktop),
           "Desktop path '%ls' does not start with expected user desktop path '%ls'\n",
           path, expectedDesktop);
        SHFree(path);
        path = NULL;
    }
    free(expectedDesktop);

    len = wcslen(userProfile) + wcslen(L"\\Documents") + 1;
    expectedDocuments1 = (PWSTR)malloc(len * sizeof(wchar_t));
    if (!expectedDocuments1)
    {
        free(userProfile);
        CoUninitialize();
        skip("Failed to allocate memory\n");
        return;
    }
    wcscpy(expectedDocuments1, userProfile);
    wcscat(expectedDocuments1, L"\\Documents");

    len = wcslen(userProfile) + wcslen(L"\\My Documents") + 1;
    expectedDocuments2 = (PWSTR)malloc(len * sizeof(wchar_t));
    if (!expectedDocuments2)
    {
        free(userProfile);
        free(expectedDocuments1);
        CoUninitialize();
        skip("Failed to allocate memory\n");
        return;
    }
    wcscpy(expectedDocuments2, userProfile);
    wcscat(expectedDocuments2, L"\\My Documents");

    hr = pSHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path);
    ok(hr == S_OK, "SHGetKnownFolderPath(FOLDERID_Documents) returned 0x%08lX, expected S_OK\n", hr);
    if (hr == S_OK)
    {
        ok(path != NULL, "Expected non-NULL path pointer for Documents folder\n");
        ok(PathStartsWith(path, expectedDocuments1) || PathStartsWith(path, expectedDocuments2),
           "Documents path '%ls' does not start with expected user documents path '%ls' or '%ls'\n",
           path, expectedDocuments1, expectedDocuments2);
        SHFree(path);
        path = NULL;
    }

    free(expectedDocuments1);
    free(expectedDocuments2);
    free(userProfile);

    CoUninitialize();
}
