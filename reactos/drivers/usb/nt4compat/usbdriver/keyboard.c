/*
 * PROJECT:         ReactOS USB Drivers
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            keyboard.c
 * PURPOSE:         Generic USB keyboard driver
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

#include "usbdriver.h"
#include "kbdmou.h"

NTSTATUS
AddRegistryEntry(IN PCWSTR PortTypeName,
                 IN PUNICODE_STRING DeviceName,
                 IN PCWSTR RegistryPath);

BOOLEAN kbd_connect(PDEV_CONNECT_DATA dev_mgr, DEV_HANDLE dev_handle);
BOOLEAN kbd_disconnect(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle);
BOOLEAN kbd_stop(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle);
VOID kbd_irq(PURB purb, PVOID pcontext);
static NTSTATUS
KeyboardCreateDevice(IN PDRIVER_OBJECT DriverObject, IN PKEYBOARD_DRVR_EXTENSION DriverExtension);
void * memscan(void * addr, int c, size_t size);

static UCHAR usb_kbd_keycode[256] =
{
     0,  0,  0,  0, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38,
    50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44,  2,  3,
     4,  5,  6,  7,  8,  9, 10, 11, 28,  1, 14, 15, 57, 12, 13, 26,
    27, 43, 43, 39, 40, 41, 51, 52, 53, 58, 59, 60, 61, 62, 63, 64,
    65, 66, 67, 68, 87, 88, 99, 70,119,110,102,104,111,107,109,106,
    105,108,103, 69, 98, 55, 74, 78, 96, 79, 80, 81, 75, 76, 77, 71,
     72, 73, 82, 83, 86,127,116,117,183,184,185,186,187,188,189,190,
    191,192,193,194,134,138,130,132,128,129,131,137,133,135,136,113,
    115,114,  0,  0,  0,121,  0, 89, 93,124, 92, 94, 95,  0,  0,  0,
    122,123, 90, 91, 85,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     29, 42, 56,125, 97, 54,100,126,164,166,165,163,161,115,114,113,
    150,158,159,128,136,177,178,176,142,152,173,140
};

/* This structure starts with the same layout as KEYBOARD_INDICATOR_TRANSLATION */
typedef struct _LOCAL_KEYBOARD_INDICATOR_TRANSLATION {
    USHORT NumberOfIndicatorKeys;
    INDICATOR_LIST IndicatorList[3];
} LOCAL_KEYBOARD_INDICATOR_TRANSLATION, *PLOCAL_KEYBOARD_INDICATOR_TRANSLATION;

static LOCAL_KEYBOARD_INDICATOR_TRANSLATION IndicatorTranslation = { 3, {
    {0x3A, KEYBOARD_CAPS_LOCK_ON},
    {0x45, KEYBOARD_NUM_LOCK_ON},
    {0x46, KEYBOARD_SCROLL_LOCK_ON}}};


