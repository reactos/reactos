/* $Id: videoprt.c,v 1.2 2003/02/17 21:24:42 gvg Exp $
 *
 * VideoPort driver
 *   Written by Rex Jolliff
 *
 * FIXME:
 * There are two ugly and temporary hacks in this file, to get the VMware driver to
 * work.
 * First, the miniport driver is allowed to call VideoPortInitialize() multiple times.
 * VideoPortInitialize() will create a device and then call the miniport's 
 * HwFindAdapter(). If that call returns with an error code, the device will be
 * deleted. The next time VideoPortInitialize() is called, it will be create a
 * new device with the same name as the first time. The first device was deleted so
 * this shouldn't be a problem, the device is created successfully. Initialization
 * then continues.
 * The problems start when it's time to start the device. When the driver is opened,
 * the caller will receive a pointer to the FIRST device, the one which was later
 * deleted. This is propably due to a problem in the Io subsystem which needs to
 * be investigated. To get around this, a pointer is kept to the last successfully
 * opened device (pdoLastOpened) and this device is used instead of the pointer
 * passed in.
 * The second problem has to do with resources. The miniport driver will call
 * VideoPortGetDeviceBase() to map a physical address to a virtual address. Later,
 * it will call VideoPortMapMemory() with the same physical address. It should
 * map to the same virtual address, but I couldn't get this to work at the moment.
 * So, as a workaround, a maximum of 2 physical addresses with their corresponding
 * virtual addresses saved. They are filled by VideoPortGetDeviceBase() and
 * looked-up by VideoPortMapMemory().
 */

#include <errors.h>
#include <ddk/ntddk.h>
#include <ddk/ntddvid.h>

#include "../../../ntoskrnl/include/internal/v86m.h"

#include "videoprt.h"

#define NDEBUG
#include <debug.h>

#define VERSION "0.0.0"

static VOID STDCALL VidStartIo(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
static NTSTATUS STDCALL VidDispatchOpenClose(IN PDEVICE_OBJECT pDO, IN PIRP Irp);
static NTSTATUS STDCALL VidDispatchDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

static BOOLEAN CsrssInitialized = FALSE;
static HANDLE CsrssHandle = 0;
static struct _EPROCESS* Csrss = NULL;

/* FIXME: see file header */
static PDEVICE_OBJECT pdoLastOpened;
static PHYSICAL_ADDRESS Phys1, Phys2;
static PVOID Virt1, Virt2;

PBYTE ReturnCsrssAddress(void)
{
  DPRINT("ReturnCsrssAddress()\n");
  return (PBYTE)Csrss;
}

//  -------------------------------------------------------  Public Interface

//    DriverEntry
//
//  DESCRIPTION:
//    This function initializes the driver.
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN  PDRIVER_OBJECT   DriverObject  System allocated Driver Object
//                                       for this driver
//    IN  PUNICODE_STRING  RegistryPath  Name of registry driver service 
//                                       key
//
//  RETURNS:
//    NTSTATUS  

STDCALL NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
  DPRINT("DriverEntry()\n");
  return(STATUS_SUCCESS);
}

VOID
VideoPortDebugPrint(IN ULONG DebugPrintLevel,
                    IN PCHAR DebugMessage, ...)
{
	char Buffer[256];
	va_list ap;

/*
	if (DebugPrintLevel > InternalDebugLevel)
		return;
*/
	va_start (ap, DebugMessage);
	vsprintf (Buffer, DebugMessage, ap);
	va_end (ap);

	DbgPrint (Buffer);
}

VP_STATUS 
STDCALL
VideoPortDisableInterrupt(IN PVOID  HwDeviceExtension)
{
  DPRINT("VideoPortDisableInterrupt\n");
  UNIMPLEMENTED;
}

VP_STATUS 
STDCALL
VideoPortEnableInterrupt(IN PVOID  HwDeviceExtension)
{
  DPRINT("VideoPortEnableInterrupt\n");
  UNIMPLEMENTED;
}

VOID 
STDCALL
VideoPortFreeDeviceBase(IN PVOID  HwDeviceExtension, 
                        IN PVOID  MappedAddress)
{
  DPRINT1("VideoPortFreeDeviceBase not implemented\n");
}

ULONG 
STDCALL
VideoPortGetBusData(IN PVOID  HwDeviceExtension,
                    IN BUS_DATA_TYPE  BusDataType,
                    IN ULONG  SlotNumber,
                    OUT PVOID  Buffer,
                    IN ULONG  Offset,
                    IN ULONG  Length)
{
  ULONG BusNumber = 0;

  DPRINT("VideoPortGetBusData\n");
  return HalGetBusDataByOffset(BusDataType, 
                               BusNumber, 
                               SlotNumber, 
                               Buffer, 
                               Offset, 
                               Length);
}

