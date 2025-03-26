// Every debug output has "Modulname text"
#define STR_MODULENAME "AC97 Stream: "

#include "shared.h"
#include "miniport.h"

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif


/*****************************************************************************
 * CMiniportStream::NonDelegatingQueryInterface
 *****************************************************************************
 * Obtains an interface.  This function works just like a COM QueryInterface
 * call and is used if the object is not being aggregated.
 */
NTSTATUS CMiniportStream::NonDelegatingQueryInterface
(
    _In_         REFIID  Interface,
    _COM_Outptr_ PVOID * Object,
    _In_         REFIID iStream,
    _In_         PUNKNOWN stream
)
{
    PAGED_CODE ();

    ASSERT (Object);

    DOUT (DBG_PRINT, ("[CMiniportStream::NonDelegatingQueryInterface]"));

    //
    // Convert for IID_IMiniportXXXStream
    //
    if (IsEqualGUIDAligned (Interface, iStream))
    {
        *Object = (PVOID)stream;
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
        *Object = (PVOID)stream;
    }
    else
    {
        *Object = NULL;
        return STATUS_INVALID_PARAMETER;
    }

    ((PUNKNOWN)*Object)->AddRef ();
    return STATUS_SUCCESS;
}


NTSTATUS CMiniportStream::Init
(
    IN  CMiniport               *Miniport_,
    IN  PUNKNOWN                PortStream_,
    IN  WavePins                Pin_,
    IN  BOOLEAN                 Capture_,
    IN  PKSDATAFORMAT           DataFormat_,
    OUT PSERVICEGROUP           *ServiceGroup_
)
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CMiniportStream::Init]"));

    ASSERT (Miniport_);
    ASSERT (DataFormat_);

    //
    // The rule here is that we return when we fail without a cleanup.
    // The destructor will relase the allocated memory.
    //
    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // Save miniport pointer and addref it.
    //
    obj_AddRef(Miniport_, (PVOID *)&Miniport);

    //
    // Save portstream interface pointer and addref it.
    //
    obj_AddRef(PortStream_, (PVOID *)&PortStream);


    //
    // Save channel ID and capture flag.
    //
    Pin = Pin_;
    Capture = Capture_;

    //
    // Save data format and current sample rate.
    //
    DataFormat = (PKSDATAFORMAT_WAVEFORMATEX)DataFormat_;
    CurrentRate = DataFormat->WaveFormatEx.nSamplesPerSec;
    NumberOfChannels = DataFormat->WaveFormatEx.nChannels;


    if (ServiceGroup_)
    {

        //
        // Create a service group (a DPC abstraction/helper) to help with
        // interrupts.
        //
        ntStatus = PcNewServiceGroup (&ServiceGroup, NULL);
        if (!NT_SUCCESS (ntStatus))
        {
            DOUT (DBG_ERROR, ("Failed to create a service group!"));
            return ntStatus;
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
        if (Pin == PIN_WAVEIN)
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
    // Call miniport specific init routine
    //
    ntStatus = Init_();
    if (!NT_SUCCESS (ntStatus))
    {
        return ntStatus;
    }

    //
    // Setup the Buffer Descriptor Base Address (BDBA) register.
    //
    WriteReg32(0,  BDList_PhysAddr.LowPart);

    //
    // Pass the ServiceGroup pointer to portcls.
    //
    obj_AddRef(ServiceGroup, (PVOID *)ServiceGroup_);

    //
    // Store the stream pointer, it is used by the ISR.
    //
    Miniport->Streams[Pin/2] = this;

    return STATUS_SUCCESS;
}


CMiniportStream::~CMiniportStream()
{
    if (Miniport)
    {
        //
        // Disable interrupts and stop DMA just in case.
        //
        if (Miniport->AdapterCommon)
        {
            WriteReg8 (X_CR, (UCHAR)0);

            //
            // Update also the topology miniport if this was the render stream.
            //
            if (Miniport->AdapterCommon->GetMiniportTopology () &&
                (Pin == PIN_WAVEOUT))
            {
                Miniport->AdapterCommon->GetMiniportTopology ()->SetCopyProtectFlag (FALSE);
            }
        }

        //
        // Remove stream from miniport Streams array.
        //
        if (Miniport->Streams[Pin/2] == this)
        {
            Miniport->Streams[Pin/2] = NULL;
        }
    }

    obj_Release((PVOID *)&Miniport);
    obj_Release((PVOID *)&ServiceGroup);
    obj_Release((PVOID *)&PortStream);
}


/*****************************************************************************
 * CMiniportStream::SetContentId
 *****************************************************************************
 * This routine gets called by drmk.sys to pass the content to the driver.
 * The driver has to enforce the rights passed.
 */
STDMETHODIMP_(NTSTATUS) CMiniportStream::SetContentId
(
    _In_  ULONG       contentId,
    _In_  PCDRMRIGHTS drmRights
)
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CMiniportStream::SetContentId]"));

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

void CMiniportStream::PowerChangeNotify
(
    IN  POWER_STATE NewState
)
{
    DOUT (DBG_PRINT, ("[CMiniportStream::PowerChangeNotify]"));

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
                PowerChangeNotify_(NewState);
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

            PowerChangeNotify_(NewState);

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

}

