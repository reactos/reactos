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

class CIrpQueue2 : public IIrpQueue
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
    CIrpQueue2(IUnknown *OuterUnknown){}
    virtual ~CIrpQueue2(){}

protected:

    PKSPIN_CONNECT m_ConnectDetails;
    PKSPIN_DESCRIPTOR m_Descriptor;

    KSPIN_LOCK m_IrpListLock;
    KSPIN_LOCK m_QueueLock;
    LIST_ENTRY m_IrpList;
    LIST_ENTRY m_FreeIrpList;

    BOOLEAN m_OutOfMapping;
    ULONG m_MaxFrameSize;
    ULONG m_Alignment;
    
    ULONG m_StreamHeaderIndex;

    volatile PIRP m_Irp;
    volatile LONG m_Ref;
};

typedef struct
{
    PVOID Tag;
    UCHAR Used;
    PVOID Data;
    LONG Length;
    
    
}KSSTREAM_TAG, *PKSSTREAM_TAG;

typedef struct
{
    ULONG nTags;

    KSSTREAM_TAG Tags[];
}KSSTREAM_DATA, *PKSSTREAM_DATA;

#define STREAM_DATA_OFFSET   (0)


NTSTATUS
NTAPI
CIrpQueue2::QueryInterface(
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
CIrpQueue2::Init(
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

    InitializeListHead(&m_IrpList);
    InitializeListHead(&m_FreeIrpList);
    KeInitializeSpinLock(&m_IrpListLock);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CIrpQueue2::AddMapping(
    IN PIRP Irp,
    OUT PULONG Data)
{
    PKSSTREAM_HEADER Header;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION IoStack;
    ULONG Length;
    PMDL Mdl;
    PKSSTREAM_DATA StreamData;
    LONG TotalStreamData;
    LONG StreamPageCount;
    LONG HeaderLength;

    PC_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

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

        if (m_Descriptor->DataFlow == KSPIN_DATAFLOW_IN)
        {
            // irp sink
            TotalStreamData += Header->DataUsed;
            HeaderLength = Header->DataUsed;
        }
        else
        {
            // irp source
            TotalStreamData += Header->FrameExtent;
            HeaderLength = Header->DataUsed;
        }
        
        // append page count
        StreamPageCount += ADDRESS_AND_SIZE_TO_SPAN_PAGES(
            MmGetMdlByteOffset(Mdl), HeaderLength);

        // move to next header / mdl
        Mdl = Mdl->Next;
        Header = (PKSSTREAM_HEADER)((ULONG_PTR)Header + Header->Size);

    }while(Length);

    // allocate stream data
    StreamData = (PKSSTREAM_DATA)AllocateItem(NonPagedPool, sizeof(KSSTREAM_DATA) + 
        sizeof(KSSTREAM_TAG) * StreamPageCount, TAG_PORTCLASS);
    if (!StreamData)
    {
        // done
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // now get a system address for the user buffers
    Header = (PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer;
    Length = IoStack->Parameters.DeviceIoControl.OutputBufferLength;
    Mdl = Irp->MdlAddress;
    
    do
    {
        /* subtract size */
        Length -= Header->Size;    

        /* get system address */
        char* Data = (char*)MmGetSystemAddressForMdlSafe(Mdl, NormalPagePriority);

        /* check for success */
        if (!Data)
        {
            FreeItem(StreamData, TAG_PORTCLASS);
            // done
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        if (m_Descriptor->DataFlow == KSPIN_DATAFLOW_IN)
        {
            HeaderLength = Header->DataUsed;
        }
        else if (m_Descriptor->DataFlow == KSPIN_DATAFLOW_OUT)
        {
            HeaderLength = Header->DataUsed;
        }
        
        // append pages
        while(HeaderLength) 
        {
            char* NextData = (char*)ROUND_TO_PAGES(Data+1);
            LONG PageSize = NextData - Data;
            
            if(PageSize > HeaderLength) 
                PageSize = HeaderLength;
            HeaderLength -= PageSize;
            
            StreamData->Tags[StreamData->nTags].Data = Data;
            StreamData->Tags[StreamData->nTags].Length = PageSize;
            StreamData->nTags++;
            
            Data = NextData;
        }

        // move to next header / mdl
        Mdl = Mdl->Next;
        Header = (PKSSTREAM_HEADER)((ULONG_PTR)Header + Header->Size);
        
    }while(Length);

    // store stream data
    Irp->Tail.Overlay.DriverContext[STREAM_DATA_OFFSET] = (PVOID)StreamData;

    *Data = TotalStreamData;

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
CIrpQueue2::GetMapping(
    OUT PUCHAR * Buffer,
    OUT PULONG BufferSize)
{
    ASSERT(0);
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
CIrpQueue2::UpdateMapping(
    IN ULONG BytesWritten)
{
    ASSERT(0);
}

ULONG
NTAPI
CIrpQueue2::NumData()
{
    // returns the amount of audio stream data available
    ASSERT(0);
    return 0;
}

BOOL
NTAPI
CIrpQueue2::CancelBuffers()
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
CIrpQueue2::GetMappingWithTag(
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
        
        if(m_Irp) {
            m_StreamHeaderIndex = 0;
        }
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
    PC_ASSERT(m_StreamHeaderIndex < StreamData->nTags);

    // setup mapping

    *VirtualAddress = StreamData->Tags[m_StreamHeaderIndex].Data;

    // store tag in irp
    StreamData->Tags[m_StreamHeaderIndex].Tag = Tag;
    StreamData->Tags[m_StreamHeaderIndex].Used = TRUE;

    // mapping size
    *ByteCount = StreamData->Tags[m_StreamHeaderIndex].Length;
    
    // increment header index
    m_StreamHeaderIndex++;

    if (m_StreamHeaderIndex == StreamData->nTags)
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
    }
    
    
    
    
    // get physical address
    *PhysicalAddress = MmGetPhysicalAddress(*VirtualAddress);

    DPRINT("GetMappingWithTag Tag %p Buffer %p Flags %lu ByteCount %lx\n", Tag, VirtualAddress, *Flags, *ByteCount);
    // done
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CIrpQueue2::ReleaseMappingWithTag(
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

        if (m_StreamHeaderIndex)
        {
            // check if the released mapping is one current processed irps
            for(Index = 0; Index < m_StreamHeaderIndex; Index++)
            {
                // check if it is the same tag
                if ((StreamData->Tags[Index].Tag == Tag) && 
                    (StreamData->Tags[Index].Used != FALSE))
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

    // check if the released mapping is one of these
    for(Index = 0; Index < StreamData->nTags; Index++)
    {
        if ((StreamData->Tags[Index].Tag == Tag) &&
            (StreamData->Tags[Index].Used != FALSE))
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
    if (Index + 1 == StreamData->nTags)
    {
        // last mapping released
        // now check if this is a looped buffer
        if (m_ConnectDetails->Interface.Id == KSINTERFACE_STANDARD_LOOPED_STREAMING)
        {
            // looped buffers are not completed when they have been played
            // they are completed when the stream is set to stop

            // re-insert irp
            KsAddIrpToCancelableQueue(&m_IrpList, &m_IrpListLock, Irp, KsListEntryTail, NULL);

            // done
            return STATUS_SUCCESS;
        }

        //
        // time to complete non looped buffer
        //

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
CIrpQueue2::HasLastMappingFailed()
{
    return m_OutOfMapping;
}

ULONG
NTAPI
CIrpQueue2::GetCurrentIrpOffset()
{
    ASSERT(0);
    return 0;
}

BOOLEAN
NTAPI
CIrpQueue2::GetAcquiredTagRange(
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
NewIrpQueue2(
    IN IIrpQueue **Queue)
{
    CIrpQueue2 *This = new(NonPagedPool, TAG_PORTCLASS)CIrpQueue2(NULL);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;
        
    DbgPrint("--------------- NewIrpQueue2: " __TIME__ " --------------\n");

    This->AddRef();

    *Queue = (IIrpQueue*)This;
    return STATUS_SUCCESS;
}

