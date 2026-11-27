/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for SHTestTokenPrivilegeW: true when the token holds the privilege, enabled or not
 * COPYRIGHT:   Copyright 2026 Leeway Open Software
 */

#include "shelltest.h"

typedef BOOL (WINAPI *FN_SHTestTokenPrivilegeW)(HANDLE, LPCWSTR);

/* Whether the token holds the privilege, read directly */
static BOOL TokenHolds(HANDLE hToken, LPCWSTR pszName)
{
    BYTE Buffer[4096];
    PTOKEN_PRIVILEGES pPrivileges = (PTOKEN_PRIVILEGES)Buffer;
    DWORD dwLength, i;
    LUID Luid;

    if (!LookupPrivilegeValueW(NULL, pszName, &Luid) ||
        !GetTokenInformation(hToken, TokenPrivileges, Buffer, sizeof(Buffer), &dwLength))
    {
        return FALSE;
    }
    for (i = 0; i < pPrivileges->PrivilegeCount; i++)
    {
        if (pPrivileges->Privileges[i].Luid.LowPart == Luid.LowPart &&
            pPrivileges->Privileges[i].Luid.HighPart == Luid.HighPart)
        {
            return TRUE;
        }
    }
    return FALSE;
}

START_TEST(SHTestTokenPrivilegeW)
{
    static const LPCWSTR Names[] =
    {
        SE_CHANGE_NOTIFY_NAME, SE_SHUTDOWN_NAME, SE_UNDOCK_NAME, SE_TCB_NAME, SE_CREATE_TOKEN_NAME
    };
    FN_SHTestTokenPrivilegeW pSHTestTokenPrivilegeW;
    HANDLE hToken;
    UINT i;

    pSHTestTokenPrivilegeW = (FN_SHTestTokenPrivilegeW)
        GetProcAddress(GetModuleHandleW(L"shell32.dll"), MAKEINTRESOURCEA(236));
    if (!pSHTestTokenPrivilegeW)
    {
        skip("SHTestTokenPrivilegeW (shell32 #236) not found\n");
        return;
    }

    ok(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken), "OpenProcessToken failed, error %lu\n",
       GetLastError());

    /* Every token holds Bypass traverse checking */
    ok(pSHTestTokenPrivilegeW(NULL, SE_CHANGE_NOTIFY_NAME), "SeChangeNotifyPrivilege not reported (effective token)\n");
    ok(pSHTestTokenPrivilegeW(hToken, SE_CHANGE_NOTIFY_NAME), "SeChangeNotifyPrivilege not reported (process token)\n");

    /* The answer is the token's own list, for held and not held privileges alike */
    for (i = 0; i < _countof(Names); i++)
    {
        BOOL bHeld = TokenHolds(hToken, Names[i]);
        ok(pSHTestTokenPrivilegeW(hToken, Names[i]) == bHeld, "%ls: expected %d\n", Names[i], bHeld);
        ok(pSHTestTokenPrivilegeW(NULL, Names[i]) == bHeld, "%ls (effective token): expected %d\n", Names[i], bHeld);
    }

    /* An interactive user's token holds SeShutdownPrivilege on a workstation (default user rights) */
    ok(TokenHolds(hToken, SE_SHUTDOWN_NAME), "the process token has no SeShutdownPrivilege\n");

    ok(!pSHTestTokenPrivilegeW(hToken, L"SeNoSuchPrivilege"), "an unknown privilege name was reported\n");
    ok(!pSHTestTokenPrivilegeW(hToken, NULL), "a NULL name was reported\n");

    CloseHandle(hToken);
}
