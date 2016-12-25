/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtUserProcessConnect
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtUserProcessConnect)
{
    HANDLE hProcess;
    NTSTATUS Status;
    USERCONNECT UserConnect = {0};

    hProcess = GetCurrentProcess();

    UserConnect.ulVersion = MAKELONG(0, 5);
    Status = NtUserProcessConnect(hProcess, (USERCONNECT*)&UserConnect, sizeof(USERCONNECT));
    TEST(NT_SUCCESS(Status));

    printf("UserConnect.ulVersion = 0x%lx\n", UserConnect.ulVersion);
    printf("UserConnect.ulCurrentVersion = 0x%lx\n", UserConnect.ulCurrentVersion);
    printf("UserConnect.dwDispatchCount = 0x%lx\n", UserConnect.dwDispatchCount);
    printf("UserConnect.siClient.psi = 0x%p\n", UserConnect.siClient.psi);
    printf("UserConnect.siClient.aheList = 0x%p\n", UserConnect.siClient.aheList);
    printf("UserConnect.siClient.pDispInfo = 0x%p\n", UserConnect.siClient.pDispInfo);
    printf("UserConnect.siClient.ulSharedDelta = 0x%lx\n", UserConnect.siClient.ulSharedDelta);

}
