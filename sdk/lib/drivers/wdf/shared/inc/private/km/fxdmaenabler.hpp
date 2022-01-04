/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDmaEnabler.hpp

Abstract:

    WDF DMA Enabler support

Environment:

    Kernel mode only.

Notes:


Revision History:

--*/

#ifndef __FX_DMA_ENABLER_HPP__
#define __FX_DMA_ENABLER_HPP__

#include "fxdmaenablercallbacks.hpp"

//
// Dma Description structure
//
typedef struct _FxDmaDescription {

    DEVICE_DESCRIPTION     DeviceDescription;

    PDMA_ADAPTER           AdapterObject;

    //
    // The size of a preallocated lookaside list for this DMA adapter
    //
    size_t                 PreallocatedSGListSize;

    size_t                 MaximumFragmentLength;

    ULONG                  NumberOfMapRegisters;

} FxDmaDescription;

enum FxDuplexDmaDescriptionType {
    FxDuplexDmaDescriptionTypeRead = 0,
    FxDuplexDmaDescriptionTypeWrite,
    FxDuplexDmaDescriptionTypeMax,
};

//
// Make sure the two enums which match to the channels in the enabler match
// corresponding values.
//

C_ASSERT(((ULONG) FxDuplexDmaDescriptionTypeRead) == ((ULONG) WdfDmaDirectionReadFromDevice));
C_ASSERT(((ULONG) FxDuplexDmaDescriptionTypeWrite) == ((ULONG) WdfDmaDirectionWriteToDevice));

//
// Declare the FxDmaEnabler class
//
class FxDmaEnabler : public FxNonPagedObject {

    friend class FxDmaTransactionBase;
    friend class FxDmaPacketTransaction;
    friend class FxDmaScatterGatherTransaction;
    friend class FxDmaSystemTransaction;

public:

