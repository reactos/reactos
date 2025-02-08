/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/hdaudbus/fdo.cpp
 * PURPOSE:         HDAUDBUS Driver
 * PROGRAMMER:      Coolstar TODO
                    Johannes Anderwald
 */

#include "driver.h"

NTSTATUS
NTAPI
Fdo_EvtDevicePrepareHardware(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PCM_RESOURCE_LIST ResourcesRaw,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated);

NTSTATUS
NTAPI
Fdo_EvtDeviceSelfManagedIoInit(
    _In_ PDEVICE_OBJECT DeviceObject
);

NTSTATUS
NTAPI
Fdo_EvtDeviceD0EntryPostInterrupts(
    _In_ PDEVICE_OBJECT Device);

NTSTATUS
NTAPI
Fdo_EvtDeviceD0Entry(
    _In_ PDEVICE_OBJECT Device);

NTSTATUS
NTAPI
PDO_CreateDevices(PDEVICE_OBJECT DeviceObject);

NTSTATUS
NTAPI
HDA_FDOStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status = STATUS_SUCCESS;
    PFDO_CONTEXT DeviceExtension;

    /* get device extension */
    DeviceExtension = (PFDO_CONTEXT)DeviceObject->DeviceExtension;
    ASSERT(DeviceExtension->IsFDO == TRUE);

    /* forward irp to lower device */
    if (!IoForwardIrpSynchronously(DeviceExtension->LowerDevice, Irp))
    {
        ASSERT(FALSE);
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    Status = Irp->IoStatus.Status;
    if (!NT_SUCCESS(Status))
    {
        // failed to start
        DPRINT1("HDA_StartDevice Lower device failed to start %x\n", Status);
        return Status;
    }
    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    Status = Fdo_EvtDevicePrepareHardware(DeviceObject,
        IoStack->Parameters.StartDevice.AllocatedResources,
        IoStack->Parameters.StartDevice.AllocatedResourcesTranslated);
    if (!NT_SUCCESS(Status))
    {
        // failed to start
        DPRINT1("HDA_StartDevice Fdo_EvtDevicePrepareHardware failed to start %x\n", Status);
        return Status;
    }

    Status = Fdo_EvtDeviceD0Entry(DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        // failed to start
        DPRINT1("HDA_StartDevice Fdo_EvtDeviceD0Entry failed to start %x\n", Status);
        return Status;
    }

    Status = Fdo_EvtDeviceD0EntryPostInterrupts(DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        // failed to start
        DPRINT1("HDA_StartDevice Fdo_EvtDeviceD0EntryPostInterrupts failed to start %x\n", Status);
        return Status;
    }

    Status = Fdo_EvtDeviceSelfManagedIoInit(DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        // failed to start
        DPRINT1("HDA_StartDevice Fdo_EvtDeviceSelfManagedIoInit failed to start %x\n", Status);
        return Status;
    }

    Status = PDO_CreateDevices(DeviceObject);
    DPRINT1("HDA_FDOStartDevice completed Status %x\n", Status);
    return Status;
}

NTSTATUS
NTAPI
HDA_FDOQueryInterface(IN PDEVICE_OBJECT FdoDevice)
{
    PFDO_CONTEXT FdoExtension;
    PBUS_INTERFACE_STANDARD BusInterface;
    PIO_STACK_LOCATION IoStack;
    IO_STATUS_BLOCK IoStatusBlock;
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;

    FdoExtension = (PFDO_CONTEXT)FdoDevice->DeviceExtension;
    BusInterface = &FdoExtension->BusInterface;

    RtlZeroMemory(BusInterface, sizeof(BUS_INTERFACE_STANDARD));
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       FdoExtension->LowerDevice,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatusBlock);

    if (Irp)
    {
        IoStack = IoGetNextIrpStackLocation(Irp);

        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        Irp->IoStatus.Information = 0;

        IoStack->MinorFunction = IRP_MN_QUERY_INTERFACE;

        IoStack->Parameters.QueryInterface.InterfaceType = &GUID_BUS_INTERFACE_STANDARD;
        IoStack->Parameters.QueryInterface.Size = sizeof(BUS_INTERFACE_STANDARD);
        IoStack->Parameters.QueryInterface.Version = 1;
        IoStack->Parameters.QueryInterface.Interface = (PINTERFACE)BusInterface;
        IoStack->Parameters.QueryInterface.InterfaceSpecificData = 0;

        Status = IoCallDriver(FdoExtension->LowerDevice, Irp);

        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatusBlock.Status;
        }
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    DPRINT("HDA_FDOQueryInterface Status: %x\n", Status);

    return Status;
}


