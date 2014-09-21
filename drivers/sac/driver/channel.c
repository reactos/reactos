/*
 * PROJECT:     ReactOS Drivers
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/sac/driver/channel.c
 * PURPOSE:     Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
ChannelIsValidType(IN SAC_CHANNEL_TYPE ChannelType)
{
    /* Check if the type is valid */
    return ((ChannelType >= VtUtf8) && (ChannelType <= Raw));
}

BOOLEAN
NTAPI
ChannelIsEqual(IN PSAC_CHANNEL Channel,
               IN PSAC_CHANNEL_ID ChannelId)
{
    /* Check if the GUIDs match */
    return IsEqualGUIDAligned(&Channel->ChannelId.ChannelGuid,
                              &ChannelId->ChannelGuid);
}

NTSTATUS
NTAPI
ChannelDereferenceHandles(IN PSAC_CHANNEL Channel)
{
    CHECK_PARAMETER(Channel);

    /* Clear the data event */
    if (Channel->HasNewDataEvent)
    {
        ChannelUninitializeEvent(Channel,
                                 HasNewDataEvent,
                                 SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT);
    }

    /* Clear the close event */
    if (Channel->CloseEvent)
    {
        ChannelUninitializeEvent(Channel,
                                 CloseEvent,
                                 SAC_CHANNEL_FLAG_CLOSE_EVENT);
    }

    /* Clear the lock event */
    if (Channel->LockEvent)
    {
        ChannelUninitializeEvent(Channel,
                                 LockEvent,
                                 SAC_CHANNEL_FLAG_LOCK_EVENT);
    }

    /* Clear the redraw event */
    if (Channel->RedrawEvent)
    {
        ChannelUninitializeEvent(Channel,
                                 RedrawEvent,
                                 SAC_CHANNEL_FLAG_REDRAW_EVENT);
    }

    /* All done */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ChannelDestroy(IN PSAC_CHANNEL Channel)
{
    CHECK_PARAMETER(Channel);

    /* Same thing as dereferencing all the handles */
    return ChannelDereferenceHandles(Channel);
}

NTSTATUS
NTAPI
ChannelOWrite(IN PSAC_CHANNEL Channel,
              IN PCHAR Buffer,
              IN ULONG BufferSize)
{
    NTSTATUS Status;
    CHECK_PARAMETER3(BufferSize < SAC_OBUFFER_SIZE);

    /* While holding the output lock, write to the output buffer */
    ChannelLockOBuffer(Channel);
    Status = Channel->ChannelOutputWrite(Channel, Buffer, BufferSize);
    ChannelUnlockOBuffer(Channel);
    return Status;
}

NTSTATUS
NTAPI
ChannelOFlush(IN PSAC_CHANNEL Channel)
{
    NTSTATUS Status;

    /* While holding the output lock, flush to the output buffer */
    ChannelLockOBuffer(Channel);
    Status = Channel->ChannelOutputFlush(Channel);
    ChannelUnlockOBuffer(Channel);
    return Status;
}

NTSTATUS
NTAPI
ChannelIWrite(IN PSAC_CHANNEL Channel,
              IN PCHAR Buffer,
              IN ULONG BufferSize)
{
    NTSTATUS Status;

    /* Write into the input buffer while holding the lock */
    ChannelLockIBuffer(Channel);
    Status = Channel->ChannelInputWrite(Channel, Buffer, BufferSize);
    ChannelUnlockIBuffer(Channel);
    return Status;
}

NTSTATUS
NTAPI
ChannelIRead(IN PSAC_CHANNEL Channel,
             IN PCHAR Buffer,
             IN ULONG BufferSize,
             IN OUT PULONG ResultBufferSize)
{
    NTSTATUS Status;

    /* Read the input buffer while holding the lock */
    ChannelLockIBuffer(Channel);
    Status = Channel->ChannelInputRead(Channel,
                                       Buffer,
                                       BufferSize,
                                       ResultBufferSize);
    ChannelUnlockIBuffer(Channel);
    return Status;
}

WCHAR
NTAPI
ChannelIReadLast(IN PSAC_CHANNEL Channel)
{
    WCHAR LastChar;

    /* Read the last character while holding the lock */
    ChannelLockIBuffer(Channel);
    LastChar = Channel->ChannelInputReadLast(Channel);
    ChannelUnlockIBuffer(Channel);
    return LastChar;
}

ULONG
NTAPI
ChannelIBufferLength(IN PSAC_CHANNEL Channel)
{
    ULONG Length;

    /* Get the input buffer length while holding the lock */
    ChannelLockOBuffer(Channel);
    Length = Channel->ChannelInputBufferLength(Channel);
    ChannelUnlockOBuffer(Channel);
    return Length;
}

NTSTATUS
NTAPI
ChannelSetRedrawEvent(IN PSAC_CHANNEL Channel)
{
    NTSTATUS Status;

    /* Set the event */
    ChannelSetEvent(Channel, RedrawEvent);
    return Status;
}

NTSTATUS
NTAPI
ChannelSetLockEvent(IN PSAC_CHANNEL Channel)
{
    NTSTATUS Status;

    /* Set the event */
    ChannelSetEvent(Channel, LockEvent);
    return Status;
}

NTSTATUS
NTAPI
ChannelClearRedrawEvent(IN PSAC_CHANNEL Channel)
{
    NTSTATUS Status;

    /* Clear the event */
    ChannelClearEvent(Channel, RedrawEvent);
    return Status;
}

NTSTATUS
NTAPI
ChannelHasRedrawEvent(IN PSAC_CHANNEL Channel,
                      OUT PBOOLEAN Present)
{
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(Present);

    /* Return if the flag is set */
    *Present = Channel->Flags & SAC_CHANNEL_FLAG_REDRAW_EVENT;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ChannelGetStatus(IN PSAC_CHANNEL Channel,
                 OUT PSAC_CHANNEL_STATUS ChannelStatus)
{
    CHECK_PARAMETER1(Channel);

    /* Read the status while holding the attribute lock */
    ChannelLockAttributes(Channel);
    *ChannelStatus = Channel->ChannelStatus;
    ChannelUnlockAttributes(Channel);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ChannelSetStatus(IN PSAC_CHANNEL Channel,
                 IN SAC_CHANNEL_STATUS ChannelStatus)
{
    CHECK_PARAMETER1(Channel);

    /* Read the status while holding the attribute lock */
    ChannelLockAttributes(Channel);
    Channel->ChannelStatus = ChannelStatus;
    ChannelUnlockAttributes(Channel);
    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
ChannelIsActive(IN PSAC_CHANNEL Channel)
{
    SAC_CHANNEL_STATUS ChannelStatus;
    BOOLEAN IsActive;

    /* Get the status */
    if (!NT_SUCCESS(ChannelGetStatus(Channel, &ChannelStatus)))
    {
        /* We couldn't even do that, assume it's inactive */
        IsActive = FALSE;
    }
    else
    {
        /* Check if the status shows activity */
        IsActive = (ChannelStatus == Active);
    }

    /* Return the state */
    return IsActive;
}

BOOLEAN
NTAPI
ChannelIsClosed(IN PSAC_CHANNEL Channel)
{
    SAC_CHANNEL_STATUS ChannelStatus;
    BOOLEAN IsClosed;

    /* Get the status */
    if (!NT_SUCCESS(ChannelGetStatus(Channel, &ChannelStatus)))
    {
        /* We couldn't even do that, assume it's inactive */
        IsClosed = FALSE;
    }
    else
    {
        /* Check if the status shows activity */
        IsClosed = ((ChannelStatus == Inactive) &&
                    (Channel->ChannelHasNewOBufferData));
    }

    /* Return the state */
    return IsClosed;
}

NTSTATUS
NTAPI
ChannelGetName(IN PSAC_CHANNEL Channel,
               OUT PWCHAR *Name)
{
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(Name);

    /* Allocate space to hold the name */
    *Name = SacAllocatePool(sizeof(Channel->NameBuffer), GLOBAL_BLOCK_TAG);
    CHECK_ALLOCATION(*Name);

    /* Lock the attributes while we copy the name */
    ChannelLockAttributes(Channel);

    /* Copy the name and null-terminate it */
    ASSERT(((wcslen(Channel->NameBuffer) + 1) * sizeof(WCHAR)) <= ((SAC_CHANNEL_NAME_SIZE + 1) * sizeof(WCHAR)));
    wcsncpy(*Name, Channel->NameBuffer, RTL_NUMBER_OF(Channel->NameBuffer)); // bug
    (*Name)[SAC_CHANNEL_NAME_SIZE] = UNICODE_NULL;

    /* Release the lock and return */
    ChannelUnlockAttributes(Channel);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ChannelSetName(IN PSAC_CHANNEL Channel,
               IN PWCHAR Name)
{
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(Name);

    /* Lock the attributes while we copy the name */
    ChannelLockAttributes(Channel);

    /* Copy the name and null-terminate it */
    ASSERT(((wcslen(Name) + 1) * sizeof(WCHAR)) <= ((SAC_CHANNEL_NAME_SIZE + 1) * sizeof(WCHAR)));
    wcsncpy(Channel->NameBuffer, Name, RTL_NUMBER_OF(Channel->NameBuffer)); // bug
    Channel->NameBuffer[SAC_CHANNEL_NAME_SIZE] = UNICODE_NULL;

    /* Release the lock and return */
    ChannelUnlockAttributes(Channel);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ChannelGetDescription(IN PSAC_CHANNEL Channel,
                      IN PWCHAR* Description)
{
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(Description);

    /* Allocate space to hold the description */
    *Description = SacAllocatePool(sizeof(Channel->DescriptionBuffer), GLOBAL_BLOCK_TAG);
    CHECK_ALLOCATION(*Description);

    /* Lock the attributes while we copy the name */
    ChannelLockAttributes(Channel);

    /* Copy the name and null-terminate it */
    ASSERT(((wcslen(Channel->DescriptionBuffer) + 1) * sizeof(WCHAR)) <= ((SAC_CHANNEL_DESCRIPTION_SIZE + 1) * sizeof(WCHAR)));
    wcsncpy(*Description, Channel->DescriptionBuffer, RTL_NUMBER_OF(Channel->DescriptionBuffer)); // bug
    (*Description)[SAC_CHANNEL_DESCRIPTION_SIZE] = UNICODE_NULL;

    /* Release the lock and return */
    ChannelUnlockAttributes(Channel);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ChannelSetDescription(IN PSAC_CHANNEL Channel,
                      IN PWCHAR Description)
{
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(Description);

    /* Lock the attributes while we copy the name */
    ChannelLockAttributes(Channel);

    /* Copy the name and null-terminate it */
    ASSERT(((wcslen(Description) + 1) * sizeof(WCHAR)) <= ((SAC_CHANNEL_NAME_SIZE + 1) * sizeof(WCHAR)));
    wcsncpy(Channel->DescriptionBuffer, Description, RTL_NUMBER_OF(Channel->DescriptionBuffer)); // bug
    Channel->DescriptionBuffer[SAC_CHANNEL_DESCRIPTION_SIZE] = UNICODE_NULL;

    /* Release the lock and return */
    ChannelUnlockAttributes(Channel);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ChannelGetApplicationType(IN PSAC_CHANNEL Channel,
                          OUT PGUID ApplicationType)
{
    CHECK_PARAMETER1(Channel);

    /* Read the application type GUID */
    ChannelLockAttributes(Channel);
    *ApplicationType = Channel->ApplicationType;
    ChannelUnlockAttributes(Channel);

    /* Always return success */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ChannelInitializeVTable(IN PSAC_CHANNEL Channel)
{
    /* What kind of channel is this? */
    switch (Channel->ChannelType)
    {
        case VtUtf8:
            /* Setup the calls for a VT-UTF8 channel */
            Channel->ChannelCreate = VTUTF8ChannelCreate;
            Channel->ChannelDestroy = VTUTF8ChannelDestroy;
            Channel->ChannelOutputFlush = VTUTF8ChannelOFlush;
            Channel->ChannelOutputEcho = VTUTF8ChannelOEcho;
            Channel->ChannelOutputWrite = VTUTF8ChannelOWrite;
            Channel->ChannelOutputRead = VTUTF8ChannelORead;
            Channel->ChannelInputWrite = VTUTF8ChannelIWrite;
            Channel->ChannelInputRead = VTUTF8ChannelIRead;
            Channel->ChannelInputReadLast = VTUTF8ChannelIReadLast;
            Channel->ChannelInputBufferIsFull = VTUTF8ChannelIBufferIsFull;
            Channel->ChannelInputBufferLength = VTUTF8ChannelIBufferLength;
            break;

        case Cmd:
            /* FIXME: TODO */
            ASSERT(FALSE);
            return STATUS_NOT_IMPLEMENTED;

        case Raw:

            /* Setup the calls for a raw channel */
            Channel->ChannelCreate = RawChannelCreate;
            Channel->ChannelDestroy = RawChannelDestroy;
            Channel->ChannelOutputFlush = RawChannelOFlush;
            Channel->ChannelOutputEcho = RawChannelOEcho;
            Channel->ChannelOutputWrite = RawChannelOWrite;
            Channel->ChannelOutputRead = RawChannelORead;
            Channel->ChannelInputWrite = RawChannelIWrite;
            Channel->ChannelInputRead = RawChannelIRead;
            Channel->ChannelInputReadLast = RawChannelIReadLast;
            Channel->ChannelInputBufferIsFull = RawChannelIBufferIsFull;
            Channel->ChannelInputBufferLength = RawChannelIBufferLength;
            break;

        default:
            /* Unsupported channel type */
            return STATUS_INVALID_PARAMETER;
    }

    /* If we got here, the channel was supported */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ChannelCreate(IN PSAC_CHANNEL Channel,
              IN PSAC_CHANNEL_ATTRIBUTES Attributes,
              IN SAC_CHANNEL_ID ChannelId)
{
    NTSTATUS Status;
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(Attributes);

    /* If a close event is being passed in, it must exist, and vice-versa */
    if (Attributes->Flag & SAC_CHANNEL_FLAG_CLOSE_EVENT)
    {
        CHECK_PARAMETER(Attributes->CloseEvent != NULL);
    }
    else
    {
        CHECK_PARAMETER(Attributes->CloseEvent == NULL);
    }

    /* If a new data event is being passed in, it must exist, and vice-versa */
    if (Attributes->Flag & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT)
    {
        CHECK_PARAMETER(Attributes->HasNewDataEvent != NULL);
    }
    else
    {
        CHECK_PARAMETER(Attributes->HasNewDataEvent == NULL);
    }

    /* If a lock event is being passed in, it must exist, and vice-versa */
    if (Attributes->Flag & SAC_CHANNEL_FLAG_LOCK_EVENT)
    {
        CHECK_PARAMETER(Attributes->LockEvent != NULL);
    }
    else
    {
        CHECK_PARAMETER(Attributes->LockEvent == NULL);
    }

    /* If a redraw event is being passed in, it must exist, and vice-versa */
    if (Attributes->Flag & SAC_CHANNEL_FLAG_REDRAW_EVENT)
    {
        CHECK_PARAMETER(Attributes->RedrawEvent != NULL);
    }
    else
    {
        CHECK_PARAMETER(Attributes->RedrawEvent == NULL);
    }

    /* Initialize the channel structure */
    RtlZeroMemory(Channel, sizeof(SAC_CHANNEL));
    Channel->ChannelId = ChannelId;
    Channel->ChannelType = Attributes->ChannelType;
    Channel->Flags = Attributes->Flag;
    if (Attributes->Flag & SAC_CHANNEL_FLAG_APPLICATION)
    {
        Channel->ApplicationType = Attributes->ChannelId;
    }

    /* Initialize all the locks and events */
    SacInitializeLock(&Channel->ChannelAttributeLock);
    SacInitializeLock(&Channel->ChannelOBufferLock);
    SacInitializeLock(&Channel->ChannelIBufferLock);
    ChannelInitializeEvent(Channel, Attributes, CloseEvent);
    ChannelInitializeEvent(Channel, Attributes, HasNewDataEvent);
    ChannelInitializeEvent(Channel, Attributes, LockEvent);
    ChannelInitializeEvent(Channel, Attributes, RedrawEvent);

    /* Set the name and description */
    ChannelSetName(Channel, Attributes->NameBuffer);
    ChannelSetDescription(Channel, Attributes->DescriptionBuffer);

    /* Initialize the function table for the type of channel this is */
    Status = ChannelInitializeVTable(Channel);
    if (!NT_SUCCESS(Status))
    {
        /* This is critical */
        SAC_DBG(SAC_DBG_INIT, "SAC Create Channel :: Failed to initialize vtable\n");
        goto FailChannel;
    }

    /* Now call the channel specific type constructor */
    Status = Channel->ChannelCreate(Channel);
    if (!NT_SUCCESS(Status))
    {
        /* This is critical */
        SAC_DBG(SAC_DBG_INIT, "SAC Create Channel :: Failed channel specific initialization\n");
        goto FailChannel;
    }

    /* Finally, mark the channel as active */
    ChannelSetStatus(Channel, Active);
    return STATUS_SUCCESS;

FailChannel:
    /* Destroy the channel and return the failure code */
    Channel->ChannelDestroy(Channel);
    return Status;
}

NTSTATUS
NTAPI
ChannelClose(IN PSAC_CHANNEL Channel)
{
    NTSTATUS Status;
    CHECK_PARAMETER(Channel);

    /* Set the channel inactive */
    ChannelSetStatus(Channel, Inactive);

    /* Set the close event */
    if (Channel->Flags & SAC_CHANNEL_FLAG_CLOSE_EVENT)
    {
        ChannelSetEvent(Channel, CloseEvent);
    }

    /* Close all the handles */
    Status = ChannelDereferenceHandles(Channel);
    return Status;
}
