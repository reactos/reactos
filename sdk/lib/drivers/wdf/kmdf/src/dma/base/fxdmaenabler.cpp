/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDmaEnabler.cpp

Abstract:

    Base for WDF DMA Enabler object

Environment:

    Kernel mode only.

Notes:


Revision History:

--*/

#include "FxDmaPCH.hpp"

extern "C" {
#include "FxDmaEnabler.tmh"
}

FxDmaEnabler::FxDmaEnabler(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    ) :
    FxNonPagedObject(FX_TYPE_DMA_ENABLER, sizeof(FxDmaEnabler), FxDriverGlobals)
{
    RtlZeroMemory(&m_SimplexAdapterInfo, sizeof(FxDmaDescription));
    RtlZeroMemory(&m_DuplexAdapterInfo, sizeof(m_DuplexAdapterInfo));

    //
    // Transaction link into list of FxDmaEnabler pointers maintained by
    // FxDevice's pnp package.
    //
    m_TransactionLink.SetTransactionedObject(this);

    m_FDO              = NULL;
    m_PDO              = NULL;
    m_CommonBufferAlignment        = 0;
    m_MaxSGElements    = WDF_DMA_ENABLER_UNLIMITED_FRAGMENTS;
    m_IsScatterGather    = FALSE;
    m_IsDuplexTransfer   = FALSE;
    m_IsSGListAllocated = FALSE;
    m_SGListSize = 0;

    m_IsAdded = FALSE;

    m_EvtDmaEnablerFill.m_Method               = NULL;
    m_EvtDmaEnablerFlush.m_Method              = NULL;
    m_EvtDmaEnablerEnable.m_Method             = NULL;
    m_EvtDmaEnablerDisable.m_Method            = NULL;
    m_EvtDmaEnablerSelfManagedIoStart.m_Method = NULL;
    m_EvtDmaEnablerSelfManagedIoStop.m_Method  = NULL;

    m_DmaEnablerFillFailed                     = FALSE;
    m_DmaEnablerEnableFailed                   = FALSE;
    m_DmaEnablerSelfManagedIoStartFailed       = FALSE;

    RtlZeroMemory(&m_SGList, sizeof(m_SGList));

    MarkDisposeOverride(ObjectDoNotLock);
}

FxDmaEnabler::~FxDmaEnabler()
{
    if (m_IsSGListAllocated) {
        if (m_IsScatterGather) {
            //
            // Scatter Gather profile - cleanup the lookaside list
            //
            ExDeleteNPagedLookasideList(&m_SGList.ScatterGatherProfile.Lookaside);

        } else if (!m_IsBusMaster) {
            //
            // System profile (not busmastering) - cleanup the preallocated
            // SG list
            //
            ExFreePool(m_SGList.SystemProfile.List);

        } else {
            //
            // Packet profile.  No special cleanup to do.
            //

        }

#if DBG
        RtlZeroMemory(&m_SGList, sizeof(m_SGList));
#endif
        m_IsSGListAllocated = FALSE;
    }
}


BOOLEAN
FxDmaEnabler::Dispose()
{
    ReleaseResources();

    if (m_IsAdded) {
        ASSERT(m_DeviceBase != NULL);
        m_DeviceBase->RemoveDmaEnabler(this);
    }

    return TRUE;
}

