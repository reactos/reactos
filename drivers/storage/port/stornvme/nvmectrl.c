/*
 * PROJECT:     ReactOS NVM Express Miniport Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Controller reset, configuration and admin queue setup
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "stornvme.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

/**
 * @brief Polls CSTS.RDY until it matches Ready, for at most CAP.TO.
 */
static
BOOLEAN
NvmpWaitForReady(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ BOOLEAN Ready)
{
    NVME_CONTROLLER_STATUS Status;
    ULONG Attempts;
    ULONG Limit;

    Limit = Adapter->ReadyTimeout / (NVME_POLL_INTERVAL_US / 1000);

    for (Attempts = 0; ; Attempts++)
    {
        Status.AsUlong = StorPortReadRegisterUlong(Adapter,
                                                   &Adapter->Registers->CSTS.AsUlong);

        /* Clearing EN is how CFS gets recovered, so only enabling gives up on it */
        if (Ready && Status.CFS)
        {
            DPRINT1("Controller reported a fatal status\n");
            return FALSE;
        }

        if (Status.RDY == (Ready ? 1u : 0u))
            return TRUE;

        if (Attempts >= Limit)
            break;

        StorPortStallExecution(NVME_POLL_INTERVAL_US);
    }

    DPRINT1("Timed out waiting for ready to become %u\n", Ready);
    return FALSE;
}


BOOLEAN
NvmpDisableController(
    _In_ PNVME_ADAPTER_EXTENSION Adapter)
{
    NVME_CONTROLLER_CONFIGURATION Config;
    NVME_CONTROLLER_STATUS Status;

    Config.AsUlong = StorPortReadRegisterUlong(Adapter,
                                               &Adapter->Registers->CC.AsUlong);
    Status.AsUlong = StorPortReadRegisterUlong(Adapter,
                                               &Adapter->Registers->CSTS.AsUlong);

    /* Clearing EN while CSTS.RDY is still 0 is undefined, give it CAP.TO first */
    if (Config.EN && !Status.RDY)
        NvmpWaitForReady(Adapter, TRUE);

    Config.EN = 0;
    StorPortWriteRegisterUlong(Adapter,
                               &Adapter->Registers->CC.AsUlong,
                               Config.AsUlong);

    return NvmpWaitForReady(Adapter, FALSE);
}


BOOLEAN
NvmpEnableController(
    _In_ PNVME_ADAPTER_EXTENSION Adapter)
{
    NVME_CONTROLLER_CONFIGURATION Config;

    Config.AsUlong = 0;
    Config.CSS = NVME_CSS_NVM_COMMAND_SET;
    Config.MPS = Adapter->PageShift - NVME_MIN_PAGE_SHIFT;
    Config.AMS = NVME_AMS_ROUND_ROBIN;
    Config.IOSQES = NVME_SQ_ENTRY_SHIFT;
    Config.IOCQES = NVME_CQ_ENTRY_SHIFT;

    /* Program CC before setting EN */
    StorPortWriteRegisterUlong(Adapter,
                               &Adapter->Registers->CC.AsUlong,
                               Config.AsUlong);

    Config.EN = 1;
    StorPortWriteRegisterUlong(Adapter,
                               &Adapter->Registers->CC.AsUlong,
                               Config.AsUlong);

    return NvmpWaitForReady(Adapter, TRUE);
}


/**
 * @brief Sends a normal shutdown notification and waits for CSTS.SHST to report it done.
 */
BOOLEAN
NvmpShutdownController(
    _In_ PNVME_ADAPTER_EXTENSION Adapter)
{
    NVME_CONTROLLER_CONFIGURATION Config;
    NVME_CONTROLLER_STATUS Status;
    ULONG Attempts;
    ULONG Limit;

    Config.AsUlong = StorPortReadRegisterUlong(Adapter, &Adapter->Registers->CC.AsUlong);
    if (!Config.EN)
        return TRUE;

    Config.SHN = NVME_CC_SHN_NORMAL_SHUTDOWN;
    StorPortWriteRegisterUlong(Adapter, &Adapter->Registers->CC.AsUlong, Config.AsUlong);

    Limit = max(Adapter->ReadyTimeout, NVME_SHUTDOWN_TIMEOUT_MS) / (NVME_POLL_INTERVAL_US / 1000);

    for (Attempts = 0; Attempts <= Limit; Attempts++)
    {
        Status.AsUlong = StorPortReadRegisterUlong(Adapter, &Adapter->Registers->CSTS.AsUlong);
        if (Status.SHST == NVME_CSTS_SHST_SHUTDOWN_COMPLETED)
            return TRUE;

        StorPortStallExecution(NVME_POLL_INTERVAL_US);
    }

    DPRINT1("Shutdown notification timed out\n");
    return FALSE;
}


