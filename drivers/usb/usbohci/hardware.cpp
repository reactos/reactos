/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbohci/hcd_controller.cpp
 * PURPOSE:     USB OHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#define INITGUID
#include "usbohci.h"
#include "hardware.h"

typedef VOID __stdcall HD_INIT_CALLBACK(IN PVOID CallBackContext);

BOOLEAN
NTAPI
InterruptServiceRoutine(
    IN PKINTERRUPT  Interrupt,
    IN PVOID  ServiceContext);

VOID
NTAPI
OhciDefferedRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2);

VOID
NTAPI
StatusChangeWorkItemRoutine(PVOID Context);

class CUSBHardwareDevice : public IUSBHardwareDevice
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
    NTSTATUS Initialize(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT FunctionalDeviceObject, PDEVICE_OBJECT PhysicalDeviceObject, PDEVICE_OBJECT LowerDeviceObject);
    NTSTATUS PnpStart(PCM_RESOURCE_LIST RawResources, PCM_RESOURCE_LIST TranslatedResources);
    NTSTATUS PnpStop(void);
    NTSTATUS HandlePower(PIRP Irp);
    NTSTATUS GetDeviceDetails(PUSHORT VendorId, PUSHORT DeviceId, PULONG NumberOfPorts, PULONG Speed);
    NTSTATUS GetBulkHeadEndpointDescriptor(struct _OHCI_ENDPOINT_DESCRIPTOR ** OutDescriptor);
    NTSTATUS GetControlHeadEndpointDescriptor(struct _OHCI_ENDPOINT_DESCRIPTOR ** OutDescriptor);
    NTSTATUS GetInterruptEndpointDescriptors(struct _OHCI_ENDPOINT_DESCRIPTOR *** OutDescriptor);
    NTSTATUS GetIsochronousHeadEndpointDescriptor(struct _OHCI_ENDPOINT_DESCRIPTOR ** OutDescriptor);
    VOID HeadEndpointDescriptorModified(ULONG HeadType);


    NTSTATUS GetDMA(OUT struct IDMAMemoryManager **m_DmaManager);
    NTSTATUS GetUSBQueue(OUT struct IUSBQueue **OutUsbQueue);

    NTSTATUS StartController();
    NTSTATUS StopController();
    NTSTATUS ResetController();
    NTSTATUS ResetPort(ULONG PortIndex);

    NTSTATUS GetPortStatus(ULONG PortId, OUT USHORT *PortStatus, OUT USHORT *PortChange);
    NTSTATUS ClearPortStatus(ULONG PortId, ULONG Status);
    NTSTATUS SetPortFeature(ULONG PortId, ULONG Feature);

    VOID SetStatusChangeEndpointCallBack(PVOID CallBack, PVOID Context);

    KIRQL AcquireDeviceLock(void);
    VOID ReleaseDeviceLock(KIRQL OldLevel);
    virtual VOID GetCurrentFrameNumber(PULONG FrameNumber);
    // local
    BOOLEAN InterruptService();
    NTSTATUS InitializeController();
    NTSTATUS AllocateEndpointDescriptor(OUT POHCI_ENDPOINT_DESCRIPTOR *OutDescriptor);

    // friend function
    friend BOOLEAN NTAPI InterruptServiceRoutine(IN PKINTERRUPT  Interrupt, IN PVOID  ServiceContext);
    friend VOID NTAPI OhciDefferedRoutine(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2);
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
    PUSBQUEUE m_UsbQueue;                                                              // usb request queue
    POHCIHCCA m_HCCA;                                                                  // hcca virtual base
    PHYSICAL_ADDRESS m_HCCAPhysicalAddress;                                            // hcca physical address
    POHCI_ENDPOINT_DESCRIPTOR m_ControlEndpointDescriptor;                             // dummy control endpoint descriptor
    POHCI_ENDPOINT_DESCRIPTOR m_BulkEndpointDescriptor;                                // dummy control endpoint descriptor
    POHCI_ENDPOINT_DESCRIPTOR m_IsoEndpointDescriptor;                                 // iso endpoint descriptor
    POHCI_ENDPOINT_DESCRIPTOR m_InterruptEndpoints[OHCI_STATIC_ENDPOINT_COUNT];        // endpoints for interrupt / iso transfers
    ULONG m_NumberOfPorts;                                                             // number of ports
    OHCI_PORT_STATUS m_PortStatus[OHCI_MAX_PORT_COUNT];                                // port change status
    PDMAMEMORYMANAGER m_MemoryManager;                                                 // memory manager
    HD_INIT_CALLBACK* m_SCECallBack;                                                   // status change callback routine
    PVOID m_SCEContext;                                                                // status change callback routine context
    WORK_QUEUE_ITEM m_StatusChangeWorkItem;                                            // work item for status change callback
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