    FxDmaEnabler(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    ~FxDmaEnabler();

    virtual
    BOOLEAN
    Dispose(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    Initialize(
        __in    PWDF_DMA_ENABLER_CONFIG  Config,
        __inout FxDeviceBase            *Device
        );

    _Must_inspect_result_
    NTSTATUS
    ConfigureSystemAdapter(
        __in PWDF_DMA_SYSTEM_PROFILE_CONFIG Config,
        __in WDF_DMA_DIRECTION              ConfigDirection
        );

    VOID
    AllocateCommonBuffer(
        __in         size_t        Length,
        __deref_out_opt PVOID    * BufferVA,
        __out PHYSICAL_ADDRESS   * BufferPA
        );

    VOID
    FreeCommonBuffer(
        __in size_t               Length,
        __in PVOID                BufferVA,
        __in PHYSICAL_ADDRESS     BufferPA
        );

    _Must_inspect_result_
    NTSTATUS
    PowerUp(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    PowerDown(
        VOID
        );

    VOID
    RevokeResources(
        VOID
        );

    VOID
    InitializeTransferContext(
        __out PVOID             Context,
        __in  WDF_DMA_DIRECTION Direction
        );

    __inline
    size_t
    GetMaximumLength(
        VOID
        )
    {
        //
        // This value is same for all the channels and equal to the value
        // provided in the DMA_ENABLER_CONFIG.
        //
        return GetReadDmaDescription()->DeviceDescription.MaximumLength;
    }

    __inline
    size_t
    GetAlignment(
        VOID
        )
    {
        return m_CommonBufferAlignment;
    }

    __inline
    WDFDMAENABLER
    GetHandle(
        VOID
        )
    {
        return (WDFDMAENABLER) GetObjectHandle();
    }

    __inline
    WDFDEVICE
    GetDeviceHandle(
        VOID
        )
    {

        return m_DeviceBase->GetHandle();
    }

    __inline
    size_t
    GetMaxSGElements(
        VOID
        )
    {
        return m_MaxSGElements;
    }

    __inline
    VOID
    SetMaxSGElements(
        __in size_t MaximumSGElements
        )
    {
        m_MaxSGElements = (ULONG) MaximumSGElements;
    }

    __inline
    WDF_DMA_PROFILE
    GetProfile(
        VOID
        )
    {
        return m_Profile;
    }

    __inline
    BOOLEAN
    SupportsChainedMdls(
        VOID
        )
    {
        //
        // The only case where we don't support chained MDLS is DMAV2
        // with packet mode.
        //

        if ((UsesDmaV3() == false) &&
            (m_IsBusMaster == TRUE) &&
            (m_IsScatterGather == FALSE)) {
            return false;
        } else {
            return true;
        }
    }

    __inline
    BOOLEAN
    IsBusMaster(
        VOID
        )
    {
        return m_IsBusMaster;
    }

    __inline
    BOOLEAN
    IsPacketBased(
        )
    {
        return m_IsScatterGather ? FALSE : TRUE ;
    }

    __inline
    FxDmaDescription*
    GetDmaDescription(
        __in WDF_DMA_DIRECTION Direction
        )
    {
        if (m_IsDuplexTransfer) {
            return &m_DuplexAdapterInfo[Direction];
        }
        else {
            return &m_SimplexAdapterInfo;
        }
    }


    __inline
    FxDmaDescription*
    GetWriteDmaDescription(
        VOID
        )
    {
        if (m_IsDuplexTransfer) {
            return &m_DuplexAdapterInfo[FxDuplexDmaDescriptionTypeWrite];
        } else {
            return &m_SimplexAdapterInfo;
        }
    }

    __inline
    FxDmaDescription*
    GetReadDmaDescription(
        VOID
        )
    {
        if (m_IsDuplexTransfer) {
            return &m_DuplexAdapterInfo[FxDuplexDmaDescriptionTypeRead];
        } else {
            return &m_SimplexAdapterInfo;
        }
    }

    BOOLEAN
    UsesDmaV3(
        VOID
        )
    {
        FxDmaDescription* description;

        //
        // It doesn't matter which direction we use below.  Direction is
        // ignored for the simplex enabler, and will be the same for both
        // channels in a duplex enabler.
        //

        description = GetDmaDescription(WdfDmaDirectionReadFromDevice);

        return description->DeviceDescription.Version == DEVICE_DESCRIPTION_VERSION3;
    }

    USHORT
    GetTransferContextSize(
        VOID
        )
    {
        return UsesDmaV3() ? DMA_TRANSFER_CONTEXT_SIZE_V1 : 0;
    }

public:
    //
    // Link into list of FxDmaEnabler pointers maintained by the pnp package.
    //
    FxTransactionedEntry m_TransactionLink;

protected:

    PDEVICE_OBJECT          m_FDO;

    PDEVICE_OBJECT          m_PDO;

    union {
        //
        // Used if the dma profile is not duplex. Common for both read & write.
        // All the information specific to the channel are stored in the struct.
        //
        FxDmaDescription        m_SimplexAdapterInfo;

        //
        // Used if the dma profile is duplex.
        //
        FxDmaDescription        m_DuplexAdapterInfo[FxDuplexDmaDescriptionTypeMax];
    };

    //
    // The profile of the DMA enabler.
    //
    WDF_DMA_PROFILE         m_Profile;

    //
    // Whether the enabler object is added to the device enabler list.
    //
    BOOLEAN                 m_IsAdded : 1;

    //
    // Whether the DMA enabler has been configured with DMA resources & has its
    // adapters.
    //
    BOOLEAN                 m_IsConfigured : 1;

    //
    // The DMA profile broken out into individual flags.
    //
    BOOLEAN                 m_IsBusMaster : 1;

    BOOLEAN                 m_IsScatterGather : 1;

    BOOLEAN                 m_IsDuplexTransfer : 1;

    //
    // Has the preallocated scatter gather list (single list or lookaside,
    // depending on profile) been allocated.  Indicates initialization state
    // of m_SGList below.
    //
    BOOLEAN                 m_IsSGListAllocated: 1;

    //
    // This value is larger of aligment value returned by HAL(which is always 1)
    // and the alignment value set on the device by WdfDeviceSetAlignmentRequirement.
    //
    ULONG                   m_CommonBufferAlignment;

    //
    // The maximum DMA transfer the enabler should support (saved from the enabler
    // config)
    //
    ULONG                   m_MaximumLength;

    ULONG                   m_MaxSGElements;

    //
    // The size of the preallocated SGList entries, in bytes.  This is for entries
    // on the lookaside list or the single entry list.
    //
    size_t                  m_SGListSize;

    //
    // Storage for scatter gather lists.  Whether the lookaside or the single entry
    // the size in bytes is described by m_SGListSize
    //
    // The m_IsSGListAllocated bit above indicates whether we've
    // initialized this structure or not.
    //
    union {

        //
        // For the scatter gather profile we have a lookaside list of SGLists
        // We allocate these dynamically because (a) we can be mapping
        // multiple transactions in parallel and (b) the number of map registers
        // for SG DMA may be very large and we don't necessarily need one per
        // transaction at all times
        //
        // For duplex channels, the entry size is larger of read & write channels.
        //
        struct {
            NPAGED_LOOKASIDE_LIST   Lookaside;
        } ScatterGatherProfile;

        //
        // A single SGList for use with system DMA transfers.  We can use a single
        // list because (a) there is only one system DMA transaction being mapped
        // at any given time (for this adapter) and (b) we don't use the physical
        // addresses or give them to the driver so they don't need to be preserved
        // very long (the list is only so the HAL has enough scratch space to tell
        // the HAL DMA extension which PA's comprise the transfer)
        //
        struct {
            PSCATTER_GATHER_LIST    List;
        } SystemProfile;

        //
        // NOTE: there is no preallocated entry for packet based bus mastering DMA
        //       because we could be mapping multiple transactions in parallel but
        //       the size of the SGList is static and can be allocated on the stack
        //

    } m_SGList;

private:
    //
    // Power-related callbacks.
    //
    FxEvtDmaEnablerFillCallback               m_EvtDmaEnablerFill;
    FxEvtDmaEnablerFlushCallback              m_EvtDmaEnablerFlush;
    FxEvtDmaEnablerEnableCallback             m_EvtDmaEnablerEnable;
    FxEvtDmaEnablerDisableCallback            m_EvtDmaEnablerDisable;
    FxEvtDmaEnablerSelfManagedIoStartCallback m_EvtDmaEnablerSelfManagedIoStart;
    FxEvtDmaEnablerSelfManagedIoStopCallback  m_EvtDmaEnablerSelfManagedIoStop;

    //
    // Note that these fields form an informal state engine.
    //
    BOOLEAN   m_DmaEnablerFillFailed;
    BOOLEAN   m_DmaEnablerEnableFailed;
    BOOLEAN   m_DmaEnablerSelfManagedIoStartFailed;

    _Must_inspect_result_
    NTSTATUS
    ConfigureBusMasterAdapters(
        __in PDEVICE_DESCRIPTION DeviceDescription,
        __in PWDF_DMA_ENABLER_CONFIG Config
        );

    _Must_inspect_result_
    NTSTATUS
    ConfigureDmaAdapter(
        __in PDEVICE_DESCRIPTION DeviceDescription,
        __in WDF_DMA_DIRECTION   ConfigDirection
        );

    _Must_inspect_result_
    NTSTATUS
    InitializeResources(
        __inout FxDmaDescription *AdapterInfo
        );

    VOID
    ReleaseResources(
        VOID
        );

    VOID
    FreeResources(
        __inout FxDmaDescription *AdapterInfo
        );

};  // end of class

#endif // _FXDMAENABLER_HPP_
