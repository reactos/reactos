/*
 * PROJECT:     ReactOS Drivers
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/sac/driver/chanmgr.c
 * PURPOSE:     Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

/* GLOBALS ********************************************************************/

SAC_CHANNEL_LOCK ChannelCreateLock;
BOOLEAN ChannelCreateEnabled;
PSAC_CHANNEL ChannelArray[SAC_MAX_CHANNELS];
LONG ChannelRefCount[SAC_MAX_CHANNELS];
LONG ChannelReaped[SAC_MAX_CHANNELS];
SAC_CHANNEL_LOCK ChannelSlotLock[SAC_MAX_CHANNELS];

/* FUNCTIONS ******************************************************************/

#define MAX_REF_COUNT 100

#define CHANNEL_SLOT_IS_IN_USE(x)   (ChannelRefCount[(x)] > 0)

FORCEINLINE
PSAC_CHANNEL
ChannelFromIndex(IN ULONG Index)
{
    return ChannelArray[Index];
}

FORCEINLINE
LONG
ChannelGetReferenceCount(IN LONG Index)
{
    return ChannelRefCount[Index];
}

FORCEINLINE
LONG
ChannelReferenceByIndex(IN LONG Index)
{
    if (ChannelGetReferenceCount(Index) > 0)
    {
        ASSERT(ChannelRefCount[Index] <= MAX_REF_COUNT);
        ASSERT(ChannelRefCount[Index] >= 1);
        _InterlockedIncrement(&ChannelRefCount[Index]);
        ASSERT(ChannelRefCount[Index] <= MAX_REF_COUNT);
        ASSERT(ChannelRefCount[Index] >= 2);
    }

    return ChannelGetReferenceCount(Index);
}

FORCEINLINE
LONG
ChannelReferenceByIndexWithLock(IN LONG Index)
{
    LONG RefCount;

    ChannelSlotLock(Index);
    RefCount = ChannelReferenceByIndex(Index);
    ChannelSlotUnlock(Index);
    return RefCount;
}

FORCEINLINE
LONG
ChannelDereferenceByIndex(IN LONG Index)
{
    ASSERT(ChannelGetReferenceCount(Index) <= MAX_REF_COUNT);
    ASSERT(ChannelGetReferenceCount(Index) > 1);
    _InterlockedDecrement(&ChannelRefCount[Index]);
    ASSERT(ChannelGetReferenceCount(Index) >= 1);
    return ChannelGetReferenceCount(Index);
}

FORCEINLINE
VOID
ChannelDereferenceByIndexWithLock(IN LONG Index)
{
    ChannelSlotLock(Index);
    ChannelDereferenceByIndex(Index);
    ChannelSlotUnlock(Index);
}

FORCEINLINE
VOID
ChannelDereferenceToZeroByIndex(IN LONG Index)
{
    ASSERT(ChannelGetReferenceCount(Index) == 1);
    ASSERT(ChannelIsActive(ChannelFromIndex(Index)) == FALSE);
    _InterlockedExchange(&ChannelRefCount[Index], 0);
}

FORCEINLINE
VOID
ChannelReferenceToOneByIndex(IN LONG Index)
{
    ASSERT(ChannelGetReferenceCount(Index) == 0);
    _InterlockedExchange(&ChannelRefCount[Index], 1);
}

FORCEINLINE
VOID
ChannelReferenceToOneByIndexWithLock(IN LONG Index)
{
    ChannelSlotLock(Index);
    ChannelReferenceToOneByIndex(Index);
    ChannelSlotUnlock(Index);
}

