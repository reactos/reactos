/*
   ReactOS specific functions for OHCI module
   by Aleksey Bragin (aleksey@reactos.com)
   Some parts of code are inspired (or even just copied) from ReactOS Videoport driver
*/

#include <ddk/ntddk.h>
#include <ddk/ntddkbd.h>
#include <ddk/ntdd8042.h>

#include <debug.h>
#include "../usb_wrapper.h"
#include "../core/hcd.h"
#include "ohci_main.h"

// declare basic init funcs
void init_wrapper(struct pci_dev *probe_dev);
int ohci_hcd_pci_init (void);
void ohci_hcd_pci_cleanup (void);
int STDCALL usb_init(void);
void STDCALL usb_exit(void);
extern struct pci_driver ohci_pci_driver;
extern const struct pci_device_id pci_ids[];


// This should be removed, but for testing purposes it's here
struct pci_dev *dev;
//struct pci_device_id *dev_id;
static bool xbox_workaround=false;

// data for embedded drivers
CONNECT_DATA KbdClassInformation;
CONNECT_DATA MouseClassInformation;
PDEVICE_OBJECT KeyboardFdo = NULL;
PDEVICE_OBJECT MouseFdo = NULL;


#define USB_OHCI_TAG TAG('u','s','b','o')

NTSTATUS AddDevice_Keyboard(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT pdo)
{
	UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\KeyboardClass0");
	PDEVICE_OBJECT fdo;
	NTSTATUS Status;

	Status = IoCreateDevice(DriverObject,
	               8, // debug
	               &DeviceName,
	               FILE_DEVICE_KEYBOARD,
	               FILE_DEVICE_SECURE_OPEN,
	               TRUE,
	               &fdo);

	if (!NT_SUCCESS(Status))
	{
		DPRINT1("IoCreateDevice for usb keyboard driver failed\n");
		return Status;
	}
	KeyboardFdo = fdo;
	fdo->Flags &= ~DO_DEVICE_INITIALIZING;
	DPRINT1("Created keyboard fdo: %p\n", fdo);

	return STATUS_SUCCESS;
}

NTSTATUS AddDevice_Mouse(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT pdo)
{
	UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\PointerClass0");
	PDEVICE_OBJECT fdo;
	NTSTATUS Status;

	Status = IoCreateDevice(DriverObject,
	               8, // debug
	               &DeviceName,
	               FILE_DEVICE_MOUSE,
	               FILE_DEVICE_SECURE_OPEN,
	               TRUE,
	               &fdo);

	if (!NT_SUCCESS(Status))
	{
		DPRINT1("IoCreateDevice for usb mouse driver failed\n");
		return Status;
	}
	MouseFdo = fdo;
	fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	DPRINT1("Created mouse fdo: %p\n", fdo);

	return STATUS_SUCCESS;
}


