#include "driver.h"
#include "nhlt.h"

EVT_WDF_DEVICE_PREPARE_HARDWARE Fdo_EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE Fdo_EvtDeviceReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY Fdo_EvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT Fdo_EvtDeviceD0Exit;
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT Fdo_EvtDeviceSelfManagedIoInit;
NTSTATUS
Fdo_Initialize(
    _In_ PFDO_CONTEXT FdoCtx
);

NTSTATUS
Fdo_Create(
	_Inout_ PWDFDEVICE_INIT DeviceInit
)
{
    WDF_CHILD_LIST_CONFIG      config;
	WDF_OBJECT_ATTRIBUTES attributes;
	WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    PFDO_CONTEXT fdoCtx;
    WDFDEVICE wdfDevice;
    NTSTATUS status;

    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "%s\n", __func__);

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = Fdo_EvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = Fdo_EvtDeviceReleaseHardware;
    pnpPowerCallbacks.EvtDeviceD0Entry = Fdo_EvtDeviceD0Entry;
    pnpPowerCallbacks.EvtDeviceD0Exit = Fdo_EvtDeviceD0Exit;
    pnpPowerCallbacks.EvtDeviceSelfManagedIoInit = Fdo_EvtDeviceSelfManagedIoInit;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    WdfDeviceInitSetPowerPageable(DeviceInit);

    //
    // WDF_ DEVICE_LIST_CONFIG describes how the framework should handle
    // dynamic child enumeration on behalf of the driver writer.
    // Since we are a bus driver, we need to specify identification description
    // for our child devices. This description will serve as the identity of our
    // child device. Since the description is opaque to the framework, we
    // have to provide bunch of callbacks to compare, copy, or free
    // any other resources associated with the description.
    //
    WDF_CHILD_LIST_CONFIG_INIT(&config,
        sizeof(PDO_IDENTIFICATION_DESCRIPTION),
        Bus_EvtDeviceListCreatePdo // callback to create a child device.
    );

    //
    // This function pointer will be called when the framework needs to copy a
    // identification description from one location to another.  An implementation
    // of this function is only necessary if the description contains description
    // relative pointer values (like  LIST_ENTRY for instance) .
    // If set to NULL, the framework will use RtlCopyMemory to copy an identification .
    // description. In this sample, it's not required to provide these callbacks.
    // they are added just for illustration.
    //
    config.EvtChildListIdentificationDescriptionDuplicate =
        Bus_EvtChildListIdentificationDescriptionDuplicate;

    //
    // This function pointer will be called when the framework needs to compare
    // two identificaiton descriptions.  If left NULL a call to RtlCompareMemory
    // will be used to compare two identificaiton descriptions.
    //
    config.EvtChildListIdentificationDescriptionCompare =
        Bus_EvtChildListIdentificationDescriptionCompare;
    //
    // This function pointer will be called when the framework needs to free a
    // identification description.  An implementation of this function is only
    // necessary if the description contains dynamically allocated memory
    // (by the driver writer) that needs to be freed. The actual identification
    // description pointer itself will be freed by the framework.
    //
    config.EvtChildListIdentificationDescriptionCleanup =
        Bus_EvtChildListIdentificationDescriptionCleanup;

    //
    // Tell the framework to use the built-in childlist to track the state
    // of the device based on the configuration we just created.
    //
    WdfFdoInitSetDefaultChildListConfig(DeviceInit,
        &config,
        WDF_NO_OBJECT_ATTRIBUTES);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, FDO_CONTEXT);
    status = WdfDeviceCreate(&DeviceInit, &attributes, &wdfDevice);
    if (!NT_SUCCESS(status)) {
        SklHdAudBusPrint(DEBUG_LEVEL_ERROR, DBG_INIT,
            "WdfDriverCreate failed %x\n", status);
        goto Exit;
    }

    {
        WDF_DEVICE_STATE deviceState;
        WDF_DEVICE_STATE_INIT(&deviceState);

        deviceState.NotDisableable = WdfFalse;
        WdfDeviceSetDeviceState(wdfDevice, &deviceState);
    }

    fdoCtx = Fdo_GetContext(wdfDevice);
    fdoCtx->WdfDevice = wdfDevice;

    status = Fdo_Initialize(fdoCtx);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

