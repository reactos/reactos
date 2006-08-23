/**
 * devmgr.c - USB driver stack project for Windows NT 4.0
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

USB_DRIVER g_driver_list[DEVMGR_MAX_DRIVERS];
USB_DEV_MANAGER g_dev_mgr;


//----------------------------------------------------------
BOOL
dev_mgr_set_if_driver(PUSB_DEV_MANAGER dev_mgr,
                      DEV_HANDLE if_handle,
                      PUSB_DRIVER pdriver,
                      PUSB_DEV pdev //if pdev != NULL, we use pdev instead if_handle, and must have dev_lock acquired.
                      )
{
    ULONG i;
    USE_BASIC_NON_PENDING_IRQL;

    if (dev_mgr == NULL || if_handle == 0 || pdriver == NULL)
        return FALSE;

    i = if_idx_from_handle(if_handle);
    if (pdev != NULL)
    {
        if (dev_state(pdev) < USB_DEV_STATE_BEFORE_ZOMB)
        {
            pdev->usb_config->interf[i].pif_drv = pdriver;
            return TRUE;
        }
        return FALSE;
    }

    if (usb_query_and_lock_dev(dev_mgr, if_handle, &pdev) != STATUS_SUCCESS)
        return FALSE;

    lock_dev(pdev, TRUE);
    if (dev_state(pdev) != USB_DEV_STATE_ZOMB)
    {
        pdev->usb_config->interf[i].pif_drv = pdriver;
    }
    unlock_dev(pdev, TRUE);
    usb_unlock_dev(pdev);
    return TRUE;
}

BOOL
dev_mgr_set_driver(PUSB_DEV_MANAGER dev_mgr,
                   DEV_HANDLE dev_handle,
                   PUSB_DRIVER pdriver,
                   PUSB_DEV pdev  //if pdev != NULL, we use pdev instead if_handle
                   )
{
    USE_BASIC_NON_PENDING_IRQL;

    if (dev_mgr == NULL || dev_handle == 0 || pdriver == NULL)
        return FALSE;

    if (pdev != NULL)
    {
        if (dev_state(pdev) < USB_DEV_STATE_BEFORE_ZOMB)
        {
            pdev->dev_driver = pdriver;
            return TRUE;
        }
        return FALSE;
    }

    if (usb_query_and_lock_dev(dev_mgr, dev_handle, &pdev) != STATUS_SUCCESS)
        return FALSE;

    lock_dev(pdev, FALSE);
    if (dev_state(pdev) < USB_DEV_STATE_BEFORE_ZOMB)
    {
        pdev->dev_driver = pdriver;
    }
    unlock_dev(pdev, FALSE);
    usb_unlock_dev(pdev);

    return TRUE;
}

BOOL
dev_mgr_post_event(PUSB_DEV_MANAGER dev_mgr, PUSB_EVENT event)
{
    KIRQL old_irql;

    if (dev_mgr == NULL || event == NULL)
        return FALSE;

    KeAcquireSpinLock(&dev_mgr->event_list_lock, &old_irql);
    InsertTailList(&dev_mgr->event_list, &event->event_link);
    KeReleaseSpinLock(&dev_mgr->event_list_lock, old_irql);

    KeSetEvent(&dev_mgr->wake_up_event, 0, FALSE);
    return TRUE;
}

VOID
dev_mgr_driver_entry_init(PUSB_DEV_MANAGER dev_mgr, PUSB_DRIVER pdrvr)
{
    // Device Info

    RtlZeroMemory(pdrvr, sizeof(USB_DRIVER) * DEVMGR_MAX_DRIVERS);

    pdrvr[RH_DRIVER_IDX].driver_init = rh_driver_init;  // in fact, this routine will init the rh device rather that the driver struct.
    pdrvr[RH_DRIVER_IDX].driver_destroy = rh_driver_destroy;    // we do not need rh to destroy currently, since that may means fatal hardware failure

    pdrvr[HUB_DRIVER_IDX].driver_init = hub_driver_init;        //no need, since dev_mgr is also a hub driver
    pdrvr[HUB_DRIVER_IDX].driver_destroy = hub_driver_destroy;

    pdrvr[UMSS_DRIVER_IDX].driver_init = umss_if_driver_init;
    pdrvr[UMSS_DRIVER_IDX].driver_destroy = umss_if_driver_destroy;

    pdrvr[COMP_DRIVER_IDX].driver_init = compdev_driver_init;
    pdrvr[COMP_DRIVER_IDX].driver_destroy = compdev_driver_destroy;

    pdrvr[GEN_DRIVER_IDX].driver_init = gendrv_driver_init;
    pdrvr[GEN_DRIVER_IDX].driver_destroy = gendrv_driver_destroy;

    pdrvr[GEN_IF_DRIVER_IDX].driver_init = gendrv_if_driver_init;
    pdrvr[GEN_IF_DRIVER_IDX].driver_destroy = gendrv_if_driver_destroy;
}

BOOL
dev_mgr_strobe(PUSB_DEV_MANAGER dev_mgr)
{
    PUSB_EVENT pevent;
    HANDLE thread_handle;

    if (dev_mgr == NULL)
        return FALSE;
    if (dev_mgr->hcd_count == 0)
        return FALSE;

    dev_mgr->term_flag = FALSE;

    if (dev_mgr->hcd_count == 0)
        return FALSE;

    KeInitializeSpinLock(&dev_mgr->event_list_lock);
    InitializeListHead(&dev_mgr->event_list);
    init_event_pool(&dev_mgr->event_pool);

    pevent = alloc_event(&dev_mgr->event_pool, 1);
    if (pevent == NULL)
    {
        destroy_event_pool(&dev_mgr->event_pool);
        return FALSE;
    }

    pevent->flags = USB_EVENT_FLAG_ACTIVE;
    pevent->event = USB_EVENT_INIT_DEV_MGR;

    pevent->process_queue = event_list_default_process_queue;
    pevent->process_event = dev_mgr_event_init;

    pevent->context = (ULONG) dev_mgr;

    KeInitializeEvent(&dev_mgr->wake_up_event, SynchronizationEvent, FALSE);

    InsertTailList(&dev_mgr->event_list, &pevent->event_link);

    if (PsCreateSystemThread(&thread_handle, 0, NULL, NULL, NULL, dev_mgr_thread, dev_mgr) != STATUS_SUCCESS)
    {
        destroy_event_pool(&dev_mgr->event_pool);
        return FALSE;
    }

    ObReferenceObjectByHandle(thread_handle,
                              THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID *) & dev_mgr->pthread, NULL);

    ZwClose(thread_handle);

    return TRUE;
}

BOOL
dev_mgr_event_init(PUSB_DEV pdev,       //always null. we do not use this param
                   ULONG event, ULONG context, ULONG param)
{
    LARGE_INTEGER due_time;
    PUSB_DEV_MANAGER dev_mgr;
    LONG i;

    usb_dbg_print(DBGLVL_MAXIMUM, ("dev_mgr_event_init(): dev_mgr=0x%x, event=0x%x\n", context, event));
    dev_mgr = (PUSB_DEV_MANAGER) context;
    if (dev_mgr == NULL)
        return FALSE;

    if (event != USB_EVENT_INIT_DEV_MGR)
        return FALSE;

    //dev_mgr->root_hub = NULL;
    KeInitializeTimer(&dev_mgr->dev_mgr_timer);

    KeInitializeDpc(&dev_mgr->dev_mgr_timer_dpc, dev_mgr_timer_dpc_callback, (PVOID) dev_mgr);

    KeInitializeSpinLock(&dev_mgr->timer_svc_list_lock);
    InitializeListHead(&dev_mgr->timer_svc_list);
    init_timer_svc_pool(&dev_mgr->timer_svc_pool);
    dev_mgr->timer_click = 0;

    init_irp_list(&dev_mgr->irp_list);

    KeInitializeSpinLock(&dev_mgr->dev_list_lock);
    InitializeListHead(&dev_mgr->dev_list);

    dev_mgr->hub_count = 0;
    InitializeListHead(&dev_mgr->hub_list);

    dev_mgr->conn_count = 0;
    dev_mgr->driver_list = g_driver_list;

    dev_mgr_driver_entry_init(dev_mgr, dev_mgr->driver_list);

    for(i = 0; i < DEVMGR_MAX_DRIVERS; i++)
    {
        if (dev_mgr->driver_list[i].driver_init == NULL)
            continue;

        if (dev_mgr->driver_list[i].driver_init(dev_mgr, &dev_mgr->driver_list[i]) == FALSE)
            break;
    }
    if (i == DEVMGR_MAX_DRIVERS)
    {
        due_time.QuadPart = -(DEV_MGR_TIMER_INTERVAL_NS - 10);

        KeSetTimerEx(&dev_mgr->dev_mgr_timer,
                     due_time, DEV_MGR_TIMER_INTERVAL_MS, &dev_mgr->dev_mgr_timer_dpc);

        return TRUE;
    }

    i--;

    for(; i >= 0; i--)
    {
        if (dev_mgr->driver_list[i].driver_destroy)
            dev_mgr->driver_list[i].driver_destroy(dev_mgr, &dev_mgr->driver_list[i]);
    }

    KeCancelTimer(&dev_mgr->dev_mgr_timer);
    KeRemoveQueueDpc(&dev_mgr->dev_mgr_timer_dpc);
    return FALSE;

}

VOID
dev_mgr_destroy(PUSB_DEV_MANAGER dev_mgr)
{
    LONG i;
    // oops...
    KeCancelTimer(&dev_mgr->dev_mgr_timer);
    KeRemoveQueueDpc(&dev_mgr->dev_mgr_timer_dpc);

    for(i = DEVMGR_MAX_DRIVERS - 1; i >= 0; i--)
        dev_mgr->driver_list[i].driver_destroy(dev_mgr, &dev_mgr->driver_list[i]);

    destroy_irp_list(&dev_mgr->irp_list);
    destroy_timer_svc_pool(&dev_mgr->timer_svc_pool);
    destroy_event_pool(&dev_mgr->event_pool);

}

VOID
dev_mgr_thread(PVOID context)
{
    PUSB_DEV_MANAGER dev_mgr;
    PUSB_EVENT pevent;
    PLIST_ENTRY pthis, pnext;
    USB_EVENT usb_event;
    LARGE_INTEGER time_out;
    NTSTATUS status;
    BOOL dev_mgr_inited;
    KIRQL old_irql;
    LONG i;

    dev_mgr = (PUSB_DEV_MANAGER) context;
    dev_mgr_inited = FALSE;
    usb_cal_cpu_freq();
    time_out.u.LowPart = (10 * 1000 * 1000) * 100 - 1;  //1 minutes
    time_out.u.HighPart = 0;
    time_out.QuadPart = -time_out.QuadPart;

    //usb_dbg_print( DBGLVL_MAXIMUM + 1, ( "dev_mgr_thread(): current uhci status=0x%x\n", uhci_status( dev_mgr->pdev_ext->uhci ) ) );

    while (dev_mgr->term_flag == FALSE)
    {
        KeAcquireSpinLock(&dev_mgr->event_list_lock, &old_irql);
        if (IsListEmpty(&dev_mgr->event_list) == TRUE)
        {
            KeReleaseSpinLock(&dev_mgr->event_list_lock, old_irql);
            status = KeWaitForSingleObject(&dev_mgr->wake_up_event, Executive, KernelMode, TRUE, &time_out);
            continue;
        }

        // usb_dbg_print( DBGLVL_MAXIMUM, ( "dev_mgr_thread(): current element in event list is 0x%x\n", \
        //                      dbg_count_list( &dev_mgr->event_list ) ) );

        dev_mgr_inited = TRUE;  //since we have post one event, if this statement is executed, dev_mgr_event_init must be called sometime later or earlier

        ListFirst(&dev_mgr->event_list, pthis);
        pevent = struct_ptr(pthis, USB_EVENT, event_link);

        while (pevent && ((pevent->flags & USB_EVENT_FLAG_ACTIVE) == 0))
        {
            //skip inactive ones
            ListNext(&dev_mgr->event_list, &pevent->event_link, pnext);
            pevent = struct_ptr(pnext, USB_EVENT, event_link);
        }

        if (pevent != NULL)
        {
            if (pevent->process_queue == NULL)
                pevent->process_queue = event_list_default_process_queue;

            pevent->process_queue(&dev_mgr->event_list, &dev_mgr->event_pool, pevent, &usb_event);
        }
        else
        {
            //no active event
            KeReleaseSpinLock(&dev_mgr->event_list_lock, old_irql);
            status = KeWaitForSingleObject(&dev_mgr->wake_up_event, Executive, KernelMode, TRUE, &time_out      // 10 minutes
                );

            usb_dbg_print(DBGLVL_MAXIMUM, ("dev_mgr_thread(): wake up, reason=0x%x\n", status));
            continue;
        }

        KeReleaseSpinLock(&dev_mgr->event_list_lock, old_irql);

        if (usb_event.process_event)
        {
            usb_event.process_event(usb_event.pdev, usb_event.event, usb_event.context, usb_event.param);
        }
        else
        {
            event_list_default_process_event(usb_event.pdev,
                                             usb_event.event, usb_event.context, usb_event.param);
        }
    }

    if (dev_mgr_inited)
    {
        for(i = 0; i < dev_mgr->hcd_count; i++)
            dev_mgr_disconnect_dev(dev_mgr->hcd_array[i]->hcd_get_root_hub(dev_mgr->hcd_array[i]));
        dev_mgr_destroy(dev_mgr);
    }
    PsTerminateSystemThread(0);
}

VOID
dev_mgr_timer_dpc_callback(PKDPC Dpc, PVOID context, PVOID SystemArgument1, PVOID SystemArgument2)
{
    PUSB_DEV_MANAGER dev_mgr;
    LIST_HEAD templist;
    PLIST_ENTRY pthis, pnext;
    static ULONG ticks = 0;

    ticks++;
    dev_mgr = (PUSB_DEV_MANAGER) context;
    if (dev_mgr == NULL)
        return;

    dev_mgr->timer_click++;
    InitializeListHead(&templist);

    KeAcquireSpinLockAtDpcLevel(&dev_mgr->timer_svc_list_lock);
    if (IsListEmpty(&dev_mgr->timer_svc_list) == TRUE)
    {
        KeReleaseSpinLockFromDpcLevel(&dev_mgr->timer_svc_list_lock);
        return;
    }

    ListFirst(&dev_mgr->timer_svc_list, pthis);
    while (pthis)
    {
        ((PTIMER_SVC) pthis)->counter++;
        ListNext(&dev_mgr->timer_svc_list, pthis, pnext);
        if (((PTIMER_SVC) pthis)->counter >= ((PTIMER_SVC) pthis)->threshold)
        {
            RemoveEntryList(pthis);
            InsertTailList(&templist, pthis);
        }
        pthis = pnext;
    }

    KeReleaseSpinLockFromDpcLevel(&dev_mgr->timer_svc_list_lock);


    while (IsListEmpty(&templist) == FALSE)
    {
        pthis = RemoveHeadList(&templist);
        ((PTIMER_SVC) pthis)->func(((PTIMER_SVC) pthis)->pdev, (PVOID) ((PTIMER_SVC) pthis)->context);
        KeAcquireSpinLockAtDpcLevel(&dev_mgr->timer_svc_list_lock);
        free_timer_svc(&dev_mgr->timer_svc_pool, (PTIMER_SVC) pthis);
        KeReleaseSpinLockFromDpcLevel(&dev_mgr->timer_svc_list_lock);
    }

}

BOOL
dev_mgr_request_timer_svc(PUSB_DEV_MANAGER dev_mgr,
                          PUSB_DEV pdev, ULONG context, ULONG due_time, TIMER_SVC_HANDLER handler)
{
    PTIMER_SVC timer_svc;
    KIRQL old_irql;

    if (dev_mgr == NULL || pdev == NULL || due_time == 0 || handler == NULL)
        return FALSE;

    KeAcquireSpinLock(&dev_mgr->timer_svc_list_lock, &old_irql);
    timer_svc = alloc_timer_svc(&dev_mgr->timer_svc_pool, 1);
    if (timer_svc == NULL)
    {
        KeReleaseSpinLock(&dev_mgr->timer_svc_list_lock, old_irql);
        return FALSE;
    }
    timer_svc->pdev = pdev;
    timer_svc->threshold = due_time;
    timer_svc->func = handler;
    timer_svc->counter = 0;

    InsertTailList(&dev_mgr->timer_svc_list, &timer_svc->timer_svc_link);
    KeReleaseSpinLock(&dev_mgr->timer_svc_list_lock, old_irql);
    return TRUE;
}

BYTE
dev_mgr_alloc_addr(PUSB_DEV_MANAGER dev_mgr, PHCD hcd)
{
    // alloc a usb addr for the device within 1-128
    if (dev_mgr == NULL || hcd == NULL)
        return 0xff;

    return hcd->hcd_alloc_addr(hcd);
}

BOOL
dev_mgr_free_addr(PUSB_DEV_MANAGER dev_mgr, PUSB_DEV pdev, BYTE addr)
{
    PHCD hcd;
    if (addr & 0x80)
        return FALSE;

    if (dev_mgr == NULL || pdev == NULL)
        return FALSE;

    hcd = pdev->hcd;
    if (hcd == NULL)
        return FALSE;
    hcd->hcd_free_addr(hcd, addr);
    return TRUE;
}

PUSB_DEV
dev_mgr_alloc_device(PUSB_DEV_MANAGER dev_mgr, PHCD hcd)
{
    BYTE addr;
    PUSB_DEV pdev;

    if ((addr = dev_mgr_alloc_addr(dev_mgr, hcd)) == 0xff)
        return NULL;

    pdev = usb_alloc_mem(NonPagedPool, sizeof(USB_DEV));
    if (pdev == NULL)
        return NULL;

    RtlZeroMemory(pdev, sizeof(USB_DEV));

    KeInitializeSpinLock(&pdev->dev_lock);
    dev_mgr->conn_count++;

    pdev->flags = USB_DEV_STATE_RESET;  //class | cur_state | low speed
    pdev->ref_count = 0;
    pdev->dev_addr = addr;

    pdev->hcd = hcd;

    pdev->dev_id = dev_mgr->conn_count; //will be used to compose dev_handle

    InitializeListHead(&pdev->default_endp.urb_list);
    pdev->default_endp.pusb_if = (PUSB_INTERFACE) pdev;
    pdev->default_endp.flags = USB_ENDP_FLAG_DEFAULT_ENDP;      //toggle | busy-count | stall | default-endp

    return pdev;
}

VOID
dev_mgr_free_device(PUSB_DEV_MANAGER dev_mgr, PUSB_DEV pdev)
{
    if (pdev == NULL || dev_mgr == NULL)
        return;

    dev_mgr_free_addr(dev_mgr, pdev, pdev->dev_addr);
    if (pdev->usb_config && pdev != pdev->hcd->hcd_get_root_hub(pdev->hcd))
    {
        //root hub has its config and desc buf allocated together,
        //so no usb_config allocated seperately
        dev_mgr_destroy_usb_config(pdev->usb_config);
        pdev->usb_config = NULL;
    }
    if (pdev->desc_buf)
    {
        usb_free_mem(pdev->desc_buf);
        pdev->desc_buf = NULL;
    }
    usb_free_mem(pdev);
    pdev = NULL;
    return;
}

//called when a disconnect is detected on the port
VOID
dev_mgr_disconnect_dev(PUSB_DEV pdev)
{
    PLIST_ENTRY pthis, pnext;
    PHUB2_EXTENSION phub_ext;
    PUSB_CONFIGURATION pconfig;
    PUSB_INTERFACE pif;
    PUSB_DEV_MANAGER dev_mgr;
    PHCD hcd;
    BOOL is_hub, found;
    ULONG dev_id;
    int i;

    USE_NON_PENDING_IRQL;

    if (pdev == NULL)
        return;

    found = FALSE;

    usb_dbg_print(DBGLVL_MAXIMUM, ("dev_mgr_disconnect_dev(): entering, pdev=0x%x\n", pdev));
    lock_dev(pdev, FALSE);
    pdev->flags &= ~USB_DEV_STATE_MASK;
    pdev->flags |= USB_DEV_STATE_BEFORE_ZOMB;
    dev_mgr = dev_mgr_from_dev(pdev);
    unlock_dev(pdev, FALSE);

    // notify dev_driver that the dev stops function before any operations
    if (pdev->dev_driver && pdev->dev_driver->disp_tbl.dev_stop)
        pdev->dev_driver->disp_tbl.dev_stop(dev_mgr, dev_handle_from_dev(pdev));

    //safe to use the dev pointer in this function.
    lock_dev(pdev, FALSE);
    pdev->flags &= ~USB_DEV_STATE_MASK;
    pdev->flags |= USB_DEV_STATE_ZOMB;
    hcd = pdev->hcd;
    dev_id = pdev->dev_id;
    unlock_dev(pdev, FALSE);

    if (dev_mgr == NULL)
        return;

    hcd->hcd_remove_device(hcd, pdev);

    //disconnect its children
    if ((pdev->flags & USB_DEV_CLASS_MASK) == USB_DEV_CLASS_HUB ||
        (pdev->flags & USB_DEV_CLASS_MASK) == USB_DEV_CLASS_ROOT_HUB)
    {
        phub_ext = hub_ext_from_dev(pdev);
        if (phub_ext)
        {
            for(i = 1; i <= phub_ext->port_count; i++)
            {
                if (phub_ext->child_dev[i])
                {
                    dev_mgr_disconnect_dev(phub_ext->child_dev[i]);
                    phub_ext->child_dev[i] = NULL;
                }
            }
        }
    }

    pconfig = pdev->usb_config;

    //remove event belong to the dev
    is_hub = ((pdev->flags & USB_DEV_CLASS_MASK) == USB_DEV_CLASS_HUB);

    if (phub_ext && is_hub)
    {
        for(i = 1; i <= phub_ext->port_count; i++)
        {
            found = hub_remove_reset_event(pdev, i, FALSE);
            if (found)
                break;
        }
    }

    //free event of the dev from the event list
    KeAcquireSpinLock(&dev_mgr->event_list_lock, &old_irql);
    ListFirst(&dev_mgr->event_list, pthis);
    while (pthis)
    {
        ListNext(&dev_mgr->event_list, pthis, pnext);
        if (((PUSB_EVENT) pthis)->pdev == pdev)
        {
            PLIST_ENTRY p1;
            RemoveEntryList(pthis);
            if ((((PUSB_EVENT) pthis)->flags & USB_EVENT_FLAG_QUE_TYPE) != USB_EVENT_FLAG_NOQUE)
            {
                //has a queue, re-insert the queue
                if (p1 = (PLIST_ENTRY) ((PUSB_EVENT) pthis)->pnext)
                {
                    InsertHeadList(&dev_mgr->event_list, p1);
                    free_event(&dev_mgr->event_pool, struct_ptr(pthis, USB_EVENT, event_link));
                    pthis = p1;
                    //note: this queue will be examined again in the next loop
                    //to find the matched dev in the queue
                    continue;
                }
            }
            free_event(&dev_mgr->event_pool, struct_ptr(pthis, USB_EVENT, event_link));
        }
        else if (((((PUSB_EVENT) pthis)->flags & USB_EVENT_FLAG_QUE_TYPE)
                  != USB_EVENT_FLAG_NOQUE) && ((PUSB_EVENT) pthis)->pnext)
        {
            //has a queue, examine the queue
            PUSB_EVENT p1, p2;
            p1 = (PUSB_EVENT) pthis;
            p2 = p1->pnext;
            while (p2)
            {
                if (p2->pdev == pdev)
                {
                    p1->pnext = p2->pnext;
                    p2->pnext = NULL;
                    free_event(&dev_mgr->event_pool, p2);
                    p2 = p1->pnext;
                }
                else
                {
                    p1 = p2;
                    p2 = p2->pnext;
                }
            }
        }
        pthis = pnext;
    }
    KeReleaseSpinLock(&dev_mgr->event_list_lock, old_irql);

    // found indicates the reset event on one of the dev's port in process
    if (found)
        hub_start_next_reset_port(dev_mgr_from_dev(pdev), FALSE);

    // remove timer-svc belonging to the dev
    KeAcquireSpinLock(&dev_mgr->timer_svc_list_lock, &old_irql);
    ListFirst(&dev_mgr->timer_svc_list, pthis);
    i = 0;
    while (pthis)
    {
        ListNext(&dev_mgr->timer_svc_list, pthis, pnext);
        if (((PUSB_EVENT) pthis)->pdev == pdev)
        {
            RemoveEntryList(pthis);
            free_timer_svc(&dev_mgr->timer_svc_pool, struct_ptr(pthis, TIMER_SVC, timer_svc_link));
            i++;
        }
        pthis = pnext;
    }
    KeReleaseSpinLock(&dev_mgr->timer_svc_list_lock, old_irql);

    // release the refcount
    if (i)
    {
        lock_dev(pdev, FALSE);
        pdev->ref_count -= i;
        unlock_dev(pdev, FALSE);
    }

    // wait for all the reference count be released
    for(;;)
    {
        LARGE_INTEGER interval;

        lock_dev(pdev, FALSE);
        if (pdev->ref_count == 0)
        {
            unlock_dev(pdev, FALSE);
            break;
        }
        unlock_dev(pdev, FALSE);
        // Wait two ms.
        interval.QuadPart = -20000;
        KeDelayExecutionThread(KernelMode, FALSE, &interval);
    }

    if (pdev->dev_driver && pdev->dev_driver->disp_tbl.dev_disconnect)
        pdev->dev_driver->disp_tbl.dev_disconnect(dev_mgr, dev_handle_from_dev(pdev));

    // we put it here to let handle valid before disconnect
    KeAcquireSpinLock(&dev_mgr->dev_list_lock, &old_irql);
    ListFirst(&dev_mgr->dev_list, pthis);
    while (pthis)
    {
        if (((PUSB_DEV) pthis) == pdev)
        {
            RemoveEntryList(pthis);
            break;
        }
        ListNext(&dev_mgr->dev_list, pthis, pnext);
        pthis = pnext;
    }
    KeReleaseSpinLock(&dev_mgr->dev_list_lock, old_irql);


    if (pdev != pdev->hcd->hcd_get_root_hub(pdev->hcd))
    {
        dev_mgr_free_device(dev_mgr, pdev);
    }
    else
    {
        //rh_destroy( pdev );
        //TRAP();
        //destroy it in dev_mgr_destroy
    }

    return;
}