_Must_inspect_result_
NTSTATUS
FxDmaEnabler::Initialize(
    __in    PWDF_DMA_ENABLER_CONFIG  Config,
    __inout FxDeviceBase            *Device
    )
{
    NTSTATUS   status;
    DEVICE_DESCRIPTION deviceDescription;
    PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();
    ULONG mapRegistersAllocated;

    RtlZeroMemory(&deviceDescription, sizeof(DEVICE_DESCRIPTION));

    //
    // Default to version 2 description (except on ARM platforms)
    //

#ifdef _ARM_
    deviceDescription.Version = DEVICE_DESCRIPTION_VERSION3;
#else
    deviceDescription.Version = DEVICE_DESCRIPTION_VERSION2;
#endif

    //
    // Make sure the device's list of enablers has been created.
    //

    status = Device->AllocateDmaEnablerList();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                        "Unable to allocate DmaEnablerList for "
                        "WDFDEVICE %p, %!STATUS!",
                        Device->GetHandle(), status);
        return status;
    }

    //
    // Retain parent FxDeviceBase object
    //
    m_DeviceBase = Device;

    //
    // Save the profile.
    //
    m_Profile = Config->Profile;

    //
    // Invariant parameters vis-a-vis kernel-mode bus-mastering DMA APIs
    // (overrided below if using system-DMA
    //
    deviceDescription.Master            = TRUE;
    deviceDescription.Dma32BitAddresses = TRUE;
    deviceDescription.InterfaceType     = PCIBus;

    //
    // Assume enabler is a bus-master
    //
    m_IsBusMaster                       = TRUE;

    //
    // Expand the profile into settings.
    //
    switch (m_Profile) {
        //
        // Packet based profiles.
        //

        case WdfDmaProfilePacket:
            deviceDescription.ScatterGather     = FALSE;
            deviceDescription.Dma64BitAddresses = FALSE;
            break;
        case WdfDmaProfilePacket64:
            deviceDescription.ScatterGather     = FALSE;
            deviceDescription.Dma64BitAddresses = TRUE;
            break;

        //
        // Scatter-gather profiles
        //

        case WdfDmaProfileScatterGather:
            deviceDescription.ScatterGather     = TRUE;
            deviceDescription.Dma64BitAddresses = FALSE;
            m_IsScatterGather = TRUE;
            break;
        case WdfDmaProfileScatterGatherDuplex:
            deviceDescription.ScatterGather     = TRUE;
            deviceDescription.Dma64BitAddresses = FALSE;
            m_IsDuplexTransfer = TRUE;
            m_IsScatterGather = TRUE;
            break;
        case WdfDmaProfileScatterGather64:
            deviceDescription.ScatterGather     = TRUE;
            deviceDescription.Dma64BitAddresses = TRUE;
            m_IsScatterGather = TRUE;
            break;
        case WdfDmaProfileScatterGather64Duplex:
            deviceDescription.ScatterGather     = TRUE;
            deviceDescription.Dma64BitAddresses = TRUE;
            m_IsDuplexTransfer = TRUE;
            m_IsScatterGather = TRUE;
            break;

        //
        // Non-PC System-mode (non-bus-mastering) profiles.  These
        // require DMA v3.
        //

        case WdfDmaProfileSystem:
            deviceDescription.ScatterGather     = FALSE;
            deviceDescription.Master            = FALSE;
            deviceDescription.Dma32BitAddresses = FALSE;
            deviceDescription.Dma64BitAddresses = FALSE;
            deviceDescription.Version = DEVICE_DESCRIPTION_VERSION3;
            m_IsBusMaster = FALSE;
            m_DeviceBase->SetDeviceTelemetryInfoFlags(DeviceInfoDmaSystem);
            break;
        case WdfDmaProfileSystemDuplex:
            deviceDescription.ScatterGather     = FALSE;
            deviceDescription.Master            = FALSE;
            deviceDescription.Dma32BitAddresses = FALSE;
            deviceDescription.Dma64BitAddresses = FALSE;
            deviceDescription.Version = DEVICE_DESCRIPTION_VERSION3;
            m_IsBusMaster = FALSE;
            m_IsDuplexTransfer = TRUE;
            m_DeviceBase->SetDeviceTelemetryInfoFlags(DeviceInfoDmaSystemDuplex);
            break;

        //
        // Unknown profile.
        //

        default:
            //
            // Just do quick exit as no resource have been allocated.
            //
            return STATUS_INVALID_PARAMETER;
    }

    //
    // Save the maximum length.
    //
    m_MaximumLength = (ULONG) Config->MaximumLength;

    //
    // An override of address width requires the DMA v3 engine.  it also requires
    // that we explicitly specify the DMA width the controller can support, but
    // we do that down below.
    //
    if (Config->AddressWidthOverride != 0) {

        //
        // Address width override is not supported for system mode DMA, since
        // the HAL runs the DMA controller in that case and it knows the
        // controller's address limitations better than the driver does.
        //
        if (m_IsBusMaster == FALSE) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDMA,
                "AddressWidthOverride set to %d.  AddressWidthOverride "
                "must be zero when using a system DMA profile "
                "(%!WDF_DMA_PROFILE!) - %!STATUS!",
                Config->AddressWidthOverride,
                Config->Profile,
                status
                );
            FxVerifierDbgBreakPoint(GetDriverGlobals());
            return status;
        }

        if ((deviceDescription.Dma64BitAddresses == FALSE) &&
            (Config->AddressWidthOverride > 32)) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDMA,
                "AddressWidthOverride set to %d.  AddressWidthOverride "
                "must be <= 32 when using a 32-bit DMA profile "
                "(%!WDF_DMA_PROFILE!) - %!STATUS!",
                Config->AddressWidthOverride,
                Config->Profile,
                status
                );
            FxVerifierDbgBreakPoint(GetDriverGlobals());
            return status;
        }

        //
        // Handle the AddressWidthOverride.  For Win8 use DMA v3 and pass the
        // value through to the HAL.  For Win7 downgrade to the next lower
        // address width.
        //
        if (IsOsVersionGreaterThanOrEqualTo(6, 2)) {
            deviceDescription.Version = DEVICE_DESCRIPTION_VERSION3;
            deviceDescription.DmaAddressWidth = Config->AddressWidthOverride;
        }
        else {

            NT_ASSERTMSGW(L"Ensure driver is not doing something earlier that "
                          L"would require DMA v3 before we downgrade them to "
                          L"DMA v2",
                          (deviceDescription.Version == DEVICE_DESCRIPTION_VERSION2));

            if (Config->AddressWidthOverride < 64) {
                deviceDescription.Dma64BitAddresses = FALSE;
            }

            if (Config->AddressWidthOverride < 32) {
                deviceDescription.Dma32BitAddresses = FALSE;
            }

            //
            // DMA V2 can't handle an address width restriction smaller than
            // 24 bits (ISA DMA).  DMA V3 will fail that also - return the same
            // error here that DMA V3 would have.
            //
            if (Config->AddressWidthOverride < 24) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDMA,
                    "AddressWidthOverride of less than 24 bits is not supported"
                    );
                return STATUS_UNSUCCESSFUL;
            }
            else if ((Config->AddressWidthOverride != 64) &&
                     (Config->AddressWidthOverride != 32)) {

                //
                // Log a warning about downgrading DMA if we are actually
                // downgrading.  if the caller uses a 64-bit DMA
                // profile with an override of 64, or 32-bit with an override
                // of 32 then silently let it go through.
                //

                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGDMA,
                    "DMA AddressWidthOverride requires Windows version 6.2 or "
                    "higher.  Windows cannot support %d bit DMA is falling back to "
                    "the next lower supported width (%d-bit)",
                    Config->AddressWidthOverride,
                    (deviceDescription.Dma32BitAddresses ? 32 : 24)
                    );
            }
        }
    }

    //
    // Allow for a specific version override (and fail if
    // that override is inconsistent with the settings).  On Win7 this will
    // fail when we get the DMA adapter.
    //
    if (Config->WdmDmaVersionOverride != 0) {

        if (Config->WdmDmaVersionOverride < deviceDescription.Version) {
            status = STATUS_INVALID_PARAMETER;

            //
            // Driver is asking for a lower version of the DMA engine than the
            // config settings imply it needs.  Fail with invalid parameter.
            //

            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDMA,
                "WdmDmaVersionOverride set to %d, conflicts with required version of %d, "
                "%!STATUS!",
                Config->WdmDmaVersionOverride,
                deviceDescription.Version,
                status
                );

            FxVerifierDbgBreakPoint(GetDriverGlobals());
            return status;
        }

        deviceDescription.Version = Config->WdmDmaVersionOverride;
    }

    //
    // Propagate some settings from the old engine's location to the new ones.
    //
    if (deviceDescription.Version >= DEVICE_DESCRIPTION_VERSION3) {

        if (deviceDescription.DmaAddressWidth == 0) {

            if (deviceDescription.Dma64BitAddresses) {
                deviceDescription.DmaAddressWidth = 64;
            } else if (deviceDescription.Dma32BitAddresses) {
                deviceDescription.DmaAddressWidth = 32;
            } else {
                //
                // Assume ISA access width.
                //

                deviceDescription.DmaAddressWidth = 24;
            }
        }
    }

    //
    // Get the FDO
    //
    m_FDO = m_DeviceBase->GetDeviceObject();
    ASSERT(m_FDO != NULL);

    //
    // Get the PDO.  PDO may be NULL in the miniport case, but on
    // x86 that will still allow for DMA (IoGetDmaAdapter special
    // cases that on x86).  On amd64 the attempt to get the DMA
    // adapter later will fail cleanly.
    //
    m_PDO = m_DeviceBase->GetPhysicalDevice();

    mapRegistersAllocated = 0;

    //
    // If this device is a bus-master then configure the profile
    // right now, since we don't need to wait for PrepareHardware
    // to find out the DMA resource.
    //
    if (m_IsBusMaster) {
        status = ConfigureBusMasterAdapters(&deviceDescription, Config);
        if (!NT_SUCCESS(status)) {
            goto End;
        }
    }

    //
    // Retain the Power event callbacks.
    //
    m_EvtDmaEnablerFill.m_Method               = Config->EvtDmaEnablerFill;
    m_EvtDmaEnablerFlush.m_Method              = Config->EvtDmaEnablerFlush;
    m_EvtDmaEnablerEnable.m_Method             = Config->EvtDmaEnablerEnable;
    m_EvtDmaEnablerDisable.m_Method            = Config->EvtDmaEnablerDisable;
    m_EvtDmaEnablerSelfManagedIoStart.m_Method = Config->EvtDmaEnablerSelfManagedIoStart;
    m_EvtDmaEnablerSelfManagedIoStop.m_Method  = Config->EvtDmaEnablerSelfManagedIoStop;

    //
    // Add this DmaEnabler to the parent device's list of dma enablers.
    //
    m_DeviceBase->AddDmaEnabler(this);
    m_IsAdded = TRUE;

    //
    // update hardware info for Telemetry
    //
    if (m_IsBusMaster) {
        m_DeviceBase->SetDeviceTelemetryInfoFlags(DeviceInfoDmaBusMaster);
    }

    //
    // Success:
    //
    status  = STATUS_SUCCESS;

