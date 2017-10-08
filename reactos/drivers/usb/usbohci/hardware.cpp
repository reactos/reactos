/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbohci/hcd_controller.cpp
 * PURPOSE:     USB OHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbohci.h"

#define NDEBUG
#include <debug.h>

typedef VOID __stdcall HD_INIT_CALLBACK(IN PVOID CallBackContext);

BOOLEAN
NTAPI
InterruptServiceRoutine(
    IN PKINTERRUPT  Interrupt,
    IN PVOID  ServiceContext);

VOID
NTAPI
OhciDeferredRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2);

VOID
NTAPI
StatusChangeWorkItemRoutine(PVOID Context);

class CUSBHardwareDevice : public IOHCIHardwareDevice
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
    // com
    IMP_IUSBHARDWAREDEVICE
    IMP_IUSBOHCIHARDWAREDEVICE


    // local
    NTSTATUS StartController();
    NTSTATUS StopController();
    BOOLEAN InterruptService();
    NTSTATUS InitializeController();
    NTSTATUS AllocateEndpointDescriptor(OUT POHCI_ENDPOINT_DESCRIPTOR *OutDescriptor);

    // friend function
    friend BOOLEAN NTAPI InterruptServiceRoutine(IN PKINTERRUPT  Interrupt, IN PVOID  ServiceContext);
    friend VOID NTAPI OhciDeferredRoutine(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2);
    friend VOID NTAPI StatusChangeWorkItemRoutine(PVOID Context);
    // constructor / destructor
    CUSBHardwareDevice(IUnknown *OuterUnknown){}
    virtual ~CUSBHardwareDevice(){}

protected:
    LONG m_Ref;                                                                        // reference count
    PDRIVER_OBJECT m_DriverObject;                                                     // driver object
    PDEVICE_OBJECT m_PhysicalDeviceObject;                                             // pdo
    PDEVICE_OBJECT m_FunctionalDeviceObject;                                           // fdo (hcd controller)
    PDEVICE_OBJECT m_NextDeviceObject;                                                 // lower device object
    KSPIN_LOCK m_Lock;                                                                 // hardware lock
    PKINTERRUPT m_Interrupt;                                                           // interrupt object
    KDPC m_IntDpcObject;                                                               // dpc object for deferred isr processing
    PVOID VirtualBase;                                                                 // virtual base for memory manager
    PHYSICAL_ADDRESS PhysicalAddress;                                                  // physical base for memory manager
    PULONG m_Base;                                                                     // OHCI operational port base registers
    PDMA_ADAPTER m_Adapter;                                                            // dma adapter object
    ULONG m_MapRegisters;                                                              // map registers count
    USHORT m_VendorID;                                                                 // vendor id
    USHORT m_DeviceID;                                                                 // device id
    POHCIQUEUE m_UsbQueue;                                                              // usb request queue
    POHCIHCCA m_HCCA;                                                                  // hcca virtual base
    PHYSICAL_ADDRESS m_HCCAPhysicalAddress;                                            // hcca physical address
    POHCI_ENDPOINT_DESCRIPTOR m_ControlEndpointDescriptor;                             // dummy control endpoint descriptor
    POHCI_ENDPOINT_DESCRIPTOR m_BulkEndpointDescriptor;                                // dummy control endpoint descriptor
    POHCI_ENDPOINT_DESCRIPTOR m_IsoEndpointDescriptor;                                 // iso endpoint descriptor
    POHCI_ENDPOINT_DESCRIPTOR m_InterruptEndpoints[OHCI_STATIC_ENDPOINT_COUNT];        // endpoints for interrupt / iso transfers
    ULONG m_NumberOfPorts;                                                             // number of ports
    PDMAMEMORYMANAGER m_MemoryManager;                                                 // memory manager
    HD_INIT_CALLBACK* m_SCECallBack;                                                   // status change callback routine
    PVOID m_SCEContext;                                                                // status change callback routine context
    WORK_QUEUE_ITEM m_StatusChangeWorkItem;                                            // work item for status change callback
    volatile LONG m_StatusChangeWorkItemStatus;                                        // work item active status
    ULONG m_SyncFramePhysAddr;                                                         // periodic frame list physical address
    ULONG m_IntervalValue;                                                             // periodic interval value
};

