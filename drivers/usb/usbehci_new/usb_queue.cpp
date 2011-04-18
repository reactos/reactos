/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/usb_queue.cpp
 * PURPOSE:     USB EHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbehci.h"
#include "hardware.h"

class CUSBQueue : public IUSBQueue
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

    NTSTATUS Initialize(IN PUSBHARDWAREDEVICE Hardware, PDMA_ADAPTER AdapterObject, IN OPTIONAL PKSPIN_LOCK Lock);
    ULONG GetPendingRequestCount();
    NTSTATUS AddUSBRequest(PURB Urb);
    NTSTATUS AddUSBRequest(IUSBRequest * Request);
    NTSTATUS CancelRequests();
    NTSTATUS CreateUSBRequest(IUSBRequest **OutRequest);

    // constructor / destructor
    CUSBQueue(IUnknown *OuterUnknown){}
    virtual ~CUSBQueue(){}

protected:
    LONG m_Ref;
    KSPIN_LOCK m_Lock;
    PDMA_ADAPTER m_Adapter;
    PVOID VirtualBase;
    PHYSICAL_ADDRESS PhysicalAddress;
    PLIST_ENTRY ExecutingList;
    PLIST_ENTRY PendingList;
    IDMAMemoryManager *m_MemoryManager;

    PQUEUE_HEAD CreateQueueHead();
    PQUEUE_TRANSFER_DESCRIPTOR CreateDescriptor(UCHAR PIDCode, ULONG TotalBytesToTransfer);
};

