/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/kernel32/mem/resnotify.c
 * PURPOSE:              Memory Resource Notification
 * PROGRAMMER:           Thomas Weidenmueller <w3seek@reactos.com>
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
HANDLE
STDCALL
CreateMemoryResourceNotification(
    MEMORY_RESOURCE_NOTIFICATION_TYPE NotificationType
    )
{
    UNICODE_STRING EventName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hEvent;
    NTSTATUS Status;

    switch(NotificationType)
    {
      case LowMemoryResourceNotification:
        RtlInitUnicodeString(&EventName, L"\\KernelObjects\\LowMemoryCondition");
        break;

      case HighMemoryResourceNotification:
        RtlInitUnicodeString(&EventName, L"\\KernelObjects\\HighMemoryCondition");
        break;

      default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &EventName,
                               0,
                               hBaseDir,
                               NULL);

    Status = NtOpenEvent(&hEvent,
                         EVENT_QUERY_STATE | SYNCHRONIZE,
                         &ObjectAttributes);
    if(!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return NULL;
    }

    return hEvent;
}


/*
 * @implemented
 */
BOOL
STDCALL
QueryMemoryResourceNotification(
    HANDLE ResourceNotificationHandle,
    PBOOL  ResourceState
    )
{
    EVENT_BASIC_INFORMATION ebi;
    NTSTATUS Status;

    if(ResourceState != NULL)
    {
      Status = NtQueryEvent(ResourceNotificationHandle,
                            EventBasicInformation,
                            &ebi,
                            sizeof(ebi),
                            NULL);
      if(NT_SUCCESS(Status))
      {
        *ResourceState = ebi.EventState;
        return TRUE;
      }

      SetLastErrorByStatus(Status);
    }
    else /* ResourceState == NULL */
    {
      SetLastError(ERROR_INVALID_PARAMETER);
    }

    return FALSE;
}

/* EOF */
