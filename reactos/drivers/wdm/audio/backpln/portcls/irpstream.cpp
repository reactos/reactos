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

    BOOLEAN m_OutOfMapping;
    ULONG m_MaxFrameSize;
    ULONG m_Alignment;
    ULONG m_TagSupportEnabled;
    volatile ULONG m_NumDataAvailable;
    volatile ULONG m_CurrentOffset;
    volatile PIRP m_Irp;
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
    ULONG StreamHeaderIndex;
    ULONG TotalStreamData;

    PKSSTREAM_HEADER CurStreamHeader;
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

    // store header
    StreamData->CurStreamHeader = Header;

    // sanity check
    PC_ASSERT(Header);

    // first calculate the numbers of stream headers
    Length = IoStack->Parameters.DeviceIoControl.OutputBufferLength;

    do
    {
        /* subtract size */
        Length -= Header->Size;

        /* increment header count */
        StreamData->StreamHeaderCount++;

        if (m_Descriptor->DataFlow == KSPIN_DATAFLOW_IN)
        {
            // irp sink
            StreamData->TotalStreamData += Header->DataUsed;
        }
        else
        {
            // irp source
            StreamData->TotalStreamData += Header->FrameExtent;
        }

        /* move to next header */
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
        StreamData->Tags = (PKSSTREAM_TAG)AllocateItem(NonPagedPool, sizeof(KSSTREAM_TAG) * StreamData->StreamHeaderCount, TAG_PORTCLASS);
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

        if (m_Descriptor->DataFlow == KSPIN_DATAFLOW_IN)
        {
            // increment available data
            InterlockedExchangeAdd((PLONG)&m_NumDataAvailable, Header->DataUsed);
        }
        else if (m_Descriptor->DataFlow == KSPIN_DATAFLOW_OUT)
        {
            // increment available data
            InterlockedExchangeAdd((PLONG)&m_NumDataAvailable, Header->FrameExtent);
        }

        // move to next header / mdl
        Mdl = Mdl->Next;
        Header = (PKSSTREAM_HEADER)((ULONG_PTR)Header + Header->Size);

    }

    // store stream data
    Irp->Tail.Overlay.DriverContext[STREAM_DATA_OFFSET] = (PVOID)StreamData;

    *Data = StreamData->TotalStreamData;

    // mark irp as pending
    IoMarkIrpPending(Irp);

    // add irp to cancelable queue
    KsAddIrpToCancelableQueue(&m_IrpList, &m_IrpListLock, Irp, KsListEntryTail, NULL);

    // disable mapping failed status
    m_OutOfMapping = FALSE;

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
        *BufferSize = StreamData->CurStreamHeader->DataUsed - Offset;
    }
    else
    {
        // source pin
        *BufferSize = StreamData->CurStreamHeader->FrameExtent - Offset;
    }

    // sanity check
    PC_ASSERT(*BufferSize);

    // store buffer
    *Buffer = &((PUCHAR)StreamData->Data[StreamData->StreamHeaderIndex])[Offset];

    // unset flag that no irps are available
    m_OutOfMapping = FALSE;

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
    InterlockedExchangeAdd((PLONG)&m_CurrentOffset, (LONG)BytesWritten);

    if (m_Descriptor->DataFlow == KSPIN_DATAFLOW_OUT)
    {
        // store written bytes (source pin)
        StreamData->CurStreamHeader->DataUsed += BytesWritten;
    }

    // decrement available data counter
    m_NumDataAvailable -= BytesWritten;

    // get audio buffer size
    if (m_Descriptor->DataFlow == KSPIN_DATAFLOW_OUT)
        Size = StreamData->CurStreamHeader->FrameExtent;
    else
        Size = StreamData->CurStreamHeader->DataUsed;

    // sanity check
    PC_ASSERT(Size);

    if (m_CurrentOffset >= Size)
    {
        // sanity check
        PC_ASSERT(Size == m_CurrentOffset);

        if (StreamData->StreamHeaderIndex + 1 < StreamData->StreamHeaderCount)
        {
            // move to next stream header
            StreamData->CurStreamHeader = (PKSSTREAM_HEADER)((ULONG_PTR)StreamData->CurStreamHeader + StreamData->CurStreamHeader->Size);

            // increment stream header index
            StreamData->StreamHeaderIndex++;

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

            // reset stream header index
            StreamData->StreamHeaderIndex = 0;

            // reset stream header
            StreamData->CurStreamHeader = (PKSSTREAM_HEADER)m_Irp->AssociatedIrp.SystemBuffer;

            // increment available data
            InterlockedExchangeAdd((PLONG)&m_NumDataAvailable, StreamData->TotalStreamData);

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
    // returns the amount of audio stream data available
    return m_NumDataAvailable;
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

    // reset number of data available
    m_NumDataAvailable = 0;

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

    /* sanity checks */
    PC_ASSERT(PhysicalAddress);
    PC_ASSERT(VirtualAddress);
    PC_ASSERT(ByteCount);
    PC_ASSERT(Flags);

    if (!m_Irp)
    {
        // get an irp from the queue
        m_Irp = KsRemoveIrpFromCancelableQueue(&m_IrpList, &m_IrpListLock, KsListEntryHead, KsAcquireAndRemoveOnlySingleItem);
    }

    // check if there is an irp
    if (!m_Irp)
    {
        // no irp available
        m_OutOfMapping = TRUE;
        DPRINT("GetMappingWithTag no mapping available\n");
        return STATUS_NOT_FOUND;
    }

    // get stream data
    StreamData = (PKSSTREAM_DATA)m_Irp->Tail.Overlay.DriverContext[STREAM_DATA_OFFSET];

    // sanity check
    PC_ASSERT(StreamData->StreamHeaderIndex < StreamData->StreamHeaderCount);

    // setup mapping
    *PhysicalAddress = MmGetPhysicalAddress(StreamData->Data[StreamData->StreamHeaderIndex]);
    *VirtualAddress = StreamData->Data[StreamData->StreamHeaderIndex];

    // store tag in irp
    StreamData->Tags[StreamData->StreamHeaderIndex].Tag = Tag;
    StreamData->Tags[StreamData->StreamHeaderIndex].Used = TRUE;

    // increment header index
    StreamData->StreamHeaderIndex++;

    // mapping size
    if (m_Descriptor->DataFlow == KSPIN_DATAFLOW_IN)
    {
        // sink pin
        *ByteCount = StreamData->CurStreamHeader->DataUsed;

        // decrement num data available
        m_NumDataAvailable -= StreamData->CurStreamHeader->DataUsed;
    }
    else
    {
        // source pin
        *ByteCount = StreamData->CurStreamHeader->FrameExtent;

        // decrement num data available
        m_NumDataAvailable -= StreamData->CurStreamHeader->FrameExtent;
    }

    if (StreamData->StreamHeaderIndex == StreamData->StreamHeaderCount)
    {
        // last mapping
        *Flags = 1;

        // insert mapping into free list
        ExInterlockedInsertTailList(&m_FreeIrpList, &m_Irp->Tail.Overlay.ListEntry, &m_IrpListLock);

        // clear irp
        m_Irp = NULL;

    }
    else
    {
        // one more mapping in the irp
        *Flags = 0;

        // move to next header
        StreamData->CurStreamHeader = (PKSSTREAM_HEADER)((ULONG_PTR)StreamData->CurStreamHeader + StreamData->CurStreamHeader->Size);
    }

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

    // first check if there is an active irp
    if (m_Irp)
    {
        // now check if there are already used mappings
        StreamData = (PKSSTREAM_DATA)m_Irp->Tail.Overlay.DriverContext[STREAM_DATA_OFFSET];

        if (StreamData->StreamHeaderIndex)
        {
            // check if the released mapping is one current processed irps
            for(Index = 0; Index < StreamData->StreamHeaderIndex; Index++)
            {
                // check if it is the same tag
                if (StreamData->Tags[Index].Tag == Tag && StreamData->Tags[Index].Used == TRUE)
                {
                    // mark mapping as released
                    StreamData->Tags[Index].Tag = NULL;
                    StreamData->Tags[Index].Used = FALSE;

                    // done
                    return STATUS_SUCCESS;
                }

            }
        }
    }

    // remove irp from used list
    CurEntry = ExInterlockedRemoveHeadList(&m_FreeIrpList, &m_IrpListLock);
    if (CurEntry == NULL)
    {
        // this should not happen
        DPRINT("ReleaseMappingWithTag Tag %p not found\n", Tag);
        return STATUS_NOT_FOUND;
    }

    // sanity check
    PC_ASSERT(CurEntry);

    // get irp from list entry
    Irp = (PIRP)CONTAINING_RECORD(CurEntry, IRP, Tail.Overlay.ListEntry);

    // get stream data
    StreamData = (PKSSTREAM_DATA)Irp->Tail.Overlay.DriverContext[STREAM_DATA_OFFSET];

    // sanity check
    PC_ASSERT(StreamData->StreamHeaderIndex == StreamData->StreamHeaderCount);

    // check if the released mapping is one of these
    for(Index = 0; Index < StreamData->StreamHeaderCount; Index++)
    {
        if (StreamData->Tags[Index].Tag == Tag &&
            StreamData->Tags[Index].Used == TRUE)
        {
            // mark mapping as released
            StreamData->Tags[Index].Tag = NULL;
            StreamData->Tags[Index].Used = FALSE;

            // done
            break;
        }
        else
        {
            //
            // we assume that mappings are released in the same order as they have been acquired
            // therefore if the current mapping is not the searched one, it must have been already
            // released
            //
            ASSERT(StreamData->Tags[Index].Tag == NULL);
            ASSERT(StreamData->Tags[Index].Used == FALSE);
        }
    }

    // check if this is the last one released mapping
    if (Index + 1 == StreamData->StreamHeaderCount)
    {
        // last mapping released
        // now check if this is a looped buffer
        if (m_ConnectDetails->Interface.Id == KSINTERFACE_STANDARD_LOOPED_STREAMING)
        {
            // looped buffers are not completed when they have been played
            // they are completed when the stream is set to stop

            // reset stream header index
            StreamData->StreamHeaderIndex = 0;

            // reset stream header
            StreamData->CurStreamHeader = (PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer;

            // increment available data
            InterlockedExchangeAdd((PLONG)&m_NumDataAvailable, StreamData->TotalStreamData);

            // re-insert irp
            KsAddIrpToCancelableQueue(&m_IrpList, &m_IrpListLock, Irp, KsListEntryTail, NULL);

            // done
            return STATUS_SUCCESS;
        }

        //
        // time to complete non looped buffer
        //

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
        ExInterlockedInsertHeadList(&m_FreeIrpList, &Irp->Tail.Overlay.ListEntry, &m_IrpListLock);
    }

    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
CIrpQueue::HasLastMappingFailed()
{
    return m_OutOfMapping;
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

