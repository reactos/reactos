/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Tests for Load/UnloadUserProfile
 * PROGRAMMERS:     Hermes Belusca-Maito
 */

#include <apitest.h>
// #include <windef.h>
// #include <winbase.h>
#include <sddl.h>
#include <userenv.h>
#include <strsafe.h>

#undef SE_RESTORE_NAME
#undef SE_BACKUP_NAME
#define SE_RESTORE_NAME     L"SeRestorePrivilege"
#define SE_BACKUP_NAME      L"SeBackupPrivilege"

/*
 * Taken from dll/win32/shell32/dialogs/dialogs.cpp ;
 * See also base/applications/shutdown/shutdown.c .
 */
static BOOL
EnablePrivilege(LPCWSTR lpszPrivilegeName, BOOL bEnablePrivilege)
{
    BOOL   Success;
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;

    Success = OpenProcessToken(GetCurrentProcess(),
                               TOKEN_ADJUST_PRIVILEGES,
                               &hToken);
    if (!Success) return Success;

    Success = LookupPrivilegeValueW(NULL,
                                    lpszPrivilegeName,
                                    &tp.Privileges[0].Luid);
    if (!Success) goto Quit;

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = (bEnablePrivilege ? SE_PRIVILEGE_ENABLED : 0);

    Success = AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL);

Quit:
    CloseHandle(hToken);
    return Success;
}

/*
 * Taken from dll/win32/userenv/sid.c .
 * We cannot directly use the USERENV.DLL export, because: 1) it is exported
 * by ordinal (#142), and: 2) it is simply not exported at all in Vista+
 * (and ordinal #142 is assigned there to LoadUserProfileA).
 */
PSID
WINAPI
GetUserSid(IN HANDLE hToken)
{
    BOOL Success;
    PSID pSid;
    ULONG Length;
    PTOKEN_USER UserBuffer;
    PTOKEN_USER TempBuffer;

    Length = 256;
    UserBuffer = LocalAlloc(LPTR, Length);
    if (UserBuffer == NULL)
        return NULL;

    Success = GetTokenInformation(hToken,
                                  TokenUser,
                                  (PVOID)UserBuffer,
                                  Length,
                                  &Length);
    if (!Success && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
    {
        TempBuffer = LocalReAlloc(UserBuffer, Length, LMEM_MOVEABLE);
        if (TempBuffer == NULL)
        {
            LocalFree(UserBuffer);
            return NULL;
        }

        UserBuffer = TempBuffer;
        Success = GetTokenInformation(hToken,
                                      TokenUser,
                                      (PVOID)UserBuffer,
                                      Length,
                                      &Length);
    }

    if (!Success)
    {
        LocalFree(UserBuffer);
        return NULL;
    }

    Length = GetLengthSid(UserBuffer->User.Sid);

    pSid = LocalAlloc(LPTR, Length);
    if (pSid == NULL)
    {
        LocalFree(UserBuffer);
        return NULL;
    }

    Success = CopySid(Length, pSid, UserBuffer->User.Sid);

    LocalFree(UserBuffer);

    if (!Success)
    {
        LocalFree(pSid);
        return NULL;
    }

    return pSid;
}

START_TEST(LoadUserProfile)
{
    BOOL Success;
    HANDLE hToken = NULL;
    PSID pUserSid = NULL;
    USHORT i;
    PROFILEINFOW ProfileInfo[2] = { {0}, {0} };

    Success = OpenThreadToken(GetCurrentThread(),
                              TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE,
                              TRUE,
                              &hToken);
    if (!Success && (GetLastError() == ERROR_NO_TOKEN))
    {
        trace("OpenThreadToken failed with error %lu, falling back to OpenProcessToken\n", GetLastError());
        Success = OpenProcessToken(GetCurrentProcess(),
                                   TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE,
                                   &hToken);
    }
    if (!Success || (hToken == NULL))
    {
        skip("Open[Thread|Process]Token failed with error %lu\n", GetLastError());
        return;
    }

    pUserSid = GetUserSid(hToken);
    ok(pUserSid != NULL, "GetUserSid failed with error %lu\n", GetLastError());
    if (pUserSid)
    {
        LPWSTR pSidStr = NULL;
        Success = ConvertSidToStringSidW(pUserSid, &pSidStr);
        ok(Success, "ConvertSidToStringSidW failed with error %lu\n", GetLastError());
        if (Success)
        {
            trace("User SID is '%ls'\n", pSidStr);
            LocalFree(pSidStr);
        }
        LocalFree(pUserSid);
        pUserSid = NULL;
    }
    else
    {
        trace("No SID available!\n");
    }

    /* Check whether ProfileInfo.lpUserName is really needed */
    ZeroMemory(&ProfileInfo[0], sizeof(ProfileInfo[0]));
    ProfileInfo[0].dwSize = sizeof(ProfileInfo[0]);
    ProfileInfo[0].dwFlags = PI_NOUI;
    ProfileInfo[0].lpUserName = NULL;
    Success = LoadUserProfileW(hToken, &ProfileInfo[0]);
    ok(!Success, "LoadUserProfile succeeded with error %lu, expected failing\n", GetLastError());
    ok(ProfileInfo[0].hProfile == NULL, "ProfileInfo[0].hProfile != NULL, expected NULL\n");
    /* Unload the user profile if we erroneously succeeded, just in case... */
    if (Success)
    {
        trace("LoadUserProfileW(ProfileInfo[0]) unexpectedly succeeded, unload the user profile just in case...\n");
        UnloadUserProfile(hToken, ProfileInfo[0].hProfile);
    }

    /* TODO: Check which privileges we do need */

    /* Enable both the SE_RESTORE_NAME and SE_BACKUP_NAME privileges */
    Success = EnablePrivilege(SE_RESTORE_NAME, TRUE);
    ok(Success, "EnablePrivilege(SE_RESTORE_NAME) failed with error %lu\n", GetLastError());
    Success = EnablePrivilege(SE_BACKUP_NAME, TRUE);
    ok(Success, "EnablePrivilege(SE_BACKUP_NAME) failed with error %lu\n", GetLastError());

    /* Check whether we can load multiple times the same user profile */
    for (i = 0; i < ARRAYSIZE(ProfileInfo); ++i)
    {
        ZeroMemory(&ProfileInfo[i], sizeof(ProfileInfo[i]));
        ProfileInfo[i].dwSize = sizeof(ProfileInfo[i]);
        ProfileInfo[i].dwFlags = PI_NOUI;
        ProfileInfo[i].lpUserName = L"toto"; // Dummy name; normally this should be the user name...
        Success = LoadUserProfileW(hToken, &ProfileInfo[i]);
        ok(Success, "LoadUserProfileW(ProfileInfo[%d]) failed with error %lu\n", i, GetLastError());
        ok(ProfileInfo[i].hProfile != NULL, "ProfileInfo[%d].hProfile == NULL\n", i);
        trace("ProfileInfo[%d].hProfile = 0x%p\n", i, ProfileInfo[i].hProfile);
    }

    i = ARRAYSIZE(ProfileInfo);
    while (i-- > 0)
    {
        trace("UnloadUserProfile(ProfileInfo[%d].hProfile)\n", i);
        Success = UnloadUserProfile(hToken, ProfileInfo[i].hProfile);
        ok(Success, "UnloadUserProfile(ProfileInfo[%d].hProfile) failed with error %lu\n", i, GetLastError());
    }

    /* Disable the privileges */
    EnablePrivilege(SE_BACKUP_NAME, FALSE);
    EnablePrivilege(SE_RESTORE_NAME, FALSE);

    /* Final cleanup */
    CloseHandle(hToken);
}
