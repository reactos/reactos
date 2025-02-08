#include "driver.h"
#include "nhlt.h"
#include "sof-tplg.h"

EVT_WDF_DEVICE_PREPARE_HARDWARE Fdo_EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE Fdo_EvtDeviceReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY Fdo_EvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_ENTRY_POST_INTERRUPTS_ENABLED Fdo_EvtDeviceD0EntryPostInterrupts;
EVT_WDF_DEVICE_D0_EXIT Fdo_EvtDeviceD0Exit;
EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT Fdo_EvtDeviceSelfManagedIoInit;

void CheckHDAGraphicsRegistryKeys(PFDO_CONTEXT fdoCtx);

NTSTATUS
HDAGraphicsPowerInterfaceCallback(
    PVOID NotificationStruct,
    PVOID Context
);

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
    pnpPowerCallbacks.EvtDeviceD0EntryPostInterruptsEnabled = Fdo_EvtDeviceD0EntryPostInterrupts;
    pnpPowerCallbacks.EvtDeviceD0Exit = Fdo_EvtDeviceD0Exit;
    pnpPowerCallbacks.EvtDeviceSelfManagedIoInit = Fdo_EvtDeviceSelfManagedIoInit;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

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

    /*{
        WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS IdleSettings;

        WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&IdleSettings, IdleCannotWakeFromS0);
        IdleSettings.IdleTimeoutType = SystemManagedIdleTimeoutWithHint;
        IdleSettings.IdleTimeout = 1000;
        IdleSettings.UserControlOfIdleSettings = IdleDoNotAllowUserControl;

        WdfDeviceAssignS0IdleSettings(wdfDevice, &IdleSettings);
    }*/

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

    CheckHDAGraphicsRegistryKeys(fdoCtx);

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
        hda_dpc);

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
    

    status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &FdoCtx->GraphicsDevicesCollectionWaitLock);
    if (!NT_SUCCESS(status))
    {
        SklHdAudBusPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
            "Error creating WDF wait lock - %!STATUS!",
            status);

        return status;
    }
    
    status = WdfCollectionCreate(WDF_NO_OBJECT_ATTRIBUTES, &FdoCtx->GraphicsDevicesCollection);

    if (!NT_SUCCESS(status))
    {
        SklHdAudBusPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
            "Error creating WDF collection - %!STATUS!",
            status);
        
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
    UNREFERENCED_PARAMETER(ResourcesRaw);

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
    fdoCtx->BusInterface.GetBusData(fdoCtx->BusInterface.Context, PCI_WHICHSPACE_CONFIG, &fdoCtx->revId, 1, sizeof(UINT8));

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

    fdoCtx->sofTplg = NULL;
    fdoCtx->sofTplgSz = 0;

    { //Check topology for Intel SOF
        SOF_TPLG sofTplg = { 0 };
        NTSTATUS status2 = GetSOFTplg(Device, &sofTplg);
        if (NT_SUCCESS(status2) && sofTplg.magic == SOFTPLG_MAGIC) {
            fdoCtx->sofTplg = ExAllocatePoolZero(NonPagedPool, sofTplg.length, SKLHDAUDBUS_POOL_TAG);
            RtlCopyMemory(fdoCtx->sofTplg, &sofTplg, sofTplg.length);
            fdoCtx->sofTplgSz = sofTplg.length;
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

    if (fdoCtx->GraphicsDevicesCollection) {
        for (ULONG i = 0; i < WdfCollectionGetCount(fdoCtx->GraphicsDevicesCollection); i++) {
            WDFIOTARGET ioTarget = (WDFIOTARGET)WdfCollectionGetItem(fdoCtx->GraphicsDevicesCollection, i);
            PGRAPHICSIOTARGET_CONTEXT ioTargetContext = GraphicsIoTarget_GetContext(ioTarget);

            if (ioTargetContext->graphicsPowerRegisterOutput.DeviceHandle && ioTargetContext->graphicsPowerRegisterOutput.UnregisterCb) {
                NTSTATUS status = ioTargetContext->graphicsPowerRegisterOutput.UnregisterCb(ioTargetContext->graphicsPowerRegisterOutput.DeviceHandle, fdoCtx);
                if (!NT_SUCCESS(status)) {
                    DbgPrint("Warning: unregister failed with status 0x%x\n", status);
                }
            }
        }
    }

    if (fdoCtx->GraphicsNotificationHandle) {
        IoUnregisterPlugPlayNotification(fdoCtx->GraphicsNotificationHandle);
    }

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

    return STATUS_SUCCESS;
}

#define ENABLE_HDA 1

NTSTATUS
Fdo_EvtDeviceD0Entry(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
)
{
    UNREFERENCED_PARAMETER(PreviousState);

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
    else if (fdoCtx->venId == VEN_AMD || fdoCtx->venId == VEN_ATI) {
        update_pci_byte(&fdoCtx->BusInterface, ATI_SB450_HDAUDIO_MISC_CNTR2_ADDR, 0x07, ATI_SB450_HDAUDIO_ENABLE_SNOOP);
    }
    else if (fdoCtx->venId == VEN_NVIDIA) {
        update_pci_byte(&fdoCtx->BusInterface, NVIDIA_HDA_TRANSREG_ADDR, 0x0f, NVIDIA_HDA_ENABLE_COHBIT);
        update_pci_byte(&fdoCtx->BusInterface, NVIDIA_HDA_ISTRM_COH, 0x01, NVIDIA_HDA_ENABLE_COHBIT);
        update_pci_byte(&fdoCtx->BusInterface, NVIDIA_HDA_OSTRM_COH, 0x01, NVIDIA_HDA_ENABLE_COHBIT);
    }

    //Reset unsolicited queue
    RtlZeroMemory(&fdoCtx->unsol_queue, sizeof(fdoCtx->unsol_queue));
    fdoCtx->unsol_rp = 0;
    fdoCtx->unsol_wp = 0;
    fdoCtx->processUnsol = FALSE;

    //Reset CORB / RIRB
    RtlZeroMemory(&fdoCtx->corb, sizeof(fdoCtx->corb));
    RtlZeroMemory(&fdoCtx->rirb, sizeof(fdoCtx->rirb));

    status = StartHDAController(fdoCtx);

    if (fdoCtx->venId == VEN_INTEL) {
        UINT32 val;
        pci_read_cfg_dword(&fdoCtx->BusInterface, INTEL_HDA_CGCTL, &val);
        val = val | INTEL_HDA_CGCTL_MISCBDCGE;
        pci_write_cfg_dword(&fdoCtx->BusInterface, INTEL_HDA_CGCTL, val);

        hda_update32(fdoCtx, VS_EM2, HDA_VS_EM2_DUM, HDA_VS_EM2_DUM);
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "hda bus initialized\n");

    return status;
}

NTSTATUS
Fdo_EvtDeviceD0EntryPostInterrupts(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState
)
{
    UNREFERENCED_PARAMETER(PreviousState);

    NTSTATUS status;
    PFDO_CONTEXT fdoCtx;

    status = STATUS_SUCCESS;
    fdoCtx = Fdo_GetContext(Device);

#if ENABLE_HDA
    for (UINT8 addr = 0; addr < HDA_MAX_CODECS; addr++) {
        KeInitializeEvent(&fdoCtx->rirb.xferEvent[addr], NotificationEvent, FALSE);
        if (((fdoCtx->codecMask >> addr) & 0x1) == 0)
            continue;

        if (fdoCtx->UseSGPCCodec && fdoCtx->GraphicsCodecAddress == addr)
            continue;

        UINT32 cmdTmpl = (addr << 28) | (AC_NODE_ROOT << 20) |
            (AC_VERB_PARAMETERS << 8);

        ULONG vendorDevice;
        if (!NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_VENDOR_ID, &vendorDevice))) { //Some codecs might need a kickstart
            //First attempt failed. Retry
            NTSTATUS status2 = RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_VENDOR_ID, &vendorDevice); //If this fails, something is wrong.
            if (!NT_SUCCESS(status2)) {
                DbgPrint("Warning: Failed to wake up codec %d: 0x%x", addr, status2);
            }
        }
    }
