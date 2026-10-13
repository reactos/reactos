/*
 * PROJECT:     ReactOS NVM Express Miniport Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Driver entry and controller discovery
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "stornvme.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

/**
 * @brief Reads a 64 bit controller register as two dwords, low half first.
 */
static
ULONGLONG
NvmpReadRegister64(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PULONGLONG Register)
{
    PULONG Half = (PULONG)Register;
    UINT64 Low;
    UINT64 High;

    Low = StorPortReadRegisterUlong(Adapter, Half);
    High = StorPortReadRegisterUlong(Adapter, Half + 1);

    return (High << 32) | Low;
}


/**
 * @brief Maps the controller registers from the access range that matches BAR0.
 *
 * Storport does not promise the register range is listed first.
 */
static
BOOLEAN
NvmpMapRegisters(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PPORT_CONFIGURATION_INFORMATION ConfigInfo)
{
    PCI_COMMON_HEADER Config;
    PACCESS_RANGE Range = NULL;
    ULONGLONG Bar;
    ULONG Index;

    if (StorPortGetBusData(Adapter,
                           PCIConfiguration,
                           ConfigInfo->SystemIoBusNumber,
                           ConfigInfo->SlotNumber,
                           &Config,
                           sizeof(Config)) != sizeof(Config))
    {
        DPRINT1("Could not read the PCI configuration header\n");
        return FALSE;
    }

    Bar = ((ULONGLONG)Config.u.type0.BaseAddresses[1] << 32) |
          (Config.u.type0.BaseAddresses[0] & NVME_MLBAR_ADDRESS_MASK);

    for (Index = 0; Index < ConfigInfo->NumberOfAccessRanges; Index++)
    {
        if ((ULONGLONG)(*ConfigInfo->AccessRanges)[Index].RangeStart.QuadPart == Bar)
        {
            Range = &(*ConfigInfo->AccessRanges)[Index];
            break;
        }
    }

    if (Range == NULL)
    {
        DPRINT1("No access range at BAR0 0x%I64x\n", Bar);
        return FALSE;
    }

    Adapter->Registers = StorPortGetDeviceBase(Adapter,
                                               ConfigInfo->AdapterInterfaceType,
                                               ConfigInfo->SystemIoBusNumber,
                                               Range->RangeStart,
                                               Range->RangeLength,
                                               !Range->RangeInMemory);
    if (Adapter->Registers == NULL)
    {
        DPRINT1("Could not map the register range\n");
        return FALSE;
    }

    Adapter->Doorbells = Adapter->Registers->Doorbells;

    return TRUE;
}


/**
 * @brief Caches CAP and VS and derives the queue and page limits from them.
 */
static
VOID
NvmpReadCapabilities(
    _In_ PNVME_ADAPTER_EXTENSION Adapter)
{
    ULONG HostShift;

    Adapter->Capabilities.AsUlonglong =
        NvmpReadRegister64(Adapter, &Adapter->Registers->CAP.AsUlonglong);
    Adapter->Version.AsUlong =
        StorPortReadRegisterUlong(Adapter, &Adapter->Registers->VS.AsUlong);

    DPRINT1("NVMe %u.%u.%u controller, %lu queue entries\n",
            Adapter->Version.MJR, Adapter->Version.MNR, Adapter->Version.TER,
            (ULONG)Adapter->Capabilities.MQES + 1);

    /* 2^(2 + DSTRD) bytes */
    Adapter->DoorbellStride = 4 << Adapter->Capabilities.DSTRD;

    Adapter->ReadyTimeout = (ULONG)Adapter->Capabilities.TO * NVME_TIMEOUT_UNIT_MS;

    /* Use 4K pages, clamped to what MPSMIN and MPSMAX allow */
    HostShift = NVME_MIN_PAGE_SHIFT;
    if (HostShift > NVME_MIN_PAGE_SHIFT + Adapter->Capabilities.MPSMAX)
        HostShift = NVME_MIN_PAGE_SHIFT + Adapter->Capabilities.MPSMAX;
    if (HostShift < NVME_MIN_PAGE_SHIFT + Adapter->Capabilities.MPSMIN)
        HostShift = NVME_MIN_PAGE_SHIFT + Adapter->Capabilities.MPSMIN;

    Adapter->PageShift = HostShift;
    Adapter->PageSize = 1UL << HostShift;

    /* MQES is 0 based */
    Adapter->AdminQueueDepth = NVME_ADMIN_QUEUE_DEPTH;
    if (Adapter->AdminQueueDepth > (ULONG)Adapter->Capabilities.MQES + 1)
        Adapter->AdminQueueDepth = (ULONG)Adapter->Capabilities.MQES + 1;

    Adapter->IoQueueDepth = NVME_IO_QUEUE_DEPTH;
    if (Adapter->IoQueueDepth > (ULONG)Adapter->Capabilities.MQES + 1)
        Adapter->IoQueueDepth = (ULONG)Adapter->Capabilities.MQES + 1;

    if (Adapter->DumpMode)
    {
        Adapter->AdminQueueDepth = min(Adapter->AdminQueueDepth, NVME_DUMP_QUEUE_DEPTH);
        Adapter->IoQueueDepth = min(Adapter->IoQueueDepth, NVME_DUMP_QUEUE_DEPTH);
    }
}


