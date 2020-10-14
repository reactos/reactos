// Every debug output has "Modulname text"
#define STR_MODULENAME "AC97 Stream: "

#include "shared.h"
#include "miniport.h"

#define WriteReg(addr, data) (Miniport->AdapterCommon-> \
    WriteBMControlRegister (m_ulBDAddr + addr, data))
#define ReadReg8(addr) (Miniport->AdapterCommon-> \
    ReadBMControlRegister8 (m_ulBDAddr + addr))

UCHAR CMiniportStream::UpdateDMA (void)
{
    // get X_CR register value
    UCHAR RegisterValue = ReadReg8(X_CR);
    UCHAR RegisterValueNew = RegisterValue;
    if(DMAEngineState == DMA_ENGINE_ON)
        RegisterValueNew |= CR_RPBM;

    // write X_CR register value
    if(RegisterValue != RegisterValueNew)
        WriteReg(X_CR, RegisterValueNew);
    return RegisterValueNew;
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
    WriteReg(X_CR, RegisterValue);

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
    WriteReg(X_CR,  RegisterValue);

    //
    // Setup the Buffer Descriptor Base Address (BDBA) register.
    //
    WriteReg(0,  BDList_PhysAddr.LowPart);
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