//=================================================================================================
// COM
//
NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::QueryInterface(
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

LPCSTR
STDMETHODCALLTYPE
CUSBHardwareDevice::GetUSBType()
{
    return "USBOHCI";
}


NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::Initialize(
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT FunctionalDeviceObject,
    PDEVICE_OBJECT PhysicalDeviceObject,
    PDEVICE_OBJECT LowerDeviceObject)
{
    BUS_INTERFACE_STANDARD BusInterface;
    PCI_COMMON_CONFIG PciConfig;
    NTSTATUS Status;
    ULONG BytesRead;
    PUSBQUEUE Queue;

    DPRINT("CUSBHardwareDevice::Initialize\n");

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
    // Create the UsbQueue class that will handle the Asynchronous and Periodic Schedules
    //
    Status = CreateUSBQueue(&Queue);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create UsbQueue!\n");
        return Status;
    }

    // get ohci queue
    m_UsbQueue = POHCIQUEUE(Queue);

    // sanity check
    ASSERT(m_UsbQueue);

    //
    // store device objects
    //
    m_DriverObject = DriverObject;
    m_FunctionalDeviceObject = FunctionalDeviceObject;
    m_PhysicalDeviceObject = PhysicalDeviceObject;
    m_NextDeviceObject = LowerDeviceObject;

    //
    // initialize device lock
    //
    KeInitializeSpinLock(&m_Lock);

    //
    // initialize status change work item
    //
    ExInitializeWorkItem(&m_StatusChangeWorkItem, StatusChangeWorkItemRoutine, PVOID(this));

    m_VendorID = 0;
    m_DeviceID = 0;

    Status = GetBusInterface(PhysicalDeviceObject, &BusInterface);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get BusInterface!\n");
        return Status;
    }

    BytesRead = (*BusInterface.GetBusData)(BusInterface.Context,
                                           PCI_WHICHSPACE_CONFIG,
                                           &PciConfig,
                                           0,
                                           PCI_COMMON_HDR_LENGTH);

    if (BytesRead != PCI_COMMON_HDR_LENGTH)
    {
        DPRINT1("Failed to get pci config information!\n");
        return STATUS_SUCCESS;
    }

    m_VendorID = PciConfig.VendorID;
    m_DeviceID = PciConfig.DeviceID;

    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::PnpStart(
    PCM_RESOURCE_LIST RawResources,
    PCM_RESOURCE_LIST TranslatedResources)
{
    ULONG Index;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptor;
    DEVICE_DESCRIPTION DeviceDescription;
    PVOID ResourceBase;
    NTSTATUS Status;
    ULONG Version;

    DPRINT("CUSBHardwareDevice::PnpStart\n");
    for(Index = 0; Index < TranslatedResources->List[0].PartialResourceList.Count; Index++)
    {
        //
        // get resource descriptor
        //
        ResourceDescriptor = &TranslatedResources->List[0].PartialResourceList.PartialDescriptors[Index];

        switch(ResourceDescriptor->Type)
        {
            case CmResourceTypeInterrupt:
            {
                KeInitializeDpc(&m_IntDpcObject,
                                OhciDeferredRoutine,
                                this);

                Status = IoConnectInterrupt(&m_Interrupt,
                                            InterruptServiceRoutine,
                                            (PVOID)this,
                                            NULL,
                                            ResourceDescriptor->u.Interrupt.Vector,
                                            (KIRQL)ResourceDescriptor->u.Interrupt.Level,
                                            (KIRQL)ResourceDescriptor->u.Interrupt.Level,
                                            (KINTERRUPT_MODE)(ResourceDescriptor->Flags & CM_RESOURCE_INTERRUPT_LATCHED),
                                            (ResourceDescriptor->ShareDisposition != CmResourceShareDeviceExclusive),
                                            ResourceDescriptor->u.Interrupt.Affinity,
                                            FALSE);

                if (!NT_SUCCESS(Status))
                {
                    //
                    // failed to register interrupt
                    //
                    DPRINT1("IoConnect Interrupt failed with %x\n", Status);
                    return Status;
                }
                break;
            }
            case CmResourceTypeMemory:
            {
                //
                // get resource base
                //
                ResourceBase = MmMapIoSpace(ResourceDescriptor->u.Memory.Start, ResourceDescriptor->u.Memory.Length, MmNonCached);
                if (!ResourceBase)
                {
                    //
                    // failed to map registers
                    //
                    DPRINT1("MmMapIoSpace failed\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                //
                // Get controllers capabilities
                //
                Version = READ_REGISTER_ULONG((PULONG)((ULONG_PTR)ResourceBase + OHCI_REVISION_OFFSET));

                DPRINT("Version %x\n", Version & 0xFFFF);

                //
                // Store Resource base
                //
                m_Base = (PULONG)ResourceBase;
                break;
            }
        }
    }


    //
    // zero device description
    //
    RtlZeroMemory(&DeviceDescription, sizeof(DEVICE_DESCRIPTION));

    //
    // initialize device description
    //
    DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
    DeviceDescription.Master = TRUE;
    DeviceDescription.ScatterGather = TRUE;
    DeviceDescription.Dma32BitAddresses = TRUE;
    DeviceDescription.DmaWidth = Width32Bits;
    DeviceDescription.InterfaceType = PCIBus;
    DeviceDescription.MaximumLength = MAXULONG;

    //
    // get dma adapter
    //
    m_Adapter = IoGetDmaAdapter(m_PhysicalDeviceObject, &DeviceDescription, &m_MapRegisters);
    if (!m_Adapter)
    {
        //
        // failed to get dma adapter
        //
        DPRINT1("Failed to acquire dma adapter\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Create Common Buffer
    //
    VirtualBase = m_Adapter->DmaOperations->AllocateCommonBuffer(m_Adapter,
                                                                 PAGE_SIZE * 4,
                                                                 &PhysicalAddress,
                                                                 FALSE);
    if (!VirtualBase)
    {
        DPRINT1("Failed to allocate a common buffer\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the DMAMemoryManager
    //
    Status = m_MemoryManager->Initialize(this, &m_Lock, PAGE_SIZE * 4, VirtualBase, PhysicalAddress, 32);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to initialize the DMAMemoryManager\n");
        return Status;
    }

    //
    // initializes the controller
    //
    Status = InitializeController();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to Initialize the controller \n");
        return Status;
    }

    //
    // Initialize the UsbQueue now that we have an AdapterObject.
    //
    Status = m_UsbQueue->Initialize(this, m_Adapter, m_MemoryManager, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to Initialize the UsbQueue\n");
        return Status;
    }

    //
    // Start the controller
    //
    DPRINT("Starting Controller\n");
    Status = StartController();

    //
    // done
    //
    return Status;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::PnpStop(void)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::GetDeviceDetails(
    OUT OPTIONAL PUSHORT VendorId,
    OUT OPTIONAL PUSHORT DeviceId,
    OUT OPTIONAL PULONG NumberOfPorts,
    OUT OPTIONAL PULONG Speed)
{
    if (VendorId)
    {
        //
        // get vendor
        //
        *VendorId = m_VendorID;
    }

    if (DeviceId)
    {
        //
        // get device id
        //
        *DeviceId = m_DeviceID;
    }

    if (NumberOfPorts)
    {
        //
        // get number of ports
        //
        *NumberOfPorts = m_NumberOfPorts;
    }

    if (Speed)
    {
        //
        // speed is 0x100
        //
        *Speed = 0x100;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::GetDMA(
    OUT struct IDMAMemoryManager **OutDMAMemoryManager)
{
    if (!m_MemoryManager)
        return STATUS_UNSUCCESSFUL;
    *OutDMAMemoryManager = m_MemoryManager;
    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::GetUSBQueue(
    OUT struct IUSBQueue **OutUsbQueue)
{
    if (!m_UsbQueue)
        return STATUS_UNSUCCESSFUL;
    *OutUsbQueue = m_UsbQueue;
    return STATUS_SUCCESS;
}


NTSTATUS
CUSBHardwareDevice::StartController(void)
{
    ULONG Control, Descriptor, FrameInterval, Periodic, Port, Reset, Index;
    ULONG NewControl, WaitInMs;
    LARGE_INTEGER Timeout;
    BOOLEAN Again = FALSE;

    //
    // check context
    //
    Control = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_CONTROL_OFFSET));

    //Save this
    NewControl = Control & OHCI_REMOTE_WAKEUP_CONNECTED;

    if ((Control & OHCI_INTERRUPT_ROUTING))
    {
        //
        // change ownership
        //
        WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_COMMAND_STATUS_OFFSET), OHCI_OWNERSHIP_CHANGE_REQUEST);
        for(Index = 0; Index < 100; Index++)
        {
            //
            // wait a bit
            //
            KeStallExecutionProcessor(100);

            //
            // check control
            //
            Control = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_CONTROL_OFFSET));
            if (!(Control & OHCI_INTERRUPT_ROUTING))
            {
                //
                // acquired ownership
                //
                break;
            }
        }

        //
        // if the ownership is still not changed, perform reset
        //
        if (Control & OHCI_INTERRUPT_ROUTING)
        {
            DPRINT1("SMM not responding\n");
        }
        else
        {
            DPRINT1("SMM has given up ownership\n");
        }
    }

    //
    // read contents of control register
    //

    Control = (READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_CONTROL_OFFSET)) & OHCI_HC_FUNCTIONAL_STATE_MASK);
    DPRINT("Controller State %x\n", Control);

    switch (Control)
    {
        case OHCI_HC_FUNCTIONAL_STATE_RESET:
            NewControl |= OHCI_HC_FUNCTIONAL_STATE_RESET;
            WaitInMs = 50;
            break;

        case OHCI_HC_FUNCTIONAL_STATE_SUSPEND:
        case OHCI_HC_FUNCTIONAL_STATE_RESUME:
            NewControl |= OHCI_HC_FUNCTIONAL_STATE_RESUME;
            WaitInMs = 10;
            break;

        default:
            WaitInMs = 0;
            break;
    }

retry:
    if (WaitInMs != 0)
    {
        // Do the state transition
        WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_CONTROL_OFFSET), NewControl);

        if (!Again)
        {
            //
            // delay is 100 ms
            //
            Timeout.QuadPart = WaitInMs;
            DPRINT("Waiting %lu milliseconds for controller to transition state\n", Timeout.LowPart);

            //
            // convert to 100 ns units (absolute)
            //
            Timeout.QuadPart *= -10000;

            //
            // perform the wait
            //
            KeDelayExecutionThread(KernelMode, FALSE, &Timeout);
        }
    }

    //
    // now reset controller
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_COMMAND_STATUS_OFFSET), OHCI_HOST_CONTROLLER_RESET);

    //
    // reset time is 10ms
    //
    for(Index = 0; Index < 100; Index++)
    {
        //
        // wait a bit
        //
        KeStallExecutionProcessor(10);

        //
        // read command status
        //
        Reset = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_COMMAND_STATUS_OFFSET));

        //
        // was reset bit cleared
        //
        if ((Reset & OHCI_HOST_CONTROLLER_RESET) == 0)
        {
            //
            // controller completed reset
            //
            break;
        }
    }

    if ((Reset & OHCI_HOST_CONTROLLER_RESET))
    {
        //
        // failed to reset controller
        //
        return STATUS_UNSUCCESSFUL;
    }

    //
    // get frame interval
    //
    FrameInterval = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_FRAME_INTERVAL_OFFSET));
    m_IntervalValue = OHCI_GET_INTERVAL_VALUE(FrameInterval);

    FrameInterval = ((FrameInterval & OHCI_FRAME_INTERVAL_TOGGLE) ^ OHCI_FRAME_INTERVAL_TOGGLE);

    DPRINT("FrameInterval %x IntervalValue %x\n", FrameInterval, m_IntervalValue);
    FrameInterval |= OHCI_FSMPS(m_IntervalValue) | m_IntervalValue;
    DPRINT("Computed FrameInterval %x\n", FrameInterval);

    //
    // write frame interval
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_FRAME_INTERVAL_OFFSET), FrameInterval);

    FrameInterval = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_FRAME_INTERVAL_OFFSET));
    DPRINT("Read FrameInterval %x\n", FrameInterval);

    //
    // 90 % periodic
    //
    Periodic = OHCI_PERIODIC(m_IntervalValue);
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_PERIODIC_START_OFFSET), Periodic);
    DPRINT("Computed Periodic Start %x\n", Periodic);

    Periodic = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_PERIODIC_START_OFFSET));
    DPRINT("Read Periodic Start %x\n", Periodic);

    // Linux does this hack for some bad controllers
    if (!(FrameInterval & 0x3FFF0000) ||
        !(Periodic))
    {
        if (!Again)
        {
            DPRINT1("Trying reset again on faulty controller\n");
            Again = TRUE;
            goto retry;
        }
        else
        {
            DPRINT1("Second reset didn't solve the problem, failing\n");
            return STATUS_UNSUCCESSFUL;
        }
    }

    //
    // lets write physical address of dummy control endpoint descriptor
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_CONTROL_HEAD_ED_OFFSET), m_ControlEndpointDescriptor->PhysicalAddress.LowPart);

    //
    // lets write physical address of dummy bulk endpoint descriptor
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_BULK_HEAD_ED_OFFSET), m_BulkEndpointDescriptor->PhysicalAddress.LowPart);

    //
    // read descriptor A
    //
    Descriptor = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_DESCRIPTOR_A_OFFSET));

    //
    // get port count (in a loop due to AMD errata)
    //
    do
    {
        KeStallExecutionProcessor(20);
        m_NumberOfPorts = OHCI_RH_GET_PORT_COUNT(Descriptor);
    } while (m_NumberOfPorts == 0);

    DPRINT("NumberOfPorts %lu\n", m_NumberOfPorts);
    ASSERT(m_NumberOfPorts < OHCI_MAX_PORT_COUNT);

    //
    // no over current protection
    //
    Descriptor |= OHCI_RH_NO_OVER_CURRENT_PROTECTION;

    //
    // power switching on
    //
    Descriptor &= ~OHCI_RH_NO_POWER_SWITCHING;

    //
    // control each port power independently
    //
    Descriptor |= OHCI_RH_POWER_SWITCHING_MODE;

    //
    // write the configuration back
    //
    DPRINT("Descriptor A: %x\n", Descriptor);
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_DESCRIPTOR_A_OFFSET), Descriptor);

    //
    // read descriptor B
    //
    Descriptor = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_DESCRIPTOR_B_OFFSET));

    //
    // set power power control for each port to use PPS
    //
    for (Port = 1; Port <= m_NumberOfPorts; Port++)
    {
        Descriptor |= (1 << (16 + Port));
    }

    //
    // write the configuration back
    //
    DPRINT("Descriptor B: %x\n", Descriptor);
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_DESCRIPTOR_B_OFFSET), Descriptor);

    //
    // HCCA alignment check
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_HCCA_OFFSET), 0xFFFFFFFF);
    KeStallExecutionProcessor(10);
    Control = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_HCCA_OFFSET));
    ASSERT((m_HCCAPhysicalAddress.LowPart & Control) == m_HCCAPhysicalAddress.LowPart);
    DPRINT("HCCA: %x Alignment mask: %x\n", m_HCCAPhysicalAddress.LowPart, Control);

    //
    // write address of HCCA
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_HCCA_OFFSET), m_HCCAPhysicalAddress.LowPart);

    //
    // now enable the interrupts
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_INTERRUPT_ENABLE_OFFSET), OHCI_NORMAL_INTERRUPTS | OHCI_MASTER_INTERRUPT_ENABLE);

    //
    // enable all queues
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_CONTROL_OFFSET), (NewControl & OHCI_REMOTE_WAKEUP_CONNECTED) | OHCI_ENABLE_LIST);

    //
    // start the controller
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_CONTROL_OFFSET), OHCI_ENABLE_LIST |
                                                                         (NewControl & OHCI_REMOTE_WAKEUP_CONNECTED) |
                                                                         OHCI_CONTROL_BULK_RATIO_1_4 |
                                                                         OHCI_HC_FUNCTIONAL_STATE_OPERATIONAL);

    //
    // wait a bit
    //
    KeStallExecutionProcessor(100);

    //
    // is the controller started
    //
    Control = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_CONTROL_OFFSET));

    //
    // assert that the controller has been started
    //
    ASSERT((Control & OHCI_HC_FUNCTIONAL_STATE_MASK) == OHCI_HC_FUNCTIONAL_STATE_OPERATIONAL);
    ASSERT((Control & OHCI_ENABLE_LIST) == OHCI_ENABLE_LIST);
    DPRINT("Control %x\n", Control);

    //
    // done
    //
    DPRINT("OHCI controller is operational\n");
    return STATUS_SUCCESS;
}

