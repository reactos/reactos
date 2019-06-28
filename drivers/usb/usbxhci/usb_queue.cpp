/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Extensible Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbxhci/usb_queue.cpp
 * PURPOSE:     USB XHCI device driver(based on Haiku XHCI driver and ReactOS EHCI)
 * PROGRAMMERS: lesanilie@gmail
 */
#include "usbxhci.h"

#define YDEBUG
#include <debug.h>

class CUSBQueue : public IXHCIQueue
{
public:
    STDMETHODIMP QueryInterface(REFIID InterfaceId, PVOID *Interface);
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

    // com interfaces
    IMP_IUSBQUEUE
    IMP_IXHCIQUEUE

    // initialize event and command ringbuffers
    NTSTATUS InitializeRingbuffers(IN PXHCIHARDWAREDEVICE Hardware, IN PDMAMEMORYMANAGER MemoryManager);

    CUSBQueue(IUnknown *OuterUnknown) {}
    virtual ~CUSBQueue() {}
protected:
    LONG m_Ref;
    PKSPIN_LOCK m_Lock;
    PXHCIHARDWAREDEVICE m_Hardware;
    PDMAMEMORYMANAGER m_MemoryManager;
    PHYSICAL_ADDRESS m_EventRingPhysicalAddress;
    PVOID m_EventRingVirtualAddress;
    ULONG m_EventRingCycleState;
    PTRB m_EventRingDequeueAddress;
    PHYSICAL_ADDRESS m_CommandRingPhysicalAddress;
    PVOID m_CommandRingVirtualAddress;
    PTRB m_CommandRingEnqueueAddress;
    ULONG m_CommandRingCycleState;
    LIST_ENTRY m_CommandRingList;

    NTSTATUS LinkCommandDescriptor(PCOMMAND_DESCRIPTOR CommandDescriptor);
};