NTSTATUS
NTAPI
Fdo_EvtDevicePrepareHardware(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PCM_RESOURCE_LIST ResourcesRaw,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated)
{
    BOOLEAN fBar0Found = FALSE;
    BOOLEAN fBar4Found = FALSE;
    NTSTATUS status;
    PFDO_CONTEXT fdoCtx;
    ULONG resourceCount;
    PCI_COMMON_CONFIG PciConfig;


    fdoCtx = Fdo_GetContext(DeviceObject);
    resourceCount = ResourcesTranslated->List[0].PartialResourceList.Count;

    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "%s\n", __func__);

    status = HDA_FDOQueryInterface(DeviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    for (ULONG i = 0; i < resourceCount; i++)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR pDescriptor;

        pDescriptor = &ResourcesTranslated->List[0].PartialResourceList.PartialDescriptors[i];

        switch (pDescriptor->Type)
        {
        case CmResourceTypeMemory:
            //Look for BAR0 and BAR4
            if (fBar0Found == FALSE) {
                SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
                    "Found BAR0: 0x%llx (size 0x%lx)\n", pDescriptor->u.Memory.Start.QuadPart, pDescriptor->u.Memory.Length);

                fdoCtx->m_BAR0.Base.Base = MmMapIoSpace(pDescriptor->u.Memory.Start, pDescriptor->u.Memory.Length, MmNonCached);
                fdoCtx->m_BAR0.Len = pDescriptor->u.Memory.Length;

                SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
                    "Mapped to %p\n", fdoCtx->m_BAR0.Base.baseptr);
                fBar0Found = TRUE;
            }
            else if (fBar4Found == FALSE) {
                SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
                    "Found BAR4: 0x%llx (size 0x%lx)\n", pDescriptor->u.Memory.Start.QuadPart, pDescriptor->u.Memory.Length);

                fdoCtx->m_BAR4.Base.Base = MmMapIoSpace(pDescriptor->u.Memory.Start, pDescriptor->u.Memory.Length, MmNonCached);
                fdoCtx->m_BAR4.Len = pDescriptor->u.Memory.Length;

                SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
                    "Mapped to %p\n", fdoCtx->m_BAR4.Base.baseptr);
                fBar4Found = TRUE;
            }
            break;
        case CmResourceTypeInterrupt:
            
            KeInitializeSpinLock(&fdoCtx->InterruptSpinLock);
            status = IoConnectInterrupt(&fdoCtx->Interrupt,
                hda_interrupt,
                DeviceObject,
                &fdoCtx->InterruptSpinLock,
                pDescriptor->u.Interrupt.Vector,
                pDescriptor->u.Interrupt.Level,
                pDescriptor->u.Interrupt.Level,
                (KINTERRUPT_MODE)(pDescriptor->Flags & CM_RESOURCE_INTERRUPT_LATCHED),
                (pDescriptor->ShareDisposition != CmResourceShareDeviceExclusive),
                pDescriptor->u.Interrupt.Affinity,
                FALSE);
            if (!NT_SUCCESS(status))
            {
                DPRINT1("[HDAUDBUS] Failed to connect interrupt. Status=%lx\n", status);
                break;
            }
        }
    }
    if (fdoCtx->m_BAR0.Base.Base == NULL) {
        status = STATUS_NOT_FOUND; //BAR0 is required
        return status;
    }

    fdoCtx->BusInterface.GetBusData(fdoCtx->BusInterface.Context, PCI_WHICHSPACE_CONFIG, &fdoCtx->venId, 0, sizeof(UINT16));
    fdoCtx->BusInterface.GetBusData(fdoCtx->BusInterface.Context, PCI_WHICHSPACE_CONFIG, &fdoCtx->devId, 2, sizeof(UINT16));
    fdoCtx->BusInterface.GetBusData(fdoCtx->BusInterface.Context, PCI_WHICHSPACE_CONFIG, &fdoCtx->revId, 1, sizeof(UINT8));
    fdoCtx->BusInterface.GetBusData(
        fdoCtx->BusInterface.Context, PCI_WHICHSPACE_CONFIG, &PciConfig, 0, PCI_COMMON_HDR_LENGTH);
    fdoCtx->subsysId = PciConfig.u.type0.SubSystemID;
    fdoCtx->subvendorId = PciConfig.u.type0.SubVendorID;
    #if 0
    //mlcap & lctl (hda_intel_init_chip)
    if (fdoCtx->venId == VEN_INTEL) {
        //read bus capabilities

        unsigned int cur_cap;
        unsigned int offset;
        unsigned int counter = 0;

        offset = hda_read16(fdoCtx, LLCH);

#define HDAC_MAX_CAPS 10

        /* Lets walk the linked capabilities list */
        do {
            cur_cap = read32(fdoCtx->m_BAR0.Base.baseptr + offset);

            SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
                "Capability version: 0x%x\n",
                (cur_cap & HDA_CAP_HDR_VER_MASK) >> HDA_CAP_HDR_VER_OFF);

            SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
                "HDA capability ID: 0x%x\n",
                (cur_cap & HDA_CAP_HDR_ID_MASK) >> HDA_CAP_HDR_ID_OFF);

            if (cur_cap == -1) {
                SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
                    "Invalid capability reg read\n");
                break;
            }

            switch ((cur_cap & HDA_CAP_HDR_ID_MASK) >> HDA_CAP_HDR_ID_OFF) {
            case HDA_ML_CAP_ID:
                SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
                    "Found ML capability\n");
                break;

            case HDA_GTS_CAP_ID:
                SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
                    "Found GTS capability offset=%x\n", offset);
                break;

            case HDA_PP_CAP_ID:
                /* PP capability found, the Audio DSP is present */
                SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
                    "Found PP capability offset=%x\n", offset);
                fdoCtx->ppcap = fdoCtx->m_BAR0.Base.baseptr + offset;
                break;

            case HDA_SPB_CAP_ID:
                /* SPIB capability found, handler function */
                SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
                    "Found SPB capability\n");
                fdoCtx->spbcap = fdoCtx->m_BAR0.Base.baseptr + offset;
                break;

            case HDA_DRSM_CAP_ID:
                /* DMA resume  capability found, handler function */
                SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
                    "Found DRSM capability\n");
                break;

            default:
                SklHdAudBusPrint(DEBUG_LEVEL_ERROR, DBG_INIT, "Unknown capability %d\n", cur_cap);
                cur_cap = 0;
                break;
            }

            counter++;

            if (counter > HDAC_MAX_CAPS) {
                SklHdAudBusPrint(DEBUG_LEVEL_ERROR, DBG_INIT, "We exceeded HDAC capabilities!!!\n");
                break;
            }

            /* read the offset of next capability */
            offset = cur_cap & HDA_CAP_HDR_NXT_PTR_MASK;

        } while (offset);
    }
    #endif
    status = GetHDACapabilities(fdoCtx);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    fdoCtx->streams = (PHDAC_STREAM)ExAllocatePoolZero(NonPagedPool, sizeof(HDAC_STREAM) * fdoCtx->numStreams, SKLHDAUDBUS_POOL_TAG);
    if (!fdoCtx->streams) {
        return STATUS_NO_MEMORY;
    }
    RtlZeroMemory(fdoCtx->streams, sizeof(HDAC_STREAM) * fdoCtx->numStreams);

    PHYSICAL_ADDRESS maxAddr;
    maxAddr.QuadPart = fdoCtx->is64BitOK ? MAXULONG64 : MAXULONG32;

    fdoCtx->posbuf = MmAllocateContiguousMemory(PAGE_SIZE, maxAddr);
    RtlZeroMemory(fdoCtx->posbuf, PAGE_SIZE);
    if (!fdoCtx->posbuf) {
        return STATUS_NO_MEMORY;
    }

    fdoCtx->rb = (UINT8 *)MmAllocateContiguousMemory(PAGE_SIZE, maxAddr);
    if (!fdoCtx->rb) {
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(fdoCtx->rb, PAGE_SIZE);

    //Init Streams
    {
        UINT8 i;
        UINT8 streamTags[2] = { 0, 0 };

        for (i = 0; i < fdoCtx->numStreams; i++) {
            int isCapture = (i >= fdoCtx->captureIndexOff &&
                i < fdoCtx->captureIndexOff + fdoCtx->captureStreams);
            /* stream tag must be unique throughout
             * the stream direction group,
             * valid values 1...15
             * use separate stream tag
             */
            UINT8 tag = ++streamTags[isCapture];

            {
                UINT64 idx = i;

                PHDAC_STREAM stream = &fdoCtx->streams[i];
                stream->FdoContext = fdoCtx;
                /* offset: SDI0=0x80, SDI1=0xa0, ... SDO3=0x160 */
                stream->sdAddr = fdoCtx->m_BAR0.Base.baseptr + (0x20 * idx + 0x80);
                /* int mask: SDI0=0x01, SDI1=0x02, ... SDO3=0x80 */
                stream->int_sta_mask = 1 << i;
                stream->idx = i;
                if (fdoCtx->venId == VEN_INTEL)
                    stream->streamTag = tag;
                else
                    stream->streamTag = i + 1;

                stream->posbuf = (UINT32 *)(((UINT8 *)fdoCtx->posbuf) + (idx * 8));

                stream->spib_addr = NULL;
                if (fdoCtx->spbcap) {
                    stream->spib_addr = fdoCtx->spbcap + HDA_SPB_BASE + (HDA_SPB_INTERVAL * idx) + HDA_SPB_SPIB;
                }

                stream->bdl = (PHDAC_BDLENTRY)MmAllocateContiguousMemory(BDL_SIZE, maxAddr);
            }

            SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
                "Stream tag (idx %d): %d\n", i, tag);
        }
    }

    fdoCtx->nhlt = NULL;
    fdoCtx->nhltSz = 0;