End:
    //
    // If errors then clean-up resources accumulated.
    //
    if (!NT_SUCCESS(status)) {
        ReleaseResources();
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxDmaEnabler::ConfigureSystemAdapter(
    __in PWDF_DMA_SYSTEM_PROFILE_CONFIG Config,
    __in WDF_DMA_DIRECTION              ConfigDirection
    )
{
    DEVICE_DESCRIPTION deviceDescription;

    NTSTATUS status;

    //
    // Check to make sure this direction isn't currently configured.
    //
    if (GetDmaDescription(ConfigDirection)->AdapterObject != NULL) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGDMA,
            "WDFDMAENABLER %p, profile %!WDF_DMA_PROFILE! "
            "Enabler has already been configured for %!WDF_DMA_DIRECTION!, %!STATUS!",
            GetHandle(), m_Profile,
            ConfigDirection,
            status
            );

        FxVerifierDbgBreakPoint(GetDriverGlobals());

        return status;
    }

    //
    // Initialize the adapter info from scratch given the Config structure
    // then copy it to the appropriate channel and do the allocation.
    //
    RtlZeroMemory(&deviceDescription, sizeof(DEVICE_DESCRIPTION));

    deviceDescription.Version = DEVICE_DESCRIPTION_VERSION3;
    deviceDescription.MaximumLength = m_MaximumLength;

    deviceDescription.DemandMode = Config->DemandMode;
    deviceDescription.AutoInitialize = Config->LoopedTransfer;

    deviceDescription.DmaWidth = Config->DmaWidth;

    deviceDescription.DeviceAddress = Config->DeviceAddress;

    //
    // Pull the remainder of the description from the provided resource.
    //
    deviceDescription.InterfaceType = Internal;
    deviceDescription.DmaChannel = Config->DmaDescriptor->u.Dma.Channel;
    deviceDescription.DmaRequestLine = Config->DmaDescriptor->u.Dma.Port;


    //
    // Run the common adapter configuration.
    //
    status = ConfigureDmaAdapter(
                &deviceDescription,
                ConfigDirection
                );

    if (!NT_SUCCESS(status)) {
        goto End;
    }

    //
    // Allocate a single SGList to pass to MapTransferEx.  Since we
    // only run a single system transfer at a time we can use the same
    // list for each transfer
    //

    {
        size_t systemSGListSize = 0;

        if (m_IsDuplexTransfer) {

            systemSGListSize = max(GetReadDmaDescription()->PreallocatedSGListSize,
                                   GetWriteDmaDescription()->PreallocatedSGListSize);
        } else {

            systemSGListSize = m_SimplexAdapterInfo.PreallocatedSGListSize;
        }

        //
        // Allocate the SG list.
        //
        m_SGList.SystemProfile.List =
            (PSCATTER_GATHER_LIST) ExAllocatePoolWithTag(NonPagedPool,
                                                         systemSGListSize,
                                                         GetDriverGlobals()->Tag);

        if (m_SGList.SystemProfile.List == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDMA,
                "Unable to allocate scatter gather list for system DMA "
                "enabler %p, %!STATUS!",
                GetHandle(), status
                );
            goto End;
        }

        m_IsSGListAllocated = TRUE;
        m_SGListSize = systemSGListSize;
    }

    //
    // For a simple enabler, both of these calls will return the same
    // DMA description entry.
    //
    if ((GetDmaDescription(
            WdfDmaDirectionReadFromDevice
            )->AdapterObject != NULL) &&
        (GetDmaDescription(
            WdfDmaDirectionWriteToDevice
            )->AdapterObject != NULL)) {
        m_IsConfigured = TRUE;
    }

