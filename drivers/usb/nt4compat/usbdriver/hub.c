/**
 * hub.c - USB driver stack project for Windows NT 4.0
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

//----------------------------------------------------------
//event pool routines
#define crash_machine() \
{ ( ( PUSB_DEV ) 0 )->flags = 0x12345; }

#define hub_if_from_dev( pdEV, pIF ) \
{\
	int i;\
	for( i = 0; i < pdEV->usb_config->if_count; i ++ )\
	{\
		if( pdEV->usb_config->interf[ i ].pusb_if_desc->bInterfaceClass\
			== USB_CLASS_HUB )\
		{\
			break;\
		}\
	}\
\
	if( i < pdEV->usb_config->if_count )\
		pIF = &pdev->usb_config->interf[ i ];\
	else\
		pIF = NULL;\
\
}

extern ULONG cpu_clock_freq;

BOOLEAN hub_check_reset_port_status(PUSB_DEV pdev, LONG port_idx);

VOID hub_reexamine_port_status_queue(PUSB_DEV hub_dev, ULONG port_idx, BOOLEAN from_dpc);

void hub_int_completion(PURB purb, PVOID pcontext);

VOID hub_get_port_status_completion(PURB purb, PVOID context);

VOID hub_clear_port_feature_completion(PURB purb, PVOID context);

VOID hub_event_examine_status_que(PUSB_DEV pdev, ULONG event, ULONG context,    //hub_ext
                                  ULONG param   //port_idx
                                  );

VOID hub_timer_wait_dev_stable(PUSB_DEV pdev,
                               PVOID context     //port-index
                               );

VOID hub_event_dev_stable(PUSB_DEV pdev,
                          ULONG event,
                          ULONG context,    //hub_ext
                          ULONG param   //port_idx
                          );

VOID hub_post_esq_event(PUSB_DEV pdev, BYTE port_idx, PROCESS_EVENT pe);

void hub_set_cfg_completion(PURB purb, PVOID pcontext);

void hub_get_hub_desc_completion(PURB purb, PVOID pcontext);

NTSTATUS hub_start_int_request(PUSB_DEV pdev);

BOOLEAN hub_connect(PDEV_CONNECT_DATA init_param, DEV_HANDLE dev_handle);

BOOLEAN hub_disconnect(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle);

BOOLEAN hub_stop(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle);

NTSTATUS hub_disable_port_request(PUSB_DEV pdev, UCHAR port_idx);

VOID hub_start_reset_port_completion(PURB purb, PVOID context);

BOOLEAN
init_event_pool(PUSB_EVENT_POOL pool)
{
    int i;

    if (pool == NULL)
        return FALSE;

    if ((pool->event_array = usb_alloc_mem(NonPagedPool, sizeof(USB_EVENT) * MAX_EVENTS)) == NULL)
        return FALSE;

    InitializeListHead(&pool->free_que);
    KeInitializeSpinLock(&pool->pool_lock);
    pool->total_count = MAX_EVENTS;
    pool->free_count = 0;

    for(i = 0; i < MAX_EVENTS; i++)
    {
        free_event(pool, &pool->event_array[i]);
    }

    return TRUE;
}

BOOLEAN
free_event(PUSB_EVENT_POOL pool, PUSB_EVENT pevent)
{
    if (pool == NULL || pevent == NULL)
    {
        return FALSE;
    }

    RtlZeroMemory(pevent, sizeof(USB_EVENT));
    InsertTailList(&pool->free_que, (PLIST_ENTRY) pevent);
    pool->free_count++;
    usb_dbg_print(DBGLVL_MAXIMUM + 1,
                  ("free_event(): alloced=0x%x, addr=0x%x\n", MAX_EVENTS - pool->free_count, pevent));

    return TRUE;
}

//null if failed
PUSB_EVENT
alloc_event(PUSB_EVENT_POOL pool, LONG count)
{
    PUSB_EVENT NewEvent;
    if (pool == NULL || count != 1)
        return NULL;

    if (pool->free_count == 0)
        return NULL;

    NewEvent = (PUSB_EVENT) RemoveHeadList(&pool->free_que);
    pool->free_count--;

    usb_dbg_print(DBGLVL_MAXIMUM + 1,
                  ("alloc_event(): alloced=0x%x, addr=0x%x\n", MAX_EVENTS - pool->free_count, NewEvent));
    return NewEvent;
}

BOOLEAN
destroy_event_pool(PUSB_EVENT_POOL pool)
{
    if (pool == NULL)
        return FALSE;

    InitializeListHead(&pool->free_que);
    pool->free_count = pool->total_count = 0;
    usb_free_mem(pool->event_array);
    pool->event_array = NULL;

    return TRUE;
}

VOID
event_list_default_process_event(PUSB_DEV pdev, ULONG event, ULONG context, ULONG param)
{
    UNREFERENCED_PARAMETER(param);
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(event);
    UNREFERENCED_PARAMETER(pdev);
}

//----------------------------------------------------------
//timer_svc pool routines

BOOLEAN
init_timer_svc_pool(PTIMER_SVC_POOL pool)
{
    int i;

    if (pool == NULL)
        return FALSE;

    pool->timer_svc_array = usb_alloc_mem(NonPagedPool, sizeof(TIMER_SVC) * MAX_TIMER_SVCS);
    InitializeListHead(&pool->free_que);
    pool->free_count = 0;
    pool->total_count = MAX_TIMER_SVCS;
    KeInitializeSpinLock(&pool->pool_lock);

    for(i = 0; i < MAX_TIMER_SVCS; i++)
    {
        free_timer_svc(pool, &pool->timer_svc_array[i]);
    }

    return TRUE;
}

BOOLEAN
free_timer_svc(PTIMER_SVC_POOL pool, PTIMER_SVC ptimer)
{
    if (pool == NULL || ptimer == NULL)
        return FALSE;

    RtlZeroMemory(ptimer, sizeof(TIMER_SVC));
    InsertTailList(&pool->free_que, (PLIST_ENTRY) & ptimer->timer_svc_link);
    pool->free_count++;

    return TRUE;
}

//null if failed
PTIMER_SVC
alloc_timer_svc(PTIMER_SVC_POOL pool, LONG count)
{
    PTIMER_SVC NewTimer;

    if (pool == NULL || count != 1)
        return NULL;

    if (pool->free_count <= 0)
        return NULL;

    NewTimer = (PTIMER_SVC) RemoveHeadList(&pool->free_que);
    pool->free_count--;
    return NewTimer;

}

BOOLEAN
destroy_timer_svc_pool(PTIMER_SVC_POOL pool)
{
    if (pool == NULL)
        return FALSE;

    usb_free_mem(pool->timer_svc_array);
    pool->timer_svc_array = NULL;
    InitializeListHead(&pool->free_que);
    pool->free_count = 0;
    pool->total_count = 0;

    return TRUE;
}

VOID
event_list_default_process_queue(PLIST_HEAD event_list,
                                 PUSB_EVENT_POOL event_pool, PUSB_EVENT usb_event, PUSB_EVENT out_event)
{
    //remove the first event from the event list, and copy it to
    //out_event

    if (event_list == NULL || event_pool == NULL || usb_event == NULL || out_event == NULL)
        return;

    RemoveEntryList(&usb_event->event_link);
    RtlCopyMemory(out_event, usb_event, sizeof(USB_EVENT));
    free_event(event_pool, usb_event);
    return;
}

BOOLEAN
psq_enqueue(PPORT_STATUS_QUEUE psq, ULONG status)
{
    if (psq == NULL)
        return FALSE;

    if (psq_is_full(psq))
        return FALSE;

    psq->port_status[psq->status_count].wPortChange = HIWORD(status);
    psq->port_status[psq->status_count].wPortStatus = LOWORD(status);

    psq->status_count++;

    usb_dbg_print(DBGLVL_MAXIMUM, ("psq_enqueue(): last status=0x%x, status count=0x%x, port_flag=0x%x\n",
                                   status, psq->status_count, psq->port_flags));
    return TRUE;

}

VOID
psq_init(PPORT_STATUS_QUEUE psq)
{
    RtlZeroMemory(psq, sizeof(PORT_STATUS_QUEUE));
    psq->port_flags = STATE_IDLE | USB_PORT_FLAG_DISABLE;
}

//return 0xffffffff if no element
ULONG
psq_outqueue(PPORT_STATUS_QUEUE psq)
{
    ULONG status;

    if (psq == NULL)
        return 0;

    if (psq_is_empty(psq))
        return 0;

    status = ((PULONG) & psq->port_status)[0];
    psq->port_status[0] = psq->port_status[1];
    psq->port_status[1] = psq->port_status[2];
    psq->port_status[2] = psq->port_status[3];
    psq->status_count--;

    return status;
}

BOOLEAN
psq_push(PPORT_STATUS_QUEUE psq, ULONG status)
{
    if (psq == NULL)
        return FALSE;

    status = ((PULONG) & psq->port_status)[0];
    psq->port_status[3] = psq->port_status[2];
    psq->port_status[2] = psq->port_status[1];
    psq->port_status[1] = psq->port_status[0];

    ((PULONG) & psq->port_status)[0] = status;

    psq->status_count++;
    psq->status_count = ((4 > psq->status_count) ? psq->status_count : 4);

    return TRUE;
}

BOOLEAN
hub_driver_init(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver)
{
    UNREFERENCED_PARAMETER(dev_mgr);

    //init driver structure, no PNP table functions
    pdriver->driver_desc.flags = USB_DRIVER_FLAG_DEV_CAPABLE;
    pdriver->driver_desc.vendor_id = 0xffff;       // USB Vendor ID
    pdriver->driver_desc.product_id = 0xffff;      // USB Product ID.
    pdriver->driver_desc.release_num = 0xffff;     // Release Number of Device

    pdriver->driver_desc.config_val = 0;           // Configuration Value
    pdriver->driver_desc.if_num = 0;               // Interface Number
    pdriver->driver_desc.if_class = USB_CLASS_HUB; // Interface Class
    pdriver->driver_desc.if_sub_class = 0;         // Interface SubClass
    pdriver->driver_desc.if_protocol = 0;          // Interface Protocol

    pdriver->driver_desc.driver_name = "USB hub";   // Driver name for Name Registry
    pdriver->driver_desc.dev_class = USB_CLASS_HUB;
    pdriver->driver_desc.dev_sub_class = 0;         // Device Subclass
    pdriver->driver_desc.dev_protocol = 0;          // Protocol Info.

    //pdriver->driver_init = hub_driver_init;       // initialized in dev_mgr_init_driver
    //pdriver->driver_destroy = hub_driver_destroy;

    pdriver->driver_ext = 0;
    pdriver->driver_ext_size = 0;

    pdriver->disp_tbl.version = 1;
    pdriver->disp_tbl.dev_connect = hub_connect;
    pdriver->disp_tbl.dev_disconnect = hub_disconnect;
    pdriver->disp_tbl.dev_stop = hub_stop;
    pdriver->disp_tbl.dev_reserved = NULL;

    return TRUE;
}

BOOLEAN
hub_driver_destroy(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdriver)
{
    UNREFERENCED_PARAMETER(dev_mgr);

    pdriver->driver_ext = NULL;
    return TRUE;
}

void
hub_reset_pipe_completion(PURB purb,    //only for reference, can not be released
                          PVOID context)
{
    PUSB_DEV pdev;
    PUSB_ENDPOINT pendp;

    USE_BASIC_NON_PENDING_IRQL;

    UNREFERENCED_PARAMETER(context);

    if (purb == NULL)
    {
        return;
    }

    pdev = purb->pdev;
    pendp = purb->pendp;

    lock_dev(pdev, TRUE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        unlock_dev(pdev, TRUE);
        return;
    }

    if (usb_error(purb->status))
    {
        //simply retry it
        unlock_dev(pdev, TRUE);
        //usb_free_mem( purb );
        return;
    }
    unlock_dev(pdev, TRUE);

    pdev = purb->pdev;
    hub_start_int_request(pdev);
    return;
}

NTSTATUS
hub_start_int_request(PUSB_DEV pdev)
{
    PURB purb;
    PUSB_INTERFACE pif;
    PHUB2_EXTENSION hub_ext;
    NTSTATUS status;
    PHCD hcd;
    USE_BASIC_NON_PENDING_IRQL;

    if (pdev == NULL)
        return STATUS_INVALID_PARAMETER;

    lock_dev(pdev, FALSE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        unlock_dev(pdev, FALSE);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }
    purb = usb_alloc_mem(NonPagedPool, sizeof(URB));
    RtlZeroMemory(purb, sizeof(URB));

    if (purb == NULL)
    {
        unlock_dev(pdev, FALSE);
        return STATUS_NO_MEMORY;
    }

    purb->flags = 0;
    purb->status = STATUS_SUCCESS;
    hub_ext = hub_ext_from_dev(pdev);
    purb->data_buffer = hub_ext->int_data_buf;
    purb->data_length = (hub_ext->port_count + 7) / 8;

    hub_if_from_dev(pdev, pif);
    usb_dbg_print(DBGLVL_ULTRA, ("hub_start_int_request(): pdev=0x%x, pif=0x%x\n", pdev, pif));
    purb->pendp = &pif->endp[0];
    purb->pdev = pdev;

    purb->completion = hub_int_completion;
    purb->context = hub_ext;

    purb->pirp = NULL;
    purb->reference = 0;
    hcd = pdev->hcd;
    unlock_dev(pdev, FALSE);

    status = hcd->hcd_submit_urb(hcd, pdev, purb->pendp, purb);
    if (status != STATUS_PENDING)
    {
        usb_free_mem(purb);
        purb = NULL;
    }

    return status;
}

void
hub_int_completion(PURB purb, PVOID pcontext)
{

    PUSB_DEV pdev;
    PHUB2_EXTENSION hub_ext;
    ULONG port_idx;
    PUSB_CTRL_SETUP_PACKET psetup;
    NTSTATUS status;
    LONG i;
    PHCD hcd;

    USE_BASIC_NON_PENDING_IRQL;

    if (purb == NULL)
        return;

    if (pcontext == NULL)
    {
        usb_free_mem(purb);
        return;
    }

    usb_dbg_print(DBGLVL_ULTRA, ("hub_int_completion(): entering...\n"));

    pdev = purb->pdev;
    hub_ext = pcontext;

    lock_dev(pdev, TRUE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        unlock_dev(pdev, TRUE);
        usb_free_mem(purb);
        return;
    }

    hcd = pdev->hcd;

    if (purb->status == STATUS_SUCCESS)
    {

        for(i = 1; i <= hub_ext->port_count; i++)
        {
            if (hub_ext->int_data_buf[i >> 3] & (1 << i))
            {
                break;
            }
        }
        if (i > hub_ext->port_count)
        {
            //no status change, re-initialize the int request
            unlock_dev(pdev, TRUE);
            usb_free_mem(purb);
            hub_start_int_request(pdev);
            return;
        }

        port_idx = (ULONG)i;

        //re-use the urb to get port status
        purb->pendp = &pdev->default_endp;
        purb->data_buffer = (PUCHAR) & hub_ext->port_status;

        purb->data_length = sizeof(USB_PORT_STATUS);
        purb->pdev = pdev;

        purb->context = hub_ext;
        purb->pdev = pdev;
        purb->completion = hub_get_port_status_completion;
        purb->reference = port_idx;

        psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;

        psetup->bmRequestType = 0xa3;   //host-device class other recepient
        psetup->bRequest = USB_REQ_GET_STATUS;
        psetup->wValue = 0;
        psetup->wIndex = (USHORT) port_idx;
        psetup->wLength = 4;

        purb->pirp = NULL;
        unlock_dev(pdev, TRUE);

        status = hcd->hcd_submit_urb(hcd, pdev, purb->pendp, purb);
        if (usb_error(status))
        {
            usb_free_mem(purb);
            purb = NULL;
        }
        else if (status == STATUS_SUCCESS)
        {
            // this is for root hub
            hcd->hcd_generic_urb_completion(purb, purb->context);
        }
        return;
    }
    else
    {
        unlock_dev(pdev, TRUE);
        if (usb_halted(purb->status))
        {
            //let's reset pipe
            usb_reset_pipe(pdev, purb->pendp, hub_reset_pipe_completion, NULL);
        }
        //unexpected error
        usb_free_mem(purb);
        purb = NULL;
    }
    return;
}

VOID
hub_get_port_status_completion(PURB purb, PVOID context)
{
    PUSB_DEV pdev;
    PUSB_ENDPOINT pendp;
    BYTE port_idx;
    PHUB2_EXTENSION hub_ext;
    PUSB_CTRL_SETUP_PACKET psetup;
    NTSTATUS status;
    PHCD hcd;

    USE_BASIC_NON_PENDING_IRQL;

    if (purb == NULL || context == NULL)
        return;

    usb_dbg_print(DBGLVL_MAXIMUM, ("hub_get_port_feature_completion(): entering...\n"));

    pdev = purb->pdev;
    pendp = purb->pendp;

    lock_dev(pdev, TRUE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        unlock_dev(pdev, TRUE);
        usb_free_mem(purb);
        return;
    }

    hcd = pdev->hcd;
    if (usb_error(purb->status))
    {
        unlock_dev(pdev, TRUE);

        purb->status = 0;
        //simply retry the request refer to item 55 in document
        status = hcd->hcd_submit_urb(hcd, pdev, pendp, purb);
        if (status != STATUS_PENDING)
        {
            if (status == STATUS_SUCCESS)
            {
                hcd->hcd_generic_urb_completion(purb, purb->context);

            }
            else
            {
                //
                // must be fatal error
                // FIXME: better to pass it to the completion for further
                // processing?
                //
                usb_free_mem(purb);
            }
        }
        return;
    }

    hub_ext = hub_ext_from_dev(pdev);
    port_idx = (BYTE) purb->reference;

    usb_dbg_print(DBGLVL_MAXIMUM, ("hub_get_port_stataus_completion(): port_idx=0x%x, hcd =0x%x, \
                  pdev=0x%x, purb=0x%x, hub_ext=0x%x, portsc=0x%x \n", port_idx, pdev->hcd, pdev,
                  purb, hub_ext, *((PULONG) purb->data_buffer)));

    psq_enqueue(&hub_ext->port_status_queue[port_idx], *((PULONG) purb->data_buffer));

    //reuse the urb to clear the feature
    RtlZeroMemory(purb, sizeof(URB));

    purb->data_buffer = NULL;
    purb->data_length = 0;
    purb->pendp = &pdev->default_endp;
    purb->pdev = pdev;

    purb->context = (PVOID) & hub_ext->port_status;
    purb->pdev = pdev;
    purb->completion = hub_clear_port_feature_completion;
    purb->reference = port_idx;

    psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;

    psetup->bmRequestType = 0x23;       //host-device class port recepient
    psetup->bRequest = USB_REQ_CLEAR_FEATURE;
    psetup->wIndex = port_idx;
    psetup->wLength = 0;
    purb->pirp = NULL;

    if (hub_ext->port_status.wPortChange & USB_PORT_STAT_C_CONNECTION)
    {
        psetup->wValue = USB_PORT_FEAT_C_CONNECTION;
    }
    else if (hub_ext->port_status.wPortChange & USB_PORT_STAT_C_ENABLE)
    {
        psetup->wValue = USB_PORT_FEAT_C_ENABLE;
    }
    else if (hub_ext->port_status.wPortChange & USB_PORT_STAT_C_SUSPEND)
    {
        psetup->wValue = USB_PORT_FEAT_C_SUSPEND;
    }
    else if (hub_ext->port_status.wPortChange & USB_PORT_STAT_C_OVERCURRENT)
    {
        psetup->wValue = USB_PORT_FEAT_C_OVER_CURRENT;
    }
    else if (hub_ext->port_status.wPortChange & USB_PORT_STAT_C_RESET)
    {
        psetup->wValue = USB_PORT_FEAT_C_RESET;
    }
    unlock_dev(pdev, TRUE);

    status = hcd->hcd_submit_urb(hcd, pdev, pendp, purb);

    // if( status != STATUS_SUCCESS )
    if (status != STATUS_PENDING)
    {
        hcd->hcd_generic_urb_completion(purb, purb->context);
    }
    /*else if( usb_error( status ) )
       {
       usb_free_mem( purb );
       return;
       } */
    return;

}