#if 0
    { //Check NHLT for Intel SST
        NTSTATUS status2 = NHLTCheckSupported(Device);
        if (NT_SUCCESS(status2)) {
            UINT64 nhltAddr;
            UINT64 nhltSz;

            status2 = NHLTQueryTableAddress(Device, &nhltAddr, &nhltSz);

            if (NT_SUCCESS(status2)) {
                PHYSICAL_ADDRESS nhltBaseAddr;
                nhltBaseAddr.QuadPart = nhltAddr;

                if (nhltAddr != 0 && nhltSz != 0) {
                    fdoCtx->nhlt = MmMapIoSpace(nhltBaseAddr, nhltSz, MmCached);
                    fdoCtx->nhltSz = nhltSz;
                }
            }
        }
    }
#endif
    fdoCtx->sofTplg = NULL;
    fdoCtx->sofTplgSz = 0;
#if 0
     { //Check topology for Intel SOF
        SOF_TPLG sofTplg = { 0 };
        NTSTATUS status2 = GetSOFTplg(Device, &sofTplg);
        if (NT_SUCCESS(status2) && sofTplg.magic == SOFTPLG_MAGIC) {
            fdoCtx->sofTplg = ExAllocatePoolWithTag(NonPagedPool, sofTplg.length, SKLHDAUDBUS_POOL_TAG);
            RtlCopyMemory(fdoCtx->sofTplg, &sofTplg, sofTplg.length);
            fdoCtx->sofTplgSz = sofTplg.length;
        }
    }