End:

    return status;
}

_Must_inspect_result_
NTSTATUS
FxDmaEnabler::ConfigureBusMasterAdapters(
    __in PDEVICE_DESCRIPTION DeviceDescription,
    __in PWDF_DMA_ENABLER_CONFIG Config
    )
{
    ULONG alignment;
    NTSTATUS status;

    //
    // Initialize map register management
    //
    DeviceDescription->MaximumLength = m_MaximumLength;

    if (m_IsDuplexTransfer) {
        status = ConfigureDmaAdapter(DeviceDescription,
                                     WdfDmaDirectionReadFromDevice);

        if (!NT_SUCCESS(status)) {
            goto End;
        }

        status = ConfigureDmaAdapter(DeviceDescription,
                                     WdfDmaDirectionWriteToDevice);
    } else {
        //
        // Direction is ignored in this case.
        //

        status = ConfigureDmaAdapter(DeviceDescription,
                                     WdfDmaDirectionReadFromDevice);
    }

    if (!NT_SUCCESS(status)) {
        goto End;
    }

    //
    // Allocate a scatter gather lookaside list if we need one.
    //

    if (m_IsScatterGather) {
        size_t sgLookasideListSize;

        sgLookasideListSize = 0;

        if (m_IsDuplexTransfer) {
            FxDmaDescription *readDmaDesc = GetReadDmaDescription();
            FxDmaDescription *writeDmaDesc = GetWriteDmaDescription();

            alignment = readDmaDesc->AdapterObject->DmaOperations->
                            GetDmaAlignment(readDmaDesc->AdapterObject);

            //
            // GetDmaAlignment returns alignment in terms of bytes
            // while we treat alignment as a mask (which is how it is set
            // in _DEVICE_OBJECT as well.
            // For example, for byte alignment GetDmaAlignment returns 1 while
            // the alignment mask is 0x00000000
            //
            // For < 1.11 drivers we keep the same behaviour as before for
            // compatibility.
            //
            if (GetDriverGlobals()->IsVersionGreaterThanOrEqualTo(1, 11) &&
                alignment > 0) {
                alignment -= 1;
            }

            m_CommonBufferAlignment = (ULONG) FxSizeTMax(m_FDO->AlignmentRequirement,
                                                         alignment);

            //
            // We will create a lookaside list based on the larger of the read &
            // write SGListSize. It's done this way so that we can allocate
            // sglist buffer when the dma-transaction object is created, where
            // we don't know the direction of DMA transfer, and make the
            // transaction initialize call fail-proof.
            //
            sgLookasideListSize = FxSizeTMax(
                                        readDmaDesc->PreallocatedSGListSize,
                                        writeDmaDesc->PreallocatedSGListSize
                                        );
        } else {

            FxDmaDescription *simplexDmaDesc = &m_SimplexAdapterInfo;

            alignment = simplexDmaDesc->AdapterObject->DmaOperations->
                                GetDmaAlignment(simplexDmaDesc->AdapterObject);

            //
            // GetDmaAlignment returns alignment in terms of bytes
            // while we treat alignment as a mask (which is how it is set
            // in _DEVICE_OBJECT as well.
            // For example, for byte alignment GetDmaAlignment returns 1 while
            // the alignment mask is 0x00000000
            //
            // For < 1.11 drivers we keep the same behaviour as before for
            // compatibility.
            //
            if (GetDriverGlobals()->IsVersionGreaterThanOrEqualTo(1, 11) &&
                alignment > 0) {
                alignment -= 1;
            }

            m_CommonBufferAlignment = (ULONG) FxSizeTMax(m_FDO->AlignmentRequirement,
                                                         alignment);

            sgLookasideListSize = simplexDmaDesc->PreallocatedSGListSize;
        }

        //
        // Initialize a LookasideList for ScatterGather list
        //
        if ((Config->Flags & WDF_DMA_ENABLER_CONFIG_NO_SGLIST_PREALLOCATION) == 0) {
            ASSERT(m_IsSGListAllocated == FALSE);

            m_SGListSize = sgLookasideListSize;

            ExInitializeNPagedLookasideList( &m_SGList.ScatterGatherProfile.Lookaside,
                                             NULL, // Allocate  OPTIONAL
                                             NULL, //  Free  OPTIONAL
                                             0, // Flag - Reserved. Must be zero.
                                             m_SGListSize,
                                             GetDriverGlobals()->Tag,
                                             0 ); // Depth - Reserved. Must be zero.

            m_IsSGListAllocated = TRUE;
        }
    }

    //
    // The DMA enabler is configured now.
    //

    m_IsConfigured = TRUE;

End:

    return status;
}