NTSTATUS
CUSBHardwareDevice::AllocateEndpointDescriptor(
    OUT POHCI_ENDPOINT_DESCRIPTOR *OutDescriptor)
{
    POHCI_ENDPOINT_DESCRIPTOR Descriptor;
    PHYSICAL_ADDRESS DescriptorAddress;
    NTSTATUS Status;

    //
    // allocate descriptor
    //
    Status = m_MemoryManager->Allocate(sizeof(OHCI_ENDPOINT_DESCRIPTOR), (PVOID*)&Descriptor, &DescriptorAddress);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to allocate descriptor
        //
        return Status;
    }

    //
    // initialize descriptor
    //
    Descriptor->Flags = OHCI_ENDPOINT_SKIP;
    Descriptor->HeadPhysicalDescriptor = 0;
    Descriptor->NextPhysicalEndpoint = 0;
    Descriptor->TailPhysicalDescriptor = 0;
    Descriptor->PhysicalAddress.QuadPart = DescriptorAddress.QuadPart;

    //
    // store result
    //
    *OutDescriptor = Descriptor;

    //
    // done
    //
    return STATUS_SUCCESS;
}

VOID
STDMETHODCALLTYPE
CUSBHardwareDevice::GetBulkHeadEndpointDescriptor(
    struct _OHCI_ENDPOINT_DESCRIPTOR ** OutDescriptor)
{
    *OutDescriptor = m_BulkEndpointDescriptor;
}