Exit:
    return status;
}

NTSTATUS
Fdo_Initialize(
    _In_ PFDO_CONTEXT FdoCtx
)
{
    NTSTATUS status;
    WDFDEVICE device;
    WDF_INTERRUPT_CONFIG interruptConfig;

    device = FdoCtx->WdfDevice;

    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "%s\n", __func__);

    //
    // Create an interrupt object for hardware notifications
    //
    WDF_INTERRUPT_CONFIG_INIT(
        &interruptConfig,
        hda_interrupt,
        NULL);

    status = WdfInterruptCreate(
        device,
        &interruptConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &FdoCtx->Interrupt);

    if (!NT_SUCCESS(status))
    {
        SklHdAudBusPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
            "Error creating WDF interrupt object - %!STATUS!",
            status);

        return status;
    }

    WDF_WORKITEM_CONFIG workItemConfig;
    WDF_WORKITEM_CONFIG_INIT(&workItemConfig, hdac_bus_process_unsol_events);

    WDF_OBJECT_ATTRIBUTES objectAttrs;
    WDF_OBJECT_ATTRIBUTES_INIT(&objectAttrs);
    objectAttrs.ParentObject = device;
    status = WdfWorkItemCreate(&workItemConfig, &objectAttrs, &FdoCtx->unsolWork);
    if (!NT_SUCCESS(status)) {
        SklHdAudBusPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
            "Error creating WDF workitem - %!STATUS!", status);
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
Fdo_EvtDevicePrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated
)
{
    BOOLEAN fBar0Found = FALSE;
    BOOLEAN fBar4Found = FALSE;
    NTSTATUS status;
    PFDO_CONTEXT fdoCtx;
    ULONG resourceCount;

    fdoCtx = Fdo_GetContext(Device);
    resourceCount = WdfCmResourceListGetCount(ResourcesTranslated);

    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "%s\n", __func__);

    status = WdfFdoQueryForInterface(Device, &GUID_BUS_INTERFACE_STANDARD, (PINTERFACE)&fdoCtx->BusInterface, sizeof(BUS_INTERFACE_STANDARD), 1, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    for (ULONG i = 0; i < resourceCount; i++)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR pDescriptor;
        UCHAR Class;
        UCHAR Type;

        pDescriptor = WdfCmResourceListGetDescriptor(
            ResourcesTranslated, i);

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
        }
    }

    if (fdoCtx->m_BAR0.Base.Base == NULL) {
        status = STATUS_NOT_FOUND; //BAR0 is required
        return status;
    }

    fdoCtx->BusInterface.GetBusData(fdoCtx->BusInterface.Context, PCI_WHICHSPACE_CONFIG, &fdoCtx->venId, 0, sizeof(UINT16));
    fdoCtx->BusInterface.GetBusData(fdoCtx->BusInterface.Context, PCI_WHICHSPACE_CONFIG, &fdoCtx->devId, 2, sizeof(UINT16));

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
                fdoCtx->mlcap = fdoCtx->m_BAR0.Base.baseptr + offset;
                break;

            case HDA_GTS_CAP_ID:
                SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
                    "Found GTS capability offset=%x\n", offset);
                fdoCtx->gtscap = fdoCtx->m_BAR0.Base.baseptr + offset;
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
                fdoCtx->drsmcap = fdoCtx->m_BAR0.Base.baseptr + offset;
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

    UINT16 gcap = hda_read16(fdoCtx, GCAP);
    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "chipset global capabilities = 0x%x\n", gcap);

    fdoCtx->captureStreams = (gcap >> 8) & 0x0f;
    fdoCtx->playbackStreams = (gcap >> 12) & 0x0f;

    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "streams (cap %d, playback %d)\n", fdoCtx->captureStreams, fdoCtx->playbackStreams);

    fdoCtx->captureIndexOff = 0;
    fdoCtx->playbackIndexOff = fdoCtx->captureStreams;
    fdoCtx->numStreams = fdoCtx->captureStreams + fdoCtx->playbackStreams;

    fdoCtx->streams = (PHDAC_STREAM)ExAllocatePoolZero(NonPagedPool, sizeof(HDAC_STREAM) * fdoCtx->numStreams, SKLHDAUDBUS_POOL_TAG);
    if (!fdoCtx->streams) {
        return STATUS_NO_MEMORY;
    }
    RtlZeroMemory(fdoCtx->streams, sizeof(HDAC_STREAM) * fdoCtx->numStreams);

    PHYSICAL_ADDRESS maxAddr;
    maxAddr.QuadPart = MAXULONG64;

    fdoCtx->posbuf = MmAllocateContiguousMemory(PAGE_SIZE, maxAddr);
    RtlZeroMemory(fdoCtx->posbuf, PAGE_SIZE);
    if (!fdoCtx->posbuf) {
        return STATUS_NO_MEMORY;
    }

    fdoCtx->rb = MmAllocateContiguousMemory(PAGE_SIZE, maxAddr);
    RtlZeroMemory(fdoCtx->rb, PAGE_SIZE);

    if (!fdoCtx->rb) {
        return STATUS_NO_MEMORY;
    }

    //Init Streams
    {
        UINT32 i;
        int streamTags[2] = { 0, 0 };

        for (i = 0; i < fdoCtx->numStreams; i++) {
            int dir = (i >= fdoCtx->captureIndexOff &&
                i < fdoCtx->captureIndexOff + fdoCtx->captureStreams);
            /* stream tag must be unique throughout
             * the stream direction group,
             * valid values 1...15
             * use separate stream tag
             */
            int tag = ++streamTags[dir];

            {
                PHDAC_STREAM stream = &fdoCtx->streams[i];
                stream->FdoContext = fdoCtx;
                /* offset: SDI0=0x80, SDI1=0xa0, ... SDO3=0x160 */
                stream->sdAddr = fdoCtx->m_BAR0.Base.baseptr + (0x20 * i + 0x80);
                /* int mask: SDI0=0x01, SDI1=0x02, ... SDO3=0x80 */
                stream->int_sta_mask = 1 << i;
                stream->idx = i;
                stream->direction = dir;
                if (fdoCtx->venId == VEN_INTEL)
                    stream->streamTag = tag;
                else
                    stream->streamTag = i + 1;
                stream->posbuf = (UINT32 *)(((UINT8 *)fdoCtx->posbuf) + (i * 8));

                if (fdoCtx->ppcap) {
                    stream->pphc_addr = fdoCtx->ppcap + HDA_PPHC_BASE +
                        (HDA_PPHC_INTERVAL * stream->idx);
                    stream->pplc_addr = fdoCtx->ppcap + HDA_PPLC_BASE +
                        (HDA_PPLC_MULTI * fdoCtx->numStreams) +
                        (HDA_PPLC_INTERVAL * stream->idx);
                }

                stream->spib_addr = NULL;
                if (fdoCtx->spbcap) {
                    stream->spib_addr = fdoCtx->spbcap + HDA_SPB_BASE + (HDA_SPB_INTERVAL * stream->idx) + HDA_SPB_SPIB;
                }

                PHYSICAL_ADDRESS maxAddr;
                maxAddr.QuadPart = MAXULONG64;
                stream->bdl = (UINT32 *)MmAllocateContiguousMemory(BDL_SIZE, maxAddr);
            }

            SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
                "Stream tag (idx %d): %d\n", i, tag);
        }
    }

    WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &fdoCtx->cmdLock);

    fdoCtx->nhlt = NULL;
    fdoCtx->nhltSz = 0;
    {
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

    status = STATUS_SUCCESS;

    return status;
}