UCHAR 
STDCALL
VideoPortGetCurrentIrql(VOID)
{
  DPRINT("VideoPortGetCurrentIrql\n");
  return KeGetCurrentIrql();
}

PVOID 
STDCALL
VideoPortGetDeviceBase(IN PVOID  HwDeviceExtension,
                       IN PHYSICAL_ADDRESS  IoAddress,
                       IN ULONG  NumberOfUchars,
                       IN UCHAR  InIoSpace)
{
  PHYSICAL_ADDRESS TranslatedAddress;
  PVOID Virtual;
  ULONG AddressSpace;
  PVIDEOPORT_EXTENSION_DATA ExtensionData = 
    MPExtensionToVPExtension(HwDeviceExtension);

  DPRINT("VideoPortGetDeviceBase\n");
  AddressSpace = (InIoSpace ? 1 : 0);

  if (HalTranslateBusAddress(PCIBus, 0, IoAddress, &AddressSpace, &TranslatedAddress))
    {
      if (AddressSpace)
	{
	return (PVOID)(DWORD)(TranslatedAddress.QuadPart);
	}
      else
	{
          Virtual = MmMapIoSpace(TranslatedAddress, NumberOfUchars, MmNonCached);
	  /* FIXME: see file header */
	  DPRINT("Mapped 0x%08x to 0x%08x\n", IoAddress.u.LowPart, Virtual);
	  if (0 == Phys1.QuadPart)
	    {
	      Virt1 = Virtual;
	    }
	  if (0 == Phys2.QuadPart)
	    {
	      Virt2 = Virtual;
	    }
	}
    }
  else
    {
    return NULL;
    }
}

VP_STATUS 
STDCALL
VideoPortGetDeviceData(IN PVOID  HwDeviceExtension,
                       IN VIDEO_DEVICE_DATA_TYPE  DeviceDataType,
                       IN PMINIPORT_QUERY_DEVICE_ROUTINE  CallbackRoutine,
                       IN PVOID Context)
{
  DPRINT("VideoPortGetDeviceData\n");
  UNIMPLEMENTED;
}