#endif
    status = STATUS_SUCCESS;

    return status;
}

NTSTATUS
NTAPI
Fdo_EvtDeviceReleaseHardware(
    _In_ PDEVICE_OBJECT Device
)
{
    PFDO_CONTEXT fdoCtx;

    fdoCtx = Fdo_GetContext(Device);

    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "%s\n", __func__);

    if (fdoCtx->nhlt)
        MmUnmapIoSpace(fdoCtx->nhlt, fdoCtx->nhltSz);

    if (fdoCtx->sofTplg)
        ExFreePoolWithTag(fdoCtx->sofTplg, SKLHDAUDBUS_POOL_TAG);

    if (fdoCtx->posbuf)
        MmFreeContiguousMemory(fdoCtx->posbuf);
    if (fdoCtx->rb)
        MmFreeContiguousMemory(fdoCtx->rb);

    if (fdoCtx->streams) {
        for (UINT32 i = 0; i < fdoCtx->numStreams; i++) {
            PHDAC_STREAM stream = &fdoCtx->streams[i];
            if (stream->bdl) {
                MmFreeContiguousMemory(stream->bdl);
                stream->bdl = NULL;
            }
        }

        ExFreePoolWithTag(fdoCtx->streams, SKLHDAUDBUS_POOL_TAG);
    }

    if (fdoCtx->m_BAR0.Base.Base)
        MmUnmapIoSpace(fdoCtx->m_BAR0.Base.Base, fdoCtx->m_BAR0.Len);
    if (fdoCtx->m_BAR4.Base.Base)
        MmUnmapIoSpace(fdoCtx->m_BAR4.Base.Base, fdoCtx->m_BAR4.Len);
    IoDisconnectInterrupt(fdoCtx->Interrupt);
    fdoCtx->Interrupt = NULL;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HDA_FDORemoveDevice(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    PFDO_CONTEXT DeviceExtension;

    /* get device extension */
    DeviceExtension = static_cast<PFDO_CONTEXT>(DeviceObject->DeviceExtension);
    ASSERT(DeviceExtension->IsFDO == TRUE);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoSkipCurrentIrpStackLocation(Irp);
    Status = IoCallDriver(DeviceExtension->LowerDevice, Irp);

    IoDetachDevice(DeviceExtension->LowerDevice);

    Status = Fdo_EvtDeviceReleaseHardware(DeviceObject);
    IoDeleteDevice(DeviceObject);

    return Status;
}