ULONG
NTAPI
StorNvmeFindAdapter(
    _In_ PVOID DeviceExtension,
    _In_ PVOID HwContext,
    _In_ PVOID BusInformation,
    _In_ PCHAR ArgumentString,
    _Inout_ PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    _In_ PBOOLEAN Again)
{
    PNVME_ADAPTER_EXTENSION Adapter = DeviceExtension;

    UNREFERENCED_PARAMETER(HwContext);
    UNREFERENCED_PARAMETER(BusInformation);
    UNREFERENCED_PARAMETER(ArgumentString);

    DPRINT1("StorNvmeFindAdapter(%p)\n", DeviceExtension);

    *Again = FALSE;

    RtlZeroMemory(Adapter, sizeof(*Adapter));
    Adapter->DumpMode = (ConfigInfo->DumpMode != 0);

    if (!NvmpMapRegisters(Adapter, ConfigInfo))
    {
        Adapter->State = NvmeAdapterFailed;
        return SP_RETURN_ERROR;
    }

    NvmpReadCapabilities(Adapter);

    if (!Adapter->Capabilities.CSS_NVM)
    {
        DPRINT1("Controller does not support the NVM command set\n");
        Adapter->State = NvmeAdapterFailed;
        return SP_RETURN_ERROR;
    }

    /* One target, each namespace is a LUN */
    ConfigInfo->NumberOfBuses = 1;
    ConfigInfo->MaximumNumberOfTargets = 1;

    /* PRP entries must be dword aligned */
    ConfigInfo->AlignmentMask = sizeof(ULONG) - 1;
    ConfigInfo->ScatterGather = TRUE;
    ConfigInfo->Master = TRUE;
    ConfigInfo->CachesData = TRUE;
    ConfigInfo->MapBuffers = STOR_MAP_NON_READ_WRITE_BUFFERS;

    ConfigInfo->SynchronizationModel = StorSynchronizeFullDuplex;

    /* Any reset resets the whole controller */
    ConfigInfo->ResetTargetSupported = FALSE;

    if (ConfigInfo->Dma64BitAddresses == SCSI_DMA64_SYSTEM_SUPPORTED)
        ConfigInfo->Dma64BitAddresses = SCSI_DMA64_MINIPORT_FULL64BIT_SUPPORTED;

    /* Storport reports Latched when MSI or MSI-X was granted */
    if (ConfigInfo->InterruptMode == Latched)
    {
        ConfigInfo->HwMSInterruptRoutine = StorNvmeMessageInterrupt;
        ConfigInfo->InterruptSynchronizationMode = InterruptSynchronizePerMessage;
        Adapter->MessageInterrupts = TRUE;
    }
    else
    {
        ConfigInfo->InterruptSynchronizationMode = InterruptSynchronizeAll;
    }

    /* Started here since the uncached extension can only be allocated in HwFindAdapter */
    if (!NvmpStartController(Adapter, ConfigInfo))
    {
        Adapter->State = NvmeAdapterFailed;
        return SP_RETURN_ERROR;
    }

    ConfigInfo->MaximumNumberOfLogicalUnits = (UCHAR)Adapter->NamespaceCount;
    ConfigInfo->MaximumTransferLength = Adapter->MaximumTransferLength;

    /* One extra page for a buffer that starts unaligned */
    ConfigInfo->NumberOfPhysicalBreaks =
        (Adapter->MaximumTransferLength / Adapter->PageSize) + 1;

    /* A queue is full with one slot still empty */
    ConfigInfo->MaxNumberOfIO = Adapter->IoQueueDepth - 1;
    ConfigInfo->MaxIOsPerLun = ConfigInfo->MaxNumberOfIO;
    ConfigInfo->InitialLunQueueDepth = ConfigInfo->MaxNumberOfIO;

    Adapter->State = NvmeAdapterFound;

    return SP_RETURN_FOUND;
}


