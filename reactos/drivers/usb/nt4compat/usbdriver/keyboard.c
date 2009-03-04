/*
 * PROJECT:         ReactOS USB Drivers
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            keyboard.c
 * PURPOSE:         Generic USB keyboard driver
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

#include "usbdriver.h"
#include "kbdmou.h"

/* Data for embedded drivers */
CONNECT_DATA KbdClassInformation;

PDEVICE_OBJECT KeyboardFdo = NULL;

NTSTATUS
AddRegistryEntry(IN PCWSTR PortTypeName,
                 IN PUNICODE_STRING DeviceName,
                 IN PCWSTR RegistryPath);

BOOLEAN kbd_connect(PDEV_CONNECT_DATA dev_mgr, DEV_HANDLE dev_handle);
BOOLEAN kbd_disconnect(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle);
BOOLEAN kbd_stop(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle);
VOID kbd_irq(PURB purb, PVOID pcontext);
static NTSTATUS
KeyboardCreateDevice(IN PDRIVER_OBJECT DriverObject);
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

    pdriver->driver_desc.config_val = 1;            // Configuration Value
    pdriver->driver_desc.if_num = 1;                // Interface Number
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

    // Zero out the class information structure
    RtlZeroMemory(&KbdClassInformation, sizeof(CONNECT_DATA));

    // Create the device
    KeyboardCreateDevice(dev_mgr->usb_driver_obj);

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
//    LONG i;
    PUSB_ENDPOINT_DESC pendp_desc = NULL;
    ULONG MaxPacketSize;

    usb_dbg_print(DBGLVL_MAXIMUM, ("kbd_connect(): entering...\n"));

    dev_mgr = param->dev_mgr;
    pdrvr = param->pdriver;
    pdev_ext = (PKEYBOARD_DRVR_EXTENSION)pdrvr->driver_ext;

    // Lock USB Device
    status = usb_query_and_lock_dev(dev_mgr, dev_handle, &pdev);
    if (status != STATUS_SUCCESS)
    {
        //usb_free_mem(desc_buf);
        usb_dbg_print(DBGLVL_MEDIUM, ("kbd_connect(): unable to query&lock device, status=0x%x\n", status));
        return FALSE;
    }

    // Endpoint-finding code
    if (param->if_desc)
    {
        // Get a pointer to the config packet from compdev
        PUCHAR Buffer = (PUCHAR)param->if_desc;
        ULONG Offset = 0;
        BOOLEAN FoundEndpoint = FALSE;

        // Find our the only needed endpoint descriptor
        while (Offset < 512)
        {
            pendp_desc = (PUSB_ENDPOINT_DESC)&Buffer[Offset];
            usb_dbg_print(DBGLVL_MAXIMUM, ("kbd_connect(): DescType=0x%x, Attrs=0x%x, Len=%d\n",
                pendp_desc->bDescriptorType, pendp_desc->bmAttributes, pendp_desc->bLength));

            if (pendp_desc->bDescriptorType == USB_DT_ENDPOINT &&
                pendp_desc->bLength == sizeof(USB_ENDPOINT_DESC))
            {
                // Found it
                FoundEndpoint = TRUE;
                break;
            }

            Offset += pendp_desc->bLength;
        }

        if (!FoundEndpoint)
            return FALSE;

        // FIXME: Check if it's INT endpoint
        // (pendp_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT)

        // endpoint must be IN
        if ((pendp_desc->bEndpointAddress & USB_DIR_IN) == 0)
            return FALSE;

        usb_dbg_print(DBGLVL_MAXIMUM, ("kbd_connect(): FoundEndpoint=%d\n", FoundEndpoint));
    }

    // Endpoint descriptor substitution code
    {
        ULONG if_idx, endp_idx;
        PUSB_ENDPOINT pendp;

            //FoundEndpoint = FALSE;
            for(if_idx = 0; if_idx < MAX_INTERFACES_PER_CONFIG /*pdev->usb_config->if_count*/; if_idx++)
            {
                for(endp_idx = 0; endp_idx < MAX_ENDPS_PER_IF /*pdev->usb_config->interf[if_idx].endp_count*/; endp_idx++)
                {
                    pendp = &pdev->usb_config->interf[if_idx].endp[endp_idx];

                    if (pendp->pusb_endp_desc != NULL)
                    {
                        //HACKHACK: for some reason this usb driver chooses different and completely wrong
                        //          endpoint. Since I don't have time to research this, I just find the
                        //          endpoint descriptor myself and copy it
                        memcpy(pendp->pusb_endp_desc, pendp_desc, pendp_desc->bLength);

                        usb_dbg_print(DBGLVL_MAXIMUM, ("kbd_connect(): [%i][%i] DescType=0x%x, Attrs=0x%x, Len=%d\n",if_idx, endp_idx,
                            pendp->pusb_endp_desc->bDescriptorType, pendp->pusb_endp_desc->bmAttributes, pendp->pusb_endp_desc->bLength));
                    }
                }
            }
    }

    // Unlock USB Device
    usb_unlock_dev(pdev);

    // Send URB
    purb = usb_alloc_mem(NonPagedPool, sizeof(URB));
    if (purb == NULL)
        return FALSE;
    RtlZeroMemory(purb, sizeof(URB));

    MaxPacketSize = pendp_desc->wMaxPacketSize;
    if (MaxPacketSize > 8)
        MaxPacketSize = 8;

    RtlZeroMemory(pdev_ext->kbd_old, 8);

    // Build a URB for our interrupt transfer
    UsbBuildInterruptOrBulkTransferRequest(purb,
                                           usb_make_handle((dev_handle >> 16), 0, 0),
                                           (PUCHAR)&pdev_ext->kbd_data,
                                           MaxPacketSize, //use max packet size
                                           kbd_irq,
                                           pdev_ext,
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
    KEYBOARD_INPUT_DATA KeyboardInputData[10];
    ULONG DataPrepared=0, DataConsumed, i;
    UCHAR ScanCode;
    NTSTATUS status;
    PKEYBOARD_DRVR_EXTENSION pdev_ext = (PKEYBOARD_DRVR_EXTENSION)pcontext;
    PUCHAR data = pdev_ext->kbd_data;
    PUCHAR data_old = pdev_ext->kbd_old;
    usb_dbg_print(DBGLVL_MAXIMUM, ("kbd_irq(): called\n"));

    ASSERT(purb);

    if (purb->status != STATUS_SUCCESS)
    {
        usb_dbg_print(DBGLVL_MAXIMUM, ("kbd_irq(): purb->status 0x%08X\n", purb->status));
        //return;
    }

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
    if (KbdClassInformation.ClassService && DataPrepared > 0)
    {
        KIRQL OldIrql;

        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        (*(PSERVICE_CALLBACK_ROUTINE)KbdClassInformation.ClassService)(
            KbdClassInformation.ClassDeviceObject,
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

// Dispatch routine for our IRPs
NTSTATUS
KbdDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

    usb_dbg_print(DBGLVL_MAXIMUM, ("KbdDispatch(DO %p, code 0x%lx) called\n",
        DeviceObject,
        IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.IoControlCode));

    if (DeviceObject == KeyboardFdo)
    {
        // it's keyboard's IOCTL
        PIO_STACK_LOCATION Stk;

        Irp->IoStatus.Information = 0;
        Stk = IoGetCurrentIrpStackLocation(Irp);

        switch (Stk->Parameters.DeviceIoControl.IoControlCode)
        {
        case IOCTL_INTERNAL_KEYBOARD_CONNECT:
            usb_dbg_print(DBGLVL_MAXIMUM, ("IOCTL_INTERNAL_KEYBOARD_CONNECT\n"));
            if (Stk->Parameters.DeviceIoControl.InputBufferLength <	sizeof(CONNECT_DATA)) {
                usb_dbg_print(DBGLVL_MAXIMUM, ("Keyboard IOCTL_INTERNAL_KEYBOARD_CONNECT "
                    "invalid buffer size\n"));
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                goto intcontfailure;
            }

            RtlCopyMemory(&KbdClassInformation,
                Stk->Parameters.DeviceIoControl.Type3InputBuffer,
                sizeof(CONNECT_DATA));

            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
#if 0
        case IOCTL_INTERNAL_I8042_KEYBOARD_WRITE_BUFFER:
            usb_dbg_print(DBGLVL_MAXIMUM, ("IOCTL_INTERNAL_I8042_KEYBOARD_WRITE_BUFFER\n"));
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
#endif
        case IOCTL_KEYBOARD_QUERY_ATTRIBUTES:
            usb_dbg_print(DBGLVL_MAXIMUM, ("IOCTL_KEYBOARD_QUERY_ATTRIBUTES\n"));
            if (Stk->Parameters.DeviceIoControl.OutputBufferLength <
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
            if (Stk->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(KEYBOARD_INDICATOR_PARAMETERS)) {
                    usb_dbg_print(DBGLVL_MAXIMUM, ("Keyboard IOCTL_KEYBOARD_QUERY_INDICATORS: "
                        "invalid buffer size\n"));
                    Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                    goto intcontfailure;
            }
            /*RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
            &DevExt->KeyboardIndicators,
            sizeof(KEYBOARD_INDICATOR_PARAMETERS));*/

            Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
            break;
        case IOCTL_KEYBOARD_QUERY_TYPEMATIC:
            usb_dbg_print(DBGLVL_MAXIMUM, ("IOCTL_KEYBOARD_QUERY_TYPEMATIC\n"));
            if (Stk->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(KEYBOARD_TYPEMATIC_PARAMETERS)) {
                    usb_dbg_print(DBGLVL_MAXIMUM, ("Keyboard IOCTL_KEYBOARD_QUERY_TYPEMATIC: "
                        "invalid buffer size\n"));
                    Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                    goto intcontfailure;
            }
            /*RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
            &DevExt->KeyboardTypematic,
            sizeof(KEYBOARD_TYPEMATIC_PARAMETERS));*/

            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        case IOCTL_KEYBOARD_SET_INDICATORS:
            usb_dbg_print(DBGLVL_MAXIMUM, ("IOCTL_KEYBOARD_SET_INDICATORS\n"));
            if (Stk->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(KEYBOARD_INDICATOR_PARAMETERS)) {
                    usb_dbg_print(DBGLVL_MAXIMUM, ("Keyboard IOCTL_KEYBOARD_SET_INDICTATORS: "
                        "invalid buffer size\n"));
                    Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                    goto intcontfailure;
            }

            /*RtlCopyMemory(&DevExt->KeyboardIndicators,
            Irp->AssociatedIrp.SystemBuffer,
            sizeof(KEYBOARD_INDICATOR_PARAMETERS));*/

            //DPRINT("%x\n", DevExt->KeyboardIndicators.LedFlags);

            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        case IOCTL_KEYBOARD_SET_TYPEMATIC:
            usb_dbg_print(DBGLVL_MAXIMUM, ("IOCTL_KEYBOARD_SET_TYPEMATIC\n"));
            if (Stk->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(KEYBOARD_TYPEMATIC_PARAMETERS)) {
                    usb_dbg_print(DBGLVL_MAXIMUM, ("Keyboard IOCTL_KEYBOARD_SET_TYPEMATIC "
                        "invalid buffer size\n"));
                    Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                    goto intcontfailure;
            }

            /*RtlCopyMemory(&DevExt->KeyboardTypematic,
            Irp->AssociatedIrp.SystemBuffer,
            sizeof(KEYBOARD_TYPEMATIC_PARAMETERS));*/

            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        case IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION:
            /* We should check the UnitID, but it's	kind of	pointless as
            * all keyboards are supposed to have the same one
            */
            /*RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
            &IndicatorTranslation,
            sizeof(LOCAL_KEYBOARD_INDICATOR_TRANSLATION));*/

            Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
            break;
        default:
            Irp->IoStatus.Status = STATUS_SUCCESS;//STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
intcontfailure:
        Status = Irp->IoStatus.Status;
    }


    if (Status == STATUS_INVALID_DEVICE_REQUEST)
    {
        usb_dbg_print(DBGLVL_MINIMUM, ("Invalid internal device request!\n"));
    }

    if (Status != STATUS_PENDING)
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

static NTSTATUS
KeyboardCreateDevice(IN PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\KeyboardPortUSB");
    PDEVEXT_HEADER DeviceExtension;
    PDEVICE_OBJECT Fdo;
    NTSTATUS Status;

    Status = AddRegistryEntry(L"KeyboardPort", &DeviceName, L"REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\usbdriver");
    if (!NT_SUCCESS(Status))
    {
        usb_dbg_print(DBGLVL_MINIMUM, ("AddRegistryEntry() for usb keyboard driver failed with status 0x%08lx\n", Status));
        return Status;
    }

    Status = IoCreateDevice(DriverObject,
        sizeof(DEVEXT_HEADER),
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
    DeviceExtension = (PDEVEXT_HEADER)Fdo->DeviceExtension;
    RtlZeroMemory(DeviceExtension, sizeof(DEVEXT_HEADER));

    DeviceExtension->dispatch = KbdDispatch;

    KeyboardFdo = Fdo;
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

