/*
 * PROJECT:     ReactOS Universal Serial Bus Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbuhci/hcd_controller.cpp
 * PURPOSE:     USB UHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbuhci.h"

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
UhciDeferredRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2);

VOID
NTAPI
TimerDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2);


VOID
NTAPI
StatusChangeWorkItemRoutine(PVOID Context);



class CUSBHardwareDevice : public IUHCIHardwareDevice
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
    IMP_IUHCIHARDWAREDEVICE

    // local
    NTSTATUS StartController();
    NTSTATUS StopController();
    NTSTATUS ResetController();
    VOID GlobalReset();
    BOOLEAN InterruptService();
    NTSTATUS InitializeController();

    // friend function
    friend BOOLEAN NTAPI InterruptServiceRoutine(IN PKINTERRUPT  Interrupt, IN PVOID  ServiceContext);
    friend VOID NTAPI UhciDeferredRoutine(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2);
    friend VOID NTAPI TimerDpcRoutine(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2);
    friend VOID NTAPI StatusChangeWorkItemRoutine(PVOID Context);
    VOID WriteRegister8(IN ULONG Register, IN UCHAR value);
    VOID WriteRegister16(ULONG Register, USHORT Value);
    VOID WriteRegister32(ULONG Register, ULONG Value);
    UCHAR ReadRegister8(ULONG Register);
    USHORT ReadRegister16(ULONG Register);
    ULONG  ReadRegister32(ULONG Register);

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
    PULONG m_Base;                                                                     // UHCI operational port base registers
    PDMA_ADAPTER m_Adapter;                                                            // dma adapter object
    ULONG m_MapRegisters;                                                              // map registers count
    USHORT m_VendorID;                                                                 // vendor id
    USHORT m_DeviceID;                                                                 // device id
    PUHCIQUEUE m_UsbQueue;                                                              // usb request queue
    ULONG m_NumberOfPorts;                                                             // number of ports
    PDMAMEMORYMANAGER m_MemoryManager;                                                 // memory manager
    HD_INIT_CALLBACK* m_SCECallBack;                                                   // status change callback routine
    PVOID m_SCEContext;                                                                // status change callback routine context
    //WORK_QUEUE_ITEM m_StatusChangeWorkItem;                                            // work item for status change callback
    ULONG m_InterruptMask;                                                             // interrupt enabled mask
    ULONG m_PortResetChange;                                                           // port reset status
    PULONG m_FrameList;                                                                // frame list
    PHYSICAL_ADDRESS m_FrameListPhysicalAddress;                                       // frame list physical address
    PUSHORT m_FrameBandwidth;                                                          // frame bandwidth
    PUHCI_QUEUE_HEAD m_QueueHead[5];                                                   // queue heads
    PHYSICAL_ADDRESS m_StrayDescriptorPhysicalAddress;                                 // physical address stray descriptor
    PUHCI_TRANSFER_DESCRIPTOR m_StrayDescriptor;                                       // stray descriptor
    KTIMER m_SCETimer;                                                                 // SCE timer
    KDPC m_SCETimerDpc;                                                                // timer dpc
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
    return "USBUHCI";
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
    Status = CreateUSBQueue((PUSBQUEUE*)&m_UsbQueue);
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
    // initialize status change work item
    //
    //ExInitializeWorkItem(&m_StatusChangeWorkItem, StatusChangeWorkItemRoutine, PVOID(this));


    // initialize timer
    KeInitializeTimer(&m_SCETimer);

    // initialize timer dpc
    KeInitializeDpc(&m_SCETimerDpc, TimerDpcRoutine, PVOID(this));


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
CUSBHardwareDevice::PnpStart(
    PCM_RESOURCE_LIST RawResources,
    PCM_RESOURCE_LIST TranslatedResources)
{
    ULONG Index;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptor;
    DEVICE_DESCRIPTION DeviceDescription;
    NTSTATUS Status;

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
                                UhciDeferredRoutine,
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
            case CmResourceTypePort:
            {
                //
                // Store Resource base
                //
                m_Base = (PULONG)ResourceDescriptor->u.Port.Start.LowPart; //FIXME
                DPRINT("UHCI Base %p Length %x\n", m_Base, ResourceDescriptor->u.Port.Length);
                break;
            }
        }
    }

    ASSERT(m_Base);

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
CUSBHardwareDevice::PnpStop(void)
{
    UNIMPLEMENTED;
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
    ULONG Index;
    USHORT Status;


    //
    // debug info
    //
    DPRINT("[USBUHCI] USBCMD: %x USBSTS %x\n", ReadRegister16(UHCI_USBCMD), ReadRegister16(UHCI_USBSTS));

    //
    // Set the run bit in the command register
    //
    WriteRegister16(UHCI_USBCMD, ReadRegister16(UHCI_USBCMD) | UHCI_USBCMD_RS);

    for(Index = 0; Index < 100; Index++)
    {
        //
        // wait a bit
        //
        KeStallExecutionProcessor(100);

        //
        // get controller status
        //
        Status = ReadRegister16(UHCI_USBSTS);
        DPRINT("[USBUHCI] Status %x\n", Status);

        if (!(Status & UHCI_USBSTS_HCHALT))
        {
            //
            // controller started
            //
            break;
        }
    }

    DPRINT("[USBUHCI] USBCMD: %x USBSTS %x\n", ReadRegister16(UHCI_USBCMD), ReadRegister16(UHCI_USBSTS));


    if ((Status & UHCI_USBSTS_HCHALT))
    {
        //
        // failed to start controller
        //
        DPRINT1("[USBUHCI] Failed to start controller Status %x\n", Status);
        ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Set the configure bit
    //
    WriteRegister16(UHCI_USBCMD, ReadRegister16(UHCI_USBCMD) | UHCI_USBCMD_CF);

    for(Index = 0; Index < 2; Index++)
    {
        //
        // get port status
        //
        Status = ReadRegister16(UHCI_PORTSC1 + Index * 2);

        //
        // clear connection change and port suspend
        //
        WriteRegister16(UHCI_PORTSC1 + Index * 2, Status & ~(UHCI_PORTSC_STATCHA | UHCI_PORTSC_SUSPEND));
    }

    DPRINT("[USBUHCI] Controller Started\n");
    DPRINT("[USBUHCI] Controller Status %x\n", ReadRegister16(UHCI_USBSTS));
    DPRINT("[USBUHCI] Controller Cmd Status %x\n", ReadRegister16(UHCI_USBCMD));
    DPRINT("[USBUHCI] Controller Interrupt Status %x\n", ReadRegister16(UHCI_USBINTR));
    DPRINT("[USBUHCI] Controller Frame %x\n", ReadRegister16(UHCI_FRNUM));
    DPRINT("[USBUHCI] Controller Port Status 0 %x\n", ReadRegister16(UHCI_PORTSC1));
    DPRINT("[USBUHCI] Controller Port Status 1 %x\n", ReadRegister16(UHCI_PORTSC1 + 2));


    // queue timer
    LARGE_INTEGER Expires;
    Expires.QuadPart = -10 * 10000;

    KeSetTimerEx(&m_SCETimer, Expires, 1000, &m_SCETimerDpc);

    //
    // done
    //
    return STATUS_SUCCESS;
}

VOID
CUSBHardwareDevice::GlobalReset()
{
    LARGE_INTEGER Timeout;

    //
    // back up start of modify register
    //
    ASSERT(m_Base);
    UCHAR sofValue = READ_PORT_UCHAR((PUCHAR)((ULONG)m_Base + UHCI_SOFMOD));

    //
    // perform global reset
    //
    WriteRegister16(UHCI_USBCMD, ReadRegister16(UHCI_USBCMD) | UHCI_USBCMD_GRESET);

    //
    // delay is 10 ms
    //
    Timeout.QuadPart = 10;
    DPRINT("Waiting %lu milliseconds for global reset\n", Timeout.LowPart);

    //
    // convert to 100 ns units (absolute)
    //
    Timeout.QuadPart *= -10000;

    //
    // perform the wait
    //
    KeDelayExecutionThread(KernelMode, FALSE, &Timeout);

    //
    // clear command register
    //
    WriteRegister16(UHCI_USBCMD, ReadRegister16(UHCI_USBCMD) & ~UHCI_USBCMD_GRESET);
    KeStallExecutionProcessor(10);


    //
    // restore start of modify register
    //
    WRITE_PORT_UCHAR((PUCHAR)((ULONG)m_Base + UHCI_SOFMOD), sofValue);
}

NTSTATUS
CUSBHardwareDevice::InitializeController()
{
    NTSTATUS Status;
    ULONG Index;
    BUS_INTERFACE_STANDARD BusInterface;
    USHORT Value;
    PHYSICAL_ADDRESS Address;

    DPRINT("[USBUHCI] InitializeController\n");

    //
    // now disable all interrupts
    //
    WriteRegister16(UHCI_USBINTR, 0);


    //
    // UHCI has two ports
    //
    m_NumberOfPorts = 2;

    //
    // get bus interface
    //
    Status = GetBusInterface(m_PhysicalDeviceObject, &BusInterface);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get BusInterface!\n");
        return Status;
    }

    //
    // reclaim ownership from BIOS
    //
    Value = 0;
    BusInterface.GetBusData(BusInterface.Context, PCI_WHICHSPACE_CONFIG, &Value, PCI_LEGSUP, sizeof(USHORT));
    DPRINT("[USBUHCI] LEGSUP %x\n", Value);

    Value = PCI_LEGSUP_USBPIRQDEN;
    BusInterface.SetBusData(BusInterface.Context, PCI_WHICHSPACE_CONFIG, &Value, PCI_LEGSUP, sizeof(USHORT));

    DPRINT("[USBUHCI] Acquired ownership\n");
    Value = 0;
    BusInterface.GetBusData(BusInterface.Context, PCI_WHICHSPACE_CONFIG, &Value, 0x60, sizeof(UCHAR));
    DPRINT("[USBUHCI] SBRN %x\n", Value);

    //
    // perform global reset
    //
    GlobalReset();

    //
    // reset controller
    //
    Status = ResetController();
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to reset controller
        //
        DPRINT1("[USBUHCI] Failed to reset controller\n");
        return Status;
    }

    //
    // allocate frame list
    //
    Status = m_MemoryManager->Allocate(NUMBER_OF_FRAMES * sizeof(ULONG), (PVOID*)&m_FrameList, &m_FrameListPhysicalAddress);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to allocate frame list
        //
        DPRINT1("[USBUHCI] Failed to allocate frame list with %x\n", Status);
        return Status;
    }

    //
    // Set base pointer and reset frame number
    //
    WriteRegister32(UHCI_FRBASEADD, m_FrameListPhysicalAddress.LowPart);
    WriteRegister16(UHCI_FRNUM, 0);

    //
    // Set the max packet size for bandwidth reclamation to 64 bytes
    //
    WriteRegister16(UHCI_USBCMD, ReadRegister16(UHCI_USBCMD) | UHCI_USBCMD_MAXP);

    //
    // now create queues
    // 0: interrupt transfers
    // 1: low speed control transfers
    // 2: full speed control transfers
    // 3: bulk transfers
    // 4: debug queue
    //
    for(Index = 0; Index < 5; Index++)
    {
        //
        // create queue head
        //
        Status = m_MemoryManager->Allocate(sizeof(UHCI_QUEUE_HEAD), (PVOID*)&m_QueueHead[Index], &Address);
        if (!NT_SUCCESS(Status))
        {
            //
            // failed to allocate queue head
            //
            DPRINT1("[USBUHCI] Failed to allocate queue head %x Index %x\n", Status, Index);
            return Status;
        }

        //
        // store queue head
        //
        m_QueueHead[Index]->PhysicalAddress = Address.LowPart;
        m_QueueHead[Index]->ElementPhysical = QH_TERMINATE;
        m_QueueHead[Index]->LinkPhysical = QH_TERMINATE;

        if (Index > 0)
        {
            //
            // link queue heads
            //
            m_QueueHead[Index-1]->LinkPhysical = m_QueueHead[Index]->PhysicalAddress | QH_NEXT_IS_QH;
            m_QueueHead[Index-1]->NextLogicalDescriptor = m_QueueHead[Index];
        }
    }

            DPRINT("Index %d QueueHead %p LinkPhysical %x ElementPhysical %x PhysicalAddress %x Request %p NextElementDescriptor %p\n",
                0,
                m_QueueHead[0], 
                m_QueueHead[0]->LinkPhysical,
                m_QueueHead[0]->ElementPhysical,
                m_QueueHead[0]->PhysicalAddress, 
                m_QueueHead[0]->Request, 
                m_QueueHead[0]->NextElementDescriptor);
            DPRINT("Index %d QueueHead %p LinkPhysical %x ElementPhysical %x PhysicalAddress %x Request %p NextElementDescriptor %p\n",
                1,
                m_QueueHead[1], 
                m_QueueHead[1]->LinkPhysical,
                m_QueueHead[1]->ElementPhysical,
                m_QueueHead[1]->PhysicalAddress, 
                m_QueueHead[1]->Request, 
                m_QueueHead[1]->NextElementDescriptor);

            DPRINT("Index %d QueueHead %p LinkPhysical %x ElementPhysical %x PhysicalAddress %x Request %p NextElementDescriptor %p\n",
                2,
                m_QueueHead[2], 
                m_QueueHead[2]->LinkPhysical,
                m_QueueHead[2]->ElementPhysical,
                m_QueueHead[2]->PhysicalAddress, 
                m_QueueHead[2]->Request, 
                m_QueueHead[2]->NextElementDescriptor);
            DPRINT("Index %d QueueHead %p LinkPhysical %x ElementPhysical %x PhysicalAddress %x Request %p NextElementDescriptor %p\n",
                3,
                m_QueueHead[3], 
                m_QueueHead[3]->LinkPhysical,
                m_QueueHead[3]->ElementPhysical,
                m_QueueHead[3]->PhysicalAddress, 
                m_QueueHead[3]->Request, 
                m_QueueHead[3]->NextElementDescriptor);
            DPRINT("Index %d QueueHead %p LinkPhysical %x ElementPhysical %x PhysicalAddress %x Request %p NextElementDescriptor %p\n",
                4,
                m_QueueHead[4], 
                m_QueueHead[4]->LinkPhysical,
                m_QueueHead[4]->ElementPhysical,
                m_QueueHead[4]->PhysicalAddress, 
                m_QueueHead[4]->Request, 
                m_QueueHead[4]->NextElementDescriptor);

    //
    // terminate last queue head with stray descriptor
    //
    Status = m_MemoryManager->Allocate(sizeof(UHCI_TRANSFER_DESCRIPTOR), (PVOID*)&m_StrayDescriptor, &m_StrayDescriptorPhysicalAddress);
    if (!NT_SUCCESS(Status))
    {
        //
        // failed to allocate queue head
        //
        DPRINT1("[USBUHCI] Failed to allocate queue head %x Index %x\n", Status, Index);
        return Status;
    }
#if 0
    //
    // init stray descriptor
    //
    m_StrayDescriptor->PhysicalAddress = m_StrayDescriptorPhysicalAddress.LowPart;
    m_StrayDescriptor->LinkPhysical = TD_TERMINATE;
    m_StrayDescriptor->Token = TD_TOKEN_NULL_DATA | (0x7f << TD_TOKEN_DEVADDR_SHIFT) | TD_TOKEN_IN;


    //
    // link to last queue head
    //
    m_QueueHead[4]->LinkPhysical = m_StrayDescriptor->PhysicalAddress;
    m_QueueHead[4]->NextLogicalDescriptor = m_StrayDescriptor;
#endif

    //
    // allocate frame bandwidth array
    //
    m_FrameBandwidth = (PUSHORT)ExAllocatePool(NonPagedPool, sizeof(USHORT) * NUMBER_OF_FRAMES);
    if (!m_FrameBandwidth)
    {
        //
        // no memory
        //
        DPRINT1("[USBUHCI] Failed to allocate memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // init frame list
    //
    for (Index = 0; Index < NUMBER_OF_FRAMES; Index++)
    {
        //
        // store frame list interrupt queue
        //
        m_FrameList[Index] =  m_QueueHead[UHCI_INTERRUPT_QUEUE]->PhysicalAddress | FRAMELIST_NEXT_IS_QH;
        m_FrameBandwidth[Index] = MAX_AVAILABLE_BANDWIDTH;


    }

    //
    // set enabled interrupt mask
    //
    m_InterruptMask = UHCI_USBSTS_USBINT | UHCI_USBSTS_ERRINT | UHCI_USBSTS_HOSTERR | UHCI_USBSTS_HCPRERR | UHCI_USBSTS_HCHALT;

    //
    // now enable interrupts
    //
    WriteRegister16(UHCI_USBINTR, UHCI_USBINTR_CRC | UHCI_USBINTR_IOC| UHCI_USBINTR_SHORT);

    DPRINT("[USBUHCI] Controller initialized\n");
    return STATUS_SUCCESS;
}

NTSTATUS
CUSBHardwareDevice::StopController(void)
{
    ASSERT(FALSE);
    //
    // failed to reset controller
    //
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
CUSBHardwareDevice::ResetController(void)
{
    ULONG Count = 0;
    USHORT Status;

    // clear run bit
    WriteRegister16(UHCI_USBCMD, ReadRegister16(UHCI_USBCMD) & ~UHCI_USBCMD_RS);

    // wait for the controller to stop
    while((ReadRegister16(UHCI_USBSTS) & UHCI_USBSTS_HCHALT) == 0)
    {
        DPRINT("[UHCI] Waiting for the controller to halt\n");
        KeStallExecutionProcessor(10);
    }

    // clear configure bit
    WriteRegister16(UHCI_USBCMD, ReadRegister16(UHCI_USBCMD) & ~UHCI_USBCMD_CF);

    //
    // reset controller
    //
    WriteRegister16(UHCI_USBCMD, UHCI_USBCMD_HCRESET);

    do
    {
        //
        // wait a bit
        //
        KeStallExecutionProcessor(100);

        //
        // get status
        //
        Status = ReadRegister16(UHCI_USBCMD);
        if (!(Status & UHCI_USBCMD_HCRESET))
        {
            //
            // controller reset completed
            //
            return STATUS_SUCCESS;
        }
    }while(Count++ < 100);

    DPRINT1("[USBUHCI] Failed to reset controller Status %x\n", Status);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
CUSBHardwareDevice::ResetPort(
    IN ULONG PortIndex)
{
    ULONG Port;
    USHORT Status;
    ULONG Index;
    LARGE_INTEGER Timeout;

    DPRINT("[UHCI] ResetPort Id %lu\n", PortIndex);

    //
    // sanity check
    //
    ASSERT(PortIndex <= 1);

    //
    // get register offset
    //
    Port = UHCI_PORTSC1 + PortIndex * 2;

    //
    // read port status
    //
    Status = ReadRegister16(Port);



    //
    // remove unwanted bits
    //
    Status &= UHCI_PORTSC_DATAMASK;

    //
    // now reset the port
    //
    WriteRegister16(Port, Status | UHCI_PORTSC_RESET);

    //
    // delay is 20 ms for port reset
    //
    Timeout.QuadPart = 20;
    DPRINT("Waiting %lu milliseconds for port reset\n", Timeout.LowPart);

    //
    // convert to 100 ns units (absolute)
    //
    Timeout.QuadPart *= -10000;

    //
    // perform the wait
    //
    KeDelayExecutionThread(KernelMode, FALSE, &Timeout);

    //
    // re-read status
    //
    Status = ReadRegister16(Port);

    //
    // remove unwanted bits
    //
    Status &= UHCI_PORTSC_DATAMASK;

    //
    // clear reset port 
    //
    WriteRegister16(Port, (Status & ~UHCI_PORTSC_RESET));


    //
    // now wait a bit
    //
    KeStallExecutionProcessor(10);

    for (Index = 0; Index < 100; Index++) 
    {
        // read port status
        Status = ReadRegister16(Port);

        // remove unwanted bits
        Status &= UHCI_PORTSC_DATAMASK;

        // enable port
        WriteRegister16(Port, Status | UHCI_PORTSC_ENABLED);

        //
        // wait a bit
        //
        KeStallExecutionProcessor(50);

        //
        // re-read port
        //
        Status = ReadRegister16(Port);

        if ((Status & UHCI_PORTSC_CURSTAT) == 0) 
        {
            // no device connected. since we waited long enough we can assume
            // that the port was reset and no device is connected.
            break;
        }

        if (Status & (UHCI_PORTSC_STATCHA | UHCI_PORTSC_ENABCHA)) 
        {
            // port enabled changed or connection status were set.
            // acknowledge either / both and wait again.
            WriteRegister16(Port, Status);
            continue;
        }

        if (Status & UHCI_PORTSC_ENABLED) 
        {
            // the port is enabled
            break;
        }
    }

    m_PortResetChange |= (1 << PortIndex);
    DPRINT("[USBUHCI] Port Index %x Status after reset %x\n", PortIndex, ReadRegister16(Port));

    //
    // is there a callback
    //
    if (m_SCECallBack)
    {
        //
        // issue callback
        //
        m_SCECallBack(m_SCEContext);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
CUSBHardwareDevice::GetPortStatus(
    ULONG PortId,
    OUT USHORT *PortStatus,
    OUT USHORT *PortChange)
{
    USHORT Status;

    //
    // sanity check
    //
    if (PortId > 1)
    {
        //
        // invalid index
        //
        DPRINT1("[UHCI] Invalid PortIndex %lu\n", PortId);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // init status
    //
    *PortStatus = 0;
    *PortChange = 0;

    //
    // read port status
    //
    Status = ReadRegister16(UHCI_PORTSC1 + PortId * 2);
    DPRINT("[USBUHCI] PortId %x Status %x\n", PortId, Status);

    // build the status
    if (Status & UHCI_PORTSC_CURSTAT)
    {
        *PortStatus |= USB_PORT_STATUS_CONNECT;
    }

    if (Status & UHCI_PORTSC_ENABLED)
    {
        *PortStatus |= USB_PORT_STATUS_ENABLE;
    }

    if (Status & UHCI_PORTSC_RESET)
    {
        *PortStatus |= USB_PORT_STATUS_RESET;
    }

    if (Status & UHCI_PORTSC_LOWSPEED)
    {
        *PortStatus |= USB_PORT_STATUS_LOW_SPEED;
    }

    if (Status & UHCI_PORTSC_STATCHA)
    {
        *PortChange |= USB_PORT_STATUS_CONNECT;
    }

    if (Status & UHCI_PORTSC_ENABCHA)
    {
        *PortChange |= USB_PORT_STATUS_ENABLE;
    }

    if (m_PortResetChange & (1 << PortId))
    {
        *PortChange |= USB_PORT_STATUS_RESET;
    }

    //
    // port always has power
    //
    *PortStatus |= USB_PORT_STATUS_POWER;
    return STATUS_SUCCESS;
}

NTSTATUS
CUSBHardwareDevice::ClearPortStatus(
    ULONG PortId,
    ULONG Feature)
{
    ULONG PortRegister;
    USHORT PortStatus;

    DPRINT("CUSBHardwareDevice::ClearPortStatus PortId %x Feature %x\n", PortId, Feature);

    //
    // sanity check
    //
    if (PortId > 1)
    {
        //
        // invalid index
        //
        DPRINT1("[UHCI] Invalid PortIndex %lu\n", PortId);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // read current status
    //
    PortRegister = UHCI_PORTSC1 + PortId * 2;
    PortStatus = ReadRegister16(PortRegister);
    DPRINT("[UHCI] PortStatus %x\n", PortStatus);

    if (Feature == C_PORT_RESET)
    {
        //
        // UHCI is not supporting port reset register bit
        //
        m_PortResetChange &= ~(1 << PortId);
    }
    else if (Feature == C_PORT_CONNECTION || Feature == C_PORT_ENABLE)
    {
        //
        // clear port status changes
        //
        WriteRegister16(PortRegister, PortStatus);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
CUSBHardwareDevice::SetPortFeature(
    ULONG PortId,
    ULONG Feature)
{
    ULONG PortRegister;

    DPRINT("[UHCI] SetPortFeature PortId %x Feature %x\n", PortId, Feature);

    //
    // sanity check
    //
    if (PortId > 1)
    {
        //
        // invalid index
        //
        DPRINT1("[UHCI] Invalid PortIndex %lu\n", PortId);
        return STATUS_INVALID_PARAMETER;
    }

    PortRegister = UHCI_PORTSC1 + PortId * 2;

    if (Feature == PORT_RESET)
    {
        //
        // reset port
        //
        return ResetPort(PortId); 
    }
    else if (Feature == PORT_ENABLE)
    {
        //
        // reset port
        //
        WriteRegister16(PortRegister, ReadRegister16(PortRegister) | UHCI_PORTSC_ENABLED);
    }
    else if (Feature == PORT_POWER)
    {
        //
        // port power is no op, it is always enabled
        //
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

BOOLEAN
NTAPI
InterruptServiceRoutine(
    IN PKINTERRUPT  Interrupt,
    IN PVOID  ServiceContext)
{
    CUSBHardwareDevice *This;
    USHORT Status, Acknowledge;

    //
    // get context
    //
    This = (CUSBHardwareDevice*) ServiceContext;

    //
    // read register
    //
    Status = This->ReadRegister16(UHCI_USBSTS);
    DPRINT("InterruptServiceRoutine %x\n", Status);

    //
    // check if the interrupt signaled are from us
    //
    if ((Status & This->m_InterruptMask) == 0)
    {
        if (Status != 0)
        {
            //
            // FIXME: received unexpected interrupt
            //
            DPRINT1("[USBUHCI] Unexpected interrupt %x\n", Status);
            This->WriteRegister16(UHCI_USBSTS, Status);
        }

        //
        // shared interrupt
        //
        return FALSE;
    }

    //
    // check for the interrupt cause
    //
    Acknowledge = 0;

    if (Status & UHCI_USBSTS_USBINT)
    {
        //
        // transfer finished
        //
        Acknowledge |= UHCI_USBSTS_USBINT;
    }

    if (Status & UHCI_USBSTS_ERRINT) 
    {
        //
        // error interrupt
        //
        Acknowledge |= UHCI_USBSTS_ERRINT;
        DPRINT("[UHCI] Error interrupt\n");
    }

    if (Status & UHCI_USBSTS_RESDET) 
    {
        //
        // resume detected
        //
        DPRINT("[UHCI] Resume detected\n");
        Acknowledge |= UHCI_USBSTS_RESDET;
    }

    if (Status & UHCI_USBSTS_HOSTERR) 
    {
        //
        // host system error
        //
        DPRINT("[UHCI] Host System Error\n");
        Acknowledge |= UHCI_USBSTS_HOSTERR;
    }

    if (Status & UHCI_USBSTS_HCPRERR) 
    {
        //
        // processing error
        //
        DPRINT("[UHCI] Process Error\n");
        Acknowledge |= UHCI_USBSTS_HCPRERR;
    }

    if (Status & UHCI_USBSTS_HCHALT) 
    {
        //
        // controller halted
        //
        DPRINT("[UHCI] Host controller halted\n");

        //
        // disable interrupts
        //
        This->WriteRegister16(UHCI_USBINTR, 0);
        This->m_InterruptMask = 0;
    }

    //
    // do we have something to acknowledge
    //
    if (Acknowledge)
    {
        //
        // acknowledge interrupt
        //
        This->WriteRegister16(UHCI_USBSTS, Acknowledge);

        //
        // queue dpc
        //
        KeInsertQueueDpc(&This->m_IntDpcObject, UlongToPtr(Status), NULL);
    }

    //
    // interrupt handled
    //
    return TRUE;
}


VOID
CUSBHardwareDevice::WriteRegister8(
    IN ULONG Register, 
    IN UCHAR Value)
{
    WRITE_PORT_UCHAR((PUCHAR)((ULONG)m_Base + Register), Value);
}


VOID
CUSBHardwareDevice::WriteRegister16(
    ULONG Register,
    USHORT Value)
{
    WRITE_PORT_USHORT((PUSHORT)((ULONG)m_Base + Register), Value);
}


VOID
CUSBHardwareDevice::WriteRegister32(
    ULONG Register,
    ULONG Value)
{
    WRITE_PORT_ULONG((PULONG)((ULONG)m_Base + Register), Value);
}


UCHAR
CUSBHardwareDevice::ReadRegister8(
    ULONG Register)
{
    return  READ_PORT_UCHAR((PUCHAR)((ULONG)m_Base + Register));
}


USHORT
CUSBHardwareDevice::ReadRegister16(
    ULONG Register)
{
    return  READ_PORT_USHORT((PUSHORT)((ULONG)m_Base + Register));
}


ULONG
CUSBHardwareDevice::ReadRegister32(
    ULONG Register)
{
    return  READ_PORT_ULONG((PULONG)((ULONG)m_Base + Register));
}

VOID
CUSBHardwareDevice::GetQueueHead(
    IN ULONG QueueHeadIndex, 
    OUT PUHCI_QUEUE_HEAD *OutQueueHead)
{
    //
    // sanity check
    //
    ASSERT(QueueHeadIndex < 5);

    //
    // store queue head
    //
    *OutQueueHead = m_QueueHead[QueueHeadIndex];
}

VOID
NTAPI
UhciDeferredRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2)
{
    CUSBHardwareDevice *This;
    ULONG Status;

    //
    // get parameters
    //
    This = (CUSBHardwareDevice*)DeferredContext;

    DPRINT("UhciDeferredRoutine\n");

    //
    // get status
    //
    Status = PtrToUlong(SystemArgument1);
    if (Status & (UHCI_USBSTS_USBINT | UHCI_USBSTS_ERRINT))
    {
        //
        // a transfer finished, inform the queue
        //
        This->m_UsbQueue->TransferInterrupt(Status & UHCI_USBSTS_USBINT);
        return;
    }

    //
    // other event
    //
    DPRINT1("[USBUHCI] Status %x not handled\n", Status);
}

VOID
NTAPI
TimerDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2)
{
    CUSBHardwareDevice *This;
    USHORT PortStatus = 0;
    USHORT PortChange = 0;

    // get parameters
    This = (CUSBHardwareDevice*)DeferredContext;

    // check port 0
    This->GetPortStatus(0, &PortStatus, &PortChange);
    if (PortChange)
    {
        // invoke status change work item routine
        StatusChangeWorkItemRoutine(DeferredContext);
        return;
    }

    // check port 1
    This->GetPortStatus(1, &PortStatus, &PortChange);
    if (PortChange)
    {
        // invoke status change work item routine
        StatusChangeWorkItemRoutine(DeferredContext);
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
NTAPI
CreateUSBHardware(
    PUSBHARDWAREDEVICE *OutHardware)
{
    PUSBHARDWAREDEVICE This;

    This = new(NonPagedPool, TAG_USBUHCI) CUSBHardwareDevice(0);

    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->AddRef();

    // return result
    *OutHardware = (PUSBHARDWAREDEVICE)This;

    return STATUS_SUCCESS;
}
