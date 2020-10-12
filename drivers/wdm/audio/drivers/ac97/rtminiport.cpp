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
 * CAC97MiniportWaveRT::NonDelegatingQueryInterface
 *****************************************************************************
 * Obtains an interface.  This function works just like a COM QueryInterface
 * call and is used if the object is not being aggregated.
 */
STDMETHODIMP_(NTSTATUS) CAC97MiniportWaveRT::NonDelegatingQueryInterface
(
    _In_         REFIID  Interface,
    _COM_Outptr_ PVOID  *Object
)
{
    PAGED_CODE ();

    ASSERT (Object);

    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRT::NonDelegatingQueryInterface]"));

    // Is it IID_IUnknown?
    if (IsEqualGUIDAligned (Interface, IID_IUnknown))
    {
        *Object = (PVOID)(PUNKNOWN)(PMINIPORTWAVERT)this;
    }
    // or IID_IMiniport ...
    else if (IsEqualGUIDAligned (Interface, IID_IMiniport))
    {
        *Object = (PVOID)(PMINIPORT)this;
    }
    // or IID_IMiniportWaveRT ...
    else if (IsEqualGUIDAligned (Interface, IID_IMiniportWaveRT))
    {
        *Object = (PVOID)(PMINIPORTWAVERT)this;
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
        Port_,
        InterruptServiceRoutine
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


/*****************************************************************************
 * Non paged code begins here
 *****************************************************************************
 */

#ifdef _MSC_VER
#pragma code_seg()
#endif
/*****************************************************************************
 * InterruptServiceRoutine
 *****************************************************************************
 * The task of the ISR is to clear an interrupt from this device so we don't
 * get an interrupt storm and schedule a DPC which actually does the
 * real work.
 */
NTSTATUS CAC97MiniportWaveRT::InterruptServiceRoutine
(
    IN  PINTERRUPTSYNC  InterruptSync,
    IN  PVOID           DynamicContext
)
{
    UNREFERENCED_PARAMETER(InterruptSync);

    ASSERT (InterruptSync);
    ASSERT (DynamicContext);

    ULONG   GlobalStatus;
    USHORT  DMAStatusRegister;

    //
    // Get our context which is a pointer to class CAC97MiniportWaveRT.
    //
    CAC97MiniportWaveRT *that = (CAC97MiniportWaveRT *)DynamicContext;

    //
    // Check for a valid AdapterCommon pointer.
    //
    if (!that->AdapterCommon)
    {
        //
        // In case we didn't handle the interrupt, unsuccessful tells the system
        // to call the next interrupt handler in the chain.
        //
        return STATUS_UNSUCCESSFUL;
    }

    //
    // From this point down, basically in the complete ISR, we cannot use
    // relative addresses (stream class base address + X_CR for example)
    // cause we might get called when the stream class is destroyed or
    // not existent. This doesn't make too much sense (that there is an
    // interrupt for a non-existing stream) but could happen and we have
    // to deal with the interrupt.
    //

    //
    // Read the global register to check the interrupt bits
    //
    GlobalStatus = that->AdapterCommon->ReadBMControlRegister32 (GLOB_STA);

    //
    // Check for weird return values. Could happen if the PCI device is already
    // disabled and another device that shares this interrupt generated an
    // interrupt.
    // The register should never have all bits cleared or set.
    //
    if (!GlobalStatus || (GlobalStatus == 0xFFFFFFFF))
    {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Check for PCM out interrupt.
    //
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    if (GlobalStatus & GLOB_STA_POINT)
    {
        //
        // Read PCM out DMA status registers.
        //
        DMAStatusRegister = (USHORT)that->AdapterCommon->
            ReadBMControlRegister16 (PO_SR);


        //
        // We could now check for every possible error condition
        // (like FIFO error) and monitor the different errors, but currently
        // we have the same action for every INT and therefore we simplify
        // this routine enormous with just clearing the bits.
        //
        if (that->Streams[PIN_WAVEOUT_OFFSET])
        {
            //
            // ACK the interrupt.
            //
            that->AdapterCommon->WriteBMControlRegister (PO_SR, DMAStatusRegister);
            ntStatus = STATUS_SUCCESS;


            //
            // Update the LVI so that we cycle around in the scatter gather list.
            //
            UCHAR CIV = that->AdapterCommon->ReadBMControlRegister8 (PO_CIV);
            that->AdapterCommon->WriteBMControlRegister (PO_LVI, (UCHAR)((CIV-1) & BDL_MASK));
        }
    }

    //
    // Check for PCM in interrupt.
    //
    if (GlobalStatus & GLOB_STA_PIINT)
    {
        //
        // Read PCM in DMA status registers.
        //
        DMAStatusRegister = (USHORT)that->AdapterCommon->
            ReadBMControlRegister16 (PI_SR);

        //
        // We could now check for every possible error condition
        // (like FIFO error) and monitor the different errors, but currently
        // we have the same action for every INT and therefore we simplify
        // this routine enormous with just clearing the bits.
        //
        if (that->Streams[PIN_WAVEIN_OFFSET])
        {
            //
            // ACK the interrupt.
            //
            that->AdapterCommon->WriteBMControlRegister (PI_SR, DMAStatusRegister);
            ntStatus = STATUS_SUCCESS;


            //
            // Update the LVI so that we cycle around in the scatter gather list.
            //
            UCHAR CIV = that->AdapterCommon->ReadBMControlRegister8 (PI_CIV);
            that->AdapterCommon->WriteBMControlRegister (PI_LVI, (UCHAR)((CIV-1) & BDL_MASK));
        }
    }

    //
    // Check for MIC in interrupt.
    //
    if (GlobalStatus & GLOB_STA_MINT)
    {
        //
        // Read MIC in DMA status registers.
        //
        DMAStatusRegister = (USHORT)that->AdapterCommon->
            ReadBMControlRegister16 (MC_SR);

        //
        // We could now check for every possible error condition
        // (like FIFO error) and monitor the different errors, but currently
        // we have the same action for every INT and therefore we simplify
        // this routine enormous with just clearing the bits.
        //
        if (that->Streams[PIN_MICIN_OFFSET])
        {
            //
            // ACK the interrupt.
            //
            that->AdapterCommon->WriteBMControlRegister (MC_SR, DMAStatusRegister);
            ntStatus = STATUS_SUCCESS;


            //
            // Update the LVI so that we cycle around in the scatter gather list.
            //
            UCHAR CIV = that->AdapterCommon->ReadBMControlRegister8 (MC_CIV);
            that->AdapterCommon->WriteBMControlRegister (MC_LVI, (UCHAR)((CIV-1) & BDL_MASK));
        }
    }

    return ntStatus;
}

#endif