NTSTATUS
NTAPI
HDA_FDOQueryBusRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    ULONG DeviceCount, CodecIndex;
    PFDO_CONTEXT DeviceExtension;
    PDEVICE_RELATIONS DeviceRelations;

    /* get device extension */
    DeviceExtension = (PFDO_CONTEXT)DeviceObject->DeviceExtension;
    ASSERT(DeviceExtension->IsFDO == TRUE);

    DeviceCount = 0;
    for (CodecIndex = 0; CodecIndex < HDA_MAX_CODECS; CodecIndex++)
    {
        if (DeviceExtension->codecs[CodecIndex] == NULL)
            continue;

        if (DeviceExtension->codecs[CodecIndex]->ChildPDO != NULL)
            DeviceCount += 1;
    }

    if (DeviceCount == 0)
    {
        ASSERT(0);
        return STATUS_UNSUCCESSFUL;
    }
        
    DeviceRelations = (PDEVICE_RELATIONS)AllocateItem(NonPagedPool, sizeof(DEVICE_RELATIONS) + (DeviceCount > 1 ? sizeof(PDEVICE_OBJECT) * (DeviceCount - 1) : 0));
    if (!DeviceRelations)
        return STATUS_INSUFFICIENT_RESOURCES;

    DeviceRelations->Count = 0;
    for (CodecIndex = 0; CodecIndex < HDA_MAX_CODECS; CodecIndex++)
    {
        if (DeviceExtension->codecs[CodecIndex] == NULL)
            continue;

        ObReferenceObject(DeviceExtension->codecs[CodecIndex]->ChildPDO);
        DeviceRelations->Objects[DeviceRelations->Count] = DeviceExtension->codecs[CodecIndex]->ChildPDO;
        DeviceRelations->Count++;
    }

    /* FIXME handle existing device relations */
    ASSERT(Irp->IoStatus.Information == 0);

    /* store device relations */
    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;

    /* done */
    return STATUS_SUCCESS;
}