BOOLEAN
NTAPI
StorNvmeInitialize(
    _In_ PVOID DeviceExtension)
{
    PNVME_ADAPTER_EXTENSION Adapter = DeviceExtension;

    DPRINT1("StorNvmeInitialize(%p)\n", DeviceExtension);

    /* The controller was already started in HwFindAdapter */
    Adapter->InterruptsConnected = TRUE;
    Adapter->State = NvmeAdapterRunning;

    return TRUE;
}


/**
 * @brief Sets the SRB status and completes the request.
 */
VOID
NvmpCompleteRequest(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PVOID Srb,
    _In_ UCHAR SrbStatus)
{
    SrbSetSrbStatus(Srb, SrbStatus);
    StorPortNotification(RequestComplete, Adapter, Srb);
}


BOOLEAN
NTAPI
StorNvmeBuildIo(
    _In_ PVOID DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PNVME_ADAPTER_EXTENSION Adapter = DeviceExtension;
    PSRBEX_DATA_POWER Power;
    PSRBEX_DATA_PNP Pnp;
    ULONG Function;
    UCHAR Lun;

    Function = SrbGetSrbFunction(Srb);

    switch (Function)
    {
        case SRB_FUNCTION_EXECUTE_SCSI:
            return NvmpBuildScsi(Adapter, Srb);

        case SRB_FUNCTION_FLUSH:
            /* Data reaches the medium through SYNCHRONIZE CACHE */
            NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_SUCCESS);
            return FALSE;

        case SRB_FUNCTION_SHUTDOWN:
            SrbGetPathTargetLun(Srb, NULL, NULL, &Lun);
            if (NvmpNamespaceFromLun(Adapter, Lun) == NULL)
            {
                NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_NO_DEVICE);
                return FALSE;
            }

            /* The dump stack is about to cut power */
            if (Adapter->DumpMode && !NvmpShutdownController(Adapter))
                DPRINT1("Controller did not finish shutting down\n");

            NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_SUCCESS);
            return FALSE;

        case SRB_FUNCTION_POWER:
            /* Only adapter power requests are accepted */
            Power = (PSRBEX_DATA_POWER)SrbGetSrbExDataByType((PSTORAGE_REQUEST_BLOCK)Srb,
                                                             SrbExDataTypePower);
            if (Power != NULL && (Power->SrbPowerFlags & SRB_POWER_FLAGS_ADAPTER_REQUEST))
                NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_SUCCESS);
            else
                NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_INVALID_REQUEST);
            return FALSE;

        case SRB_FUNCTION_PNP:
            /* Adapter stop and removal need no work here, nothing else is supported */
            Pnp = (PSRBEX_DATA_PNP)SrbGetSrbExDataByType((PSTORAGE_REQUEST_BLOCK)Srb,
                                                         SrbExDataTypePnP);
            if (Pnp != NULL && (Pnp->SrbPnPFlags & SRB_PNP_FLAGS_ADAPTER_REQUEST) &&
                (Pnp->PnPAction == StorRemoveDevice ||
                 Pnp->PnPAction == StorStopDevice ||
                 Pnp->PnPAction == StorSurpriseRemoval))
            {
                NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_SUCCESS);
            }
            else
            {
                NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_INVALID_REQUEST);
            }
            return FALSE;

        default:
            DPRINT1("Unsupported request function 0x%02lx\n", Function);
            NvmpCompleteRequest(Adapter, Srb, SRB_STATUS_INVALID_REQUEST);
            return FALSE;
    }
}


