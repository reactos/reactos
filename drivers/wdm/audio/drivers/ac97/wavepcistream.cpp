/********************************************************************************
**    Copyright (c) 1998-2000 Microsoft Corporation. All Rights Reserved.
**
**       Portions Copyright (c) 1998-1999 Intel Corporation
**
********************************************************************************/

/* The file wavepcistream.cpp was reviewed by LCA in June 2011 and is acceptable for use by Microsoft. */

// Every debug output has "Modulname text"
#define STR_MODULENAME "AC97 Stream: "

#include "wavepciminiport.h"
#include "wavepcistream.h"

IMP_CMiniportStream_SetFormat(CMiniportWaveICHStream);
IMP_CMiniportStream_QueryInterface(CMiniportWaveICHStream, IMiniportWavePciStream);

/*****************************************************************************
 * General Info
 *****************************************************************************
 * To protect the stBDList structure that is used to store mappings, we use a
 * spin lock called MapLock. This spin lock is also acquired when we change
 * the DMA registers. Normally, changes in stBDList and the DMA registers go
 * hand in hand. In case we only want to change the DMA registers, we need
 * to acquire the spin lock!
 */

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif
/*****************************************************************************
 * CreateMiniportWaveICHStream
 *****************************************************************************
 * Creates a wave miniport stream object for the AC97 audio adapter. This is
 * (nearly) like the macro STD_CREATE_BODY_ from STDUNK.H.
 */
NTSTATUS CreateMiniportWaveICHStream
(
    OUT CMiniportWaveICHStream  **MiniportPCIStream,
    IN  PUNKNOWN                pUnknownOuter,
    _When_((PoolType & NonPagedPoolMustSucceed) != 0,
       __drv_reportError("Must succeed pool allocations are forbidden. "
			 "Allocation failures cause a system crash"))
    IN  POOL_TYPE               PoolType
)
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CreateMiniportWaveICHStream]"));

    //
    // This is basically like the macro at stdunk with the change that we
    // don't cast to interface unknown but to interface MiniportIchStream.
    //
    *MiniportPCIStream = new (PoolType, PoolTag)
                        CMiniportWaveICHStream (pUnknownOuter);
    if (*MiniportPCIStream)
    {
        (*MiniportPCIStream)->AddRef ();
        return STATUS_SUCCESS;
    }

    return STATUS_INSUFFICIENT_RESOURCES;
}


/*****************************************************************************
 * CMiniportWaveICHStream::~CMiniportWaveICHStream
 *****************************************************************************
 * Destructor
 */
CMiniportWaveICHStream::~CMiniportWaveICHStream ()
{
    PAGED_CODE ();


    DOUT (DBG_PRINT, ("[CMiniportWaveICHStream::~CMiniportWaveICHStream]"));

    //
    // Print information about the scatter gather list.
    //
    DOUT (DBG_DMA, ("Head %d, Tail %d, Tag counter %d, Entries %d.",
                   stBDList.nHead, stBDList.nTail, stBDList.ulTagCounter,
                   stBDList.nBDEntries));



    //
    // Release the scatter/gather table.
    //
    if (BDList)
    {
        Wave()->AdapterObject->DmaOperations->
           FreeCommonBuffer (Wave()->AdapterObject,
                             PAGE_SIZE,
                             BDList_PhysAddr,
                             (PVOID)BDList,
                             FALSE);
        BDList = NULL;
    }

    //
    // Release the mapping table.
    //
    if (stBDList.pMapData)
    {
        ExFreePool (stBDList.pMapData);
        stBDList.pMapData = NULL;
    }
}


/*****************************************************************************
 * CMiniportWaveICHStream::Init
 *****************************************************************************
 * This routine initializes the stream object, sets up the BDL, and programs
 * the buffer descriptor list base address register for the pin being
 * initialized.
 */
