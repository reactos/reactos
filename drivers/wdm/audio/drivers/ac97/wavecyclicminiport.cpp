/********************************************************************************
**    Copyright (c) 1998-2000 Microsoft Corporation. All Rights Reserved.
**
**       Portions Copyright (c) 1998-1999 Intel Corporation
**
********************************************************************************/

// Every debug output has "Modulname text"
#define STR_MODULENAME "AC97 Wave Cyclic: "

#include "wavecyclicminiport.h"
#include "wavecyclicstream.h"


#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif

IMP_CMiniport(CMiniportWaveCyclic, IID_IMiniportWaveCyclic)

/*****************************************************************************
 * CreateAC97MiniportWaveCyclic
 *****************************************************************************
 * Creates a AC97 wave miniport object for the AC97 adapter.
 * This uses a macro from STDUNK.H to do all the work.
 */
NTSTATUS CreateAC97MiniportWaveCyclic
(
    OUT PUNKNOWN   *Unknown,
    IN  REFCLSID,
    IN  PUNKNOWN    UnknownOuter    OPTIONAL,
    _When_((PoolType & NonPagedPoolMustSucceed) != 0,
       __drv_reportError("Must succeed pool allocations are forbidden. "
			 "Allocation failures cause a system crash"))
    IN  POOL_TYPE   PoolType
)
{
    PAGED_CODE ();

    ASSERT (Unknown);

    DOUT (DBG_PRINT, ("[CreateAC97MiniportWaveCyclic]"));

    STD_CREATE_BODY_WITH_TAG_(CMiniportWaveCyclic,Unknown,UnknownOuter,PoolType,
                     PoolTag, PMINIPORTWAVECYCLIC);
}

/*****************************************************************************
 * CMiniportWaveCyclic::Init
 *****************************************************************************
 * Initializes the miniport.
 * Initializes variables and modifies the wave topology if needed.
 */
STDMETHODIMP_(NTSTATUS) CMiniportWaveCyclic::Init
(
    _In_  PUNKNOWN       UnknownAdapter,
    _In_  PRESOURCELIST  ResourceList,
    _In_  PPORTWAVECYCLIC   Port_
)
{
    PAGED_CODE ();

    NTSTATUS ntStatus = CMiniport::Init(
                            UnknownAdapter,
                            ResourceList,
                            Port_
                        );

    if (!NT_SUCCESS (ntStatus))
    {
        return ntStatus;
    }

    //
    // Create the DMA Channel object.
    //
    ntStatus = Port->NewMasterDmaChannel (&DmaChannel,      // OutDmaChannel
                                          NULL,             // OuterUnknown (opt)
                                          NULL,             // ResourceList (opt)
                                          0x1FFFE,          // MaximumLength
                                          TRUE,             // Dma32BitAddresses
                                          FALSE,            // Dma64BitAddresses
                                          Width32Bits,      // DmaWidth
                                          MaximumDmaSpeed); // DmaSpeed
    if (!NT_SUCCESS (ntStatus))
    {
        DOUT (DBG_ERROR, ("Failed on NewMasterDmaChannel!"));
        return ntStatus;
    }

    //
    // On failure object is destroyed which cleans up.
    //
    return ntStatus;
}

/*****************************************************************************
 * CMiniportWaveCyclic::NewStream
 *****************************************************************************
 * Creates a new stream.
 * This function is called when a streaming pin is created.
 * It checks if the channel is already in use, tests the data format, creates
 * and initializes the stream object.
 */
_Use_decl_annotations_
STDMETHODIMP CMiniportWaveCyclic::NewStream
(
    PMINIPORTWAVECYCLICSTREAM *Stream,
    PUNKNOWN                OuterUnknown,
    POOL_TYPE               PoolType,
    ULONG                   Pin_,
    BOOLEAN                 Capture,
    PKSDATAFORMAT           DataFormat,
    PDMACHANNEL            *DmaChannel_,
    PSERVICEGROUP          *ServiceGroup
)
{
    PAGED_CODE ();

    ASSERT (Stream);
    ASSERT (DataFormat);
    ASSERT (DmaChannel_);
    ASSERT (ServiceGroup);

    CMiniportWaveCyclicStream *pWaveCyclicStream = NULL;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    DOUT (DBG_PRINT, ("[CMiniportWaveCyclic::NewStream]"));

    //
    // Check parameters.
    //
    ntStatus = ValidateFormat (DataFormat, (WavePins)Pin_);
    if (!NT_SUCCESS (ntStatus))
    {
        return ntStatus;
    }

    //
    // Create a new stream.
    //
    ntStatus = CreateMiniportWaveCyclicStream (&pWaveCyclicStream, OuterUnknown,
                                            PoolType);

    //
    // Return in case of an error.
    //
    if (!NT_SUCCESS (ntStatus))
    {
        DOUT (DBG_ERROR, ("[NewStream] Failed to create stream!"));
        return ntStatus;
    }

    //
    // Initialize the stream.
    //

    ntStatus = pWaveCyclicStream->Init ((CMiniport*)this,
                                        NULL,
                                        (WavePins)Pin_,
                                        Capture,
                                        DataFormat,
                                        ServiceGroup);
    if (!NT_SUCCESS (ntStatus))
    {
        //
        // Release the stream and clean up.
        //
        DOUT (DBG_ERROR, ("[NewStream] Failed to init stream!"));
        pWaveCyclicStream->Release ();

        return ntStatus;
    }

    //
    // Save the pointers.
    //
    *Stream = (PMINIPORTWAVECYCLICSTREAM)pWaveCyclicStream;
    obj_AddRef(DmaChannel, (PVOID *)DmaChannel_);

    return STATUS_SUCCESS;
}