BOOLEAN
kbd_driver_init(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver)
{
    PKEYBOARD_DRVR_EXTENSION pdrvr_ext;

    if (dev_mgr == NULL || pdriver == NULL)
        return FALSE;

    //init driver structure, no PNP table functions
    pdriver->driver_desc.flags = USB_DRIVER_FLAG_IF_CAPABLE;
    pdriver->driver_desc.vendor_id = 0xffff;    // USB Vendor ID
    pdriver->driver_desc.product_id = 0xffff;   // USB Product ID.
    pdriver->driver_desc.release_num = 0x100;   // Release Number of Device

    pdriver->driver_desc.config_val = 0;            // Configuration Value
    pdriver->driver_desc.if_num = 0;                // Interface Number
    pdriver->driver_desc.if_class = USB_CLASS_HID;  // Interface Class
    pdriver->driver_desc.if_sub_class = 1;          // Interface SubClass
    pdriver->driver_desc.if_protocol = 1;           // Interface Protocol

    pdriver->driver_desc.driver_name = "USB Keyboard driver"; // Driver name for Name Registry
    pdriver->driver_desc.dev_class = USB_CLASS_HID;
    pdriver->driver_desc.dev_sub_class = 1;         // Device Subclass
    pdriver->driver_desc.dev_protocol = 1;          // Protocol Info.

    pdriver->driver_ext = usb_alloc_mem(NonPagedPool, sizeof(KEYBOARD_DRVR_EXTENSION));
    pdriver->driver_ext_size = sizeof(KEYBOARD_DRVR_EXTENSION);

    RtlZeroMemory(pdriver->driver_ext, sizeof(KEYBOARD_DRVR_EXTENSION));
    pdrvr_ext = (PKEYBOARD_DRVR_EXTENSION) pdriver->driver_ext;
    pdrvr_ext->dev_mgr = dev_mgr;

    pdriver->disp_tbl.version = 1;
    pdriver->disp_tbl.dev_connect = kbd_connect;
    pdriver->disp_tbl.dev_disconnect = kbd_disconnect;
    pdriver->disp_tbl.dev_stop = kbd_stop;
    pdriver->disp_tbl.dev_reserved = NULL;

    // Create the device
    KeyboardCreateDevice(dev_mgr->usb_driver_obj, pdrvr_ext);

    usb_dbg_print(DBGLVL_MAXIMUM, ("kbd_driver_init(): keyboard driver is initialized\n"));
    return TRUE;
}

BOOLEAN
kbd_driver_destroy(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver)
{
    //PMOUSE_DRVR_EXTENSION pdrvr_ext;
    if (dev_mgr == NULL || pdriver == NULL)
        return FALSE;

    //pdrvr_ext = (PUMSS_DRVR_EXTENSION) pdriver->driver_ext;
    //umss_delete_port_device(pdrvr_ext->port_dev_obj);
    //pdrvr_ext->port_dev_obj = NULL;

    //ASSERT(IsListEmpty(&pdrvr_ext->dev_list) == TRUE);
    usb_free_mem(pdriver->driver_ext);
    pdriver->driver_ext = NULL;
    pdriver->driver_ext_size = 0;
    usb_dbg_print(DBGLVL_MAXIMUM, ("kbd_driver_destroy(): keyboard driver is destroyed\n"));
    return TRUE;
}

BOOLEAN
kbd_connect(PDEV_CONNECT_DATA param, DEV_HANDLE dev_handle)
{
    PURB purb;
    NTSTATUS status;
    PUSB_DEV_MANAGER dev_mgr;
    PUSB_DRIVER pdrvr;
    PUSB_DEV pdev;
    PKEYBOARD_DRVR_EXTENSION pdev_ext;
    PUSB_ENDPOINT_DESC pendp_desc = NULL;
    ULONG MaxPacketSize;

    usb_dbg_print(DBGLVL_MAXIMUM, ("kbd_connect(): entering...\n"));

    dev_mgr = param->dev_mgr;
    pdrvr = param->pdriver;
    pdev_ext = (PKEYBOARD_DRVR_EXTENSION)pdrvr->driver_ext;
    pdev_ext->dev_handle = dev_handle;

    // Lock USB Device
    status = usb_query_and_lock_dev(dev_mgr, dev_handle, &pdev);
    if (status != STATUS_SUCCESS)
    {
        usb_dbg_print(DBGLVL_MEDIUM, ("kbd_connect(): unable to query&lock device, status=0x%x\n", status));
        return FALSE;
    }

    // Get pointer to the endpoint descriptor
    pendp_desc = pdev->usb_config->interf[0].endp[0].pusb_endp_desc;

    // Store max packet size
    MaxPacketSize = pendp_desc->wMaxPacketSize;
    if (MaxPacketSize > 8)
        MaxPacketSize = 8;

    // Unlock USB Device
    usb_unlock_dev(pdev);

    // Send interrupt URB
    purb = usb_alloc_mem(NonPagedPool, sizeof(URB));
    if (purb == NULL)
        return FALSE;
    RtlZeroMemory(purb, sizeof(URB));

    RtlZeroMemory(pdev_ext->kbd_old, 8);

    // Build a URB for our interrupt transfer
    UsbBuildInterruptOrBulkTransferRequest(purb,
                                           usb_make_handle((dev_handle >> 16), 0, 0),
                                           (PUCHAR)&pdev_ext->kbd_data,
                                           MaxPacketSize, //use max packet size
                                           kbd_irq,
                                           pdev_ext->device_ext,
                                           0);

    // Call USB driver stack
    status = usb_submit_urb(pdev_ext->dev_mgr, purb);
    if (status != STATUS_PENDING)
    {
        usb_free_mem(purb);
        purb = NULL;
    }

    return TRUE;
}

