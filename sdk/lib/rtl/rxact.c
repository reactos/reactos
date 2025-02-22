/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * FILE:              lib/rtl/rxact.c
 * PURPOSE:           Registry Transaction API
 * PROGRAMMERS:       Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>
#include <ndk/cmfuncs.h>

#define NDEBUG
#include <debug.h>

#define RXACT_DEFAULT_BUFFER_SIZE (4 * PAGE_SIZE)

typedef struct _RXACT_INFO
{
    ULONG Revision;
    ULONG Unknown1;
    ULONG Unknown2;
} RXACT_INFO, *PRXACT_INFO;

typedef struct _RXACT_DATA
{
    ULONG ActionCount;
    ULONG BufferSize;
    ULONG CurrentSize;
} RXACT_DATA, *PRXACT_DATA;

typedef struct _RXACT_CONTEXT
{
    HANDLE RootDirectory;
    HANDLE KeyHandle;
    BOOLEAN CanUseHandles;
    PRXACT_DATA Data;
} RXACT_CONTEXT, *PRXACT_CONTEXT;

typedef struct _RXACT_ACTION
{
    ULONG Size;
    ULONG Type;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;
    ULONG ValueType;
    ULONG ValueDataSize;
    PVOID ValueData;
} RXACT_ACTION, *PRXACT_ACTION;

enum
{
    RXactDeleteKey = 1,
    RXactSetValueKey = 2,
};

/* FUNCTIONS *****************************************************************/

static
VOID
NTAPI
RXactInitializeContext(
    PRXACT_CONTEXT Context,
    HANDLE RootDirectory,
    HANDLE KeyHandle)
{
    Context->Data = NULL;
    Context->RootDirectory = RootDirectory;
    Context->CanUseHandles = TRUE;
    Context->KeyHandle = KeyHandle;
}

