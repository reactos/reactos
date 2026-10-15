/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for RegenerateUserEnvironment
 * PROGRAMMER:  Alex Mendoza
 */

#include "shelltest.h"
#include <userenv.h>

#define NDEBUG
#include <debug.h>

typedef BOOL (WINAPI *PFN_REGENERATE_USER_ENVIRONMENT)(PVOID *pPrevEnv, BOOL bSetCurrentEnv);
static PFN_REGENERATE_USER_ENVIRONMENT pRegenerateUserEnvironment = NULL;

static BOOL IsValidEnvBlock(PVOID pEnv)
{
    LPWCH p = (LPWCH)pEnv;

    if (!p || *p == L'\0')
        return FALSE;

    /* Walk the block but bail out early if it looks corrupted */
    while (*p != L'\0')
    {
        SIZE_T len = wcslen(p);
        if (len == 0)
            return FALSE;
        p += len + 1;
    }

    return TRUE;
}

static void Test_BasicCall(void)
{
    PVOID pEnv = NULL;
    BOOL ret;

    ret = pRegenerateUserEnvironment(&pEnv, TRUE);
    ok(ret != FALSE,
       "RegenerateUserEnvironment(&pEnv, TRUE) failed with last error %lu\n",
       GetLastError());

    pEnv = NULL;
    ret = pRegenerateUserEnvironment(&pEnv, FALSE);
    ok(ret != FALSE,
       "RegenerateUserEnvironment(&pEnv, FALSE) failed with last error %lu\n",
       GetLastError());
    if (pEnv)
        DestroyEnvironmentBlock(pEnv);
}

static void Test_PreviousEnvironmentReturned(void)
{
    PVOID pPrev = NULL;
    BOOL ret;

    ret = pRegenerateUserEnvironment(&pPrev, FALSE);
    ok(ret != FALSE,
       "RegenerateUserEnvironment(&pPrev, FALSE) failed with last error %lu\n",
       GetLastError());

    if (!ret)
    {
        skip("RegenerateUserEnvironment failed; skipping pPrev checks.\n");
        return;
    }

    ok(pPrev != NULL,
       "pPrev must not be NULL when bSetCurrentEnv is FALSE\n");

    if (!pPrev)
        return;

    ok(IsValidEnvBlock(pPrev),
       "pPrev does not point to a valid environment block\n");

    DestroyEnvironmentBlock(pPrev);
}

static void Test_RegistryVariablePickedUp(void)
{
    static const WCHAR szKey[]     = L"Environment";
    static const WCHAR szVarName[] = L"__REGEN_TEST_VAR__";
    static const WCHAR szVarValue[]= L"ReactOS_RegenerateTest";

    HKEY hKey;
    LONG lRet;
    BOOL ret;
    WCHAR szBuf[64];

    lRet = RegOpenKeyExW(HKEY_CURRENT_USER, szKey, 0, KEY_SET_VALUE, &hKey);
    if (lRet != ERROR_SUCCESS)
    {
        skip("Cannot open HKCU\\Environment (error %ld); skipping registry test.\n", lRet);
        return;
    }

    /* Write canary value */
    lRet = RegSetValueExW(hKey, szVarName, 0, REG_SZ,
                          (const BYTE *)szVarValue,
                          (DWORD)((wcslen(szVarValue) + 1) * sizeof(WCHAR)));
    ok(lRet == ERROR_SUCCESS,
       "RegSetValueExW failed: %ld\n", lRet);

    if (lRet != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return;
    }

    /* Regenerate and apply to current process */
    PVOID pEnv = NULL;
    ret = pRegenerateUserEnvironment(&pEnv, TRUE);
    ok(ret != FALSE,
       "RegenerateUserEnvironment failed: %lu\n", GetLastError());

    /* The canary should be visible with GetEnvironmentVariableW */
    szBuf[0] = L'\0';
    GetEnvironmentVariableW(szVarName, szBuf, _countof(szBuf));
    ok(wcscmp(szBuf, szVarValue) == 0,
       "Expected env var '%ls' = '%ls', got '%ls'\n",
       szVarName, szVarValue, szBuf);

    /* Remove canary from registry */
    RegDeleteValueW(hKey, szVarName);
    RegCloseKey(hKey);

    PVOID pEnv2 = NULL;
    pRegenerateUserEnvironment(&pEnv2, TRUE);
}

START_TEST(RegenerateUserEnvironment)
{
    HMODULE hShell32 = GetModuleHandleW(L"shell32.dll");

    if (!hShell32)
        hShell32 = LoadLibraryW(L"shell32.dll");

    pRegenerateUserEnvironment =
        (PFN_REGENERATE_USER_ENVIRONMENT)GetProcAddress(hShell32, "RegenerateUserEnvironment");

    if (!pRegenerateUserEnvironment)
    {
        skip("RegenerateUserEnvironment not found in shell32.dll\n");
        return;
    }

    Test_BasicCall();
    Test_PreviousEnvironmentReturned();
    Test_RegistryVariablePickedUp();
}
