/**
 * umss.c - USB driver stack project for Windows NT 4.0
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

#include "usbdriver.h"

#include <srb.h>
#include <ntddscsi.h>
#include <scsi.h>

#define UMSS_EXIT_DISPATCH( dev_OBJ, staTUS, iRp ) \
{\
	iRp->IoStatus.Status = staTUS;\
    if( staTUS != STATUS_PENDING)\
    {\
		IoCompleteRequest( iRp, IO_NO_INCREMENT);\
		return staTUS;\
    }\
    IoMarkIrpPending( iRp );\
	IoStartPacket( dev_OBJ, iRp, NULL, NULL ); \
    return STATUS_PENDING;\
}

#define UMSS_COMPLETE_START_IO( dev_OBJ, staTUS, iRP ) \
{\
	iRP->IoStatus.Status = staTUS;\
	if( staTUS != STATUS_PENDING )\
	{\
		IoStartNextPacket( dev_OBJ, FALSE );\
		IoCompleteRequest( iRP, IO_NO_INCREMENT );\
	}\
	return;\
}

extern VOID gendrv_startio(IN PDEVICE_OBJECT dev_obj, IN PIRP irp);

NTSYSAPI NTSTATUS NTAPI ZwLoadDriver(IN PUNICODE_STRING DriverServiceName);

NTSYSAPI
NTSTATUS
NTAPI
ObOpenObjectByName(IN POBJECT_ATTRIBUTES ObjectAttributes,
                   IN POBJECT_TYPE ObjectType OPTIONAL,
                   IN KPROCESSOR_MODE AccessMode,
                   IN OUT PACCESS_STATE AccessState OPTIONAL,
                   IN ACCESS_MASK DesiredAccess OPTIONAL,
                   IN OUT PVOID ParseContext OPTIONAL, OUT PHANDLE Handle);

VOID NTAPI umss_start_io(IN PDEVICE_OBJECT dev_obj, IN PIRP irp);
NTSTATUS umss_port_dispatch_routine(PDEVICE_OBJECT pdev_obj, PIRP irp);
BOOLEAN umss_connect(PDEV_CONNECT_DATA dev_mgr, DEV_HANDLE dev_handle);
BOOLEAN umss_disconnect(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle);
BOOLEAN umss_stop(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle);
NTSTATUS umss_dispatch_routine(PDEVICE_OBJECT pdev_obj, PIRP irp);
VOID umss_set_cfg_completion(PURB purb, PVOID pcontext);
VOID NTAPI umss_start_create_device(IN PVOID Parameter);
BOOLEAN umss_delete_device(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdrvr, PDEVICE_OBJECT dev_obj, BOOLEAN is_if);
BOOLEAN umss_stop(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle);
NTSTATUS umss_process_srb(PDEVICE_OBJECT dev_obj, PIRP irp);
VOID umss_load_class_driver(PVOID context);
BOOLEAN umss_tsc_to_sff(PIO_PACKET io_packet);
VOID umss_fix_sff_result(PIO_PACKET io_packet, SCSI_REQUEST_BLOCK * srb);

PDEVICE_OBJECT
umss_create_port_device(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver)
{

    // currently a port device is a connection point
    // and upper driver use this to register itself
    // with umss driver for future notification of
    // pnp event. 2004-03-22 23:12:41
    CHAR dev_name[64];
    STRING string;
    NTSTATUS status;
    PDEVICE_OBJECT pdev;
    UNICODE_STRING name_string, symb_link;
    PUMSS_PORT_DEV_EXT pdev_ext;

    sprintf(dev_name, "\\Device\\usbPort_%d", (int)0);

    RtlInitString(&string, dev_name);
    RtlAnsiStringToUnicodeString(&name_string, &string, TRUE);
    pdev = NULL;

    status = IoCreateDevice(dev_mgr->usb_driver_obj,
                            sizeof(UMSS_PORT_DEV_EXT), &name_string, FILE_USB_DEV_TYPE, 0, TRUE, &pdev);

    if (status == STATUS_SUCCESS)
    {
        //
        // We do buffered io
        //
        pdev->Flags |= DO_BUFFERED_IO;

        pdev->Flags &= ~DO_DEVICE_INITIALIZING;
        pdev->StackSize = 2;    //one for fdo, one for file device obj

        pdev_ext = (PUMSS_PORT_DEV_EXT) pdev->DeviceExtension;

        pdev_ext->dev_ext_hdr.type = NTDEV_TYPE_CLIENT_DEV;
        pdev_ext->dev_ext_hdr.dispatch = umss_port_dispatch_routine;
        pdev_ext->dev_ext_hdr.start_io = NULL;
        pdev_ext->dev_ext_hdr.dev_mgr = dev_mgr;
        pdev_ext->pdriver = pdriver;

        sprintf(dev_name, "\\DosDevices\\usbPort%d", (int)0);

        RtlInitString(&string, dev_name);
        RtlAnsiStringToUnicodeString(&symb_link, &string, TRUE);
        IoCreateSymbolicLink(&symb_link, &name_string);
        RtlFreeUnicodeString(&symb_link);

    }

    RtlFreeUnicodeString(&name_string);
    return pdev;
}

BOOLEAN
umss_delete_port_device(PDEVICE_OBJECT dev_obj)
{
    CHAR dev_name[64];
    STRING string;
    UNICODE_STRING symb_link;

    if (dev_obj == NULL)
        return FALSE;

    // remove the symbolic link
    sprintf(dev_name, "\\DosDevices\\usbPort%d", (int)0);
    RtlInitString(&string, dev_name);
    RtlAnsiStringToUnicodeString(&symb_link, &string, TRUE);
    IoDeleteSymbolicLink(&symb_link);
    RtlFreeUnicodeString(&symb_link);

    if (dev_obj->ReferenceCount == 0)
    {
        IoDeleteDevice(dev_obj);
        usb_dbg_print(DBGLVL_MAXIMUM, ("umss_delete_port_device(): port device object is removed\n"));
    }
    return TRUE;
}

// FIXME!!! there can not be sent IOCTL_SUBMIT_URB_XXX while
// the IOCTL_SUBMIT_CDB_XXX are active. may confuse the device.
// not resolved yet.
// 2004-03-22 23:12:26
NTSTATUS
umss_port_dispatch_routine(PDEVICE_OBJECT pdev_obj, PIRP irp)
{
    ULONG ctrl_code;
    NTSTATUS status;
    PIO_STACK_LOCATION irp_stack;
    PUMSS_PORT_DEV_EXT pdev_ext;
    PUMSS_DRVR_EXTENSION pdrvr_ext;

    if (pdev_obj == NULL || irp == NULL)
        return STATUS_INVALID_PARAMETER;

    status = STATUS_SUCCESS;
    irp_stack = IoGetCurrentIrpStackLocation(irp);
    ctrl_code = irp_stack->Parameters.DeviceIoControl.IoControlCode;

    pdev_ext = (PUMSS_PORT_DEV_EXT) pdev_obj->DeviceExtension;

    switch (irp_stack->MajorFunction)
    {
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        {
            switch (ctrl_code)
            {
                case IOCTL_REGISTER_DRIVER:
                {
                    PCLASS_DRV_REG_INFO pcdri;
                    if (irp_stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(CLASS_DRV_REG_INFO))
                    {
                        EXIT_DISPATCH(STATUS_INVALID_PARAMETER, irp);
                    }

                    pcdri = irp->AssociatedIrp.SystemBuffer;
                    if (pcdri->fdo_driver == NULL || pcdri->add_device == NULL || pcdri->pnp_dispatch == NULL)
                    {
                        EXIT_DISPATCH(STATUS_INVALID_PARAMETER, irp);
                    }
                    pdrvr_ext = (PUMSS_DRVR_EXTENSION) pdev_ext->pdriver->driver_ext;
                    pdrvr_ext->class_driver_info.fdo_driver = pcdri->fdo_driver;
                    pdrvr_ext->class_driver_info.add_device = pcdri->add_device;
                    pdrvr_ext->class_driver_info.pnp_dispatch = pcdri->pnp_dispatch;
                    EXIT_DISPATCH(STATUS_SUCCESS, irp);
                }
                case IOCTL_REVOKE_DRIVER:
                {
                    pdrvr_ext = (PUMSS_DRVR_EXTENSION) pdev_ext->pdriver->driver_ext;
                    pdrvr_ext->class_driver_info.fdo_driver = NULL;
                    pdrvr_ext->class_driver_info.add_device = NULL;
                    pdrvr_ext->class_driver_info.pnp_dispatch = NULL;
                    EXIT_DISPATCH(STATUS_SUCCESS, irp);
                }
            }
        }
        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:
        {
            EXIT_DISPATCH(STATUS_SUCCESS, irp);
        }
    }
    EXIT_DISPATCH(STATUS_INVALID_DEVICE_REQUEST, irp);
}

BOOLEAN
umss_driver_init(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver)
{
    PUMSS_DRVR_EXTENSION pdrvr_ext;

    if (dev_mgr == NULL || pdriver == NULL)
        return FALSE;

    //init driver structure, no PNP table functions
    pdriver->driver_desc.flags = USB_DRIVER_FLAG_DEV_CAPABLE;
    pdriver->driver_desc.vendor_id = 0x0dd8;    // USB Vendor ID
    pdriver->driver_desc.product_id = 0x0003;   // USB Product ID.
    pdriver->driver_desc.release_num = 0x100;   // Release Number of Device

    pdriver->driver_desc.config_val = 1;        // Configuration Value
    pdriver->driver_desc.if_num = 1;    // Interface Number
    pdriver->driver_desc.if_class = USB_CLASS_MASS_STORAGE;     // Interface Class
    pdriver->driver_desc.if_sub_class = 0;      // Interface SubClass
    pdriver->driver_desc.if_protocol = 0;       // Interface Protocol

    pdriver->driver_desc.driver_name = "USB Mass Storage driver";       // Driver name for Name Registry
    pdriver->driver_desc.dev_class = USB_CLASS_MASS_STORAGE;
    pdriver->driver_desc.dev_sub_class = 0;     // Device Subclass
    pdriver->driver_desc.dev_protocol = 0;      // Protocol Info.

    pdriver->driver_ext = usb_alloc_mem(NonPagedPool, sizeof(UMSS_DRVR_EXTENSION));
    pdriver->driver_ext_size = sizeof(UMSS_DRVR_EXTENSION);

    RtlZeroMemory(pdriver->driver_ext, sizeof(UMSS_DRVR_EXTENSION));
    pdrvr_ext = (PUMSS_DRVR_EXTENSION) pdriver->driver_ext;
    pdrvr_ext->dev_count = 0;
    InitializeListHead(&pdrvr_ext->dev_list);
    ExInitializeFastMutex(&pdrvr_ext->dev_list_mutex);

    pdriver->disp_tbl.version = 1;
    pdriver->disp_tbl.dev_connect = umss_connect;
    pdriver->disp_tbl.dev_disconnect = umss_disconnect;
    pdriver->disp_tbl.dev_stop = umss_stop;
    pdriver->disp_tbl.dev_reserved = NULL;

    if ((pdrvr_ext->port_dev_obj = umss_create_port_device(dev_mgr, pdriver)) == NULL)
    {
        usb_free_mem(pdriver->driver_ext);
        pdriver->driver_ext = NULL;
        pdriver->driver_ext_size = 0;
        pdriver->disp_tbl.dev_connect = NULL;
        pdriver->disp_tbl.dev_stop = NULL;
        pdriver->disp_tbl.dev_disconnect = NULL;
        return FALSE;
    }

    umss_load_class_driver(NULL);

    // umss_schedule_workitem( NULL, umss_load_class_driver, NULL, 0 );
    usb_dbg_print(DBGLVL_MAXIMUM, ("umss_driver_init(): umss driver is initialized\n"));
    return TRUE;
}

BOOLEAN
umss_driver_destroy(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver)
{
    PUMSS_DRVR_EXTENSION pdrvr_ext;
    if (dev_mgr == NULL || pdriver == NULL)
        return FALSE;

    pdrvr_ext = (PUMSS_DRVR_EXTENSION) pdriver->driver_ext;
    umss_delete_port_device(pdrvr_ext->port_dev_obj);
    pdrvr_ext->port_dev_obj = NULL;

    ASSERT(IsListEmpty(&pdrvr_ext->dev_list) == TRUE);
    usb_free_mem(pdriver->driver_ext);
    pdriver->driver_ext = NULL;
    pdriver->driver_ext_size = 0;
    usb_dbg_print(DBGLVL_MAXIMUM, ("umss_driver_destroy(): umss driver is destroyed\n"));
    return TRUE;
}

PDEVICE_OBJECT
umss_create_device(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER umss_drvr, DEV_HANDLE dev_handle, BOOLEAN is_if)
{

    CHAR dev_name[64], dev_id;
    STRING string;
    NTSTATUS status;
    PDEVICE_OBJECT pdev;
    UNICODE_STRING name_string, symb_link;
    PUMSS_DRVR_EXTENSION pdrvr_ext;
    PUMSS_DEVICE_EXTENSION pdev_ext;

    usb_dbg_print(DBGLVL_MAXIMUM, ("umss_create_device(): entering...\n"));
    pdrvr_ext = (PUMSS_DRVR_EXTENSION) umss_drvr->driver_ext;
    dev_id = (UCHAR) dev_id_from_handle(dev_handle);    // pdrvr_ext->dev_count;

    if (is_if == FALSE)
        sprintf(dev_name, "\\Device\\umssdev_%d", (int)dev_id);
    else
        sprintf(dev_name, "\\Device\\umssifdev_%d", (int)dev_id);

    RtlInitString(&string, dev_name);
    RtlAnsiStringToUnicodeString(&name_string, &string, TRUE);
    pdev = NULL;

    status = IoCreateDevice(dev_mgr->usb_driver_obj,
                            sizeof(UMSS_DEVICE_EXTENSION),
                            &name_string,
                            FILE_USB_DEV_TYPE,
                            0,
                            TRUE,
                            &pdev);

    if (status == STATUS_SUCCESS)
    {
        //
        // We do direct io
        //
        pdev->Flags |= DO_DIRECT_IO;

        pdev->Flags &= ~DO_DEVICE_INITIALIZING;
        pdev->StackSize = 2;    //one for fdo, one for file device obj

        pdev_ext = (PUMSS_DEVICE_EXTENSION) pdev->DeviceExtension;

        //may be accessed by other thread
        ExAcquireFastMutex(&pdrvr_ext->dev_list_mutex);
        InsertTailList(&pdrvr_ext->dev_list, &pdev_ext->dev_obj_link);
        pdrvr_ext->dev_count++;
        ExReleaseFastMutex(&pdrvr_ext->dev_list_mutex);

        if (is_if)
            pdev_ext->flags |= UMSS_DEV_FLAG_IF_DEV;

        pdev_ext->umss_dev_id = dev_id;
        pdev_ext->pdo = pdev;
        pdev_ext->dev_handle = dev_handle;
        pdev_ext->dev_mgr = dev_mgr;
        pdev_ext->pdriver = umss_drvr;

        pdev_ext->dev_ext_hdr.type = NTDEV_TYPE_CLIENT_DEV;
        pdev_ext->dev_ext_hdr.dispatch = umss_dispatch_routine;
        pdev_ext->dev_ext_hdr.start_io = umss_start_io;
        pdev_ext->dev_ext_hdr.dev_mgr = dev_mgr;

        if (is_if == FALSE)
            sprintf(dev_name, "\\DosDevices\\umssdev%d", (int)dev_id);
        else
            sprintf(dev_name, "\\DosDevices\\umssifdev%d", (int)dev_id);

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

BOOLEAN
umss_connect(PDEV_CONNECT_DATA param, DEV_HANDLE dev_handle)
{
    PURB purb;
    NTSTATUS status;
    PUSB_CTRL_SETUP_PACKET psetup;
    PUSB_DEV_MANAGER dev_mgr;
    PUSB_DRIVER pdrvr;

    usb_dbg_print(DBGLVL_MAXIMUM, ("umss_connect(): entering...\n"));

    dev_mgr = param->dev_mgr;
    pdrvr = param->pdriver;

    //directly set the configuration
    purb = usb_alloc_mem(NonPagedPool, sizeof(URB));
    if (purb == NULL)
        return FALSE;

    psetup = (PUSB_CTRL_SETUP_PACKET) (purb)->setup_packet;
    urb_init((purb));

    purb->endp_handle = dev_handle | 0xffff;
    purb->data_buffer = NULL;
    purb->data_length = 0;
    purb->completion = umss_set_cfg_completion;
    purb->context = dev_mgr;
    purb->reference = (LONG) pdrvr;
    psetup->bmRequestType = 0;
    psetup->bRequest = USB_REQ_SET_CONFIGURATION;
    psetup->wValue = 1;
    psetup->wIndex = 0;
    psetup->wLength = 0;

    status = usb_submit_urb(dev_mgr, purb);
    if (status != STATUS_PENDING)
    {
        usb_free_mem(purb);
        return FALSE;
    }
    return TRUE;
}

VOID
umss_set_cfg_completion(PURB purb, PVOID pcontext)
{
    PUSB_CTRL_SETUP_PACKET psetup;
    PUCHAR buf;
    PWORK_QUEUE_ITEM pwork_item;
    PUMSS_CREATE_DATA pcd;
    DEV_HANDLE dev_handle;
    NTSTATUS status;
    PUSB_DEV_MANAGER dev_mgr;
    PUSB_DRIVER pdrvr;

    if (purb == NULL || pcontext == NULL)
        return;

    dev_mgr = (PUSB_DEV_MANAGER) pcontext;
    pdrvr = (PUSB_DRIVER) purb->reference;
    dev_handle = purb->endp_handle & ~0xffff;


    if (purb->status != STATUS_SUCCESS)
    {
        usb_free_mem(purb);
        return;
    }

    buf = usb_alloc_mem(NonPagedPool, 512);
    if (buf == NULL)
    {
        usb_free_mem(purb);
        return;
    }

    //now let's get the descs, one configuration, one interface and two endpoint
    psetup = (PUSB_CTRL_SETUP_PACKET) (purb)->setup_packet;
    purb->data_buffer = buf;
    purb->data_length = 512;
    purb->completion = NULL;    //this is an immediate request, no needs completion
    purb->context = dev_mgr;
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
    }
    usb_free_mem(purb);
    purb = NULL;

    if (status != STATUS_SUCCESS)
    {
        usb_free_mem(buf);
        buf = NULL;
        return;
    }

    pcd = usb_alloc_mem(NonPagedPool, sizeof(WORK_QUEUE_ITEM) + sizeof(UMSS_CREATE_DATA));
    if (pcd == NULL)
    {
        usb_free_mem(buf);
        buf = NULL;
        return;
    }

    pcd->desc_buf = buf;
    pcd->dev_handle = dev_handle;
    pcd->dev_mgr = dev_mgr;
    pcd->pdriver = pdrvr;
    pwork_item = (PWORK_QUEUE_ITEM) (&pcd[1]);

    ExInitializeWorkItem(pwork_item, umss_start_create_device, (PVOID) pcd);
    ExQueueWorkItem(pwork_item, DelayedWorkQueue);
}

VOID NTAPI
umss_start_create_device(IN PVOID Parameter)
{
    LONG i;
    PUCHAR desc_buf;
    NTSTATUS status;
    PUSB_DEV pdev;
    DEV_HANDLE dev_handle;
    PUSB_DRIVER pdrvr;
    PDEVICE_OBJECT pdev_obj;
    PUSB_DEV_MANAGER dev_mgr;
    PUMSS_CREATE_DATA pcd;
    PUSB_INTERFACE_DESC pif_desc;
    PUSB_ENDPOINT_DESC pendp_desc;
    PUMSS_DEVICE_EXTENSION pdev_ext;
    PUSB_CONFIGURATION_DESC pconfig_desc;

    USE_BASIC_NON_PENDING_IRQL;

    if (Parameter == NULL)
        return;

    pcd = (PUMSS_CREATE_DATA) Parameter;
    desc_buf = pcd->desc_buf;
    dev_mgr = pcd->dev_mgr;
    dev_handle = pcd->dev_handle;
    pdrvr = pcd->pdriver;
    usb_free_mem(pcd);
    pcd = NULL;

    status = usb_query_and_lock_dev(dev_mgr, dev_handle, &pdev);
    if (status != STATUS_SUCCESS)
    {
        usb_free_mem(desc_buf);
        return;
    }

    pdev_obj = umss_create_device(dev_mgr, pdrvr, dev_handle, FALSE);

    lock_dev(pdev, FALSE);
    if (pdev_obj == NULL ||
        dev_state(pdev) == USB_DEV_STATE_ZOMB ||
        dev_mgr_set_driver(dev_mgr, dev_handle, pdrvr, pdev) == FALSE)
    {
        usb_free_mem(desc_buf);
        unlock_dev(pdev, FALSE);

        if (pdev_obj)
            umss_delete_device(dev_mgr, pdrvr, pdev_obj, FALSE);

        usb_unlock_dev(pdev);
        return;
    }
    unlock_dev(pdev, FALSE);

    pdev_ext = (PUMSS_DEVICE_EXTENSION) pdev_obj->DeviceExtension;

    pdev_ext->desc_buf = desc_buf;
    pdev_ext->pif_desc = NULL;
    pdev_ext->pin_endp_desc = pdev_ext->pout_endp_desc = NULL;

    pconfig_desc = (PUSB_CONFIGURATION_DESC) desc_buf;
    pif_desc = (PUSB_INTERFACE_DESC) (&pconfig_desc[1]);
    //search for our if
    for(i = 0; ((UCHAR) i) < pconfig_desc->bNumInterfaces; i++)
    {
        if (pif_desc->bLength == sizeof(USB_INTERFACE_DESC) && pif_desc->bDescriptorType == USB_DT_INTERFACE)
        {
            if (pif_desc->bInterfaceClass == USB_CLASS_MASS_STORAGE)
            {
                pdev_ext->pif_desc = pif_desc;
                pdev_ext->if_idx = (UCHAR) i;
                break;
            }
            else
            {
                if (usb_skip_if_and_altif((PUCHAR *) & pif_desc) == FALSE)
                    break;
            }
        }
        else
        {
            break;
        }
    }

    if (pdev_ext->pif_desc)
    {
        pendp_desc = (PUSB_ENDPOINT_DESC) & pif_desc[1];
        for(i = 0; ((UCHAR) i) < pif_desc->bNumEndpoints; i++)
        {
            if (pendp_desc->bDescriptorType == USB_DT_ENDPOINT
                && pendp_desc->bLength == sizeof(USB_ENDPOINT_DESC))
            {
                if ((pendp_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT)
                {
                    pdev_ext->pint_endp_desc = pendp_desc;
                    pdev_ext->int_endp_idx = (UCHAR) i;
                }
                else if ((pendp_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)
                {
                    if (pendp_desc->bEndpointAddress & USB_DIR_IN)
                    {
                        pdev_ext->pin_endp_desc = pendp_desc;
                        pdev_ext->in_endp_idx = (UCHAR) i;
                    }
                    else
                    {
                        pdev_ext->pout_endp_desc = pendp_desc;
                        pdev_ext->out_endp_idx = (UCHAR) i;
                    }
                }
                pendp_desc = &pendp_desc[1];
            }
            else
                break;
        }
    }
    usb_unlock_dev(pdev);
    return;
}

BOOLEAN
umss_stop(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle)
{
    UNREFERENCED_PARAMETER(dev_handle);
    UNREFERENCED_PARAMETER(dev_mgr);
    return TRUE;
}

BOOLEAN
umss_disconnect(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle)
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

    return umss_delete_device(dev_mgr, pdrvr, dev_obj, FALSE);
}

VOID
umss_deferred_delete_device(PVOID context)
{
    PDEVICE_OBJECT dev_obj;
    PUMSS_DEVICE_EXTENSION pdev_ext;
    PUMSS_DRVR_EXTENSION pdrvr_ext;
    LARGE_INTEGER interval;

    if (context == NULL)
        return;

    dev_obj = (PDEVICE_OBJECT) context;
    pdev_ext = dev_obj->DeviceExtension;
    pdrvr_ext = (PUMSS_DRVR_EXTENSION) pdev_ext->pdriver->driver_ext;

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
    usb_dbg_print(DBGLVL_MAXIMUM, ("umss_deferred_delete_device(): delete device, 0x%x\n", dev_obj));

    ExAcquireFastMutex(&pdrvr_ext->dev_list_mutex);
    RemoveEntryList(&pdev_ext->dev_obj_link);
    pdrvr_ext->dev_count--;
    ExReleaseFastMutex(&pdrvr_ext->dev_list_mutex);

    IoDeleteDevice(dev_obj);
    return;
}

BOOLEAN
umss_delete_device(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdrvr, PDEVICE_OBJECT dev_obj, BOOLEAN is_if)
{
    CHAR dev_name[64];
    STRING string;
    UNICODE_STRING symb_link;
    PUMSS_DEVICE_EXTENSION pdev_ext;
    PUMSS_DRVR_EXTENSION pdrvr_ext;

    if (dev_obj == NULL)
        return FALSE;

    if (pdrvr == NULL || dev_mgr == NULL)
        return FALSE;

    pdev_ext = (PUMSS_DEVICE_EXTENSION) dev_obj->DeviceExtension;
    pdrvr_ext = (PUMSS_DRVR_EXTENSION) pdrvr->driver_ext;
    if (is_if == FALSE)
        sprintf(dev_name, "\\DosDevices\\umssdev%d", (int)pdev_ext->umss_dev_id);
    else
        sprintf(dev_name, "\\DosDevices\\umssifdev%d", (int)pdev_ext->umss_dev_id);

    RtlInitString(&string, dev_name);
    RtlAnsiStringToUnicodeString(&symb_link, &string, TRUE);
    IoDeleteSymbolicLink(&symb_link);
    RtlFreeUnicodeString(&symb_link);

    if (pdev_ext->desc_buf)
    {
        usb_dbg_print(DBGLVL_MAXIMUM, ("umss_delete_device(): delete desc_buf\n"));
        usb_free_mem(pdev_ext->desc_buf);
        pdev_ext->desc_buf = NULL;

    }

    if (dev_obj->ReferenceCount == 0)
    {
        ExAcquireFastMutex(&pdrvr_ext->dev_list_mutex);
        RemoveEntryList(&pdev_ext->dev_obj_link);
        pdrvr_ext->dev_count--;
        ExReleaseFastMutex(&pdrvr_ext->dev_list_mutex);

        IoDeleteDevice(dev_obj);
        return TRUE;
    }

    //
    // FIXME: how if the driver unloading happens
    // since this is called in dev_mgr_disconnect_dev, umss_schedule_workitem
    // can not protect the USB_DEV object from be deleted. so the workitem
    // can not access anything relative to the USB_DEV object. In this case
    // we will tollerate the usb_query_and_lock_dev failure since it will
    // never success when come to this point, and we won't pass dev_mgr
    // and pdev to the function. But other scenarios, usb_query_and_lock_dev
    // can not fail if dev_mgr and pdev are passed valid.
    // When driver is unloading, don't know. Wish NT will unload the driver
    // only when all the devices to the driver are deleted.
    //
    umss_schedule_workitem(dev_obj, umss_deferred_delete_device, NULL, 0);
    return TRUE;
}

VOID
umss_submit_io_packet(PDEVICE_OBJECT dev_obj, PIO_PACKET io_packet)
{
    NTSTATUS status;
    PUMSS_DEVICE_EXTENSION pdev_ext;
    PUSB_DEV pdev;

    pdev_ext = dev_obj->DeviceExtension;

    // lock the dev, the pdev_ext->pif_desc won't go away.
    if ((status = usb_query_and_lock_dev(pdev_ext->dev_mgr, pdev_ext->dev_handle, &pdev)) != STATUS_SUCCESS)
    {
        usb_dbg_print(DBGLVL_MAXIMUM, ("umss_start_io(): error, device is not valid\n"));
        UMSS_COMPLETE_START_IO(dev_obj, status, io_packet->pirp);
        return;
    }

    if (pdev_ext->pif_desc->bInterfaceProtocol == PROTOCOL_BULKONLY)
    {
        status = umss_bulkonly_startio(pdev_ext, io_packet);
    }
    else if (pdev_ext->pif_desc->bInterfaceProtocol == PROTOCOL_CB
             || pdev_ext->pif_desc->bInterfaceProtocol == PROTOCOL_CBI)
    {
        status = umss_cbi_startio(pdev_ext, io_packet);
    }
    else
    {
        status = STATUS_DEVICE_PROTOCOL_ERROR;
    }
    usb_unlock_dev(pdev);
    UMSS_COMPLETE_START_IO(dev_obj, status, io_packet->pirp);
    return;
}

VOID
NTAPI
umss_start_io(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    ULONG ctrl_code;
    NTSTATUS status;
    PIO_STACK_LOCATION irp_stack;
    PUMSS_DEVICE_EXTENSION pdev_ext;
    IO_PACKET io_packet;
    PUSER_IO_PACKET user_io_packet;

    if (dev_obj == NULL || irp == NULL)
        return;

    status = STATUS_SUCCESS;

    irp_stack = IoGetCurrentIrpStackLocation(irp);
    ctrl_code = irp_stack->Parameters.DeviceIoControl.IoControlCode;
    pdev_ext = (PUMSS_DEVICE_EXTENSION) dev_obj->DeviceExtension;

    if (irp_stack->MajorFunction == IRP_MJ_SCSI)
    {
        umss_process_srb(dev_obj, irp);
        return;
    }

    if (irp_stack->MajorFunction != IRP_MJ_DEVICE_CONTROL)
    {
        UMSS_COMPLETE_START_IO(dev_obj, STATUS_INVALID_DEVICE_REQUEST, irp);
    }

    switch (ctrl_code)
    {
        case IOCTL_UMSS_SUBMIT_CDB_IN:
        case IOCTL_UMSS_SUBMIT_CDB_OUT:
        case IOCTL_UMSS_SUBMIT_CDB:
        {
            if (irp_stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(USER_IO_PACKET))
            {
                UMSS_COMPLETE_START_IO(dev_obj, STATUS_INVALID_PARAMETER, irp);
            }

            user_io_packet = (PUSER_IO_PACKET) irp->AssociatedIrp.SystemBuffer;

            if (user_io_packet->sub_class != pdev_ext->pif_desc->bInterfaceSubClass)
            {
                // not agree with the dev's subclass
                UMSS_COMPLETE_START_IO(dev_obj, STATUS_DEVICE_PROTOCOL_ERROR, irp);
            }

            RtlZeroMemory(&io_packet, sizeof(io_packet));
            io_packet.cdb_length = user_io_packet->cdb_length;
            io_packet.lun = user_io_packet->lun;

            RtlCopyMemory(io_packet.cdb, user_io_packet->cdb, MAX_CDB_LENGTH);

            if (ctrl_code == IOCTL_UMSS_SUBMIT_CDB_IN)
                io_packet.flags |= USB_DIR_IN;

            if (ctrl_code != IOCTL_UMSS_SUBMIT_CDB)
            {
                if (irp_stack->Parameters.DeviceIoControl.OutputBufferLength == 0)
                    UMSS_COMPLETE_START_IO(dev_obj, STATUS_BUFFER_TOO_SMALL, irp);

                io_packet.data_buffer = MmGetSystemAddressForMdl(irp->MdlAddress);
                io_packet.data_length = irp_stack->Parameters.DeviceIoControl.OutputBufferLength;

                if (io_packet.data_length > MAX_BULK_TRANSFER_LENGTH)
                    UMSS_COMPLETE_START_IO(dev_obj, STATUS_BUFFER_TOO_SMALL, irp);

                //synchronize the buffer
                if (io_packet.flags & USB_DIR_IN)
                    KeFlushIoBuffers(irp->MdlAddress, TRUE, TRUE);
                else
                    KeFlushIoBuffers(irp->MdlAddress, FALSE, TRUE);
            }

            io_packet.pirp = irp;
            umss_submit_io_packet(dev_obj, &io_packet);
            return;
        }
        case IOCTL_SCSI_PASS_THROUGH:
        {
            PSCSI_PASS_THROUGH pass_through;
            IO_PACKET io_packet;

            pass_through = irp->AssociatedIrp.SystemBuffer;

            if (pass_through->DataTransferLength &&
                pass_through->DataBufferOffset != sizeof(SCSI_PASS_THROUGH))
                UMSS_COMPLETE_START_IO(dev_obj, STATUS_INVALID_PARAMETER, irp);

            if (pass_through->SenseInfoLength &&
                (pass_through->SenseInfoOffset !=
                 pass_through->DataBufferOffset + pass_through->DataTransferLength))
                UMSS_COMPLETE_START_IO(dev_obj, STATUS_INVALID_PARAMETER, irp);

            if (irp_stack->Parameters.DeviceIoControl.InputBufferLength <
                (sizeof(SCSI_PASS_THROUGH) +
                 pass_through->SenseInfoLength + pass_through->DataTransferLength))
                UMSS_COMPLETE_START_IO(dev_obj, STATUS_BUFFER_TOO_SMALL, irp);

            RtlZeroMemory(&io_packet, sizeof(io_packet));

            io_packet.flags |= IOP_FLAG_SCSI_CTRL_TRANSFER;
            if (pass_through->DataIn)
                io_packet.flags |= IOP_FLAG_DIR_IN;

            io_packet.data_buffer = (PVOID) & pass_through[1];
            io_packet.data_length = pass_through->DataTransferLength;

            if (pass_through->SenseInfoLength)
            {
                io_packet.sense_data = ((PUCHAR) pass_through) + pass_through->SenseInfoOffset;
                io_packet.sense_data_length = pass_through->SenseInfoLength;
                io_packet.flags |= IOP_FLAG_REQ_SENSE;
            }

            io_packet.cdb_length = pass_through->CdbLength;
            RtlCopyMemory(io_packet.cdb, pass_through->Cdb, sizeof(io_packet.cdb));
            io_packet.lun = 0;
            io_packet.pirp = irp;
            umss_submit_io_packet(dev_obj, &io_packet);
            return;
        }
        case IOCTL_SCSI_PASS_THROUGH_DIRECT:
        {
            PSCSI_PASS_THROUGH_DIRECT pass_through_direct;
            IO_PACKET io_packet;

            pass_through_direct = irp->AssociatedIrp.SystemBuffer;

            if (pass_through_direct->SenseInfoLength &&
                pass_through_direct->SenseInfoOffset != sizeof(SCSI_PASS_THROUGH_DIRECT))
                UMSS_COMPLETE_START_IO(dev_obj, STATUS_INVALID_PARAMETER, irp);

            if (irp_stack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SCSI_PASS_THROUGH_DIRECT) + pass_through_direct->SenseInfoLength)
                UMSS_COMPLETE_START_IO(dev_obj, STATUS_BUFFER_TOO_SMALL, irp);

            RtlZeroMemory(&io_packet, sizeof(io_packet));

            io_packet.flags |= IOP_FLAG_SCSI_CTRL_TRANSFER;
            if (pass_through_direct->DataIn)
                io_packet.flags |= IOP_FLAG_DIR_IN;

            io_packet.data_buffer = pass_through_direct->DataBuffer;
            io_packet.data_length = pass_through_direct->DataTransferLength;

            if (pass_through_direct->SenseInfoLength)
            {
                io_packet.sense_data = ((PUCHAR) pass_through_direct) + pass_through_direct->SenseInfoOffset;
                io_packet.sense_data_length = pass_through_direct->SenseInfoLength;
                io_packet.flags |= IOP_FLAG_REQ_SENSE;
            }

            io_packet.cdb_length = pass_through_direct->CdbLength;
            RtlCopyMemory(io_packet.cdb, pass_through_direct->Cdb, sizeof(io_packet.cdb));
            io_packet.lun = 0;
            io_packet.pirp = irp;
            umss_submit_io_packet(dev_obj, &io_packet);
            return;
        }
        case IOCTL_SUBMIT_URB_RD:
        case IOCTL_SUBMIT_URB_NOIO:
        case IOCTL_SUBMIT_URB_WR:
        {
            gendrv_startio(dev_obj, irp);
            return;
        }
        default:
            UMSS_COMPLETE_START_IO(dev_obj, STATUS_INVALID_DEVICE_REQUEST, irp);
    }
    return;
}

// bugbug!!! there can not be sent IOCTL_SUBMIT_URB_XXX while
// the IOCTL_SUBMIT_CDB_XXX are active. may confuse the device.
// not resolved yet.
NTSTATUS
umss_dispatch_routine(PDEVICE_OBJECT pdev_obj, PIRP irp)
{
    ULONG ctrl_code;
    NTSTATUS status;
    PIO_STACK_LOCATION irp_stack;
    PUMSS_DEVICE_EXTENSION pdev_ext;
    USE_BASIC_NON_PENDING_IRQL;

    if (pdev_obj == NULL || irp == NULL)
        return STATUS_INVALID_PARAMETER;

    status = STATUS_SUCCESS;
    irp_stack = IoGetCurrentIrpStackLocation(irp);
    ctrl_code = irp_stack->Parameters.DeviceIoControl.IoControlCode;

    pdev_ext = (PUMSS_DEVICE_EXTENSION) pdev_obj->DeviceExtension;

    switch (irp_stack->MajorFunction)
    {
        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:
        {
            return dev_mgr_dispatch(pdev_ext->dev_mgr, irp);
        }
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        {
            // function code to receive scsi request
            UMSS_EXIT_DISPATCH(pdev_obj, STATUS_PENDING, irp);
        }
        case IRP_MJ_DEVICE_CONTROL:
        {
            switch (ctrl_code)
            {
                case IOCTL_UMSS_SET_FDO:
                {
                    PDEVICE_OBJECT fdo;
                    PUSB_DEV pdev;

                    if (irp_stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(PDEVICE_OBJECT))
                    {
                        EXIT_DISPATCH(STATUS_INVALID_PARAMETER, irp);
                    }

                    fdo = (PDEVICE_OBJECT) ((PULONG) irp->AssociatedIrp.SystemBuffer)[0];
                    if (fdo == NULL)
                        EXIT_DISPATCH(STATUS_INVALID_PARAMETER, irp);
                    //
                    // we have to test the usb dev's state to determine whether set or not the fdo
                    //

                    if (usb_query_and_lock_dev(pdev_ext->dev_mgr, pdev_ext->dev_handle, &pdev) !=
                        STATUS_SUCCESS)
                        EXIT_DISPATCH(STATUS_DEVICE_DOES_NOT_EXIST, irp);

                    lock_dev(pdev, FALSE);

                    if (dev_state(pdev) >= USB_DEV_STATE_BEFORE_ZOMB || dev_state(pdev) == USB_DEV_STATE_ZOMB)
                    {
                        unlock_dev(pdev, FALSE);
                        usb_unlock_dev(pdev);
                        EXIT_DISPATCH(STATUS_DEVICE_DOES_NOT_EXIST, irp);
                    }

                    pdev_ext->fdo = fdo;
                    unlock_dev(pdev, FALSE);
                    usb_unlock_dev(pdev);
                    irp->IoStatus.Information = 0;
                    EXIT_DISPATCH(STATUS_SUCCESS, irp);
                }

                case IOCTL_GET_DEV_DESC:
                {
                    PGET_DEV_DESC_REQ pgddr;
                    if (irp_stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(GET_DEV_DESC_REQ))
                    {
                        EXIT_DISPATCH(STATUS_INVALID_PARAMETER, irp);
                    }
                    pgddr = irp->AssociatedIrp.SystemBuffer;
                    if (pgddr->dev_handle != (pdev_ext->dev_handle & 0xffff0000))
                    {
                        EXIT_DISPATCH(STATUS_INVALID_PARAMETER, irp);
                    }
                    // an immediate request
                    return dev_mgr_dispatch(pdev_ext->dev_mgr, irp);
                }
                case IOCTL_SUBMIT_URB_RD:
                case IOCTL_SUBMIT_URB_NOIO:
                case IOCTL_SUBMIT_URB_WR:
                {
                    PURB purb;
                    DEV_HANDLE endp_handle;

                    if (irp_stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(URB))
                    {
                        EXIT_DISPATCH(STATUS_INVALID_PARAMETER, irp);
                    }

                    purb = (PURB) irp->AssociatedIrp.SystemBuffer;
                    endp_handle = purb->endp_handle;
                    if (!default_endp_handle(endp_handle))
                    {
                        //no permit to other interface if interface dev
                        if ((pdev_ext->flags & UMSS_DEV_FLAG_IF_DEV)
                            && if_idx_from_handle(endp_handle) != pdev_ext->if_idx)
                            EXIT_DISPATCH(STATUS_INVALID_PARAMETER, irp);
                    }
                    // FIXME: this is dangeous
                    // return dev_mgr_dispatch( pdev_ext->dev_mgr, irp );
                    UMSS_EXIT_DISPATCH(pdev_obj, STATUS_PENDING, irp);
                }
                case IOCTL_GET_DEV_HANDLE:
                {
                    if (irp_stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(LONG))
                        EXIT_DISPATCH(STATUS_INVALID_PARAMETER, irp);

                    *((PLONG) irp->AssociatedIrp.SystemBuffer) = pdev_ext->dev_handle;
                    irp->IoStatus.Information = sizeof(LONG);
                    EXIT_DISPATCH(STATUS_SUCCESS, irp);
                }

                    //
                    // request from scsi class driver
                    //
                case IOCTL_SCSI_PASS_THROUGH:
                case IOCTL_SCSI_PASS_THROUGH_DIRECT:
                    //
                    // direct cdb request
                    //
                case IOCTL_UMSS_SUBMIT_CDB:
                case IOCTL_UMSS_SUBMIT_CDB_OUT:
                case IOCTL_UMSS_SUBMIT_CDB_IN:
                {
                    UMSS_EXIT_DISPATCH(pdev_obj, STATUS_PENDING, irp);
                }
                case IOCTL_SCSI_GET_INQUIRY_DATA:
                {
                    PSCSI_ADAPTER_BUS_INFO adapter_info;
                    PSCSI_BUS_DATA bus_data;
                    PSCSI_INQUIRY_DATA inq_dat;
                    PINQUIRYDATA inq;
                    IO_PACKET io_packet;
                    ULONG required_size;

                    required_size = sizeof(SCSI_ADAPTER_BUS_INFO)
                        + sizeof(SCSI_BUS_DATA) + sizeof(SCSI_INQUIRY_DATA) + INQUIRYDATABUFFERSIZE;

                    if (irp_stack->Parameters.DeviceIoControl.OutputBufferLength < required_size)
                        UMSS_EXIT_DISPATCH(pdev_obj, STATUS_BUFFER_TOO_SMALL, irp);

                    RtlZeroMemory(&io_packet, sizeof(io_packet));

                    adapter_info = irp->AssociatedIrp.SystemBuffer;
                    adapter_info->NumberOfBuses = 1;
                    bus_data = &adapter_info->BusData[0];
                    bus_data->NumberOfLogicalUnits = 1;
                    bus_data->InitiatorBusId = 0;
                    bus_data->InquiryDataOffset = sizeof(SCSI_ADAPTER_BUS_INFO);
                    inq_dat = (PVOID) & bus_data[1];
                    inq_dat->PathId = 0;
                    inq_dat->TargetId = pdev_ext->umss_dev_id;
                    //
                    // this is the dev_id for usb dev_manager
                    //
                    inq_dat->Lun = (UCHAR) (pdev_ext->dev_handle >> 16);
                    inq_dat->DeviceClaimed = FALSE;
                    inq_dat->InquiryDataLength = 36;
                    inq_dat->NextInquiryDataOffset = 0;
                    inq = (PINQUIRYDATA) inq_dat->InquiryData;

                    RtlZeroMemory(inq, sizeof(INQUIRYDATA));
                    inq->DeviceType = DIRECT_ACCESS_DEVICE;
                    inq->DeviceTypeQualifier = 0;
                    inq->RemovableMedia = 1;

                    //
                    // pretend to comply scsi primary 2 command set
                    //

                    inq->Versions = 0x04;

                    //
                    // the format is in scsi-2 format
                    //

                    inq->ResponseDataFormat = 0x02;

                    //
                    // we are the poor scsi device
                    //

                    inq->AdditionalLength = 31;
                    inq->SoftReset = 0;
                    inq->CommandQueue = 0;
                    inq->LinkedCommands = 0;
                    inq->RelativeAddressing = 0;
                    RtlCopyMemory(&inq->VendorId, "Unknown", 7);
                    RtlCopyMemory(&inq->ProductId, "USB Mass Storage", 16);
                    irp->IoStatus.Information = required_size;
                    EXIT_DISPATCH(STATUS_SUCCESS, irp);
                }
                case IOCTL_SCSI_GET_CAPABILITIES:
                {
                    PIO_SCSI_CAPABILITIES port_cap;

                    if (irp_stack->Parameters.DeviceIoControl.OutputBufferLength <
                        sizeof(IO_SCSI_CAPABILITIES))
                        EXIT_DISPATCH(STATUS_INVALID_PARAMETER, irp);

                    port_cap = (PIO_SCSI_CAPABILITIES) irp->AssociatedIrp.SystemBuffer;
                    port_cap->Length = sizeof(IO_SCSI_CAPABILITIES);
                    port_cap->MaximumTransferLength = 65536;
                    port_cap->MaximumPhysicalPages = 65536 / PAGE_SIZE;
                    port_cap->SupportedAsynchronousEvents = 0;
                    port_cap->AlignmentMask = 0x10;
                    port_cap->TaggedQueuing = FALSE;
                    port_cap->AdapterScansDown = FALSE;
                    port_cap->AdapterUsesPio = FALSE;
                    irp->IoStatus.Information = sizeof(IO_SCSI_CAPABILITIES);
                    EXIT_DISPATCH(STATUS_SUCCESS, irp);
                }
                case IOCTL_SCSI_GET_ADDRESS:
                {
                    PSCSI_ADDRESS paddr;
                    if (irp_stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SCSI_ADDRESS))
                        EXIT_DISPATCH(STATUS_INVALID_PARAMETER, irp);

                    paddr = (PSCSI_ADDRESS) irp->AssociatedIrp.SystemBuffer;

                    paddr->Length = sizeof(SCSI_ADDRESS);
                    paddr->PortNumber = 0;
                    paddr->PathId = 0;
                    paddr->TargetId = pdev_ext->umss_dev_id;
                    paddr->Lun = (UCHAR) (pdev_ext->dev_handle >> 16);
                    irp->IoStatus.Information = sizeof(SCSI_ADDRESS);
                    EXIT_DISPATCH(STATUS_SUCCESS, irp);
                }
                case IOCTL_SCSI_RESCAN_BUS:
                {
                    irp->IoStatus.Information = 0;
                    EXIT_DISPATCH(STATUS_SUCCESS, irp);
                }
                default:
                {
                    EXIT_DISPATCH(STATUS_INVALID_DEVICE_REQUEST, irp);
                }
            }
        }
    }
    EXIT_DISPATCH(STATUS_NOT_SUPPORTED, irp);
}

VOID
umss_reset_pipe_completion(PURB purb, PVOID context)
{
    PUMSS_DEVICE_EXTENSION pdev_ext;
    if (context == NULL)
        return;

    pdev_ext = (PUMSS_DEVICE_EXTENSION) context;
    pdev_ext->reset_pipe_status = purb->status;
    KeSetEvent(&pdev_ext->sync_event, 0, FALSE);
    return;
}

//can only be called at passive level
NTSTATUS
umss_reset_pipe(PUMSS_DEVICE_EXTENSION pdev_ext, DEV_HANDLE endp_handle)
{
    NTSTATUS status;
    PUSB_DEV pdev;

    if (pdev_ext == NULL)
        return STATUS_INVALID_PARAMETER;

    status = usb_query_and_lock_dev(pdev_ext->dev_mgr, pdev_ext->dev_handle, &pdev);

    if (status != STATUS_SUCCESS)
        return STATUS_UNSUCCESSFUL;

    status = usb_reset_pipe_ex(pdev_ext->dev_mgr, endp_handle, umss_reset_pipe_completion, pdev_ext);

    if (status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&pdev_ext->sync_event, Executive, KernelMode, TRUE, NULL);
        status = pdev_ext->reset_pipe_status;
    }
    usb_unlock_dev(pdev);
    return status;
}

BOOLEAN
umss_gen_result_srb(PIO_PACKET io_packet, PSCSI_REQUEST_BLOCK srb, NTSTATUS status)
{

    if (srb == NULL || io_packet == NULL)
    {
        return FALSE;
    }
    if (status == STATUS_SUCCESS)
    {
        PULONG dest_buf, src_buf;
        ULONG i;

        srb->SrbStatus = SRB_STATUS_SUCCESS;

        io_packet->pirp->IoStatus.Information = srb->DataTransferLength;
        if ((io_packet->pirp->Flags & IRP_READ_OPERATION) && !(io_packet->pirp->Flags & IRP_PAGING_IO))
        {
            src_buf = (PULONG) io_packet->data_buffer;
            dest_buf = (PULONG) srb->DataBuffer;
            if (src_buf && dest_buf)
            {
                for(i = 0; i < (srb->DataTransferLength >> 2); i++)
                {
                    dest_buf[i] = src_buf[i];
                }
            }
        }
    }
    else if (status == STATUS_DEVICE_DOES_NOT_EXIST)
    {
        PSENSE_DATA sense_buf;
        srb->SrbStatus = SRB_STATUS_NO_DEVICE;
        srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;

        //
        // let's build the srb status for class driver
        //

        srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        sense_buf = (PSENSE_DATA) srb->SenseInfoBuffer;

        if (!(srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE))
        {
            RtlZeroMemory(srb->SenseInfoBuffer, srb->SenseInfoBufferLength);
            sense_buf->ErrorCode = 0x70;
            sense_buf->Valid = 1;
            sense_buf->SenseKey = SCSI_SENSE_NOT_READY;
            sense_buf->AdditionalSenseCode = SCSI_ADSENSE_NO_MEDIA_IN_DEVICE;
            sense_buf->AdditionalSenseLength = 10;
        }
    }
    else if (status == USB_STATUS_STALL_PID || status == USB_STATUS_CRC ||
             status == USB_STATUS_BTSTUFF || status == USB_STATUS_DATA_OVERRUN)
    {
        PSENSE_DATA sense_buf;
        srb->SrbStatus = SRB_STATUS_ERROR;
        srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;

        srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        sense_buf = (PSENSE_DATA) srb->SenseInfoBuffer;

        if (!(srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE))
        {
            RtlZeroMemory(srb->SenseInfoBuffer, srb->SenseInfoBufferLength);
            sense_buf->ErrorCode = 0x70;
            sense_buf->Valid = 1;
            sense_buf->SenseKey = SCSI_SENSE_HARDWARE_ERROR;
            sense_buf->AdditionalSenseCode = 0;
            sense_buf->AdditionalSenseLength = 10;
        }
    }
    else
    {
        srb->SrbStatus = SRB_STATUS_ERROR;
    }

    if ((io_packet->pirp->Flags & (IRP_READ_OPERATION | IRP_WRITE_OPERATION))
        && !(io_packet->pirp->Flags & IRP_PAGING_IO))
    {
        if (io_packet->data_buffer)
        {
            usb_free_mem(io_packet->data_buffer);
            io_packet->data_buffer = NULL;
        }
    }
    return TRUE;
}

BOOLEAN
umss_gen_result_ctrl(PDEVICE_OBJECT dev_obj, PIRP irp, NTSTATUS status)
{
    PIO_STACK_LOCATION irp_stack;
    ULONG ctrl_code;
    PUMSS_DEVICE_EXTENSION pdev_ext;

    if (irp == NULL)
        return FALSE;

    irp->IoStatus.Information = 0;
    irp_stack = IoGetCurrentIrpStackLocation(irp);
    ctrl_code = irp_stack->Parameters.DeviceIoControl.IoControlCode;
    pdev_ext = dev_obj->DeviceExtension;

    switch (ctrl_code)
    {
        case IOCTL_SCSI_PASS_THROUGH:
        {
            PSCSI_PASS_THROUGH pass_through;
            pass_through = irp->AssociatedIrp.SystemBuffer;
            irp->IoStatus.Status = status;

            // we have set these two value in bulkonly.c when data transfer complete
            // pass_through_direct->DataTransferLength = pdev_ext->io_packet.data_length;
            // pass_through_direct->SenseInfoLength = pdev_ext->io_packet.sense_data_length;

            if (status == STATUS_SUCCESS)
                irp->IoStatus.Information = pass_through->SenseInfoOffset + pass_through->SenseInfoLength;
            else
                pass_through->ScsiStatus = SCSISTAT_CHECK_CONDITION;
            return TRUE;
        }
        case IOCTL_SCSI_PASS_THROUGH_DIRECT:
        {
            PSCSI_PASS_THROUGH_DIRECT pass_through_direct;

            pass_through_direct = irp->AssociatedIrp.SystemBuffer;
            pass_through_direct->ScsiStatus = 0;
            irp->IoStatus.Status = status;

            // we have set these two value in bulkonly.c when data transfer complete
            // pass_through_direct->DataTransferLength = pdev_ext->io_packet.data_length;
            // pass_through_direct->SenseInfoLength = pdev_ext->io_packet.sense_data_length;

            if (status == STATUS_SUCCESS)
                irp->IoStatus.Information =
                    pass_through_direct->SenseInfoOffset + pass_through_direct->SenseInfoLength;
            else
                pass_through_direct->ScsiStatus = SCSISTAT_CHECK_CONDITION;

            return TRUE;
        }
    }
    return FALSE;
}


VOID
umss_complete_request(PUMSS_DEVICE_EXTENSION pdev_ext, NTSTATUS status)
{
    PIRP pirp;
    KIRQL old_irql;

    PDEVICE_OBJECT dev_obj;
    PIO_STACK_LOCATION irp_stack;

    usb_dbg_print(DBGLVL_MAXIMUM, ("umss_complete_request(): entering...\n"));

    pirp = pdev_ext->io_packet.pirp;
    dev_obj = pdev_ext->pdo;

    irp_stack = IoGetCurrentIrpStackLocation(pirp);

    if (pdev_ext->io_packet.flags & IOP_FLAG_SRB_TRANSFER)
    {
        if (pdev_ext->pif_desc->bInterfaceSubClass == UMSS_SUBCLASS_SFF8070I)
        {
            umss_fix_sff_result(&pdev_ext->io_packet, irp_stack->Parameters.Scsi.Srb);
        }
        umss_gen_result_srb(&pdev_ext->io_packet, irp_stack->Parameters.Scsi.Srb, status);
    }
    else if (pdev_ext->io_packet.flags & IOP_FLAG_SCSI_CTRL_TRANSFER)
        umss_gen_result_ctrl(dev_obj, pirp, status);

    //this device has its irp queued
    if (status == STATUS_CANCELLED)
    {
        usb_dbg_print(DBGLVL_MAXIMUM, ("umss_complete_request(): status of irp is cancelled\n"));
        IoAcquireCancelSpinLock(&old_irql);
        if (dev_obj->CurrentIrp == pirp)
        {
            IoReleaseCancelSpinLock(old_irql);
            IoStartNextPacket(dev_obj, FALSE);
        }
        else
        {
            KeRemoveEntryDeviceQueue(&dev_obj->DeviceQueue, &pirp->Tail.Overlay.DeviceQueueEntry);
            IoReleaseCancelSpinLock(old_irql);
        }
    }
    else
    {
        // all requests come to this point from the irp queue
        IoStartNextPacket(dev_obj, FALSE);

        // we are going to complete the request, so set it's cancel routine to NULL
        IoAcquireCancelSpinLock(&old_irql);
        (void)IoSetCancelRoutine(pirp, NULL);
        IoReleaseCancelSpinLock(old_irql);
    }

    pirp->IoStatus.Status = status;

    if (status != STATUS_SUCCESS)
        pirp->IoStatus.Information = 0;

    IoCompleteRequest(pirp, IO_NO_INCREMENT);
    return;
}

BOOLEAN
umss_if_connect(PDEV_CONNECT_DATA params, DEV_HANDLE if_handle)
{
    PURB purb;
    LONG if_idx, i;
    PUCHAR desc_buf;
    NTSTATUS status;
    PUSB_DEV pdev;
    PUSB_DRIVER pdrvr;
    PUSB_INTERFACE_DESC pif_desc;
    PUSB_CTRL_SETUP_PACKET psetup;
    PUMSS_DEVICE_EXTENSION pdev_ext;
    PUSB_CONFIGURATION_DESC pconfig_desc;
    PUSB_DEV_MANAGER dev_mgr;
    PUSB_ENDPOINT_DESC pendp_desc;
    PUMSS_DRVR_EXTENSION pdrvr_ext;
    PDEVICE_OBJECT pdev_obj;
    USE_BASIC_NON_PENDING_IRQL;

    //configuration is already set
    purb = NULL;
    desc_buf = NULL;
    pdev = NULL;

    usb_dbg_print(DBGLVL_MAXIMUM, ("umss_if_connect(): entering...\n"));

    if (params == NULL)
        return FALSE;

    dev_mgr = params->dev_mgr;
    pdrvr = params->pdriver;

    if_idx = if_idx_from_handle(if_handle);

    purb = usb_alloc_mem(NonPagedPool, sizeof(URB));
    if (purb == NULL)
        goto ERROR_OUT;

    desc_buf = usb_alloc_mem(NonPagedPool, 512);
    if (desc_buf == NULL)
        goto ERROR_OUT;

    psetup = (PUSB_CTRL_SETUP_PACKET) (purb)->setup_packet;
    urb_init((purb));

    // now let's get the descs, one configuration, one interface and two endpoint
    psetup = (PUSB_CTRL_SETUP_PACKET) (purb)->setup_packet;
    purb->endp_handle = if_handle | 0xffff;
    purb->data_buffer = desc_buf;
    purb->data_length = 512;
    purb->completion = NULL;    // this is an immediate request, no needs completion
    purb->context = dev_mgr;
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
    }
    usb_free_mem(purb);
    purb = NULL;

    if (status != STATUS_SUCCESS)
    {
        goto ERROR_OUT;
    }

    status = usb_query_and_lock_dev(dev_mgr, if_handle, &pdev);
    if (status != STATUS_SUCCESS)
    {
        goto ERROR_OUT;
    }

#ifdef _TIANSHENG_DRIVER
    if (!((pdev->pusb_dev_desc->idVendor == 0x03eb && pdev->pusb_dev_desc->idProduct == 0x2002)
          || (pdev->pusb_dev_desc->idVendor == 0x0ea0 && pdev->pusb_dev_desc->idProduct == 0x6803)
          || (pdev->pusb_dev_desc->idVendor == 0x0ef5 && pdev->pusb_dev_desc->idProduct == 0x2202)))
    {
        // check TianSheng's product
        goto ERROR_OUT;
    }
#endif

    pdev_obj = umss_create_device(dev_mgr, pdrvr, if_handle, TRUE);
    if (pdev_obj == NULL)
    {
        goto ERROR_OUT;
    }

    lock_dev(pdev, FALSE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB ||
        dev_mgr_set_if_driver(dev_mgr, if_handle, pdrvr, pdev) == FALSE)
    {
        unlock_dev(pdev, FALSE);
        if (pdev_obj)
        {
            umss_delete_device(dev_mgr, pdrvr, pdev_obj, TRUE);
        }
        goto ERROR_OUT;
    }

    if (pdev->usb_config)
    {
        pdev->usb_config->interf[if_idx].if_ext = pdev_obj;
        pdev->usb_config->interf[if_idx].if_ext_size = 0;
    }
    // olympus dev needs special care
    if (UMSS_OLYMPUS_VENDOR_ID == pdev->pusb_dev_desc->idVendor)
        status = TRUE;
    else
        status = FALSE;

    unlock_dev(pdev, FALSE);

    pdev_ext = (PUMSS_DEVICE_EXTENSION) pdev_obj->DeviceExtension;

    pdev_ext->desc_buf = desc_buf;
    pdev_ext->pif_desc = NULL;
    pdev_ext->pin_endp_desc = pdev_ext->pout_endp_desc = NULL;
    pconfig_desc = (PUSB_CONFIGURATION_DESC) desc_buf;
    pif_desc = (PUSB_INTERFACE_DESC) (&pconfig_desc[1]);

    if (status)
        pdev_ext->flags |= UMSS_DEV_FLAG_OLYMPUS_DEV;

    //search for our if
    for(i = 0; ((UCHAR) i) < if_idx; i++)
    {
        if (usb_skip_if_and_altif((PUCHAR *) & pif_desc) == FALSE)
            break;
    }
    pdev_ext->pif_desc = pif_desc;

    if (pdev_ext->pif_desc)
    {
        pendp_desc = (PUSB_ENDPOINT_DESC) & pif_desc[1];
        for(i = 0; ((UCHAR) i) < pif_desc->bNumEndpoints; i++)
        {
            if (pendp_desc->bDescriptorType == USB_DT_ENDPOINT
                && pendp_desc->bLength == sizeof(USB_ENDPOINT_DESC))
            {
                if ((pendp_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT)
                {
                    pdev_ext->pint_endp_desc = pendp_desc;
                    pdev_ext->int_endp_idx = (UCHAR) i;
                }
                else if ((pendp_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)
                {
                    if (pendp_desc->bEndpointAddress & USB_DIR_IN)
                    {
                        pdev_ext->pin_endp_desc = pendp_desc;
                        pdev_ext->in_endp_idx = (UCHAR) i;
                    }
                    else
                    {
                        pdev_ext->pout_endp_desc = pendp_desc;
                        pdev_ext->out_endp_idx = (UCHAR) i;
                    }
                }
                pendp_desc = &pendp_desc[1];
            }
            else
                break;
        }
    }

    // notify the class driver, some device comes
    pdrvr_ext = (PUMSS_DRVR_EXTENSION) pdrvr->driver_ext;
    if (pdrvr_ext && pdrvr_ext->class_driver_info.add_device && pdrvr_ext->class_driver_info.fdo_driver)
        pdrvr_ext->class_driver_info.add_device(pdrvr_ext->class_driver_info.fdo_driver, pdev_obj);

    usb_unlock_dev(pdev);
    return TRUE;

ERROR_OUT:
    if (desc_buf)
        usb_free_mem(desc_buf);

    if (purb)
        usb_free_mem(purb);

    usb_unlock_dev(pdev);

    desc_buf = NULL;
    purb = NULL;

    return FALSE;
}

BOOLEAN
umss_if_disconnect(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE if_handle)
{
    LONG if_idx;
    NTSTATUS status;
    PUSB_DEV pdev;
    PUSB_DRIVER pdrvr = NULL;
    PDEVICE_OBJECT dev_obj = NULL;
    PUMSS_DRVR_EXTENSION pdrvr_ext;
    PUMSS_DEVICE_EXTENSION pdev_ext;

    if (dev_mgr == NULL || if_handle == 0)
        return FALSE;

    pdev = NULL;
    if_idx = if_idx_from_handle(if_handle);
    //
    // special use of the lock dev, simply use this routine to get the dev
    //
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
    if (pdev->usb_config)
    {
        pdrvr = pdev->usb_config->interf[if_idx].pif_drv;
        dev_obj = (PDEVICE_OBJECT) pdev->usb_config->interf[if_idx].if_ext;
    }
    pdev = NULL;

    // notify the class driver, some device gone
    pdrvr_ext = (PUMSS_DRVR_EXTENSION) pdrvr->driver_ext;
    pdev_ext = dev_obj->DeviceExtension;
    if (pdrvr_ext && pdrvr_ext->class_driver_info.pnp_dispatch)
        pdrvr_ext->class_driver_info.pnp_dispatch(dev_obj, UMSS_PNPMSG_DISCONNECT, NULL);

    // no need to unlock the dev
    return umss_delete_device(dev_mgr, pdrvr, dev_obj, TRUE);
}

BOOLEAN
umss_if_stop(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE if_handle)
{
    LONG if_idx;
    NTSTATUS status;
    PUSB_DEV pdev;
    PUSB_DRIVER pdrvr = NULL;
    PDEVICE_OBJECT dev_obj = NULL;
    PUMSS_DRVR_EXTENSION pdrvr_ext;
    PUMSS_DEVICE_EXTENSION pdev_ext;
    USE_BASIC_NON_PENDING_IRQL;

    if (dev_mgr == NULL || if_handle == 0)
        return FALSE;

    pdev = NULL;
    if_idx = if_idx_from_handle(if_handle);

    // special use of the lock dev, simply use this routine to get the dev
    status = usb_query_and_lock_dev(dev_mgr, if_handle, &pdev);
    if (status != STATUS_SUCCESS)
    {
        return FALSE;
    }

    lock_dev(pdev, FALSE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        return FALSE;
    }

    if (pdev->usb_config)
    {
        pdrvr = pdev->usb_config->interf[if_idx].pif_drv;
        dev_obj = (PDEVICE_OBJECT) pdev->usb_config->interf[if_idx].if_ext;
    }
    unlock_dev(pdev, FALSE);

    // notify the class driver, some device stops
    pdev_ext = dev_obj->DeviceExtension;
    pdrvr_ext = (PUMSS_DRVR_EXTENSION) pdrvr->driver_ext;
    if (pdrvr_ext && pdrvr_ext->class_driver_info.pnp_dispatch)
        pdrvr_ext->class_driver_info.pnp_dispatch(dev_obj, UMSS_PNPMSG_STOP, NULL);

    usb_unlock_dev(pdev);
    return TRUE;
}

VOID
umss_load_class_driver(PVOID context)
{
    NTSTATUS status;
    UNICODE_STRING unicode_string;

    UNREFERENCED_PARAMETER(context);

    //
    // let's load the class driver
    //
    RtlInitUnicodeString(&unicode_string,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\usbstor");
    status = ZwLoadDriver(&unicode_string);
    usb_dbg_print(DBGLVL_MAXIMUM,
                  ("umss_load_class_driver(): try to load class driver, status=0x%x\n", status));
}

BOOLEAN
umss_if_driver_init(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver)
{
    PUMSS_DRVR_EXTENSION pdrvr_ext;

    if (dev_mgr == NULL || pdriver == NULL)
        return FALSE;

    //init driver structure, no PNP table functions

    pdriver->driver_desc.flags = USB_DRIVER_FLAG_IF_CAPABLE;
    pdriver->driver_desc.vendor_id = 0x0000;    // USB Vendor ID
    pdriver->driver_desc.product_id = 0x0000;   // USB Product ID.
    pdriver->driver_desc.release_num = 0x100;   // Release Number of Device

    pdriver->driver_desc.config_val = 1;        // Configuration Value
    pdriver->driver_desc.if_num = 1;            // Interface Number
    pdriver->driver_desc.if_class = USB_CLASS_MASS_STORAGE;     // Interface Class
    pdriver->driver_desc.if_sub_class = 0;      // Interface SubClass
    pdriver->driver_desc.if_protocol = 0;       // Interface Protocol

    pdriver->driver_desc.driver_name = "USB Mass Storage interface driver";     // Driver name for Name Registry
    pdriver->driver_desc.dev_class = USB_CLASS_PER_INTERFACE;
    pdriver->driver_desc.dev_sub_class = 0;     // Device Subclass
    pdriver->driver_desc.dev_protocol = 0;      // Protocol Info.

    pdriver->driver_ext = usb_alloc_mem(NonPagedPool, sizeof(UMSS_DRVR_EXTENSION));
    pdriver->driver_ext_size = sizeof(UMSS_DRVR_EXTENSION);

    RtlZeroMemory(pdriver->driver_ext, sizeof(UMSS_DRVR_EXTENSION));

    pdrvr_ext = (PUMSS_DRVR_EXTENSION) pdriver->driver_ext;
    pdrvr_ext->dev_count = 0;
    InitializeListHead(&pdrvr_ext->dev_list);
    ExInitializeFastMutex(&pdrvr_ext->dev_list_mutex);

    pdriver->disp_tbl.version = 1;
    pdriver->disp_tbl.dev_connect = umss_if_connect;
    pdriver->disp_tbl.dev_disconnect = umss_if_disconnect;
    pdriver->disp_tbl.dev_stop = umss_if_stop;
    pdriver->disp_tbl.dev_reserved = NULL;

    if ((pdrvr_ext->port_dev_obj = umss_create_port_device(dev_mgr, pdriver)) == NULL)
    {
        usb_free_mem(pdriver->driver_ext);
        pdriver->driver_ext = NULL;
        pdriver->driver_ext_size = 0;
        pdriver->disp_tbl.dev_connect = NULL;
        pdriver->disp_tbl.dev_stop = NULL;
        pdriver->disp_tbl.dev_disconnect = NULL;
        return FALSE;
    }

    //
    // let's load the class driver
    //
    umss_load_class_driver(NULL);

    // umss_schedule_workitem( NULL, umss_load_class_driver, NULL, 0 );
    usb_dbg_print(DBGLVL_MAXIMUM, ("umss_if_driver_init(): umss driver is initialized\n"));

    return TRUE;
}

// get the driver reg information for pnp notification to class
// driver.
// bug??? how if the driver info is returned while the driver
// is being unloaded.
// So the routine must be called when usb_query_and_lock_dev is
// called.
PCLASS_DRV_REG_INFO
umss_get_if_driver_info(PUSB_DEV_MANAGER dev_mgr, PUSB_DEV pdev, DEV_HANDLE if_handle)
{
    PUMSS_DRVR_EXTENSION drvr_ext;
    ULONG if_idx;
    USE_BASIC_NON_PENDING_IRQL;

    UNREFERENCED_PARAMETER(dev_mgr);

    if_idx = if_idx_from_handle(if_handle);
    if (if_idx >= 4)            // max interfaces per config supports. defined in td.h
        return NULL;

    ASSERT(pdev != NULL);

    lock_dev(pdev, FALSE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        unlock_dev(pdev, FALSE);
        usb_unlock_dev(pdev);
        return NULL;
    }

    drvr_ext = NULL;

    if (pdev->usb_config->interf[if_idx].pif_drv)
        drvr_ext = (PUMSS_DRVR_EXTENSION) pdev->usb_config->interf[if_idx].pif_drv->driver_ext;
    else
        TRAP();

    unlock_dev(pdev, FALSE);

    if (drvr_ext == NULL)
    {
        return NULL;
    }

    return &drvr_ext->class_driver_info;
}

VOID NTAPI
umss_worker(IN PVOID reference)
{
    PUMSS_WORKER_PACKET worker_packet;
    PUSB_DEV pdev;

    usb_dbg_print(DBGLVL_MAXIMUM, ("umss_worker(): entering...\n"));
    worker_packet = (PUMSS_WORKER_PACKET) reference;
    worker_packet->completion(worker_packet->context);
    if (worker_packet->dev_mgr && worker_packet->pdev)
    {
        pdev = (PUSB_DEV) worker_packet->pdev;
        usb_unlock_dev(pdev);
        pdev = NULL;
    }
    usb_free_mem(worker_packet);
    usb_dbg_print(DBGLVL_MAXIMUM, ("umss_worker(): exit\n"));
}

/*++
Routine Description:

    Wrapper for handling worker thread callbacks, it is importent to
    lock the dev from being deleted by calling usb_query_and_lock_dev
    and in umss_worker, call the usb_unlock_dev to release the ref
    count. One exception is that the umss_if_disconnect call this
    function to delete the device object that is still held by some
    others, and deferred deletion is required.

Arguments:

    Routine - Routine to be called when this work-item is processed
    Context - Value to be passed to worker routine

Return Value:
    TRUE if work item queued
    FALSE if work item not queued

--*/
BOOLEAN
umss_schedule_workitem(PVOID context,
                       UMSS_WORKER_ROUTINE completion, PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle)
{
    BOOLEAN ret_val = TRUE;
    PWORK_QUEUE_ITEM workitem;
    PUMSS_WORKER_PACKET worker_packet;

    worker_packet = usb_alloc_mem(NonPagedPool, sizeof(WORK_QUEUE_ITEM) + sizeof(UMSS_WORKER_PACKET));
    RtlZeroMemory(worker_packet, sizeof(WORK_QUEUE_ITEM) + sizeof(UMSS_WORKER_PACKET));

    if (worker_packet)
    {
        workitem = (PWORK_QUEUE_ITEM) & worker_packet[1];
        worker_packet->completion = completion;
        worker_packet->context = context;

        if (dev_mgr != NULL && dev_handle != 0)
        {
            PUSB_DEV pdev;
            // lock the device until the workitem is executed.
            if (usb_query_and_lock_dev(dev_mgr, dev_handle, &pdev) == STATUS_SUCCESS)
            {
                worker_packet->dev_mgr = dev_mgr;
                worker_packet->pdev = pdev;
            }
            else
            {
                usb_free_mem(worker_packet);
                return FALSE;
            }
        }
        // Initialize the work-item
        ExInitializeWorkItem(workitem, umss_worker, worker_packet);

        // Schedule the work-item
        ExQueueWorkItem(workitem, DelayedWorkQueue);

        usb_dbg_print(DBGLVL_MINIMUM, ("umss_schedule_workitem(): work-item queued\n"));
    }
    else
    {
        usb_dbg_print(DBGLVL_MINIMUM, ("umss_schedule_workitem(): Failed to allocate work-item\n"));
        ret_val = FALSE;
    }

    return ret_val;
}