NTSTATUS
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

    DPRINT1("CUSBHardwareDevice::Initialize\n");

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
    Status = CreateUSBQueue(&m_UsbQueue);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create UsbQueue!\n");
        return Status;
    }

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
    // intialize status change work item
    //
    ExInitializeWorkItem(&m_StatusChangeWorkItem, StatusChangeWorkItemRoutine, PVOID(this));

    m_VendorID = 0;
    m_DeviceID = 0;

    Status = GetBusInterface(PhysicalDeviceObject, &BusInterface);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get BusInteface!\n");
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

    if (!(PciConfig.Command & PCI_ENABLE_BUS_MASTER))
    {
        DPRINT1("PCI Configuration shows this as a non Bus Mastering device!\n");
    }

    m_VendorID = PciConfig.VendorID;
    m_DeviceID = PciConfig.DeviceID;

    return STATUS_SUCCESS;
}

NTSTATUS
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

    DPRINT1("CUSBHardwareDevice::PnpStart\n");
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
                                OhciDefferedRoutine,
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

                DPRINT1("Version %x\n", Version);

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
        ASSERT(FALSE);
        return Status;
    }

    //
    // Initialize the UsbQueue now that we have an AdapterObject.
    //
    Status = m_UsbQueue->Initialize(PUSBHARDWAREDEVICE(this), m_Adapter, m_MemoryManager, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to Initialize the UsbQueue\n");
        return Status;
    }


    //
    // Stop the controller before modifying schedules
    //
    Status = StopController();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to stop the controller \n");
        ASSERT(FALSE);
        return Status;
    }


    //
    // Start the controller
    //
    DPRINT1("Starting Controller\n");
    Status = StartController();

    //
    // done
    //
    return Status;
}

NTSTATUS
CUSBHardwareDevice::PnpStop(void)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
CUSBHardwareDevice::HandlePower(
    PIRP Irp)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
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

NTSTATUS CUSBHardwareDevice::GetDMA(
    OUT struct IDMAMemoryManager **OutDMAMemoryManager)
{
    if (!m_MemoryManager)
        return STATUS_UNSUCCESSFUL;
    *OutDMAMemoryManager = m_MemoryManager;
    return STATUS_SUCCESS;
}

