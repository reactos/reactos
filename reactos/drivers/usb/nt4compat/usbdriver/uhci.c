/**
 * uhci.c - USB driver stack project for Windows NT 4.0
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
// uhci routines
//#define DEMO

#ifdef INCLUDE_EHCI

#define rh_port1_status rh_port_status[ 1 ]
#define rh_port2_status rh_port_status[ 2 ]

extern PDEVICE_OBJECT ehci_probe(PDRIVER_OBJECT drvr_obj, PUNICODE_STRING reg_path, PUSB_DEV_MANAGER dev_mgr);

#endif

#define DEFAULT_ENDP( enDP ) \
( enDP->flags & USB_ENDP_FLAG_DEFAULT_ENDP )

#define dev_from_endp( enDP ) \
( DEFAULT_ENDP( enDP )\
  ? ( ( PUSB_DEV )( enDP )->pusb_if )\
  : ( ( enDP )->pusb_if->pusb_config->pusb_dev ) )

#define endp_state( enDP ) ( ( enDP )->flags & USB_ENDP_FLAG_STAT_MASK )

#define endp_num( enDP ) \
( DEFAULT_ENDP( enDP )\
  ? 0 \
  : ( ( enDP )->pusb_endp_desc->bEndpointAddress & 0x0f ) )

#define endp_dir( enDP ) \
( DEFAULT_ENDP( enDP )\
  ? 0L\
  : ( ( enDP )->pusb_endp_desc->bEndpointAddress & USB_DIR_IN ) )

#define dev_set_state( pdEV, staTE ) \
( pdEV->flags = ( ( pdEV )->flags & ( ~USB_DEV_STATE_MASK ) ) | ( staTE ) )

#define endp_max_packet_size( enDP ) \
( DEFAULT_ENDP( enDP )\
  ? ( ( ( PUSB_DEV )enDP->pusb_if )->pusb_dev_desc ? \
	  ( ( PUSB_DEV )enDP->pusb_if )->pusb_dev_desc->bMaxPacketSize0\
	  : 8 )\
  : enDP->pusb_endp_desc->wMaxPacketSize )


#if 0
/* WTF?! */
#define release_adapter( padapTER ) \
{\
    ( ( padapTER ) ); \
}
#else
#define release_adapter( padapTER ) (void)(padapTER)
#endif

#define get_int_idx( _urb, _idx ) \
{\
	UCHAR interVAL;\
	interVAL = ( UCHAR )( ( _urb )->pipe >> 24 );\
	for( _idx = 1; _idx < 9; _idx++ )\
	{\
		interVAL >>= 1;\
		if( !interVAL )\
			break;\
	}\
	_idx --;\
}

#define uhci_insert_urb_to_schedule( uHCI, pURB, rET ) \
{\
	SYNC_PARAM sync_param;\
	sync_param.uhci = uHCI;\
	sync_param.context = pURB;\
\
	rET = KeSynchronizeExecution( uHCI->pdev_ext->uhci_int, uhci_sync_insert_urb_schedule, &sync_param );\
}

//declarations
typedef struct
{
    PUHCI_DEV uhci;
    PVOID context;
    ULONG ret;
} SYNC_PARAM, *PSYNC_PARAM;

PDEVICE_OBJECT
uhci_alloc(PDRIVER_OBJECT drvr_obj, PUNICODE_STRING reg_path, ULONG bus_addr, PUSB_DEV_MANAGER dev_mgr);

BOOLEAN uhci_init_schedule(PUHCI_DEV uhci, PADAPTER_OBJECT padapter);

BOOLEAN uhci_release(PDEVICE_OBJECT pdev);

static VOID uhci_stop(PUHCI_DEV uhci);

BOOLEAN uhci_destroy_schedule(PUHCI_DEV uhci);

BOOLEAN NTAPI uhci_sync_insert_urb_schedule(PVOID context);

VOID uhci_init_hcd_interface(PUHCI_DEV uhci);

NTSTATUS uhci_rh_submit_urb(PUSB_DEV rh, PURB purb);

NTSTATUS uhci_dispatch_irp(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp);

extern VOID rh_timer_svc_reset_port_completion(PUSB_DEV dev, PVOID context);

extern VOID rh_timer_svc_int_completion(PUSB_DEV dev, PVOID context);

ULONG debug_level = DBGLVL_MINIMUM;//DBGLVL_MAXIMUM;
PDRIVER_OBJECT usb_driver_obj = NULL;
extern USB_DEV_MANAGER g_dev_mgr;

//pending endpoint pool funcs
VOID
uhci_wait_ms(PUHCI_DEV uhci, LONG ms)
{
    LARGE_INTEGER lms;
    if (ms <= 0)
        return;

    lms.QuadPart = -10 * ms;
    KeSetTimer(&uhci->reset_timer, lms, NULL);

    KeWaitForSingleObject(&uhci->reset_timer, Executive, KernelMode, FALSE, NULL);

    return;
}

BOOLEAN
init_pending_endp_pool(PUHCI_PENDING_ENDP_POOL pool)
{
    int i;
    if (pool == NULL)
        return FALSE;

    pool->pending_endp_array =
        usb_alloc_mem(NonPagedPool, sizeof(UHCI_PENDING_ENDP) * UHCI_MAX_PENDING_ENDPS);
    InitializeListHead(&pool->free_que);
    pool->free_count = 0;
    pool->total_count = UHCI_MAX_PENDING_ENDPS;
    KeInitializeSpinLock(&pool->pool_lock);

    for(i = 0; i < MAX_TIMER_SVCS; i++)
    {
        free_pending_endp(pool, &pool->pending_endp_array[i]);
    }

    return TRUE;

}

BOOLEAN
free_pending_endp(PUHCI_PENDING_ENDP_POOL pool, PUHCI_PENDING_ENDP pending_endp)
{
    if (pool == NULL || pending_endp == NULL)
    {
        return FALSE;
    }

    RtlZeroMemory(pending_endp, sizeof(UHCI_PENDING_ENDP));
    InsertTailList(&pool->free_que, (PLIST_ENTRY) & pending_endp->endp_link);
    pool->free_count++;

    return TRUE;
}

PUHCI_PENDING_ENDP
alloc_pending_endp(PUHCI_PENDING_ENDP_POOL pool, LONG count)
{
    PUHCI_PENDING_ENDP new;
    if (pool == NULL || count != 1)
        return NULL;

    if (pool->free_count <= 0)
        return NULL;

    new = (PUHCI_PENDING_ENDP) RemoveHeadList(&pool->free_que);
    pool->free_count--;
    return new;
}

BOOLEAN
destroy_pending_endp_pool(PUHCI_PENDING_ENDP_POOL pool)
{
    if (pool == NULL)
        return FALSE;

    InitializeListHead(&pool->free_que);
    pool->free_count = pool->total_count = 0;
    usb_free_mem(pool->pending_endp_array);
    pool->pending_endp_array = NULL;

    return TRUE;

}


//end of pending endpoint pool funcs

static void
uhci_fill_td(PUHCI_TD td, ULONG status, ULONG info, ULONG buffer)
{
    td->status = status;
    td->info = info;
    td->buffer = buffer;
}

BOOLEAN
uhci_insert_td_fl(PUHCI_TD prev_td, PUHCI_TD ptd)
{
    PLIST_ENTRY temp_entry;

    if (prev_td == NULL || ptd == NULL)
        return FALSE;

    temp_entry = &prev_td->ptde->hori_link;

    ptd->link = (struct_ptr(temp_entry, TD_EXTENSION, hori_link))->ptd->phy_addr;
    prev_td->link = ptd->phy_addr;

    InsertHeadList(&prev_td->ptde->hori_link, &ptd->ptde->hori_link);
    return TRUE;
}

BOOLEAN
uhci_remove_td_fl(PUHCI_TD ptd)
{
    PUHCI_TD prev_td;

    if (ptd == NULL)
        return FALSE;

    prev_td = (struct_ptr(ptd->ptde->hori_link.Blink, TD_EXTENSION, hori_link))->ptd;
    prev_td->link = ptd->link;
    ptd->link = UHCI_PTR_TERM;

    RemoveEntryList(&ptd->ptde->hori_link);

    return FALSE;
}

BOOLEAN
uhci_insert_qh_fl(PVOID prev_item, PUHCI_QH pqh)
{
    //only horizontal link allowed
    PUHCI_QH pprev_qh;
    PUHCI_TD pprev_td;
    PLIST_ENTRY temp_entry;

    if (prev_item == NULL || pqh == NULL)
        return FALSE;

    if ((((PUHCI_TD) prev_item)->ptde->flags & UHCI_ITEM_FLAG_TYPE) == UHCI_ITEM_FLAG_QH)
    {
        pprev_qh = (PUHCI_QH) prev_item;
        temp_entry = pprev_qh->pqhe->hori_link.Flink;
        pqh->link = (struct_ptr(temp_entry, TD_EXTENSION, hori_link))->ptd->phy_addr;
        pprev_qh->link = pqh->phy_addr;

        InsertHeadList(&pprev_qh->pqhe->hori_link, &pqh->pqhe->hori_link);
    }
    else
    {
        pprev_td = ((PUHCI_TD) prev_item);

        temp_entry = pprev_td->ptde->hori_link.Flink;
        pprev_td->link = pqh->phy_addr;
        pqh->link = (struct_ptr(temp_entry, TD_EXTENSION, hori_link))->ptd->phy_addr;

        InsertHeadList(&pprev_td->ptde->hori_link, &pqh->pqhe->hori_link);
    }

    return FALSE;
}

BOOLEAN
uhci_remove_qh_fl(PUHCI_QH pqh)
{
    PVOID prev_item;
    PUHCI_QH pprevqh;
    PUHCI_TD pprevtd;

    if (pqh == NULL)
        return FALSE;

    prev_item = (struct_ptr(pqh->pqhe->hori_link.Blink, TD_EXTENSION, hori_link))->ptd;

    if ((((PUHCI_TD) prev_item)->ptde->flags & UHCI_ITEM_FLAG_TYPE) == UHCI_ITEM_FLAG_QH)
    {
        pprevqh = (PUHCI_QH) prev_item;
        pprevqh->link = pqh->link;
    }
    else
    {
        pprevtd = ((PUHCI_TD) prev_item);
        pprevtd->link = pqh->link;
    }

    RemoveEntryList(&pqh->pqhe->hori_link);

    pqh->link = UHCI_PTR_TERM;
    pqh->pqhe->hori_link.Flink = pqh->pqhe->hori_link.Blink = NULL;

    return TRUE;
}

BOOLEAN
uhci_init_frame_list(PUHCI_DEV uhci, PADAPTER_OBJECT padapter)
{
    int i;
    if (uhci == NULL || padapter == NULL)
        return FALSE;

    //note: frame_list_lock will be connected to interrupt
    KeInitializeSpinLock(&uhci->frame_list_lock);

    uhci->io_buf = HalAllocateCommonBuffer(padapter, 4096, &uhci->io_buf_logic_addr, FALSE);

    if (uhci->io_buf == NULL)
        return FALSE;

    uhci->frame_list =
        HalAllocateCommonBuffer(padapter,
                                sizeof(ULONG) * UHCI_MAX_FRAMES, &uhci->frame_list_logic_addr, FALSE);

    if (uhci->frame_list == NULL)
        return FALSE;

    RtlZeroMemory(uhci->frame_list, sizeof(ULONG) * UHCI_MAX_FRAMES);

    uhci->frame_list_cpu = usb_alloc_mem(NonPagedPool, sizeof(FRAME_LIST_CPU_ENTRY) * UHCI_MAX_FRAMES);

    if (uhci->frame_list_cpu == NULL)
        return FALSE;

    for(i = 0; i < UHCI_MAX_FRAMES; i++)
        InitializeListHead(&uhci->frame_list_cpu[i].td_link);

    uhci->frame_bw = usb_alloc_mem(NonPagedPool, sizeof(LONG) * UHCI_MAX_FRAMES);

    if (uhci->frame_bw == NULL)
        return FALSE;

    for(i = 0; i < UHCI_MAX_FRAMES; i++)
    {
        uhci->frame_bw[i] = FRAME_TIME_MAX_USECS_ALLOC;
    }
    uhci->fsbr_cnt = 0;

    return TRUE;

}

BOOLEAN
uhci_destroy_frame_list(PUHCI_DEV uhci)
{
    if (uhci == NULL)
        return FALSE;

    if (uhci->frame_list)
        HalFreeCommonBuffer(uhci->pdev_ext->padapter,
                            sizeof(ULONG) * UHCI_MAX_FRAMES,
                            uhci->frame_list_logic_addr, uhci->frame_list, FALSE);

    uhci->frame_list = NULL;
    uhci->frame_list_logic_addr.LowPart = 0;
    uhci->frame_list_logic_addr.HighPart = 0;

    if (uhci->frame_list_cpu)
        usb_free_mem(uhci->frame_list_cpu);

    uhci->frame_list_cpu = NULL;

    if (uhci->frame_bw)
        usb_free_mem(uhci->frame_bw);

    uhci->frame_bw = NULL;

    return TRUE;
}

PDEVICE_OBJECT
uhci_create_device(PDRIVER_OBJECT drvr_obj, PUSB_DEV_MANAGER dev_mgr)
{
    NTSTATUS status;
    PDEVICE_OBJECT pdev;
    PDEVICE_EXTENSION pdev_ext;

    UNICODE_STRING dev_name;
    UNICODE_STRING symb_name;

    STRING string, another_string;
    CHAR str_dev_name[64], str_symb_name[64];
    UCHAR hcd_id;

    if (drvr_obj == NULL)
        return NULL;

    ASSERT(dev_mgr != NULL);

    //note: hcd count wont increment till the hcd is registered in dev_mgr
    sprintf(str_dev_name, "%s%d", UHCI_DEVICE_NAME, dev_mgr->hcd_count);
    sprintf(str_symb_name, "%s%d", DOS_DEVICE_NAME, dev_mgr->hcd_count);

    RtlInitString(&string, str_dev_name);
    RtlAnsiStringToUnicodeString(&dev_name, &string, TRUE);

    pdev = NULL;
    status = IoCreateDevice(drvr_obj,
                            sizeof(DEVICE_EXTENSION) + sizeof(UHCI_DEV),
                            &dev_name, FILE_UHCI_DEV_TYPE, 0, FALSE, &pdev);

    if (status != STATUS_SUCCESS || pdev == NULL)
    {
        RtlFreeUnicodeString(&dev_name);
        return NULL;
    }

    pdev_ext = pdev->DeviceExtension;
    RtlZeroMemory(pdev_ext, sizeof(DEVICE_EXTENSION) + sizeof(UHCI_DEV));

    pdev_ext->dev_ext_hdr.type = NTDEV_TYPE_HCD;
    pdev_ext->dev_ext_hdr.dispatch = uhci_dispatch_irp;
    pdev_ext->dev_ext_hdr.start_io = NULL;      //we do not support startio
    pdev_ext->dev_ext_hdr.dev_mgr = dev_mgr;

    pdev_ext->pdev_obj = pdev;
    pdev_ext->pdrvr_obj = drvr_obj;

    pdev_ext->uhci = (PUHCI_DEV) & (pdev_ext[1]);

    RtlInitString(&another_string, str_symb_name);
    RtlAnsiStringToUnicodeString(&symb_name, &another_string, TRUE);

    IoCreateSymbolicLink(&symb_name, &dev_name);

    uhci_dbg_print(DBGLVL_MAXIMUM,
                   ("uhci_create_device(): dev=0x%x\n, pdev_ext= 0x%x, uhci=0x%x, dev_mgr=0x%x\n", pdev,
                    pdev_ext, pdev_ext->uhci, dev_mgr));

    RtlFreeUnicodeString(&dev_name);
    RtlFreeUnicodeString(&symb_name);

    //register with dev_mgr though it is not initilized
    uhci_init_hcd_interface(pdev_ext->uhci);
    hcd_id = dev_mgr_register_hcd(dev_mgr, &pdev_ext->uhci->hcd_interf);

    pdev_ext->uhci->hcd_interf.hcd_set_id(&pdev_ext->uhci->hcd_interf, hcd_id);
    pdev_ext->uhci->hcd_interf.hcd_set_dev_mgr(&pdev_ext->uhci->hcd_interf, dev_mgr);
    return pdev;
}

