/**
 * gendrv.c - USB driver stack project for Windows NT 4.0
 *
 * Copyright (c) 2002-2004 Zhiming  mypublic99@yahoo.com
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program (in the main directory of the distribution, the file
 * COPYING); if not, write to the Free Software Foundation,Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

//this driver is part of the dev manager responsible to manage non-driver device
#include "usbdriver.h"
#include "gendrv.h"

#define if_dev( dev_obj ) \
( ( ( ( PGENDRV_DEVICE_EXTENSION)dev_obj->DeviceExtension )->pdriver->driver_desc.flags & USB_DRIVER_FLAG_IF_CAPABLE ) != 0 )

#define GENDRV_EXIT_DISPATCH( dev_OBJ, staTUS, iRp ) \
{\
	iRp->IoStatus.Status = staTUS;\
    if( staTUS != STATUS_PENDING)\
    {\
		IoCompleteRequest( iRp, IO_NO_INCREMENT);\
		return staTUS;\
    }\
    IoMarkIrpPending( iRp );\
	IoStartPacket( dev_OBJ, iRp, NULL, gendrv_cancel_queued_irp ); \
    return STATUS_PENDING;\
}

#define GENDRV_COMPLETE_START_IO( dev_OBJ, staTUS, iRP ) \
{\
	iRP->IoStatus.Status = staTUS;\
	if( staTUS != STATUS_PENDING )\
	{\
		IoStartNextPacket( dev_OBJ, TRUE );\
		IoCompleteRequest( iRP, IO_NO_INCREMENT );\
	}\
	return;\
}

extern POBJECT_TYPE NTSYSAPI IoDriverObjectType;

extern VOID disp_urb_completion(PURB purb, PVOID context);


VOID disp_noio_urb_completion(PURB purb, PVOID context);

NTSYSAPI NTSTATUS NTAPI ZwLoadDriver(IN PUNICODE_STRING DriverServiceName);

NTSYSAPI NTSTATUS NTAPI ZwClose(IN HANDLE Handle);

NTSYSAPI
NTSTATUS
NTAPI
ObOpenObjectByName(IN POBJECT_ATTRIBUTES ObjectAttributes,
                   IN POBJECT_TYPE ObjectType OPTIONAL,
                   IN KPROCESSOR_MODE AccessMode,
                   IN OUT PACCESS_STATE AccessState OPTIONAL,
                   IN ACCESS_MASK DesiredAccess OPTIONAL,
                   IN OUT PVOID ParseContext OPTIONAL, OUT PHANDLE Handle);

BOOLEAN gendrv_if_driver_destroy(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver);

VOID gendrv_set_cfg_completion(PURB purb, PVOID context);

BOOLEAN gendrv_connect(PDEV_CONNECT_DATA param, DEV_HANDLE dev_handle);

BOOLEAN gendrv_stop(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle);

BOOLEAN gendrv_disconnect(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle);

VOID gendrv_startio(IN PDEVICE_OBJECT dev_obj, IN PIRP irp);

VOID NTAPI gendrv_cancel_queued_irp(PDEVICE_OBJECT pdev_obj, PIRP pirp);

VOID gendrv_release_ext_drvr_entry(PGENDRV_DRVR_EXTENSION pdrvr_ext, PGENDRV_EXT_DRVR_ENTRY pentry);

VOID gendrv_clean_up_queued_irps(PDEVICE_OBJECT dev_obj);

PGENDRV_EXT_DRVR_ENTRY gendrv_alloc_ext_drvr_entry(PGENDRV_DRVR_EXTENSION pdrvr_ext);

PDRIVER_OBJECT gendrv_open_ext_driver(PUNICODE_STRING unicode_string);

NTSTATUS
gendrv_get_key_value(IN HANDLE KeyHandle, IN PWSTR ValueName, OUT PKEY_VALUE_FULL_INFORMATION * Information);

NTSTATUS
gendrv_open_reg_key(OUT PHANDLE handle,
                    IN HANDLE base_handle OPTIONAL,
                    IN PUNICODE_STRING keyname, IN ACCESS_MASK desired_access, IN BOOLEAN create);

BOOLEAN gendrv_do_disconnect(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE if_handle, BOOLEAN is_if);

BOOLEAN gendrv_do_stop(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle, BOOLEAN is_if);

NTSTATUS gendrv_send_pnp_msg(ULONG msg, PDEVICE_OBJECT pdev_obj, PVOID pctx);

BOOLEAN gendrv_delete_device(PUSB_DEV_MANAGER dev_mgr, PDEVICE_OBJECT dev_obj);

PDEVICE_OBJECT gendrv_create_device(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER gen_drvr, DEV_HANDLE dev_handle);

PDRIVER_OBJECT gendrv_load_ext_drvr(PGENDRV_DRVR_EXTENSION pdrvr_ext, PUSB_DESC_HEADER pdesc);

PDRIVER_OBJECT gendrv_find_drvr_by_key(PGENDRV_DRVR_EXTENSION pdrvr_ext, ULONG key);

ULONG gendrv_make_key(PUSB_DESC_HEADER pdesc);

BOOLEAN
gendrv_driver_init(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver)
{
    PGENDRV_DRVR_EXTENSION pdrvr_ext;

    if (dev_mgr == NULL || pdriver == NULL)
        return FALSE;

    pdriver->driver_desc.flags = USB_DRIVER_FLAG_DEV_CAPABLE;
    pdriver->driver_desc.vendor_id = 0xffff;    // USB Vendor ID
    pdriver->driver_desc.product_id = 0xffff;   // USB Product ID.
    pdriver->driver_desc.release_num = 0x100;   // Release Number of Device

    pdriver->driver_desc.config_val = 0;        // Configuration Value
    pdriver->driver_desc.if_num = 0;    // Interface Number
    pdriver->driver_desc.if_class = 0xff;       // Interface Class
    pdriver->driver_desc.if_sub_class = 0xff;   // Interface SubClass
    pdriver->driver_desc.if_protocol = 0xff;    // Interface Protocol

    pdriver->driver_desc.driver_name = "USB generic dev driver";        // Driver name for Name Registry
    pdriver->driver_desc.dev_class = USB_CLASS_VENDOR_SPEC;
    pdriver->driver_desc.dev_sub_class = 0;     // Device Subclass
    pdriver->driver_desc.dev_protocol = 0;      // Protocol Info.

    pdriver->driver_ext = usb_alloc_mem(NonPagedPool, sizeof(GENDRV_DRVR_EXTENSION));
    pdriver->driver_ext_size = sizeof(GENDRV_DRVR_EXTENSION);

    RtlZeroMemory(pdriver->driver_ext, pdriver->driver_ext_size);
    pdrvr_ext = (PGENDRV_DRVR_EXTENSION) pdriver->driver_ext;

    // InitializeListHead( &pdrvr_ext->dev_list );
    InitializeListHead(&pdrvr_ext->ext_drvr_list);
    pdrvr_ext->ext_drvr_count = 0;
    ExInitializeFastMutex(&pdrvr_ext->drvr_ext_mutex);

    pdriver->disp_tbl.version = 1;
    pdriver->disp_tbl.dev_connect = gendrv_connect;
    pdriver->disp_tbl.dev_disconnect = gendrv_disconnect;
    pdriver->disp_tbl.dev_stop = gendrv_stop;
    pdriver->disp_tbl.dev_reserved = NULL;

    return TRUE;
}

BOOLEAN
gendrv_driver_destroy(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver)
{
    return gendrv_if_driver_destroy(dev_mgr, pdriver);
}

BOOLEAN
gendrv_connect(PDEV_CONNECT_DATA param, DEV_HANDLE dev_handle)
{
    PURB purb;
    PUSB_CTRL_SETUP_PACKET psetup;
    NTSTATUS status;
    PUCHAR buf;
    LONG i;
    PUSB_CONFIGURATION_DESC pconfig_desc;
    PUSB_DEV_MANAGER dev_mgr;

    if (param == NULL || dev_handle == 0)
        return FALSE;

    dev_mgr = param->dev_mgr;

    // let's set the configuration
    purb = usb_alloc_mem(NonPagedPool, sizeof(URB));
    if (purb == NULL)
        return FALSE;

    buf = usb_alloc_mem(NonPagedPool, 512);
    if (buf == NULL)
    {
        usb_dbg_print(DBGLVL_MAXIMUM, ("gendrv_connect(): can not alloc buf\n"));
        usb_free_mem(purb);
        return FALSE;
    }

    // before we set the configuration, let's search to find if there
    // exist interfaces we supported
    psetup = (PUSB_CTRL_SETUP_PACKET) (purb)->setup_packet;
    urb_init((purb));
    purb->endp_handle = dev_handle | 0xffff;
    purb->data_buffer = buf;
    purb->data_length = 512;
    purb->completion = NULL;    // this is an immediate request, no completion required
    purb->context = NULL;
    purb->reference = 0;
    psetup->bmRequestType = 0x80;
    psetup->bRequest = USB_REQ_GET_DESCRIPTOR;
    psetup->wValue = USB_DT_CONFIG << 8;
    psetup->wIndex = 0;
    psetup->wLength = 512;

    status = usb_submit_urb(dev_mgr, purb);
    if (status == STATUS_PENDING)
    {
        TRAP();
        usb_free_mem(buf);
        usb_free_mem(purb);
        return FALSE;
    }

    // check the config desc valid
    pconfig_desc = (PUSB_CONFIGURATION_DESC) buf;
    if (pconfig_desc->wTotalLength > 512)
    {
        usb_free_mem(buf);
        usb_free_mem(purb);
        usb_dbg_print(DBGLVL_MAXIMUM, ("gendrv_connect(): error, bad configuration desc\n"));
        return FALSE;
    }

    i = pconfig_desc->bConfigurationValue;
    usb_free_mem(buf);
    buf = NULL;

    //set the configuration
    urb_init(purb);
    purb->endp_handle = dev_handle | 0xffff;
    purb->data_buffer = NULL;
    purb->data_length = 0;
    purb->completion = gendrv_set_cfg_completion;
    purb->context = dev_mgr;
    purb->reference = (ULONG) param->pdriver;
    psetup->bmRequestType = 0;
    psetup->bRequest = USB_REQ_SET_CONFIGURATION;
    psetup->wValue = (USHORT) i;
    psetup->wIndex = 0;
    psetup->wLength = 0;

    usb_dbg_print(DBGLVL_MAXIMUM, ("gendrv_connect(): start config the device, cfgval=%d\n", i));
    status = usb_submit_urb(dev_mgr, purb);

    if (status != STATUS_PENDING)
    {
        usb_free_mem(purb);

        if (status == STATUS_SUCCESS)
            return TRUE;

        return FALSE;
    }

    return TRUE;
}

BOOLEAN
gendrv_event_select_driver(PUSB_DEV pdev,       //always null. we do not use this param
                           ULONG event, ULONG context, ULONG param)
{
    //
    // try to search the registry to find one driver.
    // if found, create the PDO, load the driver. 
    // and call its AddDevice.
    //
    LONG i;
    PUSB_DRIVER pdrvr;
    PGENDRV_DRVR_EXTENSION pdrvr_ext;
    PGENDRV_EXT_DRVR_ENTRY pentry;
    PGENDRV_DEVICE_EXTENSION pdev_ext;
    PUSB_CONFIGURATION_DESC pconfig_desc;
    PUSB_DEV_MANAGER dev_mgr;

    PDEVICE_OBJECT pdev_obj;
    PDRIVER_OBJECT pdrvr_obj;
    PLIST_ENTRY pthis, pnext;

    USE_BASIC_NON_PENDING_IRQL;

    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(event);

    if (pdev == NULL)
        return FALSE;

    usb_dbg_print(DBGLVL_MAXIMUM, ("gendrv_event_select_driver(): entering...\n"));

    pdrvr = (PUSB_DRIVER) param;
    //original code: pconfig_desc = (PUSB_CONFIGURATION_DESC) pdev->desc_buf[sizeof(USB_DEVICE_DESC)];
    pconfig_desc = (PUSB_CONFIGURATION_DESC) &pdev->desc_buf[sizeof(USB_DEVICE_DESC)];
    pdrvr_ext = (PGENDRV_DRVR_EXTENSION) pdrvr->driver_ext;

    //
    // well, let's do the hard work to see if there is a class driver 
    // for this device.
    // in the event routine, we have no need to check if the device is zomb or
    // not, it must be alive there.
    //
    i = gendrv_make_key((PUSB_DESC_HEADER) pdev->pusb_dev_desc);
    if (i == -1)
    {
        return FALSE;
    }

    pdrvr_obj = gendrv_find_drvr_by_key(pdrvr_ext, (ULONG) i);
    if (!pdrvr_obj)
    {
        if ((pdrvr_obj = gendrv_load_ext_drvr(pdrvr_ext, (PUSB_DESC_HEADER) pdev->pusb_dev_desc)) == NULL)
            return FALSE;
    }

    dev_mgr = dev_mgr_from_dev(pdev);
    pdev_obj = gendrv_create_device(dev_mgr, pdrvr, usb_make_handle(pdev->dev_id, 0, 0));
    if (pdev_obj == NULL)
    {
        goto ERROR_OUT;
    }

    lock_dev(pdev, FALSE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB ||
        dev_mgr_set_driver(dev_mgr, usb_make_handle(pdev->dev_id, 0, 0), pdrvr, pdev) == FALSE)
    {
        unlock_dev(pdev, FALSE);
        gendrv_delete_device(dev_mgr, pdev_obj);
        goto ERROR_OUT;
    }

    if (pdev->usb_config)
    {
        pdev->dev_obj = pdev_obj;
    }

    unlock_dev(pdev, FALSE);

    pdev_ext = (PGENDRV_DEVICE_EXTENSION) pdev_obj->DeviceExtension;
    pdev_ext->desc_buf = usb_alloc_mem(NonPagedPool, 512);
    RtlCopyMemory(pdev_ext->desc_buf, pconfig_desc, 512);

    // insert the device to the dev_list
    ExAcquireFastMutex(&pdrvr_ext->drvr_ext_mutex);
    ListFirst(&pdrvr_ext->ext_drvr_list, pthis);
    pentry = NULL;
    while (pthis)
    {
        pentry = (PGENDRV_EXT_DRVR_ENTRY) pthis;
        if (pentry->pext_drvr == pdrvr_obj)
            break;
        ListNext(&pdrvr_ext->ext_drvr_list, pthis, pnext);
        pthis = pnext;
        pentry = NULL;
    }
    ASSERT(pentry);
    InsertTailList(&pentry->dev_list, &pdev_ext->dev_obj_link);
    pdev_ext->ext_drvr_entry = pentry;
    pentry->ref_count++;
    ExReleaseFastMutex(&pdrvr_ext->drvr_ext_mutex);

    // notify the class driver, some device comes
    gendrv_send_pnp_msg(GENDRV_MSG_ADDDEVICE, pdev_obj, pdrvr_obj);
    usb_unlock_dev(pdev);
    return TRUE;

  ERROR_OUT:

    usb_unlock_dev(pdev);
    return FALSE;
}

VOID
gendrv_set_cfg_completion(PURB purb, PVOID context)
{
    DEV_HANDLE dev_handle;
    PUSB_DEV_MANAGER dev_mgr;
    PUSB_DRIVER pdriver;
    NTSTATUS status;
    PUSB_DEV pdev;
    PUSB_EVENT pevent;
    USE_BASIC_NON_PENDING_IRQL;

    if (purb == NULL || context == NULL)
        return;

    dev_handle = purb->endp_handle & ~0xffff;
    dev_mgr = (PUSB_DEV_MANAGER) context;
    pdriver = (PUSB_DRIVER) purb->reference;

    if (purb->status != STATUS_SUCCESS)
    {
        usb_free_mem(purb);
        return;
    }

    usb_free_mem(purb);
    purb = NULL;

    // set the dev state
    status = usb_query_and_lock_dev(dev_mgr, dev_handle, &pdev);
    if (status != STATUS_SUCCESS)
    {
        usb_unlock_dev(pdev);
        return;
    }
    usb_unlock_dev(pdev);       // safe to release the pdev ref since we are in urb completion


    KeAcquireSpinLockAtDpcLevel(&dev_mgr->event_list_lock);
    lock_dev(pdev, TRUE);

    if (dev_state(pdev) >= USB_DEV_STATE_BEFORE_ZOMB)
    {
        unlock_dev(pdev, TRUE);
        KeReleaseSpinLockFromDpcLevel(&dev_mgr->event_list_lock);
        return;
    }

    if (dev_mgr_set_driver(dev_mgr, dev_handle, pdriver, pdev) == FALSE)
    {
        unlock_dev(pdev, TRUE);
        KeReleaseSpinLockFromDpcLevel(&dev_mgr->event_list_lock);
        return;
    }

    //transit the state to configured
    pdev->flags &= ~USB_DEV_STATE_MASK;
    pdev->flags |= USB_DEV_STATE_CONFIGURED;

    pevent = alloc_event(&dev_mgr->event_pool, 1);
    if (pevent == NULL)
    {
        unlock_dev(pdev, TRUE);
        KeReleaseSpinLockFromDpcLevel(&dev_mgr->event_list_lock);
    }

    pevent->flags = USB_EVENT_FLAG_ACTIVE;
    pevent->event = USB_EVENT_DEFAULT;
    pevent->pdev = pdev;
    pevent->context = 0;
    pevent->param = (ULONG) pdriver;
    pevent->pnext = 0;          //vertical queue for serialized operation
    pevent->process_event = (PROCESS_EVENT) gendrv_event_select_driver;
    pevent->process_queue = event_list_default_process_queue;

    InsertTailList(&dev_mgr->event_list, &pevent->event_link);
    KeSetEvent(&dev_mgr->wake_up_event, 0, FALSE);
    unlock_dev(pdev, TRUE);
    KeReleaseSpinLockFromDpcLevel(&dev_mgr->event_list_lock);

    return;
}


BOOLEAN
gendrv_stop(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle)
{
    if (dev_mgr == NULL)
        return FALSE;
    return gendrv_do_stop(dev_mgr, dev_handle, FALSE);
}

BOOLEAN
gendrv_disconnect(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle)
{
    if (dev_mgr == NULL)
        return FALSE;
    return gendrv_do_disconnect(dev_mgr, dev_handle, FALSE);
}

BOOLEAN
gendrv_build_reg_string(PUSB_DESC_HEADER pdesc, PUNICODE_STRING pus)
{

    CHAR desc_str[128];
    STRING atemp;

    if (pdesc == NULL || pus == NULL)
        return FALSE;

    if (pdesc->bDescriptorType == USB_DT_DEVICE)
    {
        PUSB_DEVICE_DESC pdev_desc;
        pdev_desc = (PUSB_DEVICE_DESC) pdesc;
        sprintf(desc_str, "%sv_%04x&p_%04x",
                "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ehci\\device\\",
                pdev_desc->idVendor, pdev_desc->idProduct);
    }
    else if (pdesc->bDescriptorType == USB_DT_INTERFACE)
    {
        PUSB_INTERFACE_DESC pif_desc;
        pif_desc = (PUSB_INTERFACE_DESC) pdesc;
        sprintf(desc_str, "%sc_%04x&s_%04x&p_%04x",
                "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ehci\\interface\\",
                pif_desc->bInterfaceClass, pif_desc->bInterfaceSubClass, pif_desc->bInterfaceProtocol);
    }
    else
        return FALSE;

    RtlInitString(&atemp, desc_str);
    RtlAnsiStringToUnicodeString(pus, &atemp, TRUE);
    return TRUE;
}

ULONG
gendrv_make_key(PUSB_DESC_HEADER pdesc)
{
    PUSB_DEVICE_DESC pdev_desc;
    PUSB_INTERFACE_DESC pif_desc;

    if (pdesc == NULL)
        return (ULONG) - 1;
    if (pdesc->bDescriptorType == USB_DT_DEVICE)
    {
        pdev_desc = (PUSB_DEVICE_DESC) pdesc;
        return ((((ULONG) pdev_desc->idVendor) << 16) | pdev_desc->idProduct);
    }
    else if (pdesc->bDescriptorType == USB_DT_INTERFACE)
    {
        pif_desc = (PUSB_INTERFACE_DESC) pdesc;
        return ((((ULONG) pif_desc->bInterfaceClass) << 16) |
                (((ULONG) pif_desc->bInterfaceSubClass) << 8) | ((ULONG) pif_desc->bInterfaceProtocol));
    }
    return (ULONG) - 1;
}

PDRIVER_OBJECT
gendrv_find_drvr_by_key(PGENDRV_DRVR_EXTENSION pdrvr_ext, ULONG key)
{
    PGENDRV_EXT_DRVR_ENTRY pentry;
    PLIST_ENTRY pthis, pnext;
    if (pdrvr_ext == NULL || key == (ULONG) - 1)
        return NULL;

    ExAcquireFastMutex(&pdrvr_ext->drvr_ext_mutex);
    ListFirst(&pdrvr_ext->ext_drvr_list, pthis);
    while (pthis)
    {
        pentry = (PGENDRV_EXT_DRVR_ENTRY) pthis;
        if (pentry->drvr_key == key)
        {
            ExReleaseFastMutex(&pdrvr_ext->drvr_ext_mutex);
            return pentry->pext_drvr;
        }
        ListNext(&pdrvr_ext->ext_drvr_list, pthis, pnext);
        pthis = pnext;
    }
    ExReleaseFastMutex(&pdrvr_ext->drvr_ext_mutex);

    return NULL;
}

PDRIVER_OBJECT
gendrv_load_ext_drvr(PGENDRV_DRVR_EXTENSION pdrvr_ext, PUSB_DESC_HEADER pdesc)
{
    PDRIVER_OBJECT pdrvr_obj;
    PGENDRV_EXT_DRVR_ENTRY pentry;
    UNICODE_STRING usz, svc_name, svc_key, utemp;
    PKEY_VALUE_FULL_INFORMATION val_info;
    PWCHAR val_buf;
    HANDLE handle;
    NTSTATUS status;

    if (pdrvr_ext == NULL || pdesc == NULL)
        return NULL;

    // try to search and load driver from outside
    handle = NULL;
    RtlZeroMemory(&svc_key, sizeof(svc_key));
    val_info = NULL;
    RtlInitUnicodeString(&usz, L"");
    gendrv_build_reg_string(pdesc, &usz);
DbgPrint("UHCI: Trying to load driver %wZ\n", &usz);
    if (gendrv_open_reg_key(&handle, NULL, &usz, KEY_READ, FALSE) != STATUS_SUCCESS)
    {
        goto ERROR_OUT;
    }
    if (gendrv_get_key_value(handle, L"service", &val_info) != STATUS_SUCCESS)
    {
        goto ERROR_OUT;
    }

    if (val_info->DataLength > 32)
        goto ERROR_OUT;

    val_buf = (PWCHAR) (((PBYTE) val_info) + val_info->DataOffset);
    svc_key.Length = 0, svc_key.MaximumLength = 255;
    svc_key.Buffer = usb_alloc_mem(NonPagedPool, 256);

    RtlInitUnicodeString(&utemp, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    RtlAppendUnicodeStringToString(&svc_key, &utemp);
    RtlInitUnicodeString(&svc_name, val_buf);
    RtlAppendUnicodeStringToString(&svc_key, &svc_name);

    status = ZwLoadDriver(&svc_key);
    if (status != STATUS_SUCCESS)
        goto ERROR_OUT;

    svc_key.Length = 0;
    RtlZeroMemory(svc_key.Buffer, 128);
    RtlInitUnicodeString(&svc_key, L"\\Driver\\");
    RtlAppendUnicodeStringToString(&svc_key, &svc_name);
    pdrvr_obj = gendrv_open_ext_driver(&svc_key);
    if (pdrvr_obj == NULL)
        goto ERROR_OUT;

    ExAcquireFastMutex(&pdrvr_ext->drvr_ext_mutex);

    // insert the driver to the drvr list
    pentry = gendrv_alloc_ext_drvr_entry(pdrvr_ext);
    if (pentry == NULL)
    {
        ExReleaseFastMutex(&pdrvr_ext->drvr_ext_mutex);
        ObDereferenceObject(pdrvr_obj);
        goto ERROR_OUT;
    }
    pentry->pext_drvr = pdrvr_obj;
    InsertTailList(&pdrvr_ext->ext_drvr_list, &pentry->drvr_link);
    pdrvr_ext->ext_drvr_count++;
    ExReleaseFastMutex(&pdrvr_ext->drvr_ext_mutex);
    ZwClose(handle);
    return pdrvr_obj;

ERROR_OUT:
    RtlFreeUnicodeString(&usz);
    if (val_info != NULL)
    {
        usb_free_mem(val_info);
        val_info = NULL;
    }
    if (svc_key.Buffer)
        usb_free_mem(svc_key.Buffer);

    if (handle)
        ZwClose(handle);

    return NULL;
}

VOID
gendrv_release_drvr(PGENDRV_DRVR_EXTENSION pdrvr_ext, PDRIVER_OBJECT pdrvr_obj)
{
    PLIST_ENTRY pthis, pnext;
    PGENDRV_EXT_DRVR_ENTRY pentry;

    if (pdrvr_ext == NULL || pdrvr_obj == NULL)
        return;
    ExAcquireFastMutex(&pdrvr_ext->drvr_ext_mutex);
    ListFirst(&pdrvr_ext->ext_drvr_list, pthis);
    while (pthis)
    {
        pentry = (PGENDRV_EXT_DRVR_ENTRY) pthis;
        if (pentry->pext_drvr == pdrvr_obj)
        {
            ASSERT(pentry->ref_count);
            ExReleaseFastMutex(&pdrvr_ext->drvr_ext_mutex);
            return;
        }
        ListNext(&pdrvr_ext->ext_drvr_list, pthis, pnext);
        pthis = pnext;
    }
    ExReleaseFastMutex(&pdrvr_ext->drvr_ext_mutex);
}

NTSTATUS
gendrv_send_pnp_msg(ULONG msg, PDEVICE_OBJECT pdev_obj, PVOID pctx)
{
    if (pdev_obj == NULL)
        return STATUS_INVALID_PARAMETER;

    switch (msg)
    {
        case GENDRV_MSG_ADDDEVICE:
        {
            PDRIVER_OBJECT pdrvr_obj;
            if (pctx == NULL)
                return STATUS_INVALID_PARAMETER;
            pdrvr_obj = (PDRIVER_OBJECT) pctx;
            if (pdrvr_obj->DriverExtension)
            {
                return pdrvr_obj->DriverExtension->AddDevice(pdrvr_obj, pdev_obj);
            }
            return STATUS_IO_DEVICE_ERROR;
        }
        case GENDRV_MSG_STOPDEVICE:
        case GENDRV_MSG_DISCDEVICE:
        {
            NTSTATUS status;
            IO_STACK_LOCATION *irpstack;
            IRP *irp;
            // IRP_MJ_PNP_POWER
            irp = IoAllocateIrp(2, FALSE);
            if (irp == NULL)
                return STATUS_NO_MEMORY;

            irpstack = IoGetNextIrpStackLocation(irp);
            irpstack->MajorFunction = IRP_MJ_PNP_POWER;
            irpstack->MinorFunction =
                (msg == GENDRV_MSG_STOPDEVICE) ? IRP_MN_STOP_DEVICE : IRP_MN_REMOVE_DEVICE;
            status = IoCallDriver(pdev_obj, irp);
            ASSERT(status != STATUS_PENDING);
            status = irp->IoStatus.Status;
            IoFreeIrp(irp);
            return STATUS_MORE_PROCESSING_REQUIRED;
        }
    }
    return STATUS_INVALID_PARAMETER;
}


BOOLEAN
gendrv_if_connect(PDEV_CONNECT_DATA params, DEV_HANDLE if_handle)
{
    //
    // try to search the registry to find one driver.
    // if found, create the PDO, load the driver. 
    // and call its AddDevice.
    //
    LONG if_idx, i;
    NTSTATUS status;
    PUSB_DEV pdev;
    PUSB_DRIVER pdrvr;
    PUSB_INTERFACE_DESC pif_desc;
    PGENDRV_DEVICE_EXTENSION pdev_ext;
    PUSB_CONFIGURATION_DESC pconfig_desc;
    PUSB_DEV_MANAGER dev_mgr;
    PGENDRV_DRVR_EXTENSION pdrvr_ext;
    PGENDRV_EXT_DRVR_ENTRY pentry;

    PDEVICE_OBJECT pdev_obj;
    PDRIVER_OBJECT pdrvr_obj;
    PLIST_ENTRY pthis, pnext;
    USE_BASIC_NON_PENDING_IRQL;

    pdev = NULL;
    usb_dbg_print(DBGLVL_MAXIMUM, ("gendrv_if_connect(): entering...\n"));

    if (params == NULL)
        return FALSE;

    dev_mgr = params->dev_mgr;
    pdrvr = params->pdriver;
    pdrvr_ext = (PGENDRV_DRVR_EXTENSION) pdrvr->driver_ext;

    status = usb_query_and_lock_dev(dev_mgr, if_handle, &pdev);
    if (status != STATUS_SUCCESS)
    {
        goto ERROR_OUT;
    }
    // obtain the pointer to the config desc, the dev won't go away in this routine
    pconfig_desc = pdev->usb_config->pusb_config_desc;  // 
    usb_unlock_dev(pdev);
    pdev = NULL;

    if_idx = if_idx_from_handle(if_handle);
    pif_desc = (PUSB_INTERFACE_DESC) (&pconfig_desc[1]);

    for(i = 0; i < if_idx; i++)
    {
        //search for our if
        if (usb_skip_if_and_altif((PUCHAR *) & pif_desc) == FALSE)
            break;
    }
    if (pif_desc == NULL)
        return FALSE;

    //
    // well, let's do the hard work to see if there is a class driver 
    // for this device.
    //
    i = gendrv_make_key((PUSB_DESC_HEADER) pif_desc);
    if (i == -1)
    {
        return FALSE;
    }

    pdrvr_obj = gendrv_find_drvr_by_key(pdrvr_ext, (ULONG) i);
    if (!pdrvr_obj)
    {
        if ((pdrvr_obj = gendrv_load_ext_drvr(pdrvr_ext, (PUSB_DESC_HEADER) pif_desc)) == NULL)
            return FALSE;
    }


    pdev_obj = gendrv_create_device(dev_mgr, pdrvr, if_handle);
    if (pdev_obj == NULL)
    {
        goto ERROR_OUT;
    }

    lock_dev(pdev, FALSE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB ||
        dev_mgr_set_if_driver(dev_mgr, if_handle, pdrvr, pdev) == FALSE)
    {
        unlock_dev(pdev, FALSE);
        gendrv_delete_device(dev_mgr, pdev_obj);
        goto ERROR_OUT;
    }

    if (pdev->usb_config)
    {
        pdev->usb_config->interf[if_idx].if_ext = pdev_obj;
        pdev->usb_config->interf[if_idx].if_ext_size = 0;
    }

    unlock_dev(pdev, FALSE);

    pdev_ext = (PGENDRV_DEVICE_EXTENSION) pdev_obj->DeviceExtension;
    pdev_ext->desc_buf = usb_alloc_mem(NonPagedPool, 512);
    RtlCopyMemory(pdev_ext->desc_buf, pconfig_desc, 512);
    pdev_ext->if_ctx.pif_desc =
        (PUSB_INTERFACE_DESC) & pdev_ext->desc_buf[(PBYTE) pif_desc - (PBYTE) pconfig_desc];

    // insert the device to the dev_list
    ExAcquireFastMutex(&pdrvr_ext->drvr_ext_mutex);
    ListFirst(&pdrvr_ext->ext_drvr_list, pthis);
    pentry = NULL;
    while (pthis)
    {
        pentry = (PGENDRV_EXT_DRVR_ENTRY) pthis;
        if (pentry->pext_drvr == pdrvr_obj)
            break;
        ListNext(&pdrvr_ext->ext_drvr_list, pthis, pnext);
        pthis = pnext;
        pentry = NULL;
    }
    ASSERT(pentry);
    InsertTailList(&pentry->dev_list, &pdev_ext->dev_obj_link);
    pdev_ext->ext_drvr_entry = pentry;
    pentry->ref_count++;
    ExReleaseFastMutex(&pdrvr_ext->drvr_ext_mutex);

    // notify the class driver, some device comes
    gendrv_send_pnp_msg(GENDRV_MSG_ADDDEVICE, pdev_obj, pdrvr_obj);
    usb_unlock_dev(pdev);
    return TRUE;

  ERROR_OUT:

    usb_unlock_dev(pdev);
    return FALSE;
}

BOOLEAN
gendrv_do_stop(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle, BOOLEAN is_if)
{
    PUSB_DEV pdev;
    PDEVICE_OBJECT pdev_obj;
    ULONG if_idx;

    if (dev_mgr == NULL)
        return FALSE;

    // clean up the irps
    if_idx = if_idx_from_handle(dev_handle);
    if (usb_query_and_lock_dev(dev_mgr, dev_handle, &pdev) != STATUS_SUCCESS)
    {
        return FALSE;
    }
    if (is_if && pdev->usb_config)
        pdev_obj = (PDEVICE_OBJECT) pdev->usb_config->interf[if_idx].if_ext;
    else
        pdev_obj = pdev->dev_obj;

    gendrv_clean_up_queued_irps(pdev_obj);
    usb_unlock_dev(pdev);

    // send message to class drivers.
    gendrv_send_pnp_msg(GENDRV_MSG_STOPDEVICE, pdev_obj, NULL);

    return TRUE;
}

BOOLEAN
gendrv_if_stop(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle)
{
    if (dev_mgr == NULL)
        return FALSE;

    return gendrv_do_stop(dev_mgr, dev_handle, TRUE);
}

BOOLEAN
gendrv_do_disconnect(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE if_handle, BOOLEAN is_if)
{
    PUSB_DEV pdev;
    PDEVICE_OBJECT dev_obj = NULL;
    NTSTATUS status;
    PUSB_DRIVER pdrvr;
    PGENDRV_DRVR_EXTENSION pdrvr_ext = NULL;
    PGENDRV_DEVICE_EXTENSION pdev_ext = NULL;
    ULONG if_idx;

    status = usb_query_and_lock_dev(dev_mgr, if_handle, &pdev);
    if (pdev == NULL)
    {
        return FALSE;
    }
    if (status == STATUS_SUCCESS)
    {
        // must be a bug
        TRAP();
    }
    if_idx = if_idx_from_handle(if_handle);
    if (pdev->usb_config)
    {
        if (is_if)
        {
            pdrvr = pdev->usb_config->interf[if_idx].pif_drv;
            dev_obj = (PDEVICE_OBJECT) pdev->usb_config->interf[if_idx].if_ext;
        }
        else
        {
            pdrvr = pdev->dev_driver;
            dev_obj = pdev->dev_obj;
        }

        if (dev_obj == NULL)
        {
            // it means no driver was found for the device and thus no device object created
            // we just do nothing here
            return TRUE;
        }

        pdrvr_ext = (PGENDRV_DRVR_EXTENSION) pdrvr->driver_ext;
        pdev_ext = (PGENDRV_DEVICE_EXTENSION) dev_obj->DeviceExtension;
    }
    else
        TRAP();
    pdev = NULL;

    // remove the device from the list
    ExAcquireFastMutex(&pdrvr_ext->drvr_ext_mutex);
    RemoveEntryList(&pdev_ext->dev_obj_link);
    pdev_ext->ext_drvr_entry->ref_count--;
    ExReleaseFastMutex(&pdrvr_ext->drvr_ext_mutex);

    // send message to class driver
    gendrv_send_pnp_msg(GENDRV_MSG_DISCDEVICE, dev_obj, NULL);
    // delete the device object
    gendrv_delete_device(dev_mgr, dev_obj);
    return TRUE;
}

BOOLEAN
gendrv_if_disconnect(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE if_handle)
{
    return gendrv_do_disconnect(dev_mgr, if_handle, TRUE);
}

BOOLEAN
gendrv_if_driver_init(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver)
{
    PGENDRV_DRVR_EXTENSION pdrvr_ext;
    if (dev_mgr == NULL || pdriver == NULL)
        return FALSE;

    pdriver->driver_desc.flags = USB_DRIVER_FLAG_IF_CAPABLE;
    pdriver->driver_desc.vendor_id = 0x0000;    // USB Vendor ID
    pdriver->driver_desc.product_id = 0x0000;   // USB Product ID.
    pdriver->driver_desc.release_num = 0x100;   // Release Number of Device

    pdriver->driver_desc.config_val = 0;        // Configuration Value
    pdriver->driver_desc.if_num = 0;            // Interface Number
    pdriver->driver_desc.if_class = 0x0;        // Interface Class
    pdriver->driver_desc.if_sub_class = 0x0;    // Interface SubClass
    pdriver->driver_desc.if_protocol = 0x0;     // Interface Protocol

    pdriver->driver_desc.driver_name = "USB generic interface driver";  // Driver name for Name Registry
    pdriver->driver_desc.dev_class = 0;
    pdriver->driver_desc.dev_sub_class = 0;     // Device Subclass
    pdriver->driver_desc.dev_protocol = 0;      // Protocol Info.

    //we have no extra data sturcture currently

    pdriver->disp_tbl.version = 1;
    pdriver->disp_tbl.dev_connect = gendrv_if_connect;
    pdriver->disp_tbl.dev_disconnect = gendrv_if_disconnect;
    pdriver->disp_tbl.dev_stop = gendrv_if_stop;
    pdriver->disp_tbl.dev_reserved = NULL;

    pdriver->driver_ext = usb_alloc_mem(NonPagedPool, sizeof(GENDRV_DRVR_EXTENSION));
    pdriver->driver_ext_size = sizeof(GENDRV_DRVR_EXTENSION);

    RtlZeroMemory(pdriver->driver_ext, pdriver->driver_ext_size);
    pdrvr_ext = (PGENDRV_DRVR_EXTENSION) pdriver->driver_ext;

    // InitializeListHead( &pdrvr_ext->dev_list );
    InitializeListHead(&pdrvr_ext->ext_drvr_list);
    pdrvr_ext->ext_drvr_count = 0;
    ExInitializeFastMutex(&pdrvr_ext->drvr_ext_mutex);

    return TRUE;
}

BOOLEAN
gendrv_if_driver_destroy(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver)
{
    PGENDRV_DRVR_EXTENSION pdrvr_ext;
    PLIST_ENTRY pthis;
    PGENDRV_EXT_DRVR_ENTRY pentry;
    if (dev_mgr == NULL || pdriver == NULL)
        return FALSE;

    if (pdriver->driver_ext)
    {
        // should we lock it?
        // ExAcquireFastMutex( &pdrvr_ext->drvr_ext_mutex );
        pdrvr_ext = (PGENDRV_DRVR_EXTENSION) pdriver->driver_ext;
        if (pdrvr_ext->ext_drvr_count)
        {
            while (IsListEmpty(&pdrvr_ext->ext_drvr_list))
            {
                pthis = RemoveHeadList(&pdrvr_ext->ext_drvr_list);
                pentry = (PGENDRV_EXT_DRVR_ENTRY) pthis;
                if (pentry->pext_drvr)
                {
                    if (pentry->ref_count)
                    {
                        // FIXME: really fail?
                        continue;
                    }
                    ObDereferenceObject(pentry->pext_drvr);
                    gendrv_release_ext_drvr_entry(pdrvr_ext, pentry);
                }
            }
            pdrvr_ext->ext_drvr_count = 0;
        }

        usb_free_mem(pdriver->driver_ext);
        pdriver->driver_ext = NULL;
        pdriver->driver_ext_size = 0;
    }
    return TRUE;
}

PDRIVER_OBJECT
gendrv_open_ext_driver(PUNICODE_STRING unicode_string)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES oa;
    HANDLE drvr_handle;
    UNICODE_STRING oname;
    PDRIVER_OBJECT pdrvr = NULL;

    RtlZeroMemory(&oa, sizeof(oa));
    oa.Length = sizeof(oa);
    oa.ObjectName = &oname;
    oa.Attributes = OBJ_CASE_INSENSITIVE;
    RtlInitUnicodeString(&oname, L"");
    RtlAppendUnicodeStringToString(&oname, unicode_string);

    status = ObOpenObjectByName(&oa, IoDriverObjectType,   // object type
                                KernelMode,               // access mode
                                NULL,                    // access state
                                FILE_READ_DATA,         // STANDARD_RIGHTS_READ, access right
                                NULL,
                                &drvr_handle);

    if (status != STATUS_SUCCESS)
    {
        return NULL;
    }
    ObReferenceObjectByHandle(drvr_handle,
                              FILE_READ_DATA,
                              IoDriverObjectType,
                              KernelMode,
                              (PVOID)&pdrvr,
                              NULL); // OUT POBJECT_HANDLE_INFORMATION HandleInformation OPTIONAL

    ZwClose(drvr_handle);
    return pdrvr;
}

BOOLEAN
gendrv_close_ext_driver(PDRIVER_OBJECT pdrvr)
{
    if (pdrvr == NULL)
        return FALSE;
    ObDereferenceObject(pdrvr);
    return TRUE;
}

NTSTATUS
gendrv_dispatch(PDEVICE_OBJECT dev_obj, PIRP irp)
{
    IO_STACK_LOCATION *irpstack;
    PUSB_DEV_MANAGER dev_mgr;
    PDEVEXT_HEADER ext_hdr;
    NTSTATUS status;

    if (dev_obj == NULL || irp == NULL)
        return STATUS_INVALID_PARAMETER;

    ext_hdr = dev_obj->DeviceExtension;
    dev_mgr = ext_hdr->dev_mgr;

    irpstack = IoGetNextIrpStackLocation(irp);
    switch (irpstack->MajorFunction)
    {
        case IRP_MJ_PNP_POWER:
        {
            irp->IoStatus.Information = 0;
            GENDRV_EXIT_DISPATCH(dev_obj, STATUS_SUCCESS, irp);
        }
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        {
            status = STATUS_NOT_SUPPORTED;
            if (irpstack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SUBMIT_URB_RD ||
                irpstack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SUBMIT_URB_WR ||
                irpstack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SUBMIT_URB_NOIO)
            {
                PURB purb;
                DEV_HANDLE endp_handle;
                PGENDRV_DEVICE_EXTENSION pdev_ext;

                pdev_ext = dev_obj->DeviceExtension;
                if (irpstack->Parameters.DeviceIoControl.InputBufferLength < sizeof(URB))
                {
                    GENDRV_EXIT_DISPATCH(dev_obj, STATUS_INVALID_PARAMETER, irp);
                }

                purb = (PURB) irp->AssociatedIrp.SystemBuffer;
                endp_handle = purb->endp_handle;
                if (purb->data_buffer == NULL || purb->data_length == 0)
                {
                    if (irpstack->Parameters.DeviceIoControl.IoControlCode != IOCTL_SUBMIT_URB_NOIO)
                    {
                        GENDRV_EXIT_DISPATCH(dev_obj, STATUS_INVALID_PARAMETER, irp);
                    }
                }
                if (!default_endp_handle(endp_handle))
                {
                    //no permit to other interface if interface dev
                    if (if_dev(dev_obj) && if_idx_from_handle(endp_handle) != pdev_ext->if_ctx.if_idx)
                        GENDRV_EXIT_DISPATCH(dev_obj, STATUS_INVALID_PARAMETER, irp);
                }

                GENDRV_EXIT_DISPATCH(dev_obj, STATUS_PENDING, irp);
            }
            else if (irpstack->Parameters.DeviceIoControl.IoControlCode == IOCTL_GET_DEV_DESC)
            {
                // this is a synchronous call, route to dev_mgr_dispatch
                return dev_mgr_dispatch(dev_mgr, irp);
            }
            else if (irpstack->Parameters.DeviceIoControl.IoControlCode == IOCTL_GET_DEV_HANDLE)
            {
                PGENDRV_DEVICE_EXTENSION pdev_ext;
                pdev_ext = dev_obj->DeviceExtension;
                if (irpstack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(LONG))
                    GENDRV_EXIT_DISPATCH(dev_obj, STATUS_INVALID_PARAMETER, irp);

                *((PLONG) irp->AssociatedIrp.SystemBuffer) = pdev_ext->dev_handle;
                irp->IoStatus.Information = sizeof(LONG);
                GENDRV_EXIT_DISPATCH(dev_obj, STATUS_SUCCESS, irp);
            }
            GENDRV_EXIT_DISPATCH(dev_obj, STATUS_NOT_SUPPORTED, irp);
        }
        case IRP_MJ_DEVICE_CONTROL:
        {
            status = STATUS_NOT_SUPPORTED;
            if (irpstack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SUBMIT_URB_RD ||
                irpstack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SUBMIT_URB_WR ||
                irpstack->Parameters.DeviceIoControl.IoControlCode == IOCTL_SUBMIT_URB_NOIO)
            {
                PURB purb;
                DEV_HANDLE endp_handle;
                PGENDRV_DEVICE_EXTENSION pdev_ext;

                pdev_ext = dev_obj->DeviceExtension;
                if (irpstack->Parameters.DeviceIoControl.InputBufferLength < sizeof(URB))
                {
                    GENDRV_EXIT_DISPATCH(dev_obj, STATUS_INVALID_PARAMETER, irp);
                }

                purb = (PURB) irp->AssociatedIrp.SystemBuffer;
                endp_handle = purb->endp_handle;
                if (!default_endp_handle(endp_handle))
                {
                    //no permit to other interface if interface dev
                    if (if_dev(dev_obj) && if_idx_from_handle(endp_handle) != pdev_ext->if_ctx.if_idx)
                        GENDRV_EXIT_DISPATCH(dev_obj, STATUS_INVALID_PARAMETER, irp);
                }

                GENDRV_EXIT_DISPATCH(dev_obj, STATUS_PENDING, irp);
            }
            else if (irpstack->Parameters.DeviceIoControl.IoControlCode == IOCTL_GET_DEV_DESC)
            {
                // this is a synchronous call, route to dev_mgr_dispatch
                return dev_mgr_dispatch(dev_mgr, irp);
            }
            else if (irpstack->Parameters.DeviceIoControl.IoControlCode == IOCTL_GET_DEV_HANDLE)
            {
                PGENDRV_DEVICE_EXTENSION pdev_ext;
                pdev_ext = dev_obj->DeviceExtension;
                if (irpstack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(LONG))
                    GENDRV_EXIT_DISPATCH(dev_obj, STATUS_INVALID_PARAMETER, irp);

                *((PLONG) irp->AssociatedIrp.SystemBuffer) = pdev_ext->dev_handle;
                irp->IoStatus.Information = sizeof(LONG);
                GENDRV_EXIT_DISPATCH(dev_obj, STATUS_SUCCESS, irp);
            }
            GENDRV_EXIT_DISPATCH(dev_obj, STATUS_NOT_SUPPORTED, irp);
        }
    }
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);

    return STATUS_NOT_SUPPORTED;
}

BOOLEAN
gendrv_init_dev_ext_hdr(PDEVICE_OBJECT dev_obj, PUSB_DEV_MANAGER dev_mgr)
{
    PDEVEXT_HEADER dev_hdr = NULL;
    if (dev_obj == NULL || dev_mgr == NULL)
        return FALSE;

    dev_hdr = (PDEVEXT_HEADER) dev_obj->DeviceExtension;
    dev_hdr->type = NTDEV_TYPE_CLIENT_DEV;
    dev_hdr->dispatch = gendrv_dispatch;
    dev_hdr->start_io = (PDRIVER_STARTIO) gendrv_startio;
    return TRUE;
}

PDEVICE_OBJECT
gendrv_create_device(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER gen_drvr, DEV_HANDLE dev_handle)
{
    BOOLEAN is_if;
    PDEVICE_OBJECT pdev;
    PGENDRV_DEVICE_EXTENSION pdev_ext;
    ULONG dev_id;
    PGENDRV_DRVR_EXTENSION pdrvr_ext;
    CHAR dev_name[64];
    STRING string;
    UNICODE_STRING name_string, symb_link;
    NTSTATUS status;

    if (dev_mgr == NULL || gen_drvr == NULL || dev_handle == 0)
        return NULL;

    is_if = (gen_drvr->driver_desc.flags & USB_DRIVER_FLAG_IF_CAPABLE) ? 1 : 0;
    usb_dbg_print(DBGLVL_MAXIMUM, ("gendrv_create_device(): entering...\n"));
    pdrvr_ext = (PGENDRV_DRVR_EXTENSION) gen_drvr->driver_ext;
    dev_id = (UCHAR) dev_id_from_handle(dev_handle);

    if (is_if == FALSE)
        sprintf(dev_name, "\\Device\\gendev_%d", (int)dev_id);
    else
        sprintf(dev_name, "\\Device\\genifdev_%d", (int)dev_id);

    RtlInitString(&string, dev_name);
    RtlAnsiStringToUnicodeString(&name_string, &string, TRUE);
    pdev = NULL;

    status = IoCreateDevice(dev_mgr->usb_driver_obj,
                            sizeof(GENDRV_DEVICE_EXTENSION), &name_string, FILE_USB_DEV_TYPE, 0, TRUE, &pdev);

    if (status == STATUS_SUCCESS)
    {
        //
        // We do direct io
        //
        pdev->Flags |= DO_DIRECT_IO;

        pdev->Flags &= ~DO_DEVICE_INITIALIZING;
        pdev->StackSize = 2;    //one for fdo, one for file device obj

        pdev_ext = (PGENDRV_DEVICE_EXTENSION) pdev->DeviceExtension;

        //may be accessed by other thread

        gendrv_init_dev_ext_hdr(pdev, dev_mgr);

        pdev_ext->dev_id = (UCHAR) dev_id;
        pdev_ext->pdo = pdev;
        pdev_ext->dev_handle = dev_handle;
        pdev_ext->dev_mgr = dev_mgr;
        pdev_ext->pdriver = gen_drvr;

        if (is_if == FALSE)
            sprintf(dev_name, "\\DosDevices\\gendev%d", (int)dev_id);
        else
            sprintf(dev_name, "\\DosDevices\\genifdev%d", (int)dev_id);

        RtlInitString(&string, dev_name);
        RtlAnsiStringToUnicodeString(&symb_link, &string, TRUE);
        IoCreateSymbolicLink(&symb_link, &name_string);
        RtlFreeUnicodeString(&symb_link);
        KeInitializeEvent(&pdev_ext->sync_event, SynchronizationEvent, FALSE);
        KeInitializeSpinLock(&pdev_ext->dev_lock);

    }
    RtlFreeUnicodeString(&name_string);
    return pdev;
}



VOID
gendrv_deferred_delete_device(PVOID context)
{
    PDEVICE_OBJECT dev_obj;
    PGENDRV_DEVICE_EXTENSION pdev_ext;
    PGENDRV_DRVR_EXTENSION pdrvr_ext;
    LARGE_INTEGER interval;

    if (context == NULL)
        return;

    dev_obj = (PDEVICE_OBJECT) context;
    pdev_ext = dev_obj->DeviceExtension;
    pdrvr_ext = (PGENDRV_DRVR_EXTENSION) pdev_ext->pdriver->driver_ext;

    interval.QuadPart = -20000; //two ms

    for(;;)
    {
        if (dev_obj->ReferenceCount)
            KeDelayExecutionThread(KernelMode, TRUE, &interval);
        else
        {
            KeDelayExecutionThread(KernelMode, TRUE, &interval);
            if (dev_obj->ReferenceCount == 0)
                break;
        }
    }
    usb_dbg_print(DBGLVL_MAXIMUM, ("gendrv_deferred_delete_device(): delete device, 0x%x\n", dev_obj));

    ExAcquireFastMutex(&pdrvr_ext->drvr_ext_mutex);
    RemoveEntryList(&pdev_ext->dev_obj_link);
    pdev_ext->ext_drvr_entry->ref_count--;
    ExReleaseFastMutex(&pdrvr_ext->drvr_ext_mutex);

    IoDeleteDevice(dev_obj);
    return;
}

BOOLEAN
gendrv_delete_device(PUSB_DEV_MANAGER dev_mgr, PDEVICE_OBJECT dev_obj)
{
    BOOLEAN is_if;
    PUSB_DRIVER pdrvr;
    PGENDRV_DEVICE_EXTENSION pdev_ext;
    CHAR dev_name[64];
    STRING string;
    UNICODE_STRING symb_link;
    PGENDRV_DRVR_EXTENSION pdrvr_ext;

    if (dev_mgr == NULL || dev_obj == 0)
        return FALSE;

    pdev_ext = (PGENDRV_DEVICE_EXTENSION) dev_obj->DeviceExtension;
    pdrvr = pdev_ext->pdriver;
    pdrvr_ext = (PGENDRV_DRVR_EXTENSION) pdrvr->driver_ext;
    is_if = (BOOLEAN) if_dev(dev_obj);
    if (is_if == FALSE)
        sprintf(dev_name, "\\DosDevices\\gendev%d", (int)pdev_ext->dev_id);
    else
        sprintf(dev_name, "\\DosDevices\\genifdev%d", (int)pdev_ext->dev_id);

    RtlInitString(&string, dev_name);
    RtlAnsiStringToUnicodeString(&symb_link, &string, TRUE);
    IoDeleteSymbolicLink(&symb_link);
    RtlFreeUnicodeString(&symb_link);

    if (pdev_ext->desc_buf)
    {
        usb_dbg_print(DBGLVL_MAXIMUM, ("gendrv_delete_device(): delete desc_buf\n"));
        usb_free_mem(pdev_ext->desc_buf);
        pdev_ext->desc_buf = NULL;

    }

    if (dev_obj->ReferenceCount == 0)
    {
        ExAcquireFastMutex(&pdrvr_ext->drvr_ext_mutex);
        RemoveEntryList(&pdev_ext->dev_obj_link);
        pdev_ext->ext_drvr_entry->ref_count--;  //the ext_drvr_entry is actually in pdrvr_ext, so lock it.
        ExReleaseFastMutex(&pdrvr_ext->drvr_ext_mutex);

        IoDeleteDevice(dev_obj);
        return TRUE;
    }

    // borrow from umss's work routine
    return umss_schedule_workitem(dev_obj, gendrv_deferred_delete_device, NULL, 0);
}

// must have the drvr_ext_mutex acquired.
PGENDRV_EXT_DRVR_ENTRY
gendrv_alloc_ext_drvr_entry(PGENDRV_DRVR_EXTENSION pdrvr_ext)
{
    LONG i;
    if (pdrvr_ext == NULL)
        return NULL;
    if (pdrvr_ext->ext_drvr_count == GENDRV_MAX_EXT_DRVR)
        return NULL;
    for(i = 0; i < GENDRV_MAX_EXT_DRVR; i++)
    {
        if (pdrvr_ext->ext_drvr_array[i].drvr_link.Flink == NULL &&
            pdrvr_ext->ext_drvr_array[i].drvr_link.Blink == NULL)
        {
            return &pdrvr_ext->ext_drvr_array[i];
        }
    }
    return NULL;
}

// must have the drvr_ext_mutex acquired.
VOID
gendrv_release_ext_drvr_entry(PGENDRV_DRVR_EXTENSION pdrvr_ext, PGENDRV_EXT_DRVR_ENTRY pentry)
{
    if (pdrvr_ext == NULL || pentry == NULL)
        return;
    RtlZeroMemory(pentry, sizeof(GENDRV_EXT_DRVR_ENTRY));
    InitializeListHead(&pentry->dev_list);
    return;
}

NTSTATUS
gendrv_open_reg_key(OUT PHANDLE handle,
                    IN HANDLE base_handle OPTIONAL,
                    IN PUNICODE_STRING keyname, IN ACCESS_MASK desired_access, IN BOOLEAN create)
/*++

Routine Description:

    Opens or creates a VOLATILE registry key using the name passed in based
    at the BaseHandle node.

Arguments:

    Handle - Pointer to the handle which will contain the registry key that
        was opened.

    BaseHandle - Handle to the base path from which the key must be opened.

    KeyName - Name of the Key that must be opened/created.

    DesiredAccess - Specifies the desired access that the caller needs to
        the key.

    Create - Determines if the key is to be created if it does not exist.

Return Value:

   The function value is the final status of the operation.

--*/
{
    OBJECT_ATTRIBUTES object_attr;
    ULONG disposition;

    //
    // Initialize the object for the key.
    //

    InitializeObjectAttributes(&object_attr,
                               keyname, OBJ_CASE_INSENSITIVE, base_handle, (PSECURITY_DESCRIPTOR) NULL);

    //
    // Create the key or open it, as appropriate based on the caller's
    // wishes.
    //

    if (create)
    {
        return ZwCreateKey(handle,
                           desired_access,
                           &object_attr, 0, (PUNICODE_STRING) NULL, REG_OPTION_VOLATILE, &disposition);
    }
    else
    {
        return ZwOpenKey(handle, desired_access, &object_attr);
    }
    return STATUS_INVALID_PARAMETER;
}

