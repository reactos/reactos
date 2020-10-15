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

IMP_CMiniport(CMiniportWaveICH)

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
 * CMiniportWaveICH::NonDelegatingQueryInterface
 *****************************************************************************
 * Obtains an interface.  This function works just like a COM QueryInterface
 * call and is used if the object is not being aggregated.
 */
STDMETHODIMP_(NTSTATUS) CMiniportWaveICH::NonDelegatingQueryInterface
(
    _In_         REFIID  Interface,
    _COM_Outptr_ PVOID  *Object
)
{
    PAGED_CODE ();

    ASSERT (Object);

    DOUT (DBG_PRINT, ("[CMiniportWaveICH::NonDelegatingQueryInterface]"));

    // Is it IID_IUnknown?
    if (IsEqualGUIDAligned (Interface, IID_IUnknown))
    {
        *Object = (PVOID)(PUNKNOWN)(PMINIPORTWAVEPCI)this;
    }
    // or IID_IMiniport ...
    else if (IsEqualGUIDAligned (Interface, IID_IMiniport))
    {
        *Object = (PVOID)(PMINIPORT)this;
    }
    // or IID_IMiniportWavePci ...
    else if (IsEqualGUIDAligned (Interface, IID_IMiniportWavePci))
    {
        *Object = (PVOID)(PMINIPORTWAVEPCI)this;
    }
    // or IID_IPowerNotify ...
    else if (IsEqualGUIDAligned (Interface, IID_IPowerNotify))
    {
        *Object = (PVOID)(PPOWERNOTIFY)this;
    }
    else
    {
        // nothing found, must be an unknown interface.
        *Object = NULL;
        return STATUS_INVALID_PARAMETER;
    }

    //
    // We reference the interface for the caller.
    //
    ((PUNKNOWN)(*Object))->AddRef();
    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CMiniportWaveICH::~CMiniportWaveICH
 *****************************************************************************
 * Destructor.
 */
CMiniportWaveICH::~CMiniportWaveICH ()
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CMiniportWaveICH::~CMiniportWaveICH]"));

    //
    // Release the DMA channel.
    //
    if (DmaChannel)
    {
        DmaChannel->Release ();
        DmaChannel = NULL;
    }
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

    return ProcessResources();
}

/*****************************************************************************
 * CMiniportWaveICH::ProcessResources
 *****************************************************************************
 * Processes the resource list, setting up helper objects accordingly.
 * Sets up the Interrupt + Service routine and DMA.
 */
NTSTATUS CMiniportWaveICH::ProcessResources
(
)
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CMiniportWaveICH::ProcessResources]"));

    //
    // Create the DMA Channel object.
    //
    NTSTATUS ntStatus = Port->NewMasterDmaChannel (&DmaChannel,      // OutDmaChannel
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
    // Get the DMA adapter.
    //
    AdapterObject = (PDMA_ADAPTER)DmaChannel->GetAdapterObject ();

    //
    // On failure object is destroyed which cleans up.
    //
    return STATUS_SUCCESS;
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
    ULONG                   Channel_,
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
    // Validate the channel (pin id).
    //
    if ((Channel_ != PIN_WAVEOUT) && (Channel_ != PIN_WAVEIN) &&
       (Channel_ != PIN_MICIN))
    {
        DOUT (DBG_ERROR, ("[NewStream] Invalid channel passed!"));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Check if the pin is already in use
    //
    ULONG Channel = Channel_ >> 1;
    if (Streams[Channel])
    {
        DOUT (DBG_ERROR, ("[NewStream] Pin is already in use!"));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Check parameters.
    //
    ntStatus = TestDataFormat (DataFormat, (WavePins)Channel_);
    if (!NT_SUCCESS (ntStatus))
    {
        DOUT (DBG_VSR, ("[NewStream] TestDataFormat failed!"));
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
    ntStatus = pWaveICHStream->Init (this,
                                    PortStream,
                                    Channel,
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
        // In case the stream passed us a ServiceGroup, portcls will ignore all parameters
        // on a failure, so we have to release it here.
        if (*ServiceGroup)
            (*ServiceGroup)->Release();
        *ServiceGroup = NULL;
        *Stream = NULL;
        *DmaChannel_ = NULL;
        return ntStatus;
    }

    //
    // Save the pointers.
    //
    *Stream = (PMINIPORTWAVEPCISTREAM)pWaveICHStream;
    *DmaChannel_ = DmaChannel;


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