//
// UNKNOWN
//
NTSTATUS
STDMETHODCALLTYPE
CUSBQueue::QueryInterface(
    IN REFIID refiid,
    OUT PVOID *Output)
{
    if (IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PUNKNOWN(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

//
// IMP_IUSBQUEUE
//
NTSTATUS
STDMETHODCALLTYPE
CUSBQueue::Initialize(
    IN PUSBHARDWAREDEVICE Hardware,
    IN PDMA_ADAPTER AdapterObject,
    IN PDMAMEMORYMANAGER MemManager,
    IN OPTIONAL PKSPIN_LOCK Lock)
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // store the device lock
    //
    m_Lock = Lock;

    //
    // save for later use
    //
    m_Hardware = PXHCIHARDWAREDEVICE(Hardware);
    m_MemoryManager = PDMAMEMORYMANAGER(MemManager);

    //
    // initialize command list
    //
    InitializeListHead(&m_CommandRingList);

    //
    // create event and command ringbuffers
    //
    Status = InitializeRingbuffers(m_Hardware, MemManager);

    //
    // initialization done
    //
    return Status;
}

NTSTATUS
CUSBQueue::InitializeRingbuffers(
    IN PXHCIHARDWAREDEVICE Hardware,
    IN PDMAMEMORYMANAGER MemManager)
{
    NTSTATUS Status;
    PERST_ELEMENT VirtualAddressErstElement = NULL;
    PHYSICAL_ADDRESS PhysicalAddressErstElement;
    PTRB LastTransferRequestBlock;

    //
    // allocate memory for event ringbuffer
    //
    Status = m_MemoryManager->Allocate(XHCI_MAX_EVENTS * sizeof(TRB), &m_EventRingVirtualAddress, &m_EventRingPhysicalAddress);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to allocate memory for event ringbuffer. \n");
        return Status;
    }

    //
    // allocate memory for event ring segment table
    //
    Status = m_MemoryManager->Allocate(sizeof(ERST_ELEMENT), (PVOID*)&VirtualAddressErstElement, &PhysicalAddressErstElement);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to allocate memory for event ring segment table. \n");
        m_MemoryManager->Release(m_EventRingVirtualAddress, (XHCI_MAX_EVENTS * sizeof(TRB)));
        return Status;
    }

    //
    // initialize event ring segment table(only one segment for event ringbuffer)
    //
    VirtualAddressErstElement->Address.QuadPart = m_EventRingPhysicalAddress.QuadPart;
    VirtualAddressErstElement->Size = XHCI_MAX_EVENTS;
    VirtualAddressErstElement->Reserved = 0;

    //
    // get event ring dequeue pointer
    //
    m_EventRingDequeueAddress = (PTRB)m_EventRingVirtualAddress;

    //
    // ring cycle state at init is always one
    //
    m_EventRingCycleState = 1;

    //
    // one segment for event ringbuffer
    //
    Hardware->SetRuntimeRegister(XHCI_ERSTSZ_BASE, 1);

    //
    // set Event Ring Segment Table Base Address Register
    //
    Hardware->SetRuntimeRegister(XHCI_ERSTBA_LOW, PhysicalAddressErstElement.LowPart);
    Hardware->SetRuntimeRegister(XHCI_ERSTBA_HIGH, PhysicalAddressErstElement.HighPart);

    //
    // set Event Ring Dequeue Pointer Register
    //
    Hardware->SetRuntimeRegister(XHCI_ERDP_BASE_LOW, m_EventRingPhysicalAddress.LowPart);
    Hardware->SetRuntimeRegister(XHCI_ERDP_BASE_HIGH, m_EventRingPhysicalAddress.HighPart);

    //
    // allocate memory for command ringbuffer
    //
    Status = m_MemoryManager->Allocate(XHCI_MAX_COMMANDS * sizeof(TRB), &m_CommandRingVirtualAddress, &m_CommandRingPhysicalAddress);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to allocate memory for command ringbuffer. \n");

        //
        // oops
        //
        m_MemoryManager->Release(m_EventRingVirtualAddress, (XHCI_MAX_EVENTS * sizeof(TRB)));
        m_MemoryManager->Release(m_EventRingVirtualAddress, sizeof(ERST_ELEMENT));
        return Status;
    }

    //
    // set enqueue pointer and cycle state
    //
    m_CommandRingEnqueueAddress = (PTRB)m_CommandRingVirtualAddress;
    m_CommandRingCycleState = 1;

    //
    // last trb is a link trb (in order to construct a ringbuffer)
    //
    LastTransferRequestBlock = (PTRB)((ULONG_PTR)m_CommandRingVirtualAddress + (XHCI_MAX_COMMANDS * sizeof(TRB)));
    LastTransferRequestBlock->Field[0] = m_CommandRingPhysicalAddress.LowPart;
    LastTransferRequestBlock->Field[1] = m_CommandRingPhysicalAddress.HighPart;

    //
    // set Command Ring Control Register. also set up Ring Cycle State
    //
    Hardware->SetOperationalRegister(XHCI_CRCR_LOW, m_CommandRingPhysicalAddress.LowPart | XHCI_CRCR_RCS);
    Hardware->SetOperationalRegister(XHCI_CRCR_HIGH, m_CommandRingPhysicalAddress.HighPart);

    //
    // great success
    //
    return Status;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBQueue::AddUSBRequest(
    IN IUSBRequest *Req)
{
    NTSTATUS            Status;
    PXHCIREQUEST        Request;
    KIRQL               OldIrql;
    ULONG               Type, Target;
    PCOMMAND_DESCRIPTOR CommandDescriptorHead = NULL;
    PDEVICE_INFORMATION DeviceInformation;

    DPRINT("AddUSBRequest called. \n");

    //
    // get a pointer to all interfaces
    //
    Request = PXHCIREQUEST(Req);

    //
    // get request type
    //
    Type = Request->GetTransferType();

    //
    // check if supported
    //
    switch (Type)
    {
        case USB_ENDPOINT_TYPE_ISOCHRONOUS:
            Status = STATUS_NOT_SUPPORTED;
            break;
        case USB_ENDPOINT_TYPE_INTERRUPT:
        case USB_ENDPOINT_TYPE_BULK:
            Status = STATUS_NOT_SUPPORTED;
            break;
        case USB_ENDPOINT_TYPE_CONTROL:
            Target = Request->GetRequestTarget();
            Status = STATUS_SUCCESS;
            break;
        default:
            /* BUG */
            PC_ASSERT(FALSE);
            Status = STATUS_NOT_SUPPORTED;
    }
    if (!NT_SUCCESS(Status))
    {
        //
        // invalid type
        //
        return Status;
    }

    //
    // obtain device information from device address
    //
    Status = m_Hardware->GetDeviceInformationByAddress(Request->GetRequestDeviceAddress(), &DeviceInformation);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed
        //
        return Status;
    }

    //
    // set device information
    //
    Request->SetRequestDeviceInformation(DeviceInformation);

    if (Type == USB_ENDPOINT_TYPE_CONTROL && Target == USB_TARGET_XHCI)
    {
        //
        // get command descriptor
        //
        Status = Request->GetCommandDescriptor(&CommandDescriptorHead);
    }
    else // USB_TARGET_DEVICE
    {
        //
        // TODO: normal transfer or control transfer
        //
        // Status = Request->GetTransferDescriptor(&TransferDescriptorHead);
        ASSERT(FALSE);
    }

    if (!NT_SUCCESS(Status))
    {
        //
        // failed to get descriptor
        //
        return Status;
    }

    //
    // add a extra referece which is released when the request is completed
    //
    Request->AddRef();

    //
    // acquire the lock
    //
    KeAcquireSpinLock(m_Lock, &OldIrql);

    //
    // Control transfer?
    //
    if (Type == USB_ENDPOINT_TYPE_CONTROL)
    {
        if (Target == USB_TARGET_XHCI)
        {
            LinkCommandDescriptor(CommandDescriptorHead);
        }
        else // USB_TARGET_ENDPOINT
        {
            // Status = LinkControlTransferDescriptor(TransferDescriptorHead);
        }
    }
    else // Data transfer
    {
        // Status = LinkDataTransferDescriptor(TransferDescriptorHead);
    }

    //
    // release the lock
    //
    KeReleaseSpinLock(m_Lock, OldIrql);

    //
    // great success
    //
    return Status;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBQueue::CreateUSBRequest(
    IN IUSBRequest **OutRequest)
{
    PUSBREQUEST UsbRequest;
    NTSTATUS Status;

    *OutRequest = NULL;
    Status = InternalCreateUSBRequest(&UsbRequest);

    if (NT_SUCCESS(Status))
    {
        *OutRequest = UsbRequest;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBQueue::SubmitInternalCommand(
    IN PDMAMEMORYMANAGER MemoryManager,
    IN PCOMMAND_INFORMATION CommandInformation,
    IN PTRB OutTransferRequestBlock)
{
    NTSTATUS Status;
    PUSBREQUEST UsbRequest;
    PXHCIREQUEST Request;

    //
    // create request
    //
    Status = CreateUSBRequest(&UsbRequest);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to build request
        //
        DPRINT1("CreateUSBRequest failed with status %x\n", Status);
        return Status;
    }

    //
    // get internal request
    //
    Request = PXHCIREQUEST(UsbRequest);

    //
    // initialize request with command
    //
    Status = Request->InitializeWithCommand(MemoryManager, CommandInformation, OutTransferRequestBlock);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to initialize request
        //
        DPRINT1("InitializeWithCommand failed with status %x\n", Status);
        Request->Release();
        return Status;
    }

    //
    // add request to the queue
    //
    Status = AddUSBRequest(UsbRequest);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to add request
        //
        DPRINT1("Failed add request to queue with status %x\n", Status);
        Request->Release();
        return Status;
    }

    //
    // wait for request to complete
    //
    Request->GetResultStatus(&Status, NULL);

    //
    // free request
    //
    Request->Release();

    //
    // great success
    //
    return Status;
}

