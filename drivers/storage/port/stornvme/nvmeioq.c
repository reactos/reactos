/*
 * PROJECT:     ReactOS NVM Express Miniport Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Input and output queue creation
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "stornvme.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

/**
 * @brief Sets the Number of Queues feature and returns the pairs granted.
 *
 * Counts in the command and the completion are 0 based.
 */
static
BOOLEAN
NvmpRequestQueueCount(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ USHORT Wanted,
    _Out_ PUSHORT Granted)
{
    NVME_COMPLETION_ENTRY Completion;
    NVME_CDW11_FEATURE_NUMBER_OF_QUEUES Answer;
    NVME_COMMAND Command;

    *Granted = 0;

    RtlZeroMemory(&Command, sizeof(Command));

    Command.CDW0.OPC = NVME_ADMIN_COMMAND_SET_FEATURES;
    Command.u.SETFEATURES.CDW10.FID = NVME_FEATURE_NUMBER_OF_QUEUES;
    Command.u.SETFEATURES.CDW11.NumberOfQueues.NSQ = Wanted - 1;
    Command.u.SETFEATURES.CDW11.NumberOfQueues.NCQ = Wanted - 1;

    if (!NvmpIssueAdminCommand(Adapter, &Command, &Completion))
        return FALSE;

    Answer.AsUlong = Completion.DW0;

    *Granted = Answer.NSQ;
    if (Answer.NCQ < Answer.NSQ)
        *Granted = Answer.NCQ;
    (*Granted)++;

    DPRINT1("Controller granted %u submission and %u completion queues\n",
            (USHORT)(Answer.NSQ + 1), (USHORT)(Answer.NCQ + 1));

    return TRUE;
}


/**
 * @brief Creates the IO completion queue of a pair on the given interrupt vector.
 */
static
BOOLEAN
NvmpCreateCompletionQueue(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PNVME_QUEUE_PAIR Queue,
    _In_ USHORT Vector)
{
    NVME_COMMAND Command;

    RtlZeroMemory(&Command, sizeof(Command));

    Command.CDW0.OPC = NVME_ADMIN_COMMAND_CREATE_IO_CQ;
    Command.PRP1 = Queue->CompletionAddress.QuadPart;
    Command.u.CREATEIOCQ.CDW10.QID = Queue->QueueId;
    Command.u.CREATEIOCQ.CDW10.QSIZE = Queue->Depth - 1;

    Command.u.CREATEIOCQ.CDW11.PC = 1;
    Command.u.CREATEIOCQ.CDW11.IEN = 1;
    Command.u.CREATEIOCQ.CDW11.IV = Vector;

    return NvmpIssueAdminCommand(Adapter, &Command, NULL);
}


/**
 * @brief Creates the IO submission queue of a pair.
 */
static
BOOLEAN
NvmpCreateSubmissionQueue(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PNVME_QUEUE_PAIR Queue)
{
    NVME_COMMAND Command;

    RtlZeroMemory(&Command, sizeof(Command));

    Command.CDW0.OPC = NVME_ADMIN_COMMAND_CREATE_IO_SQ;
    Command.PRP1 = Queue->SubmissionAddress.QuadPart;
    Command.u.CREATEIOSQ.CDW10.QID = Queue->QueueId;
    Command.u.CREATEIOSQ.CDW10.QSIZE = Queue->Depth - 1;

    Command.u.CREATEIOSQ.CDW11.PC = 1;
    Command.u.CREATEIOSQ.CDW11.QPRIO = NVME_NVM_QUEUE_PRIORITY_MEDIUM;
    Command.u.CREATEIOSQ.CDW11.CQID = Queue->QueueId;

    return NvmpIssueAdminCommand(Adapter, &Command, NULL);
}


BOOLEAN
NvmpCreateIoQueues(
    _In_ PNVME_ADAPTER_EXTENSION Adapter)
{
    PNVME_QUEUE_PAIR Queue = &Adapter->IoQueue;
    USHORT Granted;

    /* TODO: One queue pair per processor, each on its own message vector */
    if (!NvmpRequestQueueCount(Adapter, 1, &Granted))
    {
        DPRINT1("Controller would not say how many queues it has\n");
        return FALSE;
    }

    if (Granted == 0)
    {
        DPRINT1("Controller granted no queue pair\n");
        return FALSE;
    }

    /* The completion queue must exist before the submission queue */
    if (!NvmpCreateCompletionQueue(Adapter, Queue, 0))
    {
        DPRINT1("Could not create the completion queue\n");
        return FALSE;
    }

    if (!NvmpCreateSubmissionQueue(Adapter, Queue))
    {
        DPRINT1("Could not create the submission queue\n");
        return FALSE;
    }

    DPRINT1("Queue pair %u of %lu entries is up\n", Queue->QueueId, Queue->Depth);

    return TRUE;
}