VP_STATUS 
STDCALL
VideoPortGetAccessRanges(IN PVOID  HwDeviceExtension,
                         IN ULONG  NumRequestedResources,
                         IN PIO_RESOURCE_DESCRIPTOR  RequestedResources OPTIONAL,
                         IN ULONG  NumAccessRanges,
                         IN PVIDEO_ACCESS_RANGE  AccessRanges,
                         IN PVOID  VendorId,
                         IN PVOID  DeviceId,
                         IN PULONG  Slot)
{
  PCI_SLOT_NUMBER PciSlotNumber;
  BOOLEAN FoundDevice;
  ULONG FunctionNumber;
  PCI_COMMON_CONFIG Config;
  UINT BusNumber = 0;
  PCM_RESOURCE_LIST AllocatedResources;
  NTSTATUS Status;
  UINT AssignedCount;
  CM_FULL_RESOURCE_DESCRIPTOR *FullList;
  CM_PARTIAL_RESOURCE_DESCRIPTOR *Descriptor;

  DPRINT("VideoPortGetAccessRanges\n");
  if (0 == NumRequestedResources)
    {
      DPRINT("Looking for VendorId 0x%04x DeviceId 0x%04x\n", (int)*((USHORT *) VendorId),
             (int)*((USHORT *) DeviceId));
      FoundDevice = FALSE;
      PciSlotNumber.u.AsULONG = *Slot;
      for (FunctionNumber = 0; ! FoundDevice && FunctionNumber < 8; FunctionNumber++)
	{
	  PciSlotNumber.u.bits.FunctionNumber = FunctionNumber;
	  if (sizeof(PCI_COMMON_CONFIG) ==
	      HalGetBusDataByOffset(PCIConfiguration, BusNumber, PciSlotNumber.u.AsULONG,
	                            &Config, 0, sizeof(PCI_COMMON_CONFIG)))
	    {
	      DPRINT("Slot 0x%02x (Device %d Function %d) VendorId 0x%04x DeviceId 0x%04x\n",
	             PciSlotNumber.u.AsULONG, PciSlotNumber.u.bits.DeviceNumber,
	             PciSlotNumber.u.bits.FunctionNumber, Config.VendorID, Config.DeviceID);
	      FoundDevice = (Config.VendorID == *((USHORT *) VendorId) &&
	                     Config.DeviceID == *((USHORT *) DeviceId));
	    }
	}
      if (! FoundDevice)
	{
	  return STATUS_UNSUCCESSFUL;
	}
      Status = HalAssignSlotResources(NULL, NULL, NULL, NULL, PCIBus, BusNumber,
                                      PciSlotNumber.u.AsULONG, &AllocatedResources);
      if (! NT_SUCCESS(Status))
	{
	  return Status;
	}
      AssignedCount = 0;
      for (FullList = AllocatedResources->List;
           FullList < AllocatedResources->List + AllocatedResources->Count &&
           AssignedCount < NumAccessRanges;
           FullList++)
	{
	  assert(FullList->InterfaceType == PCIBus &&
	         FullList->BusNumber == BusNumber &&
	         1 == FullList->PartialResourceList.Version &&
	         1 == FullList->PartialResourceList.Revision);
	  for (Descriptor = FullList->PartialResourceList.PartialDescriptors;
	       Descriptor < FullList->PartialResourceList.PartialDescriptors + FullList->PartialResourceList.Count &&
	       AssignedCount < NumAccessRanges;
	       Descriptor++)
	    {
	      if (CmResourceTypeMemory == Descriptor->Type)
		{
		  DPRINT("Memory range starting at 0x%08x length 0x%08x\n",
		         Descriptor->u.Memory.Start.u.LowPart, Descriptor->u.Memory.Length);
		  AccessRanges[AssignedCount].RangeStart = Descriptor->u.Memory.Start;
		  AccessRanges[AssignedCount].RangeLength = Descriptor->u.Memory.Length;
		  AccessRanges[AssignedCount].RangeInIoSpace = 0;
		  AccessRanges[AssignedCount].RangeVisible = 0; /* FIXME: Just guessing */
		  AccessRanges[AssignedCount].RangeShareable =
		    (CmResourceShareShared == Descriptor->ShareDisposition);
		}
	      else if (CmResourceTypePort == Descriptor->Type)
		{
		  DPRINT("Port range starting at 0x%04x length %d\n",
		         Descriptor->u.Memory.Start.u.LowPart, Descriptor->u.Memory.Length);
		  AccessRanges[AssignedCount].RangeStart = Descriptor->u.Port.Start;
		  AccessRanges[AssignedCount].RangeLength = Descriptor->u.Port.Length;
		  AccessRanges[AssignedCount].RangeInIoSpace = 1;
		  AccessRanges[AssignedCount].RangeVisible = 0; /* FIXME: Just guessing */
		  AccessRanges[AssignedCount].RangeShareable = 0;
		}
	      else
		{
		  ExFreePool(AllocatedResources);
		  return STATUS_UNSUCCESSFUL;
		}
	      AssignedCount++;
	    }
	}
      ExFreePool(AllocatedResources);
    }

  return STATUS_SUCCESS;
}

VP_STATUS 
STDCALL
VideoPortGetRegistryParameters(IN PVOID  HwDeviceExtension,
                               IN PWSTR  ParameterName,
                               IN UCHAR  IsParameterFileName,
                               IN PMINIPORT_GET_REGISTRY_ROUTINE  GetRegistryRoutine,
                               IN PVOID  Context)
{
  DPRINT("VideoPortGetRegistryParameters\n");
  DPRINT("ParameterName %S\n", ParameterName);
  UNIMPLEMENTED;
}

