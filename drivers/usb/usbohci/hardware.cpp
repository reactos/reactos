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
EhciDefferedRoutine(
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
    // local
    BOOLEAN InterruptService();
    NTSTATUS InitializeController();
    NTSTATUS AllocateEndpointDescriptor(OUT POHCI_ENDPOINT_DESCRIPTOR *OutDescriptor);

    // friend function
    friend BOOLEAN NTAPI InterruptServiceRoutine(IN PKINTERRUPT  Interrupt, IN PVOID  ServiceContext);
    friend VOID NTAPI EhciDefferedRoutine(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2);
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
    POHCI_ENDPOINT_DESCRIPTOR  m_IsoEndpointDescriptor;                                // iso endpoint descriptor
    POHCI_ENDPOINT_DESCRIPTOR m_InterruptEndpoints[OHCI_STATIC_ENDPOINT_COUNT];        // endpoints for interrupt / iso transfers
    ULONG m_NumberOfPorts;                                                             // number of ports
    PDMAMEMORYMANAGER m_MemoryManager;                                                 // memory manager
    HD_INIT_CALLBACK* m_SCECallBack;                                                   // status change callback routine
    PVOID m_SCEContext;                                                                // status change callback routine context
    BOOLEAN m_DoorBellRingInProgress;                                                  // door bell ring in progress
    WORK_QUEUE_ITEM m_StatusChangeWorkItem;                                            // work item for status change callback
    ULONG m_SyncFramePhysAddr;                                                         // periodic frame list physical address
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
                                EhciDefferedRoutine,
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
    // Initialize the UsbQueue now that we have an AdapterObject.
    //
    Status = m_UsbQueue->Initialize(PUSBHARDWAREDEVICE(this), m_Adapter, m_MemoryManager, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to Initialize the UsbQueue\n");
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
    ULONG Control, NumberOfPorts, Index, Descriptor;

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
    // done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
CUSBHardwareDevice::StopController(void)
{
    ULONG Control, Reset;
    ULONG Index;

    //
    // first turn off all interrupts
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_INTERRUPT_DISABLE_OFFSET), OHCI_ALL_INTERRUPTS);

    //
    // check context
    //
    Control = READ_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_CONTROL_OFFSET));

    //
    // FIXME: support routing
    //
    ASSERT((Control & OHCI_INTERRUPT_ROUTING) == 0);

    //
    // have a break
    //
    KeStallExecutionProcessor(100);

    //
    // some controllers also depend on this
    //
    WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_CONTROL_OFFSET), OHCI_HC_FUNCTIONAL_STATE_RESET);

    //
    // wait a bit
    //
    KeStallExecutionProcessor(100);

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
    UNIMPLEMENTED
    *PortStatus = 0;
    *PortChange = 0;
    return STATUS_SUCCESS;
}

NTSTATUS
CUSBHardwareDevice::ClearPortStatus(
    ULONG PortId,
    ULONG Status)
{
    UNIMPLEMENTED
    return STATUS_SUCCESS;
}


NTSTATUS
CUSBHardwareDevice::SetPortFeature(
    ULONG PortId,
    ULONG Feature)
{
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
        // reset port
        //
        WRITE_REGISTER_ULONG((PULONG)((PUCHAR)m_Base + OHCI_RH_PORT_STATUS(PortId)), OHCI_RH_PORTSTATUS_PRS);

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
    ASSERT(FALSE);
    return TRUE;
}

VOID NTAPI
EhciDefferedRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2)
{
    ASSERT(FALSE);
    return;
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