NTSTATUS STDCALL AddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT pdo)
{
	PDEVICE_OBJECT fdo;
	NTSTATUS Status;
	WCHAR DeviceBuffer[20];
	UNICODE_STRING DeviceName;
	POHCI_DRIVER_EXTENSION DriverExtension;
	POHCI_DEVICE_EXTENSION DeviceExtension;
	ULONG Size, DeviceNumber;

	DPRINT1("ohci: AddDevice called\n");

	if (xbox_workaround)
		return STATUS_INSUFFICIENT_RESOURCES; // Fail for any other host controller than the first
	
	xbox_workaround = true;

	// Allocate driver extension now
	DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);
	if (DriverExtension == NULL)
	{
		Status = IoAllocateDriverObjectExtension(
					DriverObject,
					DriverObject,
					sizeof(OHCI_DRIVER_EXTENSION),
					(PVOID *)&DriverExtension);

		if (!NT_SUCCESS(Status))
		{
			DPRINT1("Allocating DriverObjectExtension failed.\n");
			return Status;
		}
	}
   
	// Create a unicode device name
	DeviceNumber = 0; //TODO: Allocate new device number every time
	swprintf(DeviceBuffer, L"\\Device\\USBFDO-%lu", DeviceNumber);
	RtlInitUnicodeString(&DeviceName, DeviceBuffer);

	Status = IoCreateDevice(DriverObject,
		                    sizeof(OHCI_DEVICE_EXTENSION)/* + DriverExtension->InitializationData.HwDeviceExtensionSize*/,
							&DeviceName,
							FILE_DEVICE_CONTROLLER,
							0,
							FALSE,
							&fdo);

	if (!NT_SUCCESS(Status))
	{
		DPRINT("IoCreateDevice call failed with status 0x%08x\n", Status);
		return Status;
	}

	// zerofill device extension
	DeviceExtension = (POHCI_DEVICE_EXTENSION)fdo->DeviceExtension;
	RtlZeroMemory(DeviceExtension, sizeof(OHCI_DEVICE_EXTENSION));
	DeviceExtension->NextDeviceObject = IoAttachDeviceToDeviceStack(fdo, pdo);

	fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	// Initialize device extension
	DeviceExtension->DeviceNumber = DeviceNumber;
	DeviceExtension->PhysicalDeviceObject = pdo;
	DeviceExtension->FunctionalDeviceObject = fdo;
	DeviceExtension->DriverExtension = DriverExtension;

	/* Get bus number from the upper level bus driver. */
	Size = sizeof(ULONG);
	Status = IoGetDeviceProperty(
				pdo,
				DevicePropertyBusNumber,
				Size,
				&DeviceExtension->SystemIoBusNumber,
				&Size);
	
	if (!NT_SUCCESS(Status))
	{
		DPRINT("Couldn't get an information from bus driver. Panic!!!\n");
		return Status;
	}

	DPRINT("Done AddDevice\n");

	// create embedded keyboard driver
	Status = AddDevice_Keyboard(DriverObject, pdo);
	Status = AddDevice_Mouse(DriverObject, pdo);

	return Status;
}


VOID STDCALL DriverUnload(PDRIVER_OBJECT DriverObject)
{
	DPRINT1("DriverUnload()\n");

	// Exit usb device
	usb_exit();

	// Remove device (ohci_pci_driver.remove)
	ohci_pci_driver.remove(dev);

	ExFreePool(dev->slot_name);
	ExFreePool(dev);

	// Perform some cleanup
	ohci_hcd_pci_cleanup();
}

void RegisterISR(PDEVICE_OBJECT DeviceObject)
{
#if 0
	NTSTATUS Status;
	POHCI_DEVICE_EXTENSION DeviceExtension = (POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	/* Connect interrupt and enable them */
	Status = IoConnectInterrupt(
		&DeviceExtension->InterruptObject, SerialInterruptService,
		DeviceObject, NULL,
		Vector, Dirql, Dirql,
		InterruptMode, ShareInterrupt,
		Affinity, FALSE);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("hci: IoConnectInterrupt() failed with status 0x%08x\n", Status);
		return 1;
	}
#endif
}