ULONG STDCALL
VideoPortInitialize(IN PVOID  Context1,
                    IN PVOID  Context2,
                    IN PVIDEO_HW_INITIALIZATION_DATA  HwInitializationData,
                    IN PVOID  HwContext)
{
  UCHAR  Again;
  WCHAR  DeviceBuffer[20];
  WCHAR  SymlinkBuffer[20];
  NTSTATUS  Status;
  PDRIVER_OBJECT  MPDriverObject = (PDRIVER_OBJECT) Context1;
  PDEVICE_OBJECT  MPDeviceObject;
  VIDEO_PORT_CONFIG_INFO  ConfigInfo;
  PVIDEOPORT_EXTENSION_DATA  ExtensionData;
  ULONG DeviceNumber = 0;
  UNICODE_STRING DeviceName;
  UNICODE_STRING SymlinkName;
  CLIENT_ID Cid;

  DPRINT("VideoPortInitialize\n");

  /*  Build Dispatch table from passed data  */
  MPDriverObject->DriverStartIo = (PDRIVER_STARTIO) HwInitializationData->HwStartIO;

  /*  Create a unicode device name  */
  Again = FALSE;
  do
    {
      swprintf(DeviceBuffer, L"\\Device\\Video%lu", DeviceNumber);
      RtlInitUnicodeString(&DeviceName, DeviceBuffer);

      /*  Create the device  */
      Status = IoCreateDevice(MPDriverObject,
                              HwInitializationData->HwDeviceExtensionSize +
                                sizeof(VIDEOPORT_EXTENSION_DATA),
                              &DeviceName,
                              FILE_DEVICE_VIDEO,
                              0,
                              TRUE,
                              &MPDeviceObject);
      if (!NT_SUCCESS(Status))
        {
          DPRINT("IoCreateDevice call failed with status 0x%08x\n", Status);
          return Status;
        }

      MPDriverObject->DeviceObject = MPDeviceObject;

      /* initialize the miniport drivers dispatch table */
      MPDriverObject->MajorFunction[IRP_MJ_CREATE] = VidDispatchOpenClose;
      MPDriverObject->MajorFunction[IRP_MJ_CLOSE] = VidDispatchOpenClose;
      MPDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = VidDispatchDeviceControl;

      /* create symbolic link "\??\DISPLAYx" */
      swprintf(SymlinkBuffer, L"\\??\\DISPLAY%lu", DeviceNumber+1);
      RtlInitUnicodeString (&SymlinkName,
                            SymlinkBuffer);
      IoCreateSymbolicLink (&SymlinkName,
                            &DeviceName);

      ExtensionData = 
        (PVIDEOPORT_EXTENSION_DATA) MPDeviceObject->DeviceExtension;
      ExtensionData->DeviceObject = MPDeviceObject;
      ExtensionData->HwInitialize = HwInitializationData->HwInitialize;
      
      /*  Set the buffering strategy here...  */
      /*  If you change this, remember to change VidDispatchDeviceControl too */
      MPDeviceObject->Flags |= DO_BUFFERED_IO;

      RtlZeroMemory(&ConfigInfo, sizeof(VIDEO_PORT_CONFIG_INFO));
      ConfigInfo.Length = sizeof(VIDEO_PORT_CONFIG_INFO);
      ConfigInfo.AdapterInterfaceType = HwInitializationData->AdapterInterfaceType;
      /*  Call HwFindAdapter entry point  */
      /* FIXME: Need to figure out what string to pass as param 3  */
      Status = HwInitializationData->HwFindAdapter(VPExtensionToMPExtension(ExtensionData),
                                                   Context2,
                                                   NULL,
                                                   &ConfigInfo,
                                                   &Again);
      if (NO_ERROR != Status)
        {
          DPRINT("HwFindAdapter call failed with error %d\n", Status);
          IoDeleteDevice(MPDeviceObject);
	  IoDeleteSymbolicLink(&SymlinkName);

          return  Status;
        }
      /* FIXME: see file header */
      pdoLastOpened = MPDeviceObject;
      DPRINT("Setting last opened device to 0x%08x\n", pdoLastOpened);

      /* FIXME: Allocate hardware resources for device  */

      /*  Allocate interrupt for device  */
      if (HwInitializationData->HwInterrupt != NULL &&
          !(ConfigInfo.BusInterruptLevel == 0 &&
            ConfigInfo.BusInterruptVector == 0))
        {
#if 0
          ExtensionData->IRQL = ConfigInfo.BusInterruptLevel;
          ExtensionData->InterruptLevel = 
            HalGetInterruptVector(ConfigInfo.AdapterInterfaceType,
                                  ConfigInfo.SystemIoBusNumber,
                                  ConfigInfo.BusInterruptLevel,
                                  ConfigInfo.BusInterruptVector,
                                  &ExtensionData->IRQL,
                                  &ExtensionData->Affinity);
          KeInitializeSpinLock(&ExtensionData->InterruptSpinLock);
          Status = IoConnectInterrupt(&ExtensionData->InterruptObject,
                                      (PKSERVICE_ROUTINE)
                                        HwInitializationData->HwInterrupt,
                                      VPExtensionToMPExtension(ExtensionData),
                                      &ExtensionData->InterruptSpinLock,
                                      ExtensionData->InterruptLevel,
                                      ExtensionData->IRQL,
                                      ExtensionData->IRQL,
                                      ConfigInfo.InterruptMode,
                                      FALSE,
                                      ExtensionData->Affinity,
                                      FALSE);
          if (!NT_SUCCESS(Status))
            {
              DPRINT("IoConnectInterrupt failed with status 0x%08x\n", Status);
              IoDeleteDevice(MPDeviceObject);
              
              return Status;
            }
#endif
        }
      DeviceNumber++;
    }
  while (Again);

  /* FIXME: initialize timer routine for MP Driver  */
  if (HwInitializationData->HwTimer != NULL)
    {
      Status = IoInitializeTimer(MPDeviceObject,
                                 (PIO_TIMER_ROUTINE)
                                   HwInitializationData->HwTimer,
                                 VPExtensionToMPExtension(ExtensionData));
      if (!NT_SUCCESS(Status))
        {
          DPRINT("IoInitializeTimer failed with status 0x%08x\n", Status);
          
          if (HwInitializationData->HwInterrupt != NULL)
            {
              IoDisconnectInterrupt(ExtensionData->InterruptObject);
            }
          IoDeleteDevice(MPDeviceObject);
          
          return Status;
        }
    }

  return  STATUS_SUCCESS;
}