static
NTSTATUS
NTAPI
RXactpOpenTargetKey(
    HANDLE RootDirectory,
    ULONG ActionType,
    PUNICODE_STRING KeyName,
    PHANDLE KeyHandle)
{
    NTSTATUS Status;
    ULONG Disposition;
    OBJECT_ATTRIBUTES ObjectAttributes;

    /* Check what kind of action this is */
    if (ActionType == RXactDeleteKey)
    {
        /* This is a delete, so open the key for delete */
        InitializeObjectAttributes(&ObjectAttributes,
                                   KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   RootDirectory,
                                   NULL);
        Status = ZwOpenKey(KeyHandle, DELETE, &ObjectAttributes);
    }
    else if (ActionType == RXactSetValueKey)
    {
        /* This is a create, so open or create with write access */
        InitializeObjectAttributes(&ObjectAttributes,
                                   KeyName,
                                   OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                                   RootDirectory,
                                   NULL);
        Status = ZwCreateKey(KeyHandle,
                             KEY_WRITE,
                             &ObjectAttributes,
                             0,
                             NULL,
                             0,
                             &Disposition);
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    return Status;
}

static
NTSTATUS
NTAPI
RXactpCommit(
    PRXACT_CONTEXT Context)
{
    PRXACT_DATA Data;
    PRXACT_ACTION Action;
    NTSTATUS Status, TmpStatus;
    HANDLE KeyHandle;
    ULONG i;

    Data = Context->Data;

    /* The first action record starts after the data header */
    Action = (PRXACT_ACTION)(Data + 1);

    /* Loop all recorded actions */
    for (i = 0; i < Data->ActionCount; i++)
    {
        /* Translate relative offsets to actual pointers */
        Action->KeyName.Buffer = (PWSTR)((PUCHAR)Data + (ULONG_PTR)Action->KeyName.Buffer);
        Action->ValueName.Buffer = (PWSTR)((PUCHAR)Data + (ULONG_PTR)Action->ValueName.Buffer);
        Action->ValueData = (PUCHAR)Data + (ULONG_PTR)Action->ValueData;

        /* Check what kind of action this is */
        if (Action->Type == RXactDeleteKey)
        {
            /* This is a delete action. Check if we can use a handle */
            if ((Action->KeyHandle != INVALID_HANDLE_VALUE) && Context->CanUseHandles)
            {
                /* Delete the key by the given handle */
                Status = ZwDeleteKey(Action->KeyHandle);
                if (!NT_SUCCESS(Status))
                {
                    return Status;
                }
            }
            else
            {
                /* We cannot use a handle, open the key first by it's name */
                Status = RXactpOpenTargetKey(Context->RootDirectory,
                                             RXactDeleteKey,
                                             &Action->KeyName,
                                             &KeyHandle);
                if (NT_SUCCESS(Status))
                {
                    Status = ZwDeleteKey(KeyHandle);
                    TmpStatus = NtClose(KeyHandle);
                    ASSERT(NT_SUCCESS(TmpStatus));
                    if (!NT_SUCCESS(Status))
                    {
                        return Status;
                    }
                }
                else
                {
                    /* Failed to open the key, it's ok, if it was not found */
                    if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
                        return Status;
                }
            }
        }
        else if (Action->Type == RXactSetValueKey)
        {
            /* This is a set action. Check if we can use a handle */
            if ((Action->KeyHandle != INVALID_HANDLE_VALUE) && Context->CanUseHandles)
            {
                /* Set the key value using the given key handle */
                Status = ZwSetValueKey(Action->KeyHandle,
                                       &Action->ValueName,
                                       0,
                                       Action->ValueType,
                                       Action->ValueData,
                                       Action->ValueDataSize);
                if (!NT_SUCCESS(Status))
                {
                    return Status;
                }
            }
            else
            {
                /* We cannot use a handle, open the key first by it's name */
                Status = RXactpOpenTargetKey(Context->RootDirectory,
                                             RXactSetValueKey,
                                             &Action->KeyName,
                                             &KeyHandle);
                if (!NT_SUCCESS(Status))
                {
                    return Status;
                }

                /* Set the key value */
                Status = ZwSetValueKey(KeyHandle,
                                       &Action->ValueName,
                                       0,
                                       Action->ValueType,
                                       Action->ValueData,
                                       Action->ValueDataSize);

                TmpStatus = NtClose(KeyHandle);
                ASSERT(NT_SUCCESS(TmpStatus));

                if (!NT_SUCCESS(Status))
                {
                    return Status;
                }
            }
        }
        else
        {
            ASSERT(FALSE);
            return STATUS_INVALID_PARAMETER;
        }

        /* Go to the next action record */
        Action = (PRXACT_ACTION)((PUCHAR)Action + Action->Size);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlStartRXact(
    PRXACT_CONTEXT Context)
{
    PRXACT_DATA Buffer;

    /* We must not have a buffer yet */
    if (Context->Data != NULL)
    {
        return STATUS_RXACT_INVALID_STATE;
    }

    /* Allocate a buffer */
    Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, RXACT_DEFAULT_BUFFER_SIZE);
    if (Buffer == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    /* Initialize the buffer */
    Buffer->ActionCount = 0;
    Buffer->BufferSize = RXACT_DEFAULT_BUFFER_SIZE;
    Buffer->CurrentSize = sizeof(RXACT_DATA);
    Context->Data = Buffer;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlAbortRXact(
    PRXACT_CONTEXT Context)
{
    /* We must have a data buffer */
    if (Context->Data == NULL)
    {
        return STATUS_RXACT_INVALID_STATE;
    }

    /* Free the buffer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, Context->Data);

    /* Reinitialize the context */
    RXactInitializeContext(Context, Context->RootDirectory, Context->KeyHandle);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlInitializeRXact(
    HANDLE RootDirectory,
    BOOLEAN Commit,
    PRXACT_CONTEXT *OutContext)
{
    NTSTATUS Status, TmpStatus;
    PRXACT_CONTEXT Context;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    KEY_VALUE_BASIC_INFORMATION KeyValueBasicInfo;
    UNICODE_STRING ValueName;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    RXACT_INFO TransactionInfo;
    ULONG Disposition;
    ULONG ValueType;
    ULONG ValueDataLength;
    ULONG Length;
    HANDLE KeyHandle;

    /* Open or create the 'RXACT' key in the root directory */
    RtlInitUnicodeString(&KeyName, L"RXACT");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                               RootDirectory,
                               NULL);
    Status = ZwCreateKey(&KeyHandle,
                         KEY_READ | KEY_WRITE | DELETE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         0,
                         &Disposition);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Allocate a new context */
    Context = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(*Context));
    *OutContext = Context;
    if (Context == NULL)
    {
        TmpStatus = ZwDeleteKey(KeyHandle);
        ASSERT(NT_SUCCESS(TmpStatus));

        TmpStatus = NtClose(KeyHandle);
        ASSERT(NT_SUCCESS(TmpStatus));

        return STATUS_NO_MEMORY;
    }

    /* Initialize the context */
    RXactInitializeContext(Context, RootDirectory, KeyHandle);

    /* Check if we created a new key */
    if (Disposition == REG_CREATED_NEW_KEY)
    {
        /* The key is new, set the default value */
        TransactionInfo.Revision = 1;
        RtlInitUnicodeString(&ValueName, NULL);
        Status = ZwSetValueKey(KeyHandle,
                               &ValueName,
                               0,
                               REG_NONE,
                               &TransactionInfo,
                               sizeof(TransactionInfo));
        if (!NT_SUCCESS(Status))
        {
            TmpStatus = ZwDeleteKey(KeyHandle);
            ASSERT(NT_SUCCESS(TmpStatus));

            TmpStatus = NtClose(KeyHandle);
            ASSERT(NT_SUCCESS(TmpStatus));

            RtlFreeHeap(RtlGetProcessHeap(), 0, *OutContext);
            return Status;
        }

        return STATUS_RXACT_STATE_CREATED;
    }
    else
    {
        /* The key exited, get the default key value */
        ValueDataLength = sizeof(TransactionInfo);
        Status = RtlpNtQueryValueKey(KeyHandle,
                                     &ValueType,
                                     &TransactionInfo,
                                     &ValueDataLength,
                                     0);
        if (!NT_SUCCESS(Status))
        {
            TmpStatus = NtClose(KeyHandle);
            ASSERT(NT_SUCCESS(TmpStatus));
            RtlFreeHeap(RtlGetProcessHeap(), 0, Context);
            return Status;
        }

        /* Check if the value date is valid */
        if ((ValueDataLength != sizeof(TransactionInfo)) ||
            (TransactionInfo.Revision != 1))
        {
            TmpStatus = NtClose(KeyHandle);
            ASSERT(NT_SUCCESS(TmpStatus));
            RtlFreeHeap(RtlGetProcessHeap(), 0, Context);
            return STATUS_UNKNOWN_REVISION;
        }

        /* Query the 'Log' key value */
        RtlInitUnicodeString(&ValueName, L"Log");
        Status = ZwQueryValueKey(KeyHandle,
                                 &ValueName,
                                 KeyValueBasicInformation,
                                 &KeyValueBasicInfo,
                                 sizeof(KeyValueBasicInfo),
                                 &Length);
        if (!NT_SUCCESS(Status))
        {
            /* There is no 'Log', so we are done */
            return STATUS_SUCCESS;
        }

        /* Check if the caller asked to commit the current state */
        if (!Commit)
        {
            /* We have a log, that must be committed first! */
            return STATUS_RXACT_COMMIT_NECESSARY;
        }

        /* Query the size of the 'Log' key value */
        Status = ZwQueryValueKey(KeyHandle,
                                 &ValueName,
                                 KeyValueFullInformation,
                                 NULL,
                                 0,
                                 &Length);
        if (Status != STATUS_BUFFER_TOO_SMALL)
        {
            return Status;
        }

        /* Allocate a buffer for the key value information */
        KeyValueInformation = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length);
        if (KeyValueInformation == NULL)
        {
            return STATUS_NO_MEMORY;
        }

        /* Query the 'Log' key value */
        Status = ZwQueryValueKey(KeyHandle,
                                 &ValueName,
                                 KeyValueFullInformation,
                                 KeyValueInformation,
                                 Length,
                                 &Length);
        if (!NT_SUCCESS(Status))
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, KeyValueInformation);
            RtlFreeHeap(RtlGetProcessHeap(), 0, Context);
            return Status;
        }

        /* Set the Data pointer to the key value data */
        Context->Data = (PRXACT_DATA)((PUCHAR)KeyValueInformation +
                                      KeyValueInformation->DataOffset);

        /* This is an old log, don't use handles when committing! */
        Context->CanUseHandles = FALSE;

        /* Commit the data */
        Status = RXactpCommit(Context);
        if (!NT_SUCCESS(Status))
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, KeyValueInformation);
            RtlFreeHeap(RtlGetProcessHeap(), 0, Context);
            return Status;
        }

        /* Delete the old key */
        Status = NtDeleteValueKey(KeyHandle, &ValueName);
        ASSERT(NT_SUCCESS(Status));

        /* Set the data member to the allocated buffer, so it will get freed */
        Context->Data = (PRXACT_DATA)KeyValueInformation;

        /* Abort the old transaction */
        Status = RtlAbortRXact(Context);
        ASSERT(NT_SUCCESS(Status));

        return Status;
    }
}