NTSTATUS
InitLinuxWrapper(PDEVICE_OBJECT DeviceObject)
{
	NTSTATUS Status;
	POHCI_DEVICE_EXTENSION DeviceExtension = (POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	// Allocate and fill generic linux structs
	dev = ExAllocatePoolWithTag(PagedPool, sizeof(struct pci_dev), USB_OHCI_TAG);
	dev->irq = DeviceExtension->InterruptVector;
	dev->dev_ext = (PVOID)DeviceExtension;
	dev->dev.dev_ext = (PVOID)DeviceExtension;
	dev->slot_name = ExAllocatePoolWithTag(NonPagedPool, 128, USB_OHCI_TAG); // 128 max len for slot name

	// Init wrapper
	init_wrapper(dev);

	strcpy(dev->dev.name, "OpenHCI PCI-USB Controller");
	strcpy(dev->slot_name, "OHCD PCI Slot");

	// Init the OHCI HCD. Probe will be called automatically, but will fail because id=NULL
	Status = ohci_hcd_pci_init();
	//FIXME: Check status returned value

	// Init core usb
	usb_init();

	// Probe device with real id now
	ohci_pci_driver.probe(dev, pci_ids);

	// Register interrupt here
	RegisterISR(DeviceObject);

	DPRINT1("InitLinuxWrapper() done\n");

	return STATUS_SUCCESS;
}

NTSTATUS STDCALL
OHCD_PnPStartDevice(IN PDEVICE_OBJECT DeviceObject,
					IN PIRP	Irp)
{
	PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
	PDRIVER_OBJECT DriverObject;
	POHCI_DRIVER_EXTENSION DriverExtension;
	POHCI_DEVICE_EXTENSION DeviceExtension;
	PCM_RESOURCE_LIST AllocatedResources;

	NTSTATUS Status; // debug
	LONGLONG delay; // debug

	if (DeviceObject == KeyboardFdo || DeviceObject == MouseFdo)
		return STATUS_SUCCESS;

	/*
	* Get the initialization data we saved in VideoPortInitialize.
	*/
	DriverObject = DeviceObject->DriverObject;
	DriverExtension	= IoGetDriverObjectExtension(DriverObject, DriverObject);
	DeviceExtension	= (POHCI_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	/*
	* Store	some resources in the DeviceExtension.
	*/
	AllocatedResources = Stack->Parameters.StartDevice.AllocatedResources;
	if (AllocatedResources != NULL)
	{
		CM_FULL_RESOURCE_DESCRIPTOR	*FullList;
		CM_PARTIAL_RESOURCE_DESCRIPTOR *Descriptor;
		ULONG ResourceCount;
		ULONG ResourceListSize;

		/* Save	the	resource list */
		ResourceCount =	AllocatedResources->List[0].PartialResourceList.Count;
		ResourceListSize =
			FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.
			PartialDescriptors[ResourceCount]);
		DeviceExtension->AllocatedResources	= ExAllocatePool(PagedPool,	ResourceListSize);
		if (DeviceExtension->AllocatedResources	== NULL)
		{
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		RtlCopyMemory(DeviceExtension->AllocatedResources,
			AllocatedResources,
			ResourceListSize);

		/* Get the interrupt level/vector -	needed by HwFindAdapter	sometimes */
		for	(FullList =	AllocatedResources->List;
			FullList < AllocatedResources->List	+ AllocatedResources->Count;
			FullList++)
		{
			/* FIXME: Is this ASSERT ok	for	resources from the PNP manager?	*/
			/*ASSERT(FullList->InterfaceType == PCIBus &&
				FullList->BusNumber	== DeviceExtension->SystemIoBusNumber &&
				1 == FullList->PartialResourceList.Version &&
				1 == FullList->PartialResourceList.Revision);*/
			for	(Descriptor	= FullList->PartialResourceList.PartialDescriptors;
				Descriptor < FullList->PartialResourceList.PartialDescriptors +	FullList->PartialResourceList.Count;
				Descriptor++)
			{
				if (Descriptor->Type ==	CmResourceTypeInterrupt)
				{
					DeviceExtension->InterruptLevel	= Descriptor->u.Interrupt.Level;
					DeviceExtension->InterruptVector = Descriptor->u.Interrupt.Vector;
				}
				else if (Descriptor->Type ==	CmResourceTypeMemory)
				{
					DeviceExtension->BaseAddress	= Descriptor->u.Memory.Start;
					DeviceExtension->BaseAddrLength = Descriptor->u.Memory.Length;
				}
			}
		}
	}
	DPRINT1("Interrupt level: 0x%x Interrupt	Vector:	0x%x\n",
		DeviceExtension->InterruptLevel,
		DeviceExtension->InterruptVector);

	/*
	* Init wrapper with this object
	*/
	//return InitLinuxWrapper(DeviceObject);

	// debug wait
	Status = InitLinuxWrapper(DeviceObject);

	//delay = -10000000*30; // wait 30 secs
	//KeDelayExecutionThread(KernelMode, FALSE, (LARGE_INTEGER *)&delay); //wait_us(1);

	return Status;
}

// Dispatch PNP
NTSTATUS STDCALL DispatchPnp(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
   PIO_STACK_LOCATION IrpSp;
   NTSTATUS Status;

   IrpSp = IoGetCurrentIrpStackLocation(Irp);

   DPRINT("PNP for %p, minorfunc=0x%x\n", DeviceObject, IrpSp->MinorFunction);


   switch (IrpSp->MinorFunction)
   {
      case IRP_MN_START_DEVICE:
         //Status = IntVideoPortForwardIrpAndWait(DeviceObject, Irp);
         //if (NT_SUCCESS(Status) && NT_SUCCESS(Irp->IoStatus.Status))
         
		 Status = OHCD_PnPStartDevice(DeviceObject, Irp);
         Irp->IoStatus.Status = Status;
         Irp->IoStatus.Information = 0;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         break;


      case IRP_MN_REMOVE_DEVICE:
      case IRP_MN_QUERY_REMOVE_DEVICE:
      case IRP_MN_CANCEL_REMOVE_DEVICE:
      case IRP_MN_SURPRISE_REMOVAL:

      case IRP_MN_STOP_DEVICE:
         //Status = IntVideoPortForwardIrpAndWait(DeviceObject, Irp);
         //if (NT_SUCCESS(Status) && NT_SUCCESS(Irp->IoStatus.Status))
            Status = STATUS_SUCCESS;
         Irp->IoStatus.Status = Status;
         Irp->IoStatus.Information = 0;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);

		 IoDeleteDevice(DeviceObject); // just delete device for now
         break;

      case IRP_MN_QUERY_STOP_DEVICE:
      case IRP_MN_CANCEL_STOP_DEVICE:
         Status = STATUS_SUCCESS;
         Irp->IoStatus.Status = STATUS_SUCCESS;
         Irp->IoStatus.Information = 0;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         break;
         
      default:
         return STATUS_NOT_IMPLEMENTED;
         break;
   }
   
   return Status;
}

NTSTATUS STDCALL DispatchPower(PDEVICE_OBJECT fido, PIRP Irp)
{
	DbgPrint("IRP_MJ_POWER dispatch\n");
	return STATUS_SUCCESS;
}

NTSTATUS STDCALL UsbCoreDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	ULONG MajorFunction = IoGetCurrentIrpStackLocation(Irp)->MajorFunction;

	DPRINT("usbohci: Dispatching major function 0x%lx\n", MajorFunction);

	IoCompleteRequest (Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}


NTSTATUS STDCALL UsbCoreInternalDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

	DPRINT("INT_IOCTL for %p, code=0x%x\n", DeviceObject, IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.IoControlCode);

	if (DeviceObject == KeyboardFdo)
	{
		// it's keyboard's IOCTL
		PIO_STACK_LOCATION Stk;

		Irp->IoStatus.Information = 0;
		Stk = IoGetCurrentIrpStackLocation(Irp);

		switch (Stk->Parameters.DeviceIoControl.IoControlCode) {
			case IOCTL_INTERNAL_KEYBOARD_CONNECT:
				DPRINT("IOCTL_INTERNAL_KEYBOARD_CONNECT\n");
				if (Stk->Parameters.DeviceIoControl.InputBufferLength <	sizeof(CONNECT_DATA)) {
					DPRINT1("Keyboard IOCTL_INTERNAL_KEYBOARD_CONNECT "
							"invalid buffer	size\n");
					Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
					goto intcontfailure;
				}

				memcpy(&KbdClassInformation,
					Stk->Parameters.DeviceIoControl.Type3InputBuffer,
					sizeof(CONNECT_DATA));
	
				Irp->IoStatus.Status = STATUS_SUCCESS;
				break;

		case IOCTL_INTERNAL_I8042_KEYBOARD_WRITE_BUFFER:
			DPRINT("IOCTL_INTERNAL_I8042_KEYBOARD_WRITE_BUFFER\n");
			if (Stk->Parameters.DeviceIoControl.InputBufferLength <	1) {
				Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
				goto intcontfailure;
			}
/*			if (!DevExt->KeyboardInterruptObject) {
				Irp->IoStatus.Status = STATUS_DEVICE_NOT_READY;
				goto intcontfailure;
			}*/

			Irp->IoStatus.Status = STATUS_SUCCESS;
			break;
		case IOCTL_KEYBOARD_QUERY_ATTRIBUTES:
			DPRINT("IOCTL_KEYBOARD_QUERY_ATTRIBUTES\n");
			if (Stk->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof(KEYBOARD_ATTRIBUTES)) {
					DPRINT("Keyboard IOCTL_KEYBOARD_QUERY_ATTRIBUTES "
						"invalid buffer	size\n");
					Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
					goto intcontfailure;
				}
				/*memcpy(Irp->AssociatedIrp.SystemBuffer,
					&DevExt->KeyboardAttributes,
					sizeof(KEYBOARD_ATTRIBUTES));*/

				Irp->IoStatus.Status = STATUS_SUCCESS;
				break;
		case IOCTL_KEYBOARD_QUERY_INDICATORS:
			DPRINT("IOCTL_KEYBOARD_QUERY_INDICATORS\n");
			if (Stk->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof(KEYBOARD_INDICATOR_PARAMETERS)) {
					DPRINT("Keyboard IOCTL_KEYBOARD_QUERY_INDICATORS "
						"invalid buffer	size\n");
					Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
					goto intcontfailure;
				}
				/*memcpy(Irp->AssociatedIrp.SystemBuffer,
					&DevExt->KeyboardIndicators,
					sizeof(KEYBOARD_INDICATOR_PARAMETERS));*/

				Irp->IoStatus.Status = STATUS_SUCCESS;
				break;
		case IOCTL_KEYBOARD_QUERY_TYPEMATIC:
			DPRINT("IOCTL_KEYBOARD_QUERY_TYPEMATIC\n");
			if (Stk->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof(KEYBOARD_TYPEMATIC_PARAMETERS)) {
					DPRINT("Keyboard IOCTL_KEYBOARD_QUERY_TYPEMATIC	"
						"invalid buffer	size\n");
					Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
					goto intcontfailure;
				}
				/*memcpy(Irp->AssociatedIrp.SystemBuffer,
					&DevExt->KeyboardTypematic,
					sizeof(KEYBOARD_TYPEMATIC_PARAMETERS));*/

				Irp->IoStatus.Status = STATUS_SUCCESS;
				break;
		case IOCTL_KEYBOARD_SET_INDICATORS:
			DPRINT("IOCTL_KEYBOARD_SET_INDICATORS\n");
			if (Stk->Parameters.DeviceIoControl.InputBufferLength <
				sizeof(KEYBOARD_INDICATOR_PARAMETERS)) {
					DPRINT("Keyboard IOCTL_KEYBOARD_SET_INDICTATORS	"
						"invalid buffer	size\n");
					Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
					goto intcontfailure;
				}

				/*memcpy(&DevExt->KeyboardIndicators,
					Irp->AssociatedIrp.SystemBuffer,
					sizeof(KEYBOARD_INDICATOR_PARAMETERS));*/

				//DPRINT("%x\n", DevExt->KeyboardIndicators.LedFlags);

				Irp->IoStatus.Status = STATUS_SUCCESS;
				break;
		case IOCTL_KEYBOARD_SET_TYPEMATIC:
			DPRINT("IOCTL_KEYBOARD_SET_TYPEMATIC\n");
			if (Stk->Parameters.DeviceIoControl.InputBufferLength <
				sizeof(KEYBOARD_TYPEMATIC_PARAMETERS)) {
					DPRINT("Keyboard IOCTL_KEYBOARD_SET_TYPEMATIC "
						"invalid buffer	size\n");
					Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
					goto intcontfailure;
				}

				/*memcpy(&DevExt->KeyboardTypematic,
					Irp->AssociatedIrp.SystemBuffer,
					sizeof(KEYBOARD_TYPEMATIC_PARAMETERS));*/

				Irp->IoStatus.Status = STATUS_SUCCESS;
				break;
		case IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION:
			/* We should check the UnitID, but it's	kind of	pointless as
			* all keyboards	are	supposed to	have the same one
			*/
#if 0
			if (Stk->Parameters.DeviceIoControl.OutputBufferLength <
				sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION)) {
					DPRINT("IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION:	"
						"invalid buffer	size (expected)\n");
					/* It's	to query the buffer	size */
					Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
					goto intcontfailure;
				}
				Irp->IoStatus.Information =
					sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION);
#endif
				/*memcpy(Irp->AssociatedIrp.SystemBuffer,
					&IndicatorTranslation,
					sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION));*/

				Irp->IoStatus.Status = STATUS_SUCCESS;
				break;
		case IOCTL_INTERNAL_I8042_HOOK_KEYBOARD:
			/* Nothing to do here */
			Irp->IoStatus.Status = STATUS_SUCCESS;
			break;
		default:
			Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
			break;
			}

	intcontfailure:
			Status = Irp->IoStatus.Status;
	}
	else if (DeviceObject == MouseFdo)
	{
		// it's mouse's IOCTL
		PIO_STACK_LOCATION Stk;

		Irp->IoStatus.Information = 0;
		Stk = IoGetCurrentIrpStackLocation(Irp);

		switch (Stk->Parameters.DeviceIoControl.IoControlCode) {
			case IOCTL_INTERNAL_MOUSE_CONNECT:
				DPRINT("IOCTL_INTERNAL_MOUSE_CONNECT\n");
				if (Stk->Parameters.DeviceIoControl.InputBufferLength <	sizeof(CONNECT_DATA)) {
					DPRINT1("Mouse IOCTL_INTERNAL_MOUSE_CONNECT "
							"invalid buffer	size\n");
					Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
					goto intcontfailure2;
				}

				memcpy(&MouseClassInformation,
					Stk->Parameters.DeviceIoControl.Type3InputBuffer,
					sizeof(CONNECT_DATA));
	
				Irp->IoStatus.Status = STATUS_SUCCESS;
				break;

		default:
			Irp->IoStatus.Status = STATUS_SUCCESS;//STATUS_INVALID_DEVICE_REQUEST;
			break;
			}
	intcontfailure2:
			Status = Irp->IoStatus.Status;
	}
	else
	{
		DPRINT("We got IOCTL for UsbCore\n");
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}


	if (Status == STATUS_INVALID_DEVICE_REQUEST) {
		DPRINT1("Invalid internal device request!\n");
	}

	if (Status != STATUS_PENDING)
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return Status;
}


/*
 * Standard DriverEntry method.
 */
NTSTATUS STDCALL
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegPath)
{
	int i;
	USBPORT_INTERFACE UsbPortInterface;

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->DriverExtension->AddDevice = AddDevice;

	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
		DriverObject->MajorFunction[i] = UsbCoreDispatch;

	DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
	DriverObject->MajorFunction[IRP_MJ_POWER] = DispatchPower;
	DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = UsbCoreInternalDeviceControl;

	// Register in usbcore.sys
	UsbPortInterface.KbdConnectData = &KbdClassInformation;
	UsbPortInterface.MouseConnectData = &MouseClassInformation;
	
	KbdClassInformation.ClassService = NULL;
	KbdClassInformation.ClassDeviceObject = NULL;
	MouseClassInformation.ClassService = NULL;
	MouseClassInformation.ClassDeviceObject = NULL;
	
	RegisterPortDriver(DriverObject, &UsbPortInterface);

	return STATUS_SUCCESS;
}