NTSTATUS
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
    ULONG Control, NumberOfPorts, Index, Descriptor, FrameInterval, Periodic;

    //
    // first write address of HCCA
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_HCCA_OFFSET), m_HCCAPhysicalAddress.LowPart);

    //
    // lets write physical address of dummy control endpoint descriptor
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_CONTROL_HEAD_ED_OFFSET), m_ControlEndpointDescriptor->PhysicalAddress.LowPart);

    //
    // lets write physical address of dummy bulk endpoint descriptor
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_BULK_HEAD_ED_OFFSET), m_BulkEndpointDescriptor->PhysicalAddress.LowPart);

    //
    // read control register
    //
    Control = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_CONTROL_OFFSET));

    //
    // remove flags
    // 
    Control &= ~(OHCI_CONTROL_BULK_SERVICE_RATIO_MASK | OHCI_ENABLE_LIST | OHCI_HC_FUNCTIONAL_STATE_MASK | OHCI_INTERRUPT_ROUTING);

    //
    // set command status flags
    //
    Control |= OHCI_ENABLE_LIST | OHCI_CONTROL_BULK_RATIO_1_4 | OHCI_HC_FUNCTIONAL_STATE_OPERATIONAL;

    //
    // now start the controller
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_CONTROL_OFFSET), Control);

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
    DPRINT1("Control %x\n", Control);

    //
    // get frame interval
    //
    FrameInterval = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_FRAME_INTERVAL_OFFSET));
    FrameInterval = ((FrameInterval & OHCI_FRAME_INTERVAL_TOGGLE) ^ OHCI_FRAME_INTERVAL_TOGGLE);
    DPRINT1("FrameInterval %x IntervalValue %x\n", FrameInterval, m_IntervalValue);
    FrameInterval |= OHCI_FSMPS(m_IntervalValue) | m_IntervalValue;
    DPRINT1("FrameInterval %x\n", FrameInterval);

    //
    // write frame interval
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_FRAME_INTERVAL_OFFSET), FrameInterval);

    //
    // 90 % periodic
    //
    Periodic = OHCI_PERIODIC(m_IntervalValue);
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_PERIODIC_START_OFFSET), Periodic);
    DPRINT1("Periodic Start %x\n", Periodic);

    //
    // read descriptor
    //
    Descriptor = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_DESCRIPTOR_A_OFFSET));

    //
    // no over current protection
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_DESCRIPTOR_A_OFFSET), Descriptor | OHCI_RH_NO_OVER_CURRENT_PROTECTION);

    //
    // enable power on all ports
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_STATUS_OFFSET), OHCI_RH_LOCAL_POWER_STATUS_CHANGE);

    //
    // wait a bit
    //
    KeStallExecutionProcessor(10);

    //
    // write descriptor
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_DESCRIPTOR_A_OFFSET), Descriptor);



    //
    // retrieve number of ports
    //
    for(Index = 0; Index < 10; Index++)
    {
        //
        // wait a bit
        //
        KeStallExecutionProcessor(10);

        //
        // read descriptor
        //
        Descriptor = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_DESCRIPTOR_A_OFFSET));

        //
        // get number of ports
        //
        NumberOfPorts = OHCI_RH_GET_PORT_COUNT(Descriptor);

        //
        // check if we have received the ports
        //
        if (NumberOfPorts)
            break;
    }

    //
    // sanity check
    //
    ASSERT(NumberOfPorts < OHCI_MAX_PORT_COUNT);

    //
    // store number of ports
    //
    m_NumberOfPorts = NumberOfPorts;

    //
    // print out number ports
    //
    DPRINT1("NumberOfPorts %lu\n", m_NumberOfPorts);


    //
    // now enable the interrupts
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_INTERRUPT_ENABLE_OFFSET), OHCI_NORMAL_INTERRUPTS | OHCI_MASTER_INTERRUPT_ENABLE);

    //
    // done
    //
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
    // intialize descriptor
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

NTSTATUS
CUSBHardwareDevice::GetBulkHeadEndpointDescriptor(
    struct _OHCI_ENDPOINT_DESCRIPTOR ** OutDescriptor)
{
    *OutDescriptor = m_BulkEndpointDescriptor;
    return STATUS_SUCCESS;
}

NTSTATUS
CUSBHardwareDevice::GetInterruptEndpointDescriptors(
    struct _OHCI_ENDPOINT_DESCRIPTOR *** OutDescriptor)
{
    *OutDescriptor = m_InterruptEndpoints;
    return STATUS_SUCCESS;
}

NTSTATUS
CUSBHardwareDevice::GetIsochronousHeadEndpointDescriptor(
    struct _OHCI_ENDPOINT_DESCRIPTOR ** OutDescriptor)
{
    //
    // get descriptor
    //
    *OutDescriptor = m_IsoEndpointDescriptor;
    return STATUS_SUCCESS;
}

VOID
CUSBHardwareDevice::HeadEndpointDescriptorModified(
    ULONG Type)
{
    ULONG Value = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_COMMAND_STATUS_OFFSET));

    if (Type == USB_ENDPOINT_TYPE_CONTROL)
    {
        //
        // notify controller
        //
        WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_COMMAND_STATUS_OFFSET), Value | OHCI_CONTROL_LIST_FILLED);
    }
    else if (Type == USB_ENDPOINT_TYPE_BULK)
    {
        //
        // notify controller
        //
        WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_COMMAND_STATUS_OFFSET), Value | OHCI_BULK_LIST_FILLED);
    }
}