NTSTATUS
CUSBQueue::LinkCommandDescriptor(IN PCOMMAND_DESCRIPTOR CommandDescriptor)
{
    ULONG LinkTransferRequestBlock;

    //
    // put request in the queue
    //
    RtlCopyMemory(m_CommandRingEnqueueAddress, CommandDescriptor->VirtualTrbAddress, sizeof(TRB));

    if (m_CommandRingCycleState)
    {
        m_CommandRingEnqueueAddress->Field[3] |= XHCI_TRB_CYCLE_BIT;
    }
    else
    {
        m_CommandRingEnqueueAddress->Field[3] &= ~XHCI_TRB_CYCLE_BIT;
    }

    //
    // clear toggle cycle bit
    //
    m_CommandRingEnqueueAddress->Field[3] &= ~XHCI_TRB_TC_BIT;

    //
    // go to next trb
    //
    m_CommandRingEnqueueAddress++;

    //
    // check if this is penultimate trb from ring
    //
    if (m_CommandRingEnqueueAddress == ((PTRB)((ULONG_PTR)m_CommandRingVirtualAddress + (sizeof(TRB) * (XHCI_MAX_COMMANDS - 1)))))
    {
        //
        // this is a link trb, set toggle cycle because we start from beginning
        //
        LinkTransferRequestBlock = XHCI_TRB_TYPE(XHCI_TRB_TYPE_LINK) | XHCI_TRB_TC_BIT;

        //
        // set the same value for cycle bit in link trb as previous
        //
        if (m_CommandRingCycleState)
        {
            LinkTransferRequestBlock |= XHCI_TRB_CYCLE_BIT;
        }

        //
        // set link trb info
        //
        m_CommandRingEnqueueAddress->Field[3] = LinkTransferRequestBlock;

        //
        // toggle cycle bit
        //
        m_CommandRingCycleState ^= 1;

        //
        // next time we start from beginning
        //
        m_CommandRingEnqueueAddress = (PTRB)m_CommandRingVirtualAddress;
    }

    //
    // insert it in the list
    //
    InsertTailList(&m_CommandRingList, &CommandDescriptor->DescriptorListEntry);

    //
    // ring the doorbell for XHCI
    //
    m_Hardware->RingDoorbellRegister(0, 0, 0);

    //
    // success
    //
    return STATUS_SUCCESS;
}
NTSTATUS
STDMETHODCALLTYPE
CUSBQueue::AbortDevicePipe(
    IN UCHAR DeviceAddress,
    IN PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor)
{
    UNREFERENCED_PARAMETER(DeviceAddress);
    UNREFERENCED_PARAMETER(EndpointDescriptor);

    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_IMPLEMENTED;
}