VOID
STDMETHODCALLTYPE
CUSBHardwareDevice::GetInterruptEndpointDescriptors(
    struct _OHCI_ENDPOINT_DESCRIPTOR *** OutDescriptor)
{
    *OutDescriptor = m_InterruptEndpoints;
}

VOID
STDMETHODCALLTYPE
CUSBHardwareDevice::GetIsochronousHeadEndpointDescriptor(
    struct _OHCI_ENDPOINT_DESCRIPTOR ** OutDescriptor)
{
    *OutDescriptor = m_IsoEndpointDescriptor;
}

VOID
STDMETHODCALLTYPE
CUSBHardwareDevice::HeadEndpointDescriptorModified(
    ULONG Type)
{
    if (Type == USB_ENDPOINT_TYPE_CONTROL)
    {
        //
        // notify controller
        //
        WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_COMMAND_STATUS_OFFSET), OHCI_CONTROL_LIST_FILLED);
    }
    else if (Type == USB_ENDPOINT_TYPE_BULK)
    {
        //
        // notify controller
        //
        WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_COMMAND_STATUS_OFFSET), OHCI_BULK_LIST_FILLED);
    }
}

VOID
STDMETHODCALLTYPE
CUSBHardwareDevice::GetControlHeadEndpointDescriptor(
    struct _OHCI_ENDPOINT_DESCRIPTOR ** OutDescriptor)
{
    *OutDescriptor = m_ControlEndpointDescriptor;
}

