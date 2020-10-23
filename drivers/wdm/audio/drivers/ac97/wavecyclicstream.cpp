/********************************************************************************
**    Copyright (c) 1998-2000 Microsoft Corporation. All Rights Reserved.
**
**       Portions Copyright (c) 1998-1999 Intel Corporation
**
********************************************************************************/

// Every debug output has "Modulname text"
#define STR_MODULENAME "AC97 Cyclic Stream: "

#include "wavecyclicminiport.h"
#include "wavecyclicstream.h"
#define BUFFERTIME 10
#define NBUFFERS 4

#ifdef _MSC_VER
#pragma code_seg("PAGE")
#endif

IMP_CMiniportStream_QueryInterface(CMiniportWaveCyclicStream, IMiniportWavePciStream);
IMP_CMiniport_SetState(CMiniportWaveCyclicStream);

/*****************************************************************************
 * General Info
 *****************************************************************************
 * To protect the stBDList structure that is used to store mappings, we use a
 * spin lock called MapLock. This spin lock is also acquired when we change
 * the DMA registers. Normally, changes in stBDList and the DMA registers go
 * hand in hand. In case we only want to change the DMA registers, we need
 * to acquire the spin lock!
 */

/*****************************************************************************
 * CreateMiniportWaveCyclicStream
 *****************************************************************************
 * Creates a wave miniport stream object for the AC97 audio adapter. This is
 * (nearly) like the macro STD_CREATE_BODY_ from STDUNK.H.
 */
NTSTATUS CreateMiniportWaveCyclicStream
(
    OUT CMiniportWaveCyclicStream  **MiniportCyclicStream,
    IN  PUNKNOWN                pUnknownOuter,
    _When_((PoolType & NonPagedPoolMustSucceed) != 0,
       __drv_reportError("Must succeed pool allocations are forbidden. "
			 "Allocation failures cause a system crash"))
    IN  POOL_TYPE               PoolType
)
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CreateMiniportWaveCyclicStream]"));

    //
    // This is basically like the macro at stdunk with the change that we
    // don't cast to interface unknown but to interface MiniportIchStream.
    //
    *MiniportCyclicStream = new (PoolType, PoolTag)
                        CMiniportWaveCyclicStream (pUnknownOuter);
    if (*MiniportCyclicStream)
    {
        (*MiniportCyclicStream)->AddRef ();
        return STATUS_SUCCESS;
    }

    return STATUS_INSUFFICIENT_RESOURCES;
}


/*****************************************************************************
 * CMiniportWaveCyclicStream::~CMiniportWaveCyclicStream
 *****************************************************************************
 * Destructor
 */
CMiniportWaveCyclicStream::~CMiniportWaveCyclicStream ()
{
    PAGED_CODE ();


    DOUT (DBG_PRINT, ("[CMiniportWaveCyclicStream::~CMiniportWaveCyclicStream]"));

    // Release the scatter/gather table.
    BDList_Free();
}

/*****************************************************************************
 * CMiniportWaveCyclicStream::Init
 *****************************************************************************
 * This routine initializes the stream object, sets up the BDL, and programs
 * the buffer descriptor list base address register for the pin being
 * initialized.
 */
NTSTATUS CMiniportWaveCyclicStream::Init_()
{
    PAGED_CODE ();

    DOUT (DBG_PRINT, ("[CMiniportWaveCyclicStream::Init]"));

    //
    // Setup the Buffer Descriptor List (BDL)
    // Allocate 32 entries of 8 bytes (one BDL entry).
    // The pointer is aligned on a 8 byte boundary (that's what we need).
    //

    if (!BDList_Alloc())
    {
        DOUT (DBG_ERROR, ("Failed AllocateCommonBuffer!"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return ResizeBuffer();
}

STDMETHODIMP_(NTSTATUS) CMiniportWaveCyclicStream::SetFormat
(
    _In_  PKSDATAFORMAT   Format
)
{
    NTSTATUS ntStatus = CMiniportStream::SetFormat(Format);
    if (!NT_SUCCESS (ntStatus))
        return ntStatus;
    return ResizeBuffer();
}

NTSTATUS CMiniportWaveCyclicStream::ResizeBuffer(void)
{
    // calculate buffer size
    DWORD nSamples = DataFormat->WaveFormatEx.nSamplesPerSec * BUFFERTIME / 1000;
    DWORD bufferSize = DataFormat->WaveFormatEx.nBlockAlign * nSamples;
    DWORD totalSize = bufferSize * NBUFFERS;

    // allocate buffer
    PDMACHANNEL DmaChannel = Miniport->DmaChannel;
    DWORD allocSize = DmaChannel->AllocatedBufferSize();
    if(totalSize > allocSize)
    {
        DmaChannel->FreeBuffer();
        NTSTATUS ntStatus = DmaChannel->AllocateBuffer(totalSize, 0);
        if (!NT_SUCCESS (ntStatus)) {
            m_bufferSize = 0;
            return ntStatus;
        }
    }

    DmaChannel->SetBufferSize(totalSize);

    // initialize bdList
    DWORD addr = DmaChannel->PhysicalAddress().LowPart;
    for (UINT loop = 0; loop < MAX_BDL_ENTRIES; loop++)
    {
        BDList[loop].dwPtrToPhyAddress = addr + bufferSize * (loop % NBUFFERS);
        BDList[loop].wLength = (WORD)bufferSize/2;
        BDList[loop].wPolicyBits = BUP_SET | IOC_ENABLE;
    }

    // update buffer size
    m_bufferSize = bufferSize;
    return STATUS_SUCCESS;
}


STDMETHODIMP_(ULONG) CMiniportWaveCyclicStream::SetNotificationFreq
(
  _In_   ULONG  Interval,
  _Out_  PULONG FrameSize
)
{
  *FrameSize = m_bufferSize;
  return BUFFERTIME;
}

/*****************************************************************************
 * Non paged code begins here
 *****************************************************************************
 */

#ifdef _MSC_VER
#pragma code_seg()
#endif

IMP_CMiniport_NormalizePhysicalPosition(CMiniportWaveCyclicStream);

STDMETHODIMP_(void) CMiniportWaveCyclicStream::Silence
(
    _In_ PVOID Buffer,
    _In_ ULONG ByteCount
)
{
    memset(Buffer, 0, ByteCount);
}

/*****************************************************************************
 * CMiniportWaveCyclicStream::GetPosition
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
STDMETHODIMP_(NTSTATUS) CMiniportWaveCyclicStream::GetPosition
(
    _Out_ PULONG Position
)
{
    UCHAR   nCurrentIndex ;

    ASSERT (Position);

    nCurrentIndex = GetBuffPos((DWORD*)Position);
    nCurrentIndex %= NBUFFERS;
    *Position += nCurrentIndex * m_bufferSize;

    return STATUS_SUCCESS;
}

void CMiniportWaveCyclicStream::InterruptServiceRoutine()
{
    //
    // Update the LVI so that we cycle around in the scatter gather list.
    //
    UpdateLviCyclic();

    //
    // Request DPC service for PCM out.
    //
    Miniport->Port->Notify (ServiceGroup);
}