//
// IMP_IXHCIQUEUE
//

VOID
STDMETHODCALLTYPE
CUSBQueue::CompleteCommandRequest(IN PTRB TransferRequestBlock)
{
    PLIST_ENTRY Entry;
    USBD_STATUS UrbStatus = USBD_STATUS_DATA_BUFFER_ERROR;
    PCOMMAND_DESCRIPTOR CommandDescriptor;
    PXHCIREQUEST Request;
    KIRQL OldIrql;

    DPRINT("CompleteCommandRequest called!\n");

    //
    // acquire the lock
    //
    KeAcquireSpinLock(m_Lock, &OldIrql);

    if (!IsListEmpty(&m_CommandRingList))
    {
        //
        // get first command
        //
        Entry = RemoveHeadList(&m_CommandRingList);

        //
        // get pointer to command descriptor
        //
        CommandDescriptor = CONTAINING_RECORD(Entry, COMMAND_DESCRIPTOR, DescriptorListEntry);

        //
        // internal request?
        //
        if (CommandDescriptor->CompletedTrb)
        {
            //
            // copy the result
            //
            RtlCopyMemory(CommandDescriptor->CompletedTrb, TransferRequestBlock, sizeof(TRB));
        }

        //
        // Check if request was successful
        //
        if (XHCI_TRB_COMP_CODE(TransferRequestBlock->Field[2]) == XHCI_TRB_COMPLETION_SUCCESS)
        {
            UrbStatus = USBD_STATUS_SUCCESS;
        }
        else
        {
            UrbStatus = USBD_STATUS_INVALID_PARAMETER;
        }

        //
        // get request
        //
        Request = (PXHCIREQUEST)CommandDescriptor->Request;

        //
        // notify request that transfer has completed
        //
        Request->CompletionCallback(UrbStatus != USBD_STATUS_SUCCESS ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS, UrbStatus);

        //
        // free command descriptor
        //
        Request->FreeCommandDescriptor(CommandDescriptor);

        //
        // free request
        //
        Request->Release();
    }

    //
    // release the lock
    //
    KeReleaseSpinLock(m_Lock, OldIrql);

    //
    // great success
    //
    return;
}