VOID
hub_clear_port_feature_completion(PURB purb, PVOID context)
{
    BYTE port_idx;
    LONG i;
    BOOLEAN event_post, brh;
    ULONG pc;
    PHCD hcd;
    NTSTATUS status;
    PUSB_DEV pdev;
    PHUB2_EXTENSION hub_ext = NULL;
    PUSB_DEV_MANAGER dev_mgr;

    PUSB_CTRL_SETUP_PACKET psetup;

    USE_BASIC_NON_PENDING_IRQL;

    if (purb == NULL)
        return;

    if (context == NULL)
    {
        usb_free_mem(purb);
        return;
    }

    usb_dbg_print(DBGLVL_MAXIMUM, ("hub_clear_port_feature_completion(): entering...\n"));

    pdev = purb->pdev;
    port_idx = (BYTE) purb->reference;

    lock_dev(pdev, TRUE);
    dev_mgr = dev_mgr_from_dev(pdev);
    hcd = pdev->hcd;
    brh = (BOOLEAN) (dev_class(pdev) == USB_DEV_CLASS_ROOT_HUB);

    if (usb_error(purb->status))
    {
        unlock_dev(pdev, TRUE);

        purb->status = 0;

        // retry the request
        status = hcd->hcd_submit_urb(hcd, purb->pdev, purb->pendp, purb);
        if (status != STATUS_PENDING)
        {
            if (status == STATUS_SUCCESS)
            {
                hcd->hcd_generic_urb_completion(purb, purb->context);
            }
            else
            {
                //
                // FIXME: should we pass the error to the completion directly
                // instead of forstall it here?
                //
                // do not think the device is workable, no requests to it any more.
                // including the int polling
                //
                // usb_free_mem( purb );
                //
                goto LBL_SCAN_PORT_STAT;
            }
        }
        return;
    }

    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        unlock_dev(pdev, TRUE);
        usb_free_mem(purb);
        return;
    }

    pc = ((PUSB_PORT_STATUS) context)->wPortChange;

    if (pc)
    {
        // the bits are tested in ascending order
        if (pc & USB_PORT_STAT_C_CONNECTION)
        {
            pc &= ~USB_PORT_STAT_C_CONNECTION;
        }
        else if (pc & USB_PORT_STAT_C_ENABLE)
        {
            pc &= ~USB_PORT_STAT_C_ENABLE;
        }
        else if (pc & USB_PORT_STAT_C_SUSPEND)
        {
            pc &= ~USB_PORT_STAT_C_SUSPEND;
        }
        else if (pc & USB_PORT_STAT_C_OVERCURRENT)
        {
            pc &= ~USB_PORT_STAT_C_OVERCURRENT;
        }
        else if (pc & USB_PORT_STAT_C_RESET)
        {
            pc &= ~USB_PORT_STAT_C_RESET;
        }
    }
    ((PUSB_PORT_STATUS) context)->wPortChange = (USHORT) pc;

    hub_ext = hub_ext_from_dev(pdev);

    if (pc)
    {
        //some other status change on the port still active
        psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;

        if (hub_ext->port_status.wPortChange & USB_PORT_STAT_C_CONNECTION)
        {
            psetup->wValue = USB_PORT_FEAT_C_CONNECTION;
        }
        else if (hub_ext->port_status.wPortChange & USB_PORT_STAT_C_ENABLE)
        {
            psetup->wValue = USB_PORT_FEAT_C_ENABLE;
        }
        else if (hub_ext->port_status.wPortChange & USB_PORT_STAT_C_SUSPEND)
        {
            psetup->wValue = USB_PORT_FEAT_C_SUSPEND;
        }
        else if (hub_ext->port_status.wPortChange & USB_PORT_STAT_C_OVERCURRENT)
        {
            psetup->wValue = USB_PORT_FEAT_C_OVER_CURRENT;
        }
        else if (hub_ext->port_status.wPortChange & USB_PORT_STAT_C_RESET)
        {
            psetup->wValue = USB_PORT_FEAT_C_RESET;
        }
        unlock_dev(pdev, TRUE);

        status = hcd->hcd_submit_urb(hcd, pdev, purb->pendp, purb);
        if (status != STATUS_PENDING)
        {
            if (status == STATUS_SUCCESS)
            {
                usb_dbg_print(DBGLVL_MAXIMUM,
                              ("hub_clear_port_status_completion(): port_idx=0x%x, hcd=0x%x, \
                              pdev=0x%x, purb=0x%x, hub_ext=0x%x, wPortChange=0x%x \n",
                              port_idx, pdev->hcd, pdev, purb, hub_ext, pc));

                hcd->hcd_generic_urb_completion(purb, purb->context);
            }
            else
            {
                usb_dbg_print(DBGLVL_MAXIMUM, (" hub_clear_port_feature_completion(): \
                              error=0x%x\n", status));

                // usb_free_mem( purb );
                goto LBL_SCAN_PORT_STAT;
            }
        }
        return;
    }

    for(i = 1; i <= hub_ext->port_count; i++)
    {
        if (hub_ext->int_data_buf[i >> 3] & (1 << i))
        {
            break;
        }
    }

    //clear the port-change map, we have get port i's status.
    hub_ext->int_data_buf[i >> 3] &= ~(1 << i);

    //rescan to find some other port that has status change
    for(i = 1; i <= hub_ext->port_count; i++)
    {
        if (hub_ext->int_data_buf[i >> 3] & (1 << i))
        {
            break;
        }
    }

    if (i <= hub_ext->port_count)
    {
        //still has port-change pending, get the port status change
        port_idx = (UCHAR) i;

        //re-use the urb
        purb->data_buffer = (PUCHAR) & hub_ext->port_status;
        purb->data_length = sizeof(USB_PORT_STATUS);
        purb->pendp = &pdev->default_endp;
        purb->pdev = pdev;

        purb->context = hub_ext;
        purb->pdev = pdev;
        purb->completion = hub_get_port_status_completion;
        purb->reference = port_idx;

        psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;

        psetup->bmRequestType = 0xa3;   //host-device class other recepient
        psetup->bRequest = USB_REQ_GET_STATUS;
        psetup->wValue = 0;
        psetup->wIndex = port_idx;
        psetup->wLength = 4;

        purb->pirp = NULL;

        unlock_dev(pdev, TRUE);

        status = hcd->hcd_submit_urb(hcd, pdev, purb->pendp, purb);
        if (status != STATUS_PENDING)
        {
            if (status == STATUS_SUCCESS)
            {
                hcd->hcd_generic_urb_completion(purb, purb->context);
            }
            else
            {                   //must be fatal error
                // usb_free_mem( purb );
                goto LBL_SCAN_PORT_STAT;
            }
        }
        return;
    }

    unlock_dev(pdev, TRUE);