NTSTATUS
CUSBHardwareDevice::GetControlHeadEndpointDescriptor(
    struct _OHCI_ENDPOINT_DESCRIPTOR ** OutDescriptor)
{
    *OutDescriptor = m_ControlEndpointDescriptor;
    return STATUS_SUCCESS;
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
    ULONG Control, Reset, Status;
    ULONG Index, FrameInterval;

    //
    // alignment check
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_HCCA_OFFSET, 0xFFFFFFFF);
    Control = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_HCCA_OFFSET));
    DPRINT1("HcHCCA Alignment %x\n", Control);
    ASSERT((Control & 0xFFFFFFF0) == 0xFFFFFFF0);

    //
    // check context
    //
    Control = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_CONTROL_OFFSET));

    if ((Control & OHCI_INTERRUPT_ROUTING))
    {
        //
        // read command status
        //
        Status = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_COMMAND_STATUS_OFFSET));

        //
        // change ownership
        //
        WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_COMMAND_STATUS_OFFSET), Status | OHCI_OWNERSHIP_CHANGE_REQUEST);
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
    else
    {
        //
        // read contents of control register
        //
        Control = (READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_CONTROL_OFFSET)) & OHCI_HC_FUNCTIONAL_STATE_MASK);
        DPRINT1("Controller State %x\n", Control);

        if (Control != OHCI_HC_FUNCTIONAL_STATE_RESET)
        {
            //
            // OHCI 5.1.1.3.4, no SMM, BIOS active
            //
            if (Control != OHCI_HC_FUNCTIONAL_STATE_OPERATIONAL)
            {
                //
                // lets resume
                //
                WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_CONTROL_OFFSET), OHCI_HC_FUNCTIONAL_STATE_RESUME);
                Index = 0;
                do
                {
                    //
                    // wait untill its resumed
                    //
                    KeStallExecutionProcessor(10);

                    //
                    // check control register
                    //
                    Control = (READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_CONTROL_OFFSET)) & OHCI_HC_FUNCTIONAL_STATE_MASK);
                    if (Control & OHCI_HC_FUNCTIONAL_STATE_RESUME)
                    {
                        //
                        // it has resumed
                        //
                        break;
                    }

                    //
                    // check for time outs
                    //
                    Index++;
                    if(Index > 100)
                    {
                        DPRINT1("Failed to resume controller\n");
                        break;
                    }
                }while(TRUE);
            }
        }
        else
        {
            //
            // 5.1.1.3.5 OHCI, no SMM, no BIOS
            //
            Index = 0;

            //
            // some controllers also depend on this
            //
            WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_CONTROL_OFFSET), OHCI_HC_FUNCTIONAL_STATE_RESET);
            do
            {
                 //
                 // wait untill its reset
                 //
                 KeStallExecutionProcessor(10);

                 //
                 // check control register
                 //
                 Control = (READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_CONTROL_OFFSET)) & OHCI_HC_FUNCTIONAL_STATE_MASK);
                 if (Control == OHCI_HC_FUNCTIONAL_STATE_RESET)
                 {
                     //
                     // it has reset
                     //
                     break;
                 }

                 //
                 // check for time outs
                 //
                 Index++;
                 if(Index > 100)
                 {
                    DPRINT1("Failed to reset controller\n");
                    break;
                 }

            }while(TRUE);
        }
    }

    //
    // read from interval
    //
    FrameInterval = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_FRAME_INTERVAL_OFFSET));

    //
    // store interval value for later
    //
    m_IntervalValue = OHCI_GET_INTERVAL_VALUE(FrameInterval);

    DPRINT1("FrameInterval %x Interval %x\n", FrameInterval, m_IntervalValue);

    //
    // now reset controller
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_COMMAND_STATUS_OFFSET), OHCI_HOST_CONTROLLER_RESET);
 
    //
    // reset time is 10ms
    //
    for(Index = 0; Index < 10; Index++)
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
            // restore the frame interval register
            //
            WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_FRAME_INTERVAL_OFFSET), FrameInterval);

            //
            // controller completed reset
            //
            return STATUS_SUCCESS;
        }
    }

    //
    // failed to reset controller
    //
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
CUSBHardwareDevice::ResetController(void)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
CUSBHardwareDevice::ResetPort(
    IN ULONG PortIndex)
{
    ASSERT(FALSE);

    return STATUS_SUCCESS;
}