_Must_inspect_result_
NTSTATUS
FxDmaEnabler::ConfigureDmaAdapter(
    __in PDEVICE_DESCRIPTION DeviceDescription,
    __in WDF_DMA_DIRECTION   ConfigDirection
    )
{
    FxDmaDescription *dmaDesc;
    NTSTATUS status;

    //
    // Select the adapter to configure.
    //

    dmaDesc = GetDmaDescription(ConfigDirection);

    //
    // Copy the device-description we have built up so far
    // into the read and write dma description field. These
    // settings are common to both channels.
    //
    RtlCopyMemory(&dmaDesc->DeviceDescription,
                  DeviceDescription,
                  sizeof(DEVICE_DESCRIPTION));

    //
    // Then initialize resources that are private to read and write.
    //

    status = InitializeResources(dmaDesc);
    return status;
}

_Must_inspect_result_
NTSTATUS
FxDmaEnabler::InitializeResources(
    __inout FxDmaDescription *AdapterInfo
    )
{
    NTSTATUS status;
    PFX_DRIVER_GLOBALS    pFxDriverGlobals = GetDriverGlobals();

    NT_ASSERTMSG("expected caller to set DMA version",
                 AdapterInfo->DeviceDescription.Version != 0);

    //
    // Submit IoGetDmaAdapter and retain the DmaAdapter pointer.
    //
    AdapterInfo->AdapterObject =
            IoGetDmaAdapter(m_PDO,
                            &AdapterInfo->DeviceDescription,
                            (PULONG)&AdapterInfo->NumberOfMapRegisters);

    if (AdapterInfo->AdapterObject == NULL) {
        status = STATUS_UNSUCCESSFUL;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Unable to allocate DmaAdapter object for "
                            "WDFDMAENABLER %p, %!STATUS!",
                            GetHandle(), status);
        return status;
    }

    //
    // Calculate the size of the SGList.
    //
    if (m_IsScatterGather) {

        //
        // For scatter gather DMA we ask the HAL how many bytes it needs for
        // each SGList.  The HAL allocates some scratch space of its own in
        // each SGList, which BuildScatterGatherList depends on.
        //
        ULONG mapRegistersCount;

        status = AdapterInfo->AdapterObject->DmaOperations->
            CalculateScatterGatherList( AdapterInfo->AdapterObject,
                                        NULL, // Optional MDL
                                        NULL, //  CurrentVa
                                        AdapterInfo->NumberOfMapRegisters * PAGE_SIZE,
                                        (PULONG) &AdapterInfo->PreallocatedSGListSize,
                                        &mapRegistersCount);

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDMA,
                "CalculateScatterGatherList failed for "
                "WDFDMAENABLER %p, %!STATUS!", GetHandle(), status);
            return status;
        }

        ASSERT(AdapterInfo->NumberOfMapRegisters == mapRegistersCount);

    } else if (m_IsBusMaster) {

        //
        // For packet based DMA we only need a single SGList entry because
        // the HAL moves all of the data into a single continguous buffer
        //
        AdapterInfo->PreallocatedSGListSize = sizeof(SCATTER_GATHER_LIST) +
                                              sizeof(SCATTER_GATHER_ELEMENT);

    } else {

        //
        // For system DMA we need a single SGList entry per map-register
        //
        AdapterInfo->PreallocatedSGListSize = sizeof(SCATTER_GATHER_LIST) +
                                              (sizeof(SCATTER_GATHER_ELEMENT) *
                                               AdapterInfo->NumberOfMapRegisters);
    }

    ASSERT(AdapterInfo->NumberOfMapRegisters > 1);

    AdapterInfo->MaximumFragmentLength = FxSizeTMin(m_MaximumLength,
                                      ((size_t) (AdapterInfo->NumberOfMapRegisters - 1)) << PAGE_SHIFT);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                        "WDFDMAENABLER %p, profile %!WDF_DMA_PROFILE! "
                        "DmaAdapterObject %p, MapRegisters %d, "
                        "MaximumFragmentLength %I64d ", GetHandle(), m_Profile,
                        AdapterInfo->AdapterObject,
                        AdapterInfo->NumberOfMapRegisters,
                        AdapterInfo->MaximumFragmentLength);

    if (AdapterInfo->MaximumFragmentLength < m_MaximumLength) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                        "The maximum transfer length for WDFDMAENABLER %p "
                        "is reduced to %I64d from %I64d due to mapregisters limit",
                        GetHandle(), m_MaximumLength,
                        AdapterInfo->MaximumFragmentLength);
    }

    return STATUS_SUCCESS;
}