BOOLEAN
uhci_delete_device(PDEVICE_OBJECT pdev)
{
    STRING string;
    UNICODE_STRING symb_name;
    PDEVICE_EXTENSION pdev_ext;
    CHAR str_symb_name[64];


    if (pdev == NULL)
        return FALSE;

    pdev_ext = pdev->DeviceExtension;

    sprintf(str_symb_name,
            "%s%d", DOS_DEVICE_NAME, pdev_ext->uhci->hcd_interf.hcd_get_id(&pdev_ext->uhci->hcd_interf));
    RtlInitString(&string, str_symb_name);
    RtlAnsiStringToUnicodeString(&symb_name, &string, TRUE);
    IoDeleteSymbolicLink(&symb_name);
    RtlFreeUnicodeString(&symb_name);

    if (pdev_ext->res_list)
        ExFreePool(pdev_ext->res_list); //      not allocated by usb_alloc_mem

    IoDeleteDevice(pdev);
    uhci_dbg_print(DBGLVL_MAXIMUM, ("uhci_delete_device(): device deleted\n"));
    return TRUE;
}

// we can not use endp here for it is within the dev scope, and
// we can not acquire the dev-lock, fortunately we saved some
// info in urb->pipe in uhci_internal_submit_XXX.
BOOLEAN NTAPI
uhci_isr(PKINTERRUPT interrupt, PVOID context)
{
    PUHCI_DEV uhci;
    USHORT status;
    PLIST_ENTRY pthis, pnext;
    PURB purb;

    UNREFERENCED_PARAMETER(interrupt);
    UNREFERENCED_PARAMETER(context);

    uhci_dbg_print(DBGLVL_ULTRA, ("uhci_isr(): context=0x%x\n", context));

    /*
     * Read the interrupt status, and write it back to clear the
     * interrupt cause
     */
    uhci = (PUHCI_DEV) context;
    if (uhci == NULL)
        return FALSE;

    status = READ_PORT_USHORT((PUSHORT) (uhci->port_base + USBSTS));
    if (!status)                /* shared interrupt, not mine */
        return FALSE;

    if (status != 1)
    {
        uhci_dbg_print(DBGLVL_MAXIMUM, ("uhci_isr():  current uhci status=0x%x\n", status));
    }
    else
    {
        uhci_dbg_print(DBGLVL_MAXIMUM, ("uhci_isr():  congratulations, no error occurs\n"));
    }

    /* clear it */
    WRITE_PORT_USHORT((PUSHORT) (uhci->port_base + USBSTS), status);

    if (status & ~(USBSTS_USBINT | USBSTS_ERROR | USBSTS_RD))
    {
        if (status & USBSTS_HSE)
        {
            DbgPrint("uhci_isr(): host system error, PCI problems?\n");
            //for( ; ; );
        }
        if (status & USBSTS_HCPE)
        {
            DbgPrint("uhci_isr(): host controller process error. something bad happened\n");
            //for( ; ; );
            //for( ; ; );
        }
        if ((status & USBSTS_HCH))      //&& !uhci->is_suspended
        {
            DbgPrint("uhci_isr(): host controller halted. very bad\n");
            /* FIXME: Reset the controller, fix the offending TD */
        }
    }

    // don't no how to handle it yet
    //if (status & USBSTS_RD)
    //{
    //uhci_wakeup(uhci);
    //}*/

    //let's remove those force-cancel urbs from the schedule first
    ListFirst(&uhci->urb_list, pthis);
    while (pthis)
    {
        purb = (PURB) pthis;
        if (purb->flags & URB_FLAG_FORCE_CANCEL)
        {
            uhci_remove_urb_from_schedule(uhci, purb);
        }
        ListNext(&uhci->urb_list, pthis, pnext);
        pthis = pnext;
    }

    //clear the interrupt if the urb is force canceled
    uhci->skel_term_td->status &= ~TD_CTRL_IOC;

    //next we need to find if anything fininshed
    ListFirst(&uhci->urb_list, pthis);
    while (pthis)
    {
        purb = (PURB) pthis;
        if (purb->flags & URB_FLAG_IN_SCHEDULE)
        {
            if (uhci_is_xfer_finished(purb))
                uhci_remove_urb_from_schedule(uhci, purb);
        }
        ListNext(&uhci->urb_list, pthis, pnext);
        pthis = pnext;
    }

    KeInsertQueueDpc(&uhci->pdev_ext->uhci_dpc, uhci, 0);
    return TRUE;
}

BOOLEAN NTAPI
uhci_cal_cpu_freq(PVOID context)
{
    UNREFERENCED_PARAMETER(context);

    usb_cal_cpu_freq();
    return TRUE;
}

PDEVICE_OBJECT
uhci_probe(PDRIVER_OBJECT drvr_obj, PUNICODE_STRING reg_path, PUSB_DEV_MANAGER dev_mgr)
{
    LONG bus, i, j, ret = 0;
    PCI_SLOT_NUMBER slot_num;
    PPCI_COMMON_CONFIG pci_config;
    PDEVICE_OBJECT pdev;
    BYTE buffer[sizeof(PCI_COMMON_CONFIG)];
    LONG count;
    PDEVICE_EXTENSION pdev_ext;

    slot_num.u.AsULONG = 0;
    pci_config = (PPCI_COMMON_CONFIG) buffer;
    count = 0;
    pdev = NULL;

    //scan the bus to find uhci controller
    for(bus = 0; bus < 3; bus++)        /* enum bus0-bus2 */
    {
        for(i = 0; i < PCI_MAX_DEVICES; i++)
        {
            slot_num.u.bits.DeviceNumber = i;
            for(j = 0; j < PCI_MAX_FUNCTIONS; j++)
            {
                slot_num.u.bits.FunctionNumber = j;

                ret = HalGetBusData(PCIConfiguration,
                                    bus, slot_num.u.AsULONG, pci_config, PCI_COMMON_HDR_LENGTH);

                if (ret == 0)   /*no this bus */
                    break;

                if (ret == 2)   /*no device on the slot */
                    break;

                if (pci_config->BaseClass == 0x0c && pci_config->SubClass == 0x03)
                {
                    // well, we find our usb host controller, create device
#ifdef _MULTI_UHCI
                    {
                        pdev = uhci_alloc(drvr_obj, reg_path, ((bus << 8) | (i << 3) | j), dev_mgr);
                        count++;
                        if (!pdev)
                            return NULL;
                    }
#else
                    pdev = uhci_alloc(drvr_obj, reg_path, ((bus << 8) | (i << 3) | j), dev_mgr);
                    if (pdev)
                        goto LBL_LOOPOUT;
#endif
                }
            }
            if (ret == 0)
                break;
        }
    }

LBL_LOOPOUT:
    if (pdev)
    {
        pdev_ext = pdev->DeviceExtension;
        if (pdev_ext)
        {
            // acquire higher irql to eliminate pre-empty
            KeSynchronizeExecution(pdev_ext->uhci_int, uhci_cal_cpu_freq, NULL);
        }
    }
    return NULL;
}

PDEVICE_OBJECT
uhci_alloc(PDRIVER_OBJECT drvr_obj, PUNICODE_STRING reg_path, ULONG bus_addr, PUSB_DEV_MANAGER dev_mgr)
{
    LONG frd_num, prd_num;
    PDEVICE_OBJECT pdev;
    PDEVICE_EXTENSION pdev_ext;
    ULONG vector, addr_space;
    LONG bus;
    KIRQL irql;
    KAFFINITY affinity;

    DEVICE_DESCRIPTION dev_desc;
    CM_PARTIAL_RESOURCE_DESCRIPTOR *pprd;
    PCI_SLOT_NUMBER slot_num;
    NTSTATUS status;


    pdev = uhci_create_device(drvr_obj, dev_mgr);
    if (pdev == NULL)
        return pdev;
    pdev_ext = pdev->DeviceExtension;

    pdev_ext->pci_addr = bus_addr;
    bus = (bus_addr >> 8);

    slot_num.u.AsULONG = 0;
    slot_num.u.bits.DeviceNumber = ((bus_addr & 0xff) >> 3);
    slot_num.u.bits.FunctionNumber = (bus_addr & 0x07);

    //now create adapter object
    RtlZeroMemory(&dev_desc, sizeof(dev_desc));

    dev_desc.Version = DEVICE_DESCRIPTION_VERSION;
    dev_desc.Master = TRUE;
    dev_desc.ScatterGather = TRUE;
    dev_desc.Dma32BitAddresses = TRUE;
    dev_desc.BusNumber = bus;
    dev_desc.InterfaceType = PCIBus;
    dev_desc.MaximumLength =
        UHCI_MAX_POOL_TDS * sizeof(UHCI_TD) * UHCI_MAX_TD_POOLS
        + sizeof(UHCI_QH) * UHCI_MAX_POOL_QHS + sizeof(ULONG) * UHCI_MAX_FRAMES;

    pdev_ext->map_regs = 2;     // UHCI_MAX_TD_POOLS +
    //+ BYTES_TO_PAGES( ( UHCI_MAX_POOL_TDS * 64 ) * UHCI_MAX_TD_POOLS ) ;

    pdev_ext->padapter = HalGetAdapter(&dev_desc, &pdev_ext->map_regs);

    uhci_dbg_print(DBGLVL_MAXIMUM, ("uhci_alloc(): padapter=0x%x\n", pdev_ext->padapter));
    if (pdev_ext->padapter == NULL)
    {
        //fatal error
        uhci_delete_device(pdev);
        return NULL;
    }

    DbgPrint("uhci_alloc(): reg_path=%p, \n \
             uhci_alloc(): PCIBus=0x%x, bus=0x%x, bus_addr=0x%x \n \
             uhci_alloc(): slot_num=0x%x, &res_list=%p \n", reg_path, (DWORD) PCIBus, (DWORD) bus,
             (DWORD) bus_addr, (DWORD) slot_num.u.AsULONG, & pdev_ext->res_list);

    //let's allocate resources for this device
    DbgPrint("uhci_alloc(): about to assign slot res\n");
    if ((status = HalAssignSlotResources(reg_path, NULL,        //no class name yet
                                         drvr_obj, NULL,        //no support of another uhci controller
                                         PCIBus,
                                         bus, slot_num.u.AsULONG, &pdev_ext->res_list)) != STATUS_SUCCESS)
    {
        DbgPrint("uhci_alloc(): error assign slot res, 0x%x\n", status);
        release_adapter(pdev_ext->padapter);
        pdev_ext->padapter = NULL;
        uhci_delete_device(pdev);
        return NULL;
    }

    //parse the resource list
    for(frd_num = 0; frd_num < (LONG) pdev_ext->res_list->Count; frd_num++)
    {
        for(prd_num = 0; prd_num < (LONG) pdev_ext->res_list->List[frd_num].PartialResourceList.Count;
            prd_num++)
        {
            pprd = &pdev_ext->res_list->List[frd_num].PartialResourceList.PartialDescriptors[prd_num];
            if (pprd->Type == CmResourceTypePort)
            {
                RtlCopyMemory(&pdev_ext->res_port, &pprd->u.Port, sizeof(pprd->u.Port));
            }
            else if (pprd->Type == CmResourceTypeInterrupt)
            {
                RtlCopyMemory(&pdev_ext->res_interrupt, &pprd->u.Interrupt, sizeof(pprd->u.Interrupt));
            }
        }
    }

    //for port, translate them to system address
    addr_space = 1;
    if (HalTranslateBusAddress(PCIBus, bus, pdev_ext->res_port.Start, &addr_space,      //io space
                               &pdev_ext->uhci->uhci_reg_base) != (BOOLEAN) TRUE)
    {
        DbgPrint("uhci_alloc(): error, can not translate bus address\n");
        release_adapter(pdev_ext->padapter);
        pdev_ext->padapter = NULL;
        uhci_delete_device(pdev);
        return NULL;
    }

    DbgPrint("uhci_alloc(): address space=0x%x\n, reg_base=0x%x\n",
             addr_space, pdev_ext->uhci->uhci_reg_base.u.LowPart);

    if (addr_space == 0)
    {
        //port has been mapped to memory space
        pdev_ext->uhci->port_mapped = TRUE;
        pdev_ext->uhci->port_base = (PBYTE) MmMapIoSpace(pdev_ext->uhci->uhci_reg_base,
                                                         pdev_ext->res_port.Length, FALSE);

        //fatal error can not map the registers
        if (pdev_ext->uhci->port_base == NULL)
        {
            release_adapter(pdev_ext->padapter);
            pdev_ext->padapter = NULL;
            uhci_delete_device(pdev);
            return NULL;
        }
    }
    else
    {
        //io space
        pdev_ext->uhci->port_mapped = FALSE;
        pdev_ext->uhci->port_base = (PBYTE) pdev_ext->uhci->uhci_reg_base.LowPart;
    }

    //before we connect the interrupt, we have to init uhci
    pdev_ext->uhci->fsbr_cnt = 0;
    pdev_ext->uhci->pdev_ext = pdev_ext;

    if (uhci_init_schedule(pdev_ext->uhci, pdev_ext->padapter) == FALSE)
    {
        release_adapter(pdev_ext->padapter);
        pdev_ext->padapter = NULL;
        uhci_delete_device(pdev);
        return NULL;
    }

    InitializeListHead(&pdev_ext->uhci->urb_list);
    KeInitializeSpinLock(&pdev_ext->uhci->pending_endp_list_lock);
    InitializeListHead(&pdev_ext->uhci->pending_endp_list);

    uhci_dbg_print(DBGLVL_MAXIMUM, ("uhci_alloc(): pending_endp_list=0x%x\n",
                                    &pdev_ext->uhci->pending_endp_list));

    init_pending_endp_pool(&pdev_ext->uhci->pending_endp_pool);
    KeInitializeTimer(&pdev_ext->uhci->reset_timer);

    vector = HalGetInterruptVector(PCIBus,
                                   bus,
                                   pdev_ext->res_interrupt.level,
                                   pdev_ext->res_interrupt.vector,
                                   &irql,
                                   &affinity);

    //connect the interrupt
    DbgPrint("uhci_alloc(): the int=0x%x\n", vector);
    if (IoConnectInterrupt(&pdev_ext->uhci_int,
                           uhci_isr,
                           pdev_ext->uhci,
                           NULL, //&pdev_ext->uhci->frame_list_lock,
                           vector,
                           irql,
                           irql,
                           LevelSensitive,
                           TRUE,    //share the vector
                           affinity,
                           FALSE)     //No float save
        != STATUS_SUCCESS)
    {
        uhci_release(pdev);
        return NULL;
    }

    KeInitializeDpc(&pdev_ext->uhci_dpc, uhci_dpc_callback, (PVOID) pdev_ext->uhci);

    return pdev;
}

BOOLEAN
uhci_release(PDEVICE_OBJECT pdev)
{
    PDEVICE_EXTENSION pdev_ext;
    PUHCI_DEV uhci;

    if (pdev == NULL)
        return FALSE;

    pdev_ext = pdev->DeviceExtension;

    if (pdev_ext == NULL)
        return FALSE;

    uhci = pdev_ext->uhci;
    if (uhci == NULL)
        return FALSE;

    uhci_stop(uhci);
    //pdev_ext->uhci->conn_count = 0;
    pdev_ext->uhci->fsbr_cnt = 0;

    if (pdev_ext->uhci_int)
    {
        IoDisconnectInterrupt(pdev_ext->uhci_int);
        pdev_ext->uhci_int = NULL;
    }
    else
        TRAP();
    destroy_pending_endp_pool(&pdev_ext->uhci->pending_endp_pool);
    //pdev_ext->uhci->pending_endp_pool = NULL;

    uhci_destroy_schedule(uhci);

    release_adapter(pdev_ext->padapter);
    pdev_ext->padapter = NULL;

    uhci_delete_device(pdev);

    return FALSE;

}

// send cmds to start the uhc
// shamelessly copied from linux's uhci.c (reset_hc(), configure_hc() routines)
BOOLEAN
uhci_start(PHCD hcd)
{
    PUHCI_DEV uhci;
    PBYTE io_addr;
    USHORT pirq;
    PCI_SLOT_NUMBER SlotNum;
    int timeout = 10000;

    uhci = uhci_from_hcd(hcd);
    io_addr = uhci->port_base;

    /*
     * Reset the HC - this will force us to get a
     * new notification of any already connected
     * ports due to the virtual disconnect that it
     * implies.
     */
    WRITE_PORT_USHORT((PUSHORT) (io_addr + USBCMD), USBCMD_HCRESET);
    while (READ_PORT_USHORT((PUSHORT) (io_addr + USBCMD)) & USBCMD_HCRESET)
    {
        if (!--timeout)
        {
            break;
        }
    }

    /* Turn on all interrupts */
    WRITE_PORT_USHORT((PUSHORT) (io_addr + USBINTR),
                      USBINTR_TIMEOUT | USBINTR_RESUME | USBINTR_IOC | USBINTR_SP);

    /* Start at frame 0 */
    WRITE_PORT_USHORT((PUSHORT) (io_addr + USBFRNUM), 0);
    WRITE_PORT_ULONG((PULONG) (io_addr + USBFLBASEADD), uhci->frame_list_logic_addr.LowPart);

    /* Run and mark it configured with a 64-byte max packet */
    WRITE_PORT_USHORT((PUSHORT) (io_addr + USBCMD), USBCMD_RS | USBCMD_CF | USBCMD_MAXP);

    DbgPrint("uhci_start(): current uhci status=0x%x\n", uhci_status(uhci));

    /* Enable PIRQ */
    pirq = USBLEGSUP_DEFAULT;
    SlotNum.u.AsULONG = 0;
    SlotNum.u.bits.DeviceNumber = ((uhci->pdev_ext->pci_addr & 0xff) >> 3);
    SlotNum.u.bits.FunctionNumber = (uhci->pdev_ext->pci_addr & 0x07);

    DbgPrint("uhci_start(): set bus %d data at slot 0x%x\n", (uhci->pdev_ext->pci_addr >> 8),
        SlotNum.u.AsULONG);

    HalSetBusDataByOffset(PCIConfiguration, (uhci->pdev_ext->pci_addr >> 8), SlotNum.u.AsULONG,
        &pirq, USBLEGSUP, sizeof(pirq));

    return TRUE;
}

