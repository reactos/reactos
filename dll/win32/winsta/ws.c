/*
 * PROJECT:         ReactOS winsta.dll
 * FILE:            lib/winsta/ws.c
 * PURPOSE:         WinStation
 * PROGRAMMER:      Samuel Serapión
 *
 */

#include "winsta.h"
#include "reactos/ts/winsta.h"

#include <ntifs.h>
#include <ntstatus.h>
#include <peb_teb.h>

BOOLEAN
WINSTAAPI WinStationFreeMemory(PVOID Buffer)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    FIXME("WinStationFreeMemory %p not freed!\n", Buffer);
    return FALSE;
}


VOID
WINSTAAPI WinStationAutoReconnect(PVOID A)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationBroadcastSystemMessage(PVOID A,
                                           PVOID B,
                                           PVOID C,
                                           PVOID D,
                                           PVOID E,
                                           PVOID F,
                                           PVOID G,
                                           PVOID H,
                                           PVOID I,
                                           PVOID J)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationCheckAccess(PVOID A,
                                PVOID B,
                                PVOID C)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationCheckLoopBack(PVOID A,
                                  PVOID B,
                                  PVOID C,
                                  PVOID D)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationConnectA(PVOID A)
{
    UNIMPLEMENTED;
}

VOID
CALLBACK WinStationConnectCallback(PVOID A,
                                   PVOID B,
                                   PVOID C,
                                   PVOID D,
                                   PVOID E)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationConnectEx(PVOID A,
                              PVOID B)
{
    UNIMPLEMENTED;
}

BOOLEAN
WINSTAAPI WinStationConnectW(_In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ ULONG TargetSessionId,
    _In_opt_ PCWSTR Password,
    _In_ BOOLEAN bWait)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    return FALSE;
}

BOOLEAN
WINSTAAPI WinStationFreeGAPMemory(ULONG Level,
                                    PTS_ALL_PROCESSES_INFO Processes,
                                    ULONG NumberOfProcesses)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    return FALSE;
}

VOID
WINSTAAPI WinStationIsHelpAssistantSession(PVOID A,
                                           PVOID B)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationNtsdDebug(PVOID A,
                              PVOID B,
                              PVOID C,
                              PVOID D,
                              PVOID E)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationRenameA(PVOID A,
                            PVOID B,
                            PVOID C)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationRenameW(PVOID A,
                            PVOID B,
                            PVOID C)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationSendMessageA(PVOID A,
                                 PVOID B,
                                 PVOID C,
                                 PVOID D,
                                 PVOID E,
                                 PVOID F,
                                 PVOID G,
                                 PVOID H,
                                 PVOID I,
                                 PVOID J)
{
    UNIMPLEMENTED;
}

BOOLEAN
WINSTAAPI WinStationSendMessageW(HANDLE ServerHandle,
    ULONG SessionId,
    PCWSTR Title,
    ULONG TitleLength,
    PCWSTR Message,
    ULONG MessageLength,
    ULONG Style,
    ULONG Timeout,
    PULONG Response,
    BOOLEAN DoNotWait)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    return FALSE;
}

VOID
WINSTAAPI WinStationSendWindowMessage(PVOID A,
                                      PVOID B,
                                      PVOID C,
                                      PVOID D,
                                      PVOID E,
                                      PVOID F,
                                      PVOID G,
                                      PVOID H)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationSetInformationA(PVOID A,
                                    PVOID B,
                                    PVOID C,
                                    PVOID D,
                                    PVOID E)
{
    UNIMPLEMENTED;
}

BOOLEAN
WINSTAAPI WinStationSetInformationW(_In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ WINSTATIONINFOCLASS WinStationInformationClass,
    _In_reads_bytes_(WinStationInformationLength) PVOID WinStationInformation,
    _In_ ULONG WinStationInformationLength)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    return FALSE;
}

VOID
WINSTAAPI WinStationSetPoolCount(PVOID A,
                                 PVOID B,
                                 PVOID C)
{
    UNIMPLEMENTED;
}

BOOLEAN
WINSTAAPI WinStationShadow(_In_opt_ HANDLE ServerHandle,
    _In_ PCWSTR TargetServerName,
    _In_ ULONG TargetSessionId,
    _In_ UCHAR HotKeyVk,
    _In_ USHORT HotkeyModifiers) // KBD*
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    return FALSE;
}

BOOLEAN
WINSTAAPI WinStationShadowStop(_In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ BOOLEAN bWait) // ignored
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    return FALSE;
}

BOOLEAN
WINSTAAPI WinStationShutdownSystem(HANDLE ServerHandle,
    ULONG ShutdownFlags) // WSD_*
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    return FALSE;
}