NTSTATUS
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
        *PortStatus |= USB_PORT_STATUS_CONNECT;

    // did a device connect?
    if (Value & OHCI_RH_PORTSTATUS_CSC)
        *PortChange |= USB_PORT_STATUS_CONNECT;

    // port enabled
    if (Value & OHCI_RH_PORTSTATUS_PES)
        *PortStatus |= USB_PORT_STATUS_ENABLE;

    // port enabled
    if (Value & OHCI_RH_PORTSTATUS_PESC)
        *PortChange |= USB_PORT_STATUS_ENABLE;

    // port suspend
    if (Value & OHCI_RH_PORTSTATUS_PSS)
        *PortStatus |= USB_PORT_STATUS_SUSPEND;

    // port suspend
    if (Value & OHCI_RH_PORTSTATUS_PSSC)
        *PortChange |= USB_PORT_STATUS_ENABLE;

    // port reset
    if (Value & OHCI_RH_PORTSTATUS_PSS)
        *PortStatus |= USB_PORT_STATUS_RESET;

    // port reset
    if (Value & OHCI_RH_PORTSTATUS_PRSC)
        *PortChange |= USB_PORT_STATUS_RESET;

    return STATUS_SUCCESS;
}

NTSTATUS
CUSBHardwareDevice::ClearPortStatus(
    ULONG PortId,
    ULONG Status)
{
    ULONG Value;

    DPRINT("CUSBHardwareDevice::ClearPortStatus PortId %x Feature %x\n", PortId, Status);

    if (PortId > m_NumberOfPorts)
        return STATUS_UNSUCCESSFUL;

    Value = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_PORT_STATUS(PortId)));
    KeStallExecutionProcessor(100);

    if (Status == C_PORT_RESET)
    {
        do
        {
           //
           // read port status
           //
           Value = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_PORT_STATUS(PortId)));

           if ((Value & OHCI_RH_PORTSTATUS_PRS)  == 0)
           {
               //
               // reset is complete
               //
               break;
           }

           //
           // wait a bit
           //
           KeStallExecutionProcessor(100);

           //DPRINT1("Value %x Index %lu\n", Value, Index);

        }while(TRUE);

        //
        // check if reset bit is still set
        //
        if (Value & OHCI_RH_PORTSTATUS_PRS)
        {
            //
            // reset failed
            //
            DPRINT1("PortId %lu Reset failed\n", PortId);
            return STATUS_UNSUCCESSFUL;
        }

        //
        // sanity checks
        //
        ASSERT((Value & OHCI_RH_PORTSTATUS_PRS) == 0);
        ASSERT((Value & OHCI_RH_PORTSTATUS_PRSC));

        //
        // clear reset bit complete
        //
        WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_PORT_STATUS(PortId)), OHCI_RH_PORTSTATUS_PRSC);

        //
        // read status register
        //
        Value = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_PORT_STATUS(PortId)));

        //
        // reset complete bit should be cleared
        //
        ASSERT((Value & OHCI_RH_PORTSTATUS_PRSC) == 0);

        //
        // update port status
        //
        m_PortStatus[PortId].PortChange &= ~USB_PORT_STATUS_RESET;

        //
        // sanity check
        //
        ASSERT((Value & OHCI_RH_PORTSTATUS_PES));

        //
        // port is enabled
        //
        m_PortStatus[PortId].PortStatus |= USB_PORT_STATUS_ENABLE;

        //
        // re-enable root hub change
        //
        Value = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_INTERRUPT_ENABLE_OFFSET));
        WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_INTERRUPT_ENABLE_OFFSET), Value | OHCI_ROOT_HUB_STATUS_CHANGE);

    }

    if (Status == C_PORT_CONNECTION)
    {
        //
        // clear bit
        //
        WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_PORT_STATUS(PortId)), OHCI_RH_PORTSTATUS_CSC);
        m_PortStatus[PortId].PortChange &= ~USB_PORT_STATUS_CONNECT;
    }



    return STATUS_SUCCESS;
}