NTSTATUS
NTAPI
ChanMgrInitialize(VOID)
{
    ULONG i;

    /* Initialize the channel lock */
    SacInitializeLock(&ChannelCreateLock);
    ChannelCreateEnabled = TRUE;

    /* Loop through the channel arrays */
    for (i = 0; i < SAC_MAX_CHANNELS; i++)
    {
        /* Clear and initialize their locks */
        ChannelArray[i] = NULL;
        SacInitializeLock(&ChannelSlotLock[i]);

        /* Clear their statuses and reference counts */
        _InterlockedExchange(&ChannelRefCount[i], 0);
        _InterlockedExchange(&ChannelReaped[i], 1);
    }

    /* All good */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ChanMgrShutdown(VOID)
{
    /* FIXME: TODO */
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
ChanMgrGetChannelByName(IN PWCHAR Name,
                        OUT PSAC_CHANNEL* Channel)
{
    NTSTATUS Status, Status1;
    ULONG i;
    PSAC_CHANNEL CurrentChannel;
    PWCHAR ChannelName;
    BOOLEAN Found;
    CHECK_PARAMETER1(Name);
    CHECK_PARAMETER2(Channel);

    /* Assume failure */
    *Channel = NULL;
    Status = STATUS_NOT_FOUND;

    /* Loop through all channels */
    for (i = 0; i < SAC_MAX_CHANNELS; i++)
    {
        /* Reference this one and check if it's valid */
        if (ChannelReferenceByIndexWithLock(i) > 0)
        {
            /* All good, grab it */
            CurrentChannel = ChannelFromIndex(i);
            ASSERT(CurrentChannel != NULL);

            /* Get its name */
            Status1 = ChannelGetName(CurrentChannel, &ChannelName);
            ASSERT(NT_SUCCESS(Status1));

            /* Check if this is the name that was passed in */
            Found = _wcsicmp(Name, ChannelName);
            SacFreePool(ChannelName);
            if (Found)
            {
                /* We found it, return it (with a reference held) */
                *Channel = CurrentChannel;
                return STATUS_SUCCESS;
            }

            /* Not the one we want, dereference this one and keep going */
            ChannelDereferenceByIndexWithLock(i);
        }
    }

    /* No channels with this name were found */
    return Status;
}

NTSTATUS
NTAPI
ChanMgrGetByHandle(IN SAC_CHANNEL_ID ChannelId,
                   OUT PSAC_CHANNEL* TargetChannel)
{
    NTSTATUS Status;
    ULONG i;
    PSAC_CHANNEL Channel;
    CHECK_PARAMETER2(TargetChannel);

    /* Assume failure */
    *TargetChannel = NULL;
    Status = STATUS_NOT_FOUND;

    /* Loop through all channels */
    for (i = 0; i < SAC_MAX_CHANNELS; i++)
    {
        /* Reference this one and check if it's valid */
        if (ChannelReferenceByIndexWithLock(i) > 0)
        {
            /* All good, grab it */
            Channel = ChannelFromIndex(i);
            ASSERT(Channel != NULL);

            /* Check if the channel ID matches */
            if (ChannelIsEqual(Channel, &ChannelId))
            {
                /* We found it, return it (with a reference held) */
                *TargetChannel = Channel;
                return STATUS_SUCCESS;
            }

            /* Not the one we want, dereference this one and keep going */
            ChannelDereferenceByIndexWithLock(i);
        }
    }

    /* No channels with this ID were found */
    return Status;
}

NTSTATUS
NTAPI
ChanMgrReleaseChannel(IN PSAC_CHANNEL Channel)
{
    LONG Index;
    ULONG RefCount;
    PSAC_CHANNEL ThisChannel;
    CHECK_PARAMETER(Channel);

    /* Get the index of the channel */
    Index = ChannelGetIndex(Channel);

    /* Drop a reference -- there should still be at least the keepalive left */
    ChannelSlotLock(Index);
    RefCount = ChannelDereferenceByIndex(Index);
    ASSERT(RefCount > 0);

    /* Do we only have the keep-alive left, and the channel is dead? */
    if ((RefCount == 1) && !(ChannelIsActive(Channel)))
    {
        /* Check if the ??? flag is set, or if there's no output data */
        ThisChannel = ChannelFromIndex(Index);
        if (!(ThisChannel->Flags & 1))
        {
            /* Nope, we can wipe the references and get rid of it */
            ChannelDereferenceToZeroByIndex(Index);
        }
        else if (!ThisChannel->ChannelHasNewOBufferData)
        {
            /* No data, we can wipe the references and get rid of it */
            ChannelDereferenceToZeroByIndex(Index);
        }
    }

    /* We're done, we can unlock the slot now */
    ChannelSlotUnlock(Index);
    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
ChanMgrIsUniqueName(IN PWCHAR ChannelName)
{
    NTSTATUS Status;
    BOOLEAN IsUnique = FALSE;
    PSAC_CHANNEL Channel;

    /* Check if a channel with this name already exists */
    Status = ChanMgrGetChannelByName(ChannelName, &Channel);
    if (Status == STATUS_NOT_FOUND) IsUnique = TRUE;

    /* If one did, dereference it, all we wanted was to check uniqueness */
    if (NT_SUCCESS(Status)) ChanMgrReleaseChannel(Channel);

    /* Return if one was found or not */
    return IsUnique;
}

NTSTATUS
NTAPI
ChanMgrReapChannel(IN ULONG ChannelIndex)
{
    /* FIXME: TODO */
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
ChanMgrReapChannels(VOID)
{
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Loop all the channels */
    for (i = 0; i < SAC_MAX_CHANNELS; i++)
    {
        /* Lock this index and see if the channel was reaped */
        ChannelSlotLock(i);
        if (!ChannelReaped[i])
        {
            /* It was not reaped yet, so a channel should still be here */
            ASSERT(ChannelFromIndex(i) != NULL);
            if (ChannelGetReferenceCount(i) <= 0)
            {
                /* The channel has no more references, so clear the buffer flags */
                _InterlockedExchange(&ChannelArray[i]->ChannelHasNewIBufferData, 0);
                _InterlockedExchange(&ChannelArray[i]->ChannelHasNewOBufferData, 0);

                /* And reap it */
                Status = ChanMgrReapChannel(i);
            }
        }

        /* Release the lock, and move on unless reaping failed */
        ChannelSlotUnlock(i);
        if (!NT_SUCCESS(Status)) break;
    }

    /* Return reaping status */
    return Status;
}

NTSTATUS
NTAPI
ChanMgrCreateChannel(OUT PSAC_CHANNEL *Channel,
                     IN PSAC_CHANNEL_ATTRIBUTES Attributes)
{
    NTSTATUS Status;
    PSAC_CHANNEL NewChannel;
    SAC_CHANNEL_ID ChanId;
    ULONG i;
    CHECK_PARAMETER(Channel);
    CHECK_PARAMETER2(Attributes);

    /* No other channel create attempts can happen */
    ChannelLockCreates();

    /* Is the channel manager initialized? */
    if (!ChannelCreateEnabled)
    {
        /* Nope, bail out */
        Status = STATUS_UNSUCCESSFUL;
        goto ReturnStatus;
    }

    /* Reap any zombie channels */
    Status = ChanMgrReapChannels();
    if (!NT_SUCCESS(Status))
    {
        /* Bail out on error */
        Status = STATUS_UNSUCCESSFUL;
        goto ReturnStatus;
    }

    /* Check if we already have a channel with this name */
    if (!ChanMgrIsUniqueName(Attributes->NameBuffer))
    {
        /* We do, fail */
        Status = STATUS_DUPLICATE_NAME;
        goto ReturnStatus;
    }

    /* Allocate this channel */
    NewChannel = SacAllocatePool(sizeof(SAC_CHANNEL), CHANNEL_BLOCK_TAG);
    CHECK_PARAMETER_WITH_STATUS(NewChannel, STATUS_NO_MEMORY); // bug
    RtlZeroMemory(NewChannel, sizeof(SAC_CHANNEL));

    /* Loop channel slots */
    for (i = 0; i < SAC_MAX_CHANNELS; i++)
    {
        /* Find a free spot for it */
        if (ChannelReaped[i])
        {
            /* Free slot found, attempt to use it */
            ASSERT(!CHANNEL_SLOT_IS_IN_USE(i));
            InterlockedCompareExchangePointer((PVOID*)&ChannelArray[i], NewChannel, NULL);
            if (ChannelArray[i] == NewChannel) break;
        }
    }

    /* Did we not find a single free slot? */
    if (i == SAC_MAX_CHANNELS)
    {
        /* Bail out */
        goto ReturnStatus;
    }

    /* Create an ID for this channel */
    RtlZeroMemory(&ChanId, sizeof(ChanId));
    Status = ExUuidCreate(&ChanId.ChannelGuid);
    if (!NT_SUCCESS(Status))
    {
        /* Bail out if we couldn't */
        SAC_DBG(SAC_DBG_INIT, "SAC Create Channel :: Failed to get GUID\n");
        goto ReturnStatus;
    }

    /* Now create the channel proper */
    Status = ChannelCreate(NewChannel, Attributes, ChanId);
    if (NT_SUCCESS(Status))
    {
        /* Set the channel index */
        _InterlockedExchange(&NewChannel->Index, i);

        /* Add the initial reference to the channel */
        ChannelReferenceToOneByIndexWithLock(i);

        /* Return it to the caller */
        *Channel = NewChannel;

        /* This slot is now occupied */
        ASSERT(ChannelReaped[i] == 1);
        _InterlockedExchange(&ChannelReaped[i], 0);
    }
    else
    {
        /* We couldn't create it, free the buffer */
        SacFreePool(NewChannel);
    }

ReturnStatus:
    /* Return whatever the operation status was */
    ChannelUnlockCreates();
    return Status;
}

NTSTATUS
NTAPI
ChanMgrGetByHandleAndFileObject(IN SAC_CHANNEL_ID ChannelId,
                                IN PFILE_OBJECT FileObject,
                                OUT PSAC_CHANNEL* TargetChannel)
{
    NTSTATUS Status;
    PSAC_CHANNEL FoundChannel;

    /* Lookup the channel by ID first */
    Status = ChanMgrGetByHandle(ChannelId, &FoundChannel);
    if (NT_SUCCESS(Status))
    {
        /* We found it, now check if the file object matches */
        if (FoundChannel->FileObject == FileObject)
        {
            /* Yep, return success */
            *TargetChannel = FoundChannel;
        }
        else
        {
            /* Nope, drop the reference on the channel */
            ChanMgrReleaseChannel(FoundChannel);

            /* And return failure */
            *TargetChannel = NULL;
            Status = STATUS_NOT_FOUND;
        }
    }

    /* Return if we found it or not */
    return Status;
}

NTSTATUS
NTAPI
ChanMgrGetChannelIndex(IN PSAC_CHANNEL Channel,
                       IN PLONG ChannelIndex)
{
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER2(ChannelIndex);

    /* Just return the index of the channel */
    *ChannelIndex = ChannelGetIndex(Channel);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ChanMgrGetByIndex(IN LONG TargetIndex,
                  IN PSAC_CHANNEL* TargetChannel)
{
    NTSTATUS Status;
    CHECK_PARAMETER1(TargetIndex < SAC_MAX_CHANNELS);
    CHECK_PARAMETER2(TargetChannel);

    /* Assume failure */
    *TargetChannel = NULL;
    Status = STATUS_NOT_FOUND;

    /* Reference this one and check if it's valid */
    if (ChannelReferenceByIndexWithLock(TargetIndex) > 0)
    {
        /* We found it, return it (with a reference held) */
        *TargetChannel = ChannelFromIndex(TargetIndex);
        return STATUS_SUCCESS;
    }

    /* No channels with this ID were found */
    return Status;
}

NTSTATUS
NTAPI
ChanMgrGetNextActiveChannel(IN PSAC_CHANNEL CurrentChannel,
                            IN PULONG TargetIndex,
                            OUT PSAC_CHANNEL *TargetChannel)
{
    NTSTATUS Status;
    ULONG i;
    LONG ChannelIndex, StartIndex;
    PSAC_CHANNEL FoundChannel;
    BOOLEAN ChannelFound;
    CHECK_PARAMETER1(CurrentChannel);
    CHECK_PARAMETER2(TargetIndex);
    CHECK_PARAMETER3(TargetChannel);

    /* Get the current channel index */
    Status = ChanMgrGetChannelIndex(CurrentChannel, &ChannelIndex);
    if (!NT_SUCCESS(Status)) return Status;

    /* Assume failure */
    ChannelFound = FALSE;

    /* Loop through all the possible active channels */
    StartIndex = (ChannelIndex + 1) % SAC_MAX_CHANNELS;
    for (i = StartIndex; i != StartIndex; i = (i + 1) % SAC_MAX_CHANNELS)
    {
        /* Get the channel and see if it exists*/
        Status = ChanMgrGetByIndex(i, &FoundChannel);
        if (Status != STATUS_NOT_FOUND)
        {
            /* Bail out if we failed for some reason */
            if (!NT_SUCCESS(Status)) return Status;

            /* It exists -- is it active? Or, does it have output data? */
            if ((ChannelIsActive(FoundChannel)) ||
                (!(ChannelIsActive(FoundChannel)) &&
                  (FoundChannel->ChannelHasNewOBufferData)))
            {
                /* It's active or has output data, return with it */
                ChannelFound = TRUE;
                break;
            }

            /* Drop the reference on this channel and try the next one */
            Status = ChanMgrReleaseChannel(FoundChannel);
            if (!NT_SUCCESS(Status)) return Status;
        }
    }

    /* Check if we successfully found a channel */
    if ((NT_SUCCESS(Status)) && (ChannelFound))
    {
        /* Return it and its indexed. Remember we still hold the reference */
        *TargetIndex = i;
        *TargetChannel = FoundChannel;
    }

    /* All done */
    return Status;
}

NTSTATUS
NTAPI
ChanMgrChannelDestroy(IN PSAC_CHANNEL Channel)
{
    CHECK_PARAMETER1(Channel);
    CHECK_PARAMETER(ChannelGetReferenceCount(Channel->Index) > 0);

    /* Destroy the channel */
    return Channel->ChannelDestroy(Channel);
}

NTSTATUS
NTAPI
ChanMgrCloseChannel(IN PSAC_CHANNEL Channel)
{
    NTSTATUS Status;
    CHECK_PARAMETER(Channel);

    /* Check if the channel is active */
    if (ChannelIsActive(Channel))
    {
        /* Yep, close it */
        Status = ChannelClose(Channel);
    }
    else
    {
        /* Nothing to do */
        Status = STATUS_ALREADY_DISCONNECTED;
    }

    /* Handle the channel close */
    ConMgrHandleEvent(TRUE, Channel, &Status);
    return Status;
}

NTSTATUS
NTAPI
ChanMgrGetChannelCount(OUT PULONG ChannelCount)
{
    ULONG i;
    PSAC_CHANNEL Channel;
    NTSTATUS Status;
    CHECK_PARAMETER(ChannelCount);

    /* Assume no channels */
    *ChannelCount = 0;

    /* Loop every channel */
    for (i = 0; i < SAC_MAX_CHANNELS; i++)
    {
        /* See if this one exists */
        Status = ChanMgrGetByIndex(i, &Channel);
        if (Status != STATUS_NOT_FOUND)
        {
            /* Sanity checks*/
            ASSERT(NT_SUCCESS(Status));
            ASSERT(Channel != NULL);

            /* It exists -- is it active? Or, does it have output data? */
            if ((ChannelIsActive(Channel)) ||
                (!(ChannelIsActive(Channel)) &&
                 (Channel->ChannelHasNewOBufferData)))
            {
                /* It's active or has output data, increase the count */
                ++*ChannelCount;
                break;
            }

            /* Drop the reference on this channel and try the next one */
            Status = ChanMgrReleaseChannel(Channel);
            if (!NT_SUCCESS(Status)) return Status;
        }
        else
        {
            /* Channel doesn't exist, nothing wrong with that, keep going */
            Status = STATUS_SUCCESS;
        }
    }

    /* We should always succeed if we get here */
    ASSERT(NT_SUCCESS(Status));
    return Status;
}

NTSTATUS
NTAPI
ChanMgrIsFull(OUT PBOOLEAN IsFull)
{
    NTSTATUS Status;
    ULONG Count;

    /* Count the channels */
    Status = ChanMgrGetChannelCount(&Count);
    CHECK_PARAMETER(Status == STATUS_SUCCESS);

    /* Return if we hit the limit */
    *IsFull = (Count == SAC_MAX_CHANNELS);
    return Status;
}

NTSTATUS
NTAPI
ChanMgrCloseChannelsWithFileObject(IN PFILE_OBJECT FileObject)
{
    PSAC_CHANNEL Channel;
    ULONG i;
    NTSTATUS Status;
    CHECK_PARAMETER1(FileObject);

    /* Loop all channels */
    for (i = 0; i < SAC_MAX_CHANNELS; i++)
    {
        /* Try to get this one */
        Status = ChanMgrGetByIndex(i, &Channel);
        if (!NT_SUCCESS(Status)) break;

        /* Check if the FO matches, if so, close the channel */
        if (Channel->FileObject == FileObject) ChanMgrCloseChannel(Channel);

        /* Drop the reference and try the next channel(s) */
        Status = ChanMgrReleaseChannel(Channel);
        if (!NT_SUCCESS(Status)) break;
    }

    /* All done */
    return Status;
}

NTSTATUS
NTAPI
ChanMgrGenerateUniqueCmdName(IN PWCHAR ChannelName)
{
    return STATUS_NOT_IMPLEMENTED;
}