NTSTATUS
Fdo_EvtDeviceReleaseHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesTranslated
)
{
    PFDO_CONTEXT fdoCtx;

    UNREFERENCED_PARAMETER(ResourcesTranslated);

    fdoCtx = Fdo_GetContext(Device);

    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "%s\n", __func__);

    if (fdoCtx->nhlt)
        MmUnmapIoSpace(fdoCtx->nhlt, fdoCtx->nhltSz);

    if (fdoCtx->unsolWork) {
        WdfWorkItemFlush(fdoCtx->unsolWork);
    }

    if (fdoCtx->posbuf)
        MmFreeContiguousMemory(fdoCtx->posbuf);
    if (fdoCtx->rb)
        MmFreeContiguousMemory(fdoCtx->rb);

    if (fdoCtx->streams) {
        for (int i = 0; i < fdoCtx->numStreams; i++) {
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

    return STATUS_SUCCESS;
}

NTSTATUS
Fdo_EvtDeviceD0Entry(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
)
{
    NTSTATUS status;
    PFDO_CONTEXT fdoCtx;

    fdoCtx = Fdo_GetContext(Device);

    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "%s\n", __func__);

    status = STATUS_SUCCESS;

    if (fdoCtx->venId == VEN_INTEL) {
        UINT32 val;
        pci_read_cfg_dword(&fdoCtx->BusInterface, INTEL_HDA_CGCTL, &val);
        val = val & ~INTEL_HDA_CGCTL_MISCBDCGE;
        pci_write_cfg_dword(&fdoCtx->BusInterface, INTEL_HDA_CGCTL, val);
    }

    hdac_bus_init(fdoCtx);

    if (fdoCtx->venId == VEN_INTEL) {
        UINT32 val;
        pci_read_cfg_dword(&fdoCtx->BusInterface, INTEL_HDA_CGCTL, &val);
        val = val | INTEL_HDA_CGCTL_MISCBDCGE;
        pci_write_cfg_dword(&fdoCtx->BusInterface, INTEL_HDA_CGCTL, val);

        hda_update32(fdoCtx, VS_EM2, HDA_VS_EM2_DUM, HDA_VS_EM2_DUM);
    }

    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "hda bus initialized\n");
    return status;
}

