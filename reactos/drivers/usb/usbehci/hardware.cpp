/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/hcd_controller.cpp
 * PURPOSE:     USB EHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "usbehci.h"

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
EhciDefferedRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2);

VOID
NTAPI
StatusChangeWorkItemRoutine(PVOID Context);

class CUSBHardwareDevice : public IEHCIHardwareDevice
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
    IMP_IUSBEHCIHARDWARE

    // local
    BOOLEAN InterruptService();
    VOID PrintCapabilities();
    NTSTATUS StartController();
    NTSTATUS StopController();
    NTSTATUS ResetController();

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
    PULONG m_Base;                                                                     // EHCI operational port base registers
    PDMA_ADAPTER m_Adapter;                                                            // dma adapter object
    ULONG m_MapRegisters;                                                              // map registers count
    EHCI_CAPS m_Capabilities;                                                          // EHCI caps
    USHORT m_VendorID;                                                                 // vendor id
    USHORT m_DeviceID;                                                                 // device id
    PQUEUE_HEAD AsyncQueueHead;                                                        // async queue head terminator
    PEHCIQUEUE m_UsbQueue;                                                              // usb request queue
    PDMAMEMORYMANAGER m_MemoryManager;                                                 // memory manager
    HD_INIT_CALLBACK* m_SCECallBack;                                                   // status change callback routine
    PVOID m_SCEContext;                                                                // status change callback routine context
    BOOLEAN m_DoorBellRingInProgress;                                                  // door bell ring in progress
    WORK_QUEUE_ITEM m_StatusChangeWorkItem;                                            // work item for status change callback
    volatile LONG m_StatusChangeWorkItemStatus;                                        // work item status
    ULONG m_SyncFramePhysAddr;                                                         // periodic frame list physical address
    BUS_INTERFACE_STANDARD m_BusInterface;                                             // pci bus interface
    BOOLEAN m_PortResetInProgress[0xF];                                                // stores reset in progress (vbox hack)

    // read register
    ULONG EHCI_READ_REGISTER_ULONG(ULONG Offset);

    // write register
    VOID EHCI_WRITE_REGISTER_ULONG(ULONG Offset, ULONG Value);
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
    return "USBEHCI";
}

NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::Initialize(
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT FunctionalDeviceObject,
    PDEVICE_OBJECT PhysicalDeviceObject,
    PDEVICE_OBJECT LowerDeviceObject)
{
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
    // intialize status change work item
    //
    ExInitializeWorkItem(&m_StatusChangeWorkItem, StatusChangeWorkItemRoutine, PVOID(this));

    m_VendorID = 0;
    m_DeviceID = 0;

    Status = GetBusInterface(PhysicalDeviceObject, &m_BusInterface);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get BusInteface!\n");
        return Status;
    }

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

    m_VendorID = PciConfig.VendorID;
    m_DeviceID = PciConfig.DeviceID;

    return STATUS_SUCCESS;
}

VOID
STDMETHODCALLTYPE
CUSBHardwareDevice::SetCommandRegister(PEHCI_USBCMD_CONTENT UsbCmd)
{
    PULONG Register;
    Register = (PULONG)UsbCmd;
    WRITE_REGISTER_ULONG((PULONG)((ULONG)m_Base + EHCI_USBCMD), *Register);
}

VOID
STDMETHODCALLTYPE
CUSBHardwareDevice::GetCommandRegister(PEHCI_USBCMD_CONTENT UsbCmd)
{
    PULONG Register;
    Register = (PULONG)UsbCmd;
    *Register = READ_REGISTER_ULONG((PULONG)((ULONG)m_Base + EHCI_USBCMD));
}

ULONG
CUSBHardwareDevice::EHCI_READ_REGISTER_ULONG(ULONG Offset)
{
    return READ_REGISTER_ULONG((PULONG)((ULONG)m_Base + Offset));
}

VOID
CUSBHardwareDevice::EHCI_WRITE_REGISTER_ULONG(ULONG Offset, ULONG Value)
{
    WRITE_REGISTER_ULONG((PULONG)((ULONG)m_Base + Offset), Value);
}