LBL_SCAN_PORT_STAT:

    //all status changes are cleared
    if (purb)
        usb_free_mem(purb);

    purb = NULL;

    KeAcquireSpinLockAtDpcLevel(&dev_mgr->event_list_lock);
    lock_dev(pdev, TRUE);

    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        //
        // if reset is in process, the dev_mgr_disconnect_dev will continue
        // the following resets
        //
        unlock_dev(pdev, TRUE);
        KeReleaseSpinLockFromDpcLevel(&dev_mgr->event_list_lock);
        return;
    }

    //at last we wake up thread if some port have status change to process
    port_idx = 0;
    for(i = 1, event_post = FALSE; i <= hub_ext->port_count; i++)
    {
        if (psq_is_empty(&hub_ext->port_status_queue[i]) == FALSE)
        {
            if (port_state(hub_ext->port_status_queue[i].port_flags) == STATE_IDLE ||
                port_state(hub_ext->port_status_queue[i].port_flags) == STATE_WAIT_ADDRESSED)
            {
                // have status in the queue pending
                // STATE_WAIT_ADDRESSED is added to avoid some bad mannered
                // hub to disturb the reset process
                hub_post_esq_event(pdev, (BYTE) i, hub_event_examine_status_que);
            }
            else if (port_state(hub_ext->port_status_queue[i].port_flags) == STATE_WAIT_RESET_COMPLETE)
            {
                //there is only one reset at one time
                port_idx = (BYTE) i;
            }
        }
    }

    unlock_dev(pdev, TRUE);
    KeReleaseSpinLockFromDpcLevel(&dev_mgr->event_list_lock);


    if (port_idx)
        hub_check_reset_port_status(pdev, port_idx);

    //reinitialize the int request, here to reduce some uncertainty of concurrency
    hub_start_int_request(pdev);

    return;
}

VOID
hub_event_examine_status_que(PUSB_DEV pdev,
                             ULONG event,
                             ULONG context, //hub_ext
                             ULONG param    //port_idx
                             )
{
    PHUB2_EXTENSION hub_ext;
    USB_PORT_STATUS ps;
    PUSB_DEV pchild_dev;
    PTIMER_SVC ptimer;
    PUSB_DEV_MANAGER dev_mgr;

    USE_NON_PENDING_IRQL;

    UNREFERENCED_PARAMETER(event);

    if (pdev == NULL || context == 0 || param == 0)
        return;

    while (TRUE)
    {
        lock_dev(pdev, FALSE);
        if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
        {
            unlock_dev(pdev, FALSE);
            break;
        }

        dev_mgr = dev_mgr_from_dev(pdev);
        hub_ext = hub_ext_from_dev(pdev);

        if (psq_is_empty(&hub_ext->port_status_queue[param]))
        {
            set_port_state(hub_ext->port_status_queue[param].port_flags, STATE_IDLE);
            unlock_dev(pdev, FALSE);
            break;
        }

        *((ULONG *) & ps) = psq_outqueue(&hub_ext->port_status_queue[param]);


        pchild_dev = hub_ext->child_dev[param];
        hub_ext->child_dev[param] = 0;

        usb_dbg_print(DBGLVL_MAXIMUM,
                      ("hub_event_examine_status_queue(): dev_addr=0x%x, port=0x%x, wPortChange=0x%x, wPortStatus=0x%x\n",
                       pdev->dev_addr, param, ps.wPortChange, ps.wPortStatus));

        unlock_dev(pdev, FALSE);

        if (pchild_dev != NULL)
            dev_mgr_disconnect_dev(pchild_dev);

        if (((ps.wPortChange & USB_PORT_STAT_C_ENABLE) &&
             ((pdev->flags & USB_DEV_CLASS_MASK) != USB_DEV_CLASS_ROOT_HUB))
            || (ps.wPortChange & USB_PORT_STAT_C_OVERCURRENT)
            || (ps.wPortChange & USB_PORT_STAT_C_RESET)
            || ((ps.wPortChange & USB_PORT_STAT_C_CONNECTION) &&
                !(ps.wPortStatus & USB_PORT_STAT_CONNECTION)))
        {
            usb_dbg_print(DBGLVL_MAXIMUM,
                          ("hub_event_examine_status_queue(): error occured, portc=0x%x, ports=0x%x\n",
                           ps.wPortChange, ps.wPortStatus));

            lock_dev(pdev, FALSE);
            if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
            {
                unlock_dev(pdev, FALSE);
                break;
            }
            if (psq_is_empty(&hub_ext->port_status_queue[param]))
            {
                set_port_state(hub_ext->port_status_queue[param].port_flags, STATE_IDLE);
            }
            else
            {
                set_port_state(hub_ext->port_status_queue[param].port_flags, STATE_EXAMINE_STATUS_QUE);
            }
            unlock_dev(pdev, FALSE);
            continue;

        }
        else if ((ps.wPortChange & USB_PORT_STAT_C_CONNECTION)
                 && (ps.wPortStatus & USB_PORT_STAT_CONNECTION)
                 && psq_is_empty(&hub_ext->port_status_queue[param]))
        {
            KeAcquireSpinLock(&dev_mgr->timer_svc_list_lock, &old_irql);
            lock_dev(pdev, TRUE);
            if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
            {
                unlock_dev(pdev, TRUE);
                KeReleaseSpinLock(&dev_mgr->timer_svc_list_lock, old_irql);
                usb_dbg_print(DBGLVL_MAXIMUM, ("hub_event_examine_status_queue(): dev lost\n"));
                break;
            }
            ptimer = alloc_timer_svc(&dev_mgr->timer_svc_pool, 1);
            if (ptimer == NULL)
            {
                unlock_dev(pdev, TRUE);
                KeReleaseSpinLock(&dev_mgr->timer_svc_list_lock, old_irql);
                usb_dbg_print(DBGLVL_MAXIMUM,
                              ("hub_event_examine_status_queue(): timer can not allocated\n"));
                break;
            }

            //a new connection
            usb_dbg_print(DBGLVL_MAXIMUM, ("hub_event_examine_status_queue(): new connection comes\n"));

            ptimer->counter = 0;
            ptimer->threshold = 21;     //100 ms

            if (ps.wPortStatus & USB_PORT_STAT_LOW_SPEED)
                ptimer->threshold = 51; //500 ms

            ptimer->context = param;
            ptimer->pdev = pdev;
            ptimer->func = hub_timer_wait_dev_stable;
            InsertTailList(&dev_mgr->timer_svc_list, &ptimer->timer_svc_link);
            pdev->ref_count++;
            set_port_state(hub_ext->port_status_queue[param].port_flags, STATE_WAIT_STABLE);
            unlock_dev(pdev, TRUE);
            KeReleaseSpinLock(&dev_mgr->timer_svc_list_lock, old_irql);
            break;

        }
        else
        {
            usb_dbg_print(DBGLVL_MAXIMUM, ("hub_event_examine_status_queue(): unknown error\n"));
            continue;
        }
    }
    return;
}