NTSTATUS
NTAPI
RtlAddAttributeActionToRXact(
    PRXACT_CONTEXT Context,
    ULONG ActionType,
    PUNICODE_STRING KeyName,
    HANDLE KeyHandle,
    PUNICODE_STRING ValueName,
    ULONG ValueType,
    PVOID ValueData,
    ULONG ValueDataSize)
{
    ULONG ActionSize;
    ULONG RequiredSize;
    ULONG BufferSize;
    ULONG CurrentOffset;
    PRXACT_DATA NewData;
    PRXACT_ACTION Action;

    /* Validate ActionType parameter */
    if ((ActionType != RXactDeleteKey) && (ActionType != RXactSetValueKey))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Calculate the size of the new action record */
    ActionSize = ALIGN_UP_BY(ValueName->Length, sizeof(ULONG)) +
                 ALIGN_UP_BY(ValueDataSize, sizeof(ULONG)) +
                 ALIGN_UP_BY(KeyName->Length, sizeof(ULONG)) +
                 ALIGN_UP_BY(sizeof(RXACT_ACTION), sizeof(ULONG));

    /* Calculate the new buffer size we need */
    RequiredSize = ActionSize + Context->Data->CurrentSize;

    /* Check for integer overflow */
    if (RequiredSize < ActionSize)
    {
        return STATUS_NO_MEMORY;
    }

    /* Check if the buffer is large enough */
    BufferSize = Context->Data->BufferSize;
    if (RequiredSize > BufferSize)
    {
        /* Increase by a factor of 2, until it is large enough */
        while (BufferSize < RequiredSize)
        {
            BufferSize *= 2;
        }

        /* Allocate a new buffer from the heap */
        NewData = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferSize);
        if (NewData == NULL)
        {
            return STATUS_NO_MEMORY;
        }

        /* Copy the old buffer to the new one */
        RtlCopyMemory(NewData, Context->Data, Context->Data->CurrentSize);

        /* Free the old buffer and use the new one */
        RtlFreeHeap(RtlGetProcessHeap(), 0, Context->Data);
        Context->Data = NewData;
        NewData->BufferSize = BufferSize;
    }

    /* Get the next action record */
    Action = (RXACT_ACTION *)((PUCHAR)Context->Data + Context->Data->CurrentSize);

    /* Fill in the fields */
    Action->Size = ActionSize;
    Action->Type = ActionType;
    Action->KeyName = *KeyName;
    Action->ValueName = *ValueName;
    Action->ValueType = ValueType;
    Action->ValueDataSize = ValueDataSize;
    Action->KeyHandle = KeyHandle;

    /* Copy the key name (and convert the pointer to a buffer offset) */
    CurrentOffset = Context->Data->CurrentSize + sizeof(RXACT_ACTION);
    Action->KeyName.Buffer = UlongToPtr(CurrentOffset);
    RtlCopyMemory((PUCHAR)Context->Data + CurrentOffset,
                  KeyName->Buffer,
                  KeyName->Length);

    /* Copy the value name (and convert the pointer to a buffer offset) */
    CurrentOffset += ALIGN_UP_BY(KeyName->Length, sizeof(ULONG));
    Action->ValueName.Buffer = UlongToPtr(CurrentOffset);
    RtlCopyMemory((PUCHAR)Context->Data + CurrentOffset,
                  ValueName->Buffer,
                  ValueName->Length);

    /* Update the offset */
    CurrentOffset += ALIGN_UP_BY(ValueName->Length, sizeof(ULONG));

    /* Is this a set action? */
    if (ActionType == RXactSetValueKey)
    {
        /* Copy the key value data as well */
        Action->ValueData = UlongToPtr(CurrentOffset);
        RtlCopyMemory((PUCHAR)Context->Data + CurrentOffset,
                      ValueData,
                      ValueDataSize);
        CurrentOffset += ALIGN_UP_BY(ValueDataSize, sizeof(ULONG));
    }

    /* Update data site and action count */
    Context->Data->CurrentSize = CurrentOffset;
    Context->Data->ActionCount++;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlAddActionToRXact(
    PRXACT_CONTEXT Context,
    ULONG ActionType,
    PUNICODE_STRING KeyName,
    ULONG ValueType,
    PVOID ValueData,
    ULONG ValueDataSize)
{
    UNICODE_STRING ValueName;

    /* Create a key and set the default key value or delete a key. */
    RtlInitUnicodeString(&ValueName, NULL);
    return RtlAddAttributeActionToRXact(Context,
                                        ActionType,
                                        KeyName,
                                        INVALID_HANDLE_VALUE,
                                        &ValueName,
                                        ValueType,
                                        ValueData,
                                        ValueDataSize);
}

