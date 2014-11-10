/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        dll/win32/advapi32/misc/shutdown.c
 * PURPOSE:     System shutdown functions
 * PROGRAMMER:  Lee Schroeder <spaceseel at gmail dot com>
 *              Emanuele Aliberti
 */

#include <advapi32.h>

#include <ndk/exfuncs.h>

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

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
 * see AbortSystemShutdownW
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
                                     SHTDN_REASON_FLAG_PLANNED
                                     /* SHTDN_REASON_MAJOR_LEGACY_API */);
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
                                     SHTDN_REASON_FLAG_PLANNED
                                     /* SHTDN_REASON_MAJOR_LEGACY_API */);
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
    SHUTDOWN_ACTION action;
    NTSTATUS Status;
    ULONG Timeout_ms;

    DBG_UNREFERENCED_LOCAL_VARIABLE(Timeout_ms);

    /* Convert to milliseconds so we can use the value later on */
    Timeout_ms = dwTimeout * 1000;

    if (lpMachineName != NULL)
    {
        /* FIXME: Remote system shutdown not supported yet */
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }
    else /* The local system is being used */
    {
        /* FIXME: Right now, only basic shutting down and rebooting
        is supported */
        if(bRebootAfterShutdown == TRUE)
        {
            action = ShutdownReboot;
        }
        else
        {
            action = ShutdownNoReboot;
        }

        Status = NtShutdownSystem(action);
    }

    SetLastError(RtlNtStatusToDosError(Status));
    return (Status == STATUS_SUCCESS);
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

    /* Clear the values of both strings */
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
 * see InitiateShutdownW
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

    /* Clear the values of both strings */
    if (lpMachineName)
        RtlFreeUnicodeString(&MachineNameW);

    if (lpMessage)
        RtlFreeUnicodeString(&MessageW);

    SetLastError(LastError);
    return rv;
}

/* EOF */