BOOLEAN
NTAPI
StorNvmeStartIo(
    _In_ PVOID DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PNVME_ADAPTER_EXTENSION Adapter = DeviceExtension;

    NvmpPostCommand(Adapter, Srb);

    return TRUE;
}


/**
 * @brief Completes every new entry in a completion queue.
 *
 * @return TRUE if at least one entry was found.
 */
static
BOOLEAN
NvmpDrainQueue(
    _In_ PNVME_ADAPTER_EXTENSION Adapter,
    _In_ PNVME_QUEUE_PAIR Queue)
{
    NVME_COMPLETION_ENTRY Completion;
    BOOLEAN Handled = FALSE;

    while (NvmpNextCompletion(Adapter, Queue, &Completion))
    {
        NvmpCompleteFromEntry(Adapter, &Completion);
        Handled = TRUE;
    }

    return Handled;
}


BOOLEAN
NTAPI
StorNvmeInterrupt(
    _In_ PVOID DeviceExtension)
{
    PNVME_ADAPTER_EXTENSION Adapter = DeviceExtension;
    BOOLEAN Handled;

    Handled = NvmpDrainQueue(Adapter, &Adapter->IoQueue);
    Handled |= NvmpDrainAdminQueue(Adapter);

    return Handled;
}


BOOLEAN
NTAPI
StorNvmeMessageInterrupt(
    _In_ PVOID DeviceExtension,
    _In_ ULONG MessageId)
{
    PNVME_ADAPTER_EXTENSION Adapter = DeviceExtension;
    BOOLEAN Handled;

    UNREFERENCED_PARAMETER(MessageId);

    /* TODO: Drain the queue that owns MessageId once there are multiple IO queues */
    Handled = NvmpDrainQueue(Adapter, &Adapter->IoQueue);
    Handled |= NvmpDrainAdminQueue(Adapter);

    return Handled;
}


BOOLEAN
NTAPI
StorNvmeResetBus(
    _In_ PVOID DeviceExtension,
    _In_ ULONG PathId)
{
    PNVME_ADAPTER_EXTENSION Adapter = DeviceExtension;
    BOOLEAN Result;

    UNREFERENCED_PARAMETER(PathId);

    DPRINT1("StorNvmeResetBus(%p)\n", DeviceExtension);

    /* Reinitializing the controller in the middle of a dump does more harm than a timeout */
    if (Adapter->DumpMode)
        return TRUE;

    StorPortPause(Adapter, NVME_ADMIN_TIMEOUT_MS / 1000);
    Result = NvmpResetController(Adapter);
    StorPortResume(Adapter);

    return Result;
}