VOID
STDMETHODCALLTYPE
CUSBQueue::CompleteTransferRequest(PTRB TransferRequestBlock)
{
    DPRINT("CompleteCommandRequest called!\n");
    UNIMPLEMENTED_DBGBREAK();
    return;
}

BOOLEAN
STDMETHODCALLTYPE
CUSBQueue::IsEventRingEmpty(VOID)
{
    DPRINT("IsEventRingEmpty called.\n");

    //
    // check if ring is empty(cycle bit) TODO: use macro to get cycle bit?
    //
    return (m_EventRingCycleState == (m_EventRingDequeueAddress->Field[3] & XHCI_TRB_CYCLE_BIT)) ? FALSE : TRUE;
}

PTRB
STDMETHODCALLTYPE
CUSBQueue::GetEventRingDequeuePointer(VOID)
{
    PHYSICAL_ADDRESS PhysicalAddress;
    PTRB TransferRequestBlock;

    //
    // check if ring is empty(cycle bit)
    //
    if (IsEventRingEmpty())
        return NULL;

    DPRINT("TRB completion code is %lx.\n", XHCI_TRB_COMP_CODE(m_EventRingDequeueAddress->Field[2]));

    //
    // go to next transfer request block
    //
    TransferRequestBlock = m_EventRingDequeueAddress++;

    //
    // end of ringbuffer?
    //
    if (m_EventRingDequeueAddress == (PTRB)((ULONG_PTR)m_EventRingVirtualAddress + (sizeof(TRB) * XHCI_MAX_EVENTS)))
    {
        //
        // every time we start from the beginning, we have to toggle cycle bit
        //
        m_EventRingCycleState ^= 1;
        m_EventRingDequeueAddress = (PTRB)m_EventRingVirtualAddress;
    }

    //
    // set dequeue pointer. Refer to section 5.5.2.3.3
    //
    PhysicalAddress = MmGetPhysicalAddress(m_EventRingDequeueAddress);

    //
    // setup dequeue pointer(mark event as processed)
    //
    m_Hardware->SetRuntimeRegister(XHCI_ERDP_BASE_LOW, PhysicalAddress.LowPart | XHCI_ERST_EHB);
    m_Hardware->SetRuntimeRegister(XHCI_ERDP_BASE_HIGH, PhysicalAddress.HighPart);

    //
    // found trb
    //
    return TransferRequestBlock;
}

NTSTATUS
NTAPI
CreateUSBQueue(
    PUSBQUEUE *OutUsbQueue)
{
    PUSBQUEUE This;

    This = new(NonPagedPool, TAG_USBXHCI) CUSBQueue(0);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->AddRef();
    *OutUsbQueue = (PUSBQUEUE)This;

    return STATUS_SUCCESS;
}
