/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/hcd_controller.cpp
 * PURPOSE:     USB EHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#define INITGUID
#include "usbehci.h"

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
    NTSTATUS GetDeviceDetails(PULONG VendorId, PULONG DeviceId, PULONG NumberOfPorts, PULONG Speed);
    NTSTATUS GetDmaMemoryManager(OUT struct IDMAMemoryManager **OutMemoryManager);
    NTSTATUS GetUSBQueue(OUT struct IUSBQueue **OutUsbQueue);
    NTSTATUS ResetController();
    KIRQL AcquireDeviceLock(void);
    VOID ReleaseDeviceLock(KIRQL OldLevel);
    // local
    BOOLEAN InterruptService();

    // friend function
    friend BOOLEAN NTAPI InterruptServiceRoutine(IN PKINTERRUPT  Interrupt, IN PVOID  ServiceContext);

    // constructor / destructor
    CUSBHardwareDevice(IUnknown *OuterUnknown){}
    virtual ~CUSBHardwareDevice(){}

protected:
    LONG m_Ref;
    PDRIVER_OBJECT m_DriverObject;
    PDEVICE_OBJECT m_PhysicalDeviceObject;
    PDEVICE_OBJECT m_FunctionalDeviceObject;
    PDEVICE_OBJECT m_NextDeviceObject;
    KSPIN_LOCK m_Lock;
    PKINTERRUPT m_Interrupt;
    PULONG m_Base;
    PDMA_ADAPTER m_Adapter;
    ULONG m_MapRegisters;
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
    UNICODE_STRING GuidString;

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

    DPRINT1("CUSBHardwareDevice::Initialize\n");

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
                //FIXME: query capabilities and update m_Base
                //
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

    return STATUS_SUCCESS;
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
    OUT OPTIONAL PULONG VendorId,
    OUT OPTIONAL PULONG DeviceId,
    OUT OPTIONAL PULONG NumberOfPorts,
    OUT OPTIONAL PULONG Speed)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
CUSBHardwareDevice::GetDmaMemoryManager(
    OUT struct IDMAMemoryManager **OutMemoryManager)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
CUSBHardwareDevice::GetUSBQueue(
    OUT struct IUSBQueue **OutUsbQueue)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
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
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
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
    UNIMPLEMENTED
    return FALSE;
}

NTSTATUS
CreateUSBHardware(
    PUSBHARDWAREDEVICE *OutHardware)
{
    PUSBHARDWAREDEVICE This;

    This = new(NonPagedPool, 0) CUSBHardwareDevice(0);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->AddRef();

    // return result
    *OutHardware = (PUSBHARDWAREDEVICE)This;

    return STATUS_SUCCESS;
}