NTSTATUS
CUSBHardwareDevice::InitializeController()
{
    NTSTATUS Status;
    ULONG Index, Interval, IntervalIndex, InsertIndex;
    POHCI_ENDPOINT_DESCRIPTOR Descriptor;

    //
    // first allocate the hcca area
    //
    Status = m_MemoryManager->Allocate(sizeof(OHCIHCCA), (PVOID*)&m_HCCA, &m_HCCAPhysicalAddress);
    if (!NT_SUCCESS(Status))
    {
        //
        // no memory
        //
        return Status;
    }

    //
    // now allocate an endpoint for control transfers
    // this endpoint will never be removed
    //
    Status = AllocateEndpointDescriptor(&m_ControlEndpointDescriptor);
    if (!NT_SUCCESS(Status))
    {
        //
        // no memory
        //
        return Status;
    }

    //
    // now allocate an endpoint for bulk transfers
    // this endpoint will never be removed
    //
    Status = AllocateEndpointDescriptor(&m_BulkEndpointDescriptor);
    if (!NT_SUCCESS(Status))
    {
        //
        // no memory
        //
        return Status;
    }

    //
    // now allocate an endpoint for iso transfers
    // this endpoint will never be removed
    //
    Status = AllocateEndpointDescriptor(&m_IsoEndpointDescriptor);
    if (!NT_SUCCESS(Status))
    {
        //
        // no memory
        //
        return Status;
    }

    //
    // now allocate endpoint descriptors for iso / interrupt transfers interval is 1,2,4,8,16,32
    //
    for(Index = 0; Index < OHCI_STATIC_ENDPOINT_COUNT; Index++)
    {
        //
        // allocate endpoint descriptor
        //
        Status = AllocateEndpointDescriptor(&Descriptor);
        if (!NT_SUCCESS(Status))
        {
            //
            // no memory
            //
            return Status;
        }

        //
        // save in array
        //
        m_InterruptEndpoints[Index] = Descriptor;
    }


    //
    // now link the descriptors, taken from Haiku
    //
    Interval = OHCI_BIGGEST_INTERVAL;
    IntervalIndex = OHCI_STATIC_ENDPOINT_COUNT - 1;
    while (Interval > 1)
    {
        InsertIndex = Interval / 2;
        while (InsertIndex < OHCI_BIGGEST_INTERVAL)
        {
            //
            // assign endpoint address
            //
            m_HCCA->InterruptTable[InsertIndex] = m_InterruptEndpoints[IntervalIndex]->PhysicalAddress.LowPart;
            InsertIndex += Interval;
        }

        IntervalIndex--;
        Interval /= 2;
    }

    //
    // link all endpoint descriptors to first descriptor in array
    //
    m_HCCA->InterruptTable[0] = m_InterruptEndpoints[0]->PhysicalAddress.LowPart;
    for (Index = 1; Index < OHCI_STATIC_ENDPOINT_COUNT; Index++)
    {
        //
        // link descriptor
        //
        m_InterruptEndpoints[Index]->NextPhysicalEndpoint = m_InterruptEndpoints[0]->PhysicalAddress.LowPart;
    }

    //
    // Now link the first endpoint to the isochronous endpoint
    //
    m_InterruptEndpoints[0]->NextPhysicalEndpoint = m_IsoEndpointDescriptor->PhysicalAddress.LowPart;

    //
    // set iso endpoint type
    //
    m_IsoEndpointDescriptor->Flags |= OHCI_ENDPOINT_ISOCHRONOUS_FORMAT;

    //
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
CUSBHardwareDevice::StopController(void)
{
    ASSERT(FALSE);

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::ResetPort(
    IN ULONG PortIndex)
{
    ASSERT(FALSE);

    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::GetPortStatus(
    ULONG PortId,
    OUT USHORT *PortStatus,
    OUT USHORT *PortChange)
{
    ULONG Value;

    if (PortId > m_NumberOfPorts)
        return STATUS_UNSUCCESSFUL;

    // init result variables
    *PortStatus = 0;
    *PortChange = 0;

    //
    // read port status
    //
    Value = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_PORT_STATUS(PortId)));
    DPRINT("GetPortStatus PortId %x Value %x\n", PortId, Value);

    // connected
    if (Value & OHCI_RH_PORTSTATUS_CCS)
    {
        *PortStatus |= USB_PORT_STATUS_CONNECT;

        // low speed device
        if (Value & OHCI_RH_PORTSTATUS_LSDA)
            *PortStatus |= USB_PORT_STATUS_LOW_SPEED;
    }

    // did a device connect?
    if (Value & OHCI_RH_PORTSTATUS_CSC)
        *PortChange |= USB_PORT_STATUS_CONNECT;

    // port enabled
    if (Value & OHCI_RH_PORTSTATUS_PES)
        *PortStatus |= USB_PORT_STATUS_ENABLE;

    // port disconnect or hardware error
    if (Value & OHCI_RH_PORTSTATUS_PESC)
        *PortChange |= USB_PORT_STATUS_CONNECT;

    // port suspend
    if (Value & OHCI_RH_PORTSTATUS_PSS)
        *PortStatus |= USB_PORT_STATUS_SUSPEND;

    // port suspend
    if (Value & OHCI_RH_PORTSTATUS_PSSC)
        *PortChange |= USB_PORT_STATUS_ENABLE;

    // port reset started
    if (Value & OHCI_RH_PORTSTATUS_PRS)
        *PortStatus |= USB_PORT_STATUS_RESET;

    // port reset ended
    if (Value & OHCI_RH_PORTSTATUS_PRSC)
        *PortChange |= USB_PORT_STATUS_RESET;

    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::ClearPortStatus(
    ULONG PortId,
    ULONG Status)
{
    ULONG Value;

    DPRINT("CUSBHardwareDevice::ClearPortStatus PortId %x Feature %x\n", PortId, Status);

    if (PortId > m_NumberOfPorts)
        return STATUS_UNSUCCESSFUL;

    Value = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_PORT_STATUS(PortId)));

    if (Status == C_PORT_RESET)
    {
        //
        // sanity checks
        //
        ASSERT((Value & OHCI_RH_PORTSTATUS_PRSC));

        //
        // clear reset bit complete
        //
        WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_PORT_STATUS(PortId)), OHCI_RH_PORTSTATUS_PRSC);

        //
        // sanity check
        //
        ASSERT((Value & OHCI_RH_PORTSTATUS_PES));
    }

    if (Status == C_PORT_CONNECTION || Status == C_PORT_ENABLE)
    {
        //
        // clear change bits
        //
        WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_PORT_STATUS(PortId)), Value & (OHCI_RH_PORTSTATUS_CSC | OHCI_RH_PORTSTATUS_PESC));

        //
        // wait for port to stabilize
        //
        if (Status == C_PORT_CONNECTION && (Value & OHCI_RH_PORTSTATUS_CCS))
        {
            LARGE_INTEGER Timeout;

            //
            // delay is 100 ms
            //
            Timeout.QuadPart = 100;
            DPRINT1("Waiting %lu milliseconds for port to stabilize after connection\n", Timeout.LowPart);

            //
            // convert to 100 ns units (absolute)
            //
            Timeout.QuadPart *= -10000;

            //
            // perform the wait
            //
            KeDelayExecutionThread(KernelMode, FALSE, &Timeout);
        }
    }

    //
    // re-enable root hub change
    //
    DPRINT("Enabling status change\n");
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_INTERRUPT_ENABLE_OFFSET), OHCI_ROOT_HUB_STATUS_CHANGE);

    return STATUS_SUCCESS;
}


NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::SetPortFeature(
    ULONG PortId,
    ULONG Feature)
{
    ULONG Value;

    DPRINT("CUSBHardwareDevice::SetPortFeature PortId %x Feature %x\n", PortId, Feature);

    //
    // read port status
    //
    Value = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_PORT_STATUS(PortId)));


    if (Feature == PORT_ENABLE)
    {
        //
        // enable port
        //
        WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_PORT_STATUS(PortId)), OHCI_RH_PORTSTATUS_PES);
        return STATUS_SUCCESS;
    }
    else if (Feature == PORT_POWER)
    {
        LARGE_INTEGER Timeout;

        //
        // enable power
        //
        WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_PORT_STATUS(PortId)), OHCI_RH_PORTSTATUS_PPS);

        //
        // read descriptor A for the delay data
        //
        Value = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_DESCRIPTOR_A_OFFSET));

        //
        // compute the delay
        //
        Timeout.QuadPart = OHCI_RH_GET_POWER_ON_TO_POWER_GOOD_TIME(Value);

        //
        // delay is multiplied by 2 ms
        //
        Timeout.QuadPart *= 2;
        DPRINT("Waiting %lu milliseconds for port power up\n", Timeout.LowPart);

        //
        // convert to 100 ns units (absolute)
        //
        Timeout.QuadPart *= -10000;

        //
        // perform the wait
        //
        KeDelayExecutionThread(KernelMode, FALSE, &Timeout);

        return STATUS_SUCCESS;
    }
    else if (Feature == PORT_SUSPEND)
    {
        //
        // enable port
        //
        WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_PORT_STATUS(PortId)), OHCI_RH_PORTSTATUS_PSS);
        return STATUS_SUCCESS;
    }
    else if (Feature == PORT_RESET)
    {
        //
        // assert
        //
        ASSERT((Value & OHCI_RH_PORTSTATUS_CCS));

        //
        // reset port
        //
        WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_PORT_STATUS(PortId)), OHCI_RH_PORTSTATUS_PRS);

        //
        // an interrupt signals the reset completion
        //
        return STATUS_SUCCESS;
    }
    return STATUS_SUCCESS;
}