VOID
WINSTAAPI WinStationSystemShutdownStarted()
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationSystemShutdownWait(PVOID A,
                                       PVOID B)
{
    UNIMPLEMENTED;
}

BOOLEAN
WINSTAAPI WinStationTerminateProcess(HANDLE ServerHandle,
    ULONG ProcessId,
    ULONG ExitCode)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    return FALSE;
}

BOOLEAN
WINSTAAPI WinStationUnRegisterConsoleNotification(HANDLE hServer,
                                                  HWND WindowHandle)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    return FALSE;
}

HANDLE
WINSTAAPI WinStationVirtualOpen(HANDLE ServerHandle,
    ULONG SessionId,
    PCSTR Name)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    return NULL;
}

HANDLE
WINSTAAPI WinStationVirtualOpenEx(_In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ PCSTR Name,
    _In_ ULONG Flags)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    return NULL;
}

BOOLEAN
WINSTAAPI WinStationWaitSystemEvent(HANDLE hServer,
                                    ULONG Mask,
                                    PULONG Flags)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    FIXME("Stub %p 0x%08x %p\n", hServer, Mask, Flags);
    return FALSE;
}

VOID
WINSTAAPI WinStationDynVirtualChanRead(PVOID A,
                                       PVOID B,
                                       PVOID C,
                                       PVOID D,
                                       PVOID E)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationDynVirtualChanWrite(PVOID A,
                                        PVOID B,
                                        PVOID C,
                                        PVOID D)
{
    UNIMPLEMENTED;
}

BOOLEAN
WINSTAAPI WinStationRegisterConsoleNotificationEx(
    _In_opt_ HANDLE hServer,
    _In_ HWND hWnd,
    _In_ ULONG dwFlags,
    _In_ ULONG dwMask)
{
    if ( hServer == (HANDLE)-3 )
    {
        SetLastError(0x47Fu);
        return FALSE;
    }

    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    BOOLEAN v5 = RpcWinStationRegisterConsoleNotification(
        hServer,
        &Status,
        NtCurrentTeb()->ProcessEnvironmentBlock->SessionId,
        hWnd,
        dwFlags,
        dwMask);

    if ( !v5 )
    {
        SetLastError(RtlNtStatusToDosError(Status));
    }
    return v5;
}

BOOLEAN
WINSTAAPI WinStationRegisterConsoleNotification(HANDLE hServer,
                                                HWND WindowHandle,
                                                ULONG Flags)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    UNIMPLEMENTED;
    return FALSE;
}

VOID
WINSTAAPI WinStationRegisterNotificationEvent(PVOID A,
                                              PVOID B,
                                              PVOID C,
                                              PVOID D)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationUnRegisterNotificationEvent(PVOID A)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI WinStationSwitchToServicesSession()
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI _WinStationAnnoyancePopup(PVOID A,
                                    PVOID B)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI _WinStationBeepOpen(PVOID A,
                              PVOID B,
                              PVOID C)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI _WinStationBreakPoint(PVOID A,
                                PVOID B,
                                PVOID C)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI _WinStationSessionInitialized()
{
    UNIMPLEMENTED;
}

VOID
CALLBACK _WinStationCallback(PVOID A,
                             PVOID B,
                             PVOID C)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI _WinStationCheckForApplicationName(PVOID A,
                                             PVOID B,
                                             PVOID C,
                                             PVOID D,
                                             PVOID E,
                                             PVOID F,
                                             PVOID G,
                                             PVOID H,
                                             PVOID I,
                                             PVOID J,
                                             PVOID K,
                                             PVOID M)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI _WinStationFUSCanRemoteUserDisconnect(PVOID A,
                                                PVOID B,
                                                PVOID C)
{
    UNIMPLEMENTED;
}


VOID
WINSTAAPI _WinStationNotifyDisconnectPipe()
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI _WinStationNotifyNewSession(PVOID A,
                                      PVOID B)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI _WinStationReInitializeSecurity(PVOID A)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI _WinStationReadRegistry(PVOID A)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI _WinStationShadowTargetSetup(PVOID A,
                                       PVOID B)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI _WinStationUpdateSettings(PVOID A,
                                    PVOID B,
                                    PVOID C)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI _WinStationUpdateUserConfig(PVOID A)
{
    UNIMPLEMENTED;
}

VOID
WINSTAAPI _WinStationUpdateClientCachedCredentials(PVOID A,
                                                   PVOID B,
                                                   PVOID C,
                                                   PVOID D,
                                                   PVOID E,
                                                   PVOID F,
                                                   PVOID G)
{
    UNIMPLEMENTED;
}

/* EOF */
