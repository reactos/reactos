/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for NtUserProcessConnect
 * COPYRIGHT:   Copyright 2008-2020 Timo Kreuzer
 *              Copyright 2021 Hermes Belusca-Maito
 */

#include <win32nt.h>

#define NTOS_MODE_USER
#include <ndk/exfuncs.h>

START_TEST(NtUserProcessConnect)
{
    HANDLE hProcess;
    NTSTATUS Status;
    USERCONNECT UserConnect = {0};
    SYSTEM_BASIC_INFORMATION SystemInformation;
    ULONG_PTR MaximumUserModeAddress;

    hProcess = GetCurrentProcess();

    UserConnect.ulVersion = MAKELONG(0, 5); // == USER_VERSION
    // UserConnect.dwDispatchCount;
    Status = NtUserProcessConnect(hProcess, &UserConnect, sizeof(UserConnect));
    TEST(NT_SUCCESS(Status));

    printf("UserConnect.ulVersion = 0x%lx\n", UserConnect.ulVersion);
    printf("UserConnect.ulCurrentVersion = 0x%lx\n", UserConnect.ulCurrentVersion);
    printf("UserConnect.dwDispatchCount = 0x%lx\n", UserConnect.dwDispatchCount);
    printf("UserConnect.siClient.psi = 0x%p\n", UserConnect.siClient.psi);
    printf("UserConnect.siClient.aheList = 0x%p\n", UserConnect.siClient.aheList);
    printf("UserConnect.siClient.pDispInfo = 0x%p\n", UserConnect.siClient.pDispInfo);
    printf("UserConnect.siClient.ulSharedDelta = 0x%Ix\n", UserConnect.siClient.ulSharedDelta);

    /* Verify the validity of some mandatory fields */
    TEST(UserConnect.ulVersion == MAKELONG(0, 5));
    TEST(UserConnect.ulCurrentVersion == 0);
    TEST(UserConnect.siClient.ulSharedDelta != 0);

    /* Get the max um address */
    Status = NtQuerySystemInformation(SystemBasicInformation,
                                      &SystemInformation,
                                      sizeof(SystemInformation),
                                      NULL);
    TEST(NT_SUCCESS(Status));

    MaximumUserModeAddress = SystemInformation.MaximumUserModeAddress;

    /* Verify the validity of pointers -- They must be in client space */
    TEST(UserConnect.siClient.psi != NULL);
    TEST(UserConnect.siClient.aheList != NULL);
    // TEST(UserConnect.siClient.pDispInfo != NULL);
    TEST((ULONG_PTR)UserConnect.siClient.psi < MaximumUserModeAddress);
    TEST((ULONG_PTR)UserConnect.siClient.aheList < MaximumUserModeAddress);
    // TEST((ULONG_PTR)UserConnect.siClient.pDispInfo < MaximumUserModeAddress);
}
