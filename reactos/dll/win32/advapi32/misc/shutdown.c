/* $Id$
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

#include <advapi32.h>
WINE_DEFAULT_DEBUG_CHANNEL(advapi);

#define USZ {0,0,0}

/**********************************************************************
 *      AbortSystemShutdownW
 *
 * @unimplemented
 */
BOOL WINAPI
AbortSystemShutdownW(LPCWSTR lpMachineName)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *      AbortSystemShutdownA
 *
 * @unimplemented
 */
BOOL WINAPI
AbortSystemShutdownA(LPCSTR lpMachineName)
{
    ANSI_STRING MachineNameA;
    UNICODE_STRING MachineNameW;
    NTSTATUS Status;
    BOOL rv;

    RtlInitAnsiString(&MachineNameA, (LPSTR)lpMachineName);
    Status = RtlAnsiStringToUnicodeString(&MachineNameW, &MachineNameA, TRUE);
    if (STATUS_SUCCESS != Status)
    {
            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
    }

    rv = AbortSystemShutdownW(MachineNameW.Buffer);
    RtlFreeUnicodeString(&MachineNameW);
    SetLastError(ERROR_SUCCESS);
    return rv;
}

/**********************************************************************
 *      InitiateSystemShutdownW
 *
 * @implemented
 */
BOOL WINAPI
InitiateSystemShutdownW(LPWSTR lpMachineName,
                        LPWSTR lpMessage,
                        DWORD dwTimeout,
                        BOOL bForceAppsClosed,
                        BOOL bRebootAfterShutdown)
{
	return InitiateSystemShutdownExW(lpMachineName,
                                     lpMessage,
                                     dwTimeout,
                                     bForceAppsClosed,
                                     bRebootAfterShutdown,
                                     SHTDN_REASON_MAJOR_OTHER |
                                     SHTDN_REASON_MINOR_OTHER |
                                     SHTDN_REASON_FLAG_PLANNED);
}

/**********************************************************************
 *      InitiateSystemShutdownA
 *
 * @implemented
 */
BOOL
WINAPI
InitiateSystemShutdownA(LPSTR lpMachineName,
                        LPSTR lpMessage,
                        DWORD dwTimeout,
                        BOOL bForceAppsClosed,
                        BOOL bRebootAfterShutdown)
{
	return InitiateSystemShutdownExA(lpMachineName,
                                     lpMessage,
                                     dwTimeout,
                                     bForceAppsClosed,
                                     bRebootAfterShutdown,
                                     SHTDN_REASON_MAJOR_OTHER |
                                     SHTDN_REASON_MINOR_OTHER |
                                     SHTDN_REASON_FLAG_PLANNED);
}

/******************************************************************************
 * InitiateSystemShutdownExW [ADVAPI32.@]
 *
 * @unimplemented
 */
BOOL WINAPI
InitiateSystemShutdownExW(LPWSTR lpMachineName,
                          LPWSTR lpMessage,
                          DWORD dwTimeout,
                          BOOL bForceAppsClosed,
                          BOOL bRebootAfterShutdown,
                          DWORD dwReason)
{
    SHUTDOWN_ACTION Action = ShutdownNoReboot;
    NTSTATUS Status;

    if (lpMachineName)
    {
        /* FIXME: remote machine shutdown not supported yet */
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    if (dwTimeout)
    {
    }

    Status = NtShutdownSystem(Action);
    SetLastError(RtlNtStatusToDosError(Status));
    return FALSE;
}

/******************************************************************************
 * InitiateSystemShutdownExA [ADVAPI32.@]
 *
 * see InitiateSystemShutdownExW
 */
BOOL WINAPI
InitiateSystemShutdownExA(LPSTR lpMachineName,
                          LPSTR lpMessage,
                          DWORD dwTimeout,
                          BOOL bForceAppsClosed,
                          BOOL bRebootAfterShutdown,
                          DWORD dwReason)
{
    ANSI_STRING MachineNameA, MessageA;
    UNICODE_STRING MachineNameW, MessageW;
    NTSTATUS Status;
    INT LastError;
    BOOL rv;

    MachineNameW.Buffer = NULL;
    MessageW.Buffer = NULL;

    if (lpMachineName)
    {
        RtlInitAnsiString(&MachineNameA, lpMachineName);
        Status = RtlAnsiStringToUnicodeString(&MachineNameW, &MachineNameA, TRUE);
        if (STATUS_SUCCESS != Status)
        {
            if(MachineNameW.Buffer)
                RtlFreeUnicodeString(&MachineNameW);

            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
        }
    }

    if (lpMessage)
    {
        RtlInitAnsiString(&MessageA, lpMessage);
        Status = RtlAnsiStringToUnicodeString(&MessageW, &MessageA, TRUE);
        if (STATUS_SUCCESS != Status)
        {
            if (MessageW.Buffer)
                RtlFreeUnicodeString(&MessageW);

            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
        }
    }
	
    rv = InitiateSystemShutdownExW(MachineNameW.Buffer,
                                   MessageW.Buffer,
                                   dwTimeout,
                                   bForceAppsClosed,
                                   bRebootAfterShutdown,
                                   dwReason);
    LastError = GetLastError();
    if (lpMachineName)
        RtlFreeUnicodeString(&MachineNameW);

    if (lpMessage)
        RtlFreeUnicodeString(&MessageW);

    SetLastError(LastError);
    return rv;
}

/******************************************************************************
 * InitiateShutdownW [ADVAPI32.@]
 * 
 * @unimplamented
 */
DWORD WINAPI
InitiateShutdownW(LPWSTR lpMachineName,
                  LPWSTR lpMessage,
                  DWORD dwGracePeriod,
                  DWORD dwShutdownFlags,
                  DWORD dwReason)
{
    UNIMPLEMENTED;
    return ERROR_SUCCESS;
}

/******************************************************************************
 * InitiateShutdownA [ADVAPI32.@]
 * 
 * @unimplamented
 */
DWORD WINAPI
InitiateShutdownA(LPSTR lpMachineName,
                  LPSTR lpMessage,
                  DWORD dwGracePeriod,
                  DWORD dwShutdownFlags,
                  DWORD dwReason)
{
    ANSI_STRING MachineNameA, MessageA;
    UNICODE_STRING MachineNameW, MessageW;
    NTSTATUS Status;
    INT LastError;
    DWORD rv;

    MachineNameW.Buffer = NULL;
    MessageW.Buffer = NULL;

    if (lpMachineName)
    {
        RtlInitAnsiString(&MachineNameA, lpMachineName);
        Status = RtlAnsiStringToUnicodeString(&MachineNameW, &MachineNameA, TRUE);
        if (STATUS_SUCCESS != Status)
        {
            if(MachineNameW.Buffer)
                RtlFreeUnicodeString(&MachineNameW);

            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
        }
    }

    if (lpMessage)
    {
        RtlInitAnsiString(&MessageA, lpMessage);
        Status = RtlAnsiStringToUnicodeString(&MessageW, &MessageA, TRUE);
        if (STATUS_SUCCESS != Status)
        {
            if (MessageW.Buffer)
                RtlFreeUnicodeString(&MessageW);

            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
        }
    }

    rv = InitiateShutdownW(MachineNameW.Buffer,
                           MessageW.Buffer,
                           dwGracePeriod,
                           dwShutdownFlags,
                           dwReason);
    LastError = GetLastError();
    if (lpMachineName)
        RtlFreeUnicodeString(&MachineNameW);

    if (lpMessage)
        RtlFreeUnicodeString(&MessageW);

    SetLastError(LastError);
    return rv;
}

/* EOF */