NTSTATUS
gendrv_get_key_value(IN HANDLE KeyHandle, IN PWSTR ValueName, OUT PKEY_VALUE_FULL_INFORMATION * Information)
/*++

Routine Description:

    This routine is invoked to retrieve the data for a registry key's value.
    This is done by querying the value of the key with a zero-length buffer
    to determine the size of the value, and then allocating a buffer and
    actually querying the value into the buffer.

    It is the responsibility of the caller to free the buffer.

Arguments:

    KeyHandle - Supplies the key handle whose value is to be queried

    ValueName - Supplies the null-terminated Unicode name of the value.

    Information - Returns a pointer to the allocated data buffer.

Return Value:

    The function value is the final status of the query operation.

--*/
{
    UNICODE_STRING unicodeString;
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION infoBuffer;
    ULONG keyValueLength;

    PAGED_CODE();

    RtlInitUnicodeString(&unicodeString, ValueName);

    //
    // Figure out how big the data value is so that a buffer of the
    // appropriate size can be allocated.
    //

    status = ZwQueryValueKey(KeyHandle,
                             &unicodeString, KeyValueFullInformation, (PVOID) NULL, 0, &keyValueLength);
    if (status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL)
    {
        return status;
    }

    //
    // Allocate a buffer large enough to contain the entire key data value.
    //

    infoBuffer = usb_alloc_mem(NonPagedPool, keyValueLength);
    if (!infoBuffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Query the data for the key value.
    //

    status = ZwQueryValueKey(KeyHandle,
                             &unicodeString,
                             KeyValueFullInformation, infoBuffer, keyValueLength, &keyValueLength);
    if (!NT_SUCCESS(status))
    {
        usb_free_mem(infoBuffer);
        return status;
    }

    //
    // Everything worked, so simply return the address of the allocated
    // buffer to the caller, who is now responsible for freeing it.
    //

    *Information = infoBuffer;
    return STATUS_SUCCESS;
}

VOID
gendrv_startio(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    PIO_STACK_LOCATION irp_stack;
    ULONG ctrl_code;
    PUSB_DEV_MANAGER dev_mgr;
    USE_NON_PENDING_IRQL;

    if (dev_obj == NULL || irp == NULL)
        return;

    // standard process from walter oney
    IoAcquireCancelSpinLock(&old_irql);
    if (irp != dev_obj->CurrentIrp || irp->Cancel)
    {
        // already move on to other irp
        IoReleaseCancelSpinLock(old_irql);
        return;
    }
    else
    {
        (void)IoSetCancelRoutine(irp, NULL);
    }
    IoReleaseCancelSpinLock(old_irql);

    irp->IoStatus.Information = 0;

    irp_stack = IoGetCurrentIrpStackLocation(irp);
    ctrl_code = irp_stack->Parameters.DeviceIoControl.IoControlCode;
    dev_mgr = ((PDEVEXT_HEADER) dev_obj->DeviceExtension)->dev_mgr;

    if (irp_stack->MajorFunction != IRP_MJ_DEVICE_CONTROL &&
        irp_stack->MajorFunction != IRP_MJ_INTERNAL_DEVICE_CONTROL)
    {
        GENDRV_COMPLETE_START_IO(dev_obj, STATUS_INVALID_DEVICE_REQUEST, irp);
    }

    switch (ctrl_code)
    {
        case IOCTL_SUBMIT_URB_RD:
        case IOCTL_SUBMIT_URB_NOIO:
        case IOCTL_SUBMIT_URB_WR:
        {
            PURB purb;
            ULONG endp_idx, if_idx, user_buffer_length = 0;
            PUCHAR user_buffer = NULL;
            PUSB_DEV pdev;
            DEV_HANDLE endp_handle;
            PUSB_ENDPOINT pendp;

            NTSTATUS status;

            if (irp_stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(URB))
            {
                GENDRV_COMPLETE_START_IO(dev_obj, STATUS_INVALID_PARAMETER, irp);
            }

            purb = (PURB) irp->AssociatedIrp.SystemBuffer;
            if (ctrl_code == IOCTL_SUBMIT_URB_RD || ctrl_code == IOCTL_SUBMIT_URB_WR)
            {
                if (irp_stack->MajorFunction == IRP_MJ_DEVICE_CONTROL)
                {
                    user_buffer = MmGetSystemAddressForMdl(irp->MdlAddress);
                    user_buffer_length = irp_stack->Parameters.DeviceIoControl.OutputBufferLength;
                    if (user_buffer_length == 0)
                        GENDRV_COMPLETE_START_IO(dev_obj, STATUS_INVALID_PARAMETER, irp);
                }
                else
                {
                    if (purb->data_buffer == NULL || purb->data_length == 0)
                        GENDRV_COMPLETE_START_IO(dev_obj, STATUS_INVALID_PARAMETER, irp);
                    user_buffer = purb->data_buffer;
                    user_buffer_length = purb->data_length;
                }
            }

            purb->reference = 0;
            endp_handle = purb->endp_handle;

            if (usb_query_and_lock_dev(dev_mgr, endp_handle & ~0xffff, &pdev) != STATUS_SUCCESS)
            {
                GENDRV_COMPLETE_START_IO(dev_obj, STATUS_IO_DEVICE_ERROR, irp);
            }

            lock_dev(pdev, FALSE);
            if (dev_state(pdev) == USB_DEV_STATE_ZOMB || (dev_state(pdev) < USB_DEV_STATE_ADDRESSED))

            {
                status = STATUS_INVALID_DEVICE_STATE;
                goto ERROR_OUT1;
            }

            if (dev_state(pdev) == USB_DEV_STATE_ADDRESSED && !default_endp_handle(endp_handle))
            {
                status = STATUS_DEVICE_NOT_READY;
                goto ERROR_OUT1;
            }

            if_idx = if_idx_from_handle(endp_handle);
            endp_idx = endp_idx_from_handle(endp_handle);

            //if_idx exceeds the upper limit
            if (pdev->usb_config)
            {
                if (if_idx >= pdev->usb_config->if_count
                    || endp_idx >= pdev->usb_config->interf[if_idx].endp_count)
                {
                    if (!default_endp_handle(endp_handle))
                    {
                        status = STATUS_INVALID_DEVICE_STATE;
                        goto ERROR_OUT1;
                    }
                }
            }

            endp_from_handle(pdev, endp_handle, pendp);

            // FIXME: don't know what evil will let loose
            if (endp_type(pendp) != USB_ENDPOINT_XFER_CONTROL)
            {
                if (user_buffer_length > 16)
                {
                    status = STATUS_INVALID_PARAMETER;
                    goto ERROR_OUT1;
                }
            }

            purb->pirp = irp;
            purb->context = dev_mgr;
            purb->reference = ctrl_code;

            if (ctrl_code == IOCTL_SUBMIT_URB_RD || ctrl_code == IOCTL_SUBMIT_URB_WR)
            {
                purb->data_buffer = user_buffer;
                purb->data_length = user_buffer_length;
                purb->completion = disp_urb_completion;
            }
            else
            {
                purb->completion = disp_noio_urb_completion;
            }

            unlock_dev(pdev, FALSE);

            //
            // we have to register irp before the urb is scheduled to
            // avoid race condition. 
            //
            ASSERT(dev_mgr_register_irp(dev_mgr, irp, purb));
            //
            // the irp can not be canceled at this point, since it is
            // now the current irp and not in any urb queue. dev_mgr_cancel_irp
            // can not find it and simply return.
            //
            //      FIXME: there is a time window that the irp is registered and 
            // the urb is not queued. In the meantime, the cancel
            // request may come and cause the irp removed from the irp 
            // queue while fail to cancel due to urb not in any urb queue .
            // Thus from that point on, the irp can not be canceled till it 
            // is completed or hanging there forever.
            //
            status = usb_submit_urb(dev_mgr, purb);
            if (status != STATUS_PENDING)
            {
                // unmark the pending bit
                IoGetCurrentIrpStackLocation((irp))->Control &= ~SL_PENDING_RETURNED;
                dev_mgr_remove_irp(dev_mgr, irp);
            }
            usb_unlock_dev(pdev);
            if (status != STATUS_PENDING)
            {
                irp->IoStatus.Status = status;
                GENDRV_COMPLETE_START_IO(dev_obj, status, irp);
            }
            return;

          ERROR_OUT1:
            unlock_dev(pdev, FALSE);
            usb_unlock_dev(pdev);
            irp->IoStatus.Information = 0;
            GENDRV_COMPLETE_START_IO(dev_obj, status, irp);
        }
    }
    GENDRV_COMPLETE_START_IO(dev_obj, STATUS_INVALID_DEVICE_REQUEST, irp);
}

VOID
gendrv_clean_up_queued_irps(PDEVICE_OBJECT dev_obj)
{
    // called when device may not function or about to be removed, need cleanup
    KIRQL cancelIrql;
    PIRP irp, cur_irp;
    PKDEVICE_QUEUE_ENTRY packet;
    LIST_ENTRY cancel_irps, *pthis;

    //
    // cancel all the irps in the queue
    //
    if (dev_obj == NULL)
        return;

    InitializeListHead(&cancel_irps);

    // remove the irps from device queue
    IoAcquireCancelSpinLock(&cancelIrql);
    cur_irp = dev_obj->CurrentIrp;
    while ((packet = KeRemoveDeviceQueue(&dev_obj->DeviceQueue)))
    {
        irp = struct_ptr(packet, IRP, Tail.Overlay.DeviceQueueEntry);
        InsertTailList(&cancel_irps, &irp->Tail.Overlay.DeviceQueueEntry.DeviceListEntry);
    }
    IoReleaseCancelSpinLock(cancelIrql);

    // cancel the irps in process
    // we did not cancel the current irp, it will be done by hcd when
    // disconnect is detected.
    // remove_irp_from_list( &dev_mgr->irp_list, cur_irp, dev_mgr );

    while (IsListEmpty(&cancel_irps) == FALSE)
    {
        pthis = RemoveHeadList(&cancel_irps);
        irp = struct_ptr(pthis, IRP, Tail.Overlay.DeviceQueueEntry.DeviceListEntry);
        irp->IoStatus.Information = 0;
        irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
    return;
}

VOID
NTAPI
gendrv_cancel_queued_irp(PDEVICE_OBJECT dev_obj, PIRP pirp)
{
    // cancel routine for irps queued in the device queue
    PUSB_DEV_MANAGER dev_mgr;
    PDEVEXT_HEADER pdev_ext_hdr;

    pdev_ext_hdr = (PDEVEXT_HEADER) dev_obj->DeviceExtension;
    dev_mgr = pdev_ext_hdr->dev_mgr;

    if (dev_obj->CurrentIrp == pirp)
    {
        // just before start_io set the cancel routine to null
        IoReleaseCancelSpinLock(pirp->CancelIrql);
        // we did not IoStartNextPacket, leave it for dev_mgr_cancel_irp, that
        // is user have to call CancelIo again.
        return;
    }

    KeRemoveEntryDeviceQueue(&dev_obj->DeviceQueue, &pirp->Tail.Overlay.DeviceQueueEntry);
    IoReleaseCancelSpinLock(pirp->CancelIrql);

    pirp->IoStatus.Information = 0;
    pirp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(pirp, IO_NO_INCREMENT);
    // the device queue is moved on, no need to call IoStartNextPacket
    return;
}