#define ENABLE_HDA 1

NTSTATUS
NTAPI
Fdo_EvtDeviceD0Entry(
    _In_ PDEVICE_OBJECT Device)
{
    NTSTATUS status;
    PFDO_CONTEXT fdoCtx;

    fdoCtx = Fdo_GetContext(Device);

    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "%s\n", __func__);

    status = STATUS_SUCCESS;
 #if 0
    if (fdoCtx->venId == VEN_INTEL) {
        UINT32 val;
        pci_read_cfg_dword(&fdoCtx->BusInterface, INTEL_HDA_CGCTL, &val);
        val = val & ~INTEL_HDA_CGCTL_MISCBDCGE;
        pci_write_cfg_dword(&fdoCtx->BusInterface, INTEL_HDA_CGCTL, val);
    }
 #endif
    //Reset unsolicited queue
    RtlZeroMemory(&fdoCtx->unsol_queue, sizeof(fdoCtx->unsol_queue));
    fdoCtx->unsol_rp = 0;
    fdoCtx->unsol_wp = 0;
    fdoCtx->processUnsol = FALSE;

    //Reset CORB / RIRB
    RtlZeroMemory(&fdoCtx->corb, sizeof(fdoCtx->corb));
    RtlZeroMemory(&fdoCtx->rirb, sizeof(fdoCtx->rirb));

    status = StartHDAController(fdoCtx);

    #if 0
    if (fdoCtx->venId == VEN_INTEL) {
        UINT32 val;
        pci_read_cfg_dword(&fdoCtx->BusInterface, INTEL_HDA_CGCTL, &val);
        val = val | INTEL_HDA_CGCTL_MISCBDCGE;
        pci_write_cfg_dword(&fdoCtx->BusInterface, INTEL_HDA_CGCTL, val);

        hda_update32(fdoCtx, VS_EM2, HDA_VS_EM2_DUM, HDA_VS_EM2_DUM);
    }
    #endif

    if (!NT_SUCCESS(status)) {
        return status;
    }

    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "hda bus initialized\n");

    return status;
}

NTSTATUS
NTAPI
Fdo_EvtDeviceD0EntryPostInterrupts(
    _In_ PDEVICE_OBJECT Device)
{
    NTSTATUS status;
    PFDO_CONTEXT fdoCtx;

    status = STATUS_SUCCESS;
    fdoCtx = Fdo_GetContext(Device);

    for (UINT8 addr = 0; addr < HDA_MAX_CODECS; addr++) {

        KeInitializeEvent(&fdoCtx->rirb.xferEvent[addr], NotificationEvent, FALSE);

        if (((fdoCtx->codecMask >> addr) & 0x1) == 0)
            continue;

        ULONG cmdTmpl = 0;
        cmdTmpl = (addr << 28) | (AC_NODE_ROOT << 20) | (AC_VERB_PARAMETERS << 8);

        ULONG vendorDevice;
        if (!NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_VENDOR_ID, &vendorDevice))) { //Some codecs might need a kickstart
            //First attempt failed. Retry
            status = RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_VENDOR_ID, &vendorDevice); //If this fails, something is wrong.
            DPRINT1("RunSingleHDACmd Status %x addr %x cmdTmpl %x\n", status, addr, cmdTmpl);
        }
    }
    DPRINT1("Fdo_EvtDeviceD0EntryPostInterrupts done %x\n", status);
    return status;
}

NTSTATUS
NTAPI
Fdo_EvtDeviceD0Exit(
    _In_ PDEVICE_OBJECT Device
)
{
    NTSTATUS status;
    PFDO_CONTEXT fdoCtx;

    fdoCtx = Fdo_GetContext(Device);

    status = StopHDAController(fdoCtx);

    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "%s\n", __func__);

    return status;
}

