/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS Win32 Base API
 * FILE:                 dll/win32/kernel32/client/resntfy.c
 * PURPOSE:              Memory Resource Notifications
 * PROGRAMMER:           Thomas Weidenmueller <w3seek@reactos.com>
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
HANDLE
WINAPI
CreateMemoryResourceNotification(IN MEMORY_RESOURCE_NOTIFICATION_TYPE NotificationType)
{
    UNICODE_STRING EventName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hEvent;
    NTSTATUS Status;

    if (NotificationType > HighMemoryResourceNotification)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    RtlInitUnicodeString(&EventName,
                         NotificationType ?
                         L"\\KernelObjects\\HighMemoryCondition" :
                         L"\\KernelObjects\\LowMemoryCondition");

    InitializeObjectAttributes(&ObjectAttributes,
                               &EventName,
                               0,
                               NULL,
                               NULL);

    Status = NtOpenEvent(&hEvent,
                         EVENT_QUERY_STATE | SYNCHRONIZE,
                         &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return NULL;
    }

    return hEvent;
}

/*
 * @implemented
 */
BOOL
WINAPI
QueryMemoryResourceNotification(IN HANDLE ResourceNotificationHandle,
                                OUT PBOOL ResourceState)
{
    EVENT_BASIC_INFORMATION EventInfo;
    NTSTATUS Status;

    if ((ResourceNotificationHandle) &&
        (ResourceNotificationHandle != INVALID_HANDLE_VALUE) &&
        (ResourceState))
    {
        Status = NtQueryEvent(ResourceNotificationHandle,
                              EventBasicInformation,
                              &EventInfo,
                              sizeof(EventInfo),
                              NULL);
        if (NT_SUCCESS(Status))
        {
            *ResourceState = (EventInfo.EventState == 1);
            return TRUE;
        }

        BaseSetLastNTError(Status);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return FALSE;
}

/* EOF */