VOID
kbd_irq(PURB purb, PVOID pcontext)
{
    KEYBOARD_INPUT_DATA KeyboardInputData[20];
    ULONG DataPrepared=0, DataConsumed, i;
    UCHAR ScanCode;
    NTSTATUS status;
    PKEYBOARD_DRVR_EXTENSION pdev_ext;
    PKEYBOARD_DEVICE_EXTENSION DeviceExtension = (PKEYBOARD_DEVICE_EXTENSION)pcontext;
    PUCHAR data, data_old;
    usb_dbg_print(DBGLVL_MAXIMUM, ("kbd_irq(): called\n"));

    ASSERT(purb);

    if (purb->status != STATUS_SUCCESS)
    {
        usb_dbg_print(DBGLVL_MAXIMUM, ("kbd_irq(): purb->status 0x%08X\n", purb->status));
        return;
    }

    pdev_ext = DeviceExtension->DriverExtension;
    data = pdev_ext->kbd_data;
    data_old = pdev_ext->kbd_old;

    usb_dbg_print(DBGLVL_MAXIMUM, ("Kbd input: %d %d %d %d %d %d %d %d\n", data[0], data[1], data[2], data[3],
        data[4], data[5], data[6], data[7]));

    // Zero initial kbd data array
    RtlZeroMemory(&KeyboardInputData[0], sizeof(KeyboardInputData));

    // Scan modifier keys
    for (i=0; i<8; i++)
    {
        ScanCode = usb_kbd_keycode[i + 224];

        if (((data[0] >> i) & 1) != ((data_old[0] >> i) & 1))
        {
            usb_dbg_print(DBGLVL_MAXIMUM, ("Scan %d key p/r %d\n", ScanCode, (data[0] >> i) & 1));

            KeyboardInputData[DataPrepared].MakeCode = ScanCode;
            if ((data[0] >> i) & 1)
                KeyboardInputData[DataPrepared].Flags |= KEY_MAKE;
            else
                KeyboardInputData[DataPrepared].Flags |= KEY_BREAK;
            DataPrepared++;
        }
    }

    // Scan normal keys
    for (i=2; i<8; i++)
    {
        if (data_old[i] > 3 && memscan(data + 2, data_old[i], 6) == data + 8)
        {
            if (usb_kbd_keycode[data_old[i]])
            {
                // key released
                usb_dbg_print(DBGLVL_MAXIMUM, ("Scan %d key released 1\n", usb_kbd_keycode[data_old[i]]));

                KeyboardInputData[DataPrepared].MakeCode = usb_kbd_keycode[data_old[i]];
                KeyboardInputData[DataPrepared].Flags |= KEY_BREAK;
                DataPrepared++;
            }
        }

        if (data[i] > 3 && memscan(data_old + 2, data[i], 6) == data_old + 8)
        {
            if (usb_kbd_keycode[data[i]])
            {
                // key pressed
                usb_dbg_print(DBGLVL_MAXIMUM, ("Scan %d key pressed 1\n", usb_kbd_keycode[data[i]]));

                KeyboardInputData[DataPrepared].MakeCode = usb_kbd_keycode[data[i]];
                KeyboardInputData[DataPrepared].Flags |= KEY_MAKE;
                DataPrepared++;
            }
        }
    }

    // Commit the input data somewhere...
    if (DeviceExtension->ConnectData.ClassService && DataPrepared > 0)
    {
        KIRQL OldIrql;

        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        (*(PSERVICE_CALLBACK_ROUTINE)DeviceExtension->ConnectData.ClassService)(
            DeviceExtension->ConnectData.ClassDeviceObject,
            &KeyboardInputData[0],
            (&KeyboardInputData[0])+DataPrepared,
            &DataConsumed);
        KeLowerIrql(OldIrql);
    }

    // Save old keyboard data
    RtlCopyMemory(pdev_ext->kbd_old, data, sizeof(data));

    // resubmit the urb
    status = usb_submit_urb(pdev_ext->dev_mgr, purb);
}