NTSTATUS
Fdo_EvtDeviceD0Exit(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState
)
{
    NTSTATUS status;
    PFDO_CONTEXT fdoCtx;

    fdoCtx = Fdo_GetContext(Device);

    hdac_bus_stop(fdoCtx);

    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "%s\n", __func__);

    status = STATUS_SUCCESS;

    return status;
}

#define ENABLE_HDA 0

NTSTATUS
Fdo_EvtDeviceSelfManagedIoInit(
    _In_ WDFDEVICE Device
)
{
    NTSTATUS status;
    PFDO_CONTEXT fdoCtx;

    fdoCtx = Fdo_GetContext(Device);

    WdfChildListBeginScan(WdfFdoGetDefaultChildList(Device));

    fdoCtx->numCodecs = 0;
#if ENABLE_HDA
    for (int addr = 0; addr < HDA_MAX_CODECS; addr++) {
        fdoCtx->codecs[addr] = NULL;
        if (((fdoCtx->codecMask >> addr) & 0x1) == 0)
            continue;

        DbgPrint("Scan index %d\n", addr);

        UINT32 cmdTmpl = (addr << 28) | (AC_NODE_ROOT << 20) |
            (AC_VERB_PARAMETERS << 8);
        UINT32 funcType = 0, vendorDevice, subsysId, revId, nodeCount;
        if (!NT_SUCCESS(hdac_bus_exec_verb(fdoCtx, addr, cmdTmpl | AC_PAR_VENDOR_ID, &vendorDevice))) {
            continue;
        }
        if (!NT_SUCCESS(hdac_bus_exec_verb(fdoCtx, addr, cmdTmpl | AC_PAR_REV_ID, &revId))) {
            continue;
        }
        if (!NT_SUCCESS(hdac_bus_exec_verb(fdoCtx, addr, cmdTmpl | AC_PAR_NODE_COUNT, &nodeCount))) {
            continue;
        }

        fdoCtx->numCodecs += 1;

        UINT16 startID = (nodeCount >> 16) & 0x7FFF;
        nodeCount = (nodeCount & 0x7FFF);

        UINT16 mainFuncGrp = 0;
        {
            UINT16 nid = startID;
            for (int i = 0; i < nodeCount; i++, nid++) {
                UINT32 cmd = (addr << 28) | (nid << 20) |
                    (AC_VERB_PARAMETERS << 8) | AC_PAR_FUNCTION_TYPE;
                if (!NT_SUCCESS(hdac_bus_exec_verb(fdoCtx, addr, cmd, &funcType))) {
                    continue;
                }
                switch (funcType & 0xFF) {
                case AC_GRP_AUDIO_FUNCTION:
                case AC_GRP_MODEM_FUNCTION:
                    mainFuncGrp = nid;
                    break;
                }
            }
        }

        UINT32 cmd = (addr << 28) | (mainFuncGrp << 20) |
            (AC_VERB_GET_SUBSYSTEM_ID << 8);
        hdac_bus_exec_verb(fdoCtx, addr, cmd, &subsysId);

        DbgPrint("Func 0x%x, vendor: 0x%x, subsys: 0x%x, rev: 0x%x, start group 0x%x, node count %d\n", funcType, vendorDevice, subsysId, revId, startID, nodeCount);

        PDO_IDENTIFICATION_DESCRIPTION description;
        //
        // Initialize the description with the information about the detected codec.
        //
        WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(
            &description.Header,
            sizeof(description)
        );

        description.FdoContext = fdoCtx;

        description.CodecIds.CtlrDevId = fdoCtx->devId;
        description.CodecIds.CtlrVenId = fdoCtx->venId;

        description.CodecIds.CodecAddress = addr;
        description.CodecIds.FunctionGroupStartNode = startID;

        description.CodecIds.IsDSP = FALSE;

        description.CodecIds.FuncId = funcType & 0xFF;
        description.CodecIds.VenId = (vendorDevice >> 16) & 0xFFFF;
        description.CodecIds.DevId = vendorDevice & 0xFFFF;
        description.CodecIds.SubsysId = subsysId;
        description.CodecIds.RevId = (revId >> 8) & 0xFFFF;

        //
        // Call the framework to add this child to the childlist. This call
        // will internaly call our DescriptionCompare callback to check
        // whether this device is a new device or existing device. If
        // it's a new device, the framework will call DescriptionDuplicate to create
        // a copy of this description in nonpaged pool.
        // The actual creation of the child device will happen when the framework
        // receives QUERY_DEVICE_RELATION request from the PNP manager in
        // response to InvalidateDeviceRelations call made as part of adding
        // a new child.
        //
        status = WdfChildListAddOrUpdateChildDescriptionAsPresent(
            WdfFdoGetDefaultChildList(Device), &description.Header,
            NULL); // AddressDescription
    }
#endif

    fdoCtx->dspInterruptCallback = NULL;
    if (fdoCtx->m_BAR4.Base.Base) { //Populate ADSP if present
        PDO_IDENTIFICATION_DESCRIPTION description;
        //
        // Initialize the description with the information about the detected codec.
        //
        WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(
            &description.Header,
            sizeof(description)
        );

        description.FdoContext = fdoCtx;

        description.CodecIds.CtlrDevId = fdoCtx->devId;
        description.CodecIds.CtlrVenId = fdoCtx->venId;

        description.CodecIds.CodecAddress = 0x10000000;
        description.CodecIds.IsDSP = TRUE;

        //
        // Call the framework to add this child to the childlist. This call
        // will internaly call our DescriptionCompare callback to check
        // whether this device is a new device or existing device. If
        // it's a new device, the framework will call DescriptionDuplicate to create
        // a copy of this description in nonpaged pool.
        // The actual creation of the child device will happen when the framework
        // receives QUERY_DEVICE_RELATION request from the PNP manager in
        // response to InvalidateDeviceRelations call made as part of adding
        // a new child.
        //
        status = WdfChildListAddOrUpdateChildDescriptionAsPresent(
            WdfFdoGetDefaultChildList(Device), &description.Header,
            NULL); // AddressDescription
    }

    WdfChildListEndScan(WdfFdoGetDefaultChildList(Device));

    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "hda scan complete\n");
    return status;
}