VOID
uhci_stop(PUHCI_DEV uhci)
{
    PBYTE io_addr = uhci->port_base;
    // turn off all the interrupt
    WRITE_PORT_USHORT((PUSHORT) (io_addr + USBINTR), 0);
    WRITE_PORT_USHORT((PUSHORT) (io_addr + USBCMD), 0);
}

static VOID
uhci_reset(PUHCI_DEV uhci)
{
    PBYTE io_addr = uhci->port_base;

    uhci_stop(uhci);
    /* Global reset for 50ms */
    WRITE_PORT_USHORT((PUSHORT) (io_addr + USBCMD), USBCMD_GRESET);
    //uhci_wait_ms( uhci, 50 );
    usb_wait_ms_dpc(50);

    WRITE_PORT_USHORT((PUSHORT) (io_addr + USBCMD), 0);
    //uhci_wait_ms( uhci, 10 );
    usb_wait_ms_dpc(10);
}

VOID
uhci_suspend(PUHCI_DEV uhci)
{
    PBYTE io_addr = uhci->port_base;

    //uhci->is_suspended = 1;
    WRITE_PORT_USHORT((PUSHORT) (io_addr + USBCMD), USBCMD_EGSM);

}

VOID
uhci_wakeup(PUHCI_DEV uhci)
{
    PBYTE io_addr;
    unsigned int status;

    io_addr = uhci->port_base;

    WRITE_PORT_USHORT((PUSHORT) (io_addr + USBCMD), 0);

    /* wait for EOP to be sent */
    status = READ_PORT_USHORT((PUSHORT) (io_addr + USBCMD));
    while (status & USBCMD_FGR)
        status = READ_PORT_USHORT((PUSHORT) (io_addr + USBCMD));

    //uhci->is_suspended = 0;

    /* Run and mark it configured with a 64-byte max packet */
    WRITE_PORT_USHORT((PUSHORT) (io_addr + USBCMD), USBCMD_RS | USBCMD_CF | USBCMD_MAXP);

}

BOOLEAN
uhci_init_schedule(PUHCI_DEV uhci, PADAPTER_OBJECT padapter)
{
    int i, irq;

    uhci_dbg_print(DBGLVL_MAXIMUM, ("uhci_init_schedule(): entering..., uhci=0x%x\n", uhci));
    if (uhci == NULL || padapter == NULL)
        return FALSE;

    if (init_td_pool_list(&uhci->td_pool, padapter) == FALSE)
    {
        return FALSE;
    }
    if (init_qh_pool(&uhci->qh_pool, padapter) == FALSE)
    {
        return FALSE;
    }

    //since uhci is not started we can freely access all resources.
    for(i = 0; i < UHCI_MAX_SKELTDS; i++)
    {
        uhci->skel_td[i] = alloc_td(&uhci->td_pool);
        uhci_fill_td(uhci->skel_td[i], 0, (UHCI_NULL_DATA_SIZE << 21) | (0x7f << 8) | USB_PID_IN, 0);

        if (i > 0)
        {
            uhci->skel_td[i]->link = uhci->skel_td[i - 1]->phy_addr;
        }
    }

    /*for( i = UHCI_MAX_SKELTDS - 3; i >= 0; i-- )
       {
       InsertTailList( &uhci->skel_int256_td->ptde->hori_link,
       &uhci->skel_td[ i ]->ptde->hori_link );
       } */

    for(i = 0; i < UHCI_MAX_SKELQHS; i++)
    {
        uhci->skel_qh[i] = alloc_qh(&uhci->qh_pool);
        if (i > 0)
        {
            uhci->skel_qh[i - 1]->link = uhci->skel_qh[i]->phy_addr;
        }

        uhci->skel_qh[i]->element = UHCI_PTR_TERM;
    }

    uhci->skel_int1_td->link = uhci->skel_ls_control_qh->phy_addr;

    // Hack for PIIX
    uhci_fill_td(uhci->skel_term_td, 0, (UHCI_NULL_DATA_SIZE << 21) | (0x7f << 8) | USB_PID_IN, 0);
    uhci->skel_term_td->link = uhci->skel_term_td->phy_addr;

    uhci->skel_term_qh->link = UHCI_PTR_TERM;
    uhci->skel_term_qh->element = uhci->skel_term_td->phy_addr;

    InsertTailList(&uhci->skel_term_qh->pqhe->vert_link, &uhci->skel_term_td->ptde->vert_link);

    /*for( i = 0; i < UHCI_MAX_SKELQHS; i++ )
       {
       InsertTailList( &uhci->skel_int256_td->ptde->hori_link,
       &uhci->skel_qh[ i ]->pqhe->hori_link );
       } */

    if (uhci_init_frame_list(uhci, uhci->pdev_ext->padapter) == FALSE)
        uhci_destroy_frame_list(uhci);

    //well all have been chained, now scatter the int tds to frame-list
    //shamelessly pasted from linux's uhci.c :-)
    for(i = 0; i < UHCI_MAX_FRAMES; i++)
    {
        irq = 0;
        if (i & 1)
        {
            irq++;
            if (i & 2)
            {
                irq++;
                if (i & 4)
                {
                    irq++;
                    if (i & 8)
                    {
                        irq++;
                        if (i & 16)
                        {
                            irq++;
                            if (i & 32)
                            {
                                irq++;
                                if (i & 64)
                                    irq++;
                            }
                        }
                    }
                }
            }
        }

        /* Only place we don't use the frame list routines */
        uhci->frame_list[i] = uhci->skel_td[irq]->phy_addr;
    }
    return TRUE;
}

BOOLEAN
uhci_destroy_schedule(PUHCI_DEV uhci)
{
    BOOLEAN ret;

    ret = uhci_destroy_frame_list(uhci);
    ret = destroy_qh_pool(&uhci->qh_pool);
    ret = destroy_td_pool_list(&uhci->td_pool);

    return ret;

}

VOID NTAPI
uhci_cancel_pending_endp_urb(IN PVOID Parameter)
{
    PLIST_ENTRY abort_list;
    PUSB_DEV pdev;
    PURB purb;
    USE_BASIC_NON_PENDING_IRQL;

    abort_list = (PLIST_ENTRY) Parameter;

    if (abort_list == NULL)
        return;

    while (IsListEmpty(abort_list) == FALSE)
    {
        //these devs are protected by urb's ref-count
        purb = (PURB) RemoveHeadList(abort_list);
        pdev = purb->pdev;
        // purb->status is set when they are added to abort_list

        uhci_generic_urb_completion(purb, purb->context);

        lock_dev(pdev, FALSE);
        pdev->ref_count--;
        unlock_dev(pdev, FALSE);
    }
    usb_free_mem(abort_list);
    return;
}

BOOLEAN
uhci_process_pending_endp(PUHCI_DEV uhci)
{
    PUSB_DEV pdev;
    LIST_ENTRY temp_list, abort_list;
    PLIST_ENTRY pthis;
    PURB purb;
    PUSB_ENDPOINT pendp;
    NTSTATUS can_submit = STATUS_UNSUCCESSFUL;
    PWORK_QUEUE_ITEM pwork_item;
    PLIST_ENTRY cancel_list;
    USE_BASIC_IRQL;

    if (uhci == NULL)
        return FALSE;

    InitializeListHead(&temp_list);
    InitializeListHead(&abort_list);

    purb = NULL;
    uhci_dbg_print(DBGLVL_MEDIUM, ("uhci_process_pending_endp(): entering..., uhci=0x%x\n", uhci));

    lock_pending_endp_list(&uhci->pending_endp_list_lock);
    while (IsListEmpty(&uhci->pending_endp_list) == FALSE)
    {

        uhci_dbg_print(DBGLVL_MAXIMUM, ("uhci_process_pending_endp(): pending_endp_list=0x%x\n",
                                        &uhci->pending_endp_list));

        pthis = RemoveHeadList(&uhci->pending_endp_list);
        pendp = ((PUHCI_PENDING_ENDP) pthis)->pendp;
        pdev = dev_from_endp(pendp);

        lock_dev(pdev, TRUE);

        if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
        {
            unlock_dev(pdev, TRUE);
            free_pending_endp(&uhci->pending_endp_pool, struct_ptr(pthis, UHCI_PENDING_ENDP, endp_link));
            //delegate to uhci_remove_device for removing the urb queue on the endpoint
            continue;
        }

        if (endp_state(pendp) == USB_ENDP_FLAG_STALL)
        {
            while (IsListEmpty(&pendp->urb_list) == FALSE)
            {
                purb = (PURB) RemoveHeadList(&pendp->urb_list);
                purb->status = USB_STATUS_ENDPOINT_HALTED;
                InsertTailList(&abort_list, (LIST_ENTRY *) purb);
            }
            InitializeListHead(&pendp->urb_list);
            unlock_dev(pdev, TRUE);
            free_pending_endp(&uhci->pending_endp_pool, struct_ptr(pthis, UHCI_PENDING_ENDP, endp_link));
            continue;
        }


        if (IsListEmpty(&pendp->urb_list) == FALSE)
        {
            purb = (PURB) RemoveHeadList(&pendp->urb_list);
            ASSERT(purb);
        }
        else
        {
            InitializeListHead(&pendp->urb_list);
            unlock_dev(pdev, TRUE);
            free_pending_endp(&uhci->pending_endp_pool, struct_ptr(pthis, UHCI_PENDING_ENDP, endp_link));
            continue;
        }

        // if can_submit is STATUS_SUCCESS, the purb is inserted into the schedule
        uhci_dbg_print(DBGLVL_MAXIMUM, ("uhci_process_pending_endp(): endp_type=0x%x\n",
                                        endp_type(pendp)));
        switch (endp_type(pendp))
        {
            case USB_ENDPOINT_XFER_BULK:
            {
#ifdef DEMO
                can_submit = STATUS_UNSUCCESSFUL;
#else
                can_submit = uhci_internal_submit_bulk(uhci, purb);
#endif
                break;
            }
            case USB_ENDPOINT_XFER_CONTROL:
            {
                can_submit = uhci_internal_submit_ctrl(uhci, purb);
                break;
            }
            case USB_ENDPOINT_XFER_INT:
            {
                can_submit = uhci_internal_submit_int(uhci, purb);
                break;
            }
            case USB_ENDPOINT_XFER_ISOC:
            {
                can_submit = uhci_internal_submit_iso(uhci, purb);
                break;
            }
        }

        if (can_submit == STATUS_NO_MORE_ENTRIES)
        {
            //no enough bandwidth or tds
            InsertHeadList(&pendp->urb_list, (PLIST_ENTRY) purb);
            InsertTailList(&temp_list, pthis);
        }
        else
        {
            // other error or success
            free_pending_endp(&uhci->pending_endp_pool, struct_ptr(pthis, UHCI_PENDING_ENDP, endp_link));

            if (can_submit != STATUS_SUCCESS)
            {
                //abort these URBs
                InsertTailList(&abort_list, (LIST_ENTRY *) purb);
                uhci_dbg_print(DBGLVL_MEDIUM, ("uhci_process_pending_endp(): unable to submit urb 0x%x, "
                    "with status=0x%x\n", purb, can_submit));
                purb->status = can_submit;
            }

        }
        unlock_dev(pdev, TRUE);
    }

    if (IsListEmpty(&temp_list) == FALSE)
    {
        //re-append them to the pending_endp_list
        ListFirst(&temp_list, pthis);
        RemoveEntryList(&temp_list);
        MergeList(&uhci->pending_endp_list, pthis);
    }
    unlock_pending_endp_list(&uhci->pending_endp_list_lock);

    if (IsListEmpty(&abort_list) == FALSE)
    {
        PLIST_ENTRY pthis;
        cancel_list = (PLIST_ENTRY) usb_alloc_mem(NonPagedPool, sizeof(WORK_QUEUE_ITEM) + sizeof(LIST_ENTRY));
        ASSERT(cancel_list);

        ListFirst(&abort_list, pthis);
        RemoveEntryList(&abort_list);
        InsertTailList(pthis, cancel_list);

        pwork_item = (PWORK_QUEUE_ITEM) & cancel_list[1];

        // we do not need to worry the uhci_cancel_pending_endp_urb running when the
        // driver is unloading since it will prevent the dev_mgr to quit till all the
        // reference count to the dev drop to zero.
        ExInitializeWorkItem(pwork_item, uhci_cancel_pending_endp_urb, (PVOID) cancel_list);
        ExQueueWorkItem(pwork_item, DelayedWorkQueue);
    }
    return TRUE;
}

NTSTATUS
uhci_submit_urb(PUHCI_DEV uhci, PUSB_DEV pdev, PUSB_ENDPOINT pendp, PURB purb)
{
    int i;
    PUHCI_PENDING_ENDP pending_endp;
    NTSTATUS status;
    USE_BASIC_IRQL;

    if (uhci == NULL || pdev == NULL || pendp == NULL || purb == NULL)
    {
        uhci_dbg_print(DBGLVL_MEDIUM,
                   ("uhci_submit_urb(): uhci=0x%x, pdev=0x%x, pendp=0x%x, purb=0x%x "
                   "called with invalid param!\n", uhci, pdev, pendp, purb));
        return STATUS_INVALID_PARAMETER;
    }

    lock_pending_endp_list(&uhci->pending_endp_list_lock);
    lock_dev(pdev, TRUE);

    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        status = purb->status = STATUS_DEVICE_DOES_NOT_EXIST;
        goto LBL_OUT;
    }

    if (dev_class(pdev) == USB_DEV_CLASS_ROOT_HUB)
    {
        unlock_dev(pdev, TRUE);
        unlock_pending_endp_list(&uhci->pending_endp_list_lock);
        status = uhci_rh_submit_urb(pdev, purb);
        return status;
    }

    if (pendp)
        purb->pendp = pendp;
    else
        purb->pendp = &pdev->default_endp;

    if (dev_from_endp(purb->pendp) != pdev)
    {
        uhci_dbg_print(DBGLVL_MEDIUM,
                   ("uhci_submit_urb(): dev_from_endp=0x%x\n, pdev=0x%x, pendp=0x%x "
                   "devices mismatch!\n", dev_from_endp(purb->pendp), pdev, pendp));

        status = purb->status = STATUS_INVALID_PARAMETER;
        goto LBL_OUT;
    }

    if (endp_state(purb->pendp) == USB_ENDP_FLAG_STALL)
    {
        status = purb->status = USB_STATUS_ENDPOINT_HALTED;
        goto LBL_OUT;
    }

    purb->pdev = pdev;
    purb->rest_bytes = purb->data_length;

    if (endp_type(purb->pendp) == USB_ENDPOINT_XFER_BULK)
        purb->bytes_to_transfer = (purb->data_length > purb->pendp->pusb_endp_desc->wMaxPacketSize * UHCI_MAX_TDS_PER_TRANSFER ? purb->pendp->pusb_endp_desc->wMaxPacketSize * UHCI_MAX_TDS_PER_TRANSFER : purb->data_length);  //multiple transfer for large data block
    else
        purb->bytes_to_transfer = purb->data_length;

    uhci_dbg_print(DBGLVL_MEDIUM, ("uhci_submit_urb(): bytes_to_transfer=0x%x\n", purb->bytes_to_transfer));

    purb->bytes_transfered = 0;
    InitializeListHead(&purb->trasac_list);
    purb->last_finished_td = &purb->trasac_list;
    purb->flags &= ~(URB_FLAG_STATE_MASK | URB_FLAG_IN_SCHEDULE | URB_FLAG_FORCE_CANCEL);
    purb->flags |= URB_FLAG_STATE_PENDING;


    i = IsListEmpty(&pendp->urb_list);
    InsertTailList(&pendp->urb_list, &purb->urb_link);

    pdev->ref_count++;          //for urb reference

    if (i == FALSE)
    {
        //there is urb pending, simply queue it and return
        status = purb->status = STATUS_PENDING;
        goto LBL_OUT;
    }
    else if (usb_endp_busy_count(purb->pendp) && endp_type(purb->pendp) != USB_ENDPOINT_XFER_ISOC)
    {
        //
        //No urb waiting but urb overlap not allowed,
        //so leave it in queue and return, will be scheduled
        //later
        //
        status = purb->status = STATUS_PENDING;
        goto LBL_OUT;
    }

    pending_endp = alloc_pending_endp(&uhci->pending_endp_pool, 1);
    if (pending_endp == NULL)
    {
        //panic
        status = purb->status = STATUS_UNSUCCESSFUL;
        goto LBL_OUT2;
    }

    pending_endp->pendp = purb->pendp;
    InsertTailList(&uhci->pending_endp_list, (PLIST_ENTRY) pending_endp);

    unlock_dev(pdev, TRUE);
    unlock_pending_endp_list(&uhci->pending_endp_list_lock);

    uhci_process_pending_endp(uhci);
    return STATUS_PENDING;