//=================================================================================================
// COM
//
NTSTATUS
STDMETHODCALLTYPE
CUSBQueue::QueryInterface(
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
CUSBQueue::Initialize(
    IN PUSBHARDWAREDEVICE Hardware,
    PDMA_ADAPTER AdapterObject,
    IN OPTIONAL PKSPIN_LOCK Lock)
{
    NTSTATUS Status;
    PQUEUE_HEAD HeadQueueHead;

    DPRINT1("CUSBQueue::Initialize()\n");

    ASSERT(Hardware);
    ASSERT(AdapterObject);

    //
    // Create Common Buffer
    //
    VirtualBase = AdapterObject->DmaOperations->AllocateCommonBuffer(AdapterObject,
                                                                     PAGE_SIZE * 4,
                                                                     &PhysicalAddress,
                                                                     FALSE);
    if (!VirtualBase)
    {
        DPRINT1("Failed to allocate a common buffer\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Create DMAMemoryManager for use with QueueHeads and Transfer Descriptors.
    //
    Status =  CreateDMAMemoryManager(&m_MemoryManager);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create DMAMemoryManager Object\n");
        return Status;
    }

    //
    // initialize device lock
    //
    KeInitializeSpinLock(&m_Lock);

    //
    // Initialize the DMAMemoryManager
    //
    Status = m_MemoryManager->Initialize(Hardware, &m_Lock, PAGE_SIZE * 4, VirtualBase, PhysicalAddress, 32);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to initialize the DMAMemoryManager\n");
        return Status;
    }

    //
    // Create a dead QueueHead for use in Async Register
    //
    HeadQueueHead = CreateQueueHead();
    HeadQueueHead->HorizontalLinkPointer = HeadQueueHead->PhysicalAddr | QH_TYPE_QH;
    HeadQueueHead->EndPointCharacteristics.QEDTDataToggleControl = FALSE;
    HeadQueueHead->Token.Bits.InterruptOnComplete = FALSE;
    HeadQueueHead->EndPointCharacteristics.HeadOfReclamation = TRUE;
    HeadQueueHead->Token.Bits.Halted = TRUE;
    
    Hardware->SetAsyncListRegister(HeadQueueHead->PhysicalAddr);

    //
    // Set ExecutingList and create PendingList
    //
    ExecutingList = &HeadQueueHead->LinkedQueueHeads;
    PendingList = (PLIST_ENTRY) ExAllocatePool(NonPagedPool, sizeof(LIST_ENTRY));
    if (!PendingList)
    {
        DPRINT1("Pool allocation failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize ListHeads
    //
    InitializeListHead(ExecutingList);
    InitializeListHead(PendingList);

    return STATUS_SUCCESS;
}

ULONG
CUSBQueue::GetPendingRequestCount()
{
    UNIMPLEMENTED
    return 0;
}

NTSTATUS
CUSBQueue::AddUSBRequest(
    IUSBRequest * Request)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
CUSBQueue::AddUSBRequest(
    PURB Urb)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
CUSBQueue::CancelRequests()
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
CUSBQueue::CreateUSBRequest(
    IUSBRequest **OutRequest)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

PQUEUE_HEAD
CUSBQueue::CreateQueueHead()
{
    PQUEUE_HEAD QueueHead;
    PHYSICAL_ADDRESS PhysicalAddress;
    NTSTATUS Status;
    //
    // Create the QueueHead from Common Buffer
    //
    Status = m_MemoryManager->Allocate(sizeof(QUEUE_HEAD),
                                       (PVOID*)&QueueHead,
                                       &PhysicalAddress);
    if (!NT_SUCCESS(Status))
        return NULL;

    //
    // Initialize default values
    //

    QueueHead->PhysicalAddr = PhysicalAddress.LowPart;
    QueueHead->HorizontalLinkPointer = TERMINATE_POINTER;
    QueueHead->AlternateNextPointer = TERMINATE_POINTER;
    QueueHead->NextPointer = TERMINATE_POINTER;

    // 1 for non high speed, 0 for high speed device
    QueueHead->EndPointCharacteristics.ControlEndPointFlag = 0;
    QueueHead->EndPointCharacteristics.HeadOfReclamation = FALSE;
    QueueHead->EndPointCharacteristics.MaximumPacketLength = 64;

    // Set NakCountReload to max value possible
    QueueHead->EndPointCharacteristics.NakCountReload = 0xF;

    // Get the Initial Data Toggle from the Queue Element Desriptor
    QueueHead->EndPointCharacteristics.QEDTDataToggleControl = FALSE;

    QueueHead->EndPointCharacteristics.EndPointSpeed = QH_ENDPOINT_HIGHSPEED;

    QueueHead->EndPointCapabilities.NumberOfTransactionPerFrame = 0x03;

    // Interrupt when QueueHead is processed
    QueueHead->Token.Bits.InterruptOnComplete = FALSE;

    return QueueHead;
}

PQUEUE_TRANSFER_DESCRIPTOR
CUSBQueue::CreateDescriptor(
    UCHAR PIDCode,
    ULONG TotalBytesToTransfer)
{
    PQUEUE_TRANSFER_DESCRIPTOR Descriptor;
    PHYSICAL_ADDRESS PhysicalAddress;
    NTSTATUS Status;

    //
    // Create the Descriptor from Common Buffer
    //
    Status = m_MemoryManager->Allocate(sizeof(QUEUE_TRANSFER_DESCRIPTOR),
                                       (PVOID*)&Descriptor,
                                       &PhysicalAddress);
    if (!NT_SUCCESS(Status))
        return NULL;

    Descriptor->NextPointer = TERMINATE_POINTER;
    Descriptor->AlternateNextPointer = TERMINATE_POINTER;
    Descriptor->Token.Bits.DataToggle = TRUE;
    Descriptor->Token.Bits.ErrorCounter = 0x03;
    Descriptor->Token.Bits.Active = TRUE;
    Descriptor->Token.Bits.PIDCode = PIDCode;
    Descriptor->Token.Bits.TotalBytesToTransfer = TotalBytesToTransfer;
    Descriptor->PhysicalAddr = PhysicalAddress.LowPart;
    return Descriptor;
}

NTSTATUS
CreateUSBQueue(
    PUSBQUEUE *OutUsbQueue)
{
    PUSBQUEUE This;

    //
    // allocate controller
    //
    This = new(NonPagedPool, TAG_USBEHCI) CUSBQueue(0);
    if (!This)
    {
        //
        // failed to allocate
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // add reference count
    //
    This->AddRef();

    //
    // return result
    //
    *OutUsbQueue = (PUSBQUEUE)This;

    //
    // done
    //
    return STATUS_SUCCESS;
}