/**
 * @brief Writes a 64 bit controller register as two dwords, low half first.
 */
static
VOID
NvmpWriteRegister64(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PULONGLONG Register,
    _In_ PHYSICAL_ADDRESS Value)
{
    PULONG Half = (PULONG)Register;

    StorPortWriteRegisterUlong(Adapter, Half, Value.LowPart);
    StorPortWriteRegisterUlong(Adapter, Half + 1, Value.HighPart);
}


/**
 * @brief Resets the admin queue state and programs AQA, ASQ and ACQ.
 */
static
VOID
NvmpProgramAdminQueues(
    _In_ PNVME_ADAPTER_EXTENSION Adapter)
{
    PNVME_QUEUE_PAIR Queue = &Adapter->AdminQueue;
    NVME_ADMIN_QUEUE_ATTRIBUTES Attributes;

    Queue->SubmissionTail = 0;
    Queue->CompletionHead = 0;
    Queue->Phase = 1;

    RtlZeroMemory(Queue->CompletionQueue,
                  Queue->Depth * sizeof(*Queue->CompletionQueue));

    /* 0 based */
    Attributes.AsUlong = 0;
    Attributes.ASQS = Queue->Depth - 1;
    Attributes.ACQS = Queue->Depth - 1;

    StorPortWriteRegisterUlong(Adapter,
                               &Adapter->Registers->AQA.AsUlong,
                               Attributes.AsUlong);

    NvmpWriteRegister64(Adapter,
                        &Adapter->Registers->ASQ.AsUlonglong,
                        Queue->SubmissionAddress);
    NvmpWriteRegister64(Adapter,
                        &Adapter->Registers->ACQ.AsUlonglong,
                        Queue->CompletionAddress);
}


BOOLEAN
NvmpCreateAdminQueues(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PPORT_CONFIGURATION_INFORMATION ConfigInfo)
{
    PNVME_QUEUE_PAIR Queue = &Adapter->AdminQueue;
    PNVME_QUEUE_PAIR IoQueue = &Adapter->IoQueue;
    ULONG SubmissionSize;
    ULONG CompletionSize;
    ULONG IoSubmissionSize;
    ULONG IoCompletionSize;
    ULONG Offset;
    ULONG Length;

    /* One uncached allocation holds every queue and the scratch page */
    SubmissionSize = ROUND_TO_PAGES(Adapter->AdminQueueDepth * sizeof(*Queue->SubmissionQueue));
    CompletionSize = ROUND_TO_PAGES(Adapter->AdminQueueDepth * sizeof(*Queue->CompletionQueue));
    IoSubmissionSize = ROUND_TO_PAGES(Adapter->IoQueueDepth * sizeof(*IoQueue->SubmissionQueue));
    IoCompletionSize = ROUND_TO_PAGES(Adapter->IoQueueDepth * sizeof(*IoQueue->CompletionQueue));

    /* One more page so the base can be page aligned, dump mode makes no promise */
    Length = SubmissionSize + CompletionSize + PAGE_SIZE +
             IoSubmissionSize + IoCompletionSize + PAGE_SIZE;

    Adapter->QueueMemory = StorPortGetUncachedExtension(Adapter, ConfigInfo, Length);
    if (Adapter->QueueMemory == NULL)
    {
        DPRINT1("Could not obtain %lu bytes for the admin queues\n", Length);
        return FALSE;
    }

    Adapter->QueueMemorySize = Length;
    RtlZeroMemory(Adapter->QueueMemory, Length);

    Adapter->QueueMemoryAddress = StorPortGetPhysicalAddress(Adapter,
                                                             NULL,
                                                             Adapter->QueueMemory,
                                                             &Length);

    Offset = (PAGE_SIZE - (Adapter->QueueMemoryAddress.LowPart & (PAGE_SIZE - 1))) &
             (PAGE_SIZE - 1);
    Adapter->QueueMemory = (PUCHAR)Adapter->QueueMemory + Offset;
    Adapter->QueueMemoryAddress.QuadPart += Offset;

    RtlZeroMemory(Queue, sizeof(*Queue));
    RtlZeroMemory(IoQueue, sizeof(*IoQueue));

    Offset = 0;

    Queue->QueueId = NVME_ADMINQ_ID;
    Queue->Depth = Adapter->AdminQueueDepth;
    Queue->Phase = 1;

    Queue->SubmissionQueue = (PNVME_COMMAND)((PUCHAR)Adapter->QueueMemory + Offset);
    Queue->SubmissionAddress.QuadPart = Adapter->QueueMemoryAddress.QuadPart + Offset;
    Offset += SubmissionSize;

    Queue->CompletionQueue = (PNVME_COMPLETION_ENTRY)((PUCHAR)Adapter->QueueMemory + Offset);
    Queue->CompletionAddress.QuadPart = Adapter->QueueMemoryAddress.QuadPart + Offset;
    Offset += CompletionSize;

    Adapter->ScratchBuffer = (PUCHAR)Adapter->QueueMemory + Offset;
    Adapter->ScratchAddress.QuadPart = Adapter->QueueMemoryAddress.QuadPart + Offset;
    Offset += PAGE_SIZE;

    IoQueue->QueueId = NVME_IO_QUEUE_ID;
    IoQueue->Depth = Adapter->IoQueueDepth;
    IoQueue->Phase = 1;

    IoQueue->SubmissionQueue = (PNVME_COMMAND)((PUCHAR)Adapter->QueueMemory + Offset);
    IoQueue->SubmissionAddress.QuadPart = Adapter->QueueMemoryAddress.QuadPart + Offset;
    Offset += IoSubmissionSize;

    IoQueue->CompletionQueue = (PNVME_COMPLETION_ENTRY)((PUCHAR)Adapter->QueueMemory + Offset);
    IoQueue->CompletionAddress.QuadPart = Adapter->QueueMemoryAddress.QuadPart + Offset;

    NvmpProgramAdminQueues(Adapter);

    DPRINT1("Admin queues of %lu entries at 0x%I64x and 0x%I64x\n",
            Queue->Depth,
            Queue->SubmissionAddress.QuadPart,
            Queue->CompletionAddress.QuadPart);

    return TRUE;
}


