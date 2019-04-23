/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Extensible Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbxhci/hardware.cpp
 * PURPOSE:     USB XHCI device driver(based on Haiku XHCI driver and ReactOS EHCI)
 * PROGRAMMERS: lesanilie@gmail
 */
#include "usbxhci.h"

#define YDEBUG
#include <debug.h>

typedef VOID NTAPI HD_INIT_CALLBACK(IN PVOID CallBackContext);

BOOLEAN
NTAPI
InterruptServiceRoutine(
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext);

VOID
NTAPI
XhciDeferredRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2);

VOID
NTAPI
StatusChangeWorkItemRoutine(PVOID Context);

//
// implementation of the interface
//
class CUSBHardwareDevice : public IXHCIHardwareDevice
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
    IMP_IUSBHARDWAREDEVICE
    IMP_IUSBXHCIHARDWARE

    // friend function
    friend BOOLEAN NTAPI InterruptServiceRoutine(IN PKINTERRUPT  Interrupt, IN PVOID  ServiceContext);
    friend VOID NTAPI XhciDeferredRoutine(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2);
    friend  VOID NTAPI StatusChangeWorkItemRoutine(PVOID Context);
    friend BOOLEAN NTAPI IsDeviceSlotInitialized( IN ULONG PortId, IN PVOID Context);
    friend NTSTATUS NTAPI InitializeDeviceSlot(IN ULONG PortId, IN PVOID Context);
    friend NTSTATUS NTAPI DisableDeviceSlot(IN ULONG PortId, IN PVOID Context);
    friend NTSTATUS NTAPI SearchDeviceInformation(IN ULONG SearchValue, IN ULONG SearchType, OUT PDEVICE_INFORMATION *OutDeviceInformation, IN PVOID Context);

    // start/stop/reset controller
    NTSTATUS StartController(void);
    NTSTATUS StopController(void);
    NTSTATUS ResetController(void);

    // read register
    ULONG READ_CAPABILITY_REG_ULONG(ULONG Offset);
    ULONG READ_OPERATIONAL_REG_ULONG(ULONG Offset);
    ULONG READ_DOORBELL_REG_ULONG(ULONG Offset);
    ULONG READ_RUNTIME_REG_ULONG(ULONG Offset);

    // write register
    VOID WRITE_CAPABILITY_REG_ULONG(ULONG Offset, ULONG Value);
    VOID WRITE_OPERATIONAL_REG_ULONG(ULONG Offset, ULONG Value);
    VOID WRITE_DOORBELL_REG_ULONG(ULONG Offset, ULONG Value);
    VOID WRITE_RUNTIME_REG_ULONG(ULONG Offset, ULONG Value);

    CUSBHardwareDevice(IUnknown *OuterUnknown) {}
    virtual ~CUSBHardwareDevice() {}
protected:
    LONG m_Ref;
    PDRIVER_OBJECT m_DriverObject;                                                     // driver object
    PDEVICE_OBJECT m_PhysicalDeviceObject;                                             // pdo
    PDEVICE_OBJECT m_FunctionalDeviceObject;                                           // fdo (hcd controller)
    PDEVICE_OBJECT m_NextDeviceObject;                                                 // lower device object
    PDMAMEMORYMANAGER m_MemoryManager;                                                 // memory manager
    PXHCIQUEUE m_UsbQueue;                                                             // usb request queue
    KSPIN_LOCK m_Lock;                                                                 // hardware lock
    WORK_QUEUE_ITEM m_StatusChangeWorkItem;                                            // work item for status change callback
    volatile LONG m_StatusChangeWorkItemStatus;                                        // work item status
    HD_INIT_CALLBACK* m_SCECallBack;                                                   // status change callback routine
    PVOID m_SCEContext;                                                                // status change callback routine context
    USHORT m_VendorID;                                                                 // vendor id
    USHORT m_DeviceID;                                                                 // device id
    BUS_INTERFACE_STANDARD m_BusInterface;                                             // pci bus interface
    KDPC m_IntDpcObject;                                                               // dpc object for deferred isr processing
    PKINTERRUPT m_Interrupt;                                                           // interrupt object
    XHCI_CAPABILITY_REGS m_Capabilities;                                               // controller capabilities
    PULONG m_CapRegBase;                                                               // XHCI capability register
    PULONG m_OpRegBase;                                                                // XHCI operational base registers
    PULONG m_RunTmRegBase;                                                             // XHCI runtime register
    PULONG m_DoorBellRegBase;                                                          // XHCI doorbell register
    PDMA_ADAPTER m_Adapter;                                                            // dma adapter object
    ULONG m_MapRegisters;                                                              // map registers count
    PLARGE_INTEGER m_DeviceContextArray;                                               // device context array virtual address
    PHYSICAL_ADDRESS m_PhysicalDeviceContextArray;                                     // device context array physical address
    PVOID VirtualBase;                                                                 // virtual base for memory manager
    PHYSICAL_ADDRESS PhysicalAddress;                                                  // physical base for memory manager
    BOOLEAN m_PortResetInProgress[0xF];                                                // stores reset in progress
    LIST_ENTRY m_HeadDeviceList;                                                       // head of the device list
};

//
// UNKNOWN
//
NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::QueryInterface(
    IN REFIID refiid,
    OUT PVOID *OutInterface)
{
    if (IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *OutInterface = PVOID(PUNKNOWN(this));
        PUNKNOWN(*OutInterface)->AddRef();
        return STATUS_SUCCESS;
    }
    // bad IID
    return STATUS_UNSUCCESSFUL;
}

//
// IMP_IUSBHARDWAREDEVICE
//
NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::Initialize(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT FunctionalDeviceObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PDEVICE_OBJECT LowerDeviceObject)
{
    PCI_COMMON_CONFIG PciConfig;
    NTSTATUS Status;
    ULONG BytesRead;

    DPRINT("CUSBHardwareDevice::Initialize.\n");

    //
    // create DMA Memory Manager
    //
    Status = CreateDMAMemoryManager(&m_MemoryManager);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create DMA Memory Manager\n");
        return Status;
    }

    //
    // create USBQueue
    //
    Status = CreateUSBQueue((PUSBQUEUE*)&m_UsbQueue);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create");
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

    //
    // initialize device context list
    //
    InitializeListHead(&m_HeadDeviceList);

    m_VendorID = 0;
    m_DeviceID = 0;

    //
    // get bus driver interface
    //
    Status = GetBusInterface(PhysicalDeviceObject, &m_BusInterface);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get BusInterface");
        return Status;
    }

    //
    // get pci config
    //
    BytesRead = (*m_BusInterface.GetBusData)(m_BusInterface.Context,
                                             PCI_WHICHSPACE_CONFIG,
                                             &PciConfig,
                                             0,
                                             PCI_COMMON_HDR_LENGTH);
    if (BytesRead != PCI_COMMON_HDR_LENGTH)
    {
        DPRINT1("Failed to get pci config information!\n");
        return STATUS_SUCCESS;
    }

    //
    // save vendor and device ID
    //
    m_VendorID = PciConfig.VendorID;
    m_DeviceID = PciConfig.DeviceID;

    //
    // great success
    //
    return Status;
}