VOID
hub_timer_wait_dev_stable(PUSB_DEV pdev,
                          PVOID context  //port-index
                          )
{

    PHUB2_EXTENSION hub_ext;
    ULONG param;
    PUSB_DEV_MANAGER dev_mgr;

    USE_BASIC_NON_PENDING_IRQL;

    if (pdev == NULL || context == 0)
        return;

    dev_mgr = dev_mgr_from_dev(pdev);
    param = (ULONG) context;
    KeAcquireSpinLockAtDpcLevel(&dev_mgr->event_list_lock);
    lock_dev(pdev, TRUE);

    pdev->ref_count--;

    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        goto LBL_OUT;
    }

    hub_ext = hub_ext_from_dev(pdev);

    if (!psq_is_empty(&hub_ext->port_status_queue[param]))
    {
        //error occured, normally we should not receive event here
        set_port_state(hub_ext->port_status_queue[param].port_flags, STATE_EXAMINE_STATUS_QUE);

        hub_post_esq_event(pdev, (BYTE) param, hub_event_examine_status_que);
    }
    else
    {
        set_port_state(hub_ext->port_status_queue[param].port_flags, STATE_WAIT_RESET);

        hub_post_esq_event(pdev, (BYTE) param, hub_event_dev_stable);

    }

LBL_OUT:
    unlock_dev(pdev, TRUE);
    KeReleaseSpinLockFromDpcLevel(&dev_mgr->event_list_lock);
    return;
}

VOID
hub_event_dev_stable(PUSB_DEV pdev,
                     ULONG event,
                     ULONG context, //hub_ext
                     ULONG param    //port_idx
                     )
{

    PHUB2_EXTENSION hub_ext;
    PUSB_EVENT pevent, pevent1;
    PLIST_ENTRY pthis, pnext;
    BOOLEAN que_exist;
    PHCD hcd;
    PUSB_DEV_MANAGER dev_mgr;
    NTSTATUS status;
    PURB purb;
    PUSB_CTRL_SETUP_PACKET psetup;

    USE_NON_PENDING_IRQL;

    UNREFERENCED_PARAMETER(event);

    if (pdev == NULL || context == 0 || param == 0)
        return;

    dev_mgr = dev_mgr_from_dev(pdev);
    KeAcquireSpinLock(&dev_mgr->event_list_lock, &old_irql);
    lock_dev(pdev, TRUE);

    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
        goto LBL_OUT;

    hub_ext = hub_ext_from_dev(pdev);
    hcd = pdev->hcd;

    pevent = alloc_event(&dev_mgr->event_pool, 1);
    if (pevent == NULL)
        goto LBL_OUT;

    pevent->event = USB_EVENT_WAIT_RESET_PORT;
    pevent->pdev = pdev;
    pevent->context = (ULONG) hub_ext;
    pevent->param = param;
    pevent->flags = USB_EVENT_FLAG_QUE_RESET;
    pevent->process_event = NULL;       //hub_event_reset_port_complete;
    pevent->process_queue = NULL;       //hub_event_reset_process_queue;
    pevent->pnext = NULL;

    ListFirst(&dev_mgr->event_list, pthis);
    que_exist = FALSE;

    while (pthis)
    {
        //insert the event in to the wait-queue
        pevent1 = (PUSB_EVENT) pthis;
        if (pevent1->event == USB_EVENT_WAIT_RESET_PORT)
        {
            while (pevent1->pnext)
                pevent1 = pevent1->pnext;

            pevent1->pnext = pevent;
            que_exist = TRUE;
            break;
        }
        ListNext(&dev_mgr->event_list, pthis, pnext);
        pthis = pnext;
    }

    if (!que_exist)
    {
        //Let's start a reset port request
        InsertHeadList(&dev_mgr->event_list, &pevent->event_link);
        purb = usb_alloc_mem(NonPagedPool, sizeof(URB));
        RtlZeroMemory(purb, sizeof(URB));

        purb->data_buffer = NULL;
        purb->data_length = 0;
        purb->pendp = &pdev->default_endp;

        purb->context = hub_ext;
        purb->pdev = pdev;
        purb->completion = hub_start_reset_port_completion;     //hub_int_completion;
        purb->reference = param;

        psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;

        psetup->bmRequestType = 0x23;   //host-device other recepient
        psetup->bRequest = USB_REQ_SET_FEATURE;
        psetup->wValue = USB_PORT_FEAT_RESET;
        psetup->wIndex = (USHORT) param;
        psetup->wLength = 0;

        purb->pirp = NULL;
        //enter another state
        set_port_state(hub_ext->port_status_queue[param].port_flags, STATE_WAIT_RESET_COMPLETE);

        unlock_dev(pdev, TRUE);
        KeReleaseSpinLock(&dev_mgr->event_list_lock, old_irql);

        status = hcd->hcd_submit_urb(hcd, pdev, purb->pendp, purb);
        if (status != STATUS_PENDING)
        {
            //must be fatal error
            usb_free_mem(purb);
            hub_reexamine_port_status_queue(pdev, param, FALSE);
            if (hub_remove_reset_event(pdev, param, FALSE))
                hub_start_next_reset_port(dev_mgr, FALSE);
        }
        return;
    }

LBL_OUT:
    unlock_dev(pdev, TRUE);
    KeReleaseSpinLock(&dev_mgr->event_list_lock, old_irql);
    return;
}

VOID
hub_start_reset_port_completion(PURB purb, PVOID context)
{
    PUSB_DEV pdev;
    PUSB_ENDPOINT pendp;
    PUSB_DEV_MANAGER dev_mgr = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG port_idx;
    PHCD hcd;

    USE_BASIC_NON_PENDING_IRQL;;
    if (purb == NULL)
        return;

    if (context == NULL)
    {
        //fatal error no retry.
        usb_free_mem(purb);
        return;
    }

    pdev = purb->pdev;
    pendp = purb->pendp;
    port_idx = purb->reference;

    lock_dev(pdev, TRUE);

    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        unlock_dev(pdev, TRUE);
        usb_free_mem(purb);
        goto LBL_FREE_EVENT;
    }

    hcd = pdev->hcd;
    dev_mgr = dev_mgr_from_dev(pdev);
    unlock_dev(pdev, TRUE);

    status = purb->status;
    usb_free_mem(purb);

    if (!usb_error(status))
    {
        return;
    }

LBL_FREE_EVENT:
    //since we have no patient to retry the dev, we should remove the event of
    //wait_reset_port on the port from the event list. and if possible, start
    //another reset process. note other port on the dev still have chance to be
    //reset if necessary.
    hub_reexamine_port_status_queue(pdev, port_idx, TRUE);
    if (hub_remove_reset_event(pdev, port_idx, TRUE))
        hub_start_next_reset_port(dev_mgr, TRUE);
    return;
}


VOID
hub_set_address_completion(PURB purb, PVOID context)
{
    PUSB_DEV pdev, hub_dev;
    PUSB_ENDPOINT pendp;
    PUSB_DEV_MANAGER dev_mgr;
    NTSTATUS status;
    ULONG port_idx;
    PHCD hcd;
    USE_BASIC_NON_PENDING_IRQL;

    hcd_dbg_print(DBGLVL_MAXIMUM, ("hub_set_address_completion: purb=%p context=%p\n", purb, context));

    if (purb == NULL)
        return;

    if (context == NULL)
    {
        //fatal error no retry.
        usb_free_mem(purb);
        return;
    }

    pdev = purb->pdev;
    pendp = purb->pendp;
    port_idx = purb->reference;

    lock_dev(pdev, TRUE);

    hcd = pdev->hcd;
    dev_mgr = dev_mgr_from_dev(pdev);
    hub_dev = pdev->parent_dev;
    port_idx = pdev->port_idx;

    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        unlock_dev(pdev, TRUE);
        usb_free_mem(purb);
        //some error occured, let's start the next reset event
        goto LBL_RESET_NEXT;
    }

    pdev->flags &= ~USB_DEV_STATE_MASK;
    pdev->flags |= USB_DEV_STATE_ADDRESSED;

    unlock_dev(pdev, TRUE);
    status = purb->status;

    if (usb_error(status))
    {
        //retry the urb
        purb->status = 0;
        hcd_dbg_print(DBGLVL_MAXIMUM, ("hub_set_address_completion: can not set address\n"));
        status = hcd->hcd_submit_urb(hcd, pdev, pendp, purb);
        //some error occured, disable the port
        if (status != STATUS_PENDING)
        {
            usb_free_mem(purb);
            status = hub_disable_port_request(hub_dev, (UCHAR) port_idx);
        }
        return;
    }

    usb_free_mem(purb);
    //let address settle
    usb_wait_ms_dpc(10);

    //let's config the dev
    dev_mgr_start_config_dev(pdev);

LBL_RESET_NEXT:
    //second, remove the event in the queue
    hub_reexamine_port_status_queue(hub_dev, port_idx, TRUE);
    if (hub_remove_reset_event(hub_dev, port_idx, TRUE))
        hub_start_next_reset_port(dev_mgr, TRUE);
    return;
};

VOID
hub_disable_port_completion(PURB purb, PVOID pcontext)
{
    PUSB_DEV pdev;
    PUSB_DEV_MANAGER dev_mgr;
    UCHAR port_idx;
    PUSB_ENDPOINT pendp;
    PUSB_CTRL_SETUP_PACKET psetup;

    UNREFERENCED_PARAMETER(pcontext);

    if (purb == NULL)
        return;

    pdev = purb->pdev;
    pendp = purb->pendp;
    psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;
    port_idx = (UCHAR) psetup->wIndex;

    dev_mgr = dev_mgr_from_dev(pdev);

    usb_free_mem(purb);

    hub_reexamine_port_status_queue(pdev, port_idx, TRUE);
    if (hub_remove_reset_event(pdev, port_idx, TRUE))
        hub_start_next_reset_port(dev_mgr, TRUE);

    return;
}

