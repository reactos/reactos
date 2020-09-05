/********************************************************************************
**    Copyright (c) 1998-2000 Microsoft Corporation. All Rights Reserved.
**
**       Portions Copyright (c) 1998-1999 Intel Corporation
**
********************************************************************************/

/* The file rtstream.cpp was reviewed by LCA in June 2011 and is acceptable for use by Microsoft. */

// Every debug output has "Modulname text"
static char STR_MODULENAME[] = "AC97 RT Stream: ";

#include "rtminiport.h"
#include "rtstream.h"

#if (NTDDI_VERSION >= NTDDI_VISTA)

/*****************************************************************************
 * General Info
 *****************************************************************************
 * To protect the stBDList structure that is used to store mappings, we use a
 * spin lock called MapLock. This spin lock is also acquired when we change
 * the DMA registers. Normally, changes in stBDList and the DMA registers go
 * hand in hand. In case we only want to change the DMA registers, we need
 * to acquire the spin lock!
 */

#pragma code_seg("PAGE")
/*****************************************************************************
 * CreateAC97MiniportWaveRTStream
 *****************************************************************************
 * Creates a wave miniport stream object for the AC97 audio adapter. This is
 * (nearly) like the macro STD_CREATE_BODY_ from STDUNK.H.
 */
NTSTATUS CreateAC97MiniportWaveRTStream
(
    OUT CAC97MiniportWaveRTStream  **RTStream
)
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CreateAC97MiniportWaveRTStream]"));

    //
    // This is basically like the macro at stdunk with the change that we
    // don't cast to interface unknown but to interface CAC97MiniportWaveRTStream.
    //
    *RTStream = new (NonPagedPool, PoolTag) CAC97MiniportWaveRTStream (NULL);
    if (*RTStream)
    {
        (*RTStream)->AddRef ();
        return STATUS_SUCCESS;
    }

    return STATUS_INSUFFICIENT_RESOURCES;
}


/*****************************************************************************
 * CAC97MiniportWaveRTStream::~CAC97MiniportWaveRTStream
 *****************************************************************************
 * Destructor
 */
CAC97MiniportWaveRTStream::~CAC97MiniportWaveRTStream ()
{
    PAGED_CODE ();


    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRTStream::~CAC97MiniportWaveRTStream]"));

    if (Miniport)
    {
        //
        // Disable interrupts and stop DMA just in case.
        //
        if (Miniport->AdapterCommon)
        {
            Miniport->AdapterCommon->WriteBMControlRegister (m_ulBDAddr + X_CR, (UCHAR)0);

            //
            // Update also the topology miniport if this was the render stream.
            //
            if (Miniport->AdapterCommon->GetMiniportTopology () &&
                (Channel == PIN_WAVEOUT_OFFSET))
            {
                Miniport->AdapterCommon->GetMiniportTopology ()->SetCopyProtectFlag (FALSE);
            }
        }

        //
        // Remove stream from miniport Streams array.
        //
        if (Miniport->Streams[Channel] == this)
        {
            Miniport->Streams[Channel] = NULL;
        }

        //
        // Release the miniport.
        //
        Miniport->Release ();
        Miniport = NULL;
    }

    //
    // Delete the scatter gather list since it's not needed anymore
    //
    if (BDListMdl && BDList)
    {
        PortStream->UnmapAllocatedPages (BDList, BDListMdl);
        PortStream->FreePagesFromMdl (BDListMdl);
        BDListMdl = NULL;
        BDList = NULL;
    }
    if (BDList)
    {
        ExFreePool (BDList);
    }

    //
    // Release the port stream.
    //
    if (PortStream)
    {
        PortStream->Release ();
        PortStream = NULL;
    }
}


/*****************************************************************************
 * CAC97MiniportWaveRTStream::Init
 *****************************************************************************
 * This routine initializes the stream object & allocates the BDL.
 * It doesn't allocate the audio buffer or initialize the BDL.
 */