#endif

    return status;
}

NTSTATUS
Fdo_EvtDeviceD0Exit(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState
)
{
    UNREFERENCED_PARAMETER(TargetState);

    NTSTATUS status;
    PFDO_CONTEXT fdoCtx;

    fdoCtx = Fdo_GetContext(Device);

    status = StopHDAController(fdoCtx);

    SklHdAudBusPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "%s\n", __func__);

    return status;
}

void
Fdo_EnumerateCodec(
    PFDO_CONTEXT fdoCtx,
    UINT8 addr
)
{
    UINT32 cmdTmpl = (addr << 28) | (AC_NODE_ROOT << 20) |
        (AC_VERB_PARAMETERS << 8);
    ULONG funcType = 0, vendorDevice, subsysId, revId, nodeCount;
    if (!NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_VENDOR_ID, &vendorDevice))) {
        return;
    }
    if (!NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_REV_ID, &revId))) {
        return;
    }
    if (!NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmdTmpl | AC_PAR_NODE_COUNT, &nodeCount))) {
        return;
    }

    fdoCtx->numCodecs += 1;

    UINT8 startID = (nodeCount >> 16) & 0xFF;
    nodeCount = (nodeCount & 0x7FFF);

    UINT16 mainFuncGrp = 0;
    {
        UINT16 nid = startID;
        for (UINT32 i = 0; i < nodeCount; i++, nid++) {
            UINT32 cmd = (addr << 28) | (nid << 20) |
                (AC_VERB_PARAMETERS << 8) | AC_PAR_FUNCTION_TYPE;
            if (!NT_SUCCESS(RunSingleHDACmd(fdoCtx, cmd, &funcType))) {
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
    RunSingleHDACmd(fdoCtx, cmd, &subsysId);

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
    if (fdoCtx->UseSGPCCodec && addr == fdoCtx->GraphicsCodecAddress)
        description.CodecIds.IsGraphicsCodec = TRUE;
    else
        description.CodecIds.IsGraphicsCodec = FALSE;

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
    WdfChildListAddOrUpdateChildDescriptionAsPresent(
        WdfFdoGetDefaultChildList(fdoCtx->WdfDevice), &description.Header,
        NULL); // AddressDescription
}

NTSTATUS
Fdo_EvtDeviceSelfManagedIoInit(
    _In_ WDFDEVICE Device
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PFDO_CONTEXT fdoCtx;

    fdoCtx = Fdo_GetContext(Device);

    WdfChildListBeginScan(WdfFdoGetDefaultChildList(Device));

    fdoCtx->numCodecs = 0;
#if ENABLE_HDA
    for (UINT8 addr = 0; addr < HDA_MAX_CODECS; addr++) {
        fdoCtx->codecs[addr] = NULL;
        if (((fdoCtx->codecMask >> addr) & 0x1) == 0)
            continue;

        if (fdoCtx->UseSGPCCodec && fdoCtx->GraphicsCodecAddress == addr)
            continue;

        Fdo_EnumerateCodec(fdoCtx, addr);
    }

    if (fdoCtx->mlcap) {
        IoRegisterPlugPlayNotification(
            EventCategoryDeviceInterfaceChange,
            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
            (PVOID)&GUID_DEVINTERFACE_GRAPHICSPOWER,
            WdfDriverWdmGetDriverObject(WdfDeviceGetDriver(fdoCtx->WdfDevice)),
            HDAGraphicsPowerInterfaceCallback,
            (PVOID)fdoCtx,
            &fdoCtx->GraphicsNotificationHandle
        );

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