NTSTATUS
CUSBHardwareDevice::SetPortFeature(
    ULONG PortId,
    ULONG Feature)
{
    ULONG Value;

    DPRINT1("CUSBHardwareDevice::SetPortFeature PortId %x Feature %x\n", PortId, Feature);

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
        //
        // enable power
        //
        WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_PORT_STATUS(PortId)), OHCI_RH_PORTSTATUS_PPS);
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
        // wait 
        //
        KeStallExecutionProcessor(100);

        //
        // update cached settings
        //
        m_PortStatus[PortId].PortChange |= USB_PORT_STATUS_RESET;
        m_PortStatus[PortId].PortStatus &= ~USB_PORT_STATUS_ENABLE;

        //
        // is there a status change callback
        //
        if (m_SCECallBack != NULL)
        {
            //
            // issue callback
            //
            m_SCECallBack(m_SCEContext);
        }
    }
    return STATUS_SUCCESS;
}



VOID
CUSBHardwareDevice::SetStatusChangeEndpointCallBack(
    PVOID CallBack,
    PVOID Context)
{
    m_SCECallBack = (HD_INIT_CALLBACK*)CallBack;
    m_SCEContext = Context;
}

KIRQL
CUSBHardwareDevice::AcquireDeviceLock(void)
{
    KIRQL OldLevel;

    //
    // acquire lock
    //
    KeAcquireSpinLock(&m_Lock, &OldLevel);

    //
    // return old irql
    //
    return OldLevel;
}

VOID
CUSBHardwareDevice::GetCurrentFrameNumber(
    PULONG FrameNumber)
{
    ULONG Control;
    ULONG Number;


    Number = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_FRAME_INTERVAL_NUMBER_OFFSET));
    DPRINT1("FrameNumberInterval %x Frame %x\n", Number, m_HCCA->CurrentFrameNumber);

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

VOID
CUSBHardwareDevice::ReleaseDeviceLock(
    KIRQL OldLevel)
{
    KeReleaseSpinLock(&m_Lock, OldLevel);
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
        // new device has arrived
        //

        //
        // disable interrupt as it will fire untill the port has been reset
        //
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
OhciDefferedRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2)
{
    CUSBHardwareDevice *This;
    ULONG CStatus, Index, PortStatus;
    ULONG DoneHead;

    //
    // get parameters
    //
    This = (CUSBHardwareDevice*)DeferredContext;
    CStatus = (ULONG) SystemArgument1;
    DoneHead = (ULONG)SystemArgument2;

    DPRINT("OhciDefferedRoutine Status %x\n", CStatus);

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
                    DPRINT1("New device arrival at Port %d LowSpeed %x\n", Index, (PortStatus & OHCI_RH_PORTSTATUS_LSDA));

                    //
                    // enable port
                    //
                    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)This->m_Base + OHCI_RH_PORT_STATUS(Index)), OHCI_RH_PORTSTATUS_PES);


                    //
                    // store change
                    //
                    This->m_PortStatus[Index].PortStatus |= USB_PORT_STATUS_CONNECT;
                    This->m_PortStatus[Index].PortChange |= USB_PORT_STATUS_CONNECT;

                    if ((PortStatus & OHCI_RH_PORTSTATUS_LSDA))
                    {
                        //
                        // low speed device connected
                        //
                        This->m_PortStatus[Index].PortStatus |= USB_PORT_STATUS_LOW_SPEED;
                    }
                }
                else
                {
                    //
                    // device disconnected
                    //
                    DPRINT1("Device disconnected at Port %x\n", Index);

                    //
                    // update port status flags
                    //
                    This->m_PortStatus[Index].PortStatus &= ~USB_PORT_STATUS_LOW_SPEED;
                    This->m_PortStatus[Index].PortStatus &= ~USB_PORT_STATUS_CONNECT;
                    This->m_PortStatus[Index].PortChange |= USB_PORT_STATUS_CONNECT;
                }

                //
                // is there a status change callback
                //
                if (This->m_SCECallBack != NULL)
                {
                    //
                    // queue work item for processing
                    //
                    ExQueueWorkItem(&This->m_StatusChangeWorkItem, DelayedWorkQueue);
                }
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

}

NTSTATUS
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
