/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/irpstream.cpp
 * PURPOSE:         IRP Stream handling
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#ifndef YDEBUG
#define NDEBUG
#endif

#include <debug.h>

static
PIRP
RemoveHeadList_IRP(
    IN OUT PLIST_ENTRY QueueHead)
{
    PIRP Irp;
    PLIST_ENTRY CurEntry;

    for (CurEntry = QueueHead->Flink; CurEntry != QueueHead; CurEntry = CurEntry->Flink)
    {
        /* Get the IRP offset */
        Irp = (PIRP)CONTAINING_RECORD(CurEntry, IRP, Tail.Overlay.ListEntry);

        /* Remove the cancel routine */
        if (IoSetCancelRoutine(Irp, NULL))
        {
            /* Remove the IRP from the list and return it */
            RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
            return Irp;
        }
    }

    /* no non canceled irp has been found */
    return NULL;
}
class CIrpQueue : public IIrpQueue
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    STDMETHODIMP_(ULONG) AddRef()
    {
        InterlockedIncrement(&m_Ref);
        return m_Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        InterlockedDecrement(&m_Ref);

        if (!m_Ref)
        {
            delete this;
            return 0;
        }
        return m_Ref;
    }
    IMP_IIrpQueue;
    CIrpQueue(IUnknown *OuterUnknown){}
    virtual ~CIrpQueue(){}

protected:

    PKSPIN_CONNECT m_ConnectDetails;
    PKSPIN_DESCRIPTOR m_Descriptor;

    KSPIN_LOCK m_IrpListLock;
    LIST_ENTRY m_IrpList;
    LIST_ENTRY m_FreeIrpList;

    ULONG m_MaxFrameSize;
    ULONG m_Alignment;
    ULONG m_TagSupportEnabled;

    ULONG m_StreamHeaderIndex;
    ULONG m_TagIndex;
    PKSSTREAM_HEADER m_CurStreamHeader;

    ULONG m_CurrentOffset;
    PIRP m_Irp;
    volatile LONG m_Ref;
};

typedef struct
{
    PVOID Tag;
    UCHAR Used;
}KSSTREAM_TAG, *PKSSTREAM_TAG;

typedef struct
{
    ULONG StreamHeaderCount;
    ULONG nTags;

    PVOID * Data;
    PKSSTREAM_TAG Tags;
}KSSTREAM_DATA, *PKSSTREAM_DATA;

#define STREAM_DATA_OFFSET   (0)


