#ifndef __FX_DMA_ENABLER_H__
#define __FX_DMA_ENABLER_H__

#include "common/fxnonpagedobject.h"
#include "common/fxdmaenablercallbacks.h"


//
// Dma Description structure
//
typedef struct _FxDmaDescription {

    DEVICE_DESCRIPTION     DeviceDescription;

    PDMA_ADAPTER           AdapterObject;

    // 
    // The size of a preallocated lookaside list for this DMA adapter
    //
    // Might be stale after initializing a single-transfer transaction,
    // so use FxDmaEnabler::m_SGListSize when you use the list buffer.
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

    //friend class FxDmaTransactionBase;
    //friend class FxDmaPacketTransaction;
    //friend class FxDmaScatterGatherTransaction;
    //friend class FxDmaSystemTransaction;

public:

    FxDmaEnabler(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    ~FxDmaEnabler();

    __inline
    WDFDMAENABLER
    GetHandle(
        VOID
        )
    {
        return (WDFDMAENABLER) GetObjectHandle();
    }

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


private:
    //
    // Power-related callbacks.
    //
    FxEvtDmaEnablerFillCallback               m_EvtDmaEnablerFill;
    //FxEvtDmaEnablerFlushCallback              m_EvtDmaEnablerFlush;
    FxEvtDmaEnablerEnableCallback             m_EvtDmaEnablerEnable;
    //FxEvtDmaEnablerDisableCallback            m_EvtDmaEnablerDisable;
    FxEvtDmaEnablerSelfManagedIoStartCallback m_EvtDmaEnablerSelfManagedIoStart;
    //FxEvtDmaEnablerSelfManagedIoStopCallback  m_EvtDmaEnablerSelfManagedIoStop;

    //
    // Note that these fields form an informal state engine.
    //
    BOOLEAN   m_DmaEnablerFillFailed;
    BOOLEAN   m_DmaEnablerEnableFailed;
    BOOLEAN   m_DmaEnablerSelfManagedIoStartFailed;
};

#endif //__FX_DMA_ENABLER_H__