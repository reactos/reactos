/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS User API Server DLL
 * FILE:            win32ss/user/winsrv/init.c
 * PURPOSE:         Initialization
 * PROGRAMMERS:     Dmitry Philippov (shedon@mail.ru)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "winsrv.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

#define DEFAULT_AUTO_END_TASKS              FALSE
#define DEFAULT_HUNG_APP_TIMEOUT            5000
#define DEFAULT_WAIT_TO_KILL_APP_TIMEOUT    20000
#define DEFAULT_PROCESS_TERMINATE_TIMEOUT   90000

SHUTDOWN_SETTINGS ShutdownSettings =
{
    DEFAULT_AUTO_END_TASKS,
    DEFAULT_HUNG_APP_TIMEOUT,
    DEFAULT_WAIT_TO_KILL_APP_TIMEOUT,
    DEFAULT_WAIT_TO_KILL_APP_TIMEOUT,
    DEFAULT_PROCESS_TERMINATE_TIMEOUT
};

/* FUNCTIONS ******************************************************************/

static ULONG
GetRegIntFromID(IN HANDLE KeyHandle,
                IN PWCHAR ValueName,
                IN ULONG DefaultValue)
{
    UNICODE_STRING ValueString;
    ULONG Length;
    UCHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 32 * sizeof(WCHAR)];
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = (PVOID)Buffer;
    NTSTATUS Status;
    ULONG Value;

    /* Open the key */
    RtlInitUnicodeString(&ValueString, ValueName);
    Status = NtQueryValueKey(KeyHandle,
                             &ValueString,
                             KeyValuePartialInformation,
                             PartialInfo,
                             sizeof(Buffer),
                             &Length);
    if (NT_SUCCESS(Status))
    {
        if (PartialInfo->Type == REG_SZ)
        {
            /* Convert to integer */
            RtlInitUnicodeString(&ValueString, (PWCHAR)PartialInfo->Data);
            Status = RtlUnicodeStringToInteger(&ValueString, 10, &Value);
        }
        else if (PartialInfo->Type == REG_DWORD)
        {
            /* Directly retrieve the data */
            Value = *(PULONG)PartialInfo->Data;
            Status = STATUS_SUCCESS;
        }
        else
        {
            DPRINT1("Unexpected registry type %d for setting %S\n", PartialInfo->Type, ValueName);
            Status = STATUS_UNSUCCESSFUL;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        /* Use default value instead */
        Value = DefaultValue;
    }

    /* Return the value */
    return Value;
}

VOID FASTCALL
GetTimeouts(IN PSHUTDOWN_SETTINGS ShutdownSettings)
{
    NTSTATUS Status;
    UNICODE_STRING RegistryString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE CurrentUserKeyHandle;
    HANDLE KeyHandle;

    /* Initialize with defaults first */
    ShutdownSettings->AutoEndTasks         = DEFAULT_AUTO_END_TASKS;
    ShutdownSettings->HungAppTimeout       = DEFAULT_HUNG_APP_TIMEOUT;
    ShutdownSettings->WaitToKillAppTimeout = DEFAULT_WAIT_TO_KILL_APP_TIMEOUT;
    ShutdownSettings->WaitToKillServiceTimeout = ShutdownSettings->WaitToKillAppTimeout;
    ShutdownSettings->ProcessTerminateTimeout  = DEFAULT_PROCESS_TERMINATE_TIMEOUT;

    /* Open the per-user desktop key */
    Status = RtlOpenCurrentUser(MAXIMUM_ALLOWED, &CurrentUserKeyHandle);
    if (NT_SUCCESS(Status))
    {
        RtlInitUnicodeString(&RegistryString,
                             L"Control Panel\\Desktop");
        InitializeObjectAttributes(&ObjectAttributes,
                                   &RegistryString,
                                   OBJ_CASE_INSENSITIVE,
                                   CurrentUserKeyHandle,
                                   NULL);
        Status = NtOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);
        if (NT_SUCCESS(Status))
        {
            /* Read timeouts */
            ShutdownSettings->HungAppTimeout = GetRegIntFromID(KeyHandle,
                                                              L"HungAppTimeout",
                                                              DEFAULT_HUNG_APP_TIMEOUT);
            ShutdownSettings->WaitToKillAppTimeout = GetRegIntFromID(KeyHandle,
                                                                    L"WaitToKillAppTimeout",
                                                                    DEFAULT_WAIT_TO_KILL_APP_TIMEOUT);
            ShutdownSettings->AutoEndTasks = GetRegIntFromID(KeyHandle,
                                                            L"AutoEndTasks",
                                                            DEFAULT_AUTO_END_TASKS);
            /* Done */
            NtClose(KeyHandle);
        }

        /* Done */
        NtClose(CurrentUserKeyHandle);
    }

    /* Now open the control key */
    RtlInitUnicodeString(&RegistryString,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Control");
    InitializeObjectAttributes(&ObjectAttributes,
                               &RegistryString,
                               OBJ_CASE_INSENSITIVE,
                               NULL, NULL);
    Status = NtOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* Read the services timeout */
        ShutdownSettings->WaitToKillServiceTimeout = GetRegIntFromID(KeyHandle,
                                                                    L"WaitToKillServiceTimeout",
                                                                    ShutdownSettings->WaitToKillAppTimeout);

        /*
         * Retrieve the process terminate timeout.
         * See ftp://ftp.microsoft.com/MISC1/BUSSYS/WINNT/KB/Q234/6/06.TXT
         * and https://web.archive.org/web/20050216235758/http://support.microsoft.com/kb/234606/EN-US/
         * for more details.
         *
         * NOTE: Unused at the moment...
         */
        ShutdownSettings->ProcessTerminateTimeout = GetRegIntFromID(KeyHandle,
                                                                   L"ProcessTerminateTimeout",
                                                                   DEFAULT_PROCESS_TERMINATE_TIMEOUT);
        if (ShutdownSettings->ProcessTerminateTimeout < DEFAULT_HUNG_APP_TIMEOUT)
            ShutdownSettings->ProcessTerminateTimeout = DEFAULT_HUNG_APP_TIMEOUT;

        /* Done */
        NtClose(KeyHandle);
    }
}

/* ENTRY-POINT ****************************************************************/

BOOL
WINAPI
DllMain(IN HINSTANCE hInstanceDll,
        IN DWORD dwReason,
        IN LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(hInstanceDll);
    UNREFERENCED_PARAMETER(dwReason);
    UNREFERENCED_PARAMETER(lpReserved);

    return TRUE;
}

/* EOF */