/**
 * @brief Disables and re-enables the controller, then recreates the IO queues.
 *
 * Identify data is kept from startup.
 */
BOOLEAN
NvmpResetController(
    _In_ PNVME_ADAPTER_EXTENSION Adapter)
{
    PNVME_QUEUE_PAIR IoQueue = &Adapter->IoQueue;
    NVME_QUEUE_LOCK Lock;
    BOOLEAN Disabled;

    DPRINT1("Resetting the controller\n");

    NvmpAcquireQueueLock(Adapter, &Lock);
    Adapter->State = NvmeAdapterStopped;
    NvmpReleaseQueueLock(Adapter, &Lock);

    Disabled = NvmpDisableController(Adapter);

    /* Requests are only completed once the controller can no longer DMA into them */
    NvmpAcquireQueueLock(Adapter, &Lock);

    NvmpFailOutstandingRequests(Adapter, SRB_STATUS_BUS_RESET);

    IoQueue->SubmissionTail = 0;
    IoQueue->CompletionHead = 0;
    IoQueue->Phase = 1;

    RtlZeroMemory(IoQueue->CompletionQueue,
                  IoQueue->Depth * sizeof(*IoQueue->CompletionQueue));

    /* The interrupt routine also drains the admin queue, and AQA may only change while disabled */
    if (Disabled)
        NvmpProgramAdminQueues(Adapter);

    NvmpReleaseQueueLock(Adapter, &Lock);

    if (!Disabled)
    {
        DPRINT1("Controller would not go idle\n");
        Adapter->State = NvmeAdapterFailed;
        return FALSE;
    }

    if (!NvmpEnableController(Adapter))
    {
        DPRINT1("Controller would not come ready\n");
        Adapter->State = NvmeAdapterFailed;
        return FALSE;
    }

    if (!NvmpCreateIoQueues(Adapter))
    {
        Adapter->State = NvmeAdapterFailed;
        return FALSE;
    }

    NvmpAcquireQueueLock(Adapter, &Lock);
    NvmpInitializeCommandIds(Adapter);
    Adapter->State = NvmeAdapterRunning;
    NvmpReleaseQueueLock(Adapter, &Lock);

    DPRINT1("Controller is ready again\n");

    return TRUE;
}


/**
 * @brief Brings the controller from any state to ready with IO queues created.
 */
BOOLEAN
NvmpStartController(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PPORT_CONFIGURATION_INFORMATION ConfigInfo)
{
    if (!NvmpDisableController(Adapter))
    {
        DPRINT1("Controller would not go idle\n");
        return FALSE;
    }

    if (!NvmpCreateAdminQueues(Adapter, ConfigInfo))
        return FALSE;

    if (!NvmpEnableController(Adapter))
    {
        DPRINT1("Controller would not come ready\n");
        return FALSE;
    }

    if (!NvmpIdentifyController(Adapter))
        return FALSE;

    /* A controller with no active namespace still starts, with no LUNs */
    NvmpEnumerateNamespaces(Adapter);

    if (!NvmpCreateIoQueues(Adapter))
        return FALSE;

    NvmpInitializeCommandIds(Adapter);

    DPRINT1("Controller is ready\n");

    return TRUE;
}