LBL_OUT2:
    pdev->ref_count--;
    RemoveEntryList((PLIST_ENTRY) purb);

LBL_OUT:
    unlock_dev(pdev, TRUE);
    unlock_pending_endp_list(&uhci->pending_endp_list_lock);
    return status;
}

NTSTATUS
uhci_set_error_code(PURB urb, ULONG raw_status)
{
    if ((raw_status & TD_CTRL_ANY_ERROR) == 0)
    {
        //test if the urb is canceled
        if (urb->flags & URB_FLAG_FORCE_CANCEL)
            urb->status = STATUS_CANCELLED;
        else
            urb->status = STATUS_SUCCESS;
    }

    else if (raw_status & TD_CTRL_BABBLE)
        urb->status = USB_STATUS_DATA_OVERRUN;

    else if (raw_status & TD_CTRL_STALLED)
        urb->status = USB_STATUS_STALL_PID;

    else if (raw_status & TD_CTRL_DBUFERR)
        urb->status = USB_STATUS_BUFFER_OVERRUN;

    else if (raw_status & TD_CTRL_CRCTIMEO)
        urb->status = USB_STATUS_CRC;

    else if (raw_status & TD_CTRL_BITSTUFF)
        urb->status = USB_STATUS_BTSTUFF;

    else
        urb->status = STATUS_UNSUCCESSFUL;

    return urb->status;
}

BOOLEAN NTAPI
uhci_sync_remove_urb_finished(PVOID context)
{
    PUHCI_DEV uhci;
    PLIST_ENTRY pthis, pnext, ptemp;
    PURB purb;
    PSYNC_PARAM pparam;

    pparam = (PSYNC_PARAM) context;
    uhci = pparam->uhci;
    ptemp = (PLIST_ENTRY) pparam->context;

    if (uhci == NULL)
    {
        return (UCHAR) (pparam->ret = FALSE);
    }

    ListFirst(&uhci->urb_list, pthis);
    while (pthis)
    {
        //remove urbs not in the schedule
        ListNext(&uhci->urb_list, pthis, pnext);
        purb = (PURB) pthis;

        if ((purb->flags & URB_FLAG_IN_SCHEDULE) == 0)
        {
            //finished or canceled( not apply for split bulk ).
            purb->flags &= ~URB_FLAG_STATE_MASK;
            purb->flags |= URB_FLAG_STATE_FINISHED;
            RemoveEntryList(pthis);
            InsertTailList(ptemp, pthis);
        }
        pthis = pnext;
    }
    pparam->ret = TRUE;
    return (UCHAR) TRUE;
}

BOOLEAN
uhci_drop_fsbr(PUHCI_DEV uhci)
{
    if (uhci == NULL)
        return (UCHAR) FALSE;

    uhci->fsbr_cnt--;

    if (uhci->fsbr_cnt <= 0)
    {
        uhci->skel_term_qh->link = UHCI_PTR_TERM;
        uhci->fsbr_cnt = 0;
    }

    return (UCHAR) TRUE;
}