NTSTATUS
NTAPI
RtlApplyRXactNoFlush(
    PRXACT_CONTEXT Context)
{
    NTSTATUS Status;

    /* Commit the transaction */
    Status = RXactpCommit(Context);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Reset the transaction */
    Status = RtlAbortRXact(Context);
    ASSERT(NT_SUCCESS(Status));

    return Status;
}

NTSTATUS
NTAPI
RtlApplyRXact(
    PRXACT_CONTEXT Context)
{
    UNICODE_STRING ValueName;
    NTSTATUS Status;

    /* Temporarily safe the current transaction in the 'Log' key value */
    RtlInitUnicodeString(&ValueName, L"Log");
    Status = ZwSetValueKey(Context->KeyHandle,
                           &ValueName,
                           0,
                           REG_BINARY,
                           Context->Data,
                           Context->Data->CurrentSize);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Flush the key */
    Status = NtFlushKey(Context->KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        NtDeleteValueKey(Context->KeyHandle, &ValueName);
        return Status;
    }

    /* Now commit the transaction */
    Status = RXactpCommit(Context);
    if (!NT_SUCCESS(Status))
    {
        NtDeleteValueKey(Context->KeyHandle, &ValueName);
        return Status;
    }

    /* Delete the 'Log' key value */
    Status = NtDeleteValueKey(Context->KeyHandle, &ValueName);
    ASSERT(NT_SUCCESS(Status));

    /* Reset the transaction */
    Status = RtlAbortRXact(Context);
    ASSERT(NT_SUCCESS(Status));

    return STATUS_SUCCESS;
}