NTSTATUS CAC97MiniportWaveRTStream::Init
(
    IN  CAC97MiniportWaveRT     *Miniport_,
    IN  PPORTWAVERTSTREAM    PortStream_,
    IN  ULONG            Channel_,
    IN  BOOLEAN          Capture_,
    IN  PKSDATAFORMAT    DataFormat_
)
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRTStream::Init]"));

    ASSERT (Miniport_);
    ASSERT (PortStream_);
    ASSERT (DataFormat_);

    //
    // The rule here is that we return when we fail without a cleanup.
    // The destructor will relase the allocated memory.
    //
    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // Save miniport pointer and addref it.
    //
    Miniport = Miniport_;
    Miniport->AddRef ();

    //
    // Save portstream interface pointer and addref it.
    //
    PortStream = PortStream_;
    PortStream->AddRef ();

    //
    // Save channel ID and capture flag.
    //
    Channel = Channel_;
    Capture = Capture_;

    //
    // Save data format and current sample rate.
    //
    DataFormat = (PKSDATAFORMAT_WAVEFORMATEX)DataFormat_;
    CurrentRate = DataFormat->WaveFormatEx.nSamplesPerSec;
    NumberOfChannels = DataFormat->WaveFormatEx.nChannels;

    //
    // Allocate memory for the BDL.
    // First try the least expensive way, which is to allocate it from the pool.
    // If that fails (it's outside of the controller's address range which can
    // happen on 64bit machines or PAE) then use portcls's AllocatePagesForMdl.
    //
    BDList = (tBDEntry *)ExAllocatePoolWithTag (NonPagedPool,
                        MAX_BDL_ENTRIES * sizeof (tBDEntry), PoolTag);
    if (!BDList)
    {
        DOUT (DBG_ERROR, ("Failed to allocate the BD list!"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Check to see if our HW can access it.
    // If the HW cannot see the memory, free it and use AllocatePagesForMdl
    // which allocates always complete pages, so we have to waste some memory.
    //
    if (MmGetPhysicalAddress (BDList).HighPart != 0)
    {
      PHYSICAL_ADDRESS  high;

      high.HighPart = 0;
      high.LowPart = MAXULONG;
      ExFreePool (BDList);
      BDListMdl = PortStream->AllocatePagesForMdl (high, PAGE_SIZE);
      if (!BDListMdl)
      {
          DOUT (DBG_ERROR, ("Failed to allocate page for BD list!"));
          return STATUS_INSUFFICIENT_RESOURCES;
      }
      BDList = (tBDEntry *)PortStream->MapAllocatedPages (BDListMdl, MmCached);
      if (!BDList)
      {
          PortStream->FreePagesFromMdl (BDListMdl);
          BDListMdl = NULL;
          DOUT (DBG_ERROR, ("Failed to map the page for the BD list!"));
          return STATUS_INSUFFICIENT_RESOURCES;
      }
    }

    //
    // Store the base address of this DMA engine.
    //
    if (Capture)
    {
        //
        // could be PCM or MIC capture
        //
        if (Channel == PIN_WAVEIN_OFFSET)
        {
            // Base address for DMA registers.
            m_ulBDAddr = PI_BDBAR;
        }
        else
        {
            // Base address for DMA registers.
            m_ulBDAddr = MC_BDBAR;
        }
    }
    else    // render
    {
        // Base address for DMA registers.
        m_ulBDAddr = PO_BDBAR;
    }

    //
    // Reset the DMA and set the BD list pointer.
    //
    ResetDMA ();

    //
    // Now set the requested sample rate. In case of a failure, the object
    // gets destroyed and releases all memory etc.
    //
    ntStatus = SetFormat (DataFormat_);
    if (!NT_SUCCESS (ntStatus))
    {
        DOUT (DBG_ERROR, ("Stream init SetFormat call failed!"));
        return ntStatus;
    }

    //
    // Initialize the device state.
    //
    m_PowerState = PowerDeviceD0;


    //
    // Store the stream pointer, it is used by the ISR.
    //
    Miniport->Streams[Channel] = this;

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAC97MiniportWaveRTStream::NonDelegatingQueryInterface
 *****************************************************************************
 * Obtains an interface.  This function works just like a COM QueryInterface
 * call and is used if the object is not being aggregated.
 */
STDMETHODIMP_(NTSTATUS) CAC97MiniportWaveRTStream::NonDelegatingQueryInterface
(
    _In_         REFIID  Interface,
    _COM_Outptr_ PVOID * Object
)
{
    PAGED_CODE ();

    ASSERT (Object);

    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRTStream::NonDelegatingQueryInterface]"));

    //
    // Convert for IID_IMiniportWaveRTStream
    //
    if (IsEqualGUIDAligned (Interface, IID_IMiniportWaveRTStream))
    {
        *Object = (PVOID)(PMINIPORTWAVERTSTREAM)this;
    }
    //
    // Convert for IID_IDrmAudioStream
    //
    else if (IsEqualGUIDAligned (Interface, IID_IDrmAudioStream))
    {
        *Object = (PVOID)(PDRMAUDIOSTREAM)this;
    }
    //
    // Convert for IID_IUnknown
    //
    else if (IsEqualGUIDAligned (Interface, IID_IUnknown))
    {
        *Object = (PVOID)(PUNKNOWN)(PMINIPORTWAVERTSTREAM)this;
    }
    else
    {
        *Object = NULL;
        return STATUS_INVALID_PARAMETER;
    }

    ((PUNKNOWN)*Object)->AddRef ();
    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAC97MiniportWaveRTStream::SetFormat
 *****************************************************************************
 * This routine tests for proper data format (calls wave miniport) and sets
 * or changes the stream data format.
 * To figure out if the codec supports the sample rate, we just program the
 * sample rate and read it back. If it matches we return happy, if not then
 * we restore the sample rate and return unhappy.
 * We fail this routine if we are currently running (playing or recording).
 */
STDMETHODIMP_(NTSTATUS) CAC97MiniportWaveRTStream::SetFormat
(
    _In_  PKSDATAFORMAT   Format
)
{
    PAGED_CODE ();

    ASSERT (Format);

    ULONG   TempRate;
    DWORD   dwControlReg;

    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRTStream::SetFormat]"));

    //
    // Change sample rate when we are in the stop or pause states - not
    // while running!
    //
    if (DMAEngineState == DMA_ENGINE_ON)
    {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Ensure format falls in proper range and is supported.
    //
    NTSTATUS ntStatus = Miniport->TestDataFormat (Format, (WavePins)(Channel << 1));
    if (!NT_SUCCESS (ntStatus))
        return ntStatus;

    //
    // Retrieve wave format portion.
    //
    PWAVEFORMATPCMEX waveFormat = (PWAVEFORMATPCMEX)(Format + 1);

    //
    // Save current rate in this context.
    //
    TempRate = waveFormat->Format.nSamplesPerSec;

    //
    // Check if we have a codec with one sample rate converter and there are streams
    // already open.
    //
    if (Miniport->Streams[PIN_WAVEIN_OFFSET] && Miniport->Streams[PIN_WAVEOUT_OFFSET] &&
        !Miniport->AdapterCommon->GetNodeConfig (NODEC_PCM_VSR_INDEPENDENT_RATES))
    {
        //
        // Figure out at which sample rate the other stream is running.
        //
        ULONG   ulFrequency;

        if (Miniport->Streams[PIN_WAVEIN_OFFSET] == this)
            ulFrequency = Miniport->Streams[PIN_WAVEOUT_OFFSET]->CurrentRate;
        else
            ulFrequency = Miniport->Streams[PIN_WAVEIN_OFFSET]->CurrentRate;

        //
        // Check if this sample rate is requested sample rate.
        //
        if (ulFrequency != TempRate)
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    //
    // Program the AC97 to support n channels.
    //
    if (Channel == PIN_WAVEOUT_OFFSET)
    {
        dwControlReg = Miniport->AdapterCommon->ReadBMControlRegister32 (GLOB_CNT);
        dwControlReg = (dwControlReg & 0x03F) |
                       (((waveFormat->Format.nChannels >> 1) - 1) * GLOB_CNT_PCM4);
        Miniport->AdapterCommon->WriteBMControlRegister (GLOB_CNT, dwControlReg);
    }

    //
    // Check for rate support by hardware.  If it is supported, then update
    // hardware registers else return not implemented and audio stack will
    // handle it.
    //
    if (Capture)
    {
        if (Channel == PIN_WAVEIN_OFFSET)
        {
            ntStatus = Miniport->AdapterCommon->
                ProgramSampleRate (AC97REG_RECORD_SAMPLERATE, TempRate);
        }
        else
        {
            ntStatus = Miniport->AdapterCommon->
                ProgramSampleRate (AC97REG_MIC_SAMPLERATE, TempRate);
        }
    }
    else
    {
        //
        // In the playback case we might need to update several DACs
        // with the new sample rate.
        //
        ntStatus = Miniport->AdapterCommon->
            ProgramSampleRate (AC97REG_FRONT_SAMPLERATE, TempRate);

        if (Miniport->AdapterCommon->GetNodeConfig (NODEC_SURROUND_DAC_PRESENT))
        {
            ntStatus = Miniport->AdapterCommon->
                ProgramSampleRate (AC97REG_SURROUND_SAMPLERATE, TempRate);
        }
        if (Miniport->AdapterCommon->GetNodeConfig (NODEC_LFE_DAC_PRESENT))
        {
            ntStatus = Miniport->AdapterCommon->
                ProgramSampleRate (AC97REG_LFE_SAMPLERATE, TempRate);
        }
    }

    if (NT_SUCCESS (ntStatus))
    {
        //
        // print information and save the format information.
        //
        DataFormat = (PKSDATAFORMAT_WAVEFORMATEX)Format;
        CurrentRate = TempRate;
        NumberOfChannels = waveFormat->Format.nChannels;
    }

    return ntStatus;
}

/*****************************************************************************
 * CAC97MiniportWaveRTStream::SetContentId
 *****************************************************************************
 * This routine gets called by drmk.sys to pass the content to the driver.
 * The driver has to enforce the rights passed.
 */
STDMETHODIMP_(NTSTATUS) CAC97MiniportWaveRTStream::SetContentId
(
    _In_  ULONG       contentId,
    _In_  PCDRMRIGHTS drmRights
)
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRTStream::SetContentId]"));

    UNREFERENCED_PARAMETER(contentId);

    //
    // If "drmRights->DigitalOutputDisable" is set, we need to disable S/P-DIF.
    // Currently, we don't have knowledge about the S/P-DIF interface. However,
    // in case you expanded the driver with S/P-DIF features you need to disable
    // S/P-DIF or fail SetContentId. If you have HW that has S/P-DIF turned on
    // by default and you don't know how to turn off (or you cannot do that)
    // then you must fail SetContentId.
    //
    // In our case, we assume the codec has no S/P-DIF or disabled S/P-DIF by
    // default, so we can ignore the flag.
    //
    // Store the copyright flag. We have to disable PCM recording if it's set.
    //
    if (!Miniport->AdapterCommon->GetMiniportTopology ())
    {
        DOUT (DBG_ERROR, ("Topology pointer not set!"));
        return STATUS_UNSUCCESSFUL;
    }
    else
    {
        Miniport->AdapterCommon->GetMiniportTopology ()->
            SetCopyProtectFlag (drmRights->CopyProtect);
    }

    //
    // We assume that if we can enforce the rights, that the old content
    // will be destroyed. We don't need to store the content id since we
    // have only one playback channel, so we are finished here.
    //

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CAC97MiniportWaveRTStream::AllocateAudioBuffer
 *****************************************************************************
 * This functions allocates an audio buffer of the size specified and maps
 * it into the scatter gather table of the AC97 DMA engine.
 * Once audio is played the driver only changes the last valid index to make
 * the DMA cycle through this buffer over and over again.
 * The buffer needs to be freed when the stream gets destroyed.
 */
STDMETHODIMP_(NTSTATUS) CAC97MiniportWaveRTStream::AllocateAudioBuffer
(
    _In_  ULONG               size,
    _Out_ PMDL                *userModeBuffer,
    _Out_ ULONG               *bufferSize,
    _Out_ ULONG               *bufferOffset,
    _Out_ MEMORY_CACHING_TYPE *cacheType
)
{
    PAGED_CODE ();

    //
    // Make sure complete samples fit into the buffer.
    //
    if( size <= size % (NumberOfChannels * 2) )
    {
        return STATUS_UNSUCCESSFUL;
    }
    size -= size % (NumberOfChannels * 2);

    //
    // Validate that we're going to actually allocate a real amount of memory.
    //
    if (0 == size)
    {
        DOUT (DBG_WARNING, ("Zero byte memory allocation attempted."));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Allocate the buffer.
    // The AC97 has problems playing 6ch data on page breaks (a page is 4096 bytes
    // and doesn't contain complete samples of 6ch 16bit audio data). We therefore
    // allocate contiguous memory that fits complete 6ch 16bit samples, however,
    // contiguous memory is a lot more expensive to get and you might not get it
    // at all. Useing non-contiguous memory (AllocatePagesForMdl) is therefore much
    // better if your HW does support it. It is highly recommended to build future
    // HW so that it can map a variable amount of pages, that it can cycle through
    // the scatter gather list automatically and that it handles that case where
    // samples are "split" between 2 pages.
    //
    PHYSICAL_ADDRESS    low;
    PHYSICAL_ADDRESS    high;

    low.QuadPart = 0;
    high.HighPart = 0, high.LowPart = MAXULONG;
    PMDL audioBufferMdl = PortStream->AllocateContiguousPagesForMdl (low, high, size);

    //
    // Check if the allocation was successful.
    //
    if (!audioBufferMdl)
    {
        DOUT (DBG_WARNING, ("[AllocateAudioBuffer] Can not allocate RT buffer."));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // We got our memory. Program the BDL (scatter gather list) now.
    //
    //
    // Note that when you use AllocatePagesForMdl that you might get less memory
    // back. In this case you need to check the byte count of the Mdl and continue
    // with that size.
    //

    //
    // Store the information for portcls so that the buffer can be mapped to
    // the client.
    //
    *userModeBuffer = audioBufferMdl;
    *bufferSize = size;
    *bufferOffset = 0;
    *cacheType = MmCached;

    //
    // Program the BDL
    //
    for (UINT loop = 0; loop < MAX_BDL_ENTRIES; loop++)
    {
        BDList[loop].dwPtrToPhyAddress = PortStream->GetPhysicalPageAddress (audioBufferMdl, 0).LowPart;
        BDList[loop].wLength = (WORD)size/2;
        if ((loop == MAX_BDL_ENTRIES / 2) || (loop == MAX_BDL_ENTRIES - 1))
            BDList[loop].wPolicyBits = IOC_ENABLE;
        else
            BDList[loop].wPolicyBits = 0;
    }

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CAC97MiniportWaveRTStream::FreeAudioBuffer
 *****************************************************************************
 * This functions frees the previously allocated audio buffer. We don't do
 * anything special here. This callback is mainly in the case you would have
 * to reprogram HW or do something fancy with it. In our case we just delete
 * the audio buffer MDL and the scatter gather list we allocated.
 */
_Use_decl_annotations_
STDMETHODIMP_(VOID) CAC97MiniportWaveRTStream::FreeAudioBuffer
(
      PMDL                Mdl,
      ULONG               Size
)
{
    PAGED_CODE ();

    UNREFERENCED_PARAMETER(Size);

    //
    // Just delete the MDL that was allocated with AllocateContiguousPagesForMdl.
    //
    if (NULL != Mdl)
    {
        PortStream->FreePagesFromMdl (Mdl);
    }
}

/*****************************************************************************
 * CAC97MiniportWaveRTStream::GetHWLatency
 *****************************************************************************
 * Returns the HW latency of the controller + codec.
 */
STDMETHODIMP_(void) CAC97MiniportWaveRTStream::GetHWLatency
(
    _Out_ PKSRTAUDIO_HWLATENCY    hwLatency
)
{
    PAGED_CODE ();

    hwLatency->FifoSize = 32;       // 32 bytes I think
    hwLatency->ChipsetDelay = 0;    // PCI
    hwLatency->CodecDelay = 4;      // Wild guess. Take maximum.
}

/*****************************************************************************
 * CAC97MiniportWaveRTStream::GetPositionRegister
 *****************************************************************************
 * We can't support this property b/c we don't have a memory mapped position
 * register.
 */
STDMETHODIMP_(NTSTATUS) CAC97MiniportWaveRTStream::GetPositionRegister
(
    _Out_ PKSRTAUDIO_HWREGISTER   hwRegister
)
{
    PAGED_CODE ();

    UNREFERENCED_PARAMETER(hwRegister);

    return STATUS_UNSUCCESSFUL;
}

/*****************************************************************************
 * CAC97MiniportWaveRTStream::GetClockRegister
 *****************************************************************************
 * We can't support this property b/c we don't have a memory mapped clock
 * register.
 */
STDMETHODIMP_(NTSTATUS) CAC97MiniportWaveRTStream::GetClockRegister
(
    _Out_ PKSRTAUDIO_HWREGISTER   hwRegister
)
{
    PAGED_CODE ();

    UNREFERENCED_PARAMETER(hwRegister);

    return STATUS_UNSUCCESSFUL;
}


/*****************************************************************************
 * Non paged code begins here
 *****************************************************************************
 */

#pragma code_seg()
/*****************************************************************************
 * CAC97MiniportWaveRTStream::PowerChangeNotify
 *****************************************************************************
 * This functions saves and maintains the stream state through power changes.
 */
NTSTATUS CAC97MiniportWaveRTStream::PowerChangeNotify
(
    IN  POWER_STATE NewState
)
{
    NTSTATUS    ntStatus = STATUS_SUCCESS;

    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRTStream::PowerChangeNotify]"));

    //
    // We don't have to check the power state, that's already done by the wave
    // miniport.
    //

    DOUT (DBG_POWER, ("Changing state to D%d.",
                     (ULONG)NewState.DeviceState - (ULONG)PowerDeviceD0));

    switch (NewState.DeviceState)
    {
        case PowerDeviceD0:
            //
            // If we are coming from D2 or D3 we have to restore the registers cause
            // there might have been a power loss.
            //
            if ((m_PowerState == PowerDeviceD3) || (m_PowerState == PowerDeviceD2))
            {
                //
                // The scatter gather list is already arranged. A reset of the DMA
                // brings all pointers to the default state. From there we can start.
                //

                ntStatus = ResetDMA ();
            }
            break;

        case PowerDeviceD1:
            // Here we do nothing. The device has still enough power to keep all
            // it's register values.
            break;

        case PowerDeviceD2:
        case PowerDeviceD3:
            //
            // If we power down to D2 or D3 we might loose power, so we have to be
            // aware of the DMA engine resetting. In that case a play would start
            // with scatter gather entry 0 (the current index is read only).
            // This is fine with the RT port.
            //

            // Disable interrupts and stop DMA just in case.
            Miniport->AdapterCommon->WriteBMControlRegister (m_ulBDAddr + X_CR, (UCHAR)0);
            break;
    }

    //
    // Save the new state.  This local value is used to determine when to
    // cache property accesses and when to permit the driver from accessing
    // the hardware.
    //
    m_PowerState = NewState.DeviceState;
    DOUT (DBG_POWER, ("Entering D%d",
                      (ULONG)m_PowerState - (ULONG)PowerDeviceD0));

    return ntStatus;
}

/*****************************************************************************
 * CAC97MiniportWaveRTStream::SetState
 *****************************************************************************
 * This routine sets/changes the DMA engine state to play, stop, or pause
 */
STDMETHODIMP_(NTSTATUS) CAC97MiniportWaveRTStream::SetState
(
    _In_  KSSTATE State
)
{
    DOUT (DBG_PRINT, ("[CAC97MiniportWaveRTStream::SetState]"));
    DOUT (DBG_STREAM, ("SetState to %d", State));


    //
    // Start or stop the DMA engine dependent of the state.
    //
    switch (State)
    {
        case KSSTATE_STOP:
            // We reset the DMA engine which will also reset the position pointers.
            ResetDMA ();
            break;

        case KSSTATE_ACQUIRE:
            break;

        case KSSTATE_PAUSE:
            // pause now.
            PauseDMA ();
            break;

        case KSSTATE_RUN:
            //
            // Let's rock.
            //
            // Make sure we are not running already.
            if (DMAEngineState == DMA_ENGINE_ON)
            {
                return STATUS_SUCCESS;
            }

            // Kick DMA again just in case.
            ResumeDMA ();
            break;
    }

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAC97MiniportWaveRTStream::GetPosition
 *****************************************************************************
 * Gets the stream position. This is a byte count of the current position of
 * a stream running on a particular DMA engine.  We must return a sample
 * accurate count or the WaveDrv32 wave drift tests (35.2 & 36.2) will fail.
 *
 * The position is the sum of three parts:
 *     1) The total number of bytes in released buffers
 *     2) The position in the current buffer.
 *     3) The total number of bytes in played but not yet released buffers
 */
STDMETHODIMP_(NTSTATUS) CAC97MiniportWaveRTStream::GetPosition
(
    _Out_ PKSAUDIO_POSITION   Position
)
{
    UCHAR   nCurrentIndex = 0;
    DWORD   RegisterX_PICB;

    ASSERT (Position);

    if (DMAEngineState == DMA_ENGINE_OFF)
    {
        Position->PlayOffset = 0;
        Position->WriteOffset = 0;
        return STATUS_SUCCESS;
    }

    //
    // Repeat this until we get the same reading twice.  This will prevent
    // jumps when we are near the end of the buffer.
    //
    do
    {
        nCurrentIndex = Miniport->AdapterCommon->
            ReadBMControlRegister8 (m_ulBDAddr + X_CIV);

        RegisterX_PICB = (DWORD)Miniport->AdapterCommon->ReadBMControlRegister16 (m_ulBDAddr + X_PICB);
    } while (nCurrentIndex != (int)Miniport->AdapterCommon->
            ReadBMControlRegister8 (m_ulBDAddr + X_CIV));

    //
    // The HW is really running if RegisterX_PICB is not 0 or nCurrentIndex is not 0.
    //
    if (RegisterX_PICB || nCurrentIndex)
    {
        //
        // The PlayOffset we assume is the position we got from the HW. The
        // WriteOffset is PlayOffset + FIFO size.
        //
        Position->PlayOffset = BDList[nCurrentIndex].wLength - RegisterX_PICB;
        Position->PlayOffset = Position->PlayOffset << 1;
        Position->WriteOffset = Position->PlayOffset + NumberOfChannels * 2 * 8;
    }
    else
    {
        Position->PlayOffset = 0;
        Position->WriteOffset = 0;
    }

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAC97MiniportWaveRTStream::ResetDMA
 *****************************************************************************
 * This routine resets the Run/Pause bit in the control register. In addition, it
 * resets all DMA registers contents.
 */
NTSTATUS CAC97MiniportWaveRTStream::ResetDMA (void)
{
    DOUT (DBG_PRINT, ("ResetDMA"));

    //
    // Turn off DMA engine (or make sure it's turned off)
    //
    UCHAR RegisterValue = Miniport->AdapterCommon->
        ReadBMControlRegister8 (m_ulBDAddr + X_CR) & ~CR_RPBM;
    Miniport->AdapterCommon->
        WriteBMControlRegister (m_ulBDAddr + X_CR, RegisterValue);

    //
    // Reset all register contents.
    //
    RegisterValue |= CR_RR;
    Miniport->AdapterCommon->
        WriteBMControlRegister (m_ulBDAddr + X_CR, RegisterValue);

    //
    // Wait until reset condition is cleared by HW; should not take long.
    //
    ULONG count = 0;
    BOOL bTimedOut = TRUE;
    do
    {
        if (!(Miniport->AdapterCommon->
           ReadBMControlRegister8 (m_ulBDAddr + X_CR) & CR_RR))
        {
            bTimedOut = FALSE;
            break;
        }
        KeStallExecutionProcessor (1);
    } while (count++ < 10);

    if (bTimedOut)
    {
        DOUT (DBG_ERROR, ("ResetDMA TIMEOUT!!"));
    }

    //
    // We only want interrupts upon completion.
    //
    RegisterValue = CR_IOCE | CR_LVBIE;
    Miniport->AdapterCommon->
        WriteBMControlRegister (m_ulBDAddr + X_CR, RegisterValue);

    //
    // Setup the Buffer Descriptor Base Address (BDBA) register.
    //
    Miniport->AdapterCommon->
        WriteBMControlRegister (m_ulBDAddr, MmGetPhysicalAddress (BDList).LowPart);

    //
    // Set the DMA engine state.
    //
    DMAEngineState = DMA_ENGINE_OFF;


    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAC97MiniportWaveRTStream::PauseDMA
 *****************************************************************************
 * This routine pauses a hardware stream by reseting the Run/Pause bit in the
 * control registers, leaving DMA registers content intact so that the stream
 * can later be resumed.
 */
NTSTATUS CAC97MiniportWaveRTStream::PauseDMA (void)
{
    DOUT (DBG_PRINT, ("PauseDMA"));

    //
    // Only pause if we're actually on.
    //
    if (DMAEngineState != DMA_ENGINE_ON)
    {
        DMAEngineState = DMA_ENGINE_PAUSE;
        return STATUS_SUCCESS;
    }

    //
    // Turn off DMA engine by resetting the RPBM bit to 0. Don't reset any
    // registers.
    //
    UCHAR RegisterValue = Miniport->AdapterCommon->
        ReadBMControlRegister8 (m_ulBDAddr + X_CR);
    RegisterValue &= ~CR_RPBM;
    Miniport->AdapterCommon->
        WriteBMControlRegister (m_ulBDAddr + X_CR, RegisterValue);

    //
    // Set the DMA engine state.
    //
    DMAEngineState = DMA_ENGINE_PAUSE;

    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CAC97MiniportWaveRTStream::ResumeDMA
 *****************************************************************************
 * This routine sets the Run/Pause bit for the particular DMA engine to resume
 * it after it's been paused. This assumes that DMA registers content have
 * been preserved.
 */
NTSTATUS CAC97MiniportWaveRTStream::ResumeDMA (void)
{
    DOUT (DBG_PRINT, ("ResumeDMA"));

    //
    // Turn DMA engine on by setting the RPBM bit to 1. Don't do anything to
    // the registers.
    //
    UCHAR RegisterValue = Miniport->AdapterCommon->
        ReadBMControlRegister8 (m_ulBDAddr + X_CR) | CR_RPBM;
    Miniport->AdapterCommon->
        WriteBMControlRegister (m_ulBDAddr + X_CR, RegisterValue);

    //
    // Set the DMA engine state.
    //
    DMAEngineState = DMA_ENGINE_ON;

    return STATUS_SUCCESS;
}

#endif          // (NTDDI_VERSION >= NTDDI_VISTA)