NTSTATUS
NTAPI
CIrpQueue::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PUNKNOWN(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CIrpQueue::Init(
    IN PKSPIN_CONNECT ConnectDetails,
    IN PKSPIN_DESCRIPTOR Descriptor,
    IN ULONG FrameSize,
    IN ULONG Alignment,
    IN ULONG TagSupportEnabled)
{
    m_ConnectDetails = ConnectDetails;
    m_Descriptor = Descriptor;
    m_MaxFrameSize = FrameSize;
    m_Alignment = Alignment;
    m_TagSupportEnabled = TagSupportEnabled;

    InitializeListHead(&m_IrpList);
    InitializeListHead(&m_FreeIrpList);
    KeInitializeSpinLock(&m_IrpListLock);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CIrpQueue::AddMapping(
    IN PIRP Irp,
    OUT PULONG Data)
{
    PKSSTREAM_HEADER Header;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION IoStack;
    ULONG Index, Length;
    PMDL Mdl;
    PKSSTREAM_DATA StreamData;
    LONG TotalStreamData;
    LONG StreamPageCount;
    LONG HeaderLength;

    PC_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    // allocate stream data
    StreamData = (PKSSTREAM_DATA)AllocateItem(NonPagedPool, sizeof(KSSTREAM_DATA), TAG_PORTCLASS);
    if (!StreamData)
    {
        // not enough memory
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // get current irp stack location
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // lets probe the irp
    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_WRITE_STREAM)
    {
        // probe IOCTL_KS_WRITE_STREAM
        Status = KsProbeStreamIrp(Irp, KSSTREAM_WRITE | KSPROBE_ALLOCATEMDL | KSPROBE_PROBEANDLOCK | KSPROBE_SYSTEMADDRESS, 0);
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_READ_STREAM)
    {
        // probe IOCTL_KS_READ_STREAM
        Status = KsProbeStreamIrp(Irp, KSSTREAM_READ  | KSPROBE_ALLOCATEMDL | KSPROBE_PROBEANDLOCK | KSPROBE_SYSTEMADDRESS, 0);
    }

    // check for success
    if (!NT_SUCCESS(Status))
    {
        // irp probing failed
        FreeItem(StreamData, TAG_PORTCLASS);
        return Status;
    }

    // get first stream header
    Header = (PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer;

    // sanity check
    PC_ASSERT(Header);

    // first calculate the numbers of stream headers
    Length = IoStack->Parameters.DeviceIoControl.OutputBufferLength;
    Mdl = Irp->MdlAddress;

    TotalStreamData = 0;
    StreamPageCount = 0;

    do
    {
        /* subtract size */
        Length -= Header->Size;

        /* increment header count */
        StreamData->StreamHeaderCount++;

        if (m_Descriptor->DataFlow == KSPIN_DATAFLOW_IN)
        {
            // irp sink
            HeaderLength = Header->DataUsed;
        }
        else
        {
            // irp source
            HeaderLength = Header->FrameExtent;
        }

        // increment available data
        TotalStreamData += HeaderLength;

        // append page count
        StreamPageCount += ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                               MmGetMdlByteOffset(Mdl), HeaderLength);

        // move to next header / mdl
        Mdl = Mdl->Next;
        Header = (PKSSTREAM_HEADER)((ULONG_PTR)Header + Header->Size);

    }while(Length);

    // sanity check
    ASSERT(StreamData->StreamHeaderCount);

    // allocate array for storing the pointers of the data */
    StreamData->Data = (PVOID*)AllocateItem(NonPagedPool, sizeof(PVOID) * StreamData->StreamHeaderCount, TAG_PORTCLASS);
    if (!StreamData->Data)
    {
        // out of memory
        FreeItem(StreamData, TAG_PORTCLASS);

        // done
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (m_TagSupportEnabled)
    {
        // allocate array for storing the pointers of the data */
        StreamData->Tags = (PKSSTREAM_TAG)AllocateItem(NonPagedPool, sizeof(KSSTREAM_TAG) * StreamPageCount, TAG_PORTCLASS);
        if (!StreamData->Data)
        {
            // out of memory
            FreeItem(StreamData->Data, TAG_PORTCLASS);
            FreeItem(StreamData, TAG_PORTCLASS);

            // done
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }


    // now get a system address for the user buffers
    Header = (PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer;
    Mdl = Irp->MdlAddress;

    for(Index = 0; Index < StreamData->StreamHeaderCount; Index++)
    {
        /* get system address */
        StreamData->Data[Index] = MmGetSystemAddressForMdlSafe(Mdl, NormalPagePriority);

        /* check for success */
        if (!StreamData->Data[Index])
        {
            // out of resources
            FreeItem(StreamData->Data, TAG_PORTCLASS);

            if (m_TagSupportEnabled)
            {
                // free tag array
                FreeItem(StreamData->Tags, TAG_PORTCLASS);
            }

            FreeItem(StreamData, TAG_PORTCLASS);
            // done
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        // move to next header / mdl
        Mdl = Mdl->Next;
        Header = (PKSSTREAM_HEADER)((ULONG_PTR)Header + Header->Size);

    }

    // store stream data
    Irp->Tail.Overlay.DriverContext[STREAM_DATA_OFFSET] = (PVOID)StreamData;

    *Data = TotalStreamData;

    // mark irp as pending
    IoMarkIrpPending(Irp);

    // add irp to cancelable queue
    KsAddIrpToCancelableQueue(&m_IrpList, &m_IrpListLock, Irp, KsListEntryTail, NULL);

    // done
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CIrpQueue::GetMapping(
    OUT PUCHAR * Buffer,
    OUT PULONG BufferSize)
{
    PIRP Irp;
    ULONG Offset;
    PKSSTREAM_DATA StreamData;

    // check if there is an irp in the partially processed
    if (m_Irp)
    {
        // use last irp
        if (m_Irp->Cancel == FALSE)
        {
            Irp = m_Irp;
            Offset = m_CurrentOffset;
        }
        else
        {
            // irp has been cancelled
            m_Irp->IoStatus.Status = STATUS_CANCELLED;
            IoCompleteRequest(m_Irp, IO_NO_INCREMENT);
            m_Irp = Irp = NULL;
            m_CurrentOffset = 0;
        }
    }
    else
    {
        // get a fresh new irp from the queue
        m_Irp = Irp = KsRemoveIrpFromCancelableQueue(&m_IrpList, &m_IrpListLock, KsListEntryHead, KsAcquireAndRemoveOnlySingleItem);
        m_CurrentOffset = Offset = 0;

        if (m_Irp)
        {
            // reset stream header index
            m_StreamHeaderIndex = 0;

            // reset stream header
            m_CurStreamHeader = (PKSSTREAM_HEADER)m_Irp->AssociatedIrp.SystemBuffer;
        }
    }

    if (!Irp)
    {
        // no irp buffer available
        return STATUS_UNSUCCESSFUL;
    }

    // get stream data
    StreamData = (PKSSTREAM_DATA)Irp->Tail.Overlay.DriverContext[STREAM_DATA_OFFSET];

    // sanity check
    PC_ASSERT(StreamData);

    // get buffer size
    if (m_Descriptor->DataFlow == KSPIN_DATAFLOW_IN)
    {
        // sink pin
        *BufferSize = m_CurStreamHeader->DataUsed - Offset;
    }
    else
    {
        // source pin
        *BufferSize = m_CurStreamHeader->FrameExtent - Offset;
    }

    // sanity check
    PC_ASSERT(*BufferSize);

    // store buffer
    *Buffer = &((PUCHAR)StreamData->Data[m_StreamHeaderIndex])[Offset];

    return STATUS_SUCCESS;
}

VOID
NTAPI
CIrpQueue::UpdateMapping(
    IN ULONG BytesWritten)
{
    PKSSTREAM_DATA StreamData;
    ULONG Size;
    PIO_STACK_LOCATION IoStack;
    ULONG Index;
    PMDL Mdl;

    // sanity check
    ASSERT(m_Irp);

    // get stream data
    StreamData = (PKSSTREAM_DATA)m_Irp->Tail.Overlay.DriverContext[STREAM_DATA_OFFSET];

    // sanity check
    ASSERT(StreamData);

    // add to current offset
    m_CurrentOffset += BytesWritten;

    if (m_Descriptor->DataFlow == KSPIN_DATAFLOW_OUT)
    {
        // store written bytes (source pin)
        m_CurStreamHeader->DataUsed += BytesWritten;
    }

    // get audio buffer size
    if (m_Descriptor->DataFlow == KSPIN_DATAFLOW_OUT)
        Size = m_CurStreamHeader->FrameExtent;
    else
        Size = m_CurStreamHeader->DataUsed;

    // sanity check
    PC_ASSERT(Size);

    if (m_CurrentOffset >= Size)
    {
        // sanity check
        PC_ASSERT(Size == m_CurrentOffset);

        if (m_StreamHeaderIndex + 1 < StreamData->StreamHeaderCount)
        {
            // move to next stream header
            m_CurStreamHeader = (PKSSTREAM_HEADER)((ULONG_PTR)m_CurStreamHeader + m_CurStreamHeader->Size);

            // increment stream header index
            m_StreamHeaderIndex++;

            // reset offset
            m_CurrentOffset = 0;

            // done
            return;
        }

        //
        // all stream buffers have been played
        // check if this is a looped buffer
        //
        if (m_ConnectDetails->Interface.Id == KSINTERFACE_STANDARD_LOOPED_STREAMING)
        {
            // looped streaming repeat the buffers untill
            // the caller decides to stop the streams

            // re-insert irp
            KsAddIrpToCancelableQueue(&m_IrpList, &m_IrpListLock, m_Irp, KsListEntryTail, NULL);

            // clear current irp
            m_Irp = NULL;

            // reset offset
            m_CurrentOffset = 0;

            // done
            return;
        }

        Mdl = m_Irp->MdlAddress;
        for(Index = 0; Index < StreamData->StreamHeaderCount; Index++)
        {
            MmUnmapLockedPages(StreamData->Data[Index], Mdl);
            Mdl = Mdl->Next;
        }

        // free stream data array
        FreeItem(StreamData->Data, TAG_PORTCLASS);

        if (m_TagSupportEnabled)
        {
            // free tag array
            FreeItem(StreamData->Tags, TAG_PORTCLASS);
        }

        // free stream data
        FreeItem(StreamData, TAG_PORTCLASS);

        // get io stack
        IoStack = IoGetCurrentIrpStackLocation(m_Irp);

        // store operation status
        m_Irp->IoStatus.Status = STATUS_SUCCESS;

        // store operation length
        m_Irp->IoStatus.Information = IoStack->Parameters.DeviceIoControl.OutputBufferLength;

        // complete the request
        IoCompleteRequest(m_Irp, IO_SOUND_INCREMENT);

        // remove irp as it is complete
        m_Irp = NULL;

        // reset offset
        m_CurrentOffset = 0;
    }
}

ULONG
NTAPI
CIrpQueue::NumData()
{
    KIRQL OldLevel;
    ULONG NumDataAvailable;
    PLIST_ENTRY CurEntry;
    PIRP Irp;
    ULONG CurrentOffset;
    ULONG StreamHeaderIndex;
    PKSSTREAM_HEADER CurStreamHeader;
    PKSSTREAM_DATA StreamData;
    ULONG Size;

    KeAcquireSpinLock(&m_IrpListLock, &OldLevel);

    NumDataAvailable = 0;
    CurEntry = &m_IrpList;

    // current IRP state
    Irp = m_Irp;
    CurrentOffset = m_CurrentOffset;
    StreamHeaderIndex = m_StreamHeaderIndex;
    CurStreamHeader = m_CurStreamHeader;

    while (TRUE)
    {
        if (Irp != NULL)
        {
            // get stream data
            StreamData = (PKSSTREAM_DATA)Irp->Tail.Overlay.DriverContext[STREAM_DATA_OFFSET];

            // loop over stream headers
            for (; StreamHeaderIndex < StreamData->StreamHeaderCount; StreamHeaderIndex++)
            {
                // get audio buffer size
                if (m_Descriptor->DataFlow == KSPIN_DATAFLOW_OUT)
                    Size = CurStreamHeader->FrameExtent;
                else
                    Size = CurStreamHeader->DataUsed;

                // increment available data
                NumDataAvailable += Size - CurrentOffset;
                CurrentOffset = 0;

                // move to next stream header
                CurStreamHeader = (PKSSTREAM_HEADER)((ULONG_PTR)CurStreamHeader + CurStreamHeader->Size);
            }
        }

        /* iterate to next entry */
        CurEntry = CurEntry->Flink;

        /* is the end of list reached */
        if (CurEntry == &m_IrpList)
            break;

        /* get irp offset */
        Irp = (PIRP)CONTAINING_RECORD(CurEntry, IRP, Tail.Overlay.ListEntry);

        // next IRP state
        CurrentOffset = 0;
        StreamHeaderIndex = 0;
        CurStreamHeader = (PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer;
    }

    KeReleaseSpinLock(&m_IrpListLock, OldLevel);
    return NumDataAvailable;
}

BOOL
NTAPI
CIrpQueue::CancelBuffers()
{
    //TODO: own cancel routine

    // is there an active irp
    if (m_Irp)
    {
        // re-insert it to cancelable queue
        KsAddIrpToCancelableQueue(&m_IrpList, &m_IrpListLock, m_Irp, KsListEntryTail, NULL);
        //set it to zero
        m_Irp = NULL;
    }

    // cancel all irps
    KsCancelIo(&m_IrpList, &m_IrpListLock);

    // done
    return TRUE;
}

NTSTATUS
NTAPI
CIrpQueue::GetMappingWithTag(
    IN PVOID Tag,
    OUT PPHYSICAL_ADDRESS  PhysicalAddress,
    OUT PVOID  *VirtualAddress,
    OUT PULONG  ByteCount,
    OUT PULONG  Flags)
{
    PKSSTREAM_DATA StreamData;
    KIRQL OldLevel;
    ULONG Size;
    LPBYTE Data;

    /* sanity checks */
    PC_ASSERT(PhysicalAddress);
    PC_ASSERT(VirtualAddress);
    PC_ASSERT(ByteCount);
    PC_ASSERT(Flags);

    KeAcquireSpinLock(&m_IrpListLock, &OldLevel);

    if (!m_Irp)
    {
        // get an irp from the queue
        m_Irp = RemoveHeadList_IRP(&m_IrpList);

        // check if there is an irp
        if (!m_Irp)
        {
            // no irp available
            KeReleaseSpinLock(&m_IrpListLock, OldLevel);

            DPRINT("GetMappingWithTag no mapping available\n");
            return STATUS_NOT_FOUND;
        }

        // reset offset
        m_CurrentOffset = 0;

        // reset tag index
        m_TagIndex = 0;

        // reset stream header index
        m_StreamHeaderIndex = 0;

        // reset stream header
        m_CurStreamHeader = (PKSSTREAM_HEADER)m_Irp->AssociatedIrp.SystemBuffer;
    }

    // get stream data
    StreamData = (PKSSTREAM_DATA)m_Irp->Tail.Overlay.DriverContext[STREAM_DATA_OFFSET];

    // sanity check
    PC_ASSERT(m_StreamHeaderIndex < StreamData->StreamHeaderCount);

    // store tag in irp
    StreamData->Tags[m_TagIndex].Tag = Tag;
    StreamData->Tags[m_TagIndex].Used = TRUE;
    m_TagIndex++;

    // get audio buffer size
    if (m_Descriptor->DataFlow == KSPIN_DATAFLOW_OUT)
        Size = m_CurStreamHeader->FrameExtent;
    else
        Size = m_CurStreamHeader->DataUsed;

    // sanity check
    PC_ASSERT(Size);

    // setup mapping
    Data = (LPBYTE)StreamData->Data[m_StreamHeaderIndex] + m_CurrentOffset;
    *VirtualAddress = Data;

    // get byte count
    *ByteCount = (LPBYTE)ROUND_TO_PAGES(Data+1)-Data;
    if (*ByteCount > (Size - m_CurrentOffset))
        *ByteCount = (Size - m_CurrentOffset);
    m_CurrentOffset += *ByteCount;

    if (m_CurrentOffset >= Size)
    {
        // sanity check
        PC_ASSERT(Size == m_CurrentOffset);

        // increment header index
        m_StreamHeaderIndex++;

        if (m_StreamHeaderIndex == StreamData->StreamHeaderCount)
        {
            // last mapping
            *Flags = 1;

            //
            StreamData->nTags = m_TagIndex;

            // insert mapping into free list
            InsertTailList(&m_FreeIrpList, &m_Irp->Tail.Overlay.ListEntry);

            // clear irp
            m_Irp = NULL;

        }
        else
        {
            // one more mapping in the irp
            *Flags = 0;

            // move to next header
            m_CurStreamHeader = (PKSSTREAM_HEADER)((ULONG_PTR)m_CurStreamHeader + m_CurStreamHeader->Size);
        }
    }

    // get physical address
    *PhysicalAddress = MmGetPhysicalAddress(*VirtualAddress);

    KeReleaseSpinLock(&m_IrpListLock, OldLevel);

    DPRINT("GetMappingWithTag Tag %p Buffer %p Flags %lu ByteCount %lx\n", Tag, VirtualAddress, *Flags, *ByteCount);
    // done
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CIrpQueue::ReleaseMappingWithTag(
    IN PVOID Tag)
{
    PIRP Irp;
    PLIST_ENTRY CurEntry;
    PKSSTREAM_DATA StreamData;
    PIO_STACK_LOCATION IoStack;
    ULONG Index;
    KIRQL OldLevel;

    KeAcquireSpinLock(&m_IrpListLock, &OldLevel);

    // check if used list empty
    if (IsListEmpty(&m_FreeIrpList))
    {
        // get current irp
        if (!m_Irp)
        {
            KeReleaseSpinLock(&m_IrpListLock, OldLevel);

            // this should not happen
            DPRINT("ReleaseMappingWithTag Tag %p not found\n", Tag);
            return STATUS_NOT_FOUND;
        }

        Irp = m_Irp;
    }
    else
    {
        // remove irp from used list
        CurEntry = RemoveHeadList(&m_FreeIrpList);

        // get irp from list entry
        Irp = (PIRP)CONTAINING_RECORD(CurEntry, IRP, Tail.Overlay.ListEntry);
    }

    // get stream data
    StreamData = (PKSSTREAM_DATA)Irp->Tail.Overlay.DriverContext[STREAM_DATA_OFFSET];

    // release oldest in use mapping
    for (Index = 0; Index < StreamData->nTags; Index++)
    {
        if (StreamData->Tags[Index].Used != FALSE)
        {
            StreamData->Tags[Index].Used = FALSE;

            // Warn if wrong mapping released
            if (StreamData->Tags[Index].Tag != Tag)
            {
                DPRINT1("Mapping released out of order\n");
            }

            break;
        }
    }

    // If this is the current IRP, do not complete
    if (Irp == m_Irp)
    {
        KeReleaseSpinLock(&m_IrpListLock, OldLevel);
        return STATUS_SUCCESS;
    }

    // check if this is the last one released mapping
    if (Index + 1 == StreamData->nTags)
    {
        // last mapping released
        // now check if this is a looped buffer
        if (m_ConnectDetails->Interface.Id == KSINTERFACE_STANDARD_LOOPED_STREAMING)
        {
            // looped buffers are not completed when they have been played
            // they are completed when the stream is set to stop

            KeReleaseSpinLock(&m_IrpListLock, OldLevel);

            // re-insert irp
            KsAddIrpToCancelableQueue(&m_IrpList, &m_IrpListLock, Irp, KsListEntryTail, NULL);

            // done
            return STATUS_SUCCESS;
        }

        //
        // time to complete non looped buffer
        //

        KeReleaseSpinLock(&m_IrpListLock, OldLevel);

        // free stream data array
        FreeItem(StreamData->Data, TAG_PORTCLASS);

        // free stream tags array
        FreeItem(StreamData->Tags, TAG_PORTCLASS);

        // free stream data
        FreeItem(StreamData, TAG_PORTCLASS);

        // get io stack
        IoStack = IoGetCurrentIrpStackLocation(Irp);

        // store operation status
        Irp->IoStatus.Status = STATUS_SUCCESS;

        // store operation length
        Irp->IoStatus.Information = IoStack->Parameters.DeviceIoControl.OutputBufferLength;

        // complete the request
        IoCompleteRequest(Irp, IO_SOUND_INCREMENT);
    }
    else
    {
        // there are still some headers not consumed
        InsertHeadList(&m_FreeIrpList, &Irp->Tail.Overlay.ListEntry);

        KeReleaseSpinLock(&m_IrpListLock, OldLevel);
    }

    return STATUS_SUCCESS;
}

ULONG
NTAPI
CIrpQueue::GetCurrentIrpOffset()
{

    return m_CurrentOffset;
}

BOOLEAN
NTAPI
CIrpQueue::GetAcquiredTagRange(
    IN PVOID * FirstTag,
    IN PVOID * LastTag)
{
    KIRQL OldLevel;
    BOOLEAN Ret = FALSE;
    //PIRP Irp;
    //PLIST_ENTRY CurEntry;
    //PKSSTREAM_DATA StreamData;

    // lock list
    KeAcquireSpinLock(&m_IrpListLock, &OldLevel);

    // initialize to zero
    *FirstTag = NULL;
    *LastTag = NULL;

    UNIMPLEMENTED;

    // release lock
    KeReleaseSpinLock(&m_IrpListLock, OldLevel);
    // done
    return Ret;
}

NTSTATUS
NTAPI
NewIrpQueue(
    IN IIrpQueue **Queue)
{
    CIrpQueue *This = new(NonPagedPool, TAG_PORTCLASS)CIrpQueue(NULL);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->AddRef();

    *Queue = (IIrpQueue*)This;
    return STATUS_SUCCESS;
}
