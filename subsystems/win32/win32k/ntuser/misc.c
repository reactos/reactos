/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/misc.c
 * PURPOSE:         "Miscellaneous" Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

DWORD
APIENTRY
NtUserConsoleControl(DWORD dwUnknown1,
                     DWORD dwUnknown2,
                     DWORD dwUnknown3)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserHardErrorControl(DWORD dwUnknown1,
                       DWORD dwUnknown2,
                       DWORD dwUnknown3)
{
    UNIMPLEMENTED;
    return 0;
}

NTSTATUS
APIENTRY
NtUserInitializeClientPfnArrays(PPFNCLIENT pfnClientA, 
                                PPFNCLIENT pfnClientW,
                                PPFNCLIENTWORKER pfnClientWorker,
                                HINSTANCE hmodUser)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

DWORD
APIENTRY
NtUserInitTask(DWORD Unknown0,
               DWORD Unknown1,
               DWORD Unknown2,
               DWORD Unknown3,
               DWORD Unknown4,
               DWORD Unknown5,
               DWORD Unknown6,
               DWORD Unknown7,
               DWORD Unknown8,
               DWORD Unknown9,
               DWORD Unknown10,
               DWORD Unknown11)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserModifyUserStartupInfoFlags(DWORD Unknown0,
                                 DWORD Unknown1)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserRegisterTasklist(DWORD Unknown0)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserSBGetParms(DWORD Unknown0,
                 DWORD Unknown1,
                 DWORD Unknown2,
                 DWORD Unknown3)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserSoundSentry(VOID)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserSystemParametersInfo(UINT uiAction,
                           UINT uiParam,
                           PVOID pvParam,
                           UINT fWinIni)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtUserTestForInteractiveUser(DWORD dwUnknown1)
{
    UNIMPLEMENTED;
    return 0;
}

INT
APIENTRY
NtUserToUnicodeEx(UINT wVirtKey,
                  UINT wScanCode,
                  PBYTE lpKeyState,
                  LPWSTR pwszBuff,
                  INT cchBuff,
                  UINT wFlags,
                  HKL dwhkl )
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserUpdateInstance(DWORD Unknown0,
                     DWORD Unknown1,
                     DWORD Unknown2)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserUpdatePerUserSystemParameters(DWORD dwReserved,
                                    BOOL bEnable)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserUserHandleGrantAccess(IN HANDLE hUserHandle,
                            IN HANDLE hJob,
                            IN BOOL bGrant)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserValidateHandleSecure(HANDLE hHdl)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtUserWin32PoolAllocationStats(DWORD Unknown0,
                               DWORD Unknown1,
                               DWORD Unknown2,
                               DWORD Unknown3,
                               DWORD Unknown4,
                               DWORD Unknown5)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserYieldTask(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserDbgWin32HeapFail(DWORD Unknown1,
                       DWORD Unknown2)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserDbgWin32HeapStat(DWORD Unknown1,
                       DWORD Unknown2)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserSetRipFlags(DWORD Unknown)
{
    UNIMPLEMENTED;
    return 0;
}