VP_STATUS STDCALL
VideoPortInt10(IN PVOID  HwDeviceExtension,
               IN PVIDEO_X86_BIOS_ARGUMENTS  BiosArguments)
{
  KV86M_REGISTERS Regs;
  NTSTATUS Status;

  DPRINT("VideoPortInt10\n");
  KeAttachProcess(Csrss);

  memset(&Regs, 0, sizeof(Regs));
  Regs.Eax = BiosArguments->Eax;
  Regs.Ebx = BiosArguments->Ebx;
  Regs.Ecx = BiosArguments->Ecx;
  Regs.Edx = BiosArguments->Edx;
  Regs.Esi = BiosArguments->Esi;
  Regs.Edi = BiosArguments->Edi;
  Regs.Ebp = BiosArguments->Ebp;
  Status = Ke386CallBios(0x10, &Regs);

  KeDetachProcess();

  return(Status);
}

VOID 
STDCALL
VideoPortLogError(IN PVOID  HwDeviceExtension,
                  IN PVIDEO_REQUEST_PACKET  Vrp OPTIONAL,
                  IN VP_STATUS  ErrorCode,
                  IN ULONG  UniqueId)
{
  DPRINT("VideoPortLogError\n");
  UNIMPLEMENTED;
}

VP_STATUS 
STDCALL
VideoPortMapBankedMemory(IN PVOID  HwDeviceExtension,
                         IN PHYSICAL_ADDRESS  PhysicalAddress,
                         IN PULONG  Length,
                         IN PULONG  InIoSpace,
                         OUT PVOID  *VirtualAddress,
                         IN ULONG  BankLength,
                         IN UCHAR  ReadWriteBank,
                         IN PBANKED_SECTION_ROUTINE  BankRoutine,
                         IN PVOID  Context)
{
  DPRINT("VideoPortMapBankedMemory\n");
  UNIMPLEMENTED;
}

VP_STATUS 
STDCALL
VideoPortMapMemory(IN PVOID  HwDeviceExtension,
                   IN PHYSICAL_ADDRESS  PhysicalAddress,
                   IN PULONG  Length,
                   IN PULONG  InIoSpace,
                   OUT PVOID  *VirtualAddress)
{
  PHYSICAL_ADDRESS TranslatedAddress;
  ULONG AddressSpace;
  PVIDEOPORT_EXTENSION_DATA ExtensionData = 
    MPExtensionToVPExtension(HwDeviceExtension);

  DPRINT("VideoPortMapMemory\n");

  /* FIXME: see file header */
  if (Phys1.QuadPart == PhysicalAddress.QuadPart)
    {
      DPRINT("Using saved mapping #1\n");
      *VirtualAddress = Virt1;
      return STATUS_SUCCESS;
    }
  if (Phys2.QuadPart == PhysicalAddress.QuadPart)
    {
      DPRINT("Using saved mapping #2\n");
      *VirtualAddress = Virt2;
      return STATUS_SUCCESS;
    }

  AddressSpace = (*InIoSpace ? 1 : 0);

  if (HalTranslateBusAddress(PCIBus, 0, PhysicalAddress, &AddressSpace, &TranslatedAddress))
    {
      if (AddressSpace)
	{
	*VirtualAddress = (PVOID)(DWORD)(TranslatedAddress.QuadPart);
	return STATUS_SUCCESS;
	}
      else
	{
	*VirtualAddress = MmMapIoSpace(TranslatedAddress, *Length, MmNonCached);
	return NULL != *VirtualAddress ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
	}
    }
  else
    {
    return STATUS_UNSUCCESSFUL;
    }
}

UCHAR 
STDCALL
VideoPortReadPortUchar(IN PUCHAR  Port)
{
  DPRINT("VideoPortReadPortUchar\n");
  return  READ_PORT_UCHAR(Port);
}

USHORT 
STDCALL
VideoPortReadPortUshort(IN PUSHORT Port)
{
  DPRINT("VideoPortReadPortUshort\n");
  return  READ_PORT_USHORT(Port);
}

ULONG 
STDCALL
VideoPortReadPortUlong(IN PULONG Port)
{
  DPRINT("VideoPortReadPortUlong\n");
  return  READ_PORT_ULONG(Port);
}