NTSTATUS
umss_process_srb(PDEVICE_OBJECT dev_obj, PIRP irp)
{
    NTSTATUS status;
    PUSB_DEV pdev;
    PIO_STACK_LOCATION cur_stack;
    PUMSS_DEVICE_EXTENSION pdev_ext;
    PSCSI_REQUEST_BLOCK srb;

    if (dev_obj == NULL || irp == NULL)
        return STATUS_INVALID_PARAMETER;

    pdev = NULL;
    cur_stack = IoGetCurrentIrpStackLocation(irp);
    srb = cur_stack->Parameters.Scsi.Srb;

    if (srb == NULL || srb->DataTransferLength > 65536)
    {
        status = STATUS_INVALID_PARAMETER;
        goto ERROR_OUT;
    }

    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;

    pdev_ext = (PUMSS_DEVICE_EXTENSION) dev_obj->DeviceExtension;
    if ((status = usb_query_and_lock_dev(pdev_ext->dev_mgr, pdev_ext->dev_handle, &pdev)) != STATUS_SUCCESS)
    {
        PSENSE_DATA sense_buf;
        srb->SrbStatus = SRB_STATUS_NO_DEVICE;

        //
        // let's build the srb status for class driver
        //
        srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
        RtlZeroMemory(srb->SenseInfoBuffer, srb->SenseInfoBufferLength);
        if (!(srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE))
        {
            sense_buf = (PSENSE_DATA) srb->SenseInfoBuffer;
            sense_buf->ErrorCode = 0x70;
            sense_buf->Valid = 1;
            sense_buf->SenseKey = SCSI_SENSE_NOT_READY;
            sense_buf->AdditionalSenseCode = SCSI_ADSENSE_NO_MEDIA_IN_DEVICE;
            sense_buf->AdditionalSenseLength = 10;
        }
        goto ERROR_OUT;
    }

    switch (srb->Function)
    {
        case SRB_FUNCTION_EXECUTE_SCSI:
        {
            IO_PACKET io_packet;
            RtlZeroMemory(&io_packet, sizeof(io_packet));

            io_packet.flags |= IOP_FLAG_SRB_TRANSFER;
            if (srb->SrbFlags & SRB_FLAGS_DATA_IN)
                io_packet.flags |= IOP_FLAG_DIR_IN;
            if (!(srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE))
                io_packet.flags |= IOP_FLAG_REQ_SENSE;

            io_packet.cdb_length = srb->CdbLength;
            RtlCopyMemory(io_packet.cdb, srb->Cdb, sizeof(io_packet.cdb));
            io_packet.lun = 0;

            if (srb->SrbFlags & SRB_FLAGS_NO_DATA_TRANSFER)
            {
                io_packet.data_buffer = NULL;
                io_packet.data_length = 0;
            }
            else
            {
                if ((irp->Flags & (IRP_READ_OPERATION | IRP_WRITE_OPERATION))
                    && !(irp->Flags & IRP_PAGING_IO))
                {
                    //
                    // since these operations does not allign the buffer on page boundary
                    // and some unknown traps in window NT, we have to copy to a buffer
                    // we allocated
                    io_packet.data_buffer = usb_alloc_mem(NonPagedPool, srb->DataTransferLength);
                    if (irp->Flags & IRP_WRITE_OPERATION)
                    {
                        PULONG dest_buf, src_buf;
                        ULONG i;

                        dest_buf = (PULONG) io_packet.data_buffer;
                        src_buf = (PULONG) srb->DataBuffer;

                        if (src_buf && dest_buf)
                        {
                            for(i = 0; i < (srb->DataTransferLength >> 2); i++)
                            {
                                dest_buf[i] = src_buf[i];
                            }
                        }
                    }
                }
                else
                    io_packet.data_buffer = srb->DataBuffer;

                io_packet.data_length = srb->DataTransferLength;
            }

            if (io_packet.flags & IOP_FLAG_REQ_SENSE)
            {
                io_packet.sense_data = srb->SenseInfoBuffer;
                io_packet.sense_data_length = srb->SenseInfoBufferLength;
            }

            io_packet.pirp = irp;

            // do some conversions
            if (pdev_ext->pif_desc->bInterfaceSubClass == UMSS_SUBCLASS_SFF8070I)
            {
                if (umss_tsc_to_sff(&io_packet) == FALSE)
                {
                    status = STATUS_DEVICE_PROTOCOL_ERROR;

                    usb_dbg_print(DBGLVL_MAXIMUM,
                                  ("umss_process_srb(): error converting to sff proto, 0x%x\n", status));
                    srb->SrbStatus = SRB_STATUS_ERROR;
                    break;
                }
            }

            if (pdev_ext->pif_desc->bInterfaceProtocol == PROTOCOL_BULKONLY)
            {
                //
                // currently we support only transparent scsi command set
                //
                if (pdev_ext->pif_desc->bInterfaceSubClass == UMSS_SUBCLASS_SCSI_TCS ||
                    pdev_ext->pif_desc->bInterfaceSubClass == UMSS_SUBCLASS_SFF8070I)
                    status = umss_bulkonly_startio(pdev_ext, &io_packet);
                else
                    status = STATUS_DEVICE_PROTOCOL_ERROR;
            }
            else if (pdev_ext->pif_desc->bInterfaceProtocol == PROTOCOL_CB
                     || pdev_ext->pif_desc->bInterfaceProtocol == PROTOCOL_CBI)
            {
                status = umss_cbi_startio(pdev_ext, &io_packet);
            }
            else
            {
                status = STATUS_DEVICE_PROTOCOL_ERROR;
            }

            if (status != STATUS_PENDING && status != STATUS_SUCCESS)
            {
                // error occured
                usb_dbg_print(DBGLVL_MAXIMUM, ("umss_process_srb(): error sending request, 0x%x\n", status));
                srb->SrbStatus = SRB_STATUS_ERROR;
            }
            break;
        }
        case SRB_FUNCTION_CLAIM_DEVICE:
        {
            srb->DataBuffer = (PVOID) dev_obj;
        }
        case SRB_FUNCTION_SHUTDOWN:
        case SRB_FUNCTION_FLUSH:
        case SRB_FUNCTION_RESET_BUS:
        case SRB_FUNCTION_FLUSH_QUEUE:
        case SRB_FUNCTION_RELEASE_QUEUE:
        case SRB_FUNCTION_RELEASE_DEVICE:
        default:
        {
            // for usb flash disk, they are luxurious

            usb_dbg_print(DBGLVL_MAXIMUM, ("umss_process_srb(): current srb->Function=0x%x\n",
                                           srb->Function));

            status = STATUS_SUCCESS;
            srb->SrbStatus = SRB_STATUS_SUCCESS;
            break;
        }
    }

    usb_unlock_dev(pdev);
    pdev = NULL;

ERROR_OUT:
    irp->IoStatus.Status = status;
    if (status != STATUS_PENDING)
    {
        IoStartNextPacket(dev_obj, FALSE);
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    //
    // UMSS_COMPLETE_START_IO( dev_obj, status, irp );
    //
    return status;
}

BOOLEAN
umss_tsc_to_sff(PIO_PACKET io_packet)
{
    if (io_packet == NULL)
        return FALSE;

    io_packet->cdb_length = 12;
    if (io_packet->cdb[0] == SCSIOP_MODE_SENSE)
    {
        io_packet->cdb[0] = 0x5a;       // mode sense( 10 )
        io_packet->cdb[8] = io_packet->cdb[4];
        io_packet->cdb[4] = 0;
        if (io_packet->cdb[8] < 8)
            io_packet->cdb[8] = 8;

        io_packet->data_length = 8;
        return TRUE;
    }
    if (io_packet->cdb[0] == SCSIOP_REASSIGN_BLOCKS ||
        io_packet->cdb[0] == SCSIOP_RESERVE_UNIT || io_packet->cdb[0] == SCSIOP_RELEASE_UNIT)
        return FALSE;

    return TRUE;
}

VOID
umss_fix_sff_result(PIO_PACKET io_packet, SCSI_REQUEST_BLOCK *srb)
{
    PBYTE buf;
    if (io_packet->cdb[0] != 0x5a)
        return;
    // the following is not right since it has to be 0x3f, return all pages
    // if( io_packet->cdb[ 2 ] != 0 )
    // return;
    srb->DataTransferLength = 4;
    buf = io_packet->data_buffer;
    // convert the mode param to scsi II
    buf[0] = buf[1];
    buf[1] = buf[2];
    buf[2] = buf[3];
    buf[3] = 0;
    return;
}
