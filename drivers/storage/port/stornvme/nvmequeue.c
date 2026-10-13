/*
 * PROJECT:     ReactOS NVM Express Miniport Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Submission and completion queue handling
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "stornvme.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

/**
 * @brief Copies a command to the submission queue tail and rings the doorbell.
 *
 * The caller serializes access to the queue.
 */
VOID
NvmpSubmitCommand(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PNVME_QUEUE_PAIR Queue,
    _In_ PNVME_COMMAND Command)
{
    Queue->SubmissionQueue[Queue->SubmissionTail] = *Command;

    Queue->SubmissionTail++;
    if (Queue->SubmissionTail == Queue->Depth)
        Queue->SubmissionTail = 0;

    /* Entry must be visible before the doorbell write */
    KeMemoryBarrier();

    StorPortWriteRegisterUlong(Adapter,
                               NvmpSubmissionDoorbell(Adapter, Queue->QueueId),
                               Queue->SubmissionTail);
}


/**
 * @brief Consumes the completion at the head if its phase tag is current.
 *
 * @return TRUE when Completion was filled in.
 */
BOOLEAN
NvmpNextCompletion(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PNVME_QUEUE_PAIR Queue,
    _Out_ PNVME_COMPLETION_ENTRY Completion)
{
    volatile NVME_COMPLETION_ENTRY *Entry;
    NVME_COMMAND_STATUS Status;

    Entry = &Queue->CompletionQueue[Queue->CompletionHead];

    /* Read the phase tag before the rest of the entry */
    KeMemoryBarrier();

    Status.AsUshort = (USHORT)(Entry->DW3.AsUlong >> 16);
    if (Status.P != Queue->Phase)
        return FALSE;

    Completion->DW0 = Entry->DW0;
    Completion->DW1 = Entry->DW1;
    Completion->DW2.AsUlong = Entry->DW2.AsUlong;
    Completion->DW3.AsUlong = Entry->DW3.AsUlong;

    Queue->CompletionHead++;
    if (Queue->CompletionHead == Queue->Depth)
    {
        Queue->CompletionHead = 0;
        Queue->Phase ^= 1;
    }

    StorPortWriteRegisterUlong(Adapter,
                               NvmpCompletionDoorbell(Adapter, Queue->QueueId),
                               Queue->CompletionHead);

    return TRUE;
}


/**
 * @brief Completes every new entry in the admin completion queue.
 *
 * The caller holds the queue lock, or interrupts are not connected yet.
 *
 * @return TRUE if at least one entry was found.
 */
BOOLEAN
NvmpDrainAdminQueue(
    _In_ PNVME_ADAPTER_EXTENSION Adapter)
{
    NVME_COMPLETION_ENTRY Completion;
    PNVME_REQUEST_CONTEXT Context;
    USHORT CommandId;
    BOOLEAN Handled = FALSE;

    while (NvmpNextCompletion(Adapter, &Adapter->AdminQueue, &Completion))
    {
        Handled = TRUE;
        CommandId = Completion.DW3.CID;

        if (CommandId == NVME_POLLED_COMMAND_ID)
        {
            Adapter->PolledCompletion = Completion;
            Adapter->PolledDone = TRUE;
            continue;
        }

        if (CommandId >= NVME_ADMIN_SLOTS || Adapter->AdminRequests[CommandId] == NULL)
        {
            DPRINT1("Admin completion names command %u, which is not outstanding\n", CommandId);
            continue;
        }

        Context = Adapter->AdminRequests[CommandId];
        Adapter->AdminRequests[CommandId] = NULL;

        Context->AdminCompletion(Adapter, Context, &Completion);
    }

    return Handled;
}


/**
 * @brief Submits an admin command on behalf of an SRB.
 *
 * The caller holds the queue lock. The command completes through Context->AdminCompletion.
 *
 * @return FALSE if every admin slot is in use.
 */
BOOLEAN
NvmpSubmitAdminRequest(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PNVME_REQUEST_CONTEXT Context)
{
    ULONG Slots;
    ULONG Slot;

    /* Room for the polled command too, and a full queue keeps one entry empty */
    Slots = min(NVME_ADMIN_SLOTS, Adapter->AdminQueue.Depth - 2);

    for (Slot = 0; Slot < Slots; Slot++)
    {
        if (Adapter->AdminRequests[Slot] != NULL)
            continue;

        Adapter->AdminRequests[Slot] = Context;
        Context->Command.CDW0.CID = (USHORT)Slot;

        NvmpSubmitCommand(Adapter, &Adapter->AdminQueue, &Context->Command);
        return TRUE;
    }

    return FALSE;
}


/**
 * @brief Submits an admin command and polls for its completion.
 *
 * Only one runs at a time, during startup or a reset. Once interrupts are connected the
 * interrupt routine may be the one to pick up the completion.
 */
BOOLEAN
NvmpIssueAdminCommand(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PNVME_COMMAND Command,
    _Out_opt_ PNVME_COMPLETION_ENTRY Completion)
{
    NVME_COMPLETION_ENTRY Result;
    NVME_QUEUE_LOCK Lock;
    ULONG Attempts;
    ULONG Limit;
    BOOLEAN Done;

    Command->CDW0.CID = NVME_POLLED_COMMAND_ID;

    NvmpAcquireQueueLock(Adapter, &Lock);
    Adapter->PolledDone = FALSE;
    NvmpSubmitCommand(Adapter, &Adapter->AdminQueue, Command);
    NvmpReleaseQueueLock(Adapter, &Lock);

    Limit = NVME_ADMIN_TIMEOUT_MS / (NVME_POLL_INTERVAL_US / 1000);

    for (Attempts = 0; ; Attempts++)
    {
        NvmpAcquireQueueLock(Adapter, &Lock);
        NvmpDrainAdminQueue(Adapter);
        Done = Adapter->PolledDone;
        Result = Adapter->PolledCompletion;
        NvmpReleaseQueueLock(Adapter, &Lock);

        if (Done)
            break;

        if (Attempts >= Limit)
        {
            DPRINT1("Admin command 0x%02x timed out\n", Command->CDW0.OPC);
            return FALSE;
        }

        StorPortStallExecution(NVME_POLL_INTERVAL_US);
    }

    if (Completion != NULL)
        *Completion = Result;

    if (Result.DW3.Status.SC != NVME_STATUS_SUCCESS_COMPLETION ||
        Result.DW3.Status.SCT != NVME_STATUS_TYPE_GENERIC_COMMAND)
    {
        DPRINT1("Admin command 0x%02x failed, type %u code 0x%02x\n",
                Command->CDW0.OPC, Result.DW3.Status.SCT, Result.DW3.Status.SC);
        return FALSE;
    }

    return TRUE;
}
