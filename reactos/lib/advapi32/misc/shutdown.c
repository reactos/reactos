/* $Id: shutdown.c,v 1.8 2003/02/02 17:20:51 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/advapi32/misc/shutdown.c
 * PURPOSE:     System shutdown functions
 * PROGRAMMER:      Emanuele Aliberti
 * UPDATE HISTORY:
 *      19990413 EA     created
 *      19990515 EA
 */

#include <windows.h>

#define NTOS_MODE_USER
#include <ntos.h>

#define USZ {0,0,0}

/**********************************************************************
 *      AbortSystemShutdownW
 */
WINBOOL
STDCALL
AbortSystemShutdownW(LPCWSTR lpMachineName)
{
    NTSTATUS Status;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *      AbortSystemShutdownA
 */
BOOL
STDCALL
AbortSystemShutdownA(LPCSTR lpMachineName)
{
    ANSI_STRING MachineNameA;
    UNICODE_STRING MachineNameW;
    NTSTATUS Status;
    BOOL rv;

    RtlInitAnsiString(&MachineNameA, (LPSTR)lpMachineName);
    Status = RtlAnsiStringToUnicodeString(&MachineNameW, &MachineNameA, TRUE);
    if (STATUS_SUCCESS != Status) {
            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
    }
    rv = AbortSystemShutdownW(MachineNameW.Buffer);
    RtlFreeAnsiString(&MachineNameA);
    RtlFreeUnicodeString(&MachineNameW);
    SetLastError(ERROR_SUCCESS);
    return rv;
}


/**********************************************************************
 *      InitiateSystemShutdownW
 */
BOOL
STDCALL
InitiateSystemShutdownW(
    LPWSTR  lpMachineName,
    LPWSTR  lpMessage,
    DWORD   dwTimeout,
    BOOL    bForceAppsClosed,
    BOOL    bRebootAfterShutdown)
{
    SHUTDOWN_ACTION Action = ShutdownNoReboot;
    NTSTATUS        Status;

    if (lpMachineName) {
    /* FIXME: remote machine shutdown not supported yet */
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }
    if (dwTimeout) {
    }
    Status = NtShutdownSystem(Action);
    SetLastError(RtlNtStatusToDosError(Status));
    return FALSE;
}


/**********************************************************************
 *      InitiateSystemShutdownA
*/
BOOL
STDCALL
InitiateSystemShutdownA(
    LPSTR   lpMachineName,
    LPSTR   lpMessage,
    DWORD   dwTimeout,
    BOOL    bForceAppsClosed,
    BOOL    bRebootAfterShutdown)
{
    ANSI_STRING     MachineNameA;
    ANSI_STRING     MessageA;
    UNICODE_STRING  MachineNameW;
    UNICODE_STRING  MessageW;
    NTSTATUS        Status;
    INT         LastError;
    BOOL        rv;

    if (lpMachineName) {
        RtlInitAnsiString(&MachineNameA, lpMachineName);
        Status = RtlAnsiStringToUnicodeString(&MachineNameW, &MachineNameA, TRUE);
        if (STATUS_SUCCESS != Status) {
            RtlFreeAnsiString(&MachineNameA);
            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
        }
    }
    if (lpMessage) {
        RtlInitAnsiString(&MessageA, lpMessage);
        Status = RtlAnsiStringToUnicodeString(&MessageW, &MessageA, TRUE);
        if (STATUS_SUCCESS != Status) {
            if (MachineNameW.Length) {
                RtlFreeAnsiString(&MachineNameA);
                RtlFreeUnicodeString(&MachineNameW);
            }
            RtlFreeAnsiString(&MessageA);
            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
        }
    }
    rv = InitiateSystemShutdownW(
            MachineNameW.Buffer,
            MessageW.Buffer,
            dwTimeout,
            bForceAppsClosed,
            bRebootAfterShutdown);
    LastError = GetLastError();
    if (lpMachineName) {
        RtlFreeAnsiString(&MachineNameA);
        RtlFreeUnicodeString(&MachineNameW);
    }
    if (lpMessage) {
        RtlFreeAnsiString(&MessageA);
        RtlFreeUnicodeString(&MessageW);
    }
    SetLastError(LastError);
    return rv;
}

/* EOF */