VOID
STDMETHODCALLTYPE
CUSBHardwareDevice::SetStatusChangeEndpointCallBack(
    PVOID CallBack,
    PVOID Context)
{
    m_SCECallBack = (HD_INIT_CALLBACK*)CallBack;
    m_SCEContext = Context;
}

VOID
STDMETHODCALLTYPE
CUSBHardwareDevice::GetCurrentFrameNumber(
    PULONG FrameNumber)
{
    ULONG Control;
    ULONG Number;


    Number = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_FRAME_INTERVAL_NUMBER_OFFSET));
    DPRINT("FrameNumberInterval %x Frame %x\n", Number, m_HCCA->CurrentFrameNumber);

    //
    // remove reserved bits
    //
    Number &= 0xFFFF;

    //
    // store frame number
    //
    *FrameNumber = Number;

    //
    // is the controller started
    //
    Control = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_CONTROL_OFFSET));
    ASSERT((Control & OHCI_ENABLE_LIST) == OHCI_ENABLE_LIST);


}


BOOLEAN
NTAPI
InterruptServiceRoutine(
    IN PKINTERRUPT  Interrupt,
    IN PVOID  ServiceContext)
{
    CUSBHardwareDevice *This;
    ULONG DoneHead, Status, Acknowledge = 0;

    //
    // get context
    //
    This = (CUSBHardwareDevice*) ServiceContext;

    DPRINT("InterruptServiceRoutine\n");

    //
    // get done head
    //
    DoneHead = This->m_HCCA->DoneHead;

    //
    // check if zero
    //
    if (DoneHead == 0)
    {
        //
        // the interrupt was not caused by DoneHead update
        // check if something important happened
        //
        DPRINT("InterruptStatus %x  InterruptEnable %x\n", READ_REGISTER_ULONG((PULONG)((PUCHAR)This->m_Base + OHCI_INTERRUPT_STATUS_OFFSET)),
                                                            READ_REGISTER_ULONG((PULONG)((PUCHAR)This->m_Base + OHCI_INTERRUPT_ENABLE_OFFSET)));
        Status = READ_REGISTER_ULONG((PULONG)((PUCHAR)This->m_Base + OHCI_INTERRUPT_STATUS_OFFSET)) & READ_REGISTER_ULONG((PULONG)((PUCHAR)This->m_Base + OHCI_INTERRUPT_ENABLE_OFFSET)) & (~OHCI_WRITEBACK_DONE_HEAD);
        if (Status == 0)
        {
            //
            // nothing happened, appears to be shared interrupt
            //
            return FALSE;
        }
    }
    else
    {
        //
        // DoneHead update happened, check if there are other events too
        //
        Status = OHCI_WRITEBACK_DONE_HEAD;

        //
        // since ed descriptors are 16 byte aligned, the controller sets the lower bits if there were other interrupt requests
        //
        if (DoneHead & OHCI_DONE_INTERRUPTS)
        {
            //
            // get other events
            //
            Status |= READ_REGISTER_ULONG((PULONG)((PUCHAR)This->m_Base + OHCI_INTERRUPT_STATUS_OFFSET)) & READ_REGISTER_ULONG((PULONG)((PUCHAR)This->m_Base + OHCI_INTERRUPT_ENABLE_OFFSET));
        }
    }

    //
    // sanity check
    //
    ASSERT(Status != 0);

     if (Status & OHCI_WRITEBACK_DONE_HEAD)
     {
         //
         // head completed
         //
         Acknowledge |= OHCI_WRITEBACK_DONE_HEAD;
         This->m_HCCA->DoneHead = 0;
    }

    if (Status & OHCI_RESUME_DETECTED)
    {
        //
        // resume
        //
        DPRINT1("InterruptServiceRoutine> Resume\n");
        Acknowledge |= OHCI_RESUME_DETECTED;
    }


    if (Status & OHCI_UNRECOVERABLE_ERROR)
    {
        DPRINT1("InterruptServiceRoutine> Controller error\n");

        //
        // halt controller
        //
        ASSERT(FALSE);
        WRITE_REGISTER_ULONG((PULONG)((PUCHAR)This->m_Base + OHCI_CONTROL_OFFSET), OHCI_HC_FUNCTIONAL_STATE_RESET);
    }

    if (Status & OHCI_ROOT_HUB_STATUS_CHANGE)
    {
        //
        // disable interrupt as it will fire untill the port has been reset
        //
        DPRINT1("Disabling status change interrupt\n");
        WRITE_REGISTER_ULONG((PULONG)((PUCHAR)This->m_Base + OHCI_INTERRUPT_DISABLE_OFFSET), OHCI_ROOT_HUB_STATUS_CHANGE);
        Acknowledge |= OHCI_ROOT_HUB_STATUS_CHANGE;
    }

    //
    // is there something to acknowledge
    //
    if (Acknowledge)
    {
        //
        // ack change
        //
        WRITE_REGISTER_ULONG((PULONG)((PUCHAR)This->m_Base + OHCI_INTERRUPT_STATUS_OFFSET), Acknowledge);
    }

    //
    // defer processing
    //
    DPRINT("Status %x Acknowledge %x FrameNumber %x\n", Status, Acknowledge, This->m_HCCA->CurrentFrameNumber);
    KeInsertQueueDpc(&This->m_IntDpcObject, (PVOID)Status, (PVOID)(DoneHead & ~1));

    //
    // interrupt handled
    //
    return TRUE;
}