VOID NTAPI
uhci_dpc_callback(PKDPC dpc, PVOID context, PVOID sysarg1, PVOID sysarg2)
{
    PUHCI_DEV uhci;

    LIST_HEAD temp_list;
    PLIST_ENTRY pthis, pnext;
    PURB purb;
    PQH_EXTENSION pqhe;
    PUHCI_PENDING_ENDP pending_endp;
    PUSB_DEV pdev;
    PUSB_ENDPOINT pendp;

    BOOLEAN finished;
    LONG i, j;
    ULONG uhci_status, urb_status, toggle = 0;

    SYNC_PARAM sync_param;
    USE_BASIC_NON_PENDING_IRQL;

    UNREFERENCED_PARAMETER(dpc);
    UNREFERENCED_PARAMETER(sysarg2);

    uhci = (PUHCI_DEV) context;
    if (uhci == NULL)
        return;

    uhci_status = (ULONG) sysarg1;

    InitializeListHead(&temp_list);

    sync_param.uhci = uhci;
    sync_param.context = (PVOID) & temp_list;

    uhci_dbg_print(DBGLVL_MAXIMUM, ("uhci_dpc_callback(): entering..., uhci=0x%x\n", uhci));
    //remove finished urb from uhci's urb-list
    KeSynchronizeExecution(uhci->pdev_ext->uhci_int, uhci_sync_remove_urb_finished, &sync_param);

    //release resources( tds, and qhs ) the urb occupied
    while (IsListEmpty(&temp_list) == FALSE)
    {
        //not in any public queue, if do not access into dev, no race
        //condition will occur
        purb = (PURB) RemoveHeadList(&temp_list);
        urb_status = purb->status;

        //the only place we do not use this lock on non-pending-endp-list data ops
        KeAcquireSpinLockAtDpcLevel(&uhci->pending_endp_list_lock);
        while (IsListEmpty(&purb->trasac_list) == FALSE)
        {
            pthis = RemoveHeadList(&purb->trasac_list);

            if ((((PTD_EXTENSION) pthis)->flags & UHCI_ITEM_FLAG_TYPE) == UHCI_ITEM_FLAG_QH)
            {
                pqhe = (PQH_EXTENSION) pthis;
                lock_qh_pool(&uhci->qh_pool, TRUE);
                free_qh(&uhci->qh_pool, pqhe->pqh);
                unlock_qh_pool(&uhci->qh_pool, TRUE);
            }
            else
            {
                //must be a td chain
                InsertHeadList(&purb->trasac_list, pthis);
                for(i = 0, purb->bytes_transfered = 0; i < purb->td_count; i++)
                {
                    PUHCI_TD ptd;
                    // accumulate data transfered in tds
                    ptd = ((PTD_EXTENSION) pthis)->ptd;
                    if ((ptd->status & TD_CTRL_ACTIVE) == 0 && (ptd->status & TD_CTRL_ANY_ERROR) == 0)
                    {
                        j = ptd->status & 0x7ff;
                        purb->bytes_transfered += ((j == 0x7ff) ? 0 : (j + 1));

                    }
                    ListNext(&purb->trasac_list, pthis, pnext);
                    pthis = pnext;
                }

                if (urb_status & TD_CTRL_ANY_ERROR)
                {
                    if (purb->last_finished_td != NULL && purb->last_finished_td != &purb->trasac_list)
                        toggle = (((PTD_EXTENSION) purb->last_finished_td)->ptd->info & (1 << 19));
                }
                //trick, remove trasac_list
                ListFirst(&purb->trasac_list, pthis);
                RemoveEntryList(&purb->trasac_list);
                lock_td_pool(&uhci->td_pool, TRUE);
                free_tds(&uhci->td_pool, ((PTD_EXTENSION) pthis)->ptd);
                unlock_td_pool(&uhci->td_pool, TRUE);
                //termination condition
                InitializeListHead(&purb->trasac_list);
                purb->last_finished_td = NULL;
            }
        }

        if (endp_type(purb->pendp) == USB_ENDPOINT_XFER_ISOC
            || endp_type(purb->pendp) == USB_ENDPOINT_XFER_INT)
            uhci_claim_bandwidth(uhci, purb, FALSE);    //release band-width

        KeReleaseSpinLockFromDpcLevel(&uhci->pending_endp_list_lock);

        uhci_set_error_code(purb, urb_status);

        finished = TRUE;

        //since the ref_count for the urb is not released, we can safely have one
        //pointer to dev
        pdev = dev_from_endp(purb->pendp);
        pendp = purb->pendp;

        if (purb->status == USB_STATUS_BABBLE_DETECTED)
        {
            usb_dbg_print(DBGLVL_MEDIUM,
                          ("uhci_dpc_callback(): alert!!!, babble detected, severe error, reset the whole bus\n"));
            uhci_reset(uhci);
            uhci_start(&uhci->hcd_interf);
        }

        //this will let the new request in uhci_generic_urb_completion to this endp
        //be processed rather than queued in the pending_endp_list
        lock_dev(pdev, TRUE);
        usb_endp_busy_count_dec(pendp);
        unlock_dev(pdev, TRUE);

        if (usb_success(purb->status) == FALSE)
        {
            // set error code and complete the urb and purb is invalid from this point
            uhci_generic_urb_completion(purb, purb->context);
        }
        else
        {
            if ((purb->pipe & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)
            {
                purb->rest_bytes -= purb->bytes_transfered;
                if (purb->rest_bytes)
                {
                    finished = FALSE;
                }
                else
                {
                    uhci_generic_urb_completion(purb, purb->context);
                }
            }
            else
            {
                uhci_generic_urb_completion(purb, purb->context);
                //purb is now invalid
            }
        }

        KeAcquireSpinLockAtDpcLevel(&uhci->pending_endp_list_lock);
        lock_dev(pdev, TRUE);

        if (finished)
            pdev->ref_count--;

        if (urb_status & TD_CTRL_ANY_ERROR && endp_type(pendp) != USB_ENDPOINT_XFER_CONTROL)
        {
            pendp->flags &= ~USB_ENDP_FLAG_STAT_MASK;
            pendp->flags |= USB_ENDP_FLAG_STALL;
        }

        if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
        {
            unlock_dev(pdev, TRUE);
            KeReleaseSpinLockFromDpcLevel(&uhci->pending_endp_list_lock);
            if (finished == FALSE)
            {

                purb->status = STATUS_DEVICE_DOES_NOT_EXIST;
                uhci_generic_urb_completion(purb, purb->context);

                lock_dev(pdev, TRUE);
                pdev->ref_count--;
                unlock_dev(pdev, TRUE);
            }
            continue;
        }

        if (finished && IsListEmpty(&pendp->urb_list) == TRUE)
        {
            unlock_dev(pdev, TRUE);
            KeReleaseSpinLockFromDpcLevel(&uhci->pending_endp_list_lock);
            continue;
        }
        else if (finished == TRUE)
        {
            //has urb in the endp's urb-list
            if (usb_endp_busy_count(pendp) > 0)
            {
                //the urbs still have chance to be sheduled but not this time
                unlock_dev(pdev, TRUE);
                KeReleaseSpinLockFromDpcLevel(&uhci->pending_endp_list_lock);
                continue;
            }
        }

        if (finished == FALSE)
        {
            //a split bulk transfer
            purb->bytes_transfered = 0;
            purb->bytes_to_transfer =
                UHCI_MAX_TDS_PER_TRANSFER * purb->pendp->pusb_endp_desc->wMaxPacketSize
                > purb->rest_bytes
                ? purb->rest_bytes : UHCI_MAX_TDS_PER_TRANSFER * purb->pendp->pusb_endp_desc->wMaxPacketSize;

            //the urb is not finished
            purb->flags &= ~URB_FLAG_STATE_MASK;
            purb->flags |= URB_FLAG_STATE_PENDING;

            InsertHeadList(&pendp->urb_list, (PLIST_ENTRY) purb);
        }

        pending_endp = alloc_pending_endp(&uhci->pending_endp_pool, 1);
        pending_endp->pendp = pendp;
        InsertTailList(&uhci->pending_endp_list, &pending_endp->endp_link);

        unlock_dev(pdev, TRUE);
        KeReleaseSpinLockFromDpcLevel(&uhci->pending_endp_list_lock);
    }

    //ah...exhausted, let's find some in the pending_endp_list to rock
    uhci_process_pending_endp(uhci);
    return;
}

BOOLEAN
uhci_add_device(PUHCI_DEV uhci, PUSB_DEV dev)
{
    if (dev == NULL || uhci == NULL)
        return FALSE;

    return TRUE;
}

BOOLEAN NTAPI
uhci_sync_cancel_urbs_dev(PVOID context)
{
    //cancel all the urbs on one dev
    PUHCI_DEV uhci;
    PUSB_DEV pdev, dest_dev;
    PSYNC_PARAM sync_param;
    PLIST_ENTRY pthis, pnext;
    LONG count;

    sync_param = (PSYNC_PARAM) context;
    dest_dev = (PUSB_DEV) sync_param->context;
    uhci = sync_param->uhci;

    if (uhci == NULL || dest_dev == NULL)
    {
        return (UCHAR) (sync_param->ret = FALSE);
    }
    count = 0;
    ListFirst(&uhci->urb_list, pthis);
    while (pthis)
    {
        pdev = dev_from_endp(((PURB) pthis)->pendp);
        if (pdev == dest_dev)
        {
            ((PURB) pthis)->flags |= URB_FLAG_FORCE_CANCEL;
        }
        ListNext(&uhci->urb_list, pthis, pnext);
        pthis = pnext;
        count++;
    }
    if (count)
        uhci->skel_term_td->status |= TD_CTRL_IOC;

    return (UCHAR) (sync_param->ret = TRUE);
}

BOOLEAN
uhci_remove_device(PUHCI_DEV uhci, PUSB_DEV dev)
{
    PUHCI_PENDING_ENDP ppending_endp;
    PLIST_ENTRY pthis, pnext;
    PURB purb;
    LIST_HEAD temp_list;
    int i, j, k;
    SYNC_PARAM sync_param;

    USE_BASIC_IRQL;

    if (uhci == NULL || dev == NULL)
        return FALSE;

    InitializeListHead(&temp_list);

    //free pending endp that has urb queued from pending endp list
    lock_pending_endp_list(&uhci->pending_endp_list_lock);

    ListFirst(&uhci->pending_endp_list, pthis);

    while (pthis)
    {
        ppending_endp = (PUHCI_PENDING_ENDP) pthis;
        ListNext(&uhci->pending_endp_list, pthis, pnext);
        if (dev_from_endp(ppending_endp->pendp) == dev)
        {
            RemoveEntryList(pthis);
            free_pending_endp(&uhci->pending_endp_pool, struct_ptr(pthis, UHCI_PENDING_ENDP, endp_link));
        }
        pthis = pnext;
    }
    unlock_pending_endp_list(&uhci->pending_endp_list_lock);

    //cancel all the urbs in the urb-list
    sync_param.uhci = uhci;
    sync_param.context = (PVOID) dev;

    KeSynchronizeExecution(uhci->pdev_ext->uhci_int, uhci_sync_cancel_urbs_dev, &sync_param);

    //cancel all the urb in the endp's urb-list
    k = 0;
    lock_dev(dev, FALSE);
    if (dev->usb_config)
    {
        //only for configed dev
        for(i = 0; i < dev->usb_config->if_count; i++)
        {
            for(j = 0; j < dev->usb_config->interf[i].endp_count; j++)
            {
                ListFirst(&dev->usb_config->interf[i].endp[j].urb_list, pthis);
                while (pthis)
                {
                    ListNext(&dev->usb_config->interf[i].endp[j].urb_list, pthis, pnext);

                    RemoveEntryList(pthis);
                    InsertHeadList(&temp_list, pthis);
                    pthis = pnext;
                    k++;
                }

            }
        }
    }
    ListFirst(&dev->default_endp.urb_list, pthis);

    while (pthis)
    {
        ListNext(&dev->default_endp.urb_list, pthis, pnext);

        RemoveEntryList(pthis);
        InsertHeadList(&temp_list, pthis);
        pthis = pnext;
        k++;
    }
    unlock_dev(dev, FALSE);

    if (IsListEmpty(&temp_list) == FALSE)
    {
        for(i = 0; i < k; i++)
        {
            //complete those urbs with error
            pthis = RemoveHeadList(&temp_list);
            purb = (PURB) pthis;
            purb->status = STATUS_DEVICE_DOES_NOT_EXIST;
            {
                uhci_generic_urb_completion(purb, purb->context);
            }
        }
    }

    lock_dev(dev, FALSE) dev->ref_count -= k;
    unlock_dev(dev, FALSE);

    return TRUE;
}


//
// assume that the urb has its rest_bytes and bytes_to_transfer set
// and bytes_transfered is zeroed.
// dev_lock must be acquired outside
// urb comes from dev's endpoint urb-list. it is already removed from
// the endpoint urb-list.
//
NTSTATUS
uhci_internal_submit_bulk(PUHCI_DEV uhci, PURB urb)
{

    LONG max_packet_size, td_count, offset, bytes_to_transfer, data_load;
    PBYTE start_addr;
    PUHCI_TD ptd;
    PUHCI_QH pqh;
    LIST_ENTRY td_list, *pthis, *pnext;
    BOOLEAN old_toggle, toggle, ret;
    UCHAR pid;

    if (uhci == NULL || urb == NULL)
        return STATUS_INVALID_PARAMETER;

    max_packet_size = endp_max_packet_size(urb->pendp);
    if (urb->bytes_to_transfer == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    td_count = (urb->bytes_to_transfer + max_packet_size - 1) / max_packet_size;

    lock_td_pool(&uhci->td_pool, TRUE);
    if (can_transfer(&uhci->td_pool, td_count) == FALSE)
    {
        unlock_td_pool(&uhci->td_pool, TRUE);
        return STATUS_NO_MORE_ENTRIES;
    }

    ptd = alloc_tds(&uhci->td_pool, td_count);
    unlock_td_pool(&uhci->td_pool, TRUE);

    if (ptd == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    InitializeListHead(&td_list);
    InsertTailList(&ptd->ptde->vert_link, &td_list);

    ListFirst(&td_list, pthis);
    ListNext(&td_list, pthis, pnext);

    start_addr = &urb->data_buffer[urb->data_length - urb->rest_bytes];
    offset = 0;

    old_toggle = toggle = urb->pendp->flags & USB_ENDP_FLAG_DATATOGGLE ? TRUE : FALSE;
    bytes_to_transfer = urb->bytes_to_transfer;

    urb->pipe = ((max_packet_size - 1) << 21)
        | ((ULONG) endp_num(urb->pendp) << 15)
        | (dev_from_endp(urb->pendp)->dev_addr << 8)
        | ((ULONG) endp_dir(urb->pendp)) | USB_ENDPOINT_XFER_BULK;

    pid = (((ULONG) urb->pendp->pusb_endp_desc->bEndpointAddress & USB_DIR_IN) ? USB_PID_IN : USB_PID_OUT);
    while (pthis)
    {
        ptd = ((PTD_EXTENSION) pthis)->ptd;

        data_load = max_packet_size < bytes_to_transfer ? max_packet_size : bytes_to_transfer;
        ptd->purb = urb;
        uhci_fill_td(ptd,
                     (3 << TD_CTRL_C_ERR_SHIFT)
                     | (TD_CTRL_ACTIVE),
                     ((data_load - 1) << 21)
                     | (toggle << 19)
                     | ((ULONG) endp_num(urb->pendp) << 15)
                     | (dev_from_endp(urb->pendp)->dev_addr << 8)
                     | pid, MmGetPhysicalAddress(start_addr + offset).LowPart);

        bytes_to_transfer -= data_load;
        offset += data_load;

        if (pnext)
        {
            ptd->link = ((PTD_EXTENSION) pnext)->ptd->phy_addr;
        }
        else
        {
            //Last one, enable ioc and short packet detect if necessary
            ptd->link = UHCI_PTR_TERM;
            ptd->status |= TD_CTRL_IOC;
            if (bytes_to_transfer < max_packet_size && (pid == USB_PID_IN))
            {
                //ptd->status |= TD_CTRL_SPD;
            }
        }

        pthis = pnext;
        toggle ^= 1;
        if (pthis)
            ListNext(&td_list, pthis, pnext);

    }

    ListFirst(&td_list, pthis);
    RemoveEntryList(&td_list);

    lock_qh_pool(&uhci->qh_pool, TRUE);
    pqh = alloc_qh(&uhci->qh_pool);
    unlock_qh_pool(&uhci->qh_pool, TRUE);

    if (pqh == NULL)
    {
        lock_td_pool(&uhci->td_pool, TRUE);

        if (pthis)
            free_tds(&uhci->td_pool, ((PTD_EXTENSION) pthis)->ptd);

        unlock_td_pool(&uhci->td_pool, TRUE);
        return STATUS_NO_MORE_ENTRIES;

    }

    urb->td_count = td_count;

    uhci_insert_tds_qh(pqh, ((PTD_EXTENSION) pthis)->ptd);
    uhci_insert_qh_urb(urb, pqh);
    urb->pendp->flags =
        (urb->pendp->flags & ~USB_ENDP_FLAG_DATATOGGLE) | (toggle ? USB_ENDP_FLAG_DATATOGGLE : 0);
    usb_endp_busy_count_inc(urb->pendp);
    uhci_insert_urb_to_schedule(uhci, urb, ret);

    if (ret == FALSE)
    {
        // undo all we have done
        RemoveEntryList(&pqh->pqhe->vert_link); //remove qh from td_chain
        RemoveEntryList(&urb->trasac_list);

        lock_td_pool(&uhci->td_pool, TRUE);
        if (pthis)
            free_tds(&uhci->td_pool, ((PTD_EXTENSION) pthis)->ptd);
        unlock_td_pool(&uhci->td_pool, TRUE);

        lock_qh_pool(&uhci->qh_pool, TRUE);
        if (pqh)
            free_qh(&uhci->qh_pool, pqh);
        unlock_qh_pool(&uhci->qh_pool, TRUE);

        InitializeListHead(&urb->trasac_list);
        usb_endp_busy_count_dec(urb->pendp);
        urb->pendp->flags =
            (urb->pendp->flags & ~USB_ENDP_FLAG_DATATOGGLE) | (old_toggle ? USB_ENDP_FLAG_DATATOGGLE : 0);
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
uhci_internal_submit_ctrl(PUHCI_DEV uhci, PURB urb)
{
    LIST_ENTRY td_list, *pthis, *pnext;
    LONG i, td_count;
    LONG toggle;
    LONG max_packet_size, bytes_to_transfer, bytes_rest, start_idx;
    PUHCI_TD ptd;
    PUHCI_QH pqh;
    ULONG dev_addr;
    PUSB_DEV pdev;
    BOOLEAN ret;

    if (uhci == NULL || urb == NULL)
        return STATUS_INVALID_PARAMETER;

    toggle = 0;
    bytes_rest = urb->rest_bytes;
    bytes_to_transfer = urb->bytes_to_transfer;
    max_packet_size = endp_max_packet_size(urb->pendp);
    start_idx = urb->data_length - urb->rest_bytes;
    td_count = 2 + (urb->bytes_to_transfer + max_packet_size - 1) / max_packet_size;

    lock_td_pool(&uhci->td_pool, TRUE);

    if (can_transfer(&uhci->td_pool, td_count) == FALSE)
    {
        unlock_td_pool(&uhci->td_pool, TRUE);
        return STATUS_NO_MORE_ENTRIES;
    }

    ptd = alloc_tds(&uhci->td_pool, td_count);
    unlock_td_pool(&uhci->td_pool, TRUE);

    if (ptd == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    InsertTailList(&ptd->ptde->vert_link, &td_list);

    ListFirst(&td_list, pthis);
    ListNext(&td_list, pthis, pnext);

    ptd = ((PTD_EXTENSION) pthis)->ptd;

    pdev = dev_from_endp(urb->pendp);
    dev_addr = pdev->dev_addr;

    if (dev_state(pdev) <= USB_DEV_STATE_RESET)
        dev_addr = 0;

    usb_dbg_print(DBGLVL_MAXIMUM, ("uhci_internal_submit_ctrl(): dev_addr =0x%x\n", dev_addr));

    RtlCopyMemory(uhci->io_buf, urb->setup_packet, 8);

    if ((urb->setup_packet[0] & USB_DIR_IN) == 0)       //out
        RtlCopyMemory(&uhci->io_buf[8], urb->data_buffer, bytes_to_transfer);
    else
        RtlZeroMemory(&uhci->io_buf[8], bytes_to_transfer);

    uhci_fill_td(ptd,
                 (3 << TD_CTRL_C_ERR_SHIFT) | (TD_CTRL_ACTIVE),
                 (7 << 21) | (((ULONG) endp_num(urb->pendp)) << 15) | (dev_addr << 8) | (USB_PID_SETUP),
                 //uhci->io_buf_logic_addr.LowPart);
                 MmGetPhysicalAddress(urb->setup_packet).LowPart);

    ptd->link = ((PTD_EXTENSION) pnext)->ptd->phy_addr;
    pthis = pnext;
    ListNext(&td_list, pthis, pnext);

    urb->pipe = ((max_packet_size - 1) << 21)
        | ((ULONG) endp_num(urb->pendp) << 15)
        | (dev_addr << 8) | (pdev->flags & USB_DEV_FLAG_LOW_SPEED) | USB_ENDPOINT_XFER_CONTROL;

    for(i = 0, toggle = 1; ((i < td_count - 2) && pthis); i++, toggle ^= 1)
    {
        //construct tds for DATA packets of data stage.
        ptd = ((PTD_EXTENSION) pthis)->ptd;
        uhci_fill_td(ptd,
                     (3 << TD_CTRL_C_ERR_SHIFT)
                     | (TD_CTRL_ACTIVE),
                     ((bytes_to_transfer >
                       max_packet_size ? max_packet_size - 1 : bytes_to_transfer -
                       1) << 21) | (toggle << 19) | (((ULONG) endp_num(urb->
                                                                       pendp)) << 15) | (dev_addr << 8) |
                     ((urb->setup_packet[0] & USB_DIR_IN) ? USB_PID_IN : USB_PID_OUT),
                     //uhci->io_buf_logic_addr.LowPart + 8 + i * max_packet_size );
                     MmGetPhysicalAddress(&urb->data_buffer[start_idx + i * max_packet_size]).LowPart);

        if (pnext)
            ptd->link = ((PTD_EXTENSION) pnext)->ptd->phy_addr;

        if (i < td_count - 3)
        {
            bytes_to_transfer -= max_packet_size;
        }
        else
        {
            if (bytes_to_transfer > 0)
            {
                if (bytes_to_transfer < max_packet_size && (urb->setup_packet[0] & USB_DIR_IN))
                    ptd->status |= TD_CTRL_SPD;
            }
        }
        pthis = pnext;

        if (pthis)
            ListNext(&td_list, pthis, pnext);
    }

    if (pnext)
        ptd->link = ((PTD_EXTENSION) pnext)->ptd->phy_addr;

    ListFirstPrev(&td_list, pthis);
    ptd = ((PTD_EXTENSION) pthis)->ptd;

    //the last is an IN transaction
    uhci_fill_td(ptd,
                 (3 << TD_CTRL_C_ERR_SHIFT)
                 | (TD_CTRL_ACTIVE | TD_CTRL_IOC),
                 (UHCI_NULL_DATA_SIZE << 21)
                 | (1 << 19)
                 | (((ULONG) endp_num(urb->pendp)) << 15)
                 | (dev_addr << 8)
                 | ((td_count > 2)
                    ? ((urb->setup_packet[0] & USB_DIR_IN) ? USB_PID_OUT : USB_PID_IN) : USB_PID_IN), 0);

    ptd->link = UHCI_PTR_TERM;

    ListFirst(&td_list, pthis);
    RemoveEntryList(&td_list);

    lock_qh_pool(&uhci->qh_pool, TRUE);
    pqh = alloc_qh(&uhci->qh_pool);
    unlock_qh_pool(&uhci->qh_pool, TRUE);

    if (pqh == NULL)
    {
        lock_td_pool(&uhci->td_pool, TRUE);
        if (pthis)
            free_tds(&uhci->td_pool, ((PTD_EXTENSION) pthis)->ptd);
        unlock_td_pool(&uhci->td_pool, TRUE);

        return STATUS_NO_MORE_ENTRIES;
    }

    urb->td_count = td_count;

    uhci_insert_tds_qh(pqh, ((PTD_EXTENSION) pthis)->ptd);
    uhci_insert_qh_urb(urb, pqh);

    usb_endp_busy_count_inc(urb->pendp);
    uhci_insert_urb_to_schedule(uhci, urb, ret);
    if (ret == FALSE)
    {
        RemoveEntryList(&pqh->pqhe->vert_link);
        RemoveEntryList(&urb->trasac_list);

        lock_td_pool(&uhci->td_pool, TRUE);
        if (pthis)
            free_tds(&uhci->td_pool, ((PTD_EXTENSION) pthis)->ptd);
        unlock_td_pool(&uhci->td_pool, TRUE);

        lock_qh_pool(&uhci->qh_pool, TRUE);
        if (pqh)
            free_qh(&uhci->qh_pool, pqh);
        unlock_qh_pool(&uhci->qh_pool, TRUE);

        InitializeListHead(&urb->trasac_list);
        usb_endp_busy_count_dec(urb->pendp);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
uhci_internal_submit_int(PUHCI_DEV uhci, PURB urb)
{
    LONG i;
    LONG toggle = 0;
    LONG max_packet_size;
    PUHCI_TD ptd;
    BOOLEAN ret;

    if (uhci == NULL || urb == NULL)
    {
        uhci_dbg_print(DBGLVL_MEDIUM,
                   ("uhci_internal_submit_int(): uhci=0x%x, urb=0x%x "
                    "returning STATUS_INVALID_PARAMETER!\n", uhci, urb));
        return STATUS_INVALID_PARAMETER;
    }

    toggle = (urb->pendp->flags & USB_ENDP_FLAG_DATATOGGLE) ? TRUE : FALSE;
    max_packet_size = endp_max_packet_size(urb->pendp);

    if (max_packet_size < urb->data_length || max_packet_size == 0 || max_packet_size > 64)
    {
        uhci_dbg_print(DBGLVL_MEDIUM,
                   ("uhci_internal_submit_int(): max_packet_size=%d, urb->data_length=%d "
                    "returning STATUS_INVALID_PARAMETER!\n", max_packet_size, urb->data_length));
        return STATUS_INVALID_PARAMETER;
    }

    lock_td_pool(&uhci->td_pool, TRUE);
    ptd = alloc_td(&uhci->td_pool);
    unlock_td_pool(&uhci->td_pool, TRUE);

    if (ptd == NULL)
        return STATUS_NO_MORE_ENTRIES;

    for(i = 1; i <= 7; i++)
    {
        if (((ULONG) max_packet_size) >> i)
            continue;
        else
            break;
    }

    i--;
    i &= 7;

    urb->pipe = (((ULONG) urb->pendp->pusb_endp_desc->bInterval) << 24)
        | (i << 21)
        | (toggle << 19)
        | ((ULONG) endp_num(urb->pendp) << 15)
        | (((ULONG) dev_from_endp(urb->pendp)->dev_addr) << 8)
        | USB_DIR_IN | (dev_from_endp(urb->pendp)->flags & USB_DEV_FLAG_LOW_SPEED) | USB_ENDPOINT_XFER_INT;

    uhci_fill_td(ptd,
                 (3 << TD_CTRL_C_ERR_SHIFT)
                 | (TD_CTRL_ACTIVE)
                 | ((urb->data_length < max_packet_size ? TD_CTRL_SPD : 0))
                 | (TD_CTRL_IOC),
                 (((ULONG) max_packet_size - 1) << 21)
                 | (toggle << 19)
                 | ((ULONG) endp_num(urb->pendp) << 15)
                 | (((ULONG) dev_from_endp(urb->pendp)->dev_addr & 0x7f) << 8)
                 | USB_PID_IN, MmGetPhysicalAddress(urb->data_buffer).LowPart);

    toggle ^= 1;
    urb->td_count = 1;

    InitializeListHead(&urb->trasac_list);
    InsertTailList(&urb->trasac_list, &ptd->ptde->vert_link);

    //indirectly guarded by pending_endp_list_lock
    if (uhci_claim_bandwidth(uhci, urb, TRUE) == FALSE)
    {
        InitializeListHead(&urb->trasac_list);

        lock_td_pool(&uhci->td_pool, TRUE);
        free_td(&uhci->td_pool, ptd);
        unlock_td_pool(&uhci->td_pool, TRUE);

        return STATUS_NO_MORE_ENTRIES;
    }

    urb->pendp->flags = (urb->pendp->flags & ~USB_ENDP_FLAG_DATATOGGLE) | (toggle << 31);
    usb_endp_busy_count_inc(urb->pendp);

    uhci_insert_urb_to_schedule(uhci, urb, ret);

    if (ret == FALSE)
    {
        lock_td_pool(&uhci->td_pool, TRUE);
        if (ptd)
            free_td(&uhci->td_pool, ptd);
        unlock_td_pool(&uhci->td_pool, TRUE);

        InitializeListHead(&urb->trasac_list);
        usb_endp_busy_count_dec(urb->pendp);
        urb->pendp->flags = (urb->pendp->flags & ~USB_ENDP_FLAG_DATATOGGLE) | ((toggle ^ 1) << 31);
        uhci_claim_bandwidth(uhci, urb, FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
uhci_internal_submit_iso(PUHCI_DEV uhci, PURB urb)
{
    PUHCI_TD ptd;
    LIST_ENTRY td_list, *pthis, *pnext;
    int i;
    BOOLEAN toggle = FALSE, ret;

    if (uhci == NULL || urb == NULL)
        return STATUS_INVALID_PARAMETER;

    if (urb->iso_frame_count == 0)
        return STATUS_INVALID_PARAMETER;

    lock_td_pool(&uhci->td_pool, TRUE);

    if (can_transfer(&uhci->td_pool, urb->iso_frame_count) == FALSE)
    {
        unlock_td_pool(&uhci->td_pool, TRUE);
        return STATUS_NO_MORE_ENTRIES;
    }

    ptd = alloc_tds(&uhci->td_pool, urb->iso_frame_count);
    unlock_td_pool(&uhci->td_pool, TRUE);

    if (ptd == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    InsertTailList(&ptd->ptde->vert_link, &td_list);
    ListFirst(&td_list, pthis);

    urb->td_count = urb->iso_frame_count;

    urb->pipe = (((ULONG) urb->iso_packet_desc[0].length) << 21)
        | ((ULONG) endp_num(urb->pendp) << 15)
        | (((ULONG) dev_from_endp(urb->pendp)->dev_addr) << 8)
        | ((ULONG) endp_dir(urb->pendp)) | USB_ENDPOINT_XFER_ISOC;


    for(i = 0; i < urb->iso_frame_count && pthis; i++)
    {
        ptd = ((PTD_EXTENSION) pthis)->ptd;
        uhci_fill_td(ptd,
                     (3 << TD_CTRL_C_ERR_SHIFT)
                     | (TD_CTRL_ACTIVE)
                     | (TD_CTRL_IOS),
                     (((ULONG) urb->iso_packet_desc[i].length - 1) << 21)
                     | (0 << 19)
                     | ((ULONG) endp_num(urb->pendp) << 15)
                     | (((ULONG) dev_from_endp(urb->pendp)->dev_addr) << 8)
                     | ((urb->pendp->pusb_endp_desc->bEndpointAddress & USB_DIR_IN)
                        ? USB_PID_OUT : USB_PID_IN),
                     MmGetPhysicalAddress(&urb->data_buffer[urb->iso_packet_desc[i].offset]).LowPart);

        toggle ^= 1;
        ListNext(&td_list, pthis, pnext);
        pthis = pnext;
    }

    ptd->status |= TD_CTRL_IOC; //need interrupt

    ListFirst(&td_list, pthis);
    RemoveEntryList(&td_list);

    InsertTailList(pthis, &urb->trasac_list);

    //indirectly guarded by pending_endp_list_lock
    if (uhci_claim_bandwidth(uhci, urb, TRUE) == FALSE)
    {
        //bad news: we can not allocate the enough bandwidth for the urb
        RemoveEntryList(&urb->trasac_list);
        InitializeListHead(&urb->trasac_list);

        lock_td_pool(&uhci->td_pool, TRUE);
        free_tds(&uhci->td_pool, ((PTD_EXTENSION) pthis)->ptd);
        unlock_td_pool(&uhci->td_pool, TRUE);
        return STATUS_NO_MORE_ENTRIES;

    }

    usb_endp_busy_count_inc(urb->pendp);
    uhci_insert_urb_to_schedule(uhci, urb, ret);
    if (ret == FALSE)
    {
        usb_endp_busy_count_dec(urb->pendp);
        RemoveEntryList(&urb->trasac_list);

        lock_td_pool(&uhci->td_pool, TRUE);
        free_tds(&uhci->td_pool, ((PTD_EXTENSION) pthis)->ptd);
        unlock_td_pool(&uhci->td_pool, TRUE);
        uhci_claim_bandwidth(uhci, urb, FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

// runs in uhci_isr
BOOLEAN
uhci_is_xfer_finished(PURB urb)
{
    PLIST_ENTRY pthis, pnext;
    PUHCI_TD ptd;
    BOOLEAN ret = TRUE;
    PTD_EXTENSION ptde;

    if (urb->last_finished_td == NULL)
    {
        urb->last_finished_td = &urb->trasac_list;
    }

    if (&urb->trasac_list == urb->last_finished_td)
        ListFirst(&urb->trasac_list, pthis)
    else
        ListNext(&urb->trasac_list, urb->last_finished_td, pthis);

    while (pthis)
    {
        if ((((PTD_EXTENSION) pthis)->flags & UHCI_ITEM_FLAG_TYPE) != UHCI_ITEM_FLAG_TD)
        {
            ListNext(&urb->trasac_list, pthis, pnext);
            pthis = pnext;
            continue;
        }
        else
        {
            ptde = (PTD_EXTENSION) pthis;
            ptd = ptde->ptd;
            ASSERT(ptd != NULL);

            if (ptd->status & TD_CTRL_ACTIVE)
            {
                //still active
                ret = FALSE;
                break;
            }
            //let's see whether error occured
            if ((ptd->status & TD_CTRL_ANY_ERROR) == 0)
            {
                urb->last_finished_td = pthis;
                ListNext(&urb->trasac_list, pthis, pnext);
                pthis = pnext;
                continue;
            }
            else
            {
                urb->status = ptd->status;
                pthis = NULL;
                continue;
            }
        }

    }

    if (pthis == NULL)
        ret = TRUE;

    return ret;
}

// executed in isr, and have frame_list_lock acquired, so
// never try to acquire any spin-lock
// remove the bulk urb from schedule, and mark it not in
// the schedule
BOOLEAN
uhci_remove_urb_from_schedule(PUHCI_DEV uhci, PURB urb)
{
    BOOLEAN ret = FALSE;
    {
        switch (urb->pipe & USB_ENDPOINT_XFERTYPE_MASK)
        {
            case USB_ENDPOINT_XFER_BULK:
            {
                ret = uhci_remove_bulk_from_schedule(uhci, urb);
                break;
            }
            case USB_ENDPOINT_XFER_CONTROL:
            {
                ret = uhci_remove_ctrl_from_schedule(uhci, urb);
                break;
            }
            case USB_ENDPOINT_XFER_INT:
            {
                ret = uhci_remove_int_from_schedule(uhci, urb);
                break;
            }
            case USB_ENDPOINT_XFER_ISOC:
            {
                ret = uhci_remove_iso_from_schedule(uhci, urb);
                break;
            }
        }
    }
    return ret;
}

// executed in isr, and have frame_list_lock acquired, so
// never try to acquire any spin-lock
// remove the bulk urb from schedule, and mark it not in
// the schedule
BOOLEAN
uhci_remove_bulk_from_schedule(PUHCI_DEV uhci, PURB urb)
{

    PUHCI_QH pqh, pnext_qh, pprev_qh;
    PLIST_ENTRY pthis, pnext, pprev;
    LONG i;

    if (uhci == NULL || urb == NULL)
        return FALSE;

    ListFirst(&urb->trasac_list, pthis);
    pqh = ((PQH_EXTENSION) pthis)->pqh;

    ListFirst(&pqh->pqhe->hori_link, pnext);
    ListFirstPrev(&pqh->pqhe->hori_link, pprev);

    if (pprev == NULL || pnext == NULL)
        return FALSE;

    pnext_qh = struct_ptr(pnext, QH_EXTENSION, hori_link)->pqh;
    pprev_qh = struct_ptr(pprev, QH_EXTENSION, hori_link)->pqh;

    if (pprev != pnext)
    {
        //not the last one
        pprev_qh->link = pnext_qh->phy_addr;
    }
    else
    {
        //only two qhs in the list
        for(i = 0; i < UHCI_MAX_SKELQHS; i++)
        {
            if (pprev_qh == uhci->skel_qh[i])
            {
                break;
            }
        }
        ASSERT(i < UHCI_MAX_SKELQHS - 1);
        pprev_qh->link = uhci->skel_qh[i + 1]->phy_addr;
    }
    RemoveEntryList(&pqh->pqhe->hori_link);

    urb->flags &= ~URB_FLAG_IN_SCHEDULE;

    if ((urb->pipe & USB_DEV_FLAG_LOW_SPEED) == 0)
        uhci_drop_fsbr(uhci);

    return TRUE;
}

BOOLEAN
uhci_remove_iso_from_schedule(PUHCI_DEV uhci, PURB urb)
{
    PUHCI_TD ptd, pprev_td;
    PLIST_ENTRY pthis, pnext, pprev;
    int i, idx;

    if (uhci == NULL || urb == NULL)
        return FALSE;

    ListFirst(&urb->trasac_list, pthis);

    for(i = 0; i < urb->iso_frame_count && pthis; i++)
    {
        ptd = ((PTD_EXTENSION) pthis)->ptd;
        idx = (urb->iso_start_frame + i) & (UHCI_MAX_FRAMES - 1);

        ListFirstPrev(&ptd->ptde->hori_link, pprev);

        if (pprev == NULL)
            return FALSE;

        if (pprev == &uhci->frame_list_cpu[idx].td_link)
        {
            uhci->frame_list[idx] = ptd->link;
        }
        else
        {
            pprev_td = struct_ptr(pprev, TD_EXTENSION, hori_link)->ptd;
            pprev_td->link = ptd->link;
        }

        RemoveEntryList(&ptd->ptde->hori_link);
        ListNext(&urb->trasac_list, pthis, pnext);
        pthis = pnext;
    }

    urb->flags &= ~URB_FLAG_IN_SCHEDULE;
    return TRUE;
}

BOOLEAN
uhci_remove_int_from_schedule(PUHCI_DEV uhci, PURB urb)
{
    PUHCI_TD ptd, pnext_td, pprev_td;
    PLIST_ENTRY pthis, pnext, pprev;
    LONG i;

    if (uhci == NULL || urb == NULL)
        return FALSE;

    ListFirst(&urb->trasac_list, pthis);
    ptd = ((PTD_EXTENSION) pthis)->ptd;
    ListFirst(&ptd->ptde->hori_link, pnext);
    ListFirstPrev(&ptd->ptde->hori_link, pprev);

    if (pprev == NULL || pnext == NULL)
        return FALSE;

    pnext_td = struct_ptr(pnext, TD_EXTENSION, hori_link)->ptd;
    pprev_td = struct_ptr(pprev, TD_EXTENSION, hori_link)->ptd;

    if (pprev_td != pnext_td)
        pprev_td->link = pnext_td->phy_addr;
    else
    {
        //the last one
        for(i = UHCI_MAX_SKELTDS - 2; i >= 0; i--)
        {
            //UHCI_MAX_SKELTDS -1 skel tds for int transfer
            if (pprev_td == uhci->skel_td[i])
                break;
        }

        ASSERT(i >= 0);
        if (i == 0)
        {
            pprev_td->link = uhci->skel_qh[0]->phy_addr;
        }
        else
        {
            pprev_td->link = uhci->skel_td[i - 1]->phy_addr;
        }
    }
    RemoveEntryList(&ptd->ptde->hori_link);

    urb->flags &= ~URB_FLAG_IN_SCHEDULE;
    return TRUE;
}

BOOLEAN
uhci_insert_tds_qh(PUHCI_QH pqh, PUHCI_TD td_chain)
{
    if (pqh == NULL || td_chain == NULL)
        return FALSE;

    InsertTailList(&td_chain->ptde->vert_link, &pqh->pqhe->vert_link);
    pqh->element = td_chain->phy_addr;
    return TRUE;
}

BOOLEAN
uhci_insert_qh_urb(PURB urb, PUHCI_QH qh_chain)
{
    if (urb == NULL || qh_chain == NULL)
        return FALSE;

    InsertTailList(&qh_chain->pqhe->vert_link, &urb->trasac_list);
    qh_chain->pqhe->purb = urb;
    return TRUE;
}

// must have dev_lock and frame_list_lock acquired
BOOLEAN
uhci_insert_urb_schedule(PUHCI_DEV uhci, PURB urb)
{
    PUHCI_QH pqh, pskel_qh, pnext_qh;
    PUHCI_TD ptd, plast_td;
    PLIST_ENTRY pthis, pnext;
    int i;

    if (uhci == NULL || urb == NULL)
        return FALSE;

    ListFirst(&urb->trasac_list, pthis);
    if (pthis == NULL)
        return FALSE;

    InsertTailList(&uhci->urb_list, (PLIST_ENTRY) urb);

    urb->flags &= ~URB_FLAG_STATE_MASK;
    urb->flags |= URB_FLAG_STATE_IN_PROCESS | URB_FLAG_IN_SCHEDULE;


    switch (endp_type(urb->pendp))
    {
        case USB_ENDPOINT_XFER_CONTROL:
        {
            pqh = ((PQH_EXTENSION) pthis)->pqh;

            if ((dev_from_endp(urb->pendp)->flags & USB_DEV_FLAG_LOW_SPEED) == 0)
            {
                pskel_qh = uhci->skel_hs_control_qh;
                pnext_qh = uhci->skel_bulk_qh;
            }
            else
            {
                pskel_qh = uhci->skel_ls_control_qh;
                pnext_qh = uhci->skel_hs_control_qh;
            }

            ListFirstPrev(&pskel_qh->pqhe->hori_link, pthis);

            if (pthis == NULL)
                pthis = &pskel_qh->pqhe->hori_link;

            InsertTailList(&pskel_qh->pqhe->hori_link, &pqh->pqhe->hori_link);
            pqh->link = pnext_qh->phy_addr;
            struct_ptr(pthis, QH_EXTENSION, hori_link)->pqh->link = pqh->phy_addr;

            //full speed band reclaimation
            if ((urb->pipe & USB_DEV_FLAG_LOW_SPEED) == 0)
            {
                uhci->fsbr_cnt++;
                if (uhci->fsbr_cnt == 1)
                {
                    uhci->skel_term_qh->link = uhci->skel_hs_control_qh->phy_addr;
                }
            }
            return TRUE;
        }
        case USB_ENDPOINT_XFER_BULK:
        {
            pqh = ((PQH_EXTENSION) pthis)->pqh;

            ListFirstPrev(&uhci->skel_bulk_qh->pqhe->hori_link, pthis);

            if (pthis == NULL)
                pthis = &uhci->skel_bulk_qh->pqhe->hori_link;

            InsertTailList(&uhci->skel_bulk_qh->pqhe->hori_link, &pqh->pqhe->hori_link);

            pqh->link = uhci->skel_term_qh->phy_addr;
            struct_ptr(pthis, QH_EXTENSION, hori_link)->pqh->link = pqh->phy_addr;

            //full speed band reclaimation
            uhci->fsbr_cnt++;
            if (uhci->fsbr_cnt == 1)
            {
                uhci->skel_term_qh->link = uhci->skel_hs_control_qh->phy_addr;
            }

            return TRUE;
        }
        case USB_ENDPOINT_XFER_INT:
        {
            //bandwidth claim is done outside
            ptd = ((PTD_EXTENSION) pthis)->ptd;

            get_int_idx(urb, i);

            ListFirstPrev(&uhci->skel_td[i]->ptde->hori_link, pthis);
            if (pthis == NULL)
                pthis = &uhci->skel_td[i]->ptde->hori_link;

            InsertTailList(&uhci->skel_td[i]->ptde->hori_link, &ptd->ptde->hori_link);

            if (i > 0)
            {
                ptd->link = uhci->skel_td[i - 1]->phy_addr;
            }
            else if (i == 0)
            {
                ptd->link = uhci->skel_qh[0]->phy_addr;
            }
            //finally link the previous td to this td
            struct_ptr(pthis, TD_EXTENSION, hori_link)->ptd->link = ptd->phy_addr;
            return TRUE;
        }
        case USB_ENDPOINT_XFER_ISOC:
        {

            for(i = 0; i < urb->iso_frame_count; i++)
            {
                ptd = ((PTD_EXTENSION) pthis)->ptd;
                InsertTailList(&uhci->frame_list_cpu[(urb->iso_start_frame + i) & 0x3ff].td_link,
                               &ptd->ptde->hori_link);

                if (IsListEmpty(&uhci->frame_list_cpu[(urb->iso_start_frame + i) & 0x3ff].td_link) == TRUE)
                {
                    ptd->link = uhci->frame_list[(urb->iso_start_frame + i) & 0x3ff];
                    uhci->frame_list[i] = ptd->phy_addr;
                }
                else
                {
                    ListFirstPrev(&uhci->frame_list_cpu[(urb->iso_start_frame + i) & 0x3ff].td_link, pnext);
                    plast_td = struct_ptr(pnext, TD_EXTENSION, hori_link)->ptd;
                    ptd->link = plast_td->link;
                    plast_td->link = ptd->phy_addr;
                }

                ListNext(&urb->trasac_list, pthis, pnext);
                pthis = pnext;
            }
            return TRUE;

        }
    }
    return FALSE;
}

//this function used as the KeSynchronizeExecution param to delegate control to uhci_insert_urb_schedule
BOOLEAN NTAPI
uhci_sync_insert_urb_schedule(PVOID context)
{
    PSYNC_PARAM sync_param;
    PUHCI_DEV uhci;
    PURB purb;

    sync_param = (PSYNC_PARAM) context;
    if (sync_param == NULL)
        return FALSE;

    uhci = sync_param->uhci;
    purb = (PURB) sync_param->context;

    if (uhci == NULL || purb == NULL)
        return (UCHAR) (sync_param->ret = FALSE);

    return (UCHAR) (sync_param->ret = uhci_insert_urb_schedule(uhci, purb));
}

// be sure pending_endp_list_lock acquired
BOOLEAN
uhci_claim_bandwidth(PUHCI_DEV uhci,
                     PURB urb,
                     BOOLEAN claim_bw    //true to claim bandwidth, false to free bandwidth
                     )
{

    UCHAR type;
    BOOLEAN ls, can_alloc;
    LONG bus_time, us;
    LONG i, idx, j, start_frame, interval;

    if (urb == NULL)
        return FALSE;

    can_alloc = TRUE;

    type = (UCHAR) (urb->pipe & USB_ENDPOINT_XFERTYPE_MASK);
    if (type == USB_ENDPOINT_XFER_BULK || type == USB_ENDPOINT_XFER_CONTROL)
    {
        return FALSE;
    }

    ls = (urb->pipe & USB_DEV_FLAG_LOW_SPEED) ? TRUE : FALSE;

    if (type == USB_ENDPOINT_XFER_INT)
    {
        start_frame = 0;
        i = urb->data_length;
        bus_time = usb_calc_bus_time(ls, FALSE, FALSE, i);
        us = ns_to_us(bus_time);

        i = (urb->pipe >> 24);  //polling interval

        for(interval = 0, j = 0; j < 8; j++)
        {
            if (i & (1 << j))
            {
                interval = j;
            }
        }

        interval = 1 << interval;
        start_frame = interval - 1;

        if (claim_bw)
        {

            for(idx = 0; idx < UHCI_MAX_FRAMES; idx += interval)
            {
                if (uhci->frame_bw[idx] < us)
                {
                    can_alloc = FALSE;
                    break;
                }
            }

            if (!can_alloc)
            {
                return FALSE;
            }

            for(idx = start_frame; idx < UHCI_MAX_FRAMES; idx += interval)
            {
                uhci->frame_bw[idx] -= us;
            }
        }
        else
        {
            for(idx = start_frame; idx < UHCI_MAX_FRAMES; idx += interval)
            {
                uhci->frame_bw[idx] += us;
            }
        }

    }
    else if (type == USB_ENDPOINT_XFER_ISOC)
    {
        if (claim_bw)
        {
            for(i = 0; i < urb->iso_frame_count; i++)
            {
                bus_time = usb_calc_bus_time(FALSE,
                                             (urb->pipe & USB_DIR_IN)
                                             ? TRUE : FALSE, TRUE, urb->iso_packet_desc[i].length);

                urb->iso_packet_desc[i].bus_time = ns_to_us(bus_time);
            }

            for(i = 0; i < urb->iso_frame_count; i++)
            {
                if (uhci->frame_bw[(urb->iso_start_frame + i) & 0x3ff] < urb->iso_packet_desc[i].bus_time)
                {
                    can_alloc = FALSE;
                    break;
                }
            }

            if (!can_alloc)
            {
                return FALSE;
            }

            for(i = 0; i < urb->iso_frame_count; i++)
            {
                uhci->frame_bw[(urb->iso_start_frame + i) & 0x3ff] -= urb->iso_packet_desc[i].bus_time;
            }
        }
        else
        {
            for(i = 0; i < urb->iso_frame_count; i++)
            {
                uhci->frame_bw[(urb->iso_start_frame + i) & 0x3ff] += urb->iso_packet_desc[i].bus_time;
            }
        }

    }

    return TRUE;
}


//cancel a single urb
BOOLEAN NTAPI
uhci_sync_cancel_urb(PVOID context)
{
    PUHCI_DEV uhci;
    PSYNC_PARAM sync_param;
    PURB purb2, dest_urb;
    PLIST_ENTRY pthis, pnext;
    BOOLEAN found = FALSE;

    if (context == NULL)
        return FALSE;

    sync_param = (PSYNC_PARAM) context;
    uhci = sync_param->uhci;
    dest_urb = (PURB) sync_param->context;

    if (uhci == NULL || dest_urb == NULL)
        return (UCHAR) (sync_param->ret = FALSE);

    ListFirst(&uhci->urb_list, pthis);
    while (pthis)
    {
        purb2 = (PURB) pthis;
        if (purb2 == dest_urb)
        {
            found = TRUE;
            purb2->flags |= URB_FLAG_FORCE_CANCEL;
            break;
        }
        ListNext(&uhci->urb_list, pthis, pnext);
        pthis = pnext;
    }
    if (found)
        uhci->skel_term_td->status |= TD_CTRL_IOC;

    return (UCHAR) (sync_param->ret = found);
}

//note any fields of the purb can not be referenced unless it is found in some queue
NTSTATUS
uhci_cancel_urb(PUHCI_DEV uhci, PUSB_DEV pdev, PUSB_ENDPOINT pendp, PURB purb)
{
    PLIST_ENTRY pthis, pnext;
    BOOLEAN found;
    PURB purb2;

    SYNC_PARAM sync_param;

    USE_BASIC_NON_PENDING_IRQL;

    if (uhci == NULL || purb == NULL || pdev == NULL || pendp == NULL)
        return STATUS_INVALID_PARAMETER;

    lock_dev(pdev, FALSE);

    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        unlock_dev(pdev, FALSE);
        //delegate to remove device for this job
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    if (dev_from_endp(pendp) != pdev)
    {
        unlock_dev(pdev, FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    if (endp_state(pendp) == USB_ENDP_FLAG_STALL)
    {
        //it will be canceled in uhci_process_pending_endp
        unlock_dev(pdev, FALSE);
        return USB_STATUS_ENDPOINT_HALTED;
    }

    found = FALSE;
    ListFirst(&pendp->urb_list, pthis);
    while (pthis)
    {
        purb2 = (PURB) pthis;
        if (purb2 == purb)
        {
            found = TRUE;
            RemoveEntryList(pthis);
            InitializeListHead(pthis);
            break;
        }
        ListNext(&pendp->urb_list, pthis, pnext);
        pthis = pnext;
    }
    unlock_dev(pdev, FALSE);

    if (found)
    {
        purb->status = STATUS_CANCELLED;

        uhci_generic_urb_completion(purb, purb->context);

        lock_dev(pdev, FALSE);
        pdev->ref_count--;
        unlock_dev(pdev, FALSE);
        return STATUS_SUCCESS;
    }

    //      search the urb in the urb-list and try to cancel
    sync_param.uhci = uhci;
    sync_param.context = purb;

    KeSynchronizeExecution(uhci->pdev_ext->uhci_int, uhci_sync_cancel_urb, &sync_param);

    found = (BOOLEAN) sync_param.ret;

    if (found)
        return USB_STATUS_CANCELING;

    return STATUS_INVALID_PARAMETER;
}

VOID
uhci_generic_urb_completion(PURB purb, PVOID context)
{
    PUSB_DEV pdev;
    USE_NON_PENDING_IRQL;

    old_irql = KeGetCurrentIrql();
    if (old_irql > DISPATCH_LEVEL)
        TRAP();

    if (old_irql < DISPATCH_LEVEL)
        KeRaiseIrql(DISPATCH_LEVEL, &old_irql);

    if (purb == NULL)
        return;

    pdev = purb->pdev;

    if (pdev == NULL)
        return;

    lock_dev(pdev, TRUE);

    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        unlock_dev(pdev, TRUE);
        goto LBL_CLIENT_PROCESS;
    }
    if (usb_error(purb->status))
    {
        pdev->error_count++;
    }

    if (purb->pendp == &pdev->default_endp)
    {
        if (usb_halted(purb->status))
        {
            pdev->time_out_count++;
            if (pdev->time_out_count > 3)
            {
                dev_set_state(pdev, USB_DEV_STATE_ZOMB);
                uhci_dbg_print(DBGLVL_MAXIMUM,
                               ("uhci_generic_urb_completion(): contiguous error 3 times, dev 0x%x is deactivated\n",
                                pdev));
            }
        }
        else
            pdev->time_out_count = 0;

    }
    unlock_dev(pdev, TRUE);

  LBL_CLIENT_PROCESS:
    if (purb->completion)
        purb->completion(purb, context);

    if (old_irql < DISPATCH_LEVEL)
        KeLowerIrql(old_irql);

    return;
}


NTSTATUS
uhci_rh_submit_urb(PUSB_DEV pdev, PURB purb)
{
    PUSB_DEV_MANAGER dev_mgr;
    PTIMER_SVC ptimer;
    PUSB_CTRL_SETUP_PACKET psetup;
    PUHCI_DEV uhci;
    NTSTATUS status;
    USHORT port_status;
#ifndef INCLUDE_EHCI
    PHUB_EXTENSION hub_ext;
#else
    PHUB2_EXTENSION hub_ext;
#endif
    PUSB_PORT_STATUS ps, psret;
    LONG i;
    USE_NON_PENDING_IRQL;

    if (pdev == NULL || purb == NULL)
        return STATUS_INVALID_PARAMETER;

    dev_mgr = dev_mgr_from_dev(pdev);

    KeAcquireSpinLock(&dev_mgr->timer_svc_list_lock, &old_irql);
    lock_dev(pdev, FALSE);
    if (dev_state(pdev) == USB_DEV_STATE_ZOMB)
    {
        unlock_dev(pdev, FALSE);
        KeReleaseSpinLock(&dev_mgr->timer_svc_list_lock, old_irql);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    uhci = uhci_from_hcd(pdev->hcd);
    psetup = (PUSB_CTRL_SETUP_PACKET) purb->setup_packet;

#ifndef INCLUDE_EHCI
    hub_ext = ((PHUB_EXTENSION) pdev->dev_ext);
#else
    hub_ext = ((PHUB2_EXTENSION) pdev->dev_ext);
#endif

    switch (endp_type(purb->pendp))
    {
        case USB_ENDPOINT_XFER_CONTROL:
        {
            if (psetup->bmRequestType == 0xa3 && psetup->bRequest == USB_REQ_GET_STATUS)
            {
                //get-port-status
                if (psetup->wIndex == 0 || psetup->wIndex > 2 || psetup->wLength < 4)
                {
                    purb->status = STATUS_INVALID_PARAMETER;
                    break;
                }
                if (psetup->wIndex == 1)
                {
                    status = READ_PORT_USHORT((PUSHORT) (uhci->port_base + USBPORTSC1));
                    ps = &hub_ext->rh_port1_status;
                }
                else
                {
                    status = READ_PORT_USHORT((PUSHORT) (uhci->port_base + USBPORTSC2));
                    ps = &hub_ext->rh_port2_status;
                }

                psret = (PUSB_PORT_STATUS) purb->data_buffer;
                ps->wPortStatus = 0;

                if (status & USBPORTSC_CCS)
                {
                    ps->wPortStatus |= USB_PORT_STAT_CONNECTION;
                }
                if (status & USBPORTSC_PE)
                {
                    ps->wPortStatus |= USB_PORT_STAT_ENABLE;
                }
                if (status & USBPORTSC_PR)
                {
                    ps->wPortStatus |= USB_PORT_STAT_RESET;
                }
                if (status & USBPORTSC_SUSP)
                {
                    ps->wPortStatus |= USB_PORT_STAT_SUSPEND;
                }
                if (status & USBPORTSC_LSDA)
                {
                    ps->wPortStatus |= USB_PORT_STAT_LOW_SPEED;
                }

                //always power on
                ps->wPortStatus |= USB_PORT_STAT_POWER;

                //now set change field
                if (status & USBPORTSC_CSC)
                {
                    ps->wPortChange |= USB_PORT_STAT_C_CONNECTION;
                }
                if (status & USBPORTSC_PEC)
                {
                    ps->wPortChange |= USB_PORT_STAT_C_ENABLE;
                }

                //don't touch other fields, will be filled by
                //other function

                usb_dbg_print(DBGLVL_MAXIMUM,
                              ("uhci_rh_submit_urb(): get port status, wPortStatus=0x%x, wPortChange=0x%x, address=0x%x\n",
                               ps->wPortStatus, ps->wPortChange, ps));

                psret->wPortChange = ps->wPortChange;
                psret->wPortStatus = ps->wPortStatus;

                purb->status = STATUS_SUCCESS;

                break;
            }
            else if (psetup->bmRequestType == 0x23 && psetup->bRequest == USB_REQ_CLEAR_FEATURE)
            {
                //clear-port-feature
                if (psetup->wIndex == 0 || psetup->wIndex > 2)
                {
                    purb->status = STATUS_INVALID_PARAMETER;
                    break;
                }
                if (psetup->wIndex == 1)
                {
                    i = USBPORTSC1;
                    ps = &hub_ext->rh_port1_status;
                }
                else
                {
                    i = USBPORTSC2;
                    ps = &hub_ext->rh_port2_status;
                }

                purb->status = STATUS_SUCCESS;
                switch (psetup->wValue)
                {
                    case USB_PORT_FEAT_C_CONNECTION:
                    {
                        ps->wPortChange &= ~USB_PORT_STAT_C_CONNECTION;
                        SET_RH_PORTSTAT(i, USBPORTSC_CSC);
                        status = READ_PORT_USHORT((PUSHORT) (uhci->port_base + i));
                        usb_dbg_print(DBGLVL_MAXIMUM,
                                      ("uhci_rh_submit_urb(): clear csc, port%d=0x%x\n", psetup->wIndex,
                                       status));
                        break;
                    }
                    case USB_PORT_FEAT_C_ENABLE:
                    {
                        ps->wPortChange &= ~USB_PORT_STAT_C_ENABLE;
                        SET_RH_PORTSTAT(i, USBPORTSC_PEC);
                        status = READ_PORT_USHORT((PUSHORT) (uhci->port_base + i));
                        usb_dbg_print(DBGLVL_MAXIMUM,
                                      ("uhci_rh_submit_urb(): clear pec, port%d=0x%x\n", psetup->wIndex,
                                       status));
                        break;
                    }
                    case USB_PORT_FEAT_C_RESET:
                    {
                        ps->wPortChange &= ~USB_PORT_STAT_C_RESET;
                        //the reset signal is down in rh_timer_svc_reset_port_completion
                        //so enable the port here
                        status = READ_PORT_USHORT((PUSHORT)(uhci->port_base + i));
                        usb_dbg_print(DBGLVL_MAXIMUM,
                                      ("uhci_rh_submit_urb(): clear pr, enable pe, port%d=0x%x\n",
                                       psetup->wIndex, status));
                        break;
                    }
                    case USB_PORT_FEAT_ENABLE:
                    {
                        ps->wPortStatus &= ~USB_PORT_STAT_ENABLE;
                        CLR_RH_PORTSTAT(i, USBPORTSC_PE);
                        status = READ_PORT_USHORT((PUSHORT)(uhci->port_base + i));
                        usb_dbg_print(DBGLVL_MAXIMUM,
                                      ("uhci_rh_submit_urb(): clear pe, port%d=0x%x\n", psetup->wIndex,
                                       status));
                        break;
                    }
                    default:
                        purb->status = STATUS_UNSUCCESSFUL;
                }
                break;
            }
            else if (psetup->bmRequestType == 0xd3 && psetup->bRequest == HUB_REQ_GET_STATE)
            {
                // get bus state
                if (psetup->wIndex == 0 || psetup->wIndex > 2 || psetup->wLength == 0)
                {
                    purb->status = STATUS_INVALID_PARAMETER;
                    break;
                }

                if (psetup->wIndex == 1)
                {
                    i = USBPORTSC1;
                }
                else
                {
                    i = USBPORTSC2;
                }
                port_status = READ_PORT_USHORT((PUSHORT)(uhci->port_base + i));
                purb->data_buffer[0] = (port_status & USBPORTSC_LS);

                // reverse the order
                purb->data_buffer[0] ^= 0x3;
                purb->status = STATUS_SUCCESS;
                break;
            }
            else if (psetup->bmRequestType == 0x23 && psetup->bRequest == USB_REQ_SET_FEATURE)
            {
                //reset port
                if (psetup->wValue != USB_PORT_FEAT_RESET)
                {
                    purb->status = STATUS_INVALID_PARAMETER;
                    uhci_dbg_print(DBGLVL_MAXIMUM,
                                   ("uhci_rh_submit_urb(): set feature with wValue=0x%x\n", psetup->wValue));
                    break;
                }
                if (psetup->wIndex == 1)
                {
                    i = USBPORTSC1;
                }
                else
                {
                    i = USBPORTSC2;
                }

                ptimer = alloc_timer_svc(&dev_mgr->timer_svc_pool, 1);
                ptimer->threshold = 0;  // within [ 50ms, 60ms ], one tick is 10 ms
                ptimer->context = (ULONG) purb;
                ptimer->pdev = pdev;
                ptimer->func = rh_timer_svc_reset_port_completion;

                //start the timer
                pdev->ref_count += 2;   //one for timer and one for urb

                status = READ_PORT_USHORT((PUSHORT) (uhci->port_base + i));
                usb_dbg_print(DBGLVL_MAXIMUM,
                              ("uhci_rh_submit_urb(): reset port, port%d=0x%x\n", psetup->wIndex, status));
                InsertTailList(&dev_mgr->timer_svc_list, &ptimer->timer_svc_link);
                purb->status = STATUS_PENDING;
            }
            else
            {
                purb->status = STATUS_INVALID_PARAMETER;
            }
            break;
        }
        case USB_ENDPOINT_XFER_INT:
        {
            ptimer = alloc_timer_svc(&dev_mgr->timer_svc_pool, 1);
            ptimer->threshold = RH_INTERVAL;
            ptimer->context = (ULONG) purb;
            ptimer->pdev = pdev;
            ptimer->func = rh_timer_svc_int_completion;

            //start the timer
            InsertTailList(&dev_mgr->timer_svc_list, &ptimer->timer_svc_link);

            usb_dbg_print(DBGLVL_ULTRA,
                          ("uhci_rh_submit_urb(): current rh's ref_count=0x%x\n", pdev->ref_count));
            pdev->ref_count += 2;       //one for timer and one for urb

            purb->status = STATUS_PENDING;
            break;
        }
        case USB_ENDPOINT_XFER_BULK:
        case USB_ENDPOINT_XFER_ISOC:
        default:
        {
            purb->status = STATUS_INVALID_PARAMETER;
            break;
        }
    }
    unlock_dev(pdev, FALSE);
    KeReleaseSpinLock(&dev_mgr->timer_svc_list_lock, old_irql);
    return purb->status;
}

//must have rh dev_lock acquired
BOOLEAN
uhci_rh_reset_port(PHCD hcd, UCHAR port_idx)
{
    LONG i;
    PUHCI_DEV uhci;
    ULONG status;

    if (port_idx != 1 && port_idx != 2)
        return FALSE;

    if (hcd == NULL)
        return FALSE;

    if (port_idx == 1)
    {
        i = USBPORTSC1;
    }
    else
    {
        i = USBPORTSC2;
    }

    uhci = uhci_from_hcd(hcd);
    //assert the reset signal,(implicitly disable the port)
    SET_RH_PORTSTAT(i, USBPORTSC_PR);
    usb_wait_ms_dpc(50);
    //clear the reset signal, delay port enable till clearing port feature
    CLR_RH_PORTSTAT(i, USBPORTSC_PR);
    usb_wait_us_dpc(10);
    SET_RH_PORTSTAT(i, USBPORTSC_PE);
    //recovery time 10ms
    usb_wait_ms_dpc(10);
    SET_RH_PORTSTAT(i, 0x0a);

    status = READ_PORT_USHORT((PUSHORT) (uhci->port_base + i));
    usb_dbg_print(DBGLVL_MAXIMUM, ("uhci_rh_reset_port(): status after written=0x%x\n", status));

    return TRUE;
}

NTSTATUS
uhci_dispatch_irp(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp)
{
    PDEVICE_EXTENSION pdev_ext;
    PUSB_DEV_MANAGER dev_mgr;
    PUHCI_DEV uhci;

    pdev_ext = DeviceObject->DeviceExtension;
    uhci = pdev_ext->uhci;

    dev_mgr = uhci->hcd_interf.hcd_get_dev_mgr(&uhci->hcd_interf);
    return dev_mgr_dispatch(dev_mgr, irp);
}

VOID NTAPI
uhci_unload(IN PDRIVER_OBJECT DriverObject)
{
    PDEVICE_OBJECT pdev;
    PDEVICE_EXTENSION pdev_ext;
    PUSB_DEV_MANAGER dev_mgr;

    pdev = DriverObject->DeviceObject;

    if (pdev == NULL)
        return;

    pdev_ext = pdev->DeviceExtension;
    if (pdev_ext == NULL)
        return;

    dev_mgr = &g_dev_mgr;
    if (dev_mgr == NULL)
        return;
    //
    // set the termination flag
    //
    dev_mgr->term_flag = TRUE;

    //
    // wake up the thread if it is
    //
    KeSetEvent(&dev_mgr->wake_up_event, 0, FALSE);
    KeWaitForSingleObject(dev_mgr->pthread, Executive, KernelMode, TRUE, NULL);
    ObDereferenceObject(dev_mgr->pthread);
    dev_mgr->pthread = NULL;
    // for( i = 0; i < dev_mgr->hcd_count; i++ )
    //      dev_mgr->hcd_array[ i ]->hcd_release( dev_mgr->hcd_array[ i ]);
    dev_mgr_release_hcd(dev_mgr);

    return;
}

//the following are for hcd interface methods
VOID
uhci_set_dev_mgr(struct _HCD * hcd, PUSB_DEV_MANAGER dev_mgr)
{
    hcd->dev_mgr = dev_mgr;
}

PUSB_DEV_MANAGER
uhci_get_dev_mgr(struct _HCD *hcd)
{
    return hcd->dev_mgr;
}

ULONG
uhci_get_type(struct _HCD * hcd)
{
    return (hcd->flags & HCD_TYPE_MASK);
}

VOID
uhci_set_id(struct _HCD * hcd, UCHAR id)
{
    hcd->flags &= ~HCD_ID_MASK;
    hcd->flags |= (HCD_ID_MASK & id);
}

UCHAR
uhci_get_id(struct _HCD *hcd)
{
    return (UCHAR) (hcd->flags & HCD_ID_MASK);
}


UCHAR
uhci_alloc_addr(struct _HCD * hcd)
{
    LONG i;
    if (hcd == NULL)
        return 0;

    for(i = 1; i < MAX_DEVS; i++)
    {
        if (hcd->dev_addr_map[i >> 3] & (1 << (i & 7)))
        {
            continue;
        }
        else
        {
            break;
        }
    }

    if (i >= MAX_DEVS)
        return 0xff;

    hcd->dev_addr_map[i >> 3] |= (1 << (i & 7));
    hcd->conn_count++;
    return (BYTE) i;
}

VOID
uhci_free_addr(struct _HCD * hcd, UCHAR addr)
{
    if (addr & 0x80)
        return;

    if (hcd == NULL)
        return;

    hcd->dev_addr_map[addr >> 3] &= ~(1 << (addr & 7));
    return;

}

NTSTATUS
uhci_submit_urb2(struct _HCD * hcd, PUSB_DEV pdev, PUSB_ENDPOINT pendp, PURB purb)
{
    return uhci_submit_urb(uhci_from_hcd(hcd), pdev, pendp, purb);
}

PUSB_DEV
uhci_get_root_hub(struct _HCD * hcd)
{
    return uhci_from_hcd(hcd)->root_hub;
}

VOID
uhci_set_root_hub(struct _HCD * hcd, PUSB_DEV root_hub)
{
    if (hcd == NULL || root_hub == NULL)
        return;
    uhci_from_hcd(hcd)->root_hub = root_hub;
    return;
}

BOOLEAN
uhci_remove_device2(struct _HCD * hcd, PUSB_DEV pdev)
{
    if (hcd == NULL || pdev == NULL)
        return FALSE;

    return uhci_remove_device(uhci_from_hcd(hcd), pdev);
}

BOOLEAN
uhci_hcd_release(struct _HCD * hcd)
{
    PUHCI_DEV uhci;
    PDEVICE_EXTENSION pdev_ext;

    if (hcd == NULL)
        return FALSE;


    uhci = uhci_from_hcd(hcd);
    pdev_ext = uhci->pdev_ext;

    return uhci_release(pdev_ext->pdev_obj);
}

NTSTATUS
uhci_cancel_urb2(struct _HCD * hcd, PUSB_DEV pdev, PUSB_ENDPOINT pendp, PURB purb)
{
    PUHCI_DEV uhci;
    if (hcd == NULL)
        return STATUS_INVALID_PARAMETER;

    uhci = uhci_from_hcd(hcd);
    return uhci_cancel_urb(uhci, pdev, pendp, purb);
}

BOOLEAN
uhci_rh_get_dev_change(PHCD hcd, PBYTE buf)
{
    PUHCI_DEV uhci;
    ULONG status;

    if (hcd == NULL || buf == NULL)
        return FALSE;

    uhci = uhci_from_hcd(hcd);
    status = READ_PORT_USHORT((PUSHORT) (uhci->port_base + USBPORTSC1));
    usb_dbg_print(DBGLVL_ULTRA, ("uhci_rh_get_dev_change(): rh port1 status=0x%x\n", status));

    if ((status & USBPORTSC_PEC) || (status & USBPORTSC_CSC))
    {
        buf[0] |= (1 << 1);
    }

    status = READ_PORT_USHORT((PUSHORT) (uhci->port_base + USBPORTSC2));
    usb_dbg_print(DBGLVL_ULTRA, ("uhci_rh_get_dev_change(): rh port2 status=0x%x\n", status));

    if ((status & USBPORTSC_PEC) || (status & USBPORTSC_CSC))
    {
        buf[0] |= (1 << 2);
    }
    return TRUE;
}

NTSTATUS
uhci_dispatch(PHCD hcd, LONG disp_code, PVOID param)    // locking depends on type of code
{
    if (hcd == NULL)
        return FALSE;

    switch (disp_code)
    {
        case HCD_DISP_READ_PORT_COUNT:
        {
            if (param == NULL)
                return STATUS_INVALID_PARAMETER;
            *((PUCHAR) param) = 2;
            return STATUS_SUCCESS;
        }
        case HCD_DISP_READ_RH_DEV_CHANGE:
        {
            if (uhci_rh_get_dev_change(hcd, param) == FALSE)
                return STATUS_INVALID_PARAMETER;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_IMPLEMENTED;
}

VOID
uhci_init_hcd_interface(PUHCI_DEV uhci)
{
    uhci->hcd_interf.hcd_set_dev_mgr = uhci_set_dev_mgr;
    uhci->hcd_interf.hcd_get_dev_mgr = uhci_get_dev_mgr;
    uhci->hcd_interf.hcd_get_type = uhci_get_type;
    uhci->hcd_interf.hcd_set_id = uhci_set_id;
    uhci->hcd_interf.hcd_get_id = uhci_get_id;
    uhci->hcd_interf.hcd_alloc_addr = uhci_alloc_addr;
    uhci->hcd_interf.hcd_free_addr = uhci_free_addr;
    uhci->hcd_interf.hcd_submit_urb = uhci_submit_urb2;
    uhci->hcd_interf.hcd_generic_urb_completion = uhci_generic_urb_completion;
    uhci->hcd_interf.hcd_get_root_hub = uhci_get_root_hub;
    uhci->hcd_interf.hcd_set_root_hub = uhci_set_root_hub;
    uhci->hcd_interf.hcd_remove_device = uhci_remove_device2;
    uhci->hcd_interf.hcd_rh_reset_port = uhci_rh_reset_port;
    uhci->hcd_interf.hcd_release = uhci_hcd_release;
    uhci->hcd_interf.hcd_cancel_urb = uhci_cancel_urb2;
    uhci->hcd_interf.hcd_start = uhci_start;
    uhci->hcd_interf.hcd_dispatch = uhci_dispatch;

    uhci->hcd_interf.flags = HCD_TYPE_UHCI;     //hcd types | hcd id
}


NTSTATUS NTAPI
generic_dispatch_irp(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    PDEVEXT_HEADER dev_ext;

    dev_ext = (PDEVEXT_HEADER) dev_obj->DeviceExtension;

    if (dev_ext && dev_ext->dispatch)
        return dev_ext->dispatch(dev_obj, irp);

    irp->IoStatus.Information = 0;

    EXIT_DISPATCH(STATUS_UNSUCCESSFUL, irp);
}


VOID NTAPI
generic_start_io(IN PDEVICE_OBJECT dev_obj, IN PIRP irp)
{
    PDEVEXT_HEADER dev_ext;

    KIRQL old_irql;

    IoAcquireCancelSpinLock(&old_irql);
    if (irp != dev_obj->CurrentIrp || irp->Cancel)
    {
        IoReleaseCancelSpinLock(old_irql);
        return;
    }
    else
    {
        (void)IoSetCancelRoutine(irp, NULL);
        IoReleaseCancelSpinLock(old_irql);
    }

    dev_ext = (PDEVEXT_HEADER) dev_obj->DeviceExtension;

    if (dev_ext && dev_ext->start_io)
    {
        dev_ext->start_io(dev_obj, irp);
        return;
    }

    irp->IoStatus.Information = 0;
    irp->IoStatus.Status = STATUS_UNSUCCESSFUL;

    IoStartNextPacket(dev_obj, FALSE);
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
#if DBG
    NTSTATUS ntStatus = STATUS_SUCCESS;

    // should be done before any debug output is done.
    // read our debug verbosity level from the registry
    //NetacOD_GetRegistryDword( NetacOD_REGISTRY_PARAMETERS_PATH, //absolute registry path
    //                           L"DebugLevel",     // REG_DWORD ValueName
    //                           &gDebugLevel );    // Value receiver

    // debug_level = DBGLVL_MAXIMUM;
#endif

    uhci_dbg_print_cond(DBGLVL_MINIMUM, DEBUG_UHCI,
                        ("Entering DriverEntry(), RegistryPath=\n    %ws\n", RegistryPath->Buffer));

    // Remember our driver object, for when we create our child PDO
    usb_driver_obj = DriverObject;

    //
    // Create dispatch points for create, close, unload
    DriverObject->MajorFunction[IRP_MJ_CREATE] = generic_dispatch_irp;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = generic_dispatch_irp;
    DriverObject->DriverUnload = uhci_unload;

    // User mode DeviceIoControl() calls will be routed here
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = generic_dispatch_irp;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = generic_dispatch_irp;

    // User mode ReadFile()/WriteFile() calls will be routed here
    DriverObject->MajorFunction[IRP_MJ_WRITE] = generic_dispatch_irp;
    DriverObject->MajorFunction[IRP_MJ_READ] = generic_dispatch_irp;

    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = generic_dispatch_irp;
    DriverObject->MajorFunction[IRP_MJ_SCSI] = generic_dispatch_irp;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = generic_dispatch_irp;

    DriverObject->DriverStartIo = generic_start_io;
    // routines for handling system PNP and power management requests
    //DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = generic_dispatch_irp;

    // The Functional Device Object (FDO) will not be created for PNP devices until
    // this routine is called upon device plug-in.
    RtlZeroMemory(&g_dev_mgr, sizeof(USB_DEV_MANAGER));
    g_dev_mgr.usb_driver_obj = DriverObject;

#ifdef INCLUDE_EHCI
    ehci_probe(DriverObject, RegistryPath, &g_dev_mgr);
#endif

    uhci_probe(DriverObject, RegistryPath, &g_dev_mgr);

    if (dev_mgr_strobe(&g_dev_mgr) == FALSE)
    {

        dev_mgr_release_hcd(&g_dev_mgr);
        return STATUS_UNSUCCESSFUL;
    }

    dev_mgr_start_hcd(&g_dev_mgr);

    uhci_dbg_print_cond(DBGLVL_DEFAULT, DEBUG_UHCI, ("DriverEntry(): exiting... (%x)\n", ntStatus));
    return STATUS_SUCCESS;
}

//note: the initialization will be in the following order
//      uhci_probe
//      dev_mgr_strobe
//      uhci_start

//      to  kill dev_mgr_thread:
//      dev_mgr->term_flag = TRUE;
//      KeSetEvent( &dev_mgr->wake_up_event );
//      this piece of code must run at passive-level