//caller should guarantee the validity of the dev
NTSTATUS
hub_disable_port_request(PUSB_DEV pdev, UCHAR port_idx)
{
    PURB purb;
    PUSB_ENDPOINT pendp;
    PHUB2_EXTENSION hub_ext;
    PUSB_CTRL_SETUP_PACKET psetup;
    NTSTATUS status;
    PHCD hcd;
    USE_BASIC_NON_PENDING_IRQL;;

    if (pdev == NULL || port_idx == 0)
        return STATUS_INVALID_PARAMETER;

    lock_dev(pdev, FALSE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        unlock_dev(pdev, FALSE);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    purb = usb_alloc_mem(NonPagedPool, sizeof(URB));
    if (purb == NULL)
    {
        unlock_dev(pdev, FALSE);
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(purb, sizeof(URB));

    purb->flags = 0;
    purb->status = STATUS_SUCCESS;

    hub_ext = hub_ext_from_dev(pdev);

    purb->data_buffer = NULL;
    purb->data_length = 0;

    pendp = purb->pendp = &pdev->default_endp;
    purb->pdev = pdev;

    purb->completion = hub_disable_port_completion;
    purb->context = hub_ext;

    purb->pirp = NULL;
    purb->reference = 0;

    psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;

    psetup->bmRequestType = 0x23;       //host-device other recepient
    psetup->bRequest = USB_REQ_CLEAR_FEATURE;   //clear_feature
    psetup->wValue = USB_PORT_FEAT_ENABLE;
    psetup->wIndex = (USHORT) port_idx;
    psetup->wLength = 0;

    purb->pirp = NULL;
    //enter another state
    hcd = pdev->hcd;
    unlock_dev(pdev, FALSE);

    status = hcd->hcd_submit_urb(hcd, pdev, pendp, purb);
    if (status == STATUS_PENDING)
        return status;

    usb_free_mem(purb);
    return status;
}


BOOLEAN
hub_remove_reset_event(PUSB_DEV pdev, ULONG port_idx, BOOLEAN from_dpc)
{
    PUSB_DEV_MANAGER dev_mgr;
    PLIST_ENTRY pthis, pnext;
    PUSB_EVENT pevent, pnext_event;
    BOOLEAN found;

    KIRQL old_irql = 0;

    if (pdev == NULL)
        return FALSE;

    if (port_idx == 0)
        return FALSE;

    dev_mgr = dev_mgr_from_dev(pdev);
    found = FALSE;

    if (from_dpc)
        KeAcquireSpinLockAtDpcLevel(&dev_mgr->event_list_lock);
    else
        KeAcquireSpinLock(&dev_mgr->event_list_lock, &old_irql);

    ListFirst(&dev_mgr->event_list, pthis);
    while (pthis)
    {
        pevent = (PUSB_EVENT) pthis;
        if (pevent->event == USB_EVENT_WAIT_RESET_PORT &&
            (pevent->flags & USB_EVENT_FLAG_QUE_TYPE) == USB_EVENT_FLAG_QUE_RESET)
        {
            if (pevent->pdev == pdev && pevent->param == port_idx)
            {
                //remove it
                RemoveEntryList(&pevent->event_link);
                pnext_event = pevent->pnext;
                free_event(&dev_mgr->event_pool, pevent);

                if (pnext_event)
                    InsertHeadList(&dev_mgr->event_list, &pnext_event->event_link);

                found = TRUE;
                break;
            }
        }
        ListNext(&dev_mgr->event_list, pthis, pnext);
        pthis = pnext;
    }

    if (from_dpc)
        KeReleaseSpinLockFromDpcLevel(&dev_mgr->event_list_lock);
    else
        KeReleaseSpinLock(&dev_mgr->event_list_lock, old_irql);
    return found;
}

BOOLEAN
hub_start_next_reset_port(PUSB_DEV_MANAGER dev_mgr, BOOLEAN from_dpc)
{
    PLIST_ENTRY pthis, pnext;
    PUSB_EVENT pevent, pnext_event;
    PUSB_DEV pdev = NULL;
    PHUB2_EXTENSION hub_ext;
    BOOLEAN bret;
    PURB purb = NULL;
    BOOLEAN processed;
    PUSB_CTRL_SETUP_PACKET psetup;
    PHCD hcd = NULL;

    USE_NON_PENDING_IRQL;;

    if (dev_mgr == NULL)
        return FALSE;

    bret = FALSE;
    processed = FALSE;

    if (from_dpc)
        KeAcquireSpinLockAtDpcLevel(&dev_mgr->event_list_lock);
    else
        KeAcquireSpinLock(&dev_mgr->event_list_lock, &old_irql);

    ListFirst(&dev_mgr->event_list, pthis);

    while ((pevent = (PUSB_EVENT) pthis))
    {
        while (pevent->event == USB_EVENT_WAIT_RESET_PORT &&
               (pevent->flags & USB_EVENT_FLAG_QUE_TYPE) == USB_EVENT_FLAG_QUE_RESET)
        {

            processed = TRUE;

            pdev = pevent->pdev;
            lock_dev(pdev, TRUE);

            if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
            {
                unlock_dev(pdev, TRUE);
                pnext_event = pevent->pnext;
                free_event(&dev_mgr->event_pool, pevent);
                pevent = pnext_event;
                if (pevent == NULL)
                {
                    bret = FALSE;
                    break;
                }
                continue;
            }

            purb = usb_alloc_mem(NonPagedPool, sizeof(URB));
            RtlZeroMemory(purb, sizeof(URB));

            purb->data_buffer = NULL;
            purb->data_length = 0;
            purb->pendp = &pdev->default_endp;

            hub_ext = hub_ext_from_dev(pdev);
            purb->context = hub_ext;
            purb->pdev = pdev;
            purb->completion = hub_start_reset_port_completion;
            purb->reference = pevent->param;

            psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;

            psetup->bmRequestType = 0x23;       //host-device other recepient
            psetup->bRequest = 3;       //set_feature
            psetup->wValue = USB_PORT_FEAT_RESET;
            psetup->wIndex = (USHORT) pevent->param;
            psetup->wLength = 0;

            purb->pirp = NULL;
            hcd = pdev->hcd;
            set_port_state(hub_ext->port_status_queue[pevent->param].port_flags, STATE_WAIT_RESET_COMPLETE);
            unlock_dev(pdev, TRUE);

            bret = TRUE;
            break;
        }

        if (!processed)
        {
            ListNext(&dev_mgr->event_list, pthis, pnext);
            pthis = pnext;
        }
        else
            break;
    }

    if (from_dpc)
        KeReleaseSpinLockFromDpcLevel(&dev_mgr->event_list_lock);
    else
        KeReleaseSpinLock(&dev_mgr->event_list_lock, old_irql);

    if (processed && bret)
    {
        if (hcd->hcd_submit_urb(hcd, pdev, purb->pendp, purb) != STATUS_PENDING)
        {
            //fatal error
            usb_free_mem(purb);
            bret = FALSE;
            //do not know what to do
        }
    }

    if (pthis == NULL)
        bret = TRUE;

    return bret;
}

//
//must have event-list-lock and dev-lock acquired
//
VOID
hub_post_esq_event(PUSB_DEV pdev, BYTE port_idx, PROCESS_EVENT pe)
{
    PUSB_DEV_MANAGER dev_mgr;
    PUSB_EVENT pevent;

    if (pdev == NULL || port_idx == 0 || pe == NULL)
        return;

    dev_mgr = dev_mgr_from_dev(pdev);

    pevent = alloc_event(&dev_mgr->event_pool, 1);
    pevent->event = USB_EVENT_DEFAULT;
    pevent->process_queue = event_list_default_process_queue;
    pevent->process_event = pe;
    pevent->context = (ULONG) hub_ext_from_dev(pdev);
    pevent->param = port_idx;
    pevent->flags = USB_EVENT_FLAG_ACTIVE;
    pevent->pdev = pdev;
    pevent->pnext = NULL;

    InsertTailList(&dev_mgr->event_list, &pevent->event_link);
    KeSetEvent(&dev_mgr->wake_up_event, 0, FALSE);
    // usb_dbg_print( DBGLVL_MAXIMUM, ( "hub_post_esq_event(): current element in event list is 0x%x\n",
    //                      dbg_count_list( &dev_mgr->event_list ) ) );
    return;
}

// called only in hub_clear_port_feature_completion
BOOLEAN
hub_check_reset_port_status(PUSB_DEV pdev, LONG port_idx)
{
    PUSB_DEV_MANAGER dev_mgr;
    PHUB2_EXTENSION hub_ext;
    BOOLEAN bReset;
    USB_PORT_STATUS port_status;
    PUSB_DEV pdev2;
    PURB purb2;
    PHCD hcd;

    PUSB_CTRL_SETUP_PACKET psetup;
    ULONG status;

    USE_BASIC_NON_PENDING_IRQL;;

    //let's check whether the status change is a reset complete
    usb_dbg_print(DBGLVL_MAXIMUM, ("hub_check_reset_port_status(): entering...\n"));
    dev_mgr = dev_mgr_from_dev(pdev);
    KeAcquireSpinLockAtDpcLevel(&dev_mgr->dev_list_lock);
    lock_dev(pdev, TRUE);

    dev_mgr = dev_mgr_from_dev(pdev);
    hcd = pdev->hcd;

    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        unlock_dev(pdev, TRUE);
        KeReleaseSpinLockFromDpcLevel(&dev_mgr->dev_list_lock);
        return FALSE;
    }

    hub_ext = hub_ext_from_dev(pdev);
    port_status = psq_peek(&hub_ext->port_status_queue[port_idx], 0);

    bReset = FALSE;
    if (port_status.wPortChange & USB_PORT_STAT_C_RESET)
        bReset = TRUE;

    pdev2 = NULL;
    purb2 = NULL;

    if (bReset
        && (port_state(hub_ext->port_status_queue[port_idx].port_flags) == STATE_WAIT_RESET_COMPLETE)
        && (psq_count(&hub_ext->port_status_queue[port_idx]) == 1))
    {
        // a port-reset complete, empty the queue, keep the state
        psq_outqueue(&hub_ext->port_status_queue[port_idx]);
        set_port_state(hub_ext->port_status_queue[port_idx].port_flags, STATE_WAIT_ADDRESSED);

        //let's new a dev, and start the set-addr request
        if (hub_ext->child_dev[port_idx] == 0)
        {
            pdev2 = hub_ext->child_dev[port_idx] = dev_mgr_alloc_device(dev_mgr, hcd);
            if (pdev2)
            {
                purb2 = usb_alloc_mem(NonPagedPool, sizeof(URB));
                if (!purb2)
                {
                    dev_mgr_free_device(dev_mgr, pdev2);
                    pdev2 = hub_ext->child_dev[port_idx] = NULL;
                }
                else
                {
                    if (port_status.wPortStatus & USB_PORT_STAT_LOW_SPEED)
                    {
                        pdev2->flags |= USB_DEV_FLAG_LOW_SPEED;
                    }
                    else if (port_status.wPortStatus & USB_PORT_STAT_HIGH_SPEED)
                        pdev2->flags |= USB_DEV_FLAG_HIGH_SPEED;

                    pdev2->parent_dev = pdev;
                    pdev2->port_idx = (UCHAR) port_idx;
                    pdev2->ref_count++;

                    RtlZeroMemory(purb2, sizeof(URB));

                    purb2->pdev = pdev2;
                    purb2->pendp = &pdev2->default_endp;
                    purb2->context = hub_ext;
                    purb2->completion = hub_set_address_completion;

                    InitializeListHead(&purb2->trasac_list);
                    purb2->reference = port_idx;
                    purb2->pirp = 0;

                    psetup = (PUSB_CTRL_SETUP_PACKET) purb2->setup_packet;
                    psetup->bmRequestType = 0;
                    psetup->bRequest = USB_REQ_SET_ADDRESS;
                    psetup->wValue = pdev2->dev_addr;
                }
            }
        }

        if (pdev2 && purb2)
        {
            //creation success, emit the urb
            //add to dev list
            InsertTailList(&dev_mgr->dev_list, &pdev2->dev_link);

            unlock_dev(pdev, TRUE);
            KeReleaseSpinLockFromDpcLevel(&dev_mgr->dev_list_lock);

            status = hcd->hcd_submit_urb(hcd, pdev2, purb2->pendp, purb2);

            lock_dev(pdev2, TRUE);
            pdev2->ref_count--;
            usb_dbg_print(DBGLVL_MAXIMUM,
                          ("hub_check_reset_port_status(): new dev ref_count=0x%x\n", pdev2->ref_count));
            unlock_dev(pdev2, TRUE);

            if (status != STATUS_PENDING)
            {
                usb_free_mem(purb2);
                //??? do we need to lock it for SMP?
                //dev_mgr_free_device( dev_mgr, pdev2 ), let dev_mgr_thread to clean it;
                // disable the port
                if (hub_disable_port_request(pdev, (UCHAR) port_idx) != STATUS_PENDING)
                    goto LBL_RESET_FAIL;
            }

            return TRUE;
        }
    }
    else
    {
        usb_dbg_print(DBGLVL_MAXIMUM, ("hub_check_reset_port_status(): not a correct reset status\n"));
    }
    unlock_dev(pdev, TRUE);
    KeReleaseSpinLockFromDpcLevel(&dev_mgr->dev_list_lock);

LBL_RESET_FAIL:
    //Any event other than reset cause the reset process stall and another
    //pending reset-port requeset is serviced
    hub_reexamine_port_status_queue(pdev, port_idx, TRUE);
    if (hub_remove_reset_event(pdev, port_idx, TRUE))
        hub_start_next_reset_port(dev_mgr, TRUE);

    return FALSE;
}

VOID
hub_reexamine_port_status_queue(PUSB_DEV hub_dev, ULONG port_idx, BOOLEAN from_dpc)
{

    PHUB2_EXTENSION hub_ext;
    PUSB_DEV_MANAGER dev_mgr;

    USE_NON_PENDING_IRQL;;

    if (hub_dev == NULL || port_idx == 0)
        return;

    dev_mgr = dev_mgr_from_dev(hub_dev);
    if (from_dpc)
        KeAcquireSpinLockAtDpcLevel(&dev_mgr->event_list_lock);
    else
        KeAcquireSpinLock(&dev_mgr->event_list_lock, &old_irql);

    lock_dev(hub_dev, TRUE);
    if (dev_state(hub_dev) != USB_DEV_STATE_ZOMB)
    {

        hub_ext = hub_ext_from_dev(hub_dev);
        if (psq_is_empty(&hub_ext->port_status_queue[port_idx]))
        {
            set_port_state(hub_ext->port_status_queue[port_idx].port_flags, STATE_IDLE);

        }
        else
        {
            set_port_state(hub_ext->port_status_queue[port_idx].port_flags, STATE_EXAMINE_STATUS_QUE);

            hub_post_esq_event(hub_dev, (UCHAR) port_idx, hub_event_examine_status_que);
        }
    }
    unlock_dev(hub_dev, TRUE);

    if (from_dpc)
        KeReleaseSpinLockFromDpcLevel(&dev_mgr->event_list_lock);
    else
        KeReleaseSpinLock(&dev_mgr->event_list_lock, old_irql);
    return;
}

BOOLEAN
hub_connect(PDEV_CONNECT_DATA param, DEV_HANDLE dev_handle)
{
    URB urb, *purb;
    CHAR buf[512];
    DEV_HANDLE endp_handle;
    USB_DEVICE_DESC dev_desc;
    PUSB_CONFIGURATION_DESC pcfg_desc;
    PUSB_INTERFACE_DESC pif_desc;
    PUSB_CTRL_SETUP_PACKET psetup;
    NTSTATUS status;
    LONG i, j, found, cfg_val = 0;
    PUSB_DEV_MANAGER dev_mgr;
    PUSB_DEV pdev;


    if (param == NULL || dev_handle == 0)
        return FALSE;

    dev_mgr = param->dev_mgr;

    pcfg_desc = (PUSB_CONFIGURATION_DESC) buf;
    endp_handle = dev_handle | 0xffff;
    UsbBuildGetDescriptorRequest(&urb,
                                 endp_handle,
                                 USB_DT_DEVICE, 0, 0, (&dev_desc), (sizeof(USB_DEVICE_DESC)), NULL, 0, 0);

    status = usb_submit_urb(dev_mgr, &urb);
    if (status != STATUS_SUCCESS)
        return FALSE;

    found = FALSE;
    for(i = 0; i < dev_desc.bNumConfigurations; i++)
    {
        UsbBuildGetDescriptorRequest(&urb, endp_handle, USB_DT_CONFIG, (USHORT) i, 0, buf, 512, NULL, 0, 0);

        status = usb_submit_urb(dev_mgr, &urb);
        if (status != STATUS_SUCCESS)
        {
            return FALSE;
        }

        status = usb_query_and_lock_dev(dev_mgr, dev_handle, &pdev);
        if (status != STATUS_SUCCESS)
            return FALSE;

        pif_desc = (PUSB_INTERFACE_DESC) & buf[sizeof(USB_CONFIGURATION_DESC)];
        for(j = 0; j < pcfg_desc->bNumInterfaces; j++)
        {
            if (pif_desc->bInterfaceClass == USB_CLASS_HUB
                && pif_desc->bInterfaceSubClass == 0 && pif_desc->bNumEndpoints == 1)
            {
                if ((pif_desc->bInterfaceProtocol > 0 && pif_desc->bInterfaceProtocol < 3)
                    || (pif_desc->bInterfaceProtocol == 0 && pdev->flags & USB_DEV_FLAG_HIGH_SPEED)
                    || (pif_desc->bInterfaceProtocol == 0 && !usb2(pdev->hcd)))
                {
                    found = TRUE;
                    cfg_val = pcfg_desc->bConfigurationValue;
                    break;
                }
            }
            if (usb_skip_if_and_altif((PBYTE *) & pif_desc) == FALSE)
            {
                break;
            }
        }
        usb_unlock_dev(pdev);

        if (found)
            break;

        if (usb_skip_one_config((PBYTE *) & pcfg_desc) == FALSE)
        {
            break;
        }

    }
    if (found)
    {
        purb = usb_alloc_mem(NonPagedPool, sizeof(URB));
        if (purb == NULL)
            return FALSE;

        psetup = (PUSB_CTRL_SETUP_PACKET) (purb)->setup_packet;
        urb_init((purb));

        purb->endp_handle = endp_handle;
        purb->data_buffer = NULL;
        purb->data_length = 0;
        purb->completion = hub_set_cfg_completion;
        purb->context = dev_mgr;
        purb->reference = (LONG) param->pdriver;
        psetup->bmRequestType = 0;
        psetup->bRequest = USB_REQ_SET_CONFIGURATION;
        psetup->wValue = (USHORT) cfg_val;
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

    return FALSE;
}

VOID hub_set_interface_completion(PURB purb, PVOID pcontext);

VOID
hub_set_cfg_completion(PURB purb, PVOID pcontext)
{
    PUSB_DEV_MANAGER dev_mgr;
    PUSB_DRIVER pdriver;
    ULONG endp_handle, dev_handle;
    PUSB_CTRL_SETUP_PACKET psetup;
    UCHAR if_idx = 0;
    PUSB_DEV pdev;
    PUSB_INTERFACE pif;
    BOOLEAN high_speed, multiple_tt;
    NTSTATUS status;
    USE_BASIC_NON_PENDING_IRQL;;

    if (purb == NULL || pcontext == NULL)
        return;

    //pdev = NULL;
    dev_mgr = (PUSB_DEV_MANAGER) pcontext;
    endp_handle = purb->endp_handle;
    dev_handle = endp_handle & 0xffff0000;
    pdriver = (PUSB_DRIVER) purb->reference;
    high_speed = FALSE;
    multiple_tt = FALSE;

    if (purb->status != STATUS_SUCCESS)
    {
        goto LBL_ERROR;
    }

    status = usb_query_and_lock_dev(dev_mgr, purb->endp_handle, &pdev);
    if (status != STATUS_SUCCESS)
    {
        usb_unlock_dev(pdev);
        goto LBL_ERROR;
    }
    lock_dev(pdev, TRUE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        unlock_dev(pdev, TRUE);
        usb_unlock_dev(pdev);
        goto LBL_ERROR;
    }
    if (pdev->flags & USB_DEV_FLAG_HIGH_SPEED)
    {
        high_speed = TRUE;
        hub_if_from_dev(pdev, pif);
        if (pif->altif_count)
        {
            multiple_tt = TRUE;
            if_idx = pif - &pdev->usb_config->interf[0];
        }
    }
    unlock_dev(pdev, TRUE);
    usb_unlock_dev(pdev);

    if (!high_speed || !multiple_tt)
    {
        hub_set_interface_completion(purb, pcontext);
        return;
    }

    psetup = (PUSB_CTRL_SETUP_PACKET) (purb)->setup_packet;
    urb_init((purb));

    // set the mult-tt if exist
    purb->endp_handle = endp_handle;
    purb->data_buffer = NULL;
    purb->data_length = 0;
    purb->completion = hub_set_interface_completion;
    purb->context = dev_mgr;
    purb->reference = (LONG) pdriver;
    psetup->bmRequestType = 0;
    psetup->bRequest = USB_REQ_SET_INTERFACE;
    psetup->wValue = (USHORT) 1;        // alternate tt
    psetup->wIndex = if_idx;    // if index
    psetup->wLength = 0;

    status = usb_submit_urb(dev_mgr, purb);
    if (status == STATUS_PENDING)
        return;

  LBL_ERROR:
    usb_free_mem(purb);
    purb = NULL;
    return;
}

void
hub_set_interface_completion(PURB purb, PVOID pcontext)
{
    NTSTATUS status;
    PUSB_CTRL_SETUP_PACKET psetup;
    PUSB_DEV_MANAGER dev_mgr;
    PBYTE dev_ext;
    DEV_HANDLE endp_handle;
    PUSB_DRIVER pdriver;

    if (purb == NULL || pcontext == NULL)
        return;

    //pdev = NULL;
    dev_mgr = (PUSB_DEV_MANAGER) pcontext;
    endp_handle = purb->endp_handle;
    pdriver = (PUSB_DRIVER) purb->reference;

    if (purb->status != STATUS_SUCCESS)
    {
        usb_free_mem(purb);
        return;
    }

    dev_ext = usb_alloc_mem(NonPagedPool, sizeof(HUB2_EXTENSION));
    if (dev_ext == NULL)
    {
        goto LBL_OUT;
    }

    //
    //acquire hub descriptor
    //
    RtlZeroMemory(dev_ext, sizeof(HUB2_EXTENSION));
    urb_init(purb);

    purb->data_buffer = (PUCHAR) & ((HUB2_EXTENSION *) dev_ext)->hub_desc;
    purb->endp_handle = endp_handle;
    purb->data_length = sizeof(USB_HUB_DESCRIPTOR);
    purb->completion = hub_get_hub_desc_completion;
    purb->context = (PVOID) dev_mgr;
    purb->reference = (ULONG) dev_ext;
    purb->pirp = (PIRP) pdriver;

    psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;
    psetup->bmRequestType = 0xa0;
    psetup->bRequest = USB_REQ_GET_DESCRIPTOR;
    psetup->wValue = (0x29 << 8);
    psetup->wLength = sizeof(USB_HUB_DESCRIPTOR);
    status = usb_submit_urb(dev_mgr, purb);

    if (status != STATUS_PENDING)
    {
        usb_free_mem(dev_ext);
        goto LBL_OUT;
    }
    return;

LBL_OUT:
    //clear the dev_driver fields in the dev.
    usb_free_mem(purb);
    return;
}


VOID
hub_power_on_port_completion(PURB purb, PVOID pcontext)
{
    PUSB_DEV_MANAGER dev_mgr;

    if (purb == NULL)
        return;
    if (pcontext == NULL)
        goto LBL_OUT;

    dev_mgr = (PUSB_DEV_MANAGER) pcontext;

    if (purb->status != STATUS_SUCCESS)
    {
        usb_dbg_print(DBGLVL_MAXIMUM,
                      ("hub_power_on_port_completion(): port%d power on failed\n", purb->reference));
    }
    else
    {
        usb_dbg_print(DBGLVL_MAXIMUM,
                      ("hub_power_on_port_completion(): port%d power on succeed\n", purb->reference));
    }

LBL_OUT:
    usb_free_mem(purb);
    return;
}

NTSTATUS
hub_power_on_port(PUSB_DEV pdev, UCHAR port_idx)
{
    NTSTATUS status;
    PUSB_CTRL_SETUP_PACKET psetup;
    PUSB_DEV_MANAGER dev_mgr;
    PURB purb;
    PHCD hcd;

    USE_BASIC_NON_PENDING_IRQL;;
    if (pdev == NULL || port_idx == 0)
        return STATUS_INVALID_PARAMETER;

    purb = usb_alloc_mem(NonPagedPool, sizeof(URB));
    if (purb == NULL)
        return STATUS_NO_MEMORY;

    urb_init(purb);

    lock_dev(pdev, FALSE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        unlock_dev(pdev, FALSE);
        status = STATUS_DEVICE_DOES_NOT_EXIST;
        goto LBL_OUT;
    }
    dev_mgr = dev_mgr_from_dev(pdev);
    hcd = pdev->hcd;

    purb->completion = hub_power_on_port_completion;
    purb->context = (PVOID) dev_mgr;
    purb->reference = (ULONG) port_idx;
    purb->pdev = pdev;
    purb->pendp = &pdev->default_endp;

    psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;
    psetup->bmRequestType = 0x23;
    psetup->bRequest = USB_REQ_SET_FEATURE;
    psetup->wValue = USB_PORT_FEAT_POWER;
    psetup->wIndex = (WORD) port_idx;
    psetup->wLength = 0;

    unlock_dev(pdev, FALSE);

    status = hcd->hcd_submit_urb(hcd, pdev, purb->pendp, purb);

    if (status != STATUS_PENDING)
    {
        goto LBL_OUT;
    }
    return STATUS_PENDING;

LBL_OUT:
    usb_free_mem(purb);
    return status;
}

void
hub_get_hub_desc_completion(PURB purb, PVOID pcontext)
{
    PUSB_DEV_MANAGER dev_mgr;
    PHUB2_EXTENSION hub_ext;
    PUSB_DEV pdev;
    LONG i;
    PUSB_INTERFACE pif = NULL;
    ULONG status;
    LONG port_count;
    PUSB_DRIVER pdriver;
    DEV_HANDLE dev_handle;

    USE_BASIC_NON_PENDING_IRQL;;

    if (purb == NULL)
    {
        return;
    }

    dev_mgr = (PUSB_DEV_MANAGER) pcontext;
    hub_ext = (PHUB2_EXTENSION) purb->reference;
    pdriver = (PUSB_DRIVER) purb->pirp;
    dev_handle = purb->endp_handle & 0xffff0000;

    if (pcontext == NULL || purb->reference == 0)
        goto LBL_OUT;

    if (purb->status != STATUS_SUCCESS)
    {
        goto LBL_OUT;
    }

    // obtain the pointer to the dev
    status = usb_query_and_lock_dev(dev_mgr, purb->endp_handle, &pdev);
    if (status != STATUS_SUCCESS)
    {
        usb_unlock_dev(pdev);
        goto LBL_OUT;
    }
    // safe to release the pdev ref since we are in urb completion
    usb_unlock_dev(pdev);

    lock_dev(pdev, TRUE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB ||
        dev_mgr_set_driver(dev_mgr, dev_handle, pdriver, pdev) == FALSE)
    {
        unlock_dev(pdev, TRUE);
        goto LBL_OUT;
    }

    //transit the state to configured
    pdev->flags &= ~USB_DEV_STATE_MASK;
    pdev->flags |= USB_DEV_STATE_CONFIGURED;

    pdev->dev_ext = (PBYTE) hub_ext;
    pdev->dev_ext_size = sizeof(HUB2_EXTENSION);

    port_count = hub_ext->port_count = hub_ext->hub_desc.bNbrPorts;
    hub_ext->pdev = pdev;
    for(i = 0; i < pdev->usb_config->if_count; i++)
    {
        pif = &pdev->usb_config->interf[i];
        if (pif->pusb_if_desc->bInterfaceClass == USB_CLASS_HUB
            && pif->pusb_if_desc->bInterfaceSubClass == 0
            && pif->pusb_if_desc->bInterfaceProtocol < 3 && pif->pusb_if_desc->bNumEndpoints == 1)
        {
            hub_ext->pif = pif;
            break;
        }
    }
    for(i = 0; i < MAX_HUB_PORTS + 1; i++)
    {
        psq_init((PPORT_STATUS_QUEUE) hub_ext->port_status_queue);
    }

    hub_ext->multiple_tt = (pif->pusb_if_desc->bInterfaceProtocol == 2);

    unlock_dev(pdev, TRUE);
    usb_free_mem(purb);

    hub_start_int_request(pdev);

    for(i = 0; i < port_count; i++)
    {
        hub_power_on_port(pdev, (UCHAR) (i + 1));
    }
    return;

LBL_OUT:
    if (purb)
        usb_free_mem(purb);

    if (hub_ext)
        usb_free_mem(hub_ext);
    return;
}

BOOLEAN
hub_stop(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle)
{
    UNREFERENCED_PARAMETER(dev_mgr);
    UNREFERENCED_PARAMETER(dev_handle);
    return TRUE;
}

BOOLEAN
hub_disconnect(PUSB_DEV_MANAGER dev_mgr, DEV_HANDLE dev_handle)
{
    PUSB_DEV pdev;
    //special use of usb_query and lock dev
    if (usb_query_and_lock_dev(dev_mgr, dev_handle, &pdev) != STATUS_SUCCESS)
    {
        //will never be success, since the dev is already in zomb state
        //at this point, the dev is valid, ref_count is of none use,
        //no need to lock it
        if (pdev)
        {
            usb_free_mem(pdev->dev_ext);
        }
    }

    return TRUE;
}

static BOOLEAN
hub_lock_unlock_tt(PUSB_DEV pdev, UCHAR port_idx, UCHAR type, BOOLEAN lock)
{
    PHUB2_EXTENSION dev_ext;
    PULONG pmap = NULL;

    USE_BASIC_NON_PENDING_IRQL;;

    if (pdev == NULL || port_idx > 127)
        return FALSE;

    lock_dev(pdev, FALSE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        unlock_dev(pdev, FALSE);
        return FALSE;
    }

    dev_ext = hub_ext_from_dev(pdev);
    if (dev_ext == NULL)
    {
        unlock_dev(pdev, FALSE);
        return FALSE;
    }
    if (type == USB_ENDPOINT_XFER_INT || type == USB_ENDPOINT_XFER_ISOC)
    {
        pmap = dev_ext->tt_status_map;
    }
    else if (type == USB_ENDPOINT_XFER_BULK || type == USB_ENDPOINT_XFER_CONTROL)
    {
        pmap = dev_ext->tt_bulk_map;
    }

    if (lock)
    {
        if (pmap[port_idx >> 5] & (1 << port_idx))
        {
            unlock_dev(pdev, FALSE);
            return FALSE;
        }
        pmap[port_idx >> 5] |= (1 << port_idx);
    }
    else
    {
        pmap[port_idx >> 5] &= ~(1 << port_idx);
    }

    unlock_dev(pdev, FALSE);
    return TRUE;
}

BOOLEAN
hub_lock_tt(PUSB_DEV pdev,
            UCHAR port_idx,
            UCHAR type   // transfer type
            )
{
    return hub_lock_unlock_tt(pdev, port_idx, type, TRUE);
}

BOOLEAN
hub_unlock_tt(PUSB_DEV pdev, UCHAR port_idx, UCHAR type)
{
    return hub_lock_unlock_tt(pdev, port_idx, type, FALSE);
}

VOID
hub_clear_tt_buffer_completion(PURB purb, PVOID context)
{
    PUSB_CTRL_SETUP_PACKET psetup;
    PURB_HS_PIPE_CONTENT pipe_content;
    PHUB2_EXTENSION hub_ext;
    PHCD hcd;

    if (purb == NULL || context == NULL)
        return;

    hub_ext = (PHUB2_EXTENSION) context;
    psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;
    pipe_content = (PURB_HS_PIPE_CONTENT) & purb->reference;
    hub_unlock_tt(purb->pdev, (UCHAR) psetup->wIndex, (UCHAR) pipe_content->trans_type);
    usb_free_mem(purb);
    purb = NULL;
    hcd = hub_ext->pdev->hcd;

    // let those blocked urbs ( sharing the same tt )have chance to be scheduled
    if (hcd && usb2(hcd))
        hcd->hcd_submit_urb(hcd, NULL, NULL, NULL);

    return;
}

// send CLEAR_TT_BUFFER to the hub
BOOLEAN
hub_clear_tt_buffer(PUSB_DEV pdev, URB_HS_PIPE_CONTENT pipe_content, UCHAR port_idx)
{
    PURB purb;
    PUSB_CTRL_SETUP_PACKET psetup;
    PHUB2_EXTENSION hub_ext;
    PHCD hcd;
    NTSTATUS status;
    USE_BASIC_NON_PENDING_IRQL;;

    if (pdev == NULL)
        return FALSE;

    if (pipe_content.speed_high)
        return FALSE;

    lock_dev(pdev, FALSE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        unlock_dev(pdev, FALSE);
        return FALSE;
    }

    hub_ext = hub_ext_from_dev(pdev);
    if (hub_ext == NULL)
    {
        unlock_dev(pdev, FALSE);
        return FALSE;
    }
    purb = usb_alloc_mem(NonPagedPool, sizeof(URB));
    RtlZeroMemory(purb, sizeof(URB));

    if (purb == NULL)
    {
        unlock_dev(pdev, FALSE);
        return FALSE;
    }

    purb->flags = 0;
    purb->status = STATUS_SUCCESS;
    purb->data_buffer = NULL;
    purb->data_length = 0;      // ( hub_ext->port_count + 7 ) / 8;

    // hub_if_from_dev( pdev, pif );
    purb->pendp = &pdev->default_endp;
    purb->pdev = pdev;

    purb->completion = hub_clear_tt_buffer_completion;
    purb->context = hub_ext;
    purb->reference = *((PLONG) & pipe_content);

    purb->pirp = NULL;
    hcd = pdev->hcd;

    psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;
    psetup->bmRequestType = 0x23;       //host-device class other recepient
    psetup->bRequest = HUB_REQ_CLEAR_TT_BUFFER;
    psetup->wValue = (USHORT) ((pipe_content.endp_addr) | (pipe_content.dev_addr << 4) |
                               (pipe_content.trans_type << 10) | (pipe_content.trans_dir << 15));

    if (hub_ext->multiple_tt)
    {
        psetup->wIndex = (USHORT) port_idx;
    }
    else
        psetup->wIndex = 1;

    psetup->wLength = 0;
    unlock_dev(pdev, FALSE);

    status = hcd->hcd_submit_urb(hcd, pdev, purb->pendp, purb);
    if (status != STATUS_PENDING)
    {
        usb_free_mem(purb);
        purb = NULL;
        return FALSE;
    }
    return TRUE;
}

VOID
hub_event_clear_tt_buffer(PUSB_DEV pdev,        //always null. we do not use this param
                          ULONG event,
                          ULONG context,
                          ULONG param)
{
    UNREFERENCED_PARAMETER(event);
    hub_clear_tt_buffer(pdev, *((PURB_HS_PIPE_CONTENT) & context), (UCHAR) param);
    return;
}

VOID
hub_post_clear_tt_event(PUSB_DEV pdev, BYTE port_idx, ULONG pipe)
{
    PUSB_DEV_MANAGER dev_mgr;
    PUSB_EVENT pevent;
    USE_NON_PENDING_IRQL;;

    dev_mgr = dev_mgr_from_dev(pdev);

    KeAcquireSpinLock(&dev_mgr->event_list_lock, &old_irql);
    lock_dev(pdev, TRUE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        unlock_dev(pdev, TRUE);
        KeReleaseSpinLock(&dev_mgr->event_list_lock, old_irql);
        return;
    }
    pevent = alloc_event(&dev_mgr->event_pool, 1);
    if (pevent == NULL)
    {
        unlock_dev(pdev, TRUE);
        KeReleaseSpinLock(&dev_mgr->event_list_lock, old_irql);
        TRAP();
        return;
    }

    pevent->event = USB_EVENT_WAIT_RESET_PORT;
    pevent->pdev = pdev;
    pevent->context = pipe;
    pevent->param = port_idx;
    pevent->flags = USB_EVENT_FLAG_ACTIVE;
    pevent->process_queue = event_list_default_process_queue;
    pevent->process_event = hub_event_clear_tt_buffer;
    pevent->pnext = NULL;
    InsertTailList(&dev_mgr->event_list, &pevent->event_link);

    unlock_dev(pdev, TRUE);
    KeReleaseSpinLock(&dev_mgr->event_list_lock, old_irql);

    KeSetEvent(&dev_mgr->wake_up_event, 0, FALSE);
    return;
}

BOOLEAN
init_irp_list(PIRP_LIST irp_list)
{
    LONG i;
    KeInitializeSpinLock(&irp_list->irp_list_lock);
    InitializeListHead(&irp_list->irp_busy_list);
    InitializeListHead(&irp_list->irp_free_list);
    irp_list->irp_list_element_array =
        usb_alloc_mem(NonPagedPool, sizeof(IRP_LIST_ELEMENT) * MAX_IRP_LIST_SIZE);

    if (irp_list->irp_list_element_array == NULL)
        return FALSE;

    RtlZeroMemory(irp_list->irp_list_element_array, sizeof(IRP_LIST_ELEMENT) * MAX_IRP_LIST_SIZE);
    for(i = 0; i < MAX_IRP_LIST_SIZE; i++)
    {
        InsertTailList(&irp_list->irp_free_list, &irp_list->irp_list_element_array[i].irp_link);
    }
    irp_list->irp_free_list_count = MAX_IRP_LIST_SIZE;
    return TRUE;
}

VOID
destroy_irp_list(PIRP_LIST irp_list)
{
    InitializeListHead(&irp_list->irp_busy_list);
    InitializeListHead(&irp_list->irp_free_list);
    usb_free_mem(irp_list->irp_list_element_array);
    irp_list->irp_list_element_array = NULL;
    irp_list->irp_free_list_count = 0;
    return;
}

BOOLEAN
add_irp_to_list(PIRP_LIST irp_list, PIRP pirp, PURB purb)
{
    KIRQL old_irql;
    PIRP_LIST_ELEMENT pile;

    if (irp_list == NULL || pirp == NULL || purb == NULL)
        return FALSE;

    IoAcquireCancelSpinLock(&old_irql);
    KeAcquireSpinLockAtDpcLevel(&irp_list->irp_list_lock);

    if (irp_list->irp_free_list_count == 0)
    {
        KeReleaseSpinLockFromDpcLevel(&irp_list->irp_list_lock);
        IoReleaseCancelSpinLock(old_irql);
        return FALSE;
    }
    pile = (PIRP_LIST_ELEMENT) RemoveHeadList(&irp_list->irp_free_list);

    pile->pirp = pirp;
    pile->purb = purb;

    irp_list->irp_free_list_count--;
    InsertTailList(&irp_list->irp_busy_list, &pile->irp_link);
    (void)IoSetCancelRoutine(pirp, dev_mgr_cancel_irp);

    KeReleaseSpinLockFromDpcLevel(&irp_list->irp_list_lock);
    IoReleaseCancelSpinLock(old_irql);
    return TRUE;
}

PURB
remove_irp_from_list(PIRP_LIST irp_list,
                     PIRP pirp,
                     PUSB_DEV_MANAGER dev_mgr    //if dev_mgr is not NULL, the urb needs to be canceled
                     )
{
    PIRP_LIST_ELEMENT pile;
    PLIST_ENTRY pthis, pnext;
    PURB purb;
    DEV_HANDLE endp_handle;
    PUSB_DEV pdev;
    PUSB_ENDPOINT pendp;
    PHCD hcd;

    USE_NON_PENDING_IRQL;;

    if (irp_list == NULL || pirp == NULL)
        return NULL;

    KeAcquireSpinLock(&irp_list->irp_list_lock, &old_irql);

    if (irp_list->irp_free_list_count == MAX_IRP_LIST_SIZE)
    {
        KeReleaseSpinLock(&irp_list->irp_list_lock, old_irql);
        return NULL;
    }

    purb = NULL;
    ListFirst(&irp_list->irp_busy_list, pthis);
    while (pthis)
    {
        pile = struct_ptr(pthis, IRP_LIST_ELEMENT, irp_link);
        if (pile->pirp == pirp)
        {
            purb = pile->purb;
            pile->pirp = NULL;
            pile->purb = NULL;
            RemoveEntryList(pthis);
            InsertTailList(&irp_list->irp_free_list, pthis);
            irp_list->irp_free_list_count++;
            break;
        }
        ListNext(&irp_list->irp_busy_list, pthis, pnext);
        pthis = pnext;
    }

    if (purb == NULL)
    {
        // not found
        KeReleaseSpinLock(&irp_list->irp_list_lock, old_irql);
        return NULL;
    }

    endp_handle = purb->endp_handle;
    KeReleaseSpinLock(&irp_list->irp_list_lock, old_irql);

    if (dev_mgr)
    {
        // indicate we needs to cancel the urb, this condition happens only in cancel routine
        // we should notice that even the hcd_cancel_urb is called, the irp may not be canceled
        // if the urb does not exist in any queue of the host controller driver, indicating
        // it is being processed by dpc. Thus, the dpc will certainly prevent the irp in
        // completion from being canceled at the same time. On the other hand, if the
        // hcd_cancel_urb succeeds, it either directly complete the irp or queue the dpc for
        // irp completion. So, there won't be two simutaneous threads processing the same
        // irp.

        if (usb_query_and_lock_dev(dev_mgr, endp_handle, &pdev) != STATUS_SUCCESS)
            return NULL;

        lock_dev(pdev, TRUE);
        if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
        {
            unlock_dev(pdev, FALSE);
            usb_unlock_dev(pdev);
            return NULL;
        }

        hcd = pdev->hcd;
        endp_from_handle(pdev, endp_handle, pendp);
        unlock_dev(pdev, TRUE);
        hcd->hcd_cancel_urb(hcd, pdev, pendp, purb);
        usb_unlock_dev(pdev);
        return NULL;
    }
    return purb;
}

BOOLEAN
irp_list_empty(PIRP_LIST irp_list)
{
    KIRQL old_irql;
    BOOLEAN ret;
    KeAcquireSpinLock(&irp_list->irp_list_lock, &old_irql);
    ret = (BOOLEAN) (irp_list->irp_free_list_count == MAX_IRP_LIST_SIZE);
    KeReleaseSpinLock(&irp_list->irp_list_lock, old_irql);
    return ret;
}

BOOLEAN
irp_list_full(PIRP_LIST irp_list)
{
    KIRQL old_irql;
    BOOLEAN ret;
    KeAcquireSpinLock(&irp_list->irp_list_lock, &old_irql);
    ret = (BOOLEAN) (irp_list->irp_free_list_count == 0);
    KeReleaseSpinLock(&irp_list->irp_list_lock, old_irql);
    return ret;
}