VOID 
STDCALL
VideoPortReadPortBufferUchar(IN PUCHAR  Port, 
                             OUT PUCHAR  Buffer, 
                             IN ULONG  Count)
{
  DPRINT("VideoPortReadPortBufferUchar\n");
  READ_PORT_BUFFER_UCHAR(Port, Buffer, Count);
}

VOID 
STDCALL
VideoPortReadPortBufferUshort(IN PUSHORT Port, 
                              OUT PUSHORT Buffer, 
                              IN ULONG Count)
{
  DPRINT("VideoPortReadPortBufferUshort\n");
  READ_PORT_BUFFER_USHORT(Port, Buffer, Count);
}

VOID 
STDCALL
VideoPortReadPortBufferUlong(IN PULONG Port, 
                             OUT PULONG Buffer, 
                             IN ULONG Count)
{
  DPRINT("VideoPortReadPortBufferUlong\n");
  READ_PORT_BUFFER_ULONG(Port, Buffer, Count);
}

UCHAR 
STDCALL
VideoPortReadRegisterUchar(IN PUCHAR Register)
{
  DPRINT("VideoPortReadPortRegisterUchar\n");
  return  READ_REGISTER_UCHAR(Register);
}

USHORT 
STDCALL
VideoPortReadRegisterUshort(IN PUSHORT Register)
{
  DPRINT("VideoPortReadPortRegisterUshort\n");
  return  READ_REGISTER_USHORT(Register);
}

ULONG 
STDCALL
VideoPortReadRegisterUlong(IN PULONG Register)
{
  DPRINT("VideoPortReadPortRegisterUlong\n");
  return  READ_REGISTER_ULONG(Register);
}

VOID 
STDCALL
VideoPortReadRegisterBufferUchar(IN PUCHAR  Register, 
                                 OUT PUCHAR  Buffer, 
                                 IN ULONG  Count)
{
  DPRINT("VideoPortReadPortRegisterBufferUchar\n");
  READ_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);
}

VOID 
STDCALL
VideoPortReadRegisterBufferUshort(IN PUSHORT  Register, 
                                  OUT PUSHORT  Buffer, 
                                  IN ULONG  Count)
{
  DPRINT("VideoPortReadPortRegisterBufferUshort\n");
  READ_REGISTER_BUFFER_USHORT(Register, Buffer, Count);
}

VOID 
STDCALL
VideoPortReadRegisterBufferUlong(IN PULONG  Register, 
                                 OUT PULONG  Buffer, 
                                 IN ULONG  Count)
{
  DPRINT("VideoPortReadPortRegisterBufferUlong\n");
  READ_REGISTER_BUFFER_ULONG(Register, Buffer, Count);
}

BOOLEAN 
STDCALL
VideoPortScanRom(IN PVOID  HwDeviceExtension, 
                 IN PUCHAR  RomBase,
                 IN ULONG  RomLength,
                 IN PUCHAR  String)
{
  DPRINT("VideoPortScanRom\n");
  UNIMPLEMENTED;
}

ULONG 
STDCALL
VideoPortSetBusData(IN PVOID  HwDeviceExtension,
                    IN BUS_DATA_TYPE  BusDataType,
                    IN ULONG  SlotNumber,
                    IN PVOID  Buffer,
                    IN ULONG  Offset,
                    IN ULONG  Length)
{
  DPRINT("VideoPortSetBusData\n");
  return  HalSetBusDataByOffset(BusDataType,
                                0,
                                SlotNumber,
                                Buffer,
                                Offset,
                                Length);
}

VP_STATUS 
STDCALL
VideoPortSetRegistryParameters(IN PVOID  HwDeviceExtension,
                               IN PWSTR  ValueName,
                               IN PVOID  ValueData,
                               IN ULONG  ValueLength)
{
  DPRINT1("VideoPortSetRegistryParameters not implemented\n");
  return NO_ERROR;
}

VP_STATUS 
STDCALL
VideoPortSetTrappedEmulatorPorts(IN PVOID  HwDeviceExtension,
                                 IN ULONG  NumAccessRanges,
                                 IN PVIDEO_ACCESS_RANGE  AccessRange)
{
  DPRINT("VideoPortSetTrappedEmulatorPorts\n");
  UNIMPLEMENTED;
}

VOID 
STDCALL
VideoPortStartTimer(IN PVOID  HwDeviceExtension)
{
  PVIDEOPORT_EXTENSION_DATA ExtensionData = 
    MPExtensionToVPExtension(HwDeviceExtension);

  DPRINT("VideoPortStartTimer\n");
  IoStartTimer(ExtensionData->DeviceObject);
}