VOID
NTAPI
OhciDeferredRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2)
{
    CUSBHardwareDevice *This;
    ULONG CStatus, Index, PortStatus;
    ULONG DoneHead, QueueSCEWorkItem;

    //
    // get parameters
    //
    This = (CUSBHardwareDevice*)DeferredContext;
    CStatus = (ULONG) SystemArgument1;
    DoneHead = (ULONG)SystemArgument2;

    DPRINT("OhciDeferredRoutine Status %x DoneHead %x\n", CStatus, DoneHead);

    if (CStatus & OHCI_WRITEBACK_DONE_HEAD)
    {
        //
        // notify queue of event
        //
        This->m_UsbQueue->TransferDescriptorCompletionCallback(DoneHead);
    }
    if (CStatus & OHCI_ROOT_HUB_STATUS_CHANGE)
    {
        //
        // device connected, lets check which port
        //
        QueueSCEWorkItem = FALSE;
        for(Index = 0; Index < This->m_NumberOfPorts; Index++)
        {
            //
            // read port status
            //
            PortStatus = READ_REGISTER_ULONG((PULONG)((PUCHAR)This->m_Base + OHCI_RH_PORT_STATUS(Index)));

            //
            // check if there is a status change
            //
            if (PortStatus & OHCI_RH_PORTSTATUS_CSC)
            {
                //
                // did a device connect
                //
                if (PortStatus & OHCI_RH_PORTSTATUS_CCS)
                {
                    //
                    // device connected
                    //
                    DPRINT1("New device arrival at Port %lu LowSpeed %x\n", Index, (PortStatus & OHCI_RH_PORTSTATUS_LSDA));

                    //
                    // enable port
                    //
                    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)This->m_Base + OHCI_RH_PORT_STATUS(Index)), OHCI_RH_PORTSTATUS_PES);
                }
                else
                {
                    //
                    // device disconnected
                    //
                    DPRINT1("Device disconnected at Port %x\n", Index);
                }

                //
                // work to do
                //
                QueueSCEWorkItem = TRUE;
            }
            else if (PortStatus & OHCI_RH_PORTSTATUS_PESC)
            {
                //
                // device disconnected or some error condition
                //
                ASSERT(!(PortStatus & OHCI_RH_PORTSTATUS_PES));

                //
                // work to do
                //
                QueueSCEWorkItem = TRUE;
            }
            else if (PortStatus & OHCI_RH_PORTSTATUS_PRSC)
            {
                //
                // This is a port reset complete interrupt
                //
                DPRINT1("Port %lu completed reset\n", Index);

                //
                // Queue a work item
                //
                QueueSCEWorkItem = TRUE;
            }
        }

        //
        // is there a status change callback and a device connected / disconnected
        //
        if (QueueSCEWorkItem && This->m_SCECallBack != NULL)
        {
            if (InterlockedCompareExchange(&This->m_StatusChangeWorkItemStatus, 1, 0) == 0)
            {
                //
                // queue work item for processing
                //
                ExQueueWorkItem(&This->m_StatusChangeWorkItem, DelayedWorkQueue);
            }
        }
    }
}

VOID
NTAPI
StatusChangeWorkItemRoutine(
    PVOID Context)
{
    //
    // cast to hardware object
    //
    CUSBHardwareDevice * This = (CUSBHardwareDevice*)Context;

    //
    // is there a callback
    //
    if (This->m_SCECallBack)
    {
        //
        // issue callback
        //
        This->m_SCECallBack(This->m_SCEContext);
    }

    //
    // reset active status
    //
    InterlockedDecrement(&This->m_StatusChangeWorkItemStatus);
}

NTSTATUS
NTAPI
CreateUSBHardware(
    PUSBHARDWAREDEVICE *OutHardware)
{
    PUSBHARDWAREDEVICE This;

    This = new(NonPagedPool, TAG_USBOHCI) CUSBHardwareDevice(0);

    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->AddRef();

    // return result
    *OutHardware = (PUSBHARDWAREDEVICE)This;

    return STATUS_SUCCESS;
}