/*
 *****************************************************************************
 * This routine tests for proper data format (calls wave miniport) and sets
 * or changes the stream data format.
 * To figure out if the codec supports the sample rate, we just program the
 * sample rate and read it back. If it matches we return happy, if not then
 * we restore the sample rate and return unhappy.
 * We fail this routine if we are currently running (playing or recording).
 */
STDMETHODIMP_(NTSTATUS) CMiniportStream::SetFormat
(
    _In_  PKSDATAFORMAT   Format
)
{
    PAGED_CODE ();

    ASSERT (Format);

    ULONG   TempRate;
    DWORD   dwControlReg;

    DOUT (DBG_PRINT, ("[CMiniportStream::SetFormat]"));

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
    NTSTATUS ntStatus = Miniport->TestDataFormat (Format, Pin);
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
    if (Pin == PIN_WAVEOUT)
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
        if (Pin == PIN_WAVEIN)
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
 * Non paged code begins here
 *****************************************************************************
 */

#ifdef _MSC_VER
#pragma code_seg()
#endif

UCHAR CMiniportStream::UpdateDMA (void)
{
    // get X_CR register value
    UCHAR RegisterValue = ReadReg8(X_CR);
    UCHAR RegisterValueNew = RegisterValue & ~CR_RPBM;
    if(DMAEngineState == DMA_ENGINE_ON)
        RegisterValueNew |= CR_RPBM;

    // write X_CR register value
    if(RegisterValue != RegisterValueNew)
        WriteReg8(X_CR, RegisterValueNew);
    return RegisterValueNew;
}


int CMiniportStream::GetBuffPos
(
    DWORD* buffPos
)
{
    int nCurrentIndex;
    DWORD RegisterX_PICB;

    if (DMAEngineState == DMA_ENGINE_OFF)
    {
        *buffPos = 0;
        return 0;
    }

    //
    // Repeat this until we get the same reading twice.  This will prevent
    // jumps when we are near the end of the buffer.
    //
    do
    {
        nCurrentIndex = ReadReg8(X_CIV);

        RegisterX_PICB = ReadReg16 (X_PICB);
    } while (nCurrentIndex != (int)ReadReg8(X_CIV));

    *buffPos = (BDList[nCurrentIndex].wLength - RegisterX_PICB) * 2;
    return nCurrentIndex;
}

/*****************************************************************************
 * CMiniportStream::ResetDMA
 *****************************************************************************
 * This routine resets the Run/Pause bit in the control register. In addition, it
 * resets all DMA registers contents.

 */
void CMiniportStream::ResetDMA (void)
{
    DOUT (DBG_PRINT, ("ResetDMA"));

    //
    // Turn off DMA engine (or make sure it's turned off)
    //
    DMAEngineState = DMA_ENGINE_OFF;
    UCHAR RegisterValue = UpdateDMA();

    //
    // Reset all register contents.
    //
    RegisterValue |= CR_RR;
    WriteReg8(X_CR, RegisterValue);

    //
    // Wait until reset condition is cleared by HW; should not take long.
    //
    ULONG count = 0;
    BOOL bTimedOut = TRUE;
    do
    {
        if (!(ReadReg8(X_CR) & CR_RR))
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
    WriteReg8(X_CR,  RegisterValue);

    //
    // Setup the Buffer Descriptor Base Address (BDBA) register.
    //
    WriteReg32(0,  BDList_PhysAddr.LowPart);
}

/*****************************************************************************
 * CMiniportStream::ResumeDMA
 *****************************************************************************
 * This routine sets the Run/Pause bit for the particular DMA engine to resume
 * it after it's been paused. This assumes that DMA registers content have
 * been preserved.
 */
void CMiniportStream::ResumeDMA (ULONG state)
{
    DOUT (DBG_PRINT, ("ResumeDMA"));

    DMAEngineState |= state;
    UpdateDMA();
}

/*****************************************************************************
 * CMiniportStream::PauseDMA
 *****************************************************************************
 * This routine pauses a hardware stream by reseting the Run/Pause bit in the
 * control registers, leaving DMA registers content intact so that the stream
 * can later be resumed.
 */
void CMiniportStream::PauseDMA (void)
{
    DOUT (DBG_PRINT, ("PauseDMA"));

    DMAEngineState &= DMA_ENGINE_PAUSE;
    UpdateDMA();
}


void CMiniportStream::WriteReg8(ULONG addr, UCHAR data) { Miniport->AdapterCommon->
    WriteBMControlRegister (m_ulBDAddr + addr, data); }
void CMiniportStream::WriteReg16(ULONG addr, USHORT data) { Miniport->AdapterCommon->
    WriteBMControlRegister (m_ulBDAddr + addr, data); }
void CMiniportStream::WriteReg32(ULONG addr, ULONG data) { Miniport->AdapterCommon->
    WriteBMControlRegister (m_ulBDAddr + addr, data); }
UCHAR CMiniportStream::ReadReg8(ULONG addr) { return Miniport->AdapterCommon->
    ReadBMControlRegister8 (m_ulBDAddr + addr); }
USHORT CMiniportStream::ReadReg16(ULONG addr) { return Miniport->AdapterCommon->
    ReadBMControlRegister16 (m_ulBDAddr + addr); }
ULONG CMiniportStream::ReadReg32(ULONG addr) { return Miniport->AdapterCommon->
    ReadBMControlRegister32 (m_ulBDAddr + addr); }
