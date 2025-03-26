/********************************************************************************
**    Copyright (c) 1998-2000 Microsoft Corporation. All Rights Reserved.
**
**       Portions Copyright (c) 1998-1999 Intel Corporation
**
********************************************************************************/

/* The file rtminiport.cpp was reviewed by LCA in June 2011 and is acceptable for use by Microsoft. */

// Every debug output has "Modulname text"
#define STR_MODULENAME "AC97 RT Miniport: "

#include "rtminiport.h"
#include "rtstream.h"

#if (NTDDI_VERSION >= NTDDI_VISTA)

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif

IMP_CMiniport(CAC97MiniportWaveRT, IID_IMiniportRT)

/*****************************************************************************
 * CreateAC97MiniportWaveRT
 *****************************************************************************
 * Creates a RT miniport object for the AC97 adapter.
 * This uses a macro from STDUNK.H to do all the work.
 */
NTSTATUS CreateAC97MiniportWaveRT
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

    DOUT (DBG_PRINT, ("[CreateMiniportWaveRT]"));

    STD_CREATE_BODY_WITH_TAG_(CAC97MiniportWaveRT,Unknown,UnknownOuter,PoolType,
                     PoolTag, PMINIPORTWAVERT);
}


/*****************************************************************************
 * CAC97MiniportWaveRT::Init
 *****************************************************************************
 * Initializes the miniport.
 * Initializes variables and modifies the wave topology if needed.
 */
STDMETHODIMP_(NTSTATUS) CAC97MiniportWaveRT::Init
(
    _In_  PUNKNOWN      UnknownAdapter,
    _In_  PRESOURCELIST ResourceList,
    _In_  PPORTWAVERT   Port_
)
{
    PAGED_CODE ();

    return CMiniport::Init(
        UnknownAdapter,
        ResourceList,
        Port_
        );
}


/*****************************************************************************
 * CAC97MiniportWaveRT::ProcessResources
 *****************************************************************************
 * Processes the resource list, setting up helper objects accordingly.
 * Sets up the Interrupt + Service routine and DMA.
 */
NTSTATUS CAC97MiniportWaveRT::ProcessResources
(
    IN  PRESOURCELIST ResourceList
)
{
    PAGED_CODE ();

    ASSERT (ResourceList);


    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRT::ProcessResources]"));


    ULONG countIRQ = ResourceList->NumberOfInterrupts ();
    if (countIRQ < 1)
    {
        DOUT (DBG_ERROR, ("Unknown configuration for wave miniport!"));
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    //
    // Create an interrupt sync object
    //
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ntStatus = PcNewInterruptSync (&InterruptSync,
                                   NULL,
                                   ResourceList,
                                   0,
                                   InterruptSyncModeNormal);

    if (!NT_SUCCESS (ntStatus) || !InterruptSync)
    {
        DOUT (DBG_ERROR, ("Failed to create an interrupt sync!"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Register our ISR.
    //
    ntStatus = InterruptSync->RegisterServiceRoutine (InterruptServiceRoutine,
                                                      (PVOID)this, FALSE);
    if (!NT_SUCCESS (ntStatus))
    {
        DOUT (DBG_ERROR, ("Failed to register ISR!"));
        return ntStatus;
    }

    //
    // Connect the interrupt.
    //
    ntStatus = InterruptSync->Connect ();
    if (!NT_SUCCESS (ntStatus))
    {
        DOUT (DBG_ERROR, ("Failed to connect the ISR with InterruptSync!"));
        return ntStatus;
    }

    //
    // On failure object is destroyed which cleans up.
    //
    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAC97MiniportWaveRT::NewStream
 *****************************************************************************
 * Creates a new stream.
 * This function is called when a streaming pin is created.
 * It checks if the channel is already in use, tests the data format, creates
 * and initializes the stream object.
 */
STDMETHODIMP CAC97MiniportWaveRT::NewStream
(
    _Out_ PMINIPORTWAVERTSTREAM       *Stream,
    _In_  PPORTWAVERTSTREAM           PortStream,
    _In_  ULONG                   Channel_,
    _In_  BOOLEAN                 Capture,
    _In_  PKSDATAFORMAT           DataFormat
)
{
    PAGED_CODE ();

    ASSERT (Stream);
    ASSERT (PortStream);
    ASSERT (DataFormat);

    CAC97MiniportWaveRTStream *pStream = NULL;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRT::NewStream]"));

    //
    // Check parameters.
    //
    ntStatus = ValidateFormat (DataFormat, (WavePins)Channel_);
    if (!NT_SUCCESS (ntStatus))
    {
        return ntStatus;
    }

    //
    // Create a new stream.
    //
    ntStatus = CreateAC97MiniportWaveRTStream (&pStream);

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
    ntStatus = pStream->Init (this,
                              PortStream,
                              Channel,
                              Capture,
                              DataFormat);
    if (!NT_SUCCESS (ntStatus))
    {
        //
        // Release the stream and clean up.
        //
        DOUT (DBG_ERROR, ("[NewStream] Failed to init stream!"));
        pStream->Release ();
        *Stream = NULL;
        return ntStatus;
    }

    //
    // Save the pointers.
    //
    *Stream = (PMINIPORTWAVERTSTREAM)pStream;

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CAC97MiniportWaveRT::GetDeviceDescription
 *****************************************************************************
 * Gets the topology.
 */
STDMETHODIMP_(NTSTATUS) CAC97MiniportWaveRT::GetDeviceDescription
(
    _Out_ PDEVICE_DESCRIPTION   DmaDeviceDescription
)
{
    PAGED_CODE ();

    ASSERT (DmaDeviceDescription);

    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRT::GetDeviceDescription]"));

    RtlZeroMemory (DmaDeviceDescription, sizeof (DEVICE_DESCRIPTION));
    DmaDeviceDescription->Master = TRUE;
    DmaDeviceDescription->ScatterGather = TRUE;
    DmaDeviceDescription->Dma32BitAddresses = TRUE;
    DmaDeviceDescription->InterfaceType = PCIBus;
    DmaDeviceDescription->MaximumLength = 0x1FFFE;

    return STATUS_SUCCESS;
}



#endif

