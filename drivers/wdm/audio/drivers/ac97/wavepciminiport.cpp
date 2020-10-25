/********************************************************************************
**    Copyright (c) 1998-2000 Microsoft Corporation. All Rights Reserved.
**
**       Portions Copyright (c) 1998-1999 Intel Corporation
**
********************************************************************************/

/* The file wavepciminiport.cpp was reviewed by LCA in June 2011 and is acceptable for use by Microsoft. */

// Every debug output has "Modulname text"
#define STR_MODULENAME "AC97 Wave: "

#include "wavepciminiport.h"
#include "wavepcistream.h"


#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif

IMP_CMiniport(CMiniportWaveICH, IID_IMiniportWavePci)

/*****************************************************************************
 * CreateAC97MiniportWavePCI
 *****************************************************************************
 * Creates a AC97 wave miniport object for the AC97 adapter.
 * This uses a macro from STDUNK.H to do all the work.
 */
NTSTATUS CreateAC97MiniportWavePCI
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

    DOUT (DBG_PRINT, ("[CreateAC97MiniportWavePCI]"));

    STD_CREATE_BODY_WITH_TAG_(CMiniportWaveICH,Unknown,UnknownOuter,PoolType,
                     PoolTag, PMINIPORTWAVEPCI);
}

/*****************************************************************************
 * CMiniportWaveICH::Init
 *****************************************************************************
 * Initializes the miniport.
 * Initializes variables and modifies the wave topology if needed.
 */
STDMETHODIMP_(NTSTATUS) CMiniportWaveICH::Init
(
    _In_  PUNKNOWN       UnknownAdapter,
    _In_  PRESOURCELIST  ResourceList,
    _In_  PPORTWAVEPCI   Port_,
    _Out_ PSERVICEGROUP *ServiceGroup_
)
{
    PAGED_CODE ();

    //
    // No miniport service group
    //
    *ServiceGroup_ = NULL;

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
                                          NonPagedPool,     // Pool Type
                                          NULL,             // ResourceList (opt)
                                          TRUE,             // ScatterGather
                                          TRUE,             // Dma32BitAddresses
                                          FALSE,            // Dma64BitAddresses
                                          FALSE,            // IgnoreCount
                                          Width32Bits,      // DmaWidth
                                          MaximumDmaSpeed,  // DmaSpeed
                                          0x1FFFE,          // MaximumLength (128KByte -2)
                                          0);               // DmaPort
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
 * CMiniportWaveICH::NewStream
 *****************************************************************************
 * Creates a new stream.
 * This function is called when a streaming pin is created.
 * It checks if the channel is already in use, tests the data format, creates
 * and initializes the stream object.
 */
_Use_decl_annotations_
STDMETHODIMP CMiniportWaveICH::NewStream
(
    PMINIPORTWAVEPCISTREAM *Stream,
    PUNKNOWN                OuterUnknown,
    POOL_TYPE               PoolType,
    PPORTWAVEPCISTREAM      PortStream,
    ULONG                   Pin_,
    BOOLEAN                 Capture,
    PKSDATAFORMAT           DataFormat,
    PDMACHANNEL            *DmaChannel_,
    PSERVICEGROUP          *ServiceGroup
)
{
    PAGED_CODE ();

    ASSERT (Stream);
    ASSERT (PortStream);
    ASSERT (DataFormat);
    ASSERT (DmaChannel_);
    ASSERT (ServiceGroup);

    CMiniportWaveICHStream *pWaveICHStream = NULL;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    DOUT (DBG_PRINT, ("[CMiniportWaveICH::NewStream]"));

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
    ntStatus = CreateMiniportWaveICHStream (&pWaveICHStream, OuterUnknown,
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
    ntStatus = pWaveICHStream->Init ((CMiniport*)this,
                                    PortStream,
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
        pWaveICHStream->Release ();

        return ntStatus;
    }

    //
    // Save the pointers.
    //
    *Stream = (PMINIPORTWAVEPCISTREAM)pWaveICHStream;
    obj_AddRef(DmaChannel, (PVOID *)DmaChannel_);

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * Non paged code begins here
 *****************************************************************************
 */

#ifdef _MSC_VER
#pragma code_seg()
#endif
/*****************************************************************************
 * CMiniportWaveICH::Service
 *****************************************************************************
 * Processing routine for dealing with miniport interrupts.  This routine is
 * called at DISPATCH_LEVEL.
 */
STDMETHODIMP_(void) CMiniportWaveICH::Service (void)
{
    // not needed
}