VOID 
STDCALL
VideoPortStopTimer(IN PVOID  HwDeviceExtension)
{
  PVIDEOPORT_EXTENSION_DATA ExtensionData = 
    MPExtensionToVPExtension(HwDeviceExtension);

  DPRINT("VideoPortStopTimer\n");
  IoStopTimer(ExtensionData->DeviceObject);
}

BOOLEAN 
STDCALL
VideoPortSynchronizeExecution(IN PVOID  HwDeviceExtension,
                              IN VIDEO_SYNCHRONIZE_PRIORITY  Priority,
                              IN PMINIPORT_SYNCHRONIZE_ROUTINE  SynchronizeRoutine,
                              OUT PVOID  Context)
{
  DPRINT("VideoPortSynchronizeExecution\n");
  UNIMPLEMENTED;
}

VP_STATUS 
STDCALL
VideoPortUnmapMemory(IN PVOID  HwDeviceExtension,
                     IN PVOID  VirtualAddress,
                     IN HANDLE  ProcessHandle)
{
  DPRINT1("VideoPortUnmapMemory not implemented\n");
  return NO_ERROR;
}

VP_STATUS 
STDCALL
VideoPortVerifyAccessRanges(IN PVOID  HwDeviceExtension,
                            IN ULONG  NumAccessRanges,
                            IN PVIDEO_ACCESS_RANGE  AccessRanges)
{
  DPRINT1("VideoPortVerifyAccessRanges not implemented\n");
  return NO_ERROR;
}

VOID 
STDCALL
VideoPortWritePortUchar(IN PUCHAR  Port, 
                        IN UCHAR  Value)
{
  DPRINT("VideoPortWritePortUchar\n");
  WRITE_PORT_UCHAR(Port, Value);
}

VOID 
STDCALL
VideoPortWritePortUshort(IN PUSHORT  Port, 
                         IN USHORT  Value)
{
  DPRINT("VideoPortWritePortUshort\n");
  WRITE_PORT_USHORT(Port, Value);
}

VOID 
STDCALL
VideoPortWritePortUlong(IN PULONG Port, 
                        IN ULONG Value)
{
  DPRINT("VideoPortWritePortUlong\n");
  WRITE_PORT_ULONG(Port, Value);
}

VOID 
STDCALL
VideoPortWritePortBufferUchar(IN PUCHAR  Port, 
                              IN PUCHAR  Buffer, 
                              IN ULONG  Count)
{
  DPRINT("VideoPortWritePortBufferUchar\n");
  WRITE_PORT_BUFFER_UCHAR(Port, Buffer, Count);
}

VOID 
STDCALL
VideoPortWritePortBufferUshort(IN PUSHORT  Port, 
                               IN PUSHORT  Buffer, 
                               IN ULONG  Count)
{
  DPRINT("VideoPortWritePortBufferUshort\n");
  WRITE_PORT_BUFFER_USHORT(Port, Buffer, Count);
}

VOID 
STDCALL
VideoPortWritePortBufferUlong(IN PULONG  Port, 
                              IN PULONG  Buffer, 
                              IN ULONG  Count)
{
  DPRINT("VideoPortWritePortBufferUlong\n");
  WRITE_PORT_BUFFER_ULONG(Port, Buffer, Count);
}

VOID 
STDCALL
VideoPortWriteRegisterUchar(IN PUCHAR  Register, 
                            IN UCHAR  Value)
{
  DPRINT("VideoPortWriteRegisterUchar\n");
  WRITE_REGISTER_UCHAR(Register, Value);
}

VOID 
STDCALL
VideoPortWriteRegisterUshort(IN PUSHORT  Register, 
                             IN USHORT  Value)
{
  DPRINT("VideoPortWriteRegisterUshort\n");
  WRITE_REGISTER_USHORT(Register, Value);
}

VOID 
STDCALL
VideoPortWriteRegisterUlong(IN PULONG  Register, 
                            IN ULONG  Value)
{
  DPRINT("VideoPortWriteRegisterUlong\n");
  WRITE_REGISTER_ULONG(Register, Value);
}

VOID 
STDCALL
VideoPortWriteRegisterBufferUchar(IN PUCHAR  Register, 
                                  IN PUCHAR  Buffer, 
                                  IN ULONG  Count)
{
  DPRINT("VideoPortWriteRegisterBufferUchar\n");
  WRITE_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);
}

VOID STDCALL
VideoPortWriteRegisterBufferUshort(IN PUSHORT  Register,
                                   IN PUSHORT  Buffer,
                                   IN ULONG  Count)
{
  DPRINT("VideoPortWriteRegisterBufferUshort\n");
  WRITE_REGISTER_BUFFER_USHORT(Register, Buffer, Count);
}