BOOLEAN
kbd_stop(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle)
{
    UNREFERENCED_PARAMETER(dev_handle);
    UNREFERENCED_PARAMETER(dev_mgr);
    return TRUE;
}

BOOLEAN
kbd_disconnect(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle)
{
    PDEVICE_OBJECT dev_obj;
    NTSTATUS status;
    PUSB_DEV pdev;
    PUSB_DRIVER pdrvr;

    if (dev_mgr == NULL || dev_handle == 0)
        return FALSE;

    pdev = NULL;
    //special use of the lock dev, simply use this routine to get the dev
    status = usb_query_and_lock_dev(dev_mgr, dev_handle, &pdev);
    if (pdev == NULL)
    {
        return FALSE;
    }
    if (status == STATUS_SUCCESS)
    {
        // must be a bug
        TRAP();
        usb_unlock_dev(pdev);
    }
    pdrvr = pdev->dev_driver;
    dev_obj = pdev->dev_obj;
    pdev = NULL;

    return TRUE;//umss_delete_device(dev_mgr, pdrvr, dev_obj, FALSE);
}

VOID
kbd_setleds(PKEYBOARD_DEVICE_EXTENSION DeviceExtension)
{
    PURB Urb;
    PUSB_CTRL_SETUP_PACKET Setup;
    NTSTATUS Status;

    // Set LEDs request
    Urb = usb_alloc_mem(NonPagedPool, sizeof(URB));
    if (!Urb) return;
    RtlZeroMemory(Urb, sizeof(URB));

    DeviceExtension->DriverExtension->leds_old = 0;
    DeviceExtension->DriverExtension->leds = 7;

    // Convert from LedFlags to USB format
    //DeviceExtension->KeyboardIndicators.LedFlags

    // Build a URB for setting LEDs
    Setup = (PUSB_CTRL_SETUP_PACKET)Urb->setup_packet;
    urb_init((Urb));

    Urb->endp_handle = usb_make_handle((DeviceExtension->DriverExtension->dev_handle >> 16), 0, 0) | 0xffff;
    Urb->data_buffer = &DeviceExtension->DriverExtension->leds;
    Urb->data_length = 1;
    Urb->completion = NULL;
    Urb->context = NULL;
    Urb->reference = 0;
    Setup->bmRequestType = USB_DT_HID;
    Setup->bRequest = USB_REQ_SET_CONFIGURATION;
    Setup->wValue = USB_DT_CONFIG << 8;
    Setup->wIndex = 0;
    Setup->wLength = 1;

    // Call USB driver stack
    Status = usb_submit_urb(DeviceExtension->DriverExtension->dev_mgr, Urb);
    if (Status != STATUS_PENDING)
    {
        usb_free_mem(Urb);
    }
}

