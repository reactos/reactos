/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for RegCreateKeyExW.
 * COPYRIGHT:   Copyright 2023 Doug Lyons <douglyons@douglyons.com>
 */

/*
 * Idea based loosely on code from the following:
 * https://learn.microsoft.com/en-us/windows/win32/secauthz/creating-a-security-descriptor-for-a-new-object-in-c--
 */

#include <apitest.h>
#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <windef.h>
#include <aclapi.h>


START_TEST(RegCreateKeyEx)
{
    HKEY hkey_main;
    DWORD dwRes, dwDisposition;
    PACL pACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    PSID pEveryoneSID = NULL, pAdminSID = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = {SECURITY_WORLD_SID_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = {SECURITY_NT_AUTHORITY};
    EXPLICIT_ACCESSW ea[2];
    SECURITY_ATTRIBUTES sa = { 0 };
    LONG lRes;
    BOOL bRes;
    LONG ErrorCode = 0;
    HKEY hkSub = NULL;

    // If any of the test keys already exist, delete them to ensure proper testing
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"mykey", 0, KEY_ALL_ACCESS, &hkey_main) == ERROR_SUCCESS)
    {
        RegCloseKey(hkey_main);
        ErrorCode = RegDeleteKeyW(HKEY_CURRENT_USER, L"mykey");
        ok_dec(ErrorCode, ERROR_SUCCESS);
        if (ErrorCode != ERROR_SUCCESS)
        {
            skip("'HKCU\\mykey' cannot be deleted. Terminating test\n");
            goto Cleanup;
        }
    }

    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"mykey1", 0, KEY_ALL_ACCESS, &hkey_main) == ERROR_SUCCESS)
    {
        RegCloseKey(hkey_main);
        ErrorCode = RegDeleteKeyW(HKEY_CURRENT_USER, L"mykey1");
        ok_dec(ErrorCode, ERROR_SUCCESS);
        if (ErrorCode != ERROR_SUCCESS)
        {
            skip("'HKCU\\mykey1' cannot be deleted. Terminating test\n");
            goto Cleanup;
        }
    }

    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"mykey2", 0, KEY_ALL_ACCESS, &hkey_main) == ERROR_SUCCESS)
    {
        RegCloseKey(hkey_main);
        ErrorCode = RegDeleteKeyW(HKEY_CURRENT_USER, L"mykey2");
        ok_dec(ErrorCode, ERROR_SUCCESS);
        if (ErrorCode != ERROR_SUCCESS)
        {
            skip("'HKCU\\mykey2' cannot be deleted. Terminating test\n");
            goto Cleanup;
        }
    }

    // Setup GetLastError to known value for tests
    SetLastError(0xdeadbeef);

    // Create a well-known SID for the Everyone group.
    bRes = AllocateAndInitializeSid(&SIDAuthWorld, 1,
           SECURITY_WORLD_RID,
           0, 0, 0, 0, 0, 0, 0,
           &pEveryoneSID);
    ok(bRes, "AllocateAndInitializeSid Error %ld\n", GetLastError());
    if (!bRes)
    {
        skip("EveryoneSID not initialized. Terminating test\n");
        goto Cleanup;
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow Everyone read access to the key.
    ZeroMemory(&ea, sizeof(ea));
    ea[0].grfAccessPermissions = KEY_READ;
    ea[0].grfAccessMode = SET_ACCESS;
    ea[0].grfInheritance= NO_INHERITANCE;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[0].Trustee.ptstrName  = pEveryoneSID;

    // Create a SID for the BUILTIN\Administrators group.
    bRes = AllocateAndInitializeSid(&SIDAuthNT, 2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &pAdminSID);
    ok(bRes, "AllocateAndInitializeSid Error %ld\n", GetLastError());
    if (!bRes)
    {
        skip("AdminSID not initialized. Terminating test\n");
        goto Cleanup;
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow the Administrators group full access to the key.
    ea[1].grfAccessPermissions = KEY_ALL_ACCESS;
    ea[1].grfAccessMode = SET_ACCESS;
    ea[1].grfInheritance= NO_INHERITANCE;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[1].Trustee.ptstrName  = pAdminSID;

    // Create a new ACL that contains the new ACEs.
    dwRes = SetEntriesInAclW(_countof(ea), ea, NULL, &pACL);
    ok(dwRes == ERROR_SUCCESS, "SetEntriesInAcl Error %ld\n", GetLastError());
    if (dwRes != ERROR_SUCCESS)
        goto Cleanup;

    // Initialize a security descriptor.
    pSD = LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
    ok(pSD != NULL, "LocalAlloc Error %ld\n", GetLastError());
    if (pSD == NULL)
        goto Cleanup;
 
    bRes = InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    ok(bRes, "InitializeSecurityDescriptor Error %ld\n", GetLastError());
    if (!bRes)
        goto Cleanup;

    // Add the ACL to the security descriptor.
    bRes = SetSecurityDescriptorDacl(pSD,
        TRUE,     // bDaclPresent flag
        pACL,
        FALSE);   // not a default DACL
    ok(bRes, "SetSecurityDescriptorDacl Error %ld\n", GetLastError());
    if (!bRes)
        goto Cleanup;

    // Initialize a security attributes structure.
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = FALSE;

    // Use the security attributes to set the security descriptor
    // with an nlength that is 0.
    sa.nLength = 0;
    lRes = RegCreateKeyExW(HKEY_CURRENT_USER, L"mykey", 0, L"", 0,
            KEY_READ | KEY_WRITE, &sa, &hkSub, &dwDisposition);
    ok(lRes == ERROR_SUCCESS, "RegCreateKeyExW returned '%ld', expected 0", lRes);
    ok(dwDisposition == REG_CREATED_NEW_KEY, "Should have created NEW key\n");
    if (dwDisposition != REG_CREATED_NEW_KEY)
        goto Cleanup;

    // Test the -A function
    lRes = RegCreateKeyExA(HKEY_CURRENT_USER, "mykey", 0, "", 0,
            KEY_READ | KEY_WRITE, &sa, &hkSub, &dwDisposition);
    ok(lRes == ERROR_SUCCESS, "RegCreateKeyExA returned '%ld', expected 0", lRes);
    ok(dwDisposition == REG_OPENED_EXISTING_KEY, "Should have opened EXISTING key\n");
    if (dwDisposition != REG_OPENED_EXISTING_KEY)
        goto Cleanup;

    // Use the security attributes to set the security descriptor
    // with an nlength that is too short, but not 0.
    sa.nLength = sizeof(SECURITY_ATTRIBUTES) / 2;
    lRes = RegCreateKeyExW(HKEY_CURRENT_USER, L"mykey1", 0, L"", 0,
            KEY_READ | KEY_WRITE, &sa, &hkSub, &dwDisposition);
    ok(lRes == ERROR_SUCCESS, "RegCreateKeyExW returned '%ld', expected 0", lRes);
    ok(dwDisposition == REG_CREATED_NEW_KEY, "Should have created NEW key\n");
    if (dwDisposition != REG_CREATED_NEW_KEY)
        goto Cleanup;

    // Test the -A function
    lRes = RegCreateKeyExA(HKEY_CURRENT_USER, "mykey1", 0, "", 0,
            KEY_READ | KEY_WRITE, &sa, &hkSub, &dwDisposition);
    ok(lRes == ERROR_SUCCESS, "RegCreateKeyExA returned '%ld', expected 0", lRes);
    ok(dwDisposition == REG_OPENED_EXISTING_KEY, "Should have opened EXISTING key\n");
    if (dwDisposition != REG_OPENED_EXISTING_KEY)
        goto Cleanup;

    // Use the security attributes to set the security descriptor
    // with an nlength that is too long.
    sa.nLength = sizeof(SECURITY_ATTRIBUTES) + 10;
    lRes = RegCreateKeyExW(HKEY_CURRENT_USER, L"mykey2", 0, L"", 0,
            KEY_READ | KEY_WRITE, &sa, &hkSub, &dwDisposition);
    ok(lRes == ERROR_SUCCESS, "RegCreateKeyExW returned '%ld', expected 0", lRes);
    ok(dwDisposition == REG_CREATED_NEW_KEY, "Should have created NEW key\n");
    if (dwDisposition != REG_CREATED_NEW_KEY)
        goto Cleanup;

    // Test the -A function
    lRes = RegCreateKeyExA(HKEY_CURRENT_USER, "mykey2", 0, "", 0,
            KEY_READ | KEY_WRITE, &sa, &hkSub, &dwDisposition);
    ok(lRes == ERROR_SUCCESS, "RegCreateKeyExA returned '%ld', expected 0", lRes);
    ok(dwDisposition == REG_OPENED_EXISTING_KEY, "Should have opened EXISTING key\n");
    if (dwDisposition != REG_OPENED_EXISTING_KEY)
        goto Cleanup;

Cleanup:

    if (pEveryoneSID)
        FreeSid(pEveryoneSID);
    if (pAdminSID)
        FreeSid(pAdminSID);
    if (pACL)
        LocalFree(pACL);
    if (pSD)
        LocalFree(pSD);
    if (hkSub) 
        RegCloseKey(hkSub);

    // Delete the subkeys created for testing
    ErrorCode = RegDeleteKeyW(HKEY_CURRENT_USER, L"mykey");
    ok_dec(ErrorCode, ERROR_SUCCESS);

    ErrorCode = RegDeleteKeyW(HKEY_CURRENT_USER, L"mykey1");
    ok_dec(ErrorCode, ERROR_SUCCESS);

    ErrorCode = RegDeleteKeyW(HKEY_CURRENT_USER, L"mykey2");
    ok_dec(ErrorCode, ERROR_SUCCESS);
}

