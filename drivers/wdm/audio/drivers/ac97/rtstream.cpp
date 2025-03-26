/********************************************************************************
**    Copyright (c) 1998-2000 Microsoft Corporation. All Rights Reserved.
**
**       Portions Copyright (c) 1998-1999 Intel Corporation
**
********************************************************************************/

/* The file rtstream.cpp was reviewed by LCA in June 2011 and is acceptable for use by Microsoft. */

// Every debug output has "Modulname text"
#define STR_MODULENAME "AC97 RT Stream: "

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

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif
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
    
    
    return CMiniportStream::Init(Miniport_, 
                                 Channel_, 
                                 Capture_, 
                                 DataFormat_, 
                                 NULL);
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

#ifdef _MSC_VER
#pragma code_seg()
#endif



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
    DWORD   bufferPos;

    ASSERT (Position);

    if (DMAEngineState == DMA_ENGINE_OFF)
    {
        Position->PlayOffset = 0;
        Position->WriteOffset = 0;
    }
    else
    {
        nCurrentIndex = GetBuffPos(&bufferPos);
        Position->PlayOffset = bufferPos;
        Position->WriteOffset = bufferPos + NumberOfChannels * 2 * 8;
    }

    return STATUS_SUCCESS;
}

+/*****************************************************************************
+ * Non paged code begins here
+ *****************************************************************************
+ */

#ifdef _MSC_VER
#pragma code_seg()
#endif

void CMiniportWaveICHStream::InterruptServiceRoutine()
{
    //
    // Update the LVI so that we cycle around in the scatter gather list.
    //
    UpdateLviCyclic();
}

#endif          // (NTDDI_VERSION >= NTDDI_VISTA)