VOID
FxDmaEnabler::FreeResources(
    __inout FxDmaDescription *AdapterInfo
    )
{
    if (AdapterInfo->AdapterObject != NULL) {
        AdapterInfo->AdapterObject->DmaOperations->PutDmaAdapter(AdapterInfo->AdapterObject);
        AdapterInfo->AdapterObject = NULL;
    }
}

VOID
FxDmaEnabler::ReleaseResources(
    VOID
    )
{
    FreeResources(GetReadDmaDescription());
    FreeResources(GetWriteDmaDescription());

    m_IsConfigured = FALSE;

}

VOID
FxDmaEnabler::RevokeResources(
    VOID
    )
{
    //
    // Give back any system DMA resources allocated for this device
    //

    if (m_IsBusMaster == FALSE)
    {





    }
}

// ----------------------------------------------------------------------------
// ------------------------ Pnp/Power notification -----------------------------
// ----------------------------------------------------------------------------

_Must_inspect_result_
NTSTATUS
FxDmaEnabler::PowerUp(
    VOID
    )
{
    NTSTATUS              status          = STATUS_SUCCESS;
    PFX_DRIVER_GLOBALS    pFxDriverGlobals = GetDriverGlobals();
    WDFDMAENABLER         handle          = GetHandle();
    FxDmaEnablerCallbacks tag             = FxEvtDmaEnablerInvalid;

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                        "WDFDMAENABLER %p: PowerUp notification", GetHandle());

    do {
        if (m_EvtDmaEnablerFill.m_Method) {

            status = m_EvtDmaEnablerFill.Invoke( handle );

            if (!NT_SUCCESS(status)) {
                m_DmaEnablerFillFailed = TRUE;
                tag = FxEvtDmaEnablerFill;
                break;
            }
        }

        if (m_EvtDmaEnablerEnable.m_Method) {

            status = m_EvtDmaEnablerEnable.Invoke( handle );

            if (!NT_SUCCESS(status)) {
                m_DmaEnablerEnableFailed = TRUE;
                tag = FxEvtDmaEnablerEnable;
                break;
            }
        }

        if (m_EvtDmaEnablerSelfManagedIoStart.m_Method) {

            status = m_EvtDmaEnablerSelfManagedIoStart.Invoke( handle );

            if (!NT_SUCCESS(status)) {
                m_DmaEnablerSelfManagedIoStartFailed = TRUE;
                tag = FxEvtDmaEnablerSelfManagedIoStart;
                break;
            }
        }

    } WHILE (0);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "WDFDMAENABLER %p: PowerUp: "
                            "%!WdfDmaEnablerCallback! failed %!STATUS!",
                            GetHandle(), tag, status);
    }
    return status;
}