// Dispatch routine for our IRPs
NTSTATUS
KbdDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;
    PKEYBOARD_DEVICE_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;

    usb_dbg_print(DBGLVL_MAXIMUM, ("KbdDispatch(DO %p, code 0x%lx) called\n",
        DeviceObject,
        IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.IoControlCode));

    if (DeviceObject == DeviceExtension->Fdo)
    {
        // it's keyboard's IOCTL
        PIO_STACK_LOCATION Stack;

        Irp->IoStatus.Information = 0;
        Stack = IoGetCurrentIrpStackLocation(Irp);

        switch (Stack->Parameters.DeviceIoControl.IoControlCode)
        {
        case IOCTL_INTERNAL_KEYBOARD_CONNECT:
            usb_dbg_print(DBGLVL_MAXIMUM, ("IOCTL_INTERNAL_KEYBOARD_CONNECT\n"));
            if (Stack->Parameters.DeviceIoControl.InputBufferLength <	sizeof(CONNECT_DATA)) {
                usb_dbg_print(DBGLVL_MAXIMUM, ("Keyboard IOCTL_INTERNAL_KEYBOARD_CONNECT "
                    "invalid buffer size\n"));
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                goto intcontfailure;
            }

            RtlCopyMemory(&DeviceExtension->ConnectData,
                Stack->Parameters.DeviceIoControl.Type3InputBuffer,
                sizeof(CONNECT_DATA));

            Status = STATUS_SUCCESS;
            break;
        case IOCTL_KEYBOARD_QUERY_ATTRIBUTES:
            usb_dbg_print(DBGLVL_MAXIMUM, ("IOCTL_KEYBOARD_QUERY_ATTRIBUTES\n"));
            if (Stack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(KEYBOARD_ATTRIBUTES)) {
                    usb_dbg_print(DBGLVL_MAXIMUM, ("Keyboard IOCTL_KEYBOARD_QUERY_ATTRIBUTES: "
                        "invalid buffer size\n"));
                    Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                    goto intcontfailure;
            }
            /*RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
            &DevExt->KeyboardAttributes,
            sizeof(KEYBOARD_ATTRIBUTES));*/

            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        case IOCTL_KEYBOARD_QUERY_INDICATORS:
            usb_dbg_print(DBGLVL_MAXIMUM, ("IOCTL_KEYBOARD_QUERY_INDICATORS\n"));
            if (Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KEYBOARD_INDICATOR_PARAMETERS))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                RtlCopyMemory(
                    Irp->AssociatedIrp.SystemBuffer,
                    &DeviceExtension->KeyboardIndicators,
                    sizeof(KEYBOARD_INDICATOR_PARAMETERS));
                Irp->IoStatus.Information = sizeof(KEYBOARD_INDICATOR_PARAMETERS);
                Status = STATUS_SUCCESS;
            }
            break;
        case IOCTL_KEYBOARD_QUERY_TYPEMATIC:
            usb_dbg_print(DBGLVL_MAXIMUM, ("IOCTL_KEYBOARD_QUERY_TYPEMATIC\n"));
            if (Stack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(KEYBOARD_TYPEMATIC_PARAMETERS)) {
                    usb_dbg_print(DBGLVL_MAXIMUM, ("Keyboard IOCTL_KEYBOARD_QUERY_TYPEMATIC: "
                        "invalid buffer size\n"));
                    Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                    goto intcontfailure;
            }
            /*RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
            &DevExt->KeyboardTypematic,
            sizeof(KEYBOARD_TYPEMATIC_PARAMETERS));*/

            Status = STATUS_SUCCESS;
            break;
        case IOCTL_KEYBOARD_SET_INDICATORS:
            usb_dbg_print(DBGLVL_MAXIMUM, ("IOCTL_KEYBOARD_SET_INDICATORS\n"));
            if (Stack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(KEYBOARD_INDICATOR_PARAMETERS)) {
                    usb_dbg_print(DBGLVL_MAXIMUM, ("Keyboard IOCTL_KEYBOARD_SET_INDICTATORS: "
                        "invalid buffer size\n"));
                    Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                    goto intcontfailure;
            }

            RtlCopyMemory(&DeviceExtension->KeyboardIndicators,
                          Irp->AssociatedIrp.SystemBuffer,
                          sizeof(KEYBOARD_INDICATOR_PARAMETERS));

            //DPRINT("%x\n", DevExt->KeyboardIndicators.LedFlags);
            kbd_setleds(DeviceExtension);
            Status = STATUS_SUCCESS;
            break;
        case IOCTL_KEYBOARD_SET_TYPEMATIC:
            usb_dbg_print(DBGLVL_MAXIMUM, ("IOCTL_KEYBOARD_SET_TYPEMATIC\n"));
            if (Stack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(KEYBOARD_TYPEMATIC_PARAMETERS)) {
                    usb_dbg_print(DBGLVL_MAXIMUM, ("Keyboard IOCTL_KEYBOARD_SET_TYPEMATIC "
                        "invalid buffer size\n"));
                    Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                    goto intcontfailure;
            }

            /*RtlCopyMemory(&DevExt->KeyboardTypematic,
            Irp->AssociatedIrp.SystemBuffer,
            sizeof(KEYBOARD_TYPEMATIC_PARAMETERS));*/

            Status = STATUS_SUCCESS;
            break;
        case IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION:
            if (Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                RtlCopyMemory(
                    Irp->AssociatedIrp.SystemBuffer,
                    &IndicatorTranslation,
                    sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION));
                Irp->IoStatus.Information = sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION);
                Status = STATUS_SUCCESS;
            }
            break;
        default:
            Status = STATUS_SUCCESS;//STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