VOID STDCALL
VideoPortWriteRegisterBufferUlong(IN PULONG  Register,
                                  IN PULONG  Buffer,
                                  IN ULONG  Count)
{
  DPRINT("VideoPortWriteRegisterBufferUlong\n");
  WRITE_REGISTER_BUFFER_ULONG(Register, Buffer, Count);
}

VOID STDCALL
VideoPortZeroDeviceMemory(OUT PVOID  Destination,
			  IN ULONG  Length)
{
  DPRINT("VideoPortZeroDeviceMemory\n");
  UNIMPLEMENTED;
}


//  -------------------------------------------  Nondiscardable statics

//    VidDispatchOpenClose
//
//  DESCRIPTION:
//    Answer requests for Open/Close calls: a null operation
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    Standard dispatch arguments
//
//  RETURNS:
//    NTSTATUS
//

static NTSTATUS STDCALL
VidDispatchOpenClose(IN PDEVICE_OBJECT pDO,
                     IN PIRP Irp)
{
  PIO_STACK_LOCATION IrpStack;
  PVIDEOPORT_EXTENSION_DATA  ExtensionData;

  DPRINT("VidDispatchOpenClose() called\n");

  /* FIXME: see file header */
  DPRINT("Using device 0x%08x instead of 0x%08x\n", pdoLastOpened, pDO);
  pDO = pdoLastOpened;
  IrpStack = IoGetCurrentIrpStackLocation(Irp);

  if (IrpStack->MajorFunction == IRP_MJ_CREATE &&
      CsrssInitialized == FALSE)
    {
      DPRINT("Referencing CSRSS\n");
      Csrss = PsGetCurrentProcess();
      CsrssInitialized = TRUE;
      DPRINT("Csrss %p\n", Csrss);
      ExtensionData = (PVIDEOPORT_EXTENSION_DATA) pDO->DeviceExtension;
      if (ExtensionData->HwInitialize(VPExtensionToMPExtension(ExtensionData)))
	{
	  Irp->IoStatus.Status = STATUS_SUCCESS;
	}
      else
	{
	  Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	}
    }
  else
    {
    Irp->IoStatus.Status = STATUS_SUCCESS;
    }

  Irp->IoStatus.Information = FILE_OPENED;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}

//    VidStartIo
//
//  DESCRIPTION:
//    Get the next requested I/O packet started
//
//  RUN LEVEL:
//    DISPATCH_LEVEL
//
//  ARGUMENTS:
//    Dispatch routine standard arguments
//
//  RETURNS:
//    NTSTATUS
//

static VOID STDCALL
VidStartIo(IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp)
{
  DPRINT("VidStartIo\n");
  UNIMPLEMENTED;
}

//    VidDispatchDeviceControl
//
//  DESCRIPTION:
//    Answer requests for device control calls
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    Standard dispatch arguments
//
//  RETURNS:
//    NTSTATUS
//

static NTSTATUS STDCALL
VidDispatchDeviceControl(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp)
{
  PIO_STACK_LOCATION IrpStack;
  PVIDEO_REQUEST_PACKET vrp;

  DPRINT("VidDispatchDeviceControl\n");
  /* FIXME: See file header */
  DPRINT("Using device 0x%08x instead of 0x%08x\n", pdoLastOpened, DeviceObject);
  DeviceObject = pdoLastOpened;
  IrpStack = IoGetCurrentIrpStackLocation(Irp);

  // Translate the IRP to a VRP
  vrp = ExAllocatePool(PagedPool, sizeof(VIDEO_REQUEST_PACKET));
  vrp->StatusBlock = ExAllocatePool(PagedPool, sizeof(STATUS_BLOCK));
  vrp->IoControlCode      = IrpStack->Parameters.DeviceIoControl.IoControlCode;

  // We're assuming METHOD_BUFFERED
  vrp->InputBuffer        = Irp->AssociatedIrp.SystemBuffer;
  vrp->InputBufferLength  = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
  vrp->OutputBuffer       = Irp->UserBuffer;
  vrp->OutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

  // Call the Miniport Driver with the VRP
  DeviceObject->DriverObject->DriverStartIo(VPExtensionToMPExtension(DeviceObject->DeviceExtension), (PIRP)vrp);

  // Translate the VRP back into the IRP for OutputBuffer
  Irp->UserBuffer                                             = vrp->OutputBuffer;
  IrpStack->Parameters.DeviceIoControl.OutputBufferLength = vrp->OutputBufferLength;
  Irp->IoStatus.Status = vrp->StatusBlock->Status;
  Irp->IoStatus.Information = vrp->StatusBlock->Information;

  // Free the VRP
  ExFreePool(vrp->StatusBlock);
  ExFreePool(vrp);

  return STATUS_SUCCESS;
}