VOID
CUSBHardwareDevice::PrintCapabilities()
{
    if (m_Capabilities.HCSParams.PortPowerControl)
    {
        DPRINT1("Controler EHCI has Port Power Control\n");
    }

    DPRINT1("Controller Port Routing Rules %lu\n", m_Capabilities.HCSParams.PortRouteRules);
    DPRINT1("Number of Ports per Companion Controller %lu\n", m_Capabilities.HCSParams.PortPerCHC);
    DPRINT1("Number of Companion Controller %lu\n", m_Capabilities.HCSParams.CHCCount);

    if (m_Capabilities.HCSParams.PortIndicator)
    {
        DPRINT1("Controller has Port Indicators Support\n");
    }

    if (m_Capabilities.HCSParams.DbgPortNum)
    {
        DPRINT1("Controller has Debug Port Support At Port %x\n", m_Capabilities.HCSParams.DbgPortNum);
    }

    if (m_Capabilities.HCCParams.EECPCapable)
    {
        DPRINT1("Controller has Extended Capabilities Support\n");
    }

    if (m_Capabilities.HCCParams.ParkMode)
    {
        DPRINT1("Controller supports Asynchronous Schedule Park\n");
    }

    if (m_Capabilities.HCCParams.VarFrameList)
    {
        DPRINT1("Controller supports Programmable Frame List Size\n");
    }

    if (m_Capabilities.HCCParams.CurAddrBits)
    {
        DPRINT1("Controller uses 64-Bit Addressing\n");
    }
}

NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::PnpStart(
    PCM_RESOURCE_LIST RawResources,
    PCM_RESOURCE_LIST TranslatedResources)
{
    ULONG Index, Count;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptor;
    DEVICE_DESCRIPTION DeviceDescription;
    PHYSICAL_ADDRESS AsyncPhysicalAddress;
    PVOID ResourceBase;
    NTSTATUS Status;
    UCHAR Value;
    UCHAR PortCount;

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
                m_Capabilities.Length = READ_REGISTER_UCHAR((PUCHAR)ResourceBase + EHCI_CAPLENGTH);
                m_Capabilities.HCIVersion = READ_REGISTER_USHORT((PUSHORT)((ULONG_PTR)ResourceBase + EHCI_HCIVERSION));
                m_Capabilities.HCSParamsLong = READ_REGISTER_ULONG((PULONG)((ULONG_PTR)ResourceBase + EHCI_HCSPARAMS));
                m_Capabilities.HCCParamsLong = READ_REGISTER_ULONG((PULONG)((ULONG_PTR)ResourceBase + EHCI_HCCPARAMS));

                DPRINT1("Controller Capabilities Length 0x%x\n", m_Capabilities.Length);
                DPRINT1("Controller EHCI Version 0x%x\n", m_Capabilities.HCIVersion);
                DPRINT1("Controller EHCI Caps HCSParamsLong 0x%lx\n", m_Capabilities.HCSParamsLong);
                DPRINT1("Controller EHCI Caps HCCParamsLong 0x%lx\n", m_Capabilities.HCCParamsLong);
                DPRINT1("Controller has %lu Ports\n", m_Capabilities.HCSParams.PortCount);

                //
                // print capabilities
                //
                PrintCapabilities();

                if (m_Capabilities.HCSParams.PortRouteRules)
                {
                    Count = 0;
                    PortCount = max(m_Capabilities.HCSParams.PortCount/2, (m_Capabilities.HCSParams.PortCount+1)/2);
                    do
                    {
                        //
                        // each entry is a 4 bit field EHCI 2.2.5
                        //
                        Value = READ_REGISTER_UCHAR((PUCHAR)(ULONG)ResourceBase + EHCI_HCSP_PORTROUTE + Count);
                        m_Capabilities.PortRoute[Count*2] = (Value & 0xF0);

                        if ((Count*2) + 1 < m_Capabilities.HCSParams.PortCount)
                            m_Capabilities.PortRoute[(Count*2)+1] = (Value & 0x0F);

                        Count++;
                    } while(Count < PortCount);
                }

                //
                // Set m_Base to the address of Operational Register Space
                //
                m_Base = (PULONG)((ULONG)ResourceBase + m_Capabilities.Length);
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
    // Create a queuehead for the Async Register
    //
    m_MemoryManager->Allocate(sizeof(QUEUE_HEAD), (PVOID*)&AsyncQueueHead, &AsyncPhysicalAddress);

    AsyncQueueHead->PhysicalAddr = AsyncPhysicalAddress.LowPart;
    AsyncQueueHead->HorizontalLinkPointer = AsyncQueueHead->PhysicalAddr | QH_TYPE_QH;
    AsyncQueueHead->EndPointCharacteristics.HeadOfReclamation = TRUE;
    AsyncQueueHead->EndPointCharacteristics.EndPointSpeed = QH_ENDPOINT_HIGHSPEED;
    AsyncQueueHead->Token.Bits.Halted = TRUE;

    AsyncQueueHead->EndPointCapabilities.NumberOfTransactionPerFrame = 0x01;
    AsyncQueueHead->NextPointer = TERMINATE_POINTER;
    AsyncQueueHead->CurrentLinkPointer = TERMINATE_POINTER;

    InitializeListHead(&AsyncQueueHead->LinkedQueueHeads);

    //
    // Initialize the UsbQueue now that we have an AdapterObject.
    //
    Status = m_UsbQueue->Initialize(PUSBHARDWAREDEVICE(this), m_Adapter, m_MemoryManager, &m_Lock);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to Initialize the UsbQueue\n");
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
STDMETHODCALLTYPE
CUSBHardwareDevice::PnpStop(void)
{
    UNIMPLEMENTED
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
        *NumberOfPorts = m_Capabilities.HCSParams.PortCount;
    //FIXME: What to returned here?
    if (Speed)
        *Speed = 0x200;
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
    EHCI_USBCMD_CONTENT UsbCmd;
    ULONG UsbSts, FailSafe, ExtendedCapsSupport, Caps, Index;
    UCHAR Value;
    LARGE_INTEGER Timeout;

    //
    // are extended caps supported
    //
    ExtendedCapsSupport = (m_Capabilities.HCCParamsLong >> EHCI_ECP_SHIFT) & EHCI_ECP_MASK;
    if (ExtendedCapsSupport)
    {
        DPRINT1("[EHCI] Extended Caps Support detected!\n");

        //
        // sanity check
        //
        ASSERT(ExtendedCapsSupport >= PCI_COMMON_HDR_LENGTH);
        m_BusInterface.GetBusData(m_BusInterface.Context, PCI_WHICHSPACE_CONFIG, &Caps, ExtendedCapsSupport, sizeof(ULONG));

        //
        // OS Handoff Synchronization support capability. EHCI 5.1
        //
        if ((Caps & EHCI_LEGSUP_CAPID_MASK) == EHCI_LEGSUP_CAPID)
        {
            //
            // is it bios owned
            //
            if ((Caps & EHCI_LEGSUP_BIOSOWNED))
            {
                DPRINT1("[EHCI] Controller is BIOS owned, acquring control\n");

                //
                // acquire ownership
                //
                Value = 1;
                m_BusInterface.SetBusData(m_BusInterface.Context, PCI_WHICHSPACE_CONFIG, &Value, ExtendedCapsSupport+3, sizeof(UCHAR));

                for(Index = 0; Index < 20; Index++)
                {
                    //
                    // get status
                    //
                    m_BusInterface.GetBusData(m_BusInterface.Context, PCI_WHICHSPACE_CONFIG, &Caps, ExtendedCapsSupport, sizeof(ULONG));
                    if ((Caps & EHCI_LEGSUP_BIOSOWNED))
                    {
                        //
                        // lets wait a bit
                        //
                        Timeout.QuadPart = 50;
                        DPRINT1("Waiting %lu milliseconds for port reset\n", Timeout.LowPart);

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
                if ((Caps & EHCI_LEGSUP_BIOSOWNED))
                {
                    //
                    // failed to aquire ownership
                    //
                    DPRINT1("[EHCI] failed to acquire ownership\n");
                }
                else if ((Caps & EHCI_LEGSUP_OSOWNED))
                {
                    //
                    // HC OS Owned Semaphore EHCI 2.1.7
                    //
                    DPRINT1("[EHCI] acquired ownership\n");
                }
#if 0
                //
                // explictly clear the bios owned flag 2.1.7
                //
                Value = 0;
                m_BusInterface.SetBusData(m_BusInterface.Context, PCI_WHICHSPACE_CONFIG, &Value, ExtendedCapsSupport+2, sizeof(UCHAR));

                //
                // clear SMI interrupt EHCI 2.1.8
                //
                Caps = 4;
                m_BusInterface.SetBusData(m_BusInterface.Context, PCI_WHICHSPACE_CONFIG, &Caps, ExtendedCapsSupport+4, sizeof(ULONG));
#endif
            }
        }
    }

    //
    // get command register
    //
    GetCommandRegister(&UsbCmd);

    //
    // disable running schedules
    //
    UsbCmd.PeriodicEnable = FALSE;
    UsbCmd.AsyncEnable = FALSE;
    SetCommandRegister(&UsbCmd);

    //
    // Wait for execution to start
    //
    for (FailSafe = 100; FailSafe > 1; FailSafe--)
    {
        KeStallExecutionProcessor(100);
        UsbSts = EHCI_READ_REGISTER_ULONG(EHCI_USBSTS);

        if (!(UsbSts & EHCI_STS_PSS) && (UsbSts & EHCI_STS_ASS))
        {
            break;
        }
    }

    if ((UsbSts & (EHCI_STS_PSS | EHCI_STS_ASS)))
    {
        DPRINT1("Failed to stop running schedules %x\n", UsbSts);
        //ASSERT(FALSE);
    }


    //
    // Stop the controller if its running
    //
    UsbSts = EHCI_READ_REGISTER_ULONG(EHCI_USBSTS);
    if (!(UsbSts & EHCI_STS_HALT))
    {
        DPRINT1("Stopping Controller %x\n", UsbSts);
        StopController();
    }

    //
    // Reset the controller
    //
    ResetController();

    //
    // check caps
    //
    if (m_Capabilities.HCCParams.CurAddrBits)
    {
        //
        // disable 64-bit addressing
        //
        EHCI_WRITE_REGISTER_ULONG(EHCI_CTRLDSSEGMENT, 0x0);
    }

    //
    // Enable Interrupts and start execution
    //
    ULONG Mask = EHCI_USBINTR_INTE | EHCI_USBINTR_ERR | EHCI_USBINTR_ASYNC | EHCI_USBINTR_HSERR | EHCI_USBINTR_PC;
    EHCI_WRITE_REGISTER_ULONG(EHCI_USBINTR, Mask);

    KeStallExecutionProcessor(10);

    ULONG Status = EHCI_READ_REGISTER_ULONG(EHCI_USBINTR);

    DPRINT1("Interrupt Mask %x\n", Status);
    ASSERT((Status & Mask) == Mask);

    //
    // Assign the SyncList Register
    //
    EHCI_WRITE_REGISTER_ULONG(EHCI_PERIODICLISTBASE, m_SyncFramePhysAddr);

    //
    // Set Schedules to Enable and Interrupt Threshold to 1ms.
    //
    RtlZeroMemory(&UsbCmd, sizeof(EHCI_USBCMD_CONTENT));

    UsbCmd.PeriodicEnable = TRUE;
    UsbCmd.IntThreshold = 0x8; //1ms
    UsbCmd.Run = TRUE;
    UsbCmd.FrameListSize = 0x0; //1024

    if (m_Capabilities.HCCParams.ParkMode)
    {
        //
        // enable async park mode
        //
        UsbCmd.AsyncParkEnable = TRUE;
        UsbCmd.AsyncParkCount = 3;
    }

    SetCommandRegister(&UsbCmd);


    //
    // Wait for execution to start
    //
    for (FailSafe = 100; FailSafe > 1; FailSafe--)
    {
        KeStallExecutionProcessor(100);
        UsbSts = EHCI_READ_REGISTER_ULONG(EHCI_USBSTS);

        if (!(UsbSts & EHCI_STS_HALT) && (UsbSts & EHCI_STS_PSS))
        {
            break;
        }
    }

    if (UsbSts & EHCI_STS_HALT)
    {
        DPRINT1("Could not start execution on the controller\n");
        //ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    if (!(UsbSts & EHCI_STS_PSS))
    {
        DPRINT1("Could not enable periodic scheduling\n");
        //ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Assign the AsyncList Register
    //
    EHCI_WRITE_REGISTER_ULONG(EHCI_ASYNCLISTBASE, AsyncQueueHead->PhysicalAddr);

    //
    // get command register
    //
    GetCommandRegister(&UsbCmd);

    //
    // preserve bits
    //
    UsbCmd.AsyncEnable = TRUE;

    //
    // enable async
    //
    SetCommandRegister(&UsbCmd);

    //
    // Wait for execution to start
    //
    for (FailSafe = 100; FailSafe > 1; FailSafe--)
    {
        KeStallExecutionProcessor(100);
        UsbSts = EHCI_READ_REGISTER_ULONG(EHCI_USBSTS);

        if ((UsbSts & EHCI_STS_ASS))
        {
            break;
        }
    }

    if (!(UsbSts & EHCI_STS_ASS))
    {
        DPRINT1("Failed to enable async schedule UsbSts %x\n", UsbSts);
        //ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    DPRINT1("UsbSts %x\n", UsbSts);
    GetCommandRegister(&UsbCmd);

    DPRINT1("UsbCmd.PeriodicEnable %x\n", UsbCmd.PeriodicEnable);
    DPRINT1("UsbCmd.AsyncEnable %x\n", UsbCmd.AsyncEnable);
    DPRINT1("UsbCmd.IntThreshold %x\n", UsbCmd.IntThreshold);
    DPRINT1("UsbCmd.Run %x\n", UsbCmd.Run);
    DPRINT1("UsbCmd.FrameListSize %x\n", UsbCmd.FrameListSize);

    //
    // Set port routing to EHCI controller
    //
    EHCI_WRITE_REGISTER_ULONG(EHCI_CONFIGFLAG, 1);

    DPRINT1("EHCI Started!\n");
    return STATUS_SUCCESS;
}

NTSTATUS
CUSBHardwareDevice::StopController(void)
{
    EHCI_USBCMD_CONTENT UsbCmd;
    ULONG UsbSts, FailSafe;

    //
    // Disable Interrupts and stop execution
    //
    EHCI_WRITE_REGISTER_ULONG (EHCI_USBINTR, 0);

    GetCommandRegister(&UsbCmd);
    UsbCmd.Run = FALSE;
    SetCommandRegister(&UsbCmd);

    for (FailSafe = 100; FailSafe > 1; FailSafe--)
    {
        KeStallExecutionProcessor(10);
        UsbSts = EHCI_READ_REGISTER_ULONG(EHCI_USBSTS);
        if (UsbSts & EHCI_STS_HALT)
        {
            break;
        }
    }

    if (!(UsbSts & EHCI_STS_HALT))
    {
        DPRINT1("EHCI ERROR: Controller is not responding to Stop request!\n");
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
CUSBHardwareDevice::ResetController(void)
{
    EHCI_USBCMD_CONTENT UsbCmd;
    ULONG FailSafe;

    GetCommandRegister(&UsbCmd);
    UsbCmd.HCReset = TRUE;
    SetCommandRegister(&UsbCmd);

    for (FailSafe = 100; FailSafe > 1; FailSafe--)
    {
        KeStallExecutionProcessor(100);
        GetCommandRegister(&UsbCmd);
        if (!UsbCmd.HCReset)
            break;
    }

    if (UsbCmd.HCReset)
    {
        DPRINT1("EHCI ERROR: Controller is not responding to reset request!\n");
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::ResetPort(
    IN ULONG PortIndex)
{
    ULONG PortStatus;
    LARGE_INTEGER Timeout;

    if (PortIndex > m_Capabilities.HCSParams.PortCount)
        return STATUS_UNSUCCESSFUL;

    PortStatus = EHCI_READ_REGISTER_ULONG(EHCI_PORTSC + (4 * PortIndex));

    ASSERT(!EHCI_IS_LOW_SPEED(PortStatus));
    ASSERT(PortStatus & EHCI_PRT_CONNECTED);

    //
    // Reset and clean enable
    //
    PortStatus |= EHCI_PRT_RESET;
    PortStatus &= EHCI_PORTSC_DATAMASK;
    EHCI_WRITE_REGISTER_ULONG(EHCI_PORTSC + (4 * PortIndex), PortStatus);

    //
    // delay is 50 ms for port reset as per USB 2.0 spec
    //
    Timeout.QuadPart = 50;
    DPRINT1("Waiting %lu milliseconds for port reset\n", Timeout.LowPart);

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

NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::GetPortStatus(
    ULONG PortId,
    OUT USHORT *PortStatus,
    OUT USHORT *PortChange)
{
    ULONG Value;
    USHORT Status = 0, Change = 0;

    if (PortId > m_Capabilities.HCSParams.PortCount)
        return STATUS_UNSUCCESSFUL;

    //
    // Get the value of the Port Status and Control Register
    //
    Value = EHCI_READ_REGISTER_ULONG(EHCI_PORTSC + (4 * PortId));

    //
    // If the PowerPortControl is 0 then host controller does not have power control switches
    if (!m_Capabilities.HCSParams.PortPowerControl)
    {
        Status |= USB_PORT_STATUS_POWER;
    }
    else
    {
        // Check the value of PortPower
        if (Value & EHCI_PRT_POWER)
        {
            Status |= USB_PORT_STATUS_POWER;
        }
    }

    // Get Connected Status
    if (Value & EHCI_PRT_CONNECTED)
    {
        Status |= USB_PORT_STATUS_CONNECT;

        // EHCI only supports high speed
        Status |= USB_PORT_STATUS_HIGH_SPEED;
    }

    // Get Enabled Status
    if (Value & EHCI_PRT_ENABLED)
        Status |= USB_PORT_STATUS_ENABLE;

    // Is it suspended?
    if (Value & EHCI_PRT_SUSPEND)
        Status |= USB_PORT_STATUS_SUSPEND;

    // a overcurrent is active?
    if (Value & EHCI_PRT_OVERCURRENTACTIVE)
        Status |= USB_PORT_STATUS_OVER_CURRENT;

    // In a reset state?
    if ((Value & EHCI_PRT_RESET) || m_PortResetInProgress[PortId])
    {
        Status |= USB_PORT_STATUS_RESET;
        Change |= USB_PORT_STATUS_RESET;
    }

    // This indicates a connect or disconnect
    if (Value & EHCI_PRT_CONNECTSTATUSCHANGE)
        Change |= USB_PORT_STATUS_CONNECT;

    // This is set to indicate a critical port error
    if (Value & EHCI_PRT_ENABLEDSTATUSCHANGE)
        Change |= USB_PORT_STATUS_ENABLE;

    *PortStatus = Status;
    *PortChange = Change;

    return STATUS_SUCCESS;
}

NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::ClearPortStatus(
    ULONG PortId,
    ULONG Status)
{
    ULONG Value;
    LARGE_INTEGER Timeout;

    DPRINT("CUSBHardwareDevice::ClearPortStatus PortId %x Feature %x\n", PortId, Status);

    if (PortId > m_Capabilities.HCSParams.PortCount)
        return STATUS_UNSUCCESSFUL;

    if (Status == C_PORT_RESET)
    {
        // reset done
        m_PortResetInProgress[PortId] = FALSE;

        // Clear reset
        Value = EHCI_READ_REGISTER_ULONG(EHCI_PORTSC + (4 * PortId));
        Value &= (EHCI_PORTSC_DATAMASK | EHCI_PRT_ENABLED);
        Value &= ~EHCI_PRT_RESET;
        EHCI_WRITE_REGISTER_ULONG(EHCI_PORTSC + (4 * PortId), Value);

        //
        // wait for reset bit to clear
        //
        do
        {
            Value = EHCI_READ_REGISTER_ULONG(EHCI_PORTSC + (4 * PortId));

            if (!(Value & EHCI_PRT_RESET))
                break;

            KeStallExecutionProcessor(20);
        } while (TRUE);

        //
        // delay is 50 ms
        //
        Timeout.QuadPart = 50;
        DPRINT1("Waiting %lu milliseconds for port to recover after reset\n", Timeout.LowPart);

        //
        // convert to 100 ns units (absolute)
        //
        Timeout.QuadPart *= -10000;

        //
        // perform the wait
        //
        KeDelayExecutionThread(KernelMode, FALSE, &Timeout);

        //
        // check the port status after reset
        //
        Value = EHCI_READ_REGISTER_ULONG(EHCI_PORTSC + (4 * PortId));
        if (!(Value & EHCI_PRT_CONNECTED))
        {
            DPRINT1("No device is here after reset. Bad controller/device?\n");
            return STATUS_UNSUCCESSFUL;
        }
        else if (EHCI_IS_LOW_SPEED(Value))
        {
            DPRINT1("Low speed device connected. Releasing ownership\n");
            EHCI_WRITE_REGISTER_ULONG(EHCI_PORTSC + (4 * PortId), Value | EHCI_PRT_RELEASEOWNERSHIP);
            return STATUS_DEVICE_NOT_CONNECTED;
        }
        else if (!(Value & EHCI_PRT_ENABLED))
        {
            DPRINT1("Full speed device connected. Releasing ownership\n");
            EHCI_WRITE_REGISTER_ULONG(EHCI_PORTSC + (4 * PortId), Value | EHCI_PRT_RELEASEOWNERSHIP);
            return STATUS_DEVICE_NOT_CONNECTED;
        }
        else
        {
            DPRINT1("High speed device connected\n");
            return STATUS_SUCCESS;
        }
    }
    else if (Status == C_PORT_CONNECTION)
    {
        //
        // reset status change bits
        //
        Value = EHCI_READ_REGISTER_ULONG(EHCI_PORTSC + (4 * PortId));
        EHCI_WRITE_REGISTER_ULONG(EHCI_PORTSC + (4 * PortId), Value);

        if (Value & EHCI_PRT_CONNECTED)
        {
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

    return STATUS_SUCCESS;
}


NTSTATUS
STDMETHODCALLTYPE
CUSBHardwareDevice::SetPortFeature(
    ULONG PortId,
    ULONG Feature)
{
    DPRINT("CUSBHardwareDevice::SetPortFeature\n");

    if (PortId > m_Capabilities.HCSParams.PortCount)
        return STATUS_UNSUCCESSFUL;

    if (Feature == PORT_ENABLE)
    {
        //
        // FIXME: EHCI Ports can only be disabled via reset
        //
        DPRINT1("PORT_ENABLE not supported for EHCI\n");
    }

    if (Feature == PORT_RESET)
    {
        //
        // call the helper
        //
        ResetPort(PortId);

        // reset in progress
        m_PortResetInProgress[PortId] = TRUE;

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

    if (Feature == PORT_POWER)
    {
        if (m_Capabilities.HCSParams.PortPowerControl)
        {
            ULONG Value;
            LARGE_INTEGER Timeout;

            //
            // enable port power
            //
            Value = EHCI_READ_REGISTER_ULONG(EHCI_PORTSC + (4 * PortId)) | EHCI_PRT_POWER;
            EHCI_WRITE_REGISTER_ULONG(EHCI_PORTSC + (4 * PortId), Value);

            //
            // delay is 20 ms
            //
            Timeout.QuadPart = 20;
            DPRINT1("Waiting %lu milliseconds for port power up\n", Timeout.LowPart);

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
    return STATUS_SUCCESS;
}

VOID
STDMETHODCALLTYPE
CUSBHardwareDevice::SetAsyncListRegister(
    ULONG PhysicalAddress)
{
    EHCI_WRITE_REGISTER_ULONG(EHCI_ASYNCLISTBASE, PhysicalAddress);
}

VOID
STDMETHODCALLTYPE
CUSBHardwareDevice::SetPeriodicListRegister(
    ULONG PhysicalAddress)
{
    //
    // store physical address
    //
    m_SyncFramePhysAddr = PhysicalAddress;
}

struct _QUEUE_HEAD *
STDMETHODCALLTYPE
CUSBHardwareDevice::GetAsyncListQueueHead()
{
    return AsyncQueueHead;
}

ULONG
STDMETHODCALLTYPE
CUSBHardwareDevice::GetPeriodicListRegister()
{
    UNIMPLEMENTED
    return NULL;
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

BOOLEAN
NTAPI
InterruptServiceRoutine(
    IN PKINTERRUPT  Interrupt,
    IN PVOID  ServiceContext)
{
    CUSBHardwareDevice *This;
    ULONG CStatus;

    This = (CUSBHardwareDevice*) ServiceContext;
    CStatus = This->EHCI_READ_REGISTER_ULONG(EHCI_USBSTS);

    CStatus &= (EHCI_ERROR_INT | EHCI_STS_INT | EHCI_STS_IAA | EHCI_STS_PCD | EHCI_STS_FLR);
    DPRINT("InterruptServiceRoutine CStatus %lx\n", CStatus);

    //
    // Check that it belongs to EHCI
    //
    if (!CStatus)
        return FALSE;

    //
    // Clear the Status
    //
    This->EHCI_WRITE_REGISTER_ULONG(EHCI_USBSTS, CStatus);

    if (CStatus & EHCI_STS_FATAL)
    {
        This->StopController();
        DPRINT1("EHCI: Host System Error!\n");
        return TRUE;
    }

    if (CStatus & EHCI_ERROR_INT)
    {
        DPRINT1("EHCI Status = 0x%x\n", CStatus);
    }

    if (CStatus & EHCI_STS_HALT)
    {
        DPRINT1("Host Error Unexpected Halt\n");
        // FIXME: Reset controller\n");
        return TRUE;
    }

    KeInsertQueueDpc(&This->m_IntDpcObject, This, (PVOID)CStatus);
    return TRUE;
}

VOID NTAPI
EhciDefferedRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2)
{
    CUSBHardwareDevice *This;
    ULONG CStatus, PortStatus, PortCount, i, ShouldRingDoorBell, QueueSCEWorkItem;
    NTSTATUS Status = STATUS_SUCCESS;
    EHCI_USBCMD_CONTENT UsbCmd;

    This = (CUSBHardwareDevice*) SystemArgument1;
    CStatus = (ULONG) SystemArgument2;

    DPRINT("EhciDefferedRoutine CStatus %lx\n", CStatus);

    //
    // check for completion of async schedule
    //
    if (CStatus & (EHCI_STS_RECL| EHCI_STS_INT | EHCI_ERROR_INT))
    {
        //
        // check if there is a door bell ring in progress
        //
        if (This->m_DoorBellRingInProgress == FALSE)
        {
            if (CStatus & EHCI_ERROR_INT)
            {
                //
                // controller reported error
                //
                DPRINT1("CStatus %lx\n", CStatus);
                //ASSERT(FALSE);
            }

            //
            // inform IUSBQueue of a completed queue head
            //
            This->m_UsbQueue->InterruptCallback(Status, &ShouldRingDoorBell);

            //
            // was a queue head completed?
            //
            if (ShouldRingDoorBell)
            {
                //
                // set door ring bell in progress status flag
                //
                This->m_DoorBellRingInProgress = TRUE;

                //
                // get command register
                //
                This->GetCommandRegister(&UsbCmd);

                //
                // set door rang bell bit
                //
                UsbCmd.DoorBell = TRUE;

                //
                // update command status
                //
                This->SetCommandRegister(&UsbCmd);
            }
        }
    }

    //
    // check if the controller has acknowledged the door bell
    //
    if (CStatus & EHCI_STS_IAA)
    {
        //
        // controller has acknowledged, assert we rang the bell
        //
        PC_ASSERT(This->m_DoorBellRingInProgress == TRUE);

        //
        // now notify IUSBQueue that it can free completed requests
        //
        This->m_UsbQueue->CompleteAsyncRequests();

        //
        // door ring bell completed
        //
        This->m_DoorBellRingInProgress = FALSE;
    }

    This->GetDeviceDetails(NULL, NULL, &PortCount, NULL);
    if (CStatus & EHCI_STS_PCD)
    {
        QueueSCEWorkItem = FALSE;
        for (i = 0; i < PortCount; i++)
        {
            PortStatus = This->EHCI_READ_REGISTER_ULONG(EHCI_PORTSC + (4 * i));

            //
            // Device connected or removed
            //
            if (PortStatus & EHCI_PRT_CONNECTSTATUSCHANGE)
            {
                if (PortStatus & EHCI_PRT_CONNECTED)
                {
                    DPRINT1("Device connected on port %lu\n", i);

                    if (This->m_Capabilities.HCSParams.CHCCount)
                    {
                        if (PortStatus & EHCI_PRT_ENABLED)
                        {
                            DPRINT1("Misbeaving controller. Port should be disabled at this point\n");
                        }

                        if (EHCI_IS_LOW_SPEED(PortStatus))
                        {
                            DPRINT1("Low speed device connected. Releasing ownership\n");
                            This->EHCI_WRITE_REGISTER_ULONG(EHCI_PORTSC + (4 * i), PortStatus | EHCI_PRT_RELEASEOWNERSHIP);
                            continue;
                        }
                    }

                    //
                    // work to do
                    //
                    QueueSCEWorkItem = TRUE;
                }
                else
                {
                    DPRINT1("Device disconnected on port %lu\n", i);

                    //
                    // work to do
                    //
                    QueueSCEWorkItem = TRUE;
                }
            }
        }

        //
        // is there a status change callback and a high speed device connected / disconnected
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

    This = new(NonPagedPool, TAG_USBEHCI) CUSBHardwareDevice(0);

    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->AddRef();

    // return result
    *OutHardware = (PUSBHARDWAREDEVICE)This;

    return STATUS_SUCCESS;
}