intcontfailure:
        Status = Irp->IoStatus.Status;
    }


    if (Status == STATUS_INVALID_DEVICE_REQUEST)
    {
        usb_dbg_print(DBGLVL_MINIMUM, ("Invalid internal device request!\n"));
    }

    Irp->IoStatus.Status = Status;
    if (Status != STATUS_PENDING)
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

static NTSTATUS
KeyboardCreateDevice(IN PDRIVER_OBJECT DriverObject, IN PKEYBOARD_DRVR_EXTENSION DriverExtension)
{
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\KeyboardPortUSB");
    PKEYBOARD_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT Fdo;
    NTSTATUS Status;

    Status = AddRegistryEntry(L"KeyboardPort", &DeviceName, L"REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\usbdriver");
    if (!NT_SUCCESS(Status))
    {
        usb_dbg_print(DBGLVL_MINIMUM, ("AddRegistryEntry() for usb keyboard driver failed with status 0x%08lx\n", Status));
        return Status;
    }

    Status = IoCreateDevice(DriverObject,
        sizeof(KEYBOARD_DEVICE_EXTENSION),
        &DeviceName,
        FILE_DEVICE_KEYBOARD,
        FILE_DEVICE_SECURE_OPEN,
        TRUE,
        &Fdo);

    if (!NT_SUCCESS(Status))
    {
        usb_dbg_print(DBGLVL_MINIMUM, ("IoCreateDevice() for usb keyboard driver failed with status 0x%08lx\n", Status));
        return Status;
    }
    DeviceExtension = (PKEYBOARD_DEVICE_EXTENSION)Fdo->DeviceExtension;
    RtlZeroMemory(DeviceExtension, sizeof(KEYBOARD_DEVICE_EXTENSION));

    DeviceExtension->hdr.dispatch = KbdDispatch;
    DeviceExtension->DriverExtension = DriverExtension;
    DriverExtension->device_ext = DeviceExtension;

    DeviceExtension->Fdo = Fdo;
    Fdo->Flags &= ~DO_DEVICE_INITIALIZING;
    usb_dbg_print(DBGLVL_MEDIUM, ("Created keyboard Fdo: %p\n", Fdo));

    return STATUS_SUCCESS;
}

/**
 * memscan - Find a character in an area of memory.
 * @addr: The memory area
 * @c: The byte to search for
 * @size: The size of the area.
 *
 * returns the address of the first occurrence of @c, or 1 byte past
 * the area if @c is not found
 */
void * memscan(void * addr, int c, size_t size)
{
	unsigned char * p = (unsigned char *) addr;

	while (size) {
		if (*p == c)
			return (void *) p;
		p++;
		size--;
	}
  	return (void *) p;
}