NTSTATUS CMiniportWaveICHStream::Init
(
    IN  CMiniportWaveICH        *Miniport_,
    IN  PPORTWAVEPCISTREAM      PortStream_,
    IN  WavePins                Pin,
    IN  BOOLEAN                 Capture_,
    IN  PKSDATAFORMAT           DataFormat_,
    OUT PSERVICEGROUP           *ServiceGroup_
)
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CMiniportWaveICHStream::Init]"));

    ASSERT (Miniport_);
    ASSERT (PortStream_);
    ASSERT (DataFormat_);
    ASSERT (ServiceGroup_);

    //
    // The rule here is that we return when we fail without a cleanup.
    // The destructor will relase the allocated memory.
    //

    //
    // Initialize the BDL spinlock.
    //
    KeInitializeSpinLock (&MapLock);

    //
    // Setup the Buffer Descriptor List (BDL)
    // Allocate 32 entries of 8 bytes (one BDL entry). We allocate two tables
    // because we need one table as a backup.
    // The pointer is aligned on a 8 byte boundary (that's what we need).
    //
    BDList = (tBDEntry *)Miniport_->AdapterObject->DmaOperations->
         AllocateCommonBuffer (Miniport_->AdapterObject,
                               MAX_BDL_ENTRIES * sizeof (tBDEntry) * 2,
                               &BDList_PhysAddr,
                               FALSE);

    if (!BDList)
    {
        DOUT (DBG_ERROR, ("Failed AllocateCommonBuffer!"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // calculate the (backup) pointer.
    stBDList.pBDEntryBackup = (tBDEntry *)BDList + MAX_BDL_ENTRIES;

    //
    // Allocate a buffer for the 32 possible mappings. We allocate two tables
    // because we need one table as a backup
    //
    stBDList.pMapData =
        (tMapData *)ExAllocatePoolWithTag (NonPagedPool, sizeof(tMapData) *
                                    MAX_BDL_ENTRIES * 2, PoolTag);
    if (!stBDList.pMapData)
    {
        DOUT (DBG_ERROR, ("Failed to allocate the back up buffer!"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // calculate the (backup) pointer.
    stBDList.pMapDataBackup = stBDList.pMapData + MAX_BDL_ENTRIES;


    NTSTATUS ntStatus = CMiniportStream::Init(Miniport_,
                                              PortStream_,
                                              Pin,
                                              Capture_,
                                              DataFormat_,
                                              ServiceGroup_);
    if (!NT_SUCCESS (ntStatus))
        return ntStatus;

    PPREFETCHOFFSET PreFetchOffset;
    //
    // Query for the new interface "PreFetchOffset" and use
    // function offered there in case the interface is offered.
    //
    if (NT_SUCCESS(PortStream->QueryInterface(IID_IPreFetchOffset, (PVOID *)&PreFetchOffset)))
    {
        // why don't we pad by 32 sample frames
        PreFetchOffset->SetPreFetchOffset(32 * (DataFormat->WaveFormatEx.nChannels * 2));
        PreFetchOffset->Release();
    }

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CMiniportWaveICHStream::GetAllocatorFraming
 *****************************************************************************
 * Returns the framing requirements for this device.
 * That is sample size (for one sample) and preferred frame (buffer) size.
 */
STDMETHODIMP_(NTSTATUS) CMiniportWaveICHStream::GetAllocatorFraming
(
    _Out_ PKSALLOCATOR_FRAMING AllocatorFraming
)
{
    PAGED_CODE ();

    ULONG SampleSize;

    DOUT (DBG_PRINT, ("[CMiniportWaveICHStream::GetAllocatorFraming]"));

    //
    // Determine sample size in bytes.  Always number of
    // channels * 2 (because 16-bit).
    //
    SampleSize = DataFormat->WaveFormatEx.nChannels * 2;

    //
    // Report the suggested requirements.
    //
    AllocatorFraming->RequirementsFlags =
        KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY |
        KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;
    AllocatorFraming->Frames = 8;

    //
    // Currently, arbitrarily selecting 10ms as the frame target size.
    //
    //  This value needs to be sample block aligned for AC97 to work correctly.
    //  Assumes 100Hz minimum sample rate (otherwise FrameSize is 0 bytes)
    //
    AllocatorFraming->FrameSize = SampleSize * (DataFormat->WaveFormatEx.nSamplesPerSec / 100);
    AllocatorFraming->FileAlignment = FILE_LONG_ALIGNMENT;
    AllocatorFraming->PoolType = NonPagedPool;

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
 * CMiniportWaveICHStream::PowerChangeNotify_
 *****************************************************************************
 * This functions saves and maintains the stream state through power changes.
 */
void CMiniportWaveICHStream::PowerChangeNotify_
(
    IN  POWER_STATE NewState
)
{
    KIRQL       OldIrql;

    KeAcquireSpinLock (&MapLock,&OldIrql);

    if(NewState.DeviceState == PowerDeviceD0)
    {
        ResetDMA ();

        // Restore the remaining DMA registers, that is last valid index
        // only if the index is not pointing to 0. Note that the index is
        // equal to head + entries.
        if (stBDList.nTail)
        {
            Miniport->AdapterCommon->WriteBMControlRegister (m_ulBDAddr + X_LVI,
                                    (UCHAR)((stBDList.nTail - 1) & BDL_MASK));
        }
    }
    else
    {

        // Disable interrupts and stop DMA just in case.
        Miniport->AdapterCommon->WriteBMControlRegister (m_ulBDAddr + X_CR, (UCHAR)0);

        // Get current index
        int nCurrentIndex = (int)Miniport->AdapterCommon->
            ReadBMControlRegister8 (m_ulBDAddr + X_CIV);

        //
        // First move the BD list to the beginning.
        //
        // In case the DMA engine was stopped, current index may point to an
        // empty BD entry. When we start the DMA engine it would then play this
        // (undefined) entry, so we check here for that condition.
        //
        if ((nCurrentIndex == ((stBDList.nHead - 1) & BDL_MASK)) &&
           (stBDList.nBDEntries != MAX_BDL_ENTRIES - 1))
        {
            nCurrentIndex = stBDList.nHead;     // point to head
        }

        //
        // Move BD list to (0-((current - head) & mask)) & mask, where
        // ((current - head) & mask) is the difference between head and
        // current index, no matter where they are :)
        //
        MoveBDList (stBDList.nHead, (stBDList.nTail - 1) & BDL_MASK,
                (0 - ((nCurrentIndex - stBDList.nHead) & BDL_MASK)) & BDL_MASK);

        //
        // Update structure.
        //
        stBDList.nHead = (0 - ((nCurrentIndex - stBDList.nHead) & BDL_MASK)) &
            BDL_MASK;
        stBDList.nTail = (stBDList.nHead + stBDList.nBDEntries) & BDL_MASK;
    }

    KeReleaseSpinLock (&MapLock,OldIrql);
}

/*****************************************************************************
 * CMiniportWaveICHStream::SetState
 *****************************************************************************
 * This routine sets/changes the DMA engine state to play, stop, or pause
 */
STDMETHODIMP_(NTSTATUS) CMiniportWaveICHStream::SetState
(
    _In_  KSSTATE State
)
{
    KIRQL OldIrql;

    DOUT (DBG_PRINT, ("[CMiniportWaveICHStream::SetState]"));
    DOUT (DBG_STREAM, ("SetState to %d", State));


    //
    // Start or stop the DMA engine dependent of the state.
    //
    switch (State)
    {
        case KSSTATE_STOP:
            // acquire the mapping spin lock
            KeAcquireSpinLock (&MapLock,&OldIrql);

            // Just pause DMA. If we have mappings left in our queue,
            // portcls will call RevokeMappings to destroy them.
            PauseDMA ();

            // release the mapping spin lock
            KeReleaseSpinLock (&MapLock,OldIrql);

            // Release processed mappings
            ReleaseUsedMappings ();

            // Reset the position counters.
            TotalBytesMapped = TotalBytesReleased = 0;
            break;

        case KSSTATE_ACQUIRE:
        case KSSTATE_PAUSE:
            // acquire the mapping spin lock
            KeAcquireSpinLock (&MapLock,&OldIrql);

            // pause now.
            PauseDMA ();

            // release the mapping spin lock
            KeReleaseSpinLock (&MapLock,OldIrql);

            // Release processed mappings
            ReleaseUsedMappings ();

            break;

        case KSSTATE_RUN:
            //
            // Let's rock.
            //


            // Make sure we are not running already.
            if (DMAEngineState & DMA_ENGINE_ON)
            {
                return STATUS_SUCCESS;
            }

            // Release processed mappings.
            ReleaseUsedMappings ();

            // Get new mappings.
            GetNewMappings ();

            // acquire the mapping spin lock
            KeAcquireSpinLock (&MapLock,&OldIrql);

            // Kick DMA again just in case.
            ResumeDMA (DMA_ENGINE_PEND);

            // release the mapping spin lock
            KeReleaseSpinLock (&MapLock,OldIrql);
            break;
    }

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CMiniportWaveICHStream::MoveBDList
 *****************************************************************************
 * Moves the BDList.
 * This function is used to remove entries from the scatter gather list or to
 * move the valid entries to the top. The mapping table which is hard linked
 * to the scatter gather entries is moved too.
 * The function does not change any variables in tBDList.
 * The mapping spin lock must be held when calling this routine.
 * We use this function to remove mappings (RevokeMappings) or to rearrange the
 * list for powerdown/up management (the DMA starts at position zero again).
 * Note that there is a simple way of doing this also. When you zero the buffer
 * length in the scatter gather, the DMA engine ignores the entry and continues
 * with the next. But our way is more generic and if you ever want to port the
 * driver to another DMA engine you might be thankful for this code.
 */
void CMiniportWaveICHStream::MoveBDList
(
    IN  int nFirst,
    IN  int nLast,
    IN  int nNewPos
)
{
    DOUT (DBG_PRINT, ("[CMiniportWaveICHStream::MoveBDList]"));

    //
    // Print information about the scatter gather list.
    //
    DOUT (DBG_DMA, ("Moving BD entry %d-%d to %d.", nFirst, nLast, nNewPos));

    //
    // First copy the tables to a save place.
    //
    RtlCopyMemory ((PVOID)stBDList.pBDEntryBackup,
                   (PVOID)BDList,
                   sizeof (tBDEntry) * MAX_BDL_ENTRIES);
    RtlCopyMemory ((PVOID)stBDList.pMapDataBackup,
                   (PVOID)stBDList.pMapData,
                   sizeof (tMapData) * MAX_BDL_ENTRIES);

    //
    // We move all the entries in blocks to the new position.
    //
    int nBlockCounter = 0;
    do
    {
        nBlockCounter++;
        //
        // We must copy the block when the index wraps around (ring buffer)
        // or we are at the last entry.
        //
        if (((nNewPos + nBlockCounter) == MAX_BDL_ENTRIES) ||   // wrap around
            ((nFirst + nBlockCounter) == MAX_BDL_ENTRIES) ||    // wrap around
            ((nFirst + nBlockCounter) == (nLast + 1)))          // last entry
        {
            //
            // copy one block (multiple entries).
            //
            RtlCopyMemory ((PVOID)&BDList[nNewPos],
                           (PVOID)&stBDList.pBDEntryBackup[nFirst],
                           sizeof (tBDEntry) * nBlockCounter);
            RtlCopyMemory ((PVOID)&stBDList.pMapData[nNewPos],
                           (PVOID)&stBDList.pMapDataBackup[nFirst],
                           sizeof (tMapData) * nBlockCounter);
            // adjust the index
            nNewPos = (nNewPos + nBlockCounter) & BDL_MASK;
            nFirst = (nFirst + nBlockCounter) & BDL_MASK;
            nBlockCounter = 0;
        }
    // nBlockCounter should be zero when the end condition hits.
    } while (((nFirst + nBlockCounter - 1) & BDL_MASK) != nLast);
}


/*****************************************************************************
 * CMiniportWaveICHStream::Service
 *****************************************************************************
 * This routine is called by the port driver in response to the interrupt
 * service routine requesting service on the stream's service group.
 * Requesting service on the service group results in a DPC being scheduled
 * that calls this routine when it runs.
 */
STDMETHODIMP_(void) CMiniportWaveICHStream::Service (void)
{

    DOUT (DBG_PRINT, ("Service"));

    // release all mappings
    ReleaseUsedMappings ();

    // get new mappings
    GetNewMappings ();
}


/*****************************************************************************
 * CMiniportWaveICHStream::NormalizePhysicalPosition
 *****************************************************************************
 * Given a physical position based on the actual number of bytes transferred,
 * this function converts the position to a time-based value of 100ns units.
 */
STDMETHODIMP_(NTSTATUS) CMiniportWaveICHStream::NormalizePhysicalPosition
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


/*****************************************************************************
 * CMiniportWaveICHStream::GetPosition
 *****************************************************************************
 * Gets the stream position. This is a byte count of the current position of
 * a stream running on a particular DMA engine.  We must return a sample
 * accurate count or the MiniportDrv32 wave drift tests (35.2 & 36.2) will fail.
 *
 * The position is the sum of three parts:
 *     1) The total number of bytes in released buffers
 *     2) The position in the current buffer.
 *     3) The total number of bytes in played but not yet released buffers
 */
STDMETHODIMP_(NTSTATUS) CMiniportWaveICHStream::GetPosition
(
    _Out_ PULONGLONG   Position
)
{
    KIRQL   OldIrql;
    UCHAR   nCurrentIndex = 0;
    DWORD   buffPos;

    ASSERT (Position);

    //
    // Acquire the mapping spin lock.
    //
    KeAcquireSpinLock (&MapLock, &OldIrql);

    //
    // Start with TotalBytesReleased (mappings released).
    //
    *Position = TotalBytesReleased;

    //
    // If we have entries in the list, we may have buffers that have not been
    // released but have been at least partially played.
    //
    if (DMAEngineState != DMA_ENGINE_OFF)
    {
        nCurrentIndex = GetBuffPos(&buffPos);

        //
        // Add in our position in the current buffer
        //
        *Position += buffPos;

        //
        // Total any buffers that have been played and not released.
        //
        if (nCurrentIndex != ((stBDList.nHead -1) & BDL_MASK))
        {
            int i = stBDList.nHead;
            while (i != nCurrentIndex)
            {
                *Position += (ULONGLONG)stBDList.pMapData[i].ulBufferLength;
                i = (i + 1) & BDL_MASK;
            }
        }
    }

    DOUT (DBG_POSITION, ("[GetPosition] POS: %08x'%08x\n", (DWORD)(*Position >> 32), (DWORD)*Position));

    //
    // Release the mapping spin lock.
    //
    KeReleaseSpinLock (&MapLock, OldIrql);


    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CMiniportWaveICHStream::RevokeMappings
 *****************************************************************************
 * This routine is used by the port to revoke mappings previously delivered
 * to the miniport stream that have not yet been unmapped.  This would
 * typically be called in response to an I/O cancellation request.
 */
STDMETHODIMP_(NTSTATUS) CMiniportWaveICHStream::RevokeMappings
(
    _In_  PVOID     FirstTag,
    _In_  PVOID     LastTag,
    _Out_ PULONG    MappingsRevoked
)
{
    DbgPrint("[CMiniportWaveICHStream::RevokeMappings]\n");

    ASSERT (MappingsRevoked);

    KIRQL   OldIrql;
    ULONG   ulOldDMAEngineState;
    int     nCurrentIndex, nSearchIndex, nFirst, nLast, nNumMappings;

    DOUT (DBG_PRINT, ("[CMiniportWaveICHStream::RevokeMappings]"));

    //
    // print information about the scatter gather list.
    //
    DOUT (DBG_DMA, ("Head %d, Tail %d, Tag counter %d, Entries %d.",
                   stBDList.nHead, stBDList.nTail, stBDList.ulTagCounter,
                   stBDList.nBDEntries));

    //
    // Start accessing the mappings.
    //
    KeAcquireSpinLock (&MapLock, &OldIrql);

    //
    // Save old DMA engine state.
    //
    ulOldDMAEngineState = DMAEngineState;

    //
    // First stop the DMA engine so it won't process the next buffer in the
    // scatter gather list which might be one of the revoked buffers.
    //
    PauseDMA ();

    // Get current index
    nCurrentIndex = Miniport->AdapterCommon->
        ReadBMControlRegister8 (m_ulBDAddr + X_CIV);

    //
    // We always rearrange the scatter gather list. That means we reset the DMA
    // engine and move the BD list so that the entry where the current index
    // pointed to is located at position 0.
    //
    ResetDMA ();

    //
    // Return immediately if we just have 1 entry in our BD list.
    //
    if (!stBDList.nBDEntries || (stBDList.nBDEntries == 1))
    {
        *MappingsRevoked = stBDList.nBDEntries;
        stBDList.nHead = stBDList.nTail = stBDList.nBDEntries = 0;

        //
        // CIV and LVI of DMA registers are set to 0 already.
        //
        KeReleaseSpinLock (&MapLock, OldIrql);
        return STATUS_SUCCESS;
    }

    //
    // First move the BD list to the beginning.  In case the DMA engine was
    // stopped, current index may point to an empty BD entry.
    //
    if ((nCurrentIndex == ((stBDList.nHead - 1) & BDL_MASK)) &&
       (stBDList.nBDEntries != MAX_BDL_ENTRIES - 1))
    {
        nCurrentIndex = stBDList.nHead;     // point to head
    }

    //
    // Move BD list to (0-((current - head) & mask)) & mask
    // where ((current - head) & mask) is the difference between head and
    // current index, no matter where they are :)
    //
    MoveBDList (stBDList.nHead, (stBDList.nTail - 1) & BDL_MASK,
            (0 - ((nCurrentIndex - stBDList.nHead) & BDL_MASK)) & BDL_MASK);

    //
    // Update structure.
    //
    stBDList.nHead = (0 - ((nCurrentIndex - stBDList.nHead) & BDL_MASK)) &
        BDL_MASK;
    stBDList.nTail = (stBDList.nHead + stBDList.nBDEntries) & BDL_MASK;

    //
    // Then we have to search for the tags. If we wouldn't have to rearrange the
    // scatter gather list all the time, then we could use the tag as an index
    // to the array, but the only way to clear DMA caches is a reset which has
    // the side-effect that we have to rearrange the BD list (see above).
    //
    // search for first...
    //
    nSearchIndex = stBDList.nHead;
    do
    {
        if ((void *)ULongToPtr(stBDList.pMapData[nSearchIndex].ulTag) == FirstTag)
            break;
        nSearchIndex = (nSearchIndex + 1) & BDL_MASK;
    } while (nSearchIndex != stBDList.nTail);
    nFirst = nSearchIndex;

    //
    // Search for last...
    //
    nSearchIndex = stBDList.nHead;
    do
    {
        if ((void *)ULongToPtr(stBDList.pMapData[nSearchIndex].ulTag) == LastTag)
            break;
        nSearchIndex = (nSearchIndex + 1) & BDL_MASK;
    } while (nSearchIndex != stBDList.nTail);
    nLast = nSearchIndex;


    //
    // Check search result.
    //
    if ((nFirst == stBDList.nTail) || (nLast == stBDList.nTail))
    {
        DOUT (DBG_ERROR, ("!!! Entry not found !!!"));

        //
        // restart DMA in case it was running
        //
        if ((ulOldDMAEngineState & DMA_ENGINE_ON) && stBDList.nBDEntries)
            ResumeDMA (DMA_ENGINE_PEND);
        *MappingsRevoked = 0;
        KeReleaseSpinLock (&MapLock, OldIrql);
        return STATUS_UNSUCCESSFUL;         // one of the tags not found
    }

    // Print the index numbers found.
    DOUT (DBG_DMA, ("Removing entries %d (%p) to %d (%p).", nFirst, FirstTag,
                  nLast, LastTag));

    //
    // Calculate the entries between the indizes.
    //
    if (nLast < nFirst)
    {
        nNumMappings = ((nLast + MAX_BDL_ENTRIES) - nFirst) + 1;
    }
    else
    {
        nNumMappings = (nLast - nFirst) + 1;
    }

    //
    // Print debug inormation.
    //
    DOUT (DBG_DMA, ("Found entries: %d-%d, %d entries.", nFirst, nLast,
                   nNumMappings));

    //
    // Now remove the revoked buffers.  Move the BD list and modify the
    // status information.
    //
    if (nFirst < stBDList.nTail)
    {
        //
        // In this case, both first and last are >= the current index (0)
        //
        if (nLast != ((stBDList.nTail - 1) & BDL_MASK))
        {
            //
            // Not the last entry, so move the BD list + mappings.
            //
            MoveBDList ((nLast + 1) & BDL_MASK, (stBDList.nTail - 1) & BDL_MASK,
                        nFirst);
        }
        stBDList.nTail = (stBDList.nTail - nNumMappings) & BDL_MASK;
    }

    //
    // In this case, at least first is "<" than current index (0)
    //
    else
    {
        //
        // Check for last.
        //
        if (nLast < stBDList.nTail)
        {
            //
            // Last is ">=" current index and first is "<" current index (0).
            // Remove MAX_DBL_ENTRIES - first entries in front of current index.
            //
            if (nFirst != stBDList.nHead)
            {
                //
                // Move from head towards current index.
                //
                MoveBDList (stBDList.nHead, nFirst - 1,
                           (stBDList.nHead + (MAX_BDL_ENTRIES - nFirst)) &
                           BDL_MASK);
            }

            //
            // Adjust head.
            //
            stBDList.nHead = (stBDList.nHead + (MAX_BDL_ENTRIES - nFirst)) &
                BDL_MASK;

            //
            // Remove nLast entries from CIV to tail.
            //
            if (nLast != ((stBDList.nTail - 1) & BDL_MASK))
            {
                //
                // Not the last entry, so move the BD list + mappings.
                //
                MoveBDList (nLast + 1, (stBDList.nTail - 1) & BDL_MASK, 0);
            }

            //
            // Adjust tail.
            //
            stBDList.nTail = (stBDList.nTail - (nLast + 1)) & BDL_MASK;
        }

        //
        // Last is "<" current index and first is "<" current index (0).
        //
        else
        {
            //
            // Remove nNumMappings entries in front of current index.
            //
            if (nFirst != stBDList.nHead)
            {
                //
                // Move from head towards current index.
                //
                MoveBDList (stBDList.nHead, nFirst - 1,
                           (nLast - nNumMappings) + 1);
            }

            //
            // Adjust head.
            //
            stBDList.nHead = (stBDList.nHead + nNumMappings) & BDL_MASK;
        }
    }

    //
    // In all cases, reduce the number of mappings.
    //
    stBDList.nBDEntries -= nNumMappings;

    //
    // Print debug information.
    //
    DOUT (DBG_DMA, ("Number of mappings is now %d, Head is %d, Tail is %d",
            stBDList.nBDEntries, stBDList.nHead, stBDList.nTail));

    //
    // Reprogram the last valid index only when tail != 0
    //
    if (stBDList.nTail)
    {
        Miniport->AdapterCommon->WriteBMControlRegister (m_ulBDAddr + X_LVI,
                    (UCHAR)((stBDList.nTail - 1) & BDL_MASK));
    }

    //
    // Just un-pause the DMA engine if it was running before and there are
    // still entries left and tail != 0.
    //
    if ((ulOldDMAEngineState & DMA_ENGINE_ON) && stBDList.nBDEntries
       && stBDList.nTail)
    {
        ResumeDMA (DMA_ENGINE_PEND);
    }

    //
    // Release the mapping spin lock and return the number of mappings we
    // revoked.
    //
    KeReleaseSpinLock (&MapLock, OldIrql);
    *MappingsRevoked = nNumMappings;


    return STATUS_SUCCESS;
}


/*****************************************************************************
 * CMiniportWaveICHStream::MappingAvailable
 *****************************************************************************
 * This routine is called by the port driver to notify the stream that there
 * are new mappings available.  Note that this is ONLY called after the stream
 * has previously had a GetMapping() call fail due to lack of available
 * mappings.
 */
STDMETHODIMP_(void) CMiniportWaveICHStream::MappingAvailable (void)
{
    DOUT (DBG_PRINT, ("MappingAvailable"));

    //
    // Release processed mappings.
    //
    ReleaseUsedMappings ();

    //
    // Process the new mappings.
    //
    GetNewMappings ();
}


/*****************************************************************************
 * CMiniportWaveICHStream::GetNewMappings
 *****************************************************************************
 * This routine is called when new mappings are available from the port driver.
 * The routine places mappings into the input mapping queue. AC97 can handle up
 * to 32 entries (descriptors). We program the DMA registers if we have at least
 * one mapping in the queue. The mapping spin lock must be held when calling
 * this routine.
 */
NTSTATUS CMiniportWaveICHStream::GetNewMappings (void)
{
    KIRQL OldIrql;

    NTSTATUS    ntStatus = STATUS_SUCCESS;
    ULONG       ulBytesMapped = 0;
    int         nInsertMappings = 0;
    int         nTail;                  // aut. variable

    DOUT (DBG_PRINT, ("[CMiniportWaveICHStream::GetNewMappings]"));

    // acquire the mapping spin lock
    KeAcquireSpinLock (&MapLock,&OldIrql);

#if (DBG)
    if (Miniport->AdapterCommon->ReadBMControlRegister16 (m_ulBDAddr + X_SR) & SR_CELV)
    {
        //
        // We starve.  :-(
        //
        DOUT (DBG_DMA, ("[GetNewMappings] We starved ... Head %d, Tail %d, Entries %d.",
                stBDList.nHead, stBDList.nTail, stBDList.nBDEntries));
    }
#endif

    //
    // Get available mappings up to the max of 31 that we can hold in the BDL.
    //
    while (stBDList.nBDEntries < (MAX_BDL_ENTRIES - 1))
    {
        //
        // Get the information from the list.
        //
        ULONG               Flags;
        ULONG               ulTag = stBDList.ulTagCounter++;
        ULONG               ulBufferLength;
        PHYSICAL_ADDRESS    PhysAddr;
        PVOID               VirtAddr;


        // Release the mapping spin lock
        KeReleaseSpinLock (&MapLock,OldIrql);

        //
        // Try to get the mapping from the port.
        // Here comes the problem: When calling GetMapping or ReleaseMapping we
        // cannot hold our spin lock. So we need to buffer the return values and
        // stick the information into the structure later when we have the spin
        // lock acquired.
        //
        ntStatus = PortStream->GetMapping ((PVOID)ULongToPtr(ulTag),
                                           (PPHYSICAL_ADDRESS)&PhysAddr,
                                           &VirtAddr,
                                           &ulBufferLength,
                                           &Flags);

        // Acquire the mapping spin lock
        KeAcquireSpinLock (&MapLock,&OldIrql);

        //
        // Quit this loop when we run out of mappings.
        //
        if (!NT_SUCCESS (ntStatus))
        {
            break;
        }

        // Sanity check: The audio stack will not give you data
        // that you cannot handle, but an application could use
        // DirectKS and send bad buffer down.

        // One mapping needs to be <0x1FFFF bytes for mono
        // streams on the AC97.
        if (ulBufferLength > 0x1FFFE)
        {
            // That is a little too long. That should never happen.
            DOUT (DBG_ERROR, ("[GetNewMappings] Buffer length too long!"));
            ulBufferLength = 0x1FFFE;
        }

        // The AC97 can only handle WORD aligned buffers.
        if (PhysAddr.LowPart & 0x01)
        {
            // we cannot play that! Set the buffer length to 0 so
            // that the HW will skip the buffer.
            DOUT (DBG_WARNING, ("[GetNewMappings] Buffer address unaligned!"));
            ulBufferLength = 0;
        }

        // The AC97 cannot handle unaligned mappings with respect
        // to the frame size (eg. 42 bytes on 4ch playback).
        if (ulBufferLength % NumberOfChannels)
        {
            // modify the length (don't play the rest of the bytes)
            DOUT (DBG_WARNING, ("[GetNewMappings] Buffer length unaligned!"));
            ulBufferLength -= ulBufferLength % NumberOfChannels;
        }

        //
        // Save the mapping.
        //
        nTail = stBDList.nTail;
        stBDList.pMapData[nTail].ulTag = ulTag;
        stBDList.pMapData[nTail].PhysAddr = PhysAddr;
        stBDList.pMapData[nTail].pVirtAddr = VirtAddr;
        stBDList.pMapData[nTail].ulBufferLength = ulBufferLength;
        ulBytesMapped += ulBufferLength;

        //
        // Fill in the BDL entry with pointer to physical address and length.
        //
        BDList[nTail].dwPtrToPhyAddress = PhysAddr.LowPart;
        BDList[nTail].wLength = (WORD)(ulBufferLength >> 1);
        BDList[nTail].wPolicyBits = BUP_SET;

        //
        // Generate an interrupt when portcls tells us to or roughly every 10ms.
        //
        if (Flags || (ulBytesMapped > (CurrentRate * NumberOfChannels * 2) / 100))
        {
            BDList[nTail].wPolicyBits |= IOC_ENABLE;
            ulBytesMapped = 0;
        }

        //
        // Take the new mapping into account.
        //
        stBDList.nTail = (stBDList.nTail + 1) & BDL_MASK;
        stBDList.nBDEntries++;
        TotalBytesMapped += (ULONGLONG)ulBufferLength;
        nInsertMappings++;

        //
        // Set last valid index (LVI) register! We need to do this here to avoid inconsistency
        // of the BDList with the HW. Note that we need to release spin locks every time
        // we call into portcls, that means we can be interrupted by ReleaseUsedMappings.
        //
        Miniport->AdapterCommon->WriteBMControlRegister (m_ulBDAddr + X_LVI, (UCHAR)nTail);
    }

    //
    // If there were processed mappings, print out some debug messages and eventually try to
    // restart DMA engine.
    //
    if (nInsertMappings)
    {
        //
        // Print debug information ...
        //
        DOUT (DBG_DMA, ("[GetNewMappings] Got %d mappings.", nInsertMappings));
        DOUT (DBG_DMA, ("[GetNewMappings] Head %d, Tail %d, Entries %d.",
                stBDList.nHead, stBDList.nTail, stBDList.nBDEntries));

        if(stBDList.nBDEntries >= 2)
            ResumeDMA (DMA_ENGINE_PAUSE);
    }

    // Release the mapping spin lock
    KeReleaseSpinLock (&MapLock,OldIrql);

    return ntStatus;
}


/*****************************************************************************
 * CMiniportWaveICHStream::ReleaseUsedMappings
 *****************************************************************************
 * This routine unmaps previously mapped memory that the hardware has
 * completed processing on.  This routine is typically called at DPC level
 * from the stream deferred procedure call that results from a stream
 * interrupt. The mapping spin lock must be held when calling this routine.
 */
NTSTATUS CMiniportWaveICHStream::ReleaseUsedMappings (void)
{
    KIRQL OldIrql;
    int   nMappingsReleased = 0;

    DOUT (DBG_PRINT, ("[CMiniportWaveICHStream::ReleaseUsedMappings]"));

    // acquire the mapping spin lock
    KeAcquireSpinLock (&MapLock,&OldIrql);

    //
    // Clean up everything to that index.
    //
    while (stBDList.nBDEntries)
    {
        //
        // Get current index
        //
        int nCurrentIndex = (int)Miniport->AdapterCommon->
            ReadBMControlRegister8 (m_ulBDAddr + X_CIV);

        //
        // When CIV is == Head -1 we released all mappings.
        //
        if (nCurrentIndex == ((stBDList.nHead - 1) & BDL_MASK))
        {
           break;
        }

        //
        // Check if CIV is between head and tail.
        //
        if (nCurrentIndex < stBDList.nHead)
        {
            //
            // Check for CIV being outside range.
            //
            if ((nCurrentIndex + MAX_BDL_ENTRIES) >=
                (stBDList.nHead + stBDList.nBDEntries))
            {
                DOUT (DBG_ERROR, ("[ReleaseUsedMappings] CIV out of range!"));
                break;
            }
        }
        else
        {
            //
            // Check for CIV being outside range.
            //
            if (nCurrentIndex >= (stBDList.nHead + stBDList.nBDEntries))
            {
                DOUT (DBG_ERROR, ("[ReleaseUsedMappings] CIV out of range!"));
                break;
            }
        }

        //
        // Check to see if we've released all the buffers.
        //
        if (stBDList.nHead == nCurrentIndex)
        {
            if (nCurrentIndex == ((stBDList.nTail - 1) & BDL_MASK))
            {
                //
                // A special case is starvation or stop of stream, when the
                // DMA engine finished playing the buffers, CVI is equal LVI
                // and SR_CELV is set.
                //
                if (!(Miniport->AdapterCommon->
                     ReadBMControlRegister16 (m_ulBDAddr + X_SR) & SR_CELV))
                {
                    // It is still playing the last buffer.
                    break;
                }

                //
                // In case the CVI=LVI bit is set, nBDEntries should be 1
                //
                if (stBDList.nBDEntries != 1)
                {
                    DOUT (DBG_ERROR, ("[ReleaseUsedMappings] Inconsitency: Tail reached and Entries != 1."));
                }
            }
            else
            {
                //
                // Bail out. Current Index did not move.
                //
                break;
            }
        }

        //
        // Save the tag and remove the entry from the list.
        //
        ULONG   ulTag = stBDList.pMapData[stBDList.nHead].ulTag;
        TotalBytesReleased += (ULONGLONG)stBDList.pMapData[stBDList.nHead].ulBufferLength;
        stBDList.nBDEntries--;
        stBDList.nHead = (stBDList.nHead + 1) & BDL_MASK;
        nMappingsReleased++;

        // Release the mapping spin lock
        KeReleaseSpinLock (&MapLock,OldIrql);

        //
        // Release this entry.
        //
        PortStream->ReleaseMapping ((PVOID)ULongToPtr(ulTag));

        // acquire the mapping spin lock
        KeAcquireSpinLock (&MapLock,&OldIrql);
    }


    // Print some debug information in case we released mappings.
    if (nMappingsReleased)
    {
        //
        // Print release information and return.
        //
        DOUT (DBG_DMA, ("[ReleaseUsedMappings] Head %d, Tail %d, Entries %d.",
                       stBDList.nHead, stBDList.nTail, stBDList.nBDEntries));
    }

    // Release the mapping spin lock
    KeReleaseSpinLock (&MapLock,OldIrql);

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * Non paged code begins here
 *****************************************************************************
 */

#ifdef _MSC_VER
#pragma code_seg()
#endif

void CMiniportWaveICHStream::InterruptServiceRoutine()
{
    //
    // Request DPC service for PCM out.
    //
    if ((Wave()->Port) && (ServiceGroup))
    {
        Wave()->Port->Notify (ServiceGroup);
    }
    else
    {
        //
        // Bad, bad.  Shouldn't print in an ISR!
        //
        DOUT (DBG_ERROR, ("WaveOut INT fired but no stream object there."));
    }
}