_Must_inspect_result_
NTSTATUS
FxDmaEnabler::PowerDown(
    VOID
    )
{
    NTSTATUS              status          = STATUS_SUCCESS;
    NTSTATUS              localStatus;
    PFX_DRIVER_GLOBALS    pFxDriverGlobals = GetDriverGlobals();
    WDFDMAENABLER         handle          = GetHandle();
    FxDmaEnablerCallbacks tag             = FxEvtDmaEnablerInvalid;

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                        "WDFDMAENABLER %p: PowerDown notification", GetHandle());

    do {

        if (m_EvtDmaEnablerSelfManagedIoStop.m_Method) {

            localStatus = m_EvtDmaEnablerSelfManagedIoStop.Invoke( handle );

            if (!NT_SUCCESS(localStatus)) {
                tag = FxEvtDmaEnablerSelfManagedIoStop;
                status = (NT_SUCCESS(status)) ? localStatus : status;
            }
        }

        if (m_EvtDmaEnablerDisable.m_Method &&
            m_DmaEnablerFillFailed == FALSE)
        {
            localStatus = m_EvtDmaEnablerDisable.Invoke( handle );

            if (!NT_SUCCESS(localStatus)) {
                tag = FxEvtDmaEnablerDisable;
                status = (NT_SUCCESS(status)) ? localStatus : status;
            }
        }

        if (m_EvtDmaEnablerFlush.m_Method     &&
            m_DmaEnablerFillFailed   == FALSE &&
            m_DmaEnablerEnableFailed == FALSE)
        {
            localStatus = m_EvtDmaEnablerFlush.Invoke( handle );

            if (!NT_SUCCESS(localStatus)) {
                tag = FxEvtDmaEnablerFlush;
                status = (NT_SUCCESS(status)) ? localStatus : status;
            }
        }

    } WHILE (0);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "WDFDMAENABLER %p: PowerDown: "
                            "%!WdfDmaEnablerCallback! failed %!STATUS!",
                            GetHandle(), tag, status);
    }

    return status;
}