NTSTATUS
NTAPI
Fdo_EvtDeviceSelfManagedIoInit(
    _In_ PDEVICE_OBJECT DeviceObject
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PFDO_CONTEXT fdoCtx;

    fdoCtx = Fdo_GetContext(DeviceObject);

    fdoCtx->numCodecs = 0;
    for (UINT8 addr = 0; addr < HDA_MAX_CODECS; addr++) {
        fdoCtx->codecs[addr] = NULL;
        if (((fdoCtx->codecMask >> addr) & 0x1) == 0)
            continue;

        UINT32 cmdTmpl = (addr << 28) | (AC_NODE_ROOT << 20) |
            (AC_VERB_PARAMETERS << 8);
        ULONG funcType = 0, vendorDevice, subsysId, revId, nodeCount;
        if (!NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_VENDOR_ID, &vendorDevice))) {
            continue;
        }
        if (!NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_REV_ID, &revId))) {
            continue;
        }
        if (!NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_NODE_COUNT, &nodeCount))) {
            continue;
        }

        
        UINT8 startID = (nodeCount >> 16) & 0xFF;
        nodeCount = (nodeCount & 0x7FFF);
        DPRINT1("StartId %u NodeCount %u\n", startID, nodeCount);
        UINT16 mainFuncGrp = 0;
        UINT16 nid = startID;
        {
            for (UINT32 i = 0; i < nodeCount; i++, nid++) {
                UINT32 cmd = (addr << 28) | (nid << 20) |
                    (AC_VERB_PARAMETERS << 8) | AC_PAR_FUNCTION_TYPE;
                if (!NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmd, &funcType))) {
                    continue;
                }
                switch (funcType & 0xFF) {
                case AC_GRP_AUDIO_FUNCTION:
                case AC_GRP_MODEM_FUNCTION:
                    mainFuncGrp = funcType;
                    DPRINT1("nid %x\n", nid);
                    break;
                }
            }
        }

        UINT32 cmd = (addr << 28) | (nid << 20) | (AC_VERB_GET_SUBSYSTEM_ID << 8);
        NTSTATUS Status = RunSingleHDACmd(fdoCtx, cmd, &subsysId);
        DPRINT1("subsystemId %x status %x\n", subsysId, Status);
        if (subsysId == 0)
        {
            subsysId = (fdoCtx->subvendorId << 16) | fdoCtx->subsysId;
            DPRINT1("Using pci subsysid %x\n", subsysId);
        }

        fdoCtx->CodecIds[fdoCtx->numCodecs].CtlrDevId = fdoCtx->devId;
        fdoCtx->CodecIds[fdoCtx->numCodecs].CtlrVenId = fdoCtx->venId;

        fdoCtx->CodecIds[fdoCtx->numCodecs].CodecAddress = addr;
        fdoCtx->CodecIds[fdoCtx->numCodecs].FunctionGroupStartNode = startID;

        fdoCtx->CodecIds[fdoCtx->numCodecs].IsDSP = mainFuncGrp & AC_GRP_AUDIO_FUNCTION;

        fdoCtx->CodecIds[fdoCtx->numCodecs].FuncId = funcType & 0xFF;
        fdoCtx->CodecIds[fdoCtx->numCodecs].VenId = (vendorDevice >> 16) & 0xFFFF;
        fdoCtx->CodecIds[fdoCtx->numCodecs].DevId = vendorDevice & 0xFFFF;
        fdoCtx->CodecIds[fdoCtx->numCodecs].SubsysId = subsysId;
        fdoCtx->CodecIds[fdoCtx->numCodecs].RevId = (revId >> 8) & 0xFFFF;

        if (fdoCtx->CodecIds[fdoCtx->numCodecs].IsDSP)
        {
            UINT32 resetCmd = (addr << 28) | (AC_NODE_ROOT << 20) | (AC_VERB_SET_CODEC_RESET << 8);
            ULONG resetResponse = 0;
            if (NT_SUCCESS(RunSingleHDACmd(fdoCtx, resetCmd, &nodeCount)))
            {
                DPRINT1("resetResponse %x\n", resetResponse);
            }

            UINT32 cmdTmpl = (addr << 28) | (startID << 20) | (AC_VERB_PARAMETERS << 8);
            ULONG nodeCount = 0;
            if (NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_NODE_COUNT, &nodeCount)))
            {
                ULONG startingNode = (nodeCount >> 16) & 0xFF;
                ULONG totalWidgets = nodeCount & 0xF;
                DPRINT1("TotalWidgets %u StartingNode %x\n", totalWidgets, startingNode);
                for (ULONG widget = 0; widget < totalWidgets; widget++)
                {
                    UINT32 node = startingNode + widget;
                    UINT32 cmdTmpl = (addr << 28) | (node << 20) | (AC_VERB_PARAMETERS << 8);
                    ULONG widgetCaps = 0;
                    if (NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_AUDIO_WIDGET_CAP, &widgetCaps)))
                    {
                    DPRINT1(
                        "widgetCaps %x NodeType %x PowerCntrl %x Digital %x Stripe %x\n", widgetCaps,
                        (widgetCaps >> 20) & 0xF, (widgetCaps >> 10) & 0x1, (widgetCaps >> 9) & 0x1,
                        (widgetCaps >> 5) & 0x1);
                        ULONG PowerCntrl = (widgetCaps >> 10) & 0x1;
                        if (PowerCntrl)
                        {
                            ULONG PowerState = 0;
                            if (NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_VERB_GET_POWER_STATE, &widgetCaps)))
                            {
                                DPRINT1("PowerState %x\n", PowerState);
                            }
                            ULONG SetPowerState = 0;
                            if (NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_VERB_SET_POWER_STATE, &widgetCaps)))
                            {
                                DPRINT1("Set PowerState %x\n", SetPowerState);
                            }
                        }
                    }
                    ULONG supportedPCM = 0;
                    if (NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_PCM, &supportedPCM)))
                    {
                        DPRINT1("supportedPCM %x\n", supportedPCM);
                    }
                    ULONG supportedFormats = 0;
                    if (NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_STREAM, &supportedPCM)))
                    {
                        DPRINT1("supportedFormats %x\n", supportedFormats);
                    }
                    ULONG amplifierInput = 0;
                    if (NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_AMP_IN_CAP, &amplifierInput)))
                    {
                        DPRINT1("amplifierInput %x\n", amplifierInput);
                    }
                    ULONG amplifierOutput = 0;
                    if (NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_AMP_OUT_CAP, &amplifierOutput)))
                    {
                        DPRINT1("amplifierOutput %x\n", amplifierOutput);
                    }
                    DPRINT1("-------------------------------------------\n");
                }


            }
            ULONG nodeCaps = 0;
            if (NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_AUDIO_FG_CAP, &nodeCaps)))
            {
                DPRINT1("nodeCaps %x\n", nodeCaps);
            }
            ULONG widgetCaps = 0;
            if (NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_AUDIO_WIDGET_CAP, &widgetCaps)))
            {
                DPRINT1(
                    "widgetCaps %x NodeType %x PowerCntrl %x Digital %x Stripe %x\n", widgetCaps,
                    (widgetCaps >> 20) & 0xF, (widgetCaps >> 10) & 0x1, (widgetCaps >> 9) & 0x1,
                    (widgetCaps >> 5) & 0x1);
            }
            ULONG supportedPCM = 0;
            if (NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_PCM, &supportedPCM)))
            {
                DPRINT1("supportedPCM %x\n", supportedPCM);
            }
            ULONG supportedFormats = 0;
            if (NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_STREAM, &supportedPCM)))
            {
                DPRINT1("supportedFormats %x\n", supportedFormats);
            }
            ULONG amplifierInput = 0;
            if (NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_AMP_IN_CAP, &amplifierInput)))
            {
                DPRINT1("amplifierInput %x\n", amplifierInput);
            }
            ULONG amplifierOutput = 0;
            if (NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_AMP_OUT_CAP, &amplifierOutput)))
            {
                DPRINT1("amplifierOutput %x\n", amplifierOutput);
            }
        }


        fdoCtx->numCodecs += 1;
    }

    fdoCtx->dspInterruptCallback = NULL;
    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "hda scan complete\n");
    return status;
}
