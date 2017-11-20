/*
 * PROJECT:         Filesystem Filter Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/filters/fltmgr/Misc.c
 * PURPOSE:         Uncataloged functions
 * PROGRAMMERS:     Ged Murphy (gedmurphy@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "fltmgr.h"
#include "fltmgrint.h"

#define NDEBUG
#include <debug.h>


/* DATA *********************************************************************/

#define REG_SERVICES_KEY    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\"
#define REG_PATH_LENGTH     512


/* INTERNAL FUNCTIONS ******************************************************/


NTSTATUS
FltpOpenFilterServicesKey(
    _In_ PFLT_FILTER Filter,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PUNICODE_STRING SubKey,
    _Out_ PHANDLE Handle)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ServicesKey;
    UNICODE_STRING Path;
    WCHAR Buffer[REG_PATH_LENGTH];

    /* Setup a local buffer to hold the services key path */
    Path.Length = 0;
    Path.MaximumLength = REG_PATH_LENGTH;
    Path.Buffer = Buffer;

    /* Build up the serices key name */
    RtlInitUnicodeString(&ServicesKey, REG_SERVICES_KEY);
    RtlCopyUnicodeString(&Path, &ServicesKey);
    RtlAppendUnicodeStringToString(&Path, &Filter->Name);

    if (SubKey)
    {
        /* Tag on any child key */
        RtlAppendUnicodeToString(&Path, L"\\");
        RtlAppendUnicodeStringToString(&Path, SubKey);
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &Path,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open and return the key handle param*/
    return ZwOpenKey(Handle, DesiredAccess, &ObjectAttributes);
}

NTSTATUS
FltpReadRegistryValue(_In_ HANDLE KeyHandle,
                      _In_ PUNICODE_STRING ValueName,
                      _In_opt_ ULONG Type,
                      _Out_writes_bytes_(BufferSize) PVOID Buffer,
                      _In_ ULONG BufferSize,
                      _Out_opt_ PULONG BytesRequired)
{
    PKEY_VALUE_PARTIAL_INFORMATION Value = NULL;
    ULONG ValueLength = 0;
    NTSTATUS Status;

    PAGED_CODE();

    /* Get the size of the buffer required to hold the string */
    Status = ZwQueryValueKey(KeyHandle,
                             ValueName,
                             KeyValuePartialInformation,
                             NULL,
                             0,
                             &ValueLength);
    if (Status != STATUS_BUFFER_TOO_SMALL && Status != STATUS_BUFFER_OVERFLOW)
    {
        return Status;
    }

    /* Allocate the buffer */
    Value = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePoolWithTag(PagedPool,
                                                                  ValueLength,
                                                                  FM_TAG_TEMP_REGISTRY);
    if (Value == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quit;
    }

    /* Now read in the value */
    Status = ZwQueryValueKey(KeyHandle,
                             ValueName,
                             KeyValuePartialInformation,
                             Value,
                             ValueLength,
                             &ValueLength);
    if (!NT_SUCCESS(Status))
    {
        goto Quit;
    }

    /* Make sure we got the type expected */
    if (Value->Type != Type)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Quit;
    }

    if (BytesRequired)
    {
        *BytesRequired = Value->DataLength;
    }

    /* Make sure the caller buffer is big enough to hold the data */
    if (!BufferSize || BufferSize < Value->DataLength)
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto Quit;
    }

    /* Copy the data into the caller buffer */
    RtlCopyMemory(Buffer, Value->Data, Value->DataLength);

Quit:

    if (Value)
        ExFreePoolWithTag(Value, FM_TAG_TEMP_REGISTRY);

    return Status;
}