// ----------------------------------------------------------------------------
// ------------------------ COMMON BUFFER SECTION -----------------------------
// ----------------------------------------------------------------------------

VOID
FxDmaEnabler::AllocateCommonBuffer(
    __in         size_t         Length,
    __deref_out_opt  PVOID    * BufferVA,
    __out  PHYSICAL_ADDRESS   * BufferPA
    )
{
    ULONG result;
    PDMA_ADAPTER adapterObject;
    PFX_DRIVER_GLOBALS    pFxDriverGlobals = GetDriverGlobals();

    *BufferVA = NULL;
    BufferPA->QuadPart = 0;

    if (!NT_SUCCESS(RtlSizeTToULong(Length, &result))) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
            "WDFDMAENABLER %p AllocateCommonBuffer:  could cast value %I64d to a "
            "ULONG", GetHandle(), Length);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    //
    // It doesn't matter which channel we use for allocating common buffers
    // because the addressing capability of all the channels of this DMA enablers
    // are same.
    //
    adapterObject = GetReadDmaDescription()->AdapterObject;

    *BufferVA = adapterObject->DmaOperations->
        AllocateCommonBuffer( adapterObject,
                              result,
                              BufferPA,
                              TRUE /* CacheEnabled */ );
}


VOID
FxDmaEnabler::FreeCommonBuffer(
    __in size_t               Length,
    __in PVOID                BufferVA,
    __in PHYSICAL_ADDRESS     BufferPA
    )
{
    PDMA_ADAPTER adapterObject;

    adapterObject = GetReadDmaDescription()->AdapterObject;

    adapterObject->DmaOperations->
        FreeCommonBuffer( adapterObject,
                          (ULONG) Length,
                          BufferPA,
                          BufferVA,
                          TRUE /* CacheEnabled */ );
}

VOID
FxDmaEnabler::InitializeTransferContext(
    __out PVOID             Context,
    __in  WDF_DMA_DIRECTION Direction
    )
{
    PDMA_ADAPTER adapter = GetDmaDescription(Direction)->AdapterObject;

    NT_ASSERTMSG(
        "should not call this routine if enabler is not using DMAv3",
        UsesDmaV3()
        );

    PDMA_OPERATIONS dmaOperations =
        adapter->DmaOperations;

    dmaOperations->InitializeDmaTransferContext(adapter, Context);
}