// read register
ULONG CUSBHardwareDevice::READ_CAPABILITY_REG_ULONG(ULONG Offset)
{
    return READ_REGISTER_ULONG((PULONG)((ULONG)m_CapRegBase + Offset));
}

ULONG CUSBHardwareDevice::READ_OPERATIONAL_REG_ULONG(ULONG Offset)
{
    return READ_REGISTER_ULONG((PULONG)((ULONG)m_OpRegBase + Offset));
}

ULONG CUSBHardwareDevice::READ_DOORBELL_REG_ULONG(ULONG Offset)
{
    return READ_REGISTER_ULONG((PULONG)((ULONG)m_DoorBellRegBase + Offset));
}

ULONG CUSBHardwareDevice::READ_RUNTIME_REG_ULONG(ULONG Offset)
{
    return READ_REGISTER_ULONG((PULONG)((ULONG)m_RunTmRegBase + Offset));
}

// write register
VOID
CUSBHardwareDevice::WRITE_CAPABILITY_REG_ULONG(ULONG Offset, ULONG Value)
{
    WRITE_REGISTER_ULONG((PULONG)((ULONG)m_CapRegBase + Offset), Value);
}

VOID
CUSBHardwareDevice::WRITE_OPERATIONAL_REG_ULONG(ULONG Offset, ULONG Value)
{
    WRITE_REGISTER_ULONG((PULONG)((ULONG)m_OpRegBase + Offset), Value);
}

VOID
CUSBHardwareDevice::WRITE_DOORBELL_REG_ULONG(ULONG Offset, ULONG Value)
{
    WRITE_REGISTER_ULONG((PULONG)((ULONG)m_DoorBellRegBase + Offset), Value);
}

VOID
CUSBHardwareDevice::WRITE_RUNTIME_REG_ULONG(ULONG Offset, ULONG Value)
{
    WRITE_REGISTER_ULONG((PULONG)((ULONG)m_RunTmRegBase + Offset), Value);
}

NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::PnpStart(
    IN PCM_RESOURCE_LIST RawResources,
    IN PCM_RESOURCE_LIST TranslatedResources)
{
    ULONG Count;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptor;
    DEVICE_DESCRIPTION DeviceDescription;
    PVOID ResourceBase;
    NTSTATUS Status;

    ASSERT(KeGetCurrentIrql() <= PASSIVE_LEVEL);

    DPRINT("CUSBHardwareDevice::PnpStart\n");
    for (Count = 0; Count < TranslatedResources->List[0].PartialResourceList.Count; Count++)
    {
        //
        // get resource descriptor
        //
        ResourceDescriptor = &TranslatedResources->List[0].PartialResourceList.PartialDescriptors[Count];

        switch (ResourceDescriptor->Type)
        {
            case CmResourceTypeInterrupt:
            {
                //
                // initialize interrupt DPC object
                //
                KeInitializeDpc(&m_IntDpcObject,
                                XhciDeferredRoutine,
                                this);

                //
                // connect ISR
                //
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
                    // failed to register ISR
                    //
                    DPRINT1("IoConnect Interrupt failed %x\n", Status);
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
                    // failed to map memory
                    //
                    DPRINT1("MmMapIoSpace failed. \n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                //
                // get capabilites
                //
                m_Capabilities.CapLength = READ_REGISTER_UCHAR((PUCHAR)ResourceBase + XHCI_CAPLENGTH);
                m_Capabilities.HciVersion = READ_REGISTER_USHORT((PUSHORT)((ULONG_PTR)ResourceBase + XHCI_HCIVERSION));
                m_Capabilities.HcsParams1Long = READ_REGISTER_ULONG((PULONG)((ULONG_PTR)ResourceBase + XHCI_HCSPARAMS1));
                m_Capabilities.HcsParams2Long = READ_REGISTER_ULONG((PULONG)((ULONG_PTR)ResourceBase + XHCI_HCSPARAMS2));
                m_Capabilities.HcsParams3Long = READ_REGISTER_ULONG((PULONG)((ULONG_PTR)ResourceBase + XHCI_HCSPARAMS3));
                m_Capabilities.HccParams1Long = READ_REGISTER_ULONG((PULONG)((ULONG_PTR)ResourceBase + XHCI_HCCPARAMS1));
                m_Capabilities.DoorBellOffset = READ_REGISTER_ULONG((PULONG)((ULONG_PTR)ResourceBase + XHCI_DBOFF));
                m_Capabilities.RunTmRegSpaceOff = READ_REGISTER_ULONG((PULONG)((ULONG_PTR)ResourceBase + XHCI_RTSOFF));
                m_Capabilities.HccParams2Long = READ_REGISTER_ULONG((PULONG)((ULONG_PTR)ResourceBase + XHCI_HCCPARAMS2));

                DPRINT1("Controller CapLength        : 0x%x\n", m_Capabilities.CapLength);
                DPRINT1("Controller HciVersion       : 0x%x\n", m_Capabilities.HciVersion);
                DPRINT1("Controller HcsParams1Long   : 0x%x\n", m_Capabilities.HcsParams1Long);
                DPRINT1("Controller HcsParams2Long   : 0x%x\n", m_Capabilities.HcsParams2Long);
                DPRINT1("Controller HcsParams3Long   : 0x%x\n", m_Capabilities.HcsParams3Long);
                DPRINT1("Controller HccParams1Long   : 0x%x\n", m_Capabilities.HccParams1Long);
                DPRINT1("Controller DoorBellOffset   : 0x%x\n", m_Capabilities.DoorBellOffset);
                DPRINT1("Controller RunTmRegSpaceOff : 0x%x\n", m_Capabilities.RunTmRegSpaceOff);
                DPRINT1("Controller HccParams2Long   : 0x%x\n", m_Capabilities.HccParams2Long);

                DPRINT1("Controller Port Count       : 0x%x\n", m_Capabilities.HcsParams1.MaxPorts);

                //
                // save XHCI registers addresses for later use
                //
                m_CapRegBase = (PULONG)ResourceBase;
                m_OpRegBase = (PULONG)((ULONG)ResourceBase + m_Capabilities.CapLength);
                m_DoorBellRegBase = (PULONG)((ULONG)ResourceBase + m_Capabilities.DoorBellOffset);
                m_RunTmRegBase = (PULONG)((ULONG)ResourceBase + m_Capabilities.RunTmRegSpaceOff);
                break;
            }
        }
    }

    //
    // zero it
    //
    RtlZeroMemory(&DeviceDescription, sizeof(DEVICE_DESCRIPTION));

    //
    // initialize device description
    //
    DeviceDescription.Version           = DEVICE_DESCRIPTION_VERSION;
    DeviceDescription.Master            = TRUE;
    DeviceDescription.ScatterGather     = TRUE;
    DeviceDescription.Dma32BitAddresses = TRUE;
    DeviceDescription.DmaWidth          = Width32Bits;
    DeviceDescription.InterfaceType     = PCIBus;
    DeviceDescription.MaximumLength     = MAXULONG;

    //
    // get dma adapter
    //
    m_Adapter = IoGetDmaAdapter(m_PhysicalDeviceObject, &DeviceDescription, &m_MapRegisters);
    if (!m_Adapter)
    {
        DPRINT("Failed to acquire dma adapter\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // create common buffer
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
    // initialize the DMAMemoryManager
    //
    Status = m_MemoryManager->Initialize(this, &m_Lock, PAGE_SIZE * 4, VirtualBase, PhysicalAddress, 32);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to initialize DMAMemoryManager\n");
        return Status;
    }

    //
    // start the controller
    //
    DPRINT1("Start Controller\n");
    Status = StartController();

    return Status;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::PnpStop(VOID)
{
    UNIMPLEMENTED_DBGBREAK();
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
        *VendorId = m_VendorID;
    if (DeviceId)
        *DeviceId = m_DeviceID;
    if (NumberOfPorts)
        *NumberOfPorts = m_Capabilities.HcsParams1.MaxPorts;
    if (Speed)
        *Speed = 0x300; // of course ;)
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
CUSBHardwareDevice::StartController(VOID)
{
    NTSTATUS Status;
    ULONG ExtendedCapPointer, CapabilityRegister;
    LARGE_INTEGER Timeout;
    ULONG Index = 0;
    ULONG UsbStatus, Count;
    PHYSICAL_ADDRESS  PhysicalBaseAddress;
    PLARGE_INTEGER VirtualBaseAddress;
    ULONG Register;
    ULONG Size;

    //
    // get extended capabilities pointer
    //
    ExtendedCapPointer = m_Capabilities.HccParams1.xECP << 2;

    do
    {
        //
        // no extended capabilites list?(must rework this)
        //
        if (!ExtendedCapPointer)
            break;

        CapabilityRegister = READ_CAPABILITY_REG_ULONG(ExtendedCapPointer);

        //
        // looking for USB Legacy Support
        //
        if ((CapabilityRegister & XHCI_ECP_MASK) == XHCI_LEGSUP_CAPID)
        {
            if (CapabilityRegister & XHCI_LEGSUP_BIOSOWNED)
            {
                DPRINT1("The host controller is BIOS owned.\n");
                WRITE_CAPABILITY_REG_ULONG(ExtendedCapPointer, CapabilityRegister | XHCI_LEGSUP_OSOWNED);

                Timeout.QuadPart = 50;
                DPRINT1("Waiting %lu miliseconds timeout.\n", Timeout.LowPart);

                Index = 0;
                do
                {
                    CapabilityRegister = READ_CAPABILITY_REG_ULONG(ExtendedCapPointer);
                    if (!(CapabilityRegister & XHCI_LEGSUP_BIOSOWNED))
                    {
                        //
                        // BIOS released XHCI
                        //
                        break;
                    }

                    //
                    // convert to 100 ms units
                    //
                    Timeout.QuadPart *= -10000;

                    //
                    // perform the wait
                    //
                    KeDelayExecutionThread(KernelMode, FALSE, &Timeout);

                    Index++;
                } while (Index < 20);
            }

            if (CapabilityRegister & XHCI_LEGSUP_BIOSOWNED)
            {
                DPRINT1("Controller is still BIOS owned.\n");
            }
            else if (CapabilityRegister & XHCI_LEGSUP_OSOWNED)
            {
                DPRINT1("BIOS released controller onwnership.\n");
            }
            break;
        }

        //
        // go to Next xHCI Extended Capability Pointer
        //
        ExtendedCapPointer += (((CapabilityRegister >> XHCI_NEXT_CAP_SHIFT) & XHCI_ECP_MASK) << 2);

    } while (CapabilityRegister & XHCI_NEXT_CAP_MASK);

    //
    // check if controller is not halted
    //
    UsbStatus = READ_OPERATIONAL_REG_ULONG(XHCI_USBSTS);
    if (!(UsbStatus & XHCI_STS_HCH))
    {
        DPRINT1("Stopping Controller. \n");
        StopController();
    }

    ResetController();

    //
    // TODO: get ports speed from extended capability
    //

    //
    // activate device slots
    //
    ASSERT(m_Capabilities.HcsParams1.MaxSlots);
    WRITE_OPERATIONAL_REG_ULONG(XHCI_CONFIG, m_Capabilities.HcsParams1.MaxSlots);

    //
    // disable device notifications
    //
    WRITE_OPERATIONAL_REG_ULONG(XHCI_DNCTRL, 0);

    //
    // allocate memory for device context base address array + scratchpad pointer
    //
    Status = m_MemoryManager->Allocate((m_Capabilities.HcsParams1.MaxSlots + 1) * sizeof(PHYSICAL_ADDRESS), (PVOID*)&m_DeviceContextArray, &m_PhysicalDeviceContextArray);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to allocate memory for DCBA.\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // allocate memory for scratchpad buffers(for xHCI internal use)
    //
    Count = (m_Capabilities.HcsParams2.MaxScratchPadBufsHi << 5) | m_Capabilities.HcsParams2.MaxScratchPadBufsLo;

    //
    // calculate the size of the scratchpad buffers array
    //
    Size = Count ? sizeof(PHYSICAL_ADDRESS) * Count : sizeof(PHYSICAL_ADDRESS);

    //
    // allocate memory for scratchpad buffers array
    //
    Status = m_MemoryManager->Allocate(Size, (PVOID*)&VirtualBaseAddress, &PhysicalBaseAddress);
    if (!NT_SUCCESS(Status))
    {
        //
        // TODO: free prev allocated resources
        //
        DPRINT1("Failed to allocate memory for scrachpad array.\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // get XHCI page size
    //
    Size = READ_OPERATIONAL_REG_ULONG(XHCI_PAGE_SIZE);
    Index = 0;
    while (!(Size & 0x1))
    {
        Index = Index + 1;
        Size = Size >> 1;
    }

    //
    // compute page size(default page size is 4096)
    //
    Size = XHCI_GET_PAGE_SIZE(Index);

    //
    // initialize scratchpad buffers array
    //
    for (Index = 0; Index < Count; Index++)
    {
        Status = m_MemoryManager->Allocate(Size, (PVOID*)&VirtualBase, &PhysicalAddress);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to allocate memory for scratchpad buffers.\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // setup element
        //
        VirtualBaseAddress[Index].QuadPart = PhysicalAddress.QuadPart;
    }

    //
    // first element in device context array must be a pointer to scratchpad buffers array
    //
    m_DeviceContextArray[0].QuadPart = PhysicalBaseAddress.QuadPart;

    //
    // set Device Context Base Address Array Pointer
    //
    WRITE_OPERATIONAL_REG_ULONG(XHCI_DCBAAP_LOW, m_PhysicalDeviceContextArray.LowPart);
    WRITE_OPERATIONAL_REG_ULONG(XHCI_DCBAAP_HIGH, m_PhysicalDeviceContextArray.HighPart);

    //
    // initialize UsbQueue
    //
    Status = m_UsbQueue->Initialize(PUSBHARDWAREDEVICE(this), m_Adapter, m_MemoryManager, &m_Lock);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to initialize UsbQueue\n");
        return Status;
    }

    DPRINT1("Enable interrupts and start the controller.\n");
    Register = READ_RUNTIME_REG_ULONG(XHCI_IMAN_BASE);
    WRITE_RUNTIME_REG_ULONG(XHCI_IMAN_BASE, Register | XHCI_IMAN_INTR_ENA);

    //
    // start controller
    //
    Register = READ_OPERATIONAL_REG_ULONG(XHCI_USBCMD);
    WRITE_OPERATIONAL_REG_ULONG(XHCI_USBCMD, Register | XHCI_CMD_RUN | XHCI_CMD_EIE | XHCI_CMD_HSEE);

    //
    // wait for controller to start
    //
    Index = 0;
    do
    {
        KeStallExecutionProcessor(10);

        UsbStatus = READ_OPERATIONAL_REG_ULONG(XHCI_USBSTS);

        Index++;
    } while (Index < 100 && (UsbStatus & XHCI_STS_HCH));

    if (UsbStatus & XHCI_STS_HCH)
    {
        DPRINT1("Controller is not responding to start request\n");
        return STATUS_UNSUCCESSFUL;
    }

    //
    // great success
    //
    return STATUS_SUCCESS;
}

NTSTATUS
CUSBHardwareDevice::StopController(void)
{
    ULONG UsbStatus;
    ULONG Index = 0;

    //
    // halt the controller
    //
    WRITE_OPERATIONAL_REG_ULONG(XHCI_USBCMD, 0);

    //
    // wait for controller to halt
    //
    do
    {
        //
        // stall the processor for 10 microseconds
        //
        KeStallExecutionProcessor(10);

        UsbStatus = READ_OPERATIONAL_REG_ULONG(XHCI_USBSTS);

        Index++;
    } while (Index < 100 && !(UsbStatus & XHCI_STS_HCH));

    if (!(UsbStatus & XHCI_STS_HCH))
    {
        DPRINT1("Controller is not responding to stop request.\n");
        return STATUS_UNSUCCESSFUL;
    }

    //
    // great success
    //
    return STATUS_SUCCESS;
}

NTSTATUS
CUSBHardwareDevice::ResetController(void)
{
    ULONG UsbStatus, UsdCommand;
    ULONG Index = 0;

    //
    // reset the controller
    //
    UsdCommand = READ_OPERATIONAL_REG_ULONG(XHCI_USBCMD);
    WRITE_OPERATIONAL_REG_ULONG(XHCI_USBCMD, UsdCommand | XHCI_CMD_HCRST);

    //
    // wait for controller to halt
    //
    do
    {
        //
        // stall the processor for 10 microseconds
        //
        KeStallExecutionProcessor(10);

        UsbStatus = READ_OPERATIONAL_REG_ULONG(XHCI_USBSTS);

        Index++;
    } while (Index < 100 && !(UsbStatus & XHCI_STS_HCH));

    if (!(UsbStatus & XHCI_STS_HCH))
    {
        DPRINT1("Controller is not responding to reset request.\n");
        return STATUS_UNSUCCESSFUL;
    }

    //
    // wait till controller not ready is cleared
    //
    Index = 0;
    do
    {
        //
        // stall the processor for 10 microseconds
        //
        KeStallExecutionProcessor(10);

        UsbStatus = READ_OPERATIONAL_REG_ULONG(XHCI_USBSTS);

        Index++;
    } while (Index < 100 && (UsbStatus & XHCI_STS_CNR));

    if (UsbStatus & XHCI_STS_CNR)
    {
        //
        // failed
        //
        DPRINT1("Controller is not ready.\n");
        return STATUS_UNSUCCESSFUL;
    }

    //
    // great success
    //
    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::ResetPort(
    IN ULONG PortId)
{
    ULONG Status;
    LARGE_INTEGER Timeout;

    DPRINT("CUSBHardwareDevice::ResetPort\n");

    if (PortId > m_Capabilities.HcsParams1.MaxPorts)
        return STATUS_UNSUCCESSFUL;

    //
    // get port status
    //
    Status = READ_OPERATIONAL_REG_ULONG(XHCI_PORTSC(PortId));

    //
    // reset the port
    //
    Status |= XHCI_PORT_RESET;
    WRITE_OPERATIONAL_REG_ULONG(XHCI_PORTSC(PortId), Status);

    Timeout.QuadPart = 50;
    DPRINT1("Waiting %lu miliseconds for port reset\n", Timeout.LowPart);

    //
    // convert to 100 ns units
    //
    Timeout.QuadPart *= -10000;

    //
    // put the current thread in a wait state
    //
    KeDelayExecutionThread(KernelMode, FALSE, &Timeout);

    //
    // great success
    //
    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::GetPortStatus(
    IN ULONG PortId,
    OUT USHORT *PortStatus,
    OUT USHORT *PortChange)
{
    ULONG Value, Speed;
    USHORT Status = 0, Change = 0;

    DPRINT("CUSBHardwareDevice::GetPortStatus\n");

    //
    // sanity check
    //
    if (PortId > m_Capabilities.HcsParams1.MaxPorts)
        return STATUS_UNSUCCESSFUL;

    //
    // get port status
    //
    Value = READ_OPERATIONAL_REG_ULONG(XHCI_PORTSC(PortId));

    //
    // check if host controller have port power control
    //
    if (m_Capabilities.HccParams1.PPC)
    {
        //
        // get port power control state
        //
        if (Value & XHCI_PORT_POWER)
        {
            Status |= USB_PORT_STATUS_POWER;
        }
    }
    else // no port power control
    {
        Status |= USB_PORT_STATUS_POWER;
    }

    if (Value & XHCI_PORT_CONNECT_STATUS)
    {
        Status |= USB_PORT_STATUS_CONNECT;

        //
        // get port speed
        //
        Speed = XHCI_PORT_GET_SPEED(Value);
        if (Speed == XHCI_PORT_LOW_SPEED)
        {
            Status |= USB_PORT_STATUS_LOW_SPEED;
        }
        else if (Speed == XHCI_PORT_HIGH_SPEED)
        {
            Status |= USB_PORT_STATUS_HIGH_SPEED;
        }
        else if (Speed == XHCI_PORT_SUPER_SPEED)
        {
            //Status |= USB_PORT_STATUS_SUPER_SPEED;
        }
    }

    //
    // get Enabled Status
    //
    if (Value & XHCI_PORT_ENABLED)
        Status |= USB_PORT_STATUS_ENABLE;

    //
    // is port overcurrent active?
    //
    if (Value & XHCI_PORT_OVER_CURRENT_ACTIVE)
        Status |= USB_PORT_STATUS_OVER_CURRENT;

    //
    // in a reset state?
    //
    if ((Value & XHCI_PORT_RESET) || m_PortResetInProgress[PortId])
    {
        Status |= USB_PORT_STATUS_RESET;
        Change |= USB_PORT_STATUS_RESET;
    }

    //
    // connect or disconnect?
    //
    if (Value & XHCI_PORT_CONNECT_STATUS_CHANGE)
    {
        Change |= USB_PORT_STATUS_CONNECT;
    }

    //
    // port transition
    //
    if ((Value & XHCI_PORT_ENABLED_CHANGE))
        Change |= USB_PORT_STATUS_ENABLE;

    *PortStatus = Status;
    *PortChange = Change;

    //
    // great success
    //
    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::ClearPortStatus(
    IN ULONG PortId,
    IN ULONG Status)
{
    ULONG PortStatus;
    LARGE_INTEGER Timeout;

    DPRINT("CUSBHardwareDevice::ClearPortStatus\n");

    if (PortId > m_Capabilities.HcsParams1.MaxPorts)
        return STATUS_UNSUCCESSFUL;

    //
    // Get port status
    //
    PortStatus = READ_OPERATIONAL_REG_ULONG(XHCI_PORTSC(PortId));

    if (Status == C_PORT_RESET)
    {
        //
        // Section 5.4.8, Table 39, bit 4: PR remains set until reset signaling is completed by the root hub.
        //
        m_PortResetInProgress[PortId] = FALSE;

        //
        // when port reset sequence is initiated, XHCI generate a port reset event and set PR bit to 0
        // section: 4.19.1.1 USB2 Root Hub Port Port State Machine
        //
        do
        {
            //
            // get port status
            //
            PortStatus = READ_OPERATIONAL_REG_ULONG(XHCI_PORTSC(PortId));

            //
            // wait till PR bit is cleared
            //
            KeStallExecutionProcessor(20);
        } while (PortStatus & XHCI_PORT_RESET);
    }
    else if (Status == C_PORT_CONNECTION)
    {
        //
        // clear CSC in order to recognize feature CSCs
        //
        WRITE_OPERATIONAL_REG_ULONG(XHCI_PORTSC(PortId), PortStatus | XHCI_PORT_CONNECT_STATUS_CHANGE);

        if (PortStatus & XHCI_PORT_CONNECT_STATUS)
        {
            //
            // delay is 50 ms
            //
            Timeout.QuadPart = 50;

            //
            // convert to 100ns units
            //
            Timeout.QuadPart *= -10000;

            //
            // put the current thread in a wait state
            //
            KeDelayExecutionThread(KernelMode, FALSE, &Timeout);
        }
    }
    else if (Status == C_PORT_ENABLE)
    {
        //
        // clear Port Enabled Change bit
        //
        WRITE_OPERATIONAL_REG_ULONG(XHCI_PORTSC(PortId), PortStatus | XHCI_PORT_ENABLED_CHANGE);
    }

    //
    // great success
    //
    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::SetPortFeature(
    IN ULONG PortId,
    IN ULONG Feature)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG PortStatus;
    LARGE_INTEGER Timeout;

    DPRINT("CUSBHardwareDevice::SetPortFeature\n.");

    if (PortId > m_Capabilities.HcsParams1.MaxPorts)
        return STATUS_INVALID_PARAMETER;

    //
    // get port status
    //
    PortStatus = READ_OPERATIONAL_REG_ULONG(XHCI_PORTSC(PortId));

    if (Feature == PORT_ENABLE)
    {
        DPRINT("PORT_ENABLE called\n");
        ASSERT(PortStatus & XHCI_PORT_ENABLED);

        //
        // TODO: initialize slot
        //
    }
    else if (Feature == PORT_SUSPEND)
    {
        DPRINT("PORT_SUSPEND called.\n");
        //
        // TODO: check device slot state
        //
    }
    else if (Feature == PORT_RESET)
    {
        DPRINT("PORT_RESET called.\n");
        ResetPort(PortId);

        //
        // Section 5.4.8, Table 39, bit 4: PR remains set until reset signaling is completed by the root hub.
        //
        m_PortResetInProgress[PortId] = TRUE;

        //
        // callback registered?
        //
        if (m_SCECallBack != NULL)
        {
            //
            // call it
            //
            m_SCECallBack(m_SCEContext);
        }
    }
    else if (Feature == PORT_POWER)
    {
        //
        // power on
        //
        WRITE_OPERATIONAL_REG_ULONG(XHCI_PORTSC(PortId), PortStatus | XHCI_PORT_POWER);

        //
        // delay is 20 ms
        //
        Timeout.QuadPart = 20;

        //
        // convert to 100ns units
        //
        Timeout.QuadPart *= -10000;

        //
        // put the current thread in a wait state
        //
        KeDelayExecutionThread(KernelMode, FALSE, &Timeout);
    }

    //
    // great success
    //
    return Status;
}

VOID
STDMETHODCALLTYPE
CUSBHardwareDevice::SetStatusChangeEndpointCallBack(
    IN PVOID CallBack,
    IN PVOID Context)
{
    m_SCECallBack = (HD_INIT_CALLBACK*)CallBack;
    m_SCEContext = Context;
    return;
}

LPCSTR
STDMETHODCALLTYPE
CUSBHardwareDevice::GetUSBType(VOID)
{
    return "USBXHCI";
}

//
// IMP_IUSBXHCIHARDWARE
//
VOID
STDMETHODCALLTYPE
CUSBHardwareDevice::SetRuntimeRegister(
    IN ULONG Offset,
    IN ULONG Value)
{
    WRITE_RUNTIME_REG_ULONG(Offset, Value);
}

ULONG
STDMETHODCALLTYPE
CUSBHardwareDevice::GetRuntimeRegister(IN ULONG Offset)
{
    return READ_RUNTIME_REG_ULONG(Offset);
}

VOID
STDMETHODCALLTYPE
CUSBHardwareDevice::SetOperationalRegister(
    IN ULONG Offset,
    IN ULONG Value)
{
    WRITE_OPERATIONAL_REG_ULONG(Offset, Value);
}

NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::GetDeviceInformationByAddress(
    IN ULONG DeviceAddress,
    OUT PDEVICE_INFORMATION *OutDeviceInformation)
{
    PDEVICE_INFORMATION DeviceInformation = NULL;
    NTSTATUS Status;

    //
    // search device by address
    //
    Status = SearchDeviceInformation(DeviceAddress, XHCI_SEARCH_DEVICE_BY_ADDRESS, &DeviceInformation, this);
    if (!NT_SUCCESS(Status))
    {
        //
        // not found
        //
        return Status;
    }

    //
    // set out pointer
    //
    *OutDeviceInformation = DeviceInformation;

    //
    // found
    //
    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::GetDeviceInformationBySlotId(
    IN ULONG SlotId,
    OUT PDEVICE_INFORMATION *OutDeviceInformation)
{
    PDEVICE_INFORMATION DeviceInformation = NULL;
    NTSTATUS Status;

    //
    // search device by ID
    //
    Status = SearchDeviceInformation(SlotId, XHCI_SEARCH_DEVICE_BY_SLOT_ID, &DeviceInformation, this);
    if (!NT_SUCCESS(Status))
    {
        //
        // not found
        //
        return Status;
    }

    //
    // set out pointer
    //
    *OutDeviceInformation = DeviceInformation;

    //
    // found
    //
    return STATUS_SUCCESS;
}

VOID
STDMETHODCALLTYPE
CUSBHardwareDevice::RingDoorbellRegister(
    IN ULONG SlotId,
    IN ULONG Endpoint,
    IN ULONG StreamId)
{
    WRITE_DOORBELL_REG_ULONG(XHCI_DOORBELL(SlotId), XHCI_DOORBELL_TARGET(Endpoint) | XHCI_DOORBELL_STREAMID(StreamId));
}

BOOLEAN
NTAPI
InterruptServiceRoutine(
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext)
{
    ULONG UsbStatus, InterruptStatus;
    CUSBHardwareDevice *This;

    This = (CUSBHardwareDevice*)ServiceContext;
    UsbStatus = This->READ_OPERATIONAL_REG_ULONG(XHCI_USBSTS);

    //
    // check if the interrupt belong to XHCI
    //
    if (!(UsbStatus & XHCI_STS_EINT))
    {
        DPRINT1("Interrupt don't belong to XHCI.\n");
        return FALSE;
    }

    //
    // clear EINT before Interrupt Pending flag
    //
    This->WRITE_OPERATIONAL_REG_ULONG(XHCI_USBSTS, UsbStatus);

    if (UsbStatus & XHCI_STS_HCH)
    {
        DPRINT("Host Controller Halted.\n");
        return TRUE;
    }

    if (UsbStatus & XHCI_STS_HSE)
    {
        DPRINT("Host System Error.\n");
        return TRUE;
    }

    if (UsbStatus & XHCI_STS_HCE)
    {
        DPRINT("Host controller Error.\n");
        return TRUE;
    }

    //
    // clear interrupt pending bit(RW1C)
    //
    InterruptStatus = This->READ_RUNTIME_REG_ULONG(XHCI_IMAN_BASE);
    This->WRITE_RUNTIME_REG_ULONG(XHCI_IMAN_BASE, InterruptStatus);

    DPRINT("Event interrupt.\n");
    KeInsertQueueDpc(&This->m_IntDpcObject, This, (PVOID)UsbStatus);

    //
    // great success
    //
    return TRUE;
}

VOID
NTAPI
XhciDeferredRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2)
{
    ULONG Type;
    ULONG QueueSCEWorkItem = FALSE;
    PTRB TransferRequestBlock = NULL;
    CUSBHardwareDevice *This;
    ULONG PortId;
    NTSTATUS Status;
    USHORT PortChange, PortStatus;

    This = (CUSBHardwareDevice*)SystemArgument1;

    //
    // using while loop in a DPC is not that bad as it looks
    //
    while (TRUE)
    {
        //
        // get first event from ringbuffer
        //
        TransferRequestBlock = This->m_UsbQueue->GetEventRingDequeuePointer();
        if (!TransferRequestBlock)
        {
            //
            // no more TRBs
            //
            break;
        }

        //
        // get TRB type
        //
        Type = XHCI_TRB_GET_TYPE(TransferRequestBlock->Field[3]);
        if (Type == XHCI_TRB_TYPE_COMMAND_COMPLETION)
        {
            DPRINT("Command completion.\n");
            This->m_UsbQueue->CompleteCommandRequest(TransferRequestBlock);
        }
        else if (Type == XHCI_TRB_TYPE_TRANSFER)
        {
            DPRINT("Complete transfer.\n");
            //
            //FIXME: transfer request vs control request
            //
            This->m_UsbQueue->CompleteTransferRequest(TransferRequestBlock);
        }
        else if (Type == XHCI_TRB_TYPE_PORT_STATUS_CHANGE)
        {
            DPRINT("Port Status Change Event\n");

            //
            // queue SCE
            //
            QueueSCEWorkItem = TRUE;
        }
    }

    //
    // port status change?
    //
    for (PortId = 1; QueueSCEWorkItem && (PortId < This->m_Capabilities.HcsParams1.MaxPorts); PortId++)
    {
        //
        // get port status
        //
        Status = This->GetPortStatus(PortId, &PortStatus, &PortChange);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("Failed to get status on port %x\n", PortId);
            continue;
        }
        if (PortStatus & (USB_PORT_STATUS_CONNECT | USB_PORT_STATUS_ENABLE))
        {
            //
            // check if slot is initialized
            //
            if (IsDeviceSlotInitialized(PortId, This))
            {
                continue;
            }

            //
            // enable slot associated with the port
            //
            Status = InitializeDeviceSlot(PortId, This);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("Failed to initialize device slot on port %x\n", PortId);
            }
        }
        else if (!(PortStatus & USB_PORT_STATUS_CONNECT))
        {
            //
            // check if slot is initialized
            //
            if (!IsDeviceSlotInitialized(PortId, This))
            {
                continue;
            }

            //
            // disable slot associated with the port(4.6.4 Disable Slot)
            //
            Status = DisableDeviceSlot(PortId, This);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("Failed to disable device slot on port %x\n", PortId);
            }
        }
    }

    //
    // queue work item
    //
    if (QueueSCEWorkItem && This->m_SCECallBack != NULL)
    {
        if (InterlockedCompareExchange(&This->m_StatusChangeWorkItemStatus, 1, 0) == 0)
        {
            ExQueueWorkItem(&This->m_StatusChangeWorkItem, DelayedWorkQueue);
        }
    }
}

BOOLEAN
NTAPI
IsDeviceSlotInitialized(
    IN ULONG PortId,
    IN PVOID Context)
{
    PDEVICE_INFORMATION DeviceInformation = NULL;
    NTSTATUS Status;

    //
    // search device by port id
    //
    Status = SearchDeviceInformation(PortId, XHCI_SEARCH_DEVICE_BY_PORT_ID, &DeviceInformation, Context);
    if (!NT_SUCCESS(Status))
    {
        //
        // not found
        //
        return FALSE;
    }
    else
    {
        //
        // found
        //
        return TRUE;
    }
}

NTSTATUS
NTAPI
InitializeDeviceSlot(IN ULONG PortId, IN PVOID Context)
{
    PLIST_ENTRY Entry;
    PDEVICE_INFORMATION DeviceInformation = NULL;
    KIRQL               OldIrql;
    TRB                 CompletedTrb;
    COMMAND_INFORMATION CommandInformation;
    NTSTATUS            Status;
    ULONG               SlotId, PortStatus;
    CUSBHardwareDevice  *This = (CUSBHardwareDevice*)Context;

    //
    // enable slot command
    //
    CommandInformation.CommandType = XHCI_TRB_TYPE_ENABLE_SLOT;

    //
    // send command
    //
    Status = This->m_UsbQueue->SubmitInternalCommand(This->m_MemoryManager, &CommandInformation, &CompletedTrb);
    if (!NT_SUCCESS(Status))
    {
        //
        // no more slots?
        //
        DPRINT1("Failed to enable slot with status %x\n", Status);
        return Status;
    }

    //
    // save slot id for later use
    //
    SlotId = XHCI_TRB_GET_SLOT(CompletedTrb.Field[3]);

    //
    // allocate memory for new device
    //
    DeviceInformation = (PDEVICE_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, sizeof(DEVICE_INFORMATION), TAG_USBXHCI);
    if (!DeviceInformation)
    {
        //
        // allocation failed
        //
        DPRINT1("Memory allocation failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // clear it
    //
    RtlZeroMemory(DeviceInformation, sizeof(DEVICE_INFORMATION));

    //
    // allocate memory for device input context
    //
    Status = This->m_MemoryManager->Allocate(sizeof(INPUT_DEVICE_CONTEXT),
                                             (PVOID*)&DeviceInformation->InputContextAddress,
                                             &DeviceInformation->PhysicalInputContextAddress);
    if (!NT_SUCCESS(Status))
    {
        //
        // allocation failed
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // nothing to drop
    //
    DeviceInformation->InputContextAddress->InputControl.DropFlags = 0;

    //
    // configure slot context and endpoint 0
    //
    DeviceInformation->InputContextAddress->InputControl.AddFlags = 3;

    //
    // get port status
    //
    PortStatus = This->READ_OPERATIONAL_REG_ULONG(XHCI_PORTSC(PortId));

    //
    // set device speed
    //
    DeviceInformation->InputContextAddress->SlotContext.DeviceInfo |= XHCI_SLOT_SPEED(XHCI_PORT_GET_SPEED(PortStatus));

    //
    // TODO: Set route string and number of entries(oly one for default endpoint)
    //
    DeviceInformation->InputContextAddress->SlotContext.DeviceInfo |= XHCI_SLOT_NUM_ENTRIES(1) | XHCI_SLOT_ROUTE(PortId);

    //
    // FIXME: set root hub port(default port ID == 1)
    //
    DeviceInformation->InputContextAddress->SlotContext.DeviceInfo2 = XHCI_SLOT_RH_PORT(1);

    //
    // set interrupter traget
    //
    DeviceInformation->InputContextAddress->SlotContext.TTInfo = XHCI_SLOT_IRQ_TARGET(0);

    //
    // set initial address and slot state
    //
    DeviceInformation->InputContextAddress->SlotContext.DeviceState = XHCI_SLOT_SLOT_STATE(0) | XHCI_SLOT_DEVICE_ADDRESS(0);

    //
    // allocate memory for device context
    //
    Status = This->m_MemoryManager->Allocate(sizeof(DEVICE_CONTEXT),
                                             (PVOID*)&DeviceInformation->DeviceContextAddress,
                                             &DeviceInformation->PhysicalDeviceContextAddress);
    if (!NT_SUCCESS(Status))
    {
        //
        // allocation failed
        //
        DPRINT("Failed to allocate memory for device context\n");
        goto Cleanup;
    }

    //
    // setup the entry in DCBAA
    //
    This->m_DeviceContextArray[SlotId] = DeviceInformation->PhysicalDeviceContextAddress;

    //
    // allocate memory for default endpoint TRBs
    //
    Status = This->m_MemoryManager->Allocate(sizeof(TRB) * XHCI_MAX_TRANSFERS,
                                             (PVOID*)&DeviceInformation->Endpoints[0].VirtualRingbufferAddress,
                                             &DeviceInformation->Endpoints[0].PhysicalRingbufferAddress);
    if (!NT_SUCCESS(Status))
    {
        //
        // allocation failed
        //
        DPRINT("Failed to allocate memory for default endpoint ringbuffer\n");
        goto Cleanup;
    }

    //
    // set up the enqueue and dequeue pointer
    //
    DeviceInformation->Endpoints[0].DequeuePointer = DeviceInformation->Endpoints[0].EnqueuePointer = (PTRB)DeviceInformation->Endpoints[0].VirtualRingbufferAddress;

    //
    // initialize linked list of the descriptors
    //
    InitializeListHead(&DeviceInformation->Endpoints[0].DescriptorListHead);

    //
    // TODO: 1. get device speed
    //       2. configure endpoint
    //

    //
    // put device in 'default' state
    //
    CommandInformation.CommandType     = XHCI_TRB_TYPE_ADDRESS_DEVICE;
    CommandInformation.InputContext    = DeviceInformation->PhysicalInputContextAddress;
    CommandInformation.BlockSetRequest = TRUE;
    CommandInformation.SlotId          = SlotId;

    //
    // send command
    //
    Status = This->m_UsbQueue->SubmitInternalCommand(This->m_MemoryManager, &CommandInformation, &CompletedTrb);
    if (!NT_SUCCESS(Status))
    {
        //
        // command failed
        //
        DPRINT("Failed to address device with status %x\n", Status);
        goto Cleanup;
    }

    //
    // set information about the device context
    //
    DeviceInformation->State                      = XHCI_DEVICE_SLOT_STATE_DEFAULT;
    DeviceInformation->SlotId                     = SlotId;
    DeviceInformation->PortId                     = PortId;
    DeviceInformation->Endpoints[0].EndpointState = XHCI_ENDPOINT_STATE_RUNNING;
    DeviceInformation->Address                    = DeviceInformation->DeviceContextAddress->SlotContext.DeviceState & 0xFF;

    //
    // acquire lock
    //
    KeAcquireSpinLock(&This->m_Lock, &OldIrql);

    //
    // insert it in the list
    //
    InsertTailList(&This->m_HeadDeviceList, &DeviceInformation->DeviceListEntry);

    //
    // release lock
    //
    KeReleaseSpinLock(&This->m_Lock, OldIrql);

    //
    // great success
    //
    return STATUS_SUCCESS;

Cleanup:

    //
    // free slot
    //
    CommandInformation.CommandType = XHCI_TRB_TYPE_DISABLE_SLOT;

    //
    // send command
    //
    This->m_UsbQueue->SubmitInternalCommand(This->m_MemoryManager, &CommandInformation, &CompletedTrb);

    //
    // free input context
    //
    if (DeviceInformation->InputContextAddress)
    {
        This->m_MemoryManager->Release(DeviceInformation->InputContextAddress, sizeof(INPUT_DEVICE_CONTEXT));
    }

    //
    // free device context
    //
    if (DeviceInformation->DeviceContextAddress)
    {
        This->m_MemoryManager->Release(DeviceInformation->DeviceContextAddress, sizeof(DEVICE_CONTEXT));
    }

    //
    // free ringbuffers
    //
    if (DeviceInformation->Endpoints[0].VirtualRingbufferAddress)
    {
        This->m_MemoryManager->Release(DeviceInformation->Endpoints[0].VirtualRingbufferAddress, sizeof(TRB) * XHCI_MAX_TRANSFERS);
    }

    //
    // free device information
    //
    ExFreePoolWithTag(DeviceInformation, TAG_USBXHCI);

    //
    // failed
    //
    return Status;
}

NTSTATUS
NTAPI
SearchDeviceInformation(
    IN ULONG SearchValue,
    IN ULONG SearchType,
    OUT PDEVICE_INFORMATION *OutDeviceInformation,
    IN PVOID Context)
{
    PLIST_ENTRY Entry;
    PDEVICE_INFORMATION DeviceInformation = NULL;
    KIRQL OldIrql;
    CUSBHardwareDevice *This = (CUSBHardwareDevice*)Context;

    //
    // acquire lock
    //
    KeAcquireSpinLock(&This->m_Lock, &OldIrql);

    //
    // search for device address
    //
    Entry = This->m_HeadDeviceList.Flink;
    while (Entry != &This->m_HeadDeviceList)
    {
        //
        // get the device information
        //
        DeviceInformation = CONTAINING_RECORD(Entry, DEVICE_INFORMATION, DeviceListEntry);

        //
        // is this the device we are looking for
        //
        if (((SearchType == XHCI_SEARCH_DEVICE_BY_SLOT_ID) && (DeviceInformation->SlotId == SearchValue)) ||
            ((SearchType == XHCI_SEARCH_DEVICE_BY_PORT_ID) && (DeviceInformation->PortId == SearchValue)) ||
            ((SearchType == XHCI_SEARCH_DEVICE_BY_ADDRESS) && (DeviceInformation->Address == SearchValue)) )
        {
            ASSERT(DeviceInformation->State >= XHCI_DEVICE_SLOT_STATE_DEFAULT);

            //
            // set out device info
            //
            *OutDeviceInformation = DeviceInformation;

            //
            // release lock
            //
            KeReleaseSpinLock(&This->m_Lock, OldIrql);

            //
            // done
            //
            return STATUS_SUCCESS;
        }

        //
        // next entry
        //
        Entry = Entry->Flink;
    }

    //
    // release lock
    //
    KeReleaseSpinLock(&This->m_Lock, OldIrql);

    return STATUS_NO_SUCH_DEVICE;
}

NTSTATUS
NTAPI
DisableDeviceSlot(IN ULONG PortId, IN PVOID Context)
{
    //PLIST_ENTRY Entry;
    //PDEVICE_INFORMATION DeviceInformation = NULL;
    //KIRQL OldIrql;
    //CUSBHardwareDevice *This = (CUSBHardwareDevice*)Context;

    UNREFERENCED_PARAMETER(PortId);
    UNREFERENCED_PARAMETER(Context);

    UNIMPLEMENTED_DBGBREAK();

    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
StatusChangeWorkItemRoutine(PVOID Context)
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

//
// IMP_IUSBXHCIHARDWARE
//

NTSTATUS
NTAPI
CreateUSBHardware(PUSBHARDWAREDEVICE *OutHardware)
{
    PUSBHARDWAREDEVICE This;

    This = new(NonPagedPool, TAG_USBXHCI) CUSBHardwareDevice(0);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->AddRef();
    *OutHardware = (PUSBHARDWAREDEVICE)This;

    return STATUS_SUCCESS;
}