SCSI_ADAPTER_CONTROL_STATUS
NTAPI
StorNvmeAdapterControl(
    _In_ PVOID DeviceExtension,
    _In_ SCSI_ADAPTER_CONTROL_TYPE ControlType,
    _In_ PVOID Parameters)
{
    PNVME_ADAPTER_EXTENSION Adapter = DeviceExtension;
    PSCSI_SUPPORTED_CONTROL_TYPE_LIST List;

    DPRINT("StorNvmeAdapterControl(%u)\n", ControlType);

    switch (ControlType)
    {
        case ScsiQuerySupportedControlTypes:
        {
            List = Parameters;

            if (List->MaxControlType > ScsiStopAdapter)
                List->SupportedTypeList[ScsiStopAdapter] = TRUE;
            if (List->MaxControlType > ScsiRestartAdapter)
                List->SupportedTypeList[ScsiRestartAdapter] = TRUE;

            return ScsiAdapterControlSuccess;
        }

        case ScsiStopAdapter:
            /* Also used for D3, so the controller gets a shutdown notification first */
            if (!NvmpShutdownController(Adapter))
                DPRINT1("Controller did not finish shutting down\n");

            if (!NvmpDisableController(Adapter))
                DPRINT1("Controller would not go idle on stop\n");

            /* Only once the controller can no longer DMA into them */
            NvmpFailOutstandingRequests(Adapter, SRB_STATUS_BUS_RESET);

            Adapter->State = NvmeAdapterStopped;
            return ScsiAdapterControlSuccess;

        case ScsiRestartAdapter:
            if (!NvmpResetController(Adapter))
                return ScsiAdapterControlUnsuccessful;

            return ScsiAdapterControlSuccess;

        default:
            return ScsiAdapterControlUnsuccessful;
    }
}


SCSI_UNIT_CONTROL_STATUS
NTAPI
StorNvmeUnitControl(
    _In_ PVOID DeviceExtension,
    _In_ SCSI_UNIT_CONTROL_TYPE ControlType,
    _In_ PVOID Parameters)
{
    PSCSI_SUPPORTED_CONTROL_TYPE_LIST List;

    UNREFERENCED_PARAMETER(DeviceExtension);

    DPRINT("StorNvmeUnitControl(%u)\n", ControlType);

    switch (ControlType)
    {
        case ScsiQuerySupportedUnitControlTypes:
        {
            List = Parameters;

            if (List->MaxControlType > ScsiUnitStart)
                List->SupportedTypeList[ScsiUnitStart] = TRUE;
            if (List->MaxControlType > ScsiUnitRemove)
                List->SupportedTypeList[ScsiUnitRemove] = TRUE;

            return ScsiUnitControlSuccess;
        }

        case ScsiUnitStart:
        case ScsiUnitRemove:
            /* FIXME: Track which namespaces are in play */
            return ScsiUnitControlSuccess;

        default:
            return ScsiUnitControlNotSupported;
    }
}


NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    HW_INITIALIZATION_DATA InitData;

    DPRINT1("StorNvme DriverEntry(%p %wZ)\n", DriverObject, RegistryPath);

    RtlZeroMemory(&InitData, sizeof(InitData));

    InitData.HwInitializationDataSize = sizeof(InitData);
    InitData.AdapterInterfaceType = PCIBus;

    InitData.HwFindAdapter = StorNvmeFindAdapter;
    InitData.HwInitialize = StorNvmeInitialize;
    InitData.HwBuildIo = StorNvmeBuildIo;
    InitData.HwStartIo = StorNvmeStartIo;
    InitData.HwInterrupt = StorNvmeInterrupt;
    InitData.HwResetBus = StorNvmeResetBus;
    InitData.HwAdapterControl = StorNvmeAdapterControl;
    InitData.HwUnitControl = StorNvmeUnitControl;

    InitData.DeviceExtensionSize = sizeof(NVME_ADAPTER_EXTENSION);

    /* Extra space so a page aligned PRP list page always fits */
    InitData.SrbExtensionSize = sizeof(NVME_REQUEST_CONTEXT) + (2 * PAGE_SIZE);

    InitData.NumberOfAccessRanges = 1;

    InitData.MapBuffers = STOR_MAP_NON_READ_WRITE_BUFFERS;
    InitData.TaggedQueuing = TRUE;
    InitData.AutoRequestSense = TRUE;
    InitData.MultipleRequestPerLu = TRUE;
    InitData.NeedPhysicalAddresses = TRUE;

    InitData.SrbTypeFlags = SRB_TYPE_FLAG_STORAGE_REQUEST_BLOCK;
    InitData.AddressTypeFlags = ADDRESS_TYPE_FLAG_BTL8;

    InitData.FeatureSupport = STOR_FEATURE_DEVICE_NAME_NO_SUFFIX;

    return StorPortInitialize(DriverObject, RegistryPath, &InitData, NULL);
}
