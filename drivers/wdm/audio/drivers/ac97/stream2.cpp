// Every debug output has "Modulname text"
#define STR_MODULENAME "AC97 Stream2: "

#include "shared.h"
#include "miniport.h"

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif

PVOID CMiniportStream::BDList_Alloc()
{
    // get DMA_ADAPTER object
    PDMA_ADAPTER AdapterObject = (PDMA_ADAPTER)
      Miniport->DmaChannel->GetAdapterObject();

    // allocate DBList
    BDList = (tBDEntry *)AdapterObject->DmaOperations->
         AllocateCommonBuffer (AdapterObject,
                               MAX_BDL_ENTRIES * sizeof (tBDEntry),
                               &BDList_PhysAddr, FALSE);
    return BDList;
}

void CMiniportStream::BDList_Free()
{
    if (BDList)
    {
        // get DMA_ADAPTER object
        PDMA_ADAPTER AdapterObject = (PDMA_ADAPTER)
          Miniport->DmaChannel->GetAdapterObject();

        // free DBList
        AdapterObject->DmaOperations->
           FreeCommonBuffer (AdapterObject,
                             PAGE_SIZE,
                             BDList_PhysAddr,
                             (PVOID)BDList,
                             FALSE);
        BDList = NULL;
    }
}

/*****************************************************************************
 * CMiniportStream::PowerChangeNotify
 *****************************************************************************
 * This functions saves and maintains the stream state through power changes.
 */

void CMiniportStream::PowerChangeNotify_
(
    IN  POWER_STATE NewState
)
{
    if(NewState.DeviceState == PowerDeviceD0)
    {
        //
        // The scatter gather list is already arranged. A reset of the DMA
        // brings all pointers to the default state. From there we can start.
        //

        ResetDMA ();
    }
    else
    {
        // Disable interrupts and stop DMA just in case.
        Miniport->AdapterCommon->WriteBMControlRegister (m_ulBDAddr + X_CR, (UCHAR)0);
    }
}


/*****************************************************************************
 * CMiniportStream::SetState
 *****************************************************************************
 * This routine sets/changes the DMA engine state to play, stop, or pause
 */
NTSTATUS CMiniportStream::SetState
(
    _In_  KSSTATE State
)
{
    DOUT (DBG_PRINT, ("[CMiniportStream::SetState]"));
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
            UpdateLviCyclic();
            break;
    }

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
 * CMiniportStream::NormalizePhysicalPosition
 *****************************************************************************
 * Given a physical position based on the actual number of bytes transferred,
 * this function converts the position to a time-based value of 100ns units.
 */
NTSTATUS CMiniportStream::NormalizePhysicalPosition
(
    _Inout_ PLONGLONG PhysicalPosition
)
{
    ULONG SampleSize;

    DOUT (DBG_PRINT, ("NormalizePhysicalPosition"));

    //
    // Determine the sample size in bytes
    //
    SampleSize = DataFormat->WaveFormatEx.nChannels * 2;

    //
    // Calculate the time in 100ns steps.
    //
    *PhysicalPosition = (_100NS_UNITS_PER_SECOND / SampleSize *
                         *PhysicalPosition) / CurrentRate;

    return STATUS_SUCCESS;
}
